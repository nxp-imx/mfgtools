#pragma once
#include "stbase.h"

class CStLogger : public CStBaseToLogger
{
public:
	CStLogger(CString _logfilename, BOOL _log);
	virtual ~CStLogger(void);
	virtual ST_ERROR Log(CString _text);

private:
	BOOL            m_doLog;
	CStdioFile		m_logfile;
	CFileException	m_exception;

};
