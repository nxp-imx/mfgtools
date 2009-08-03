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


