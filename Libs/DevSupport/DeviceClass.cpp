#include "stdafx.h"
#include "DeviceClass.h"
#include "UsbHubMgr.h"
#include "DeviceManager.h"

extern HANDLE g_ConfigActiveEvent;

DeviceClass::DeviceClass(LPCGUID iFaceGuid, LPCGUID devGuid, LPCTSTR enumerator, DeviceType type)
: Property(true)
, _deviceInfoSet(INVALID_HANDLE_VALUE)
, _deviceClassType(type)
{
	if (iFaceGuid)
		_classIfaceGuid.put(*iFaceGuid);
	if (devGuid)
		_classDevGuid.put(*devGuid);
	if (enumerator)
		_enumerator.put(enumerator);

	memset(&_imageListData, 0, sizeof(_imageListData));

	_classDesc.describe(this, _T("Class Description"), _T(""));
	_classIfaceGuid.describe(this, _T("Device Interface GUID"), _T("Describes the device class interface."));
	_classDevGuid.describe(this, _T("Device Class GUID"), _T("Describes the device class."));
	_classIconIndex.describe(this, _T("Icon"), _T("Index to the system icon."));
	_enumerator.describe(this, _T("Enumerator"), _T(""));
	_classIconIndex.put(-1);

	_deviceInfoSet = GetClassDevs();

	devicesMutex = CreateMutex(NULL, FALSE, NULL);
}

DeviceClass::~DeviceClass(void)
{
	_classIconIndex.put(-1);

	WaitForSingleObject(devicesMutex, INFINITE);
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

	if ( _imageListData.cbSize != 0 )
	{
		gSetupApi().SetupDiDestroyClassImageList(&_imageListData);
		memset(&_imageListData, 0, sizeof(_imageListData));
	}
	
	_propertyList.clear();
}

HDEVINFO DeviceClass::GetClassDevs()
{
	// reset the list if it exists
	if ( _deviceInfoSet != INVALID_HANDLE_VALUE )
	{
		gSetupApi().SetupDiDestroyDeviceInfoList(_deviceInfoSet);
		_deviceInfoSet = INVALID_HANDLE_VALUE;
	}

	// decide the criteria for the devices
	if ( *_classIfaceGuid.get() != GUID_NULL /*&& gWinVersionInfo().IsWinNT()*/ )
	{
		_deviceInfoSet = gSetupApi().apiSetupDiGetClassDevs(_classIfaceGuid.get(), NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	}
	else
	{
		DWORD flags = 0;
		LPCGUID pGuid;
		CStdString enumerator;
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
		
		_deviceInfoSet = gSetupApi().apiSetupDiGetClassDevs(pGuid, enumerator.empty() ? NULL : enumerator.GetBuffer(), 0, flags);
	}
	
	return _deviceInfoSet;
}

int32_t DeviceClass::EnumDeviceInterfaceDetails(int32_t index, CStdString& devPath, PSP_DEVINFO_DATA pDevData)
{
	uint32_t error;
	SP_DEVICE_INTERFACE_DATA interfaceData;
	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	if (!gSetupApi().SetupDiEnumDeviceInterfaces(_deviceInfoSet, NULL, _classIfaceGuid.get(), index, &interfaceData))
	{
		error = GetLastError();
		if (error == ERROR_NO_MORE_ITEMS)
			return error;
		else
		{
			ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
			throw;
		}
	}

	DWORD requiredSize = 0;
	if (!gSetupApi().apiSetupDiGetDeviceInterfaceDetail(_deviceInfoSet, &interfaceData, NULL, 0, &requiredSize, NULL))
	{
		error = GetLastError();
		if (error != ERROR_INSUFFICIENT_BUFFER)
		{
			ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
			throw;
		}
	}

	PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
	detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	if (!gSetupApi().apiSetupDiGetDeviceInterfaceDetail(_deviceInfoSet, &interfaceData, detailData, requiredSize, NULL, pDevData))
	{
		free(detailData);
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
		throw;
	}

	devPath = (PTSTR)detailData->DevicePath;
	free(detailData);

	return ERROR_SUCCESS;
}

Device* DeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	Device * dev = new Device(deviceClass, deviceInfoData.DevInst, path);
	if ( dev->IsUsb() )
	{
		if ( dev->ValidateUsbIds() )
		{
			return dev;
		}
		else
		{
			delete dev;
			return NULL;
		}
	}
	else
		return dev;
}

