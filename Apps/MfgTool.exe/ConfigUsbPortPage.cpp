/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// ConfigUSBPortPage.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigUSBPortPage.h"
#include "PortMgrDlg.h"
#include "StMfgTool.h"

#include "../../Libs/DevSupport/Volume.h"

// CConfigUSBPortPage dialog
extern TCHAR FirstDriveFromMask(ULONG unitmask);

IMPLEMENT_DYNAMIC(CConfigUSBPortPage, CPropertyPage)
CConfigUSBPortPage::CConfigUSBPortPage()
	: CPropertyPage(CConfigUSBPortPage::IDD)
	, m_num_enabled_ports(0)
	, m_stale(0)
	, m_hDeviceManagerCallback(0)
	, m_bPortsChanged(FALSE)
{
	m_MaxEnabledPorts = AfxGetApp()->GetProfileInt(_T("Options"), _T("MaxUSBPorts"), DEFAULT_PORTS);
	// Make sure nobody tried to manually change
	// it in registry to something we don't support.
	if( m_MaxEnabledPorts != 4 && m_MaxEnabledPorts != 8 && m_MaxEnabledPorts != 12 && m_MaxEnabledPorts != 16)
		m_MaxEnabledPorts = 8;

	m_num_enabled_ports = AfxGetApp()->GetProfileInt(_T("Options"), _T("EnabledPorts"), 0);
}

CConfigUSBPortPage::~CConfigUSBPortPage()
{
}

#ifdef _DEBUG
void CConfigUSBPortPage::Dump(CDumpContext& dc) const
{
	CPropertyPage::Dump(dc);
	dc << "CConfigUSBPortPage = " << this;
}
#endif

void CConfigUSBPortPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_USB_TREE, m_tree_ctrl);
}

BEGIN_MESSAGE_MAP(CConfigUSBPortPage, CPropertyPage)
	ON_NOTIFY(NM_CLICK, IDC_USB_TREE, OnNMClickUsbTree)
	ON_NOTIFY(TVN_KEYDOWN, IDC_USB_TREE, OnTvnKeydownUsbTree)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_USBPORTS_ENABLED_PORTS, &CConfigUSBPortPage::OnCbnSelchangeUsbportsEnabledPorts)
END_MESSAGE_MAP()

// CConfigUSBPortPage message handlers
void CConfigUSBPortPage::OnDestroy()
{
	CPropertyPage::OnDestroy();

	DoTreeOperation(OP_DELETE_ALL);

	gDeviceManager::Instance().Unregister(m_hDeviceManagerCallback);
}

void CConfigUSBPortPage::OnOK()
{
	CPropertyPage::OnOK();

	CClientDC dc(this);
	CFont* def_font = dc.SelectObject(&m_hub_required_font);
	dc.SelectObject(def_font);

	// Done with the font. Delete the font object.
	m_hub_required_font.DeleteObject();

	if( m_bPortsChanged )
	{
		UINT testCount = 0;
		AfxGetApp()->WriteProfileInt(_T("Options"), _T("MaxUSBPorts"), m_MaxEnabledPorts);
		AfxGetApp()->WriteProfileInt(_T("Options"), _T("EnabledPorts"), m_num_enabled_ports);

		// write out new port info
		for( int i = 0; i < MAX_PORTS; ++i )
		{
			CString csPanel;

			csPanel.Format(L"Panel %C", 'A' + i);
			if( m_pPortMappings[i] )
			{
				AfxGetApp()->WriteProfileString(csPanel, _T("Hub Name"), m_pPortMappings[i]->GetName());
				AfxGetApp()->WriteProfileInt(csPanel, _T("Hub Index"), (int)m_pPortMappings[i]->GetIndex());
				++testCount;
			}
			else
			{
				AfxGetApp()->WriteProfileString(csPanel, _T("Hub Name"), L"");
				AfxGetApp()->WriteProfileInt(csPanel, _T("Hub Index"), (int)-1);
			}
		}
		ASSERT(testCount <= m_num_enabled_ports);
	}

	// set the refresh flag so mainfrm will re-create the port dialogs if modified
	AfxGetApp()->WriteProfileInt(_T("Options"), _T("Refresh"), m_bPortsChanged);
}

