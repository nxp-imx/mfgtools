#include "stdafx.h"
#include "Volume.h"
#include "VolumeDeviceClass.h"
#include "KernelApi.h"


Volume::Volume(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
, _diskNumber(-1)
{
	Trash();
	
	_volumeName.describe(this, _T("Volume Path"), _T(""));
	_logicalDrive.describe(this, _T("Drive Letter"), _T(""));
	_friendlyName.describe(this, _T("Friendly Name"), _T(""));
	_diskNumber.describe(this, _T("Physical Disk Number"), _T(""));

    _hEvent = CreateEvent( 
		NULL,    // default security attribute 
        TRUE,    // manual-reset event 
        FALSE,    // initial state = not-signaled 
		_logicalDrive.get().GetBuffer());   // unnamed event object 

	int num = _diskNumber.get();
}

Volume::~Volume(void)
{
	if ( _hEvent != NULL )
	{
		CloseHandle(_hEvent);
		_hEvent = NULL;
	}
	Trash();
}

void Volume::Trash()
{
//	memset(&_scsiSenseData, 0, sizeof(_scsiSenseData));
//	_diskNumbers.erase(_diskNumbers.begin(), _diskNumbers.end());
//	_disks.erase(_disks.begin(), _disks.end());
//	_removableDevices.erase(_removableDevices.begin(), _removableDevices.end());
}

/// <summary>
/// Gets the volume's name.
/// </summary>
CStdString Volume::volumeName::get()
{
	Volume* vol = dynamic_cast<Volume*>(_owner);
	ASSERT(vol);

	DWORD error;

	if (_value.IsEmpty())
    {
		if (gWinVersionInfo().IsWinNT())
		{
			if (!gKernelApi().apiGetVolumeNameForVolumeMountPoint(vol->_path.get() + _T("\\"), _value))
			{
				error = GetLastError();
			}
		}
    }
    return _value;
}

/// <summary>
/// Gets the volume's logical drive in the form [letter]:\
/// </summary>
CStdString Volume::logicalDrive::get()
{
	Volume* vol = dynamic_cast<Volume*>(_owner);
	ASSERT(vol);

	if ((_value.IsEmpty()) && (!vol->_volumeName.get().IsEmpty()))
    {
		std::map<CStdString, CStdString>::iterator item;
		item = ((VolumeDeviceClass*)vol->_deviceClass)->_logicalDrives.find(vol->_volumeName.get());
		if ( item != ((VolumeDeviceClass*)vol->_deviceClass)->_logicalDrives.end() )
			_value = item->second;
    }
    return _value;
}

/// <summary>
/// Property: Gets the volume's friendly name from the 1st Disk device.
/// </summary>
CStdString Volume::friendlyName::get()
{
	if ( _value.IsEmpty() )
	{
		// get the friendly name from the Device object as a default
		Device* dev = dynamic_cast<Device*>(_owner);
		ASSERT(dev);

		_value = dev->_friendlyName.get();
		
		// now try and get the friendly name from our parent USBSTOR node
		if ( dev->Parent() )
		{
			_value = dev->Parent()->_friendlyName.get();
		}
/*
		Volume* vol = dynamic_cast<Volume*>(_owner);
		ASSERT(vol);

		if (!vol->Disks().empty())
		{
			std::list<Device*>::iterator disk = vol->Disks().begin();
			
			_value = (*disk)->_friendlyName.get();
		}
*/	}
	return _value;
}

/// <summary>
/// Property: Gets the volume's Physical Disk Number.
/// </summary>
int32_t Volume::diskNumber::get()
{
	if ( Value == -1 )
	{
		Volume* vol = dynamic_cast<Volume*>(_owner);
		ASSERT(vol);

		if (!vol->_logicalDrive.get().IsEmpty())
        {
			if ( gWinVersionInfo().IsWinNT() )
			{
				CStdString str;
				str.Format(_T("\\\\.\\%s"), vol->_logicalDrive.get());
//				HANDLE hFile = CreateFile(str, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				HANDLE hFile = CreateFile(str, 0, 0, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					DWORD error = GetLastError();
					ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
					ATLTRACE(_T("*** Error: %d, Drive: %s\n"), error, vol->_logicalDrive.get().c_str());
//clw					throw;
				}
				
				DWORD bytesReturned = 0;
				STORAGE_DEVICE_NUMBER driveNumber;
				if (!DeviceIoControl(hFile, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &driveNumber, sizeof(driveNumber), &bytesReturned, NULL))
				{
					// do nothing here on purpose
					DWORD error = GetLastError();
					ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
					ATLTRACE(_T("*** Error: %d, Drive: %s\n"), error, vol->_logicalDrive.get().c_str());
				}
				CloseHandle(hFile);

				if (bytesReturned > 0)
				{
					Value = driveNumber.DeviceNumber;
				}
			}
			else
			{
				ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
				throw;
				//TODO: Can't use CreateFile() to open a volume on Win98
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
	CStdString path = _path.get();
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
/*
Device* Volume::UsbDevice()
{
//	return Device::UsbDevice();
	if (!Disks().empty())
    {
		std::list<Device*>::iterator disk;
		for(disk = Disks().begin(); disk != Disks().end(); ++disk)
        {
			// return the 1st USBDevice
			return (*disk)->UsbDevice();
        }
    }
    return NULL;
}
*/
/// <summary>
/// Returns true if the USB IDs match the filter string.
/// </summary>
/*
bool Volume::ValidateUsbIds()
{
	if (!Disks().empty())
    {
		std::list<Device*>::iterator disk;
		for(disk = Disks().begin(); disk != Disks().end(); ++disk)
        {
			// get the filters for the DiskDeviceClass
			// from the VolumeDeviceClass
			if ( !_deviceClass->GetFilters().empty() )
				_disksClass.SetFilters(_deviceClass->GetFilters());
			
			if ((*disk)->ValidateUsbIds())
                return true;
        }
    }
    return false;
}
*/
/// <summary>
/// Gets a list of underlying disks for this volume.
/// </summary>
/*
std::list<Device*>& Volume::Disks()
{
    if (_disks.empty())
    {
		if (!DiskNumbers().empty())
        {
			// foreach (int index in DiskNumbers)
			std::vector<uint32_t>::iterator index;
			for(index=DiskNumbers().begin(); index != DiskNumbers().end(); ++index)
            {
				if ( *index < _disksClass.Devices().size() )
                {
					_disks.push_back(_disksClass.Devices()[*index]);
                }
            }
        }
    }
    return _disks;
}
*/
/*
std::vector<uint32_t>& Volume::DiskNumbers()
{
	DWORD error;

	if (_diskNumbers.empty())
    {
		if (!_logicalDrive.get().IsEmpty())
        {
			if ( gWinVersionInfo().IsWinNT() )
			{
				CStdString str;
				str.Format(_T("\\\\.\\%s"), _logicalDrive.get());
				HANDLE hFile = CreateFile(str, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					error = GetLastError();
					ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
					ATLTRACE(_T("*** Error: %d, Drive: %s\n"), error, _logicalDrive.get().c_str());
//clw					throw;
				}
				
				int size = 0x400; // some big size
				PVOLUME_DISK_EXTENTS buffer = (PVOLUME_DISK_EXTENTS)malloc(size);
				DWORD bytesReturned = 0;
				if (!DeviceIoControl(hFile, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, buffer, size, &bytesReturned, NULL))
				{
					// do nothing here on purpose
					error = GetLastError();
				}
				CloseHandle(hFile);

				if (bytesReturned > 0)
				{
					DWORD numberOfDiskExtents = buffer->NumberOfDiskExtents;
					for (DWORD i = 0; i < numberOfDiskExtents; i++)
					{
						DWORD diskNumber = buffer->Extents[i].DiskNumber;
						_diskNumbers.push_back(diskNumber);
					}
				}
				free(buffer);
			}
			else
			{
				ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
				throw;
				//TODO: Can't use CreateFile() to open a volume on Win98
			}
		}
    }
    return _diskNumbers;
}
*/
/// <summary>
/// Gets a list of removable devices for this volume.
/// </summary>
/*
std::list<Device*>& Volume::RemovableDevices()
{
    if (_removableDevices.empty())
    {
        if (Disks().empty())
        {
			_removableDevices = Device::RemovableDevices();
        }
        else
        {
            // foreach (Device disk in Disks)
			std::list<Device*>::iterator disk;
			for(disk = Disks().begin(); disk != Disks().end(); ++disk)
			{
                // foreach (Device device in disk.RemovableDevices)
				std::list<Device*>::iterator device;
				for(device = (*disk)->RemovableDevices().begin(); device != (*disk)->RemovableDevices().end(); ++device)
                {
					_removableDevices.push_back(*device);
                }
            }
        }
    }
    return _removableDevices;
}
*/
/// <summary>
/// Compares the current instance with another object of the same type.
/// </summary>
/// <param name="obj">An object to compare with this instance.</param>
/// <returns>A 32-bit signed integer that indicates the relative order of the comparands.</returns>
int32_t Volume::CompareTo(Device* device)
{
	Volume* volume = dynamic_cast<Volume*>(device);
	if ( volume == NULL )
	{
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
		throw;
	}
	
	if (_logicalDrive.get().IsEmpty())
        return 1;

	if (volume->_logicalDrive.get().IsEmpty())
        return -1;

	return _logicalDrive.get().CompareNoCase(volume->_logicalDrive.get());
}

bool Volume::SelfTest(bool eject)
{
	std::list<Device*>::iterator item;

	CStdString name = _volumeName.get();
	CStdString drive = _logicalDrive.get();
//	bool is_usb = IsUsb();
//	std::list<Device*> disks = Disks();
//	for ( item=disks.begin(); item != disks.end(); ++item)
//		(*item)->SelfTest(eject);
//	std::list<Device*> rd = RemovableDevices();
//	for ( item=rd.begin(); item != rd.end(); ++item)
//		(*item)->SelfTest(eject);
//	std::vector<uint32_t> disk_numbers = DiskNumbers();
	int32_t cmp = CompareTo(this);

	return Device::SelfTest(eject);
}

int32_t Volume::WaitForCmdToFinish()
{    
//	ATLTRACE2(_T("  +Volume::WaitForCmdToFinish() vol:%s\r\n"), _logicalDrive.get().c_str());
  
//    HANDLE hTimer = CreateWaitableTimer(NULL, true, _T("SendCommandTimer"));
//    LARGE_INTEGER waitTime; 
//    waitTime.QuadPart = timeout * (-10000000);
//    SetWaitableTimer(hTimer, &waitTime, 0, NULL, NULL, false);

    HANDLE waitHandles[1] = { _hEvent };
    DWORD waitResult;
    bool done = false;
    while( !done )
    {
        waitResult = MsgWaitForMultipleObjectsEx(1, &waitHandles[0], INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
        switch (waitResult)
        {
            case WAIT_TIMEOUT:
				ATLTRACE2(_T("  -Volume::WaitForCmdToFinish() vol:%s - Timeout(%d).\r\n"), _logicalDrive.get().c_str(), waitResult);
                return ERROR_SEM_TIMEOUT;
            case WAIT_OBJECT_0 + 1:
                {
                    // got a message that we need to handle while we are waiting.
                    MSG msg ; 
                    // Read all of the messages in this next loop, 
                    // removing each message as we read it.
                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
                    { 
                        // If it is a quit message, exit.
                        if (msg.message == WM_QUIT)  
                        {
                            done = true;
//                            break;
                        }
                        // Otherwise, dispatch the message.
                        DispatchMessage(&msg); 
                    } // End of PeekMessage while loop.
			        ATLTRACE2(_T("   Volume::WaitForCmdToFinish() vol:%s - Got a message(%0x).\r\n"), _logicalDrive.get().c_str(), msg.message);
                    break;
                }
            case WAIT_ABANDONED:
			    ATLTRACE2(_T("  -Volume::WaitForCmdToFinish() vol:%s - Wait abandoned.\r\n"), _logicalDrive.get().c_str());
                return ERROR_OPERATION_ABORTED;
            case WAIT_OBJECT_0:
            case WAIT_IO_COMPLETION:
                // this is what we are really waiting on.
//			    ATLTRACE2(_T("  -Volume::WaitForCmdToFinish() vol:%s - I/O satisfied by CompletionRoutine.\r\n"), _logicalDrive.get().c_str());
                return ERROR_SUCCESS;
		    case WAIT_FAILED:
            {
			    int32_t error = GetLastError();
 			    ATLTRACE2(_T("  -Volume::WaitForCmdToFinish() vol:%s - Wait failed(%d).\r\n"), _logicalDrive.get().c_str(), error);
                return error;
            }
            default:
 			    ATLTRACE2(_T("  -Volume::WaitForCmdToFinish() vol:%s - Undefined error(%d).\r\n"), _logicalDrive.get().c_str(), waitResult);
                return ERROR_WRITE_FAULT;
        }
    }
	ATLTRACE2(_T("  -Volume::WaitForCmdToFinish() vol:%s WM_QUIT\r\n"), _logicalDrive.get().c_str());
    return ERROR_OPERATION_ABORTED;
}

uint32_t Volume::SendCommand(HANDLE hDrive, StApi& api, uint8_t* additionalInfo, NotifyStruct& nsInfo)
{
	// If it is not a SCSI Api, return error.
//	if ( api.GetType() != API_TYPE_ST_SCSI && api.GetType() != API_TYPE_SCSI )
//		return ERROR_INVALID_PARAMETER;

    // reset our sense data holder in case we need to 
	// save off fresh data
//	memset(&_scsiSenseData, 0, sizeof(_scsiSenseData));
	
	// init parameter if it is used
	if (additionalInfo)
		*additionalInfo = SCSISTAT_GOOD;

	// make sure the command itself is ready
	api.PrepareCommand();

	// Allocate the SCSI request
	DWORD totalSize = sizeof(_NT_SCSI_REQUEST) + api.GetTransferSize();
	_NT_SCSI_REQUEST* pRequest = (_NT_SCSI_REQUEST*)malloc(totalSize);
    if ( pRequest == NULL )
    {
        nsInfo.inProgress = false;
        Notify(nsInfo);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
	memset (pRequest, 0, totalSize);

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
	pRequest->PassThrough.CdbLength = (uint8_t)api.GetCdbSize();
	// Data
	pRequest->PassThrough.DataBufferOffset = offsetof (_NT_SCSI_REQUEST, DataBuffer[0]);
	pRequest->PassThrough.DataTransferLength = api.GetTransferSize();
	pRequest->PassThrough.DataIn = api.IsWriteCmd() ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_IN;
	if ( api.IsWriteCmd() && api.GetTransferSize() )
		memcpy( pRequest->DataBuffer, api.GetCmdDataPtr(), api.GetTransferSize() );
	// Sense Information
	pRequest->PassThrough.SenseInfoOffset = offsetof (_NT_SCSI_REQUEST, SenseData);
	pRequest->PassThrough.SenseInfoLength = sizeof (pRequest->SenseData);
	// Timeout
//	if (pRequest->PassThrough.Cdb[1] == 7)
//	    pRequest->PassThrough.TimeOutValue = 120; // seconds
//	else
		pRequest->PassThrough.TimeOutValue = api.GetTimeout(); // seconds

	ResetEvent(_hEvent);
	memset(&_overLapped, 0, sizeof(_overLapped));
    _overLapped.hEvent = _hEvent;

	// Sending command
	uint32_t err = ERROR_SUCCESS;
	DWORD dwBytesReturned;
	BOOL bResult = ::DeviceIoControl (
		hDrive,
		IOCTL_SCSI_PASS_THROUGH,
		pRequest,
		totalSize,
		pRequest,
		totalSize,
		&dwBytesReturned,
		&_overLapped);

	if (!bResult)
		err = GetLastError();
	else
		if ( (err = WaitForCmdToFinish()) == ERROR_SUCCESS )
		{
			api.ProcessResponse(pRequest->DataBuffer, 0, api.GetTransferSize());
		}

    // Update the UI
    nsInfo.position = api.GetTransferSize();
    Notify(nsInfo);

    api.ScsiSenseStatus = pRequest->PassThrough.ScsiStatus;
	api.ScsiSenseData = pRequest->SenseData;

	if (additionalInfo)
        *additionalInfo = api.ScsiSenseStatus;

	if ( pRequest != NULL )
		free(pRequest); 

    return err;
}

uint32_t Volume::SendCommand(StApi& api, uint8_t* additionalInfo)
{
/*
	// If it is not a SCSI Api, return error.
	if ( api.GetType() != API_TYPE_ST_SCSI && api.GetType() != API_TYPE_SCSI )
		return ERROR_INVALID_PARAMETER;
*/
    // tell the UI we are beginning a command.
    NotifyStruct nsInfo(api.GetName(), api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice, 0);
//    nsInfo.direction = api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice;
    Notify(nsInfo);

    HANDLE hDrive = ::CreateFile (
		_path.get()/*.c_str()*/, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);

	if ( hDrive == INVALID_HANDLE_VALUE )
		return GetLastError();

    uint32_t ret = SendCommand(hDrive, api, additionalInfo, nsInfo);

    CloseHandle(hDrive);

    // tell the UI we are done
    nsInfo.inProgress = false;
    Notify(nsInfo);

    return ret;

}
/*
CStdString Volume::GetSendCommandErrorStr()
{
	CStdString msg;
	switch ( _scsiSenseStatus )
	{
	case SCSISTAT_GOOD:
		msg.Format(_T("SCSI Status: GOOD(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	case SCSISTAT_CHECK_CONDITION:
		msg.Format(_T("SCSI Status: CHECK_CONDITION(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	case SCSISTAT_CONDITION_MET:
		msg.Format(_T("SCSI Status: CONDITION_MET(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	case SCSISTAT_BUSY:
		msg.Format(_T("SCSI Status: BUSY(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	case SCSISTAT_INTERMEDIATE:
		msg.Format(_T("SCSI Status: INTERMEDIATE(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	case SCSISTAT_INTERMEDIATE_COND_MET:
		msg.Format(_T("SCSI Status: INTERMEDIATE_COND_MET(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	case SCSISTAT_RESERVATION_CONFLICT:
		msg.Format(_T("SCSI Status: RESERVATION_CONFLICT(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	case SCSISTAT_COMMAND_TERMINATED:
		msg.Format(_T("SCSI Status: COMMAND_TERMINATED(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	case SCSISTAT_QUEUE_FULL:
		msg.Format(_T("SCSI Status: QUEUE_FULL(0x%02X)\r\n"), _scsiSenseStatus);
		break;
	default:
		msg.Format(_T("SCSI Status: UNKNOWN(0x%02X)\r\n"), _scsiSenseStatus);
	}

	msg += _T("Sense data:\r\n");
	msg.AppendFormat(_T("   ErrorCode: 0x%02X\r\n"), _scsiSenseData.ErrorCode);
	msg.AppendFormat(_T("   Valid: %s\r\n"), _scsiSenseData.Valid ? _T("true") : _T("false"));
	msg.AppendFormat(_T("   SegmentNumber: 0x%02X\r\n"), _scsiSenseData.SegmentNumber);
	SenseKey key(_scsiSenseData.SenseKey);
	msg.AppendFormat(_T("   SenseKey: %s\r\n"), key.ToString().c_str());
	msg.AppendFormat(_T("   IncorrectLength: %s\r\n"), _scsiSenseData.IncorrectLength ? _T("true") : _T("false"));
	msg.AppendFormat(_T("   EndOfMedia: %s\r\n"), _scsiSenseData.EndOfMedia ? _T("true") : _T("false"));
	msg.AppendFormat(_T("   FileMark: %s\r\n"), _scsiSenseData.FileMark ? _T("true") : _T("false"));
	msg.AppendFormat(_T("   Information[0-3]: %02X %02X %02X %02X \r\n"), _scsiSenseData.Information[0], _scsiSenseData.Information[1], _scsiSenseData.Information[2], _scsiSenseData.Information[3]);
	msg.AppendFormat(_T("   AdditionalSenseLength: 0x%02X\r\n"), _scsiSenseData.AdditionalSenseLength);
	msg.AppendFormat(_T("   CommandSpecificInformation[0-3]: %02X %02X %02X %02X\r\n"), _scsiSenseData.CommandSpecificInformation[0], _scsiSenseData.CommandSpecificInformation[1], _scsiSenseData.CommandSpecificInformation[2], _scsiSenseData.CommandSpecificInformation[3]);
	msg.AppendFormat(_T("   AdditionalSenseCode: 0x%02X\r\n"), _scsiSenseData.AdditionalSenseCode);
	msg.AppendFormat(_T("   AdditionalSenseCodeQualifier: 0x%02X\r\n"), _scsiSenseData.AdditionalSenseCodeQualifier);
	msg.AppendFormat(_T("   FieldReplaceableUnitCode: 0x%02X\r\n"), _scsiSenseData.FieldReplaceableUnitCode);
	msg.AppendFormat(_T("   SenseKeySpecific[0-2]: %02X %02X %02X\r\n"), _scsiSenseData.SenseKeySpecific[0], _scsiSenseData.SenseKeySpecific[1], _scsiSenseData.SenseKeySpecific[2]);

	return msg;
}
*/
uint32_t Volume::ResetChip()
{
	api::StChipReset api;

	return SendCommand(api);
}

uint32_t Volume::ResetToRecovery()
{
	api::StResetToRecovery api;

	return SendCommand(api);
}

uint32_t Volume::OldResetToRecovery()
{
	uint32_t error;
	
	// First get the Allocation Table From the Media so we can find bootmanager
	api::StGetLogicalTable apiGetTable;

	error = SendCommand(apiGetTable);
	if ( error == ERROR_SUCCESS )
	{
		// Get the DriveNumber for Bootmanager so we can erase it.
		uint8_t bootmanagerDriveNumber = apiGetTable.GetEntryArray().GetDrive(media::DriveTag_Bootmanger).DriveNumber;
	
		api::StEraseLogicalDrive apiEraseDrive(bootmanagerDriveNumber);

		error = SendCommand(apiEraseDrive);
		if ( error == ERROR_SUCCESS )
		{
			// Last we Reset the device expecting that it will come up in recovery-mode
			api::StChipReset apiReset;

			error = SendCommand(apiEraseDrive);
		}
	}

	return error;
}

int32_t Volume::WriteDrive(const HANDLE hVolume, const uint8_t driveNumber, const std::vector<uint8_t>& data, const size_t dataSize, Device::UI_Callback callback)
{
	int32_t error = ERROR_SUCCESS;
	// For Notifying the UI
	NotifyStruct nsInfo(_T("Volume::WriteDrive()"), Device::NotifyStruct::dataDir_ToDevice, 0);
//    nsInfo.direction = Device::NotifyStruct::dataDir_ToDevice;

	// Dummy info
	NotifyStruct dummyInfo(_T("Not used"), Device::NotifyStruct::dataDir_Off, 0);

	// Get the sector size
	StGetLogicalDriveInfo apiInfo(driveNumber, DriveInfoSectorSizeInBytes);
	error = SendCommand(hVolume, apiInfo, NULL, dummyInfo);
	if ( error != ERROR_SUCCESS )
	{
		ATLTRACE(_T("CVolume::WriteDrive() - Failed to get sector size rc=0x%X"), error);
		return error;
	}

	uint32_t sectorSize = apiInfo.GetSectorSize();
	uint32_t sectorsPerWrite = Device::MaxTransferSizeInBytes / sectorSize;
	uint8_t buffer[Device::MaxTransferSizeInBytes ];

	uint32_t byteIndex, numBytesToWrite = 0;
	for ( byteIndex = 0; byteIndex < dataSize; byteIndex += numBytesToWrite )
	{
		// Init the buffer to 0xFF
		memset(buffer, 0xff, sizeof(buffer));

		// Get some data
		numBytesToWrite = min(sectorsPerWrite*sectorSize, dataSize - byteIndex);
		memcpy_s(buffer, sizeof(buffer), &data[byteIndex], numBytesToWrite);

		// Get bytes to write in terms of sectors
		uint32_t sectorsToWrite = numBytesToWrite / sectorSize;
		if ( numBytesToWrite % sectorSize )
			++sectorsToWrite;
		
        // Write the data to the device
        StWriteLogicalDriveSector apiWrite(driveNumber, sectorSize, byteIndex/sectorSize, sectorsToWrite, buffer);
        error = SendCommand(hVolume, apiWrite, NULL, dummyInfo);
		if( error != ERROR_SUCCESS )
		{
			ATLTRACE(_T("CVolume::WriteDrive() - Failed to write sectors; driveNum:%d sector:%x sectorsToWrite:%x rc=0x%X"), driveNumber, byteIndex/sectorSize, sectorsToWrite, error);
			break;
		}

		// Update the UI
		nsInfo.position += numBytesToWrite;
		callback(nsInfo);
	}
	m_BytesWritten = nsInfo.position;
	return error;
}

int32_t Volume::ReadDrive(const HANDLE hVolume, const uint8_t driveNumber, std::vector<uint8_t>& data, const size_t dataSize, Device::UI_Callback callback)
{
	int32_t error = ERROR_SUCCESS;
	// For Notifying the UI
	NotifyStruct nsInfo(_T("Volume::ReadDrive()"), Device::NotifyStruct::dataDir_FromDevice, 0);
//    nsInfo.direction = Device::NotifyStruct::dataDir_FromDevice;

	// Dummy info
	NotifyStruct dummyInfo(_T("Not used"), Device::NotifyStruct::dataDir_Off, 0);

	data.clear();

	// Get the sector size
	StGetLogicalDriveInfo apiInfo(driveNumber, DriveInfoSectorSizeInBytes);
	error = SendCommand(hVolume, apiInfo, NULL, dummyInfo);
	if ( error != ERROR_SUCCESS )
	{
		ATLTRACE(_T("CVolume::ReadDrive() - Failed to get sector size rc=0x%X"), error);
		return error;
	}

	uint32_t sectorSize = apiInfo.GetSectorSize();
	uint32_t sectorsPerRead = Device::MaxTransferSizeInBytes / sectorSize;

	uint32_t byteIndex, numBytesToRead = 0;
	for ( byteIndex = 0; byteIndex < dataSize; byteIndex += numBytesToRead )
	{
		// Get bytes to read in terms of sectors
		numBytesToRead = min(sectorsPerRead*sectorSize, dataSize - byteIndex);
		uint32_t sectorsToRead = numBytesToRead / sectorSize;
		if ( numBytesToRead % sectorSize )
			++sectorsToRead;
		
        // Read the data from the device
        StReadLogicalDriveSector apiRead(driveNumber, sectorSize, byteIndex/sectorSize, sectorsToRead);
        error = SendCommand(hVolume, apiRead, NULL, dummyInfo);
		if( error != ERROR_SUCCESS )
		{
			ATLTRACE(_T("CVolume::ReadDrive() - Failed to read sectors; driveNum:%d sector:%x sectorsToRead:%x rc=0x%X"), driveNumber, byteIndex/sectorSize, sectorsToRead, error);
			break;
		}

		// Put the data from the device in the return vector
		data.resize(data.size() + numBytesToRead, 0xFF); 
		if ( memcpy_s(&data[byteIndex], numBytesToRead, apiRead.GetDataPtr(), numBytesToRead) != ERROR_SUCCESS )
		{
			error = ERROR_CANNOT_COPY;
			break;
		}

		// Update the UI
		nsInfo.position += numBytesToRead;
		callback(nsInfo);
	}
   
	m_BytesRead = nsInfo.position;
	return error;
}

#define LOCK_TIMEOUT        1000       // 1 Second
#define LOCK_RETRIES        10

// Locks a volume. A locked volume can be accessed only through handles to the file object that locks the volume.

HANDLE Volume::Lock(LockType lockType)
{
	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
	int nTryCount;

	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;

    CStdString disk;
    if ( lockType == LockType_Physical )
	{
		if ( _diskNumber.get() != -1 )
		{
			disk.Format(_T("\\\\.\\PhysicalDrive%d"), _diskNumber.get());
		}
		else
		{
			TRACE(_T("*** Volume::Lock - FAILED\n")); 
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

    if ( hDrive == INVALID_HANDLE_VALUE )
        return hDrive;

	// Do this in a loop until a timeout period has expired
	for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
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

	TRACE(_T("*** Volume::Lock - FAILED\n")); 
    return INVALID_HANDLE_VALUE;
}

// Unlocks a volume.
int32_t Volume::Unlock(HANDLE hDrive, bool close)
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
	TRACE(_T("*** Volume::Unlock - FAILED\n")); 
	return GetLastError();
}

// Dismounts a volume.
int32_t Volume::Dismount(HANDLE hDrive)
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
				FSCTL_DISMOUNT_VOLUME,
				NULL, 0,
				NULL, 0,
				&dwBytesReturned,
				NULL 
			) )
		{		
			return ERROR_SUCCESS;
		}
		Sleep( dwSleepAmount );
	}

	return GetLastError();
}

// Invalidates the cached partition table and re-enumerates the device.
int32_t Volume::Update(HANDLE hDrive)
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
				IOCTL_DISK_UPDATE_PROPERTIES,
				NULL, 0,
				NULL, 0,
				&dwBytesReturned,
				NULL 
			) )
		{		
			return ERROR_SUCCESS;
		}
		Sleep( dwSleepAmount );
	}

	return GetLastError();
}

