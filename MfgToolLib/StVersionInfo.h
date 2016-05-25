/*
 * Copyright 2009-2013, 2016 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


// StVersionInfo.h: interface for the StVersionInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STVERSIONINFO_H__INCLUDED_)
#define AFX_STVERSIONINFO_H__INCLUDED_

struct StVersionInfo
{
	StVersionInfo(USHORT major=0, USHORT minor=0, USHORT rev=0);
        StVersionInfo(LPCSTR pVersionString);
	virtual ~StVersionInfo()
	{
	}

	bool operator != (const StVersionInfo&) const;
	bool operator == (const StVersionInfo&) const;
	bool operator > (const StVersionInfo& rhsVer) const;

	void clear()
	{
		_major = _minor = _revision = 0;
	} // Set version fields to 0

    const UCHAR * const data() { return (UCHAR*) &_major; };
    const size_t size() const { return sizeof(_major) + sizeof(_minor) + sizeof(_revision); };
	const CString toString() const;


	const USHORT GetMajor() const { return _major; };
	const USHORT GetMinor() const { return _minor; };
	const USHORT GetRevision() const { return _revision; };

    int SetVersion(LPCSTR pVersionString, bool bcd = false);
	void SetMajor(USHORT major, bool bcd = false);
	void SetMinor(USHORT minor, bool bcd = false);
	void SetRevision(USHORT revision, bool bcd = false);


private:
    USHORT	_major;
	USHORT	_minor;
	USHORT	_revision;

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
		resultVersion =    ((bcdNumber & 0x0000000f) * 100);
		resultVersion += ( ((bcdNumber & 0x000000f0) >> 4)  * 1000);
		resultVersion += ( ((bcdNumber & 0x00000f00) >> 8)  * 1);
		resultVersion += ( ((bcdNumber & 0x0000f000) >> 12) * 10);
		return resultVersion;
	}
};

#endif // !defined(AFX_STVERSIONINFO_H__INCLUDED_)
