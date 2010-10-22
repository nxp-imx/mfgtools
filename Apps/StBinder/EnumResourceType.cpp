/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "StdAfx.h"
#include "..\\..\\common\\updater_res.h"
#include "..\\..\\common\\updater_restypes.h"
#include "EnumResourceType.h"

CEnumResourceType::CEnumResourceType(PVOID _pCallerClass, LPFN_ENUM_NAMERESTYPE_CALLBACK _pfnCallback, HMODULE _hModule, LPTSTR _szResType)
{
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;

	m_pCallerClass = _pCallerClass;
	m_pfnNameIdCallback = _pfnCallback;
	m_szResType = _szResType;
	m_hModule = _hModule;

	m_pFwResInfo = NULL;
    hResInfo = FindResourceEx( m_hModule,
	    					L_STMP_RESINFO_TYPE,
		    				MAKEINTRESOURCE(IDR_BOUND_RESOURCE_TABLE_LEN),
			    			MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
    if ( hResInfo )
    {
	    hRes = LoadResource( m_hModule, hResInfo);
    	if ( hRes )
	   		pPtr = LockResource(hRes);
       	if ( pPtr )
		{
	       	m_iFwResInfoCount = *((int *)pPtr);

		    hResInfo = FindResourceEx( m_hModule,
								L_STMP_RESINFO_TYPE,
								MAKEINTRESOURCE(IDR_BOUND_RESOURCE_TABLE),
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
		    if ( hResInfo )
		    {
				hRes = LoadResource( m_hModule, hResInfo);
		    	if ( hRes )
	   				m_pFwResInfo = (PSTFWRESINFO)LockResource(hRes);
			}
		}
    }

}

CEnumResourceType::CEnumResourceType(PVOID _pCallerClass, LPFN_ENUM_IDRESTYPE_CALLBACK _pfnCallback, HMODULE _hModule, LPTSTR _szResType)
{
	m_pCallerClass = _pCallerClass;
	m_pfnResIdCallback = _pfnCallback;
	m_szResType = _szResType;
	m_hModule = _hModule;
	m_pFwResInfo = NULL;
}

CEnumResourceType::~CEnumResourceType(void)
{
}

void CEnumResourceType::Begin()
{
	EnumResourceNames(m_hModule, m_szResType, (ENUMRESNAMEPROC)EnumResNameProc, (LONG_PTR)this);
}

BOOL CEnumResourceType::EnumResNameProc(
								HMODULE _hModule,
								LPCTSTR _lpszType,
							    LPTSTR _lpszName,
								LONG_PTR _lParam)
{
	DWORD dwErr;
	int iResId = (int)(((ULONG_PTR)_lpszName) & 0x0000FFFF);

	if ( iResId >= IDR_STMP3REC_SYS )
		EnumResourceLanguages(_hModule, _lpszType, (LPCTSTR)_lpszName, (ENUMRESLANGPROC)EnumResLanguageProc, _lParam);
	else
		RollYourOwnEnumLanguages(_hModule, _lpszType, iResId, _lParam);

	dwErr = GetLastError();
	return TRUE; // continue
}

BOOL CEnumResourceType::EnumResLanguageProc(
								HANDLE _hModule,
							    LPCTSTR _lpszType,
							    LPCTSTR _lpszName,
							    WORD _wIDLanguage,
								LONG_PTR _lParam)
{
	CEnumResourceType *thisClass = (CEnumResourceType *)_lParam;

	if ( IS_INTRESOURCE(_lpszName) )
	{
		int iResId = (int) (((ULONG_PTR) _lpszName) & 0x0000FFFF);
		thisClass->m_pfnResIdCallback(thisClass->m_pCallerClass, thisClass->m_hModule, iResId, _wIDLanguage);
	}


	return TRUE; // continue

UNREFERENCED_PARAMETER(_hModule);
UNREFERENCED_PARAMETER(_lpszType);
UNREFERENCED_PARAMETER(_lParam);

}

void CEnumResourceType::RollYourOwnEnumLanguages(HMODULE _hModule,
												LPCTSTR _lpszType,
												int _iResId,
												LONG_PTR _lParam)
{
	CEnumResourceType *thisClass = (CEnumResourceType *)_lParam;
   	HRSRC hResInfo;

	// look up the resource name in the table to get the resource id and language id
	for (int i = 0; i < thisClass->m_iFwResInfoCount; ++i)
		if (_iResId == thisClass->m_pFwResInfo[i].iResId)
		{
			CString csResName = thisClass->m_pFwResInfo[i].szResourceName;

		   	hResInfo = FindResourceEx( _hModule,
						   				_lpszType,
										MAKEINTRESOURCE(thisClass->m_pFwResInfo[i].iResId),
										thisClass->m_pFwResInfo[i].wLangId);

			if ( hResInfo )
				thisClass->m_pfnNameIdCallback(thisClass->m_pCallerClass,
												thisClass->m_hModule,
												csResName,
												thisClass->m_pFwResInfo[i].iResId,
												thisClass->m_pFwResInfo[i].wLangId);
		}
}
