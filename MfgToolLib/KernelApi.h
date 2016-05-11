/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#ifndef __linux__
	typedef BOOL (WINAPI *PGetVolumeNameForVolumeMountPointA)(
		LPCSTR lpszVolumeMountPoint,
		LPSTR lpszVolumeName,
		DWORD cchBufferLength);

	typedef BOOL (WINAPI *PGetVolumeNameForVolumeMountPointW)(
		LPCWSTR lpszVolumeMountPoint,
		LPWSTR lpszVolumeName,
		DWORD cchBufferLength);
#endif
	HMODULE hModuleKernel32;
    bool available;
#ifndef __linux__
	PGetVolumeNameForVolumeMountPointA GetVolumeNameForVolumeMountPointA;

#if defined(UNICODE)
	PGetVolumeNameForVolumeMountPointW GetVolumeNameForVolumeMountPointW;
#endif
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
