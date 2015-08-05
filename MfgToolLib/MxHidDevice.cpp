/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
//#include <Assert.h>
//#include <cfgmgr32.h>
//#include <basetyps.h>
//#include <setupapi.h>
//#include <initguid.h>
//#include <hidclass.h>
#include "Device.h"
#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"
#include <sys/stat.h>
#include "MxHidDevice.h"

//#define DCD_WRITE
//static PIvtHeader g_pIVT = NULL;

DWORD EndianSwap(DWORD x)
{
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
MxHidDevice::MxHidDevice(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
: Device(deviceClass, devInst, path, handle)
{
	m_hid_drive_handle = (unsigned long)INVALID_HANDLE_VALUE;
    //m_sync_event_tx = NULL;
    //m_sync_event_rx = NULL;
    m_pReadReport = NULL;
    m_pWriteReport = NULL;
/*	if ( path.Find(_T("0054")) != -1 )
	{
		_chipFamily = MX6Q;
	}
	else if ( path.Find(_T("0061")) != -1 )
	{
		_chipFamily = MX6D;
	}
	else if ( path.Find(_T("0063")) != -1 )
	{
		_chipFamily = MX6SL;
	}
	else
	{
		_chipFamily = MX50;
	} */
}
void MxHidDevice::NotifyOpen()
{
	CString filter;
	OP_STATE_ARRAY *pOpStates = GetOpStates((MFGLIB_VARS *)m_pLibHandle);
	OP_STATE_ARRAY::iterator it = pOpStates->begin();
	COpState *pCurrentState = NULL;
	libusb_device_descriptor descObj;
	libusb_device *pDevice = libusb_get_device(m_libusbdevHandle);
	libusb_get_device_descriptor(pDevice, &descObj);
	for(; it!=pOpStates->end(); it++)
	{
		if( descObj.idVendor==(*it)->uiVid && descObj.idProduct==(*it)->uiPid )
		{
			pCurrentState = (*it);
			break;
		}
	}

	if(pCurrentState)
	{
		_chiFamilyName = pCurrentState->strDevice;
	}

	switch(pCurrentState->opDeviceType)
	{
		case DEV_HID_MX6Q:
			_chipFamily = MX6Q;
			_chiFamilyName = "MX6Q";
			break;
		case DEV_HID_MX6D:
			_chipFamily = MX6D;
			break;
		case DEV_HID_MX6SL:
			_chipFamily = MX6SL;
			break;
		case DEV_HID_MX6SX:
			_chipFamily = MX6SX;
			break;
		case DEV_HID_MX7D:
			_chipFamily = MX7D;
			break;
		case DEV_HID_MX6UL:
			_chipFamily = MX6UL;
			break;
		default:
			_chipFamily = MX50;
			break;
	}

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("new MxHidDevice[%p]"), this);
}

void MxHidDevice::Reset(DEVINST devInst, CString path)
{
	Device::Reset(devInst, path);
}

MxHidDevice::~MxHidDevice()
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("delete MxHidDevice[%p]"), this);
}

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HANDLE, PVOID);
typedef UINT (CALLBACK* LPFNDLLFUNC2)(PVOID);
int MxHidDevice::AllocateIoBuffers()
{
#if 0
	int error = ERROR_SUCCESS;

	// Open the device
    HANDLE hHidDevice = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0/* FILE_FLAG_OVERLAPPED */, NULL);
	if( hHidDevice == INVALID_HANDLE_VALUE )
    {
		error = GetLastError();
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers().CreateFile ERROR:(%d)"), error);
        return error;
    }
	// Get the Capabilities including the max size of the report buffers
	HINSTANCE hHidDll = LoadLibrary(_T("hid.dll"));
	if (hHidDll == NULL)
    {
		error = GetLastError();
        CloseHandle(hHidDevice);
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers().LoadLibrary(hid.dll) ERROR:(%d)"), error);
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
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers().GetProcAddress(HidD_GetPreparsedData) ERROR:(%d)"), error);
        return error;
    }

	LPFNDLLFUNC2 lpfnDllFunc2 = (LPFNDLLFUNC2)GetProcAddress(hHidDll, "HidD_FreePreparsedData");
	if (!lpfnDllFunc2)
    {
        // handle the error
		error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers().GetProcAddress(HidD_FreePreparsedData) ERROR:(%d)"), error);
        return error;
    }

	if ( !lpfnDllFunc1(hHidDevice, &PreparsedData) )
    {
		error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers().HidD_GetPreparsedData ERROR:(%d)"), error);
		return error != ERROR_SUCCESS ? error : ERROR_GEN_FAILURE;
    }

	lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(hHidDll, "HidP_GetCaps");
    if (!lpfnDllFunc1)
    {
        // handle the error
		error = GetLastError();
        CloseHandle(hHidDevice);
		lpfnDllFunc2(PreparsedData);
        FreeLibrary(hHidDll);
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers().GetProcAddress(HidP_GetCaps) ERROR:(%d)"), error);
        return error;
    }
	else
	{
		if ( lpfnDllFunc1(PreparsedData, &m_Capabilities) != HIDP_STATUS_SUCCESS )
		{
			CloseHandle(hHidDevice);
			lpfnDllFunc2(PreparsedData);
			FreeLibrary(hHidDll);
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers().GetCaps ERROR:(%d)"), HIDP_STATUS_INVALID_PREPARSED_DATA);
			return HIDP_STATUS_INVALID_PREPARSED_DATA;
		}
	}

	lpfnDllFunc2(PreparsedData);
	FreeLibrary(hHidDll);
    CloseHandle(hHidDevice);

	// Allocate a Read and Write Report buffers
	FreeIoBuffers();
	if ( m_Capabilities.InputReportByteLength )
	{
		m_pReadReport = (_MX_HID_DATA_REPORT*)malloc(m_Capabilities.InputReportByteLength);
		if ( m_pReadReport == NULL )
		{
	        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers(). Failed to allocate memory (1)"));
            return ERROR_NOT_ENOUGH_MEMORY;
		}
	}
	if ( m_Capabilities.OutputReportByteLength )
    {
        m_pWriteReport = (_MX_HID_DATA_REPORT*)malloc(m_Capabilities.OutputReportByteLength);
        if ( m_pWriteReport == NULL )
		{
	        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" MxHidDevice::AllocateIoBuffers(). Failed to allocate memory (2)"));
            return ERROR_NOT_ENOUGH_MEMORY;
		}
    }
