/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// CConfigPlayerOpInfoListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigPlayerListCtrl.h"
#include "UpdateCommandList.h"
#include "ConfigPlayerProfilePage.h"

// CConfigPlayerOpInfoListCtrl

IMPLEMENT_DYNAMIC(CConfigPlayerListCtrl, CListCtrl)
CConfigPlayerListCtrl::CConfigPlayerListCtrl()
: CListCtrl()
{
	// do this here so it doesn't get re-created every time the dialog opens
	m_il_state.Create(IDB_OP_STATE, 14, 1, RGB(255,255,255));
}

CConfigPlayerListCtrl::~CConfigPlayerListCtrl()
{
}


BEGIN_MESSAGE_MAP(CConfigPlayerListCtrl, CListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomDraw)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
    ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CConfigPlayerListCtrl::SetEditMode(BOOL _edit_mode)
{
	long lStyleOld = GetWindowLong(m_hWnd, GWL_STYLE);
	_edit_mode ? lStyleOld |= LVS_EDITLABELS : lStyleOld &= ~(LVS_EDITLABELS);
	SetWindowLong(m_hWnd, GWL_STYLE, lStyleOld);
    Invalidate(TRUE);
}

void CConfigPlayerListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if( GetFocus() != this ) SetFocus();
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CConfigPlayerListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if( GetFocus() != this ) SetFocus();
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CConfigPlayerListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    LVHITTESTINFO lvhti;
    lvhti.pt = point;
	SubItemHitTest(&lvhti);
	// don't let the default handler cycle through all 3 state images
    if (lvhti.iItem != -1 && lvhti.flags & LVHT_ONITEMSTATEICON )
        return;
    else
        CListCtrl::OnLButtonDblClk(nFlags, point);
}

void CConfigPlayerListCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    LVHITTESTINFO lvhti;
    lvhti.pt = point;
	SubItemHitTest(&lvhti);
	// don't let the default handler cycle through all 3 state images
    if (lvhti.iItem != -1 && lvhti.flags & LVHT_ONITEMSTATEICON )
        return;
    else
		CListCtrl::OnRButtonDown(nFlags, point);
}

void CConfigPlayerListCtrl::OnNMCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
	NMCUSTOMDRAW &pNMCD = pLVCD->nmcd;

	if ( !(GetWindowLong(m_hWnd, GWL_STYLE) & LVS_EDITLABELS) ) {
		*pResult = CDRF_DODEFAULT;
		return;
	}

	switch (pNMCD.dwDrawStage)
	{
		case CDDS_PREPAINT:

			// We want item prepaint notifications, so...
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
		{
			int iRow = (int)pNMCD.dwItemSpec;

			OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) pNMCD.lItemlParam;
            pLVCD->clrTextBk = pOPLCD->crBackground;

			// We want item post-paint notifications, so...
			*pResult = CDRF_DODEFAULT; // | CDRF_NOTIFYPOSTPAINT;
			break;
		}
		default:
			*pResult = CDRF_DODEFAULT;
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// GetColumns
int CConfigPlayerListCtrl::GetColumns()
{
	return GetHeaderCtrl()->GetItemCount();
}

///////////////////////////////////////////////////////////////////////////////
// GetItemData
//
// The GetItemData and SetItemData functions allow for app-specific data
// to be stored, by using an extra field in the XLISTCTRLDATA struct.
//
DWORD_PTR CConfigPlayerListCtrl::GetItemData(int nItem)
{
	ASSERT(nItem >= 0);
	ASSERT(nItem < GetItemCount());
	if ((nItem < 0) || nItem >= GetItemCount())
		return 0;

	OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(nItem);
	if (!pOPLCD)
	{
		return 0;
	}

	return pOPLCD->dwItemData;
}

