/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StVersionInfo.h: interface for the StVersionInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STVERSIONINFO_H__INCLUDED_)
#define AFX_STVERSIONINFO_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Libs/Public/StdString.h"
#include "Common/StdInt.h"

struct StVersionInfo
{
	StVersionInfo(uint16_t major=0, uint16_t minor=0, uint16_t rev=0);
    StVersionInfo(LPCSTR pVersionString);
	virtual ~StVersionInfo() {};

	bool operator != (const StVersionInfo&) const;
	bool operator == (const StVersionInfo&) const;
	bool operator > (const StVersionInfo& rhsVer) const;
	void clear() { _major = _minor = _revision = 0; }; // Set version fields to 0
    const uint8_t * const data() { return (uint8_t*) &_major; };
    const size_t size() const { return sizeof(_major) + sizeof(_minor) + sizeof(_revision); };
	const CStdString toString() const;


	const uint16_t GetMajor() const { return _major; };
	const uint16_t GetMinor() const { return _minor; };
	const uint16_t GetRevision() const { return _revision; };

    int32_t SetVersion(LPCSTR pVersionString, bool bcd = false);
	void SetMajor(uint16_t major, bool bcd = false);
	void SetMinor(uint16_t minor, bool bcd = false);
	void SetRevision(uint16_t revision, bool bcd = false);


private:
    uint16_t	_major;
	uint16_t	_minor;
	uint16_t	_revision;
    
    ////////////////////////////////////////////////////////////////////////////////
	//! Converts a four digit BCD number to the decimal equivalent.
	//! Remember that the BCD value is big endian but read as a 16-bit little 
	//! endian number. So we have to byte swap during this conversion.
	//!
	//! \param bcdNumber BCD value in reverse byte order.
	//! \return A decimal version of \a bcdNumber.
	////////////////////////////////////////////////////////////////////////////////
	inline uint16_t BCD2D(uint16_t bcdNumber) {
		uint16_t resultVersion;
		resultVersion =    ((bcdNumber & 0x0000000f) * 100);
		resultVersion += ( ((bcdNumber & 0x000000f0) >> 4)  * 1000);
		resultVersion += ( ((bcdNumber & 0x00000f00) >> 8)  * 1);
		resultVersion += ( ((bcdNumber & 0x0000f000) >> 12) * 10);
		return resultVersion;
	} 
};

#endif // !defined(AFX_STVERSIONINFO_H__INCLUDED_)
