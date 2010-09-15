// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include <afxpriv.h>
#include "StMfgTool.h"
#include "MainFrm.h"
#include "stmsg.h"
#include "defaultprofile.h"
#include "..\\..\\Libs\\WinSupport\\StSplashWnd.h"
#ifdef RESTRICTED_PC_IDS
extern TCHAR g_ThisPCId[];
#endif

#include "../../Libs/DevSupport/WindowsVersionInfo.h"

HANDLE g_ConfigActiveEvent;
extern HANDLE g_HIDMutex;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame
// for remembering the position and state of the frame across executions
const CRect CMainFrame::s_rectDefault(10, 10, 1255, 0637);  // static
const TCHAR CMainFrame::s_profileHeading[] = _T("Window size");
const TCHAR CMainFrame::s_profileRect[] = _T("Rect");
const TCHAR CMainFrame::s_profileIcon[] = _T("icon");
const TCHAR CMainFrame::s_profileMax[] = _T("max");

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(ID_START_STOP_TOGGLE, OnBnClickedStartStopToggle)     // user pressed the start/stop button
	ON_COMMAND(ID_OPTIONS_CONFIGURATION, OnOptionsConfigurationMenu)	// user selected the Configuration menu
	ON_COMMAND(ID_OPTIONS_CLEAN_REGISTRY, OnOptionsCleanRegistryMenu)	// user selected the Clean Registry menu
	ON_UPDATE_COMMAND_UI(ID_START_STOP_TOGGLE, OnUpdateStartStopToggle) // gray out the start button
//	ON_UPDATE_COMMAND_UI(IDCANCEL, OnUpdateExit)						// gray out the exit button
	ON_WM_DESTROY()
//	ON_COMMAND(ID_VIEW_LOG, OnViewLog)
	ON_WM_CLOSE()
    ON_MESSAGE(WM_MSG_LOGEVENT, OnLogEvent)
    ON_MESSAGE(WM_MSG_LOG_OP_EVENT, OnLogOpEvent)
	ON_MESSAGE(WM_MSG_UPDATE_STATUS, OnStatusUpdate)
	ON_MESSAGE(WM_MSG_ISSTOPPED, OnIsStopped)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction
CMainFrame::CMainFrame()
: m_start(STOPPED)
, m_bFirstTime(TRUE)
, m_p_config_mgr(NULL)
, m_pRegScrubDlg(NULL)
, m_p_unsafe_window_closer_thread(NULL)
, m_p_event_logger(NULL)
{
	m_bAutoMenuEnable = FALSE;

//    _CrtSetBreakAlloc (885);        //Chris this is the function...it's actually a macro.

}

CMainFrame::~CMainFrame()
{
	if(m_pRegScrubDlg)
		delete m_pRegScrubDlg;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// removing WS_THICKFRAME makes main window NOT sizeable
	// todo: clw need to play with  | FWS_SNAPTOBARS
	cs.style = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU /*| FWS_SNAPTOBARS*//*| WS_THICKFRAME */;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    cs.lpszClass = _T("CStMfgToolAppClass");
	return TRUE;
}

// CMainFrame diagnostics
#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
	dc << "CMainFrame = " << this;
}
#endif //_DEBUG

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Set the icon for the frame.
	HICON m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	g_ConfigActiveEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	// get the configuration manager
	m_p_config_mgr = theApp.GetConfigMgr();
	if ( m_p_config_mgr == NULL ) {
		TRACE0("Failed to get the configuration manager\n");
		return -1;      // fail to create
	}

	CRegCheckDlg *pRegCheckDlg = new CRegCheckDlg(m_p_config_mgr);;
	if ( pRegCheckDlg == NULL ) {
		TRACE0("Failed to get the RegCheckDlg\n");
		return -1;      // fail to create
	}
	else
		pRegCheckDlg->DoModal();
	delete(pRegCheckDlg);
	pRegCheckDlg = NULL;

	// create RegScrub
	m_pRegScrubDlg = new CRegScrubDlg();;
	if ( m_pRegScrubDlg == NULL ) {
		TRACE0("Failed to get the RegScrubDlg\n");
		return -1;      // fail to create
	}

	// turn on/off the AutoPlay rejection mechanism
	bool rejectAutoPlay = m_p_config_mgr->GetAutoPlayEnabled() == TRUE;
	CString driveList = m_p_config_mgr->GetAutoPlayDrvList();
	gDeviceManager::Instance().SetCancelAutoPlay(rejectAutoPlay, driveList);

	// start the thread that watches for unsafe removal dialogs and closes them
	if( gWinVersionInfo().IsWin2K() ) {
		m_hEventKill = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_p_unsafe_window_closer_thread = AfxBeginThread( UnsafeWindowCloserThreadProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED );
		m_p_unsafe_window_closer_thread->ResumeThread();
	}

	// create the status bar
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// create the status, start/stop button bar
	if (!m_DlgBar.Create(this, IDD_BUTTON_DLG,
		CBRS_BOTTOM|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_BORDER_ANY, AFX_IDW_CONTROLBAR_FIRST))
	{
		TRACE0("Failed to create Button DlgBar\n");
		return -1;      // fail to create
	}