///////////////////////////////////////////////////////////////////////////////
// SetItemData
BOOL CConfigPlayerListCtrl::SetItemData(int nItem, DWORD dwData)
{
	ASSERT(nItem >= 0);
	ASSERT(nItem <= GetItemCount());
	if ((nItem < 0) || nItem > GetItemCount())
		return FALSE;

	OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(nItem);
	if (!pOPLCD)
	{
		return FALSE;
	}

	pOPLCD->dwItemData = dwData;

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// SetItemToolTipText
BOOL CConfigPlayerListCtrl::SetItemToolTipText(int nItem, int nSubItem, LPCTSTR lpszToolTipText)
{
	ASSERT(nItem >= 0);
	ASSERT(nItem < GetItemCount());
	if ((nItem < 0) || nItem >= GetItemCount())
		return FALSE;
	ASSERT(nSubItem >= 0);
	ASSERT(nSubItem < GetColumns());
	if ((nSubItem < 0) || nSubItem >= GetColumns())
		return FALSE;

	OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(nItem);
	if (!pOPLCD)
	{
		return FALSE;
	}

	pOPLCD[nSubItem].strToolTip = lpszToolTipText;

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// GetItemToolTipText
CString CConfigPlayerListCtrl::GetItemToolTipText(int nItem, int nSubItem)
{
	CString strToolTip;
	strToolTip = _T("");

	ASSERT(nItem >= 0);
	ASSERT(nItem < GetItemCount());
	if ((nItem < 0) || nItem >= GetItemCount())
		return strToolTip;
	ASSERT(nSubItem >= 0);
	ASSERT(nSubItem < GetColumns());
	if ((nSubItem < 0) || nSubItem >= GetColumns())
		return strToolTip;

	OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(nItem);
	if (pOPLCD)
	{
		strToolTip = pOPLCD[nSubItem].strToolTip;
	}

	return strToolTip;
}
///////////////////////////////////////////////////////////////////////////////
// OnToolHitTest
BOOL CConfigPlayerListCtrl::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
	LVHITTESTINFO lvhitTestInfo;
	
	lvhitTestInfo.pt = point;
    CConfigPlayerListCtrl* pList = (CConfigPlayerListCtrl*)this;
	
	int nItem = pList->SubItemHitTest(&lvhitTestInfo);
	int nSubItem = lvhitTestInfo.iSubItem;
//	TRACE(_T("in CConfigPlayerOpInfoListCtrl::OnToolHitTest: %d,%d\n"), nItem, nSubItem);

	UINT nFlags = lvhitTestInfo.flags;

	// nFlags is 0 if the SubItemHitTest fails
	// Therefore, 0 & <anything> will equal false
	if (nFlags & LVHT_ONITEMLABEL)
	{
		// If it did fall on a list item,
		// and it was also hit one of the
		// item specific subitems we wish to show tool tips for
		
		// get the client (area occupied by this control
		RECT rcClient;
		GetClientRect(&rcClient);
		
		// fill in the TOOLINFO structure
		pTI->hwnd = m_hWnd;
		pTI->uId = (UINT) (nItem * 1000 + nSubItem + 1);
		pTI->lpszText = LPSTR_TEXTCALLBACK;
		pTI->rect = rcClient;
		
		return (int)pTI->uId;	// By returning a unique value per listItem,
							// we ensure that when the mouse moves over another
							// list item, the tooltip will change
	}
	else
	{
		//Otherwise, we aren't interested, so let the message propagate
		return -1;
	}
}

///////////////////////////////////////////////////////////////////////////////
// OnToolTipText
BOOL CConfigPlayerListCtrl::OnToolTipText(UINT /*id*/, NMHDR * pNMHDR, LRESULT * pResult)
{
	UINT_PTR nID = pNMHDR->idFrom;
//	TRACE(_T("in CConfigPlayerOpInfoListCtrl::OnToolTipText: id=%d\n"), nID);
	
	// check if this is the automatic tooltip of the control
	if (nID == 0) 
		return TRUE;	// do not allow display of automatic tooltip,
						// or our tooltip will disappear
	
	// handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	
	*pResult = 0;
	
	// get the mouse position
	const MSG* pMessage;
	pMessage = GetCurrentMessage();
	ASSERT(pMessage);
	CPoint pt;
	pt = pMessage->pt;		// get the point from the message
	ScreenToClient(&pt);	// convert the point's coords to be relative to this control
	
	// see if the point falls onto a list item
	
	LVHITTESTINFO lvhitTestInfo;
	
	lvhitTestInfo.pt = pt;
	
	int nItem = SubItemHitTest(&lvhitTestInfo);
	int nSubItem = lvhitTestInfo.iSubItem;
	
	UINT nFlags = lvhitTestInfo.flags;
	
	// nFlags is 0 if the SubItemHitTest fails
	// Therefore, 0 & <anything> will equal false
	if (nFlags & LVHT_ONITEMLABEL)
	{
		// If it did fall on a list item,
		// and it was also hit one of the
		// item specific subitems we wish to show tooltips for
		
		CString strToolTip;
		strToolTip = _T("");

		OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(nItem);
		if (pOPLCD)
		{
			strToolTip = pOPLCD[nSubItem].strToolTip;
		}

		if (!strToolTip.IsEmpty())
		{
			// If there was a CString associated with the list item,
			// copy it's text (up to 80 characters worth, limitation 
			// of the TOOLTIPTEXT structure) into the TOOLTIPTEXT 
			// structure's szText member
			
#ifndef _UNICODE
			if (pNMHDR->code == TTN_NEEDTEXTA)
				lstrcpyn(pTTTA->szText, strToolTip, 80);
			else
				_mbstowcsz(pTTTW->szText, strToolTip, 80);
#else
			if (pNMHDR->code == TTN_NEEDTEXTA)
				_wcstombsz(pTTTA->szText, strToolTip, 80);
			else
				lstrcpyn(pTTTW->szText, strToolTip, 80);
#endif
			return FALSE;	 // we found a tool tip,
		}
	}
	
	return FALSE;	// we didn't handle the message, let the 
					// framework continue propagating the message
}

///////////////////////////////////////////////////////////////////////////////
// InsertItem
int CConfigPlayerListCtrl::InsertItem(const LVITEM* pItem)
{
	ASSERT(pItem->iItem >= 0);
	if (pItem->iItem < 0)
		return -1;

	int index = CListCtrl::InsertItem(pItem);

	if (index < 0)
		return index;

	OPLISTCTRLDATA *pOPLCD = new OPLISTCTRLDATA [GetColumns()];
	ASSERT(pOPLCD);
	if (!pOPLCD)
		return -1;

	pOPLCD[0].crText       = ::GetSysColor(COLOR_WINDOWTEXT);
	pOPLCD[0].crBackground = ::GetSysColor(COLOR_WINDOW);
    if ( pItem->mask && LVIF_PARAM )
        pOPLCD[0].dwItemData = (DWORD)pItem->lParam;

	CListCtrl::SetItemData(index, (DWORD_PTR) pOPLCD);

	return index;
}

///////////////////////////////////////////////////////////////////////////////
// InsertItem
int CConfigPlayerListCtrl::InsertItem(int nItem, LPCTSTR lpszItem)
{
	ASSERT(nItem >= 0);
	if (nItem < 0)
		return -1;

	return InsertItem(nItem,
					  lpszItem,
					  -1,
					  -1);
}

///////////////////////////////////////////////////////////////////////////////
// InsertItem
int CConfigPlayerListCtrl::InsertItem(int nItem,
						   LPCTSTR lpszItem,
						   COLORREF crText,
						   COLORREF crBackground)
{
	ASSERT(nItem >= 0);
	if (nItem < 0)
		return -1;

	int index = CListCtrl::InsertItem(nItem, lpszItem);

	if (index < 0)
		return index;

	OPLISTCTRLDATA *pOPLCD = new OPLISTCTRLDATA [GetColumns()];
	ASSERT(pOPLCD);
	if (!pOPLCD)
		return -1;

    pOPLCD[0].crText       = (crText == -1) ? ::GetSysColor(COLOR_WINDOWTEXT) : crText;
	pOPLCD[0].crBackground = (crBackground == -1) ? ::GetSysColor(COLOR_WINDOW) : crBackground;

	CListCtrl::SetItemData(index, (DWORD_PTR) pOPLCD);

	return index;
}

///////////////////////////////////////////////////////////////////////////////
// SetItem
int CConfigPlayerListCtrl::SetItem(const LVITEM* pItem)
{
	ASSERT(pItem->iItem >= 0);
	if (pItem->iItem < 0)
		return -1;

	BOOL rc = CListCtrl::SetItem(pItem);

	if (!rc)
		return FALSE;

	OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(pItem->iItem);
	if (pOPLCD)	{
		rc = TRUE;
	}
	else {
		rc = FALSE;
	}

	return rc;
}

///////////////////////////////////////////////////////////////////////////////
// SetItemText
//
// This function will set the text and colors for a subitem.  If lpszText
// is NULL, only the colors will be set.  If a color value is -1, the display
// color will be set to the default Windows color.
//
BOOL CConfigPlayerListCtrl::SetItemText(int nItem, int nSubItem, LPCTSTR lpszText,
					COLORREF crText, COLORREF crBackground)
{
	ASSERT(nItem >= 0);
	ASSERT(nItem < GetItemCount());
	if ((nItem < 0) || nItem >= GetItemCount())
		return FALSE;
	ASSERT(nSubItem >= 0);
	ASSERT(nSubItem < GetColumns());
	if ((nSubItem < 0) || nSubItem >= GetColumns())
		return FALSE;

	BOOL rc = TRUE;

	if (nItem < 0)
		return FALSE;

	if (lpszText)
		rc = CListCtrl::SetItemText(nItem, nSubItem, lpszText);

	OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(nItem);
	if (pOPLCD)
	{
		pOPLCD[nSubItem].crText       = (crText == -1) ? ::GetSysColor(COLOR_WINDOWTEXT) : crText;
		pOPLCD[nSubItem].crBackground = (crBackground == -1) ? ::GetSysColor(COLOR_WINDOW) : crBackground;
	}

	UpdateSubItem(nItem, nSubItem);

	return rc;
}

///////////////////////////////////////////////////////////////////////////////
// UpdateSubItem
void CConfigPlayerListCtrl::UpdateSubItem(int nItem, int nSubItem)
{
	ASSERT(nItem >= 0);
	ASSERT(nItem < GetItemCount());
	if ((nItem < 0) || nItem >= GetItemCount())
		return;
	ASSERT(nSubItem >= 0);
	ASSERT(nSubItem < GetColumns());
	if ((nSubItem < 0) || nSubItem >= GetColumns())
		return;

	CRect rect;
	if (nSubItem == -1)
	{
		GetItemRect(nItem, &rect, LVIR_BOUNDS);
	}
	else
	{
		GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect);
	}

	InvalidateRect(&rect);
	UpdateWindow();
}

