/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "Device.h"

/// <summary>
/// A MxRomDevice device.
/// </summary>
class MxRomDevice : public Device
{
public:
	MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
    virtual ~MxRomDevice(void);
};