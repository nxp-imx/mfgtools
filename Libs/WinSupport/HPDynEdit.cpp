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
#include "HPDynEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CHPDynEdit::CHPDynEdit(CRect& rcLoc, LPCTSTR lpszText, int nLimit, CPoint& ptAct,
				   int fFlags, CFont* pFont, CWnd* pParent)
{
	if (Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | fFlags,
		rcLoc, pParent, 1))
	{
		SetFont(pFont);
		SetWindowText(lpszText);
		SetLimitText(nLimit);
		m_pPrevWnd = SetFocus();
		int nPos = CharFromPos(ptAct - rcLoc.TopLeft());
		SetSel(nPos, nPos);
	}
	m_bDone = TRUE;
}


BEGIN_MESSAGE_MAP(CHPDynEdit, CEdit)
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillFocus)
END_MESSAGE_MAP()


void CHPDynEdit::OnKillFocus()
{
	GetOwner()->SendMessage(HPEM_ENDEDIT, m_bDone);
	DestroyWindow();
}

BOOL CHPDynEdit::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message==WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
			m_bDone = FALSE;
		case VK_RETURN:
			if (IsWindow(m_pPrevWnd->m_hWnd))
				m_pPrevWnd->SetFocus();
			else
				GetParent()->SetFocus();
			return TRUE;
		}
	}
	return CEdit::PreTranslateMessage(pMsg);
}

void CHPDynEdit::PostNcDestroy()
{
	delete this;
}