///////////////////////////////////////////////////////////////////////////////
// DeleteItem
BOOL CConfigPlayerListCtrl::DeleteItem(int nItem)
{
	ASSERT(nItem >= 0);
	ASSERT(nItem < GetItemCount());
	if ((nItem < 0) || nItem >= GetItemCount())
		return FALSE;

	OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(nItem);
	if (pOPLCD)
		delete [] pOPLCD;
	CListCtrl::SetItemData(nItem, 0);
	return CListCtrl::DeleteItem(nItem);
}

///////////////////////////////////////////////////////////////////////////////
// DeleteAllItems
BOOL CConfigPlayerListCtrl::DeleteAllItems()
{
	int n = GetItemCount();
	for (int i = 0; i < n; i++)
	{
		OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(i);
		if (pOPLCD)
			delete [] pOPLCD;
		CListCtrl::SetItemData(i, 0);
	}

	return CListCtrl::DeleteAllItems();
}

void CConfigPlayerListCtrl::OnDestroy()
{
	int n = GetItemCount();
	for (int i = 0; i < n; i++)
	{
		OPLISTCTRLDATA *pOPLCD = (OPLISTCTRLDATA *) CListCtrl::GetItemData(i);
		if (pOPLCD)
			delete [] pOPLCD;
		CListCtrl::SetItemData(i, 0);
	}

    CListCtrl::OnDestroy();
}

void CConfigPlayerListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	BOOL bCanMoveUp, bCanMoveDown, bCanEdit, bCanBeEnabled, selection;
    COpInfo * pInfo = NULL;
    LVHITTESTINFO lvhti;
    CPoint client_pt(point);
	ScreenToClient(&client_pt);
	lvhti.pt = client_pt;
	SubItemHitTest(&lvhti);

    // no menu if you rt-click on the header control
    CRect header_rect;
    CHeaderCtrl* pHeader = GetHeaderCtrl();
    pHeader->GetWindowRect(&header_rect);
    if ( header_rect.PtInRect(point) )
        return;

    if (lvhti.iItem != -1 && lvhti.flags & LVHT_ONITEMLABEL ) {
	    pInfo = (COpInfo*)GetItemData(lvhti.iItem);
//		bCanMoveDown = lvhti.iItem < ( GetItemCount() - ( pInfo->GetProfile()->GetEditMode() == PROFILE_MODE_READ_ONLY ? 1 : 2 ) );
//		bCanEdit = lvhti.iItem != GetItemCount()-1 || pInfo->GetProfile()->GetEditMode() == PROFILE_MODE_READ_ONLY;
		bCanEdit = lvhti.iItem != GetItemCount();
//		bCanMoveUp = lvhti.iItem > 0 && bCanEdit;
		bCanMoveUp =	bCanEdit && lvhti.iItem > 0;
		bCanMoveDown =	bCanEdit &&
						lvhti.iItem < ( GetItemCount() - 1 );
		bCanBeEnabled = pInfo->GetStatus() == OPINFO_OK;
    }
    else {
   		bCanMoveDown = FALSE;
		bCanEdit = FALSE;
		bCanMoveUp = FALSE;
		bCanBeEnabled = FALSE;
    }

	CMenu cntxtMenu;
