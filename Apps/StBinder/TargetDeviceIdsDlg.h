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
#include "StCfgResOptions.h"


// CTargetDeviceIdsDlg dialog

class CTargetDeviceIdsDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CTargetDeviceIdsDlg)

public:
	CTargetDeviceIdsDlg();   // standard constructor
	virtual ~CTargetDeviceIdsDlg();

// Dialog Data
	enum { IDD = IDD_TARGET_IDS };

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CVisValidEdit m_SCSIMfgId;
	CVisValidEdit m_SCSIProdId;
	CVisValidEdit m_MTPMfgId;
	CVisValidEdit m_MTPProdId;
	CVisValidEdit m_USBVendorId;
	CVisValidEdit m_USBProdId;
	CVisValidEdit m_SecondaryUSBProdId;
};
