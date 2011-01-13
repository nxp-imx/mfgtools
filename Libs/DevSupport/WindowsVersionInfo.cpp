/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "WindowsVersionInfo.h"

// The one and only WindowsVersionInfo object
WindowsVersionInfo& gWinVersionInfo()
{
	static WindowsVersionInfo wvi;
	return wvi;
};
