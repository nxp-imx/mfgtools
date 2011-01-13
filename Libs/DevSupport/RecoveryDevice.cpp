/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "RecoveryDevice.h"

RecoveryDevice::RecoveryDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
, m_RecoveryHandle(INVALID_HANDLE_VALUE)
{
}

RecoveryDevice::~RecoveryDevice(void)
{
	DWORD error = Close();
	if ( error != ERROR_SUCCESS )
		ATLTRACE2(_T("   RecoveryDevice::~RecoveryDevice() Error closing handle 0x%x(%d).\r\n"), error, error);
}

DWORD RecoveryDevice::Open()
{
	DWORD error = ERROR_SUCCESS;

	m_RecoveryHandle = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ /*|FILE_SHARE_WRITE*/, NULL, OPEN_EXISTING, 0/*FILE_FLAG_OVERLAPPED*/, NULL);

    if( m_RecoveryHandle == INVALID_HANDLE_VALUE )        
        error = GetLastError();
 
    return error;
}

DWORD RecoveryDevice::Close()
{
    DWORD error = ERROR_SUCCESS;

	if( m_RecoveryHandle != INVALID_HANDLE_VALUE )
	{
        if ( !CloseHandle(m_RecoveryHandle) )
			error = GetLastError();

		m_RecoveryHandle = INVALID_HANDLE_VALUE;
	}
	
	return error;
}

uint32_t RecoveryDevice::Download(const StFwComponent& fwComponent, Device::UI_Callback callbackFn)
{
    // Open the device
	uint32_t error = Open();

	if( error != ERROR_SUCCESS )
    {
		ATLTRACE2(_T("   RecoveryDevice::Download() Error opening device 0x%x(%d).\r\n"), error, error);
        return error;
    }

	// For Notifying the UI
	NotifyStruct nsInfo(_T("RecoveryDevice::Download()"), Device::NotifyStruct::dataDir_ToDevice, 0);
//    nsInfo.direction = Device::NotifyStruct::dataDir_ToDevice;

	// For IO completion
//	FileIO_CompletionRoutine callback(this, &RecoveryDevice::IoCompletion);

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
/*		memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
		_fileOverlapped.hEvent = this;
		BOOL temp = WriteFileEx(m_RecoveryHandle, buffer, numBytesToWrite, &_fileOverlapped, RecoveryDevice::IoCompletion);
		if ( (nsInfo.error=GetLastError()) != ERROR_SUCCESS )
		{
			ATLTRACE2(_T(" -RecoveryDevice::Download()  ERROR:(%d)\r\n"), nsInfo.error);
			break;
		}

		int32_t writeResult = ProcessTimeOut(7);
*/        // Write the data to the device
		DWORD bytesWritten = 0;
		if( !WriteFile( m_RecoveryHandle, buffer, numBytesToWrite, &bytesWritten, NULL) )
		{
			error = GetLastError();
			ATLTRACE2(_T("   RecoveryDevice::Download() Error writing to device 0x%x(%d).\r\n"), error, error);
			break;
		}

		// Update the UI
		nsInfo.position += numBytesToWrite;
		callbackFn(nsInfo);
	}
   
	// Close the device;
	error = Close();
	if ( error != ERROR_SUCCESS )
		ATLTRACE2(_T("   RecoveryDevice::Download() Error closing handle 0x%x(%d).\r\n"), error, error);

	return error;
}

