/*
 * Copyright 2012-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "PortMgr.h"
#include "../MfgToolLib/MfgToolLib_Export.h"
#include <map>
#include <tuple>

// CMfgTool_MultiPanelApp:
// See MfgTool_MultiPanel.cpp for the implementation of this class
//

class CMfgTool_MultiPanelApp : public CWinAppEx
{
public:
	CMfgTool_MultiPanelApp();

// Overrides
public:
	virtual BOOL InitInstance();

//	BOOL FirstInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();

	BOOL ParseMyCommandLine(CString strCmdLine);

public:
	typedef CTypedPtrArray<CObArray, CPortMgr*>  CPortMgrArray;
	CPortMgrArray m_PortMgr_Array;

	CString m_strProfileName;
	CString m_strListName;
	std::map<CString, CString> m_uclKeywords;
	// For usb-port specific parameters, we use as key a tuple <hub, index, parameter name>
	typedef std::tuple<UINT, UINT, CString> UsbPortKey;
	std::map<UsbPortKey, CString> m_usbPortKeywords;
	BOOL m_IsAutoStart;
	BOOL ParseCfgFile(CString strFilename, BOOL bMsgBox);
	BOOL IsSectionExist(CString strSection, CString strFilename);

	OPERATIONS_INFORMATION m_OperationsInformation;
	PHASES_INFORMATION m_PhasesInformation;

	//UINT GetStateCommandSize(MX_DEVICE_STATE devState);
	UINT GetStateCommandSize(int index);

	INSTANCE_HANDLE m_pLibHandle;

	int FindDeviceIndex(DWORD operationID);
};

extern CMfgTool_MultiPanelApp theApp;

extern void gDeviceChangeNotify(DEVICE_CHANGE_NOTIFY *pnsinfo);
extern void gProgressUpdate(OPERATE_RESULT *puiInfo);
