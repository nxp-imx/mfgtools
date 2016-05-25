/*
 * Copyright 2009-2011, 2016 Freescale Semiconductor, Inc.
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

//
// These resource definitions MUST MATCH between StBinder and the updater.
//
#pragma once

#define ST_MAX_RESNAMELEN				128

#define BASESDK_MAX						64
#define COMPANYNAME_MAX					64
#define PRODDESC_MAX					64
#define PRODNAME_MAX					64
#define APPTITLE_MAX					64
#define COPYRIGHT_MAX					128
#define COMMENT_MAX						128
#define SCSIMFG_MAX						8
#define SCSIPROD_MAX					16
#define MTPMFG_MAX						64
#define MTPPROD_MAX						64
#define USBVENDOR_MAX					4
#define USBPROD_MAX						4
#define DRIVELABEL_MAX					32

#define L_STMP_FW_RESTYPE				L"STMPFWRESTYPE"
#define L_STMP_RECV_RESTYPE				L"STMPRECVRESTYPE"
#define L_STMP_RESINFO_TYPE				L"STMPRESINFOTYPE"


typedef enum
{
	DLG_DEFAULT		= 0,
	DLG_MINIMAL		= 1,
	DLG_STANDARD	= 2,
	DLG_ADVANCED	= 3
} USER_DLG_TYPE;

typedef struct _fwresinfo
{
	int		iResId;
	LANGID	wLangId;
	TCHAR	szResourceName[ST_MAX_RESNAMELEN];
} STFWRESINFO, *PSTFWRESINFO;

typedef struct _DRIVE_DESC {
   wchar_t  wchName[MAX_PATH];
   wchar_t  wchDescription[MAX_PATH];
   UCHAR    uchType;                    // Minimum: 1 for data drive, 3 for system drives, 0 for non-volatile
   UCHAR    uchTag;
//   BOOLEAN  bEncrypted;
   ULONG    ulRequestedKB;
} DRIVE_DESC, *PDRIVE_DESC;
