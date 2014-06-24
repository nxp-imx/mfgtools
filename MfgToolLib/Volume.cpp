/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "Volume.h"
#include "VolumeDeviceClass.h"
#include "KernelApi.h"
#include "DeviceManager.h"

#include "DiskDeviceClass.h"

#include "MfgToolLib_Export.h"

#include "UpdateTransportProtocol.Api.h"
#include "UpdateUIInfo.h"

Volume::Volume(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
: Device(deviceClass, devInst, path, handle)
, _diskNumber(-1)
, _Disk(NULL)
{
	_volumeName.describe(this, _T("Volume Path"), _T(""));
	_logicalDrive.describe(this, _T("Drive Letter"), _T(""));
	_friendlyName.describe(this, _T("Friendly Name"), _T(""));
	_diskNumber.describe(this, _T("Physical Disk Number"), _T(""));

	_diskNumber.get();

    _hEvent = CreateEvent( 
		NULL,    // default security attribute 
        TRUE,    // manual-reset event 
        FALSE,    // initial state = not-signaled 
		_logicalDrive.get().GetBuffer());   // unnamed event object 

	m_pBuffer = (UCHAR *)malloc(sizeof(_NT_SCSI_REQUEST) + MAX_SCSI_DATA_TRANSFER_SIZE);

	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("new Volume[%p]"), this);
}

Volume::~Volume(void)
{
	if ( _hEvent != NULL )
	{
		CloseHandle(_hEvent);
		_hEvent = NULL;
	}
	free(m_pBuffer);
	m_pBuffer = NULL;

	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("delete Volume[%p]"), this);
}

/// <summary>
/// Gets the volume's name.
/// </summary>
CString Volume::volumeName::get()
{
	Volume* vol = dynamic_cast<Volume*>(_owner);
	ASSERT(vol);

	DWORD error;

	if (_value.IsEmpty())
    {
		if (!gKernelApi().apiGetVolumeNameForVolumeMountPoint(vol->_path.get() + _T("\\"), _value))
		{
			error = GetLastError();
		}
    }
    return _value;
}

/// <summary>
/// Gets the volume's logical drive in the form [letter]:\
/// </summary>
CString Volume::logicalDrive::get()
{
	Volume* vol = dynamic_cast<Volume*>(_owner);
	ASSERT(vol);

	if ((_value.IsEmpty()) && (!vol->_volumeName.get().IsEmpty()))
    {
		std::map<CString, CString>::iterator item;
		item = ((VolumeDeviceClass*)vol->_deviceClass)->_logicalDrives.find(vol->_volumeName.get());
		if ( item != ((VolumeDeviceClass*)vol->_deviceClass)->_logicalDrives.end() )
			_value = item->second;
    }
    return _value;
}

/// <summary>
/// Property: Gets the volume's friendly name from the 1st Disk device.
/// </summary>
CString Volume::friendlyName::get()
{
	if ( _value.IsEmpty() )
	{
		// get the friendly name from the Device object as a default
		Volume* vol = dynamic_cast<Volume*>(_owner);
		ASSERT(vol);

		// now try and get the friendly name from our parent USBSTOR node
		if ( vol->StorageDisk() )
		{
			_value = vol->StorageDisk()->_friendlyName.get();
		}
	}
	return _value;
}

/// <summary>
/// Property: Gets the volume's Physical Disk Number.
/// </summary>
int Volume::diskNumber::get()
{
	if ( Value == -1 )
	{
		Volume* vol = dynamic_cast<Volume*>(_owner);
		ASSERT(vol);

		if (!vol->_logicalDrive.get().IsEmpty())
        {
			CString str;
			str.Format(_T("\\\\.\\%s"), vol->_logicalDrive.get());
			HANDLE hFile = CreateFile(str, 0, 0, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				DWORD error = GetLastError();
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("*** Error: %d, Drive: %s\n"), error, vol->_logicalDrive.get());
			}
			
			DWORD bytesReturned = 0;
			STORAGE_DEVICE_NUMBER driveNumber;
			if (!DeviceIoControl(hFile, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &driveNumber, sizeof(driveNumber), &bytesReturned, NULL))
			{
				// do nothing here on purpose
				DWORD error = GetLastError();
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("*** Error: %d, Drive: %s\n"), error, vol->_logicalDrive.get());
			}
			CloseHandle(hFile);

			if (bytesReturned > 0)
			{
				Value = driveNumber.DeviceNumber;
			}
		}
    }
	return Value;
}

/// <summary>
/// Gets a value indicating whether this volume is a based on USB devices.
/// </summary>

bool Volume::IsUsb()
{
	CString path = _path.get();
	path.MakeUpper();
	if ( path.Find(_T("REMOVABLEMEDIA")) != -1 )
		return true;
	else if ( path.Find(_T("USBSTOR")) != -1 )
		return true;
	else
		return false;
}

/// <summary>
/// Gets the device connected to the USB bus.
/// </summary>
Device* Volume::UsbDevice()
{
	Disk* pDisk = StorageDisk();

	if(pDisk)
		return pDisk->UsbDevice();
	else 
		return NULL;
}

