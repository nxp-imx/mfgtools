/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "Device.h"
#pragma warning( disable : 4200 )
#include <usbioctl.h>
#pragma warning( default : 4200 )

#include "MfgToolLib_Export.h"
#include "DeviceManager.h"
#include "DeviceClass.h"
#include "HubClass.h"

//CMutex Device::m_mutex(FALSE, _T("Device.SendCommand"));

Device::Device(DeviceClass *deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
: Property(true)
, _parent(NULL)
, _deviceInfoSet(INVALID_HANDLE_VALUE)
{
	m_pLibHandle = handle;

	_classGuid.put(GUID_NULL);
	_hubIndex.put(0);
	_classGuidStr.Empty();
	_capabilities = Unknown;
	_usbPath.put(_T(""));
	memset(&_deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
	
	_deviceClass = deviceClass;
	_hDevInst = devInst;

	//w98 - In Windows 98 the hub path starts with \DosDevices\
    //w98 - Replace it with \\.\ so we can use it with CreateFile.
	path.Replace(_T("\\DosDevices"), _T("\\\\."));
    //wxp - In Windows XP the hub path may start with \??\
    //wxp - Replace it with \\.\ so we can use it for CreateFile
	path.Replace(_T("\\??"), _T("\\\\."));
	path.Replace(_T("\\\\?"), _T("\\\\."));
	_path.put(path);
	
	InitDevInfo();

    m_dwIndex = -1;
	m_hDevCanDeleteEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	
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
    _hub.describe(this, _T("USB Hub Path"), _T("Path of Hub USB Device is connected to."));
    _hubIndex.describe(this, _T("USB Hub Port"), _T("Hub Port the USB Device is connected to."));
    if (Parent() != NULL)
    {
		_parent->describe(this, _T("Parent"), _T("Parent device in the system DevNode tree."));
    }
}

void Device::Reset(DEVINST devInst, CString path)
{
}

Device::~Device()
{
	_classGuid.put(GUID_NULL);
    _hubIndex.put(0);
    _capabilities = Unknown;
    _classGuidStr.Empty();
    _usbPath.put(_T(""));

	if ( _parent )
    {
        delete _parent;
        _parent = NULL;
    }

	if ( _deviceInfoSet != INVALID_HANDLE_VALUE )
    {
        gSetupApi().SetupDiDestroyDeviceInfoList(_deviceInfoSet);
        _deviceInfoSet = INVALID_HANDLE_VALUE;
    }

	if(m_hDevCanDeleteEvent != NULL)
	{
		::CloseHandle(m_hDevCanDeleteEvent);
		m_hDevCanDeleteEvent = NULL;
	}
}

/// <summary>
/// Gets the device's instance handle.
/// </summary>
DEVINST Device::InstanceHandle()
{ 
    return _hDevInst;
}

DWORD Device::InitDevInfo()
{
	DWORD error = ERROR_SUCCESS;
	
	// Make a new devInfoSet that is not tied to a class
    // so we can create Parents and such. Also, we will be able 
    // to get our own Registry Properties and not have to ask the DeviceClass.
	_deviceInfoSet = gSetupApi().SetupDiCreateDeviceInfoList(NULL,NULL);	//create an empty device information set
	if( _deviceInfoSet == INVALID_HANDLE_VALUE )
    {
		error = GetLastError();
		return error;
	}
	
	CString devInstanceId;
	DWORD hr = gSetupApi().apiCM_Get_Device_ID(_hDevInst, devInstanceId);
    if (hr != 0)
    {
        error = GetLastError();
        return error;
    }
	
	// Initialize our SP_DEVINFO_DATA struct for future calls to 
    // SetupDiGetDeviceRegistryProperty()
    _deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	if (!gSetupApi().apiSetupDiOpenDeviceInfo(_deviceInfoSet, devInstanceId.GetBuffer(), NULL, 0, &_deviceInfoData))
    {
        error = GetLastError();
		devInstanceId.ReleaseBuffer();
        return error;
    }
	devInstanceId.ReleaseBuffer();

    return error;
}

/// <summary>
/// Gets the device's parent device or null if this device has not parent.
/// </summary>
Device* Device::Parent()
{
    if(_parent == NULL)
    {
        DEVINST parentDevInst = 0;
        DWORD hr = gSetupApi().CM_Get_Parent(&parentDevInst, _deviceInfoData.DevInst, 0);

        if(hr == 0)
        {
            _parent = new Device(_deviceClass, parentDevInst, _T(""), m_pLibHandle);
        }
    }

    return _parent;
}

/// <summary>
/// Property: Gets the device's path.
/// </summary>
CString Device::path::get()
{ 
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

    if( _value.IsEmpty() )
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
/// Gets the device connected to the USB bus.
/// </summary>
Device* Device::UsbDevice()
{   
    if (Parent() == NULL)
        return NULL;

    if (_enumerator.get().CompareNoCase(_T("USB")) == 0)
        return this;

    return Parent()->UsbDevice();
}

/// <summary>
/// Property: Gets the device's USB device path w/o the first 4 characters.
/// </summary>
CString Device::usbPath::get()
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

CString Device::GetProperty( DWORD property, CString defaultValue )
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
                //ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
                throw;
            }
    

            // Why would we get INVALID_DATA here, but IsDevNodeOk() above returns true?
            //
            // CString msg;
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
        //ATLTRACE2(_T("ERROR! Device::GetProperty() - SetupDiGetDeviceRegistryProperty() error. (%d)\r\n"), error);
        if (error != ERROR_INVALID_DATA)
        {
            //ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
            throw;
        }
        
        free(propertyBuffer);
        return defaultValue;
    }

    CString value = (PTSTR)propertyBuffer;
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
            throw;
        }       
        return defaultValue;
    }

    return propertyBuffer;
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
        CString str = dev->GetProperty(SPDRP_CLASSGUID, CString(_T("")));
        IIDFromString((LPOLESTR)str.GetBuffer(), &_value);
		str.ReleaseBuffer();
    }

    return &_value;
}

