/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "DeviceClass.h"
#include "HubClass.h"
#include "UsbHub.h"
#include "DeviceManager.h"
#include "MfgToolLib_Export.h"

DeviceClass::DeviceClass(LPCGUID iFaceGuid, LPCGUID devGuid, LPCTSTR enumerator, DEV_CLASS_TYPE type, INSTANCE_HANDLE handle)
: Property(true)
, _deviceInfoSet(INVALID_HANDLE_VALUE)
, _deviceClassType(type)
{	
	if(iFaceGuid)
		_classIfaceGuid.put(*iFaceGuid);
	if(devGuid)
        _classDevGuid.put(*devGuid);
	if (enumerator)
        _enumerator.put(enumerator);

	m_pLibHandle = handle;
	
	_classDesc.describe(this, _T("Class Description"), _T(""));
	_classIfaceGuid.describe(this, _T("Device Interface GUID"), _T("Describes the device class interface."));
	_classDevGuid.describe(this, _T("Device Class GUID"), _T("Describes the device class."));
	_enumerator.describe(this, _T("Enumerator"), _T(""));

	devicesMutex = CreateMutex(NULL, FALSE, NULL);
	if(devicesMutex == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("create DeviceClass::devicesMutex failed"));
		throw 1;
	}

	m_msc_vid = 0;
	m_msc_pid = 0;
}

DeviceClass::~DeviceClass()
{
	WaitForSingleObject(devicesMutex, INFINITE);
	//delete all devices that are created
	while ( _devices.size() > 0 )
    {
        Device * dev = _devices.back();
        _devices.pop_back();
        delete dev;
    }
	while ( _oldDevices.size() > 0 )
    {
        Device * dev = _oldDevices.back();
        _oldDevices.pop_back();
        delete dev;
    }
	ReleaseMutex(devicesMutex);
    CloseHandle(devicesMutex);

	if ( _deviceInfoSet != INVALID_HANDLE_VALUE )
    {
        gSetupApi().SetupDiDestroyDeviceInfoList(_deviceInfoSet);
        _deviceInfoSet = INVALID_HANDLE_VALUE;
    }

	_propertyList.clear();
}

