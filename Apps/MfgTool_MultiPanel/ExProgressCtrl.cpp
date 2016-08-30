/*
 * Copyright 2012-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "stdafx.h"
#include "ExProgressCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CExProgressCtrl::CExProgressCtrl()
{
    // default colors
    m_crBarColor = CLR_DEFAULT;
    m_crBarBkColor = CLR_DEFAULT;
}

CExProgressCtrl::~CExProgressCtrl()
{
}

BEGIN_MESSAGE_MAP(CExProgressCtrl, CProgressCtrl)
ON_WM_ERASEBKGND()
ON_WM_PAINT()
ON_MESSAGE(PBM_SETBARCOLOR, OnSetBarColor)
ON_MESSAGE(PBM_SETBKCOLOR, OnSetBarBkColor)
ON_MESSAGE(PBM_GETBARCOLOR, OnGetBarColor)
ON_MESSAGE(PBM_GETBKCOLOR, OnGetBarBkColor)
ON_MESSAGE(PBM_SETRANGE32, OnSetRange32)
ON_MESSAGE(PBM_SETPOS, OnSetPos)
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrl message handlers

BOOL CExProgressCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CExProgressCtrl::OnPaint()
{
    CPaintDC PaintDC(this);						// device context for painting
    CDC dcMemory;
    CRect rectDC;
    CBitmap memBitmap;
    CBitmap *pOldBitmap;
    
    COLORREF crBarColor		= (COLORREF)OnGetBarColor(0, 0);
    COLORREF crBarBkColor	= (COLORREF)OnGetBarBkColor(0, 0);
    
    PaintDC.GetClipBox(&rectDC);
    dcMemory.CreateCompatibleDC(&PaintDC);
    memBitmap.CreateCompatibleBitmap(&PaintDC, rectDC.Width(), rectDC.Height());
    pOldBitmap = dcMemory.SelectObject(&memBitmap);
    
    CRect ClientRect, LeftRect, RightRect;
    GetClientRect(&ClientRect);
    DrawEdge(dcMemory, ClientRect, EDGE_SUNKEN, BF_ADJUST | BF_RECT | BF_FLAT);
    LeftRect = RightRect = ClientRect;
    
    double dFraction = (double)(m_nPos - m_nLow) / ((double)(m_nHigh - m_nLow));
    LeftRect.right = LeftRect.left + (int)((LeftRect.Width()) * dFraction);
    RightRect.left = LeftRect.right;
    dcMemory.FillSolidRect(LeftRect, crBarColor);
    dcMemory.FillSolidRect(RightRect, crBarBkColor);
    
    PaintDC.BitBlt(rectDC.left, rectDC.top, rectDC.Width(), rectDC.Height(),
					&dcMemory, rectDC.left, rectDC.top, SRCCOPY);
	PaintDC.SelectObject(pOldBitmap);
	memBitmap.DeleteObject();
	dcMemory.DeleteDC();
}

LRESULT CExProgressCtrl::OnSetBarColor(WPARAM, LPARAM crBarColor)
{
    COLORREF crOldBarColor = m_crBarColor;
    m_crBarColor = (COLORREF)crBarColor;
    
    RedrawWindow();
    
    return ((LRESULT)crOldBarColor);
}

LRESULT CExProgressCtrl::OnSetBarBkColor(WPARAM, LPARAM crBarBkColor)
{
    COLORREF crOldBarBkColor = m_crBarBkColor;
    m_crBarBkColor = (COLORREF)crBarBkColor;
    
    RedrawWindow();
    
    return ((LRESULT)crOldBarBkColor);
}

LRESULT CExProgressCtrl::OnGetBarColor(WPARAM, LPARAM)
{
    COLORREF cr;
    if(m_crBarColor == CLR_DEFAULT)
    {
        cr = ::GetSysColor(COLOR_BTNFACE);
    }
    else
    {
        cr = m_crBarColor;
    }
    
    return (LRESULT)cr;
}

LRESULT CExProgressCtrl::OnGetBarBkColor(WPARAM, LPARAM)
{
    COLORREF cr;
    if(m_crBarBkColor == CLR_DEFAULT)
    {
        cr = ::GetSysColor(COLOR_BTNFACE);
    }
    else
    {
        cr = m_crBarBkColor;
    }
    
    return (LRESULT)cr;
}

LRESULT CExProgressCtrl::OnSetRange32(WPARAM nLow, LPARAM nHigh)
{
    int nOldLow = m_nLow;
    int nOldHigh = m_nHigh;
    
    m_nLow = (int)nLow;
    m_nHigh = (int)nHigh;
    
    return (MAKELRESULT(LOWORD(nOldLow), LOWORD(nOldHigh)));
}

LRESULT CExProgressCtrl::OnSetPos(WPARAM nNewPos, LPARAM)
{
    int nOldPos = m_nPos;
    m_nPos = (int)nNewPos;
    if(m_nPos < m_nLow)
    {
        m_nPos = m_nLow;
    }
    else if(m_nPos > m_nHigh)
    {
        m_nPos = m_nHigh;
    }
    
    RedrawWindow();
    
    return ((LRESULT)nOldPos);
}
