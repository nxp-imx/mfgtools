#pragma once

#include "StVersionInfo.h"

// CCustomSupport
class CCustomSupport
{
public:
	CCustomSupport();
	virtual ~CCustomSupport();

	CString GetVersionString(CStVersionInfo* _ver);

protected:
//	DECLARE_MESSAGE_MAP()
};


