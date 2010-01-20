#include "stdafx.h"
#include "Device.h"
#pragma warning( disable : 4200 )
#include "Libs/WDK/usbioctl.h"
#pragma warning( default : 4200 )

CMutex Device::m_mutex(FALSE, _T("Device.SendCommand"));

Device::Device(DeviceClass *deviceClass, DEVINST devInst, CStdString path)
: Property(true)
, _parent(NULL)
, _deviceInfoSet(INVALID_HANDLE_VALUE)
{
	if ( deviceClass == NULL )
	{
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
		throw;
	}
	assert ( deviceClass->_deviceInfoSet );

	Trash();
	_deviceClass = deviceClass;
	_hDevInst = devInst;
	_path.put(path);

	InitDevInfo();

	RegisterProperties();
}

void Device::Trash()
{
	_classGuid.put(GUID_NULL);
	_classIconIndex.put(-1);
	_hubIndex.put(0);
    _capabilities = Unknown;
    _classGuidStr.Empty();
	_usbPath.put(_T(""));

	memset(&_deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));

	if ( _parent )
	{
		delete _parent;
		_parent = NULL;
	}

	_removableDevices.erase(_removableDevices.begin(), _removableDevices.end());

	if ( _deviceInfoSet != INVALID_HANDLE_VALUE )
	{
		gSetupApi().SetupDiDestroyDeviceInfoList(_deviceInfoSet);
		_deviceInfoSet = INVALID_HANDLE_VALUE;
	}

    _callbacks.clear();

}

Device::~Device(void)
{
	Trash();
}

void Device::RegisterProperties()
{
	_description.describe(this, _T("Device Description"), _T(""));
	_friendlyName.describe(this, _T("Friendly Name"), _T(""));
	_path.describe(this, _T("Path"), _T(""));
	_usbPath.describe(this, _T("UsbPath"), _T(""));
	_driver.describe(this, _T("Driver"), _T(""));
	_deviceInstanceID.describe(this, _T("Device Instance Id"), _T(""));
	_enumerator.describe(this, _T("Enumerator"), _T(""));
	_classStr.describe(this, _T("Class"), _T(""));
	_classDesc.describe(this, _T("Class Description"), _T(""));
	_classGuid.describe(this, _T("Device Class GUID"), _T("Describes the device class."));
	_classIconIndex.describe(this, _T("Icon"), _T("Index to the system icon."));
	_hub.describe(this, _T("USB Hub Path"), _T("Path of Hub USB Device is connected to."));
	_hubIndex.describe(this, _T("USB Hub Port"), _T("Hub Port the USB Device is connected to."));
	if (Parent() != NULL)
	{
		_parent->describe(this, _T("Parent"), _T("Parent device in the system DevNode tree."));
	}
}

/// <summary>
/// Gets the device's instance handle.
/// </summary>
uint32_t Device::InstanceHandle()
{ 
	return _hDevInst;
}

/// <summary>
/// Property: Gets the device's path.
/// </summary>
CStdString Device::path::get()
{ 
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( _value.IsEmpty() )
	{
		HKEY hKey;
		DWORD error = gSetupApi().CM_Open_DevNode_Key(
			dev->_deviceInfoData.DevInst,
			KEY_QUERY_VALUE, 
			0, 
			RegDisposition_OpenExisting,
			&hKey, 
			CM_REGISTRY_HARDWARE);
		
		if(error == ERROR_SUCCESS)
		{
			BYTE buffer[MAX_PATH];
			DWORD bufsize = MAX_PATH;
			error = RegQueryValueEx( hKey, _T("SymbolicName"), NULL, NULL, (PBYTE)&buffer, &bufsize);
			RegCloseKey(hKey);
			if ( error == ERROR_SUCCESS )
			{
				_value = (PTSTR)buffer;
		        // change path to something we can use for CreateFile()
                // TODO: check to see if the Device Arrival path is the same form as this
                // and if so , use the whole path when doing comparisons.
                _value.Replace(_T("\\??"), _T("\\\\."));
            }
		}
	}
	return _value;
}

