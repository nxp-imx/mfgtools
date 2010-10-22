/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once


#include "PlayerProfile.h"
#include "stmsg.h"
//#define WM_MSG_PD_EVENT	WM_USER+36

class CPortMgrDlg : public CDialogBar
{
	DECLARE_DYNAMIC(CPortMgrDlg)

public:
	CPortMgrDlg(CWnd* pParent = NULL, int _index = 0);   // standard constructor
	virtual ~CPortMgrDlg();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	void SetUsbPort(usb::Port* _port);
	void UpdateUI(LPCTSTR _status, int _opDelta = 0, int _taskRange = -1, int _taskPosition = 0);
	usb::Port* GetUsbPort(void) { return m_p_usb_port; };
	INT_PTR GetHubIndex(void) { return m_usb_hub_index; };
	LPCTSTR GetHubName(void) { return (LPCTSTR)m_usb_hub_name; };
	CString GetPanel(void) { return m_label; };
    int GetPanelIndex(void) { return m_port_display_index; }
	DWORD_PTR GetOpMode(void) { return m_mode; };
	void KillMonitor(void);
	void CloseDlg();
	typedef enum PortDlgEvents { PD_EVNT_START, PD_EVNT_STOP,
								 PD_EVNT_DEVICE_ARRIVAL, PD_EVNT_DEVICE_REMOVAL, 
								 PD_EVNT_VOLUME_ARRIVAL, PD_EVNT_VOLUME_REMOVAL, 
								 PD_EVNT_HUB_ARRIVAL, PD_EVNT_HUB_REMOVAL,
								 PD_EVNT_CONFIG_CHANGE, PD_EVNT_OP_COMPLETE,
								 PD_EVNT_START_NOTIFY}; 

	static const PTCHAR PortEventStrings[];
	typedef enum OP_MGR_MODE{ OPMODE_INVALID = 0, OPMODE_MONITOR, OPMODE_RUNNING, OPMODE_ALL_OPS_COMPLETE };
	static CString OpModeToString(OP_MGR_MODE mode)
	{
		CString str;
		switch(mode)
		{
			case OPMODE_MONITOR:
				str = _T("MONITOR");
				break;
			case OPMODE_RUNNING:
				str = _T("RUNNING");
				break;
			case OPMODE_ALL_OPS_COMPLETE:
				str = _T("ALL_OPS_COMPLETE");
				break;
			case OPMODE_INVALID:
			default:
				str = _T("INVALID");
				break;
		}
		return str;
	}

	enum _tag_e_DLG_ID{ IDD = IDD_PORT_DLG };
	void OpCompleteTick(DWORD _dwElapsedTime);
/*
	class CDevList
	{
		private:
			CStringList m_list;
			CPortMgrDlg * m_parent;
//			POSITION Find( LPCTSTR searchValue, POSITION startAfter = NULL );
		public:
			CDevList(void) : m_mutex(FALSE, NULL) {};
			CMutex m_mutex;
			void Init(CPortMgrDlg* _parent) {m_parent = _parent;};
			POSITION AddTail( LPCTSTR newElement );
			CString GetTail(void);
			CString RemoveTail(void);
			INT_PTR GetCount(void);
	}m_dev_list;
	CDevList* GetDriverKeys(void);
*/
protected:
	typedef CTypedPtrList<CObList, COperation*>  COpList;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
	afx_msg LRESULT OnInitDialog(UINT wParam, LONG lParam);
	afx_msg void OnTimer( UINT_PTR nIDEvent );
	afx_msg LRESULT OnMsgStateChange(WPARAM _event_type, LPARAM _msg_string);
	BOOL OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo); // may replace OnMsgStateChange??
	HANDLE m_hDevChangeCallback;
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	CSize CalcDynamicLayout(int nLength, DWORD dwMode);
	CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	afx_msg void OnNcPaint();
	DECLARE_MESSAGE_MAP()

	usb::Port* FindDlgPort(void);

//	CStatic m_op_count_text_ctrl;
	CStatic m_port_group_ctrl;
	CEdit m_port_drive_ctrl;
	CStatic m_port_drive_text_ctrl;
	CStatic m_port_operation_text_ctrl;
	CEdit m_port_status_ctrl;
	CProgressCtrl m_port_progress_ctrl;
	CProgressCtrl m_port_task_progress_ctrl;
	CBrush m_br_background;

	
	CString m_label;			// group control label, registry subdirectory
	int m_port_display_index;	// used in the label above
	bool m_start;					// keeps track of the start/stop button in the MainFrame
	OP_MGR_MODE m_mode;
	usb::Port* m_p_usb_port;		// pointer to our I/O class
	CString m_usb_hub_name;			// registry setting for persistent port assignment
	uint32_t m_usb_hub_index;		// registry setting for persistent port assignment

	HANDLE m_hComPort;
	BOOL m_useComPortMsgs;

//clw	CString m_dev_path;				// used for comparison on DEVCHANGE_DEV_REMOVAL

private:
	INT_PTR CreateMonitor(void);
	INT_PTR CreateOps(void);
	void DeleteOps(void);
	OP_MGR_MODE ProcessOps(void);
	void Localize();
	BOOL FitsProfile(CString dev_path);
	CPlayerProfile* m_p_profile;
	CPlayerProfile::COpInfoList* m_p_ops;
	COpList m_op_list;
	COperation * m_p_monitor_op;
	COperation * m_p_curr_op;
    POSITION m_curr_op_pos;
	INT_PTR m_duration;
	bool m_op_error;

	ULONG   m_op_count;
    ULONG   m_op_error_count;
	DWORD	m_TaskElapsedTime;
	BOOL	m_OpsRunning;
public:
    afx_msg void OnDestroy();
};
