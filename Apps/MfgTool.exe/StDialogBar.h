/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxext.h"
#include "StColorBtn.h"
#include "../../libs/winsupport/ColorStaticST.h"
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

	CStColorBtn		m_StartStopBtn;
	ULONG			m_SuccessfulOps;
	ULONG			m_ErrorOps;

	DECLARE_MESSAGE_MAP()

private:
	HANDLE			m_hDataAccess;
	ULARGE_INTEGER	m_ulTotalTicks;

	CFont			m_StartStopFont;
	CFont			m_StoppingFont;

	CColorStaticST	m_StatusText;

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
};
