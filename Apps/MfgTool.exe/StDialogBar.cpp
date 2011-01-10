/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "StdAfx.h"
#include "StDialogBar.h"
#include "MainFrm.h"
#include "stmfgtool.h"

#define ELAPSED_TIMER		3999

#define SECONDS_PER_HR		3600
#define SECONDS_PER_MIN	60

CStDialogBar::CStDialogBar()
{
	m_TimerActive = FALSE;
	m_hDataAccess = CreateMutex(NULL, FALSE, NULL);
	m_SuccessfulOps = 0;
	m_ErrorOps = 0;
	m_ulTotalTicks.QuadPart = 0;
	m_bCheckForStop = FALSE;
	m_SettingsEnabled = FALSE;
}

CStDialogBar::~CStDialogBar(void)
{
	CloseHandle(m_hDataAccess);
}



BEGIN_MESSAGE_MAP(CStDialogBar, CDialogBar)
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_STATUS_PROFILE_COMBO, &CStDialogBar::OnCbnSelchangeStatusProfileCombo)
	ON_CBN_SELCHANGE(IDC_STATUS_MULTI_COMBO_PROFILE, &CStDialogBar::OnCbnSelchangeStatusProfileCombo)
END_MESSAGE_MAP()

// This OnCtlColor handler will change the color of a static control
// with the ID of IDC_STATUS_STATUS.
HBRUSH CStDialogBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   // Call the base class implementation first! Otherwise, it may
   // undo what we're trying to accomplish here.
   HBRUSH hbr = CDialogBar::OnCtlColor(pDC, pWnd, nCtlColor);

   // Are we painting the IDC_STATUS_STATUS control? We can use
   // CWnd::GetDlgCtrlID() to perform the most efficient test.
   if (pWnd->GetDlgCtrlID() == IDC_STATUS_STATUS)
   {
		if( m_p_config_mgr->IsPlayerProfileValid() &&
			m_p_config_mgr->GetNumEnabledPorts()   &&
			m_p_config_mgr->GetNumEnabledOps())
		{
			// Set the text color to red
			pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		}
		else
		{
			pDC->SetTextColor(RGB(200,0,0));
		}
   }

   // Return handle to our CBrush object
   return hbr;
}

void CStDialogBar::SetDisplay(CConfigMgrSheet *_p_config_mgr)
{
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(logFont));
	CString resStr;

	GetLocaleInfo( LOCALE_USER_DEFAULT,				// locale identifier
					LOCALE_STIME,					// information type
					(LPWSTR)&m_TimeSeparator,		// information buffer
					4);								// size of buffer

	//  Create a font for the Start/Stop button text
	// Since design guide says toolbars are fixed height so is the font.
	logFont.lfHeight = -14;
	logFont.lfWeight = FW_BOLD;
	logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	_tcscpy_s(logFont.lfFaceName, LF_FACESIZE, _T("MS Shell Dlg"));
	if (!m_StartStopFont.CreateFontIndirect(&logFont))
			TRACE0("Could Not create font for Start/Stop\n");
	else
		GetDlgItem(ID_START_STOP_TOGGLE)->SetFont(&m_StartStopFont, TRUE);

	logFont.lfHeight = -12;
	if (!m_StoppingFont.CreateFontIndirect(&logFont))
			TRACE0("Could Not create Stopping... font Start/Stop\n");

   	m_StatusText.SubclassDlgItem(IDC_STATUS_STATUS, this);

//	GetDlgItem(IDC_STATUS_PROFILE)->SetWindowTextW(L"");
//	GetDlgItem(IDC_STATUS_VIDPID)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_STATUS)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_FW_VER)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_START)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_ELAPSED)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_AVG_DURATION)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_OK_UPDATES)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_FAILED_UPDATES)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_FAILURE_RATE)->SetWindowTextW(L"");

	resStr.LoadStringW(IDS_STATUS_GRP_TEXT);
	CVersionInfo * _info = theApp.GetVersionInfo();
	if ( _info )
	{
		resStr.AppendFormat(_T(" (v%s)"), _info->GetProductVersionAsString());
	}
	GetDlgItem(IDC_STATUS_GRP_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_PROFILE_TEXT);
	GetDlgItem(IDC_STATUS_PROFILE_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_STATUS_TEXT);
	GetDlgItem(IDC_STATUS_STATUS_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_FW_VER_TEXT);
	GetDlgItem(IDC_STATUS_FW_VER_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_START_TEXT);
	GetDlgItem(IDC_STATUS_START_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_ELAPSED_TEXT);
	GetDlgItem(IDC_STATUS_ELAPSED_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_AVG_DURATION_TEXT);
	GetDlgItem(IDC_STATUS_AVG_DURATION_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_OK_UPDATES_TEXT);
	GetDlgItem(IDC_STATUS_OK_UPDATES_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_FAILED_UPDATES_TEXT);
	GetDlgItem(IDC_STATUS_FAILED_UPDATES_TEXT)->SetWindowTextW(resStr);
	resStr.LoadStringW(IDS_STATUS_FAILURE_RATE_TEXT);
	GetDlgItem(IDC_STATUS_FAILURE_RATE_TEXT)->SetWindowTextW(resStr);

	m_StartStopBtn.SubclassDlgItem(ID_START_STOP_TOGGLE, this); 

	m_p_config_mgr = _p_config_mgr;

}