// Open an event logging thread if the value was set in the registry.
    if (theApp.GetProfileInt(_T("Options"), _T("Enable Event Logging"), 1))
    {
        m_p_event_logger = new CEventLogger(/*TODO: pass the file handle to the constructor*/);
	    if (!m_p_event_logger->CreateThread()) {
		    ATLTRACE("Warning: Unable to create event logger thread\n");
		    delete m_p_event_logger;
		    m_p_event_logger = NULL;
        }
    }

	// create the Port Dialogs
	for ( UINT i=0; i<m_p_config_mgr->GetMaxPorts(); ++i ) {
		CPortMgrDlg * new_port_dlg = new CPortMgrDlg(this, i);
		if (new_port_dlg && new_port_dlg->Create(this, IDD_PORT_DLG,
			CBRS_LEFT|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_BORDER_ANY, AFX_IDW_CONTROLBAR_FIRST + 1 + i)) 
		{
				m_port_dlg_array.Add(new_port_dlg);
		}
		else {
			TRACE1("Failed to create Port %d DlgBar\n", i+1);
			return -1;      // fail to create
		}
	}

	//SetTitle();

	m_DlgBar.SetDisplay(m_p_config_mgr);
	m_DlgBar.SetProfiles();
	m_DlgBar.SelectProfile( m_DlgBar.WhatProfile());
	m_DlgBar.SetProfileStatus();

	if (m_p_config_mgr->IsPlayerProfileValid() &&
		m_p_config_mgr->GetNumEnabledPorts()   &&
		m_p_config_mgr->GetNumEnabledOps())
	{
		m_DlgBar.Enable();
	}
	else
		m_DlgBar.Disable();


	return 0;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	// Route messages to the splash screen while it is visible
    if (CStSplashWnd::PreTranslateAppMessage(pMsg)) 
	{
		return TRUE;
	}
	
	return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnBnClickedStartStopToggle()
{
	HWND hwnd; CPortMgrDlg* dlg;
	CMenu* mmenu = GetMenu();
	CString resStr;
	switch ( m_start )
	{
		case STOPPED:
			m_start = RUNNING;
			// don't let the user change the configuration while we are running
			m_DlgBar.Start(m_p_config_mgr->GetPlayerProfileName());
			mmenu->EnableMenuItem(ID_OPTIONS_CONFIGURATION, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			mmenu->EnableMenuItem(ID_OPTIONS_CLEAN_REGISTRY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			mmenu->EnableMenuItem(ID_APP_EXIT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

			// make this the last saved profile in the registry
			AfxGetApp()->WriteProfileString(_T("Player Profile"), _T("Player Description"), m_p_config_mgr->GetPlayerProfileName());

#ifdef SERIALIZE_HID
			WaitForSingleObject(g_HIDMutex, INFINITE);
#endif
            // Write the time stamp and profile name to the log file
            LogTimeStamp();
			// now tell all the CPortMgrDlg(s) the start button has been pressed
			for(hwnd = ::GetWindow(m_hWnd,GW_CHILD); hwnd; hwnd = ::GetWindow(hwnd,GW_HWNDNEXT))	{
				dlg = (CPortMgrDlg*)FromHandle(hwnd);
				if( dlg && dlg->IsKindOf( RUNTIME_CLASS( CPortMgrDlg ) ) ) {
					if ( dlg->GetUsbPort() != NULL )
					{
						dlg->PostMessage(WM_MSG_PD_EVENT, (WPARAM)CPortMgrDlg::PD_EVNT_START, 0);
					}
				} // end if (CPortMgrDlg)
			} // end for CHILD(s) of the CMainFrame

#ifdef SERIALIZE_HID
			Sleep(2000);  // let all get on the hook then let them go
			ReleaseMutex(g_HIDMutex);
#endif
			break;
		case RUNNING:
			m_start = WAITING;
			m_DlgBar.Stop();
			// now tell all the CPortMgrDlg(s) the stop button has been pressed
			for(hwnd = ::GetWindow(m_hWnd,GW_CHILD); hwnd; hwnd = ::GetWindow(hwnd,GW_HWNDNEXT))	{
				dlg = (CPortMgrDlg*)FromHandle(hwnd);
				if( dlg && dlg->IsKindOf( RUNTIME_CLASS( CPortMgrDlg ) ) ) {
					if ( dlg->GetUsbPort() != NULL )
						dlg->PostMessage(WM_MSG_PD_EVENT, (WPARAM)CPortMgrDlg::PD_EVNT_STOP, 0);
				} // end if (CPortMgrDlg)
			} // end for CHILD(s) of the CMainFrame
			//LogRunComplete();
			break;
		case WAITING:
			m_start = STOPPED;
			// allow the user change the configuration since we are not running
			m_DlgBar.Idle();
			mmenu->EnableMenuItem(ID_OPTIONS_CONFIGURATION, MF_BYCOMMAND | MF_ENABLED);
			if( !IsPlatformVista() )
				mmenu->EnableMenuItem(ID_OPTIONS_CLEAN_REGISTRY, MF_BYCOMMAND | MF_ENABLED);
			mmenu->EnableMenuItem(ID_APP_EXIT, MF_BYCOMMAND | MF_ENABLED);

			break;
	}
}

BOOL CMainFrame::WaitForOpsToFinish( LONGLONG secs )
{
	int working = 1;
	int count = 0;
	CPortMgrDlg* dlg;

	while ( working && count < secs ) {
		working = 0;
		for (int i=0; i<m_port_dlg_array.GetCount(); ++i ) {
			dlg = m_port_dlg_array.GetAt(i);
			if ( dlg->GetUsbPort() != NULL ) {
				if ( dlg->GetOpMode() == CPortMgrDlg::OPMODE_RUNNING || 
					 dlg->GetOpMode() == CPortMgrDlg::OPMODE_INVALID )
					 ++working;
			}
		}
		if ( working )
			Sleep(1000);
	}
	return (working == 0); // success = 1
}


// gray out the Start button if there is no valid Player 
// Profile or no assigned Port Dlgs
void CMainFrame::OnUpdateExit(CCmdUI *pCmdUI)
{
	if ( m_start == STOPPED )
	{
		m_DlgBar.Enable();
		pCmdUI->Enable(TRUE);
	}
	else if ( m_start == WAITING || m_start == RUNNING )
	{
		m_DlgBar.Disable();
		pCmdUI->Enable(FALSE);
	}

}


// gray out the Start button if there is no valid Player 
// Profile or no assigned Port Dlgs
void CMainFrame::OnUpdateStartStopToggle(CCmdUI *pCmdUI)
{
	if ( m_p_config_mgr->IsPlayerProfileValid() &&
		 m_p_config_mgr->GetNumEnabledPorts()   &&
		 m_p_config_mgr->GetNumEnabledOps()     && 
		 m_start != WAITING ) {
		pCmdUI->Enable(TRUE);
	}
	else {
		if ( m_start == WAITING ) {
			int working = 0;
			for (int i=0; i<m_port_dlg_array.GetCount(); ++i ) {
				CPortMgrDlg * dlg = m_port_dlg_array.GetAt(i);
				if ( dlg->GetUsbPort() != NULL ) {
					if ( dlg->GetOpMode() == CPortMgrDlg::OPMODE_RUNNING )
						++working;
				}
			}
			if ( !working ) {
				m_DlgBar.Enable();
				OnBnClickedStartStopToggle();
				pCmdUI->Enable(TRUE);
				return;
			}
		} // if ( waiting )
		pCmdUI->Enable(FALSE);
	}
}


// this launches the clean registry dialog when someone
// clicks the Clean Registry Menu Item
void CMainFrame::OnOptionsCleanRegistryMenu()
{
	if (m_p_config_mgr->IsPlayerProfileValid())
	{
		m_pRegScrubDlg->DoModal();
	}
	else  
	{
		MessageBeep(0);
		CString resStr;
		resStr.LoadStringW(IDS_VALID_PROFILE_TO_SCRUB);
		AfxMessageBox(resStr, MB_ICONSTOP | MB_OK);
	}

}


// this launches the configuration dialog when someone
// clicks the Configuration Menu Item
void CMainFrame::OnOptionsConfigurationMenu()
{
	BOOL profileChanges = FALSE;
	UINT uPorts = m_p_config_mgr->GetMaxPorts();

	SetEvent( g_ConfigActiveEvent );
	if (m_p_config_mgr->DoModal() == IDOK)
	{
		profileChanges = TRUE;
		// save current profile selection from DlgBar
		// Remove and re-populate Profile selection combobox (profiles may have beed added/removed)
		// Try to select previously selected profile.
		CString csCurrentProfile;// = m_DlgBar.WhatProfile();
		m_DlgBar.ClearProfiles();
		m_DlgBar.SetProfiles();

		if( csCurrentProfile.IsEmpty() )
			m_DlgBar.SelectProfile(m_p_config_mgr->GetPlayerProfileName());
		else
			m_DlgBar.SelectProfile(csCurrentProfile);

		if (m_p_config_mgr->IsPlayerProfileValid() &&
			m_p_config_mgr->GetNumEnabledPorts()   &&
			m_p_config_mgr->GetNumEnabledOps())
		{
			m_DlgBar.Enable();
		}
		else
			m_DlgBar.Disable();
	}
	ResetEvent( g_ConfigActiveEvent );

	if( profileChanges || theApp.GetProfileInt(_T("Options"), _T("Refresh"), FALSE ) )
	{
		for (int i=0; i<m_port_dlg_array.GetCount(); ++i ) 
		{
			CPortMgrDlg * dlg = m_port_dlg_array.GetAt(i);
			if ( dlg->GetUsbPort() != NULL ) 
			{
				if ( dlg->GetOpMode() == CPortMgrDlg::OPMODE_MONITOR )
				{
					dlg->KillMonitor();
				}
			}
		}
		m_port_dlg_array.RemoveAll();

		// Destroy all the PortMgrDlg windows
		for (UINT i = 0; i < uPorts; ++i)
		{
			for (HWND hWndChild = ::GetTopWindow(m_hWnd); hWndChild != NULL;
				hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
			{
				CWnd* pWnd = CWnd::FromHandlePermanent(hWndChild);

				if (pWnd->IsKindOf( RUNTIME_CLASS( CPortMgrDlg ) ) ){
					pWnd->DestroyWindow();
					break;
				}
			}
		}


		// Re-create the Port Dialogs
		for ( UINT i=0; i<m_p_config_mgr->GetMaxPorts(); ++i ) {
			CPortMgrDlg * new_port_dlg = new CPortMgrDlg(this, i);
			if (new_port_dlg && new_port_dlg->Create(this, IDD_PORT_DLG,
				CBRS_LEFT|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_BORDER_ANY, AFX_IDW_CONTROLBAR_FIRST + 1 + i)) 
			{
					m_port_dlg_array.Add(new_port_dlg);
			}
			else {
				TRACE1("Failed to create Port %d DlgBar\n", i+1);
				return;      // fail to create
			}
		}

		theApp.WriteProfileInt(_T("Options"), _T("Refresh"), 0x0000);

		// This resizes appropriately and displays the window
		ActivateFrame();
	}
	else
	{
		// tell all the CPortMgrDlg(s) the configuration may have changed. Even though we may have cancelled,
		// we might have re-generated the profiles or operations lists.
		HWND hwnd;
		for(hwnd = ::GetWindow(this->m_hWnd,GW_CHILD); hwnd; hwnd = ::GetWindow(hwnd,GW_HWNDNEXT))
		{
			if( FromHandle(hwnd)->IsKindOf( RUNTIME_CLASS( CPortMgrDlg ) ) )
			{
				::PostMessage(hwnd, WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_CONFIG_CHANGE, 0);
			} // end if (CPortMgrDlg)
		} // end for CHILD(s) of the CMainFrame
	}



	// add Filters to DeviceManager
//clw	if ( m_p_config_mgr->GetPlayerProfile() && m_p_config_mgr->GetPlayerProfile()->IsValid() )
//clw	{
//clw		gDeviceManager::Instance().UpdateDeviceFilters(m_p_config_mgr->GetPlayerProfile()->GetUsbVid(), m_p_config_mgr->GetPlayerProfile()->GetUsbPid());
//clw	}

	// tell all the CPortMgrDlg(s) the configuration may have changed
//	HWND hwnd;
//	for(hwnd = ::GetWindow(this->m_hWnd,GW_CHILD); hwnd; hwnd = ::GetWindow(hwnd,GW_HWNDNEXT))	{
//		if( FromHandle(hwnd)->IsKindOf( RUNTIME_CLASS( CPortMgrDlg ) ) ) {
//			::PostMessage(hwnd, WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_CONFIG_CHANGE, 0);
//		} // end if (CPortMgrDlg)
//	} // end for CHILD(s) of the CMainFrame

    // Start or stop the event logging thread
    if (theApp.GetProfileInt(_T("Options"), _T("Enable Event Logging"), 1))
    {
        if (m_p_event_logger == NULL)
        {
            m_p_event_logger = new CEventLogger(/*TODO: pass the file handle to the constructor*/);
	        if (!m_p_event_logger->CreateThread()) {
		        ATLTRACE("Warning: Unable to create event logger thread\n");
		        delete m_p_event_logger;
		        m_p_event_logger = NULL;
            }
        }
    }
    else if (m_p_event_logger)
    {
        m_p_event_logger->PostThreadMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_KILL, 0);
		WaitForSingleObject(m_p_event_logger->m_hThread, INFINITE);
        m_p_event_logger = NULL;
    }
}

// adds the current player profile to the main window title bar
void CMainFrame::SetTitle(void)
{
	CString csTitle, csTemp;

	GetWindowText(csTitle);
	csTemp = csTitle.Left(csTitle.Find(_T(" - ")));
	if ( csTemp.IsEmpty()) {
		csTitle.Append(_T(" - "));
	}
	else {
		csTitle.Format(_T("%s - "), csTemp);
	}
	if (m_p_config_mgr->IsPlayerProfileValid()) {
		csTitle.Append(m_p_config_mgr->GetPlayerProfileName());
	}
	else {
		CString resStr;
		resStr.LoadString(IDS_INVALID_PLAYER_PROFILE);
		csTitle.Append(resStr);
	}
	SetWindowText(csTitle);
}

void CMainFrame::OnBnClickedCancel()
{
	OnClose();
}

void CMainFrame::OnClose()
{
	bool working = false;

	if ( m_start != STOPPED ) 
	{
		for (int i=0; i<m_port_dlg_array.GetCount(); ++i ) 
		{
			CPortMgrDlg * dlg = m_port_dlg_array.GetAt(i);
			if ( dlg->GetUsbPort() != NULL ) 
			{
				if ( dlg->GetOpMode() == CPortMgrDlg::OPMODE_RUNNING )
					working=true;
			}
		}
	}

	if ( working )
	{
		CString resStr;
		resStr.LoadString(IDS_STOP_REQUIRED);
		AfxMessageBox(resStr, MB_ICONINFORMATION | MB_OK);
	}
	else
	{
		for (int i=0; i<m_port_dlg_array.GetCount(); ++i ) 
		{
			CPortMgrDlg * dlg = m_port_dlg_array.GetAt(i);
			if ( dlg->GetUsbPort() != NULL ) 
			{
				if ( dlg->GetOpMode() == CPortMgrDlg::OPMODE_MONITOR )
				{
					dlg->KillMonitor();
				}
			}
		}

		// todo: clw - stop running threads?
		// stop the thread that closes the Win 2k 'unsafe removal' dialog
		if (m_p_unsafe_window_closer_thread) 
		{
			VERIFY(SetEvent(m_hEventKill));
			DWORD thread_id = m_p_unsafe_window_closer_thread->m_nThreadID;
			DWORD ret = WaitForSingleObject(m_p_unsafe_window_closer_thread->m_hThread, 10000);
			CloseHandle(m_hEventKill);
			ATLTRACE(_T("== Killed UnsafeWindowCloserThreadProc %s. (%#x, %d)\n"), ret?_T("ERROR"):_T("successfully"), thread_id, thread_id);
		}

        // Close the log file and end the update logger thread
        if (m_p_event_logger)
		{
            m_p_event_logger->PostThreadMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_KILL, 0);
			WaitForSingleObject(m_p_event_logger->m_hThread, INFINITE);
		}
		CFrameWnd::OnClose();
	}
}

// restores the windows last position
void CMainFrame::ActivateFrame(int nCmdShow)
{
    CString strText;
    BOOL bIconic, bMaximized;
    UINT flags;
    WINDOWPLACEMENT wndpl;
    CRect org_rect, rect;
	int curPos= 0;

	CMenu* mmenu = GetMenu();
	if( IsPlatformVista() )
		mmenu->EnableMenuItem(ID_OPTIONS_CLEAN_REGISTRY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	// turn off the logger for now
//	mmenu->EnableMenuItem(ID_VIEW_LOG, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

    // this puts all the PortDlg in their proper place
	ResizeWindow();
	GetWindowRect(&org_rect);

//	if (m_bFirstTime) {
        m_bFirstTime = FALSE;
        strText = AfxGetApp()->GetProfileString(s_profileHeading,
                                                s_profileRect);
        if (!strText.IsEmpty()) {
			rect.left = _tstoi(strText.Tokenize(_T(" "), curPos));
            rect.top = _tstoi(strText.Tokenize(_T(" "), curPos));
			rect.right = rect.left + org_rect.Width();
			rect.bottom = rect.top + org_rect.Height();
        }
        else {
			rect = org_rect;
        }

        bIconic = AfxGetApp()->GetProfileInt(s_profileHeading,
                                             s_profileIcon, 0);
        bMaximized = AfxGetApp()->GetProfileInt(s_profileHeading,
                                                s_profileMax, 0);   
        if (bIconic) {
            nCmdShow = SW_SHOWMINNOACTIVE;
            if (bMaximized) {
                flags = WPF_RESTORETOMAXIMIZED;
            }
            else {
                flags = WPF_SETMINPOSITION;
            }
        }
        else {
            if (bMaximized) {
                nCmdShow = SW_SHOWMAXIMIZED;
                flags = WPF_RESTORETOMAXIMIZED;
            }
            else {
                nCmdShow = SW_NORMAL;
                flags = WPF_SETMINPOSITION;
            }
        }
        wndpl.length = sizeof(WINDOWPLACEMENT);
        wndpl.showCmd = nCmdShow;
        wndpl.flags = flags;
        wndpl.ptMinPosition = CPoint(0, 0);
        wndpl.ptMaxPosition =
            CPoint(-::GetSystemMetrics(SM_CXBORDER),
                   -::GetSystemMetrics(SM_CYBORDER));
        wndpl.rcNormalPosition = rect;

		// sets window's position and minimized/maximized status
        BOOL bRet;
		bRet = SetWindowPlacement(&wndpl);
//    }
    CFrameWnd::ActivateFrame(nCmdShow);
}

// saves the window's last position
void CMainFrame::OnDestroy()
{
    CString strText;
    BOOL bIconic = FALSE, bMaximized = FALSE;

    WINDOWPLACEMENT wndpl;
    wndpl.length = sizeof(WINDOWPLACEMENT);
    // gets current window position and
    //  iconized/maximized status
    BOOL bRet;
	bRet = GetWindowPlacement(&wndpl);
    if (wndpl.showCmd == SW_SHOWNORMAL) {
        bIconic = FALSE;
        bMaximized = FALSE;
    }
    else if (wndpl.showCmd == SW_SHOWMAXIMIZED) {
        bIconic = FALSE;
        bMaximized = TRUE;
    } 
    else if (wndpl.showCmd == SW_SHOWMINIMIZED) {
        bIconic = TRUE;
        if (wndpl.flags) {
            bMaximized = TRUE;
        }
        else {
            bMaximized = FALSE;
        }
    }
    strText.Format(_T("%04d %04d %04d %04d"),
                   wndpl.rcNormalPosition.left,
                   wndpl.rcNormalPosition.top,
                   wndpl.rcNormalPosition.right,
                   wndpl.rcNormalPosition.bottom);
    AfxGetApp()->WriteProfileString(s_profileHeading,
                                    s_profileRect, strText);
    AfxGetApp()->WriteProfileInt(s_profileHeading,
                                 s_profileIcon, bIconic);
    AfxGetApp()->WriteProfileInt(s_profileHeading,
                                 s_profileMax, bMaximized);
    CFrameWnd::OnDestroy();
}

// arranges the Port Mgr Dlgs
void CMainFrame::RecalcLayout(BOOL bNotify)
{
	CFrameWnd::RecalcLayout(bNotify);

	AFX_SIZEPARENTPARAMS layout;
	HWND hWndLeftOver = NULL;

	layout.bStretch = FALSE;
	layout.sizeTotal.cx = layout.sizeTotal.cy = 0;
	GetClientRect(&layout.rect);    // starting rect comes from client rect
	layout.hDWP = NULL; // not actually doing layout

	CRect bar_rect, new_rect; CSize size; INT_PTR i=0, row=0, col=0;
	for (HWND hWndChild = ::GetTopWindow(m_hWnd); hWndChild != NULL;
		hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
	{
		UINT_PTR nIDC = ::GetDlgCtrlID(hWndChild);
		CWnd* pWnd = CWnd::FromHandlePermanent(hWndChild);
		if (pWnd != NULL) {
			::SendMessage(hWndChild, WM_SIZEPARENT, 0, (LPARAM)&layout);
			if (pWnd->IsKindOf( RUNTIME_CLASS( CPortMgrDlg ) ) ){
				row = i/4; col = i%4;
				size = ((CDialogBar*)pWnd)->m_sizeDefault;
				new_rect.SetRect(size.cx*col, size.cy*row, 
								 size.cx*col+size.cx, size.cy*row+size.cy);
				pWnd->MoveWindow(&new_rect, FALSE);
				++i;
			}
		}
	}
}

// snaps the main frame to the Port Dlgs
void CMainFrame::ResizeWindow(void)
{
	WINDOWPLACEMENT wndpl;
	CRect r_port_group_ctrl;
	RecalcLayout(FALSE);

	m_DlgBar.GetWindowRect(&r_port_group_ctrl);
	int button_bar_height = r_port_group_ctrl.Height() + GetSystemMetrics(SM_CYMENU)*2;

	CWnd *pWnd = m_port_dlg_array[m_port_dlg_array.GetSize()-1]->GetDlgItem(ID_PORT_GROUP_BOX);
	pWnd->GetWindowRect(&r_port_group_ctrl);
	r_port_group_ctrl.InflateRect(0,0,10,10+button_bar_height);
	ScreenToClient(&r_port_group_ctrl);
	r_port_group_ctrl.top = r_port_group_ctrl.left = 0;
	ClientToScreen(&r_port_group_ctrl);
	CalcWindowRect(&r_port_group_ctrl, CWnd::adjustBorder);
	
	GetWindowPlacement(&wndpl);
	wndpl.showCmd = SW_HIDE; 
	wndpl.rcNormalPosition = r_port_group_ctrl;
	SetWindowPlacement(&wndpl);
}

// centers the start/stop button in the button bar
// after ResizeWindow changes the size of the main frame
void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	CRect r_frame, r_button;
	CWnd *pWnd;
	m_DlgBar.GetClientRect(&r_frame);

	// move the start button
	pWnd = m_DlgBar.GetDlgItem(ID_START_STOP_TOGGLE);
	pWnd->GetWindowRect(&r_button);
	m_DlgBar.ScreenToClient(&r_button);
	r_button.OffsetRect(r_frame.right - (r_button.right+7), 0);
	pWnd->MoveWindow(&r_button, TRUE);

}

//void CMainFrame::OnViewLog()
//{
//	theApp.GetLogMgrDlg()->ShowWindow(SW_SHOW);
//	theApp.GetLogMgrDlg()->LogIt( theApp.GetUSBPortMgr()->GetPortDataStr() );
//}

//----------------------------------------------------------------
// LogTimeStamp()
//
// Write the current time and profile name to the log file.
//----------------------------------------------------------------
void CMainFrame::LogTimeStamp()
{
    CTime time = CTime::GetCurrentTime();
    CString cstr_time = _T("\n");
    cstr_time += time.Format("%#c");
    cstr_time += _T("\n");
#ifdef RESTRICTED_PC_IDS
	cstr_time += _T("System ID: ");
	cstr_time += g_ThisPCId;
    cstr_time += _T("\n");
#endif

    BSTR bstr_time = cstr_time.AllocSysString();
    PostMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_time);
}

//----------------------------------------------------------------
// LogRunComplete()
//
// Write the current time and profile name to the log file.
//----------------------------------------------------------------
void CMainFrame::LogRunComplete()
{
	CString text;
	text.Format(_T("**Successful Operations:\t%d\n**Failed Operations:\t%d\n"),
		m_DlgBar.m_SuccessfulOps, m_DlgBar.m_ErrorOps);
	LogTimeStamp();
    BSTR bstr_log_text = text.AllocSysString();
    PostMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
}

//----------------------------------------------------------------
// OnLogEvent()
//
// Write an event information string to the log file.
//----------------------------------------------------------------
LRESULT CMainFrame::OnLogEvent(WPARAM _event_type, LPARAM _event_data)
{
	if (m_p_event_logger)
        m_p_event_logger->PostThreadMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, _event_data);

    return TRUE;
}

//----------------------------------------------------------------
// OnLogOpEvent()
//
// Write an event information string to the log file.  Prefix it with the 
// current count of ops.
//----------------------------------------------------------------
LRESULT CMainFrame::OnLogOpEvent(WPARAM _event_type, LPARAM _event_data)
{
    CString text;
	BSTR bstr_log_text;

	text.Format(_T("%d - %s"), m_DlgBar.m_SuccessfulOps + m_DlgBar.m_ErrorOps + 1,(LPCTSTR)_event_data);
	bstr_log_text = text.AllocSysString();

	if (m_p_event_logger)
        m_p_event_logger->PostThreadMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM) bstr_log_text);

	return TRUE;
}

