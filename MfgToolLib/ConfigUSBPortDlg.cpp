/*
 * Copyright (C) 2009-2012, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

// ConfigUSBPortDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MfgToolLib.h"
#include "ConfigUSBPortDlg.h"

//BOOL g_bConfigUsbPortDlgOpen;
//HWND g_hConfigUsbPortDlg;


// CConfigUSBPortDlg dialog

IMPLEMENT_DYNAMIC(CConfigUSBPortDlg, CDialog)

CConfigUSBPortDlg::CConfigUSBPortDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigUSBPortDlg::IDD, pParent)
{

}

CConfigUSBPortDlg::~CConfigUSBPortDlg()
{
}

void CConfigUSBPortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_USB_TREE, m_tree_ctrl);
}


BEGIN_MESSAGE_MAP(CConfigUSBPortDlg, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_USB_TREE, OnNMClickUsbTree)
    ON_NOTIFY(TVN_KEYDOWN, IDC_USB_TREE, OnTvnKeydownUsbTree)
	ON_WM_DESTROY()
	//ON_MESSAGE(UM_DEVICE_CHANGE_CFG_DLG_NOTIFY, OnDeviceChangeNotify)
	ON_MESSAGE(UM_DEVICE_CHANGE, OnDeviceChangeNotify)
END_MESSAGE_MAP()


// CUSBTreeCtrl
CConfigUSBPortDlg::CUSBTreeCtrl::CUSBTreeCtrl()
{
    m_il_state.Create(IDB_USB_STATE, 14, 1, RGB(255,255,255));
}

CConfigUSBPortDlg::CUSBTreeCtrl::~CUSBTreeCtrl()
{
}

void CConfigUSBPortDlg::CUSBTreeCtrl::PreSubclassWindow()
{
    SetImageList( &m_il_state, TVSIL_STATE );
    m_tool_tip_ctrl = GetToolTips();
    m_tool_tip_ctrl->Activate(FALSE);
    m_tool_tip_ctrl->SetDelayTime(TTDT_INITIAL, 3);

    CTreeCtrl::PreSubclassWindow();
}

BEGIN_MESSAGE_MAP(CConfigUSBPortDlg::CUSBTreeCtrl, CTreeCtrl)
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_NOTIFY_REFLECT(TVN_GETINFOTIP, OnTvnGetInfoTip)
END_MESSAGE_MAP()

// CUSBTreeCtrl message handlers
void CConfigUSBPortDlg::CUSBTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    UINT uFlags;
    HTREEITEM hti = HitTest(point,&uFlags);

    if( hti && uFlags & TVHT_ONITEM )   
	{   
        PortNodeInfo* info = (PortNodeInfo*)GetItemData(hti);        
        m_tool_tip_ctrl->Activate(true);
        return;
    }

	CTreeCtrl::OnRButtonDown(nFlags, point);
}

void CConfigUSBPortDlg::CUSBTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
    UINT uFlags;
    HTREEITEM hti = HitTest(point,&uFlags);

    if( hti && uFlags & TVHT_ONITEM )   
	{
        m_tool_tip_ctrl->Activate(false);
        return;
    }

    CTreeCtrl::OnRButtonUp(nFlags, point);
}

void CConfigUSBPortDlg::CUSBTreeCtrl::OnTvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMTVGETINFOTIP>(pNMHDR);

    PortNodeInfo* info = (PortNodeInfo*)GetItemData(pGetInfoTip->hItem);
    CString msg_str;

    //msg_str.CopyChars(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, msg_str, __min(pGetInfoTip->cchTextMax, msg_str.GetLength()) );

    *pResult = 0;
}


// CConfigUSBPortDlg message handlers

BOOL CConfigUSBPortDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	//g_bConfigUsbPortDlgOpen = TRUE;
	//g_hConfigUsbPortDlg = m_hWnd;
//	RegisterHWND(m_hWnd);

	UINT connectedPorts = 0;

	m_num_enabled_ports = 0;
	m_MaxEnabledPorts = 1;

	for(UINT i=0; i<MAX_CMDOP_NUMS; i++)
	{
		m_pPortMappings[i] = NULL;
		if( i < m_MaxEnabledPorts )
		{
			if(g_CmdOperationArray[i] != NULL)
			{
				usb::Port* pPort = g_CmdOperationArray[i]->m_p_usb_port;
				if(pPort != NULL)
				{
					m_pPortMappings[i] = pPort;
                    ++connectedPorts;
				}
			}	//end if(g_CmdOperationArray[i] != NULL)
		}	//end if( i < m_MaxEnabledPorts )
	}	//end for(int i=0; i<MAX_CMDOP_NUMS; i++)

	m_num_enabled_ports = connectedPorts;

	DoTreeOperation(OP_INIT_TREE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//void CConfigUSBPortDlg::DoTreeOperation(CConfigUSBPortDlg::e_TREE_OP _op_type, const DeviceClass::NotifyStruct* pNsinfo)
void CConfigUSBPortDlg::DoTreeOperation(CConfigUSBPortDlg::e_TREE_OP _op_type, const MxNotifyStruct* pNsinfo)
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
                 ( !pInfo->pUsbHub->Port(pInfo->PortIndex)->IsHub() ) ) // and a HUB is not connected to the port
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
                if((pNsinfo->Hub.Compare(pInfo->pUsbHub->_path.get()) == 0) && (pInfo->PortIndex == pNsinfo->HubIndex))
                {
                    usb::Port* pPort = pInfo->pUsbHub->Port(pInfo->PortIndex);
                    pPort->Refresh();
                    SetTreeItemInfo(hti, pInfo->pUsbHub, pInfo->PortIndex);
                }
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
            std::list<Device*> usbControllerList = g_devClasses[DeviceClass::DeviceTypeUsbController]->Devices();
            if( usbControllerList.empty() )
                break;

            // Insert a "Computer" node at the head of the tree
            CString resStr = _T("My Computer");
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
                resStr = _T("Controller");
                HTREEITEM htiController = m_tree_ctrl.InsertItem(resStr, htiRoot);
                SetTreeItemInfo( htiController, pController );
                
                // insert the controller's root_hub
                usb::Hub* pRootHub = pController->GetRootHub();
                HTREEITEM htiRootHub = m_tree_ctrl.InsertItem(pRootHub->name(), htiController);
                SetTreeItemInfo( htiRootHub, pRootHub, 0 );

                // insert the root_hub's ports ( and any downstream hubs and their ports as well)
                InsertHubPorts(pRootHub, htiRootHub, TRUE);
            }

            // if we found no external hub, display required text
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

HTREEITEM CConfigUSBPortDlg::GetNextTreeItem( HTREEITEM _hti /*= 0 */ )
{
    BOOL walkDone = FALSE;
    static HTREEITEM hti_last;
    HTREEITEM hti_next = 0, hti = 0;

    if ( _hti == TVI_ROOT )
	{
        hti_next = m_tree_ctrl.GetRootItem();
	}
    else if ( _hti )
	{
        hti = _hti;
	}
    else
	{
        hti = hti_last;
	}

    if ( !hti ) 
	{
        hti_next = 0;
    }
    
    while (!walkDone) 
	{
        if ( hti_next )
            break;
        // find the next node
        hti_next = m_tree_ctrl.GetNextItem(hti, TVGN_CHILD);
        if (hti_next) 
		{
            hti = hti_next;
            continue;
        }

        for (;;) 
		{
            hti_next = m_tree_ctrl.GetNextSiblingItem(hti);
            if (hti_next) 
			{
                hti = hti_next;
                break;
            }

            hti_next = m_tree_ctrl.GetParentItem(hti);
            if (hti_next) 
			{
                hti = hti_next;
            }
            else
            {
                walkDone = TRUE;
                break;
            }
        }
    } // end while(!walkDone)

    hti_last = hti_next;

    return hti_next;
}