//	CMenu cntxtSubMenu;
	CString resStr, resStr2;

/*	cntxtSubMenu.CreatePopupMenu();

	resStr.LoadString(IDS_OPERATION_UPDATE);
	cntxtSubMenu.AppendMenu(MF_STRING | MF_ENABLED , IDM_NEW_UPDATE_OP , resStr);
	resStr.LoadString(IDS_OPERATION_COPY);
	cntxtSubMenu.AppendMenu(MF_STRING | MF_ENABLED , IDM_NEW_COPY_OP , resStr);
	resStr.LoadString(IDS_OPERATION_LOAD);
	cntxtSubMenu.AppendMenu(MF_STRING | MF_ENABLED , IDM_NEW_LOAD_OP , resStr);
	resStr.LoadString(IDS_OPERATION_OTP_REGISTER);
	cntxtSubMenu.AppendMenu(MF_STRING | MF_ENABLED , IDM_NEW_OTP_OP , resStr);
	cntxtSubMenu.AppendMenu(MF_STRING | MF_ENABLED , IDM_NEW_UTP_UPDATE_OP , _T("UTP_UPDATE"));
	cntxtSubMenu.AppendMenu(MF_STRING | MF_ENABLED , IDM_NEW_MX_UPDATE_OP , _T("MX_UPDATE"));

	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };

	// we have to use a seperate string for this; otherwise the data is overwritten in the
	// next LoadString().
	resStr2.LoadString(IDS_MENU_NEW);
    mii.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
    mii.wID = IDM_ST_NEW;
	mii.hSubMenu = cntxtSubMenu.GetSafeHmenu();
	mii.dwTypeData = (LPWSTR)resStr2.GetString();
*/

	cntxtMenu.CreatePopupMenu();

	resStr.LoadString(IDS_MENU_ENABLE);
	cntxtMenu.AppendMenu(MF_STRING | bCanBeEnabled ? MF_ENABLED : (MF_GRAYED | MF_DISABLED), IDM_ENABLED , resStr);
    if (pInfo) cntxtMenu.CheckMenuItem(IDM_ENABLED, pInfo->IsEnabled() ? MF_CHECKED : MF_UNCHECKED);
