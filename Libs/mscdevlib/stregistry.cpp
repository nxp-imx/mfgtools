/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StRegistry.cpp: implementation of the CStRegistry class.
//
//////////////////////////////////////////////////////////////////////

#include "StHeader.h"
#include "StTrace.h"
#include "StRegistry.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStRegistry::CStRegistry(string _name):CStBase(_name)
{

}

CStRegistry::~CStRegistry()
{

}

ST_ERROR CStRegistry::FindDriveLettersForScsiDevice(string _str_dev_name, wstring& _drive_letters)
{
    HKEY hStorageKey, hDeviceKey;
	wchar_t KeyName[MAX_PATH], szValueName[MAX_PATH];
	BYTE szValueData[MAX_PATH];
	DWORD dwNameSize, dwSecondIndex, dwDataSize, dwType;
	FILETIME FileTime;
	BOOL Found = FALSE;

	// open the root registry key for our drivers
	swprintf( KeyName, MAX_PATH, L"ENUM\\SCSI\\%S", _str_dev_name.c_str() );
	if (RegOpenKeyW(
			HKEY_LOCAL_MACHINE, 
			KeyName, 
			&hStorageKey) != ERROR_SUCCESS
		)
	{
		CStTrace::trace( "No key" );
		return STERR_FAILED_TO_OPEN_REGISTRY_KEY;
	}

	// enum all the keys and find the one we're looking for
	dwSecondIndex = 0;
	dwNameSize = MAX_PATH - 1;
	_drive_letters = wstring(L"");
	while(RegEnumKeyExW(hStorageKey, dwSecondIndex, szValueName, &dwNameSize, NULL, NULL, NULL, &FileTime) == ERROR_SUCCESS)
	{
//		CStTrace::trace( szValueName );
		
		// this is our key, but there could be more than one, so pick the newest
		if (RegOpenKeyW(
				hStorageKey, 
				szValueName, 
				&hDeviceKey) == ERROR_SUCCESS 
			)
		{
			dwDataSize = MAX_PATH - 1;
			if ( ( RegQueryValueExW( 
				hDeviceKey, 
				L"SCSILUN", 
				NULL,
				&dwType,					
				(LPBYTE)szValueName, 
				&dwDataSize) == ERROR_SUCCESS ) && (szValueName[0] == L'0') )
			{
				dwDataSize = MAX_PATH - 1;
				if( RegQueryValueExW(
						hDeviceKey,			
						L"CurrentDriveLetterAssignment",					
						NULL,						
						&dwType,					
						szValueData,				
						&dwDataSize					
					) == ERROR_SUCCESS )
				{
					char drive[4];
					drive[0] = szValueData[0];
					drive[1] = ':';
					drive[2] = '\\';
					drive[3] = 0;
					// it could be that an old entry still remains in the registry and has been 
					// mapped to some other device.
					if( ::GetDriveTypeA(drive) == DRIVE_REMOVABLE )
					{
						// the only other reason that this drive doesn't belongs to our device 
						// if some other removable device has been mapped to this drive letter,
						// let the caller of this routine handle it.
						_drive_letters += szValueData[0];
						Found = TRUE;
						CStTrace::trace( "Got Stmp drive" );
					}
				}
			}
			RegCloseKey( hDeviceKey );
		}

		dwSecondIndex ++;
		dwNameSize = MAX_PATH - 1;
	}

	RegCloseKey( hStorageKey );
	if( Found == FALSE)
		return STERR_FAILED_TO_FIND_DRIVE_LETTER_IN_REGISTRY;
	return STERR_NONE;
}
