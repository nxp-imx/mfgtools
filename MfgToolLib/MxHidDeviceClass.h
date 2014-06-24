/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "DeviceClass.h"

class MxHidDeviceClass : public DeviceClass
{
public:
    MxHidDeviceClass(INSTANCE_HANDLE handle);
    virtual ~MxHidDeviceClass(void);

	Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path);

    CString ToString() { return _T("MxHidDeviceClass"); }
};
