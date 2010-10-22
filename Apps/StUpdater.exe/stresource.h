/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StResource.h: interface for the CStResource class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRESOURCE_H__E694A91E_FAE8_4834_901C_EED53CCF202C__INCLUDED_)
#define AFX_STRESOURCE_H__E694A91E_FAE8_4834_901C_EED53CCF202C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef CMap<int, int, int, int> CStMap_Err_Rid;

class CStResource : public CStBaseToResource
{
public:
	CStResource(CStConfigInfo*, LANGID	_default_language);
	virtual ~CStResource();

	ST_ERROR GetResourceString(int _res_id, CString& _str, BOOL _for_logging_only=FALSE);
	ST_ERROR GetResourceString(int _res_id, CString& _str, UCHAR drive_number, BOOL _for_logging_only=FALSE);
	ST_ERROR LoadIcon(int _res_id, HICON& _icon);
	ST_ERROR LoadBitmap(int _res_id, HBITMAP& _bitmap);
	virtual ST_ERROR GetErrorMessage(USHORT _sense_code, wstring& _msg, BOOL _for_logging_only=FALSE);
	virtual ST_ERROR GetResourceString( int _res_id, wstring& _res_string );
	virtual LPVOID GetDefaultCfgResource(int _res_id, DWORD _size);

	ST_ERROR GetErrorMessage(ST_ERROR _err, CString& _msg, BOOL _for_logging_only=FALSE);
	ST_ERROR GetErrorMessage(ST_ERROR _err, CString& _msg, UCHAR _drive_index, BOOL _for_logging_only=FALSE);
	
	void GetTaskName(TASK_TYPE _current_task, CString& _task_name, BOOL _for_logging_only=FALSE);
	CString GetAboutVersionString();
	CString GetVersionString();
	CString GetTitle();
	LANGID GetDefaultLanguageId(){ return m_default_language; }

	void SetLoggingTo(LANGID);

	CStMap_Err_Rid m_p_map_err_rid;
	CStMap_Err_Rid m_p_map_sense_code_rid;	

private:

	CStConfigInfo * m_p_config_info;
	HINSTANCE		m_h_string_res;
	HINSTANCE		m_h_string_res_logging;
	LANGID			m_logging_language_id;
	LANGID			m_default_language;
};

#endif // !defined(AFX_STRESOURCE_H__E694A91E_FAE8_4834_901C_EED53CCF202C__INCLUDED_)
