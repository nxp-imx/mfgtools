/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// DeviceApiDlg.h : header file
//

#pragma once

#include "Libs/DevSupport/DeviceManager.h"
#include "Libs/DevSupport/Volume.h"
#include "Libs/DevSupport/StFormatInfo.h"

#include "Libs\Loki\Functor.h"

// StFormatDlg dialog

class StFormatDlg : public CDialog
{
// Construction
public:
	StFormatDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~StFormatDlg();

// Dialog Data
	enum { IDD = IDD_ST_FORMAT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	Volume* _pVolume;
    uint32_t _sectors;
    uint32_t _sectorSize;
    uint8_t _dataDriveNumber;

    StFormatInfo* _pDiskParams;
	void FillDeviceCombo();
	void UpdateStatus();
    int32_t GetNandParams(uint32_t* pNumSectors, uint32_t* pSectorSize, uint8_t* pDriveNumber);
    CString FormatSize( ULONG_PTR size, INT_PTR mode=0 );
public:
	CButton _reject_autoplay_ctrl;
	afx_msg void OnBnClickedRejectAutoplayCheck();
	CComboBox _selectDeviceCtrl;
	afx_msg void OnCbnSelchangeSelectDeviceCombo();
	CStatic _statusCtrl;
	CStatic _nandSectorsCtrl;
	CStatic _nandSectorSizeCtrl;
	CComboBox _fileSystemCtrl;
	CComboBox _clusterSizeCtrl;
	CComboBox _rootEntriesCtrl;
	CComboBox _numFatsCtrl;
    CEdit _volumeLabelCtrl;
    CEdit _imageInfoCtrl;
	CButton _formatCtrl;
    CProgressCtrl _progressCtrl;
	bool OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo);
protected:
	virtual void OnCancel();
public:
    void FillFileSystemCombo();
    void FillClusterSizeCombo(const uint32_t sectorSize);
    void FillRootEntriesCombo();
    void FillNumFatsCombo();
    afx_msg void OnCbnSelchangeFileSystemCombo();
    afx_msg void OnCbnSelchangeClusterSizeCombo();
    afx_msg void OnCbnSelchangeRootEntriesCombo();
    afx_msg void OnEnChangeVolumeLabelEdit();
    afx_msg void OnCbnSelchangeNumFatsCombo();
    afx_msg void OnBnClickedFormatButton();
	afx_msg void OnBnClickedDumpButton();
};
