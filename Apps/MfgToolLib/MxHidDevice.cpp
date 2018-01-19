/*
 * Copyright 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <Assert.h>
#include <cfgmgr32.h>
#include <basetyps.h>
#include <setupapi.h>
#include <initguid.h>
#include <hidclass.h>
#include "Device.h"
#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"

#include "MxHidDevice.h"
#include "libfdt.h"

//#define DCD_WRITE
//static PIvtHeader g_pIVT = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
MxHidDevice::MxHidDevice(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
: Device(deviceClass, devInst, path, handle)
{
	m_hid_drive_handle = INVALID_HANDLE_VALUE;
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
	CString filter;
	OP_STATE_ARRAY *pOpStates = GetOpStates((MFGLIB_VARS *)m_pLibHandle);
	OP_STATE_ARRAY::iterator it = pOpStates->begin();
	COpState *pCurrentState = NULL;
	for(; it!=pOpStates->end(); it++)
	{
		filter.Format(_T("vid_%04x&pid_%04x"), (*it)->uiVid, (*it)->uiPid);
		path.MakeUpper();
		filter.MakeUpper();
		if( path.Find(filter) != -1 )
		{	//find
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
	case DEV_HID_MX6ULL:
		_chipFamily = MX6ULL;
		break;
	case DEV_HID_MX6SLL:
		_chipFamily = MX6SLL;
		break;
	case DEV_HID_MX7ULP:
		_chipFamily = MX7ULP;
		break;
	case DEV_HID_K32H844P:
		_chipFamily = K32H844P;
		break;
	case DEV_HID_MX8MQ:
		_chipFamily = MX8MQ;
		break;
	case DEV_HID_MX8QM:
		_chipFamily = MX8QM;
		break; 
	case DEV_HID_MX8QXP:
		_chipFamily = MX8QXP;
		break;
	case DEV_HID_MXRT102X:
		_chipFamily = MXRT102X;
		break;
	case DEV_HID_MXRT105X:
		_chipFamily = MXRT105X;
		break;
	default:
		_chipFamily = MX50;
		break;
	}

	m_habState = HabUnknown;

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

	// Open the device 
    if (!OpenUSBHandle(&m_hid_drive_handle,_path.get()))
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR,  _T(" ERROR: OpenUSBHandle failed."));
		return FALSE;
    }

	return TRUE;
}

BOOL MxHidDevice::OpenUSBHandle(HANDLE* pHandle, CString pipePath)
{
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
}

#ifndef DCD_WRITE
BOOL MxHidDevice::InitMemoryDevice(CString filename)
{
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
		for ( ; cmd != cmds.end(); ++cmd )
		{
			MemoryInitCommand* pCmd = (MemoryInitCommand*)(*cmd);
            SDPCmd.format = pCmd->GetFormat();
            SDPCmd.data = pCmd->GetData();
            SDPCmd.address = pCmd->GetAddress();
			if ( !WriteReg(&SDPCmd) )
            {
                TRACE("In InitMemoryDevice(): write memory failed\n");
                CloseMxHidHandle();
                return FALSE;
            }
		}
	}

    //Clear device handle and report id    
    CloseMxHidHandle();
    
	return TRUE;
}

#else

BOOL MxHidDevice::InitMemoryDevice(CString filename)
{
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
		}
		free(pMemPara);
	}

	//Clear device handle and report id    
    CloseMxHidHandle();

	return TRUE;
}
#endif
BOOL MxHidDevice::CloseMxHidHandle()
{
	if( m_hid_drive_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hid_drive_handle);
        m_hid_drive_handle = INVALID_HANDLE_VALUE;
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
		UINT MaxHidTransSize = m_Capabilities.OutputReportByteLength - 1;

		if(this->_chipFamily >= MX8QM)
			RegCount = ((RegCount + MaxHidTransSize - 1) / MaxHidTransSize) * MaxHidTransSize;

		SDPCmd.dataCount = RegCount;
		if (this->_chipFamily == MX7ULP)
			SDPCmd.address = 0x2f018000;
		else if (this->_chipFamily == K32H844P)
			SDPCmd.address = 0x8000;
		else if (this->_chipFamily == MX8QM || this->_chipFamily == MX8QXP)
			SDPCmd.address = 0x2000e400;
		else
			SDPCmd.address = 0x00910000;//IRAM free space

		if(!SendCmd(&SDPCmd))
		{
			return FALSE;
		}
	    
		while(RegCount > 0)
		{
			UINT ByteCntTransfered = (RegCount > MaxHidTransSize) ? MaxHidTransSize : RegCount;
			RegCount -= ByteCntTransfered;

			if(!SendData(DataBuf, ByteCntTransfered))
			{
				return FALSE;
			}

			DataBuf += ByteCntTransfered;
			SDPCmd.address += ByteCntTransfered;
		}

		if (!GetCmdAck(ROM_WRITE_ACK))
		{
			return FALSE;
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
    memset((UCHAR *)m_pWriteReport, 0, m_Capabilities.OutputReportByteLength);
    m_pWriteReport->ReportId = (unsigned char)REPORT_ID_SDP_CMD;
    PLONG pTmpSDPCmd = (PLONG)(m_pWriteReport->Payload);

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
	int    nBytesWrite; // for bytes actually written

	if( !WriteFile(m_hid_drive_handle, _buf, _size, (PULONG) &nBytesWrite, NULL) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("MxHidDevice::Write() Error writing to device 0x%x."), GetLastError());
		return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
	}

    return ERROR_SUCCESS;
}

// Read from HID device
int MxHidDevice::Read(void* _buf, UINT _size)
{
	int    nBytesRead; // for bytes actually read

	Sleep(35);

	if( !ReadFile(m_hid_drive_handle, _buf, _size, (PULONG) &nBytesRead, NULL) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("MxHidDevice::Read() Error reading from device 0x%x."), GetLastError());
		return GetLastError();
	}
	return ERROR_SUCCESS;
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
	if ( Write((unsigned char *)m_pWriteReport, m_Capabilities.OutputReportByteLength) != ERROR_SUCCESS)
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
	if (Write((unsigned char *)m_pWriteReport, m_Capabilities.OutputReportByteLength) != ERROR_SUCCESS)
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
    memset((UCHAR *)m_pReadReport, 0, m_Capabilities.InputReportByteLength);

    //Get Report4, Device to Host:
	if ( Read( (UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength ) != ERROR_SUCCESS)
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
    memset((UCHAR *)m_pReadReport, 0, m_Capabilities.InputReportByteLength);

    //Get Report3, Device to Host:
    //4 bytes HAB mode indicating Production/Development part
	if ( Read( (UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength )  != ERROR_SUCCESS)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to read HAB type from ROM!!!"));  
		return FALSE;
	}
	if ( (*(unsigned int *)(m_pReadReport->Payload) != HabEnabled)  && 
		 (*(unsigned int *)(m_pReadReport->Payload) != HabDisabled) ) 
	{
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("HAB type mismatch: 0x%x!!!"), *(unsigned int *)(m_pReadReport->Payload));
		m_habState = HabUnknown;
		return FALSE;	
	}

	m_habState = *(unsigned int *)m_pReadReport->Payload == HabEnabled ? HabEnabled : HabDisabled;
	return TRUE;
}

MxHidDevice::HAB_t MxHidDevice::GetHABState()
{
	if (m_habState != HabUnknown)
	{
		return  m_habState;
	}
	
	// Send "Error Status" command to get the HAB state.
	// "Error Status" doesn't write any data to ROM.
	// "Read Memory" can be disabled by Fuse.
	SDPCmd SDPCmd;

	SDPCmd.command = ROM_KERNEL_CMD_ERROR_STATUS;
	SDPCmd.dataCount = 0;
	SDPCmd.format = 0;
	SDPCmd.data = 0;
	SDPCmd.address = 0;

	OpenMxHidHandle();

	if (!SendCmd(&SDPCmd))
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to send error status command to ROM!!!"));
		goto GetHABStateFailed;
	}

	if (!GetHABType())
		goto GetHABStateFailed;

	// Just read the left data. HAB state is already got, and no need to check to result.
	Read((UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength);

	OpenMxHidHandle();
	return m_habState;

GetHABStateFailed:
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to Get HAB state!!!"));
	OpenMxHidHandle();
	return HabUnknown;
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

BOOL MxHidDevice::Jump(UINT RAMAddress, BOOL isPlugin)
{
    SDPCmd SDPCmd;
    CString LogStr;

	if(this->_chipFamily >= MX7D) {

		SDPCmd.command = ROM_KERNEL_CMD_SKIP_DCD_HEADER;
		SDPCmd.dataCount = 0;
		SDPCmd.format = 0;
		SDPCmd.data = 0;
		SDPCmd.address = 0;

		if(!SendCmd(&SDPCmd))
			return FALSE;

		if (!GetCmdAck(ROM_OK_ACK))
		{
			return FALSE;
		}
	}

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

	if (isPlugin)
	{
		GetHABType();

		Sleep(300); // Wait for plugin finish run

		GetCmdAck(ROM_OK_ACK); /*omit rom return error code, if use plug-in mode*/
	}
	else
	{
		/* ROM jump to address, no chance send back ACK*/
	}

