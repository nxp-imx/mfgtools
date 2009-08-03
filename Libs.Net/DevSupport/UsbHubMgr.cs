using System;

namespace DevSupport.DeviceManager
{
    public sealed class UsbDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the UsbDeviceClass class.
        /// </summary>
        private UsbDeviceClass()
            : base(Guid.Empty /*Win32.GUID_DEVINTERFACE_USB_DEVICE*/, Win32.GUID_DEVCLASS_USB, "USB")
        {}

        /// <summary>
        /// Gets the single UsbDeviceClass instance.
        /// </summary>
        public static UsbDeviceClass Instance
        {
            get { return Utils.Singleton<UsbDeviceClass>.Instance; }
        }
    }

    public sealed class UsbHubClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the UsbHubClass class.
        /// </summary>
        private UsbHubClass()
            : base(/*Guid.Empty */Win32.GUID_DEVINTERFACE_USB_HUB, Win32.GUID_DEVCLASS_USB, "USB")
        {}

        /// <summary>
        /// Gets the single UsbHubClass instance.
        /// </summary>
        public static UsbHubClass Instance
        {
            get { return Utils.Singleton<UsbHubClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            UsbHub hub = new UsbHub(deviceInstance, path, this.Count + 1);
	        
            // Enumerator finds all USB devices, so we don't create the device if it doesn't have any ports.
            if (hub.NumberOfPorts != 0)
            {
                return hub;
            }
            else
            {
                hub.Dispose();
                return null;
            }
        }
 
    }
}
