/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once


// CUsbFilterListCtrl

class CUsbFilterListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CUsbFilterListCtrl)

public:
	CUsbFilterListCtrl();
	virtual ~CUsbFilterListCtrl();
	CMenu m_usb_filter_menu;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


