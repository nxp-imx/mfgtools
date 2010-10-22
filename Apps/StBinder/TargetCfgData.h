/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "..\\..\\common\\updater_res.h"
#include "..\\..\\common\\updater_restypes.h"
#include "stmedia.h"

#pragma pack(push, 2)


typedef struct    // This is the Directory Entry stored in resources
{
	BYTE Width;
	BYTE Height;
	BYTE Colors;
	BYTE Reserved;
	WORD Planes;
	WORD BitsPerPixel;
	DWORD ImageSize;
	WORD ResourceID;
} ICONDIRENTRY_GRP, *PICONDIRENTRY_GRP;

typedef struct    // This is the Directory Entry stored in .ico file
{
	BYTE Width;
	BYTE Height;
	BYTE Colors;
	BYTE Reserved;
	WORD Planes;
	WORD BitsPerPixel;
	DWORD ImageSize;
	DWORD ImageOffset;	// DWORD instead of WORD
} ICONDIRENTRY_ICO, *PICONDIRENTRY_ICO;

typedef struct	// This is the actual RT_GROUP_ICON structure header also
{				// Directory entries for each image follow	
	WORD Reserved;
	WORD ResourceType;
	WORD ImageCount;
	ICONDIRENTRY_ICO DirEntry[];
} ICONHEADER, *PICONHEADER;

typedef struct			// This is the actual RT_GROUP_ICON structure header also
{						// Directory entries for each image follow
  USHORT Reserved1;		// reserved, must be 0
  USHORT ResourceType;	// type is 1 for icons
  USHORT ImageCount;
} RT_GROUP;

typedef struct			// This is the actual RT_GROUP_ICON structure header also
{						// Directory entries for each image follow
  USHORT Reserved1;		// reserved, must be 0
  USHORT ResourceType;	// type is 1 for icons
  USHORT ImageCount;	// number of icons in structure
  ICONDIRENTRY_GRP DirEntry[]; // ImageCount number of these
} GROUPICON, *PGROUPICON;

#pragma pack(pop)


typedef struct _UpdaterCfgOptions
{
	USHORT		CfgVersion;
	WCHAR		BaseSDK[BASESDK_MAX+1];
	WCHAR		CompanyName[COMPANYNAME_MAX+1];
	WCHAR		ProductDesc[PRODDESC_MAX+1];
	WCHAR		ProductName[PRODNAME_MAX+1];
	WCHAR		AppTitle[APPTITLE_MAX+1];
	WCHAR		Copyright[COPYRIGHT_MAX+1];
	WCHAR		Comment[COMMENT_MAX+1];
	WCHAR		SCSIMfg[SCSIMFG_MAX+1];
	WCHAR		SCSIProd[SCSIPROD_MAX+1];
	WCHAR		MtpMfg[MTPMFG_MAX+1];
	WCHAR		MtpProd[MTPPROD_MAX+1];
	WCHAR		DriveLabel[DRIVELABEL_MAX+1];
	USHORT		USBVendor;
	USHORT		USBProd;
	USHORT		SecondaryUSBProd;
	BOOL		AllowAutoRecovery;
	BOOL		ForceRecovery;
	BOOL		RebootToPlayerMsg;
	BOOL		LowNANDSolution;
	BOOL		UseLocalFileResources;
	USER_DLG_TYPE DialogType;		
	BOOL		FormatDataArea;
	BOOL		EraseMedia;
	BOOL		MinDlgFmtMsg;
	USHORT		DefaultFS;
	BOOL		AutoStart;
	BOOL		AutoClose;
	USHORT		UpdMajorVersion;
	USHORT		UpdMinorVersion;
	USHORT		ProdMajorVersion;
	USHORT		ProdMinorVersion;
	USHORT		DriveArrayCount;
	media::LogicalDriveArray DriveArray;
} UPDATER_CFG_OPTIONS, *PUPDATER_CFG_OPTIONS;

typedef struct _TargetCfgResData
{
	CString		ProfileName;
	CString		CompanyImagePathname;
	CString		UpdaterIconPathname;
	HICON		CurrentCompanyIcon;
	HBITMAP		CurrentCompanyBitmap;
	HICON		CurrentUpdaterIcon;
	BOOL		IsBMP;
	DWORD		dwMaxCustomBitmapSize;
	PGROUPICON	pCompanyGrpIconHeader;
	PGROUPICON	pUpdaterGrpIconHeader;

	UPDATER_CFG_OPTIONS	Options;
} TARGET_CFG_DATA, *PTARGET_CFG_DATA;
