/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// ColorStaticST.cpp : implementation file
//

#include "stdafx.h"
#include "ColorStaticST.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorStaticST

CColorStaticST::CColorStaticST()
{
	// Set default parent window and notification message
	m_pParent = NULL;
	m_nMsg = WM_USER;

	// By default the control is not blinking
	m_bTextBlink = FALSE;
	m_nTextBlinkStep = 0;
	m_bBkBlink = FALSE;
	m_nBkBlinkStep = 0;
	m_nTimerId = 0;

	// Set default foreground text
	m_crTextColor = ::GetSysColor(COLOR_BTNTEXT);

	// Set default foreground text (when blinking)
	m_crBlinkTextColors[0] = m_crTextColor;
	m_crBlinkTextColors[1] = m_crTextColor;

	// Set default background text
	m_crBkColor = ::GetSysColor(COLOR_BTNFACE);

	// Set default background text (when blinking)
	m_crBlinkBkColors[0] = m_crBkColor;
	m_crBlinkBkColors[1] = m_crBkColor;

	// Set default background brush
	m_brBkgnd.CreateSolidBrush(m_crBkColor);

	// Set default background brush (when blinking)
	m_brBlinkBkgnd[0].CreateSolidBrush(m_crBkColor);
	m_brBlinkBkgnd[1].CreateSolidBrush(m_crBkColor);
} // End of CColorStaticST


CColorStaticST::~CColorStaticST()
{
} // End of ~CColorStaticST


BEGIN_MESSAGE_MAP(CColorStaticST, CStatic)
	//{{AFX_MSG_MAP(CColorStaticST)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorStaticST message handlers


HBRUSH CColorStaticST::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	// Set foreground color
	// If control is blinking (text)
	if (m_bTextBlink == TRUE)
	{
		pDC->SetTextColor(m_crBlinkTextColors[m_nTextBlinkStep]);
	}
	else
	{
		pDC->SetTextColor(m_crTextColor);
	}

	// Set background color & brush
	// If control is blinking (background)
	if (m_bBkBlink == TRUE)
	{
		pDC->SetBkColor(m_crBlinkBkColors[m_nBkBlinkStep]);
		return (HBRUSH)m_brBlinkBkgnd[m_nBkBlinkStep];
	}
	// If control is not blinking (background)
	pDC->SetBkColor(m_crBkColor);

	// Return a non-NULL brush if the parent's handler should not be called
    return (HBRUSH)m_brBkgnd;
	
	UNREFERENCED_PARAMETER(nCtlColor);
} // End of CtlColor


void CColorStaticST::OnDestroy() 
{
	CStatic::OnDestroy();
	
	// Destroy timer (if any)
	if (m_nTimerId > 0) KillTimer(m_nTimerId);

	// Destroy resources
    m_brBkgnd.DeleteObject();
    m_brBlinkBkgnd[0].DeleteObject();
    m_brBlinkBkgnd[1].DeleteObject();
} // End of OnDestroy


void CColorStaticST::SetTextColor(COLORREF crTextColor)
{
	// Set new foreground color
	if (crTextColor != 0xffffffff)
	{
		m_crTextColor = crTextColor;
	}
	else // Set default foreground color
	{
		m_crTextColor = ::GetSysColor(COLOR_BTNTEXT);
	}

	// Repaint control
	Invalidate();
} // End of SetTextColor


COLORREF CColorStaticST::GetTextColor()
{
	return m_crTextColor;
} // End of GetTextColor


void CColorStaticST::SetBkColor(COLORREF crBkColor)
{
	// Set new background color
	if (crBkColor != 0xffffffff)
	{
		m_crBkColor = crBkColor;
	}
	else // Set default background color
	{
		m_crBkColor = ::GetSysColor(COLOR_BTNFACE);
	}

    m_brBkgnd.DeleteObject();
    m_brBkgnd.CreateSolidBrush(m_crBkColor);

	// Repaint control
	Invalidate();
} // End of SetBkColor


COLORREF CColorStaticST::GetBkColor()
{
	return m_crBkColor;
} // End of GetBkColor


