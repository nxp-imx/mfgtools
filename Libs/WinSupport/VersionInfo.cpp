/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/*
Module : VersionInfo.CPP
Purpose: Implementation for a MFC class encapsulation of Version Infos
Created: PJN / 10-04-2000
History: None


Copyright (c) 2000 by PJ Naughter.  
All rights reserved.

*/

//////////////// Includes ////////////////////////////////////////////
#include "stdafx.h"
#include "VersionInfo.h"


//////////////// Macros / Locals /////////////////////////////////////
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//////////////// Implementation //////////////////////////////////////

CVersionInfo::CVersionInfo()
{
  m_pVerData = NULL;
  m_pffi = NULL;
  m_wLangID = 0;
  m_wCharset = 1252; //Use the ANSI code page as a default
  m_pTranslations = NULL;
  m_nTranslations = 0;
}

CVersionInfo::~CVersionInfo()
{
  Unload();
}

void CVersionInfo::Unload()
{
  m_pffi = NULL;
  if (m_pVerData)
  {
    delete [] m_pVerData;
    m_pVerData = NULL;
  }
  m_wLangID = 0;
  m_wCharset = 1252; //Use the ANSI code page as a default
  m_pTranslations = NULL;
  m_nTranslations = 0;
}

BOOL CVersionInfo::Load(const CString& sFileName)
{
  //Free up any previous memory lying around
  Unload();

  //Need to make a copy of the string as "GetFileVersionInfoSize"
  //is prototyped as taking a "LPTSTR" not a "LPCTSTR".
  TCHAR* pszFileName = new TCHAR[sFileName.GetLength() + 1];
  _tcscpy_s(pszFileName, sFileName.GetLength()+1, sFileName);

  BOOL bSuccess = FALSE;
  DWORD dwHandle = 0;
  DWORD dwSize = GetFileVersionInfoSize(pszFileName, &dwHandle);
  if (dwSize)
  {
    m_pVerData = new BYTE[dwSize];
    if (GetFileVersionInfo(pszFileName, dwHandle, dwSize, m_pVerData))
    {
      //Get the fixed size version info data
      UINT nLen = 0;
      if (VerQueryValue(m_pVerData, _T("\\"), (LPVOID*) &m_pffi, &nLen))
      {
        //Retreive the Lang ID and Character set ID
        if (VerQueryValue(m_pVerData, _T("\\VarFileInfo\\Translation"), (LPVOID*) &m_pTranslations, &nLen) && nLen >= sizeof(TRANSLATION)) 
        {
          m_nTranslations = nLen / sizeof(TRANSLATION);
          m_wLangID = m_pTranslations[0].m_wLangID;
          m_wCharset = m_pTranslations[0].m_wCodePage;
        }
        bSuccess = TRUE;
      }
      else
        TRACE(_T("Failed to query file size version info for file %s, LastError:%d\n"), sFileName, ::GetLastError());
    }
    else
      TRACE(_T("Failed to read in version info for file %s, LastError:%d\n"), sFileName, ::GetLastError());
  }
  else
    TRACE(_T("Failed to get version info for file %s, LastError:%d\n"), sFileName, ::GetLastError());

  //Free up the memory we used
  if (!bSuccess)
  {
    if (m_pVerData)
    {
      delete [] m_pVerData;
      m_pVerData = NULL;
    }
  }
  delete [] pszFileName;

  return bSuccess;
}

VS_FIXEDFILEINFO* CVersionInfo::GetFixedFileInfo()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  return m_pffi;
}

DWORD CVersionInfo::GetFileFlagsMask()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  return m_pffi->dwFileFlagsMask;
}

DWORD CVersionInfo::GetFileFlags()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  return m_pffi->dwFileFlags;
}

DWORD CVersionInfo::GetOS()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  return m_pffi->dwFileOS;
}

DWORD CVersionInfo::GetFileType()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  return m_pffi->dwFileType;
}

