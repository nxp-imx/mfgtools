/*
 * Copyright 2009-2014, 2016 Freescale Semiconductor, Inc.
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
    return ERROR_SUCCESS;
}

void CALLBACK HidDevice::IoCompletion(DWORD dwErrorCode,                // completion code
                                      DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
                                      LPOVERLAPPED lpOverlapped)        // pointer to structure with I/O information
{
    HidDevice* pDevice =  dynamic_cast<HidDevice*>((HidDevice*)lpOverlapped->hEvent);

    switch (dwErrorCode)
    {
        case 0:
            break;
        default:
            TRACE(_T("   HidDevice::IoCompletion() 0x%x - Undefined Error(%x).\r\n"), pDevice, dwErrorCode);
            break;
    }

}


UINT32 HidDevice::SendCommand(StApi& api, UINT8* additionalInfo)
{
    return ERROR_SUCCESS;
}

bool HidDevice::ProcessWriteCommand(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
    return true;
}

bool HidDevice::ProcessReadStatus(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
    return true;
}

// Changes data in pApi parameter, therefore must use 'StAp&*' instead of 'const StApi&'
bool HidDevice::ProcessReadData(const HANDLE hDevice, StApi* pApi, NotifyStruct& nsInfo)
{
    return true;
}

bool HidDevice::ProcessWriteData(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo)
{
    return true;
}

INT32 HidDevice::ProcessTimeOut(const INT32 timeout)
{
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
