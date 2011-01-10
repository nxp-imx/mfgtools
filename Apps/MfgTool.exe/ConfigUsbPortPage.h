/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "../../Libs/DevSupport/DeviceManager.h"
#include "../../Libs/DevSupport/UsbController.h"

#define DEFAULT_PORTS	4
#define MAX_PORTS		16

// CConfigUSBPortPage dialog
class CConfigUSBPortPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigUSBPortPage)

public:
	CConfigUSBPortPage();
	virtual ~CConfigUSBPortPage();
	virtual void OnOK();
	virtual void OnCancel();
	UINT GetMaxPorts(void) { return m_MaxEnabledPorts; };
	void SetStale(UINT_PTR _missed_msg);
	INT_PTR GetNumEnabledPorts(void) { return m_num_enabled_ports; };
	void SetNumEnabledPorts(INT_PTR iNumEnabledPorts) {m_num_enabled_ports = iNumEnabledPorts;};
	INT_PTR AddRef(void) { if (m_num_enabled_ports == m_MaxEnabledPorts) return 0; else return ++m_num_enabled_ports; };
	INT_PTR Release(void) { ASSERT(m_num_enabled_ports);  return --m_num_enabled_ports; };
// Dialog Data
	enum _tag_e_DLG_ID{ IDD = IDD_CONF_USB_PORT_DLG };
	
protected:
	typedef enum _tag_e_ERROR{ PORTINFO_ERR_OK = 0, PORTINFO_ERR_NO_PORT_LIST } e_ERROR;
	typedef enum _tag_e_TREE_OP{ OP_NONE = 0, OP_DELETE_ALL, OP_GRAY_UNCHECKED, OP_UNCHECK_GRAYED, 
								 OP_REFRESH_PORTS, OP_INIT_TREE, OP_UNCHECK_ALL } e_TREE_OP;
	typedef enum CbState_t{ CbStateNone = 0, CbStateUnchecked, CbStateChecked, CbStateGrayed };
	struct PortNodeInfo{
		PortNodeInfo(usb::Hub *const pHub, const uint8_t portIndex, const CbState_t state = CbStateNone, const uint8_t dlgIndex= -1)
			: pUsbHub(pHub)
			, PortIndex(portIndex)
		    , CbState(state)
		    , DlgIndex(dlgIndex)
		{};
		usb::Hub *const pUsbHub;
		const uint8_t   PortIndex;
		CbState_t       CbState;
		int8_t		    DlgIndex; // -1 means unassigned
	};

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnNMClickUsbTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnKeydownUsbTree(NMHDR *pNMHDR, LRESULT *pResult);
	bool OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo);
	virtual BOOL OnSetActive();
	DECLARE_MESSAGE_MAP()

	UINT m_num_enabled_ports;
	UINT	m_MaxEnabledPorts;
	usb::Port*	m_pPortMappings[MAX_PORTS];
	UINT_PTR m_stale;
	HANDLE m_hDeviceManagerCallback;
	BOOL m_bPortsChanged;
//	CColorStaticST  m_hub_required_ctrl;
	CFont			m_hub_required_font;
	BOOL m_bFoundSecondaryController;

	CbState_t EnablePort(HTREEITEM _hti);
	PortNodeInfo* SetTreeItemInfo(const HTREEITEM _hti, const usb::Controller *const _pUsbController = NULL);
	PortNodeInfo* SetTreeItemInfo(const HTREEITEM _hti, usb::Hub *const _pUsbHub, const uint8_t _index);
	CbState_t GetPortState(PortNodeInfo* _info);

	BOOL SetPortItemText(HTREEITEM _hti, PortNodeInfo* _p_info);
	HTREEITEM GetNextTreeItem( HTREEITEM _hti = 0 );
	void DoTreeOperation(e_TREE_OP _op_type);
	void InsertHubPorts(usb::Hub *const pHub, const HTREEITEM htiHub, BOOL _bInsertPorts);

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
public:
	afx_msg void OnCbnSelchangeUsbportsEnabledPorts();
};
