// PPNumEdit.cpp : implementation file
//

#include "stdafx.h"
#include "PPNumEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPPNumEdit

CPPNumEdit::CPPNumEdit()
{
	m_bValidValue = TRUE;
	m_bNotificate = TRUE;

	SetDefaultColors(FALSE);
}

CPPNumEdit::~CPPNumEdit()
{
}


BEGIN_MESSAGE_MAP(CPPNumEdit, CEdit)
	//{{AFX_MSG_MAP(CPPNumEdit)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_CONTROL_REFLECT(EN_UPDATE, OnUpdate)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPPNumEdit message handlers

HBRUSH CPPNumEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	if (!m_bValidValue)
	{
		pDC->SetBkColor(m_crNotValidBk);
		pDC->SetTextColor(m_crNotValidFg);
		return m_brNotValidBk;
	}
	pDC->SetBkColor(m_crValidBk);
	pDC->SetTextColor(m_crValidFg);
	return m_brValidBk;
}

BOOL CPPNumEdit::PreCreateWindow(CREATESTRUCT& cs) 
{
	TRACE(_T("CPPNumEdit::PreCreateWindow()\n"));

	cs.style &= ~ES_LOWERCASE;
	cs.style |= ES_UPPERCASE;
	
	return CEdit::PreCreateWindow(cs);
}

BOOL CPPNumEdit::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class

	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			SendNotify(PPNUMEDIT_ENTER_DATA);
			return TRUE;
		case VK_ESCAPE:
			SendNotify(PPNUMEDIT_CANCEL_DATA);
			return TRUE;
		case VK_TAB: 
			if (::GetKeyState(VK_SHIFT) < 0)
				SendNotify(PPNUMEDIT_MOVE_PREV_FIELD);
			else
				SendNotify(PPNUMEDIT_MOVE_NEXT_FIELD);
			return TRUE;
		}

		if (::GetKeyState(VK_CONTROL) < 0)
		{
			switch (pMsg->wParam) 
			{
			case 'H':
				SendNotify(PPNUMEDIT_HOTKEY_HEX);
				return TRUE;
			case 'D':
				SendNotify(PPNUMEDIT_HOTKEY_DEC);
				return TRUE;
			case 'B':
				SendNotify(PPNUMEDIT_HOTKEY_BIN);
				return TRUE;
			case 'O':
				SendNotify(PPNUMEDIT_HOTKEY_OCT);
				return TRUE;
			case 'A':
				SendNotify(PPNUMEDIT_HOTKEY_ASCII);
				return TRUE;
			}
		}
	}

	return CEdit::PreTranslateMessage(pMsg);
}

BOOL CPPNumEdit::GetNotify()
{
	return m_bNotificate;
}

LRESULT CPPNumEdit::SendNotify(UINT uNotifyCode)
{
	TRACE(_T("CPPNumEdit::SendNotify()\t%X\n"), uNotifyCode);

	if (uNotifyCode >= PPNUMEDIT_MAX_EVENTS)
		return 0;

	//If the notification code is not CANCEL or the editing value is not validate
	//then don't send the notification
	if ((uNotifyCode != PPNUMEDIT_CANCEL_DATA) && (!IsValidate()))
		return 0;

	// Make sure this is a valid window
	if (!IsWindow(GetSafeHwnd()))
		return 0;

	// See if the user wants to be notified
	if (GetNotify() == FALSE)
		return 0;

	NM_PPNUM_EDIT lpnm;
	
	lpnm.iValue		  = GetValue();
	lpnm.iEvent		  = uNotifyCode;
	lpnm.hdr.hwndFrom = m_hWnd;
    lpnm.hdr.idFrom   = GetDlgCtrlID();

	if (uNotifyCode == PPNUMEDIT_CANCEL_DATA)
		lpnm.hdr.code = UDM_PPNUMEDIT_CANCEL;
	else if (uNotifyCode == PPNUMEDIT_ENTER_DATA)
		lpnm.hdr.code = UDM_PPNUMEDIT_ENTER;
	else if (uNotifyCode < PPNUMEDIT_HOTKEY_HEX)
		lpnm.hdr.code = UDM_PPNUMEDIT_MOVE;
	else lpnm.hdr.code = UDM_PPNUMEDIT_HOTKEY;


	CWnd *pOwner = GetOwner();
    if (pOwner && IsWindow(pOwner->m_hWnd))
        return pOwner->SendMessage(WM_NOTIFY, lpnm.hdr.idFrom, (LPARAM)&lpnm);
    else
        return 0;
}

BOOL CPPNumEdit::DestroyWindow() 
{
	return CEdit::DestroyWindow();
}

void CPPNumEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	TRACE(_T("CPPNumEdit::OnChar(\'%c\' (%d), %d, 0x%04x)\n"), 
		nChar, nChar, nRepCnt, nFlags);

	if (nChar >= 0x20)
	{
		if (m_strText.GetLength() >= (int)m_nCharsInValue)
		{
			int nSelBegin = 0;
			int nSelEnd = 0;
			GetSel(nSelBegin, nSelEnd);
			if (nSelBegin == nSelEnd)
				return;
		}
		
		if ((nChar >= (TCHAR)'a') && (nChar <= (TCHAR)'f'))
			nChar -= ('a' - 'A');
		switch (m_nIndexValue)
		{
		case PPNUM_VALUE_DEC:
			if ((nChar < (TCHAR)'0') || (nChar > (TCHAR)'9'))
				return;
			break;
		case PPNUM_VALUE_HEX:
			if ((nChar < (TCHAR)'0') || (nChar > (TCHAR)'F'))
				return;
			if ((nChar > (TCHAR)'9') && (nChar < (TCHAR)'A'))
				return;
			break;
		case PPNUM_VALUE_BIN:
			if ((nChar < (TCHAR)'0') || (nChar > (TCHAR)'1'))
				return;
			break;
		case PPNUM_VALUE_OCT:
			if ((nChar < (TCHAR)'0') || (nChar > (TCHAR)'7'))
				return;
			break;
		case PPNUM_VALUE_ASCII:
			break;
		}
	}
	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

void CPPNumEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	TRACE(_T("CPPNumEdit::OnKeyDown(\'%c\' (0x%02x), %d, 0x%04x)\n"), nChar, nChar, nRepCnt, nFlags);

	int nStart = 0;
	int nEnd = 0;
	GetSel(nStart, nEnd);
	TRACE (_T("%d - %d\n"), nStart, nEnd);

	switch(nChar) 
	{
	case VK_DOWN:
		SendNotify(PPNUMEDIT_MOVE_DOWN);
		return;
	case VK_UP:
		SendNotify(PPNUMEDIT_MOVE_UP);
		return;
	case VK_RIGHT:
		if (((nStart >= m_strText.GetLength()) && (nEnd == nStart)) || (::GetKeyState(VK_CONTROL) < 0))
		{
			SendNotify(PPNUMEDIT_MOVE_RIGHT);
			return;
		}
		break;
	case VK_LEFT:
		if (((nStart == 0) && (nEnd == nStart)) || (::GetKeyState(VK_CONTROL) < 0))
		{
			SendNotify(PPNUMEDIT_MOVE_LEFT);
			return;
		}
		break;
	case VK_PRIOR:
		SendNotify(PPNUMEDIT_MOVE_PAGE_UP);
		return;
	case VK_NEXT:
		SendNotify(PPNUMEDIT_MOVE_PAGE_DOWN);
		return;
	case VK_HOME:
		if (::GetKeyState(VK_CONTROL) < 0)
		{
			SendNotify(PPNUMEDIT_MOVE_BEGIN_LINE);
			return;
		}
		break;
	case VK_END:
		if (::GetKeyState(VK_CONTROL) < 0)
		{
			SendNotify(PPNUMEDIT_MOVE_END_LINE);
			return;
		}
		break;
	case VK_TAB: 
		if (::GetKeyState(VK_SHIFT) < 0)
			SendNotify(PPNUMEDIT_MOVE_PREV_FIELD);
		else
			SendNotify(PPNUMEDIT_MOVE_NEXT_FIELD);
		return;
	}
	
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CPPNumEdit::OnUpdate() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CEdit::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	TRACE(_T("CPPNumEdit::OnUpdate()\n"));
	
	m_bValidValue = IsValidate();
}

BOOL CPPNumEdit::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	return CEdit::OnEraseBkgnd(pDC);
}

