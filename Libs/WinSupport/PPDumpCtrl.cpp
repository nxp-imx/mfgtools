// PPDumpCtrl.cpp : implementation file for the CPPDumpCtrl control class
// written by Eugene Pustovoyt
//

// History
/*
	07 Aug 2002 - First release. Version 1.0
-----------------------------------------------------------------			
	14 Aug 2002:  Release version 1.1
		FIX: Fixed error of the thumbtrack messages when the buffer large 
		     then 32K (thanks to Bill Morrison - Rosinante Software)
		ADD: Support tooltip 
		ADD: New style PPDUMP_DATA_LOW_HIGH
		ADD: New message UDM_PPDUMPCTRL_SELECTION and new structure NM_PPDUMP_SEL
		ADD: Support mouse wheel (thanks to Darren Schroeder)
		FIX: Other minor error
-----------------------------------------------------------------
    19 Aug 2002: Release version 1.2
	    ADD: Added new formating chars to the format string of the tooltip text 
		ADD: Added function SetSpecialCharView and GetTooltip
		ADD: Two functions SetTrackMouseMove, IsTrackMouseMove and new style
			 PPDUMP_TRACK_MOUSE_MOVE
		FIX: Leak memory with GetDC()
		UPD: Now the control based on CWnd instead CEdit
*/
#include "stdafx.h"
#include "PPDumpCtrl.h"
#include "memdc.h"
//clw#include "..\STMP EditView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define nIdEdit	1
const CString m_csNameFields [] = {_T("Addr"), _T("Hex"), _T("Dec"), _T("Bin"), _T("Oct"), _T("Ascii"), _T("Sector"), _T("Block")};

/////////////////////////////////////////////////////////////////////////////
// CPPDumpCtrl

CPPDumpCtrl::CPPDumpCtrl()
: m_nDataEndian(LITTLE_ENDIAN)
, m_nDataWordSize(1)
{
	RegisterWindowClass();
}

CPPDumpCtrl::~CPPDumpCtrl()
{
	if (m_hMenu)	
		::DestroyMenu(m_hMenu);
	
	KillEdit();
}

BEGIN_MESSAGE_MAP(CPPDumpCtrl, CWnd)
	//{{AFX_MSG_MAP(CPPDumpCtrl)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_SIZE()
	ON_WM_ENABLE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_NOTIFY (UDM_PPNUMEDIT_ENTER, nIdEdit, NotifyEditEnter)
	ON_NOTIFY (UDM_PPNUMEDIT_CANCEL, nIdEdit, NotifyEditCancel)
	ON_NOTIFY (UDM_PPNUMEDIT_MOVE, nIdEdit, NotifyEditMove)
	ON_NOTIFY (UDM_PPNUMEDIT_HOTKEY, nIdEdit, NotifyEditHotKeys)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPPDumpCtrl message handlers

// Register the window class if it has not already been registered.
BOOL CPPDumpCtrl::RegisterWindowClass()
{
    WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if (!(::GetClassInfo(hInst, PPDUMPCTRL_CLASSNAME, &wndcls)))
    {
        // otherwise we need to register a new class
        wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc      = ::DefWindowProc;
        wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
        wndcls.hInstance        = hInst;
        wndcls.hIcon            = NULL;
        wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndcls.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wndcls.lpszMenuName     = NULL;
        wndcls.lpszClassName    = PPDUMPCTRL_CLASSNAME;

        if (!AfxRegisterClass(&wndcls))
        {
            AfxThrowResourceException();
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CPPDumpCtrl::Create(const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwStyle /* = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE */) 
{
	TRACE(_T("CPPDumpCtrl::Create()\n"));
	ASSERT(pParentWnd->GetSafeHwnd());

    if (!CWnd::Create(PPDUMPCTRL_CLASSNAME, NULL, dwStyle, rect, pParentWnd, nID))
        return FALSE;

//    m_hParentWnd = pParentWnd->GetSafeHwnd();
	return TRUE;
}

void CPPDumpCtrl::OnEnable(BOOL bEnable) 
{
	CWnd::OnEnable(bEnable);
	
	Invalidate(FALSE);
}

BOOL CPPDumpCtrl::Initialise()
{
	m_pNewData = NULL;
	m_pOldData = NULL;
	m_pEdit = NULL;
    m_pDoc = NULL;

	m_hMenu = NULL;
	m_hParentWnd = NULL;

	m_nLengthData = 1;
	m_nRealLengthData = 0;
	m_nOffsetAddress = 0;
	m_nBeginAddress = 0;

	m_nCurArea = -1;
	m_nCurrentAddr = -1;
	m_nEditedArea = -1;
	m_nEditedAddress = -1;
	m_nAddressToolTip = -1;
	SetSelectRange(0, -1, FALSE);

	m_rLastTrackRect.SetRectEmpty();
//	SetAddressWordSize(32);
    SetDataWordSize(1);
	SetCharsInData();

	m_bMouseOverCtrl = FALSE;
	m_bFocused = FALSE;
	m_bPressedLButton = FALSE;

	// No tooltip created
	m_pToolTip.m_hWnd = NULL;
	m_sFormatToolTip = "";
	m_chSpecCharView = NULL;

	SetDefaultStyles(FALSE);
	SetDefaultFont(FALSE);
	SetDefaultColors(FALSE);

	m_crDisableFg = ::GetSysColor(COLOR_BTNSHADOW);
	m_crDisableBk = ::GetSysColor(COLOR_BTNFACE);

	return TRUE;
}

BOOL CPPDumpCtrl::PreCreateWindow(CREATESTRUCT& cs) 
{
	TRACE(_T("CPPDumpCtrl::PreCreateWindow()\n"));	
	
	if(!CWnd::PreCreateWindow(cs)) 
		return FALSE;
	
//	cs.style |= WS_VSCROLL | WS_HSCROLL;
	
	return TRUE;
}

void CPPDumpCtrl::PreSubclassWindow() 
{
	TRACE(_T("CPPDumpCtrl::PreSubclassWindow()\n"));

	CWnd::PreSubclassWindow();

	Initialise();
}

BOOL CPPDumpCtrl::PreTranslateMessage(MSG* pMsg) 
{	
	InitToolTip();
	LRESULT ret;
	
	switch (pMsg->message)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_VSCROLL:
	case WM_HSCROLL:
		if (m_pToolTip.m_hWnd != NULL)
			m_pToolTip.Activate(FALSE);
		m_nAddressToolTip = -1;
		break;
	}
	m_pToolTip.RelayEvent(pMsg);

	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		if (::GetKeyState(VK_CONTROL) < 0)
		{
			switch (pMsg->wParam)
			{
			case 'H':
				HandleHotkeys(PPDUMP_HOTKEY_HEX);
				return TRUE;
			case 'D':
				HandleHotkeys(PPDUMP_HOTKEY_DEC);
				return TRUE;
			case 'B':
				HandleHotkeys(PPDUMP_HOTKEY_BIN);
				return TRUE;
			case 'O':
				HandleHotkeys(PPDUMP_HOTKEY_OCT);
				return TRUE;
			case 'A':
				HandleHotkeys(PPDUMP_HOTKEY_ASCII);
				return TRUE;
			}
		}
		else if (pMsg->wParam == VK_RETURN)
		{
			int nArea = GetNextField(0x11);
			if ((m_pEdit == NULL) && (nArea >= 0))
			{
				m_nEditedAddress = m_nCaretAddrBegin;
				m_nEditedArea = nArea;
				SetEditedValue();
				return TRUE;
			}
		}
	}
	else if (pMsg->message == WM_RBUTTONDOWN)
	{
		if (m_hMenu && !m_rLastTrackRect.IsRectEmpty() && (m_nCurArea > 0x11))
		{
			HMENU hSubMenu = ::GetSubMenu(m_hMenu, 0);
			if (GetNotify())
			{
				NM_PPDUMP_MENU lpnm;
				
				lpnm.iArea = m_nCurArea;
				lpnm.iAddress	  = m_nCurrentAddr;
				lpnm.hMenu		  = hSubMenu;
				lpnm.hdr.hwndFrom = m_hWnd;
				lpnm.hdr.idFrom   = GetDlgCtrlID();
				lpnm.hdr.code     = UDM_PPDUMPCTRL_MENU_CALLBACK;
				
				ret = ::SendMessage(m_hParentWnd, WM_NOTIFY, lpnm.hdr.idFrom, (LPARAM)&lpnm);
			}
			if (hSubMenu)
			{
				DWORD dwRetValue = ::TrackPopupMenuEx(hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, pMsg->pt.x, pMsg->pt.y, m_hParentWnd, NULL);
				if (dwRetValue)
					::PostMessage(m_hParentWnd, WM_COMMAND, MAKEWPARAM(dwRetValue, 0), (LPARAM)NULL);
				CPoint pt = pMsg->pt;
                ScreenToClient(&pt);
                m_nCurArea = GetDataUnderCursor(pt, &m_nCurrentAddr);
				m_rLastTrackRect = GetRectAddress(m_nCurrentAddr, m_nCurArea);
				Invalidate();
			}
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CPPDumpCtrl::OnSize(UINT nType, int cx, int cy) 
{
	if (!::IsWindow(m_hWnd))
        return;
//    OnVScroll(SB_TOP, 0, 0);
	KillEdit();
	CWnd::OnSize(nType, cx, cy);
}

BOOL CPPDumpCtrl::OnEraseBkgnd(CDC* pDC) 
{
	//overridden for flicker-free drawing.
	return TRUE;
}

void CPPDumpCtrl::OnSetFocus(CWnd* pOldWnd) 
{
	TRACE(_T("CPPDumpCtrl::OnSetFocus()\n"));
	m_bFocused = TRUE;
	Invalidate(FALSE);
}

void CPPDumpCtrl::OnKillFocus(CWnd* pNewWnd) 
{
	TRACE(_T("CPPDumpCtrl::OnKillFocus()\n"));

	CWnd::OnKillFocus(pNewWnd);

	m_bFocused = FALSE;
	
	// TODO: Add your message handler code here
	Invalidate(FALSE);
	
}

void CPPDumpCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	TRACE(_T("CPPDumpCtrl::OnLButtonDown()\n"));
	// TODO: Add your message handler code here and/or call default
	if ((!m_rLastTrackRect.IsRectEmpty()) && (m_nCurArea > 0x11))
	{
		KillEdit();
		SetSelectRange(m_nCurrentAddr);
		m_bPressedLButton = IsEnableSelect();
	}
	
	CWnd::OnLButtonDown(nFlags, point);
}

void CPPDumpCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	TRACE(_T("CPPDumpCtrl::OnLButtonUp()\n"));

	if (m_bPressedLButton)
	{
		// Make sure this is a valid window
		if (IsWindow(GetSafeHwnd()) && GetNotify())
		{
			NM_PPDUMP_SEL lpnm;
			GetSelectRange(&lpnm.iFirstAddr, &lpnm.iLastAddr);
			if (lpnm.iFirstAddr != lpnm.iLastAddr)
			{
				lpnm.hdr.hwndFrom = m_hWnd;
				lpnm.hdr.idFrom   = GetDlgCtrlID();
				lpnm.hdr.code     = UDM_PPDUMPCTRL_SELECTION;
				::SendMessage(m_hParentWnd, WM_NOTIFY, lpnm.hdr.idFrom, (LPARAM)&lpnm);
			}
		}
	}
	m_bPressedLButton = FALSE;
	
	CWnd::OnLButtonUp(nFlags, point);
}

void CPPDumpCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	TRACE(_T("CPPDumpCtrl::OnRButtonDown()\n"));
	// TODO: Add your message handler code here and/or call default
//	CWnd::OnRButtonDown(nFlags, point);
}

void CPPDumpCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	TRACE(_T("CPPDumpCtrl::OnLButtonDblClk()\n"));
	//If in-place edit is exist, then delete him
	BOOL bExist = FALSE;
	
	if (m_pEdit != NULL)
	{
		//The edit is exist
		if (!m_rLastTrackRect.IsRectEmpty())
		{
			//Under the mouse is real data
			if ((m_nEditedArea != 0x1) && (m_nEditedArea != 0x11))
				CompleteEditValue(m_pEdit->GetValue(), FALSE); //If edit is exist then get the value
			
			bExist = KillEdit();
			//If address field was editing then redraw caontrol
			if ((m_nEditedArea & 0xF) == 0x1)
				Invalidate(FALSE);
		}
		else
		{
			m_pEdit->SetFocus();
			return;
		}
	}
	// block or sector TODO: *clw* open the next smaller view in another window
	// for now, just return
	if ( m_nCurArea == 0x17 || m_nCurArea == 0x18 )
		return;

	BOOL bEnable = FALSE;
	if (!m_rLastTrackRect.IsRectEmpty())
	{
		//Under the mouse is real data
		if (IsReadOnly())
		{
			//If the control - read only
			if ((m_nCurArea & 0xF) == 0x1)
				bEnable = TRUE; //With READ_ONLY enabled editing address field
		}
		else bEnable = TRUE; //The edit was enabled
	}

	if (m_nCurArea > 0x11)
		SetSelectRange(m_nCurrentAddr);

	if (bEnable)
	{
		int nAddr = -1;
		if (m_nCurArea == 0x11)
			nAddr = m_nCurrentAddr;
		else if ((!bExist) || (m_nCurArea > 0x10))
			nAddr = m_nCaretAddrBegin;

		SetEditedValue(nAddr, m_nCurArea);
	}
	else
		CWnd::OnLButtonDown(nFlags, point);
}

void CPPDumpCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	CDC * pDC = GetDC(); //CClientDC dc(this);

	TrackMouseMove(FALSE, pDC);

//	m_nCurArea = GetAreaCursor(point);
//	m_nCurrentAddr = m_nBeginAddress;

	m_nCurArea = GetDataUnderCursor(point, &m_nCurrentAddr);
	m_rLastTrackRect = GetRectAddress(m_nCurrentAddr, m_nCurArea);

	//Tracking to select data
	if (m_bPressedLButton && (!m_rLastTrackRect.IsRectEmpty()) && (m_nCurArea > 0x11))
	{
		int nBegin = m_nCaretAddrBegin;
		int nEnd = m_nCaretAddrEnd;

		if (m_nCurrentAddr >= m_nCaretAddrFirst)
		{
			nBegin = m_nCaretAddrFirst;
			nEnd = m_nCurrentAddr;
		}
		else if (m_nCurrentAddr < m_nCaretAddrFirst)
		{
			nEnd = m_nCaretAddrFirst;
			nBegin = m_nCurrentAddr;
		}

		if ((nEnd != m_nCaretAddrEnd) || (nBegin != m_nCaretAddrBegin))
		{
			//If select range of the address was changed 
			m_nCaretAddrBegin = nBegin;
			m_nCaretAddrEnd = nEnd;
			Invalidate(FALSE);
		}
	}
	
	TrackMouseMove(TRUE, pDC);
		
	if (((m_nCurArea < 0) || (m_nCurArea > 0x10)) && (m_nStyle & PPDUMP_BAR_ALL))
		UpdateControlBar(pDC);

	CWnd::OnMouseMove(nFlags, point);

	//Setup event of leave mouse
	CWnd*				wndUnderMouse = NULL;
	CWnd*				wndActive = this;
	TRACKMOUSEEVENT		csTME;

	ClientToScreen(&point);
	wndUnderMouse = WindowFromPoint(point);

	wndActive = GetActiveWindow();

	if (wndUnderMouse && wndUnderMouse->m_hWnd == m_hWnd && wndActive)
	{
		if (!m_bMouseOverCtrl)
		{
			m_bMouseOverCtrl = TRUE;
			csTME.cbSize = sizeof(csTME);
			csTME.dwFlags = TME_LEAVE;
			csTME.hwndTrack = m_hWnd;
			::_TrackMouseEvent(&csTME);
		}
	}
	else m_bMouseOverCtrl = FALSE;

	if (m_pToolTip.m_hWnd != NULL)
	{
		if ((m_nCurrentAddr >= 0) && (m_nCurArea > 0x11))
		{
			if (m_nCurrentAddr != m_nAddressToolTip)
			{
				m_pToolTip.Activate(FALSE);
				m_pToolTip.UpdateTipText(GetToolTipString(m_nCurrentAddr), this, 1);
				//			m_pToolTip.Activate(TRUE);
				m_nAddressToolTip = m_nCurrentAddr;
			}
			else m_pToolTip.Activate(TRUE);
		}
		else
		{	
			m_pToolTip.UpdateTipText(_T(""), this, 1);
			m_pToolTip.Activate(FALSE);
			m_nAddressToolTip = -1;
		}
	}
	else m_nAddressToolTip = -1;

	if (pDC)
		ReleaseDC(pDC);
}

// Handler for WM_MOUSELEAVE
LRESULT CPPDumpCtrl::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	m_bMouseOverCtrl = FALSE;
	TrackMouseMove();

	return 0;
} // End of OnMouseLeave

