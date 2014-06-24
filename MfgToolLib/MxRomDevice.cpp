/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once
#include "stdafx.h"
#include "MxRomDevice.h"
#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"

MxRomDevice::MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
    : Device(deviceClass, devInst, path, handle)
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("new MxRomDevice"));
}

MxRomDevice::~MxRomDevice()
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("delete MxRomDevice"));
}