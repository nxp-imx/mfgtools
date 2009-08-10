#pragma once
#include "stmsg.h"
// CEventLogger

class CEventLogger : public CWinThread
{
public:
	DECLARE_DYNCREATE(CEventLogger)
	CEventLogger();           // protected constructor used by dynamic creation
	virtual ~CEventLogger();

	typedef enum LogEvents {INVALID_LOGEVENT = -1, LOGEVENT_KILL, LOGEVENT_APPEND};

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

protected:
    CStdioFile m_file;
	virtual afx_msg void OnMsgLogEvent(WPARAM wEvent, LPARAM lpData);
	DECLARE_MESSAGE_MAP()
};