/// <summary>
/// Property: Gets the device's description.
/// </summary>
CString Device::description::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

    if ( _value.IsEmpty() )
    {
        _value = dev->GetProperty(SPDRP_DEVICEDESC, CString(_T("")));
    }

    return _value;
}

/// <summary>
/// Property: Gets the device's friendly name.
/// </summary>
CString Device::friendlyName::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

    if ( _value.IsEmpty() )
    {
        //temp _value = dev->GetProperty(SPDRP_FRIENDLYNAME, CString(_T("")));
		_value = dev->GetProperty(SPDRP_FRIENDLYNAME, CString(_T("Linux File-Stor-lul")));
    }

    return _value;
}

/// <summary>
/// Property: Gets the device's driver instance.
/// </summary>
CString Device::driver::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

    if ( _value.IsEmpty() )
    {
        _value = dev->GetProperty(SPDRP_DRIVER, CString(_T("")));
    }

    return _value;
}

/// <summary>
/// Property: Gets the device instance id.
/// </summary>
CString Device::deviceInstanceID::get()
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
CString Device::enumerator::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

//w98 this doesn't work in W98. There is no "Enumerator" value
//w98 suggest maybe parsing deviceinstanceid?
//w98 CM_Get_DevNode_Registry_Property(CM_DRP_ENUMERATOR_NAME)
//w98 have to implement apiCM_Get_DevNode_Registry_Property in setupapi.cpp first
    if ( _value.IsEmpty() )
    {
		_value = dev->GetProperty(SPDRP_ENUMERATOR_NAME, CString(_T("")));           
    }
    return _value;
}

/// <summary>
/// Property: Gets the device's class name.
/// </summary>
CString Device::classStr::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

    if ( _value.IsEmpty() )
    {
        _value = (dev->GetProperty(SPDRP_CLASS, CString(_T(""))));
    }

    return _value;
}

