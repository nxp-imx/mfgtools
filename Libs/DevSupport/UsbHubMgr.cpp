#include "UsbHubMgr.h"
#include "UsbHub.h"

/// <summary>
/// Initializes a new instance of the usb::HubMgr class.
/// </summary>
usb::HubMgr::HubMgr()
: DeviceClass(NULL/*&GUID_DEVINTERFACE_USB_HUB*/, &GUID_DEVCLASS_USB, _T("USB"), DeviceTypeUsbHub)
{
}

usb::HubMgr::~HubMgr(void)
{
}

Device* usb::HubMgr::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	usb::Hub * hub = new usb::Hub(deviceClass, deviceInfoData.DevInst, path);
	
	// Enumerator finds all USB devices, so we don't create the device if it doesn't have any ports.
	if ( hub->GetNumPorts() == 0 )
	{
		delete hub;
		hub = NULL;
	}
	else
	{
		hub->_index.put((int32_t)_devices.size()+1);
	}

	return hub;
}

void usb::HubMgr::RefreshHubs()
{
	// Init the list of hubs
	DeviceClass::Devices();

	std::list<Device*>::iterator deviceItem;

	for ( deviceItem = _devices.begin(); deviceItem != _devices.end(); ++deviceItem )
	{
		usb::Hub* pHub = dynamic_cast<usb::Hub*>(*deviceItem);
		pHub->RefreshPort(0); // 0 means refresh all the ports
	}
}

usb::Hub* usb::HubMgr::FindHubByDriver(LPCTSTR driverName)
{
	CStdString driverNameStr = driverName;
	usb::Hub* pHub = NULL;

	if ( driverNameStr.empty() )
	{
		return NULL;
	}

	if ( _devices.empty() )
	{
		DeviceClass::Devices();
	}

	// Find the Hub in our list of hubs
	std::list<Device*>::iterator hub;
	for ( hub = _devices.begin(); hub != _devices.end(); ++hub )
	{
		if ( driverNameStr.CompareNoCase( (*hub)->_driver.get() ) == 0 )
		{
			pHub = dynamic_cast<usb::Hub*>(*hub);
			break;
		}
	}

	return pHub;
}

usb::Hub* usb::HubMgr::FindHubByPath(LPCTSTR pathName)
{
	CStdString pathNameStr = pathName;
	usb::Hub* pHub = NULL;

	if ( pathNameStr.empty() )
	{
		return NULL;
	}

	if ( _devices.empty() )
	{
		DeviceClass::Devices();
	}

	// Find the Hub in our list of hubs
	std::list<Device*>::iterator hub;
	for ( hub = _devices.begin(); hub != _devices.end(); ++hub )
	{
		if ( pathNameStr.CompareNoCase( (*hub)->_path.get() ) == 0 )
		{
			pHub = dynamic_cast<usb::Hub*>(*hub);
			break;
		}
	}

	return pHub;
}
