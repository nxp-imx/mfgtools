/*
 * Copyright (C) 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "DeviceClass.h"
//#include "StdInt.h"
//extern "C" {
//#include <hidsdi.h>
//}

class HidDeviceClass : public DeviceClass
{
friend class DeviceManager;
private:
	HidDeviceClass(INSTANCE_HANDLE handle);
    virtual ~HidDeviceClass(void);

private:
   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path);

public:
    CString ToString() { return _T("HidDeviceClass"); }
};