/// <summary>
/// Property: Gets the description for the device class.
/// </summary>
CString Device::classDesc::get()
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
/// Property: Gets the USB Hub device path that the device is connected to.
/// The USB Hub device is the parent of the USB device
/// </summary>
CString Device::hub::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

    _value = _T("");

    if (dev->UsbDevice() != NULL)
    {
        if(dev->UsbDevice()->Parent() != NULL)
        {
			//if(dev->UsbDevice()->Parent()->_enumerator.get().CompareNoCase(_T("USB")) == 0)
			if(dev->UsbDevice()->Parent()->_classStr.get().CompareNoCase(_T("USB")) == 0)
            {
                //_value = dev->UsbDevice()->Parent()->_path.get();
				CString hubDriver = dev->UsbDevice()->Parent()->_driver.get();
				usb::HubClass *pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
				usb::Hub* pHub = pHubClass->FindHubByDriver(hubDriver);
				if ( pHub )
					_value = pHub->_path.get();
            }
        }
    }

    return _value;
}

int Device::hubIndex::getmsc(USHORT vid, USHORT pid)
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);

	DWORD bytes;
	PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverName ;
	CString TraceStr;

	//if(sLock.IsLocked())
	{
		if ( Value == 0 )
		{
			CString hubPath = dev->_hub.get();
			if (hubPath.IsEmpty())
			{	
				return Value;
			}
			// Open the hub.
			DWORD error;
			//HANDLE hHub = CreateFile(hubPath, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
			hubPath.Replace(_T("\\DosDevices"), _T("\\\\."));
			hubPath.Replace(_T("\\??"), _T("\\\\."));
			HANDLE hHub = CreateFile(hubPath, (GENERIC_READ|GENERIC_WRITE), FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("hubIndex::getmsc() hHub is %x, hubPath is %s"), hHub, hubPath);
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
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("hubIndex::getmsc() GetErrorCode %d for IOCTL_USB_GET_NODE_INFORMATION"),error);
				return Value;
			}

			// Loop through the Ports on the Hub any get the unique DriverKey if there is a device connected to
			// the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our Hub Index.
			//
			// Port index is 1-based
			for	(UCHAR index=1; index<=NodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts; index++) 
			{
				USB_NODE_CONNECTION_INFORMATION_EX ConnectionInformation;
				ConnectionInformation.ConnectionIndex = index;
				Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, &ConnectionInformation, sizeof(ConnectionInformation),
										&ConnectionInformation, sizeof(ConnectionInformation), &BytesReturned, NULL);
				if (!Success) 
				{
					continue;
				}

				// There is a device connected to this Port
				if ( ConnectionInformation.ConnectionStatus == DeviceConnected )
				{
					if(ConnectionInformation.DeviceDescriptor.idVendor != vid ||
						ConnectionInformation.DeviceDescriptor.idProduct != pid)
					{
						continue;
					}

					bytes = sizeof(DWORD)*2 + MAX_PATH*2;
					driverName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)malloc(bytes);
					driverName->ConnectionIndex = index; 

					// Get the name of the driver key of the device attached to the specified port.
					Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, driverName,
											bytes, driverName, bytes, &BytesReturned, NULL);
					
					if (!Success) 
					{
						continue;
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
	} 
//	if(Value == 0)
//	{
//		Value = dev->GetProperty(SPDRP_ADDRESS, 0);
//	}

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device::hubIndex::getmsc, return the port index is: %d"), Value);
	return Value;
}


