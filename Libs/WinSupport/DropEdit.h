/*********************************************************************

   Copyright (C) 2002 Smaller Animals Software, Inc.

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

   3. This notice may not be removed or altered from any source distribution.

   http://www.smalleranimals.com
   smallest@smalleranimals.com
**********************************************************************/

/**********************************************************************
	Shortcut expansion code, 1996 by Rob Warner
	part of "CShortcut"
	rhwarner@southeast.net
	http://users.southeast.net/~rhwarner

	modified for CDropEdit, Chris Losinger
**********************************************************************/

#if !defined(AFX_DROPEDIT_H__1D8BBDC1_784C_11D1_8159_444553540000__INCLUDED_)
#define AFX_DROPEDIT_H__1D8BBDC1_784C_11D1_8159_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// DropEdit.h : header file
//

#include <shlobj.h>

/////////////////////////////////////////////////////////////////////////////
// CDropEdit window

class CDropEdit : public CEdit
{
// Construction
public:
	CDropEdit();

// Attributes
public:

// Operations
public:
	inline void SetUseDir(bool u)			{m_bUseDir=u;}
	inline bool IsUseDir() const			{return m_bUseDir;}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDropEdit)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDropEdit();

protected:
	CString ExpandShortcut(CString &inFile);

	// Generated message map functions
protected:
	//{{AFX_MSG(CDropEdit)
	afx_msg void OnDropFiles(HDROP dropInfo);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	bool		m_bUseDir;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DROPEDIT_H__1D8BBDC1_784C_11D1_8159_444553540000__INCLUDED_)