void CColorStaticST::SetBlinkTextColors(COLORREF crBlinkTextColor1, COLORREF crBlinkTextColor2)
{
	// Set new blink text colors
	m_crBlinkTextColors[0] = crBlinkTextColor1;
	m_crBlinkTextColors[1] = crBlinkTextColor2;
} // End of SetBlinkTextColors


void CColorStaticST::StartTextBlink(BOOL bStart, UINT nElapse)
{
	UINT nCount;

	// Destroy any previous timer
	if (m_nTimerId > 0)	
	{
		KillTimer(m_nTimerId);
		m_nTimerId = 0;
	}

	m_bTextBlink = bStart;
	m_nTextBlinkStep = 0;

	// Start timer
	if (m_bTextBlink == TRUE) 
	{
		switch (nElapse)
		{
			case ST_FLS_SLOW:
				nCount = 2000;
				break;
			case ST_FLS_NORMAL:
				nCount = 1000;
				break;
			case ST_FLS_FAST:
				nCount = 500;
				break;
			default:
				nCount = nElapse;
				break;
		}
		m_nTimerId = SetTimer(1, nCount, NULL); 
	}
} // End of StartTextBlink


void CColorStaticST::SetBlinkBkColors(COLORREF crBlinkBkColor1, COLORREF crBlinkBkColor2)
{
	// Set new blink background colors
	m_crBlinkBkColors[0] = crBlinkBkColor1;
	m_crBlinkBkColors[1] = crBlinkBkColor2;

    m_brBlinkBkgnd[0].DeleteObject();
    m_brBlinkBkgnd[0].CreateSolidBrush(m_crBlinkBkColors[0]);
    m_brBlinkBkgnd[1].DeleteObject();
    m_brBlinkBkgnd[1].CreateSolidBrush(m_crBlinkBkColors[1]);

	// Repaint control
	Invalidate();
} // End of SetBlinkBkColor


void CColorStaticST::StartBkBlink(BOOL bStart, UINT nElapse)
{
	UINT nCount;

	// Destroy any previous timer
	if (m_nTimerId > 0)	
	{
		KillTimer(m_nTimerId);
		m_nTimerId = 0;
	}

	m_bBkBlink = bStart;
	m_nBkBlinkStep = 0;

	// Start timer
	if (m_bBkBlink == TRUE) 
	{
		switch (nElapse)
		{
			case ST_FLS_SLOW:
				nCount = 2000;
				break;
			case ST_FLS_NORMAL:
				nCount = 1000;
				break;
			case ST_FLS_FAST:
				nCount = 500;
				break;
			default:
				nCount = nElapse;
				break;
		}
		m_nTimerId = SetTimer(1, nCount, NULL);
	}
} // End of StartBkBlink


void CColorStaticST::EnableNotify(CWnd* pParent, UINT nMsg)
{
	m_pParent = pParent;
	m_nMsg = nMsg;
} // End of EnableNotify


const short CColorStaticST::GetVersionI()
{
	return 10; // Divide by 10 to get actual version
} // End of GetVersionI


const char* CColorStaticST::GetVersionC()
{
	return "1.0";
} // End of GetVersionC


void CColorStaticST::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == m_nTimerId)	
	{
		// If control is blinking (text) switch its color
		if (m_bTextBlink == TRUE) m_nTextBlinkStep = !m_nTextBlinkStep;

		// If control is blinking (background) switch its color
		if (m_bBkBlink == TRUE) m_nBkBlinkStep = !m_nBkBlinkStep;

		// If there is any blinking in action then repaint the control
		// and send the notification message (if any)
		if (m_bBkBlink == TRUE || m_bTextBlink == TRUE) 
		{
			Invalidate();
			// Send notification message only on rising blink
			if (m_pParent != NULL && (m_nBkBlinkStep == 1 || m_nTextBlinkStep == 1)) 
				m_pParent->PostMessage(m_nMsg, GetDlgCtrlID(), 0);
		}
	}
	else
	CStatic::OnTimer(nIDEvent);
} // End of OnTimer
