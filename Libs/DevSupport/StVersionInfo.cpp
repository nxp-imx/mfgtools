// StVersionInfo.cpp: implementation of the StVersionInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "StVersionInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

StVersionInfo::StVersionInfo(uint16_t major, uint16_t minor, uint16_t rev)
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
int32_t StVersionInfo::SetVersion(LPCSTR pVersionString, bool bcd)
{
	uint32_t major=0, minor=0, revision=0;

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

void StVersionInfo::SetMajor(uint16_t major, bool bcd)
{
	_major = bcd ? BCD2D(major) : major;
}

void StVersionInfo::SetMinor(uint16_t minor, bool bcd)
{
	_minor = bcd ? BCD2D(minor) : minor;
}

void StVersionInfo::SetRevision(uint16_t revision, bool bcd)
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

const CStdString StVersionInfo::toString() const
{
    CStdString str;
    str.Format(_T("%03d.%03d.%03d"), GetMajor(), GetMinor(), GetRevision());
	
    return str;
}
