using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;

using Microsoft.Win32.SafeHandles;

namespace DevSupport.DeviceManager
{
    public sealed class UsbControllerClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the UsbControllerClass class.
        /// </summary>
        private UsbControllerClass()
            : base(Win32.GUID_DEVINTERFACE_USB_HOST_CONTROLLER, Win32.GUID_DEVCLASS_USB, "PCI")
        { }

        /// <summary>
        /// Gets the single UsbControllerClass instance.
        /// </summary>
        public static UsbControllerClass Instance
        {
            get { return Utils.Singleton<UsbControllerClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new UsbController(deviceInstance, path);
        }

        /// <summary>
        /// Gets a set of Device Instances described by the Device Class that can be used to
        /// create instances of the Devices. Override allows extension for DeviceClass-derived types.
        /// In the USB Controller case, in Win2K, we add controllers named \\.\HCD0.
        /// </summary>
        /// <returns>Dictionary( DevInst, Path )</returns>
        protected override  Dictionary<IntPtr, String> GetDevInstDataSet()
        {
            // Call the base class to get the Device Instances using the 
            // GUID_DEVINTERFACE_USB_HOST_CONTROLLER method.
            Dictionary<IntPtr, String> devInstDataSet = base.GetDevInstDataSet();

            // Add something else for Win2K 
            if (Environment.OSVersion.Version < new Version(5, 1))
            {
                // Iterate over some Host Controller names and try to open them.
                for (int hcNum = 0; hcNum < 10; ++hcNum)
                {
                    String controllerName = String.Format(@"\\.\HCD{0}", hcNum);

                    Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
                    secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES)); 
                    SafeFileHandle hHCDev = Win32.CreateFile(
                        controllerName,
                        Win32.GENERIC_WRITE,
                        Win32.FILE_SHARE_WRITE,
                        ref secAttribs,
                        Win32.OPEN_EXISTING,
                        0,
                        IntPtr.Zero);

                    // If the handle is valid, then we've successfully opened a Host Controller.
                    if ( !hHCDev.IsInvalid )
                    {
                        String controllerDriver = String.Empty;
                        IntPtr devInst;

                        try
                        {
                            // Obtain the driver key name for this host controller.
                            controllerDriver = GetDeviceDriverKeyName(hHCDev);
                        }
                        catch (Win32Exception e)
                        {
                            // Don't think we need to abort if we don't get the Controller's driver key.
                            // Just make sure it is closed and try the next one.
                            Trace.WriteLine(e.Message);
                        }
                        finally
                        {
                            hHCDev.Close();
                        }

                        if (!String.IsNullOrEmpty(controllerDriver))
                        {
                            // Got the driver key, now get the corresponding DevInst.
                            devInst = DeviceManager.FindDeviceInstance(controllerDriver);

                            if (devInst != IntPtr.Zero)
                            {
                                // Add the Controller's DevInst to the set if it is not already there.
                                if (devInstDataSet.ContainsKey(devInst) == false)
                                    devInstDataSet[devInst] = controllerName;
                            }
                        }
                    
                    } // if ( opened controller )
                
                } // for ( 10 controller names )
            
            } // if ( Win2K )

            return devInstDataSet;
        }

        /// <summary>
        /// Gets the Driver key for a USB Host Controller by sending a DeviceIoControl to the
        /// opened device. Worker function for GetDevInstDataSet().
        /// </summary>
        /// <param name="hController"> Handle to a USB controller.</param>
        /// <returns>Driver key name for this host controller.</returns>
        /// <exception cref="System.ComponentModel.Win32Exception">DeviceIoControl() failure.</exception>
        private String GetDeviceDriverKeyName(SafeFileHandle hController)
        {
            String driver = String.Empty;

            int nBytesReturned;
            Win32.USB_HCD_DRIVERKEY_NAME controllerDriver = new Win32.USB_HCD_DRIVERKEY_NAME();
            int nBytes = Marshal.SizeOf(controllerDriver);
            IntPtr ptrControllerDriver = Marshal.AllocHGlobal(nBytes);

            // get the Driver Key
            if (Win32.DeviceIoControl(hController, Win32.IOCTL_GET_HCD_DRIVERKEY_NAME, ptrControllerDriver,
                nBytes, ptrControllerDriver, nBytes, out nBytesReturned, IntPtr.Zero))
            {
                // SUCCESS
                controllerDriver = (Win32.USB_HCD_DRIVERKEY_NAME)Marshal.PtrToStructure(ptrControllerDriver, typeof(Win32.USB_HCD_DRIVERKEY_NAME));
                driver = controllerDriver.DriverKeyName;

                Marshal.FreeHGlobal(ptrControllerDriver);
            }
            else
            {
                // FAILED
                int error = Marshal.GetLastWin32Error();
                Marshal.FreeHGlobal(ptrControllerDriver);

                throw new Win32Exception(error);
            }

            return driver;
        }
    }
}