#endif

     m_pWriteReport = (_MX_HID_DATA_REPORT*)malloc(1025);
     m_pReadReport = (_MX_HID_DATA_REPORT*)malloc(1025);

	return ERROR_SUCCESS;
}

void MxHidDevice::FreeIoBuffers()
{
    if( m_pReadReport )
    {
        free( m_pReadReport );
        m_pReadReport = NULL;
    }

    if( m_pWriteReport )
    {
        free( m_pWriteReport );
        m_pWriteReport = NULL;
    }
}

BOOL MxHidDevice::OpenMxHidHandle()
{
	int err = ERROR_SUCCESS;

	memset(&m_Capabilities, 0, sizeof(m_Capabilities));

	err = AllocateIoBuffers();
	if ( err != ERROR_SUCCESS )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR,  _T(" ERROR: AllocateIoBuffers failed. %d"),err);
		return FALSE;
	}

/*	// Open the device
    if (!OpenUSBHandle(&m_hid_drive_handle,_path.get()))
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR,  _T(" ERROR: OpenUSBHandle failed."));
		return FALSE;
    }
*/
	return TRUE;
}

BOOL MxHidDevice::OpenUSBHandle(HANDLE* pHandle, CString pipePath)
{
#ifndef __linux__
	*pHandle = CreateFile(pipePath,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0/* FILE_FLAG_OVERLAPPED */,
		NULL);

	if (*pHandle == INVALID_HANDLE_VALUE)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("MxHidDevice::OpenUSBHandle() Failed to open (%s) = %d"), pipePath, GetLastError());
		return FALSE;
	}

	return TRUE;
#else

/*
	ret = libusb_open(rdev, &m_libusbdevHandle);
        if (ret)
		printf(stderr, "Could not open device, ret=%i\n", ret);
*/




#endif
}

#ifndef DCD_WRITE
BOOL MxHidDevice::InitMemoryDevice(CString filename)
{
#if 0
	USES_CONVERSION;
	SDPCmd SDPCmd;

    //Create device handle and report id
    OpenMxHidHandle();

    SDPCmd.command = ROM_KERNEL_CMD_WR_MEM;
    SDPCmd.dataCount = 4;

	FILE * scriptFile = _tfopen(filename, _T("r"));
	if( scriptFile==NULL )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't open file %s."), filename);
		return FALSE;
	}
	struct _stat64i32 FileLen;
	_tstat(filename, &FileLen);
	CAnsiString cmdString;
	std::fread(cmdString.GetBufferSetLength(FileLen.st_size),sizeof(char), (unsigned int)FileLen.st_size,scriptFile);
	cmdString.ReleaseBuffer();

	XNode script;
	if ( script.Load(A2T(cmdString)) != NULL )
	{
		XNodes cmds = script.GetChilds(_T("CMD"));
		XNodes::iterator cmd = cmds.begin();
		for ( ; cmd != cmds.end(); ++cmd )
		{
			MemoryInitCommand* pCmd = (MemoryInitCommand*)(*cmd);
            SDPCmd.format = pCmd->GetFormat();
            SDPCmd.data = pCmd->GetData();
            SDPCmd.address = pCmd->GetAddress();
			if ( !WriteReg(&SDPCmd) )
            {
                TRACE(_T("In InitMemoryDevice(): write memory failed\n"));
                CloseMxHidHandle();
                return FALSE;
            }
		}
	}

    //Clear device handle and report id
    CloseMxHidHandle();
    #endif
	return TRUE;
}

