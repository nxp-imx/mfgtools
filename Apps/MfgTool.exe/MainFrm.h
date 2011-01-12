/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// MainFrm.h : interface of the CMainFrame class
//
#pragma once

#include "PortMgrDlg.h"
#include "ConfigMgrSheet.h"
//#include "RegScrubDlg.h"
#include "EventLogger.h"
#include "StDialogBar.h"
//#include "RegCheckDlg.h"

class CMainFrame : public CFrameWnd
{
	friend UINT UnsafeWindowCloserThreadProc(LPVOID pParam);
public:
	CMainFrame();
	virtual ~CMainFrame();
	typedef enum e_START_STATE { STOPPED, RUNNING, WAITING };

	typedef CTypedPtrArray<CObArray, CPortMgrDlg*>  CPortMgrDlgArray;
	CWinThread* m_p_unsafe_window_closer_thread;
	HANDLE m_hEventKill;

	void LogTimeStamp();
	void LogRunComplete();

protected: 

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	DECLARE_DYNAMIC(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow = -1);
	BOOL IsPlatformVista();
 	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void ResizeWindow(void);
	virtual void RecalcLayout(BOOL bNotify = TRUE);

	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
	afx_msg void OnBnClickedStartStopToggle();
	afx_msg void OnBnClickedAutoScan();
	afx_msg void OnOptionsConfigurationMenu();
//	afx_msg void OnOptionsCleanRegistryMenu();
	afx_msg void OnUpdateOptionsConfiguration(CCmdUI *pCmdUI);
	afx_msg void OnUpdateExit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateStartStopToggle(CCmdUI *pCmdUI);
	afx_msg void OnUpdateAutoScan(CCmdUI *pCmdUI);
    afx_msg LRESULT OnLogEvent(WPARAM _event_type, LPARAM _event_data);
    afx_msg LRESULT OnLogOpEvent(WPARAM _event_type, LPARAM _event_data);
	afx_msg LRESULT OnStatusUpdate(WPARAM _status, LPARAM _elapsedTime);
	afx_msg LRESULT OnStatusUpdateVersion(WPARAM _wparam, LPARAM _lparam);
	afx_msg LRESULT OnChangeProfile(WPARAM _wparam, LPARAM _lparam);
	afx_msg LRESULT OnIsStopped(WPARAM _wparam, LPARAM _lparam);
//	afx_msg void OnViewLog();
	void SetTitle(void);
	BOOL WaitForOpsToFinish( LONGLONG secs );
	static const CRect s_rectDefault;
    static const TCHAR s_profileHeading[];
    static const TCHAR s_profileRect[];
    static const TCHAR s_profileIcon[];
    static const TCHAR s_profileMax[];
    BOOL m_bFirstTime;

	CConfigMgrSheet *m_p_config_mgr;
	CPortMgrDlgArray m_port_dlg_array;
	CStDialogBar   m_DlgBar;
	CStatusBar   m_wndStatusBar;
	e_START_STATE m_start;

//	CRegScrubDlg *m_pRegScrubDlg;
    CEventLogger *m_p_event_logger;     // used for update operation logging.


public:
	CPortMgrDlg * GetPortDlg(INT_PTR _index) { return m_port_dlg_array[_index]; }; 
};


