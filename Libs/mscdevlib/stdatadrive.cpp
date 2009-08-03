// StDataDrive.cpp: implementation of the CStDataDrive class.
//
//////////////////////////////////////////////////////////////////////
#include "StHeader.h"
#include "StGlobals.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "ddildl_defs.h"
#include "StDdiApi.h"
#include "StUpdater.h"
#include "StConfigInfo.h"
#include "StRecoveryDev.h"
#include "StScsi.h"
#include "StDataDrive.h"
#include "StSystemDrive.h"
#include "StHiddenDataDrive.h"
#include "StProgress.h"
#include "StUsbMscDev.h"
#include "StDataDrive.h"
#include <assert.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStDataDrive::CStDataDrive(string _name):CStDrive(_name)
{
	m_p_updater				= NULL;
	m_p_scsi				= NULL;
	m_drive_index			= 0;
	m_sector_size			= 0;
	m_upgrade_entry.Type	= DriveTypeData;
	m_upgrade_entry.SizeInBytes	= 0;
}

CStDataDrive::~CStDataDrive()
{

}

void CStDataDrive::Trash()
{
}


ST_ERROR CStDataDrive::SetSizeInBytes(ULONGLONG _size)
{
	m_upgrade_entry.SizeInBytes = _size;
	return STERR_NONE;
}

//
// Initializes m_p_scsi object
//             m_sector_size
//             m_current_entry
//
ST_ERROR CStDataDrive::Initialize(CStScsi* _p_scsi, CStUpdater* _p_updater, MEDIA_ALLOCATION_TABLE_ENTRY* _entry, WORD _ROMRevID)
{
	m_p_scsi = _p_scsi;
	m_p_updater = _p_updater;
    m_ROMRevID  = _ROMRevID;

	if( _entry )
	{
		m_current_entry.DriveNumber = _entry->DriveNumber;
		m_current_entry.SizeInBytes = _entry->SizeInBytes;
		m_current_entry.Tag			= _entry->Tag;
		m_current_entry.Type		= _entry->Type;
	}

	return STERR_NONE; //GetSectorSize(m_sector_size);
}

wchar_t CStDataDrive::GetDriveLetter()
{
	if( m_p_scsi )
		return m_p_scsi->GetDriveLetter();
	return '\0';
}

ULONGLONG CStDataDrive::GetSizeInBytes()
{
	return m_current_entry.SizeInBytes;
}

ST_ERROR CStDataDrive::ReadCapacity(PREAD_CAPACITY_DATA _p_read_capacity)
{
	ST_ERROR err=STERR_NONE;
	CStReadCapacity api;
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	assert(m_p_scsi);
	err = m_p_scsi->SendDdiApiCommand(&api);
	if(err != STERR_NONE)
		return err;

	return api.GetCapacity(_p_read_capacity);
}

ST_ERROR CStDataDrive::WriteSector(CStByteArray* _sector, ULONG _num_sectors, ULONG _start_sector_number, ULONG _sector_size)
{
	assert(m_p_scsi);
	return m_p_scsi->WriteSector(_sector, _num_sectors, _start_sector_number, _sector_size);
}

ST_ERROR CStDataDrive::ReadSector(CStByteArray* _sector, ULONG _num_sectors, ULONG _start_sector_number, ULONG _sector_size)
{
	assert(m_p_scsi);
	return m_p_scsi->ReadSector(_sector, _num_sectors, _start_sector_number, _sector_size);
}

CStDataDrivePtrArray::CStDataDrivePtrArray(size_t _size, string _name):
	CStArray<CStDataDrive*>(_size, _name)
{
	CStDataDrive* drive;
	for(size_t index=0; index<_size; index ++)
	{
		drive = new CStDataDrive;
		SetAt(index, drive);
	}
}

CStDataDrivePtrArray::~CStDataDrivePtrArray()
{
	Trash();
}

CStDataDrivePtrArray::CStDataDrivePtrArray(const CStDataDrivePtrArray& _ar):
	CStArray<CStDataDrive*>( _ar )
{
	*this = _ar;
}


CStDataDrivePtrArray& CStDataDrivePtrArray::operator=( const CStDataDrivePtrArray& _ar)
{
	Trash();

	CStDataDrive* drive;
	
	for(size_t index=0; index<_ar.GetCount(); index ++)
	{
		CStDataDrive* temp = *_ar.GetAt(index);
		drive = new CStDataDrive;
		*drive = *temp;
		SetAt(index, drive);
	}
	
	return *this;
}

void CStDataDrivePtrArray::Trash()
{
	CStDataDrive* drive;
	for(size_t index=0; index<GetCount(); index ++)
	{
		drive = *GetAt(index);
		delete drive;
		SetAt(index, NULL);
	}
}