HDEVINFO DeviceClass::GetDevInfoSet()
{
	// reset the list if it exists
    if( _deviceInfoSet != INVALID_HANDLE_VALUE )
    {
        gSetupApi().SetupDiDestroyDeviceInfoList(_deviceInfoSet);
        _deviceInfoSet = INVALID_HANDLE_VALUE;
		//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceClass::GetDevInfoSet--SetupDiDestroyDeviceInfoLists()"));
    }
	
	// decide the criteria for the devices
    if( *_classIfaceGuid.get() != GUID_NULL )
    {
        _deviceInfoSet = gSetupApi().apiSetupDiGetClassDevs(_classIfaceGuid.get(), NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
		//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceClass::GetDevInfoSet--apiSetupDiGetClassDevs(), _deviceInfoSet: 0x%X"), _deviceInfoSet);
    }
    else
    {
        DWORD flags = 0;
        LPCGUID pGuid;
        CString enumerator;
        if ( _enumerator.get().IsEmpty() )
        {
            pGuid = _classDevGuid.get();
            enumerator = _T("");
            flags = DIGCF_PRESENT;
        }
        else
        {
            pGuid = NULL;
            enumerator = _enumerator.get();
            flags = DIGCF_PRESENT | DIGCF_ALLCLASSES;
        }
        
		_deviceInfoSet = gSetupApi().apiSetupDiGetClassDevs(pGuid, enumerator.IsEmpty() ? NULL : enumerator.GetBuffer(), 0, flags);
		enumerator.ReleaseBuffer();
    }
    
    return _deviceInfoSet;
}

void DeviceClass::DestroyDevInfoSet()
{
	if( _deviceInfoSet != INVALID_HANDLE_VALUE )
	{
		gSetupApi().SetupDiDestroyDeviceInfoList(_deviceInfoSet);
        _deviceInfoSet = INVALID_HANDLE_VALUE;
		//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceClass::GetDevInfoSet--DestroyDevInfoSet()"));
	}
}

DWORD DeviceClass::EnumDeviceInterfaceDetails(DWORD index, CString& devPath, PSP_DEVINFO_DATA pDevData)
{
	DWORD error;
	SP_DEVICE_INTERFACE_DATA interfaceData;
	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	
	if(!gSetupApi().SetupDiEnumDeviceInterfaces(_deviceInfoSet, NULL, _classIfaceGuid.get(), index, &interfaceData))
    {
        error = GetLastError();
        if(error == ERROR_NO_MORE_ITEMS)
		{
            return error;
		}
    }
	
	DWORD requiredSize = 0;
	if(!gSetupApi().apiSetupDiGetDeviceInterfaceDetail(_deviceInfoSet, &interfaceData, NULL, 0, &requiredSize, NULL))
	{
		error = GetLastError();
		//return error;
	}
	PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
    detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	if(!gSetupApi().apiSetupDiGetDeviceInterfaceDetail(_deviceInfoSet, &interfaceData, detailData, requiredSize, NULL, pDevData))
    {
        free(detailData);
        error = GetLastError();
		return error;
    }
	devPath = (PTSTR)detailData->DevicePath;
    free(detailData);
	
	return ERROR_SUCCESS;
}

//Enum all devices according to GUID
DEVICES_ARRAY& DeviceClass::Devices()
{
	if( _devices.empty() )
	{
		DWORD error;
		DWORD index = 0;  //from 0, find one by one
		
		GetDevInfoSet();
		while(TRUE)
		{
			SP_DEVINFO_DATA devData;
			devData.cbSize = sizeof(SP_DEVINFO_DATA);
			CString devPath = _T("");
			if ( *_classIfaceGuid.get() != GUID_NULL )
			{
				error = EnumDeviceInterfaceDetails(index, devPath, &devData); //mainly for get devPath
				if(error == ERROR_NO_MORE_ITEMS) //all have found
				{
					break;
				}
			}
			else
			{
				if( !gSetupApi().SetupDiEnumDeviceInfo(_deviceInfoSet, index, &devData) )
				{
					error = GetLastError();
					if(error == ERROR_NO_MORE_ITEMS)
					{
						break;
					}
				}
			}
			
			//find one, so create device
			Device* device = CreateDevice(this, devData, devPath);
			// Create device will return NULL if there are filters in place
            // and the device does not match a filter
			if( device != NULL )
			{
				// init the hub and hub index fields, because if we wait till the 
                // device is gone, it will be too late.
				if( (device->_deviceClass->_deviceClassType != DeviceTypeUsbController) &&
					(device->_deviceClass->_deviceClassType != DeviceTypeUsbHub))
				{
					device->_hubIndex.get();	//get the port index in the hub that this device is connected to
				}
                _devices.push_back(device);
			}
			
			index++;
		}
		DestroyDevInfoSet();
	}
	
	return _devices;
}

CString DeviceClass::classDesc::get()
{
    DWORD error;
    if( _value.IsEmpty() )
    {
        DeviceClass* devClass = dynamic_cast<DeviceClass*>(_owner);
        ASSERT(devClass);

        TCHAR buffer[MAX_PATH];
        DWORD RequiredSize;
        if(gSetupApi().apiSetupDiGetClassDescription(devClass->_classDevGuid.get(), (PTSTR)buffer, MAX_PATH, &RequiredSize))
            _value = buffer;
        else
            error = GetLastError();
    }
    return _value;
}

Device* DeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
    Device* dev = new Device(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);

	return dev;
}

