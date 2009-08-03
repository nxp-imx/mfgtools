using System;

namespace DevSupport.DeviceManager
{
    public sealed class HidDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the HidDeviceClass class.
        /// </summary>
        private HidDeviceClass()
            : base(Win32.GUID_DEVINTERFACE_HID, Win32.GUID_DEVCLASS_HIDCLASS, "HID")
        { }

        /// <summary>
        /// Gets the single HidDeviceClass instance.
        /// </summary>
        public static HidDeviceClass Instance
        {
            get { return Utils.Singleton<HidDeviceClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new HidDevice(deviceInstance, path);

            // add it to our list of devices if there are no filters
            /*	        if ( _filters.empty() )
                        {
                            dev = new HidDevice(deviceClass, deviceInfoData.DevInst, path);
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
                                    dev = new HidDevice(deviceClass, deviceInfoData.DevInst, path);
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
                                    dev = new HidDevice(deviceClass, deviceInfoData.DevInst, path);
                                    return dev;
                                }
                            }
                        }
        	
                        return NULL;
            */
        }
    }
}
/*
size_t HidDeviceClass::AddFilter(uint16_t vid, uint16_t pid)
{
	CStdString filter;
	filter.Format(_T("hid#vid_%04x&pid_%04x"), vid, pid);
	_filters.push_back(filter);
	return _filters.size();
}

size_t HidDeviceClass::AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance)
{
	CStdString filter;
	filter.Format(_T("hid#vid_%s&pid_%s#%s"), vid, pid, instance);
	_filters.push_back(filter);
	return _filters.size();
}
*/
