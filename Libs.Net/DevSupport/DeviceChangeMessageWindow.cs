/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// Type of device change event.
    /// </summary>
    public enum DeviceChangeEvent
    {
        Unknown,
        DeviceArrival,
        DeviceRemoval,
        VolumeArrival,
        VolumeRemoval,
        HubArrival,
        HubRemoval
    }

    /// <summary>
    /// Monitors WM_DEVICECHANGE native Windows messages and asynchronously invokes DeviceChangedMsg event
    /// for the following event types: USB Device Arrival/Removal, Volume Arrival/Removal, and USB Hub Arrival/Removal.
    /// </summary>
    public class DeviceChangeWindow : NativeWindow, IDisposable
    {
        internal delegate void DeviceChangedMsgHandler(DeviceChangeEvent devEvent, String devDetails);
        internal event DeviceChangedMsgHandler DeviceChangedMsg;

        private IntPtr _UsbDevNotifyHandle;
        private IntPtr _UsbHubNotifyHandle;
        
        public DeviceChangeWindow()
        {
            /// Make WndProc() start getting messages.
            CreateParams createParams = new CreateParams();
            createParams.ClassName = "STATIC";
            CreateHandle(createParams);
        
            /// Register for usb devices notifications.
            Win32.DEV_BROADCAST_DEVICEINTERFACE_NONAME broadcastInterface = new Win32.DEV_BROADCAST_DEVICEINTERFACE_NONAME();
            broadcastInterface.dbcc_devicetype = Win32.DBT_DEVTYP_DEVICEINTERFACE;
            broadcastInterface.dbcc_classguid = Win32.GUID_DEVINTERFACE_USB_DEVICE;
            IntPtr buffer = Marshal.AllocHGlobal(new IntPtr(broadcastInterface.dbcc_size));
            Marshal.StructureToPtr(broadcastInterface, buffer, false);
            _UsbDevNotifyHandle = Win32.RegisterDeviceNotification(Handle, buffer, Win32.DEVICE_NOTIFY_WINDOW_HANDLE);

            /// Register for usb hub notifications.
            broadcastInterface.dbcc_classguid = Win32.GUID_DEVINTERFACE_USB_HUB;
            Marshal.StructureToPtr(broadcastInterface, buffer, true);
            _UsbHubNotifyHandle = Win32.RegisterDeviceNotification(Handle, buffer, Win32.DEVICE_NOTIFY_WINDOW_HANDLE);

            Marshal.FreeHGlobal(buffer);
        }

        #region IDisposable Implemetation

        /// <summary>
        /// Used to determine if Dispose() has already been called.
        /// </summary>
        private bool _Disposed = false;

        /// <summary>
        /// Finalizer calls Dispose(false) worker function.
        /// </summary>
        ~DeviceChangeWindow()
        {
            // Call our helper method. 
            // Specifying "false" signifies that the GC triggered the clean up.
            Dispose(false);
        }

        /// <summary>
        /// IDisposable Implemetation.
        /// </summary>
        public void Dispose()
        {
            // Call our helper method. 
            // Specifying "true" signifies that the object user triggered the clean up.
            Dispose(true);

            // Now suppress finalization.
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Dispose Pattern worker function.
        /// </summary>
        /// <param name="disposing">false - called from Finalizer, true - called from Dispose()</param>
        private void Dispose(bool disposing)
        {
            // Be sure we have not already been disposed!
            if (!_Disposed)
            {
                // If disposing equal true, dispose all managed resources.
                if (disposing)
                {
                    // Dispose managed resources.
                }
                
                // Clean up unmanaged resources here.
                
                if (_UsbDevNotifyHandle != IntPtr.Zero)
                {
                    bool ret = Win32.UnregisterDeviceNotification(_UsbDevNotifyHandle);
                    _UsbDevNotifyHandle = IntPtr.Zero;
                }

                if (_UsbHubNotifyHandle != IntPtr.Zero)
                {
                    bool ret = Win32.UnregisterDeviceNotification(_UsbHubNotifyHandle);
                    _UsbHubNotifyHandle = IntPtr.Zero;
                }

                // A window is not eligible for garbage collection when it is associated with
                // a window handle. To ensure proper garbage collection, handles must either be
                // destroyed manually using DestroyHandle or released using ReleaseHandle.
                DestroyHandle();
            }
            _Disposed = true;
        }

        /// <summary>
        /// 
        /// </summary>

        #endregion

        /// <summary>
        /// Monitor WM_DEVICECHANGE messages.
        /// </summary>
        /// <param name="msg">Native Windows message.</param>
        protected override void WndProc(ref Message msg)
        {
            if (msg.Msg == Win32.WM_DEVICECHANGE)
            {
                OnDeviceChange(msg);
            }
            base.WndProc(ref msg);
        }

        /// <summary>
        /// Worker function for WM_DEVICECHANGE messages. Invokes DeviceChangedMsg event.
        /// </summary>
        /// <param name="msg">Native Windows message - WM_DEVICECHANGE</param>
        private void OnDeviceChange(Message msg)
        {
            DeviceChangeEvent devEvent = DeviceChangeEvent.Unknown;
            String devDetails = String.Empty;

            if (msg.LParam != IntPtr.Zero)
            {
                Win32.DEV_BROADCAST_HDR db = (Win32.DEV_BROADCAST_HDR)Marshal.PtrToStructure(msg.LParam, typeof(Win32.DEV_BROADCAST_HDR));

                switch (msg.WParam.ToInt32())
                {
                    case Win32.DBT_DEVICEARRIVAL:
                        if (db.dbch_devicetype == Win32.DBT_DEVTYP_DEVICEINTERFACE)
                        {
                            Win32.DEV_BROADCAST_DEVICEINTERFACE dbdi = (Win32.DEV_BROADCAST_DEVICEINTERFACE)Marshal.PtrToStructure(msg.LParam, typeof(Win32.DEV_BROADCAST_DEVICEINTERFACE));
                            if (dbdi.dbcc_classguid == Win32.GUID_DEVINTERFACE_USB_DEVICE)
                            {
                                devEvent = DeviceChangeEvent.DeviceArrival;
                                devDetails = dbdi.dbcc_name;
                            }
                            else if (dbdi.dbcc_classguid == Win32.GUID_DEVINTERFACE_USB_HUB)
                            {
                                devEvent = DeviceChangeEvent.HubArrival;
                                devDetails = dbdi.dbcc_name;
                            }
                        }
                        else if (db.dbch_devicetype == Win32.DBT_DEVTYP_VOLUME)
                        {
                            Win32.DEV_BROADCAST_VOLUME dbv = (Win32.DEV_BROADCAST_VOLUME)Marshal.PtrToStructure(msg.LParam, typeof(Win32.DEV_BROADCAST_VOLUME));
                            devEvent = DeviceChangeEvent.VolumeArrival;
                            devDetails = DrivesFromMask(dbv.dbcv_unitmask);
                        }
                        break;
                    case Win32.DBT_DEVICEREMOVECOMPLETE:
                        if (db.dbch_devicetype == Win32.DBT_DEVTYP_DEVICEINTERFACE)
                        {
                            Win32.DEV_BROADCAST_DEVICEINTERFACE dbdi = (Win32.DEV_BROADCAST_DEVICEINTERFACE)Marshal.PtrToStructure(msg.LParam, typeof(Win32.DEV_BROADCAST_DEVICEINTERFACE));

                            if (dbdi.dbcc_classguid == Win32.GUID_DEVINTERFACE_USB_DEVICE)
                            {
                                devEvent = DeviceChangeEvent.DeviceRemoval;
                                devDetails = dbdi.dbcc_name;
                            }
                            else if (dbdi.dbcc_classguid == Win32.GUID_DEVINTERFACE_USB_HUB)
                            {
                                devEvent = DeviceChangeEvent.HubRemoval;
                                devDetails = dbdi.dbcc_name;
                            }
                        }
                        else if (db.dbch_devicetype == Win32.DBT_DEVTYP_VOLUME)
                        {
                            Win32.DEV_BROADCAST_VOLUME dbv = (Win32.DEV_BROADCAST_VOLUME)Marshal.PtrToStructure(msg.LParam, typeof(Win32.DEV_BROADCAST_VOLUME));
                            devEvent = DeviceChangeEvent.VolumeRemoval;
                            devDetails = DrivesFromMask(dbv.dbcv_unitmask);
                        }
                        break;
                    default:
                        Trace.Assert(false, "Invalid msg.WParam.");
                        break;
                } // end switch (nEventType)

                Trace.WriteLine(String.Format("*** DeviceChangeWindow.OnDeviceChange(), {0}, {1}, {2}({3})", devEvent, devDetails, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

                // let's figure out what to do with the WM_DEVICECHANGE message
                // after we get out of this loop so we don't miss any messages.
                if (DeviceChangedMsg != null)
                {
                    DeviceChangedMsg.BeginInvoke(devEvent, devDetails, null, null);
                }

            } // end if (lpdb)

            msg.Result = new IntPtr(1); // true
            return;
        }

        /// <summary>
        /// Worker function for OnDeviceChange() to get drive letters from the bitmask.
        /// </summary>
        /// <param name="UnitMask">Logical unit mask identifying one or more logical units. Each bit in the mask corresponds to one logical drive. Bit 0 represents drive A, bit 1 represents drive B, and so on.</param>
        /// <returns>String representing the drives identified in the UnitMask parameter in the form of "F" or "FG" for multiple drives.</returns>
        private static String DrivesFromMask(uint UnitMask)
        {
            String DriveStr = "";
            Char DriveCh;

            for (DriveCh = 'A'; DriveCh <= 'Z'; ++DriveCh, UnitMask >>= 1)
            {
                if ((UnitMask & 1) == 1)
                    DriveStr += DriveCh;
            }
            return DriveStr;
        }
    
    } // class DeviceChangeWindow

} // namespace DevSupport.DeviceManager
