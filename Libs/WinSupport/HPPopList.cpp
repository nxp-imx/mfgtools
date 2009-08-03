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

#include "stdafx.h"
#include "HPPopList.h"
#include "HotPropCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CHPPopList::CHPPopList(CRect& rcRect, int nValue, BOOL bKey, INT_PTR nItems, BOOL bUnfold, BOOL bFlat,
	CFont* pFont, CWnd* pParent)
{
	LPCTSTR lpszClass = AfxRegisterWndClass(0,
		LoadCursor(NULL, IDC_ARROW), NULL, NULL);

	LONG nBottom = rcRect.bottom;
	LONG nTop = rcRect.top;
	int nSize = (int)nItems * 18 + 6;

	CRect rcWork;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);

	if (nSize > rcWork.Height())
	{
		bUnfold = FALSE;
		m_bScroll = TRUE;
		m_nHeight = (rcWork.Height() - 6 - 2*10) / 18;
		nSize = m_nHeight * 18 + 6 + 2*10;
	}
	else
		m_bScroll = FALSE;

	int nInit = bUnfold ? 18 + 6 : nSize;

	// calculate best position and orientation
	if (nBottom + nSize <= rcWork.bottom)
	{
		rcRect.top = nBottom;
		rcRect.bottom = nBottom + nInit;
		m_bTop = FALSE;
	}
	else if (nTop - nSize >= rcWork.top)
	{
		rcRect.bottom = nTop;
		rcRect.top = nTop - nInit;
		m_bTop = TRUE;
	}
	else if (nTop - rcWork.top < rcWork.bottom - nBottom)
	{
		rcRect.top = rcWork.bottom - nSize;
		rcRect.bottom = rcRect.top + nInit;
		m_bTop = FALSE;
	}
	else
	{
		rcRect.bottom = rcWork.top + nSize;
		rcRect.top = rcRect.bottom - nInit;
		m_bTop = TRUE;
	}

	m_pPrevWnd = GetFocus();

	// use syncactive to prevent the main window from being deactivated
	Create(lpszClass, _T(""), WS_POPUP|WS_VISIBLE|MFS_SYNCACTIVE, rcRect, pParent);

	m_pParent = pParent;
	m_pFont = pFont;

	m_nItems = (int)nItems;
	m_nValue = nValue;
	m_nHot = bKey ? nValue : -1;	// hilite the current item if called from keyboard
	m_nReturn = -1;
	m_bCapture = FALSE;
	m_nScrollDir = 0;

	if (bUnfold && m_nItems>1)
	{
		m_nHeight = 1;	// start unfolding
		SetTimer(1, 120 / m_nItems, NULL);
	}
	else if (!m_bScroll)
		m_nHeight = m_nItems;

	m_nFirst = 0;
	EnsureVisible(nValue);

	m_bFlatStyle = bFlat;
}


BEGIN_MESSAGE_MAP(CHPPopList, CMiniFrameWnd)
	ON_WM_KILLFOCUS()
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_CAPTURECHANGED()
	ON_WM_TIMER()
END_MESSAGE_MAP()


void CHPPopList::OnKillFocus(CWnd* pNewWnd)
{
	m_pParent->SendMessage(HPPM_ENDPOP, m_nReturn);
	DestroyWindow();
}

