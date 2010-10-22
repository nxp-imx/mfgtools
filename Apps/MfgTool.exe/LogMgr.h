/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// CLogMgr.h : header file
//

#pragma once

class CLogMgr : public CObject
{
public:
	CLogMgr();   // standard constructor
	virtual ~CLogMgr();

	void LogIt(CString Data);

protected:

	CString m_LoggingDataStr;
};
