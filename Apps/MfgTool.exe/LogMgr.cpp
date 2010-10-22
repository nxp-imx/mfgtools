/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
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
