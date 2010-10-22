/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// DynamicLED.cpp : implementation file
//

#include "stdafx.h"
#include "DynamicLED.h"


#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
    static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDynamicLED

CDynamicLED::CDynamicLED()
{
    // Initialize the variables
    m_bBright = false;
    m_bBlinking = false;
    m_nShape = ID_SHAPE_ROUND;
    m_TimerHandle=0;
    m_nBlinkRate=300;
}

CDynamicLED::~CDynamicLED()
{
}

BEGIN_MESSAGE_MAP(CDynamicLED, CStatic)
    //{{AFX_MSG_MAP(CDynamicLED)
    ON_WM_PAINT()
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
    END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDynamicLED message handlers


void CDynamicLED::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
    // If the timer value is zero, we dont want to do anything
    // It means that the LED is in a switched off state. So just return

    // Get the Device Context
    // CClientDC dc(this);

    // Get the rectangle of the window where we are going to draw
    CRect rcClient;
    GetClientRect(&rcClient);

    // If the pen has been selected already, then we have to delete it
    // so that it doesnt throw an assertion

    if(m_PenBright.m_hObject!=NULL)
        m_PenBright.DeleteObject();

    if(m_BrushBright.m_hObject!=NULL)
        m_BrushBright.DeleteObject();

    if(m_PenDark.m_hObject!=NULL)
        m_PenDark.DeleteObject();

    if(m_BrushDark.m_hObject!=NULL)
        m_BrushDark.DeleteObject(); 


    m_PenBright.CreatePen(0,1,m_OnColor);
    m_BrushBright.CreateSolidBrush(m_OnColor);

    m_PenDark.CreatePen(0,1,m_OffColor);
    m_BrushDark.CreateSolidBrush(m_OffColor);

    if(m_bBright==TRUE)
    {
        // If we have to switch on the LED to display the bright colour select
        // the bright pen and brush that we have created above

        dc.SelectObject(&m_PenBright);
        dc.SelectObject(&m_BrushBright);

        m_BrushCurrent.m_hObject = m_BrushBright.m_hObject;

        // m_bBright = FALSE;
    }
    else
    {
        // If we have to switch off the LED to display the dark colour select
        // the bright pen and brush that we have created above

        dc.SelectObject(&m_PenDark);
        dc.SelectObject(&m_BrushDark);

        m_BrushCurrent.m_hObject = m_BrushDark.m_hObject;

        // m_bBright = TRUE;
    }

    // If the round shape has been selected for the control 
    if(m_nShape==ID_SHAPE_ROUND)
    {
        // Draw the actual colour of the LED
        dc.Ellipse(rcClient);

        // Draw a thick dark gray coloured circle
        CPen Pen;
        Pen.CreatePen(0,2,RGB(128,128,128));
        dc.SelectObject(&Pen);
        dc.Ellipse(rcClient);

        // Draw a thin light gray coloured circle
        CPen Pen2;
        Pen2.CreatePen(0,1,RGB(192,192,192));
        dc.SelectObject(&Pen2);
        dc.Ellipse(rcClient);

        // Draw a white arc at the bottom
        CPen Pen3;
        Pen3.CreatePen(0,1,RGB(255,255,255));
        dc.SelectObject(&Pen3);

        // The arc function is just to add a 3D effect for the control
        CPoint ptStart,ptEnd;
        ptStart = CPoint(rcClient.Width()/2,rcClient.bottom);
        ptEnd = CPoint(rcClient.right,rcClient.top);

        dc.MoveTo(ptStart);
        dc.Arc(rcClient,ptStart,ptEnd);

        CBrush Brush;
        Brush.CreateSolidBrush(RGB(255,255,255));
        dc.SelectObject(&Brush);

        // Draw the actual ellipse
        dc.Ellipse(rcClient.left+4,rcClient.top+4,rcClient.left+6,rcClient.top+6);
    }
    else if(m_nShape==ID_SHAPE_SQUARE)
    {
        // If you have decided that your LED is going to look square in shape, then

        // Draw the actual rectangle
        dc.FillRect(rcClient,&m_BrushCurrent);

        // The code below gives a 3D look to the control. It does nothing more

        // Draw the dark gray lines
        CPen Pen;
        Pen.CreatePen(0,1,RGB(128,128,128));
        dc.SelectObject(&Pen);

        dc.MoveTo(rcClient.left,rcClient.bottom);
        dc.LineTo(rcClient.left,rcClient.top);
        dc.LineTo(rcClient.right,rcClient.top);


        // Draw the light gray lines
        CPen Pen2;
        Pen2.CreatePen(0,1,RGB(192,192,192));
        dc.SelectObject(&Pen2);

        dc.MoveTo(rcClient.right,rcClient.top);
        dc.LineTo(rcClient.right,rcClient.bottom);
        dc.LineTo(rcClient.left,rcClient.bottom); 
    }
}

void CDynamicLED::OnTimer(UINT nIDEvent) 
{
    m_bBright = !m_bBright;

    RedrawWindow();
    CStatic::OnTimer(nIDEvent);
}

void CDynamicLED::SetOnOff(eStates state)
{
//  enum eStates { state_Off, state_On, state_Toggle, state_Blink };

    switch (state)
    {
    case CDynamicLED::state_On:
        SetBlink(0);
        m_bBright = true;
        break;
    case CDynamicLED::state_Toggle:
        SetBlink(0);
        m_bBright = !m_bBright;
        break;
    case CDynamicLED::state_Blink:
        if ( !m_bBlinking )
        {
            SetBlink(m_nBlinkRate);
            m_bBright = true;
        }
        break;
    case CDynamicLED::state_Off:
    default:
        SetBlink(0);
        m_bBright = false;
        break;
    }

    RedrawWindow();
}

CDynamicLED::eStates CDynamicLED::GetOnOff()
{
//  enum eStates { state_Off, state_On, state_Toggle, state_Blink };

    if ( m_bBlinking )
		return state_Blink;
	else if ( m_bBright )
		return state_On;
	else
		return state_Off;
}

void CDynamicLED::SetBlink(int iTime_in_ms)
{
    if(m_TimerHandle)
    {
        ::KillTimer(this->m_hWnd,m_TimerHandle);
        m_TimerHandle=0;
    }

    if(iTime_in_ms > 0)
    {
        m_TimerHandle = ::SetTimer(this->m_hWnd,1001,iTime_in_ms,NULL);
        m_bBlinking = true;
    }
    else
        m_bBlinking = false;
}

void CDynamicLED::SetColor(COLORREF on, COLORREF off)
{
    m_OnColor = on;
    m_OffColor = off;
}

void CDynamicLED::SetColor(CDynamicLED::eColors color)
{
//  enum color { color_Red, color_Yellow, color_Green, color_Blue };

    switch(color)
    {
    case color_Yellow:
        m_OnColor =  RGB(200,200,0);
        m_OffColor =  RGB(150,150,0);
        break;
    case color_Green:
        m_OnColor =  RGB(0,250,0);
        m_OffColor =  RGB(0,150,0);
        break;
    case color_Blue:
        m_OnColor =  RGB(0,0,250);
        m_OffColor =  RGB(0,0,150);
        break;
    case color_Red:
    default:
        m_OnColor =  RGB(250,0,0);
        m_OffColor =  RGB(150,0,0);
        break;
    }
}

void CDynamicLED::GetColor(COLORREF& on, COLORREF& off)
{
    on =  m_OnColor;
    off =  m_OffColor;
}

void CDynamicLED::SetShape(int iShape)
{
    m_nShape=iShape;
}
