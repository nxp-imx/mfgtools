/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/********************************************************************
*
* Copyright (C) 1999-2000 Sven Wiegand
* Copyright (C) 2000-2001 ToolsCenter
* 
* This file is free software; you can redistribute it and/or
* modify, but leave the headers intact and do not remove any 
* copyrights from the source.
*
* If you have further questions, suggestions or bug fixes, visit 
* our homepage
*
*    http://www.ToolsCenter.org
*
********************************************************************/

#if !defined(AFX_FILEVERSION_H__F828004C_7680_40FE_A08D_7BB4FF05B4CC__INCLUDED_)
#define AFX_FILEVERSION_H__F828004C_7680_40FE_A08D_7BB4FF05B4CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winver.h>

class CFileVersionInfo
{
// construction/destruction
public:
	CFileVersionInfo();
	virtual ~CFileVersionInfo();

// operations
public:
//	BOOL Create(HMODULE hModule = NULL);
	BOOL Create(LPTSTR lpszFileName);
	BOOL ApplyVersionChanges(HANDLE _hTargetUpdate, LPVOID _data);

// attribute operations
public:
	WORD GetFileVersion(int nIndex) const;
	WORD GetProductVersion(int nIndex) const;
	DWORD GetFileFlagsMask() const;
	DWORD GetFileFlags() const;
	DWORD GetFileOs() const;
	DWORD GetFileType() const;
	DWORD GetFileSubtype() const;
	void GetFileDate(CTime& _t) const;

	CString GetCompanyName() const;
	CString GetFileDescription() const;
	CString GetFileVersion() const;
	CString GetInternalName() const;
	CString GetLegalCopyright() const;
	CString GetOriginalFileName() const;
	CString GetProductName() const;
	CString GetProductVersion() const;
	CString GetComment() const;
	CString GetLegalTrademarks() const;
	CString GetPrivateBuild() const;
	CString GetSpecialBuild() const;

// implementation helpers
protected:
	virtual void Reset();
	BOOL GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough = FALSE);
	void BlankOut(LPVOID _pPtr, DWORD _size);

// attributes
private:
	LPVOID				m_lpData;
	DWORD				m_dwFileVersionInfoSize;
	VS_FIXEDFILEINFO	m_FileInfo;
	LPVOID				m_pFileVersionMS;
	LPVOID				m_pFileVersionLS;
	LPVOID				m_pCompanyName;
	LPVOID				m_pProductName;
	LPVOID				m_pProductDesc;
	LPVOID				m_pProductVersion;
	LPVOID				m_pCopyRight;
	LPVOID				m_pComment;

	CString m_strCompanyName;
	CString m_strFileDescription;
	CString m_strFileVersion;
	CString m_strInternalName;
	CString m_strLegalCopyright;
	CString m_strOriginalFileName;
	CString m_strProductName;
	CString m_strProductVersion;
	CString m_strComment;
	CString m_strLegalTrademarks;
	CString m_strPrivateBuild;
	CString m_strSpecialBuild;
};

#endif // !defined(AFX_FILEVERSION_H__F828004C_7680_40FE_A08D_7BB4FF05B4CC__INCLUDED_)
