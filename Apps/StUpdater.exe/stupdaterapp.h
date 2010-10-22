/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StUpdaterApp.h : main header file for the STUPDATERAPP application
//

#if !defined(AFX_STUPDATERAPP_H__F02BD20C_310E_4007_9ABB_903031F70AF5__INCLUDED_)
#define AFX_STUPDATERAPP_H__F02BD20C_310E_4007_9ABB_903031F70AF5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "..\\..\\common\\updater_restypes.h"
#include "StUpdaterDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CStUpdaterApp:
// See StUpdaterApp.cpp for the implementation of this class
//
class CStResource;
class CAutoPlayReject;

class CStUpdaterApp : public CWinApp
{
	friend UINT UnsafeWindowCloserThreadProc(LPVOID pParam);
public:
	CStUpdaterApp();
	~CStUpdaterApp();

	void OnStatusDlgCancelled();
	BOOL PumpMessages();
	CStResource* GetResource(){ return m_p_resource; }
	CStCmdLineProcessor* GetCmdLineProcessor(){ return m_p_cmdline_processor; }
	CStLogger* GetLogger(){ return m_p_logger; }
	CStUpdater* GetUpdater(){ return m_p_updater; }
	void HandleError( ST_ERROR _err, CWnd* _parent, BOOL _log_only = FALSE );
	ST_ERROR CheckForAnotherInstanceRunning(CStConfigInfo*);
	BOOL BringTheRunningAppToFront();
	LANGID	GetLangId() { return m_default_language; }

private:

	BOOL					m_status_dlg_cancelled;
	CStResource*			m_p_resource;
	CStConfigInfo*			m_p_config_info;
	CStCmdLineProcessor*	m_p_cmdline_processor;
	CStLogger*				m_p_logger;
	CStUpdater*				m_p_updater;
	LANGID					m_default_language;
	USER_DLG_TYPE			m_DefaultDialog;
    CStUpdaterDlg*          m_dlg;

	BOOL IsUserAdmin(void);
	void GetErrMsg( ST_ERROR _err, CString& _err_msg, CString& _err_msg_details, BOOL _log_only=FALSE );
	void SetDefaultLanguage();

	CWinThread* m_p_unsafe_window_closer_thread;
	HANDLE m_hEventKill;
	static UINT UnsafeWindowCloserThreadProc( LPVOID pParam );
	static CString GetDlgCaption(LPCTSTR _module, UINT _id);
	void SetDefaultDialog();

	CAutoPlayReject AutoPlayReject;

public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStUpdaterApp)
	public:
	virtual int ExitInstance();
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CStUpdaterApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//
// Usage: SetThreadName (-1, "MainThread");
//
//#include <windows.h>
#define MS_VC_EXCEPTION 0x406D1388

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName( DWORD dwThreadID, char* threadName);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STUPDATERAPP_H__F02BD20C_310E_4007_9ABB_903031F70AF5__INCLUDED_)