void CConfigUSBPortPage::OnCancel()
{
	CPropertyPage::OnCancel();

	CClientDC dc(this);
	CFont* def_font = dc.SelectObject(&m_hub_required_font);
	dc.SelectObject(def_font);

	// Done with the font. Delete the font object.
	m_hub_required_font.DeleteObject();
}

BOOL CConfigUSBPortPage::OnInitDialog()
{
	CString csText;
	CComboBox *pEnabledPorts = (CComboBox *)GetDlgItem(IDC_USBPORTS_ENABLED_PORTS);
	UINT connectedPorts = 0;


	CPropertyPage::OnInitDialog();

	for (UINT i = 0; (UINT)i < MAX_PORTS; ++i)
	{
		CPortMgrDlg* pDlg;
		// unassigned port				
		m_pPortMappings[i] = NULL;
		if( i < m_MaxEnabledPorts )
		{
			pDlg = theApp.GetPortDlg(i);
			if ( pDlg ) 
			{
				usb::Port* pPort = pDlg->GetUsbPort();
				if( pPort )
				{
					m_pPortMappings[i] = pPort;
					++connectedPorts;
				}
			}	
		}
	}

	// if the number of attached ports has changed (hub removed) change teh number of enabled ports
	if ( connectedPorts < m_num_enabled_ports && connectedPorts <= m_MaxEnabledPorts )
		m_num_enabled_ports = connectedPorts;


	// Create a DeviceManager::DeviceChangeCallback to objectize the 
	// callback member function. In this example, the Functor 'cmd'
	// is created to objectize CDeviceApiDlg::OnDeviceChangeNotify().
	DeviceManager::DeviceChangeCallback cmd(this, &CConfigUSBPortPage::OnDeviceChangeNotify);
	//
    // Register with DeviceManager to call us back for any device change (default parameters).
	m_hDeviceManagerCallback = gDeviceManager::Instance().Register(cmd);

	pEnabledPorts->AddString(L"4");
	pEnabledPorts->AddString(L"8");
	pEnabledPorts->AddString(L"12");
	pEnabledPorts->AddString(L"16");

	csText.Format(L"%d", m_MaxEnabledPorts);
	pEnabledPorts->SelectString(-1, csText);

	csText.LoadStringW(IDS_CFG_USBPORT_MAX_PORTS);
	GetDlgItem(IDC_USBPORTS_PORTS_ENABLED_TEXT)->SetWindowTextW(csText);
/*
	// Set no hub warning text.
    csText.LoadStringW(IDS_HUB_REQUIRED);
   	SetDlgItemText(IDC_HUB_REQUIRED, csText);
    // Create the colored and blinking <Danger>
   	m_hub_required_ctrl.SubclassDlgItem(IDC_HUB_REQUIRED, this);
    // Set a special font
   	CFont * p_font = m_hub_required_ctrl.GetFont();
    LOGFONT l_font;
   	p_font->GetLogFont(&l_font);
    l_font.lfHeight = -12;
   	l_font.lfWeight = FW_SEMIBOLD;
    if ( m_hub_required_font.CreateFontIndirect(&l_font) )
   	{
	    m_hub_required_ctrl.SetFont(&m_hub_required_font);// RGB(206,17,5)
    }
   	// Set background blinking colors
    // Medium and light red
   	m_hub_required_ctrl.SetBkColor(RGB(243,243,243));
    m_hub_required_ctrl.SetBlinkTextColors(RGB(128,0,0), RGB(255,0,0));
    m_hub_required_ctrl.ShowWindow(SW_HIDE);
    // Start text blinking
   	m_hub_required_ctrl.StartTextBlink(TRUE, CColorStaticST::ST_FLS_FAST);
*/
	DoTreeOperation(OP_INIT_TREE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigUSBPortPage::OnSetActive()
{
	if ( m_stale )
		PostMessage( WM_MSG_PD_EVENT, m_stale, 0 );

	return CPropertyPage::OnSetActive();
}

void CConfigUSBPortPage::OnNMClickUsbTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	// 1 to cancel default processing, 0 to allow default processing
	*pResult = 0;
	const MSG* msg = m_tree_ctrl.GetCurrentMessage();
	CPoint pt = msg->pt;
	UINT uFlags=0;
	m_tree_ctrl.ScreenToClient(&pt);
	HTREEITEM hti = m_tree_ctrl.HitTest(pt,&uFlags);

	if( uFlags & TVHT_ONITEMSTATEICON )	{
		EnablePort(hti);
	}
}

void CConfigUSBPortPage::OnTvnKeydownUsbTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	// 1 to cancel default processing, 0 to allow default processing
	*pResult = 0;
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);

	if( pTVKeyDown->wVKey == VK_SPACE )	{
		HTREEITEM hti = m_tree_ctrl.GetSelectedItem();
		if (hti) {
			EnablePort(hti);
		}
	}
}

