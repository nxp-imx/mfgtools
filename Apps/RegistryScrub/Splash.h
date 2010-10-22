/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// Splash.h : header file
// Copyright (c) 2005 SigmaTel, Inc.

#pragma once

#include "resource.h"

// CSplash dialog

class CSplash : public CDialog
{
	DECLARE_DYNAMIC(CSplash)

public:
	CSplash(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSplash();

	BOOL Create(CWnd* pParent);

	// Dialog Data
	enum { IDD = IDD_SPLASH_DIALOG };

protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support	
	DECLARE_MESSAGE_MAP()
};
