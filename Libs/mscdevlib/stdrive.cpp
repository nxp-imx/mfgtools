/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StDrive.cpp: implementation of the CStDrive class.
//
//////////////////////////////////////////////////////////////////////

#include "StHeader.h"
#include "ddildl_defs.h"
#include "StConfigInfo.h"
#include "StGlobals.h"
#include "ddildl_defs.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "stddiapi.h"
#include "StUpdater.h"
#include "stfwcomponent.h"
#include "StScsi.h"
#include "StDrive.h"

#define TRANSFER_RATE_BYTES_PER_SECOND 1500000 // according to usb 1.1 speed

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStDrive::CStDrive(string _name):CStBase(_name)
{
	m_p_scsi = NULL;
	m_sector_size = 0;
	m_drive_index = 0xFF;
}

CStDrive::~CStDrive()
{

}

//
// Initializes _entry with m_upgrade_entry.
//
ST_ERROR CStDrive::FillTableEntryToAllocate(MEDIA_ALLOCATION_TABLE_ENTRY* _p_entry)
{
	if( _p_entry )
	{
		_p_entry->DriveNumber	= m_upgrade_entry.DriveNumber;
		_p_entry->SizeInBytes	= m_upgrade_entry.SizeInBytes;
		_p_entry->Tag			= m_upgrade_entry.Tag;
		_p_entry->Type			= m_upgrade_entry.Type;
	}

	return STERR_NONE;
}

ST_ERROR CStDrive::SetUpgradeTableEntry(MEDIA_ALLOCATION_TABLE_ENTRY* _p_entry)
{
	if(!_p_entry)
		return STERR_NONE;
	m_upgrade_entry.DriveNumber	= _p_entry->DriveNumber;
	m_upgrade_entry.SizeInBytes	= _p_entry->SizeInBytes;
	m_upgrade_entry.Tag			= _p_entry->Tag;
	m_upgrade_entry.Type		= _p_entry->Type;

	return STERR_NONE;
}

ST_ERROR CStDrive::SetCurrentTableEntry(MEDIA_ALLOCATION_TABLE_ENTRY* _p_entry)
{
	if(!_p_entry)
		return STERR_NONE;
	m_current_entry.DriveNumber	= _p_entry->DriveNumber; 
	m_current_entry.SizeInBytes	= _p_entry->SizeInBytes;
	m_current_entry.Tag			= _p_entry->Tag;
	m_current_entry.Type		= _p_entry->Type;

	return STERR_NONE;
}

ST_ERROR CStDrive::GetSectorSize(ULONG& _size)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoSectorSizeInBytes, sizeof(ULONG));
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetSectorSize(_size);
}

ST_ERROR CStDrive::EraseDrive()
{
	CString str;
	str.Format(_T(" Erasing Logical Drive %d."), m_current_entry.DriveNumber);
	GetUpdater()->GetLogger()->Log(str);

	CStEraseLogicalDrive api(m_current_entry.DriveNumber);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	return m_p_scsi->SendDdiApiCommand(&api);
}

ST_ERROR CStDrive::SetDriveIndex(UCHAR _drive_num)
{
	m_drive_index = _drive_num;
	return STERR_NONE;
}

ST_ERROR CStDrive::GetTimeToDownload(ULONG& time_in_seconds)
{
	time_in_seconds = (ULONG)(m_current_entry.SizeInBytes / TRANSFER_RATE_BYTES_PER_SECOND);
	if( time_in_seconds == 0 )
	{
		time_in_seconds = 1; // rounded to 1 sec.
	}
	return STERR_NONE;
}

UCHAR CStDrive::GetTag()
{
	return m_upgrade_entry.Tag;
}

void CStDrive::SetTag(UCHAR _tag)
{
	m_upgrade_entry.Tag = _tag;
}

void CStDrive::SetScsi(CStScsi* _p_scsi)
{
	m_p_scsi = _p_scsi;
}

ST_ERROR CStDrive::GetSizeInSectors(ULONGLONG& _size)
{
	ST_ERROR result=STERR_NONE;

	CStGetLogicalDriveInfo  api(m_current_entry.DriveNumber, DriveInfoSizeInSectors, sizeof(ULONGLONG));
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetSizeInSectors(_size);
}
