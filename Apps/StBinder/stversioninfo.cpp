/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StVersionInfo.cpp: implementation of the CStVersionInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "StVersionInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStVersionInfo::CStVersionInfo()
{
	m_high = m_mid = m_low = 0;
}

CStVersionInfo::~CStVersionInfo()
{

}

USHORT CStVersionInfo::GetHigh() const
{
	return m_high;
}

USHORT CStVersionInfo::GetMid() const
{
	return m_mid;
}

USHORT CStVersionInfo::GetLow() const
{
	return m_low;
}

void CStVersionInfo::SetHigh(DWORD _high, bool _bcd)
{
	m_high =  _bcd ? BCD2D((USHORT)_high) : (USHORT)_high;
}

void CStVersionInfo::SetMid(DWORD _mid, bool _bcd)
{
	m_mid =  _bcd ? BCD2D((USHORT)_mid) : (USHORT)_mid;
}

void CStVersionInfo::SetLow(DWORD _low, bool _bcd)
{
	m_low =  _bcd ? BCD2D((USHORT)_low) : (USHORT)_low;
}


void CStVersionInfo::GetVersionString(CString& _verStr)
{
    CString csTmp;

    csTmp.Format(L"%03d.%03d.%03d", GetHigh(), GetMid(), GetLow());

    _verStr = csTmp;
	return;
}



