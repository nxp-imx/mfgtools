/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "ConfigPlayerProfilePage.h"
#include "ConfigUSBPortPage.h"
#include "ConfigGeneralPage.h"

// CConfigMgrSheet

class CConfigMgrSheet : public CPropertySheet
{
public:
	DECLARE_DYNAMIC(CConfigMgrSheet);

	CConfigMgrSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CConfigMgrSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CConfigMgrSheet();

protected:
	CConfigPlayerProfilePage m_player_profile_page;
	CConfigUSBPortPage m_usb_port_page;
	CConfigGeneralPage m_general_page;
	void AddControlPages(void);
	DECLARE_MESSAGE_MAP()
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	LPCTSTR GetPlayerProfileName(void) { return m_player_profile_page.GetProfileName(); };
	BOOL IsPlayerProfileValid(void) { return m_player_profile_page.IsProfileValid(); };
	INT_PTR GetNumEnabledOps(void) { return m_player_profile_page.GetNumEnabledOps(); };
	CPlayerProfile* GetPlayerProfile(void) { return m_player_profile_page.GetProfile(); };
	UINT GetMaxPorts(void) { return m_usb_port_page.GetMaxPorts(); };
	INT_PTR GetNumEnabledPorts(void) { return m_usb_port_page.GetNumEnabledPorts(); };
	void SetNumEnabledPorts(INT_PTR iNumEnabledPorts) { m_usb_port_page.SetNumEnabledPorts(iNumEnabledPorts); };
	INT_PTR AddRefPort(void) { return m_usb_port_page.AddRef(); };
	INT_PTR ReleasePort(void) { return m_usb_port_page.Release(); };
	void SetUSBPageStale(UINT_PTR _missed_msg) { m_usb_port_page.SetStale(_missed_msg); };
	CString GetAutoPlayDrvList(void) { return m_general_page.GetAutoPlayDrvList(); };
	BOOL GetAutoPlayEnabled(void) { return m_general_page.GetAutoPlayEnabled(); };
	BOOL OpsRestricted(void) { return m_general_page.GetOpsRestricted(); };
	DWORD InitConfigMgr(void);
	void SelectPlayerProfile( LPCTSTR _szProfile); // called from DialogBar
	LPCTSTR GetListProfileName(int index) {return m_player_profile_page.GetListProfileName(index); };
	CPlayerProfile* GetListProfile(int index) {return m_player_profile_page.GetProfile(index); };
    virtual BOOL OnInitDialog();
};


