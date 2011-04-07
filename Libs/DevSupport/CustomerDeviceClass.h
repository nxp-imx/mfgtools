/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "DeviceClass.h"

//
//  Define the customer class guid *OUTSIDE* the #ifndef/#endif to allow
//  multiple includes with precompiled headers.
//
DEFINE_GUID( GUID_DEVINTERFACE_CUSTOMER_DEV, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
             0xC0, 0x4F, 0xB9, 0x51, 0xED);
// Obsolete GUID naming convention.
#define GUID_CLASS_INPUT GUID_DEVINTERFACE_CUSTOMER_DEV

class CustomerDeviceClass : public DeviceClass
{
friend class DeviceManager;
private:
	CustomerDeviceClass(void);
	virtual ~CustomerDeviceClass(void);

	size_t AddFilter(LPCTSTR Prefix, uint16_t vid, uint16_t pid);
	size_t AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance = _T(""));

private:
   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

public:
	CStdString ToString() { return _T("CustomerDeviceClass"); }
};
