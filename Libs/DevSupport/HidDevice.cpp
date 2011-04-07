/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "HidDevice.h"

HidDevice::HidDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
, _status(CSW_CMD_PASSED)
, _pReadReport(NULL)
, _pWriteReport(NULL)
, _chipFamily(ChipUnknown)
, _habType(HabUnknown)
{
    memset(&_Capabilities, 0, sizeof(_Capabilities));

    int32_t err = AllocateIoBuffers();
	if (err != ERROR_SUCCESS)
	{
        TRACE("HidDevice::InitHidDevie() AllocateIoBuffers fail!\r\n");
    }
}

HidDevice::~HidDevice(void)
{
    FreeIoBuffers();
}

void HidDevice::FreeIoBuffers()
{
    if(_pReadReport)
    {
        free(_pReadReport);
        _pReadReport = NULL;
    }
    
    if(_pWriteReport)
    {
        free(_pWriteReport);
        _pWriteReport = NULL;
    }
}

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HANDLE, PVOID);
typedef UINT (CALLBACK* LPFNDLLFUNC2)(PVOID);
// Modiifes _Capabilities member variable
// Modiifes _pReadReport member variable
// Modiifes _pWriteReport member variable
int32_t HidDevice::AllocateIoBuffers()
{
    int32_t error = ERROR_SUCCESS;

	// Open the device
	//One may meet Error 32(The file cannot be accessed because another process uses it) 
	//under some complicated environment when openning a device without shared mode.
    HANDLE hHidDevice = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if( hHidDevice == INVALID_HANDLE_VALUE )
    {
		error = GetLastError();
//t        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().CreateFile ERROR:(%d)\r\n"), error);
        return error;
    }

    // Get the Capabilities including the max size of the report buffers
    HINSTANCE hHidDll = LoadLibrary(_T("hid.dll"));
    if (hHidDll == NULL)
    {
		error = GetLastError();
        CloseHandle(hHidDevice);
        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().LoadLibrary(hid.dll) ERROR:(%d)\r\n"), error);
        return error;
    }

    PHIDP_PREPARSED_DATA  PreparsedData = NULL;
    LPFNDLLFUNC1 lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(hHidDll, "HidD_GetPreparsedData");
    if (!lpfnDllFunc1)
    {
        // handle the error
		error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);       
        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().GetProcAddress(HidD_GetPreparsedData) ERROR:(%d)\r\n"), error);
        return error;
    }

	LPFNDLLFUNC2 lpfnDllFunc2 = (LPFNDLLFUNC2)GetProcAddress(hHidDll, "HidD_FreePreparsedData");
    if (!lpfnDllFunc2)
    {
        // handle the error
		error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);       
        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().GetProcAddress(HidD_FreePreparsedData) ERROR:(%d)\r\n"), error);
        return error;
    }

	// if ( !HidD_GetPreparsedData(hHidDevice, &PreparsedData) )
	if ( !lpfnDllFunc1(hHidDevice, &PreparsedData) )
    {
		error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);       
        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().HidD_GetPreparsedData ERROR:(%d)\r\n"), error);
		return error != ERROR_SUCCESS ? error : ERROR_GEN_FAILURE;
    }

    lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(hHidDll, "HidP_GetCaps");
    if (!lpfnDllFunc1)
    {
        // handle the error
		error = GetLastError();
        CloseHandle(hHidDevice);
		// HidD_FreePreparsedData(PreparsedData);
		lpfnDllFunc2(PreparsedData);
        FreeLibrary(hHidDll);       
        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().GetProcAddress(HidP_GetCaps) ERROR:(%d)\r\n"), error);
        return error;
    }
    else 
	{
		// if ( HidP_GetCaps(PreparsedData, &_Capabilities) != HIDP_STATUS_SUCCESS )
		if ( lpfnDllFunc1(PreparsedData, &_Capabilities) != HIDP_STATUS_SUCCESS )
		{
			CloseHandle(hHidDevice);
			// HidD_FreePreparsedData(PreparsedData);
			lpfnDllFunc2(PreparsedData);
			FreeLibrary(hHidDll);       
			ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().GetCaps ERROR:(%d)\r\n"), HIDP_STATUS_INVALID_PREPARSED_DATA);
			return HIDP_STATUS_INVALID_PREPARSED_DATA;
		}
	}
	// HidD_FreePreparsedData(PreparsedData);
	lpfnDllFunc2(PreparsedData);
	FreeLibrary(hHidDll);       
    CloseHandle(hHidDevice);

    // Allocate a Read and Write Report buffers
    FreeIoBuffers();

    if ( _Capabilities.InputReportByteLength )
    {
        _pReadReport = (_ST_HID_DATA_REPORT*)malloc(_Capabilities.InputReportByteLength);
        if ( _pReadReport == NULL )
		{
	        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers(). Failed to allocate memory (1)\r\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
		}
    }

    if ( _Capabilities.OutputReportByteLength )
    {
        _pWriteReport = (_ST_HID_DATA_REPORT*)malloc(_Capabilities.OutputReportByteLength);
        if ( _pWriteReport == NULL )
		{
	        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers(). Failed to allocate memory (2)\r\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
		}
    }

    return ERROR_SUCCESS;
}

