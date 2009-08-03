// StFwLoaderDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "Libs/WinSupport/MRUCombo.h"
#include "Libs/WinSupport/HyperLink.h"

#include "Libs/DevSupport/Observer.h"
#include "Libs/DevSupport/StFwComponent.h"
#include "Libs/DevSupport/RecoveryDevice.h"
#include "Libs/DevSupport/DeviceManager.h"

// CStFwLoaderDlg dialog
class CStFwLoaderDlg : public CDialog, public Observer
{
// Construction
public:
	CStFwLoaderDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_STFWLOADER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
//	virtual void SetTotalTasks(uint32_t /*_total_tasks*/){};
//	virtual void UpdateGrandProgress(){};
	virtual void SetCurrentTask(uint32_t taskId, uint32_t taskRange);
	virtual void UpdateProgress(const Device::NotifyStruct& nsInfo);
	void OnLoadStateChange();

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	// download
	CStatic m_connection_ctrl;
	CButton m_load_ctrl;
	CEdit m_status_ctrl;
	CProgressCtrl m_progress_ctrl;
	int m_auto_load;
	int m_load;

	// file
	CMRUComboBox m_file_combo_ctrl;
	CStatic m_file_size_ctrl;
	CStatic m_date_ctrl;
	CStatic m_file_name_ctrl;
	CStatic m_product_version_ctrl;
	CStatic m_component_version_ctrl;
	CStatic m_tag_ctrl;
	CHyperLink	m_empty_list_ctrl;
	
    // options
	CButton m_auto_download_ctrl;
	CButton m_always_on_top_ctrl;
	CButton m_reject_autoplay_ctrl;

	StFwComponent m_fw;
   	RecoveryDevice* m_p_dev;

public:
	bool OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo);

	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnCbnSelchangeFileCombo();
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedAutoDownloadCheck();
	afx_msg void OnBnClickedOnTopCheck();
	afx_msg void OnBnClickedRejectAutoplayCheck();
	afx_msg void OnStnClickedEmptyListLink();
	afx_msg void OnBnClickedLoadButton();
	DECLARE_MESSAGE_MAP()
protected:
    virtual void OnCancel();
};
