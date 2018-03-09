/*
 * Copyright 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
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
#pragma once

#include "Device.h"
#include "StHidApi.h"
#include "MfgToolLib.h"

class  CCmdOpreation;

extern "C" {
#include <hidsdi.h>
}

class HidDevice : public Device
{
public:
	HidDevice(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle, COpState *pCurrent);
    virtual ~HidDevice(void);

    virtual UINT32 SendCommand(StApi& api, UINT8* additionalInfo = NULL, int cmdOpIndex=-1);
    virtual CString GetSendCommandErrorStr();
    static void CALLBACK IoCompletion(DWORD dwErrorCode,           // completion code
                                      DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
                                      LPOVERLAPPED lpOverlapped);        // pointer to structure with I/O information
	virtual UINT32 ResetChip();
    DWORD GetHabType();

private:
#pragma pack(1)
    //------------------------------------------------------------------------------
    // HID Command Block Wrapper (CBW)
    //------------------------------------------------------------------------------
    struct _ST_HID_CBW
    {
		UINT32 Signature;        // Signature: 0x43544C42, o "BLTC" (little endian) for the BLTC CBW
		UINT32 Tag;              // Tag: to be returned in the csw
		UINT32 XferLength;       // XferLength: number of bytes to transfer
		UINT8  Flags;            // Flags:
                               //   Bit 7: direction - device shall ignore this bit if the
                               //     XferLength field is zero, otherwise:
                               //     0 = data-out from the host to the device,
                               //     1 = data-in from the device to the host.
                               //   Bits 6..0: reserved - shall be zero.
    UINT8 Reserved[2];       // Reserved - shall be zero.
    _ST_HID_CDB Cdb;           // cdb: the command descriptor block
    };
    // Signature value for _ST_HID_CBW
    static const UINT32 CBW_BLTC_SIGNATURE = 0x43544C42; // "BLTC" (little endian)
    static const UINT32 CBW_PITC_SIGNATURE = 0x43544950; // "PITC" (little endian)
    // Flags values for _ST_HID_CBW
    static const UINT8 CBW_DEVICE_TO_HOST_DIR = 0x80; // "Data Out"
    static const UINT8 CBW_HOST_TO_DEVICE_DIR = 0x00; // "Data In"

    //------------------------------------------------------------------------------
    // HID Command Status Wrapper (CSW)
    //------------------------------------------------------------------------------
    struct _ST_HID_CSW
    {
    UINT32 Signature;        // Signature: 0x53544C42, or "BLTS" (little endian) for the BLTS CSW
    UINT32 Tag;              // Tag: matches the value from the CBW
    UINT32 Residue;          // Residue: number of bytes not transferred
    UINT8  Status;           // Status:
                                //  00h command passed ("good status")
                                //  01h command failed
                                //  02h phase error
                                //  03h to FFh reserved
    };
    // Signature value for _ST_HID_CSW
    static const UINT32 CSW_BLTS_SIGNATURE = 0x53544C42; // "BLTS" (little endian)
    static const UINT32 CSW_PITS_SIGNATURE = 0x53544950; // "PITS" (little endian)
    // Status values for _ST_HID_CSW
    static const UINT8 CSW_CMD_PASSED = 0x00;
    static const UINT8 CSW_CMD_FAILED = 0x01;
    static const UINT8 CSW_CMD_PHASE_ERROR = 0x02;

    struct _ST_HID_COMMMAND_REPORT
    {
        UINT8 ReportId;
        _ST_HID_CBW Cbw;
    };

    struct _ST_HID_DATA_REPORT
    {
        UINT8 ReportId;
        UINT8 Payload[1];
    };

    struct _ST_HID_STATUS_REPORT
    {
        UINT8 ReportId;
        _ST_HID_CSW Csw;
    };
#pragma pack()

    //------------------------------------------------------------------------------
    // HID Device Variables
    //------------------------------------------------------------------------------
    HIDP_CAPS _Capabilities;
    _ST_HID_DATA_REPORT* _pReadReport;
    _ST_HID_DATA_REPORT* _pWriteReport;
    OVERLAPPED  _fileOverlapped;
    UINT8 _status;

    INT32 AllocateIoBuffers();
    void FreeIoBuffers();

    bool ProcessWriteCommand(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo);
    bool ProcessReadData(const HANDLE hDevice, StApi* pApi, NotifyStruct& nsInfo);
    bool ProcessWriteData(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo, int);
    bool ProcessReadStatus(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo);
    INT32 ProcessTimeOut(const INT32 timeout);

    enum HAB_t
    {
        HabUnknown  = -1,
        bltc_sec_config_disabled = 0,
        bltc_sec_config_fab ,
        bltc_sec_config_production ,
        bltc_sec_config_engineering
    };

    enum ChipFamily_t
    {
        ChipUnknown = 0,
        MX23,
        MX28
    };

    ChipFamily_t _chipFamily;
    HAB_t _habType;
    ChipFamily_t GetChipFamily();
    HAB_t GetHABType(ChipFamily_t chipType);
	void UpdateUI(int index, int pos, int max);
};
