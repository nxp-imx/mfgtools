/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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
#include "DeviceClass.h"
#include "MxHidDevice.h"
#include "MxHidDeviceClass.h"
#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"

MxHidDeviceClass::MxHidDeviceClass(INSTANCE_HANDLE handle)
: DeviceClass(&GUID_DEVINTERFACE_HID, &GUID_DEVCLASS_HIDCLASS, _T("MXHID"), DeviceTypeMxHid, handle)
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("new MxHidDeviceClass"));
}

MxHidDeviceClass::~MxHidDeviceClass(void)
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("delete MxHidDeviceClass"));
}

Device* MxHidDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
    //Check if the path matches our device pid&vid.
    //An example of value of path: "\\?\hid#vid_413c&pid_2011&mi_01&col02#8&2598dfbd&0&0001#{4d1e55b2-f16f-11cf-88cb-001111000030}"

    //add filiters
	OP_STATE_ARRAY *pOpStates = GetOpStates((MFGLIB_VARS *)m_pLibHandle);
	CString filter;
	BOOL isRightDevice = FALSE;
	OP_STATE_ARRAY::iterator it = pOpStates->begin();
	COpState *pCurrentState = NULL;
	CString msg = path;
	for(; it!=pOpStates->end(); it++)
	{
		filter.Format(_T("vid_%04x&pid_%04x"), (*it)->uiVid, (*it)->uiPid);
		//path.MakeUpper();
		msg.MakeUpper();
		filter.MakeUpper();
		if( msg.Find(filter) != -1 )
		{	//find
			isRightDevice = TRUE;
			pCurrentState = (*it);
			break;
		}
	}
	if(isRightDevice) //OK, find the device
	{
		return new MxHidDevice(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);
	}
	else
	{
		return NULL;
	}
}
