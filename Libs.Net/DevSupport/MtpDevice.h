/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "Device.h"
#include "Observer.h"
#include "StFwComponent.h"

#include "Common/StdString.h"
#include "Common/StdInt.h"

#include <mswmdm.h>

class MtpDevice : public Device
{
public:
	MtpDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~MtpDevice(void);
	virtual uint32_t SendCommand(StApi& api, uint8_t* additionalInfo = NULL);
	virtual CStdString GetSendCommandErrorStr();
	virtual uint32_t ResetChip();
	virtual uint32_t ResetToRecovery();
	virtual uint32_t OldResetToRecovery();
//	CStdString GetSerialNumberStr();
/*
    virtual CStdString GetErrorStr();
	
   	static void CALLBACK IoCompletion(DWORD dwErrorCode,           // completion code
			                          DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
							          LPOVERLAPPED lpOverlapped);        // pointer to structure with I/O information
*/

private:
	IWMDMDevice3 *_pWmdmDevice3;
	uint16_t _responseCode;
//	static HANDLE m_SyncEvent;	
//    HANDLE m_RecoveryHandle;
//    DWORD m_ErrorStatus;
//    OVERLAPPED	m_FileOverlapped;


//    BOOL Open(void);
//    BOOL Close(void);

//    uint32_t MtpDevice::Write(uint8_t* pBuf, size_t Size);
};
