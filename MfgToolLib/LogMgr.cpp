/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "logmgr.h"
//#include <mswmdm_i.c>

extern CString g_strVersion;

CMfgLogMgr::CMfgLogMgr()
{
	TCHAR buffer[MAX_PATH];
    ::GetModuleFileName(NULL, (LPTSTR)buffer, MAX_PATH);
    CString filename = buffer;

	int pos = filename.ReverseFind(_T('\\'));
	filename = filename.Left(pos+1);	//+1 for add '\' at the last
    filename += LOG_FILE_NAME;
	m_file = _tfopen(filename, _T("r+"));
	struct _stat64i32 FileLen;
	_tstat(filename, &FileLen);
	//BOOL bret = m_file.Open(filename, CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyNone | CFile::osWriteThrough | CFile::typeText);
	if(m_file==NULL)
	{
		throw 1;
	}
    if (m_file!=NULL)
        std::fseek(m_file,FileLen.st_size,SEEK_SET);

	CString strDllVersion = _T("");
	strDllVersion += g_strVersion;
	strDllVersion += _T("\n");
	WriteToLogFile(strDllVersion);

	time_t timer;
	time(&timer);// = CTime::GetCurrentTime();
	CString cstr_time = _T("");
    cstr_time.AppendFormat(_T("%s"),ctime(&timer));
	cstr_time += _T("   Start new logging\n");
	WriteToLogFile(cstr_time);
}

CMfgLogMgr::CMfgLogMgr(CString strFilename)
{
	m_file = _tfopen(strFilename, _T("a+"));
	struct _stat64i32 FileLen;
	_tstat(strFilename, &FileLen);
	//BOOL bret = m_file.Open(strFilename, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeReadWrite | CFile::shareDenyNone | CFile::osWriteThrough | CFile::typeText);
	if(m_file==NULL)
	{
		throw 1;
	}
	if (m_file != NULL)
		std::fseek(m_file, FileLen.st_size, SEEK_SET);

	CString strDllVersion = _T("");
	strDllVersion += g_strVersion;
	strDllVersion += _T("\n");// added to this function to presumably add logging of version  similar to function above
	WriteToLogFile(strDllVersion);

	time_t timer;
	time(&timer);// = CTime::GetCurrentTime();
	CString cstr_time = _T("");
	cstr_time.AppendFormat(_T("%s"), ctime(&timer));
	cstr_time += _T("   Start new logging\n");
	WriteToLogFile(cstr_time);
}

CMfgLogMgr::~CMfgLogMgr()
{
    if (m_file!=NULL)
	{
        std::fclose(m_file);
		m_file = NULL;
	}
}

DWORD CMfgLogMgr::WriteToLogFile(CString& strMsg)
{
	if (m_file != NULL)
    {
		_fputts(strMsg, m_file);
		std::fflush(m_file);
		return WRITE_SUCCESS;
	}
	else
	{
		return WRITE_ERROR;
	}
}

void CMfgLogMgr::PrintLog(DWORD moduleID, DWORD levelID, const TCHAR * format, ... )
{
    TCHAR* buffer;
    va_list args;
    int len;
    
    va_start(args, format);
    len = _vsctprintf(format, args)+1;
    buffer = (TCHAR*)malloc(len*sizeof(TCHAR));
    std::vswprintf(buffer,len, format, args);
    va_end(args);

    CString str;
    str.Format(_T("ModuleID[%d] LevelID[%d]: %s\n"),moduleID, levelID, buffer);
	if (m_file != NULL)
    {
		_fputts(str,m_file);
		std::fflush(m_file);
	}
    
    free(buffer);
}



