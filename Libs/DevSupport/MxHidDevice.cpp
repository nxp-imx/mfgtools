/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include <Assert.h>
#include <cfgmgr32.h>
#include <basetyps.h>
#include <setupapi.h>
#include <initguid.h>
extern "C" {
#include <hidsdi.h>
}
#include <hidclass.h>
#include "Device.h"
#include "MxHidDevice.h"

#define DEVICE_TIMEOUT			INFINITE // 5000 ms
#define DEVICE_READ_TIMEOUT   10
#define DCD_WRITE
//The WriteFileEx function is designed solely for asynchronous operation.
//The Write Function is designed solely for synchronous operation.
#define ASYNC_READ_WRITE 0

HANDLE MxHidDevice::m_sync_event_tx = NULL;
HANDLE MxHidDevice::m_sync_event_rx = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
MxHidDevice::MxHidDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
{
    m_hid_drive_handle = INVALID_HANDLE_VALUE;
    m_sync_event_tx = NULL;
    m_sync_event_rx = NULL;
    m_pReadReport = NULL;
    m_pWriteReport = NULL;
    _chipFamily = MX50;
}

MxHidDevice::~MxHidDevice()
{
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

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HANDLE, PVOID);
typedef UINT (CALLBACK* LPFNDLLFUNC2)(PVOID);
// Modiifes m_Capabilities member variable
// Modiifes m_pReadReport member variable
// Modiifes m_pWriteReport member variable
int32_t MxHidDevice::AllocateIoBuffers()
{
    int32_t error = ERROR_SUCCESS;

	// Open the device
    HANDLE hHidDevice = CreateFile(_path.get(), 0, 0, NULL, OPEN_EXISTING, 0, NULL);

    if( hHidDevice == INVALID_HANDLE_VALUE )
    {
		error = GetLastError();
//t        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers().CreateFile ERROR:(%d)\r\n"), error);
        return error;
    }

    // Get the Capabilities including the max size of the report buffers
    HINSTANCE hHidDll = LoadLibrary(_T("hid.dll"));
    if (hHidDll == NULL)
    {
		error = GetLastError();
        CloseHandle(hHidDevice);
        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers().LoadLibrary(hid.dll) ERROR:(%d)\r\n"), error);
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
        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers().GetProcAddress(HidD_GetPreparsedData) ERROR:(%d)\r\n"), error);
        return error;
    }

	LPFNDLLFUNC2 lpfnDllFunc2 = (LPFNDLLFUNC2)GetProcAddress(hHidDll, "HidD_FreePreparsedData");
    if (!lpfnDllFunc2)
    {
        // handle the error
		error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);       
        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers().GetProcAddress(HidD_FreePreparsedData) ERROR:(%d)\r\n"), error);
        return error;
    }

	// if ( !HidD_GetPreparsedData(hHidDevice, &PreparsedData) )
	if ( !lpfnDllFunc1(hHidDevice, &PreparsedData) )
    {
		error = GetLastError();
        CloseHandle(hHidDevice);
        FreeLibrary(hHidDll);       
        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers().HidD_GetPreparsedData ERROR:(%d)\r\n"), error);
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
        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers().GetProcAddress(HidP_GetCaps) ERROR:(%d)\r\n"), error);
        return error;
    }
    else 
	{
		// if ( HidP_GetCaps(PreparsedData, &m_Capabilities) != HIDP_STATUS_SUCCESS )
		if ( lpfnDllFunc1(PreparsedData, &m_Capabilities) != HIDP_STATUS_SUCCESS )
		{
			CloseHandle(hHidDevice);
			// HidD_FreePreparsedData(PreparsedData);
			lpfnDllFunc2(PreparsedData);
			FreeLibrary(hHidDll);       
			ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers().GetCaps ERROR:(%d)\r\n"), HIDP_STATUS_INVALID_PREPARSED_DATA);
			return HIDP_STATUS_INVALID_PREPARSED_DATA;
		}
	}
	// HidD_FreePreparsedData(PreparsedData);
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
	        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers(). Failed to allocate memory (1)\r\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
		}
    }

    if ( m_Capabilities.OutputReportByteLength )
    {
        m_pWriteReport = (_MX_HID_DATA_REPORT*)malloc(m_Capabilities.OutputReportByteLength);
        if ( m_pWriteReport == NULL )
		{
	        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers(). Failed to allocate memory (2)\r\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
		}
    }

    return ERROR_SUCCESS;
}

