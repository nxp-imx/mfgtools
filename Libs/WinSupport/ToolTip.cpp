/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "tooltip.h" // class's header file

/** Constructor
*  \exception xToolTip
*/
CToolTip::CToolTip(/*const UINT id*/)
{
        // Initialize common controls
        INITCOMMONCONTROLSEX iccex;

        iccex.dwICC = ICC_WIN95_CLASSES;
        iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        if(!InitCommonControlsEx(&iccex))
        throw xToolTip();

        ti.uId = 1000/*id*/;
}

/** Create new tool
*  \param hInstance Application instance.
*  \param hwndParent Mother window handle.
*  \exception xToolTip()
*/
void CToolTip::Create(HINSTANCE hInstance, HWND hwndParent)
{
        RECT rect;

        Initialize(hInstance, hwndParent);

        // Get coordinates of the main client area
        if(!GetClientRect (hwndParent, &rect))
        throw xToolTip();

        ti.rect.left = rect.left;
        ti.rect.top = rect.top;
        ti.rect.right = rect.right;
        ti.rect.bottom = rect.bottom;

        if(!SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti))
        throw xToolTip();
}

/** Destructor.
*/
CToolTip::~CToolTip()
{

}

/** Set tooltip text position in client coordinates.
*  \param xPos The x-coordinate of the point at which
*  the ToolTip will be displayed.
*  \param yPos The y-coordinate of the point at which
*  the ToolTip will be displayed.
*  \exception xToolTip()
*/
void CToolTip::SetPos(int xPos, int yPos)
{
        POINT pt;

        pt.x = xPos;
        pt.y = yPos;
        if(!ClientToScreen(GetParent(hwndTT), &pt))
        throw xToolTip();
        // -25 set text to over mouse pointer.
        SendMessage(hwndTT, TTM_TRACKPOSITION, 0,
        (LPARAM)MAKELONG(pt.x, pt.y));
}

/** Activates or deactivates a ToolTip control.
*  \param fActivate Activation flag. If this parameter is TRUE, the ToolTip
*  control is activated. If it is FALSE, the ToolTip control is deactivated.
*/
void CToolTip::Activate(BOOL fActivate)
{
        SendMessage(hwndTT, TTM_ACTIVATE, fActivate, 0);
}

/** Sets the ToolTip text for a tool.
*  \param szText Pointer to the buffer that contains the text for the tool.
*/
void CToolTip::SetText(LPTSTR szText)
{
        // No return value
        ti.lpszText = szText;
        SendMessage(hwndTT, TTM_UPDATETIPTEXT, 0, (LPARAM) &ti);
}

/** Set tooltip text position in screen coordinates.
*  \param xPos The x-coordinate of the point at which
*  the ToolTip will be displayed.
*  \param yPos The y-coordinate of the point at which
*  the ToolTip will be displayed.
*/
void CToolTipScreen::SetPos(int xPos, int yPos)
{
        // -25 set text to over mouse pointer.
        SendMessage(hwndTT, TTM_TRACKPOSITION, 0,
        (LPARAM)MAKELONG(xPos, yPos-25));
}

void CToolTipScreen::Activate(BOOL fActivate)
{
        SendMessage(hwndTT, TTM_TRACKACTIVATE, fActivate, (LPARAM) &ti);
}

void CToolTipScreen::Create(HINSTANCE hInstance, HWND hwndParent)
{
        Initialize(hInstance, hwndParent);
        TrackInit(hInstance, hwndParent);
}

/** Initialize common data structures for tooltip control.
*  \param hInstance Application instance.
*  \param hwndParent Mother window handle.
*  \exception xToolTip()
*/
void CToolTip::Initialize(HINSTANCE hInstance, HWND hwndParent)
{
        // Create a tooltip window
        hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,
        NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwndParent, NULL, hInstance, NULL);
        if(hwndTT == NULL)
        throw xToolTip();

        if(!SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
        throw xToolTip();

        // Initialize members of the toolinfo structure
        ti.cbSize = sizeof(TOOLINFO);
        ti.hwnd = hwndParent;
        ti.hinst = hInstance;
        ti.lpszText = _T("ToolTip");
        ti.uFlags = TTF_SUBCLASS;

        SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);
		SendMessage(hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, SHRT_MAX);
		SendMessage(hwndTT, TTM_SETDELAYTIME, TTDT_INITIAL, 200);
		SendMessage(hwndTT, TTM_SETDELAYTIME, TTDT_RESHOW, 200);
}

/** Constructor
*  \exception xToolTip()
*/
CToolTipAlways::CToolTipAlways(/*CONST UINT id*/) : CToolTip(/*id*/)
{
        tm.cbSize = sizeof(TRACKMOUSEEVENT);
        tm.dwFlags = TME_LEAVE;
}

CToolTipAlways::~CToolTipAlways()
{
}

/** Proceed mother window messages.
*  \param msg Message.
*  \param wParam Message parameter.
*  \param lParam Message parameter.
*/
void CToolTipAlways::MsgProcedure(UINT msg, WPARAM wParam, LPARAM lParam)
{
        switch(msg)
        {
                case WM_MOUSEMOVE:
                SetPos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                TrackMouseEvent(&tm);
                Activate(TRUE);
                break;

                case WM_MOUSELEAVE:
                Activate(FALSE);
                break;
        }
}

/** Create new tool
*  \param hInstance Application instance.
*  \param hwndParent Mother window handle.
*  \exception xToolTip()
*/
void CToolTipAlways::Create(HINSTANCE hInstance, HWND hwndParent)
{
        Initialize(hInstance, hwndParent);
        TrackInit(hInstance, hwndParent);
        tm.hwndTrack = hwndParent;
}

/** Initialize common data structures for Tracking ToolTips.
*  \param hInstance Application instance.
*  \param hwndParent Mother window handle.
*  \exception xToolTip()
*/
void CToolTip::TrackInit(HINSTANCE hInstance, HWND hwndParent)
{
        ti.uFlags = TTF_SUBCLASS | TTF_TRACK ;
        // ToolTip control will cover the whole window
        ti.rect.left = 0;
        ti.rect.top = 0;
        ti.rect.right = 0;
        ti.rect.bottom = 0;

        if(!SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti))
        throw xToolTip();
}

/** Activates or deactivates a ToolTip control.
*  \param fActivate Activation flag. If this parameter is TRUE, the ToolTip
*  control is activated. If it is FALSE, the ToolTip control is deactivated.
*/
void CToolTipAlways::Activate(BOOL fActivate)
{
        SendMessage(hwndTT, TTM_TRACKACTIVATE, fActivate, (LPARAM) &ti);
}
