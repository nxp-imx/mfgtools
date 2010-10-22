/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxwin.h"


// COtherFirmwareLANGID dialog

class COtherFirmwareLANGID : public CDialog
{
	DECLARE_DYNAMIC(COtherFirmwareLANGID)

public:
	COtherFirmwareLANGID(LANGID * _pLangID, CWnd* pParent = NULL);   // standard constructor
	virtual ~COtherFirmwareLANGID();

	LANGID	*m_pLangID;

// Dialog Data
	enum { IDD = IDD_OTHER_LANGID };

protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CEdit PrimaryCtl;
	CEdit SublangCtl;
	CListBox m_LocaleList;
	afx_msg void OnLbnDblclkLocaleList();
	CComboBox m_LocaleListC;
	afx_msg void OnLbnSelchangeLocaleList();
};
