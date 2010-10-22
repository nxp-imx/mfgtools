/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "stsystemdrive.h"

#pragma pack (push, 1)
 
typedef struct _HiddenDataDriveFormat {

    BYTE First16Bytes[16];
    ULONG JanusSignature0;
    ULONG JanusSignature1;
    USHORT JanusFormatID;
    USHORT JanusBootSectorOffset;
    ULONG JanusDriveTotalSectors;

} HIDDEN_DATA_DRIVE_FORMAT, *P_HIDDEN_DATA_DRIVE_FORMAT;

class CStHiddenDataDrive : public CStSystemDrive {

public:

	CStHiddenDataDrive(string name="CStHiddenDataDrive");
	CStHiddenDataDrive(const CStHiddenDataDrive& sysdrive);
	CStHiddenDataDrive& operator=(const CStHiddenDataDrive&);
	virtual ~CStHiddenDataDrive();
	
	virtual ST_ERROR Initialize(CStScsi* pScsi, CStUpdater* _p_updater, MEDIA_ALLOCATION_TABLE_ENTRY* _entry, WORD _ROMRevID);

	virtual ST_ERROR Download( ULONG& _num_iterations );
    virtual ST_ERROR GetCurrentComponentVersion(CStVersionInfo& ver);
    virtual ST_ERROR GetCurrentProjectVersion(CStVersionInfo& ver);
	virtual ST_ERROR GetUpgradeComponentVersion(CStVersionInfo& ver);
	virtual ST_ERROR GetUpgradeProjectVersion(CStVersionInfo& ver);

	virtual ST_ERROR SetComponentVersion(CStVersionInfo ver);
	virtual ST_ERROR SetProjectVersion(CStVersionInfo ver);

private:

    HIDDEN_DATA_DRIVE_FORMAT m_HiddenDriveData;
    WORD    m_ROMRevID;
};

class CStHiddenDataDrivePtrArray : public CStArray<class CStHiddenDataDrive*> {
public:
	CStHiddenDataDrivePtrArray(size_t size, WORD _langid, string name="CStHiddenDataDrivePtrArray");
	virtual ~CStHiddenDataDrivePtrArray();

};
#pragma pack (pop)
