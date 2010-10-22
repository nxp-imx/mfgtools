/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxcmn.h"

/////////////////////////////////////////////////////////////////////////////
// CDragDropTreeCtrl window
#include "TreeDropTarget.h"

typedef HRESULT (CALLBACK FAR * LPFN_TREE_DROP_FILES_CALLBACK)(CTreeCtrl*, PVOID pCallerClass, CString&, UINT&);

class CStGrpTreeCtrl :
	public CTreeCtrl
{
public:
	CStGrpTreeCtrl(void);
	virtual ~CStGrpTreeCtrl(void);

	enum DLDropFlags
	{
		DL_ACCEPT_FILES =       0x01,	// Allow files to be dropped
		DL_ACCEPT_FOLDERS =		0x02,	// Allow folders to be droppped
		DL_ALLOW_DUPLICATES =	0x04,	// Allow a pathname to be dropped even if its already in the list (ignored if you specify a callback function)
		DL_FILTER_EXTENSION =	0x10,	// Only accept files with the specified extension. Specify in csFileExt
		DL_USE_CALLBACK =		0x20,	// Receive a callback for each item dropped, specified in pfnCallback (you have responsibility for inserting items into the list)

		DL_FOLDER_TYPE =		0x40,	// Returned to the callback function - indicating the type of path dropped
		DL_FILE_TYPE =			0x80
	};

	struct DROPTREEMODE { 
		UINT iMask;								// Specifies what type of items to accept - a combination of the above flags
		CString csFileExt;						// The file extension on which to filter. Use the format ".extension". Ignored unless DL_FILTER_EXTENSION is specified
		PVOID pCallerClass;						// Caller provided class ptr
		LPFN_TREE_DROP_FILES_CALLBACK pfnCallback;	// Address of your callback function. Ignored unless DL_USE_CALLBACK is specified
	}; 
	BOOL SetDropMode(CStGrpTreeCtrl::DROPTREEMODE& dropMode);

//	void SetMouseMoveCallback(MOUSEMOVECALLBACK& mmcb);
//	MOUSEMOVECALLBACK	m_MMCallBack;

private:
	DROPTREEMODE m_dropMode;
	CTreeDropTarget m_CTreeDropTarget;

protected:
	afx_msg void OnDropFiles(HDROP dropInfo);
	BOOL ValidatePathname(CString& csPathname, UINT& iPathType);
	CString ExpandShortcut(CString& csFilename);

//	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
