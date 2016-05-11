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


#include "stdafx.h"
#include "KernelApi.h"
//#include "WindowsVersionInfo.h"

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
