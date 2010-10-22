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
    public sealed class RecoveryDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the RecoveryDeviceClass class.
        /// </summary>
        private RecoveryDeviceClass()
            : base(Guid.Empty, Win32.GUID_DEVCLASS_STMP3XXX_USB_BULK_DEVICE, null)
        { }

        /// <summary>
        /// Gets the single RecoveryDeviceClass instance.
        /// </summary>
        public static RecoveryDeviceClass Instance
        {
            get { return Utils.Singleton<RecoveryDeviceClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new RecoveryDevice(deviceInstance, path);

	        // add it to our list of devices if there are no filters
/*	        if ( _filters.empty() )
	        {
		        dev = new RecoveryDevice(deviceClass, deviceInfoData.DevInst, path);
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
				        dev = new RecoveryDevice(deviceClass, deviceInfoData.DevInst, path);
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
				        dev = new RecoveryDevice(deviceClass, deviceInfoData.DevInst, path);
				        Sleep(1000);
				        return dev;
			        }
		        }
	        }
        	
	        return NULL;
*/        
        }
    }
}


