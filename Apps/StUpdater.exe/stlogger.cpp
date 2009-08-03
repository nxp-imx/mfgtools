#include "Stdafx.h"
#include "sterror.h"
#include ".\stlogger.h"

CStLogger::CStLogger(CString _logfilename, BOOL _log)
: m_doLog(_log)
{
	if ( _log )
	{
		if ( _logfilename.IsEmpty() )
		{
			_logfilename = _T("updater_log.txt");
		}
		if( !m_logfile.Open( _logfilename, CFile::modeCreate | CFile::shareDenyWrite |
			/*CFile::modeNoTruncate |*/ CFile::modeWrite | CFile::typeText, &m_exception ))
		{
			m_last_error = STERR_FAILED_TO_OPEN_FILE;
		}
		else
		{
			m_logfile.SeekToEnd();
		}
	}
	else
		m_last_error = STERR_FAILED_TO_OPEN_FILE;
}

CStLogger::~CStLogger(void)
{
	if (m_doLog)
	{
		m_logfile.Flush();
		m_logfile.Close();
	}
}

ST_ERROR CStLogger::Log(CString _text)
{
	if ( m_doLog )
	{
		if(m_last_error == STERR_FAILED_TO_OPEN_FILE)
			return m_last_error;

		CTime t = CTime::GetCurrentTime();
		CString timestamp = t.Format(CString("%c: "));
		m_logfile.WriteString( timestamp + _text + CString("\n"));
		m_logfile.Flush();
	}

	return STERR_NONE;
}