void CStDialogBar::OnTimer(UINT nEventID)
{
	if( m_TimerActive )
	{
		CString csTimeString;
		CTimeSpan TimeSpan = CTime::GetCurrentTime() - m_StartCTime;
		LONG secs, mins, hrs;

		secs = TimeSpan.GetSeconds();
		mins = TimeSpan.GetMinutes();
		hrs = (LONG)TimeSpan.GetTotalHours();
		csTimeString.Format(L"%d%s%02d%s%02d",
					hrs, m_TimeSeparator, mins, m_TimeSeparator, secs);

		GetDlgItem(IDC_STATUS_ELAPSED)->SetWindowTextW(csTimeString);

		if (m_bCheckForStop)
			((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_ISSTOPPED, 0, 0);
	}

	CDialogBar::OnTimer(nEventID);
}

void CStDialogBar::Start(LPCTSTR _csProfile)
{
	TCHAR		szStartTime[64];
	CString		resStr;
    time_t		ltime;
	CComboBox *pProfileCombo = GetProfileCombo();

	time( &ltime );
    _wctime_s( szStartTime, 64, &ltime );

	m_StartCTime = CTime::GetCurrentTime();

	if ( !GetDlgItem(ID_START_STOP_TOGGLE)->IsWindowEnabled() )
		GetDlgItem(ID_START_STOP_TOGGLE)->EnableWindow(TRUE);

	pProfileCombo->EnableWindow(FALSE);

	GetDlgItem(IDC_STATUS_START)->SetWindowTextW(szStartTime);
	GetDlgItem(IDC_STATUS_ELAPSED)->SetWindowTextW(L"0:0:0");
	GetDlgItem(IDC_STATUS_AVG_DURATION)->SetWindowTextW(L"");
	GetDlgItem(IDC_STATUS_OK_UPDATES)->SetWindowTextW(L"0");
	GetDlgItem(IDC_STATUS_FAILED_UPDATES)->SetWindowTextW(L"0");
	GetDlgItem(IDC_STATUS_FAILURE_RATE)->SetWindowTextW(L"0%");

	resStr.LoadStringW(IDS_STOP);
	GetDlgItem(ID_START_STOP_TOGGLE)->SetWindowTextW(resStr);

	m_StartStopBtn.SetColors(RGB_STOP_RED, RGB_BLACK);


	SetTimer(ELAPSED_TIMER, 1000, NULL);
	m_TimerActive = TRUE;
}

void CStDialogBar::Stop()
{
	CString resStr;

	GetDlgItem(ID_START_STOP_TOGGLE)->SetFont(&m_StoppingFont, TRUE);

	resStr.LoadStringW(IDS_STOPPING);
	GetDlgItem(ID_START_STOP_TOGGLE)->SetWindowTextW(resStr);
	m_StartStopBtn.SetColors(RGB_STOPPING_YELLOW, RGB_STOPPING_RED_TEXT);

	if( GetDlgItem(ID_START_STOP_TOGGLE)->IsWindowEnabled() )
		GetDlgItem(ID_START_STOP_TOGGLE)->EnableWindow(FALSE);

	m_bCheckForStop = TRUE;
}

void CStDialogBar::Idle()
{
	CString resStr;
	CComboBox *pProfileCombo = GetProfileCombo();

	if ( m_TimerActive )
	{
		KillTimer(ELAPSED_TIMER);
		m_TimerActive = FALSE;
	}
	m_bCheckForStop = FALSE;

	if ( !GetDlgItem(ID_START_STOP_TOGGLE)->IsWindowEnabled() )
		GetDlgItem(ID_START_STOP_TOGGLE)->EnableWindow(TRUE);

	GetDlgItem(ID_START_STOP_TOGGLE)->SetFont(&m_StartStopFont, TRUE);

	resStr.LoadStringW(IDS_START);
	GetDlgItem(ID_START_STOP_TOGGLE)->SetWindowTextW(resStr);
	m_StartStopBtn.SetColors(RGB_GO_GREEN, RGB_WHITE);

	if( !pProfileCombo->IsWindowEnabled() )
		pProfileCombo->EnableWindow(TRUE);

}

void CStDialogBar::Disable()
{

	GetDlgItem(ID_START_STOP_TOGGLE)->SetWindowTextW(L"");
	m_StartStopBtn.SetColors(GetSysColor (COLOR_BTNFACE), RGB_WHITE);

	if ( GetDlgItem(ID_START_STOP_TOGGLE)->IsWindowEnabled() )
		GetDlgItem(ID_START_STOP_TOGGLE)->EnableWindow(FALSE);

	if ( m_TimerActive )
	{
		KillTimer(ELAPSED_TIMER);
		m_TimerActive = FALSE;
	}

}

void CStDialogBar::Enable()
{
	CString resStr;

	if ( !GetDlgItem(ID_START_STOP_TOGGLE)->IsWindowEnabled() )
		GetDlgItem(ID_START_STOP_TOGGLE)->EnableWindow(TRUE);

	GetDlgItem(ID_START_STOP_TOGGLE)->SetFont(&m_StartStopFont, TRUE);

	resStr.LoadStringW(IDS_START);
	GetDlgItem(ID_START_STOP_TOGGLE)->SetWindowTextW(resStr);
	m_StartStopBtn.SetColors(RGB_GO_GREEN, RGB_WHITE);
}

void CStDialogBar::UpdateData(BOOL _statusOK, DWORD _tickCount)
{
	CString csDisplay;
	DWORD dwAvgTickCount, dwHrs, dwMins, dwSecs;

	WaitForSingleObject(m_hDataAccess, INFINITE);

	if( _statusOK )
		++m_SuccessfulOps;
	else
		++m_ErrorOps;


	if ( m_SuccessfulOps > 0 )
	{
		csDisplay.Format(L"%d", m_SuccessfulOps);
		GetDlgItem(IDC_STATUS_OK_UPDATES)->SetWindowTextW(csDisplay);

		m_ulTotalTicks.QuadPart += _tickCount;
		dwAvgTickCount = (DWORD)(m_ulTotalTicks.QuadPart / m_SuccessfulOps);
		dwSecs = dwAvgTickCount/1000;
		dwHrs = dwSecs / SECONDS_PER_HR;
		dwMins = (dwSecs % SECONDS_PER_HR) / SECONDS_PER_MIN;
		dwSecs = dwSecs - ( (dwHrs * SECONDS_PER_HR) + (dwMins * SECONDS_PER_MIN) );
		csDisplay.Format(L"%d%s%02d%s%02d",
					dwHrs ,
					m_TimeSeparator,
					dwMins,
					m_TimeSeparator,
					dwSecs);

		GetDlgItem(IDC_STATUS_AVG_DURATION)->SetWindowTextW(csDisplay);
	}
	if ( m_ErrorOps > 0 )
	{
		csDisplay.Format(L"%d", m_ErrorOps);
		GetDlgItem(IDC_STATUS_FAILED_UPDATES)->SetWindowTextW(csDisplay);

		double dFailureRate = ((double)(m_ErrorOps * 100) / (double)(m_SuccessfulOps + m_ErrorOps) );
		csDisplay.Format(L"%#.2f", dFailureRate);
		csDisplay.AppendChar('%');
		GetDlgItem(IDC_STATUS_FAILURE_RATE)->SetWindowTextW(csDisplay);
	}
	ReleaseMutex(m_hDataAccess);
}


void CStDialogBar::SelectProfile(CString _csProfile)
{
	CComboBox *pProfileCombo = GetProfileCombo();

	if( !_csProfile.IsEmpty() )
	{
		pProfileCombo->SelectString(-1, _csProfile);

		m_p_config_mgr->SelectPlayerProfile((LPCTSTR) _csProfile);
	}

	SetProfileStatus();
}

void CStDialogBar::SetEnabledPorts(INT_PTR iNumEnabledPorts)
{
	m_p_config_mgr->SetNumEnabledPorts(iNumEnabledPorts);
}

void CStDialogBar::SetProfileStatus()
{
	CString csVersion, resStr;
	LPCTSTR szProfile;
	CComboBox *pProfileCombo = GetProfileCombo();

	szProfile = m_p_config_mgr->GetPlayerProfileName();
	if (szProfile)
	{
		pProfileCombo->SelectString(-1, m_p_config_mgr->GetPlayerProfileName());

		if( m_p_config_mgr->IsPlayerProfileValid() &&
			m_p_config_mgr->GetNumEnabledPorts()   &&
			m_p_config_mgr->GetNumEnabledOps())
		{
//			m_StatusText.SendMessage(WM_CTLCOLOR_REFLECT, SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
			resStr.LoadStringW(IDS_OK);
			GetDlgItem(IDC_STATUS_STATUS)->SetWindowTextW(resStr);

			csVersion = (m_p_config_mgr->GetPlayerProfile())->GetProductVersion();
			GetDlgItem(IDC_STATUS_FW_VER)->SetWindowTextW(csVersion);
			Enable();
		}
		else
		{
//			m_StatusText.SetTextColor(RGB(200,0,0));
			if( !m_p_config_mgr->IsPlayerProfileValid() )
			{
				resStr.LoadStringW(IDS_INVALID_PLAYER_PROFILE);
			}
			else if( !m_p_config_mgr->GetNumEnabledPorts() )
			{
				resStr.LoadStringW(IDS_STATUS_NO_PORTS_SELECTED);
			}
			else if( !m_p_config_mgr->GetNumEnabledOps() )
			{
				resStr.LoadStringW(IDS_STATUS_NO_OPS_SELECTED);
			}

			GetDlgItem(IDC_STATUS_STATUS)->SetWindowTextW(resStr);

			GetDlgItem(IDC_STATUS_FW_VER)->SetWindowTextW(L"");
			Disable();
		}
	}
}

CString CStDialogBar::WhatProfile()
{
	CComboBox *pProfileCombo = GetProfileCombo();
	int curSel = pProfileCombo->GetCurSel();
	CString csStr;

	csStr.Empty();

	if( curSel != CB_ERR )
		pProfileCombo->GetLBText(curSel, csStr);

	if (csStr.IsEmpty())
	{
		csStr = AfxGetApp()->GetProfileString(_T("Player Profile"), _T("Player Description"));
	}

	return csStr;
}

void CStDialogBar::ClearProfiles()
{
	CComboBox *pProfileCombo = (CComboBox *)GetDlgItem(IDC_STATUS_PROFILE_COMBO);
	CComboBox *pProfileMultiCombo = (CComboBox *)GetDlgItem(IDC_STATUS_MULTI_COMBO_PROFILE);

	while( pProfileCombo->GetCount() )
	{
		pProfileCombo->DeleteString(0);
		pProfileMultiCombo->DeleteString(0);
	}
}

void CStDialogBar::OnCbnSelchangeStatusProfileCombo()
{
	CString csProfile;
	CComboBox *pProfileCombo = GetProfileCombo();

	pProfileCombo->GetLBText(pProfileCombo->GetCurSel(), csProfile);

	SelectProfile( csProfile );
}

void CStDialogBar::SetProfiles()
{
	CComboBox *pProfileCombo = (CComboBox *)GetDlgItem(IDC_STATUS_PROFILE_COMBO);
	CComboBox *pProfileMultiCombo = (CComboBox *)GetDlgItem(IDC_STATUS_MULTI_COMBO_PROFILE);
	int index = 0;
	LPCTSTR szProfileName;

	while ( (szProfileName = m_p_config_mgr->GetListProfileName(index)) != NULL)
	{
		pProfileCombo->AddString(szProfileName);
		pProfileMultiCombo->AddString(szProfileName);
		++index;
	}
}

CComboBox * CStDialogBar::GetProfileCombo()
{
	CComboBox *pProfileCombo = (CComboBox *)GetDlgItem(IDC_STATUS_PROFILE_COMBO);
	CComboBox *pProfileMultiCombo = (CComboBox *)GetDlgItem(IDC_STATUS_MULTI_COMBO_PROFILE);
	CComboBox *pProfileMultiSettingsCombo = (CComboBox *)GetDlgItem(IDC_STATUS_MULTI_COMBO_SETTINGS);

	if (!m_SettingsEnabled)
	{
		pProfileCombo->ShowWindow(SW_SHOWNORMAL);
		pProfileMultiCombo->ShowWindow(SW_HIDE);
		pProfileMultiSettingsCombo->ShowWindow(SW_HIDE);
		return pProfileCombo;
	}
	else
	{
		pProfileCombo->ShowWindow(SW_HIDE);
		pProfileMultiCombo->ShowWindow(SW_SHOWNORMAL);
		pProfileMultiSettingsCombo->ShowWindow(SW_SHOWNORMAL);
		return pProfileMultiCombo;
	}
}



