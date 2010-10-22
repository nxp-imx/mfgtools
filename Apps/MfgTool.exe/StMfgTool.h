/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StMfgTool.h : main header file for the StMfgTool application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "ConfigMgrSheet.h"
#include "LogMgrDlg.h"
#include "MainFrm.h"

#include "../../Libs/WinSupport/VersionInfo.h"

//#define SERIALIZE_HID TRUE					// undefined version requires 1 device per USB controller

typedef enum _PLATFORM {
	OS_98 = 0,
	OS_ME,
	OS_2K,
	OS_XP,
    OS_XP64,
	OS_MAC9,
	OS_MACX,
    OS_VISTA32,
    OS_VISTA64,
	OS_UNSUPPORTED
} PLATFORM;

// CStMfgToolApp:
// See StMfgTool.cpp for the implementation of this class
//

class CStMfgToolApp : public CWinApp
{
public:
	CStMfgToolApp();
// Overrides
public:
	virtual BOOL InitInstance();
//clw    BOOL IsMTPSupported();
// Implementation
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnAppAbout();
	virtual int ExitInstance();
	CRect m_rectInitialFrame;
	CMainFrame *m_p_main_frame;
//clw	CUSBPortMgr *m_p_usb_port_mgr;
	CConfigMgrSheet *m_p_config_mgr;
	CLogMgrDlg *m_pLogMgrDlg;
	CVersionInfo m_version_info;
    // Add a static BOOL that indicates whether the class was
    // registered so that you can unregister it in ExitInstance
    static BOOL bClassRegistered;
public:
//clw	CUSBPortMgr* GetUSBPortMgr(void) {return m_p_usb_port_mgr;};
	CConfigMgrSheet* GetConfigMgr(void) {return m_p_config_mgr;};
	CLogMgrDlg* GetLogMgrDlg(void) {return m_pLogMgrDlg;};
	CPortMgrDlg* GetPortDlg(INT_PTR _idx) { if( m_p_main_frame->GetSafeHwnd() ) { if(_idx < m_p_config_mgr->GetNumEnabledPorts() ) return m_p_main_frame->GetPortDlg(_idx); else return NULL; } else return NULL; };
	CVersionInfo * GetVersionInfo(void) { return &m_version_info; };
protected:
    BOOL FirstInstance(void);
	PLATFORM GetPlatform();
	BOOL IsRecoveryDriverInstalled();
	LPVOID StLoadStmp3RecResource(USHORT resID, PDWORD pdwSize, BOOL& isLocal);
	BOOL StWriteStmp3RecResource(LPVOID _pPtr, ULONG _dwSize, LPTSTR _pathName);
    BOOL InstallRecoveryDriver();


};

extern CStMfgToolApp theApp;