CConfigUSBPortPage::CbState_t
CConfigUSBPortPage::EnablePort(HTREEITEM _hti)
{
	CbState_t state = (CbState_t)(m_tree_ctrl.GetItemState( _hti, TVIS_STATEIMAGEMASK )>>12);
	PortNodeInfo * pInfo = (PortNodeInfo*)m_tree_ctrl.GetItemData(_hti);

	if ( pInfo == NULL )
		return CbStateNone;

	Sleep(250);	// allows previous tree operation to complete.
				// if you were quick you could enable another item before it became checked/greyed.

	 // UNCHECKED -> CHECKED
	if (state == CbStateUnchecked ) 
	{
		ASSERT(pInfo->pUsbHub);
		ASSERT(pInfo->PortIndex != 0);
		ASSERT(pInfo->DlgIndex == -1);
		pInfo->CbState = CbStateChecked;
		// assign it to an unassigned Port Dlg and
		// remember who we assigned it to
		int dlgIndex;
		for ( dlgIndex = 0; dlgIndex < (int)m_MaxEnabledPorts; ++dlgIndex )
		{
			if( m_pPortMappings[dlgIndex] == NULL )
			{
				m_pPortMappings[dlgIndex] = pInfo->pUsbHub->Port(pInfo->PortIndex);
				pInfo->DlgIndex = dlgIndex;
				m_bPortsChanged = TRUE;
				break;
			}

		} // end for ( find an unassigned display )
		ASSERT(pInfo->DlgIndex != -1);
		// if that's all the ports we support, disable the unchecked ports
		// !NOTE: must be done AFTER pDlg->SetUSBPort
		if ( AddRef() == m_MaxEnabledPorts )
		{
			DoTreeOperation(OP_GRAY_UNCHECKED);
		}
		// make the node label reflect the Port Dlg assignment
		SetPortItemText(_hti, pInfo);
	}
	 // CHECKED -> UNCHECKED
	else if (state == CbStateChecked )
	{
		ASSERT(pInfo->pUsbHub);
		ASSERT(pInfo->PortIndex != 0);
		ASSERT(pInfo->DlgIndex != -1);
		pInfo->CbState = CbStateUnchecked;
		// have to do this because the state will automatically 
		// go to grayed after checked unless we manage it
		m_tree_ctrl.SetItemState(_hti, INDEXTOSTATEIMAGEMASK( CbStateChecked+1 ), TVIS_STATEIMAGEMASK);
		// unassign the Port Dlg
		ASSERT(m_pPortMappings[pInfo->DlgIndex] == pInfo->pUsbHub->Port(pInfo->PortIndex));
		m_pPortMappings[pInfo->DlgIndex] = NULL;
		pInfo->DlgIndex = -1;
		m_bPortsChanged = TRUE;
		// if now we are able to support more ports, enable the disabled ports 
		// !NOTE: must be done AFTER pDlg->SetUSBPort
		if ( Release() == m_MaxEnabledPorts-1 )
		{
			DoTreeOperation(OP_UNCHECK_GRAYED);
		}
		// make the node label reflect the Port Dlg assignment
		SetPortItemText(_hti, pInfo);
	}
	// GRAYED -> GRAYED
	else
	{
		pInfo->CbState = CbStateGrayed;
		// have to do this because the state will automatically 
		// go to unchecked after grayed unless we manage it
		m_tree_ctrl.SetItemState(_hti, INDEXTOSTATEIMAGEMASK( CbStateGrayed-1 ), TVIS_STATEIMAGEMASK);
	}

	return pInfo->CbState;
}

