/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "Operation.h"

// COpMonitor

class COpMonitor : public COperation
{
	DECLARE_DYNCREATE(COpMonitor)

protected:
	virtual ~COpMonitor(void);
	DECLARE_MESSAGE_MAP()
	afx_msg void OnMsgStateChange(WPARAM nEventType, LPARAM dwData);

public:
	COpMonitor(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo* pOpInfo = NULL);
	virtual BOOL InitInstance(void);
//	virtual OpTypes GetType(void) { return COperation::MONITOR_OP; };
};


