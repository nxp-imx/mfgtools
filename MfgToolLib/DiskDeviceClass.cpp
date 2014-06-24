/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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
DiskDeviceClass::DiskDeviceClass(INSTANCE_HANDLE handle)
: DeviceClass(&GUID_DEVINTERFACE_DISK, &GUID_DEVCLASS_DISKDRIVE, _T(""), DeviceTypeDisk, handle)
{
	//The system-supplied storage class drivers register an instance of GUID_DEVINTERFACE_DISK for a hard disk storage device. 
}

DiskDeviceClass::~DiskDeviceClass(void)
{
}

Device* DiskDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
	Disk* disk = new Disk(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);
	
	// Only Create USB Disks
	if ( disk->IsUsb() )
	{
		return disk;
	}
	
	delete disk;
	
	return NULL;
}

std::list<Device*>& DiskDeviceClass::Refresh()
{
	SP_DEVINFO_DATA enumData;
	enumData.cbSize = sizeof(SP_DEVINFO_DATA);
	CString enumPath = _T("");
	std::map<CString, SP_DEVINFO_DATA> enumDevInfo;

	//remove _devices first
//	std::list<Device*>::iterator removeDev;
//	for ( removeDev = _devices.begin(); removeDev != _devices.end(); ++removeDev )
//	{
//		delete *removeDev;
//		_devices.erase(removeDev);
//	}

	GetDevInfoSet();
	for (int index=0; /*no condition*/; ++index)
	{
		if ( EnumDeviceInterfaceDetails(index, enumPath, &enumData) != ERROR_SUCCESS )
			break; // Enum() will return ERROR_NO_MORE_ITEMS when done.

		enumPath.Replace(_T("\\??"), _T("\\\\."));
		enumPath.Replace(_T("\\\\?"), _T("\\\\."));

		enumDevInfo[enumPath] = enumData;
	} // end for(all Disks in system)
	DestroyDevInfoSet();
/*
	std::list<Device*>::iterator myDev;
	std::list<std::list<Device*>::iterator> removeList;
	std::map<CString, SP_DEVINFO_DATA>::iterator enumDevData;

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
*/
	std::list<Device*>::iterator myDev;
	std::map<CString, SP_DEVINFO_DATA>::iterator enumDevData;
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
	}


	// if there are any devices in the system that are not in my list, create them
//	std::map<CString, SP_DEVINFO_DATA>::iterator enumDevData;
	for ( enumDevData = enumDevInfo.begin(); enumDevData != enumDevInfo.end(); ++enumDevData )
	{
		Device* pDevice = CreateDevice(this, (*enumDevData).second, (*enumDevData).first);
		if ( pDevice && pDevice->IsUsb() )
		{
			WaitForSingleObject(devicesMutex, INFINITE);
			pDevice->_hubIndex.get();
			_devices.push_back(pDevice);
			ReleaseMutex(devicesMutex);
		}
	}

	return _devices;
}