/// <summary>
/// Property: Gets the device's USB device path w/o the first 4 characters.
/// </summary>
CStdString Device::usbPath::get()
{ 
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( _value.IsEmpty() )
	{
        if ( /*dev->IsUsb()*/dev->UsbDevice() )
        {
            // Trim the path because the first 4 characters of the path vary
		    // depending on mechanisms used to obtian the path. For example,
		    // some paths start with \\?\ while others start with \??\ with
		    // all other characters being identical.
            _value = dev->UsbDevice()->_path.get().GetBuffer() + 4;
        }
	}
	return _value;
}

/// <summary>
/// Property: Gets the device's class name.
/// </summary>
CStdString Device::classStr::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( _value.IsEmpty() )
	{
		_value = (dev->GetProperty(SPDRP_CLASS, CStdString(_T(""))));
	}

	return _value;
}

/// <summary>
/// Property: Gets the device's class Guid.
/// </summary>
LPGUID Device::classGuid::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( _value == GUID_NULL )
	{
		CStdString str = dev->GetProperty(SPDRP_CLASSGUID, CStdString(_T("")));
		IIDFromString((LPOLESTR)str.c_str(), &_value);
	}

	return &_value;
}

/// <summary>
/// Property: Gets the device's description.
/// </summary>
CStdString Device::description::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( _value.IsEmpty() )
	{
		_value = dev->GetProperty(SPDRP_DEVICEDESC, CStdString(_T("")));
	}

	return _value;
}

/// <summary>
/// Property: Gets the device's friendly name.
/// </summary>
CStdString Device::friendlyName::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( _value.IsEmpty() )
	{
		_value = dev->GetProperty(SPDRP_FRIENDLYNAME, CStdString(_T("")));
	}

	return _value;
}

/// <summary>
/// Property: Gets the device's driver instance.
/// </summary>
CStdString Device::driver::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( _value.IsEmpty() )
	{
		_value = dev->GetProperty(SPDRP_DRIVER, CStdString(_T("")));
	}

	return _value;
}

/// <summary>
/// Property: Gets the device instance id.
/// </summary>
CStdString Device::deviceInstanceID::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( _value.IsEmpty() )
	{
        gSetupApi().apiCM_Get_Device_ID(dev->_deviceInfoData.DevInst, _value);
	}

	return _value;
}

/// <summary>
/// Property: Gets the device's enumerator. ex. "USB"
/// </summary>
CStdString Device::enumerator::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

//w98 this doesn't work in W98. There is no "Enumerator" value
//w98 suggest maybe parsing deviceinstanceid?
//w98 CM_Get_DevNode_Registry_Property(CM_DRP_ENUMERATOR_NAME)
//w98 have to implement apiCM_Get_DevNode_Registry_Property in setupapi.cpp first
	if ( _value.IsEmpty() )
	{
		if (gWinVersionInfo().IsWinNT())
		{
			_value = dev->GetProperty(SPDRP_ENUMERATOR_NAME, CStdString(_T("")));
		}
		else
		{	//w98 ERROR_INVALID_REG_PROPERTY (...|0x209)
			//w98 so we have to use the CM_ function with the CM_DRP_ENUMERATOR_NAME arg
			//w98 for this property.
			ULONG type, length = MAX_PATH; BYTE Buffer[MAX_PATH]; 
			DWORD error = gSetupApi().apiCM_Get_DevNode_Registry_Property(dev->_hDevInst, CM_DRP_ENUMERATOR_NAME, &type,
			Buffer, &length, 0);

			_value = (PTSTR)Buffer;
		}			
	}
	return _value;
}

/// <summary>
/// Property: Gets the description for the device class.
/// </summary>
CStdString Device::classDesc::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	DWORD error;
	if ( _value.IsEmpty() )
	{
		TCHAR buffer[MAX_PATH];
		DWORD RequiredSize;
		if (gSetupApi().apiSetupDiGetClassDescription(dev->_classGuid.get(), (PTSTR)buffer, MAX_PATH, &RequiredSize))
			_value = buffer;
		else
			error = GetLastError();
	}
	return _value;
}