#else

BOOL MxHidDevice::InitMemoryDevice(CString filename)
{
#if 0
	USES_CONVERSION;

	SDPCmd SDPCmd;

	//Create device handle and report id
	OpenMxHidHandle();

	SDPCmd.command = ROM_KERNEL_CMD_WR_MEM;
    SDPCmd.dataCount = 4;

	CFile scriptFile;
	if( !scriptFile.Open(filename, CFile::modeRead | CFile::shareDenyNone, NULL) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't open file %s."), filename);
		return FALSE;
	}
	CStringT<char,StrTraitMFC<char> > cmdString;
	scriptFile.Read(cmdString.GetBufferSetLength((int)scriptFile.GetLength()), (unsigned int)scriptFile.GetLength());
	cmdString.ReleaseBuffer();

	XNode script;
	if ( script.Load(A2T(cmdString)) != NULL )
	{
		XNodes cmds = script.GetChilds(_T("CMD"));
		XNodes::iterator cmd = cmds.begin();
		//There are no more than 200 regs to be written
		FslMemoryInit * pMemPara = (FslMemoryInit * )malloc(0x1000);
		if(pMemPara == NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't malloc FslMemoryInit memory."));
			return FALSE;
		}
		UINT RegCount = 0;
		for ( ; cmd != cmds.end(); ++cmd, RegCount++)
		{
			MemoryInitCommand* pCmd = (MemoryInitCommand*)(*cmd);
            pMemPara[RegCount].format = EndianSwap(pCmd->GetFormat());
            pMemPara[RegCount].addr = EndianSwap(pCmd->GetAddress());
            pMemPara[RegCount].data = EndianSwap(pCmd->GetData());
		}

		if ( !DCDWrite((PUCHAR)pMemPara,RegCount) )
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to initialize memory!"));
			free(pMemPara);
			return FALSE;
		/}
		free(pMemPara);
	}

	//Clear device handle and report id
    CloseMxHidHandle();
#endif
	return TRUE;
}
#endif
BOOL MxHidDevice::CloseMxHidHandle()
{
	if( m_hid_drive_handle != (unsigned long)INVALID_HANDLE_VALUE)
    {
//        CloseHandle(m_hid_drive_handle);
        m_hid_drive_handle = (unsigned long)INVALID_HANDLE_VALUE;
    }

    FreeIoBuffers();

    return TRUE;
}

BOOL MxHidDevice::WriteReg(PSDPCmd pSDPCmd)
{
	if(!SendCmd(pSDPCmd))
		return FALSE;

	if ( !GetCmdAck(ROM_WRITE_ACK) )
	{
		return FALSE;
	}

    return TRUE;
}

BOOL MxHidDevice::DCDWrite(PUCHAR DataBuf, UINT RegCount)
{
	SDPCmd SDPCmd;
    SDPCmd.command = ROM_KERNEL_CMD_DCD_WRITE;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = 0;

	if(_chipFamily == MX50)
	{
		//i.mx50
		while(RegCount)
		{
			//The data count here is based on register unit.
			SDPCmd.dataCount = (RegCount > MAX_DCD_WRITE_REG_CNT) ? MAX_DCD_WRITE_REG_CNT : RegCount;
			RegCount -= SDPCmd.dataCount;
			UINT ByteCnt = SDPCmd.dataCount*sizeof(RomFormatDCDData);

			if(!SendCmd(&SDPCmd))
			{
				return FALSE;
			}

			if(!SendData(DataBuf, ByteCnt))
			{
				return FALSE;
			}

			if (!GetCmdAck(ROM_WRITE_ACK) )
			{
				return FALSE;
			}

			DataBuf += ByteCnt;
		}
	}
	else
	{
		SDPCmd.dataCount = RegCount;
		SDPCmd.address = 0x00910000;//IRAM free space

		if(!SendCmd(&SDPCmd))
		{
			return FALSE;
		}

		UINT MaxHidTransSize = 1025 -1;

		while(RegCount > 0)
		{
			UINT ByteCntTransfered = (RegCount > MaxHidTransSize) ? MaxHidTransSize : RegCount;
			//ByteCntTransfered = 1025;
			RegCount -= ByteCntTransfered;
			if(!SendData(DataBuf, ByteCntTransfered))
			{
				return FALSE;
			}

			if (!GetCmdAck(ROM_WRITE_ACK) )
			{
				return FALSE;
			}

			DataBuf += ByteCntTransfered;
			SDPCmd.address += ByteCntTransfered;
		}
	}

	return TRUE;
}

