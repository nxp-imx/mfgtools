/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#if !defined(AFX_COLORBOX_H__1F411462_E4B2_11D8_B14D_002018574596__INCLUDED_)
#define AFX_COLORBOX_H__1F411462_E4B2_11D8_B14D_002018574596__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StColorBtn.h : header file
//

/*
	CStColorBtn is a very simple button that shows a color and
	let the user change the color by clicking on it.

    It's not more complicated than that :-).

    Get the latest version at http://www.codeproject.com

    PEK
  */
/////////////////////////////////////////////////////////////////////////////
// CStColorBtn window

class CStColorBtn : public CButton
{
// Construction
public:
	CStColorBtn();

// Attributes
public:

// Operations
public:


	//Redraw the box
	void Redraw();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStColorBtn)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStColorBtn();
	void SetColors(COLORREF bkGrd, COLORREF text);

	// Generated message map functions
protected:
	COLORREF m_color;
	COLORREF m_bkColor;
	COLORREF m_textColor;

	//{{AFX_MSG(CStColorBtn)
	afx_msg BOOL OnClickedEx();
	//}}AFX_MSG

//	DECLARE_MESSAGE_MAP()
};


#define RGB_BLACK				RGB(0,0,0)
#define RGB_STOP_RED			RGB(200,0,0)
#define RGB_GO_GREEN			RGB(0,200,0)
#define RGB_STOPPING_YELLOW		RGB(255,255,0)
#define RGB_STOPPING_RED_TEXT	RGB(128,0,0)
#define RGB_WHITE				RGB(255,255,255)

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORBOX_H__1F411462_E4B2_11D8_B14D_002018574596__INCLUDED_)
