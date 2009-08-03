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


