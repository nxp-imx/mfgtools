/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
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
#include "HidDevice.h"
//#include "logmgr.h"
//#include <Assert.h>

HidDevice::HidDevice(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
: Device(deviceClass, devInst, path, handle)
, _status(CSW_CMD_PASSED)
, _pReadReport(NULL)
, _pWriteReport(NULL)
, _chipFamily(ChipUnknown)
, _habType(HabUnknown)
{
    memset(&_Capabilities, 0, sizeof(_Capabilities));

    INT32 err = AllocateIoBuffers();
    if (err != ERROR_SUCCESS)
    {
        TRACE(_T("HidDevice::InitHidDevie() AllocateIoBuffers fail!\r\n"));
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
INT32 HidDevice::AllocateIoBuffers()
{
	INT32 error = ERROR_SUCCESS;
#if 0
    // Open the device
    //One may meet Error 32(The file cannot be accessed because another process uses it)
    //under some complicated environment when openning a device without shared mode.
    HANDLE hHidDevice = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if( hHidDevice == INVALID_HANDLE_VALUE )
    {
        error = GetLastError();
        TRACE(_T(" HidDevice::AllocateIoBuffers().CreateFile ERROR:(%d)\r\n"), error);
        return error;
    }

    // Get the Capabilities including the max size of the report buffers
    HINSTANCE hHidDll = LoadLibrary(_T("hid.dll"));
    if (hHidDll == NULL)
    {
        error = GetLastError();
        CloseHandle(hHidDevice);
        TRACE(_T(" HidDevice::AllocateIoBuffers().LoadLibrary(hid.dll) ERROR:(%d)\r\n"), error);
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
        TRACE(_T(" HidDevice::AllocateIoBuffers().GetProcAddress(HidD_GetPreparsedData) ERROR:(%d)\r\n"), error);
        return error;
    }

    LPFNDLLFUNC2 lpfnDllFunc2 = (LPFNDLLFUNC2)GetProcAddress(hHidDll, "HidD_FreePreparsedData");
    if (!lpfnDllFunc2)
    {
        // handle the error
        error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);
        TRACE(_T(" HidDevice::AllocateIoBuffers().GetProcAddress(HidD_FreePreparsedData) ERROR:(%d)\r\n"), error);
        return error;
    }

    // if ( !HidD_GetPreparsedData(hHidDevice, &PreparsedData) )
    if ( !lpfnDllFunc1(hHidDevice, &PreparsedData) )
    {
        error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);
        TRACE(_T(" HidDevice::AllocateIoBuffers().HidD_GetPreparsedData ERROR:(%d)\r\n"), error);
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
        TRACE(_T(" HidDevice::AllocateIoBuffers().GetProcAddress(HidP_GetCaps) ERROR:(%d)\r\n"), error);
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
            TRACE(_T(" HidDevice::AllocateIoBuffers().GetCaps ERROR:(%d)\r\n"), HIDP_STATUS_INVALID_PREPARSED_DATA);
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
            TRACE(_T(" HidDevice::AllocateIoBuffers(). Failed to allocate memory (1)\r\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( _Capabilities.OutputReportByteLength )
    {
        _pWriteReport = (_ST_HID_DATA_REPORT*)malloc(_Capabilities.OutputReportByteLength);
        if ( _pWriteReport == NULL )
        {
            TRACE(_T(" HidDevice::AllocateIoBuffers(). Failed to allocate memory (2)\r\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
#endif
    return ERROR_SUCCESS;
}

void CALLBACK HidDevice::IoCompletion(DWORD dwErrorCode,                // completion code
                                      DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
                                      LPOVERLAPPED lpOverlapped)        // pointer to structure with I/O information
{
    HidDevice* pDevice =  dynamic_cast<HidDevice*>((HidDevice*)lpOverlapped->hEvent);

    switch (dwErrorCode)
    {
#if 0
        case ERROR_HANDLE_EOF:
            TRACE(_T("   HidDevice::IoCompletion() 0x%x - ERROR_HANDLE_EOF.\r\n"), pDevice);
            break;
#endif
        case 0:
            break;
        default:
            TRACE(_T("   HidDevice::IoCompletion() 0x%x - Undefined Error(%x).\r\n"), pDevice, dwErrorCode);
            break;
    }

}


UINT32 HidDevice::SendCommand(StApi& api, UINT8* additionalInfo)
{
#if 0
    _status = CSW_CMD_PASSED;

    // tell the UI we are beginning a command.
    NotifyStruct nsInfo(api.GetName(), api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice, api.GetTransferSize());
    // Make sure there we have buffers and such
    if ( _pReadReport == NULL || _pWriteReport == NULL )
    {
        nsInfo.error = ERROR_NOT_ENOUGH_MEMORY;
        nsInfo.status.AppendFormat(_T(" ERROR: No report buffers."), nsInfo.error);

        return nsInfo.error;
    }
    // Open the device
    HANDLE hHidDevice = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if( hHidDevice == INVALID_HANDLE_VALUE )
    {
        nsInfo.error = GetLastError();
        CString msg;
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        nsInfo.status.AppendFormat(_T(" ERROR: %S (%d)"), msg, nsInfo.error);

        TRACE(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
        return nsInfo.error;
    }

    // make sure the command itself is ready
    static UINT32 cmdTag(0);
    api.PrepareCommand();
    api.SetTag(++cmdTag);

    if ( !ProcessWriteCommand(hHidDevice, api, nsInfo) ) //CBW
    {
        CloseHandle(hHidDevice);

        CString msg;
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        nsInfo.status.AppendFormat(_T(" ERROR: %S (%d)"), msg, nsInfo.error);

        TRACE(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
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

                CString msg;
                FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
                nsInfo.status.AppendFormat(_T(" ERROR: %S (%d)"), msg, nsInfo.error);

                TRACE(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
                return nsInfo.error;
            }
        }
        else
        {
           if(!ProcessReadData(hHidDevice, &api, nsInfo))
            {
                CloseHandle(hHidDevice);

                CString msg;
                FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
                nsInfo.status.AppendFormat(_T(" ERROR: %S (%d)"), msg, nsInfo.error);

                TRACE(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
                return nsInfo.error;
            }
        }
    }

    if(!ProcessReadStatus(hHidDevice, api, nsInfo)) //CSW_REPORT
    {
        CloseHandle(hHidDevice);

        CString msg;
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nsInfo.error, 0, msg.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        nsInfo.status.AppendFormat(_T(" ERROR: %S (%d)"), msg, nsInfo.error);

        TRACE(_T("-HidDevice::SendCommand(%s)  ERROR:(%d)\r\n"), api.GetName(), nsInfo.error);
        return nsInfo.error;
    }

    CloseHandle(hHidDevice);


    // init parameter if it is used
    if (additionalInfo)
        *additionalInfo = _status;

    nsInfo.inProgress = false;
    nsInfo.error = _status;
#endif
    return ERROR_SUCCESS;
}

bool HidDevice::ProcessWriteCommand(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
#if 0
//  TRACE(_T(" HidDevice::ProcessWriteCommand() dev:0x%x, tag:%d\r\n"), this, api.GetTag());
    _ST_HID_CBW cbw = {0};

    UINT16 writeSize = _Capabilities.OutputReportByteLength;
    if(writeSize < 1)
    {
        nsInfo.error = ERROR_BAD_LENGTH;
        TRACE(_T(" -HidDevice::ProcessWriteCommand()  ERROR:(%d)\r\n"), nsInfo.error);
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
        TRACE(_T(" -HidDevice::ProcessWriteCommand()  ERROR:(%d)\r\n"), nsInfo.error);
        return false;
    }

    nsInfo.error = ProcessTimeOut(api.GetTimeout());
    if( nsInfo.error != ERROR_SUCCESS )
    {
        BOOL success = CancelIo(hDevice);
        TRACE(_T(" -HidDevice::ProcessWriteCommand()  ERROR:(%d)\r\n"), nsInfo.error);
        return false;
    }

//    TRACE(_T(" -HidDevice::ProcessWriteCommand()\r\n"));
#endif
    return true;
}

bool HidDevice::ProcessReadStatus(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
//  TRACE(_T(" HidDevice::ProcessReadStatus() dev:0x%x, tag:%d\r\n"), this, api.GetTag());
#if 0
    UINT16 readSize = _Capabilities.InputReportByteLength;

    // Allocate the CSW_REPORT
    memset(_pReadReport, 0xDB, readSize);

    memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
    _fileOverlapped.hEvent = this;
    if(!ReadFileEx(hDevice, _pReadReport, readSize, &_fileOverlapped, HidDevice::IoCompletion))
    {
        nsInfo.error = GetLastError();
        TRACE(_T(" -HidDevice::ProcessReadStatus()  ERROR:(%d)\r\n"), nsInfo.error);
        return false;
    }

    nsInfo.error = ProcessTimeOut(api.GetTimeout());
    if( nsInfo.error != ERROR_SUCCESS )
    {
        BOOL success = CancelIo(hDevice);
        TRACE(_T(" -HidDevice::ProcessReadStatus()  ERROR:(%d)\r\n"), nsInfo.error);
        return false;
    }

    // check status
    if ( _pReadReport->ReportId == ((api.GetType()==API_TYPE_BLTC) ? HID_BLTC_REPORT_TYPE_STATUS_IN : HID_PITC_REPORT_TYPE_STATUS_IN) &&
        ((_ST_HID_STATUS_REPORT*)_pReadReport)->Csw.Tag == api.GetTag())
    {
        /**m_AdditionalInfo =*/ _status = ((_ST_HID_STATUS_REPORT*)_pReadReport)->Csw.Status;
    }
#endif
//    TRACE(_T(" -HidDevice::ProcessReadStatus\r\n"));
    return true;
}

// Changes data in pApi parameter, therefore must use 'StAp&*' instead of 'const StApi&'
bool HidDevice::ProcessReadData(const HANDLE hDevice, StApi* pApi, NotifyStruct& nsInfo)
{
//  TRACE(_T(" HidDevice::ProcessReadData() dev:0x%x, tag:%d\r\n"), this, pApi->GetTag());
#if 0
    UINT16 readSize = _Capabilities.InputReportByteLength;

    for ( UINT32 offset=0; offset < pApi->GetTransferSize(); offset += (readSize - 1) )
    {
//      TRACE(_T(" HidDevice::ProcessReadData() dev:0x%x, tag:%d, offset:0x%x\r\n"), this, pApi->GetTag(), offset);

        memset(_pReadReport, 0xDB, readSize);

        memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
        _fileOverlapped.hEvent = this;
        BOOL temp = ReadFileEx(hDevice, _pReadReport, readSize, &_fileOverlapped, HidDevice::IoCompletion);
        if ( (nsInfo.error=GetLastError()) != ERROR_SUCCESS )
        {
            TRACE(_T(" -HidDevice::ProcessReadData()  ERROR:(%d)\r\n"), nsInfo.error);
            if ( !temp )
                return false;
        }

        nsInfo.error = ProcessTimeOut(pApi->GetTimeout());
        if( nsInfo.error != ERROR_SUCCESS )
        {
            BOOL success = CancelIo(hDevice);
            TRACE(_T(" -HidDevice::ProcessReadData()  ERROR:(%d)\r\n"), nsInfo.error);
            return false;
        }

        if ( _pReadReport->ReportId == ((pApi->GetType()==API_TYPE_BLTC) ? HID_BLTC_REPORT_TYPE_DATA_IN : HID_PITC_REPORT_TYPE_DATA_IN) )
            pApi->ProcessResponse(&_pReadReport->Payload[0], offset, readSize - 1);

        // Update the UI
        nsInfo.position = min(pApi->GetTransferSize(), offset + (readSize - 1));
    }
//  TRACE(_T(" -HidDevice::ProcessReadData()\r\n"));
#endif
    return true;
}

bool HidDevice::ProcessWriteData(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
//  TRACE(_T(" HidDevice::ProcessWriteData() dev:0x%x, tag:%d\r\n"), this, api.GetTag());
#if 0
    UINT16 writeSize = _Capabilities.OutputReportByteLength;

    for ( UINT32 offset=0; offset < api.GetTransferSize(); offset += (writeSize - 1) )
    {
        memset(_pWriteReport, 0xFF, writeSize);
        _pWriteReport->ReportId = (api.GetType()==API_TYPE_BLTC) ? HID_BLTC_REPORT_TYPE_DATA_OUT : HID_PITC_REPORT_TYPE_DATA_OUT;


        memcpy(&_pWriteReport->Payload[0],  api.GetCmdDataPtr()+offset, min((UINT32)(writeSize - 1), (api.GetTransferSize() - offset)));

//c        TRACE(_T("ProcessWriteData()->Loop\r\n"));
        memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
//      TRACE(_T(" HidDevice::ProcessWriteData() dev:0x%x, tag:%d, offset:0x%x\r\n"), this, api.GetTag(), offset);

        _fileOverlapped.hEvent = this;
        if(!WriteFileEx(hDevice, _pWriteReport, writeSize, &_fileOverlapped, HidDevice::IoCompletion))
        {
            nsInfo.error = GetLastError();
            TRACE(_T(" -HidDevice::ProcessWriteData()  ERROR:(%d)\r\n"), nsInfo.error);
            return false;
        }

        nsInfo.error = ProcessTimeOut(api.GetTimeout());
        if( nsInfo.error != ERROR_SUCCESS )
        {
            BOOL success = CancelIo(hDevice);
            TRACE(_T(" -HidDevice::ProcessWriteData()  ERROR:(%d)\r\n"), nsInfo.error);
            return false;
        }

        // Update the UI
        nsInfo.position = min(api.GetTransferSize(), offset + (writeSize - 1));
    }
//  TRACE(_T(" -HidDevice::ProcessWriteData()\r\n"));
#endif
    return true;
}

INT32 HidDevice::ProcessTimeOut(const INT32 timeout)
{
//  TRACE(_T("  +HidDevice::ProcessTimeOut() 0x%x\r\n"), this);
#if 0
    HANDLE hTimer = CreateWaitableTimer(NULL, true, _T("SendCommandTimer"));
    LARGE_INTEGER waitTime;
    waitTime.QuadPart = timeout * (-10000000);
    SetWaitableTimer(hTimer, &waitTime, 0, NULL, NULL, false);

    HANDLE waitHandles[1] = { hTimer  };
    DWORD waitResult;
    bool done = false;
    while( !done )
    {
        waitResult = MsgWaitForMultipleObjectsEx(1, &waitHandles[0], 0xFFFFFFFFFF, QS_ALLINPUT, MWMO_ALERTABLE);
        switch (waitResult)
        {
            case WAIT_OBJECT_0:
            case WAIT_TIMEOUT:
                TRACE(_T("  -HidDevice::ProcessTimeOut() 0x%x - Timeout(%d) %d seconds.\r\n"), this, waitResult, timeout);
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
                        }
                        // Otherwise, dispatch the message.
                        DispatchMessage(&msg);
                    } // End of PeekMessage while loop.
                    TRACE(_T("   HidDevice::ProcessTimeOut() 0x%x - Got a message(%0x).\r\n"), this, msg.message);
                    break;
                }
            case WAIT_ABANDONED:
                TRACE(_T("  -HidDevice::ProcessTimeOut() 0x%x - Wait abandoned.\r\n"), this);
                return ERROR_OPERATION_ABORTED;
            case WAIT_IO_COMPLETION:
                // this is what we are really waiting on.
//              TRACE(_T("  -HidDevice::ProcessTimeOut() 0x%x - I/O satisfied by CompletionRoutine.\r\n"), this);
                return ERROR_SUCCESS;
            case WAIT_FAILED:
            {
                INT32 error = GetLastError();
                TRACE(_T("  -HidDevice::ProcessTimeOut() 0x%x - Wait failed(%d).\r\n"), this, error);
                return error;
            }
            default:
                TRACE(_T("  -HidDevice::ProcessTimeOut() 0x%x - Undefined error(%d).\r\n"), this, waitResult);
                return ERROR_WRITE_FAULT;
        }
    }
    TRACE(_T("  -HidDevice::ProcessTimeOut() 0x%x WM_QUIT\r\n"), this);
    return ERROR_OPERATION_ABORTED;

#endif
    return ERROR_SUCCESS;
}


CString HidDevice::GetSendCommandErrorStr()
{

    CString msg;

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

UINT32 HidDevice::ResetChip()
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
//      HabEnabled: if is prodction
//      HabDisabled: if is development/disable
//-------------------------------------------------------------------------------------
HidDevice::HAB_t HidDevice::GetHABType(ChipFamily_t chipType)
{
    HAB_t habType = HabUnknown;
#if 0
    _ST_HID_CBW cbw = {0};
    _ST_HID_CDB::_CDBHIDINFO _cbd;
    UINT32 bltcStatus;
    UINT32 errcode;
    HANDLE hHidDevice =(long) INVALID_HANDLE_VALUE;

    // Make sure there we have buffers and such
    if ( _pReadReport == NULL || _pWriteReport == NULL )
    {
        goto EXIT;
    }

    // Open the device
/*
    hHidDevice = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
*/
    if( hHidDevice ==(long) INVALID_HANDLE_VALUE )
    {
        TRACE(_T("-HidDevice::GetHABType() Create Device Handle fail!\r\n"));
        goto EXIT;
    }

    //Send Command:CBW
    UINT16 writeSize = _Capabilities.OutputReportByteLength;
    if(writeSize < 1)
    {
        errcode = 24L;
        TRACE(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
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

    memcpy(&_pWriteReport->Payload[0], &cbw, sizeof(cbw) /*writeSize - 1*/);

    memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
    _fileOverlapped.hEvent =(UINT) this;
    //BOOL temp = WriteFileEx(hHidDevice, _pWriteReport, writeSize, &_fileOverlapped, HidDevice::IoCompletion);
    if ( (errcode=GetLastError()) != ERROR_SUCCESS )
    {
        TRACE(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
        goto EXIT;
    }

    errcode = ProcessTimeOut(ST_SCSI_DEFAULT_TIMEOUT);
    if( errcode != ERROR_SUCCESS )
    {
        BOOL success = CancelIo(hHidDevice);
        TRACE(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
        goto EXIT;
    }

    // Get status
    UINT16 readSize = _Capabilities.InputReportByteLength;
    memset(_pReadReport, 0x00, readSize);
    memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
    _fileOverlapped.hEvent =(UINT) this;
//    temp = ReadFileEx(hHidDevice, _pReadReport, readSize, &_fileOverlapped, HidDevice::IoCompletion);
    if ( (errcode=GetLastError()) != ERROR_SUCCESS )
    {
        TRACE(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
        if ( !temp )
            goto EXIT;
    }

    errcode = ProcessTimeOut(ST_SCSI_DEFAULT_TIMEOUT);
    if( errcode != ERROR_SUCCESS )
    {
        BOOL success = CancelIo(hHidDevice);
        TRACE(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
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
        TRACE(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
        goto EXIT;
    }

    errcode = ProcessTimeOut(ST_SCSI_DEFAULT_TIMEOUT);
    if( errcode != ERROR_SUCCESS )
    {
        BOOL success = CancelIo(hHidDevice);
        TRACE(_T(" -HidDevice::GetHABType()  ERROR:(%d)\r\n"), errcode);
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
    if( hHidDevice != INVALID_HANDLE_VALUE )
        CloseHandle(hHidDevice);
    return HabUnknown;

//The following codes is more easy,but there is a data abort for "HidInquiry api"
/*
    UINT8 moreInfo = 0;
    UINT8 InfoPage_secConfig = 0x3;
    UINT32 infoParam = 0x0;
    HidInquiry api(InfoPage_secConfig, infoParam);

    UINT32 err;
    CString _strResponse;
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

        CString strTemp;
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, strTemp.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        _strResponse.AppendFormat(_T("%s"), strTemp.c_str());
    }
    TRACE(_T(" HidDevice::GetHABType() %s.\r\n"), _strResponse.c_str());

    habType = (HAB_t)api.GetSecConfig();*/
#endif
    return habType;

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
