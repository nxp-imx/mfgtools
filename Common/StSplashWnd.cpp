/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// SplashWnd.cpp : implementation file
//
// Based on the Visual C++ splash screen component.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StSplashWnd.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
//   Splash Screen class

BOOL CStSplashWnd::m_bShowSplashWnd = TRUE;
CStSplashWnd* CStSplashWnd::m_pSplashWnd;

CStSplashWnd::CStSplashWnd()
{
	m_bShow = FALSE;
	m_crTextCol = RGB(0,0,0);
	m_iX = 0;
	m_iY = 0;

	VERIFY(m_cfStatusText.CreateFont
			   (14,                       // nHeight
				0,                        // nWidth
				0,                        // nEscapement
				0,                        // nOrientation
				FW_LIGHT,			      // nWeight
				FALSE,                    // bItalic
				FALSE,                    // bUnderline
				FALSE,                    // cStrikeOut
				ANSI_CHARSET,             // nCharSet
				OUT_DEFAULT_PRECIS,		  // nOutPrecision
				CLIP_DEFAULT_PRECIS,      // nClipPrecision
				PROOF_QUALITY,            // nQuality
				VARIABLE_PITCH | FF_ROMAN,// nPitchAndFamily
				L"MS Shell Dlg"));      // lpszFacename
}

CStSplashWnd::~CStSplashWnd()
{
	// Clear the static window pointer.
	ASSERT(m_pSplashWnd == this);
	m_pSplashWnd = NULL;
}

BEGIN_MESSAGE_MAP(CStSplashWnd, CWnd)
	//{{AFX_MSG_MAP(CStSplashWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CStSplashWnd::EnableSplashScreen(BOOL bEnable /*= TRUE*/)
{
	m_bShowSplashWnd = bEnable;
}


BOOL CStSplashWnd::ShowSplashScreen(UINT uTimeOut, UINT uBitmapID,
									CString csDesc, CString csVers, CString csCopyRight,
									CWnd* pParentWnd /*= NULL*/)
{
	ASSERT(uTimeOut && uBitmapID);

	if (!m_bShowSplashWnd || m_pSplashWnd != NULL) {
		return FALSE;
	}

	// Allocate a new splash screen, and create the window.
	m_pSplashWnd = new CStSplashWnd;

	if (!m_pSplashWnd->m_bitmap.LoadBitmap(uBitmapID)) {
		return FALSE;
	}

	m_pSplashWnd->m_csDesc = csDesc;
	m_pSplashWnd->m_csVers = csVers;
	m_pSplashWnd->m_csCopyRight = csCopyRight;

	BITMAP bm;
	m_pSplashWnd->m_bitmap.GetBitmap(&bm);

	CString strWndClass = AfxRegisterWndClass(0,
		AfxGetApp()->LoadStandardCursor(IDC_ARROW));

	if (!m_pSplashWnd->CreateEx(WS_EX_WINDOWEDGE, strWndClass, NULL, WS_POPUP | WS_THICKFRAME | WS_VISIBLE,
		0, 0, 10, 10, pParentWnd->GetSafeHwnd(), NULL))
	{
		TRACE0("Failed to create splash screen.\n");
		delete m_pSplashWnd;
		return FALSE;
	}


	if (csCopyRight.FindOneOf(_T("\n")) != -1)
		m_pSplashWnd->m_VersionAreaHeight = 65;
	else
		m_pSplashWnd->m_VersionAreaHeight = 50;  

	// Center the window.
	m_pSplashWnd->ShowWindow(SW_HIDE);
	m_pSplashWnd->SetWindowPos(&wndTop, 0 , 0, bm.bmWidth, bm.bmHeight+m_pSplashWnd->m_VersionAreaHeight, SWP_NOZORDER);
	m_pSplashWnd->CenterWindow();
	m_pSplashWnd->ShowWindow(SW_SHOW);
	m_pSplashWnd->Invalidate(TRUE);
	m_pSplashWnd->UpdateWindow();
	// Set a timer to destroy the splash screen.
	m_pSplashWnd->SetTimer(1, uTimeOut, NULL);

	return TRUE;
}

BOOL CStSplashWnd::PreTranslateAppMessage(MSG* pMsg)
{
	if (m_pSplashWnd == NULL)
		return FALSE;

	// If we get a keyboard or mouse message, hide the splash screen.
	if (pMsg->message == WM_KEYDOWN ||
	    pMsg->message == WM_SYSKEYDOWN ||
	    pMsg->message == WM_LBUTTONDOWN ||
	    pMsg->message == WM_RBUTTONDOWN ||
	    pMsg->message == WM_MBUTTONDOWN ||
	    pMsg->message == WM_NCLBUTTONDOWN ||
	    pMsg->message == WM_NCRBUTTONDOWN ||
	    pMsg->message == WM_NCMBUTTONDOWN)
	{
		m_pSplashWnd->HideSplashScreen();
		return TRUE;	// message handled here
	}

	return FALSE;	// message not handled
}



void CStSplashWnd::HideSplashScreen()
{
	// Destroy the window, and update the mainframe.
	DestroyWindow();
	AfxGetMainWnd()->UpdateWindow();
}



void CStSplashWnd::CloseSplashScreen()
{
	// Destroy the window, and update the mainframe.
	m_pSplashWnd->DestroyWindow();
	AfxGetMainWnd()->UpdateWindow();
}



void CStSplashWnd::PostNcDestroy()
{
	// Free the C++ class.
	delete this;
}


//#include "resource.h"