/// <summary>
//-------------------------------------------------------------------------------------
// Function to 16 byte SDP command format, these 16 bytes will be sent by host to
// device in SDP command field of report 1 data structure
//
// @return
//		a report packet to be sent.
//-------------------------------------------------------------------------------------
//
VOID MxHidDevice::PackSDPCmd(PSDPCmd pSDPCmd)
{
	if(m_pWriteReport == NULL)
	{
		return;
	}
    memset((UCHAR *)m_pWriteReport, 0, 1025);
    m_pWriteReport->ReportId = (unsigned char)REPORT_ID_SDP_CMD;
    uint32_t* pTmpSDPCmd = (uint32_t*)(m_pWriteReport->Payload);

	pTmpSDPCmd[0] = (  ((pSDPCmd->address  & 0x00FF0000) << 8)
		          | ((pSDPCmd->address  & 0xFF000000) >> 8)
		          |  (pSDPCmd->command   & 0x0000FFFF) );

	pTmpSDPCmd[1] = (   (pSDPCmd->dataCount & 0xFF000000)
		          | ((pSDPCmd->format   & 0x000000FF) << 16)
		          | ((pSDPCmd->address  & 0x000000FF) <<  8)
		          | ((pSDPCmd->address  & 0x0000FF00) >>  8 ));

	pTmpSDPCmd[2] = (   (pSDPCmd->data     & 0xFF000000)
		          | ((pSDPCmd->dataCount & 0x000000FF) << 16)
		          |  (pSDPCmd->dataCount & 0x0000FF00)
		          | ((pSDPCmd->dataCount & 0x00FF0000) >> 16));

	pTmpSDPCmd[3] = (  ((0x00  & 0x000000FF) << 24)
		          | ((pSDPCmd->data     & 0x00FF0000) >> 16)
		          |  (pSDPCmd->data     & 0x0000FF00)
		          | ((pSDPCmd->data     & 0x000000FF) << 16));

}

int MxHidDevice::Write(UCHAR* _buf, ULONG _size)
{

#ifdef __linux__
//    struct libusb_device_handle  is the structure of handle
    int ret;
    int transferCount=_size;
    int last_trans = 0;
    int report=_buf[0];
    const int control_transfer =
	LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS |
	LIBUSB_RECIPIENT_INTERFACE;
   // do{
    ret = libusb_control_transfer(m_libusbdevHandle, control_transfer,
		HID_SET_REPORT,
		(HID_REPORT_TYPE_OUTPUT << 8) | report,
		0, _buf+last_trans, _size, 1000);
    last_trans += (ret > 0) ? ret - 1 : 0;
    if (ret > 0)
	    ret = 0;

   // }while(last_trans<_size || ret<0);
    return ret;
#else
	int    nBytesWrite; // for bytes actually written
	if( !WriteFile(m_hid_drive_handle, _buf, _size, (PULONG) &nBytesWrite, NULL) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("MxHidDevice::Write() Error writing to device 0x%x."), GetLastError());
		return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
	}

    return ERROR_SUCCESS;
#endif
}

// Read from HID device
int MxHidDevice::Read(void* _buf, UINT _size)
{
	int    nBytesRead; // for bytes actually read


#ifndef __linux__
	if( !ReadFile(m_hid_drive_handle, _buf, _size, (PULONG) &nBytesRead, NULL) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("MxHidDevice::Read() Error reading from device 0x%x."), GetLastError());
		return GetLastError();
	}
	return ERROR_SUCCESS;

#else
	const int interrupt_transfer =  LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN;
	int ret = libusb_interrupt_transfer(m_libusbdevHandle, interrupt_transfer,(unsigned char *) _buf, _size, &nBytesRead, 10000);
	if(ret!=0){
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("MxHidDevice::Read() Error reading from device 0x%d with size %d."), ret,_size);
	    return errno;
	}
	return ERROR_SUCCESS;
#endif
}

BOOL MxHidDevice::SendCmd(PSDPCmd pSDPCmd)
{
	//First, pack the command to a report.
	PackSDPCmd(pSDPCmd);

	if(m_pWriteReport == NULL)
	{
		return FALSE;
	}
	//Send the report to USB HID device
	if ( Write((unsigned char *)m_pWriteReport, 17) <0)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL MxHidDevice::SendData(const unsigned char * DataBuf, UINT ByteCnt)
{
	if(m_pWriteReport == NULL)
	{
		return FALSE;
	}
	memcpy(m_pWriteReport->Payload, DataBuf, ByteCnt);

	m_pWriteReport->ReportId = REPORT_ID_DATA;

	if (Write((unsigned char *)m_pWriteReport,/*ByteCnt + 1*/1025) <0)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL MxHidDevice::GetCmdAck(UINT RequiredCmdAck)
{
	if(!GetHABType())
	{
		return FALSE;
	}

	if(!GetDevAck(RequiredCmdAck))
	{
		return FALSE;
	}

    return TRUE;
}

//Report4, Device to Host
BOOL MxHidDevice::GetDevAck(UINT RequiredCmdAck)
{
    memset((UCHAR *)m_pReadReport, 0, 1025);

    //Get Report4, Device to Host:
	if ( Read( (UCHAR *)m_pReadReport, 1025 ) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	if (*(unsigned int *)(m_pReadReport->Payload) != RequiredCmdAck)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("WriteReg(): Invalid write ack: 0x%x\n"), ((PULONG)m_pReadReport)[0]);
		return FALSE;
	}
    return TRUE;
}

//Device to Host
BOOL MxHidDevice::GetHABType()
{

    CString LogStr;
    memset((UCHAR *)m_pReadReport, 0, 1025);

    //Get Report3, Device to Host:
    //4 bytes HAB mode indicating Production/Development part
	if ( Read( (UCHAR *)m_pReadReport, 1025 )  != ERROR_SUCCESS)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to read HAB type from ROM!!!"));
		return FALSE;
	}
	if ( (*(unsigned int *)(m_pReadReport->Payload) != HabEnabled)  &&
		 (*(unsigned int *)(m_pReadReport->Payload) != HabDisabled) )
	{
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("HAB type mismatch: 0x%x!!!"), *(unsigned int *)(m_pReadReport->Payload));
		return FALSE;
	}

	return TRUE;
}

