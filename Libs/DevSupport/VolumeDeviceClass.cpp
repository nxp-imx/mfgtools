/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "VolumeDeviceClass.h"
#include "Volume.h"
#include "KernelApi.h"

/// <summary>
/// Initializes a new instance of the VolumeDeviceClass class.
/// </summary>


VolumeDeviceClass::VolumeDeviceClass(void)
 : DeviceClass(&GUID_DEVINTERFACE_VOLUME, &/*GUID_DEVCLASS_DISKDRIVE*/GUID_DEVCLASS_VOLUME, _T("STORAGE"), DeviceTypeMsc)
{
	InitDriveLettersMap();
}

VolumeDeviceClass::~VolumeDeviceClass(void)
{
	_logicalDrives.erase(_logicalDrives.begin(), _logicalDrives.end());
}

uint32_t VolumeDeviceClass::InitDriveLettersMap()
{
	CStdString drive_letter;
	CStdString volume_path;
	DWORD driveBits = ::GetLogicalDrives();

	if (gWinVersionInfo().IsWinNT())
	{
		TCHAR ch;
		for (ch = _T('A'); ch <= _T('Z'); ++ch) {
			if (driveBits & 0x1)
			{
				drive_letter.Format(_T("%c:\\"), ch);
				if (gKernelApi().apiGetVolumeNameForVolumeMountPoint(drive_letter, volume_path))
				{
					drive_letter.Replace(_T("\\"), _T(""));
					_logicalDrives[volume_path.c_str()] = drive_letter.c_str();
				}
			}
			driveBits >>= 1;
			if ( driveBits == 0 )
				break;
		}
	}
	else
	{
		//w98 what todo in Win98?
		//w98 throw;
		ATLTRACE(_T("*** IMPLEMENT W98: Line %d of file %s\n"), __LINE__, __TFILE__);
	}

	return (uint32_t)_logicalDrives.size();

}

Device* VolumeDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	Volume* volume = new Volume(deviceClass, deviceInfoData.DevInst, path);
	
	// Only Create USB Volumes
	if ( volume->IsUsb() )
	{
		if ( _filters.empty() )
		{
			return volume;
		}
		else
		{
			// check the IDs against the filter
			if ( volume->ValidateUsbIds() )
			{
				return volume;
			}
		}
	}
	
	delete volume;
	return NULL;

}

DeviceClass::NotifyStruct VolumeDeviceClass::AddUsbDevice(LPCTSTR path)
{
	Device * pDevice = NULL;
	NotifyStruct nsInfo = {0};
    
//t	ATLTRACE2(_T("VolumeDeviceClass::AddUsbDevice()  %s\r\n"), path);

	// if it is not a Drive letter, then return
	if ( path[0] == _T('\\') )
		return nsInfo;
	
	CStdString msgLetter = path;

	assert( msgLetter.GetLength() == 1 );

	// see if it is already in our list of Volumes()
	std::list<Device*>::iterator device;
	for ( device=_devices.begin(); device != _devices.end(); ++device )
	{
		Volume* vol = dynamic_cast<Volume*>(*device);
		CStdString drvLetter = vol->_logicalDrive.get();

		if ( drvLetter.Find(msgLetter) != -1 )
		{
			pDevice = (*device);
			break;
		}
	}

	if ( pDevice == NULL )
	{
		// Refresh the VolumeName/DriveLetter map
		InitDriveLettersMap();

		// it's not in our vector of constructed volumes
		// so lets get a new list of our devices from Windows
		// and see if it is there.

		int32_t error;

		SP_DEVINFO_DATA devData;
		devData.cbSize = sizeof(SP_DEVINFO_DATA);
		CStdString devPath = _T("");

		GetClassDevs();

		for (int32_t index=0; /*no condition*/; ++index)
		{
			error = EnumDeviceInterfaceDetails(index, devPath, &devData);
			if ( error != ERROR_SUCCESS )
			{	// No match
				// Enum() will return ERROR_NO_MORE_ITEMS when done
				// but regardless, we can't add the volume
				pDevice = NULL;
				break;
			}
			else
			{
//				ATLTRACE(_T("VolumeDeviceClass::FindDeviceByUsbPath()\n"));
				pDevice = CreateDevice(this, devData, devPath);
				if ( pDevice && pDevice->IsUsb() )
				{
					Volume* vol = dynamic_cast<Volume*>(pDevice);
					CStdString drvLetter = vol->_logicalDrive.get();

					if ( drvLetter.Find(msgLetter) != -1 )
					{
						// found the new volume
						// so add it to our vector
						WaitForSingleObject(devicesMutex, INFINITE);
						_devices.push_back(pDevice);
						ReleaseMutex(devicesMutex);
//t						ATLTRACE2(_T("VolumeDeviceClass::AddUsbDevice()  Created new(%d) - %s:\r\n"), _devices.size(), msgLetter.c_str());
						break;
					}
				} // end if(UsbDevice)

				// if we got here, the volume isn't the volume we
				// are looking for, so clean up.
				if ( pDevice )
				{
					delete pDevice;
					pDevice = NULL;
				}
			}
		} // end for(all Volumes in system)
	}
	
	if ( pDevice )
	{
		nsInfo.Device = pDevice;
		nsInfo.Type = _deviceClassType;
		nsInfo.HubIndex = ((Volume*)pDevice)->StorageDisk()->_hubIndex.getmsc();
		nsInfo.Hub = ((Volume*)pDevice)->StorageDisk()->_hub.get();

		RefreshPort(nsInfo.Hub, nsInfo.HubIndex);

		for ( device=_oldDevices.begin(); device != _oldDevices.end(); ++device )
		{
			Volume* vol = dynamic_cast<Volume*>(*device);
			CStdString drvLetter = vol->_logicalDrive.get();

			if ( drvLetter.Find(msgLetter) != -1 )
			{
//t				ATLTRACE2(_T("VolumeDeviceClass::AddUsbDevice()  Found previous volume(%d): %s\r\n"), _oldDevices.size(), msgLetter.c_str());
				delete (*device);
				_oldDevices.erase(device);
				break;
			}
		}
		if ( pDevice->_description.get().IsEmpty() )
		{
			nsInfo.Device = NULL;
			ATLTRACE2(_T("ERROR! VolumeDeviceClass::AddUsbDevice() %s: Could not get Device Description from the registry.\r\n"), msgLetter.c_str());
		}
	}

	return nsInfo;
}