BOOL CStSplashWnd::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Code für die Behandlungsroutine für Nachrichten hier einfügen und/oder Standard aufrufen

CRect rect, rect2;    // We use rect to get the client area size
 GetClientRect(rect);

 CBitmap* pbmpOldBitmap1;
 CBitmap bmpMainBitmap2;
 CBitmap* pbmpOldBitmap2;
 CDC dcMem1;
 CDC dcMem2;
	CPaintDC dc(this);
	dc.SetBkMode(OPAQUE);
	dc.FillSolidRect(&rect, RGB(192,192,192));
 dcMem1.CreateCompatibleDC(NULL); // Create double CDCs
 dcMem2.CreateCompatibleDC(NULL);

 // We create an empty bitmap of the client rect size
 bmpMainBitmap2.CreateCompatibleBitmap(pDC,rect.Width(),rect.Height()-m_VersionAreaHeight);

 // Select the bitmaps
 pbmpOldBitmap1 = dcMem1.SelectObject(&m_bitmap);
 pbmpOldBitmap2 = dcMem2.SelectObject(&bmpMainBitmap2);
 
 dcMem1.GetClipBox(rect2);
 // We stretch the small bitmap to the background
 dcMem2.StretchBlt(0,0,rect.Width(),rect.Height()-m_VersionAreaHeight,&dcMem1,0,0,rect2.Width(),rect2.Height(),SRCCOPY);

 // We blit the big one to the screen
 pDC->BitBlt(0,0,rect.Width(),rect.Height()-m_VersionAreaHeight,&dcMem2,0,0,SRCCOPY);

 // Cleaning up
 dcMem1.SelectObject(pbmpOldBitmap1);
 dcMem1.DeleteDC();
 dcMem2.SelectObject(pbmpOldBitmap2);
 dcMem2.DeleteDC();	

return TRUE;
//return CWnd::OnEraseBkgnd(pDC);
}



void CStSplashWnd::OnPaint()
{
//	DEBUG NOTE: If you set a breakpoint in this code the text will not be displayed.

	CPaintDC dc(this);
	int nOldBkMode = 0, iX = 0, iY = 0;
	CDC dcImage;
	CRect rectC;
	GetClientRect(&rectC);
	ClientToScreen(rectC);
	int iCX = rectC.Width();
	int iCY = rectC.Height();

	if (dcImage.CreateCompatibleDC(&dc))
	{
		BITMAP bm;
		m_bitmap.GetBitmap(&bm);

		if(m_iX<bm.bmWidth)	
		{
			m_iX = bm.bmWidth;
			m_iY = bm.bmHeight-m_VersionAreaHeight;
		}

		dc.SetTextColor(m_crTextCol);
		nOldBkMode = dc.SetBkMode(TRANSPARENT);
		SelectObject(dc.m_hDC, m_cfStatusText.m_hObject );

		if(!m_csDesc.IsEmpty())
		{
			m_csTextExt = dc.GetOutputTextExtent(m_csDesc);
			dc.TextOut (5,	iCY-(m_csTextExt.cy+(m_VersionAreaHeight-15)), 
							m_csDesc);

 			if(m_csTextExt.cx>iX)
 					iX = m_csTextExt.cx;
 			if(m_csTextExt.cy>iY)
 					iY = m_csTextExt.cy;
		}

		if(!m_csVers.IsEmpty())
		{
			m_csTextExt = dc.GetOutputTextExtent(m_csVers);
			dc.TextOut (5,	iCY-(m_csTextExt.cy+(m_VersionAreaHeight-30)), 
							m_csVers);
 			if(m_csTextExt.cx>iX)
 					iX = m_csTextExt.cx;
 			if(m_csTextExt.cy>iY)
 					iY = m_csTextExt.cy;
		}

		if(m_csCopyRight)
		{
			CString tmp1;
			CString tmp2 = _T("");

			if (m_VersionAreaHeight > 50)
			{
				int pos = m_csCopyRight.FindOneOf(_T("\n"));
				tmp1 = m_csCopyRight.Left(pos);
				tmp2 = m_csCopyRight.Right(m_csCopyRight.GetLength()-(pos+1));
			}
			else
				tmp1 = m_csCopyRight;

			m_csTextExt = dc.GetOutputTextExtent(tmp1);
			dc.TextOut (5,	iCY-(m_csTextExt.cy+(m_VersionAreaHeight-45)), 
							tmp1);
 			if(m_csTextExt.cx>iX)
 					iX = m_csTextExt.cx;
 			if(m_csTextExt.cy>iY)
 					iY = m_csTextExt.cy;

			if (!tmp2.IsEmpty())
			{
				m_csTextExt = dc.GetOutputTextExtent(tmp2);
				dc.TextOut (5,	iCY-(m_csTextExt.cy+(m_VersionAreaHeight-60)), 
								tmp2);
			}
		}

		if(m_iX<iX||m_iY<iY)
		{
			m_iX = iX;
			m_iY = iY;  // only if we have added extra text lines
			SetWindowPos(&wndTop, 0, 0, m_iX, m_iY+m_VersionAreaHeight, SWP_NOZORDER);
			CenterWindow();
			RedrawWindow();
		}

		if(!m_bShow) 
		{
			ShowWindow(SW_SHOW);
			m_bShow = TRUE;
		}
	}
}




void CStSplashWnd::OnTimer(UINT nIDEvent)
{
	// Destroy the splash screen window.
	HideSplashScreen();
UNREFERENCED_PARAMETER(nIDEvent);
}