/// <summary>
/// Gets a the underlying disk for this volume.
/// </summary>
Disk* Volume::StorageDisk()
{
	if ( _Disk == NULL )
	{
//		std::list<Device*> disks = (dynamic_cast<DiskDeviceClass *>(g_devClasses[DeviceClass::DeviceTypeDisk]))->Refresh();
		std::list<Device*> disks = (dynamic_cast<DiskDeviceClass *>(g_devClasses[DeviceClass::DeviceTypeDisk]))->_devices;
		std::list<Device*>::iterator device;
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceTypeDisk--_devices size: %d"), disks.size());
		for(device = disks.begin(); device !=disks.end(); ++device)
		{
			Disk* pDisk = dynamic_cast<Disk*>(*device);
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Volume--StorageDisk, Disk->_driveNumber:%d, Volume->_diskNumber:%d"), pDisk->_driveNumber.get(), _diskNumber.get());

			if ( pDisk->_driveNumber.get() == _diskNumber.get() )
			{
				_Disk = pDisk;
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceTypeDisk--find Disk device: %p"), _Disk);
				break;
			}
		}
	}
	return _Disk;
}

#define LOCK_TIMEOUT        1000       // 1 Second
#define LOCK_RETRIES        10

// Locks a volume. A locked volume can be accessed only through handles to the file object that locks the volume.

HANDLE Volume::Lock(LockType lockType)
{
//	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
//	int nTryCount;

	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;

    CString disk;
    if ( lockType == LockType_Physical )
	{
		if ( _diskNumber.get() != -1 )
		{
			disk.Format(_T("\\\\.\\PhysicalDrive%d"), _diskNumber.get());
		}
		else
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("*** Volume::Lock - FAILED")); 
			return INVALID_HANDLE_VALUE;
		}
    }
    else
    {
        disk = _path.get();
    }

	HANDLE hDrive = ::CreateFile (
		disk,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

//   if ( hDrive == INVALID_HANDLE_VALUE )
        return hDrive;

	// Do this in a loop until a timeout period has expired
/*	for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
	{
        if (::DeviceIoControl(
				hDrive,
				FSCTL_LOCK_VOLUME,
				NULL, 0,
				NULL, 0,
				&dwBytesReturned,
				NULL 
			) )
		{
//			TRACE(_T("****Lock took %d tries\n"), nTryCount); 
			return hDrive;
		}
        DWORD err = GetLastError();
		Sleep( dwSleepAmount );
	}

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("*** Volume::Lock - FAILED")); 
    return INVALID_HANDLE_VALUE; */
}

// Unlocks a volume.
int Volume::Unlock(HANDLE hDrive, bool close)
{
	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
	int nTryCount;

	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;

    // Do this in a loop until a timeout period has expired
	for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
	{
        if (::DeviceIoControl(
				hDrive,
				FSCTL_UNLOCK_VOLUME,
				NULL, 0,
				NULL, 0,
				&dwBytesReturned,
				NULL 
			) )
		{		
	        if (close)
				CloseHandle(hDrive);
			return ERROR_SUCCESS;
		}
		Sleep( dwSleepAmount );
	}

    if (close)
		CloseHandle(hDrive);

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("*** Volume::Unlock - FAILED")); 

	return GetLastError();
}

UINT Volume::SendCommand(StApi& api, UCHAR* additionalInfo)
{
/*
	// If it is not a SCSI Api, return error.
	if ( api.GetType() != API_TYPE_ST_SCSI && api.GetType() != API_TYPE_SCSI )
		return ERROR_INVALID_PARAMETER;
*/
    // tell the UI we are beginning a command.
    NotifyStruct nsInfo(api.GetName(), api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice, 0);
//    nsInfo.direction = api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice;
//    Notify(nsInfo);

    HANDLE hDrive = ::CreateFile (
		_path.get()/*.c_str()*/, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0/* FILE_FLAG_OVERLAPPED */,
		NULL);

	if ( hDrive == INVALID_HANDLE_VALUE )
		return GetLastError();

    UINT ret = SendCommand(hDrive, api, additionalInfo, nsInfo);

    CloseHandle(hDrive);

    // tell the UI we are done
//    nsInfo.inProgress = false;
//    Notify(nsInfo);

    return ret;
}

