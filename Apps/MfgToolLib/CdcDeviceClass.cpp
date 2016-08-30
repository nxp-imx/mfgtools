/*
 * Copyright 2015, Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "stdafx.h"
#include <Assert.h>
#include <cfgmgr32.h>
#include <basetyps.h>
#include <setupapi.h>
#include <initguid.h>

#include "DeviceClass.h"
#include "CDCDevice.h"
#include "CDCDeviceClass.h"
#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"


CDCDeviceClass::CDCDeviceClass(INSTANCE_HANDLE handle)
: DeviceClass(&GUID_DEVINTERFACE_USB_DEVICE, NULL, _T("USB"), DeviceTypeKBLCDC, handle)
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("new CDCDeviceClass"));
}

CDCDeviceClass::~CDCDeviceClass(void)
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("delete CDCDeviceClass"));
}

Device* CDCDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
	//Check if the path matches our device pid&vid.
	//An example of value of path: "\\?\cdc#vid_413c&pid_2011&mi_01&col02#8&2598dfbd&0&0001#{4d1e55b2-f16f-11cf-88cb-001111000030}"

	OP_STATE_ARRAY *pOpStates = GetOpStates((MFGLIB_VARS *)m_pLibHandle);
	CString filter;
	BOOL isRightDevice = FALSE;
	OP_STATE_ARRAY::iterator it = pOpStates->begin();
	COpState *pCurrentState = NULL;
	CString msg = path;

	SP_DEVINFO_DATA deviceInfoSet;
	ZeroMemory(&deviceInfoSet, sizeof(SP_DEVINFO_DATA));
	deviceInfoSet.cbSize = sizeof(SP_DEVINFO_DATA);

	// Check whether the device's VID and PID is in the ucl2.xml
	for (; it != pOpStates->end(); it++)
	{
		filter.Format(_T("vid_%04x&pid_%04x"), (*it)->uiVid, (*it)->uiPid);
		msg.MakeUpper();
		filter.MakeUpper();
		if (msg.Find(filter) != -1)
		{	//find
			isRightDevice = TRUE;
			pCurrentState = (*it);
			break;
		}
	}
	if (isRightDevice) //OK, find the device
	{
		if (deviceInfoData.ClassGuid == GUID_DEVCLASS_PORTS)
		{
			deviceInfoSet = deviceInfoData;
		}
		else if (deviceInfoData.ClassGuid == GUID_DEVCLASS_USB)
		{
			// Check if there is a child node is PORT device.
			// if yes, return true with the first PORT child node.
			// if no, return false.
			if (!FindChildPort(deviceInfoData, &deviceInfoSet))
				return NULL;
		}

		CString commPort;
		CString friendlyName;
		int index;
		BYTE *pBuf = NULL;
		ULONG bufSize = 0;
		gSetupApi().apiSetupDiGetDeviceRegistryProperty(_deviceInfoSet, &deviceInfoSet, SPDRP_FRIENDLYNAME, NULL, NULL, 0, &bufSize);

		pBuf = (BYTE*)malloc(bufSize);
		gSetupApi().apiSetupDiGetDeviceRegistryProperty(_deviceInfoSet, &deviceInfoSet, SPDRP_FRIENDLYNAME, NULL, pBuf, bufSize, &bufSize);
		friendlyName = CString((LPCWSTR)pBuf);

		index = friendlyName.ReverseFind(_T('('));
		commPort = friendlyName.Mid(index + 1);
		commPort.TrimRight(_T(')'));
		if (pBuf != NULL)
		{
			free(pBuf);
			pBuf = NULL;
		}

		return new CDCDevice(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle, commPort, friendlyName);
	}

	return NULL;
}

BOOL CDCDeviceClass::FindChildPort(SP_DEVINFO_DATA father, PSP_DEVINFO_DATA child)
{
	std::vector<std::pair<SP_DEVINFO_DATA, DEVINST>>stackList;
	SP_DEVINFO_DATA fatherDeviceInfo;
	SP_DEVINFO_DATA childDeviceInfo;
	DEVINST childInst = 0;
	CString deviceInstanceId;

	ZeroMemory(&childDeviceInfo, sizeof(SP_DEVINFO_DATA));
	childDeviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	//push father node
	stackList.push_back(std::make_pair(father, childInst));
	while (true)
	{
		//if there is no node in the stack, return.
		if (stackList.size() == 0)
		{
			return false;
		}
		//pop a node
		fatherDeviceInfo = stackList.back().first;
		childInst = stackList.back().second;
		stackList.pop_back();
		// use CM_Get_Child to get first child node, and use CM_Get_Sibling for the rests
		if (childInst == 0)
		{
			if (gSetupApi().CM_Get_Child(&childInst, fatherDeviceInfo.DevInst, 0) != CR_SUCCESS)
				continue;
		}
		else
		{
			if (gSetupApi().CM_Get_Sibling(&childInst, fatherDeviceInfo.DevInst, 0) != CR_SUCCESS)
				continue;
		}
		//Get child node's info.
		gSetupApi().apiCM_Get_Device_ID(childInst, deviceInstanceId);
		gSetupApi().apiSetupDiOpenDeviceInfo(_deviceInfoSet, (PCTSTR)deviceInstanceId.GetString(), NULL, DIOD_INHERIT_CLASSDRVS | DIOD_CANCEL_REMOVE, &childDeviceInfo);

		//If it is PORT device, return it.
		if (childDeviceInfo.ClassGuid == GUID_DEVCLASS_PORTS)
		{
			*child = childDeviceInfo;
			return true;
		}
		//If it is USB device. so this child node also has grand child. push father and child to stack.
		else if (childDeviceInfo.ClassGuid == GUID_DEVCLASS_USB)
		{
			stackList.push_back(std::make_pair(fatherDeviceInfo, childInst));
			stackList.push_back(std::make_pair(childDeviceInfo, 0));
			continue;
		}
		// If it is other device, pass it and then enumerate the left childs.
		else
		{
			stackList.push_back(std::make_pair(fatherDeviceInfo, childInst));
			continue;
		}
	}
}