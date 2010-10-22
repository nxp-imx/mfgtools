/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// BitBurnerDlg.h : header file
//
#pragma once

#include "Libs/DevSupport/DeviceManager.h"
#include "Libs/DevSupport/HidDevice.h"
#include "Libs/OtpAccessPitc/StOtpAccessPitc.h"
#include "StOtpRegs3700.h"

#include "Libs/WinSupport/HotPropCtrl.h"
#include "Libs/WinSupport/DynamicLED.h"
#include "afxwin.h"
#include "afxcmn.h"

// CBitBurnerDlg dialog
class CBitBurnerDlg : public CDialog
{
// Construction
public:
	CBitBurnerDlg(CWnd* pParent = NULL);	// standard constructor
	~CBitBurnerDlg();

// Dialog Data
	HICON _hIcon;
	enum _IDD { IDD = IDD_BITBURNER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	void OnToolTipNotify(NMHDR * pNMHDR, LRESULT * pResult );
	void SetWindowTitle( LPCTSTR deviceID );
	DECLARE_MESSAGE_MAP()

// Implementation
protected:
    // device
    HidDevice* _pDevice;
	CComboBox _selectDeviceCtrl;
	void FillDeviceCombo();
	afx_msg void OnCbnSelchangeSelectDeviceCombo();
	bool OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo);
    CEdit _cmdStatusCtrl;
    CDynamicLED _cmdLedCtrl;
    void OnTalkyLED(const Device::NotifyStruct& nsInfo);

    // pitc
    StOtpAccessPitc* _pOtpPitc;
    CEdit _pitcInfoCtrl;
   	CDynamicLED	_pitcLedCtrl;
    CButton _pitcDownloadCtrl;
    CProgressCtrl _pitcDownloadProgressCtrl;
    afx_msg void OnBnClickedLoadPitcButton();
    void OnDownloadPitcProgress(const Device::NotifyStruct& nsInfo);

    // registers
    StOtpRegs* _pOtpRegs;
	StOtpRegs::OtpRegister* _pCurrentReg;
	CListCtrl _otpRegisterListCtrl;
    void FillRegisterList();
	void LockRegisterFields(const StOtpRegs::OtpRegister *const pRegister) const;
	afx_msg void OnLvnItemchangedRegisterList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnHotPropUpdate(WPARAM nID, LPARAM nValue);

	CHotPropCtrl* _pHotProp;
	CEdit _otpRegInfoCtrl;
	CEdit _CurrentRegisterValueCtrl;
	CEdit _NewRegisterValueCtrl;
	CButton _lockCtrl;
	CButton _writeCtrl;
	CButton _writeAndLockCtrl;
public:
	afx_msg void OnBnClickedLockButton();
	afx_msg void OnBnClickedWriteButton();
	afx_msg void OnBnClickedWriteNLockButton();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