UINT Volume::SendCommand(HANDLE hDrive, StApi& api, UCHAR* additionalInfo, NotifyStruct& nsInfo)
{
	
	// init parameter if it is used
	if (additionalInfo)
		*additionalInfo = SCSISTAT_GOOD;

	// Allocate the SCSI request
	DWORD totalSize = sizeof(_NT_SCSI_REQUEST) + api.GetTransferSize();
	_NT_SCSI_REQUEST* pRequest = (_NT_SCSI_REQUEST*)m_pBuffer;
    if ( pRequest == NULL )
    {
        nsInfo.inProgress = false;
//        Notify(nsInfo);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

	//
	// Set up structure for DeviceIoControl
	//
	// Length
	pRequest->PassThrough.Length = sizeof (pRequest->PassThrough);
	// SCSI Address
	//	sScsiRequest.sPass.PathId = psDeviceInfo->sScsiAddress.PathId;
	//	sScsiRequest.sPass.TargetId = psDeviceInfo->sScsiAddress.TargetId;
	//	sScsiRequest.sPass.Lun = psDeviceInfo->sScsiAddress.Lun;
	// CDB Information
	memcpy (pRequest->PassThrough.Cdb, api.GetCdbPtr(), api.GetCdbSize());
	pRequest->PassThrough.CdbLength = (UCHAR)api.GetCdbSize();
	// Data
	pRequest->PassThrough.DataBufferOffset = offsetof (_NT_SCSI_REQUEST, DataBuffer[0]);
	pRequest->PassThrough.DataTransferLength = api.GetTransferSize();
	pRequest->PassThrough.DataIn = api.IsWriteCmd() ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_IN;
	if ( api.IsWriteCmd() && api.GetTransferSize() )
		memcpy( pRequest->DataBuffer, api.GetCmdDataPtr(), api.GetTransferSize() );
	// Sense Information
	pRequest->PassThrough.SenseInfoOffset = offsetof (_NT_SCSI_REQUEST, SenseData);
	pRequest->PassThrough.SenseInfoLength = sizeof (pRequest->SenseData);
	// Handling the device has not responded situation 
	pRequest->PassThrough.ScsiStatus = SCSISTAT_COMMAND_TERMINATED;
	pRequest->SenseData.SenseKey = SCSI_SENSE_UNIQUE;
	pRequest->SenseData.AdditionalSenseCode = (ScsiUtpMsg::EXIT & 0xFF00)>>8;			// EXIT 0x8001
	pRequest->SenseData.AdditionalSenseCodeQualifier = (ScsiUtpMsg::EXIT & 0x00FF);	// EXIT 0x8001
	pRequest->SenseData.Information[0] = 0xff;
	pRequest->SenseData.Information[1] = 0xff;
	pRequest->SenseData.Information[2] = 0xff;
	pRequest->SenseData.Information[3] = 0xff;		
	// Timeout
//	if (pRequest->PassThrough.Cdb[1] == 7)
//	    pRequest->PassThrough.TimeOutValue = 120; // seconds
//	else
		pRequest->PassThrough.TimeOutValue = api.GetTimeout(); // seconds

	//This is a sync transfer, no need to wait an event.
	/*ResetEvent(_hEvent);
	memset(&_overLapped, 0, sizeof(_overLapped));
    _overLapped.hEvent = _hEvent;*/

	// Sending command
	//unsigned int start= ::GetCurrentTime();
	DWORD dwBytesReturned;
	BOOL bResult = ::DeviceIoControl (
		hDrive,
		IOCTL_SCSI_PASS_THROUGH,
		pRequest,
		totalSize,
		pRequest,
		totalSize,
		&dwBytesReturned,
		NULL);
//		&_overLapped);
	

	if (!bResult)
	{
        nsInfo.inProgress = false;
        nsInfo.error = GetLastError();
        nsInfo.status.Format(_T("Failed to send command.\n"));
        
//        Notify(nsInfo);        
	}

	/*unsigned int end= ::GetCurrentTime();
	ATLTRACE(_T("start %d end %d delta %d\r\n"), start, end, end-start);
	
	else
		if ( (err = WaitForCmdToFinish()) == ERROR_SUCCESS )
		{
			//Below code only forms some useless information in which each data is printed to a response string but really time consuming.
			api.ProcessResponse(pRequest->DataBuffer, 0, api.GetTransferSize());
		}
	
	api.ProcessResponse(pRequest->DataBuffer, 0, api.GetTransferSize());

	ATLTRACE(_T("WaitForCmdToFinish time: %d\r\n"),::GetCurrentTime() - end);
    //It is not a good way to update the UI here since this function is really critical to performance.
	//Please leave UI work to outside function.
    nsInfo.position = api.GetTransferSize();
    Notify(nsInfo);*/

	if ( !api.IsWriteCmd() )
		api.ProcessResponse(pRequest->DataBuffer, 0, api.GetTransferSize());

	api.ScsiSenseStatus = pRequest->PassThrough.ScsiStatus;
	api.ScsiSenseData = pRequest->SenseData;

	if (additionalInfo)
        *additionalInfo = api.ScsiSenseStatus;

    return nsInfo.error;
}

void Volume::NotifyUpdateUI(int cmdOpIndex, int position, int maximum)
{
	UI_UPDATE_INFORMATION _uiInfo;
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibHandle)->g_CmdOpThreadID[cmdOpIndex];
	_uiInfo.bUpdatePhaseProgress = FALSE;
	_uiInfo.bUpdateCommandsProgress = FALSE;
	_uiInfo.bUpdateDescription = FALSE;
	_uiInfo.bUpdateProgressInCommand = TRUE;
	_uiInfo.ProgressIndexInCommand = position;
	_uiInfo.ProgressRangeInCommand = maximum;
	((MFGLIB_VARS *)m_pLibHandle)->g_CmdOperationArray[cmdOpIndex]->ExecuteUIUpdate(&_uiInfo);
}