BOOL MxHidDevice::Jump()
{
	//Create device handle and report id
    OpenMxHidHandle();

	if(!Jump(m_jumpAddr))
    {
        //Clear device handle and report id
        CloseMxHidHandle();
        return FALSE;
    }

    //Clear device handle and report id
    CloseMxHidHandle();

    return TRUE;
}

BOOL MxHidDevice::Jump(UINT RAMAddress)
{
    SDPCmd SDPCmd;
    CString LogStr;

    SDPCmd.command = ROM_KERNEL_CMD_JUMP_ADDR;
    SDPCmd.dataCount = 0;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = RAMAddress;

	if(!SendCmd(&SDPCmd))
	{
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to send jump command to ROM!!!"));
		return FALSE;

	}

	if(!GetHABType())
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to get HAB type from ROM, ignoredro!!!"));
        return FALSE;
    }

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("*********MxHidDevice[%p] Jump to Ramkernel successfully!**********"), this);

	return TRUE;
}

BOOL MxHidDevice::RunPlugIn(UCHAR *pFileDataBuf, ULONGLONG dwFileSize)
{
	UCHAR* pDataBuf = NULL;
	DWORD * pPlugIn = NULL;
	DWORD PlugInDataOffset= 0,ImgIVTOffset= 0,BootDataImgAddrIndex= 0,PhyRAMAddr4KRL= 0;
	DWORD PlugInAddr = 0;
	PIvtHeader pIVT = NULL,pIVT2 = NULL;

	PBootData pPluginDataBuf =NULL;
	//Create device handle and report id
    OpenMxHidHandle();

	/*
	if ( fwFile.Open(fwFilename, CFile::modeRead | CFile::shareDenyWrite) == 0 )
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Firmware file %s failed to open.errcode is %d\n"), fwFilename,GetLastError());
        goto ERR_HANDLE;
    }

	fwSize = fwFile.GetLength();
    pDataBuf = (UCHAR*)malloc((size_t)fwSize);
    fwFile.Read(pDataBuf, (UINT)fwSize);
    fwFile.Close();
	*/
	pDataBuf = (UCHAR*)malloc((size_t)dwFileSize);
	if(pDataBuf == NULL)
	{
		return FALSE;
	}
	memcpy(pDataBuf, pFileDataBuf, (size_t)dwFileSize);

	//pDataBuf = pFileDataBuf;
	//Search for IVT
    pPlugIn = (DWORD *)pDataBuf;
	//ImgIVTOffset indicates the IVT's offset from the beginning of the image.
	while(ImgIVTOffset < dwFileSize &&
		(pPlugIn[ImgIVTOffset/sizeof(DWORD)] != IVT_BARKER_HEADER &&
		 pPlugIn[ImgIVTOffset/sizeof(DWORD)] != IVT_BARKER2_HEADER
		))
	{
		ImgIVTOffset+= 0x100;
	}
	if(ImgIVTOffset >= dwFileSize)
	{
		goto ERR_HANDLE;
	}

	//Now we find IVT
	pIVT = (PIvtHeader) (pPlugIn + ImgIVTOffset/sizeof(DWORD));
	//Now we have to judge DCD way or plugin way used in the image
	//The method is to check plugin flag in boot data region
    // IVT boot data format
    //   0x00    IMAGE START ADDR
    //   0x04    IMAGE SIZE
    //   0x08    PLUGIN FLAG
	pPluginDataBuf = (PBootData)(pPlugIn + ImgIVTOffset/sizeof(DWORD) + (pIVT->BootData - pIVT->SelfAddr)/sizeof(DWORD));

	if(pPluginDataBuf->PluginFlag)
	{
		//Plugin mode

		//---------------------------------------------------------
		//Run plugin in IRAM
		//Download plugin data into IRAM.
		PlugInAddr = pIVT->ImageStartAddr;
		PlugInDataOffset = pIVT->ImageStartAddr - pIVT->SelfAddr;
		if (!TransData(PlugInAddr, pPluginDataBuf->ImageSize, (PUCHAR)((uint64_t)pIVT + PlugInDataOffset)))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): TransData(0x%X, 0x%X,0x%X) failed."),
				PlugInAddr, pPluginDataBuf->ImageSize, ((uint64_t)pIVT + PlugInDataOffset));
			goto ERR_HANDLE;
		}

		if(!AddIvtHdr(PlugInAddr))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): Failed to addhdr to RAM address: 0x%x."), PlugInAddr);
			goto ERR_HANDLE;
		}

		if( !Jump(m_jumpAddr))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): Failed to jump to RAM address: 0x%x."), m_jumpAddr);
			goto ERR_HANDLE;
		}

		//---------------------------------------------------------
		//Download eboot to ram
		//Search IVT2.
		//ImgIVTOffset indicates the IVT's offset from the beginning of the image.
		DWORD IVT2Offset = ImgIVTOffset + sizeof(IvtHeader);

		while(IVT2Offset < dwFileSize &&
			(pPlugIn[IVT2Offset/sizeof(DWORD)] != IVT_BARKER_HEADER &&
			pPlugIn[IVT2Offset/sizeof(DWORD)] != IVT_BARKER2_HEADER))
			IVT2Offset+= sizeof(DWORD);

		if(IVT2Offset >= dwFileSize)
		{
			goto ERR_HANDLE;
		}
		pIVT2 = (PIvtHeader)(pPlugIn + IVT2Offset/sizeof(DWORD));
		BootDataImgAddrIndex = (DWORD *)pIVT2 - pPlugIn;
		BootDataImgAddrIndex += (pIVT2->BootData - pIVT2->SelfAddr)/sizeof(DWORD);
		PhyRAMAddr4KRL = pPlugIn[BootDataImgAddrIndex] + IVT_OFFSET - ImgIVTOffset;
		if (!TransData(PhyRAMAddr4KRL, (unsigned int)dwFileSize, (PUCHAR)((uint64_t)pDataBuf)))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): TransData(0x%X, 0x%X,0x%X) failed.\n"),
				PhyRAMAddr4KRL, dwFileSize, pDataBuf);
			goto ERR_HANDLE;
		}

		DWORD ImgStartAddr = pIVT2->ImageStartAddr;
		if(!AddIvtHdr(ImgStartAddr))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): Failed to addhdr to RAM address: 0x%x.\n"), ImgStartAddr);
			goto ERR_HANDLE;
		}
	}
	else
	{
        if(NULL != pIVT->DCDAddress)
        {
    		//DCD mode
    		DWORD * pDCDRegion = pPlugIn + ImgIVTOffset/sizeof(DWORD) + (pIVT->DCDAddress - pIVT->SelfAddr)/sizeof(DWORD);
    		//i.e. DCD_BE  0xD2020840              ;DCD_HEADR Tag=0xd2, len=64*8+4+4, ver= 0x40
    		//i.e. DCD_BE  0xCC020404              ;write dcd cmd headr Tag=0xcc, len=64*8+4, param=4
    		//The first 2 32bits data in DCD region is used to give some info about DCD data.
    		//Here big endian format is used, so it must be converted.
    		if(HAB_TAG_DCD != EndianSwap(*pDCDRegion)>>24)
    		{
    			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DCD header tag doesn't match!"));
    			return FALSE;
    		}

			if( (_chipFamily >= MX6Q) )
			{
				//The DCD_WRITE command handling was changed from i.MX508.
				//Now the DCD is  performed by HAB and therefore the format of DCD is the same format as in regular image.
				//The DCD_WRITE parameters require size and address. Size is the size of entire DCD file including header.
				//Address is the temporary address that USB will use for storing the DCD file before processing.

    			DWORD DCDHeader = EndianSwap(*pDCDRegion);
    			//Total dcd data bytes:
    			INT TotalDCDDataCnt = (DCDHeader & 0x00FFFF00) >> 8;

    			if(TotalDCDDataCnt > HAB_DCD_BYTES_MAX)
    			{
    				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DCD data excceeds max limit!!!"));
    				return FALSE;
    			}

    			if ( !DCDWrite((PUCHAR)(pDCDRegion),TotalDCDDataCnt) )
    			{
    				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to initialize memory!"));
    				return FALSE;
    			}
    		}
    		else
    		{
    			DWORD DCDDataCount = ((EndianSwap(*pDCDRegion) & 0x00FFFF00)>>8)/sizeof(DWORD);

    			PRomFormatDCDData pRomFormatDCDData = (PRomFormatDCDData)malloc(DCDDataCount*sizeof(RomFormatDCDData));
    			//There are several segments in DCD region, we have to extract DCD data with segment unit.
    			//i.e. Below code shows how non-DCD data is inserted to finish a delay operation, we must avoid counting them in.
    			/*DCD_BE 0xCF001024   ; Tag = 0xCF, Len = 1*12+4=0x10, parm = 4

    			; Wait for divider to update
    			DCD_BE 0x53FD408C   ; Address
    			DCD_BE 0x00000004   ; Mask
    			DCD_BE 0x1FFFFFFF   ; Loop

    			DCD_BE 0xCC031C04   ; Tag = 0xCC, Len = 99*8+4=0x031c, parm = 4*/

    			DWORD CurDCDDataCount = 1, ValidRegCount=0;

    			//Quit if current DCD data count reaches total DCD data count.
    			while(CurDCDDataCount < DCDDataCount)
    			{
    				DWORD DCDCmdHdr = EndianSwap(*(pDCDRegion+CurDCDDataCount));
    				CurDCDDataCount++;
    				if((DCDCmdHdr >> 24) == HAB_CMD_WRT_DAT)
    				{
    					DWORD DCDDataSegCount = (((DCDCmdHdr & 0x00FFFF00) >>8) -4)/sizeof(ImgFormatDCDData);
    					PImgFormatDCDData pImgFormatDCDData = (PImgFormatDCDData)(pDCDRegion + CurDCDDataCount);
    					//Must convert image dcd data format to ROM dcd format.
    					for(DWORD i=0; i<DCDDataSegCount; i++)
    					{
    						pRomFormatDCDData[ValidRegCount].addr = pImgFormatDCDData[i].Address;
    						pRomFormatDCDData[ValidRegCount].data = pImgFormatDCDData[i].Data;
    						pRomFormatDCDData[ValidRegCount].format = EndianSwap(32);
    						ValidRegCount++;
    						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("{%d,0x%08x,0x%08x}, "),32, EndianSwap(pImgFormatDCDData[i].Address),EndianSwap(pImgFormatDCDData[i].Data));
    					}
    					CurDCDDataCount+=DCDDataSegCount*sizeof(ImgFormatDCDData)/sizeof(DWORD);
    				}
    				else if((DCDCmdHdr >> 24) == HAB_CMD_CHK_DAT)
    				{
    					CurDCDDataCount += (((DCDCmdHdr & 0x00FFFF00) >>8) -4)/sizeof(DWORD);
    				}
    			}

    			if ( !DCDWrite((PUCHAR)(pRomFormatDCDData),ValidRegCount) )
    			{
    				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to initialize memory!"));
    				free(pRomFormatDCDData);
    				return FALSE;
    			}
    			free(pRomFormatDCDData);
    		}
        }

		//---------------------------------------------------------
		//Download boot data to ram
		PhyRAMAddr4KRL = pIVT->SelfAddr - ImgIVTOffset;
		pIVT->DCDAddress = 0;

		if (!TransData(PhyRAMAddr4KRL, (unsigned int)dwFileSize, (PUCHAR)((uint64_t)pDataBuf)))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): TransData(0x%X, 0x%X,0x%X) failed."),
				PhyRAMAddr4KRL, dwFileSize, pDataBuf);
			goto ERR_HANDLE;
		}

        m_jumpAddr = pIVT->SelfAddr;
        //m_jumpAddr = pIVT->ImageStartAddr;
		/*if(!AddIvtHdr(pIVT->ImageStartAddr))
		{
			TRACE(_T("RunPlugIn(): Failed to addhdr to RAM address: 0x%x.\n"), pIVT->ImageStartAddr);
			goto ERR_HANDLE;
		}*/
	}

    //Clear device handle and report id
    CloseMxHidHandle();
	if(pDataBuf != NULL)
	{
		free(pDataBuf);
	}
    return TRUE;