BOOL MxHidDevice::OpenUSBHandle(HANDLE* pHandle, CStdString pipePath)
{
    #if ASYNC_READ_WRITE
	*pHandle = CreateFile(pipePath,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);
    #else
    *pHandle = CreateFile(pipePath,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
    #endif

	if (*pHandle == INVALID_HANDLE_VALUE) {
		TRACE(_T("MxHidDevice::OpenUSBHandle() Failed to open (%s) = %d"), pipePath, GetLastError());
		return FALSE;
	}

	return TRUE;
}


BOOL MxHidDevice::OpenMxHidHandle()
{
    int err = ERROR_SUCCESS;

    #if ASYNC_READ_WRITE
    // create TX and RX events
    m_sync_event_tx = ::CreateEvent( NULL, TRUE, FALSE, NULL );
    if( !m_sync_event_tx )
    {
        assert(false);
        TRACE((__FUNCTION__ " ERROR: CreateEvent failed.ErrCode 0x%x(%d)\n"),GetLastError(),GetLastError());
        return FALSE;
    }
    m_sync_event_rx = ::CreateEvent( NULL, TRUE, FALSE, NULL );
    if( !m_sync_event_rx )
    {
        assert(false);
        TRACE((__FUNCTION__ " ERROR: CreateEvent failed.ErrCode 0x%x(%d)\n"),GetLastError(),GetLastError());
        return FALSE;
    }
    #endif

    memset(&m_Capabilities, 0, sizeof(m_Capabilities));

    err = AllocateIoBuffers();
	if ( err != ERROR_SUCCESS )
	{
		TRACE((__FUNCTION__ " ERROR: AllocateIoBuffers failed. %d\n"),err);
		return FALSE;
	}

    // Open the device 
    if (!OpenUSBHandle(&m_hid_drive_handle,_path.get()))
    {
        TRACE(__FUNCTION__ " ERROR: OpenUSBHandle failed.\n");
		return FALSE;
    }
    
    return TRUE;
}

BOOL MxHidDevice::CloseMxHidHandle()
{
    #if ASYNC_READ_WRITE
    if( m_sync_event_tx != NULL )
    {
        CloseHandle(m_sync_event_tx);
        m_sync_event_tx = NULL;
    }
    if( m_sync_event_rx != NULL )
    {
        CloseHandle(m_sync_event_rx);
        m_sync_event_rx = NULL;
    }
    #endif

    if( m_hid_drive_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hid_drive_handle);
        m_hid_drive_handle = INVALID_HANDLE_VALUE;
    }

    FreeIoBuffers();
        
    return TRUE;
}


