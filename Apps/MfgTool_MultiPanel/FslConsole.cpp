/*
 * Copyright 2012-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <io.h>
#include <fcntl.h> 
#include <iostream>
#include "FslConsole.h"
#include "CommonDef.h"

#include "../MfgToolLib/MfgToolLib_Export.h"

using namespace std;

CFSLConsole *g_pConsole = NULL;
BOOL g_bAttachConsole = FALSE;

IMPLEMENT_DYNCREATE(CFSLConsole, CWinThread)

CFSLConsole::CFSLConsole()
{
	m_bAutoDelete = FALSE; //Don't destroy the object at thread termination
}

CFSLConsole::~CFSLConsole()
{
}

BOOL CFSLConsole::CreateConsole()
{
	BOOL bret;

	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		bret = TRUE;
	}
	else
	{
		bret = AllocConsole();
	}
	
	if(bret)
	{
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		ios_base::sync_with_stdio(); 
	}

	return bret;
}

void CFSLConsole::DeleteConsole()
{
	FreeConsole();
}

BOOL CFSLConsole::Open()
{
	m_hThreadStartEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if(m_hThreadStartEvent == NULL)
	{
		CString strMsg;
		strMsg.Format(_T("CFSLConsole::Open---Create m_hThreadStartEvent error"));
		OutputToConsole(strMsg);
		return FALSE;
	}

	if( CreateThread() != 0 )
	{
		::WaitForSingleObject(m_hThreadStartEvent, INFINITE);
	}
	else
	{
		::CloseHandle(m_hThreadStartEvent);
		return FALSE;
	}
	::CloseHandle(m_hThreadStartEvent);
	m_hThreadStartEvent = NULL;

	return TRUE;
}

void CFSLConsole::Close()
{
}

BOOL CFSLConsole::InitInstance()
{
	SetEvent(m_hThreadStartEvent);

	return TRUE;
}

BEGIN_MESSAGE_MAP(CFSLConsole, CWinThread)
    ON_THREAD_MESSAGE(UM_MODIFY_LINE, OnModifySpecifiedLine)
END_MESSAGE_MAP()

void CFSLConsole::OnModifySpecifiedLine(WPARAM MsgPos, LPARAM szMsg)
{
	MSG_CURSOR_POSITION *pMsgPos = (MSG_CURSOR_POSITION *)MsgPos;
	CString strMsgNew = (LPCTSTR)szMsg;
	SysFreeString((BSTR)szMsg);

	GoToXY(pMsgPos->x, pMsgPos->y);

	CString strSpaces = _T("");
	for(int i=0; i<pMsgPos->length; i++)
	{
		strSpaces += _T(" ");
	}
	_tprintf(_T("%s"), strSpaces);
	_tprintf(_T("\r"));

	_tprintf(_T("%s"), strMsgNew);

	pMsgPos->length = strMsgNew.GetLength();
}