/// <summary>
/// Property: Gets the system icon index for the device class.
/// </summary>
int32_t Device::classIconIndex::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	DWORD error;
	if ( Value == -1 )
		if ( !gSetupApi().SetupDiGetClassImageIndex(dev->_deviceClass->ImageListPtr(), dev->_classGuid.get(), (PINT)&Value))
			error = GetLastError();
//	BOOL ret = SetupDiGetClassBitmapIndex(dev->_classGuid.get(), (PINT)&_value);
//	if (!ret)
//		error = GetLastError();

	return Value;
}

/// <summary>
/// Property: Gets the USB Hub device path that the device is connected to.
/// The USB Hub device is the parent of the USB device
/// </summary>
CStdString Device::hub::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( (dev->UsbDevice() != NULL) &&
		 (dev->UsbDevice()->Parent() != NULL)  && 
		 (dev->UsbDevice()->Parent()->_enumerator.get().CompareNoCase(_T("USB")) == 0) )
	{
		_value = dev->UsbDevice()->Parent()->_path.get();
	}
	else
		_value = _T("");

	return _value;
}

/// <summary>
/// Property: Gets the USB Hub index that the device is connected to.
/// Hub Ports are 1-based. Zero(0) represents an INVALID_HUB_INDEX.
/// </summary>
int32_t Device::hubIndex::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	if ( Value == 0 )
	{
		CStdString hubPath = dev->_hub.get();
		if (hubPath.empty())
			return Value;
		
		// Open the hub.
		DWORD error;
		//w98 - In Windows 98 the hub path starts with \DosDevices\
		//w98 - Replace it with \\.\ so we can use it with CreateFile.
		hubPath.Replace(_T("\\DosDevices"), _T("\\\\."));
		//wxp - In Windows XP the hub path may start with \??\
		//wxp - Replace it with \\.\ so we can use it for CreateFile
		hubPath.Replace(_T("\\??"), _T("\\\\."));
		HANDLE hHub = CreateFile(hubPath, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hHub == INVALID_HANDLE_VALUE) 
		{
			error = GetLastError();
			return Value;
		}

		// See how many Ports are on the Hub
		USB_NODE_INFORMATION NodeInformation;
		DWORD BytesReturned;
		BOOL Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_INFORMATION, &NodeInformation, sizeof(NodeInformation),
											&NodeInformation, sizeof(NodeInformation),&BytesReturned, NULL);
		if (!Success) 
		{
			error = GetLastError();
			CloseHandle(hHub);
			return Value;
		}

		// Loop through the Ports on the Hub any get the unique DriverKey if there is a device connected to
		// the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our Hub Index.
		//
		// Port index is 1-based
		for	(uint8_t index=1; index<=NodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts; index++) 
		{
			USB_NODE_CONNECTION_INFORMATION_EX ConnectionInformation;
			ConnectionInformation.ConnectionIndex = index;
			Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, &ConnectionInformation, sizeof(ConnectionInformation),
									&ConnectionInformation, sizeof(ConnectionInformation), &BytesReturned, NULL);
			if (!Success) 
			{
				error = GetLastError();
				CloseHandle(hHub);
				return Value;
			}

			// There is a device connected to this Port
			if ( ConnectionInformation.ConnectionStatus == DeviceConnected )
			{
				DWORD bytes = sizeof(DWORD)*2 + MAX_PATH*2;
				PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)malloc(bytes);
				driverName->ConnectionIndex = index; 
				// Get the name of the driver key of the device attached to the specified port.
				Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, driverName,
										bytes, driverName, bytes, &BytesReturned, NULL);
				if (!Success) 
				{
					error = GetLastError();
					CloseHandle(hHub);
					free (driverName);
					return Value;
				}

				// If the Driver Keys match, this is the correct Port
				if ( dev->UsbDevice()->_driver.get().CompareNoCase((PWSTR)driverName->DriverKeyName) == 0 )
				{			
					Value = index;
					free (driverName);
					break;
				}
				free (driverName);
			} // end if(connected)
		} // end for(ports)

		CloseHandle(hHub);
		hHub = INVALID_HANDLE_VALUE;
	}
	return Value;
}

