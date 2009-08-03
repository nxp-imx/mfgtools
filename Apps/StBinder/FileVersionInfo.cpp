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

#include "stdafx.h"
#include "FileVersionInfo.h"
#include "TargetCfgData.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern TARGET_CFG_DATA g_ResCfgData;

//-------------------------------------------------------------------
// CFileVersionInfo
//-------------------------------------------------------------------

CFileVersionInfo::CFileVersionInfo()
{
	m_lpData = NULL;
	Reset();
}


CFileVersionInfo::~CFileVersionInfo()
{
	if (m_lpData)
		delete[] m_lpData;
}


BOOL CFileVersionInfo::GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough/*= FALSE*/)
{
	LPWORD lpwData;

	for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData)+unBlockSize; lpwData+=2)
	{
		if (*lpwData == wLangId)
		{
			dwId = *((DWORD*)lpwData);
			return TRUE;
		}
	}

	if (!bPrimaryEnough)
		return FALSE;

	for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData)+unBlockSize; lpwData+=2)
	{
		if (((*lpwData)&0x00FF) == (wLangId&0x00FF))
		{
			dwId = *((DWORD*)lpwData);
			return TRUE;
		}
	}

	return FALSE;
}


//BOOL CFileVersionInfo::Create(HMODULE hModule /*= NULL*/)
//{
//	CString	strPath;
//
//	GetModuleFileName(hModule, strPath.GetBuffer(_MAX_PATH), _MAX_PATH);
//	strPath.ReleaseBuffer();
//	return Create(strPath);
//}


BOOL CFileVersionInfo::Create(LPTSTR lpszFileName)
{
	Reset();

	DWORD	dwHandle;
	m_dwFileVersionInfoSize = GetFileVersionInfoSize((LPTSTR)lpszFileName, &dwHandle);
	if (!m_dwFileVersionInfoSize)
		return FALSE;

	m_lpData = (LPVOID)new BYTE[m_dwFileVersionInfoSize];
	if (!m_lpData)
	{
		DWORD err = GetLastError();
		return FALSE;
	}

	try
	{
		if (!GetFileVersionInfo((LPTSTR)lpszFileName, dwHandle, m_dwFileVersionInfoSize, m_lpData))
			throw FALSE;

		// catch default information
		LPVOID	lpInfo;
		UINT		unInfoLen;
		if (VerQueryValue(m_lpData, _T("\\"), &lpInfo, &unInfoLen))
		{
			ASSERT(unInfoLen == sizeof(m_FileInfo));
			if (unInfoLen == sizeof(m_FileInfo))
				memcpy(&m_FileInfo, lpInfo, unInfoLen);
			m_pFileVersionMS = (LPVOID)((char *)lpInfo+8);
			m_pFileVersionLS = (LPVOID)((char *)lpInfo+12);

		}

		// find best matching language and codepage
		VerQueryValue(m_lpData, _T("\\VarFileInfo\\Translation"), &lpInfo, &unInfoLen);
		
		DWORD	dwLangCode = 0;
		if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, FALSE))
		{
			if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, TRUE))
			{
				if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), dwLangCode, TRUE))
				{
					if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), dwLangCode, TRUE))
						// use the first one we can get
						dwLangCode = *((DWORD*)lpInfo);
				}
			}
		}
		

		CString	strSubBlock;
		strSubBlock = _T("\\StringFileInfo\\040904b0\\");
		
		// catch string table
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("CompanyName")), &lpInfo, &unInfoLen))
		{
			m_pCompanyName = lpInfo;
			m_strCompanyName = CString((LPCTSTR)lpInfo);
		}
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("FileDescription")), &lpInfo, &unInfoLen))
		{
			m_pProductDesc = lpInfo;
			m_strFileDescription = CString((LPCTSTR)lpInfo);
		}
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("ProductName")), &lpInfo, &unInfoLen))
		{
			m_pProductName = lpInfo;
			m_strProductName = CString((LPCTSTR)lpInfo);
		}
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("LegalCopyright")), &lpInfo, &unInfoLen))
		{
			m_pCopyRight = lpInfo;
			m_strLegalCopyright = CString((LPCTSTR)lpInfo);
		}
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("Comment")), &lpInfo, &unInfoLen))
		{
			m_pComment = lpInfo;
			m_strComment = CString((LPCTSTR)lpInfo);
		}
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("OriginalVersion")), &lpInfo, &unInfoLen))
			m_strFileVersion = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("InternalName")), &lpInfo, &unInfoLen))
			m_strInternalName = CString((LPCTSTR)lpInfo);

		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("OriginalFileName")), &lpInfo, &unInfoLen))
			m_strOriginalFileName = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("ProductVersion")), &lpInfo, &unInfoLen))
		{
			m_pProductVersion = lpInfo;
			m_strProductVersion = CString((LPCTSTR)lpInfo);
		}
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("LegalTrademarks")), &lpInfo, &unInfoLen))
			m_strLegalTrademarks = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("PrivateBuild")), &lpInfo, &unInfoLen))
			m_strPrivateBuild = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(m_lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("SpecialBuild")), &lpInfo, &unInfoLen))
			m_strSpecialBuild = CString((LPCTSTR)lpInfo);

	}
	catch (BOOL)
	{
		delete[] m_lpData;
		m_lpData = NULL;
		return FALSE;
	}

	return TRUE;
}