//	if(!GetHABType())
//    {
//        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Failed to get HAB type from ROM, ignoredro!!!"));
//        return FALSE;
//    }
	
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("*********MxHidDevice[%p] Jump to Ramkernel successfully!**********"), this);
	
	return TRUE;
}


BOOL MxHidDevice::RunDCD(DWORD* pDCDRegion)
{
	if (pDCDRegion == NULL)
		return FALSE;

	//i.e. DCD_BE  0xD2020840              ;DCD_HEADR Tag=0xd2, len=64*8+4+4, ver= 0x40    
	//i.e. DCD_BE  0xCC020404              ;write dcd cmd headr Tag=0xcc, len=64*8+4, param=4
	//The first 2 32bits data in DCD region is used to give some info about DCD data.
	//Here big endian format is used, so it must be converted.
	if (HAB_TAG_DCD != EndianSwap(*pDCDRegion) >> 24)
	{
		TRACE(CString("DCD header tag doesn't match!\n"));
		return FALSE;
	}

	if (this->_chipFamily != MX50)
	{
		//The DCD_WRITE command handling was changed from i.MX508.
		//Now the DCD is  performed by HAB and therefore the format of DCD is the same format as in regular image. 
		//The DCD_WRITE parameters require size and address. Size is the size of entire DCD file including header. 
		//Address is the temporary address that USB will use for storing the DCD file before processing.

		DWORD DCDHeader = EndianSwap(*pDCDRegion);
		//Total dcd data bytes:
		INT TotalDCDDataCnt = (DCDHeader & 0x00FFFF00) >> 8;

		if (!DCDWrite((PUCHAR)(pDCDRegion), TotalDCDDataCnt))
		{
			_tprintf(_T("Failed to initialize memory!\r\n"));
			return FALSE;
		}
	}
	else
	{
		DWORD DCDDataCount = ((EndianSwap(*pDCDRegion) & 0x00FFFF00) >> 8) / sizeof(DWORD);

		PRomFormatDCDData pRomFormatDCDData = (PRomFormatDCDData)malloc(DCDDataCount * sizeof(RomFormatDCDData));
		//There are several segments in DCD region, we have to extract DCD data with segment unit.
		//i.e. Below code shows how non-DCD data is inserted to finish a delay operation, we must avoid counting them in.
		/*DCD_BE 0xCF001024   ; Tag = 0xCF, Len = 1*12+4=0x10, parm = 4

		; Wait for divider to update
		DCD_BE 0x53FD408C   ; Address
		DCD_BE 0x00000004   ; Mask
		DCD_BE 0x1FFFFFFF   ; Loop

		DCD_BE 0xCC031C04   ; Tag = 0xCC, Len = 99*8+4=0x031c, parm = 4*/

		DWORD CurDCDDataCount = 1, ValidRegCount = 0;

		//Quit if current DCD data count reaches total DCD data count.
		while (CurDCDDataCount < DCDDataCount)
		{
			DWORD DCDCmdHdr = EndianSwap(*(pDCDRegion + CurDCDDataCount));
			CurDCDDataCount++;
			if ((DCDCmdHdr >> 24) == HAB_CMD_WRT_DAT)
			{
				DWORD DCDDataSegCount = (((DCDCmdHdr & 0x00FFFF00) >> 8) - 4) / sizeof(ImgFormatDCDData);
				PImgFormatDCDData pImgFormatDCDData = (PImgFormatDCDData)(pDCDRegion + CurDCDDataCount);
				//Must convert image dcd data format to ROM dcd format.
				for (DWORD i = 0; i < DCDDataSegCount; i++)
				{
					pRomFormatDCDData[ValidRegCount].addr = pImgFormatDCDData[i].Address;
					pRomFormatDCDData[ValidRegCount].data = pImgFormatDCDData[i].Data;
					pRomFormatDCDData[ValidRegCount].format = EndianSwap(32);
					ValidRegCount++;
					TRACE(CString("{%d,0x%08x,0x%08x},\n"), 32, EndianSwap(pImgFormatDCDData[i].Address), EndianSwap(pImgFormatDCDData[i].Data));
				}
				CurDCDDataCount += DCDDataSegCount * sizeof(ImgFormatDCDData) / sizeof(DWORD);
			}
			else if ((DCDCmdHdr >> 24) == HAB_CMD_CHK_DAT)
			{
				CurDCDDataCount += (((DCDCmdHdr & 0x00FFFF00) >> 8) - 4) / sizeof(DWORD);
			}
		}

		if (!DCDWrite((PUCHAR)(pRomFormatDCDData), ValidRegCount))
		{
			_tprintf(_T("Failed to initialize memory!\r\n"));
			free(pRomFormatDCDData);
			return FALSE;
		}
		free(pRomFormatDCDData);
	}
	return TRUE;
}

