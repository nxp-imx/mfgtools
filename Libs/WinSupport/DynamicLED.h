/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// DynamicLED.h : header file
//
#if !defined(AFX_DYNAMICLED_H__7AA00BEC_B6E4_48A7_9719_3A15C0AB217A__INCLUDED_)
#define AFX_DYNAMICLED_H__7AA00BEC_B6E4_48A7_9719_3A15C0AB217A__INCLUDED_

#if _MSC_VER > 1000
    #pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

#define ID_SHAPE_ROUND 3001
#define ID_SHAPE_SQUARE 3002

/////////////////////////////////////////////////////////////////////////////
// CDynamicLED window
class CDynamicLED : public CStatic
{
    // Construction
public:
    enum eColors { color_Red, color_Yellow, color_Green, color_Blue };
    enum eStates { state_Off, state_On, state_Toggle, state_Blink };
    CDynamicLED();
    void GetColor(COLORREF& on, COLORREF& off);
    void SetColor(COLORREF on,COLORREF off = RGB(0,0,0));
    void SetColor(eColors);
    void SetBlink(int iTime_in_ms); // 0 means: blink off
    void SetOnOff(eStates state);
	eStates GetOnOff();
    virtual ~CDynamicLED();
    void SetShape(int iShape);

private:
    // The pens and brushes needed to do the drawing
    CPen m_PenBright,m_PenDark;
    CBrush m_BrushBright,m_BrushDark,m_BrushCurrent;

    // This variable is used to store the shape and color
    // set by the user for resetting the led later
    UINT m_nShape;
    bool m_bBright;
    bool m_bBlinking;
    UINT m_nBlinkRate;

    // Operations
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDynamicLED)
    //}}AFX_VIRTUAL

private:
    UINT_PTR m_TimerHandle;
    COLORREF m_OnColor;
    COLORREF m_OffColor;
    // Generated message map functions

protected:
    //{{AFX_MSG(CDynamicLED)
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT nIDEvent);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DYNAMICLED_H__7AA00BEC_B6E4_48A7_9719_3A15C0AB217A__INCLUDED_)