CStdString DeviceClass::classDesc::get()
{
	DWORD error;
	if ( _value.IsEmpty() )
	{
		DeviceClass* devClass = dynamic_cast<DeviceClass*>(_owner);
		ASSERT(devClass);

		TCHAR buffer[MAX_PATH];
		DWORD RequiredSize;
		if (gSetupApi().apiSetupDiGetClassDescription(devClass->_classDevGuid.get(), (PTSTR)buffer, MAX_PATH, &RequiredSize))
			_value = buffer;
		else
			error = GetLastError();
	}
	return _value;
}

PSP_CLASSIMAGELIST_DATA DeviceClass::ImageListPtr()
{
	if ( _imageListData.cbSize == 0 )
	{
		_imageListData.cbSize = sizeof(_imageListData);
        gSetupApi().SetupDiGetClassImageList(&_imageListData);
	}

	return &_imageListData;
}

HIMAGELIST DeviceClass::ImageList()
{
	return ImageListPtr()->ImageList;
}

int32_t DeviceClass::classIconIndex::get()
{
	DWORD error;
	if ( Value == -1 )
	{
		DeviceClass* devClass = dynamic_cast<DeviceClass*>(_owner);
		ASSERT(devClass);

		if (!gSetupApi().SetupDiGetClassImageIndex(devClass->ImageListPtr(), devClass->_classDevGuid.get(), (PINT)&Value))
			error = GetLastError();
	}
	ASSERT(Value != -1);
	return Value;
}

size_t DeviceClass::SetFilters(StdStringArray filters)
{
	_filters = filters;
	return _filters.size();
}

StdStringArray& DeviceClass::GetFilters()
{
	return _filters;
}

size_t DeviceClass::ClearFilters()
{
	size_t size = _filters.size();
	_filters.erase(_filters.begin(), _filters.end());
	return size;
}

size_t DeviceClass::AddFilter(uint16_t vid, uint16_t pid)
{
	CStdString filter;
	filter.Format(_T("vid_%04x&pid_%04x"), vid, pid);
	_filters.push_back(filter);
	return _filters.size();
}

size_t DeviceClass::AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance)
{
	CStdString filter;
	filter.Format(_T("vid_%s"), vid, pid, instance);
    if ( pid )
    {
        filter.AppendFormat(_T("&pid_%s"), vid);
        if ( instance )
        {
            filter.AppendFormat(_T("#%s"), instance);
        }
    }
   
	_filters.push_back(filter);
	return _filters.size();
}