int FitGetImageNodeOffset(UCHAR *fit, int image_node, char * type, int index)
{
	int offset;
	int len;
	const char * config;
	int config_node = 0;

	offset = fdt_path_offset(fit, "/configurations");
	if (offset < 0)
	{
		TRACE(_T("Can't found configurations\n"));
		return offset;
	}

	config = (const char *)fdt_getprop(fit, offset, "default", &len);
	for (int node = fdt_first_subnode(fit, offset); node >= 0; node = fdt_next_subnode(fit, node))
	{
		const char *name = fdt_get_name(fit, node, &len);
		if (strcmp(config, name) == 0)
		{
			config_node = node;
			break;
		}
	}

	if (config_node <= 0)
	{
		TRACE(_T("can't find default config"));
		return config_node;
	}

	char * name = (char*)fdt_getprop(fit, config_node, type, &len);
	if (!name)
	{
		TRACE(_T("can't find type\n"));
		return -1;
	}

	char *str = name;
	for (int i = 0; i < index; i++)
	{
		str = strchr(str, '\0') + 1;
		if (!str || (str - name) > len)
		{
			TRACE(_T("can't found index %d node"), index);
			return -1;
		}
	}

	return fdt_subnode_offset(fit, image_node, str);
}

int FitGetIntProp(UCHAR *fit, int node, char *prop, int *value)
{
	int len;
	const void * data = fdt_getprop(fit, node, prop, &len);
	if (data == NULL)
	{
		TRACE(_T("failure to get load prop\n"));
		return -1;
	}
	*value = fdt32_to_cpu(*(int*)data);
	return 0;
}

