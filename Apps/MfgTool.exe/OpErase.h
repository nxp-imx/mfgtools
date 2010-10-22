/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "Operation.h"

// COpErase

class COpErase : public COperation
{
	DECLARE_DYNCREATE(COpErase)

protected:
	DECLARE_MESSAGE_MAP()

	virtual ~COpErase();
	INT_PTR DoErase(void);

	enum OpState_t{ INVALID = 0, WAITING, ERASING, COMPLETE };
	OpState_t m_OpState;
	CString GetOpStateString(OpState_t _state) const;
	DWORD	m_dwStartTime;

public:
	COpErase(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo* pOpInfo = NULL);
	afx_msg void OnMsgStateChange(WPARAM nEventType, LPARAM dwData);
};