//Added by Darren Schroeder
BOOL CPPDumpCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if (zDelta < 0)
        OnVScroll(SB_LINEDOWN,0,0);
    else
        OnVScroll(SB_LINEUP,0,0);

    return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CPPDumpCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    ATLTRACE(_T("CPPDumpCtrl::OnVScroll() "));
    if ( !m_pVScroll.GetSafeHwnd() ) {
        ATLTRACE(_T(": no scroll bar.\n"));
        return;
    }

    //If in-place edit is exist, then delete him
	if (m_nEditedArea > 0x10)
		KillEdit();

	// TODO: Add your message handler code here and/or call default
	int nMax;
	int nMin;
	int nNow = m_pVScroll.GetScrollPos();
    ATLTRACE(_T("old_pos:%d "), nNow);
	m_pVScroll.GetScrollRange(&nMin, &nMax);
	switch (nSBCode)
	{
	case SB_TOP: 
		nNow = nMin; 
		break;
	case SB_BOTTOM: 
		nNow = nMax; 
		break;
	case SB_LINEDOWN: 
		nNow ++; 
		break;
	case SB_LINEUP: 
		nNow --; 
		break;
	case SB_PAGEDOWN: 
		nNow += (m_nMaxDataOnScreen / ( m_nDataInLines /** m_nDataWordSize*/ )); 
		break;
	case SB_PAGEUP: 
		nNow -= (m_nMaxDataOnScreen / ( m_nDataInLines /** m_nDataWordSize*/ )); 
		break;
//	case SB_THUMBPOSITION: 
	case SB_THUMBTRACK: 
		//Added 
		SCROLLINFO si;
		ZeroMemory (&si, sizeof(SCROLLINFO));
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_TRACKPOS;
		// Call GetScrollInfo to get current tracking 
		// position in si.nTrackPos
		if (!m_pVScroll.GetScrollInfo (&si, SIF_TRACKPOS))
			return; // GetScrollInfo failed
		nNow = si.nTrackPos ;
		break;
	}

	if (nNow > nMax)
		nNow = nMax;
	else if (nNow < nMin)
		nNow = nMin;

	m_pVScroll.SetScrollPos(nNow, true);
    ATLTRACE(_T("new_pos:%d\n"), nNow);
	
	nNow = GetScrollAddress(nNow);//m_nDataInLines;
    if ( nNow > m_nRealLengthData )
        nNow = m_nRealLengthData;

	if (nNow != m_nBeginAddress)
	{
		m_nBeginAddress = nNow;
		Invalidate(FALSE);
		SendNotify(UDM_PPDUMPCTRL_BEGIN_ADDR, m_nBeginAddress, 0);

		CPoint pt;
		::GetCursorPos(&pt);
		ScreenToClient(&pt);
		OnMouseMove(0, pt);
	}
	
	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CPPDumpCtrl::OnPaint() 
{
	TRACE(_T("CPPDumpCtrl::OnPaint()\n"));
	CPaintDC dc(this); // device context for painting

	//If the window is not visible then don't draw the control
	if (!::IsWindow(m_hWnd))
		return;

	if (m_nCaretAddrBegin >= m_nLengthData)
	{
		m_nCaretAddrBegin = 0;
		m_nCaretAddrEnd = 0;
	}

	//Set colors of the control
	BOOL bDisable = GetStyle() & WS_DISABLED;
	if (bDisable)
	{
		// if in-place edit is exist
		KillEdit();
	}
	
	//Once create the vertical scrollbar
	if (m_pVScroll.m_hWnd == NULL)
		m_pVScroll.Create (WS_CHILD | WS_VISIBLE | SBS_VERT | SBS_RIGHTALIGN, CRect (0, 0, 100, 100), this, 100);
	m_pVScroll.EnableWindow(!bDisable);
	
	// Get the client rect.
	CRect rect, rcClient;
	GetClientRect(rect);
	rcClient = rect;

	// Create a memory device-context. This is done to help reduce
	// screen flicker, since we will paint the entire control to the
	// off screen device context first.CDC memDC;
	CMemDC memDC(&dc);
//	CDC memDC;
//	CBitmap bitmap;
//	memDC.CreateCompatibleDC(&dc);
//	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
//	CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

	memDC.FillSolidRect(rect, bDisable ? m_crDisableBk : m_crColor [PPDUMP_COLOR_DATA_BK]);

	CFont * pFont = memDC.SelectObject(&m_font);
	memDC.SetBkMode(TRANSPARENT);

	memDC.SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	
	TEXTMETRIC tm;
	memDC.GetTextMetrics(&tm);
	
	//Gets the size of the current font
	m_nWidthFont = tm.tmAveCharWidth;
	m_nHeightFont = (int)((double)tm.tmHeight * 1.2);

	//Vertical Splits of the client area
	VerticalSplitClientArea(&memDC, rect);

	//Draws the control bar
	rect.top = DrawControlBar(&memDC, rect, bDisable);

	//Draws the status bar
	rect.top = DrawStatusBar(&memDC, rect, bDisable);

	//Adds the rects of the fields
	for (int i = 0; i < (PPDUMP_BAR_MAX_AREAS + 1); i++)
	{
		m_rFieldArea [i].bottom = rect.bottom;
		m_rFieldArea [i].top = rect.top;
	}

	m_nMaxDataOnScreen = GetMaxDataOnScreen();

	//Draws the fields
	if ((m_nStyle & PPDUMP_FIELD_ADDRESS) && m_nDataInLines)
		DrawAddressField(&memDC, bDisable);
	if (m_nStyle & PPDUMP_FIELD_HEX)
		DrawHexField(&memDC, bDisable);
	if (m_nStyle & PPDUMP_FIELD_DEC)
		DrawDecField(&memDC, bDisable);
	if (m_nStyle & PPDUMP_FIELD_BIN)
		DrawBinField(&memDC, bDisable);
	if (m_nStyle & PPDUMP_FIELD_OCT)
		DrawOctField(&memDC, bDisable);
	if (m_nStyle & PPDUMP_FIELD_ASCII)
		DrawASCIIField(&memDC, bDisable);
	if (m_nStyle & PPDUMP_FIELD_SECTOR)
		DrawSectorField(&memDC, bDisable);
	if (m_nStyle & PPDUMP_FIELD_BLOCK)
		DrawBlockField(&memDC, bDisable);

	//Get the rect of the scrollbar
	rect = m_rFieldArea [PPDUMP_BAR_MAX_AREAS];
	rect.left = rect.right - ::GetSystemMetrics(SM_CXVSCROLL);

	//Calculate the limits of the scrollbar
	INT_PTR nRowsOfData = 0;
	if (m_nDataInLines)	{
		nRowsOfData = GetMaxRows();
	}
	else m_nDataInLines = 1; 


	m_pVScroll.SetScrollRange(0, (int)nRowsOfData-(m_nMaxDataOnScreen/m_nDataInLines), FALSE);
	m_pVScroll.SetScrollPos((int)(nRowsOfData ? GetAddressRow(m_nBeginAddress) : 0), FALSE);
	m_pVScroll.ShowWindow(nRowsOfData > 0);

	//Redraws the scrollbar
	m_pVScroll.MoveWindow(rect);
	
	if (nRowsOfData > 0)
	{
		//Get the clip region
		CRgn rgn;
		rgn.CreateRectRgnIndirect(rect);
		if (m_nLengthData > 0)
			dc.SelectClipRgn(&rgn, RGN_XOR);
	}
	TrackMouseMove(TRUE, &memDC);

	if (m_bFocused)
	{
		memDC.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		memDC.SetBkColor(::GetSysColor(COLOR_WINDOW));
		CPen pen (PS_SOLID, 1, ::GetSysColor(COLOR_WINDOWTEXT)),
			* penOld = memDC.SelectObject(&pen);
//		rect = rcClient;
//		rect.DeflateRect(2, 2, 2, 2);
		memDC.DrawFocusRect(rcClient);
		memDC.SelectObject(penOld);
	}

	//Copy the memory device context back into the original DC via BitBlt().
//	dc.BitBlt(rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), &memDC, 0,0, SRCCOPY);
	
	//Cleanup resources.
	memDC.SelectObject(pFont);
//	memDC.SelectObject(pOldBitmap);
//	memDC.DeleteDC();
//	bitmap.DeleteObject();
}

BOOL CPPDumpCtrl::KillEdit()
{
	//If edit is exist then kill him
	if (m_pEdit != NULL)
	{
		delete m_pEdit;
		m_pEdit = NULL;
		return TRUE;
	}
	return FALSE;
}

void CPPDumpCtrl::InitToolTip()
{
	if (m_pToolTip.m_hWnd == NULL)
	{
		// Create ToolTip control
		m_pToolTip.Create(this);
		// Create inactive
		m_pToolTip.Activate(TRUE);
		// Enable multiline
		m_pToolTip.SendMessage(TTM_SETMAXTIPWIDTH, 0, 400);
		// Sets delay time to show the tooltip
	} // if*/
}

///////////////////////////////////////////////////////////
//
// Format string:
//   % [direction] [length] [type]
//
//   direction - the direction of data ('+'(blank) = default value or '-' = negative)
//               Default value is a high->low, Negative value is a low->high
//   length    - the number of the byte ('0'-'4', when '0'(blank) default value of the control)
//   type      - the type of the value:
//
//            R - address
//            H - hex
//            D - dec
//            B - bin
//            O - oct
//            A - ascii
//
//
///////////////////////////////////////////////////////////
CString CPPDumpCtrl::GetToolTipString(int nAddress)
{
	//Format string is empty
	if (m_sFormatToolTip.IsEmpty())
		return m_sFormatToolTip;

	CString str = _T("");

	int nLength = m_sFormatToolTip.GetLength();
	BOOL bCmd = FALSE;
	int  nDir;
	int  nNum;
	int  nBeginLinePos = 0;
	int  j;
	for (int i = 0; i < nLength; i++)
	{
		if (bCmd)
		{
			switch (m_sFormatToolTip.GetAt(i))
			{
			case (TCHAR)'+':
				nDir = 1;
				break;
			case (TCHAR)'-':
				nDir = 2;
				break;
			case (TCHAR)'0':
			case (TCHAR)'1':
			case (TCHAR)'2':
			case (TCHAR)'3':
			case (TCHAR)'4':
				nNum = m_sFormatToolTip.GetAt(i) - (TCHAR)'0';
				break;
			case (TCHAR)'R':
				if (!nNum)
				{
					nNum = m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] / 2;
					if (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] & 0x1)
						nNum ++;
				}
				str += FormatingString(PPDUMP_BAR_AREA_ADDRESS, m_pDoc->MapFileOffsetToDeviceAddress(nAddress), nNum);
				bCmd = FALSE;
				break;
			case (TCHAR)'H':
				str += FormatingString(PPDUMP_BAR_AREA_HEX, GetDataFromCurrentAddress(nAddress, TRUE ,nDir, nNum), nNum);
				bCmd = FALSE;
				break;
			case (TCHAR)'D':
				str += FormatingString(PPDUMP_BAR_AREA_DEC, GetDataFromCurrentAddress(nAddress, TRUE ,nDir, nNum), nNum);
				bCmd = FALSE;
				break;
			case (TCHAR)'B':
				str += FormatingString(PPDUMP_BAR_AREA_BIN, GetDataFromCurrentAddress(nAddress, TRUE ,nDir, nNum), nNum);
				bCmd = FALSE;
				break;
			case (TCHAR)'O':
				str += FormatingString(PPDUMP_BAR_AREA_OCT, GetDataFromCurrentAddress(nAddress, TRUE ,nDir, nNum), nNum);
				bCmd = FALSE;
				break;
			case (TCHAR)'A':
				str += FormatingString(PPDUMP_BAR_AREA_ASCII, GetDataFromCurrentAddress(nAddress, TRUE ,nDir, nNum), nNum);
				bCmd = FALSE;
				break;
			case (TCHAR)'n':
				str += _T("\r\n");
				nBeginLinePos = str.GetLength();
				bCmd = FALSE;
				break;
			case (TCHAR)'t':
				nNum = 4 - ((str.GetLength() - nBeginLinePos) % 4);
				for (j = 0; j < nNum; j++)
					str += (TCHAR)' '; //emulation of the tabulation
				bCmd = FALSE;
				break;
			default:
				bCmd = FALSE;
				str += m_sFormatToolTip.GetAt(i);
				break;
			}
		}
		else
		{
			if (m_sFormatToolTip.GetAt(i) == (TCHAR)'%')
			{
				bCmd = TRUE;
				nDir = 0;
				nNum = 0;
			}
			else str += m_sFormatToolTip.GetAt(i);
		}
	}

	return str;
}

///////////////////////////////////////////////////////////////////////////
//
// nIndex - the index of the value
//
///////////////////////////////////////////////////////////////////////////
CString CPPDumpCtrl::FormatingString(int nIndex, int nValue, int nLength /* = 0 */)
{
	CString str = _T("");
	CString str1 = _T("");
	PBYTE pch;
	int i;
	
	if (!nLength)
		nLength = m_nDataWordSize;
	
	switch (nIndex)
	{
	case PPDUMP_BAR_AREA_ADDRESS:
	case PPDUMP_BAR_AREA_HEX:
		//Hex
		str1.Format(_T("%%.%dX"), nLength * 2); //Two chars to one byte
		str.Format(str1, nValue);
		break;
	case PPDUMP_BAR_AREA_DEC:
		//Dec
		str.Format(_T("%d"), nValue);
		break;
	case PPDUMP_BAR_AREA_BIN:
		//Bin
		nLength *= 8;
		for (i = 0; i < nLength; i++)
		{
			if (!(i % 4) && i)
				str = _T(" ") + str;
			str = ((nValue & 0x1) ? (TCHAR)'1' : (TCHAR)'0') + str;
			nValue >>= 1;
		}
		break;
	case PPDUMP_BAR_AREA_OCT:
		//Oct
		str.Format(_T("%o"), nValue);
		break;
	case PPDUMP_BAR_AREA_ASCII:
		//Ascii
		pch = (PBYTE)&nValue;
		for (i = nLength-1; i >= 0; i--)
		{
			if ((pch[i] < 0x20) && m_chSpecCharView)
				str += m_chSpecCharView;
			else
				str += (TCHAR)pch[i];
		}
		break;
	}

	return str;
}

LRESULT CPPDumpCtrl::SendNotify(UINT uNotifyCode, UINT nAddress, UINT nValue)
{
	TRACE(_T("CPPDumpCtrl::SendNotify()\t%X\n"), uNotifyCode);

	// Make sure this is a valid window
	if (!IsWindow(GetSafeHwnd()))
		return 0L;

	// See if the user wants to be notified
	if (!GetNotify())
		return 0L;

	NM_PPDUMP_CTRL lpnm;
	
	lpnm.iAddress	  = nAddress;
	lpnm.iValue		  = nValue;
	lpnm.hdr.hwndFrom = m_hWnd;
    lpnm.hdr.idFrom   = GetDlgCtrlID();
    lpnm.hdr.code     = uNotifyCode;
	
	LRESULT ret = ::SendMessage(m_hParentWnd, WM_NOTIFY, lpnm.hdr.idFrom, (LPARAM)&lpnm);

	return 0L;
}

void CPPDumpCtrl::MoveCaretAddress(int nIndex, BOOL bEdited /* = FALSE */, UINT nValue /* = 0 */)
{
	if (bEdited)
		CompleteEditValue(nValue); //The notification from the edit
	else KillEdit();

	//The current address is the caret address
	int nNow = m_nCaretAddrBegin;
	int nBegin = 0;
	
	switch (nIndex)
	{
	case PPDUMP_MOVE_LEFT:
		nNow = m_nCaretAddrBegin - 1;
		break;
	case PPDUMP_MOVE_RIGHT:
		nNow = m_nCaretAddrBegin + 1;
		break;
	case PPDUMP_MOVE_UP:
		nNow = m_nCaretAddrBegin - m_nDataInLines;
		break;
	case PPDUMP_MOVE_DOWN:
		nNow = m_nCaretAddrBegin + m_nDataInLines;
		break;
	case PPDUMP_MOVE_PAGE_UP:
		nNow = m_nCaretAddrBegin - m_nMaxDataOnScreen;
		if (nNow < 0)
		{
			nBegin = 0;
			nNow = m_nCaretAddrBegin % m_nDataInLines;
		}
		break;
	case PPDUMP_MOVE_PAGE_DOWN:
		nNow = m_nCaretAddrBegin + m_nMaxDataOnScreen;
		break;
	case PPDUMP_MOVE_FIRST_DATA:
		nNow = 0;
		break;
	case PPDUMP_MOVE_LAST_DATA:
		nNow = m_nLengthData - 1;
		break;
	case PPDUMP_MOVE_BEGIN_LINE:
		nNow = (nNow / (m_nDataInLines*m_nDataWordSize) ) * m_nDataInLines * m_nDataWordSize;
		break;
	case PPDUMP_MOVE_END_LINE:
		nNow = (nNow / (m_nDataInLines*m_nDataWordSize)) * m_nDataInLines + m_nDataInLines - 1;
		if (nNow >= m_nLengthData)
			nNow = m_nLengthData - 1;
		break;
	case PPDUMP_MOVE_NEXT_FIELD:
		m_nEditedArea = GetNextField(m_nEditedArea);
		break;
	case PPDUMP_MOVE_PREV_FIELD:
		m_nEditedArea = GetPrevField(m_nEditedArea);
		break;
	}
	
	if (nNow >= m_nLengthData)
	{
		nBegin = (m_nCaretAddrBegin / m_nDataInLines) * m_nDataInLines + m_nDataInLines;
		
		if (nBegin < m_nLengthData)
			nNow = m_nLengthData - 1;
		else
			nNow = m_nCaretAddrBegin;
	}
	else if (nNow >= 0)
		SetSelectRange(nNow, -1, FALSE);
	else nNow = 0;

	SetVisibleAddress(nNow);

	if (bEdited)
	{
		m_nEditedAddress = m_nCaretAddrBegin;
		SetEditedValue();
	}
}

////////////////////////////////////////////////////////////////////
// Sets the first address on the screen, so that the specified
// address on the screen has been seen
//
//
////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::SetVisibleAddress(int nAddress)
{
	BOOL bMove = FALSE; //m_nBeginAddress no move

	//Gets the address of first data in line
	int nVal = (nAddress / m_nDataInLines) * m_nDataInLines;

	if (nVal < m_nBeginAddress)
	{
		//New address less then current
		bMove = TRUE;
		m_nBeginAddress = nVal;
	}
	else if (nVal >= (m_nBeginAddress + m_nMaxDataOnScreen))
	{
		//New address more then current
		bMove = TRUE;
		m_nBeginAddress = nVal - m_nMaxDataOnScreen + m_nDataInLines;
	}

	//If current address was moved then to notify the parent
	if (bMove && GetNotify())
	{
		SendNotify(UDM_PPDUMPCTRL_BEGIN_ADDR, m_nBeginAddress, 0);
	}
	Invalidate(FALSE);

	return bMove;
}