void CALLBACK HidDevice::IoCompletion(DWORD dwErrorCode,                // completion code
								      DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
									  LPOVERLAPPED lpOverlapped)        // pointer to structure with I/O information
{
//	ATLTRACE2(_T("  +HidDevice::IoCompletion() dev: 0x%x, fn: 0x%x\r\n"), lpOverlapped->hEvent, HidDevice::IoCompletion);
	HidDevice* pDevice =  dynamic_cast<HidDevice*>((HidDevice*)lpOverlapped->hEvent);
//	if ( pDevice )
//        pDevice->_lastError = dwErrorCode;


//	CSingleLock sLock(&m_mutex);
//	if ( sLock.IsLocked() )
//	{
//		ATLTRACE2( _T("   HidDevice::IoCompletion() 0x%x: Had to wait for lock.\r\n"), pDevice );
//	}
//	int retries;
//	for ( retries = 100; !sLock.Lock(50) && retries; --retries ) {
//		ATLTRACE2( _T("  HidDevice::IoCompletion() 0x%x: Waiting for lock. Try: %d\r\n"), pDevice, 100-retries );
//		Sleep(5);
//	}
//	if ( retries == 0 )
//	{
//		ATLTRACE2( _T("   HidDevice::IoCompletion() 0x%x: ERROR! Could not get lock.\r\n"), pDevice);
//		ASSERT(0);
//		return;
//	}

	switch (dwErrorCode)
	{
		case ERROR_HANDLE_EOF:
			ATLTRACE2(_T("   HidDevice::IoCompletion() 0x%x - ERROR_HANDLE_EOF.\r\n"), pDevice);
            break;
		case 0:
//			ATLTRACE2(_T("   HidDevice::IoCompletion() 0x%x - Transferred %d bytes.\r\n"), pDevice, dwNumberOfBytesTransfered);
            break;
		default:
			ATLTRACE2(_T("   HidDevice::IoCompletion() 0x%x - Undefined Error(%x).\r\n"), pDevice, dwErrorCode);
            break;
	}

//	sLock.Unlock();
//	ReleaseSemaphore(HidDevice::m_IOSemaphoreComplete, 1, NULL);
}


