/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "MfgToolLib.h"
#include "VolumeDeviceClass.h"
#include "Volume.h"
#include "KernelApi.h"
#include "HubClass.h"
#include "UsbHub.h"
#include "DeviceManager.h"
#include "DiskDeviceClass.h"

#include "MfgToolLib_Export.h"

/// <summary>
/// Initializes a new instance of the VolumeDeviceClass class.
/// </summary>


VolumeDeviceClass::VolumeDeviceClass(INSTANCE_HANDLE handle)
 : DeviceClass(&GUID_DEVINTERFACE_VOLUME, &/*GUID_DEVCLASS_DISKDRIVE*/GUID_DEVCLASS_VOLUME, _T("STORAGE"), DeviceTypeMsc, handle)
{
	InitDriveLettersMap();
}

VolumeDeviceClass::~VolumeDeviceClass(void)
{
	_logicalDrives.erase(_logicalDrives.begin(), _logicalDrives.end());
}

UINT VolumeDeviceClass::InitDriveLettersMap()
{
	CString drive_letter;
	CString volume_path;
	DWORD driveBits = ::GetLogicalDrives();

	//Only used in Windows NT and above
	TCHAR ch;
	for (ch = _T('A'); ch <= _T('Z'); ++ch) 
	{
		if (driveBits & 0x1)
		{
			drive_letter.Format(_T("%c:\\"), ch);
			if (gKernelApi().apiGetVolumeNameForVolumeMountPoint(drive_letter, volume_path))
			{
				drive_letter.Replace(_T("\\"), _T(""));
				_logicalDrives[volume_path.GetBuffer()] = drive_letter.GetBuffer();
			}
		}
		driveBits >>= 1;
		if ( driveBits == 0 )
			break;
	}

	return (UINT)_logicalDrives.size();

}

Device* VolumeDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
	Volume* volume = new Volume(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);
	
	// Only Create USB Volumes
	if ( volume->IsUsb() )
	{
		Disk* pdisk;
		pdisk = volume->StorageDisk();
		if(pdisk == NULL)
		{
			delete volume;
			return NULL;
		}
		int portIndex = pdisk->_hubIndex.getmsc(m_msc_vid, m_msc_pid);
		CString hubPath = pdisk->_hub.get();
		if (hubPath.IsEmpty())
		{	
			delete volume;
			return NULL;
		}
		// Open the hub.
		DWORD error;
		HANDLE hHub = CreateFile(hubPath, (GENERIC_READ|GENERIC_WRITE), FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hHub == INVALID_HANDLE_VALUE) 
		{
			error = GetLastError();
			delete volume;
			return NULL;
		}
		USB_NODE_CONNECTION_INFORMATION_EX ConnectionInformation;
		ConnectionInformation.ConnectionIndex = portIndex;
		DWORD BytesReturned;
		BOOL Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, &ConnectionInformation, sizeof(ConnectionInformation),
								&ConnectionInformation, sizeof(ConnectionInformation), &BytesReturned, NULL);
		if (!Success)
		{
			error = GetLastError();
			CloseHandle(hHub);
			delete volume;
			return NULL;
		}
		if( (ConnectionInformation.DeviceDescriptor.idVendor != m_msc_vid) || 
			(ConnectionInformation.DeviceDescriptor.idProduct != m_msc_pid) )
		{
			delete volume;
			return NULL;
		}
		CloseHandle(hHub);

		return volume;
	}
	
	delete volume;
	return NULL;
}