WORD CFileVersionInfo::GetFileVersion(int nIndex) const
{
	if (nIndex == 0)
		return (WORD)(m_FileInfo.dwFileVersionLS & 0x0000FFFF);
	else if (nIndex == 1)
		return (WORD)((m_FileInfo.dwFileVersionLS & 0xFFFF0000) >> 16);
	else if (nIndex == 2)
		return (WORD)(m_FileInfo.dwFileVersionMS & 0x0000FFFF);
	else if (nIndex == 3)
		return (WORD)((m_FileInfo.dwFileVersionMS & 0xFFFF0000) >> 16);
	else
		return 0;
}


WORD CFileVersionInfo::GetProductVersion(int nIndex) const
{
	if (nIndex == 0)
		return (WORD)(m_FileInfo.dwProductVersionLS & 0x0000FFFF);
	else if (nIndex == 1)
		return (WORD)((m_FileInfo.dwProductVersionLS & 0xFFFF0000) >> 16);
	else if (nIndex == 2)
		return (WORD)(m_FileInfo.dwProductVersionMS & 0x0000FFFF);
	else if (nIndex == 3)
		return (WORD)((m_FileInfo.dwProductVersionMS & 0xFFFF0000) >> 16);
	else
		return 0;
}


DWORD CFileVersionInfo::GetFileFlagsMask() const
{
	return m_FileInfo.dwFileFlagsMask;
}


DWORD CFileVersionInfo::GetFileFlags() const
{
	return m_FileInfo.dwFileFlags;
}


DWORD CFileVersionInfo::GetFileOs() const
{
	return m_FileInfo.dwFileOS;
}


DWORD CFileVersionInfo::GetFileType() const
{
	return m_FileInfo.dwFileType;
}


DWORD CFileVersionInfo::GetFileSubtype() const
{
	return m_FileInfo.dwFileSubtype;
}


void CFileVersionInfo::GetFileDate(CTime& _t) const
{
	FILETIME	ft;
	ft.dwLowDateTime = m_FileInfo.dwFileDateLS;
	ft.dwHighDateTime = m_FileInfo.dwFileDateMS;
	_t = CTime(ft);
}


CString CFileVersionInfo::GetCompanyName() const
{
	return m_strCompanyName;
}


CString CFileVersionInfo::GetFileDescription() const
{
	return m_strFileDescription;
}


CString CFileVersionInfo::GetFileVersion() const
{
	return m_strFileVersion;
}


CString CFileVersionInfo::GetInternalName() const
{
	return m_strInternalName;
}


CString CFileVersionInfo::GetLegalCopyright() const
{
	return m_strLegalCopyright;
}


CString CFileVersionInfo::GetOriginalFileName() const
{
	return m_strOriginalFileName;
}


CString CFileVersionInfo::GetProductName() const
{
	return m_strProductName;
}


CString CFileVersionInfo::GetProductVersion() const
{
	return m_strProductVersion;
}

CString CFileVersionInfo::GetComment() const
{
	return m_strComment;
}

CString CFileVersionInfo::GetLegalTrademarks() const
{
	return m_strLegalTrademarks;
}


CString CFileVersionInfo::GetPrivateBuild() const
{
	return m_strPrivateBuild;
}


CString CFileVersionInfo::GetSpecialBuild() const
{
	return m_strSpecialBuild;
}


