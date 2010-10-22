/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
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


