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


//#define CString std::string
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <string.h>
#include <cstring>
#include <iostream>
#include <cstdlib>

#include <AtlConv.h>

#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <cerrno>
#include <math.h>
#include <wchar.h>
#include "sched.h"
#include "semaphore.h"
#include "pthread.h"
#include <limits>
#include <time.h>
#include <cstdio>
#include <cfgmgr32.h>
#include <stdint.h>
#include <initguid.h>
#include <devguid.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <SetupAPI.h>
#include <Dbt.h>
#include "CString.h"
#include <libusb.h>

#ifdef DEBUG
#define ASSERT assert
#else
#define ASSERT
#endif


#define TRACE 1? 0: _tprintf
#define libusbVolume Volume
#define TRACE 1? 0: _tprintf
#define sleep(x) Sleep(x)

//definitions from scsi.h that are defined here so they don't use _CDB namespace - parallels lnx_def.h
struct _CDB6INQUIRY {
	UCHAR OperationCode;    // 0x12 - SCSIOP_INQUIRY
	UCHAR Reserved1 : 5;
	UCHAR LogicalUnitNumber : 3;
	UCHAR PageCode;
	UCHAR IReserved;
	UCHAR AllocationLength;
	UCHAR Control;
} CDB6INQUIRY;

struct _CDB12 {
	UCHAR OperationCode;
	UCHAR RelativeAddress : 1;
	UCHAR Reserved1 : 2;
	UCHAR ForceUnitAccess : 1;
	UCHAR DisablePageOut : 1;
	UCHAR LogicalUnitNumber : 3;
	UCHAR LogicalBlock[4];
	UCHAR TransferLength[4];
	UCHAR Reserved2;
	UCHAR Control;
} CDB12;
#if 0

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
#define _tcschr strchr
#define _totlower tolower
#define	_tcslen strlen
#define _istspace isspace
#define _ttol atol
#define _tcspbrk strpbrk
#define _tcsnicmp _strnicmp
//#define .Format =sprintf
#define TCHAR char
#define _T


#define _TCHAR_DEFINED
#define __AFXWIN_H__
#define IsEmpty empty

#endif

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;

struct thread_msg{
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	thread_msg() : message(), wParam(), lParam() {}
	thread_msg(UINT msg, WPARAM wPar, LPARAM lPar):message(msg),wParam(wPar),lParam(lPar){}


};

int gettimeofday(struct timeval *, void* n = nullptr);

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  116444736000000000Ui64 // CORRECT
#else
#define DELTA_EPOCH_IN_MICROSECS  116444736000000000ULL // CORRECT
#endif
//std::string str_format(const std::string fmt_str, ...);