void CHPPopList::OnPaint()
{
	CPaintDC dc(this);

	CRect rcClient;
	GetClientRect(&rcClient);

	COLORREF crBack, crHalf;
	
	if (m_bFlatStyle)
	{
		crBack = GetSysColor(COLOR_MENU);
		crHalf = CHotPropCtrl::LightenColor(GetSysColor(COLOR_HIGHLIGHT), 0.7);
	}
	else
	{
		crBack = GetSysColor(COLOR_BTNFACE);
		crHalf = CHotPropCtrl::LightenColor(crBack, 0.5);
	}

	dc.FillSolidRect(&rcClient, crBack);

	if (m_bFlatStyle)
	{
		COLORREF crFrame = GetSysColor(COLOR_BTNSHADOW);
		dc.Draw3dRect(&rcClient, crFrame, crFrame);	// flat border
	}
	else
		dc.DrawEdge(&rcClient, BDR_RAISEDINNER, BF_RECT);

	dc.SetBkMode(TRANSPARENT);
	dc.SelectObject(m_pFont);

	// calculate visible part
	int nFirst, nLast;
	if (m_bScroll)
	{
		nFirst = m_nFirst;
		nLast = m_nFirst + m_nHeight;
	}
	else
	{
		nFirst = m_bTop ? 0 : m_nItems - m_nHeight;
		nLast = m_bTop ? m_nHeight : m_nItems;
	}

	for (int i=nFirst; i<nLast; i++)
	{
		CRect rcItem = rcClient;
		rcItem.top = 3 + (i-nFirst)*18;
		rcItem.bottom = rcItem.top + 18;

		if (m_bScroll)
			rcItem.OffsetRect(0, 10);

		rcItem.DeflateRect(3, 0);
		if (m_nHot==i && (m_bScroll || m_nHeight==m_nItems))
		{
			dc.FillSolidRect(&rcItem, crHalf);
			if (m_bFlatStyle)
			{
				COLORREF crHigh = GetSysColor(COLOR_HIGHLIGHT);
				dc.Draw3dRect(&rcItem, crHigh, crHigh);
			}
			else
				dc.DrawEdge(&rcItem, BDR_SUNKENOUTER, BF_RECT);
		}

		rcItem.DeflateRect(14, 1, 1, 1);

		DrawInfo dinfo;
		dinfo.m_pDC = &dc;
		dinfo.m_rcRect = rcItem;
		m_pParent->SendMessage(HPPM_DRAWITEM, i, (LPARAM)&dinfo);

		if (m_nValue==i)	// draw a bullet to the left of current item
		{
			CFont font;
			font.CreatePointFont(100, _T("Marlett"));
			dc.SelectObject(&font);

			if (m_nHot==i)
				dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHT));

			rcItem.left = 3;
			dc.DrawText(_T("i"), 1, &rcItem, DT_SINGLELINE | DT_VCENTER);

			if (m_nHot==i)
				dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
			dc.SelectObject(m_pFont);
		}
	}

	// draw up/down arrows
	if (m_bScroll)
	{
		CRect rcUp = rcClient;
		rcUp.DeflateRect(3, 3);
		CRect rcDown = rcUp;
		rcUp.bottom = rcUp.top + 10;
		rcDown.top = rcDown.bottom - 10;

		if (m_nScrollDir < 0)
		{
			dc.FillSolidRect(&rcUp, crHalf);
			if (m_bFlatStyle)
			{
				COLORREF crHigh = GetSysColor(COLOR_HIGHLIGHT);
				dc.Draw3dRect(&rcUp, crHigh, crHigh);
			}
			else
				dc.DrawEdge(&rcUp, BDR_SUNKENOUTER, BF_RECT);
		}
		else if (m_nScrollDir > 0)
		{
			dc.FillSolidRect(&rcDown, crHalf);
			if (m_bFlatStyle)
			{
				COLORREF crHigh = GetSysColor(COLOR_HIGHLIGHT);
				dc.Draw3dRect(&rcDown, crHigh, crHigh);
			}
			else
				dc.DrawEdge(&rcDown, BDR_SUNKENOUTER, BF_RECT);
		}

		CFont font;
		font.CreatePointFont(100, _T("Marlett"));
		dc.SelectObject(&font);

		dc.SetTextColor(GetSysColor(m_nScrollDir < 0 ? COLOR_HIGHLIGHT : COLOR_WINDOWTEXT));
		dc.DrawText(_T("5"), 1, &rcUp, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

		dc.SetTextColor(GetSysColor(m_nScrollDir > 0 ? COLOR_HIGHLIGHT : COLOR_WINDOWTEXT));
		dc.DrawText(_T("6"), 1, &rcDown, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

		if (m_nScrollDir)
			dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
		dc.SelectObject(m_pFont);
	}
}
				

void CHPPopList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
	case VK_RETURN:
		m_nReturn = m_nHot;
	case VK_ESCAPE:
		if (IsWindow(m_pPrevWnd->m_hWnd))
			m_pPrevWnd->SetFocus();
		else
			GetParent()->SetFocus();
		break;

	case VK_UP:
		KeyScrollTo(m_nHot - 1);
		break;

	case VK_DOWN:
		KeyScrollTo(m_nHot + 1);
		break;	

	case VK_PRIOR:
		if (!m_bScroll)
			KeyScrollTo(0);
		else if (m_nHot == 0)
			KeyScrollTo(m_nItems - 1);
		else if (m_nHot == m_nFirst)
			KeyScrollTo(max(0, m_nHot - m_nHeight + 1));
		else
			KeyScrollTo(m_nFirst);
		break;

	case VK_NEXT:
		InvalidateItem(m_nHot);
		if (!m_bScroll)
			KeyScrollTo(m_nItems - 1);
		else if (m_nHot == m_nItems - 1)
			KeyScrollTo(0);
		else if (m_nHot == m_nFirst + m_nHeight - 1)
			KeyScrollTo(min(m_nItems - 1, m_nHot + m_nHeight - 1));
		else
			KeyScrollTo(m_nFirst + m_nHeight - 1);
		break;

	case VK_HOME:
		KeyScrollTo(0);
		break;

	case VK_END:
		KeyScrollTo(m_nItems - 1);
		break;
	}
}


