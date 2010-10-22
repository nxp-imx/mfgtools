/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "TargetCustomerDlg.h"
#include "TargetDeviceIdsDlg.h"
#include "TargetOptionsDlg.h"
#include "TargetDriveArrayDlg.h"
#include "TargetMiscDlg.h"

// CTargetConfigSheet

class CTargetConfigSheet : public CPropertySheet
{
public:
	DECLARE_DYNAMIC(CTargetConfigSheet);

	CTargetConfigSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CTargetConfigSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CTargetConfigSheet();


protected:
	CTargetCustomerDlg		m_customer_page;
	CTargetDeviceIdsDlg		m_deviceids_page;
	CTargetOptionsDlg		m_options_page;
	CTargetDriveArrayDlg	m_drivearray_page;
	CTargetMiscDlg			m_misc_page;

	void AddControlPages(void);

	DECLARE_MESSAGE_MAP()

public:
    virtual BOOL OnInitDialog();
};


