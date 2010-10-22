/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/*********************************************************************
* HotProp Control, version 1.10 (January 14, 2004)
* Copyright (C) 2002-2004 Michal Mecinski.
*
* You may freely use and modify this code, but don't remove
* this copyright note.
*
* THERE IS NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, FOR
* THIS CODE. THE AUTHOR DOES NOT TAKE THE RESPONSIBILITY
* FOR ANY DAMAGE RESULTING FROM THE USE OF IT.
*
* E-mail: mimec@mimec.org
* WWW: http://www.mimec.org
********************************************************************/

#pragma once

#define HPEM_ENDEDIT		(WM_USER+1702)	// bDone

class CHPDynEdit : public CEdit
{
public:
	CHPDynEdit(CRect& rcLoc, LPCTSTR lpszText, int nLimit,
		CPoint& ptAct, int fFlags, CFont* pFont, CWnd* pParent);

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void PostNcDestroy();

protected:
	BOOL m_bDone;
	CWnd* m_pPrevWnd;

protected:
	afx_msg void OnKillFocus();
	DECLARE_MESSAGE_MAP()
};

