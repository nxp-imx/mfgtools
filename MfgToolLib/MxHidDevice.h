/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#ifndef   __MXHIDDEVICE_H__
#define   __MXHIDDEVICE_H__

#pragma once

#include "Device.h"
#include <hidsdi.h>

#include "XMLite.h"

#define MAX_SIZE_PER_FLASH_COMMAND					0x200000	/* 50k */
#define MAX_SIZE_PER_DOWNLOAD_COMMAND			0x200000

/* Command Packet Format: Header(2)+Address(4)+Format(1)+ByteCount(4)+Data(4) */
#define ROM_KERNEL_CMD_RD_MEM							0x0101
#define ROM_KERNEL_CMD_WR_MEM							0x0202
#define ROM_KERNEL_CMD_WR_FILE							0x0404
#define ROM_KERNEL_CMD_ERROR_STATUS				0x0505
#define RAM_KERNEL_CMD_HEADER							0x0606
//#define ROM_KERNEL_CMD_RE_ENUM 0x0909
#define ROM_KERNEL_CMD_DCD_WRITE					0x0A0A
#define ROM_KERNEL_CMD_JUMP_ADDR					0x0B0B

#define MAX_DCD_WRITE_REG_CNT		85
#define ROM_WRITE_ACK						0x128A8A12
#define ROM_STATUS_ACK					0x88888888
#define ROM_JUMP_STATUS_ACK			0x00bbcb90

#define REPORT_ID_SDP_CMD				1
#define REPORT_ID_DATA						2

#define IVT_BARKER_HEADER				0x402000D1
#define IVT_BARKER2_HEADER				0x412000D1
#define ROM_TRANSFER_SIZE				0x400
#define IVT_OFFSET								0x400
#define HAB_CMD_WRT_DAT					0xcc  /**< Write Data */
#define HAB_CMD_CHK_DAT					0xcf  /**< Check Data */
#define HAB_TAG_DCD							0xd2       /**< Device Configuration Data */
#define HAB_DCD_BYTES_MAX				1768

//DCD binary data format:
//4 bytes for format	4 bytes for register1 address	4 bytes for register1 value to set
//4 bytes for format	4 bytes for register2 address	4 bytes for register2 value to set
//...
typedef struct
{
    UINT format;
    UINT addr;
    UINT data;
} FslMemoryInit;

// Address ranges for Production parts:

