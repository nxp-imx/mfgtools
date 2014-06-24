/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

// Why do we have to go through this? A: Macro expansion rules. ( clw: I guess? )
// Note: these MACROS are also defined in SetupApi.h. We might have to do something about that.
#define STRIZE2(A)		#A
#define STRIZE(A)		STRIZE2(A)
#define FINDPROC(A) if ((A = (P ## A)GetProcAddress(hModuleKernel32, STRIZE(A))) == NULL) available = FALSE
#define FINDPROCH(H,A) if ((A = (P ## A)GetProcAddress(H, STRIZE(A))) == NULL) available = FALSE

class KernelApi
{
	typedef BOOL (WINAPI *PGetVolumeNameForVolumeMountPointA)(
		LPCSTR lpszVolumeMountPoint,
		LPSTR lpszVolumeName,
		DWORD cchBufferLength);

	typedef BOOL (WINAPI *PGetVolumeNameForVolumeMountPointW)(
		LPCWSTR lpszVolumeMountPoint,
		LPWSTR lpszVolumeName,
		DWORD cchBufferLength);

	HMODULE hModuleKernel32;
    bool available;
    
	PGetVolumeNameForVolumeMountPointA GetVolumeNameForVolumeMountPointA;

#if defined(UNICODE)
	PGetVolumeNameForVolumeMountPointW GetVolumeNameForVolumeMountPointW;
#endif

public:
	BOOL apiGetVolumeNameForVolumeMountPoint(
		CString ssVolumeMountPoint,
		CString& ssVolumeName);

	bool IsAvailable() const { return available; }

	KernelApi(void);
	~KernelApi(void);
};

extern KernelApi& gKernelApi();