// For the ROOT_NODE and Controller nodes
CConfigUSBPortDlg::PortNodeInfo* 
CConfigUSBPortDlg::SetTreeItemInfo(const HTREEITEM _hti, const usb::Controller *const _pUsbController)
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
CConfigUSBPortDlg::PortNodeInfo* 
CConfigUSBPortDlg::SetTreeItemInfo(const HTREEITEM _hti, usb::Hub *const _pUsbHub, const BYTE _index)
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

CConfigUSBPortDlg::CbState_t 
CConfigUSBPortDlg::GetPortState(PortNodeInfo* _info)
{
    // if it isn't a port, or a port with a hub connected to it
    if (  (_info->pUsbHub == NULL) || ( _info->PortIndex == 0 ) || _info->pUsbHub->Port(_info->PortIndex)->IsHub() )
    {
        // a PortMgrDlg can only be associated with a port
        _info->DlgIndex = -1;
        
        // if it is a hub, or a port with a hub cnnected to it, gray the box
        if (  ((_info->pUsbHub != NULL) && (_info->PortIndex == 0)) || _info->pUsbHub->Port(_info->PortIndex)->IsHub() )
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

BOOL CConfigUSBPortDlg::SetPortItemText(HTREEITEM _hti, PortNodeInfo* _pInfo)
{
    CString ui_str, label; 
	int pos = 0;

    ASSERT(_pInfo);
    ASSERT(_pInfo->pUsbHub);

    // if it is a PORT
    if ( _pInfo->PortIndex != 0 )
    {
        usb::Port* pPort = _pInfo->pUsbHub->Port(_pInfo->PortIndex);        
        // if it is a PORT with a HUB connected to it
        if ( pPort->IsHub() )
        {
            if ((usb::Hub*)pPort->GetDevice() != NULL)
                label.Format( _T("Port %d - Hub %d"), _pInfo->PortIndex, ((usb::Hub*)pPort->GetDevice())->_index.get() ); 
            else
                label = _T("DISABLED");
        }
        else
        {
            if ( _pInfo->DlgIndex == -1 )
            {
                ui_str = _T("(Unassigned)");
            }
            else
            {
                ui_str.Format(_T("(Panel %C)"), _T('A') + _pInfo->DlgIndex);
            }
            
            if ( pPort->Connected() == DeviceConnected )
            {
                label.Format( _T("Port %d %s [Connected] "), _pInfo->PortIndex, ui_str );
                if ( pPort->GetDevice() == NULL )
                {
                    label.Append( _T("Unknown"));
                }
                else
                {
                    CString devDescription = _T("");
                    switch( pPort->GetDevice()->GetDeviceType() )
                    {
                        case DeviceClass::DeviceTypeMsc:
                        {
//temp                            Volume* pVolume = dynamic_cast<Volume*>(pPort->GetDevice());
//temp                            devDescription = pVolume->_friendlyName.get();
//temp                            devDescription.AppendFormat(_T(" (%s)"),pPort->GetDriveLetters());
                            break;
                        }
                        case DeviceClass::DeviceTypeHid:
                        case DeviceClass::DeviceTypeMxHid:
                        case DeviceClass::DeviceTypeUsbController:
                        case DeviceClass::DeviceTypeUsbHub:
                        default:
                            devDescription = pPort->GetDevice()->_description.get();
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
        label.Format( _T("Hub %d"), _pInfo->pUsbHub->_index.get() ); 
    }

    return ( m_tree_ctrl.SetItemText(_hti, label) );
}

void CConfigUSBPortDlg::InsertHubPorts(usb::Hub *const pHub, const HTREEITEM htiHub, BOOL _bInsertPorts)
{
    BYTE portIndex;

    for ( portIndex = 1; portIndex <= pHub->GetNumPorts(); ++portIndex )
    {
        HTREEITEM htiPort;
        usb::Port* pPort = pHub->Port(portIndex);
        pPort->Refresh();

        if( _bInsertPorts || pPort->IsHub() )
        {      
            htiPort = m_tree_ctrl.InsertItem(pPort->name(), htiHub);
            SetTreeItemInfo( htiPort, pHub, portIndex );
            m_tree_ctrl.EnsureVisible(htiPort);
            m_bFoundSecondaryController = TRUE;
        }
        if ( pPort->IsHub() )
        {
            usb::Hub* pPortHub = dynamic_cast<usb::Hub*>(pPort->GetDevice());
            if (pPortHub)
                InsertHubPorts(pPortHub, htiPort, TRUE);
        }
    }
}

void CConfigUSBPortDlg::OnNMClickUsbTree(NMHDR *pNMHDR, LRESULT *pResult)
{
    // 1 to cancel default processing, 0 to allow default processing
    *pResult = 0;
    const MSG* msg = m_tree_ctrl.GetCurrentMessage();
    CPoint pt = msg->pt;
    UINT uFlags=0;
    m_tree_ctrl.ScreenToClient(&pt);
    HTREEITEM hti = m_tree_ctrl.HitTest(pt,&uFlags);

    if( uFlags & TVHT_ONITEMSTATEICON ) 
	{
        EnablePort(hti);
    }
}

void CConfigUSBPortDlg::OnTvnKeydownUsbTree(NMHDR *pNMHDR, LRESULT *pResult)
{
    // 1 to cancel default processing, 0 to allow default processing
    *pResult = 0;
    LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);

    if( pTVKeyDown->wVKey == VK_SPACE ) 
	{
        HTREEITEM hti = m_tree_ctrl.GetSelectedItem();
        if (hti) 
		{
            EnablePort(hti);
        }
    }
}

CConfigUSBPortDlg::CbState_t CConfigUSBPortDlg::EnablePort(HTREEITEM _hti)
{
    CbState_t state = (CbState_t)(m_tree_ctrl.GetItemState( _hti, TVIS_STATEIMAGEMASK )>>12);
    PortNodeInfo * pInfo = (PortNodeInfo*)m_tree_ctrl.GetItemData(_hti);

    if ( pInfo == NULL )
	{
        return CbStateNone;
	}

    Sleep(250); // allows previous tree operation to complete.
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


void CConfigUSBPortDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: Add your message handler code here
	//g_bConfigUsbPortDlgOpen = FALSE;
	//g_hConfigUsbPortDlg = NULL;
//	UnregisterHWND(m_hWnd);
	DoTreeOperation(OP_DELETE_ALL);
}

LRESULT CConfigUSBPortDlg::OnDeviceChangeNotify(WPARAM wParam, LPARAM lParam)
{
	MxNotifyStruct* pnsInfo = (MxNotifyStruct *)wParam;

	switch(pnsInfo->Event)
	{
	case MX_DEVICE_ARRIVAL_EVT:
    case MX_VOLUME_ARRIVAL_EVT:
	case MX_DEVICE_REMOVAL_EVT:
    case MX_VOLUME_REMOVAL_EVT:
		DoTreeOperation(OP_REFRESH_PORTS, pnsInfo);
		break;
	case MX_HUB_ARRIVAL_EVT:
    case MX_HUB_REMOVAL_EVT:
		DoTreeOperation(OP_INIT_TREE);
		break;
	}

	return 0;
}

void CConfigUSBPortDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	if(m_bPortsChanged)
	{
		// write out new port info
		CString csPanel;
		for( int i = 0; i < MAX_CMDOP_NUMS; ++i )
		{
			csPanel.Format(_T("Panel%dInfo"), i);
			CString strTemp;
			if(m_pPortMappings[i])
			{
			/*	::WritePrivateProfileString(csPanel, _T("HubName"), m_pPortMappings[i]->GetName(), theApp.m_strParamFilename);
				strTemp.Format(_T("%d"), m_pPortMappings[i]->GetIndex());
				::WritePrivateProfileString(csPanel, _T("PortIndex"), strTemp, theApp.m_strParamFilename);
				strTemp.Format(_T("%d"), m_pPortMappings[i]->GetParentHub()->_index.get());
				::WritePrivateProfileString(csPanel, _T("HubIndex"), strTemp, theApp.m_strParamFilename);
				strTemp = m_pPortMappings[i]->GetDevice()->_description.get();
				::WritePrivateProfileString(csPanel, _T("DevcieDesc"), strTemp, theApp.m_strParamFilename); */
			}
			else
			{
			/*	::WritePrivateProfileString(csPanel, _T("HubName"), _T(""), theApp.m_strParamFilename);
				strTemp.Format(_T("%d"), 0);
				::WritePrivateProfileString(csPanel, _T("PortIndex"), strTemp, theApp.m_strParamFilename);
				strTemp.Format(_T("%d"), -1);
				::WritePrivateProfileString(csPanel, _T("HubIndex"), strTemp, theApp.m_strParamFilename);
				strTemp = _T("");
				::WritePrivateProfileString(csPanel, _T("DevcieDesc"), strTemp, theApp.m_strParamFilename); */
			}
		}
	}

	CDialog::OnOK();
}