#if ASYNC_READ_WRITE
// Write to HID device
int MxHidDevice::Write(UCHAR * _buf,ULONG _size)
{
    DWORD status;

    // Preparation
    m_overlapped.Offset				= 0;
    m_overlapped.OffsetHigh			= 0;
    m_overlapped.hEvent				= (PVOID)(ULONGLONG)_size;

    ::ResetEvent( MxHidDevice::m_sync_event_tx );

    // Write to the device
    if( !::WriteFileEx( m_hid_drive_handle, _buf, _size, &m_overlapped, 
        MxHidDevice::WriteCompletionRoutine ) )
    {
        TRACE(_T("MxHidDevice::Write()fail. Error writing to device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
        return ::GetLastError();
    }

    // wait for completion
    if( (status = ::WaitForSingleObjectEx( m_sync_event_tx, INFINITE, TRUE )) == WAIT_TIMEOUT )
    {
        TRACE(_T("MxHidDevice::Write()fail. WaitForSingleObjectEx TimeOut.\r\n"));
        ::CancelIo( m_hid_drive_handle );
        return WAIT_TIMEOUT;
    }
	
	if( m_overlapped.Offset == 0 )
	{
        TRACE(_T("MxHidDevice::Write() fail.m_overlapped.Offset is 0.\r\n"));
        return -13 ;
	}
    else
        return ERROR_SUCCESS;
}

VOID MxHidDevice::WriteCompletionRoutine( 
    DWORD _err_code, 
    DWORD _bytes_transferred, 
    LPOVERLAPPED _lp_overlapped
    )
{
    if( ((ULONG)(ULONGLONG)_lp_overlapped->hEvent != _bytes_transferred) || _err_code )
    {
        *(BOOL *)&_lp_overlapped->Offset = 0;
    }
    else
    {
        *(BOOL *)&_lp_overlapped->Offset = _bytes_transferred;
    }

    ::SetEvent(MxHidDevice::m_sync_event_tx );
}

// Read from HID device
int MxHidDevice::Read(void* _buf, UINT _size)
{
    DWORD status;

    // Preparation
    m_overlapped.Offset				= 0;
    m_overlapped.OffsetHigh			= 0;
    m_overlapped.hEvent				= (PVOID)(ULONGLONG)_size;

    ::ResetEvent( MxHidDevice::m_sync_event_rx );

    //  The read command does not sleep very well right now.
    Sleep(35); 

    // Read from device
    if( !::ReadFileEx( m_hid_drive_handle, _buf, _size, &m_overlapped, 
        MxHidDevice::ReadCompletionRoutine ) )
    {
        TRACE(_T("MxHidDevice::Read()fail. Error reading from device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
        return ::GetLastError();
    }

    // wait for completion
    if( (status = ::WaitForSingleObjectEx( m_sync_event_rx, DEVICE_READ_TIMEOUT, TRUE )) == WAIT_TIMEOUT )
    {
        TRACE(_T("MxHidDevice::Read()fail. WaitForSingleObjectEx TimeOut.\r\n"));
        ::CancelIo( m_hid_drive_handle );
        return WAIT_TIMEOUT;
    }

    if( m_overlapped.Offset == 0 )
    {
        TRACE(_T("MxHidDevice::Read()fail.m_overlapped.Offset is 0.\r\n"));
        return -13 /*STERR_FAILED_TO_WRITE_FILE_DATA*/;
    }
    else
        return ERROR_SUCCESS;
}

VOID MxHidDevice::ReadCompletionRoutine( 
        DWORD _err_code, 
        DWORD _bytes_transferred, 
        LPOVERLAPPED _lp_overlapped
        )
{
    if( ((ULONG)(ULONGLONG)_lp_overlapped->hEvent != _bytes_transferred) || _err_code )
    {
        *(BOOL *)&_lp_overlapped->Offset = 0;
    }
    else
    {
        *(BOOL *)&_lp_overlapped->Offset = _bytes_transferred;
    }

    if( m_sync_event_rx != NULL) {
        ::SetEvent( m_sync_event_rx );
    }
}

#else

int MxHidDevice::Write(UCHAR* _buf, ULONG _size)
{
	int    nBytesWrite; // for bytes actually written

	if( !WriteFile(m_hid_drive_handle, _buf, _size, (PULONG) &nBytesWrite, NULL) )
	{
		TRACE(_T("MxHidDevice::Write() Error writing to device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		return FALSE;
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
		TRACE(_T("MxHidDevice::Read() Error reading from device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

#endif

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

//Report1 
BOOL MxHidDevice::SendCmd(PSDPCmd pSDPCmd)
{
	//First, pack the command to a report.
	PackSDPCmd(pSDPCmd);

	//Send the report to USB HID device
	if ( Write((unsigned char *)m_pWriteReport, m_Capabilities.OutputReportByteLength) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return TRUE;
}

//Report 2
BOOL MxHidDevice::SendData(const unsigned char * DataBuf, UINT ByteCnt)
{
	memcpy(m_pWriteReport->Payload, DataBuf, ByteCnt);

	m_pWriteReport->ReportId = REPORT_ID_DATA;
	if (Write((unsigned char *)m_pWriteReport, m_Capabilities.OutputReportByteLength) != ERROR_SUCCESS)
		return FALSE;	

	return TRUE;
}

//Report3, Device to Host
BOOL MxHidDevice::GetHABType()
{
    memset((UCHAR *)m_pReadReport, 0, m_Capabilities.InputReportByteLength);

    //Get Report3, Device to Host:
    //4 bytes HAB mode indicating Production/Development part
	if ( Read( (UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength )  != ERROR_SUCCESS)
	{
		return FALSE;
	}
	if ( (*(unsigned int *)(m_pReadReport->Payload) != HabEnabled)  && 
		 (*(unsigned int *)(m_pReadReport->Payload) != HabDisabled) ) 
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
		TRACE("WriteReg(): Invalid write ack: 0x%x\n", ((PULONG)m_pReadReport)[0]);
		return FALSE; 
	}

    return TRUE;
}

BOOL MxHidDevice::GetCmdAck(UINT RequiredCmdAck)
{
	if(!GetHABType())
		return FALSE;

	if(!GetDevAck(RequiredCmdAck))
		return FALSE;

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
        //TRACE("Transfer Size: %d\n", TransSize);
    }

	return TRUE;
}

BOOL MxHidDevice::TransData(UINT address, UINT byteCount, const unsigned char * pBuf)
{
    SDPCmd SDPCmd;

    SDPCmd.command = ROM_KERNEL_CMD_WR_FILE;
    SDPCmd.dataCount = byteCount;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = address;

	if(!SendCmd(&SDPCmd))
		return FALSE;
    
    Sleep(10);

    UINT MaxHidTransSize = m_Capabilities.OutputReportByteLength -1;
    UINT TransSize;
    
    while(byteCount > 0)
    {
        TransSize = (byteCount > MaxHidTransSize) ? MaxHidTransSize : byteCount;

		if(!SendData(pBuf, TransSize))
			return FALSE;

        byteCount -= TransSize;
        pBuf += TransSize;
        //TRACE("Transfer Size: %d\n", MaxHidTransSize);
    }
    
    //below function should be invoked for mx50
	if ( !GetCmdAck(ROM_STATUS_ACK) )
	{
		return FALSE;
	}

	return TRUE;
}

BOOL MxHidDevice::Jump(UINT RAMAddress)
{
    SDPCmd SDPCmd;

    SDPCmd.command = ROM_KERNEL_CMD_JUMP_ADDR;
    SDPCmd.dataCount = 0;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = RAMAddress;

	if(!SendCmd(&SDPCmd))
		return FALSE;

	if(!GetHABType())
		return FALSE;
	
	TRACE("*********Jump to Ramkernel successfully!**********\r\n");
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

BOOL MxHidDevice::Download(PImageParameter pImageParameter,StFwComponent *fwComponent, Device::UI_Callback callbackFn)
{
    //Create device handle and report id
    OpenMxHidHandle();
    
	UCHAR* pBuffer = (UCHAR*)fwComponent->GetDataPtr();
    ULONGLONG dataCount = fwComponent->size();
    
	DWORD byteIndex, numBytesToWrite = 0;
	for ( byteIndex = 0; byteIndex < dataCount; byteIndex += numBytesToWrite )
	{
		// Get some data
		numBytesToWrite = (DWORD)min(MAX_SIZE_PER_DOWNLOAD_COMMAND, dataCount - byteIndex);

		if (!TransData(pImageParameter->PhyRAMAddr4KRL + byteIndex, numBytesToWrite, pBuffer + byteIndex))
		{
			TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X, 0x%X, 0x%X) failed.\n"), \
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
            TRACE(_T("DownloadImage(): AddHdr(0x%x) failed.\n"),ImageStartAddr);
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

BOOL MxHidDevice::DCDWrite(PUCHAR DataBuf, UINT RegCount)
{
	SDPCmd SDPCmd;
    SDPCmd.command = ROM_KERNEL_CMD_DCD_WRITE;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = 0;

	//Must reverse uint32 endian to adopt the requirement of ROM
	for(UINT i=0; i<RegCount*sizeof(stMemoryInit); i+=4)
	{
		UINT TempData = ((DataBuf[i]<<24) | (DataBuf[i+1]<<16) | (DataBuf[i+2] << 8) | (DataBuf[i+3]));
		((PUINT)DataBuf)[i/4] = TempData;
	}

    while(RegCount)
    {
		SDPCmd.dataCount = (RegCount > MAX_DCD_WRITE_REG_CNT) ? MAX_DCD_WRITE_REG_CNT : RegCount;
		RegCount -= SDPCmd.dataCount;
		UINT ByteCnt = SDPCmd.dataCount*sizeof(stMemoryInit);

		if(!SendCmd(&SDPCmd))
			return FALSE;

		if(!SendData(DataBuf, ByteCnt))
			return FALSE;

		if (!GetCmdAck(ROM_WRITE_ACK) )
		{
			return FALSE;
		}

		DataBuf += ByteCnt;
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
	CFileException fileException;
	if( !scriptFile.Open(filename, CFile::modeRead | CFile::shareDenyNone, &fileException) )
	{
		TRACE( _T("Can't open file %s, error = %u\n"), filename, fileException.m_cause );
	}

	CStringT<char,StrTraitMFC<char> > cmdString;
	scriptFile.Read(cmdString.GetBufferSetLength(scriptFile.GetLength()), scriptFile.GetLength());
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
	CFileException fileException;
	if( !scriptFile.Open(filename, CFile::modeRead | CFile::shareDenyNone, &fileException) )
	{
		TRACE( _T("Can't open file %s, error = %u\n"), filename, fileException.m_cause );
	}

	CStringT<char,StrTraitMFC<char> > cmdString;
	scriptFile.Read(cmdString.GetBufferSetLength((int)scriptFile.GetLength()), (unsigned int)scriptFile.GetLength());
	cmdString.ReleaseBuffer();

	XNode script;
	if ( script.Load(A2T(cmdString)) != NULL )
	{
		XNodes cmds = script.GetChilds(_T("CMD"));
		XNodes::iterator cmd = cmds.begin();
		//There are no more than 200 regs to be written.
		stMemoryInit * pMemPara = (stMemoryInit * )malloc(0x1000);
		UINT RegCount = 0;
		for ( ; cmd != cmds.end(); ++cmd, RegCount++)
		{
			MemoryInitCommand* pCmd = (MemoryInitCommand*)(*cmd);
            pMemPara[RegCount].format = pCmd->GetFormat();
            pMemPara[RegCount].addr = pCmd->GetAddress();
            pMemPara[RegCount].data = pCmd->GetData();
		}
		if ( !DCDWrite((PUCHAR)pMemPara,RegCount) )
		{
			TRACE(_T("Failed to initialize memory!\r\n"));
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

BOOL MxHidDevice::AddIvtHdr(UINT32 ImageStartAddr)
{
	UINT FlashHdrAddr;

	//transfer length of ROM_TRANSFER_SIZE is a must to ROM code.
	unsigned char FlashHdr[ROM_TRANSFER_SIZE] = { 0 };	

	// Otherwise, create a header and append the data
	if(_chipFamily == MX50)
	{
    	PIvtHeader pIvtHeader = (PIvtHeader)FlashHdr;

    	FlashHdrAddr = ImageStartAddr - sizeof(IvtHeader);

        //Read the data first
    	if ( !ReadData(FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr) )
    	{
    		TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X, 0x%X) failed.\n"), \
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
	}
    
    //Send the IVT header to destiny address
	if ( !TransData(FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr) )
	{
		TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X, 0x%X) failed.\n"), \
            FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr);
		return FALSE;
	}
    
    //Verify the data
   	unsigned char Tempbuf[ROM_TRANSFER_SIZE] = { 0 };
    if ( !ReadData(FlashHdrAddr, ROM_TRANSFER_SIZE, Tempbuf) )
	{
		TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X, 0x%X) failed.\n"), \
            FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr);
		return FALSE;
	}

    if(memcmp(FlashHdr, Tempbuf, ROM_TRANSFER_SIZE)!= 0 )
	{
		TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X, 0x%X) failed.\n"), \
            FlashHdrAddr, ROM_TRANSFER_SIZE, FlashHdr);
		return FALSE;
	}

    m_jumpAddr = FlashHdrAddr;
    
	return TRUE;
}

BOOL MxHidDevice::RunPlugIn(CString fwFilename)
{
    CFile fwFile;
	UCHAR* pDataBuf = NULL;
	ULONGLONG fwSize = 0;
	DWORD * pPlugIn = NULL;
	DWORD PlugInDataOffset= 0,ImgIVTOffset= 0,BootDataImgAddrIndex= 0,PhyRAMAddr4KRL= 0;
    DWORD PlugInAddr = 0;
	PIvtHeader pIVT = NULL,pIVT2 = NULL;

    //Create device handle and report id
    OpenMxHidHandle();

    if ( fwFile.Open(fwFilename, CFile::modeRead | CFile::shareDenyWrite) == 0 )
    {
        TRACE(_T("Firmware file %s failed to open.errcode is %d\n"), fwFilename,GetLastError());
        goto ERR_HANDLE;
    }

    fwSize = fwFile.GetLength();
    pDataBuf = (UCHAR*)malloc((size_t)fwSize);
    fwFile.Read(pDataBuf, (UINT)fwSize);
    fwFile.Close();
    
	//Search for IVT
    pPlugIn = (DWORD *)pDataBuf;
    while(pPlugIn[ImgIVTOffset/sizeof(DWORD)] != IVT_BARKER_HEADER && ImgIVTOffset <= fwSize)
		ImgIVTOffset+= 0x100;
	
	if(ImgIVTOffset >= fwSize)
		goto ERR_HANDLE;

	pIVT = (PIvtHeader) (pPlugIn + ImgIVTOffset/sizeof(DWORD));
	DWORD IVT2Offset = ImgIVTOffset + sizeof(IvtHeader);

	while(pPlugIn[IVT2Offset/sizeof(DWORD)] != IVT_BARKER_HEADER && IVT2Offset <= fwSize)
		IVT2Offset+= sizeof(DWORD);
	
	if(IVT2Offset >= fwSize)
		goto ERR_HANDLE;
  
    //---------------------------------------------------------
    //Run plugin in Iram
    PlugInAddr = pIVT->ImageStartAddr;
	PlugInDataOffset = pIVT->ImageStartAddr - pIVT->SelfAddr;
	if (!TransData(PlugInAddr, 0x1000, (PUCHAR)((DWORD)pIVT + PlugInDataOffset)))
	{
		TRACE(_T("RunPlugIn(): TransData(0x%X, 0x%X,0x%X) failed.\n"), \
			PlugInAddr, 0x1000, ((DWORD)pIVT + PlugInDataOffset));
		goto ERR_HANDLE;
	}

	if(!AddIvtHdr(PlugInAddr))
	{
        TRACE(_T("RunPlugIn(): Failed to addhdr to RAM address: 0x%x.\n"), PlugInAddr);
		goto ERR_HANDLE;
	}
    
    if( !Jump(m_jumpAddr))
	{
        TRACE(_T("RunPlugIn(): Failed to jump to RAM address: 0x%x.\n"), m_jumpAddr);
		goto ERR_HANDLE;
	}

    //---------------------------------------------------------
    //Download eboot to ram
    pIVT2 = (PIvtHeader)(pPlugIn + IVT2Offset/sizeof(DWORD));
    BootDataImgAddrIndex = (DWORD *)pIVT2 - pPlugIn;
	BootDataImgAddrIndex += (pIVT2->BootData - pIVT2->SelfAddr)/sizeof(DWORD);
	PhyRAMAddr4KRL = pPlugIn[BootDataImgAddrIndex] + IVT_OFFSET - ImgIVTOffset;
    if (!TransData(PhyRAMAddr4KRL, (unsigned int)fwSize, (PUCHAR)((DWORD)pDataBuf)))
	{
		TRACE(_T("RunPlugIn(): TransData(0x%X, 0x%X,0x%X) failed.\n"), \
			PhyRAMAddr4KRL, fwSize, pDataBuf);
		goto ERR_HANDLE;
	}
    
    DWORD ImgStartAddr = pIVT2->ImageStartAddr;
	if(!AddIvtHdr(ImgStartAddr))
	{
        TRACE(_T("RunPlugIn(): Failed to addhdr to RAM address: 0x%x.\n"), ImgStartAddr);
		goto ERR_HANDLE;
	}

    //Clear device handle and report id    
    CloseMxHidHandle();
    return TRUE;

ERR_HANDLE:
    //Clear device handle and report id    
    CloseMxHidHandle();
    return FALSE;
}

