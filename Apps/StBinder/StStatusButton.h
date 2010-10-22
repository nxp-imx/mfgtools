/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxwin.h"
#include "afxcmn.h"

class CStStatusButton :
	public CButton
{
public:
	CStStatusButton(void);
public:
	virtual ~CStStatusButton(void);

	void SetMouseMoveCallback(MOUSEMOVECALLBACK& mmcb);
	MOUSEMOVECALLBACK	m_MMCallBack;

protected:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
