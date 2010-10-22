/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StTrace.h: interface for the CStTrace class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STTRACE_H__BC056D94_A036_4961_9140_8E2645F60676__INCLUDED_)
#define AFX_STTRACE_H__BC056D94_A036_4961_9140_8E2645F60676__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CStTrace  
{
public:
	CStTrace();
	virtual ~CStTrace();

	static void trace( string );
};

#endif // !defined(AFX_STTRACE_H__BC056D94_A036_4961_9140_8E2645F60676__INCLUDED_)
