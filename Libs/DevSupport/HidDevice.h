#pragma once

#include "Device.h"
#include "StHidApi.h"

extern "C" {
#include <api/hidsdi.h>
}

class HidDevice : public Device
{
public:
	HidDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~HidDevice(void);
    
	virtual uint32_t SendCommand(StApi& api, uint8_t* additionalInfo = NULL);
	virtual CStdString GetSendCommandErrorStr();
   	static void CALLBACK IoCompletion(DWORD dwErrorCode,           // completion code
			                          DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
							          LPOVERLAPPED lpOverlapped);        // pointer to structure with I/O information
	virtual uint32_t ResetChip();

private:
#pragma pack(1)
    //------------------------------------------------------------------------------
    // HID Command Block Wrapper (CBW)
    //------------------------------------------------------------------------------
    struct _ST_HID_CBW
    {
    uint32_t Signature;        // Signature: 0x43544C42, o "BLTC" (little endian) for the BLTC CBW
    uint32_t Tag;              // Tag: to be returned in the csw
    uint32_t XferLength;       // XferLength: number of bytes to transfer
    uint8_t  Flags;            // Flags:
                               //   Bit 7: direction - device shall ignore this bit if the
                               //     XferLength field is zero, otherwise:
                               //     0 = data-out from the host to the device,
                               //     1 = data-in from the device to the host.
                               //   Bits 6..0: reserved - shall be zero.
    uint8_t Reserved[2];       // Reserved - shall be zero.
    _ST_HID_CDB Cdb;           // cdb: the command descriptor block
    };
    // Signature value for _ST_HID_CBW
    static const uint32_t CBW_BLTC_SIGNATURE = 0x43544C42; // "BLTC" (little endian)
    static const uint32_t CBW_PITC_SIGNATURE = 0x43544950; // "PITC" (little endian)
    // Flags values for _ST_HID_CBW
    static const uint8_t CBW_DEVICE_TO_HOST_DIR = 0x80; // "Data Out"
    static const uint8_t CBW_HOST_TO_DEVICE_DIR = 0x00; // "Data In"
    
    //------------------------------------------------------------------------------
    // HID Command Status Wrapper (CSW)
    //------------------------------------------------------------------------------
    struct _ST_HID_CSW
    {
    uint32_t Signature;        // Signature: 0x53544C42, or "BLTS" (little endian) for the BLTS CSW
    uint32_t Tag;              // Tag: matches the value from the CBW
    uint32_t Residue;          // Residue: number of bytes not transferred
    uint8_t  Status;           // Status:
                                //  00h command passed ("good status")
                                //  01h command failed
                                //  02h phase error
                                //  03h to FFh reserved
    };
    // Signature value for _ST_HID_CSW
    static const uint32_t CSW_BLTS_SIGNATURE = 0x53544C42; // "BLTS" (little endian)
    static const uint32_t CSW_PITS_SIGNATURE = 0x53544950; // "PITS" (little endian)
    // Status values for _ST_HID_CSW
    static const uint8_t CSW_CMD_PASSED = 0x00;
    static const uint8_t CSW_CMD_FAILED = 0x01;
    static const uint8_t CSW_CMD_PHASE_ERROR = 0x02;

	struct _ST_HID_COMMMAND_REPORT
	{
		uint8_t ReportId;
		_ST_HID_CBW Cbw;
	};

	struct _ST_HID_DATA_REPORT
	{
		uint8_t ReportId;
		uint8_t Payload[1];
	};

	struct _ST_HID_STATUS_REPORT
	{
		uint8_t ReportId;
		_ST_HID_CSW Csw;
	};
#pragma pack()

    //------------------------------------------------------------------------------
    // HID Device Variables
    //------------------------------------------------------------------------------
    HIDP_CAPS _Capabilities;
    _ST_HID_DATA_REPORT* _pReadReport;
	_ST_HID_DATA_REPORT* _pWriteReport;
//    int32_t _lastError;

//	static HANDLE m_IOSemaphoreComplete;

    OVERLAPPED	_fileOverlapped;
    
//    uint8_t* m_AdditionalInfo;
 
//    uint16_t m_MaxReadPktSize;
//    uint16_t m_MaxWritePktSize;

	uint8_t _status;


    int32_t AllocateIoBuffers();
    void FreeIoBuffers();

    bool ProcessWriteCommand(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo);
    bool ProcessReadData(const HANDLE hDevice, StApi* pApi, NotifyStruct& nsInfo);
    bool ProcessWriteData(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo);
    bool ProcessReadStatus(const HANDLE hDevice, const StApi& api, NotifyStruct& nsInfo);
    int32_t ProcessTimeOut(const int32_t timeout);
};