/// <summary>
/// Property: Gets the system icon index for the device class.
/// </summary>
int32_t Device::maxPacketSize::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	DWORD error;
	if ( Value == -1 )
		if ( !gSetupApi().SetupDiGetClassImageIndex(dev->_deviceClass->ImageListPtr(), dev->_classGuid.get(), (PINT)&Value))
			error = GetLastError();
//	BOOL ret = SetupDiGetClassBitmapIndex(dev->_classGuid.get(), (PINT)&_value);
//	if (!ret)
//		error = GetLastError();

	return Value;
}


/// <summary>
/// Gets the device's capabilities.
/// </summary>
Device::DeviceCapabilities Device::Capabilities()
{
	if (_capabilities == Unknown)
    {
        _capabilities = (DeviceCapabilities)GetProperty(SPDRP_CAPABILITIES, (DWORD)0);
    }
    
	return _capabilities;
}

/// <summary>
/// Gets a value indicating whether this device is a USB device.
/// </summary>
bool Device::IsUsb()
{
	if (Parent() == NULL)
        return false;

	if (_enumerator.get().CompareNoCase(_T("USB")) == 0)
		return true;

//	if (_enumerator.get().CompareNoCase(_T("ROOT")) == 0)
//		return true;
/*
	if (_enumerator.get().CompareNoCase(_T("USBSTOR")) == 0)
		return true;
*/
//	if (Parent() == NULL)
//        return false;

    return Parent()->IsUsb();
}

/// <summary>
/// Returns true if the USB IDs match the filter string.
/// </summary>
bool Device::ValidateUsbIds()
{
	if (Parent() == NULL)
        return false;

	if (_enumerator.get().CompareNoCase(_T("USB")) == 0)
	{
		// add it to our list of devices if there are no filters
		if ( _deviceClass->_filters.empty() )
		{
			return true;
		}
		else
		{
			// if there are filters, return true if we match one of them
			for (size_t idx=0; idx<_deviceClass->_filters.size(); ++idx)
			{
				CStdString devPath = _path.get();
				if ( devPath.ToUpper().Find(_deviceClass->_filters[idx].ToUpper()) != -1 )
					return true;
			}
			return false;
		}
	}
    
//	if (Parent() == NULL)
//        return false;

    return Parent()->ValidateUsbIds();
}

/// <summary>
/// Gets the device's parent device or null if this device has not parent.
/// </summary>
Device* Device::Parent()
{
    if (_parent == NULL)
    {
        DEVINST parentDevInst = 0;
		DWORD hr = gSetupApi().CM_Get_Parent(&parentDevInst, _deviceInfoData.DevInst, 0);
        if (hr == 0)
        {
			_parent = new Device(_deviceClass, parentDevInst, _T(""));
        }
    }
    return _parent;
}

/// <summary>
/// Gets the device connected to the USB bus.
/// </summary>
Device* Device::UsbDevice()
{
	if (Parent() == NULL)
		return NULL;

	if (_enumerator.get().CompareNoCase(_T("USB")) == 0)
		return this;

//	if (Parent() == NULL)
//        return false;

	return Parent()->UsbDevice();
}

/// <summary>
/// Gets this device's list of removable devices.
/// Removable devices are parent devices that can be removed.
/// </summary>
std::list<Device*>& Device::RemovableDevices()
{
    if ( _removableDevices.empty() )
    {
		if ((Capabilities() & Removable) != 0)
        {
			_removableDevices.push_back(this);
        }
        else
        {
            Device* daddy = Parent();
			if (daddy != NULL)
            {
				std::list<Device*>::iterator it;
				for (it = daddy->RemovableDevices().begin(); it != daddy->RemovableDevices().end(); it++)
				{
					_removableDevices.push_back(*it);
				}
            }
        }
    }
    return _removableDevices;
}

