using System;

namespace DevSupport.DeviceManager
{
    public sealed class WinUsbDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the WinUsbDeviceClass class.
        /// </summary>
        private WinUsbDeviceClass()
            : base(Win32.GUID_DEVINTERFACE_WINUSB_BULK_DEVICE, Guid.Empty, null)
        { }

        /// <summary>
        /// Gets the single WinUsbDeviceClass instance.
        /// </summary>
        public static WinUsbDeviceClass Instance
        {
            get { return Utils.Singleton<WinUsbDeviceClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new WinUsbDevice(deviceInstance, path);
        }
    }
}


