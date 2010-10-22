/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxwin.h"
#include "OverwriteEdit.h"
#include "Libs/WinSupport/HyperLink.h"
#include "Libs/WinSupport/MRUCombo.h"
#include "Libs/WinSupport/MRUCombo.h"

#include "Libs/DevSupport/StFwComponent.h"
//#include "Libs/DevSupport/StFwComponent.h"

// CAttachDataDlg dialog

class CAttachDataDlg : public CDialog
{
	DECLARE_DYNAMIC(CAttachDataDlg)

public:
	CAttachDataDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAttachDataDlg();
    const std::vector<uint8_t>& GetData() const { return _returnData; };

// Dialog Data
	enum { IDD = IDD_ATTACH_DATA_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
    void EnableFirmwareGroup(const bool bEnable);

	DECLARE_MESSAGE_MAP()
public:
    CButton _fwSourceFileCtrl;
    CMRUComboBox _fwFileComboCtrl;
    CButton _fwFileBrowseCtrl;
    CHyperLink _fwFileEmptyLinkCtrl;

    CButton _fwSourceResourceCtrl;
    CComboBox _fwResourceComboCtrl;

    CEdit _fwPathCtrl;
    CStatic _fwTypeIdLabel;
    CStatic _fwTypeIdCtrl;
    CStatic _fwCompVerLabel;
    CStatic _fwComponentVersionCtrl;
    CStatic _fwProdVerLabel;
    CStatic _fwProductVersionCtrl;
    CStatic _fwLangLabel;
    CStatic _fwLanguageCtrl;
    CStatic _fwTagLabel;
    CStatic _fwTagCtrl;

    COverwriteEdit _hexInputCtrl;

    CEdit _versionInputCtrl;

    CStatic _sizeCtrl;

public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedSelectSourceRadio();
    afx_msg void OnBnClickedFirmwareSourceRadio();
    afx_msg void OnCbnSelchangeFirmwareFileCombo();
    afx_msg void OnBnClickedFirmwareFileBrowseButton();
    afx_msg void OnStnClickedFirmwareFileEmptyLink();
    afx_msg void OnDropFiles(HDROP hDropInfo);
    afx_msg void OnBnClickedCancel();
    afx_msg void OnEnChangeVersionInputEdit();

private:
    StFwComponent _fw;
    StVersionInfo _versionInfo;
    std::vector<uint8_t> _hexData;
    std::vector<uint8_t> _returnData;
};