ERR_HANDLE:
    //Clear device handle and report id
    CloseMxHidHandle();
	if(pDataBuf != NULL)
	{
		free(pDataBuf);
	}
    return FALSE;
}

BOOL MxHidDevice::TransData(UINT address, UINT byteCount, const unsigned char * pBuf)
{
    SDPCmd SDPCmd;

    SDPCmd.command = ROM_KERNEL_CMD_WR_FILE;
    SDPCmd.dataCount = byteCount;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = address;
    LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T(" Trans Data address is %x \n"),address);
	if(!SendCmd(&SDPCmd))
		return FALSE;



    UINT MaxHidTransSize = 1025 - 1;
    UINT TransSize;

    while(byteCount > 0)
    {
        TransSize = (byteCount > MaxHidTransSize) ? MaxHidTransSize : byteCount;
	if(!SendData(pBuf,TransSize ))
			return FALSE;

        byteCount -= TransSize;
        pBuf += TransSize;
    }

    //below function should be invoked for mx50
	if ( !GetCmdAck(ROM_STATUS_ACK) )
	{
		return FALSE;
	}

	return TRUE;
}

BOOL MxHidDevice::AddIvtHdr(UINT32 ImageStartAddr)
{
	UINT FlashHdrAddr;

	//transfer length of ROM_TRANSFER_SIZE is a must to ROM code.
	unsigned char FlashHdr[ROM_TRANSFER_SIZE] = { 0 };

	// create a header and append the data

	PIvtHeader pIvtHeader = (PIvtHeader)FlashHdr;

	FlashHdrAddr = ImageStartAddr;// - sizeof(IvtHeader);

    //Read the data first
	if ( !ReadData(FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("AddIvtHdr(): ReadData(0x%X, 0x%X, 0x%X) failed."),
            FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr);
		return FALSE;
	}

    if(pIvtHeader->IvtBarker != IVT_BARKER_HEADER)
    {
		FlashHdrAddr = ImageStartAddr - sizeof(IvtHeader);
		//Read the data first
		if ( !ReadData(FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr) )
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("ReadData(0x%X, 0x%X, 0x%X) failed.\n"),
				FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr);
			return FALSE;
		}
    	//Add IVT header to the image.
        //Clean the IVT header region
