/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// TreeDropTarget.cpp : implementation file
// For OLE Drag and Drop between tree controls
// Designed and developed by Vinayak Tadas
// vinayakt@aditi.com
// 

#include "stdafx.h"
#include "mm_callback.h"
#include "TreeDroptarget.h"
#include "StResGrpTreeCtrl.h"

#define RECT_BORDER	10

//HWND CTreeDropTarget::m_shWndTreeCtrl = NULL;

/////////////////////////////////////////////////////////////////////////////
// CTreeDropTarget

/********************************************************************
OnDragEnter()
	Called when the user drags the object in Tree control.
********************************************************************/
DROPEFFECT CTreeDropTarget::OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, 
	DWORD dwKeyState, CPoint point 
)
{
	
	DROPEFFECT dropeffectRet = DROPEFFECT_COPY;
	if ( (dwKeyState & MK_SHIFT) == MK_SHIFT)
		dropeffectRet = DROPEFFECT_MOVE;
	return dropeffectRet;
}

/********************************************************************
OnDragOver()
	Called when the user drags the object over Tree control.
********************************************************************/
		
DROPEFFECT CTreeDropTarget::OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, 
	DWORD dwKeyState, CPoint point )
{
	
	DROPEFFECT dropeffectRet = DROPEFFECT_COPY;
	UINT hitFlags = 0;
	if ( (dwKeyState & MK_SHIFT) == MK_SHIFT)
		dropeffectRet = DROPEFFECT_MOVE;
	// Expand and highlight the item under the mouse and 
	m_pDestTreeCtrl = (CStResGrpTreeCtrl *)pWnd;
	HTREEITEM hTItem = m_pDestTreeCtrl->HitTest(point, &hitFlags);
	if ( hTItem != NULL ) 
	{
		m_pDestTreeCtrl->Expand(hTItem, TVE_EXPAND);
		m_pDestTreeCtrl->SelectDropTarget(hTItem);
	}	
	
	// Scroll Tree control depending on mouse position
	CRect rectClient;
	pWnd->GetClientRect(&rectClient);
	pWnd->ClientToScreen(rectClient);
	pWnd->ClientToScreen(&point);
	int nScrollDir = -1;
	if ( point.y >= rectClient.bottom - RECT_BORDER)
		nScrollDir = SB_LINEDOWN;
	else
	if ( (point.y <= rectClient.top + RECT_BORDER) )
		nScrollDir = SB_LINEUP;

	
	if ( nScrollDir != -1 ) 
	{
		int nScrollPos = pWnd->GetScrollPos(SB_VERT);
		WPARAM wParam = MAKELONG(nScrollDir, nScrollPos);
		pWnd->SendMessage(WM_VSCROLL, wParam);
	}
	
	nScrollDir = -1;
	if ( point.x <= rectClient.left + RECT_BORDER )
		nScrollDir = SB_LINELEFT;
	else
	if ( point.x >= rectClient.right - RECT_BORDER)
		nScrollDir = SB_LINERIGHT;
	
	if ( nScrollDir != -1 ) 
	{
		int nScrollPos = pWnd->GetScrollPos(SB_VERT);
		WPARAM wParam = MAKELONG(nScrollDir, nScrollPos);
		pWnd->SendMessage(WM_HSCROLL, wParam);
	}
	
	return dropeffectRet;
}

/********************************************************************
OnDragLeave()
	Called when the user drags the object out of Tree control.
********************************************************************/
void CTreeDropTarget::OnDragLeave( CWnd* pWnd )
{
	// Remove Highlighting 
	m_pDestTreeCtrl = (CStResGrpTreeCtrl *)pWnd;
	m_pDestTreeCtrl->SendMessage(TVM_SELECTITEM, TVGN_DROPHILITE,0);
}



/********************************************************************
GetItemImages()
	Gets image indexes associated with a tree item
********************************************************************/
/*
void CTreeDropTarget::GetItemImages(HTREEITEM hSrcTItem, int &nSelItemImage, int &nNonSelItemImage)
{
	
	CImageList *pImageList = m_pSourceTreeCtrl->GetImageList(TVSIL_NORMAL);
	//If no image list is associated with the tree control, return
	if (pImageList == NULL )
	{
		nSelItemImage =0;
		nNonSelItemImage = 0;
	}
	else
	{
		// If no image list is associated with Destination tree control
		// Set the image list of source tree control
		if ( m_pDestTreeCtrl->GetImageList(TVSIL_NORMAL) == NULL )
		{
			m_pDestTreeCtrl->SetImageList(pImageList, TVSIL_NORMAL);
		}
		// Get the image indexes
		m_pSourceTreeCtrl->GetItemImage(hSrcTItem, nSelItemImage, nNonSelItemImage);
	}
}
*/