void CPPDumpCtrl::NotifyEditEnter(NMHDR * pNMHDR, LRESULT * result)
{
	TRACE (_T("CPPDumpCtrl::NotifyEditEnter()\n"));

	*result = 0;
	NM_PPNUM_EDIT * pNotify = (NM_PPNUM_EDIT*)pNMHDR;
	CompleteEditValue(pNotify->iValue);
}

void CPPDumpCtrl::NotifyEditCancel(NMHDR * pNMHDR, LRESULT * result)
{
	TRACE (_T("CPPDumpCtrl::NotifyEditCancel()\n"));

	*result = 0;
	KillEdit();
}

void CPPDumpCtrl::NotifyEditMove(NMHDR * pNMHDR, LRESULT * result)
{
	TRACE (_T("CPPDumpCtrl::NotifyEditMove()\n"));

	*result = 0;
	if ((m_nEditedArea == 0x1) || (m_nEditedArea == 0x11))
		return;

	NM_PPNUM_EDIT * pNotify = (NM_PPNUM_EDIT*)pNMHDR;
	MoveCaretAddress(pNotify->iEvent, TRUE, pNotify->iValue);
}

void CPPDumpCtrl::NotifyEditHotKeys(NMHDR * pNMHDR, LRESULT * result)
{
	TRACE (_T("CPPDumpCtrl::NotifyEditHotKeys()\n"));

	*result = 0;
	NM_PPNUM_EDIT * pNotify = (NM_PPNUM_EDIT*)pNMHDR;
	HandleHotkeys(pNotify->iEvent, TRUE, pNotify->iValue);
}

void CPPDumpCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
	case VK_LEFT:
		MoveCaretAddress(PPDUMP_MOVE_LEFT);
		return;
	case VK_RIGHT:
		MoveCaretAddress(PPDUMP_MOVE_RIGHT);
		return;
	case VK_UP:
		MoveCaretAddress(PPDUMP_MOVE_UP);
		return;
	case VK_DOWN:
		MoveCaretAddress(PPDUMP_MOVE_DOWN);
		return;
	case VK_PRIOR:
		MoveCaretAddress(PPDUMP_MOVE_PAGE_UP);
		return;
	case VK_NEXT:
		MoveCaretAddress(PPDUMP_MOVE_PAGE_DOWN);
		return;
	case VK_HOME:
		if (::GetKeyState(VK_CONTROL) < 0)
			MoveCaretAddress(PPDUMP_MOVE_FIRST_DATA);
		else
			MoveCaretAddress(PPDUMP_MOVE_BEGIN_LINE);
		return;
	case VK_END: 
		if (::GetKeyState(VK_CONTROL) < 0)
			MoveCaretAddress(PPDUMP_MOVE_LAST_DATA);
		else
			MoveCaretAddress(PPDUMP_MOVE_END_LINE);
		return;
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CPPDumpCtrl::HandleHotkeys(UINT uNotifyCode, BOOL bFromEdit /* = FALSE */, UINT nValue /* = 0 */)
{
	if (IsReadOnly())
		return FALSE;

	int nArea = -1;
	
	switch(uNotifyCode) 
	{
	case PPDUMP_HOTKEY_HEX:
		if (IsExistField(0x2))
			nArea = 0x2;
		break;
	case PPDUMP_HOTKEY_DEC:
		if (IsExistField(0x3))
			nArea = 0x3;
		break;
	case PPDUMP_HOTKEY_BIN:
		if (IsExistField(0x4))
			nArea = 0x4;
		break;
	case PPDUMP_HOTKEY_OCT:
		if (IsExistField(0x5))
			nArea = 0x5;
		break;
	case PPDUMP_HOTKEY_ASCII:
		if (IsExistField(0x6))
			nArea = 0x6;
		break;
	}

	if (nArea < 0)
		return FALSE;

	if (bFromEdit && (m_nEditedArea == nArea))
		return FALSE;

	if (!bFromEdit)
		m_nEditedAddress = m_nCaretAddrBegin;
	else
		KillEdit();
	
	m_nEditedArea = nArea;
	UpdateControlBar(NULL, FALSE, m_nEditedAddress);
	SetEditedValue();
	
	return TRUE;
}

int CPPDumpCtrl::GetNextField(int nArea)
{
	int nOldArea = nArea;
	for (int i = 0; i < 5; i++)
	{
		nArea ++;
		if ((nArea & 0xF) > 6)
			nArea = (nArea & 0xF0) + 0x2;
		if (IsExistField(nArea))
			return nArea;
	}
	return nOldArea;
}

int CPPDumpCtrl::GetPrevField(int nArea)
{
	int nOldArea = nArea;
	for (int i = 0; i < 5; i++)
	{
		nArea --;
		if ((nArea & 0xF) < 2)
			nArea = (nArea & 0xF0) + 0x6;
		if (IsExistField(nArea))
			return nArea;
	}
	return nOldArea;
}

BOOL CPPDumpCtrl::IsExistField(int nArea)
{
	switch (nArea)
	{
	case 0x01:
		return (m_nStyle & PPDUMP_BAR_ADDRESS);
	case 0x02:
		return (m_nStyle & PPDUMP_BAR_HEX);
	case 0x03:
		return (m_nStyle & PPDUMP_BAR_DEC);
	case 0x04:
		return (m_nStyle & PPDUMP_BAR_BIN);
	case 0x05:
		return (m_nStyle & PPDUMP_BAR_OCT);
	case 0x06:
		return (m_nStyle & PPDUMP_BAR_ASCII);
	case 0x07:
		return (m_nStyle & PPDUMP_BAR_SECTOR);
	case 0x08:
		return (m_nStyle & PPDUMP_BAR_BLOCK);
	case 0x09:
		return (m_nStyle & PPDUMP_BAR_REGION);
	case 0x0A:
		return (m_nStyle & PPDUMP_BAR_DRIVE);
	case 0x11:
		return (m_nStyle & PPDUMP_FIELD_ADDRESS);
	case 0x12:
		return (m_nStyle & PPDUMP_FIELD_HEX);
	case 0x13:
		return (m_nStyle & PPDUMP_FIELD_DEC);
	case 0x14:
		return (m_nStyle & PPDUMP_FIELD_BIN);
	case 0x15:
		return (m_nStyle & PPDUMP_FIELD_OCT);
	case 0x16:
		return (m_nStyle & PPDUMP_FIELD_ASCII);
	case 0x17:
		return (m_nStyle & PPDUMP_FIELD_SECTOR);
	case 0x18:
		return (m_nStyle & PPDUMP_FIELD_BLOCK);
	}
	return FALSE;
}

int CPPDumpCtrl::GetAddressForControlBar()
{
	int nAddr = 0;
	if (m_pEdit == NULL)
	{
		//The edit is not exist
		if (m_rLastTrackRect.IsRectEmpty())
			nAddr = m_nCaretAddrBegin;
		else if (m_nCurArea < 0x10)
			nAddr = m_nCaretAddrBegin;
		else
			nAddr = m_nCurrentAddr;
	}
	else
	{
		//The edit is exist
		if (m_nEditedArea < 0x11)
			nAddr = m_nEditedAddress;
		else if (m_rLastTrackRect.IsRectEmpty())
			nAddr = m_nEditedAddress;
		else if (m_nCurArea < 0x10)
			nAddr = m_nEditedAddress;
		else
			nAddr = m_nCurrentAddr;
	}

	return nAddr;
}

int CPPDumpCtrl::GetMaxDataOnScreen()
{
	CRect rect = m_rFieldArea[PPDUMP_BAR_MAX_AREAS];
	rect.top += m_nHeightFont / 4;
    
	
    int nLines = rect.Height() / m_nHeightFont;

	
    return (nLines * m_nDataInLines /** m_nDataWordSize*/);
}

BOOL CPPDumpCtrl::CompleteEditValue(UINT nValue, BOOL bDelete /* = TRUE */)
{
	//If edited value is not exist
	if (m_pEdit == NULL)
		return FALSE;

	//If value with error
	if (!m_pEdit->IsValidate())
		return FALSE;

	//If value was not changed
	if (m_pEdit->IsChanged())
	{
		if ((m_nEditedArea & 0xF) == 1)
		{
			//if edited value is an address
			SetSelectRange(nValue - m_nOffsetAddress, -1);
		}
		else
			SendNotify(UDM_PPDUMPCTRL_CHANGE_DATA, m_nEditedAddress, nValue);
	}

	if (bDelete)
	{
		KillEdit();
		Invalidate(FALSE);
	}
	return TRUE;
}

void CPPDumpCtrl::SetEditedValue(int nAddr /* = -1 */, int nArea /* = -1 */)
{
	TRACE(_T("CPPDumpCtrl::SetEditedValue()\n"));
	
	//If editing address as parameter then keep him
	if (nAddr >= 0)
		m_nEditedAddress = nAddr;
	//If editing area as parameter then keep it
	if (nArea >= 0)
		m_nEditedArea = nArea;

	if (IsReadOnly() && ((m_nEditedArea & 0xF) != 0x1))
		return;

	//The editing value must be on the screen
	if (m_nEditedArea > 0x10)
		SetVisibleAddress(m_nEditedAddress);
	
	CRect rect = GetRectAddress(m_nEditedAddress, m_nEditedArea);
	
	m_pEdit = new CPPNumEdit;
	m_pEdit->Create(WS_CHILD | WS_VISIBLE | ES_CENTER, rect, this, nIdEdit);
	m_pEdit->SetFont(&m_font);
	m_pEdit->SetColor(CPPNumEdit::PPNUM_COLOR_VALID_FG, m_crColor [PPDUMP_COLOR_EDIT_FG], FALSE);
	m_pEdit->SetColor(CPPNumEdit::PPNUM_COLOR_VALID_BK, m_crColor [PPDUMP_COLOR_EDIT_BK], FALSE);
	m_pEdit->SetColor(CPPNumEdit::PPNUM_COLOR_NOT_VALID_FG, m_crColor [PPDUMP_COLOR_EDIT_ERR_FG], FALSE);
	m_pEdit->SetColor(CPPNumEdit::PPNUM_COLOR_NOT_VALID_BK, m_crColor [PPDUMP_COLOR_EDIT_ERR_BK]);
	UINT nIndexValue;
	UINT nTypeValue = (m_nStyle & PPDUMP_WORD_DATA) ? CPPNumEdit::PPNUM_VALUE_WORD : CPPNumEdit::PPNUM_VALUE_BYTE;
	UINT nMin = 0;
	UINT nMax = 0x1;
	
	UINT nValue;

	if ((m_nEditedArea & 0x0F) == 0x1)
		nValue = m_nEditedAddress + m_nOffsetAddress;
	else
		nValue = GetDataFromCurrentAddress(m_nEditedAddress);
	
	switch (m_nEditedArea & 0xF)
	{
	case 0x01:
	case 0x07:
	case 0x08:
		nTypeValue = CPPNumEdit::PPNUM_VALUE_CUSTOM;
		nMin = m_nOffsetAddress;
		nMax = nMin + m_nLengthData - 1;
	case 0x02:
		nIndexValue = CPPNumEdit::PPNUM_VALUE_HEX;
		break;
	case 0x03:
		nIndexValue = CPPNumEdit::PPNUM_VALUE_DEC;
		break;
	case 0x04:
		nIndexValue = CPPNumEdit::PPNUM_VALUE_BIN;
		break;
	case 0x05:
		nIndexValue = CPPNumEdit::PPNUM_VALUE_OCT;
		break;
	case 0x06:
		nIndexValue = CPPNumEdit::PPNUM_VALUE_ASCII;
		break;
	}
	for (int i = 0; i < m_nDataWordSize*2; ++i )
		nMax |= 0xf << i*4;
	m_pEdit->SetValue(nValue, nIndexValue, 0, 0, nMax);
	m_pEdit->SetFocus();
	m_pEdit->SetSel(0, -1);
}

///////////////////////////////////////////////////////////////////
//
// int GetDataUnderCursor(CPoint pt, LPINT nAddress)
// Gets the data under the cursor
// 
// Parameters:
//   pt       [in]  - the current coordinates of the cursor
//   nAddress [out] - the address of the data (if exist) under the cursor
// 
// Return value
//   The area of the data under the cursor
//
///////////////////////////////////////////////////////////////////
int CPPDumpCtrl::GetDataUnderCursor(CPoint pt, INT_PTR* nAddress)
{
	*nAddress = -1;
	int nArea = -1;
	CRect rect;

	//Determines the area under the cursor
	if (m_nStyle & PPDUMP_BAR_ALL)
	{
		if (m_rBarArea [PPDUMP_BAR_MAX_AREAS].PtInRect(pt))
		{
			//The cursor into the Control Bar
			for (int i = 0; (i < PPDUMP_BAR_MAX_AREAS) && (nArea == -1); i++)
			{
				if (IsExistField(i + 1))
				{
					if (m_rBarArea [i].PtInRect(pt))
					{
						nArea = i + 1;
						rect = m_rBarArea [i];
					}
				}
			}
			if (nArea == -1)
				return -1;
		}
	}

	if ((m_nStyle & PPDUMP_FIELD_ALL) && (nArea == -1))
	{
		if (m_rFieldArea [PPDUMP_BAR_MAX_AREAS].PtInRect(pt))
		{
			//The cursor into the Data Fields
			for (int i = 0; (i < PPDUMP_BAR_MAX_AREAS) && (nArea == -1); i++)
			{
				if (IsExistField(i + 0x11))
				{
					if (m_rFieldArea [i].PtInRect(pt))
					{
						nArea = i + 0x11;
						rect = m_rFieldArea [i];
					}
				}
			}
			if (nArea == -1)
				return -1;
		}
	}

	//Determines address under the cursor
	if (nArea < 0x10)
	{
		//The cursor over the control bar
		*nAddress = GetAddressForControlBar();
		return nArea;
	}

	int nColumn;
	rect.top += m_nHeightFont / 4;
	rect.bottom = rect.top + (rect.Height() / m_nHeightFont) * m_nHeightFont;
	if (nArea != 0x16)
	{
		//For all data fields except Ascii Data Field
		rect.left += m_nWidthFont / 2;
		rect.right -= (m_nWidthFont / 2 + 1);
		nColumn = (pt.x - rect.left) / m_nWidthFont / (m_nCharsData [nArea - 0x11] + 1);
	}
	else
	{
		//For Ascii Data Field Only
		rect.left += m_nWidthFont - 1;
		rect.right -= (m_nWidthFont + 1);
		nColumn = (pt.x - rect.left) / ((m_nWidthFont*m_nCharsData [nArea - 0x11]) + 1);
	}
	if (!rect.PtInRect(pt))
		return - 1;

	//Retrive row and column data
	INT_PTR nRow = GetRowFromVOffset(pt.y - rect.top);

	INT_PTR nAddr = GetRowColAddress(nRow, nColumn); 

	if ((nAddr >= m_nLengthData) || !rect.PtInRect(pt))
		return - 1;
	
	*nAddress = nAddr;
	
	return nArea;
}

//////////////////////////////////////////////////////////////////////////
// Gets the rect of the address in specified area
//
//  In: nArea - the specified area
//      nAddress - the requered address
// Out: The bounding rect
//////////////////////////////////////////////////////////////////////////
CRect CPPDumpCtrl::GetRectAddress(int nAddress, int nArea)
{
	CRect rect;
	
	if (nAddress == -1) {
		rect.SetRectEmpty();
		return rect;
	}
	
	if (nArea > 0x10)
	{
		nArea -= 0x11;

		INT_PTR nRow = GetAddressRow(nAddress) - GetAddressRow(m_nBeginAddress);
		INT_PTR nCol = GetAddressColumn(nAddress);
		
		rect = m_rFieldArea [nArea];
		
//		rect.top += GetAddressVOffset(nAddress/* - m_nBeginAddress*/) + m_nHeightFont / 4;
		rect.top += (LONG)(GetRowVOffset(nRow) + m_nHeightFont / 4);
		rect.bottom = rect.top + m_nHeightFont;
		
		if (nArea != PPDUMP_BAR_AREA_ASCII)
		{
			rect.left += m_nWidthFont / 2;
			rect.left = rect.left + nCol * m_nWidthFont * (m_nCharsData [nArea] + 1);
			rect.right = rect.left + m_nWidthFont * (m_nCharsData [nArea] + 1);
		}
		else
		{
			rect.left += m_nWidthFont - 1;
			rect.left += (LONG)(nCol * ((m_nWidthFont*m_nCharsData [nArea]) + 1));
			rect.right = (LONG)(rect.left + (m_nWidthFont*m_nCharsData [nArea]) + 2);
		}
	}
	else if (nArea > 0)
		rect = m_rBarArea [nArea - 1];
	else
		rect.SetRectEmpty();
	
	return rect;
}