BOOL CConfigUSBPortPage::SetPortItemText(HTREEITEM _hti, PortNodeInfo* _pInfo)
{
	CString ui_str, label; int pos = 0;

	ASSERT(_pInfo);
	ASSERT(_pInfo->pUsbHub);

	// if it is a PORT
	if ( _pInfo->PortIndex != 0 )
	{
		usb::Port* pPort = _pInfo->pUsbHub->Port(_pInfo->PortIndex);

		// for changing between pages witout closing the config dialog
//clw		if ( m_stale )
//clw		{
//clw			pPort->Refresh();
//clw		}
		
		// if it is a PORT with a HUB connected to it
		if ( pPort->_isHub.get() )
		{
			if ((usb::Hub*)pPort->_device != NULL)
				label.Format( IDS_CFG_USBPORT_HUB_PORT, _pInfo->PortIndex, ((usb::Hub*)pPort->_device)->_index.get() );	
			else
				label = _T("DISABLED");
		}
		else
		{
			if ( _pInfo->DlgIndex == -1 )
			{
				ui_str.LoadString(IDS_CFG_USBPORT_UNASSIGNED);
			}
			else
			{
				ui_str.Format(IDS_CFG_USBPORT_PANEL, _T('A') + _pInfo->DlgIndex);
			}
			
			if ( pPort->_connected.get() == DeviceConnected )     // IDS_CFG_USBPORT_PORT clw
			{
				label.Format( _T("Port %d %s [Connected] "), _pInfo->PortIndex, ui_str );
				if ( pPort->_device == NULL )
				{
					label.Append( _T("Unknown"));
				}
				else
				{
					CString devDescription = _T("");
					switch( pPort->_device->GetDeviceType() )
					{
						case DeviceClass::DeviceTypeMsc:
						{
							Volume* pVolume = dynamic_cast<Volume*>(pPort->_device);
							devDescription = pVolume->_friendlyName.get();
							devDescription.AppendFormat(_T(" (%s)"),pPort->GetDriveLetters().c_str());
							break;
						}
						case DeviceClass::DeviceTypeHid:
                        case DeviceClass::DeviceTypeMxHid:
						//case DeviceClass::DeviceTypeMtp:
						case DeviceClass::DeviceTypeRecovery:
						case DeviceClass::DeviceTypeUsbController:
						case DeviceClass::DeviceTypeUsbHub:
						default:
							devDescription = pPort->_device->_description.get().c_str();
					}

					label += devDescription;
				}
			}
			else
			{
				label.Format( _T("Port %d %s [Not connected]"), _pInfo->PortIndex, ui_str );
			}
		}
	}
	// if it is a ROOT HUB
	else if ( _pInfo->PortIndex == 0 )
	{
		label.Format( IDS_CFG_USBPORT_HUB, _pInfo->pUsbHub->_index.get() );	
	}

	return ( m_tree_ctrl.SetItemText(_hti, label) );
}