Device* DeviceClass::FindDeviceByUsbPath(CString pathToFind, const DeviceListType devListType, const DeviceListAction devListAction )
{
	if (pathToFind.IsEmpty())
    {
        return NULL;
    }
	Device * pDevice = NULL;
	CString devInstPathToFind = pathToFind.Left(pathToFind.ReverseFind(_T('#')));
	devInstPathToFind.Replace(_T('#'), _T('\\'));
	switch ( devListType )
	{
	case DeviceListType_Current:
		{
			// Find the Device in our list of CURRENT devices.
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::FindDeviceByUsbPath--DeviceListType_Current, _devices.size: %d"), _devices.size());
			std::list<Device*>::iterator deviceIt;
			for(deviceIt=_devices.begin(); deviceIt!=_devices.end(); ++deviceIt)
			{
				if((*deviceIt)->IsUsb())
				{
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::FindDeviceByUsbPath--DeviceListType_Current, devInstPathToFind: %s, _deviceInstanceID: %s"), devInstPathToFind, (*deviceIt)->UsbDevice()->_deviceInstanceID.get());
					if( devInstPathToFind.CompareNoCase( (*deviceIt)->UsbDevice()->_deviceInstanceID.get() ) == 0 )
					{
						pDevice = (*deviceIt);
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::FindDeviceByUsbPath--DeviceListType_Current, Find the device"));
						break;
					}
				}
			}
		}
		break;
	case DeviceListType_New:
		{
			// Get a new list of our devices from Windows and see if it is there.
			DWORD error;
			SP_DEVINFO_DATA devData;
			devData.cbSize = sizeof(SP_DEVINFO_DATA);
			CString devPath = _T("");

			GetDevInfoSet();
			for(int index=0; /*no condition*/; ++index)
			{
				if ( *_classIfaceGuid.get() != GUID_NULL /*&&	gWinVersionInfo().IsWinNT()*/ )
				{
					memset(&devData, 0, sizeof(SP_DEVINFO_DATA));
					devData.cbSize = sizeof(SP_DEVINFO_DATA);
					devPath.Empty();
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceClass::FindDeviceByUsbPath() - DeviceListType_New--index: %d"), index);
					error = EnumDeviceInterfaceDetails(index, devPath, &devData);
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceClass::FindDeviceByUsbPath() - DeviceListType_New--devPath: %s"), devPath);
				//	if ( error != ERROR_SUCCESS )
				//	{	// No match
				//		// Enum() will return ERROR_NO_MORE_ITEMS when done
				//		// but regardless, we can't add the device
				//		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceClass::FindDeviceByUsbPath() - EnumDeviceInterfaceDetails failed, errcode: %d"), error);
				//		pDevice = NULL;
				//		break;
				//	}
					if( error == ERROR_NO_MORE_ITEMS )
					{
						pDevice = NULL;
						break;
					}
				}
				else
				{
					if (!gSetupApi().SetupDiEnumDeviceInfo(_deviceInfoSet, index, &devData))
					{
						// Enum() will return ERROR_NO_MORE_ITEMS when done
						// but regardless, we can't add the device
						//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceClass::FindDeviceByUsbPath() - SetupDiEnumDeviceInfo failed"));
						pDevice = NULL;
						break;
					}
				}
//				ATLTRACE(_T("%s::FindDeviceByUsbPath()  Enumerated device. %s\r\n"), this->ToString().c_str(), devPath.c_str());
				if( !(devPath.IsEmpty()) )
				{
					pDevice = CreateDevice(this, devData, devPath);
					if( pDevice && pDevice->IsUsb() )
					{
//						ATLTRACE(_T("%s::FindDeviceByUsbPath()  Created device. %s\r\n"), this->ToString().c_str(), devPath.c_str());
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::FindDeviceByUsbPath--DeviceListType_New, devInstPathToFind: %s, _deviceInstanceID: %s"), devInstPathToFind, pDevice->UsbDevice()->_deviceInstanceID.get());
						if( devInstPathToFind.CompareNoCase( pDevice->UsbDevice()->_deviceInstanceID.get() ) == 0 )
						{
							// Found what we are looking for
							WaitForSingleObject(devicesMutex, INFINITE);
							DWORD portindex = pDevice->_hubIndex.get();
							_devices.push_back(pDevice);
							ReleaseMutex(devicesMutex);
							LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::FindDeviceByUsbPath--DeviceListType_New, Find the device, Port: %d"), portindex);
							break;
						}
					}
					// if we got here, the device isn't the device we are looking for, so clean up.
					if ( pDevice )
					{
						delete pDevice;
						pDevice = NULL;
					}
				}
			}
			DestroyDevInfoSet();
		}
		break;
	}

	return pDevice;

#if 0
	if (pathToFind.IsEmpty())
    {
        return NULL;
    }

    Device * pDevice = NULL;
	CString devInstPathToFind = pathToFind.Left(pathToFind.ReverseFind(_T('#')));
	devInstPathToFind.Replace(_T('#'), _T('\\'));

	// existing application device list or new OS device list?
    switch ( devListType )
	{
		case DeviceListType_Old:
		{
			// Find the Device in our list of OLD devices.
			std::list<Device*>::iterator device;
			for ( device=_oldDevices.begin(); device != _oldDevices.end(); ++device )
			{
				if ( (*device)->IsUsb() )
				{
					if ( devInstPathToFind.CompareNoCase( (*device)->UsbDevice()->_deviceInstanceID.get() ) == 0 )
					{
						if ( devListAction == DeviceListAction_Remove )
						{
							delete (*device);
							_oldDevices.erase(device);
						}
						break;
					}
				}
			}
			break;
		}
		case DeviceListType_Current:
		{
			// Find the Device in our list of CURRENT devices.
			std::list<Device*>::iterator device;
			for ( device=_devices.begin(); device != _devices.end(); ++device )
			{
				if ( (*device)->IsUsb() )
				{
					if ( devInstPathToFind.CompareNoCase( (*device)->UsbDevice()->_deviceInstanceID.get() ) == 0 )
					{
						pDevice = (*device);

						if ( devListAction == DeviceListAction_Remove )
						{
							WaitForSingleObject(devicesMutex, INFINITE);
							_oldDevices.push_back(pDevice);
							_devices.erase(device);
							ReleaseMutex(devicesMutex);
						}
						break;
					}
				}
			}
			break;
		}
		case DeviceListType_New:
		{
			// Get a new list of our devices from Windows and see if it is there.
			DWORD error;

			SP_DEVINFO_DATA devData;
			devData.cbSize = sizeof(SP_DEVINFO_DATA);
			CString devPath = _T("");

			GetDevInfoSet();
			for (int index=0; /*no condition*/; ++index)
			{
				if ( *_classIfaceGuid.get() != GUID_NULL /*&&	gWinVersionInfo().IsWinNT()*/ )
				{
					error = EnumDeviceInterfaceDetails(index, devPath, &devData);
					if ( error != ERROR_SUCCESS )
					{	// No match
						// Enum() will return ERROR_NO_MORE_ITEMS when done
						// but regardless, we can't add the device
						pDevice = NULL;
						break;
					}
				}
				else
				{
					if (!gSetupApi().SetupDiEnumDeviceInfo(_deviceInfoSet, index, &devData))
					{
						// Enum() will return ERROR_NO_MORE_ITEMS when done
						// but regardless, we can't add the device
						pDevice = NULL;
						break;
					}
				}

//				ATLTRACE(_T("%s::FindDeviceByUsbPath()  Enumerated device. %s\r\n"), this->ToString().c_str(), devPath.c_str());
				pDevice = CreateDevice(this, devData, devPath);
				if ( pDevice && pDevice->IsUsb() )
				{
//					ATLTRACE(_T("%s::FindDeviceByUsbPath()  Created device. %s\r\n"), this->ToString().c_str(), devPath.c_str());
					if ( devInstPathToFind.CompareNoCase( pDevice->UsbDevice()->_deviceInstanceID.get() ) == 0 )
					{
						// Found what we are looking for
						if ( devListAction == DeviceListAction_Add )
						{
							WaitForSingleObject(devicesMutex, INFINITE);
							_devices.push_back(pDevice);
							ReleaseMutex(devicesMutex);
						}
						break;
					}
				}
				// if we got here, the device isn't the device we
				// are looking for, so clean up.
				if ( pDevice )
				{
					delete pDevice;
					pDevice = NULL;
				}
			}
			break;
		}  // end case DeviceListNew:
		
		default:
			break;
	} // end switch (devListType)

	return pDevice;
#endif
}

