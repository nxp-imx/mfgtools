// Logger.cpp : implementation file
//

#include "stdafx.h"
#include "LogMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CLogger dialog
CLogMgr::CLogMgr()
	: CObject()
	, m_LoggingDataStr (_T(""))
{

}

CLogMgr::~CLogMgr(void)
{
}

// CLogMgr message handlers


void CLogMgr::LogIt(CString Data)
{
	m_LoggingDataStr+=Data;
}