BOOL MxHidDevice::LoadFitImage(UCHAR *fit, ULONGLONG dataCount)
{

	int image_offset = fdt_path_offset(fit, "/images");
	if (image_offset < 0)
	{
		TRACE(_T("Can't find /images\n"));
		return FALSE;
	}

	int depth = 0;
	int count = 0;
	int node_offset = 0;
	int entry = 0;

	int size = fdt_totalsize(fit);
	size = (size + 3) & ~3;
	int base_offset = (size + 3) & ~3;

	int firmware;
	firmware = FitGetImageNodeOffset(fit, image_offset, "firmware", 0);
	if (firmware < 0)
	{
		TRACE(_T("can't find firmware\n"));
		return FALSE;
	}

	int firmware_load, offset, firmware_len;

	if (!FitGetIntProp(fit, firmware, "data-offset", &offset))
	{
		offset = base_offset + offset;
	}
	else if (FitGetIntProp(fit, firmware, "data-position", &offset))
	{
		TRACE(_T("can't find data-offset or data-position\n"));
		return FALSE;
	}

	if (FitGetIntProp(fit, firmware, "load", &firmware_load) ||
		FitGetIntProp(fit, firmware, "data-size", &firmware_len))
	{
		TRACE(_T("can't find load data-len\n"));
		return FALSE;
	}

	if (!Download(fit + offset, firmware_len, firmware_load))
		return FALSE;


	int fdt;
	fdt = FitGetImageNodeOffset(fit, image_offset, "fdt", 0);
	if (fdt < 0)
	{
		TRACE(_T("can't find fdt\n"));
		return FALSE;
	}

	int fdt_load, fdt_size;

	if (!FitGetIntProp(fit, fdt, "data-offset", &offset))
	{
		offset = base_offset + offset;
	}
	else if (FitGetIntProp(fit, fdt, "data-position", &offset))
	{
		TRACE(_T("can't find data-offset or data-position\n"));
		return FALSE;
	}

	if (FitGetIntProp(fit, fdt, "data-size", &fdt_size))
	{
		TRACE(_T("can't find data-len\n"));
		return FALSE;
	}

	fdt_load = firmware_load + firmware_len;

	if (!Download(fit + offset, fdt_size, fdt_load))
		return FALSE;

	int load_node;
	int index = 0;
	do
	{
		load_node = FitGetImageNodeOffset(fit, image_offset, "loadables", index);
		if (load_node >= 0)
		{
			int load, offset, len, entry;

			if (!FitGetIntProp(fit, load_node, "data-offset", &offset))
			{
				offset = base_offset + offset;
			}
			else if (FitGetIntProp(fit, load_node, "data-position", &offset))
			{
				TRACE(_T("can't find data-offset or data-position\n"));
				return FALSE;
			}

			if (FitGetIntProp(fit, load_node, "load", &load) ||
				FitGetIntProp(fit, load_node, "data-size", &len))
			{
				TRACE(_T("can't find load data-len\n"));
				return FALSE;
			}
			if (!Download(fit + offset, len, load))
				return FALSE;

			if (FitGetIntProp(fit, load_node, "entry", &entry) == 0)
			{
				AddIvtHdr(entry);
			}
		}
		index++;
	} while (load_node >= 0);

	return TRUE;
}

