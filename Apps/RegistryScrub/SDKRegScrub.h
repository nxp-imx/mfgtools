/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// SDKRegScrub.h : main header file for the PROJECT_NAME application
// Copyright (c) 2005 SigmaTel, Inc.

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

// CSDKRegScrubApp:
// See SDKRegScrub.cpp for the implementation of this class

class CSDKRegScrubApp : public CWinApp
{
public:
	CSDKRegScrubApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CSDKRegScrubApp theApp;
