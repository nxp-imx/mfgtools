/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "StVersionInfo.h"

// CCustomSupport
class CCustomSupport
{
public:
	CCustomSupport();
	virtual ~CCustomSupport();

	CString GetVersionString(CStVersionInfo* _ver);

protected:
//	DECLARE_MESSAGE_MAP()
};


