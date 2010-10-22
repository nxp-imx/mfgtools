/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StBinder.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CStBinderApp:
// See StBinder.cpp for the implementation of this class
//

class CStBinderApp : public CWinApp
{
public:
	CStBinderApp();

// Overrides
	public:
	virtual BOOL InitInstance();
	BOOL GetLangIds();
	void SaveLangIds();
	BOOL PumpMessages();
	BOOL GetString(UINT _iResID, CString& _str);

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CStBinderApp theApp;
