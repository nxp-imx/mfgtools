using System;

namespace DevSupport.DeviceManager
{
    public sealed class MtpDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the MtpDeviceClass class.
        /// </summary>
        private MtpDeviceClass()
            : base(Guid.Empty, Win32.GUID_DEVCLASS_MTP, null)
        {
        }

        /// <summary>
        /// Gets the single MtpDeviceClass instance.
        /// </summary>
        public static MtpDeviceClass Instance
        {
            get { return Utils.Singleton<MtpDeviceClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new WpdDevice(deviceInstance, path);
        }
    }
}