void CConfigUSBPortPage::DoTreeOperation(CConfigUSBPortPage::e_TREE_OP _op_type)
{
	HTREEITEM hti;
	PortNodeInfo * pInfo = NULL;

	switch ( _op_type )
	{
		case OP_DELETE_ALL:
			hti = GetNextTreeItem(TVI_ROOT);
			while ( hti )
			{
				delete (PortNodeInfo*)m_tree_ctrl.GetItemData(hti);
				hti = GetNextTreeItem();
			}
			m_tree_ctrl.DeleteAllItems();
			break;
		
		case OP_GRAY_UNCHECKED:
			hti = GetNextTreeItem(TVI_ROOT);
			while ( hti )
			{
				pInfo = (PortNodeInfo*)m_tree_ctrl.GetItemData(hti);
				
				if ( pInfo && pInfo->CbState == CbStateUnchecked )
				{
					pInfo->CbState = CbStateGrayed;
					m_tree_ctrl.SetItemState(hti, INDEXTOSTATEIMAGEMASK( pInfo->CbState ), TVIS_STATEIMAGEMASK);
				}
				hti = GetNextTreeItem();
			}
			break;

		case OP_UNCHECK_ALL:
			hti = GetNextTreeItem(TVI_ROOT);
			while ( hti )
			{
				pInfo = (PortNodeInfo*)m_tree_ctrl.GetItemData(hti);
				
				if ( pInfo && (pInfo->CbState == CbStateChecked || pInfo->CbState == CbStateGrayed) )
				{
					pInfo->CbState = CbStateUnchecked;
					pInfo->DlgIndex = -1;
					m_tree_ctrl.SetItemState(hti, INDEXTOSTATEIMAGEMASK( pInfo->CbState ), TVIS_STATEIMAGEMASK);
				}
				hti = GetNextTreeItem();
			}
			m_num_enabled_ports = 0;
			break;
		
		case OP_UNCHECK_GRAYED:
			hti = GetNextTreeItem(TVI_ROOT);
			while ( hti )
			{
				pInfo = (PortNodeInfo*)m_tree_ctrl.GetItemData(hti);
				
				if ( (  pInfo && pInfo->CbState == CbStateGrayed )&& // if the node is grayed and
					 (  pInfo->pUsbHub && pInfo->PortIndex != 0 ) && // if the node is a port
					 ( !pInfo->pUsbHub->Port(pInfo->PortIndex)->_isHub.get() ) ) // and a HUB is not connected to the port
				{
					pInfo->CbState = CbStateUnchecked;
					m_tree_ctrl.SetItemState(hti, INDEXTOSTATEIMAGEMASK( pInfo->CbState ), TVIS_STATEIMAGEMASK);
				}
				hti = GetNextTreeItem();
			}
			break;
		case OP_REFRESH_PORTS:
			// OP_INIT_TREE must have been called before calling OP_REFRESH_PORTS
			hti = GetNextTreeItem(TVI_ROOT);
			while ( hti )
			{
				pInfo = (PortNodeInfo*)m_tree_ctrl.GetItemData(hti);
				
				if ( pInfo && pInfo->pUsbHub && pInfo->PortIndex != 0 ) // if the node  is  a port
				{
					SetTreeItemInfo(hti, pInfo->pUsbHub, pInfo->PortIndex);
				}
				hti = GetNextTreeItem();
			}
			if ( m_num_enabled_ports == m_MaxEnabledPorts )
			{
				DoTreeOperation( OP_GRAY_UNCHECKED );
			}
			m_stale = false;
			break;
		
		case OP_INIT_TREE:
		{
			// Empty the tree
			DoTreeOperation(OP_DELETE_ALL);

			// Get the list of the USB Controllers
			std::list<Device*> usbControllerList = gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbController]->Devices();

			//assert( !usbControllerList.empty() );
			if( usbControllerList.empty() )
				break;

			// Insert a "Computer" node at the head of the tree
			CString resStr;
			resStr.LoadString(IDS_CFG_USBPORT_MYCOMPUTER);
			HTREEITEM htiRoot = m_tree_ctrl.InsertItem(resStr, TVI_ROOT);
			SetTreeItemInfo( htiRoot );
			
			// Loop through the USB Controllers
			std::list<Device*>::iterator controllerItem;

			m_bFoundSecondaryController = FALSE;
			for ( controllerItem = usbControllerList.begin(); controllerItem != usbControllerList.end(); ++controllerItem )
			{
				// cast our Device* to a usb::Controller*
				usb::Controller* pController = dynamic_cast<usb::Controller*>(*controllerItem);
				
				// insert the controller
				resStr.LoadString(IDS_CFG_USBPORT_CONTROLLER);
				HTREEITEM htiController = m_tree_ctrl.InsertItem(resStr, htiRoot);
				SetTreeItemInfo( htiController, pController );
				
				// insert the controller's root_hub
				usb::Hub* pRootHub = pController->GetRootHub();
				HTREEITEM htiRootHub = m_tree_ctrl.InsertItem(pRootHub->name(), htiController);
				SetTreeItemInfo( htiRootHub, pRootHub, 0 );


				// insert the root_hub's ports ( and any downstream hubs and their ports as well)
#ifdef SERIALIZE_HID
				InsertHubPorts(pRootHub, htiRootHub, FALSE);
#else
				InsertHubPorts(pRootHub, htiRootHub, TRUE);
#endif
			}

			// if we found no external hub, display required text
#ifdef SERIALIZE_HID
/*			if ( m_bFoundSecondaryController )
			    m_hub_required_ctrl.ShowWindow(SW_HIDE);
			else
			    m_hub_required_ctrl.ShowWindow(SW_SHOW);
*/
#endif

			if ( m_num_enabled_ports == m_MaxEnabledPorts )
				DoTreeOperation( OP_GRAY_UNCHECKED );
			// scroll to the top
			m_tree_ctrl.EnsureVisible(m_tree_ctrl.GetRootItem());
			m_stale = false;
			break;
		}

		default:
			break;

	} // end switch ( _op_type )
}