void CPPDumpCtrl::TrackMouseMove(BOOL bDrawn /* = FALSE */, CDC * pDC /* = NULL */)
{
	if (!m_rLastTrackRect.IsRectEmpty() && IsTrackMouseMove())
	{
		BOOL bReleaseDC = FALSE;
		if (pDC == NULL)
		{
			pDC = GetDC();
			bReleaseDC = TRUE;
		}

		if (bDrawn)
		{
			pDC->Draw3dRect(m_rLastTrackRect, m_crColor[PPDUMP_COLOR_MOUSE_TRACK], m_crColor[PPDUMP_COLOR_MOUSE_TRACK]);
		}
		else
		{
			pDC->Draw3dRect(m_rLastTrackRect, m_crColor[PPDUMP_COLOR_DATA_BK], m_crColor[PPDUMP_COLOR_DATA_BK]);
			m_rLastTrackRect.SetRectEmpty();
		}
		if (bReleaseDC)
			ReleaseDC(pDC);

	}
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::TrackDataField  (public member function)
//    Track the data of the hex field
//
//  Parameters :
//		point	    [in] - Specifies the x- and y-coordinate of the cursor
//
//  Returns :
//		Number of the Data under the mouse, if none = -1
//
/////////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::TrackDataField(UINT nIndex, CPoint point)
{
	CRect rect = m_rFieldArea [nIndex];
	rect.left += m_nWidthFont / 2;
	rect.right -= (m_nWidthFont / 2 + 1);
	rect.top += m_nHeightFont / 4;
	rect.bottom = rect.top + (rect.Height() / m_nHeightFont) * m_nHeightFont;

	if (!rect.PtInRect(point))
		return FALSE;
	
	//Retrive row and column data
	int nColumn = (point.x - rect.left) / m_nWidthFont / (m_nCharsData [nIndex] + 1);
//	int nRow = (point.y - rect.top) / m_nHeightFont;
	INT_PTR nRow = GetRowFromVOffset(point.y - rect.top);
	INT_PTR nAddr = GetRowColAddress(nRow, nColumn); 

//	int nData = nRow * m_nDataInLines + nColumn; 
	
//	if ((m_nBeginAddress + nData) < m_nLengthData)
    if (nAddr < m_nLengthData)
	{
//		m_nCurrentAddr = m_nBeginAddress + nData;
        m_nCurrentAddr = nAddr;

		m_rLastTrackRect.left = rect.left + nColumn * m_nWidthFont * (m_nCharsData [nIndex] + 1);
//		m_rLastTrackRect.top = rect.top + m_nHeightFont * nRow;
        m_rLastTrackRect.top = GetAddressVOffset(nAddr);
        m_rLastTrackRect.right = m_rLastTrackRect.left + m_nWidthFont * (m_nCharsData [nIndex] + 1);
		m_rLastTrackRect.bottom = m_rLastTrackRect.top + m_nHeightFont;

		return TRUE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::TrackDataAsciiField  (public member function)
//    Track the data of the hex field
//
//  Parameters :
//		point	    [in] - Specifies the x- and y-coordinate of the cursor
//
//  Returns :
//		Number of the Data under the mouse, if none = -1
//
/////////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::TrackDataAsciiField(CPoint point)
{
	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_ASCII];
	rect.left += m_nWidthFont - 1;
	rect.right -= (m_nWidthFont + 1);
	rect.top += m_nHeightFont / 4;
	rect.bottom = rect.top + (rect.Height() / m_nHeightFont) * m_nHeightFont;

	if (!rect.PtInRect(point))
		return FALSE;
	
	int nColumn = (point.x - rect.left) / (m_nWidthFont + 1);
	int nRow = (point.y - rect.top) / m_nHeightFont;

	int nData = nRow * m_nDataInLines + nColumn; 
	
	if ((m_nBeginAddress + nData) < m_nLengthData)
	{
		m_nCurrentAddr = m_nBeginAddress + nData;

		m_rLastTrackRect.left = rect.left + nColumn * (m_nWidthFont + 1);
		m_rLastTrackRect.top = rect.top + m_nHeightFont * nRow;
		m_rLastTrackRect.right = m_rLastTrackRect.left + m_nWidthFont + 2;
		m_rLastTrackRect.bottom = m_rLastTrackRect.top + m_nHeightFont;

		return TRUE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawAddressField  (public member function)
//    Draws the address field of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::DrawAddressField(CDC * pDC, BOOL bDisable)
{
	if (!(m_nStyle & PPDUMP_FIELD_ADDRESS))
		return;

	COLORREF crTemp = pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_ADDRESS_FG]);
	CPen pen(PS_SOLID, 0, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);

	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_ADDRESS];

	if (m_nStyle & PPDUMP_SEPARATOR_LINES)
	{
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	rect.DeflateRect(0, m_nHeightFont / 4, 0, m_nHeightFont);
    CRect tmp_rect(rect);

	for (INT_PTR nAddr = m_nBeginAddress; (nAddr < m_nLengthData) && (tmp_rect.top <= rect.bottom); NULL)
	{
        tmp_rect = rect;
        BOOL is_block_end;
        tmp_rect.top += (LONG)(GetAddressVOffset(nAddr) - GetAddressVOffset(m_nBeginAddress));
        DrawAddressValue(pDC, tmp_rect, (UINT)(nAddr + m_nOffsetAddress), FALSE, bDisable);
        nAddr += GetRowLength(GetAddressRow(nAddr + m_nOffsetAddress), &is_block_end) * m_nDataWordSize;

        if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
            if ( is_block_end ) {
                tmp_rect.top += m_nHeightFont + 3;
		        pDC->MoveTo(tmp_rect.left, tmp_rect.top);
		        pDC->LineTo(tmp_rect.right, tmp_rect.top);
            }
        }
	}
	pDC->SelectObject(penOld);
	pDC->SetTextColor(crTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawHexField  (public member function)
//    Draws the hex field of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::DrawHexField(CDC * pDC, BOOL bDisable)
{
	if (!(m_nStyle & PPDUMP_FIELD_HEX))
		return;

	COLORREF crTemp = pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	CPen pen(PS_SOLID, 0, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);

	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_HEX];

	if (m_nStyle & PPDUMP_SEPARATOR_LINES)
	{
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	rect.DeflateRect(0, m_nHeightFont / 4, 0, m_nHeightFont);

	UINT nNewValue, nOldValue;
    CRect tmp_rect(rect);

	for (int nAddr = m_nBeginAddress; (nAddr < m_nLengthData) && (tmp_rect.top <= rect.bottom); NULL)
	{
        tmp_rect = rect;
        BOOL is_block_end;
        tmp_rect.top += (LONG)(GetAddressVOffset(nAddr) - GetAddressVOffset(m_nBeginAddress));

        INT_PTR len = GetRowLength(GetAddressRow(nAddr + m_nOffsetAddress), &is_block_end);
		for (int i = 0; (i < len) && (nAddr < m_nLengthData); i++)
		{
			nNewValue = GetDataFromCurrentAddress(nAddr, TRUE);
			nOldValue = GetDataFromCurrentAddress(nAddr, FALSE);
			if (!bDisable) {
				if ( m_nStyle & PPDUMP_DATA_BLOCKS && IsRedundantArea(nAddr) )
					pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_RDATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
				else
					pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_DATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
			}
            tmp_rect.left = DrawHexValue(pDC, tmp_rect, nNewValue, FALSE, IsAddressSelected(nAddr), bDisable);
			tmp_rect.left -= m_nWidthFont;
			nAddr += m_nDataWordSize;
		}
        if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
            if ( is_block_end ) {
                tmp_rect.top += m_nHeightFont + 3;
                tmp_rect.left = rect.left;
		        pDC->MoveTo(tmp_rect.left, tmp_rect.top);
		        pDC->LineTo(tmp_rect.right, tmp_rect.top);
            }
        }
	}
	pDC->SelectObject(penOld);
	pDC->SetTextColor(crTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawDecField  (public member function)
//    Draws the dec field of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::DrawDecField(CDC * pDC, BOOL bDisable)
{
	if (!(m_nStyle & PPDUMP_FIELD_DEC))
		return;

	COLORREF crTemp = pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	CPen pen(PS_SOLID, 0, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);

	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_DEC];

	if (m_nStyle & PPDUMP_SEPARATOR_LINES)
	{
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	rect.DeflateRect(0, m_nHeightFont / 4, 0, m_nHeightFont);

	UINT nNewValue, nOldValue;
	CRect rData;

	for (int nAddr = m_nBeginAddress; (nAddr < m_nLengthData) && (rect.top <= rect.bottom); NULL)
	{
		rData = rect;
		for (int i = 0; (i < m_nDataInLines) && (nAddr < m_nLengthData); i++)
		{
			nNewValue = GetDataFromCurrentAddress(nAddr);
			nOldValue = GetDataFromCurrentAddress(nAddr, FALSE);
			if (!bDisable)
				pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_DATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
			rData.left = DrawDecValue(pDC, rData, nNewValue, FALSE, IsAddressSelected(nAddr), bDisable);
			rData.left -= m_nWidthFont;
			nAddr ++;
		}
		rect.top += m_nHeightFont;
	}
	pDC->SelectObject(penOld);
	pDC->SetTextColor(crTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawBinField  (public member function)
//    Draws the bin field of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::DrawBinField(CDC * pDC, BOOL bDisable)
{
	if (!(m_nStyle & PPDUMP_FIELD_BIN))
		return;

	COLORREF crTemp = pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	CPen pen(PS_SOLID, 0, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);

	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_BIN];

	if (m_nStyle & PPDUMP_SEPARATOR_LINES)
	{
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	rect.DeflateRect(0, m_nHeightFont / 4, 0, m_nHeightFont);

	UINT nNewValue, nOldValue;
	CRect rData;

	for (int nAddr = m_nBeginAddress; (nAddr < m_nLengthData) && (rect.top <= rect.bottom); NULL)
	{
		rData = rect;
		for (int i = 0; (i < m_nDataInLines) && (nAddr < m_nLengthData); i++)
		{
			nNewValue = GetDataFromCurrentAddress(nAddr);
			nOldValue = GetDataFromCurrentAddress(nAddr, FALSE);
			if (!bDisable)
				pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_DATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
			rData.left = DrawBinValue(pDC, rData, nNewValue, FALSE, IsAddressSelected(nAddr), bDisable);
			rData.left -= m_nWidthFont;
			nAddr ++;
		}
		rect.top += m_nHeightFont;
	}
	pDC->SelectObject(penOld);
	pDC->SetTextColor(crTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawOctField  (public member function)
//    Draws the oct field of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::DrawOctField(CDC * pDC, BOOL bDisable)
{
	if (!(m_nStyle & PPDUMP_FIELD_OCT))
		return;

	COLORREF crTemp = pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	CPen pen(PS_SOLID, 0, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);

	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_OCT];

	if (m_nStyle & PPDUMP_SEPARATOR_LINES)
	{
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	rect.DeflateRect(0, m_nHeightFont / 4, 0, m_nHeightFont);

	UINT nNewValue, nOldValue;
	CRect rData;

	for (int nAddr = m_nBeginAddress; (nAddr < m_nLengthData) && (rect.top <= rect.bottom); NULL)
	{
		rData = rect;
		for (int i = 0; (i < m_nDataInLines) && (nAddr < m_nLengthData); i++)
		{
			nNewValue = GetDataFromCurrentAddress(nAddr);
			nOldValue = GetDataFromCurrentAddress(nAddr, FALSE);
			if (!bDisable)
				pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_DATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
			rData.left = DrawOctValue(pDC, rData, nNewValue, FALSE, IsAddressSelected(nAddr), bDisable);
			rData.left -= m_nWidthFont;
			nAddr ++;
		}
		rect.top += m_nHeightFont;
	}
	pDC->SelectObject(penOld);
	pDC->SetTextColor(crTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawAsciiField  (public member function)
//    Draws the ascii field of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::DrawASCIIField(CDC * pDC, BOOL bDisable)
{
	if (!(m_nStyle & PPDUMP_FIELD_ASCII))
		return;

	COLORREF crTemp = pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	CPen pen(PS_SOLID, 0, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);

	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_ASCII];

	if (m_nStyle & PPDUMP_SEPARATOR_LINES)
	{
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	rect.DeflateRect(0, m_nHeightFont / 4, 0, m_nHeightFont);

	UINT nNewValue, nOldValue;
    CRect tmp_rect(rect);

	for (int nAddr = m_nBeginAddress; (nAddr < m_nLengthData) && (tmp_rect.top <= rect.bottom); NULL)
	{
        tmp_rect = rect;
        BOOL is_block_end;
        tmp_rect.top += (LONG)(GetAddressVOffset(nAddr) - GetAddressVOffset(m_nBeginAddress));
        INT_PTR len = GetRowLength(GetAddressRow(nAddr), &is_block_end);
		for (int i = 0; (i < len) && (nAddr < m_nLengthData); i++)
		{
			nNewValue = GetDataFromCurrentAddress(nAddr);
			nOldValue = GetDataFromCurrentAddress(nAddr, FALSE);
			if (!bDisable) {
				if ( m_nStyle & PPDUMP_DATA_BLOCKS && IsRedundantArea(nAddr) )
					pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_RDATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
				else
					pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_DATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
			}
			tmp_rect.left = DrawAsciiValue(pDC, tmp_rect, nNewValue, FALSE, IsAddressSelected(nAddr), bDisable);
			tmp_rect.left -= m_nWidthFont * 2;
			nAddr += m_nDataWordSize;
		}
        if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
            if ( is_block_end ) {
                tmp_rect.top += m_nHeightFont + 3;
                tmp_rect.left = rect.left;
		        pDC->MoveTo(tmp_rect.left, tmp_rect.top);
		        pDC->LineTo(tmp_rect.right, tmp_rect.top);
            }
        }
	}
	pDC->SelectObject(penOld);
	pDC->SetTextColor(crTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::VerticalSplitClientArea  (public member function)
//    The Verical spliting of the client area to the areas of the data fields
//
//  Parameters :
//		pDC		    [in] - the device context
//		rect		[in] - rect of the control
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::VerticalSplitClientArea(CDC * pDC, CRect rect)
{
	m_rFieldArea [PPDUMP_BAR_MAX_AREAS] = rect;
	if (m_nStyle & PPDUMP_FIELD_ADDRESS)
	{
		m_rFieldArea [PPDUMP_BAR_AREA_ADDRESS] = rect;
		m_rFieldArea [PPDUMP_BAR_AREA_ADDRESS].right = m_rFieldArea [PPDUMP_BAR_AREA_ADDRESS].left + m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] + 2);
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_ADDRESS].right;
	}

	UINT nTemp = 0;
	int nMaxChars = (rect.Width() - ::GetSystemMetrics(SM_CXVSCROLL)) / m_nWidthFont;
	double nWidth = rect.Width() - ::GetSystemMetrics(SM_CXVSCROLL);
	double nOneData = 0;

	if (m_nStyle & PPDUMP_FIELD_HEX)
	{
		nTemp += m_nCharsData [PPDUMP_BAR_AREA_HEX] + 1;
		nMaxChars --;
		nOneData += (m_nCharsData [PPDUMP_BAR_AREA_HEX] + 1) * m_nWidthFont;
		nWidth -= m_nWidthFont;
	}
	if (m_nStyle & PPDUMP_FIELD_DEC)
	{
		nTemp += m_nCharsData [PPDUMP_BAR_AREA_DEC] + 1;
		nMaxChars --;
		nOneData += (m_nCharsData [PPDUMP_BAR_AREA_DEC] + 1) * m_nWidthFont;
		nWidth -= m_nWidthFont;
	}
	if (m_nStyle & PPDUMP_FIELD_BIN)
	{
		nTemp += m_nCharsData [PPDUMP_BAR_AREA_BIN] + 1;
		nMaxChars --;
		nOneData += (m_nCharsData [PPDUMP_BAR_AREA_BIN] + 1) * m_nWidthFont;
		nWidth -= m_nWidthFont;
	}
	if (m_nStyle & PPDUMP_FIELD_OCT)
	{
		nTemp += m_nCharsData [PPDUMP_BAR_AREA_OCT] + 1;
		nMaxChars --;
		nOneData += (m_nCharsData [PPDUMP_BAR_AREA_OCT] + 1) * m_nWidthFont;
		nWidth -= m_nWidthFont;
	}
	if (m_nStyle & PPDUMP_FIELD_ASCII)
	{
		nTemp += m_nCharsData [PPDUMP_BAR_AREA_ASCII];
		nMaxChars -= 2;
		nOneData += (m_nCharsData [PPDUMP_BAR_AREA_ASCII]) * m_nWidthFont + 1;
		nWidth -= (m_nWidthFont * 2 - 1);
	}
	if (m_nStyle & PPDUMP_FIELD_SECTOR)
	{
		nTemp += m_nCharsData [PPDUMP_BAR_AREA_SECTOR] + 1;
		nMaxChars --;
		nOneData += (m_nCharsData [PPDUMP_BAR_AREA_SECTOR] + 1) * m_nWidthFont;
		nWidth -= m_nWidthFont;
	}
	if (m_nStyle & PPDUMP_FIELD_BLOCK)
	{
		nTemp += m_nCharsData [PPDUMP_BAR_AREA_BLOCK] + 1;
		nMaxChars --;
		nOneData += (m_nCharsData [PPDUMP_BAR_AREA_BLOCK] + 1) * m_nWidthFont;
		nWidth -= m_nWidthFont;
	}
	if ((nMaxChars <= 0) || (nTemp == 0))
	{
		m_nDataInLines = 1;
//		return;
	}

	m_nDataInLines = (int)(nWidth / nOneData);
    if (m_nDataInLines <= 0)
        m_nDataInLines = 1;
	
	if (m_nStyle & PPDUMP_FIELD_HEX)
	{
		m_rFieldArea [PPDUMP_BAR_AREA_HEX] = rect;
		rect.left += m_nWidthFont * (m_nDataInLines * (m_nCharsData [PPDUMP_BAR_AREA_HEX] + 1) + 1);
		m_rFieldArea [PPDUMP_BAR_AREA_HEX].right = rect.left;
	}
	if (m_nStyle & PPDUMP_FIELD_DEC)
	{
		m_rFieldArea [PPDUMP_BAR_AREA_DEC] = rect;
		rect.left += m_nWidthFont * (m_nDataInLines * (m_nCharsData [PPDUMP_BAR_AREA_DEC] + 1) + 1);
		m_rFieldArea [PPDUMP_BAR_AREA_DEC].right = rect.left;
	}
	if (m_nStyle & PPDUMP_FIELD_BIN)
	{
		m_rFieldArea [PPDUMP_BAR_AREA_BIN] = rect;
		rect.left += m_nWidthFont * (m_nDataInLines * (m_nCharsData [PPDUMP_BAR_AREA_BIN] + 1) + 1);
		m_rFieldArea [PPDUMP_BAR_AREA_BIN].right = rect.left;
	}
	if (m_nStyle & PPDUMP_FIELD_OCT)
	{
		m_rFieldArea [PPDUMP_BAR_AREA_OCT] = rect;
		rect.left += m_nWidthFont * (m_nDataInLines * (m_nCharsData [PPDUMP_BAR_AREA_OCT] + 1) + 1);
		m_rFieldArea [PPDUMP_BAR_AREA_OCT].right = rect.left;
	}
	if (m_nStyle & PPDUMP_FIELD_ASCII)
	{
		m_rFieldArea [PPDUMP_BAR_AREA_ASCII] = rect;
		rect.left += m_nWidthFont * (m_nDataInLines * m_nCharsData [PPDUMP_BAR_AREA_ASCII] + 2) + m_nDataInLines;
		m_rFieldArea [PPDUMP_BAR_AREA_ASCII].right = rect.left;
	}
	if (m_nStyle & PPDUMP_FIELD_SECTOR)
	{
		m_rFieldArea [PPDUMP_BAR_AREA_SECTOR] = rect;
		rect.left += m_nWidthFont * (m_nDataInLines * (m_nCharsData [PPDUMP_BAR_AREA_SECTOR] + 1) + 1);
		m_rFieldArea [PPDUMP_BAR_AREA_SECTOR].right = rect.left;
	}
	if (m_nStyle & PPDUMP_FIELD_BLOCK)
	{
		m_rFieldArea [PPDUMP_BAR_AREA_BLOCK] = rect;
		rect.left += m_nWidthFont * (m_nDataInLines * (m_nCharsData [PPDUMP_BAR_AREA_BLOCK] + 1) + 1);
		m_rFieldArea [PPDUMP_BAR_AREA_BLOCK].right = rect.left;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::UpdateControlBar  (public member function)
//    Update the control bar of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::UpdateControlBar(CDC * pDC /* = NULL */, BOOL bDisable /* = FALSE */, int nNewAddr /* = -1 */)
{
//	CClientDC dc(this);
	BOOL bReleaseDC = FALSE;
	if (pDC == NULL)
	{
		pDC = GetDC(); //&dc;
		bReleaseDC = TRUE;
	}

	pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	
	CFont * pFont = pDC->SelectObject(&m_font);
	pDC->SetBkMode(TRANSPARENT);

	INT_PTR nAddr;

	if (nNewAddr < 0)
	{
		if ((m_pEdit != NULL) && (m_nEditedArea < 0x10))
		{
				return;
		}
		else nAddr = GetAddressForControlBar();
	}
	else nAddr = nNewAddr;
	
	if (m_nStyle & PPDUMP_BAR_BLOCK)
		DrawAddressValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_BLOCK],
		m_pView->GetDocument()->MapFileOffsetToDeviceAddress(nAddr)/m_pView->GetDocument()->GetBlockSize(),
		TRUE, bDisable);
	if (m_nStyle & PPDUMP_BAR_SECTOR)
		DrawAddressValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_SECTOR],
		m_pView->GetDocument()->MapFileOffsetToDeviceAddress(nAddr)/m_pView->GetDocument()->GetRawSectorSize(),
		TRUE, bDisable);
	if (m_nStyle & PPDUMP_BAR_ADDRESS)
		DrawAddressValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_ADDRESS], nAddr + m_nOffsetAddress, TRUE, bDisable);
	
	// get current and previous values
	// don't do this before BLOCk, SECTOR or ADDRESS *clw*
	UINT nNewValue = 0;
    if (m_nStyle & ( PPDUMP_BAR_HEX | PPDUMP_BAR_DEC | PPDUMP_BAR_BIN | PPDUMP_BAR_OCT | PPDUMP_BAR_ASCII ) )
        nNewValue = GetDataFromCurrentAddress(nAddr, TRUE);
	
	if (m_nStyle & PPDUMP_BAR_HEX)
		DrawHexValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_HEX], nNewValue, TRUE, bDisable);
	if (m_nStyle & PPDUMP_BAR_DEC)
		DrawDecValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_DEC], nNewValue, TRUE, bDisable);
	if (m_nStyle & PPDUMP_BAR_BIN)
		DrawBinValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_BIN], nNewValue, TRUE, bDisable);
	if (m_nStyle & PPDUMP_BAR_OCT)
		DrawOctValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_OCT], nNewValue, TRUE, bDisable);
	if (m_nStyle & PPDUMP_BAR_ASCII)
		DrawAsciiValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_ASCII], nNewValue, TRUE, bDisable);


	pDC->SelectObject(pFont);

	if (bReleaseDC)
		ReleaseDC(pDC);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawControlBar  (public member function)
