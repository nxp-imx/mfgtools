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

#include "StHeader.h"
#include "StVersionInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStVersionInfo::CStVersionInfo(string _base):CStBase(_base)
{
	Reset();
}

CStVersionInfo::CStVersionInfo(const CStVersionInfo& _ver)
{
	*this = _ver;
}

CStVersionInfo& CStVersionInfo::operator=(const CStVersionInfo& _ver)
{
	SetHigh(_ver.GetHigh());
	SetMid(_ver.GetMid());
	SetLow(_ver.GetLow());
	m_loaded_from_resource = _ver.m_loaded_from_resource;

	m_last_error		= _ver.m_last_error;
	m_system_last_error = _ver.m_system_last_error;
	m_obj_name			= _ver.m_obj_name;

	return *this;
}

CStVersionInfo::~CStVersionInfo()
{

}

void CStVersionInfo::Reset()
{
	m_high = m_mid = m_low = 0;
	m_loaded_from_resource = FALSE;
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

BOOL CStVersionInfo::operator != (CStVersionInfo& _ver)
{
	return !(*this == _ver);
}

BOOL CStVersionInfo::operator == (CStVersionInfo& _ver)
{
	if( (_ver.GetHigh() == GetHigh()) && (_ver.GetMid() == GetMid()) &&
		(_ver.GetLow() == GetLow()) )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CStVersionInfo::operator > (const CStVersionInfo& rhsVer) const
{
	if( m_high > rhsVer.m_high )
	{
		return true;
	}
	else if ( m_high < rhsVer.m_high )
	{
		return false;
	}
	else
	{
		// major versions are equal, check minor versions
		if( m_mid > rhsVer.m_mid )
		{
			return true;
		}
		else if ( m_mid < rhsVer.m_mid )
		{
			return false;
		}
		else
		{
			// major and minor versions are equal, check revisions
			if( m_low > rhsVer.m_low )
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

wstring CStVersionInfo::GetVersionString()
{
	wchar_t ver[MAX_PATH];
	swprintf(ver, MAX_PATH, L"%03d.%03d.%03d", GetHigh(), GetMid(), GetLow());
	return ver;
}

CStVersionInfoPtrArray::CStVersionInfoPtrArray(size_t _size, string _name):
	CStArray<CStVersionInfo*>(_size, _name)
{
	CStVersionInfo* ver;
	for(size_t index=0; index<_size; index ++)
	{
		ver = new CStVersionInfo;
		SetAt(index, ver);
	}
}

CStVersionInfoPtrArray::~CStVersionInfoPtrArray()
{
	CStVersionInfo* ver;
	for(size_t index=0; index<GetCount(); index ++)
	{
		ver = *GetAt(index);
		delete ver;
		SetAt(index, NULL);
	}
}

