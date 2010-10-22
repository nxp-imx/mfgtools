/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// UsbEject version 1.0 March 2006
// written by Simon Mourier <email: simon [underscore] mourier [at] hotmail [dot] com>

using System;
//using System.Collections.Generic;
//using System.Collections.ObjectModel;
//using System.Text;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// The device class for disk devices.
    /// </summary>
    public class DiskDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the DiskDeviceClass class.
        /// </summary>
        private DiskDeviceClass()
            :base(Win32.GUID_DEVINTERFACE_DISK, Win32.GUID_DEVCLASS_DISKDRIVE, String.Empty)
        {
        }

        /// <summary>
        /// Gets the single DiskDeviceClass instance.
        /// </summary>
        public static DiskDeviceClass Instance
        {
            get { return Utils.Singleton<DiskDeviceClass>.Instance; }
        }


    }
}
