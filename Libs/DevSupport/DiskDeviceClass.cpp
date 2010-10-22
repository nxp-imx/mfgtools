/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "DiskDeviceClass.h"
#include "Disk.h"

/// <summary>
/// Initializes a new instance of the DiskDeviceClass class.
/// </summary>
DiskDeviceClass::DiskDeviceClass(void)
: DeviceClass(&GUID_DEVINTERFACE_DISK, &GUID_DEVCLASS_DISKDRIVE, _T(""), DeviceTypeDisk)
{
}

DiskDeviceClass::~DiskDeviceClass(void)
{
}

Device* DiskDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	Disk* disk = new Disk(deviceClass, deviceInfoData.DevInst, path);
	
	// Only Create USB Disks
	if ( disk->IsUsb() )
	{
		if ( _filters.empty() )
		{
			return disk;
		}
		else
		{
			// check the IDs against the filter
			if ( disk->ValidateUsbIds() )
			{
				return disk;
			}
		}
	}
	
	delete disk;
	return NULL;
}
/*
DeviceClass::NotifyStruct DiskDeviceClass::AddUsbDevice(LPCTSTR path)
{
	NotifyStruct nsInfo = {0};
    Device * pDevice = NULL;
	CStdString pathToFind = path + 4;

	ATLTRACE(_T("%s::AddUsbDevice()  %s\r\n"), this->ToString().c_str(), path);

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

		ATLTRACE(_T("%s::AddUsbDevice()  Found device. PORT: %d HUB: %s\r\n"), this->ToString().c_str(), nsInfo.HubIndex, nsInfo.Hub.c_str());
		
		// Delete device from old device list since it's in our current list
		FindDeviceByUsbPath(pathToFind, DeviceListType_Old, DeviceListAction_Remove);

		RefreshPort(nsInfo.Hub, nsInfo.HubIndex);
	}

	ATLTRACE(_T("%s::AddUsbDevice()  %s device.\r\n"), this->ToString().c_str(), pDevice ? _T("FOUND") : _T("DID NOT FIND"));

	return nsInfo;
}

DeviceClass::NotifyStruct DiskDeviceClass::RemoveUsbDevice(LPCTSTR path)
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
*/
std::list<Device*>& DiskDeviceClass::Refresh()
{
	SP_DEVINFO_DATA enumData;
	enumData.cbSize = sizeof(SP_DEVINFO_DATA);
	CStdString enumPath = _T("");
	std::map<CStdString, SP_DEVINFO_DATA> enumDevInfo;

	GetClassDevs();

	for (int32_t index=0; /*no condition*/; ++index)
	{
		if ( EnumDeviceInterfaceDetails(index, enumPath, &enumData) != ERROR_SUCCESS )
			break; // Enum() will return ERROR_NO_MORE_ITEMS when done.

		enumPath.Replace(_T("\\??"), _T("\\\\."));

		enumDevInfo[enumPath] = enumData;

	} // end for(all Disks in system)

	std::list<Device*>::iterator myDev;
	std::list<std::list<Device*>::iterator> removeList;
	std::map<CStdString, SP_DEVINFO_DATA>::iterator enumDevData;

	// loop through my list of disks
	for ( myDev = _devices.begin(); myDev != _devices.end(); ++myDev )
	{
		enumDevData = enumDevInfo.find((*myDev)->_path.get());
		if ( enumDevData != enumDevInfo.end() )
		{
			// myDev is currently present in the system,
			// take it out of the enum list and check my next dev.
			enumDevInfo.erase(enumDevData);
		}
		else
		{
			// my dev is not present in the system and needs to be removed from my dev list.
			removeList.push_back(myDev);
		}
	}

	// remove my devices that are not in the system
	std::list<Device*>::iterator removeDev;
	while ( removeList.size() > 0 )
	{
		removeDev = removeList.back();
		removeList.pop_back();
		delete *removeDev;
		_devices.erase(removeDev);
	}

	// if there are any devices in the system that are not in my list, create them
	for ( enumDevData = enumDevInfo.begin(); enumDevData != enumDevInfo.end(); ++enumDevData )
	{
		Device* pDevice = CreateDevice(this, (*enumDevData).second, (*enumDevData).first);
		if ( pDevice && pDevice->IsUsb() )
		{
			WaitForSingleObject(devicesMutex, INFINITE);
			_devices.push_back(pDevice);
			ReleaseMutex(devicesMutex);
		}
	}

	return _devices;
}