LRESULT CMainFrame::OnStatusUpdate(WPARAM _status, LPARAM _elapsedTime)
{
	m_DlgBar.UpdateData((BOOL)_status, (ULONG) _elapsedTime);

    return TRUE;
}

LRESULT CMainFrame::OnIsStopped(WPARAM _wparam, LPARAM _lparam)
{
	int working = 0;
	CPortMgrDlg* dlg;

	for (int i=0; i<m_port_dlg_array.GetCount(); ++i ) {
		dlg = m_port_dlg_array.GetAt(i);
		if ( dlg->GetUsbPort() != NULL ) {
			if ( dlg->GetOpMode() == CPortMgrDlg::OPMODE_RUNNING || 
				 dlg->GetOpMode() == CPortMgrDlg::OPMODE_INVALID )
				 ++working;
		}
	}
	
	if ( !working )
	{
		CMenu* mmenu = GetMenu();

		m_start = STOPPED;
		m_DlgBar.Idle();
		mmenu->EnableMenuItem(ID_OPTIONS_CONFIGURATION, MF_BYCOMMAND | MF_ENABLED);
		if( !IsPlatformVista() )
			mmenu->EnableMenuItem(ID_OPTIONS_CLEAN_REGISTRY, MF_BYCOMMAND | MF_ENABLED);
		mmenu->EnableMenuItem(ID_APP_EXIT, MF_BYCOMMAND | MF_ENABLED);
		LogRunComplete();
	}
	return TRUE;
}


UINT UnsafeWindowCloserThreadProc( LPVOID pParam )
{
	CMainFrame * p_frame_class = (CMainFrame*)pParam;
	CWnd* wnd = NULL;
	CString resStr;

	resStr.LoadString(IDS_UNSAFE_DEVICE_REMOVAL);
	ATLTRACE(_T("== UnsafeWindowCloserThreadProc (%#x, %d)\n"), p_frame_class->m_p_unsafe_window_closer_thread->m_nThreadID, p_frame_class->m_p_unsafe_window_closer_thread->m_nThreadID);
	while( WaitForSingleObject(p_frame_class->m_hEventKill, 0) == WAIT_TIMEOUT) {
		wnd = CWnd::FindWindow(NULL, resStr);
		if( wnd ) {
			wnd->SendMessage( WM_CLOSE );
			ATLTRACE("== Closed an Unsafe Removal window.\n");
			wnd = NULL;
		}
		Sleep(25);
	}
	return 0;
}


BOOL CMainFrame::IsPlatformVista()
{ 
    OSVERSIONINFOA osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA((OSVERSIONINFOA*)&osvi);
    if (osvi.dwMajorVersion >= 6)
		return TRUE;
	else
		return FALSE;
}