BOOL MxHidDevice::RunPlugIn(UCHAR *pFileDataBuf, ULONGLONG dwFileSize)
{
	UCHAR* pDataBuf = NULL;
	DWORD * pPlugIn = NULL;
	DWORD PlugInDataOffset= 0,ImgIVTOffset= 0,BootDataImgAddrIndex= 0,PhyRAMAddr4KRL= 0;
	DWORD PlugInAddr = 0;
	PIvtHeader pIVT = NULL,pIVT2 = NULL;
	ULONGLONG dwsize = dwFileSize;

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

	dwFileSize = (dwFileSize + ROM_ECC_SIZE_ALIGN - 1) / ROM_ECC_SIZE_ALIGN * ROM_ECC_SIZE_ALIGN;

	pDataBuf = (UCHAR*)malloc((size_t)dwFileSize);
	if(pDataBuf == NULL)
	{
		return FALSE;
	}
	memcpy(pDataBuf, pFileDataBuf, (size_t)dwsize);

	if (_chipFamily >= MX8QM)
	{
		return RunMxMultiImg(pDataBuf, dwFileSize);
	}

	PBootData pPluginDataBuf;

	//pDataBuf = pFileDataBuf;
	//Search for IVT
	while(1)
	{
		pPlugIn = (DWORD *)pDataBuf;
		//ImgIVTOffset indicates the IVT's offset from the beginning of the image.
		while (ImgIVTOffset < dwFileSize &&
			(pPlugIn[ImgIVTOffset / sizeof(DWORD)] != IVT_BARKER_HEADER &&
				pPlugIn[ImgIVTOffset / sizeof(DWORD)] != IVT_BARKER2_HEADER
				))
		{
			ImgIVTOffset += 0x100;
		}
		if (ImgIVTOffset >= dwFileSize)
		{
			goto ERR_HANDLE;
		}

		//Now we find IVT
		pIVT = (PIvtHeader)(pPlugIn + ImgIVTOffset / sizeof(DWORD));
		//Now we have to judge DCD way or plugin way used in the image
		//The method is to check plugin flag in boot data region
		// IVT boot data format
		//   0x00    IMAGE START ADDR
		//   0x04    IMAGE SIZE
		//   0x08    PLUGIN FLAG
		pPluginDataBuf = (PBootData)(pPlugIn + ImgIVTOffset / sizeof(DWORD) + (pIVT->BootData - pIVT->SelfAddr) / sizeof(DWORD));

		if (pPluginDataBuf->PluginFlag & 0xFFFFFFFE)
			ImgIVTOffset += 0x100;
		else
			break;
	} 

	if(pPluginDataBuf->PluginFlag || pIVT->Reserved)
	{
		//Plugin mode
	  
		//---------------------------------------------------------
		//Run plugin in IRAM
		//Download plugin data into IRAM.
		PlugInAddr = pIVT->ImageStartAddr;
		PlugInDataOffset = pIVT->ImageStartAddr - pIVT->SelfAddr;
		if (!TransData(pIVT->SelfAddr, pPluginDataBuf->ImageSize, (PUCHAR)((DWORD)pIVT)))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): TransData(0x%X, 0x%X,0x%X) failed."),
				PlugInAddr, pPluginDataBuf->ImageSize, ((DWORD)pIVT + PlugInDataOffset));
			goto ERR_HANDLE;
		}
		
		if( !Jump(pIVT->SelfAddr, true))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): Failed to jump to RAM address: 0x%x."), m_jumpAddr);
			goto ERR_HANDLE;
		}

		if (pIVT->Reserved)
		{
				Sleep(200);
				Uboot_header *pImage = (Uboot_header*)(pDataBuf + pIVT->Reserved + ImgIVTOffset);
				if (EndianSwap(pImage->magic) == 0x27051956)
				{
					PhyRAMAddr4KRL = EndianSwap(pImage->load);
					int CodeOffset = pIVT->Reserved + sizeof(Uboot_header) + ImgIVTOffset;
					unsigned int ExecutingAddr = EndianSwap(pImage->entry);

					if (!TransData(PhyRAMAddr4KRL, (unsigned int)(dwFileSize - CodeOffset), pDataBuf + CodeOffset))
					{
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): TransData(0x%X, 0x%X,0x%X) failed.\n"),
							PhyRAMAddr4KRL, dwFileSize, pDataBuf);
						goto ERR_HANDLE;
					}

					AddIvtHdr(ExecutingAddr);
				}
				else if (fdt_check_header((UCHAR*)pImage) == 0)
				{
					if (!LoadFitImage((UCHAR*)pImage, dwFileSize - ( (UCHAR*)pImage - (UCHAR*)pFileDataBuf)))
					{
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Load Fit image failure\n"));
						goto ERR_HANDLE;
					}

				}
				else
				{
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("uBoot Magic number wrong\n"));
					goto ERR_HANDLE;
				}
		}
		else
		{
			//---------------------------------------------------------
			//Download eboot to ram		
			//Search IVT2.
			//ImgIVTOffset indicates the IVT's offset from the beginning of the image.
			DWORD IVT2Offset = ImgIVTOffset + sizeof(IvtHeader);

			while (IVT2Offset < dwFileSize &&
				(pPlugIn[IVT2Offset / sizeof(DWORD)] != IVT_BARKER_HEADER &&
					pPlugIn[IVT2Offset / sizeof(DWORD)] != IVT_BARKER2_HEADER))
				IVT2Offset += sizeof(DWORD);

			if (IVT2Offset >= dwFileSize)
			{
				goto ERR_HANDLE;
			}
			pIVT2 = (PIvtHeader)(pPlugIn + IVT2Offset / sizeof(DWORD));
			BootDataImgAddrIndex = (DWORD *)pIVT2 - pPlugIn;
			BootDataImgAddrIndex += (pIVT2->BootData - pIVT2->SelfAddr) / sizeof(DWORD);
			PhyRAMAddr4KRL = pPlugIn[BootDataImgAddrIndex] + IVT_OFFSET - ImgIVTOffset;
			if (!TransData(PhyRAMAddr4KRL, (unsigned int)dwFileSize, (PUCHAR)((DWORD)pDataBuf)))
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("RunPlugIn(): TransData(0x%X, 0x%X,0x%X) failed.\n"),
					PhyRAMAddr4KRL, dwFileSize, pDataBuf);
				goto ERR_HANDLE;
			}

			m_jumpAddr = pIVT2->SelfAddr;
		}
	}
	else
	{
        if(NULL != pIVT->DCDAddress)
        {
			DWORD * pDCDRegion = pPlugIn + ImgIVTOffset / sizeof(DWORD) + (pIVT->DCDAddress - pIVT->SelfAddr) / sizeof(DWORD);

			RunDCD(pDCDRegion);
        }
		//---------------------------------------------------------
			//Download boot data to ram
		PhyRAMAddr4KRL = pIVT->SelfAddr - ImgIVTOffset;

		if (this->_chipFamily < MX7D)
			pIVT->DCDAddress = 0;

		if (!TransData(PhyRAMAddr4KRL, (unsigned int)dwFileSize, (PUCHAR)((DWORD)pDataBuf)))
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

	UINT MaxHidTransSize = m_Capabilities.OutputReportByteLength - 1;

	if (_chipFamily >= MX8QM)
		byteCount = ((byteCount + MaxHidTransSize - 1) / MaxHidTransSize) * MaxHidTransSize;

    SDPCmd.command = ROM_KERNEL_CMD_WR_FILE;
    SDPCmd.dataCount = byteCount;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = address;

	if(!SendCmd(&SDPCmd))
		return FALSE;
    
    Sleep(10);

    UINT TransSize;
    
    while(byteCount > 0)
    {
        TransSize = (byteCount > MaxHidTransSize) ? MaxHidTransSize : byteCount;

		if(!SendData(pBuf, TransSize))
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
        ZeroMemory(FlashHdr, sizeof(IvtHeader));
        
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

    UINT MaxHidTransSize = m_Capabilities.InputReportByteLength -1;
    
    while(byteCount > 0)
    {
        UINT TransSize = (byteCount > MaxHidTransSize) ? MaxHidTransSize : byteCount;

        memset((UCHAR *)m_pReadReport, 0, m_Capabilities.InputReportByteLength);

        if ( Read( (UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength )  != ERROR_SUCCESS)
        {
            return FALSE;
        }

        memcpy(pBuf, m_pReadReport->Payload, TransSize);
        pBuf += TransSize;

        byteCount -= TransSize;
    }

	return TRUE;
}

DWORD MxHidDevice::GetIvtOffset(DWORD *start, ULONGLONG dataCount)
{
	//Search for a valid IVT Code starting from the given address
	DWORD ImgIVTOffset = 0;

	while (ImgIVTOffset < dataCount &&
		(start[ImgIVTOffset / sizeof(DWORD)] != IVT_BARKER_HEADER &&
			start[ImgIVTOffset / sizeof(DWORD)] != IVT_BARKER2_HEADER &&
			start[ImgIVTOffset / sizeof(DWORD)] != MX8_IVT_BARKER_HEADER &&
			start[ImgIVTOffset / sizeof(DWORD)] != MX8_IVT2_BARKER_HEADER
			))
		ImgIVTOffset += 0x100;

	if (ImgIVTOffset >= dataCount)
		return -1;

	return ImgIVTOffset;
}

BOOL MxHidDevice::RunMxMultiImg(UCHAR* pBuffer, ULONGLONG dataCount)
{
	DWORD *pImg = (DWORD*)pBuffer;
	DWORD * pDCDRegion;
	DWORD ImageOffset = 0;
	DWORD ImgIVTOffset = GetIvtOffset(pImg, MX8_INITIAL_IMAGE_SIZE);
	PIvtHeaderV2 pIVT = NULL, pIVT2 = NULL;
	PBootDataV2 pBootData1 = NULL, pBootData2 = NULL;
	unsigned int i;

	//if we did not find a valid IVT within INITIAL_IMAGE_SIZE we have a non zero ImageOffset
	if (ImgIVTOffset < 0)
	{
		TRACE(_T("Not a valid image.\n"));
		return FALSE;
	}

	pImg += ImageOffset / sizeof(DWORD);
	if (pImg[ImgIVTOffset / sizeof(DWORD)] != MX8_IVT_BARKER_HEADER && pImg[ImgIVTOffset / sizeof(DWORD)] != MX8_IVT2_BARKER_HEADER)
	{
		TRACE(_T("Not a valid image.\n"));
		return FALSE;
	}

	pIVT = (PIvtHeaderV2)(pImg + ImgIVTOffset / sizeof(DWORD));

	if (pImg[ImgIVTOffset / sizeof(DWORD)] == MX8_IVT2_BARKER_HEADER)
	{
		pIVT2 = (PIvtHeaderV2)(pImg + ImgIVTOffset + pIVT->Next / sizeof(DWORD));
	}
	else
	{
		pIVT2 = pIVT + 1; // The IVT for the second container is immediatly after IVT1
	}

	pBootData1 = (PBootDataV2)((DWORD*)pIVT + (pIVT->BootData - pIVT->SelfAddr) / sizeof(DWORD));
	pBootData2 = (PBootDataV2)((DWORD*)pIVT2 + (pIVT2->BootData - pIVT2->SelfAddr) / sizeof(DWORD));


	if (pIVT->DCDAddress)
	{
		pDCDRegion = (DWORD*)pIVT + (pIVT->DCDAddress - pIVT->SelfAddr) / sizeof(DWORD);
		if (!RunDCD(pDCDRegion))
			return FALSE;
	}

	// Load Initial Image
	assert((pIVT->SelfAddr - ImgIVTOffset) < (1ULL << 32));
	if (!Download((UCHAR*)pImg, MX8_INITIAL_IMAGE_SIZE, (UINT)(SCUViewAddr(pIVT->SelfAddr) - ImgIVTOffset)))
		return FALSE;

	//Load all the images in the first container to their respective Address
	for (i = 0; i < (pBootData1->NrImages); ++i) {
		assert(pBootData1->Images[i].ImageAddr < (1ULL << 32));
		if (!Download((UCHAR*)pImg + pBootData1->Images[i].Offset - IVT_OFFSET_SD,
			pBootData1->Images[i].ImageSize,
			(UINT)SCUViewAddr(pBootData1->Images[i].ImageAddr)))
			return FALSE;
	}

	//Load all the images in the second container to their respective Address
	for (i = 0; i < (pBootData2->NrImages); ++i) {
		assert(pBootData2->Images[i].ImageAddr < (1ULL << 32));
		if (!Download((UCHAR*)pImg + pBootData2->Images[i].Offset - IVT_OFFSET_SD,
			pBootData2->Images[i].ImageSize,
			(UINT)SCUViewAddr(pBootData2->Images[i].ImageAddr)))
			return FALSE;
	}

	m_jumpAddr = pIVT->SelfAddr;
	return TRUE;
}

BOOL MxHidDevice::Download(UCHAR* pBuffer, ULONGLONG dataCount, UINT RAMAddress)
{
	//if(pMxFunc->Task == TRANS)
	DWORD byteIndex, numBytesToWrite = 0;

	for (byteIndex = 0; byteIndex < dataCount; byteIndex += numBytesToWrite)
	{
		// Get some data
		numBytesToWrite = min(MAX_SIZE_PER_DOWNLOAD_COMMAND, (DWORD)dataCount - byteIndex);

		if (!TransData(RAMAddress + byteIndex, numBytesToWrite, pBuffer + byteIndex))
		{
			TRACE(_T("Download(): TransData(0x%X, 0x%X,0x%X) failed.\n"), \
				RAMAddress + byteIndex, numBytesToWrite, pBuffer + byteIndex);
			return FALSE;
		}
	}
	return TRUE;
}

BOOL MxHidDevice::Download(PImageParameter pImageParameter, UCHAR *pFileDataBuf, ULONGLONG dwFileSize, int cmdOpIndex)
{
	//Create device handle and report id
    OpenMxHidHandle();

	if(!Download(pFileDataBuf, dwFileSize, pImageParameter->PhyRAMAddr4KRL))
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR,_T("Download Failure\n"));
		goto ERR_HANDLE;
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

