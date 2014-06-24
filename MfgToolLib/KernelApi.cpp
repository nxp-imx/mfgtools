/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "KernelApi.h"
#include "WindowsVersionInfo.h"

KernelApi::KernelApi(void)
: GetVolumeNameForVolumeMountPointA(NULL),
#if defined(UNICODE)
  GetVolumeNameForVolumeMountPointW(NULL),
#endif
  available(false),
  hModuleKernel32(NULL)
{
    memset(this, 0, sizeof(*this));
    hModuleKernel32 = LoadLibrary(_T("KERNEL32.dll"));
    if (hModuleKernel32)
    {	    
		// We got the library, that should work no matter what we're running on.
		FINDPROC(GetVolumeNameForVolumeMountPointA);
		// Get wide versions of functions if on unicode
#if defined(UNICODE)
		//if (gWinVersionInfo().IsWinNT())
		//{
			FINDPROC(GetVolumeNameForVolumeMountPointW);
		//}
#endif
		if ( GetVolumeNameForVolumeMountPointA )
		{
#if defined(UNICODE)	
			// if NT based
			//if (gWinVersionInfo().IsWinNT())
			{
				// Check we have the wide versions as well
				if ( GetVolumeNameForVolumeMountPointW )
				{
					available = true;
				}
			}
#else
			available = true;
#endif
		} // end if (ANSI function pointers)
	} // end if (hModuleKernel32)
//    TRACEC(TRACE_USBENUM, "Leave KernelAPI constructor, hModuleKernel32=%p.\n", hModuleKernel32);
}

KernelApi::~KernelApi(void)
{
//    TRACEC(TRACE_USBENUM, "Enter KernelAPI destructor, hModuleKernel32=%p.\n", hModuleKernel32);
	if (hModuleKernel32)
		FreeLibrary(hModuleKernel32);
//    TRACEC(TRACE_USBENUM, "Leave KernelAPI destructor.\n");
}

BOOL KernelApi::apiGetVolumeNameForVolumeMountPoint(
		CString ssVolumeMountPoint,
		CString& ssVolumeName)
{
//	LPCWSTR lpszVolumeMountPoint,
//	LPWSTR lpszVolumeName,
//	DWORD cchBufferLength);
	TCHAR Buffer[MAX_PATH];
	memset(Buffer, 0, MAX_PATH);
	BOOL success;
	ssVolumeName.Empty();

#if defined(UNICODE)
	USES_CONVERSION;
    // if on NT based ODS use Wide versions
    //if (gWinVersionInfo().IsWinNT())
    {
		success = GetVolumeNameForVolumeMountPointW(ssVolumeMountPoint.GetBuffer(), Buffer, MAX_PATH);
		ssVolumeMountPoint.ReleaseBuffer();
		if (success)
			ssVolumeName = Buffer;
		
		return success;		
    }
#else
    Success = GetVolumeNameForVolumeMountPoint(ssVolumeMountPoint.GetBuffer(), Buffer, MAX_PATH);
	ssVolumeMountPoint.ReleaseBuffer();
	if (success)
		ssVolumeName = Buffer;

	return success;
#endif
}

// The one and only KernelApi object
KernelApi& gKernelApi()
{
	static KernelApi kApi;
	return kApi;
};
