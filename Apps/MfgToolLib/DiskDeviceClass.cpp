/*
 * Copyright 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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

Device* DiskDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path, COpState *pCurrent)
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
		Device* pDevice = CreateDevice(this, (*enumDevData).second, (*enumDevData).first, NULL);
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