void CFileVersionInfo::Reset()
{
	if (m_lpData)
	{
		delete[] m_lpData;
	}
	m_lpData = NULL;
	ZeroMemory(&m_FileInfo, sizeof(m_FileInfo));
	m_strCompanyName.Empty();
	m_strFileDescription.Empty();
	m_strFileVersion.Empty();
	m_strInternalName.Empty();
	m_strLegalCopyright.Empty();
	m_strOriginalFileName.Empty();
	m_strProductName.Empty();
	m_strProductVersion.Empty();
	m_strComment.Empty();
	m_strLegalTrademarks.Empty();
	m_strPrivateBuild.Empty();
	m_strSpecialBuild.Empty();
}


BOOL CFileVersionInfo::ApplyVersionChanges(HANDLE _hTargetUpdate, LPVOID _data)
{
	PTARGET_CFG_DATA pTargetCfgData = (PTARGET_CFG_DATA) _data;
	wchar_t szVersion[20];

	if (!m_lpData)
		return FALSE;

	// Save changes to the data block
	BlankOut(m_pCompanyName, COMPANYNAME_MAX);
	BlankOut(m_pProductName, APPTITLE_MAX);
	BlankOut(m_pProductDesc, PRODDESC_MAX);
	BlankOut(m_pCopyRight, COPYRIGHT_MAX);
	BlankOut(m_pComment, COMMENT_MAX);

	swprintf_s(szVersion, 20, _T("%d.%03d.%d.%03d"),
								pTargetCfgData->Options.UpdMajorVersion,
								pTargetCfgData->Options.UpdMinorVersion,
								pTargetCfgData->Options.ProdMajorVersion,
								pTargetCfgData->Options.ProdMinorVersion);


	// fixed version info
	m_FileInfo.dwProductVersionMS = 0;
	m_FileInfo.dwProductVersionLS = 0;
/*
	m_FileInfo.dwProductVersionMS &= (0xFFFF0000 & (pTargetCfgData->Options.UpdMajorVersion << 16));
	m_FileInfo.dwProductVersionMS &= (0x0000FFFF & pTargetCfgData->Options.UpdMinorVersion);
	m_FileInfo.dwProductVersionLS &= (0xFFFF0000 & (pTargetCfgData->Options.ProdMajorVersion << 16));
	m_FileInfo.dwProductVersionLS &= (0x0000FFFF & pTargetCfgData->Options.ProdMinorVersion);
*/
	m_FileInfo.dwProductVersionMS = pTargetCfgData->Options.UpdMajorVersion << 16;
	m_FileInfo.dwProductVersionMS |= pTargetCfgData->Options.UpdMinorVersion;
	m_FileInfo.dwProductVersionLS = pTargetCfgData->Options.ProdMajorVersion << 16;
	m_FileInfo.dwProductVersionLS |= pTargetCfgData->Options.ProdMinorVersion;

	*(DWORD *)m_pFileVersionMS = m_FileInfo.dwProductVersionMS;
	*(DWORD *)m_pFileVersionLS = m_FileInfo.dwProductVersionLS;

	// variable version info
	memcpy(m_pCompanyName, &pTargetCfgData->Options.CompanyName[0], wcslen((wchar_t *)pTargetCfgData->Options.CompanyName)*sizeof(wchar_t));
	memcpy(m_pProductName, &pTargetCfgData->Options.AppTitle[0], wcslen((wchar_t *)pTargetCfgData->Options.AppTitle)*sizeof(wchar_t));
	memcpy(m_pProductDesc, &pTargetCfgData->Options.ProductDesc[0], wcslen((wchar_t *)pTargetCfgData->Options.ProductDesc)*sizeof(wchar_t));
	memcpy(m_pCopyRight, &pTargetCfgData->Options.Copyright[0], wcslen((wchar_t *)pTargetCfgData->Options.Copyright)*sizeof(wchar_t));
	memcpy(m_pComment, &pTargetCfgData->Options.Comment[0], wcslen((wchar_t *)pTargetCfgData->Options.Comment)*sizeof(wchar_t));
	memcpy(m_pProductVersion, &szVersion[0], wcslen((wchar_t *)szVersion)*sizeof(wchar_t));

	UpdateResource(_hTargetUpdate,       // update resource handle 
	    					RT_VERSION,
		    				MAKEINTRESOURCE(VS_VERSION_INFO),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),  // language
							m_lpData,      // ptr to resource info 
							m_dwFileVersionInfoSize); // size of resource info. 

	return TRUE;
}

void CFileVersionInfo::BlankOut(LPVOID _pPtr, DWORD _size)
{
	wchar_t *pw = (wchar_t *)_pPtr;

	for (DWORD i = 0; i < _size-1; ++i)
		*pw++ = (wchar_t)' ';
	*pw = (wchar_t)'\0'; // set NULL at end
}
