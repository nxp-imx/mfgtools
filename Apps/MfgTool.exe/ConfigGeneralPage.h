/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxwin.h"

// CConfigGeneralPage dialog

class CConfigGeneralPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigGeneralPage)

public:
	CConfigGeneralPage();
	virtual ~CConfigGeneralPage();
	virtual void OnOK();
	virtual void OnCancel();

// Dialog Data
	enum { IDD = IDD_CONF_GENERAL_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedRestrictOps();
	afx_msg void OnBnClickedSupressAutoplay();
	afx_msg void OnBnClickedAutoplayAllDrvsRadio();
	afx_msg void OnBnClickedAutoplaySelDrvsRadio();
	afx_msg void OnLvnItemchangedIdcAutoplayDrvList(NMHDR *pNMHDR, LRESULT *pResult);

	void InitAutoPlayListCtrl(void);

	BOOL m_ops_restricted;
    BOOL m_event_log_enable;
	BOOL m_use_comport_msgs;
//	CComboBox m_language_ctrl;
//	CString m_language;

	BOOL m_supress_autoplay;
	CButton m_all_drvs_ctrl;
	CButton m_sel_drvs_ctrl;
	CString m_autoplay_sel_drv_list;
	CListCtrl m_autoplay_drv_list_ctrl;
private:
	void Localize();
public:
	CString GetAutoPlayDrvList(void);
	BOOL GetAutoPlayEnabled(void);
	BOOL GetOpsRestricted(void) { return m_ops_restricted; };
    afx_msg void OnBnClickedEventLogEnableCheck();
    afx_msg void OnBnClickedEventUseComPortMsgs();

};
