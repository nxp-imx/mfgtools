/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StVersionInfo.h: interface for the CStVersionInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STVERSIONINFO_H__72EA2FE9_CF2B_403B_91DB_A14C9C3F14F7__INCLUDED_)
#define AFX_STVERSIONINFO_H__72EA2FE9_CF2B_403B_91DB_A14C9C3F14F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CStVersionInfo
{
public:
	CStVersionInfo();
	virtual ~CStVersionInfo();

	USHORT GetHigh() const;
	USHORT GetMid() const;
	USHORT GetLow() const;

	void SetHigh(DWORD high, bool _bcd = false);
	void SetMid(DWORD Mid, bool _bcd = false);
	void SetLow(DWORD low, bool _bcd = false);

	void GetVersionString(CString& _verStr);

private:
	USHORT	m_high;
	USHORT	m_mid;
	USHORT	m_low;

	////////////////////////////////////////////////////////////////////////////////
	//! Converts a four digit BCD number to the decimal equivalent.
	//! Remember that the BCD value is big endian but read as a 16-bit little 
	//! endian number. So we have to byte swap during this conversion.
	//!
	//! \param bcdNumber BCD value in reverse byte order.
	//! \return A decimal version of \a bcdNumber.
	////////////////////////////////////////////////////////////////////////////////
	inline USHORT BCD2D(USHORT bcdNumber) {
		USHORT resultVersion;
		resultVersion = ((bcdNumber & 0x000f) * 100);
		resultVersion += ( ((bcdNumber & 0x00f0) >> 4) * 1000);
		resultVersion += ( ((bcdNumber & 0x0f00) >> 8) * 1);
		resultVersion += ( ((bcdNumber & 0xf000) >> 12) * 10);
		return resultVersion;
	} 
};

#endif // !defined(AFX_STVERSIONINFO_H__72EA2FE9_CF2B_403B_91DB_A14C9C3F14F7__INCLUDED_)