DeviceClass::NotifyStruct DeviceClass::AddUsbDevice(LPCTSTR path)
{
	NotifyStruct nsInfo = {0};
	Device * pDevice = NULL;
	CString pathToFind = path + 4;

	int RetryCount = 0;
	while( RetryCount < RETRY_COUNT )
	{
		// see if it is already in our list of Devices()
		pDevice = FindDeviceByUsbPath(pathToFind, DeviceListType_Current, DeviceListAction_None);
		if ( pDevice == NULL )
		{
			// it's not in our Collection of constructed devices
			// so lets get a new list of our devices from Windows
			// and see if it is there.
			pDevice = FindDeviceByUsbPath(pathToFind, DeviceListType_New, DeviceListAction_Add);
			if(pDevice != NULL)
			{
				//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::AddUsbDevice() successful %s add to current list"), path);
				break;
			}
			else
			{
				RetryCount++;
			}
		}
		else
		{
			break;
		}
	}
	if(RetryCount < RETRY_COUNT)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::AddUsbDevice() successful %s add to current list, retrycount: %d"), pathToFind, RetryCount);
	}
	else
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::AddUsbDevice() failed %s NOT belong to me"), pathToFind);
	}

	if ( pDevice != NULL )
	{
		nsInfo.Device = pDevice;
        nsInfo.Type = _deviceClassType;
		nsInfo.DriverLetter = _T('');
        nsInfo.PortIndex = pDevice->_hubIndex.get();
        nsInfo.Hub = pDevice->_hub.get(); 
		// Find our Hub in gDeviceManager's list of [Hub].Devices()
		usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
		usb::Hub *pHub = pHubClass->FindHubByPath(nsInfo.Hub);
		if(pHub != NULL)
		{
			nsInfo.HubIndex = pHub->_index.get();
		}

		// Delete device from old device list since it's in our current list
//		FindDeviceByUsbPath(pathToFind, DeviceListType_Old, DeviceListAction_Remove);

		if(RefreshPort(nsInfo.Hub, nsInfo.PortIndex) != ERROR_SUCCESS)
		{
			nsInfo.Device = NULL;
		}
	}

	return nsInfo;
}