DeviceClass::NotifyStruct VolumeDeviceClass::RemoveUsbDevice(LPCTSTR path)
{
	NotifyStruct nsInfo = {0};

	// if it is not a Drive letter, then return
	if ( path[0] == _T('\\') )
		return nsInfo;
	
	CStdString msgLetter = path;
	
	assert( msgLetter.GetLength() == 1 );

	std::list<Device*>::iterator device;
	for ( device=_devices.begin(); device != _devices.end(); ++device )
	{
		if ( (*device)->IsUsb() )
		{
			Volume* vol = dynamic_cast<Volume*>(*device);
			CStdString drvLetter = vol->_logicalDrive.get();

			if ( drvLetter.Find(msgLetter) != -1 )
			{
				nsInfo.Device = (*device);
				nsInfo.Type = _deviceClassType;
				nsInfo.HubIndex = (*device)->_hubIndex.get();
				nsInfo.Hub = (*device)->_hub.get();

//t				ATLTRACE2(_T("VolumeDeviceClass::RemoveUsbDevice() -REMOVED- %s:\r\n"), msgLetter.c_str());

				break;
			}
		}
	}
	if ( nsInfo.Device )
	{
		ClearPort(nsInfo.Hub, nsInfo.HubIndex);

		WaitForSingleObject(devicesMutex, INFINITE);
		_oldDevices.push_back((*device));
//		delete (*device);
		_devices.erase(device);
		ReleaseMutex(devicesMutex);
	}

	return nsInfo;
}

std::list<Device*>& VolumeDeviceClass::Refresh()
{

	CStdString driveLetter;

	ATLTRACE2(_T("VolumeDeviceClass::Refresh()\r\n"));

	// Refresh the VolumeName/DriveLetter map
	InitDriveLettersMap();

	// Get a new list of our devices from Windows
	// and see if our list matches.
	std::map<CStdString, CStdString>::iterator drive;
	for ( drive=_logicalDrives.begin(); drive != _logicalDrives.end(); ++drive )
	{

		driveLetter = drive->second;
		driveLetter.Replace(_T(":"), _T(""));
		AddUsbDevice(driveLetter.c_str());
	}
/*
	std::list<Device*>::iterator device;
	for ( device=_devices.begin(); device != _devices.end(); ++device )
	{
		if ( (*device)->IsUsb() )
		{
			Volume* vol = dynamic_cast<Volume*>(*device);
			CStdString drvLetter = vol->_logicalDrive.get();

			if ( drvLetter.Find(msgLetter) != -1 )
			{
				nsInfo.Device = (*device);
				nsInfo.Type = _deviceClassType;
				nsInfo.HubIndex = (*device)->_hubIndex.get();
				nsInfo.Hub = (*device)->_hub.get();

				ATLTRACE2(_T("VolumeDeviceClass::RemoveUsbDevice() -REMOVED- %s:\r\n"), msgLetter.c_str());

				break;
			}
		}
	}
*/
	return _devices;
}