DWORD Device::InitDevInfo()
{
	DWORD error = ERROR_SUCCESS;

	// Make a new devInfoSet that is not tied to a class
	// so we can create Parents and such. Also, we will be able 
	// to get our own Registry Properties and not have to ask the DeviceClass.
	_deviceInfoSet = gSetupApi().SetupDiCreateDeviceInfoList(NULL,NULL);
	if ( _deviceInfoSet == INVALID_HANDLE_VALUE )
	{
		error = GetLastError();
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
        throw;
	}

	CStdString devInstanceId;
    DWORD hr = gSetupApi().apiCM_Get_Device_ID(_hDevInst, devInstanceId);
    if (hr != 0)
	{
		error = GetLastError();
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
        throw;
	}

	// Initialize our SP_DEVINFO_DATA struct for future calls to 
	// SetupDiGetDeviceRegistryProperty()
	_deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	if (!gSetupApi().apiSetupDiOpenDeviceInfo(_deviceInfoSet, devInstanceId.c_str(), NULL, 0, &_deviceInfoData))
	{
		error = GetLastError();
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
        throw;
	}

    return error;
}

CStdString Device::GetProperty( DWORD property, CStdString defaultValue )
{
    DWORD propertyRegDataType = 0;
    DWORD requiredSize = 0;
    DWORD propertyBufferSize = 0;
	PBYTE propertyBuffer = NULL;
	DWORD error;

	if ( !gSetupApi().IsDevNodeOk(this->_deviceInfoData.DevInst) )
	{
		return defaultValue;
	}

	if ( !gSetupApi().apiSetupDiGetDeviceRegistryProperty(_deviceInfoSet, &_deviceInfoData, property, &propertyRegDataType, NULL, 0, &requiredSize))
	{
		error = GetLastError();
		if (error != ERROR_INSUFFICIENT_BUFFER)
		{
			if (error != ERROR_INVALID_DATA)
			{
				ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
                throw;
			}
	

			// Why would we get INVALID_DATA here, but IsDevNodeOk() above returns true?
			//
			// CStdString msg;
			// FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
			// ATLTRACE2(_T("ERROR! Device::GetProperty() - SetupDiGetDeviceRegistryProperty(null) error. %s"), msg.c_str());
			return defaultValue;
		}
	}
	
	propertyBuffer = (PBYTE)malloc(requiredSize);
	propertyBufferSize = requiredSize;

	if ( !gSetupApi().apiSetupDiGetDeviceRegistryProperty(_deviceInfoSet, &_deviceInfoData, property, &propertyRegDataType, 
		propertyBuffer, propertyBufferSize, &requiredSize))
	{
		error = GetLastError();
		ATLTRACE2(_T("ERROR! Device::GetProperty() - SetupDiGetDeviceRegistryProperty() error. (%d)\r\n"), error);
		if (error != ERROR_INVALID_DATA)
		{
			ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
			throw;
		}
		
		free(propertyBuffer);
		return defaultValue;
	}

	CStdString value = (PTSTR)propertyBuffer;
    free(propertyBuffer);

	return value;
}

DWORD Device::GetProperty(DWORD property, DWORD defaultValue)
{
    DWORD propertyRegDataType = 0;
    DWORD requiredSize = 0;
    DWORD propertyBufferSize = sizeof(DWORD);
	DWORD propertyBuffer;
	DWORD error;

	if ( !gSetupApi().IsDevNodeOk(this->_deviceInfoData.DevInst) )
	{
		return defaultValue;
	}

	if ( !gSetupApi().apiSetupDiGetDeviceRegistryProperty(_deviceInfoSet, &_deviceInfoData, property, &propertyRegDataType, 
		(PBYTE)&propertyBuffer, propertyBufferSize, &requiredSize))
	{
		error = GetLastError();
		if (error != ERROR_INVALID_DATA)
		{
			ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
			throw;
		}		
		return defaultValue;
	}

    return propertyBuffer;
}