#ifdef __linux__
	memset(FlashHdr,0,sizeof(IvtHeader));
#else
        ZeroMemory(FlashHdr, sizeof(IvtHeader));
#endif

        //Fill IVT header parameter
    	pIvtHeader->IvtBarker = IVT_BARKER_HEADER;
    	pIvtHeader->ImageStartAddr = ImageStartAddr;
    	pIvtHeader->SelfAddr = FlashHdrAddr;

        //Send the IVT header to destiny address
    	if ( !TransData(FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr) )
    	{
    		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("AddIvtHdr(): TransData(0x%X, 0x%X, 0x%X) failed.\n"),
                FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr);
    		return FALSE;
    	}

        //Verify the data
       	unsigned char Tempbuf[ROM_TRANSFER_SIZE] = { 0 };
        if ( !ReadData(FlashHdrAddr, ROM_TRANSFER_SIZE, Tempbuf) )
    	{
    		return FALSE;
    	}

        if(memcmp(FlashHdr, Tempbuf, ROM_TRANSFER_SIZE)!= 0 )
    	{
    		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DownloadImage(): TransData(0x%X, 0x%X, 0x%X) failed.\n"),
                FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr);
    		return FALSE;
    	}
    }

    m_jumpAddr = FlashHdrAddr;

	return TRUE;
}