DWORD CVersionInfo::GetFileSubType()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  return m_pffi->dwFileSubtype;
}

FILETIME CVersionInfo::GetCreationTime()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  FILETIME CreationTime;
  CreationTime.dwHighDateTime = m_pffi->dwFileDateMS; 
  CreationTime.dwLowDateTime = m_pffi->dwFileDateLS; 
  return CreationTime;
}

unsigned __int64  CVersionInfo::GetFileVersion()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  unsigned __int64 nFileVersion = 0;
  nFileVersion = m_pffi->dwFileVersionLS;
  nFileVersion += (((unsigned __int64)m_pffi->dwFileVersionMS) << 32);
  return nFileVersion;
}

unsigned __int64  CVersionInfo::GetProductVersion()
{
  ASSERT(m_pVerData); //Must have been loaded successfully
  unsigned __int64 nProductVersion = 0;
  nProductVersion = m_pffi->dwProductVersionLS;
  nProductVersion += (((unsigned __int64)m_pffi->dwProductVersionMS) << 32);
  return nProductVersion;
}

CString CVersionInfo::GetValue(const CString& sKey)
{
  ASSERT(m_pVerData);

  //For the string to query with
  CString sVal;
  CString sQueryValue;
  sQueryValue.Format(_T("\\StringFileInfo\\%04x%04x\\%s"), m_wLangID, m_wCharset, sKey);

  //Need to make a copy of the string as "VerQueryValue"
  //is prototyped as taking a "LPTSTR" not a "LPCTSTR".
  TCHAR* pszQueryValue = new TCHAR[sQueryValue.GetLength() + 1];
  _tcscpy_s(pszQueryValue, sQueryValue.GetLength()+1, sQueryValue);

  //Do the query
  LPCTSTR pVal = NULL;
  UINT nLen = 0;
  if (VerQueryValue(m_pVerData, pszQueryValue, (LPVOID*)&pVal, &nLen)) 
    sVal = pVal;

  //Free up the memory we allocated
  delete [] pszQueryValue;

  return sVal;
}

CString CVersionInfo::GetCompanyName()
{
  return GetValue(_T("CompanyName"));
}

CString CVersionInfo::GetFileDescription()
{
  return GetValue(_T("FileDescription"));
}

CString CVersionInfo::GetFileVersionAsString()
{
  return GetValue(_T("FileVersion"));
}

CString CVersionInfo::GetInternalName()
{
  return GetValue(_T("InternalName"));
}

CString CVersionInfo::GetLegalCopyright()
{
  return GetValue(_T("LegalCopyright"));
}

CString CVersionInfo::GetOriginalFilename()
{
  return GetValue(_T("OriginalFilename"));
}

CString CVersionInfo::GetProductName()
{
  return GetValue(_T("Productname"));
}

CString CVersionInfo::GetProductVersionAsString()
{
  return GetValue(_T("ProductVersion"));
}
  
int CVersionInfo::GetNumberOfTranslations()
{
  return m_nTranslations;
}

CString CVersionInfo::GetComments()
{
  return GetValue(_T("Comments"));
}

CString CVersionInfo::GetLegalTrademarks()
{
  return GetValue(_T("LegalTrademarks"));
}

CString CVersionInfo::GetPrivateBuild()
{
  return GetValue(_T("PrivateBuild"));
}

CString CVersionInfo::GetSpecialBuild()
{
  return GetValue(_T("SpecialBuild"));
}

CVersionInfo::TRANSLATION* CVersionInfo::GetTranslation(int nIndex)
{
  ASSERT(nIndex >= 0 && nIndex < m_nTranslations);
  ASSERT(m_pTranslations);
  return &m_pTranslations[nIndex];
}

void CVersionInfo::SetTranslation(int nIndex)
{
  TRANSLATION* pTranslation = GetTranslation(nIndex);
  m_wLangID = pTranslation->m_wLangID;
  m_wCharset = pTranslation->m_wCodePage;
}