/// <summary>
/// A MxHidDevice device.
/// </summary>
class MxHidDevice : public Device
{
    struct libusb_device_handle *m_Handle;
public:
    MxHidDevice(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
    virtual ~MxHidDevice();

public:
	class MemoryInitCommand : public XNode
	{
	public:
		// [XmlAttribute("addr")]
		unsigned int GetAddress()
		{
			unsigned int addr = 0; // default to 0

			CString attr = GetAttrValue(_T("addr"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				addr = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				addr = (unsigned int)_tstoi64(attr);
			}

			return addr;
		};

		// [XmlAttribute("data")]
		unsigned int GetData()
		{
			unsigned int data = 0; // default to 0

			CString attr = GetAttrValue(_T("data"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				data = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				data = (unsigned int)_tstoi64(attr);
			}

			return data;
		};

		// [XmlAttribute("format")]
		unsigned int GetFormat()
		{
			unsigned char format = 0; // default to 0

			CString attr = GetAttrValue(_T("format"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				format = (unsigned char)_tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				format = (unsigned char)_tstoi64(attr);
			}

			return format;
		};

		CString ToString()
		{
			CString str;
			str.Format(_T("addr=\"0x%X\" data=\"0x%X\" format=\"%d\""), GetAddress(), GetData(), GetFormat());
			return str;
		}
	};

	typedef struct _SDPCmd
	{
        short command;
        char format;
        uint32_t address;
        uint32_t dataCount;
        uint32_t data;
	} __attribute__ ((__packed__)) SDPCmd, * PSDPCmd;

	#pragma pack(1)
    typedef struct _MX_HID_DATA_REPORT
    {
        UCHAR ReportId;
        UCHAR Payload[1];
    }MX_HID_DATA_REPORT;
	#pragma pack()

	enum ChipFamily_t
    {
        ChipUnknown = 0,
        MX50,
		MX6Q,
		MX6D,
		MX6SL,
		MX6SX,
		MX7D
    };

	enum HAB_t
	{
		HabUnknown  = -1,
		HabEnabled  = 0x12343412,
		HabDisabled = 0x56787856
	};

	//DCD binary data format:
	//4 bytes for format	4 bytes for register1 address	4 bytes for register1 value to set
	//4 bytes for format	4 bytes for register2 address	4 bytes for register2 value to set
	//...
	typedef struct _RomFormatDCDData
	{
		UINT format;
		UINT addr;
		UINT data;
	}RomFormatDCDData, *PRomFormatDCDData;

	typedef struct _ImgFormatDCDData
	{
		uint32_t Address;
		uint32_t Data;
	}ImgFormatDCDData, *PImgFormatDCDData;


	typedef struct _IvtHeader
	{
           uint32_t IvtBarker;
           uint32_t ImageStartAddr;
           uint32_t Reserved;
	   uint32_t DCDAddress;
	   uint32_t BootData;
           uint32_t SelfAddr;
           uint32_t Reserved2[2];
	}IvtHeader, *PIvtHeader;

	typedef struct _BootData
	{
           uint32_t ImageStartAddr;
           uint32_t ImageSize;
	   uint32_t PluginFlag;
	}BootData, *PBootData;

	enum MemorySection
	{
		MemSectionOTH = 0x00,
		MemSectionAPP = 0xAA,
		MemSectionCSF = 0xCC,
		MemSectionDCD = 0xEE
	};

	typedef struct _ImageParameter
	{
        UINT PhyRAMAddr4KRL;//The physical address in RAM where an image locates.
        MemorySection loadSection;
		MemorySection setSection;
		BOOL HasFlashHeader;//Does an image have a flash header or ivt header.
		UINT CodeOffset;//The offset in an image where the first byte of code locates, the parameter is used to skip flash header or ivt header embedded in an image.
	}ImageParameter, *PImageParameter;

	_MX_HID_DATA_REPORT		*m_pReadReport;
    _MX_HID_DATA_REPORT		*m_pWriteReport;
	HIDP_CAPS	m_Capabilities;
	HANDLE	    m_hid_drive_handle;
	ChipFamily_t _chipFamily;
	CString		_chiFamilyName;
	UINT m_jumpAddr;

	static MemorySection StringToMemorySection(CString section)
	{
		if ( section.CompareNoCase(_T("DCD")) == 0 )
			return MemSectionDCD;
		else if ( section.CompareNoCase(_T("CSF")) == 0 )
			return MemSectionCSF;
		else if ( section.CompareNoCase(_T("APP")) == 0 )
			return MemSectionAPP;
		else return MemSectionOTH;
	}

	enum MemoryAction
	{
		MemAction_None,
		MemAction_Set,
		MemAction_Jump
	};
	static MemoryAction StringToMemoryAction(CString action)
	{
		if ( action.CompareNoCase(_T("Set")) == 0 )
			return MemAction_Set;
		else if ( action.CompareNoCase(_T("Jump")) == 0 )
			return MemAction_Jump;
		else return MemAction_None;
	}

public:
	BOOL InitMemoryDevice(CString filename);
	BOOL OpenMxHidHandle();
	BOOL CloseMxHidHandle();
	int AllocateIoBuffers();
	void FreeIoBuffers();
	BOOL OpenUSBHandle(HANDLE* pHandle, CString pipePath);
	BOOL WriteReg(PSDPCmd pSDPCmd);
	BOOL DCDWrite(PUCHAR DataBuf, UINT RegCount);
	BOOL SendCmd(PSDPCmd pSDPCmd);
	BOOL SendData(const unsigned char * DataBuf, UINT ByteCnt);
	BOOL GetHABType();
	BOOL GetDevAck(UINT RequiredCmdAck);
	VOID PackSDPCmd(PSDPCmd pSDPCmd);
	int Read(void* buf, UINT size);
	int Write(UCHAR* buf, ULONG size);
	BOOL GetCmdAck(UINT RequiredCmdAck);
	BOOL Jump();
	BOOL Jump(UINT RAMAddress);
	//BOOL RunPlugIn(HANDLE hFileMapping, ULONGLONG dwFileSize);
	BOOL RunPlugIn(UCHAR *pFileDataBuf, ULONGLONG dwFileSize);
	BOOL TransData(UINT address, UINT byteCount, const unsigned char * pBuf);
	BOOL AddIvtHdr(UINT32 ImageStartAddr);
	BOOL ReadData(UINT address, UINT byteCount, unsigned char * pBuf);
	//BOOL Download(PImageParameter pImageParameter, HANDLE hFileMapping, ULONGLONG dwFileSize);
	BOOL Download(PImageParameter pImageParameter, UCHAR *pFileDataBuf, ULONGLONG dwFileSize, int cmdOpIndex);
	void Reset(DEVINST devInst, CString path);
	void SetChipFamily();
	void NotifyOpen();
};

#endif //  __MXHIDDEVICE_H__
