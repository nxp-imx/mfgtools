/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
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