/// <summary>
/// Gets the list of devices of this device class.
/// </summary>
std::list<Device*>& DeviceClass::Devices()
{
    uint32_t error;

	if ( _devices.empty() )
    {
		int32_t index = 0;
        while (true)
        {
			GetClassDevs();
			
			SP_DEVINFO_DATA devData;
			devData.cbSize = sizeof(SP_DEVINFO_DATA);
			CStdString devPath = _T("");

			if ( *_classIfaceGuid.get() != GUID_NULL /*&&
				 gWinVersionInfo().IsWinNT()*/ )
			{
				error = EnumDeviceInterfaceDetails(index, devPath, &devData);

				if ( error == ERROR_NO_MORE_ITEMS )
					break;
			}
			else
			{
				if (!gSetupApi().SetupDiEnumDeviceInfo(_deviceInfoSet, index, &devData))
				{
					error = GetLastError();
					if (error != ERROR_NO_MORE_ITEMS)
					{
						ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
						throw;
					}
					break;
				}
			}

//			ATLTRACE(_T("DeviceClass::Devices()\n"));
			Device* device = CreateDevice(this, devData, devPath);
			// Create device will return NULL if there are filters in place
			// and the device does not match a filter
			if ( device != NULL )
			{
				// init the hub and hub index fields, because if we wait till the 
				// device is gone, it will be too late.
				device->_hubIndex.get();
				_devices.push_back(device);
			}

            index++;
        }
    }

    return _devices;
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

Device* DeviceClass::FindDeviceByUsbPath(CStdString pathToFind, const DeviceListType devListType, const DeviceListAction devListAction )
{

	if (pathToFind.IsEmpty())
    {
        return NULL;
    }

    Device * pDevice = NULL;

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
					if ( pathToFind.CompareNoCase( (*device)->_usbPath.get() ) == 0 )
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
					if ( pathToFind.CompareNoCase( (*device)->_usbPath.get() ) == 0 )
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

			int32_t error;

			SP_DEVINFO_DATA devData;
			devData.cbSize = sizeof(SP_DEVINFO_DATA);
			CStdString devPath = _T("");

			GetClassDevs();
			
			for (int32_t index=0; /*no condition*/; ++index)
			{
				if ( *_classIfaceGuid.get() != GUID_NULL /*&&
						gWinVersionInfo().IsWinNT()*/ )
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

//				ATLTRACE(_T("DeviceClass::FindDeviceByUsbPath()\n"));
				pDevice = CreateDevice(this, devData, devPath);
				if ( pDevice && pDevice->IsUsb() )
				{
					if ( pathToFind.CompareNoCase( pDevice->_usbPath.get() ) == 0 )
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
}

DeviceClass::NotifyStruct DeviceClass::AddUsbDevice(LPCTSTR path)
{
	NotifyStruct nsInfo = {0};
    Device * pDevice = NULL;
	CStdString pathToFind = path + 4;

//t	ATLTRACE2(_T("%s::AddUsbDevice()  %s\r\n"), this->ToString().c_str(), path);

    // see if it is already in our list of Devices()
    pDevice = FindDeviceByUsbPath(pathToFind, DeviceListType_Current, DeviceListAction_None);

    if ( pDevice == NULL )
    {
        // it's not in our Collection of constructed devices
        // so lets get a new list of our devices from Windows
        // and see if it is there.
        pDevice = FindDeviceByUsbPath(pathToFind, DeviceListType_New, DeviceListAction_Add);
    }

	if ( pDevice )
	{
		nsInfo.Device = pDevice;
		nsInfo.Type = _deviceClassType;
		nsInfo.HubIndex = pDevice->_hubIndex.get();
		nsInfo.Hub = pDevice->_hub.get();

		// Delete device from old device list since it's in our current list
		FindDeviceByUsbPath(pathToFind, DeviceListType_Old, DeviceListAction_Remove);

		RefreshPort(nsInfo.Hub, nsInfo.HubIndex);
	}

	return nsInfo;
}

DeviceClass::NotifyStruct DeviceClass::RemoveUsbDevice(LPCTSTR path)
{
	NotifyStruct nsInfo = {0};

	CStdString trimmedPath = path + 4;

	// see if it is in our list of Devices
    Device* pDevice = FindDeviceByUsbPath(trimmedPath, DeviceListType_Current, DeviceListAction_Remove);

	if ( pDevice )
	{
		nsInfo.Device = pDevice;
		nsInfo.Type = _deviceClassType;
		nsInfo.HubIndex = pDevice->_hubIndex.get();
		nsInfo.Hub = pDevice->_hub.get();

		ClearPort(nsInfo.Hub, nsInfo.HubIndex);

//t		ATLTRACE2(_T("%s::RemoveUsbDevice() - %s\r\n"), this->ToString().c_str(), path);
	}

	return nsInfo;
}

int32_t DeviceClass::RefreshPort(const CStdString hubPath, const int32_t hubIndex)
{
//	if (WaitForSingleObject( g_ConfigActiveEvent, 0) == WAIT_TIMEOUT)
//		return ERROR_SUCCESS;

	// Find our Hub in gDeviceManager's list of [Hub].Devices()
	usb::HubMgr* pHubManager = dynamic_cast<usb::HubMgr*>(gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbHub]);
	
	assert(pHubManager);
	
	usb::Hub* pHub = pHubManager->FindHubByPath(hubPath);

	if ( pHub )
	{
		return pHub->RefreshPort(hubIndex);
	}
	else
	{
		return ERROR_FILE_NOT_FOUND;
	}

}

int32_t DeviceClass::ClearPort(const CStdString hubPath, const int32_t hubIndex)
{
	// Find our Hub in gDeviceManager's list of [Hub].Devices()
	usb::HubMgr* pHubManager = dynamic_cast<usb::HubMgr*>(gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbHub]);
	
	assert(pHubManager);
	
	usb::Hub* pHub = pHubManager->FindHubByPath(hubPath);

	if ( pHub )
	{
		return pHub->ClearPort(hubIndex);
	}
	else
	{
		return ERROR_FILE_NOT_FOUND;
	}

}
