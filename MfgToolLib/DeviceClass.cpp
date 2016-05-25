/*
 * Copyright 2009-2013, 2016 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
	devicesMutex = new pthread_mutex_t;

	pthread_mutex_init(devicesMutex,NULL);// = CreateMutex(NULL, FALSE, NULL);
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
#if 0
	pthread_mutex_lock(devicesMutex);// , INFINITE);

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
	pthread_mutex_unlock(devicesMutex);
    pthread_mutex_destroy(devicesMutex);

	if ( _deviceInfoSet != INVALID_HANDLE_VALUE )
    {
        gSetupApi().SetupDiDestroyDeviceInfoList(_deviceInfoSet);
        _deviceInfoSet = INVALID_HANDLE_VALUE;
    }

	_propertyList.clear();
#endif
}

HDEVINFO DeviceClass::GetDevInfoSet()
{
#if 0	// reset the list if it exists
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
#endif
    return _deviceInfoSet;
}

void DeviceClass::DestroyDevInfoSet()
{
#if 0
	if( _deviceInfoSet != INVALID_HANDLE_VALUE )
	{
		gSetupApi().SetupDiDestroyDeviceInfoList(_deviceInfoSet);
        _deviceInfoSet = INVALID_HANDLE_VALUE;
		//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceClass::GetDevInfoSet--DestroyDevInfoSet()"));
	}
#endif
}

DWORD DeviceClass::EnumDeviceInterfaceDetails(DWORD index, CString& devPath, PSP_DEVINFO_DATA pDevData)

{
#if 0
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
	#endif
	return ERROR_NO_MORE_ITEMS;
}

//Enum all devices according to GUID
DEVICES_ARRAY& DeviceClass::Devices()
{
#if 0
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
	#endif
	return _devices;
}

CString DeviceClass::classDesc::get()
{
#if 0
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
#endif
    return _value;
}

Device* DeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
    Device* dev = new Device(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);

	return dev;
}

Device* DeviceClass::FindDeviceByUsbPath(CString pathToFind, const DeviceListType devListType, const DeviceListAction devListAction )
{
#if 0
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
							pthread_mutex_lock(devicesMutex);// , INFINITE);
							DWORD portindex = pDevice->_hubIndex.get();
							_devices.push_back(pDevice);
							pthread_mutex_unlock(devicesMutex);
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
#endif
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

DeviceClass::NotifyStruct DeviceClass::AddUsbDevice(LPCTSTR path,libusb_device *dev)
{
#ifndef __linux__

	NotifyStruct nsInfo = {0};
	CString pathToFind = path + 4;
	Device * pDevice = NULL;


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
		nsInfo.DriverLetter = _T('\0');
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
#else

	NotifyStruct nsInfo = {0};
	SP_DEVINFO_DATA devData;
	devData.cbSize = sizeof(SP_DEVINFO_DATA);

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Add USB device\n"));
	Device *pDevice = CreateDevice(this,devData , _T(""));
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Create Device %p\n"), pDevice);
	int rc=libusb_open(dev,&pDevice->m_libusbdevHandle);
	 if (LIBUSB_SUCCESS != rc) {
	    if(rc==LIBUSB_ERROR_ACCESS){
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("failed to open no access\n"));
	    }
	    LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("could not open USB Device\n"));
	    delete pDevice;
	    return nsInfo;
	}
	if (libusb_kernel_driver_active(pDevice->m_libusbdevHandle, 0)){
               libusb_detach_kernel_driver(pDevice->m_libusbdevHandle, 0);
	}
	pDevice->NotifyOpen();

        rc = libusb_claim_interface(pDevice->m_libusbdevHandle, 0);
	if (rc) {
                fprintf(stderr, "Failed to claim interface\n");
		delete pDevice;
		return nsInfo;
         }

	if ( pDevice != NULL )
         {
         nsInfo.pDevice = pDevice;
         nsInfo.Type = _deviceClassType;
         nsInfo.DriverLetter = _T('\0');
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
 //              FindDeviceByUsbPath(pathToFind, DeviceListType_Old, DeviceListAction_Remove);
         }

	 return nsInfo;

#endif
}

DeviceClass::NotifyStruct DeviceClass::RemoveUsbDevice(LPCTSTR path)
{

	NotifyStruct nsInfo = {0};
#if 0
	CString trimmedPath = path + 4;

	// see if it is in our list of Devices
    Device* pDevice = FindDeviceByUsbPath(trimmedPath, DeviceListType_Current, DeviceListAction_Remove);
	if(pDevice != NULL)
	{
		nsInfo.Device = pDevice;
        nsInfo.Type = _deviceClassType;
		nsInfo.DriverLetter = _T('\0');
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
#endif
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
