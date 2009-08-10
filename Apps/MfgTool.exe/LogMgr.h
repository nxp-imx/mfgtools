// CLogMgr.h : header file
//

#pragma once

class CLogMgr : public CObject
{
public:
	CLogMgr();   // standard constructor
	virtual ~CLogMgr();

	void LogIt(CString Data);

protected:

	CString m_LoggingDataStr;
};