DeviceClass::NotifyStruct DeviceClass::RemoveUsbDevice(LPCTSTR path)
{
	NotifyStruct nsInfo = {0};
	CString trimmedPath = path + 4;

	// see if it is in our list of Devices
    Device* pDevice = FindDeviceByUsbPath(trimmedPath, DeviceListType_Current, DeviceListAction_Remove);
	if(pDevice != NULL)
	{
		nsInfo.Device = pDevice;
        nsInfo.Type = _deviceClassType;
		nsInfo.DriverLetter = _T('');
		nsInfo.PortIndex = pDevice->_hubIndex.get();
        nsInfo.Hub = pDevice->_hub.get();
		// Find our Hub in gDeviceManager's list of [Hub].Devices()
		usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
		usb::Hub *pHub = pHubClass->FindHubByPath(nsInfo.Hub);
		if(pHub != NULL)
		{
			nsInfo.HubIndex = pHub->_index.get();
		}
		ClearPort(nsInfo.Hub, nsInfo.PortIndex);
	}
	
	return nsInfo;
}

void DeviceClass::SetMSCVidPid(USHORT vid, USHORT pid)
{
	m_msc_vid = vid;
	m_msc_pid = pid;
}

std::list<Device*>& DeviceClass::Refresh()
{
	while ( _devices.size() > 0 )
	{
		Device * dev = _devices.back();
		_devices.pop_back();
		delete dev;
	}

	return Devices();
}

int DeviceClass::RefreshPort(const CString hubPath, const int hubIndex)
{
	// Find our Hub in gDeviceManager's list of [Hub].Devices()
	usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
	
	ASSERT(pHubClass);
	
	usb::Hub* pHub = pHubClass->FindHubByPath(hubPath);

	if ( pHub )
	{
		return pHub->RefreshPort(hubIndex);
	}
	else
	{
		return ERROR_FILE_NOT_FOUND;
	}
}

int DeviceClass::ClearPort(const CString hubPath, const int hubIndex)
{
	// Find our Hub in gDeviceManager's list of [Hub].Devices()
	usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
	
	ASSERT(pHubClass);
	
	usb::Hub* pHub = pHubClass->FindHubByPath(hubPath);

	if ( pHub )
	{
		return pHub->ClearPort(hubIndex);
	}
	else
	{
		return ERROR_FILE_NOT_FOUND;
	}
}



