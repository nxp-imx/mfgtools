#include "stdafx.h"
#include "UsbHub.h"

usb::Hub::Hub(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
, _ports(true)
{
	_ports.describe(this, _T("USB Ports"), _T(""));
	_index.describe(this, _T("Index"), _T(""));

	memset(&_nodeInformation, 0, sizeof(_nodeInformation));
	_nodeInformation.NodeType = UsbHub;

	CreatePorts();
}

usb::Hub::~Hub(void)
{
	while ( !_ports.getList()->empty() )
	{
		Property * port = _ports.getList()->back();
		_ports.getList()->pop_back();
		delete port;
	}
}

/// <summary>
/// Property: Set a index associated with the hub in the system for easy reference.
/// </summary>
void usb::Hub::index::put(int32_t val)
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	Value = val;

	dev->name().Format(_T("%s (Hub %d)"), dev->_description.get().c_str(), val);
}

// Provides access to the individual Ports.
// portNumbers start at 1 not 0
usb::Port* usb::Hub::Port(const size_t portNumber)
{
	assert ( portNumber <= _ports.getList()->size() );

	usb::Port* pPort = dynamic_cast<usb::Port*>(_ports.getList()->at(portNumber-1));
	
	return pPort;

}

HANDLE usb::Hub::Open()
{
	SECURITY_ATTRIBUTES SecurityAttrib; // Needed for Win2000
	SecurityAttrib.bInheritHandle = false;
	SecurityAttrib.lpSecurityDescriptor = NULL;
	SecurityAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);

	CStdString filePath = _path.get();
	filePath.Replace(_T("\\??"), _T("\\\\."));

	return CreateFile(
		filePath,
		/*GENERIC_WRITE*/0,
		/*FILE_SHARE_READ|FILE_SHARE_WRITE*/0,
		&SecurityAttrib, 
		OPEN_EXISTING, 0, NULL);
}

int32_t usb::Hub::CreatePorts()
{
	DWORD error = ERROR_SUCCESS;

	HANDLE hHub = Open();
	if ( hHub == INVALID_HANDLE_VALUE )
	{
		// Probably not a port. We had to look at all USB devices in order to get all hubs. If it
		// fails to open or doesn't have any ports, we don't create a usb::Hub device.
		error=GetLastError();
//		ATLTRACE(_T("*** ERROR 0x%X (%d): Line %d of file %s\n"), error, error, __LINE__, __TFILE__);
		return 0;
	}

	// See how many Ports are on the Hub
	DWORD BytesReturned;
	BOOL Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_INFORMATION, &_nodeInformation, sizeof(_nodeInformation),
										&_nodeInformation, sizeof(_nodeInformation),&BytesReturned, NULL);

	CloseHandle(hHub);

	if (!Success) 
	{
		error = GetLastError();
		return 0;
	}

	// Loop through the Ports on the Hub any get the unique DriverKey if there is a device connected to
	// the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our Hub Index.
	//
	uint8_t index;
	// Port index is 1-based
	for	( index=1; index<=GetNumPorts(); ++index ) 
	{
		Property* pPort = new usb::Port(this, index);
		_ports.getList()->push_back(pPort);
	} // end for(ports)

	return GetNumPorts();
}

int32_t usb::Hub::RefreshPort(const int32_t portNumber)
{
	usb::Port* pPort = NULL;
	int32_t rc = ERROR_SUCCESS;

	if ( portNumber == 0 )
	{
		property::Property::PropertyIterator port;
		for (port = _ports.getList()->begin(); port != _ports.getList()->end(); ++port)
		{
			pPort = dynamic_cast<usb::Port*>(*port);
			rc = pPort->Refresh();
			if (rc != ERROR_SUCCESS)
				break;
		} // end for(ports)
	}
	else
	{
		pPort = dynamic_cast<usb::Port*>(_ports.getList()->at(portNumber-1));
		rc = pPort->Refresh();
	}
	return rc;
}

int32_t usb::Hub::ClearPort(const int32_t portNumber)
{
	usb::Port* pPort = NULL;
	int32_t rc = ERROR_FILE_NOT_FOUND;

	if ( portNumber == 0 )
	{
		property::Property::PropertyIterator port;
		for (port = _ports.getList()->begin(); port != _ports.getList()->end(); ++port)
		{
			pPort = dynamic_cast<usb::Port*>(*port);
			pPort->Clear();
			rc = ERROR_SUCCESS;
		} // end for(ports)
	}
	else
	{
		pPort = dynamic_cast<usb::Port*>(_ports.getList()->at(portNumber-1));
		pPort->Clear();
		rc = ERROR_SUCCESS;
	}
	return rc;
}
