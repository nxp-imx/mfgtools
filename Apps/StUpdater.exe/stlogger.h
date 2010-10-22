/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "stbase.h"

class CStLogger : public CStBaseToLogger
{
public:
	CStLogger(CString _logfilename, BOOL _log);
	virtual ~CStLogger(void);
	virtual ST_ERROR Log(CString _text);

private:
	BOOL            m_doLog;
	CStdioFile		m_logfile;
	CFileException	m_exception;

};
