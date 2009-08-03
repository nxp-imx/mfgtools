using System;

namespace DevSupport.DeviceManager
{
    public sealed class UsbDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the UsbDeviceClass class.
        /// </summary>
        public UsbDeviceClass(UsbDeviceClass.Scope scope)
            : base(Guid.Empty /*Win32.GUID_DEVINTERFACE_USB_DEVICE*/, Win32.GUID_DEVCLASS_USB, "USB")
        {
            _Scope = scope;
        }

        internal override Device CreateDevice(Win32.SP_DEVINFO_DATA deviceInfoData, String path)
        {
            return new UsbDevice(deviceInfoData.devInst, path);
        }

        public UsbDevice FindDeviceByPath(String devicePath)
        {

            if (String.IsNullOrEmpty(devicePath))
	        {
		        return null;
	        }

	        // Find the Hub in our list of hubs
	        foreach ( Device dev in Devices )
	        {
                if (dev != null)
                {
                    if (String.Compare(dev.Path, devicePath, true) == 0)
                    {
                        return dev;
                    }
                }
	        }

	        return null;
        }

        public UsbHub FindHubByDriver(String driverName)
        {
            if (String.IsNullOrEmpty(driverName))
	        {
		        return null;
	        }

	        // Find the Hub in our list of hubs
	        foreach ( UsbHub hub in Devices )
	        {
                if (hub != null)
                {
                    if (String.Compare(hub.Driver, driverName, true) == 0)
                    {
                        return hub;
                    }
                }
	        }

	        return null;
        }
    }
}
/*
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

*/