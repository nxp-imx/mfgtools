/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

using PortableDeviceApiLib;
using PortableDeviceTypesLib;
using DevSupport.WPD;

namespace DevSupport.DeviceManager
{
    public sealed class WpdDeviceClass : DeviceClass
    {
        public bool IsSupported;
        /// <summary>
        /// Initializes a new instance of the WpdDeviceClass class.
        /// </summary>
        private WpdDeviceClass()
            : base(PortableDeviceGuids.GUID_DEVINTERFACE_WPD, Win32.GUID_DEVCLASS_WPD, null)
        {
            try
            {
                PortableDeviceApiLib.IPortableDeviceValues _CommandValues = (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceValuesClass();
                IsSupported = true;
            }
            catch (COMException ce)
            {
                IsSupported = false;
                String msg = ce.Message;
                Trace.WriteLine(String.Format("WpdDeviceClass() - Windows Media Format 11 Runtime is not installed. {0}", msg));
            }
        }

        /// <summary>
        /// Gets the single WpdDeviceClass instance.
        /// </summary>
        public static WpdDeviceClass Instance
        {
            get { return Utils.Singleton<WpdDeviceClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new WpdDevice(deviceInstance, path);

            // add it to our list of devices if there are no filters
            /*	        if ( _filters.empty() )
                        {
                            dev = new WpdDevice(deviceClass, deviceInfoData.DevInst, path);
                            Sleep(1000);
                            return dev;
                        }
                        else
                        {
                            // if there are filters, don't add it unless it matches
                            for (size_t idx=0; idx<_filters.size(); ++idx)
                            {
                                if ( path.IsEmpty() )
                                {
                                    dev = new WpdDevice(deviceClass, deviceInfoData.DevInst, path);
                                    if ( dev->_path.get().ToUpper().Find(_filters[idx].ToUpper()) != -1 )
                                    {
                                        Sleep(1000);
                                        return dev;
                                    }
                                    else
                                        delete dev;
                                }
                                else if ( path.ToUpper().Find(_filters[idx].ToUpper()) != -1 )
                                {
                                    dev = new WpdDevice(deviceClass, deviceInfoData.DevInst, path);
                                    Sleep(1000);
                                    return dev;
                                }
                            }
                        }
        	
                        return NULL;
            */
        }

        // Overridden to remove the GUID_DEVINTERFACE_USB_DEVICE from devPath before doing the compare.
        // The DeviceArrival/DeviceRemoval messages from the system use the GUID_DEVINTERFACE_USB_DEVICE
        // while the device Path uses the GUID_DEVINTERFACE_WPD.
        public override Device FindDeviceByUsbPath(String devPath, bool refresh)
        {

            if (String.IsNullOrEmpty(devPath))
            {
                return null;
            }

            // Refresh the Device list?
            if (refresh)
            {
                _Devices.Clear();
                _Devices = null;
            }

            // Find the Device in our list of devices.
            foreach (Device dev in Devices)
            {
                if (dev.UsbDevice != null)
                {
                    // Remove the GUID_DEVINTERFACE_USB_DEVICE from devPath before doing the compare.
                    String usbGuid = Win32.GUID_DEVINTERFACE_USB_DEVICE.ToString("B").ToUpper();
                    devPath = devPath.ToUpper().Replace(usbGuid, "");
                    if (String.Compare(dev.UsbDevice.Path, 4, devPath, 4, devPath.Length - 4, true) == 0)
                    {
                        return dev;
                    }
                }
            }

            return null;
        }

    }
}
