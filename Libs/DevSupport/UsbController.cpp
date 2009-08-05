#include "stdafx.h"
#include "UsbController.h"
#include "DeviceManager.h"

#pragma warning( disable : 4200 )
#include <usbioctl.h>
#pragma warning( default : 4200 )

usb::Controller::Controller(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
{
	int32_t error = Initialize();
}

usb::Controller::~Controller(void)
{
}

HANDLE usb::Controller::Open()
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

int32_t usb::Controller::Initialize()
{
	int32_t errorCode = ERROR_SUCCESS;

	// reset the member variable
	_rootHubFilename.put(_T(""));
	
	HANDLE hController = Open();
	if ( hController == INVALID_HANDLE_VALUE )
	{
		errorCode=GetLastError();
		ATLTRACE(_T("*** ERROR 0x%X (%d): Line %d of file %s\n"), errorCode, errorCode, __LINE__, __TFILE__);
		throw;
		return errorCode;
	}

	BOOL success;
	DWORD bytesReturned;
	CStdString rootHubFilename = _T("\\\\.\\");
	struct {uint32_t Length; wchar_t Name[MAX_PATH];} unicodeName;

	// Get the system name of our root hub for interrogation
	success	= DeviceIoControl(hController, IOCTL_USB_GET_ROOT_HUB_NAME, &unicodeName,
							  sizeof(unicodeName),&unicodeName, sizeof(unicodeName), &bytesReturned, NULL); 

	CloseHandle(hController);

	if (!success) 
	{
		errorCode=GetLastError();
		ATLTRACE(_T("*** ERROR 0x%X (%d): Line %d of file %s\n"), errorCode, errorCode, __LINE__, __TFILE__);
		throw;
		return errorCode;
	}

	rootHubFilename.append(&unicodeName.Name[0]);

	// save the Root Hub Filename to our member variable
	_rootHubFilename.put(rootHubFilename);
	_rootHubFilename.describe(this, _T("Root Hub Filename"), _T("Filename used to talk to the Controller's root hub."));

	return ERROR_SUCCESS;

}

usb::Hub * usb::Controller::GetRootHub()
{
	usb::Hub* pHub = NULL;

	CStdString pathToFind = (LPCTSTR)_rootHubFilename.get() + 4;

	// Find our Hub in gDeviceManager's list of [Hub].Devices()
	std::list<Device*> HubList = gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbHub]->Devices();
	std::list<Device*>::iterator hub;
	for ( hub = HubList.begin(); hub != HubList.end(); ++hub )
	{
		if ( pathToFind.CompareNoCase( (*hub)->_usbPath.get() ) == 0 )
		{
//t			ATLTRACE2(_T("DeviceClass::AddUsbDevice()  Found existing(%d): %s\r\n"), _devices.size(), pDevice->_usbPath.get().c_str());
			pHub = dynamic_cast<usb::Hub*>(*hub);
			break;
		}
	}

	return pHub;
}
