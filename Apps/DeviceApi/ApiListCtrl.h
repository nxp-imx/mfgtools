/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once


// CApiListCtrl

class CApiListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CApiListCtrl)

public:
	CApiListCtrl();
	virtual ~CApiListCtrl();
	CMenu _paramMenu;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


