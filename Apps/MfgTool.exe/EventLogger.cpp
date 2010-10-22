/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// UpdateLog.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "EventLogger.h"


// CEventLogger

IMPLEMENT_DYNCREATE(CEventLogger, CWinThread)

CEventLogger::CEventLogger()
{
}

CEventLogger::~CEventLogger()
{
}

BOOL CEventLogger::InitInstance()
{
    WCHAR buffer[MAX_PATH];
    ::GetCurrentDirectory(MAX_PATH, (LPWSTR)buffer);
    CString filename = buffer;
    filename += _T("\\mfgtool.log");

    m_file.Open(filename, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareExclusive | CFile::osWriteThrough | CFile::typeText);

    if (m_file.m_hFile != CFile::hFileNull)
        m_file.SeekToEnd();

    return TRUE;
}

int CEventLogger::ExitInstance()
{
    if (m_file.m_hFile != CFile::hFileNull)
		m_file.Close();
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CEventLogger, CWinThread)
    ON_THREAD_MESSAGE(WM_MSG_LOGEVENT, OnMsgLogEvent)
END_MESSAGE_MAP()


// CEventLogger message handlers

//----------------------------------------------------------------
// OnMsgLogEvent()
//
// Handle update logging events.
//----------------------------------------------------------------
void CEventLogger::OnMsgLogEvent(WPARAM wEvent, LPARAM lpData)
{
    switch (wEvent)
    {
        case LOGEVENT_KILL:
            AfxEndThread(0);
            break;
        case LOGEVENT_APPEND:
        {
            CString text = (LPCTSTR)lpData;
            if (m_file.m_hFile != CFile::hFileNull)
            {
                m_file.WriteString(text);
            }
			m_file.Flush();
            SysFreeString((BSTR)lpData);
            break;
        }
        default:
            break;
    }
}