//    Draws the control bar of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//		rect		[in] - rect of the control
//
//  Returns :
//		rect.bottom		 - the bottom of the control bar
//
/////////////////////////////////////////////////////////////////////////////
int CPPDumpCtrl::DrawControlBar(CDC * pDC, CRect rect, BOOL bDisable /* = FALSE */)
{
	if (!(m_nStyle & PPDUMP_BAR_ALL))
		return rect.top;

	rect.bottom = CalculateControlBar(rect);

	//draws the background and the frame of the control bar
//	pDC->DrawFrameControl(rect, DFC_BUTTON, DFCS_BUTTONPUSH);
	pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));
	pDC->Draw3dRect(rect, ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW));
	m_rBarArea [PPDUMP_BAR_MAX_AREAS] = rect; //remember the rect of the control bar

	INT_PTR nAddr = GetAddressForControlBar();

	//get current and previous values
	UINT nNewValue = 0;
    if ( !( m_nStyle & (PPDUMP_BAR_BLOCK | PPDUMP_BAR_SECTOR) ) )
        nNewValue = GetDataFromCurrentAddress(nAddr);

	int nLength = 0;
	if (m_nStyle & PPDUMP_BAR_BLOCK)
	{
		INT_PTR block = m_pView->GetDocument()->MapFileOffsetToDeviceAddress(nAddr)/m_pView->GetDocument()->GetBlockSize();
		pDC->TextOut(m_rBarArea [PPDUMP_BAR_AREA_BLOCK].left - (m_csNameFields [PPDUMP_BAR_AREA_BLOCK].GetLength() + 1) * m_nWidthFont, 
			m_rBarArea [PPDUMP_BAR_AREA_BLOCK].top, m_csNameFields [PPDUMP_BAR_AREA_BLOCK] + _T(":"));
		DrawAddressValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_BLOCK], block, TRUE, bDisable);
	}

	if (m_nStyle & PPDUMP_BAR_SECTOR)
	{
	    INT_PTR sector = m_pView->GetDocument()->MapFileOffsetToDeviceAddress(nAddr)/m_pView->GetDocument()->GetRawSectorSize();
		pDC->TextOut(m_rBarArea [PPDUMP_BAR_AREA_SECTOR].left - (m_csNameFields [PPDUMP_BAR_AREA_SECTOR].GetLength() + 1) * m_nWidthFont, 
			m_rBarArea [PPDUMP_BAR_AREA_SECTOR].top, m_csNameFields [PPDUMP_BAR_AREA_SECTOR] + _T(":"));
		DrawAddressValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_SECTOR], sector, TRUE, bDisable);
	}

	if (m_nStyle & PPDUMP_BAR_ADDRESS)
	{
		pDC->TextOut(m_rBarArea [PPDUMP_BAR_AREA_ADDRESS].left - (m_csNameFields [PPDUMP_BAR_AREA_ADDRESS].GetLength() + 1) * m_nWidthFont, 
			m_rBarArea [PPDUMP_BAR_AREA_ADDRESS].top, m_csNameFields [PPDUMP_BAR_AREA_ADDRESS] + _T(":"));
		DrawAddressValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_ADDRESS], nAddr + m_nOffsetAddress, TRUE, bDisable);
	}

	if (m_nStyle & PPDUMP_BAR_HEX)
	{
		pDC->TextOut(m_rBarArea [PPDUMP_BAR_AREA_HEX].left - (m_csNameFields [PPDUMP_BAR_AREA_HEX].GetLength() + 1) * m_nWidthFont, 
			m_rBarArea [PPDUMP_BAR_AREA_HEX].top, m_csNameFields [PPDUMP_BAR_AREA_HEX] + _T(":"));
		DrawHexValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_HEX], nNewValue, TRUE, FALSE, bDisable);
	}

	if (m_nStyle & PPDUMP_BAR_DEC)
	{
		pDC->TextOut(m_rBarArea [PPDUMP_BAR_AREA_DEC].left - (m_csNameFields [PPDUMP_BAR_AREA_DEC].GetLength() + 1) * m_nWidthFont, 
			m_rBarArea [PPDUMP_BAR_AREA_DEC].top, m_csNameFields [PPDUMP_BAR_AREA_DEC] + _T(":"));
		DrawDecValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_DEC], nNewValue, TRUE, FALSE, bDisable);
	}

	if (m_nStyle & PPDUMP_BAR_BIN)
	{
		pDC->TextOut(m_rBarArea [PPDUMP_BAR_AREA_BIN].left - (m_csNameFields [PPDUMP_BAR_AREA_BIN].GetLength() + 1) * m_nWidthFont, 
			m_rBarArea [PPDUMP_BAR_AREA_BIN].top, m_csNameFields [PPDUMP_BAR_AREA_BIN] + _T(":"));
		DrawBinValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_BIN], nNewValue, TRUE, FALSE, bDisable);
	}

	if (m_nStyle & PPDUMP_BAR_OCT)
	{
		pDC->TextOut(m_rBarArea [PPDUMP_BAR_AREA_OCT].left - (m_csNameFields [PPDUMP_BAR_AREA_OCT].GetLength() + 1) * m_nWidthFont, 
			m_rBarArea [PPDUMP_BAR_AREA_OCT].top, m_csNameFields [PPDUMP_BAR_AREA_OCT] + _T(":"));
		rect.left = DrawOctValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_OCT], nNewValue, TRUE, FALSE, bDisable);
	}

	if (m_nStyle & PPDUMP_BAR_ASCII)
	{
		pDC->TextOut(m_rBarArea [PPDUMP_BAR_AREA_ASCII].left - (m_csNameFields [PPDUMP_BAR_AREA_ASCII].GetLength() + 1) * m_nWidthFont, 
			m_rBarArea [PPDUMP_BAR_AREA_ASCII].top, m_csNameFields [PPDUMP_BAR_AREA_ASCII] + _T(":"));
		rect.left = DrawAsciiValue(pDC, m_rBarArea [PPDUMP_BAR_AREA_ASCII], nNewValue, TRUE, FALSE, bDisable);
	}

	return m_rBarArea [PPDUMP_BAR_MAX_AREAS].bottom;
}

int CPPDumpCtrl::CalculateControlBar(CRect rect)
{
	int nLength = 0;
	
	rect.left += m_nWidthFont;
	rect.right -= m_nWidthFont;
	rect.top += m_nHeightFont / 6;
	rect.bottom = rect.top + m_nHeightFont;

	CRect rBar = rect;

	for (int i = 0; i < PPDUMP_BAR_MAX_AREAS; i++)
		m_rBarArea [i].SetRectEmpty();

	if (m_nStyle & PPDUMP_BAR_BLOCK)
	{
		nLength = m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] + 3 + m_csNameFields [PPDUMP_BAR_AREA_BLOCK].GetLength());
		if (rBar.Width() > nLength)
		{
			if ((rect.left + nLength) > rect.right)
			{
				rect.left = rBar.left;
				rect.top += m_nHeightFont;
				rect.bottom += m_nHeightFont;
			}
			m_rBarArea [PPDUMP_BAR_AREA_BLOCK].right = rect.left + nLength;
			m_rBarArea [PPDUMP_BAR_AREA_BLOCK].left = m_rBarArea [PPDUMP_BAR_AREA_BLOCK].right - m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] + 2);
			m_rBarArea [PPDUMP_BAR_AREA_BLOCK].top = rect.top;
			m_rBarArea [PPDUMP_BAR_AREA_BLOCK].bottom = rect.bottom - m_nHeightFont / 6;
			rect.left = m_rBarArea [PPDUMP_BAR_AREA_BLOCK].right + m_nWidthFont;
		}
	}

	if (m_nStyle & PPDUMP_BAR_SECTOR)
	{
		nLength = m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] + 3 + m_csNameFields [PPDUMP_BAR_AREA_SECTOR].GetLength());
		if (rBar.Width() > nLength)
		{
			if ((rect.left + nLength) > rect.right)
			{
				rect.left = rBar.left;
				rect.top += m_nHeightFont;
				rect.bottom += m_nHeightFont;
			}
			m_rBarArea [PPDUMP_BAR_AREA_SECTOR].right = rect.left + nLength;
			m_rBarArea [PPDUMP_BAR_AREA_SECTOR].left = m_rBarArea [PPDUMP_BAR_AREA_SECTOR].right - m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] + 2);
			m_rBarArea [PPDUMP_BAR_AREA_SECTOR].top = rect.top;
			m_rBarArea [PPDUMP_BAR_AREA_SECTOR].bottom = rect.bottom - m_nHeightFont / 6;
			rect.left = m_rBarArea [PPDUMP_BAR_AREA_SECTOR].right + m_nWidthFont;
		}
	}

	if (m_nStyle & PPDUMP_BAR_ADDRESS)
	{
		nLength = m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] + 3 + m_csNameFields [PPDUMP_BAR_AREA_ADDRESS].GetLength());
		if (rBar.Width() > nLength)
		{
			if ((rect.left + nLength) > rect.right)
			{
				rect.left = rBar.left;
				rect.top += m_nHeightFont;
				rect.bottom += m_nHeightFont;
			}
			m_rBarArea [PPDUMP_BAR_AREA_ADDRESS].right = rect.left + nLength;
			m_rBarArea [PPDUMP_BAR_AREA_ADDRESS].left = m_rBarArea [PPDUMP_BAR_AREA_ADDRESS].right - m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] + 2);
			m_rBarArea [PPDUMP_BAR_AREA_ADDRESS].top = rect.top;
			m_rBarArea [PPDUMP_BAR_AREA_ADDRESS].bottom = rect.bottom - m_nHeightFont / 6;
			rect.left = m_rBarArea [PPDUMP_BAR_AREA_ADDRESS].right + m_nWidthFont;
		}
	}

	if (m_nStyle & PPDUMP_BAR_HEX)
	{
		nLength = m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_HEX] + 3 + m_csNameFields [PPDUMP_BAR_AREA_HEX].GetLength());
		if (rBar.Width() > nLength)
		{
			if ((rect.left + nLength) > rect.right)
			{
				rect.left = rBar.left;
				rect.top += m_nHeightFont;
				rect.bottom += m_nHeightFont;
			}
			m_rBarArea [PPDUMP_BAR_AREA_HEX].right = rect.left + nLength;
			m_rBarArea [PPDUMP_BAR_AREA_HEX].left = m_rBarArea [PPDUMP_BAR_AREA_HEX].right - m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_HEX] + 2);
			m_rBarArea [PPDUMP_BAR_AREA_HEX].top = rect.top;
			m_rBarArea [PPDUMP_BAR_AREA_HEX].bottom = rect.bottom - m_nHeightFont / 6;
			rect.left = m_rBarArea [PPDUMP_BAR_AREA_HEX].right + m_nWidthFont;
		}
	}

	if (m_nStyle & PPDUMP_BAR_DEC)
	{
		nLength = m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_DEC] + 3 + m_csNameFields [PPDUMP_BAR_AREA_DEC].GetLength());
		if (rBar.Width() > nLength)
		{
			if ((rect.left + nLength) > rect.right)
			{
				rect.left = rBar.left;
				rect.top += m_nHeightFont;
				rect.bottom += m_nHeightFont;
			}
			m_rBarArea [PPDUMP_BAR_AREA_DEC].right = rect.left + nLength;
			m_rBarArea [PPDUMP_BAR_AREA_DEC].left = m_rBarArea [PPDUMP_BAR_AREA_DEC].right - m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_DEC] + 2);
			m_rBarArea [PPDUMP_BAR_AREA_DEC].top = rect.top;
			m_rBarArea [PPDUMP_BAR_AREA_DEC].bottom = rect.bottom - m_nHeightFont / 6;
			rect.left = m_rBarArea [PPDUMP_BAR_AREA_DEC].right + m_nWidthFont;
		}
	}

	if (m_nStyle & PPDUMP_BAR_BIN)
	{
		nLength = m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_BIN] + 3 + m_csNameFields [PPDUMP_BAR_AREA_BIN].GetLength());
		if (rBar.Width() > nLength)
		{
			if ((rect.left + nLength) > rect.right)
			{
				rect.left = rBar.left;
				rect.top += m_nHeightFont;
				rect.bottom += m_nHeightFont;
			}
			m_rBarArea [PPDUMP_BAR_AREA_BIN].right = rect.left + nLength;
			m_rBarArea [PPDUMP_BAR_AREA_BIN].left = m_rBarArea [PPDUMP_BAR_AREA_BIN].right - m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_BIN] + 2);
			m_rBarArea [PPDUMP_BAR_AREA_BIN].top = rect.top;
			m_rBarArea [PPDUMP_BAR_AREA_BIN].bottom = rect.bottom - m_nHeightFont / 6;
			rect.left = m_rBarArea [PPDUMP_BAR_AREA_BIN].right + m_nWidthFont;
		}
	}

	if (m_nStyle & PPDUMP_BAR_OCT)
	{
		nLength = m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_OCT] + 3 + m_csNameFields [PPDUMP_BAR_AREA_OCT].GetLength());
		if (rBar.Width() > nLength)
		{
			if ((rect.left + nLength) > rect.right)
			{
				rect.left = rBar.left;
				rect.top += m_nHeightFont;
				rect.bottom += m_nHeightFont;
			}
			m_rBarArea [PPDUMP_BAR_AREA_OCT].right = rect.left + nLength;
			m_rBarArea [PPDUMP_BAR_AREA_OCT].left = m_rBarArea [PPDUMP_BAR_AREA_OCT].right - m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_OCT] + 2);
			m_rBarArea [PPDUMP_BAR_AREA_OCT].top = rect.top;
			m_rBarArea [PPDUMP_BAR_AREA_OCT].bottom = rect.bottom - m_nHeightFont / 6;
			rect.left = m_rBarArea [PPDUMP_BAR_AREA_OCT].right + m_nWidthFont;
		}
	}

	if (m_nStyle & PPDUMP_BAR_ASCII)
	{
		nLength = m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ASCII] + 3 + m_csNameFields [PPDUMP_BAR_AREA_ASCII].GetLength());
		if (rBar.Width() > nLength)
		{
			if ((rect.left + nLength) > rect.right)
			{
				rect.left = rBar.left;
				rect.top += m_nHeightFont;
				rect.bottom += m_nHeightFont;
			}
			m_rBarArea [PPDUMP_BAR_AREA_ASCII].right = rect.left + nLength;
			m_rBarArea [PPDUMP_BAR_AREA_ASCII].left = m_rBarArea [PPDUMP_BAR_AREA_ASCII].right - m_nWidthFont * (m_nCharsData [PPDUMP_BAR_AREA_ASCII] + 2);
			m_rBarArea [PPDUMP_BAR_AREA_ASCII].top = rect.top;
			m_rBarArea [PPDUMP_BAR_AREA_ASCII].bottom = rect.bottom - m_nHeightFont / 6;
			rect.left = m_rBarArea [PPDUMP_BAR_AREA_ASCII].right + m_nWidthFont;
		}
	}
	return rect.bottom;
}

