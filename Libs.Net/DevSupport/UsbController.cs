/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;

using Microsoft.Win32.SafeHandles;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A USB Controller device.
    /// </summary>
    public sealed class UsbController : Device
    {
        /// <summary>
        /// Creates an instance of a UsbController. Do not call this constructor
        /// directly. Instead, use the UsbController.Instance to call the
        /// CreateDevice method.
        /// </summary>
        internal UsbController(IntPtr deviceInstance, string path)
            : base(deviceInstance, path)
        {
        }

        /// <summary>
        /// The child device of a USB Host Controller is a USB Root UsbHub.
        /// </summary>
        public override Device Child
        {
            get
            {
                IntPtr childDevInst = IntPtr.Zero;
                if (Win32.CM_Get_Child(out childDevInst, DeviceInstance, 0) == Win32.CONFIGRET.CR_SUCCESS)
                {
                    foreach (UsbHub hub in UsbHubClass.Instance)
                    {
                        if (hub.DeviceInstance == childDevInst)
                            return hub;
                    }
                }

                return null;
            }
        }

        /// <summary>
        /// The USB Root UsbHub of the USB Host Controller.
        /// </summary>
        public UsbHub RootHub
        {
            get
            {
                if (_RootHub == null)
                {
                    _RootHub = Child as UsbHub;
                }
                
                return _RootHub;
            }
        }
        private UsbHub _RootHub;

        /// <summary>
        /// Get the USB Root Hub device name by querying the device. Don't really need to use this
        /// property since we know the RootHub is the Child device of the Controller. We can get the
        /// same info through the registry without having to open/close the hardware.
        /// </summary>
        public String RootHubFileName
	    {
		    get 
            {
                if (String.IsNullOrEmpty(_RootHubFileName))
                {
                    _RootHubFileName = this.GetRootHubFileName();
                }
                
                return _RootHubFileName;
            }
	    }
        private String _RootHubFileName;

        /// <summary>
        /// Worker function for the RootHubFileName filename property.
        /// </summary>
        private String GetRootHubFileName()
        {
	        String fileName = String.Empty;
        	
	        SafeFileHandle hController = Open();
	        if ( hController.IsInvalid )
	        {
                throw new Win32Exception(Marshal.GetLastWin32Error());
	        }
            
            int nBytesReturned;
            Win32.USB_ROOT_HUB_NAME hubName = new Win32.USB_ROOT_HUB_NAME();
            int nBytes = Marshal.SizeOf(hubName);
            IntPtr ptrHubName = Marshal.AllocHGlobal(nBytes);

            // get the UsbHub Name
            if ( Win32.DeviceIoControl(hController, Win32.IOCTL_USB_GET_ROOT_HUB_NAME, ptrHubName,
                nBytes, ptrHubName, nBytes, out nBytesReturned, IntPtr.Zero) )
            {
                hubName = (Win32.USB_ROOT_HUB_NAME)Marshal.PtrToStructure(ptrHubName, typeof(Win32.USB_ROOT_HUB_NAME));
                fileName = @"\\.\" + hubName.RootHubName;

                hController.Close();
                Marshal.FreeHGlobal(ptrHubName);
            }
            else
            {
                hController.Close();
                Marshal.FreeHGlobal(ptrHubName);

                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            return fileName;
        }

        /// <summary>
        /// Opens the USB Controller device. Returns a handle to use with DeviceIoControl() commands.
        /// </summary>
        private SafeFileHandle Open()
        {
            // Needed for Win2000
            Win32.SECURITY_ATTRIBUTES securityAttrib = new Win32.SECURITY_ATTRIBUTES();
            securityAttrib.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
//            securityAttrib.bInheritHandle = false;
//            securityAttrib.lpSecurityDescriptor = new IntPtr();
//            securityAttrib.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));// sizeof(Win32.SECURITY_ATTRIBUTES);
//            IntPtr ptrSecurityAttrib = Marshal.AllocHGlobal(securityAttrib.nLength);
//            Marshal.StructureToPtr(securityAttrib, ptrSecurityAttrib, true);

            return Win32.CreateFile(
                Path.Replace(@"\??", @"\\."),
                Win32.GENERIC_WRITE,
                Win32.FILE_SHARE_WRITE,
                ref securityAttrib,
                Win32.OPEN_EXISTING, 0, IntPtr.Zero);
        }

    } // class UsbController

} // namespace DevSupport.DeviceManager
