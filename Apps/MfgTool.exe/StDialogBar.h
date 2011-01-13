/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxext.h"
#include "Libs/Public/StColorBtn.h"
#include "ConfigMgrSheet.h"

class CStDialogBar :
	public CDialogBar
{
public:
	CStDialogBar(void);
	virtual ~CStDialogBar(void);

	void SetDisplay(CConfigMgrSheet *_p_config_mgr);
	void Start(LPCTSTR _csProfile);
	void Stop();
	void Idle();
	void Disable();
	void Enable();
	void UpdateData(BOOL status, DWORD _tickCount);
	void OnTimer(UINT nEventID);
	void SelectProfile(CString _csProfile);
	void SetProfileStatus();
	CString WhatProfile(void);
	void ClearProfiles();
	void SetProfiles(void);
	void SetEnabledPorts(INT_PTR iNumEnabledPorts);

	CStColorBtn		m_StartStopBtn;
	ULONG			m_SuccessfulOps;
	ULONG			m_ErrorOps;

	DECLARE_MESSAGE_MAP()

private:
	HANDLE			m_hDataAccess;
	ULARGE_INTEGER	m_ulTotalTicks;

	CFont			m_StartStopFont;
	CFont			m_StoppingFont;

	CStatic			m_StatusText;

	CTime			m_StartCTime;
	BOOL			m_TimerActive;
	BOOL			m_bCheckForStop;

	WCHAR			m_TimeSeparator[5];

	CComboBox		*m_pProfileCombo;
	CConfigMgrSheet *m_p_config_mgr;

	BOOL			m_SettingsEnabled;

	CComboBox *		GetProfileCombo(void);

public:
	afx_msg void OnCbnSelchangeStatusProfileCombo();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
