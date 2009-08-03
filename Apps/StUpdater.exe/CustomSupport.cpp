// CustomSupport.cpp : implementation file
//

#include "stdafx.h"
#include "CustomSupport.h"


// CCustomSupport
CCustomSupport::CCustomSupport()
{
}

CCustomSupport::~CCustomSupport()
{
}

CString CCustomSupport::GetVersionString(CStVersionInfo* _ver)
{
		CString str;
		USHORT low = _ver->GetLow();
		while ( low > 100 ) low -= 100;
		str.Format(_T("%c%02d"), _ver->GetMid()==0 ? _T('X') : _T('A'), low);
		
		return str;
}