/// <summary>
/// Ejects the device.
/// </summary>
/// <param name="allowUI">Pass true to allow the Windows shell to display any related UI element, false otherwise.</param>
/// <returns>null if no error occured, otherwise a contextual text.</returns>
//
//w98 TODO: CM_Request_Device_Eject doesn't exist on Win98, so we have to use
//w98 Shane's code from the Win98 Eject app.
DWORD Device::Eject(bool allowUI)
{
    DWORD hr = CR_SUCCESS;
	PNP_VETO_TYPE veto = PNP_VetoTypeUnknown;
	std::list<Device*>::iterator it;

	for(it = RemovableDevices().begin(); it != RemovableDevices().end(); ++it)
    {
        if (allowUI)
        {
            hr = gSetupApi().apiCM_Request_Device_Eject((*it)->InstanceHandle(), NULL, NULL, 0);
            // don't handle errors, there should be a UI for this
			if ( hr != CR_SUCCESS )
				break;
        }
        else
        {
			TCHAR str[1024];
			DWORD hr = gSetupApi().apiCM_Request_Device_Eject((*it)->InstanceHandle(), &veto, str, 1024);
			if ( hr != CR_SUCCESS )
				break;
        }

    }
    return hr;
}

#pragma warning( push )
#pragma warning( disable : 4172 )
HANDLE Device::RegisterCallback(UI_Callback callbackFn)
{
	_callbacks[&callbackFn] = callbackFn;
	return &callbackFn;
};
#pragma warning( pop )

bool Device::UnregisterCallback(HANDLE hObserver)
{
    if ( hObserver )
        return _callbacks.erase(hObserver) == 1;
    else
    {
        bool cleared = !_callbacks.empty();
        _callbacks.clear();
        return cleared;
    }
};

/// <summary>
/// Gets the Device Type from the Device's DeviceClass.
/// </summary>
/// <returns>uint32_t that indicates DeviceClass::DeviceType.</returns>
uint32_t Device::GetDeviceType()
{
    return _deviceClass->_deviceClassType;
};

/// <summary>
/// Compares the current instance with another object of the same type.
/// </summary>
/// <param name="obj">An object to compare with this instance.</param>
/// <returns>A 32-bit signed integer that indicates the relative order of the comparands.</returns>
int32_t Device::CompareTo(Device* device)
{
	// TODO: there has to be something better to key off of for comparing devices
	// than this Index variable.
	if (device == NULL)
	{
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
		throw;
	}

	if ( IsUsb() && device->IsUsb() )
		return _usbPath.get().CompareNoCase(device->_usbPath.get());
	else
		return -1;
}

bool Device::SelfTest(bool eject)
{
	CStdString path = _path.get();
	CStdString usbPath = _usbPath.get();
	uint32_t handle = InstanceHandle();
	CStdString class_str = _classStr.get();
	LPGUID class_guid = _classGuid.get();
	CStdString class_desc = _classDesc.get();
	int32_t icon_idx = _classIconIndex.get();
	CStdString desc = _description.get();
	CStdString name = _friendlyName.get();
	CStdString driver = _driver.get();
	CStdString devInstanceID = _deviceInstanceID.get();
	CStdString enumerator = _enumerator.get();
	int32_t hub_index = _hubIndex.get();
	DeviceCapabilities cap = Capabilities();
	bool is_usb = IsUsb();
	bool is_valid = ValidateUsbIds();
	Device* parent = Parent();
	if (parent)
	{
		bool test = parent->SelfTest();
	}
	std::list<Device*>& rd = RemovableDevices();
	if (eject)
	{
		DWORD veto_false = Eject(false);
		DWORD veto_true = Eject(true);
	}
	int32_t cmp = CompareTo(this);

	return true;
}

void Device::Notify(const NotifyStruct& nsInfo)
{
    // Update the UI
    std::map<HANDLE, UI_Callback>::iterator callback;
    for ( callback = _callbacks.begin(); callback != _callbacks.end(); ++callback )
    {
        (*callback).second(nsInfo);
    }
}