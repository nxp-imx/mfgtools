// DeviceApiDlg.h : header file
//

#pragma once
//#include "afxwin.h"

#include "OverwriteEdit.h"
#include "ApiListCtrl.h"

#include "Libs/DevSupport/DeviceManager.h"
//#include "Libs/DevSupport/Volume.h"
//#include "Libs/DevSupport/HidDevice.h"
//#include "Libs/DevSupport/StApi.h"
//#include "Libs/DevSupport/StHidApi.h"

#include "Libs/WinSupport/DynamicLED.h"

//#include "Libs/Loki/Functor.h"
//#include "afxcmn.h"


// CDeviceApiDlg dialog
class CDeviceApiDlg : public CDialog
{
friend class COverwriteEdit;
friend class CApiListCtrl;
// Construction
public:
	CDeviceApiDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_DEVICEAPI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


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
	Device* _pDevice;
	StApi::StApiList _apiList;
	StApi* _currentApi;
	void FillDeviceCombo();
	void FillApiList();
	void UpdateStatus();
	void UpdateCommandParams();
	void FormatCDB();
	void ParseCDB();
	bool IsCustomCommand();
    media::MediaAllocationTable _driveArray;

public:
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedRefreshButton();
	afx_msg void OnBnClickedRejectAutoplayCheck();
	CComboBox _selectDeviceCtrl;
	afx_msg void OnCbnSelchangeSelectDeviceCombo();
	COverwriteEdit _commandCtrl;
	CImageList _imageList;
	CEdit _responseCtrl;
	afx_msg void OnBnClickedSendButton();
	afx_msg void OnLvnItemchangedApiList(NMHDR *pNMHDR, LRESULT *pResult);
	CApiListCtrl _selectApiCtrl;
	CStatic _cdbSizeCtrl;
	CEdit _xferSizeCtrl;
	CStatic _statusCtrl;
	CButton _sendCtrl;
	CButton _readCtrl;
	CButton _writeCtrl;
    CDynamicLED _cmdLedCtrl;
    CProgressCtrl _cmdProgressCtrl;
	CButton _reject_autoplay_ctrl;
	afx_msg void OnBnClickedWriteRadio();
	afx_msg void OnBnClickedReadRadio();
	afx_msg void OnEnChangeXferSizeEdit();
	bool OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo);
	LRESULT OnEnChangeCommandEdit(WPARAM, LPARAM);
    void OnTalkyLED(const Device::NotifyStruct& nsInfo);
protected:
	virtual void OnCancel();
};