BOOL MxHidDevice::ReadData(UINT address, UINT byteCount, unsigned char * pBuf)
{
    SDPCmd SDPCmd;

    SDPCmd.command = ROM_KERNEL_CMD_RD_MEM;
    SDPCmd.dataCount = byteCount;
    SDPCmd.format = 32;
    SDPCmd.data = 0;
    SDPCmd.address = address;

	if(!SendCmd(&SDPCmd))
		return FALSE;

	if(!GetHABType())
		return FALSE;

    UINT MaxHidTransSize = 1025 -1;

    while(byteCount > 0)
    {
        UINT TransSize = (byteCount > MaxHidTransSize) ? MaxHidTransSize : byteCount;

        memset((UCHAR *)m_pReadReport, 0, 1025);

        if ( Read( (UCHAR *)m_pReadReport, 1025 )  != ERROR_SUCCESS)
        {
            return FALSE;
        }

        memcpy(pBuf, m_pReadReport->Payload, TransSize);
        pBuf += TransSize;

        byteCount -= TransSize;
    }

	return TRUE;
}

BOOL MxHidDevice::Download(PImageParameter pImageParameter, UCHAR *pFileDataBuf, ULONGLONG dwFileSize, int cmdOpIndex)
{
	//Create device handle and report id
    OpenMxHidHandle();

	UCHAR* pBuffer = pFileDataBuf;

	DWORD byteIndex, numBytesToWrite = 0;
	for ( byteIndex = 0; byteIndex < dwFileSize; byteIndex += numBytesToWrite )
	{
		// Get some data
		numBytesToWrite = (DWORD)std::min((long long unsigned int)MAX_SIZE_PER_DOWNLOAD_COMMAND, dwFileSize - byteIndex);

		if (!TransData(pImageParameter->PhyRAMAddr4KRL + byteIndex, numBytesToWrite, pBuffer + byteIndex))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Download(): TransData(0x%X, 0x%X, 0x%X, 0x%X) failed."),
                pImageParameter->PhyRAMAddr4KRL + byteIndex, numBytesToWrite, pImageParameter->loadSection, pBuffer + byteIndex);
			goto ERR_HANDLE;
		}
	}

	// If we are downloading to DCD or CSF, we don't need to send
	if ( pImageParameter->loadSection == MemSectionDCD || pImageParameter->loadSection == MemSectionCSF )
	{
		return TRUE;
	}

	if( pImageParameter->setSection == MemSectionAPP)
    {
    	UINT32 ImageStartAddr = 0;
    	if(_chipFamily == MX50)
    	{
    		ImageStartAddr = pImageParameter->PhyRAMAddr4KRL + pImageParameter->CodeOffset;
    	}

        if(!AddIvtHdr(ImageStartAddr))
        {
            LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DownloadImage(): AddHdr(0x%x) failed."),ImageStartAddr);
            goto ERR_HANDLE;
        }
    }

    //Clear device handle and report id
    CloseMxHidHandle();

    return TRUE;

ERR_HANDLE:
    //Clear device handle and report id
    CloseMxHidHandle();

	return FALSE;
}
