using System;

namespace DevSupport.DeviceManager
{
    public sealed class ScsiDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the RecoveryDeviceClass class.
        /// </summary>
        private ScsiDeviceClass()
            : base(Guid.Empty, Win32.GUID_DEVCLASS_SCSIADAPTER, null)
        { }

        /// <summary>
        /// Gets the single RecoveryDeviceClass instance.
        /// </summary>
        public static ScsiDeviceClass Instance
        {
            get { return Utils.Singleton<ScsiDeviceClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new ScsiDevice(deviceInstance, path);

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


