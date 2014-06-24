/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "stdafx.h"
#include <fstream>   
#include <iostream>

//if want to change log file name, change it, don't change code
#define LOG_FILE_NAME		_T("MfgTool.log")

#define WRITE_SUCCESS		0
#define WRITE_ERROR			1


//////////////////////////////////////////////////////////////////////
//
// MfgLogMgr
//
//////////////////////////////////////////////////////////////////////
class CMfgLogMgr
{
public:
    CMfgLogMgr();
	CMfgLogMgr(CString strFilename);
    ~CMfgLogMgr();

	void PrintLog(DWORD moduleID, DWORD levelID, const wchar_t * format, ... );

	DWORD WriteToLogFile(CString& strMsg);

private:
    CStdioFile m_file; 
};