void CConfigUSBPortPage::InsertHubPorts(usb::Hub *const pHub, const HTREEITEM htiHub, BOOL _bInsertPorts)
{
	uint8_t portIndex;

	for ( portIndex = 1; portIndex <= pHub->GetNumPorts(); ++portIndex )
	{
		HTREEITEM htiPort;
//clw		pHub->Port(portIndex)->Refresh();

		if( _bInsertPorts || pHub->Port(portIndex)->_isHub.get() )
		{	// only insert hubs and ports on secondary hubs (external)
			htiPort = m_tree_ctrl.InsertItem(pHub->Port(portIndex)->name(), htiHub);
			SetTreeItemInfo( htiPort, pHub, portIndex );
			m_tree_ctrl.EnsureVisible(htiPort);
			m_bFoundSecondaryController = TRUE;
		}
		if ( pHub->Port(portIndex)->_isHub.get() )
		{
			usb::Hub* pPortHub = dynamic_cast<usb::Hub*>(pHub->Port(portIndex)->_device);
			if (pPortHub)
				InsertHubPorts(pPortHub, htiPort, TRUE);
		}
	}
}

// For the ROOT_NODE and Controller nodes
CConfigUSBPortPage::PortNodeInfo* 
CConfigUSBPortPage::SetTreeItemInfo(const HTREEITEM _hti, const usb::Controller *const _pUsbController)
{
	PortNodeInfo* pNodeInfo = (PortNodeInfo*)m_tree_ctrl.GetItemData(_hti);

	if ( pNodeInfo == NULL )
	{
		pNodeInfo = new PortNodeInfo(NULL, 0, CbStateNone, -1);
	}
	
	if ( pNodeInfo )
	{
		m_tree_ctrl.SetItemData( _hti, (DWORD_PTR) pNodeInfo );
		m_tree_ctrl.SetItemState(_hti, INDEXTOSTATEIMAGEMASK( pNodeInfo->CbState ), TVIS_STATEIMAGEMASK);
	}

	return pNodeInfo;
}

// For RootHubs _index =0, For Ports we have pHub and _index ( 1 or greater)
CConfigUSBPortPage::PortNodeInfo* 
CConfigUSBPortPage::SetTreeItemInfo(const HTREEITEM _hti, usb::Hub *const _pUsbHub, const uint8_t _index)
{
	PortNodeInfo * pNodeInfo = (PortNodeInfo*)m_tree_ctrl.GetItemData(_hti);
	
	if ( pNodeInfo == NULL )
	{
		pNodeInfo = new PortNodeInfo(_pUsbHub, _index);
	}

	if ( pNodeInfo )
	{

		GetPortState( pNodeInfo );
		
		// if the node is a HUB or a PORT
		if ( pNodeInfo->pUsbHub )
		{
			SetPortItemText(_hti, pNodeInfo);
		}
		
		m_tree_ctrl.SetItemData( _hti, (DWORD_PTR) pNodeInfo );
		m_tree_ctrl.SetItemState(_hti, INDEXTOSTATEIMAGEMASK( pNodeInfo->CbState ), TVIS_STATEIMAGEMASK);
	}

	return pNodeInfo;
}

