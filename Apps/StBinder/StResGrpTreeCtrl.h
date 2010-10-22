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

typedef HRESULT (CALLBACK FAR * LPFN_TREE_DROP_FILES_CALLBACK)(CTreeCtrl*, PVOID pCallerClass, HDROP dropInfo);

class CStResGrpTreeCtrl :
	public CTreeCtrl
{
public:
	CStResGrpTreeCtrl(void);
	virtual ~CStResGrpTreeCtrl(void);

	struct DROPTREEMODE { 
		PVOID pCallerClass;							// Caller provided class ptr
		LPFN_TREE_DROP_FILES_CALLBACK pfnCallback;	// Address of your callback function. 
	}; 
	BOOL SetDropMode(CStResGrpTreeCtrl::DROPTREEMODE& dropMode);

	void SetMouseMoveCallback(MOUSEMOVECALLBACK& mmcb);
	MOUSEMOVECALLBACK	m_MMCallBack;

private:
	DROPTREEMODE m_dropMode;
	CTreeDropTarget m_CTreeDropTarget;

protected:
	afx_msg void OnDropFiles(HDROP dropInfo);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
