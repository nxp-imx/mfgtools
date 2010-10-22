/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#ifndef CTOOLTIP_H
#define CTOOLTIP_H

//#define _WIN32_IE 0x0501
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

/**
*  Simple windows ToolTip class.
*  \author Torak
*  \version 0.1
*  \date    12.11.2004
*  \warning This class may explode in your face.
*
*  \code
*  // Example usage:
*
*  LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
*  {
*      static CToolTipAlways tip(1000);
*
*      tip.MsgProcedure(msg, wParam, lParam);
*
*      switch (msg)
*      {
*              case WM_CREATE:
*                  tip.Create(hInst, hwnd);
*                  tip.SetText("Hello");
*              break;
*      }
*  ... plah
*
*  \endcode
*  \note This code compile fine under MinGW :)
*/
class CToolTip
{
        public:
        CToolTip(/*const UINT id*/);
        ~CToolTip();
        void Create(HINSTANCE hInstance, HWND hwndParent);
        void SetPos(int x, int y);
        void Activate(BOOL fActivate);
        void SetText(LPTSTR szText);
        HWND GetHwnd() { return hwndTT; };
        /**
        * Simple exception class.
        */
        class xToolTip {};
        protected:
        void Initialize(HINSTANCE hInstance, HWND hwndParent);
        void TrackInit(HINSTANCE hInstance, HWND hwndParent);

        HWND hwndTT;    /**< Tooltip window handle. */
        TOOLINFO ti;    /**< Information about a tool. */
};

/**
* Display ToolTip in any place on the screen.
*/
class CToolTipScreen : public CToolTip
{
        public:
        /** Constructor
        *  \exception xToolTip()
        */
        CToolTipScreen(/*CONST UINT id*/) : CToolTip(/*id*/) {}
        /** Destructor
        */
        ~CToolTipScreen() {}
        void SetPos(int xPos, int yPos);
        void Activate(BOOL fActivate);
        void Create(HINSTANCE hInstance, HWND hwndParent);
};

/**
* Does not fade away tooltips.
*/
class CToolTipAlways : public CToolTip
{
        public:
        CToolTipAlways(/*CONST UINT id*/);
        ~CToolTipAlways();
        void MsgProcedure(UINT msg, WPARAM wParam, LPARAM lParam);
        void Create(HINSTANCE hInstance, HWND hwndParent);
        void Activate(BOOL fActivate);
//        private:
        TRACKMOUSEEVENT tm;
};

#endif // CTOOLTIP_H