int32_t RecoveryDevice::ProcessTimeOut(const int32_t timeout)
{    
//	ATLTRACE2(_T("  +RecoveryDevice::ProcessTimeOut() 0x%x\r\n"), this);
  
    HANDLE hTimer = CreateWaitableTimer(NULL, true, _T("SendCommandTimer"));
    LARGE_INTEGER waitTime; 
    waitTime.QuadPart = timeout * (-10000000);
    SetWaitableTimer(hTimer, &waitTime, 0, NULL, NULL, false);

    HANDLE waitHandles[1] = { hTimer  };
    DWORD waitResult;
    bool done = false;
    while( !done )
    {
        waitResult = MsgWaitForMultipleObjectsEx(1, &waitHandles[0], INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
        switch (waitResult)
        {
            case WAIT_OBJECT_0:
            case WAIT_TIMEOUT:
			    ATLTRACE2(_T("  -RecoveryDevice::ProcessTimeOut() 0x%x - Timeout(%d) %d seconds.\r\n"), this, waitResult, timeout);
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
			        ATLTRACE2(_T("   RecoveryDevice::ProcessTimeOut() 0x%x - Got a message(%0x).\r\n"), this, msg.message);
                    break;
                }
            case WAIT_ABANDONED:
			    ATLTRACE2(_T("  -RecoveryDevice::ProcessTimeOut() 0x%x - Wait abandoned.\r\n"), this);
                return ERROR_OPERATION_ABORTED;
            case WAIT_IO_COMPLETION:
                // this is what we are really waiting on.
//			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - I/O satisfied by CompletionRoutine.\r\n"), this);
                return ERROR_SUCCESS;
		    case WAIT_FAILED:
            {
			    int32_t error = GetLastError();
 			    ATLTRACE2(_T("  -RecoveryDevice::ProcessTimeOut() 0x%x - Wait failed(%d).\r\n"), this, error);
                return error;
            }
            default:
 			    ATLTRACE2(_T("  -RecoveryDevice::ProcessTimeOut() 0x%x - Undefined error(%d).\r\n"), this, waitResult);
                return ERROR_WRITE_FAULT;
        }
    }
	ATLTRACE2(_T("  -RecoveryDevice::ProcessTimeOut() 0x%x WM_QUIT\r\n"), this);
    return ERROR_OPERATION_ABORTED;
}

void CALLBACK RecoveryDevice::IoCompletion(DWORD dwErrorCode,                // completion code
								      DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
									  LPOVERLAPPED lpOverlapped)        // pointer to structure with I/O information
{
//	ATLTRACE2(_T("  +RecoveryDevice::IoCompletion() dev: 0x%x, fn: 0x%x\r\n"), lpOverlapped->hEvent, RecoveryDevice::IoCompletion);
	RecoveryDevice* pDevice =  dynamic_cast<RecoveryDevice*>((RecoveryDevice*)lpOverlapped->hEvent);
//	if ( pDevice )
//        pDevice->_lastError = dwErrorCode;


//	CSingleLock sLock(&m_mutex);
//	if ( sLock.IsLocked() )
//	{
//		ATLTRACE2( _T("   RecoveryDevice::IoCompletion() 0x%x: Had to wait for lock.\r\n"), pDevice );
//	}
//	int retries;
//	for ( retries = 100; !sLock.Lock(50) && retries; --retries ) {
//		ATLTRACE2( _T("  RecoveryDevice::IoCompletion() 0x%x: Waiting for lock. Try: %d\r\n"), pDevice, 100-retries );
//		Sleep(5);
//	}
//	if ( retries == 0 )
//	{
//		ATLTRACE2( _T("   RecoveryDevice::IoCompletion() 0x%x: ERROR! Could not get lock.\r\n"), pDevice);
//		ASSERT(0);
//		return;
//	}

	switch (dwErrorCode)
	{
		case ERROR_HANDLE_EOF:
			ATLTRACE2(_T("   RecoveryDevice::IoCompletion() 0x%x - ERROR_HANDLE_EOF.\r\n"), pDevice);
            break;
		case 0:
//			ATLTRACE2(_T("   RecoveryDevice::IoCompletion() 0x%x - Transferred %d bytes.\r\n"), pDevice, dwNumberOfBytesTransfered);
            break;
		default:
			ATLTRACE2(_T("   RecoveryDevice::IoCompletion() 0x%x - Undefined Error(%x).\r\n"), pDevice, dwErrorCode);
            break;
	}

//	sLock.Unlock();
//	ReleaseSemaphore(RecoveryDevice::m_IOSemaphoreComplete, 1, NULL);
}
