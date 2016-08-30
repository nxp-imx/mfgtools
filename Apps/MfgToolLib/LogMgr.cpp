/*
 * Copyright 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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



