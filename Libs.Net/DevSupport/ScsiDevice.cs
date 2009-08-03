using System;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A volume device.
    /// </summary>
    public class ScsiDevice : Device//, IComparable
    {
        internal ScsiDevice(/*DeviceClass deviceClass,*/ IntPtr deviceInstance, string path/*, int index*/)
            : base(/*deviceClass,*/ deviceInstance, path/*, index*/)
        {
        }
    }
}
/*
RecoveryDevice::RecoveryDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
{
    m_ErrorStatus=ERROR_SUCCESS;
}

RecoveryDevice::~RecoveryDevice(void)
{
}

BOOL RecoveryDevice::Open()
{
	m_RecoveryHandle = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if( m_RecoveryHandle == INVALID_HANDLE_VALUE)        
    {
        m_ErrorStatus=GetLastError();
        return FALSE;
    }
 
    m_FileOverlapped.hEvent = 0;
	m_FileOverlapped.Offset = 0;
	m_FileOverlapped.OffsetHigh = 0;

    m_SyncEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	
    if( !m_SyncEvent )
	{
		m_ErrorStatus = GetLastError();
		return FALSE;
	}

    return TRUE;
}

BOOL RecoveryDevice::Close()
{
    if( m_SyncEvent )
	{
		CloseHandle(m_SyncEvent);
		m_SyncEvent = NULL;
	}

    if(m_RecoveryHandle)
        return CloseHandle(m_RecoveryHandle);
    else
        return 0; // error       
}

void CALLBACK RecoveryDevice::IoCompletion(DWORD dwErrorCode,           // completion code
								      DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
									  LPOVERLAPPED lpOverlapped)        // pointer to structure with I/O information
{
    if( ((ULONG)(ULONGLONG)lpOverlapped->hEvent != dwNumberOfBytesTransfered) || dwErrorCode )
	{
		*(BOOL *)&lpOverlapped->Offset = 0;
	}
	else
	{
		*(BOOL *)&lpOverlapped->Offset = dwNumberOfBytesTransfered;
	}

    SetEvent(m_SyncEvent);
}


uint32_t RecoveryDevice::Download(const StFwComponent& fwComponent, Device::UI_Callback callbackFn)
{
	int32_t error = ERROR_SUCCESS;

    // Open the device
	if(!Open())
    {
        Close();
        ATLTRACE2(_T("  ERROR:(%d)\r\n"), m_ErrorStatus);
        return m_ErrorStatus;
    }

	// For Notifying the UI
	NotifyStruct nsInfo(_T("RecoveryDevice::Download()"));
    nsInfo.direction = Device::NotifyStruct::dataDir_ToDevice;

	uint8_t buffer[RecoveryDevice::PipeSize];
	size_t dataSize = fwComponent.size();
	uint32_t byteIndex, numBytesToWrite = 0;
	for ( byteIndex = 0; byteIndex < dataSize; byteIndex += numBytesToWrite )
	{
		// Init the buffer to 0xFF
		memset(buffer, 0xff, sizeof(buffer));

		// Get some data
		numBytesToWrite = min(RecoveryDevice::PipeSize, dataSize - byteIndex);
		memcpy_s(buffer, sizeof(buffer), &fwComponent.GetData()[byteIndex], numBytesToWrite);

        // Write the data to the device
		error = Write(buffer, numBytesToWrite);
		if( error != ERROR_SUCCESS )
			break;

		// Update the UI
		nsInfo.position += numBytesToWrite;
		callbackFn(nsInfo);
	}
   
	return error;
}

uint32_t RecoveryDevice::Write(uint8_t* pBuf, size_t Size)
{
	DWORD status;

	// Preparation
	m_FileOverlapped.Offset		= 0;
    m_FileOverlapped.OffsetHigh	= 0;
	m_FileOverlapped.hEvent		= (PVOID)(ULONGLONG)Size;

	ResetEvent(m_SyncEvent);

	// Write to the device
	if( !WriteFileEx( m_RecoveryHandle, pBuf, (uint32_t)Size, &m_FileOverlapped, 
		RecoveryDevice::IoCompletion ) )
	{
        m_ErrorStatus=GetLastError();
		return m_ErrorStatus;
	}

	// wait for completion
	if( (status = WaitForSingleObjectEx( m_SyncEvent, RecoveryDevice::DeviceTimeout, TRUE )) == WAIT_TIMEOUT )
	{
		CancelIo( m_RecoveryHandle );
        m_ErrorStatus=ERROR_SEM_TIMEOUT;
		return m_ErrorStatus;
	}

	if( m_FileOverlapped.Offset == 0 )
        m_ErrorStatus=ERROR_WRITE_FAULT;
    else
        m_ErrorStatus=ERROR_SUCCESS;
    
    return m_ErrorStatus;
}


// not used at this time 
CStdString RecoveryDevice::GetErrorStr()
{
	CStdString msg;

	return msg;
}
*/