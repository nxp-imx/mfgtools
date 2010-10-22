/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/*
Module : VersionInfo.H
Purpose: Interface for an MFC class encapsulation of Version Infos
Created: PJN / 10-04-2000

Copyright (c) 2000 by PJ Naughter.  
All rights reserved.

*/


/////////////////////////////// Defines ///////////////////////////////////////
#ifndef __VERSIONINFO_H__
#define __VERSIONINFO_H__


//Pull in the win32 version Library
#pragma comment(lib, "version.lib")
 

/////////////////////////////// Classes ///////////////////////////////////////


class CVersionInfo
{
public:
  struct TRANSLATION
  {
    WORD m_wLangID;   //e.g. 0x0409 LANG_ENGLISH, SUBLANG_ENGLISH_USA
    WORD m_wCodePage; //e.g. 1252 Codepage for Windows:Multilingual
  };

//Constructors / Destructors
  CVersionInfo();
  ~CVersionInfo();

//methods:
  BOOL                          Load(const CString& sFileName);
  VS_FIXEDFILEINFO*             GetFixedFileInfo();
  DWORD                         GetFileFlagsMask();
  DWORD                         GetFileFlags();
  DWORD                         GetOS();
  DWORD                         GetFileType();
  DWORD                         GetFileSubType();
  FILETIME                      GetCreationTime();
  unsigned __int64              GetFileVersion();
  unsigned __int64              GetProductVersion();
  CString                       GetValue(const CString& sKeyName);
  CString                       GetComments();
  CString                       GetCompanyName();
  CString                       GetFileDescription();
  CString                       GetFileVersionAsString();
  CString                       GetInternalName();
  CString                       GetLegalCopyright();
  CString                       GetLegalTrademarks();
  CString                       GetOriginalFilename();
  CString                       GetPrivateBuild();
  CString                       GetProductName();
  CString                       GetProductVersionAsString();
  CString                       GetSpecialBuild();
  int                           GetNumberOfTranslations();
  TRANSLATION*                  GetTranslation(int nIndex);
  void                          SetTranslation(int nIndex);
  
protected:
//Methods
  void Unload();

//Data
  WORD              m_wLangID;       //The current language ID of the resource
  WORD              m_wCharset;      //The current Character set ID of the resource
  LPVOID            m_pVerData;      //Pointer to the Version info blob
  TRANSLATION*      m_pTranslations; //Pointer to the "\\VarFileInfo\\Translation" version info
  int               m_nTranslations; //The number of translated version infos in the resource
  VS_FIXEDFILEINFO* m_pffi;          //Pointer to the fixed size version info data
};


#endif //__VERSIONINFO_H__

