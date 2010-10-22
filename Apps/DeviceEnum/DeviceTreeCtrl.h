/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once


// CDeviceTreeCtrl

class CDeviceTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CDeviceTreeCtrl)

public:
	CDeviceTreeCtrl();
	virtual ~CDeviceTreeCtrl();

protected:
	CMenu m_eject_menu;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