//	cntxtMenu.AppendMenu(MF_SEPARATOR , MF_SEPARATOR   ,	_T("") );

//	cntxtMenu.InsertMenuItem( 1, &mii, TRUE);

//	resStr.LoadString(IDS_MENU_EDIT);
//	cntxtMenu.AppendMenu(MF_STRING | (bCanEdit ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)), IDM_ST_EDIT, resStr);
//	resStr.LoadString(IDS_MENU_DELETE);
//	cntxtMenu.AppendMenu(MF_STRING | (bCanEdit ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)), IDM_ST_DELETE, resStr);
	cntxtMenu.AppendMenu(MF_SEPARATOR , MF_SEPARATOR   ,	_T("") );
	resStr.LoadString(IDS_MENU_MOVE_UP);
	cntxtMenu.AppendMenu(MF_STRING | (bCanMoveUp ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)), IDM_ST_MOVE_UP, resStr);
	resStr.LoadString(IDS_MENU_MOVE_DOWN);
	cntxtMenu.AppendMenu(MF_STRING | (bCanMoveDown ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)), IDM_ST_MOVE_DOWN, resStr);
	selection = cntxtMenu.TrackPopupMenu(TPM_LEFTALIGN, point.x+10, point.y, GetParent(), NULL); 
	cntxtMenu.DestroyMenu();
}

/////////////////////////////////////////////////////////////////////////////
// CComboOptionItem
CComboBox * CConfigPlayerListCtrl::ComboOptionItem(int nItem, int nSubItem, COpInfo * pOpInfo)
{
	CString strFind = GetItemText(nItem, nSubItem);

	//basic code start
	CRect rect;
	int offset = 0;
	// Make sure that the item is visible
	if( !EnsureVisible(nItem, TRUE)) 
		return NULL;

	GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect);
	// Now scroll if we need to expose the column
	CRect rcClient;
	GetClientRect(rcClient);
	if( offset + rect.left < 0 || offset + rect.left > rcClient.right )
	{
		CSize size;
		size.cx = offset + rect.left;
		size.cy = 0;
		Scroll(size);
		rect.left -= size.cx;
	}
	
	rect.left += offset;	
	rect.right = rect.left + GetColumnWidth(nSubItem);
	if(rect.right > rcClient.right) 
	   rect.right = rcClient.right;
	//basic code end

	rect.bottom += 100 * rect.Height();//dropdown area
	
	DWORD dwStyle =  WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST;
	CComboBox * pComboOptionItem = new CComboOptionItem(nItem, nSubItem);
	pComboOptionItem->Create(dwStyle, rect, this, NULL);
	pComboOptionItem->ModifyStyleEx(0,WS_EX_CLIENTEDGE);
	
	LoadCommandLists(pComboOptionItem, pOpInfo);

	pComboOptionItem->ShowDropDown();
	pComboOptionItem->SelectString(-1, strFind.GetBuffer(1));

	m_pOpInfo = pOpInfo;
	// The returned pointer should not be saved
	return pComboOptionItem;
}