void CPPNumEdit::SetValue(UINT nValue, 
						  UINT nIndexValue /* = PPNUM_VALUE_HEX */, 
						  UINT nTypeValue /* = PPNUM_VALUE_BYTE */, 
						  UINT nMin /* = 0 */, 
						  UINT nMax /* = 0xFF */)
{
	TRACE(_T("CPPNumEdit::SetValue()\n"));
	//Sets limits of the value
	m_nIndexValue = nIndexValue;

	switch (nTypeValue)
	{
	case PPNUM_VALUE_CUSTOM:
		if (nMax < nMin)
		{
			m_nMaxLimit = nMin;
			m_nMinLimit = nMax;
		}
		else
		{
			m_nMinLimit = nMin;
			m_nMaxLimit = nMax;
		}
		break;
	case PPNUM_VALUE_BYTE:
		m_nMinLimit = 0x00;
		m_nMaxLimit = 0xFF;
		break;
	case PPNUM_VALUE_WORD:
		m_nMinLimit = 0x0000;
		m_nMaxLimit = 0xFFFF;
		break;
	}
	
	//Check the range of the value
	if ((nValue < m_nMinLimit) || (nValue > m_nMaxLimit))
		nValue = m_nMinLimit;
	m_nValue = nValue;

	//Convert the value to the string
	nValue = m_nMaxLimit;
	m_nCharsInValue = 0;
	CString str;
	switch (nIndexValue)
	{
	case PPNUM_VALUE_DEC:
		m_strText.Format(_T("%d"), m_nValue);
		while (nValue)
		{
			m_nCharsInValue ++;
			nValue /= 10;
		}
		break;
	case PPNUM_VALUE_HEX:
		ModifyStyle (0, ES_UPPERCASE);
		while (nValue)
		{
			m_nCharsInValue ++;
			nValue >>= 4;
		}
		str.Format(_T("%%.%dX"), m_nCharsInValue);
		m_strText.Format(str, m_nValue);
		break;
	case PPNUM_VALUE_BIN:
		while (nValue)
		{
			m_nCharsInValue ++;
			nValue >>= 1;
		}
		nMin = m_nValue;
		m_strText = "";
		for (nValue = 0; nValue < m_nCharsInValue; nValue ++)
		{
			if (!(nValue & 0x7) && (nValue))
				m_strText = _T("-") + m_strText;
			
			m_strText = ((nMin & 0x1) ? _T("1") : _T("0")) + m_strText;
			nMin >>= 1;
		}
		break;
	case PPNUM_VALUE_OCT:
		m_strText.Format(_T("%o"), m_nValue);
		while (nValue)
		{
			m_nCharsInValue ++;
			nValue /= 8;
		}
		break;
	case PPNUM_VALUE_ASCII:
		ModifyStyle (ES_UPPERCASE, 0);
		m_nCharsInValue = 1;
		m_strText = (m_nValue > 0x1F) ? (TCHAR)m_nValue : (TCHAR)'.';
		break;
	}

	m_strOriginal = m_strText;
	SetWindowText(m_strText);
}

UINT CPPNumEdit::GetValue()
{
	TRACE(_T("CPPNumEdit::GetValue()\n"));

	GetWindowText(m_strText);
	
	UINT nValue = 0;
	switch (m_nIndexValue)
	{
	case PPNUM_VALUE_DEC:
		nValue = _tcstoul (m_strText, 0, 10);
		break;
	case PPNUM_VALUE_HEX:
		nValue = _tcstoul (m_strText, 0, 16);
		break;
	case PPNUM_VALUE_BIN:
		nValue = _tcstoul (m_strText, 0, 2);
		break;
	case PPNUM_VALUE_OCT:
		nValue = _tcstoul (m_strText, 0, 8);
		break;
	case PPNUM_VALUE_ASCII:
		if (m_strText.GetLength())
		{
			nValue = LOBYTE(m_strText.GetAt(0));
		}
		else
			nValue = 0;
		break;
	}

	return nValue;
}

BOOL CPPNumEdit::IsChanged()
{
	TRACE(_T("CPPNumEdit::IsChanged()\n"));

	return (m_strText.Compare(m_strOriginal));
}

BOOL CPPNumEdit::IsValidate()
{
	TRACE(_T("CPPNumEdit::IsValidate()\n"));
	UINT nValue = GetValue();

	return ((nValue >= m_nMinLimit) && (nValue <= m_nMaxLimit));
}

void CPPNumEdit::SetDefaultColors(BOOL bRedraw /* = TRUE */)
{
	TRACE(_T("CPPNumEdit::SetDefaultColors()\n"));

	SetColor(PPNUM_COLOR_VALID_FG, ::GetSysColor(COLOR_WINDOWTEXT), FALSE);
	SetColor(PPNUM_COLOR_VALID_BK, ::GetSysColor(COLOR_WINDOW), FALSE);
	SetColor(PPNUM_COLOR_NOT_VALID_FG, RGB (255, 255, 255), FALSE);
	SetColor(PPNUM_COLOR_NOT_VALID_BK, RGB (255, 0, 0), bRedraw);
}

void CPPNumEdit::SetColor(UINT nIndex, COLORREF crColor, BOOL bRedraw /* = TRUE */)
{
	TRACE(_T("CPPNumEdit::SetColor()\tIndex = %d\n"), nIndex);

	switch (nIndex)
	{
	case PPNUM_COLOR_VALID_FG:
		m_crValidFg = crColor;
		break;
	case PPNUM_COLOR_VALID_BK:
		m_crValidBk = crColor;
		m_brValidBk.DeleteObject();
		m_brValidBk.CreateSolidBrush(crColor);
		break;
	case PPNUM_COLOR_NOT_VALID_FG:
		m_crNotValidFg = crColor;
		break;
	case PPNUM_COLOR_NOT_VALID_BK:
		m_crNotValidBk = crColor;
		m_brNotValidBk.DeleteObject();
		m_brNotValidBk.CreateSolidBrush(crColor);
		break;
	}

	if (bRedraw)
		Invalidate();
}
