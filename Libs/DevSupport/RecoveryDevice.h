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

class RecoveryDevice : public Device
{
public:
	RecoveryDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~RecoveryDevice(void);
    uint32_t Download(const StFwComponent& fwComponent, Device::UI_Callback callbackFn); 	

private:
	static const uint32_t PipeSize = 4096;      //TODO:??? where did this come from?
	static const uint32_t DeviceTimeout = 1000; // 1 second
    HANDLE m_RecoveryHandle;
    OVERLAPPED	_fileOverlapped;

    DWORD Open(void);
    DWORD Close(void);
	int32_t ProcessTimeOut(const int32_t timeout);
	static void CALLBACK IoCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
};