CConfigUSBPortPage::CbState_t 
CConfigUSBPortPage::GetPortState(PortNodeInfo* _info)
{
	// if it isn't a port, or a port with a hub connected to it
	if (  (_info->pUsbHub == NULL) || ( _info->PortIndex == 0 ) || _info->pUsbHub->Port(_info->PortIndex)->_isHub.get() )
	{
		// a PortMgrDlg can only be associated with a port
		_info->DlgIndex = -1;
		
		// if it is a hub, or a port with a hub cnnected to it, gray the box
		if (  ((_info->pUsbHub != NULL) && (_info->PortIndex == 0)) ||
			    _info->pUsbHub->Port(_info->PortIndex)->_isHub.get() )
		{
			_info->CbState = CbStateGrayed;
		}
		// else its a controller or the root node, so no box
		else
		{
			_info->CbState = CbStateNone;
		}
		
		return _info->CbState;
	}
	
	// search through the Port Dlgs to see if one has a matching 
	// Hub Name and Hub Index
	int dlgIndex;
	for ( dlgIndex = 0; dlgIndex < (int)m_MaxEnabledPorts; ++dlgIndex)
	{
		if( m_pPortMappings[dlgIndex] &&
			(m_pPortMappings[dlgIndex]->GetIndex() == _info->PortIndex ) &&
			(m_pPortMappings[dlgIndex]->GetName() == _info->pUsbHub->_path.get()) )
		{
			if( m_pPortMappings[dlgIndex] != _info->pUsbHub->Port(_info->PortIndex) )
			{
				m_pPortMappings[dlgIndex] = _info->pUsbHub->Port(_info->PortIndex);
				m_bPortsChanged = TRUE;
			}
			_info->DlgIndex = dlgIndex;
			_info->CbState = CbStateChecked;

			return CbStateChecked;
		}

	} // end for ( find the assigned display )

	// not assigned so it is unchecked
	_info->DlgIndex = -1;
	_info->CbState = CbStateUnchecked;

	return CbStateUnchecked;
}


HTREEITEM CConfigUSBPortPage::GetNextTreeItem( HTREEITEM _hti /*= 0 */ )
{
	BOOL walkDone = FALSE;
	static HTREEITEM hti_last;
	HTREEITEM hti_next = 0, hti = 0;

	if ( _hti == TVI_ROOT )
		hti_next = m_tree_ctrl.GetRootItem();
	else if ( _hti )
		hti = _hti;
	else
		hti = hti_last;

	if ( !hti ) {
		hti_next = 0;
	}
	
	while (!walkDone) {
		// do work
		if ( hti_next )
			break;
		// end work

		// find the next node
		hti_next = m_tree_ctrl.GetNextItem(hti, TVGN_CHILD);
		if (hti_next) {
			hti = hti_next;
			continue;
		}

		for (;;) {
			hti_next = m_tree_ctrl.GetNextSiblingItem(hti);
			if (hti_next) {
				hti = hti_next;
				break;
			}

			hti_next = m_tree_ctrl.GetParentItem(hti);
			if (hti_next) {
				hti = hti_next;
			}
			else
			{
				walkDone = 1;
				break;
			}
		}
	} // end while(!walkDone)

	hti_last = hti_next;
	return hti_next;
}

void CConfigUSBPortPage::SetStale(UINT_PTR _missed_msg)
{	
	if ( m_stale == CPortMgrDlg::PD_EVNT_HUB_ARRIVAL ||
		 m_stale == CPortMgrDlg::PD_EVNT_HUB_REMOVAL )
		 return;
	else
		m_stale = _missed_msg; 
}

