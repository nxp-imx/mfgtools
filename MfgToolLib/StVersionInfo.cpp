/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
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


// StVersionInfo.cpp: implementation of the StVersionInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StVersionInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

StVersionInfo::StVersionInfo(USHORT major, USHORT minor, USHORT rev)
	: _major(major)
	, _minor(minor)
	, _revision(rev)
{
}

StVersionInfo::StVersionInfo(LPCSTR pVersionString)
	: _major(0)
	, _minor(0)
	, _revision(0)
{
    SetVersion(pVersionString);
}

// TODO: I don't remember about this BCD stuff. May still have to implement it.
int StVersionInfo::SetVersion(LPCSTR pVersionString, bool bcd)
{
	UINT major=0, minor=0, revision=0;

    if( sscanf_s(pVersionString, "%u.%u.%u", &major, &minor, &revision) != EOF )
    {
	    SetMajor(major);
	    SetMinor(minor);
	    SetRevision(revision);

        return ERROR_SUCCESS;
    }
    else
    {
	    SetMajor(0);
	    SetMinor(0);
	    SetRevision(0);

        return ERROR_INVALID_DATA;
    }
}

void StVersionInfo::SetMajor(USHORT major, bool bcd)
{
	_major = bcd ? BCD2D(major) : major;
}

void StVersionInfo::SetMinor(USHORT minor, bool bcd)
{
	_minor = bcd ? BCD2D(minor) : minor;
}

void StVersionInfo::SetRevision(USHORT revision, bool bcd)
{
	_revision = bcd ? BCD2D(revision) : revision;
}

bool StVersionInfo::operator != (const StVersionInfo& ver) const
{
	return !(*this == ver);
}

bool StVersionInfo::operator == (const StVersionInfo& ver) const
{
	if( (ver.GetMajor() == GetMajor()) && (ver.GetMinor() == GetMinor()) &&
		(ver.GetRevision() == GetRevision()) )
	{
		return true;
	}
	return false;
}

bool StVersionInfo::operator > (const StVersionInfo& rhsVer) const
{
	if( rhsVer._major > _major )
	{
		return true;
	}
	else if ( rhsVer._major < _major )
	{
		return false;
	}
	else
	{
		// major versions are equal, check minor versions
		if( rhsVer._minor > _minor )
		{
			return true;
		}
		else if ( rhsVer._minor < _minor )
		{
			return false;
		}
		else
		{
			// major and minor versions are equal, check revisions
			if( rhsVer._revision > _revision )
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}

const CString StVersionInfo::toString() const
{
    CString str;
    str.Format(_T("%03d.%03d.%03d"), GetMajor(), GetMinor(), GetRevision());

    return str;
}