int CPPDumpCtrl::DrawAddressValue(CDC * pDC, CRect rect, UINT nAddress, BOOL bBkgnd /* = FALSE */, BOOL bDisable /* = FALSE */)
{
	int nLength = m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] / 2;
	if (m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] & 0x1)
		nLength ++;

	CString str = FormatingString(PPDUMP_BAR_AREA_ADDRESS, m_pDoc->MapFileOffsetToDeviceAddress(nAddress), nLength);
	str = str.Right(m_nCharsData [PPDUMP_BAR_AREA_ADDRESS]);
	str = (TCHAR)' ' + str + (TCHAR)' ';
	
	return DrawStringValue(pDC, rect, str, bBkgnd, FALSE, bDisable);
}

int CPPDumpCtrl::DrawHexValue(CDC * pDC, CRect rect, UINT nValue, BOOL bBkgnd /* = FALSE */, BOOL bCaret /* = FALSE */, BOOL bDisable /* = FALSE */)
{
	int nLength = m_nCharsData [PPDUMP_BAR_AREA_HEX] / 2;
	if (m_nCharsData [PPDUMP_BAR_AREA_HEX] & 0x1)
		nLength ++;

	CString str = FormatingString(PPDUMP_BAR_AREA_HEX, nValue, nLength);
	str = (TCHAR)' ' + str + (TCHAR)' ';
	
	return DrawStringValue(pDC, rect, str, bBkgnd, bCaret, bDisable);
}

int CPPDumpCtrl::DrawDecValue(CDC * pDC, CRect rect, UINT nValue, BOOL bBkgnd /* = FALSE */, BOOL bCaret /* = FALSE */, BOOL bDisable /* = FALSE */)
{
	CString str = FormatingString(PPDUMP_BAR_AREA_DEC, nValue);

	str = _T("        ") + str;
	str += (TCHAR)' ';
	str = str.Right(m_nCharsData [PPDUMP_BAR_AREA_DEC] + 2);

	return DrawStringValue(pDC, rect, str, bBkgnd, bCaret, bDisable);
}

int CPPDumpCtrl::DrawOctValue(CDC * pDC, CRect rect, UINT nValue, BOOL bBkgnd /* = FALSE */, BOOL bCaret /* = FALSE */, BOOL bDisable /* = FALSE */)
{
	CString str = FormatingString(PPDUMP_BAR_AREA_OCT, nValue);

	str = _T("        ") + str;
	str += (TCHAR)' ';
	str = str.Right(m_nCharsData [PPDUMP_BAR_AREA_OCT] + 2);
	
	return DrawStringValue(pDC, rect, str, bBkgnd, bCaret, bDisable);
}

int CPPDumpCtrl::DrawBinValue(CDC * pDC, CRect rect, UINT nValue, BOOL bBkgnd /* = FALSE */, BOOL bCaret /* = FALSE */, BOOL bDisable /* = FALSE */)
{
	CString str = FormatingString(PPDUMP_BAR_AREA_BIN, nValue);
	str = (TCHAR)' ' + str + (TCHAR)' ';
	
	return DrawStringValue(pDC, rect, str, bBkgnd, bCaret, bDisable);
}

int CPPDumpCtrl::DrawAsciiValue(CDC * pDC, CRect rect, UINT nValue, BOOL bBkgnd /* = FALSE */, BOOL bCaret /* = FALSE */, BOOL bDisable /* = FALSE */)
{
	CString str = FormatingString(PPDUMP_BAR_AREA_ASCII, nValue);
	str = (TCHAR)' ' + str + (TCHAR)' ';

	CSize size = pDC->GetTextExtent(str);
	rect.right = rect.left + size.cx + 1;
	
	if (!bDisable)
	{
		if (bBkgnd)
		{
			pDC->FillSolidRect(rect, m_crColor [PPDUMP_COLOR_DATA_BK]);
		}
		
		if (bCaret)
		{
			CRect rCaret = rect;
			rCaret.bottom = rCaret.top + m_nHeightFont;
			rCaret.DeflateRect(m_nWidthFont, 1, m_nWidthFont + 1, 1);
			pDC->FillSolidRect(rCaret, m_crColor [PPDUMP_COLOR_CARET_BK]);
		}
	}

	pDC->TextOut(rect.left, rect.top, str);

	return rect.right;
}

int CPPDumpCtrl::DrawStringValue(CDC * pDC, CRect rect, CString str, BOOL bBkgnd /* = FALSE */, BOOL bCaret /* = TRUE */, BOOL bDisable /* = FALSE */)
{
	CSize size = pDC->GetTextExtent(str);
	rect.right = rect.left + size.cx;
	
	if (!bDisable)
	{
		if (bBkgnd)
			pDC->FillSolidRect(rect, m_crColor [PPDUMP_COLOR_DATA_BK]);
		
		if (bCaret)
		{
			CRect rCaret = rect;
			rCaret.bottom = rCaret.top + m_nHeightFont;
			rCaret.DeflateRect(m_nWidthFont / 2 + 1, 1, m_nWidthFont / 2 + 2, 1);
			pDC->FillSolidRect(rCaret, m_crColor [PPDUMP_COLOR_CARET_BK]);
		}
	}

	pDC->TextOut(rect.left, rect.top, str);

	return rect.right;
}