DeviceClass::NotifyStruct VolumeDeviceClass::AddUsbDevice(LPCTSTR path)
{	//path is like 'C', 'D' and so on, only one character
	Device * pDevice = NULL;
	NotifyStruct nsInfo = {0};
    
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("VolumeDeviceClass::AddUsbDevice()  %s"), path);

	// if it is not a Drive letter, then return
	if ( path[0] == _T('\\') )
		return nsInfo;
	
	CString msgLetter = path;
	ASSERT( msgLetter.GetLength() == 1 );

	int RetryCount = 0;
	while( RetryCount < RETRY_COUNT )
	{
		// see if it is already in our list of Volumes()
		std::list<Device*>::iterator device;
		for ( device=_devices.begin(); device != _devices.end(); ++device )
		{
			Volume* vol = dynamic_cast<Volume*>(*device);
			CString drvLetter = vol->_logicalDrive.get();
			if( drvLetter.Find(msgLetter) != -1 )
			{
				pDevice = (*device);	//it is already in our list of Volumes
				break;
			}
		}
		if ( pDevice == NULL )
		{
			//it is not in our list of Volumes
			// Refresh the VolumeName/DriveLetter map
			InitDriveLettersMap();
			// it's not in our vector of constructed volumes
			// so lets get a new list of our devices from Windows
			// and see if it is there.
			int error;
			SP_DEVINFO_DATA devData;
			devData.cbSize = sizeof(SP_DEVINFO_DATA);
			CString devPath = _T("");
			GetDevInfoSet();
			for (int index=0; ; ++index)
			{
				devPath.Empty();
				error = EnumDeviceInterfaceDetails(index, devPath, &devData);
			//	if ( error != ERROR_SUCCESS )
			//	{	// No match
			//		// Enum() will return ERROR_NO_MORE_ITEMS when done
			//		// but regardless, we can't add the volume
			//		pDevice = NULL;
			//		break;
			//	}
				if( error == ERROR_NO_MORE_ITEMS )
				{
					pDevice = NULL;
					break;
				}
				else
				{
					if( !(devPath.IsEmpty()) )
					{
						pDevice = CreateDevice(this, devData, devPath);
						if ( pDevice && pDevice->IsUsb() )
						{
							Volume* vol = dynamic_cast<Volume*>(pDevice);
							CString drvLetter = vol->_logicalDrive.get();

							if ( drvLetter.Find(msgLetter) != -1 )
							{
								// found the new volume
								// so add it to our vector
								WaitForSingleObject(devicesMutex, INFINITE);
								pDevice->_hubIndex.get();
								_devices.push_back(pDevice);
								ReleaseMutex(devicesMutex);
								//TraceStr.Format(_T("VolumeDeviceClass::AddUsbDevice()  Created new(%d) - %s:\r\n"), _devices.size(), msgLetter.c_str());
								break;
							}
						}
						// if we got here, the volume isn't the volume we
						// are looking for, so clean up.
						if ( pDevice )
						{
							delete pDevice;
							pDevice = NULL;
						}
					}
				}
			} // end for(all Volumes in system)
			DestroyDevInfoSet();
			if(pDevice != NULL)
			{
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

/*	// see if it is already in our list of Volumes()
	std::list<Device*>::iterator device;
	for ( device=_devices.begin(); device != _devices.end(); ++device )
	{
		Volume* vol = dynamic_cast<Volume*>(*device);
		CString drvLetter = vol->_logicalDrive.get();

		if ( drvLetter.Find(msgLetter) != -1 )
		{
			pDevice = (*device);	//it is already in our list of Volumes
			break;
		}
	}

	int RetryCount = 0;
	while( RetryCount < RETRY_COUNT )
	{
		//it is not in our list of Volumes
		if ( pDevice == NULL )
		{
			// Refresh the VolumeName/DriveLetter map
			InitDriveLettersMap();
			// it's not in our vector of constructed volumes
			// so lets get a new list of our devices from Windows
			// and see if it is there.
			int error;
			SP_DEVINFO_DATA devData;
			devData.cbSize = sizeof(SP_DEVINFO_DATA);
			CString devPath = _T("");

			GetDevInfoSet();
			for (int index=0; ; ++index)
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
					pDevice = CreateDevice(this, devData, devPath);
					if ( pDevice && pDevice->IsUsb() )
					{
						Volume* vol = dynamic_cast<Volume*>(pDevice);
						CString drvLetter = vol->_logicalDrive.get();

						if ( drvLetter.Find(msgLetter) != -1 )
						{
							// found the new volume
							// so add it to our vector
							WaitForSingleObject(devicesMutex, INFINITE);
							_devices.push_back(pDevice);
							ReleaseMutex(devicesMutex);
							//TraceStr.Format(_T("VolumeDeviceClass::AddUsbDevice()  Created new(%d) - %s:\r\n"), _devices.size(), msgLetter.c_str());
							//LOGPRINT(TraceStr);
							break;
						}
					}

					// if we got here, the volume isn't the volume we
					// are looking for, so clean up.
					if ( pDevice )
					{
						delete pDevice;
						pDevice = NULL;
					}
				}
			} // end for(all Volumes in system)
			DestroyDevInfoSet();
			RetryCount++;
		}
		else
		{
			break;
		}
	} */
	if(RetryCount >= RETRY_COUNT)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("VolumeDeviceClass::AddUsbDevice() failed %s"), path);
	}
	else
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("VolumeDeviceClass::AddUsbDevice() successful %s add to current list, retrycount: %d"), path, RetryCount);
	}

