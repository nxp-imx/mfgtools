// StError.cpp: implementation of the CStError class.
//
//////////////////////////////////////////////////////////////////////

#include "StHeader.h"
#include "StError.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStError::CStError()
{
	m_err_in_obj_name = "";
	m_last_error = STERR_NONE;
	m_system_last_error = 0;
	m_drive_index = 0xFF;
	m_more_error_information = L"";
}

CStError::~CStError()
{
}

void CStError::ClearStatus()
{
	m_err_in_obj_name = "";
	m_last_error = STERR_NONE;
	m_system_last_error = 0;
	m_more_error_information = L"";
}


void CStError::SaveStatus(CStBase* _p_base)
{
	m_err_in_obj_name = _p_base->GetObjName();
	m_last_error = _p_base->GetLastError();
	m_system_last_error = _p_base->GetSystemLastError();
}

void CStError::SaveStatus(CStBase* _p_base, UCHAR _drive_index)
{
	m_drive_index = _drive_index;
	SaveStatus(_p_base);
}

void CStError::SaveStatus(CStBase* _p_base, wstring _more_information)
{
	m_more_error_information = _more_information;
	SaveStatus(_p_base);
}

void CStError::SaveStatus(CStBase* _p_base, UCHAR _drive_index, wstring _more_information)
{
	SaveStatus(_p_base, _more_information);
	SaveStatus(_p_base, _drive_index);
}
void CStError::SaveStatus(ST_ERROR _last_error, long _system_last_error)
{
	m_last_error = _last_error;
	m_system_last_error = _system_last_error;
}