bool CConfigUSBPortPage::OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo)
{

	bool ret = ERROR_SUCCESS;

	switch(nsInfo.Event)
	{
		case DeviceManager::DEVICE_ARRIVAL_EVT:
		case DeviceManager::VOLUME_ARRIVAL_EVT:
		case DeviceManager::DEVICE_REMOVAL_EVT:
		case DeviceManager::VOLUME_REMOVAL_EVT:
				m_stale = DeviceManager::DEVICE_ARRIVAL_EVT;
				DoTreeOperation(OP_REFRESH_PORTS);
				break;
		case DeviceManager::HUB_ARRIVAL_EVT:
		case DeviceManager::HUB_REMOVAL_EVT:
				m_stale = DeviceManager::HUB_ARRIVAL_EVT;	
				DoTreeOperation(OP_INIT_TREE);
				break;
		default:
			;// Process other WM_DEVICECHANGE notifications for other 
			;// devices or reasons.
	}

	return ret;
}

// CUSBTreeCtrl
CConfigUSBPortPage::CUSBTreeCtrl::CUSBTreeCtrl()
{
	m_il_state.Create(IDB_USB_STATE, 14, 1, RGB(255,255,255));
}

CConfigUSBPortPage::CUSBTreeCtrl::~CUSBTreeCtrl()
{
}

void CConfigUSBPortPage::CUSBTreeCtrl::PreSubclassWindow()
{
	SetImageList( &m_il_state, TVSIL_STATE );
	m_tool_tip_ctrl = GetToolTips();
	m_tool_tip_ctrl->Activate(FALSE);
	m_tool_tip_ctrl->SetDelayTime(TTDT_INITIAL, 3);

	CTreeCtrl::PreSubclassWindow();
}

BEGIN_MESSAGE_MAP(CConfigUSBPortPage::CUSBTreeCtrl, CTreeCtrl)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_NOTIFY_REFLECT(TVN_GETINFOTIP, OnTvnGetInfoTip)
END_MESSAGE_MAP()

// CUSBTreeCtrl message handlers
void CConfigUSBPortPage::CUSBTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	UINT uFlags;
	HTREEITEM hti = HitTest(point,&uFlags);

	if( hti && uFlags & TVHT_ONITEM )	{
		
		PortNodeInfo* info = (PortNodeInfo*)GetItemData(hti);
//clw		CString msg_str = info->p_usb_obj->GetPortInfoStr();
//clw		if (msg_str.IsEmpty())
//clw			msg_str = _T("No decice connected.");
		
		m_tool_tip_ctrl->Activate(true);
		return;
	}
		CTreeCtrl::OnRButtonDown(nFlags, point);
}

void CConfigUSBPortPage::CUSBTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	UINT uFlags;
	HTREEITEM hti = HitTest(point,&uFlags);

	if( hti && uFlags & TVHT_ONITEM )	{
		m_tool_tip_ctrl->Activate(false);
		return;
	}

	CTreeCtrl::OnRButtonUp(nFlags, point);
}

void CConfigUSBPortPage::CUSBTreeCtrl::OnTvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMTVGETINFOTIP>(pNMHDR);

	PortNodeInfo* info = (PortNodeInfo*)GetItemData(pGetInfoTip->hItem);
	CString msg_str;
//clw	msg_str = info->p_usb_obj->GetPortInfoStr();
//clw	if (msg_str.IsEmpty())
//clw		msg_str = _T("No decice connected.");

	msg_str.CopyChars(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, msg_str, __min(pGetInfoTip->cchTextMax, msg_str.GetLength()) );

	*pResult = 0;
}

void CConfigUSBPortPage::OnCbnSelchangeUsbportsEnabledPorts()
{
	TCHAR szValue[5];
	UINT savedMax = m_MaxEnabledPorts;

	CComboBox *pEnabledPorts = (CComboBox *)GetDlgItem(IDC_USBPORTS_ENABLED_PORTS);

	pEnabledPorts->GetLBText(pEnabledPorts->GetCurSel(), szValue);
	m_MaxEnabledPorts = _tstoi(szValue);

	m_bPortsChanged = TRUE;

	if( m_MaxEnabledPorts < savedMax )
	{	// need to clear all selected ports
		int dlgIndex;

		DoTreeOperation( OP_UNCHECK_ALL );

		for ( dlgIndex = 0; dlgIndex < (int)MAX_PORTS; ++dlgIndex )
		{
			if ( m_pPortMappings[dlgIndex] != NULL )
				m_pPortMappings[dlgIndex] = NULL;
		}
	}
}
