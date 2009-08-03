// DeviceEnumDlg.h : header file
//

#pragma once
#include "UsbFilterListCtrl.h"
#include "DeviceTreeCtrl.h"

#include "Libs/DevSupport/DeviceManager.h"
#include "Libs/WinSupport/ColumnTreeWnd.h"


// CDeviceEnumDlg dialog
class CDeviceEnumDlg : public CDialog
{
// Construction
public:
	CDeviceEnumDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum eIDD{ IDD = IDD_DEVICEENUM_DIALOG };
	typedef enum eDEV_CLASS{RECOVERY=0, MASS_STORAGE, MTP, HID, USB_CONTROLLER};
	static const PTCHAR DeviceTypeDesc[];

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
    virtual void OnCancel();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_os_version_ctrl;
	CListCtrl m_dev_type_ctrl;
	CUsbFilterListCtrl m_usb_ids_filter_ctrl;
	CDeviceTreeCtrl m_device_tree_ctrl;
	CButton m_reject_autoplay_ctrl;
private:
	DWORD m_device_type_bits;
	SP_CLASSIMAGELIST_DATA m_classImageList;
	CImageList m_ImageList;
	CColumnTreeWnd m_TreeWnd;

	void InitFilterListCtrl();
	void InitDeviceTree();
	HTREEITEM InsertDevices();
	void InitPropertyTree();
	HTREEITEM InsertPropertyItems(Property::PropertyList* pList, HTREEITEM parent);
public:
	bool OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo);
	afx_msg void OnBnClickedRefreshButton();
	afx_msg void OnTvnSelchangedDeviceTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedDevicetypeList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedUsbFilterList(NMHDR *pNMHDR, LRESULT *pResult);
	void UpdateUsbFilters(void);
	afx_msg void OnBnClickedRejectAutoplayCheck();
};
