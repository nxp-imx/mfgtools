/*
 * Copyright (C) 2009-2012, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "UsbHub.h"
#include "UsbController.h"
#include "DeviceClass.h"
#include "DeviceManager.h"
#include "UsbPort.h"
#include "MfgToolLib.h"

//extern BOOL g_bConfigUsbPortDlgOpen;
//extern HWND g_hConfigUsbPortDlg;

//#define UM_DEVICE_CHANGE_CFG_DLG_NOTIFY		(WM_USER+107)

// CConfigUSBPortDlg dialog

class CConfigUSBPortDlg : public CDialog
{
	DECLARE_DYNAMIC(CConfigUSBPortDlg)

public:
	CConfigUSBPortDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConfigUSBPortDlg();

// Dialog Data
	enum { IDD = IDD_CONF_USB_PORT_DLG };

	class CUSBTreeCtrl : public CTreeCtrl
    {
        public:
            CUSBTreeCtrl();
            virtual ~CUSBTreeCtrl();
            virtual void PreSubclassWindow();
            DECLARE_MESSAGE_MAP()
            afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
            afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
            afx_msg void OnTvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
            CToolTipCtrl *m_tool_tip_ctrl;
            CImageList m_il_state;
    }m_tree_ctrl;

	typedef enum CbState_t
	{ 
		CbStateNone = 0, 
		CbStateUnchecked, 
		CbStateChecked, 
		CbStateGrayed 
	};

	struct PortNodeInfo
	{
        PortNodeInfo(usb::Hub *const pHub, const BYTE portIndex, const CbState_t state = CbStateNone, const BYTE dlgIndex= -1)
            : pUsbHub(pHub)
            , PortIndex(portIndex)
            , CbState(state)
            , DlgIndex(dlgIndex)
        {};
        usb::Hub *const pUsbHub;
        const BYTE   PortIndex;
        CbState_t  CbState;
        int  DlgIndex; // -1 means unassigned
    };

	typedef enum _tag_e_TREE_OP
	{ 
		OP_NONE = 0, 
		OP_DELETE_ALL, 
		OP_GRAY_UNCHECKED, 
		OP_UNCHECK_GRAYED, 
		OP_REFRESH_PORTS, 
		OP_INIT_TREE, 
		OP_UNCHECK_ALL
	} e_TREE_OP;

	UINT m_num_enabled_ports;
    UINT m_MaxEnabledPorts;
	UINT_PTR m_stale;
	BOOL m_bFoundSecondaryController;
	usb::Port*  m_pPortMappings[MAX_CMDOP_NUMS];
	BOOL m_bPortsChanged;

	//void DoTreeOperation(e_TREE_OP _op_type, const DeviceClass::NotifyStruct* pNsinfo = NULL);
	void DoTreeOperation(e_TREE_OP _op_type, const MxNotifyStruct* pNsinfo = NULL);
	HTREEITEM GetNextTreeItem( HTREEITEM _hti = 0 );
	PortNodeInfo* SetTreeItemInfo(const HTREEITEM _hti, const usb::Controller *const _pUsbController = NULL);
    PortNodeInfo* SetTreeItemInfo(const HTREEITEM _hti, usb::Hub *const _pUsbHub, const BYTE _index);
	CbState_t GetPortState(PortNodeInfo* _info);
	BOOL SetPortItemText(HTREEITEM _hti, PortNodeInfo* _p_info);
	void InsertHubPorts(usb::Hub *const pHub, const HTREEITEM htiHub, BOOL _bInsertPorts);

	INT_PTR AddRef() 
	{ 
		if (m_num_enabled_ports == m_MaxEnabledPorts) 
		{
			return 0;
		}
		else
		{
			return ++m_num_enabled_ports;
		}
	};

	INT_PTR Release() 
	{ 
		ASSERT(m_num_enabled_ports);  
		return --m_num_enabled_ports; 
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CbState_t EnablePort(HTREEITEM _hti);
    afx_msg void OnNMClickUsbTree(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnKeydownUsbTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnDeviceChangeNotify(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
protected:
	virtual void OnOK();
};
