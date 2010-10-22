/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/*********************************************************************
* HotProp Control, version 1.10 (January 14, 2004)
* Copyright (C) 2002-2004 Michal Mecinski.
*
* You may freely use and modify this code, but don't remove
* this copyright note.
*
* THERE IS NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, FOR
* THIS CODE. THE AUTHOR DOES NOT TAKE THE RESPONSIBILITY
* FOR ANY DAMAGE RESULTING FROM THE USE OF IT.
*
* E-mail: mimec@mimec.org
* WWW: http://www.mimec.org
********************************************************************/

#pragma once

#define HPPM_ENDPOP		(WM_USER+1703)	// bDone
#define HPPM_DRAWITEM	(WM_USER+1704)	// nItem, pDrawInfo


class CHPPopList : public CMiniFrameWnd
{
public:
	CHPPopList(CRect& rcRect, int nValue, BOOL bKey, INT_PTR nItems, BOOL bUnfold, BOOL bFlat,
		CFont* pFont, CWnd* pParent);

public:
	struct DrawInfo // used by HPPM_DRAWITEM
	{					
		CDC* m_pDC;
		CRect m_rcRect;
	};

protected:
	int m_nItems;
	int m_nValue;
	int m_nReturn;
	CWnd* m_pParent;
	CFont* m_pFont;
	int m_nHot;
	BOOL m_bCapture;
	BOOL m_bFlatStyle;
	int m_nHeight;
	BOOL m_bTop;
	BOOL m_bScroll;
	int m_nFirst;
	int m_nScrollDir;
	CWnd* m_pPrevWnd;

protected:
	void InvalidateItem(int nPos);
	void InvalidateArrows();
	void EnsureVisible(int nPos);
	void KeyScrollTo(int nPos);
	
protected:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnPaint();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnTimer(UINT nIDEvent);
	DECLARE_MESSAGE_MAP()
};