int CPPDumpCtrl::DrawSolidBox(CDC * pDC, CRect rect, COLORREF clr, BOOL bDisable /* = FALSE */)
{
	CSize size = pDC->GetTextExtent(_T("    "));
	rect.right = rect.left + size.cx;
	
	if (!bDisable)
	{
		CRect rCaret = rect;
		rCaret.bottom = rCaret.top + m_nHeightFont;
		rCaret.DeflateRect(m_nWidthFont / 2 + 1, 1, m_nWidthFont / 2 + 1, 1);
		pDC->FillSolidRect(rCaret, clr);
	}

	return rect.right;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawStatusBar  (public member function)
//    Draws the status bar of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//		rect		[in] - rect of the control
//
//  Returns :
//		rect.bottom		 - the top of the control bar
//
/////////////////////////////////////////////////////////////////////////////
int CPPDumpCtrl::DrawStatusBar(CDC * pDC, CRect rect, BOOL bDisable /* = FALSE */)
{
	if (!(m_nStyle & PPDUMP_NAMED_FIELDS))
		return rect.top;

	rect.bottom = rect.top + m_nHeightFont;
	
	//draws the background and the frame of the control bar
	pDC->FillSolidRect(rect, bDisable ? m_crDisableBk : m_crColor [PPDUMP_COLOR_DATA_BK]);
	pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_TEXT_HEADER]);
	CPen pen (PS_SOLID, 1, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);
	pDC->MoveTo(rect.left, rect.bottom);
	pDC->LineTo(rect.right, rect.bottom);

	if (m_nStyle & PPDUMP_FIELD_ADDRESS)
	{
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_ADDRESS].left;
		rect.right = m_rFieldArea [PPDUMP_BAR_AREA_ADDRESS].right;
		pDC->DrawText(m_csNameFields [PPDUMP_BAR_AREA_ADDRESS], rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	if (m_nStyle & PPDUMP_FIELD_HEX)
	{
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_HEX].left;
		rect.right = m_rFieldArea [PPDUMP_BAR_AREA_HEX].right;
		pDC->DrawText(m_csNameFields [PPDUMP_BAR_AREA_HEX], rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	if (m_nStyle & PPDUMP_FIELD_DEC)
	{
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_DEC].left;
		rect.right = m_rFieldArea [PPDUMP_BAR_AREA_DEC].right;
		pDC->DrawText(m_csNameFields [PPDUMP_BAR_AREA_DEC], rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	if (m_nStyle & PPDUMP_FIELD_BIN)
	{
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_BIN].left;
		rect.right = m_rFieldArea [PPDUMP_BAR_AREA_BIN].right;
		pDC->DrawText(m_csNameFields [PPDUMP_BAR_AREA_BIN], rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	if (m_nStyle & PPDUMP_FIELD_OCT)
	{
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_OCT].left;
		rect.right = m_rFieldArea [PPDUMP_BAR_AREA_OCT].right;
		pDC->DrawText(m_csNameFields [PPDUMP_BAR_AREA_OCT], rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	if (m_nStyle & PPDUMP_FIELD_ASCII)
	{
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_ASCII].left;
		rect.right = m_rFieldArea [PPDUMP_BAR_AREA_ASCII].right;
		pDC->DrawText(m_csNameFields [PPDUMP_BAR_AREA_ASCII], rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	if (m_nStyle & PPDUMP_FIELD_SECTOR)
	{
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_SECTOR].left;
		rect.right = m_rFieldArea [PPDUMP_BAR_AREA_SECTOR].right;
		pDC->DrawText(m_csNameFields [PPDUMP_BAR_AREA_SECTOR], rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

    	if (m_nStyle & PPDUMP_FIELD_BLOCK)
	{
		rect.left = m_rFieldArea [PPDUMP_BAR_AREA_BLOCK].left;
		rect.right = m_rFieldArea [PPDUMP_BAR_AREA_BLOCK].right;
		pDC->DrawText(m_csNameFields [PPDUMP_BAR_AREA_BLOCK], rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	pDC->SelectObject(penOld);

	return rect.bottom;
}

///////////////////////////////////////////////////////////////////
// CPPDumpCtrl::RecalculateWorkData()
// Recalculates working length of the data depending on their type
///////////////////////////////////////////////////////////////////
void CPPDumpCtrl::RecalculateWorkData(DWORD nNewStyle)
{
	if ((nNewStyle ^ m_nStyle) & PPDUMP_WORD_DATA)
		m_nBeginAddress = 0;

	m_nLengthData = (nNewStyle & PPDUMP_WORD_DATA) ? m_nRealLengthData / 2 : m_nRealLengthData;
} //End of RecalculateWorkData

void  CPPDumpCtrl::SetDataWordSize(INT_PTR _size, BOOL bRedraw /*TRUE*/)  // byte-size
{ 
    m_nDataWordSize = _size;
    SetCharsInData();

    SetSelectRange(m_nBeginAddress, -1, FALSE); //reset the selection block

	if (bRedraw)
	{
		// Redraw control
		Invalidate(FALSE);
	}

};
///////////////////////////////////////////////////////////////////
// CPPDumpCtrl::SetCharsInData()
// Recalculate the sizes of other fields
///////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetCharsInData()
{
	//Calculate the length of the address fields
	m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] = 1;
	m_nCharsData [PPDUMP_BAR_AREA_SECTOR] = 1;
	m_nCharsData [PPDUMP_BAR_AREA_BLOCK] = 1;
	UINT nLength = (m_nOffsetAddress + m_nLengthData - 1) >> 4;
	if (nLength)
	{
		while (nLength)
		{
			m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] ++;
			// TODO: *clw* fix the following hard-coded hacks
			if (m_nStyle & PPDUMP_BAR_SECTOR)
				if ( nLength / 528 )
					m_nCharsData [PPDUMP_BAR_AREA_SECTOR] ++;
			if (m_nStyle & PPDUMP_BAR_BLOCK)
				if ( nLength / (528 * 256) )
					m_nCharsData [PPDUMP_BAR_AREA_BLOCK] ++;
			nLength >>= 4;
			
		}
	}

	m_nCharsData [PPDUMP_BAR_AREA_ADDRESS] = 8;
	m_nCharsData [PPDUMP_BAR_AREA_SECTOR] = 6;
	m_nCharsData [PPDUMP_BAR_AREA_BLOCK] = 4;

    //Customize the length of the fields
	m_nCharsData [PPDUMP_BAR_AREA_ASCII] = 1; //the ascii field - 1 chars always
	m_nCharsData [PPDUMP_BAR_AREA_SECTOR] = 2; //the sector field - 2 chars always
	m_nCharsData [PPDUMP_BAR_AREA_BLOCK] = 2; //the block field - 2 chars always
	if (m_nStyle & PPDUMP_WORD_DATA)
	{
		m_nCharsData [PPDUMP_BAR_AREA_DEC] = 5;
		m_nCharsData [PPDUMP_BAR_AREA_HEX] = 4;
		m_nCharsData [PPDUMP_BAR_AREA_BIN] = 17;
		m_nCharsData [PPDUMP_BAR_AREA_OCT] = 6;
	}
	else
	{
		m_nCharsData [PPDUMP_BAR_AREA_ASCII] = 1 * m_nDataWordSize;
		m_nCharsData [PPDUMP_BAR_AREA_DEC] = 3 * m_nDataWordSize;
		m_nCharsData [PPDUMP_BAR_AREA_HEX] = 2 * m_nDataWordSize;
		m_nCharsData [PPDUMP_BAR_AREA_BIN] = ((2+8) * m_nDataWordSize)-1;
		m_nCharsData [PPDUMP_BAR_AREA_OCT] = 3 * m_nDataWordSize;
	}
} //End of SetCharsInData

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::GetDataFromCurrentAddress (protected member function)
//    Gets data specified by address from array
//
//  Parameters :
//		nAddress    [in] - the address into array
//		bNewData	[in] - if TRUE then data gets from pNewData array, else from pOldData array
//		nDir	    [in] - the direction reading data
//
//  Returns :
//		the data from specified address
//
/////////////////////////////////////////////////////////////////////////////
DWORD CPPDumpCtrl::GetDataFromCurrentAddress(DWORD nAddress, BOOL bNewData /* = TRUE */, int nDir, int nByte)
{
	BYTE * pArray = bNewData ? m_pNewData : m_pOldData;
	
//	if (pArray == NULL)
//		return 0;
	
	//Customize the length of the data
	if (!nByte)
		nByte = m_nDataWordSize;

	//Customize the direction reading data
	if (!nDir)
		nDir = m_nDataEndian;

	if (nDir == 2)
		nAddress += (nByte - 1);

	DWORD nData = 0;
	for (int i = 0; i < nByte; i++)
	{
		nData <<= 8;
//		nData += *(pArray + nAddress);
		nData += (*m_pDoc)[nAddress];
        nAddress = (nDir == 2) ? nAddress - 1 : nAddress + 1;
	}

	return nData;
} // end of GetDataFromCurrentAddress

//***************************************************************************//
//* Public methods
//***************************************************************************//

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetFont  (public member function)
//    Sets the new font to the control
//
//  Parameters :
//		font		[in] - new font
//		bRedraw		[in] - redraw control
//
//  Returns :
//		Nonzero if successful; otherwise 0.
//
/////////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::SetFont(CFont & font, BOOL bRedraw /* = TRUE */)
{
	LOGFONT lf;
	font.GetLogFont (&lf);

	return SetFont(lf, bRedraw);;
}  // End of SetFont

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetFont  (public member function)
//    Sets the new font to the control
//
//  Parameters :
//		lf			[in] - structure LOGFONT for the new font
//		bRedraw		[in] - redraw control
//
//  Returns :
//		Nonzero if successful; otherwise 0.
//
/////////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::SetFont(LOGFONT & lf, BOOL bRedraw /* = TRUE */)
{
	KillEdit();

	if (NULL != (HFONT)m_font) 
		m_font.DeleteObject();

	BOOL bReturn = m_font.CreateFontIndirect (&lf);
	
	if (bRedraw)
	{
		// Redraw control
		Invalidate(FALSE);
	}

	return bReturn;
}  // End of SetFont

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetFont  (public member function)
//    Sets the new font to the control
//
//  Parameters :
//		font		[in] - new font
//		bRedraw		[in] - redraw control
//
//  Returns :
//		Nonzero if successful; otherwise 0.
//
/////////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::SetFont(LPCTSTR lpszFaceName, int nSizePoints /* = 8 */,
									BOOL bUnderline /* = FALSE */, BOOL bBold /* = FALSE */,
									BOOL bStrikeOut /* = FALSE */, BOOL bItalic /* = FALSE */, 
									BOOL bRedraw /* = TRUE */)
{
	CDC* pDC = GetDC ();
	LOGFONT lf;
	memset (&lf, 0, sizeof(LOGFONT));

	_tcscpy (lf.lfFaceName, lpszFaceName);
	lf.lfHeight = -MulDiv (nSizePoints, GetDeviceCaps (pDC->m_hDC, LOGPIXELSY), 72);
	lf.lfUnderline = bUnderline;
	if (TRUE == bBold) 
	{
		lf.lfWeight = FW_BOLD;
	}
	lf.lfStrikeOut = bStrikeOut;
	lf.lfItalic = bItalic;

	if (pDC)
		ReleaseDC(pDC);

	return SetFont(lf, bRedraw);
}  // End of SetFont

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetDefaultFonts  (public member function)
//    Sets default fonts of the control
//
//  Parameters :
//		bRedraw		[in] - redraw control
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetDefaultFont(BOOL bRedraw /* = TRUE */)
{
	SetFont(_T("Courier"), 8, FALSE, FALSE, FALSE, FALSE, bRedraw);
} // End of SetDefaultFonts

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetPointerData  (public member function)
//    Sets pointers to the new data array and the old data array
//
//  Parameters :
//		nLenght		[in] - lenght data array
//		pNewData	[in] - pointer to the new data array
//		pOldData	[in] - pointer to the old data array. If NULL, then pOldData = pNewData
//		bRedraw		[in] - redraw control
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetPointerData(DWORD nLength, LPBYTE pNewData, CSTMPDoc* pDoc /* = NULL */, BOOL bRedraw /* = TRUE */)
{
	KillEdit();

	//Sets the pointers to the data arrays
	m_pNewData = pNewData;
//	if (NULL == pOldData)
//		m_pOldData = pNewData;
//	else
//		m_pOldData = pOldData;
    m_pDoc = pDoc;

	m_nRealLengthData = nLength; //remember the real length of array
	m_nBeginAddress = 0;
	m_nLengthData = (m_nStyle & PPDUMP_WORD_DATA) ? nLength / 2 : nLength;

	SetCharsInData(); //recalculates the sizes of other fields
	SetBeginAddress(0, FALSE); //sets the first address on the screen
	SetSelectRange(m_nBeginAddress, -1, FALSE); //reset the selection block

	if (bRedraw)
	{
		// Redraw control
		Invalidate(FALSE);
	}
} // End of SetPointerData

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetOffsetViewAddress  (public member function)
//    Sets address which will add to the real address data array for view
//
//  Parameters :
//		nAddress	[in] - offset address
//		bRedraw		[in] - redraw control
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetOffsetViewAddress(int nAddress /* = 0 */, BOOL bRedraw /* = TRUE */)
{
	KillEdit();

	if (nAddress < 0)
		nAddress = 0;

	m_nOffsetAddress = nAddress;
	SetCharsInData(); //recalculates the sizes of other fields
	SetSelectRange(m_nBeginAddress, -1, FALSE); //reset the selection block
	
	if (bRedraw)
	{
		// Redraw control
		Invalidate(FALSE);
	}
} // End of SetOffsetViewAddress

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetStyles  (public member function)
//    Sets the new styles of the control
//
//  Parameters :
//		nStyle		[in] - new style
//		bRedraw		[in] - redraw control
//
//  Returns :
//		Old styles
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetStyles(DWORD nStyle, BOOL bRedraw /* = TRUE */)
{
	ModifyStyles(nStyle, -1, bRedraw);
}  // End of SetStyles

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::ModifyStyles  (public member function)
//    Modifify the styles of the control
//
//  Parameters :
//		nAddStyle	 [in] - The styles to add
//		nRemoveStyle [in] - The styles to remove
//		bRedraw		 [in] - redraw control
//
//  Returns :
//		Old styles
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::ModifyStyles(DWORD nAddStyle, DWORD nRemoveStyle, BOOL bRedraw /* = TRUE */)
{
	KillEdit();

	DWORD nTemp = m_nStyle;

	nTemp &= ~nRemoveStyle;
	nTemp |= nAddStyle;

	//
	RecalculateWorkData(nTemp);
	m_nStyle = nTemp;
	
	//Re-sets how much chars in the data of the fields
	SetCharsInData();
	SetBeginAddress(0, FALSE);
	SetSelectRange(m_nBeginAddress, -1, FALSE);
	
	if (bRedraw)
	{
		// Redraw control
		Invalidate(FALSE);
	}
}  // End of ModifyStyles

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::GetStyles (public member function)
//    Gets the current styles of the control
//
//  Parameters :
//
//  Returns :
//		Current styles
//
/////////////////////////////////////////////////////////////////////////////
DWORD CPPDumpCtrl::GetStyles()
{
	return m_nStyle;
}  // End of GetStyles

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetDefaultStyles  (public member function)
//    Sets the new styles of the control
//
//  Parameters :
//		bRedraw		[in] - redraw control
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetDefaultStyles(BOOL bRedraw /* = TRUE */)
{
	ModifyStyles(PPDUMP_FIELD_ADDRESS | PPDUMP_FIELD_HEX | PPDUMP_FIELD_ASCII |
			     PPDUMP_BAR_ADDRESS | PPDUMP_BAR_DEC | PPDUMP_BAR_HEX | PPDUMP_BAR_BIN |
			     PPDUMP_BAR_ASCII | PPDUMP_TRACK_MOUSE_MOVE,
				 -1, bRedraw);
}  // End of SetDefaultStyles

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetDataBlockSize  (public member function)
//    Sets the size of the horizontal data break lines and turns them on/off
//
//  Parameters :
//		nSize       [in] - the amount of data in a section
//		bActivate	[in] - turn on/off the data sizing
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetDataBlockSize(INT_PTR nSize /* = 0 */, INT_PTR nRSize /* = 0 */, BOOL bActivate /* = TRUE */)
{
	if (nSize < 0)
		nSize = 0;

	if ((nSize >= m_nLengthData) || !m_nDataInLines)
		return;

    m_nDataBlockSize = nSize;
	m_nRedundantSize = nRSize;

    if (bActivate && nSize)
	{
		// add style and Redraw control
		ModifyStyles(PPDUMP_DATA_BLOCKS, 0, TRUE);
	}
    else
    {
		// remove style and Redraw control
		ModifyStyles(0, PPDUMP_DATA_BLOCKS, TRUE);
    }
}  // End of SetDataBlockSize

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::GetDataDataBlockSize  (public member function)
//    Gets the size of the data sections
//
//  Parameters :
//		None
//
//  Returns :
//		Gets the size of the data sections
//
/////////////////////////////////////////////////////////////////////////////
INT_PTR CPPDumpCtrl::GetDataBlockSize(void)
{
	return m_nDataBlockSize;
}  // End of GetDataBlockSize

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetSpecialCharView  (public member function)
//    Sets the character which will drawn instead special characters (0 - 31)
//
//  Parameters :
//		chSymbol	[in] - the character which will draw instead the special characters (0 - 31)
//		bRedraw		[in] - redraw control
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetSpecialCharView(TCHAR chSymbol /* = NULL */, BOOL bRedraw /* = TRUE */)
{
	m_chSpecCharView = chSymbol;
	if (bRedraw)
	{
		// Redraw control
		Invalidate(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetBeginAddress  (public member function)
//    Sets the new styles of the control
//
//  Parameters :
//		nAddress    [in] - the first viewing address
//		bRedraw		[in] - redraw control
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetBeginAddress(int nAddress /* = 0 */, BOOL bRedraw /* = TRUE */)
{
	if (nAddress < 0)
		nAddress = 0;

	if ((nAddress >= m_nLengthData) || !m_nDataInLines)
		return;

	KillEdit();

//    m_nBeginAddress = (nAddress / (m_nDataInLines * m_nDataWordSize)) * (m_nDataInLines * m_nDataWordSize);
    m_nBeginAddress = GetScrollAddress(GetAddressRow(nAddress));
    if (bRedraw)
	{
		// Redraw control
		Invalidate(FALSE);
	}
}  // End of SetBeginAddress

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::GetBeginAddress  (public member function)
//    Gets the first viewing address
//
//  Parameters :
//		None
//
//  Returns :
//		The first viewing address on the screen
//
/////////////////////////////////////////////////////////////////////////////
int CPPDumpCtrl::GetBeginAddress()
{
	return m_nBeginAddress;
}  // End of GetBeginAddress

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetColor (public member function)
//    Set the color
//
//  Parameters :
//		nIndex  [in] - index of the color
//		crColor [in] - new color
//		bRedraw [in] - redraw control
//
//  Returns :
//		Previous color
//
/////////////////////////////////////////////////////////////////////////////
COLORREF CPPDumpCtrl::SetColor(int nIndex, COLORREF crColor, BOOL bRedraw /* = TRUE */)
{
	if (nIndex >= PPDUMP_COLOR_MAX)
		return RGB (0, 0, 0);

	COLORREF crReturn = m_crColor [nIndex];
	m_crColor [nIndex] = crColor;
	
	if (m_pEdit != NULL)
	{
		switch (nIndex)
		{
		case PPDUMP_COLOR_EDIT_FG:
			m_pEdit->SetColor(CPPNumEdit::PPNUM_COLOR_VALID_FG, crColor, bRedraw);
			bRedraw = FALSE;
			break;
		case PPDUMP_COLOR_EDIT_BK:
			m_pEdit->SetColor(CPPNumEdit::PPNUM_COLOR_VALID_BK, crColor, bRedraw);
			bRedraw = FALSE;
			break;
		case PPDUMP_COLOR_EDIT_ERR_FG:
			m_pEdit->SetColor(CPPNumEdit::PPNUM_COLOR_NOT_VALID_FG, crColor, bRedraw);
			bRedraw = FALSE;
			break;
		case PPDUMP_COLOR_EDIT_ERR_BK:
			m_pEdit->SetColor(CPPNumEdit::PPNUM_COLOR_NOT_VALID_BK, crColor, bRedraw);
			bRedraw = FALSE;
			break;
		}
	}

	if (bRedraw)
	{
		// Redraw control
		Invalidate(FALSE);
	}

	return crReturn;
}  // End of SetColor

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::GetColor (public member function)
//    Set the color
//
//  Parameters :
//		nIndex  [in] - index of the color
//
//  Returns :
//		Current color
//
/////////////////////////////////////////////////////////////////////////////
COLORREF CPPDumpCtrl::GetColor(int nIndex)
{
	if (nIndex >= PPDUMP_COLOR_MAX)
		return RGB (0, 0, 0);

	return m_crColor [nIndex];
}  // End of GetColor

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetDefaultColors (public member function)
//    Set the color as default
//
//  Parameters :
//		bRedraw [in] - redraw control
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetDefaultColors(BOOL bRedraw /* = TRUE */)
{
	SetColor (PPDUMP_COLOR_DATA_FG, ::GetSysColor(COLOR_WINDOWTEXT), FALSE);
	SetColor (PPDUMP_COLOR_DATA_BK, ::GetSysColor(COLOR_WINDOW), FALSE);
	SetColor (PPDUMP_COLOR_DATA_CHANGE_FG, RGB (255, 0, 0), FALSE);
	SetColor (PPDUMP_COLOR_EDIT_FG, RGB (0, 0, 0), FALSE);
	SetColor (PPDUMP_COLOR_EDIT_BK, RGB (255, 255, 0), FALSE);
	SetColor (PPDUMP_COLOR_EDIT_ERR_FG, RGB (255, 255, 255), FALSE);
	SetColor (PPDUMP_COLOR_EDIT_ERR_BK, RGB (255, 0, 0), FALSE);
	SetColor (PPDUMP_COLOR_CARET_BK, RGB (0, 255, 255), FALSE);
	SetColor (PPDUMP_COLOR_ADDRESS_FG, RGB (0, 176, 0), FALSE);
	SetColor (PPDUMP_COLOR_SEPARATORS, ::GetSysColor(COLOR_BTNSHADOW), FALSE);
	SetColor (PPDUMP_COLOR_TEXT_HEADER, ::GetSysColor(COLOR_BTNSHADOW), FALSE);
	SetColor (PPDUMP_COLOR_MOUSE_TRACK, ::GetSysColor(COLOR_3DSHADOW), bRedraw);
	SetColor (PPDUMP_COLOR_RDATA_FG, RGB (255, 0, 0), FALSE);
	SetColor (PPDUMP_COLOR_SECTOR, RGB (64, 128, 128), FALSE);
	SetColor (PPDUMP_COLOR_BAD_BLK, RGB (255, 128, 128), FALSE);
	SetColor (PPDUMP_COLOR_STMP_BLK, RGB (64, 124, 191), FALSE);
	SetColor (PPDUMP_COLOR_ERASED_BLK, ::GetSysColor(COLOR_BTNSHADOW), FALSE);
	SetColor (PPDUMP_COLOR_CONFIG_BLK, RGB (255, 255, 128), FALSE);
	SetColor (PPDUMP_COLOR_GOOD_BLK, RGB (64, 128, 128), FALSE);
	SetColor (PPDUMP_COLOR_REGION_BG, RGB (147, 254, 141), FALSE);
}  // End of SetDefaultColors

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetSelectRange (public member function)
//    Sets the range of the caret
//
//  Parameters :
//		nBegin   [in] - Begin of caret
//		nEnd     [in] - End of caret
//		bVisible [in] - Moves begin address on the screen
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetSelectRange(int nBegin /* = 0 */, int nEnd /* = -1 */, BOOL bVisible /* = TRUE */)
{
	//If first address is negative
	if (nBegin < 0)
		nBegin = 0;

	if ((nEnd < 0) || !IsEnableSelect())
		nEnd = nBegin; //nEnd equal nBegin
	else if (nEnd < nBegin)
	{
		//If nEnd less nBegin then to swap nBegin and nEnd
		int nTemp = nBegin;
		nBegin = nEnd;
		nEnd = nTemp;
	}

	//If nBegin or nAddr more then length of the data
	if ((nBegin >= m_nLengthData) || (nEnd >= m_nLengthData))
		return;

	KillEdit();

	m_nCaretAddrFirst = nBegin;
	m_nCaretAddrBegin = nBegin;
	m_nCaretAddrEnd = nEnd;
	
	if (bVisible)
		SetVisibleAddress(m_nCaretAddrBegin);
} //End of SetSelectRange

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::GetSelectRange (public member function)
//    Gets the range of the caret
//
//  Parameters :
//		nBegin [out] - Begin address
//		nEnd   [out] - End address
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::GetSelectRange(LPINT nBegin, LPINT nEnd)
{
	*nBegin = m_nCaretAddrBegin;
	*nEnd = m_nCaretAddrEnd;
} //End of GetSelectRange

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::IsAddressSelected (public member function)
//    ...
//
//  Parameters :
//		nAddress [in] - Test address
//
//  Returns :
//		If testing address in the caret range then - TRUE, else - FALSE
//
/////////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::IsAddressSelected(int nAddress)
{
	return ((nAddress >= m_nCaretAddrBegin) && (nAddress <= m_nCaretAddrEnd));
} //End of AddressInCaretRange


/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::SetReadOnly (public member function)
//    Sets the read only style
//
//  Parameters :
//		bReadOnly [in] - If TRUE the control will available for read only
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetReadOnly(BOOL bReadOnly /* = TRUE */)
{
	if ((m_pEdit != NULL) && ((m_nEditedArea & 0xF) > 0x1) && bReadOnly)
		KillEdit();

	if (bReadOnly) 
		ModifyStyles(PPDUMP_READ_ONLY, 0);
	else 
		ModifyStyles(0, PPDUMP_READ_ONLY);
} // End of ReadOnly

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::IsReadOnly (public member function)
//    Call this member function to determine if the control is read-only.
//    User can view the data, but cannot change it.
//
//  Returns :
//    TRUE if the control is read-only
//
/////////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::IsReadOnly()
{
	return (BOOL)(m_nStyle & PPDUMP_READ_ONLY);
} // End of IsReadOnly

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::EnableSelect (public member function)
//    Enables/Disables the selection of the block of the data
//
//  Parameters :
//		bEnable [in] - If TRUE the control can selecting the block of the data
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::EnableSelect(BOOL bEnable /* = TRUE */)
{
	if (bEnable) 
		ModifyStyles(PPDUMP_SELECTING_DATA, 0);
	else 
		ModifyStyles(0, PPDUMP_SELECTING_DATA);

	if (!bEnable)
	{
		//If select is enabled
		m_bPressedLButton = FALSE;
		SetSelectRange(m_nCaretAddrBegin);
	}
} //End of EnableSelect

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::IsEnableSelect (public member function)
//    Determines can user select the data or not.
//
//  Returns :
//    TRUE if user can select the block of the data.
//
/////////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::IsEnableSelect()
{
	return (BOOL)(m_nStyle & PPDUMP_SELECTING_DATA);
} // End of IsEnableSelect

/////////////////////////////////////////////////////////////////////
// This function associates a menu to the control.
// The menu will be displayed clicking the control.
//
// Parameters:
//		[IN]	nMenu
//				ID number of the menu resource.
//				Pass NULL to remove any menu from the control.
//		[IN]	bRedraw
//				If TRUE the control will be repainted.
//
// Return value:
//		TRUE - Function executed successfully.
//		FALSE - Failed loading the specified resource.
//
///////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::SetMenu(UINT nMenu, BOOL bRedraw /* = TRUE */)
{
	HINSTANCE	hInstResource = NULL;

	// Destroy any previous menu
	if (m_hMenu)
	{
		::DestroyMenu(m_hMenu);
		m_hMenu = NULL;
	} // if

	// Load menu
	if (nMenu)
	{
		// Find correct resource handle
		hInstResource = AfxFindResourceHandle(MAKEINTRESOURCE(nMenu), RT_MENU);
		// Load menu resource
		m_hMenu = ::LoadMenu(hInstResource, MAKEINTRESOURCE(nMenu));
		// If something wrong
		if (m_hMenu == NULL)
			return FALSE;
	} // if

	// Repaint the control
	if (bRedraw) 
		Invalidate();

	return TRUE;
} // End of SetMenu

/////////////////////////////////////////////////////////////////////
// This function sets or removes the notification messages from the control.
//
// Parameters:
//		bNotify [in] - If TRUE the control will be send the notification 
//					   to parent window
//					   Else the notification will not send
///////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetNotify(BOOL bNotify /* = TRUE */)
{
	HWND hWnd = NULL;

	if (bNotify)
		hWnd = GetParent()->m_hWnd;

	SetNotify(hWnd);
} //End SetNotify

/////////////////////////////////////////////////////////////////////
// This function sets or removes the notification messages from the control.
//
// Parameters:
//		hWnd [in] -    If non-NULL the control will be send the notification 
//					   to specified window
//					   Else the notification will not send
///////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetNotify(HWND hWnd, CSTMPViewHex* pView)
{
	m_hParentWnd = hWnd;
	m_pView = pView;
} //End SetNotify

/////////////////////////////////////////////////////////////////////
// This function determines will be send the notification messages from 
// the control or not.
//
// Return value:
//	TRUE if the control will be notified the specified window
///////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::GetNotify()
{
	return (m_hParentWnd != NULL);
}  //End GetNotify

///////////////////////////////////////////////////////////////////////
// This function sets the format string of the text to show in the control tooltip.
//
// Parameters:
//		[IN]	nText
//				ID number of the string resource containing the format string
//			    of the text to show.
//		[IN]	bActivate
//				If TRUE the tooltip will be created active.
///////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetTooltipText(int nText, BOOL bActivate /* = TRUE */)
{
	CString sToolTip;
	// Load string resource
	sToolTip.LoadString(nText);
	// If string resource is not empty
	SetTooltipText(sToolTip, bActivate);
} // End of SetTooltipText

///////////////////////////////////////////////////////////////////////
// This function sets the format string of the text to show in the control tooltip.
//
// Parameters:
//		[IN]	sFormatTip
//				Pointer to a null-terminated string containing the format string
//			    of the text to show.
//		[IN]	bActivate
//				If TRUE the tooltip will be created active.
///////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetTooltipText(CString sFormatTip, BOOL bActivate /* = TRUE */)
{
	// Initialize ToolTip
	InitToolTip();

	m_sFormatToolTip = sFormatTip;
	if (sFormatTip.IsEmpty())
		bActivate = FALSE;

	// If there is no tooltip defined then add it
	if (m_pToolTip.GetToolCount() == 0)
	{
		CRect rect; 
		GetClientRect(rect);
		m_pToolTip.AddTool(this, _T(""), rect, 1);
	} // if

	m_pToolTip.Activate(bActivate);
} // End of SetTooltipText

////////////////////////////////////////////////////////////////////////
// This function enables or disables the control tooltip.
//
// Parameters:
//		[IN]	bActivate
//				If TRUE the tooltip will be activated.
////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::ActivateTooltip(BOOL bActivate /* = TRUE */)
{
	// If there is no tooltip then do nothing
	if (m_pToolTip.GetToolCount() == 0) 
		return;

	// Activate tooltip
	m_pToolTip.Activate(bActivate);
} // End of ActivateTooltip

////////////////////////////////////////////////////////////////////////
// This function gets pointer to the tooltip object
//
// Return values:
//	Function returns the pointer to the tooltip object
////////////////////////////////////////////////////////////////////////
CToolTipCtrl * CPPDumpCtrl::GetTooltip()
{
	// Initialize ToolTip
	InitToolTip();

	return &m_pToolTip;
} // End of GetTooltip

/////////////////////////////////////////////////////////////////////
// This function enables or disables the track mouse.
//
// Parameters:
//	bTrack	[in] - If TRUE the track mouse will enable
//	bRedraw	[in] - If TRUE the control will be repainted.
//
///////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::SetTrackMouseMove(BOOL bTrack /* = TRUE */, BOOL bRedraw /* = TRUE */)
{
	if (IsTrackMouseMove() != bTrack)
	{
		if (bTrack)
			m_nStyle |= PPDUMP_TRACK_MOUSE_MOVE;
		else
			m_nStyle &= ~PPDUMP_TRACK_MOUSE_MOVE;
		// Repaint the control
		if (bRedraw) 
			Invalidate();
	}
} // End of SetTrackMouseMove

////////////////////////////////////////////////////////////////////////
// CPPDumpCtrl::IsTrackMouseMove
// This function returns whether the track mouse is enabled
//
// Return values:
//	Function returns TRUE if track mouse is enabled
////////////////////////////////////////////////////////////////////////
BOOL CPPDumpCtrl::IsTrackMouseMove()
{
	return (BOOL)(m_nStyle & PPDUMP_TRACK_MOUSE_MOVE);
} // End of IsTrackMouseMove

INT_PTR CPPDumpCtrl::GetDataWordSize(void)
{
//    INT_PTR size = m_nStyle & PPDUMP_WORD_DATA ? 2 : 1;
    return m_nDataWordSize;
}

INT_PTR CPPDumpCtrl::GetAddressColumn(INT_PTR _addr)
{
    INT_PTR num_short_lines, num_missing_chars_per_short_line = 0, col;
    if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
        // how many short lines are there?
        num_short_lines = _addr / m_nDataBlockSize;
        // how many missing chars are there in the short line?
        if ( m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize) )
            num_missing_chars_per_short_line = (m_nDataInLines * m_nDataWordSize) - (m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize));
        
        _addr += num_short_lines * num_missing_chars_per_short_line;
}

    col = (_addr % (m_nDataInLines * m_nDataWordSize))/m_nDataWordSize;

    return col; // 0-based
}

INT_PTR CPPDumpCtrl::GetAddressRow(INT_PTR _addr)
{
    INT_PTR num_short_lines, num_missing_chars_per_short_line = 0, row;
    if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
        // how many short lines are there?
        num_short_lines = _addr / m_nDataBlockSize;
        // how many missing chars are there in the short line?
        if ( m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize) )
            num_missing_chars_per_short_line = (m_nDataInLines * m_nDataWordSize) - (m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize) );
        
        _addr += num_short_lines * num_missing_chars_per_short_line;// * m_nDataWordSize;
    }

    row = _addr / (m_nDataInLines * m_nDataWordSize);

    return row; // 0-based
}

INT_PTR CPPDumpCtrl::GetRowLength(INT_PTR _row, BOOL *_is_block_end)
{
    INT_PTR num_rows_per_block, len = m_nDataInLines;
    
    
	if (_is_block_end)
        *_is_block_end = FALSE;
    
    if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
        // if this row is a short row?
        num_rows_per_block = m_nDataBlockSize/(m_nDataInLines * m_nDataWordSize) + ((m_nDataBlockSize%(m_nDataInLines * m_nDataWordSize))?1:0);

        if ( ((_row+1) % num_rows_per_block) == 0 ) {
            if ( m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize) )
                len = ( m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize) ) / m_nDataWordSize;
            if (_is_block_end)
                *_is_block_end = TRUE;
        }
    }

    return len;
}