//	if(pDevice)
//	{
//		(dynamic_cast<DiskDeviceClass *>(g_devClasses[DeviceClass::DeviceTypeDisk]))->Refresh();
//	}

	if ( pDevice && pDevice->UsbDevice() )
	{
		//TraceStr.Format(_T("It is a USB device.\r\n"));
		//LOGPRINT(TraceStr);
		nsInfo.Device = pDevice;
		nsInfo.Type = _deviceClassType;
		nsInfo.DriverLetter = path[0];
		nsInfo.PortIndex = ((Volume*)pDevice)->StorageDisk()->_hubIndex.getmsc(m_msc_vid, m_msc_pid);
		nsInfo.Hub = ((Volume*)pDevice)->StorageDisk()->_hub.get();
		// Find our Hub in gDeviceManager's list of [Hub].Devices()
		usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
		usb::Hub *pHub = pHubClass->FindHubByPath(nsInfo.Hub);
		if(pHub != NULL)
		{
			nsInfo.HubIndex = pHub->_index.get();
		}

		RefreshPort(nsInfo.Hub, nsInfo.PortIndex);

/*		for ( device=_oldDevices.begin(); device != _oldDevices.end(); ++device )
		{
			Volume* vol = dynamic_cast<Volume*>(*device);
			CString drvLetter = vol->_logicalDrive.get();

			if ( drvLetter.Find(msgLetter) != -1 )
			{
				//TraceStr.Format(_T("VolumeDeviceClass::AddUsbDevice()  Found previous volume(%d): %s\r\n"), _oldDevices.size(), msgLetter.c_str());
				//LOGPRINT(TraceStr);
				delete (*device);
				_oldDevices.erase(device);
				break;
			}
		} */
		if ( pDevice->_description.get().IsEmpty() )
		{
			nsInfo.Device = NULL;
			//TraceStr.Format(_T("ERROR! VolumeDeviceClass::AddUsbDevice() %s: Could not get Device Description from the registry.\r\n"), msgLetter.c_str());
			//LOGPRINT(TraceStr);
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
	
	CString msgLetter = path;
	
	ASSERT( msgLetter.GetLength() == 1 );

	std::list<Device*>::iterator device;
	for ( device=_devices.begin(); device != _devices.end(); ++device )
	{
		if ( (*device)->IsUsb() )
		{
			Volume* vol = dynamic_cast<Volume*>(*device);
			CString drvLetter = vol->_logicalDrive.get();

			if ( drvLetter.Find(msgLetter) != -1 )
			{
				nsInfo.Device = (*device);
				nsInfo.Type = _deviceClassType;
				nsInfo.DriverLetter = path[0];
//				nsInfo.PortIndex = (*device)->_hubIndex.get();//((Volume*)(*device))->StorageDisk()->_hubIndex.getmsc(m_msc_vid, m_msc_pid);
				nsInfo.PortIndex = ((Volume*)(*device))->StorageDisk()->_hubIndex.getmsc(m_msc_vid, m_msc_pid);
//				if(nsInfo.PortIndex == 0)
//					nsInfo.PortIndex = ((Volume*)(*device))->StorageDisk()->_hubIndex.getmsc(m_msc_vid, m_msc_pid);
				
//				if(nsInfo.PortIndex == 0)
//				{
//					Disk *pdisk = ((Volume*)(*device))->StorageDisk();
					//TRACE(_T("Disk[%p]"), pdisk);
//					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("first PortIndex is 0, so Disk[%p]"), pdisk);
//					nsInfo.PortIndex = pdisk->_hubIndex.getmsc(m_msc_vid, m_msc_pid);
//				}
				
				nsInfo.Hub = (*device)->_hub.get();
				// Find our Hub in gDeviceManager's list of [Hub].Devices()
				usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
				usb::Hub *pHub = pHubClass->FindHubByPath(nsInfo.Hub);
				if(pHub != NULL)
				{
					nsInfo.HubIndex = pHub->_index.get();
				}
				break;
			}
		}
	}
	if ( nsInfo.Device )
	{
		ClearPort(nsInfo.Hub, nsInfo.PortIndex);

/*		WaitForSingleObject(devicesMutex, INFINITE);
		_oldDevices.push_back((*device));
//		delete (*device);
		_devices.erase(device);
		ReleaseMutex(devicesMutex); */
	}

	return nsInfo;
}

std::list<Device*>& VolumeDeviceClass::Refresh()
{
	CString driveLetter;

	// Refresh the VolumeName/DriveLetter map
	InitDriveLettersMap();

	// Get a new list of our devices from Windows
	// and see if our list matches.
	std::map<CString, CString>::iterator drive;
	for ( drive=_logicalDrives.begin(); drive != _logicalDrives.end(); ++drive )
	{
		driveLetter = drive->second;
		driveLetter.Replace(_T(":"), _T(""));
		AddUsbDevice(driveLetter);
	}
/*
	std::list<Device*>::iterator device;
	for ( device=_devices.begin(); device != _devices.end(); ++device )
	{
		if ( (*device)->IsUsb() )
		{
			Volume* vol = dynamic_cast<Volume*>(*device);
			CString drvLetter = vol->_logicalDrive.get();

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