/// <summary>
/// Property: Gets the USB Hub index that the device is connected to.
/// Hub Ports are 1-based. Zero(0) represents an INVALID_HUB_INDEX.
/// </summary>
int Device::hubIndex::get2()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

	if(Value == 0)
	{
//		CString hubPath = dev->_hub.get();
//		if (hubPath.IsEmpty())
//		{
//			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, hubPath is NULL"));
//            return Value;
//		}
		// Open the hub.
//        DWORD error;
		//w98 - In Windows 98 the hub path starts with \DosDevices\
        //w98 - Replace it with \\.\ so we can use it with CreateFile.
//        hubPath.Replace(_T("\\DosDevices"), _T("\\\\."));
        //wxp - In Windows XP the hub path may start with \??\
        //wxp - Replace it with \\.\ so we can use it for CreateFile
//        hubPath.Replace(_T("\\??"), _T("\\\\."));
//		HANDLE hHub = CreateFile(hubPath, (GENERIC_READ|GENERIC_WRITE), FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
//        if (hHub == INVALID_HANDLE_VALUE) 
//        {
//            error = GetLastError();
//			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, CreateFile(hHub) failed, errcode: %d"), error);
//            return Value;
//        }
		// See how many Ports are on the Hub
//		USB_NODE_INFORMATION NodeInformation;
//        DWORD BytesReturned;
//        BOOL Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_INFORMATION, &NodeInformation, sizeof(NodeInformation),
//                                            &NodeInformation, sizeof(NodeInformation),&BytesReturned, NULL);
//        if (!Success) 
//        {
//            error = GetLastError();
//            CloseHandle(hHub);
//			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, IOCTL_USB_GET_NODE_INFORMATION failed, errcode: %d"), error);
//            return Value;
//        }
		// Port index is 1-based
		Value = dev->GetProperty(SPDRP_ADDRESS, 0);

//		CloseHandle(hHub);
//        hHub = INVALID_HANDLE_VALUE;
	}
/*
    if ( Value == 0 )
    {
        CString hubPath = dev->_hub.get();
		if (hubPath.IsEmpty())
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, hubPath is NULL"));
            return Value;
		}
        
        // Open the hub.
        DWORD error;
        //w98 - In Windows 98 the hub path starts with \DosDevices\
        //w98 - Replace it with \\.\ so we can use it with CreateFile.
        hubPath.Replace(_T("\\DosDevices"), _T("\\\\."));
        //wxp - In Windows XP the hub path may start with \??\
        //wxp - Replace it with \\.\ so we can use it for CreateFile
        hubPath.Replace(_T("\\??"), _T("\\\\."));
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device::hubIndex::get, hubPath is: %s"), hubPath);
        //HANDLE hHub = CreateFile(hubPath, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
		HANDLE hHub = CreateFile(hubPath, (GENERIC_READ|GENERIC_WRITE), FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hHub == INVALID_HANDLE_VALUE) 
        {
            error = GetLastError();
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, CreateFile(hHub) failed, errcode: %d"), error);
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
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, IOCTL_USB_GET_NODE_INFORMATION failed, errcode: %d"), error);
            return Value;
        }

        // Loop through the Ports on the Hub any get the unique DriverKey if there is a device connected to
        // the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our Hub Index.
        //
        // Port index is 1-based
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device::hubIndex::get, the hub has port numbers: %d"), NodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts);
        for (UCHAR index=1; index<=NodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts; index++) 
        {
            USB_NODE_CONNECTION_INFORMATION_EX ConnectionInformation;
            ConnectionInformation.ConnectionIndex = index;
            Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, &ConnectionInformation, sizeof(ConnectionInformation),
                                    &ConnectionInformation, sizeof(ConnectionInformation), &BytesReturned, NULL);
            if (!Success) 
            {
                error = GetLastError();
				if(error == ERROR_DEVICE_NOT_CONNECTED)
				{
					continue;
				}
                CloseHandle(hHub);
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX failed, errcode: %d"), error);
                return Value;
            }

            // There is a device connected to this Port
            if ( ConnectionInformation.ConnectionStatus == DeviceConnected )
            {
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device::hubIndex::get, the port(%d) has connected to a device"), index);
                DWORD bytes = sizeof(DWORD)*2 + MAX_PATH*2;
                PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)malloc(bytes);
				memset(driverName, 0, bytes);
                driverName->ConnectionIndex = index; 
                // Get the name of the driver key of the device attached to the specified port.
                Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, driverName,
                                        bytes, driverName, bytes, &BytesReturned, NULL);
                if (!Success) 
                {
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME failed, DriverKey: %s"), (PWSTR)driverName->DriverKeyName);
					int retrycnt = 1;
					while(retrycnt <= 5)
					{
						Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, driverName,
                                        bytes, driverName, bytes, &BytesReturned, NULL);
						if(!Success)
						{
							retrycnt++;
						}
						else
						{
							break;
						}
					}
					if(!Success)
					{
						error = GetLastError();
						CloseHandle(hHub);
						free (driverName);
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Device::hubIndex::get, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME failed, errcode: %d, RetryCount: %d"), error, retrycnt);
						return Value;
					}
                }

                // If the Driver Keys match, this is the correct Port
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device::hubIndex::get, the port(%d) connected device's DriverKeyName is %s, and the find device driverkey is: %s"), index, (PWSTR)driverName->DriverKeyName, dev->UsbDevice()->_driver.get());
                if ( dev->UsbDevice()->_driver.get().CompareNoCase((PWSTR)driverName->DriverKeyName) == 0 )
                {           
                    Value = index;
                    free (driverName);
					driverName = NULL;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device::hubIndex::get, find the matched port(%d)"), Value);
                    break;
                }
                free (driverName);
				driverName = NULL;
            } // end if(connected)
        } // end for(ports)

        CloseHandle(hHub);
        hHub = INVALID_HANDLE_VALUE;
    }
*/
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device::hubIndex::get, return the port index is: %d"), Value);
    return Value;
}