void CConfigPlayerListCtrl::LoadCommandLists(CComboBox * pOptionList,COpInfo* pOpInfo)
{
    USES_CONVERSION;

	CFile commandFile;
	CFileException fileException;
	UCL	  uclNode;
	LPCTSTR filename = pOpInfo->GetUclPathname();

	pOptionList->ResetContent();

	if( commandFile.Open(filename, CFile::modeRead, &fileException) )
	{
		CStringT<char,StrTraitMFC<char> > uclString;
		commandFile.Read(uclString.GetBufferSetLength((int)commandFile.GetLength()), (UINT)commandFile.GetLength());
		uclString.ReleaseBuffer();

		if ( uclNode.Load(A2T(uclString)) != NULL )
		{
			UCL::CommandLists lists = uclNode.GetCommandLists();

			UCL::CommandLists::iterator it = lists.begin();
			for( ; it != lists.end(); ++(it))
			{
				int index = pOptionList->AddString(CString((*it)->GetName()));
				pOptionList->SetItemDataPtr(index, (*it));
			}

			if ( pOptionList->SelectString(0, pOpInfo->GetUclInstallSection()) == CB_ERR )
				pOptionList->SetCurSel(0);
		}
		else
		{
			TRACE( _T("Can't parse ucl.xml file.\n"));
		}
	}
	else
	{
		TRACE( _T("Can't open file %s, error = %u\n"), pOpInfo->GetUclPathname(), fileException.m_cause );
	}
}

void CConfigPlayerListCtrl::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO*)pNMHDR;
 	LV_ITEM *plvItem = &plvDispInfo->item;

	if( plvItem->iItem != -1 &&  // valid item
		plvItem->pszText )		// valid text
	{
		SetItemText( plvItem->iItem, plvItem->iSubItem, plvItem->pszText ,-1 , RGBDEF_VALID);
		m_pOpInfo->SetUclInstallSection(plvItem->pszText);
	}

	*pResult = 0;
}

CComboOptionItem::CComboOptionItem( int nItem, int nSubItem)
{
	m_nItem		= nItem;
	m_nSubItem	= nSubItem;

	m_bVK_ESCAPE = FALSE;
}

CComboOptionItem::~CComboOptionItem()
{
}

BEGIN_MESSAGE_MAP(CComboOptionItem, CComboBox)
	//{{AFX_MSG_MAP(CComboOptionItem)
	ON_WM_NCDESTROY()
	ON_WM_CHAR()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCloseup)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CComboOptionItem message handlers
BOOL CComboOptionItem::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if( pMsg->message == WM_KEYDOWN )	
	{		
		if(pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)	
		{
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);			
			return 1;
		}	
	}	
	
	return CComboBox::PreTranslateMessage(pMsg);
}

void CComboOptionItem::OnNcDestroy() 
{
	CComboBox::OnNcDestroy();
	
	// TODO: Add your message handler code here
	delete this;	
}

void CComboOptionItem::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	if(nChar == VK_ESCAPE || nChar == VK_RETURN)	
	{		
		if( nChar == VK_ESCAPE)
			m_bVK_ESCAPE = 1;
		GetParent()->SetFocus();		
		return;	
	}	
	
	CComboBox::OnChar(nChar, nRepCnt, nFlags);
}

void CComboOptionItem::OnKillFocus(CWnd* pNewWnd) 
{
	int nIndex = GetCurSel();

	CComboBox::OnKillFocus(pNewWnd);

	CString str;	
	GetWindowText(str);
	// Send Notification to parent of ListView ctrl	
	LV_DISPINFO lvDispinfo;
	lvDispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	lvDispinfo.hdr.idFrom = GetDlgCtrlID();//that's us
	lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
	lvDispinfo.item.mask = LVIF_TEXT | LVIF_PARAM;	
	lvDispinfo.item.iItem = m_nItem;
	lvDispinfo.item.iSubItem = m_nSubItem;
	lvDispinfo.item.pszText = m_bVK_ESCAPE ? NULL : LPTSTR((LPCTSTR)str);
	lvDispinfo.item.cchTextMax = str.GetLength();
	lvDispinfo.item.lParam = GetItemData(GetCurSel());
	if(nIndex!=CB_ERR)
		GetParent()->GetParent()->SendMessage(
			WM_NOTIFY,
			GetParent()->GetDlgCtrlID(),
			(LPARAM)&lvDispinfo);
	PostMessage(WM_CLOSE);
}

void CComboOptionItem::OnCloseup() 
{
	GetParent()->SetFocus();	
}

int CComboOptionItem::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CFont* font = GetParent()->GetFont();	
	SetFont(font);

	SetFocus();	
	
	return 0;
}

