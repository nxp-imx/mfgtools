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

	BOOL bret = m_file.Open(filename, CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyNone | CFile::osWriteThrough | CFile::typeText);
	if(!bret)
	{
		throw 1;
	}
    if (m_file.m_hFile != CFile::hFileNull)
        m_file.SeekToEnd();

	CString strDllVersion = _T("");
	strDllVersion += g_strVersion;
	strDllVersion += _T("\n");
	WriteToLogFile(strDllVersion);

	CTime time = CTime::GetCurrentTime();
	CString cstr_time = _T("");
    cstr_time += time.Format("%#c");
	cstr_time += _T("   Start new logging\n");
	WriteToLogFile(cstr_time);
}

CMfgLogMgr::CMfgLogMgr(CString strFilename)
{
	BOOL bret = m_file.Open(strFilename, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeReadWrite | CFile::shareDenyNone | CFile::osWriteThrough | CFile::typeText);
	if(!bret)
	{
		throw 1;
	}
    if (m_file.m_hFile != CFile::hFileNull)
        m_file.SeekToEnd();

    CTime time = CTime::GetCurrentTime();
    CString cstr_time = _T("");
    cstr_time += time.Format("%#c");
	cstr_time += _T("   Start new logging\n");

    //PrintLog(0,0,cstr_time.GetBuffer());
    //cstr_time.ReleaseBuffer();
	WriteToLogFile(cstr_time);
}

CMfgLogMgr::~CMfgLogMgr()
{
    if (m_file.m_hFile != CFile::hFileNull)
	{
        m_file.Close();
	}
}

DWORD CMfgLogMgr::WriteToLogFile(CString& strMsg)
{
	if (m_file.m_hFile != CFile::hFileNull)
    {
		m_file.WriteString(strMsg);
        m_file.Flush();
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
    if (m_file.m_hFile != CFile::hFileNull)
    {
		m_file.WriteString(str);
        m_file.Flush();
	}
    
    free(buffer);
}



