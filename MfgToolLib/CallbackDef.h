/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "MfgToolLib_Export.h"

typedef void *HANDLE_CALLBACK;

typedef struct 
{
	DWORD OperationID;
	PCALLBACK_DEVICE_CHANGE pfunc;
}DeviceChangeCallbackStruct;

typedef struct
{
	DWORD OperationID;
	PCALLBACK_OPERATE_RESULT pfunc;
}OperateResultUpdateStruct;