void CHPPopList::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect rcClient;
	GetClientRect(&rcClient);
	rcClient.DeflateRect(3, 3);

	if (!m_bCapture)	// similar to the CHotPropCtrl
	{
		SetCapture();
		m_bCapture = TRUE;
	}
	else
	{
		CPoint ptScr = point;
		ClientToScreen(&ptScr);
		if (WindowFromPoint(ptScr)!=this)
		{
			ReleaseCapture();
			return;
		}
	}

	int nItem = -1;
	int nLastDir = m_nScrollDir;
	m_nScrollDir = 0;

	if (rcClient.PtInRect(point))
	{
		if (m_bScroll)
		{
			if (point.y < rcClient.top + 10)
			{
				m_nScrollDir = -1;
				if (nLastDir != -1)
					SetTimer(2, 50, NULL);
			}
			else if (point.y >= rcClient.bottom - 10)
			{
				m_nScrollDir = 1;
				if (nLastDir != 1)
					SetTimer(2, 50, NULL);
			}
			else
				nItem = (point.y - 3 - 10)/18 + m_nFirst;
		}
		else
			nItem = (point.y - 3)/18;
	}

	if (nItem != m_nHot)
	{
		InvalidateItem(m_nHot);
		InvalidateItem(nItem);
		EnsureVisible(nItem);
		m_nHot = nItem;
	}

	if (m_nScrollDir != nLastDir)
	{
		CRect rcUp = rcClient;
		rcUp.bottom = rcUp.top + 10;
		CRect rcDown = rcClient;
		rcDown.top = rcDown.bottom - 10;
		InvalidateRect(&rcUp, FALSE);
		InvalidateRect(&rcDown, FALSE);
	}
}


void CHPPopList::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nHot>=0)
	{
		m_nReturn = m_nHot;	// accept choice
		if (IsWindow(m_pPrevWnd->m_hWnd))
			m_pPrevWnd->SetFocus();
		else
			GetParent()->SetFocus();
	}
}


void CHPPopList::InvalidateItem(int nPos)
{
	if (m_bScroll)
		nPos -= m_nFirst;

	if (nPos >= 0 && nPos < m_nHeight)
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		rcClient.top = 3 + nPos*18;
		rcClient.bottom = rcClient.top + 18;
		if (m_bScroll)
			rcClient.OffsetRect(0, 10);
		InvalidateRect(&rcClient, FALSE);
	}
}		

void CHPPopList::InvalidateArrows()
{
	CRect rcUp;
	GetClientRect(&rcUp);
	rcUp.DeflateRect(3, 3);
	CRect rcDown = rcUp;

	rcUp.bottom = rcUp.top + 10;
	rcDown.top = rcDown.bottom - 10;

	InvalidateRect(&rcUp, FALSE);
	InvalidateRect(&rcDown, FALSE);
}

void CHPPopList::EnsureVisible(int nPos)
{
	if (nPos>=0 && m_bScroll)
	{
		int nPrev = m_nFirst;

		if (nPos < m_nFirst)
			m_nFirst = nPos;
		else if (nPos >= m_nFirst + m_nHeight)
			m_nFirst = nPos - m_nHeight + 1;

		if (m_nFirst != nPrev)
		{
			CRect rcScroll;
			GetClientRect(&rcScroll);
			rcScroll.DeflateRect(3, m_bScroll ? 3 + 10 : 3);
			ScrollWindow(0, (nPrev - m_nFirst)*18, &rcScroll, &rcScroll);
		}
	}
}

void CHPPopList::KeyScrollTo(int nPos)
{
	if (nPos < 0)
		nPos = m_nItems - 1;
	else if (nPos >= m_nItems)
		nPos = 0;

	if (nPos != m_nHot)
	{
		InvalidateItem(m_nHot);
		InvalidateItem(nPos);

		m_nHot = nPos;

		if (m_nScrollDir)
		{
			m_nScrollDir = 0;
			InvalidateArrows();
		}

		EnsureVisible(m_nHot);
	}
}

void CHPPopList::OnCaptureChanged(CWnd *pWnd)
{
	m_bCapture = FALSE;

	if (m_nHot>=0)
	{
		InvalidateItem(m_nHot);
		m_nHot = -1;
	}
}

void CHPPopList::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == 1)
	{
		if (m_nHeight >= m_nItems)	// unfolding done
		{
			if (m_nHot>=0)
				InvalidateItem(m_nHot);
			KillTimer(1);
			return;
		}
		m_nHeight++;

		CRect rcRect;	// expand window
		GetWindowRect(&rcRect);
		if (m_bTop)
			rcRect.top -= 18;
		else
			rcRect.bottom += 18;
		MoveWindow(&rcRect, FALSE);

		// scroll and draw the new item
		rcRect -= rcRect.TopLeft();
		if (m_bTop)
		{
			ScrollWindow(0, -18);
			rcRect.top = rcRect.bottom - 20;
		}
		else
		{
			ScrollWindow(0, 18);
			rcRect.bottom = 20;
		}
		InvalidateRect(&rcRect, FALSE);
	}
	else if (nIDEvent == 2)
	{
		if (!m_nScrollDir)
		{
			KillTimer(2);
			return;
		}

		if (m_nScrollDir < 0 && m_nFirst > 0)
		{
			m_nFirst--;

			CRect rcScroll;
			GetClientRect(&rcScroll);
			rcScroll.DeflateRect(3, 3 + 10);
			ScrollWindow(0, 18, &rcScroll, &rcScroll);
		}
		else if (m_nScrollDir > 0 && m_nFirst < (m_nItems - m_nHeight))
		{
			m_nFirst++;

			CRect rcScroll;
			GetClientRect(&rcScroll);
			rcScroll.DeflateRect(3, 3 + 10);
			ScrollWindow(0, -18, &rcScroll, &rcScroll);
		}
	}
}
		