int Device::hubIndex::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
	if(Value == 0)
	{
		CString hubPath = dev->_hub.get();
		if (hubPath.IsEmpty())
		{
			return Value;
		}
		// Open the hub.
		DWORD error;
		hubPath.Replace(_T("\\DosDevices"), _T("\\\\."));
		hubPath.Replace(_T("\\??"), _T("\\\\."));
		HANDLE hHub = CreateFile(hubPath, (GENERIC_READ|GENERIC_WRITE), FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
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
		// Loop through the Ports on the Hub
		for(UCHAR index=1; index<=NodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts; index++)
		{
			USB_NODE_CONNECTION_INFORMATION_EX ConnectionInformation;
            ConnectionInformation.ConnectionIndex = index;
			Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, &ConnectionInformation, sizeof(ConnectionInformation),
                                    &ConnectionInformation, sizeof(ConnectionInformation), &BytesReturned, NULL);
            if (!Success) 
            {
                continue;
			}
			// There is a device connected to this Port
            if ( ConnectionInformation.ConnectionStatus == DeviceConnected )
			{
				DWORD bytes = sizeof(DWORD)*2 + MAX_PATH*2;
				PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)malloc(bytes);
				memset(driverName, 0, bytes);
                driverName->ConnectionIndex = index;
				// Get the name of the driver key of the device attached to the specified port.
                Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, driverName,
                                        bytes, driverName, bytes, &BytesReturned, NULL);
                if (!Success)
				{
					continue;
				}
				// If the Driver Keys match, this is the correct Port
				if( dev->UsbDevice()->_driver.get().CompareNoCase((PWSTR)driverName->DriverKeyName) == 0 )
				{
					Value = index;
                    free (driverName);
					driverName = NULL;
					break;
				}
			}
		}
		CloseHandle(hHub);
        hHub = INVALID_HANDLE_VALUE;
	}

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
/*
    if (_enumerator.get().CompareNoCase(_T("USBSTOR")) == 0)
        return true;
*/

    return Parent()->IsUsb();
}

/// <summary>
/// Returns true if the USB IDs match the filter string.
/// </summary>
bool Device::ValidateUsbIds()
{
/*
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
                CString devPath = _path.get();
                if ( devPath.ToUpper().Find(_deviceClass->_filters[idx].ToUpper()) != -1 )
                    return true;
            }
            return false;
        }
    }

    return Parent()->ValidateUsbIds(); */
	return true;
}

/// <summary>
/// Gets the Device Type from the Device's DeviceClass.
/// </summary>
/// <returns>DWORD that indicates DeviceClass::DeviceType.</returns>
DWORD Device::GetDeviceType()
{
	return _deviceClass->_deviceClassType;
};

DWORD Device::GetDeviceWndIndex(void)
{
    return m_dwIndex;
}

void Device::SetDeviceWndIndex(DWORD dwIndex)
{
    m_dwIndex = dwIndex;
}