#define VERT_SPACE  8
INT_PTR CPPDumpCtrl::GetAddressVOffset(INT_PTR _addr, INT_PTR *_row)
{
    INT_PTR offset = 0;
    INT_PTR num_short_lines;
    if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
        // how many short lines are there?
        num_short_lines = _addr / m_nDataBlockSize;

        // VERT_SPACE is just a number that I picked for vertical spacing
        offset = num_short_lines * VERT_SPACE;
    }
    
    if (_row) {
        *_row = GetAddressRow(_addr);
        offset += (*_row) * m_nHeightFont;
    }
    else
        offset += GetAddressRow(_addr) * m_nHeightFont;

    return offset;
}

INT_PTR CPPDumpCtrl::GetRowVOffset(INT_PTR _row)
{
    INT_PTR offset = 0;
    INT_PTR addr, num_short_lines;
    
    addr = _row * m_nDataInLines * m_nDataWordSize + m_nBeginAddress;
    
    if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
        
//        while ( _row != GetAddressRow(addr) )
//            addr += m_nDataInLines;

        // how many short lines are there?
        num_short_lines = (addr / m_nDataBlockSize) - (m_nBeginAddress / m_nDataBlockSize);
//        if (num_short_lines)
//            if (m_nBeginAddress && (m_nBeginAddress % m_nDataBlockSize == 0))
//                --num_short_lines;

        // VERT_SPACE is just a number that I picked for vertical spacing
        offset = num_short_lines * VERT_SPACE;
    }
    
    offset += (GetAddressRow(addr) - GetAddressRow(m_nBeginAddress)) * m_nHeightFont;

    return offset;
}

INT_PTR CPPDumpCtrl::GetAddressHOffset(INT_PTR _addr)
{
    INT_PTR offset = 0;

    offset = GetAddressColumn(_addr) * m_nWidthFont;

    return offset;
}

INT_PTR CPPDumpCtrl::GetRowColAddress(INT_PTR _row, INT_PTR _col)
{
    INT_PTR offset = 0;
    INT_PTR addr;
    
    addr = m_nBeginAddress + _row * m_nDataInLines * m_nDataWordSize;
    
    if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
        
        INT_PTR temp = addr % m_nDataBlockSize;
        if ( (temp != (m_nDataInLines * m_nDataWordSize)) && (m_nDataBlockSize % (m_nDataInLines*m_nDataWordSize) != 0) ) {
            for ( ; addr - temp > m_nBeginAddress; temp += m_nDataBlockSize) {
                addr -= (m_nDataInLines*m_nDataWordSize) - (m_nDataBlockSize % (m_nDataInLines*m_nDataWordSize));
                if ( temp < (m_nDataInLines*m_nDataWordSize) - (m_nDataBlockSize % (m_nDataInLines*m_nDataWordSize)) )
                    temp = addr % m_nDataBlockSize;
            }
        }
    }

	if (GetRowLength(GetAddressRow(addr))-1 < _col)
        return -1;
   
    return addr + (_col * m_nDataWordSize);
}

INT_PTR CPPDumpCtrl::GetScrollAddress(INT_PTR _pos)
{
    INT_PTR addr;
    
    addr = _pos * m_nDataInLines * m_nDataWordSize;
    
    if ( (m_nStyle & PPDUMP_DATA_BLOCKS) && (m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize)) ) {
        addr -= ( _pos / (( m_nDataBlockSize / (m_nDataInLines * m_nDataWordSize) )+1)) * ((m_nDataInLines * m_nDataWordSize) - (m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize)));
    }

    return addr;
}

INT_PTR CPPDumpCtrl::GetRowFromVOffset(INT_PTR _offset)
{
    INT_PTR row = 0;
    
    row = _offset / m_nHeightFont;

    if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
        
        for ( ; row > 0; row--) {
            if ( GetRowVOffset(row) < _offset )
                break;
        }
    }

    return row;
}

INT_PTR CPPDumpCtrl::GetMaxRows(void)
{
    ASSERT(m_nDataInLines);

    INT_PTR num_short_lines, num_missing_chars_per_short_line = 0, num_rows, temp_len = m_nRealLengthData;
    if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
        // how many short lines are there?
        num_short_lines = temp_len / m_nDataBlockSize;
        // how many missing chars are there in the short line?
        if ( m_nDataBlockSize % m_nDataInLines )
            num_missing_chars_per_short_line = m_nDataInLines - ((m_nDataBlockSize % (m_nDataInLines * m_nDataWordSize) / m_nDataWordSize) );
        
        temp_len += num_short_lines * num_missing_chars_per_short_line * m_nDataWordSize;
    }

    num_rows = temp_len / ( m_nDataInLines * m_nDataWordSize );
    if (temp_len % ( m_nDataInLines * m_nDataWordSize) )
        ++num_rows;
    
//    ATLTRACE(_T("num rows = %d\n"), num_rows);
    return num_rows; // 1-based
}

BOOL CPPDumpCtrl::IsRedundantArea(INT_PTR _addr)
{
	INT_PTR rel_addr = _addr % m_nDataBlockSize;
	if ( rel_addr >= m_nDataBlockSize - m_nRedundantSize )
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawSectorField  (public member function)
//    Draws the sector field of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::DrawSectorField(CDC * pDC, BOOL bDisable)
{
	if (!(m_nStyle & PPDUMP_FIELD_SECTOR))
		return;

	COLORREF crTemp = pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	CPen pen(PS_SOLID, 0, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);

	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_SECTOR];

	if (m_nStyle & PPDUMP_SEPARATOR_LINES)
	{
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	rect.DeflateRect(0, m_nHeightFont / 4, 0, m_nHeightFont);

	UINT nNewValue, nOldValue;
    CRect tmp_rect(rect);

	for (int nAddr = m_nBeginAddress; (nAddr < m_nLengthData) && (tmp_rect.top <= rect.bottom); NULL)
	{
        tmp_rect = rect;
        BOOL is_block_end;
        tmp_rect.top += (LONG)(GetAddressVOffset(nAddr) - GetAddressVOffset(m_nBeginAddress));

        INT_PTR len = GetRowLength(GetAddressRow(nAddr + m_nOffsetAddress), &is_block_end);
		for (int i = 0; (i < len) && (nAddr < m_nLengthData); i++)
		{
//			nNewValue = GetDataFromCurrentAddress(nAddr);
//			nOldValue = GetDataFromCurrentAddress(nAddr, FALSE);
//			if (!bDisable) {
//				if ( m_nStyle & PPDUMP_DATA_BLOCKS && IsRedundantArea(nAddr) )
//					pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_RDATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
//				else
//					pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_DATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
//			}
            COLORREF clr = m_pView->GetAddressColor(nAddr);
			tmp_rect.left = DrawSolidBox(pDC, tmp_rect, clr, bDisable);
			tmp_rect.left -= m_nWidthFont;
			nAddr += m_nDataWordSize;
		}
        if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
            if ( is_block_end ) {
                tmp_rect.top += m_nHeightFont + 3;
                tmp_rect.left = rect.left;
		        pDC->MoveTo(tmp_rect.left, tmp_rect.top);
		        pDC->LineTo(tmp_rect.right, tmp_rect.top);
            }
        }
	}
	pDC->SelectObject(penOld);
	pDC->SetTextColor(crTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
//  CPPDumpCtrl::DrawBlockField  (public member function)
//    Draws the block field of the control
//
//  Parameters :
//		pDC		    [in] - the device context
//
//  Returns :
//		None
//
/////////////////////////////////////////////////////////////////////////////
void CPPDumpCtrl::DrawBlockField(CDC * pDC, BOOL bDisable)
{
	if (!(m_nStyle & PPDUMP_FIELD_BLOCK))
		return;

	COLORREF crTemp = pDC->SetTextColor(bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_DATA_FG]);
	CPen pen(PS_SOLID, 0, bDisable ? m_crDisableFg : m_crColor [PPDUMP_COLOR_SEPARATORS]);
	CPen * penOld = pDC->SelectObject(&pen);

	CRect rect = m_rFieldArea [PPDUMP_BAR_AREA_BLOCK];

	if (m_nStyle & PPDUMP_SEPARATOR_LINES)
	{
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
	}

	rect.DeflateRect(0, m_nHeightFont / 4, 0, m_nHeightFont);

	UINT nNewValue, nOldValue;
    CRect tmp_rect(rect);

	for (int nAddr = m_nBeginAddress; (nAddr < m_nLengthData) && (tmp_rect.top <= rect.bottom); NULL)
	{
        tmp_rect = rect;
        BOOL is_block_end;
        tmp_rect.top += (LONG)(GetAddressVOffset(nAddr) - GetAddressVOffset(m_nBeginAddress));

        INT_PTR len = GetRowLength(GetAddressRow(nAddr + m_nOffsetAddress), &is_block_end);
		for (int i = 0; (i < len) && (nAddr < m_nLengthData); i++)
		{
//			nNewValue = GetDataFromCurrentAddress(nAddr);
//			nOldValue = GetDataFromCurrentAddress(nAddr, FALSE);
//			if (!bDisable) {
//				if ( m_nStyle & PPDUMP_DATA_BLOCKS && IsRedundantArea(nAddr) )
//					pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_RDATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
//				else
//					pDC->SetTextColor((nNewValue == nOldValue) ? m_crColor [PPDUMP_COLOR_DATA_FG] : m_crColor [PPDUMP_COLOR_DATA_CHANGE_FG]);
//			}
            COLORREF clr = m_pView->GetAddressColor(nAddr);
			tmp_rect.left = DrawSolidBox(pDC, tmp_rect, clr, bDisable);
			tmp_rect.left -= m_nWidthFont;
			nAddr += m_nDataWordSize;
		}
        if ( m_nStyle & PPDUMP_DATA_BLOCKS ) {
            if ( is_block_end ) {
                tmp_rect.top += m_nHeightFont + 3;
                tmp_rect.left = rect.left;
		        pDC->MoveTo(tmp_rect.left, tmp_rect.top);
		        pDC->LineTo(tmp_rect.right, tmp_rect.top);
            }
        }
	}
	pDC->SelectObject(penOld);
	pDC->SetTextColor(crTemp);
}
