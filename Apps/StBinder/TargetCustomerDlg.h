/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "afxwin.h"
#include "../../Libs/WinSupport/visvaledit.h"
#include "TargetCfgData.h"

// CTargetCustomerDlg dialog

class CTargetCustomerDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CTargetCustomerDlg)

public:
	CTargetCustomerDlg();   // standard constructor
	virtual ~CTargetCustomerDlg();

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

// Dialog Data
	enum { IDD = IDD_TARGET_CUSTOMER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	HBITMAP m_bitmap;
	HICON	m_companyicon;
	HICON	m_updatericon;
public:
	CVisValidEdit m_TargetCustomerName;
	CVisValidEdit m_TargetProdDesc;
	CVisValidEdit m_TargetProdName;
	CVisValidEdit m_UpdMajorVersion;
	CVisValidEdit m_UpdMinorVersion;
	CVisValidEdit m_ProdMajorVersion;
	CVisValidEdit m_ProdMinorVersion;
	CVisValidEdit m_TargetAppTitle;
	CVisValidEdit m_TargetCopyright;
	CVisValidEdit m_TargetCompanyImagePathname;
	CVisValidEdit m_TargetUpdaterIconPathname;
	CVisValidEdit m_TargetComment;
	CString m_CompanyImagePathname;
	CString m_UpdaterIconPathname;
	CStatic m_CompanyIcon;
	CStatic m_CompanyBitmap;
	CStatic m_UpdaterIcon;
	afx_msg void OnBnClickedTargetCustBrowseCompanyImage();
	afx_msg void OnBnClickedTargetCustBrowseUpdaterIcon();
};