uint32_t HidDevice::SendCommand(StApi& api, uint8_t* additionalInfo)
{
//	ATLTRACE2(_T("+HidDevice::SendCommand(%s) dev:0x%x\r\n"), api.GetName(), this);
    _status = CSW_CMD_PASSED;

	// Serialize HID communication at least temporarily
//	CSingleLock sLock(&m_mutex);
//	int retries;
//	for ( retries = 100; !sLock.Lock(50) && retries; --retries ) {
//		ATLTRACE2( _T("->HidDevice::SendCommand() 0x%x: Waiting for lock. Try: %d\r\n"), this, 100-retries+1 );
//		Sleep(5);
//	}
//	if ( retries == 0 )
//	{
//		ATLTRACE2( _T("-HidDevice::SendCommand() 0x%x: ERROR! Could not get lock.\r\n"), this);
//		ASSERT(0);
//		return ERROR_BUSY;
//	}

	// tell the UI we are beginning a command.
    NotifyStruct nsInfo(api.GetName(), api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice, api.GetTransferSize());
//    nsInfo.direction = api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice;
    Notify(nsInfo);
/*
    // If it is not a HID Api, return error.
	if ( api.GetType() != API_TYPE_BLTC && api.GetType() != API_TYPE_PITC )
    {
		nsInfo.error = ERROR_INVALID_PARAMETER;
        nsInfo.status.AppendFormat(_T(" ERROR: Not a HID API."), nsInfo.error);
        Notify(nsInfo);

        ATLTRACE2(_T("HidDevice::SendCommand(%s) ERROR:(%d) Not a HID API.\r\n"), api.GetName(), nsInfo.error);
        return nsInfo.error;
    }
*/
    // Make sure there we have buffers and such
    if ( _pReadReport == NULL || _pWriteReport == NULL )
    {
		nsInfo.error = ERROR_NOT_ENOUGH_MEMORY;
        nsInfo.status.AppendFormat(_T(" ERROR: No report buffers."), nsInfo.error);
        Notify(nsInfo);
//		sLock.Unlock();

        return nsInfo.error;
    }
	
    // Open the device
    HANDLE hHidDevice = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if( hHidDevice == INVALID_HANDLE_VALUE )
    {
		nsInfo.error = GetLastError();
	    CStdString msg;
	    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        nsInfo.status.AppendFormat(_T(" ERROR: %s (%d)"), msg.c_str(), nsInfo.error);
        Notify(nsInfo);
//		sLock.Unlock();

        ATLTRACE2(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
        return nsInfo.error;
    }
//	ATLTRACE2(_T("->HidDevice::SendCommand  OPEN:%s\r\n"), _path.get().c_str());

    // make sure the command itself is ready
    static uint32_t cmdTag(0);
    api.PrepareCommand();
	api.SetTag(++cmdTag);

    if ( !ProcessWriteCommand(hHidDevice, api, nsInfo) ) //CBW
    {
        CloseHandle(hHidDevice);

	    CStdString msg;
	    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        nsInfo.status.AppendFormat(_T(" ERROR: %s (%d)"), msg.c_str(), nsInfo.error);
        Notify(nsInfo);
//		sLock.Unlock();

        ATLTRACE2(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
        return nsInfo.error;
    }

    // Read/Write Data
    if ( api.GetTransferSize() > 0 )
    {
        if ( api.IsWriteCmd() )
        {
            if(!ProcessWriteData(hHidDevice, api, nsInfo))
            {
                CloseHandle(hHidDevice);

	            CStdString msg;
	            FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
                nsInfo.status.AppendFormat(_T(" ERROR: %s (%d)"), msg.c_str(), nsInfo.error);
                Notify(nsInfo);
//				sLock.Unlock();

                ATLTRACE2(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
                return nsInfo.error;
            }
        }
        else
        {
           if(!ProcessReadData(hHidDevice, &api, nsInfo))
            {
                CloseHandle(hHidDevice);

	            CStdString msg;
	            FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
                nsInfo.status.AppendFormat(_T(" ERROR: %s (%d)"), msg.c_str(), nsInfo.error);
                Notify(nsInfo);
//				sLock.Unlock();

				ATLTRACE2(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
                return nsInfo.error;
            }
        }
    }

    if(!ProcessReadStatus(hHidDevice, api, nsInfo)) //CSW_REPORT
    {
        CloseHandle(hHidDevice);

        CStdString msg;
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        nsInfo.status.AppendFormat(_T(" ERROR: %s (%d)"), msg.c_str(), nsInfo.error);
        Notify(nsInfo);
//		sLock.Unlock();

		ATLTRACE2(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
        return nsInfo.error;
    }	

    CloseHandle(hHidDevice);

//	sLock.Unlock();

	// init parameter if it is used
    if (additionalInfo)
        *additionalInfo = _status;

//	ATLTRACE2(_T("-HidDevice::SendCommand(%s) dev:0x%x\r\n"), api.GetName(), this);
    nsInfo.inProgress = false;
	nsInfo.error = _status;
    Notify(nsInfo);
    return ERROR_SUCCESS;
}

bool HidDevice::ProcessWriteCommand(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
//	ATLTRACE2(_T(" HidDevice::ProcessWriteCommand() dev:0x%x, tag:%d\r\n"), this, api.GetTag());
	_ST_HID_CBW cbw = {0};

    uint16_t writeSize = _Capabilities.OutputReportByteLength;
    if(writeSize < 1)
    {
        nsInfo.error = ERROR_BAD_LENGTH;
        ATLTRACE2(_T(" -HidDevice::ProcessWriteCommand()  ERROR:(%d)\r\n"), nsInfo.error);
        return false;
    }

    // Send CBW 
    memset(_pWriteReport, 0xDB, writeSize);
    _pWriteReport->ReportId = (api.GetType()==API_TYPE_BLTC) ? HID_BLTC_REPORT_TYPE_COMMAND_OUT : HID_PITC_REPORT_TYPE_COMMAND_OUT;
	cbw.Tag = api.GetTag();
	cbw.Signature = (api.GetType()==API_TYPE_BLTC) ? CBW_BLTC_SIGNATURE : CBW_PITC_SIGNATURE;
	cbw.XferLength = api.GetTransferSize();
	cbw.Flags = api.IsWriteCmd() ? CBW_HOST_TO_DEVICE_DIR : CBW_DEVICE_TO_HOST_DIR;
	memcpy(&cbw.Cdb, api.GetCdbPtr(), api.GetCdbSize());

	memcpy(&_pWriteReport->Payload[0], &cbw, sizeof(cbw) /*writeSize - 1*/); // TODO: clw remove this comment

    memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
    _fileOverlapped.hEvent = this;
    BOOL temp = WriteFileEx(hDevice, _pWriteReport, writeSize, &_fileOverlapped, HidDevice::IoCompletion);
    if ( (nsInfo.error=GetLastError()) != ERROR_SUCCESS )
	{
		ATLTRACE2(_T(" -HidDevice::ProcessWriteCommand()  ERROR:(%d)\r\n"), nsInfo.error);
        return false;
    }

    nsInfo.error = ProcessTimeOut(api.GetTimeout());
    if( nsInfo.error != ERROR_SUCCESS )
	{
       	BOOL success = CancelIo(hDevice);
   		ATLTRACE2(_T(" -HidDevice::ProcessWriteCommand()  ERROR:(%d)\r\n"), nsInfo.error);
		return false;
	}

//    ATLTRACE2(_T(" -HidDevice::ProcessWriteCommand()\r\n"));
    return true;
}

bool HidDevice::ProcessReadStatus(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
//	ATLTRACE2(_T(" HidDevice::ProcessReadStatus() dev:0x%x, tag:%d\r\n"), this, api.GetTag());
 
    uint16_t readSize = _Capabilities.InputReportByteLength;

    // Allocate the CSW_REPORT
    memset(_pReadReport, 0xDB, readSize);

    memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
    _fileOverlapped.hEvent = this;
    if(!ReadFileEx(hDevice, _pReadReport, readSize, &_fileOverlapped, HidDevice::IoCompletion))        
    {
        nsInfo.error = GetLastError();
   		ATLTRACE2(_T(" -HidDevice::ProcessReadStatus()  ERROR:(%d)\r\n"), nsInfo.error);
        return false;
    }

    nsInfo.error = ProcessTimeOut(api.GetTimeout());
    if( nsInfo.error != ERROR_SUCCESS )
	{
       	BOOL success = CancelIo(hDevice);
   		ATLTRACE2(_T(" -HidDevice::ProcessReadStatus()  ERROR:(%d)\r\n"), nsInfo.error);
		return false;
	}

    // check status
    if ( _pReadReport->ReportId == ((api.GetType()==API_TYPE_BLTC) ? HID_BLTC_REPORT_TYPE_STATUS_IN : HID_PITC_REPORT_TYPE_STATUS_IN) &&
		((_ST_HID_STATUS_REPORT*)_pReadReport)->Csw.Tag == api.GetTag())
    {
        /**m_AdditionalInfo =*/ _status = ((_ST_HID_STATUS_REPORT*)_pReadReport)->Csw.Status;
    }

//    ATLTRACE2(_T(" -HidDevice::ProcessReadStatus\r\n"));
    return true;
}

// Changes data in pApi parameter, therefore must use 'StAp&*' instead of 'const StApi&'
bool HidDevice::ProcessReadData(const HANDLE hDevice, StApi* pApi, NotifyStruct& nsInfo)
{
//	ATLTRACE2(_T(" HidDevice::ProcessReadData() dev:0x%x, tag:%d\r\n"), this, pApi->GetTag());

    uint16_t readSize = _Capabilities.InputReportByteLength;

    for ( uint32_t offset=0; offset < pApi->GetTransferSize(); offset += (readSize - 1) )
    {
//		ATLTRACE2(_T(" HidDevice::ProcessReadData() dev:0x%x, tag:%d, offset:0x%x\r\n"), this, pApi->GetTag(), offset);
		
		memset(_pReadReport, 0xDB, readSize);

        memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
        _fileOverlapped.hEvent = this;
		BOOL temp = ReadFileEx(hDevice, _pReadReport, readSize, &_fileOverlapped, HidDevice::IoCompletion);
        if ( (nsInfo.error=GetLastError()) != ERROR_SUCCESS )
        {
   			ATLTRACE2(_T(" -HidDevice::ProcessReadData()  ERROR:(%d)\r\n"), nsInfo.error);
            if ( !temp )
                return false;
        }

        nsInfo.error = ProcessTimeOut(pApi->GetTimeout());
        if( nsInfo.error != ERROR_SUCCESS )
	    {
       	    BOOL success = CancelIo(hDevice);
   		    ATLTRACE2(_T(" -HidDevice::ProcessReadData()  ERROR:(%d)\r\n"), nsInfo.error);
		    return false;
	    }

        if ( _pReadReport->ReportId == ((pApi->GetType()==API_TYPE_BLTC) ? HID_BLTC_REPORT_TYPE_DATA_IN : HID_PITC_REPORT_TYPE_DATA_IN) )
            pApi->ProcessResponse(&_pReadReport->Payload[0], offset, readSize - 1);

        // Update the UI
        nsInfo.position = min(pApi->GetTransferSize(), offset + (readSize - 1));
        Notify(nsInfo);
    }
//	ATLTRACE2(_T(" -HidDevice::ProcessReadData()\r\n"));
    return true;
}

bool HidDevice::ProcessWriteData(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
//	ATLTRACE2(_T(" HidDevice::ProcessWriteData() dev:0x%x, tag:%d\r\n"), this, api.GetTag());

    uint16_t writeSize = _Capabilities.OutputReportByteLength;
    
    for ( uint32_t offset=0; offset < api.GetTransferSize(); offset += (writeSize - 1) )
    {
        memset(_pWriteReport, 0xFF, writeSize);
        _pWriteReport->ReportId = (api.GetType()==API_TYPE_BLTC) ? HID_BLTC_REPORT_TYPE_DATA_OUT : HID_PITC_REPORT_TYPE_DATA_OUT;


        memcpy(&_pWriteReport->Payload[0],  api.GetCmdDataPtr()+offset, min((uint32_t)(writeSize - 1), (api.GetTransferSize() - offset)));

//c        ATLTRACE2(_T("ProcessWriteData()->Loop\r\n"));
        memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
//		ATLTRACE2(_T(" HidDevice::ProcessWriteData() dev:0x%x, tag:%d, offset:0x%x\r\n"), this, api.GetTag(), offset);

		_fileOverlapped.hEvent = this;
        if(!WriteFileEx(hDevice, _pWriteReport, writeSize, &_fileOverlapped, HidDevice::IoCompletion))
        {
            nsInfo.error = GetLastError();
   		    ATLTRACE2(_T(" -HidDevice::ProcessWriteData()  ERROR:(%d)\r\n"), nsInfo.error);
            return false;
        }

        nsInfo.error = ProcessTimeOut(api.GetTimeout());
        if( nsInfo.error != ERROR_SUCCESS )
	    {
       	    BOOL success = CancelIo(hDevice);
   		    ATLTRACE2(_T(" -HidDevice::ProcessWriteData()  ERROR:(%d)\r\n"), nsInfo.error);
		    return false;
	    }

        // Update the UI
        nsInfo.position = min(api.GetTransferSize(), offset + (writeSize - 1));
        Notify(nsInfo);
    }
//	ATLTRACE2(_T(" -HidDevice::ProcessWriteData()\r\n"));
    return true;
}

int32_t HidDevice::ProcessTimeOut(const int32_t timeout)
{    
//	ATLTRACE2(_T("  +HidDevice::ProcessTimeOut() 0x%x\r\n"), this);
  
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
			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - Timeout(%d) %d seconds.\r\n"), this, waitResult, timeout);
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
			        ATLTRACE2(_T("   HidDevice::ProcessTimeOut() 0x%x - Got a message(%0x).\r\n"), this, msg.message);
                    break;
                }
            case WAIT_ABANDONED:
			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - Wait abandoned.\r\n"), this);
                return ERROR_OPERATION_ABORTED;
            case WAIT_IO_COMPLETION:
                // this is what we are really waiting on.
//			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - I/O satisfied by CompletionRoutine.\r\n"), this);
                return ERROR_SUCCESS;
		    case WAIT_FAILED:
            {
			    int32_t error = GetLastError();
 			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - Wait failed(%d).\r\n"), this, error);
                return error;
            }
            default:
 			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - Undefined error(%d).\r\n"), this, waitResult);
                return ERROR_WRITE_FAULT;
        }
    }
	ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x WM_QUIT\r\n"), this);
    return ERROR_OPERATION_ABORTED;
}


CStdString HidDevice::GetSendCommandErrorStr()
{
	CStdString msg;

	switch ( _status )
	{
	    case CSW_CMD_PASSED:
		    msg.Format(_T("HID Status: PASSED(0x%02X)\r\n"), _status);
		    break;
	    case CSW_CMD_FAILED:
    		msg.Format(_T("HID Status: FAILED(0x%02X)\r\n"), _status);
		    break;
	    case CSW_CMD_PHASE_ERROR:
    		msg.Format(_T("HID Status: PHASE_ERROR(0x%02X)\r\n"), _status);
		    break;
	    default:
    		msg.Format(_T("HID Status: UNKNOWN(0x%02X)\r\n"), _status);
	}

	return msg;
}

uint32_t HidDevice::ResetChip()
{
	api::HidDeviceReset api;

	return SendCommand(api);
}

HidDevice::ChipFamily_t HidDevice::GetChipFamily()
{
    if ( _chipFamily == Unknown )
    {
		CString devPath = UsbDevice()->_path.get();
		devPath.MakeUpper();

		if ( devPath.Find(_T("VID_066F&PID_3780")) != -1 )
		{
			_chipFamily = MX23;
		}
		else if ( devPath.Find(_T("VID_15A2&PID_004F")) != -1 )
		{
			_chipFamily = MX28;
		}
    }

	return _chipFamily;
}

//-------------------------------------------------------------------------------------		
// Function to get HAB_TYPE value
//
// @return
//		HabEnabled: if is prodction
//		HabDisabled: if is development/disable
//-------------------------------------------------------------------------------------
HidDevice::HAB_t HidDevice::GetHABType(ChipFamily_t chipType)
{	
	HAB_t habType = HabUnknown;
	
	_ST_HID_CBW cbw = {0};
	_ST_HID_CDB::_CDBHIDINFO _cbd;
	uint32_t bltcStatus;
	uint32_t errcode;

	// Make sure there we have buffers and such
    if ( _pReadReport == NULL || _pWriteReport == NULL )
    {
		goto EXIT;
    }
	
    // Open the device
    HANDLE hHidDevice = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if( hHidDevice == INVALID_HANDLE_VALUE )
    {
        ATLTRACE2(_T("-HidDevice::GetHABType() Create Device Handle fail!\r\n"));
        goto EXIT;
    }

	//Send Command:CBW
	uint16_t writeSize = _Capabilities.OutputReportByteLength;
	if(writeSize < 1)
    {
        errcode = ERROR_BAD_LENGTH;
        ATLTRACE2(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
        goto EXIT;
    }
	
    memset(_pWriteReport, 0x00, writeSize);
    _pWriteReport->ReportId = HID_BLTC_REPORT_TYPE_COMMAND_OUT;
	cbw.Tag = 0;
	cbw.Signature = CBW_BLTC_SIGNATURE ;
	cbw.XferLength = sizeof(bltcStatus);
	cbw.Flags = CBW_DEVICE_TO_HOST_DIR;
	
	memset(&_cbd,0,sizeof(_cbd));
	_cbd.Command = BLTC_INQUIRY;
	_cbd.InfoPage = 0x03;//k_inquiry_page_sec_config(0x03)
	memcpy(&cbw.Cdb,&_cbd, sizeof(_cbd));

	memcpy(&_pWriteReport->Payload[0], &cbw, sizeof(cbw) /*writeSize - 1*/); // 

    memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
    _fileOverlapped.hEvent = this;
    BOOL temp = WriteFileEx(hHidDevice, _pWriteReport, writeSize, &_fileOverlapped, HidDevice::IoCompletion);
    if ( (errcode=GetLastError()) != ERROR_SUCCESS )
	{
		ATLTRACE2(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
        goto EXIT;
    }

    errcode = ProcessTimeOut(ST_SCSI_DEFAULT_TIMEOUT);
    if( errcode != ERROR_SUCCESS )
    {
   	    BOOL success = CancelIo(hHidDevice);
		ATLTRACE2(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
	    goto EXIT;
    }

	// Get status
	uint16_t readSize = _Capabilities.InputReportByteLength;
	memset(_pReadReport, 0x00, readSize);
	memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
	_fileOverlapped.hEvent = this;
	temp = ReadFileEx(hHidDevice, _pReadReport, readSize, &_fileOverlapped, HidDevice::IoCompletion);
	if ( (errcode=GetLastError()) != ERROR_SUCCESS )
	{
		ATLTRACE2(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
		if ( !temp )
			goto EXIT;
	}

	errcode = ProcessTimeOut(ST_SCSI_DEFAULT_TIMEOUT);
	if( errcode != ERROR_SUCCESS )
	{
		BOOL success = CancelIo(hHidDevice);
		ATLTRACE2(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
		goto EXIT;
	}

	if ( _pReadReport->ReportId == HID_BLTC_REPORT_TYPE_DATA_IN)
		habType = (HAB_t)_pReadReport->Payload[0];

	//Check CSW
	readSize = _Capabilities.InputReportByteLength;
    memset(_pReadReport, 0x00, readSize);
    memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
    _fileOverlapped.hEvent = this;
    if(!ReadFileEx(hHidDevice, _pReadReport, readSize, &_fileOverlapped, HidDevice::IoCompletion))        
    {
        errcode = GetLastError();
   		ATLTRACE2(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
        goto EXIT;
    }

    errcode = ProcessTimeOut(ST_SCSI_DEFAULT_TIMEOUT);
    if( errcode != ERROR_SUCCESS )
	{
       	BOOL success = CancelIo(hHidDevice);
   		ATLTRACE2(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
		goto EXIT;
	}

    // check status
    _status = ((_ST_HID_STATUS_REPORT*)_pReadReport)->Csw.Status;
    if ( _status != ERROR_SUCCESS )
    {
        goto EXIT;
    }
	
	CloseHandle(hHidDevice);
	return habType;
EXIT:
	CloseHandle(hHidDevice);
	return HabUnknown;

//The following codes is more easy,but there is a data abort for "HidInquiry api"
/*
	uint8_t moreInfo = 0;
	uint8_t InfoPage_secConfig = 0x3;
	uint32_t infoParam = 0x0;
	HidInquiry api(InfoPage_secConfig, infoParam);

	uint32_t err;
	CStdString _strResponse;
	err = SendCommand(api,&moreInfo);
	if ( err == ERROR_SUCCESS ) 
	{
		_strResponse = api.ResponseString().c_str();
		if ( _strResponse.IsEmpty() )
			_strResponse = _T("OK");
	}
	else
	{
		_strResponse.Format(_T("Error: SendCommand(%s) failed. (%d)\r\n"), api.GetName(), err);

        CStdString strTemp;
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, strTemp.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        _strResponse.AppendFormat(_T("%s"), strTemp.c_str());		
	}
	ATLTRACE2(_T(" HidDevice::GetHABType() %s.\r\n"), _strResponse.c_str());

	habType = (HAB_t)api.GetSecConfig();

	return habType;
*/
}

DWORD HidDevice::GetHabType()
{
    _chipFamily = GetChipFamily();

	if(_chipFamily == MX28)
	{
        if(_habType == HabUnknown)
		    _habType = GetHABType(_chipFamily);
	}

	return _habType;
}
