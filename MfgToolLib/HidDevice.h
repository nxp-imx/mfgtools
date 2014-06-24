/*
 * Copyright (C) 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "Device.h"
#include "StHidApi.h"

extern "C" {
#include <hidsdi.h>
}

class HidDevice : public Device
{
public:
	HidDevice(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
    virtual ~HidDevice(void);

    virtual UINT32 SendCommand(StApi& api, UINT8* additionalInfo = NULL);
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
    bool ProcessWriteData(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo);
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
};
