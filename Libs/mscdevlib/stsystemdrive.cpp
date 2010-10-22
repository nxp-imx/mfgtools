/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StSystemDrive.cpp: implementation of the CStSystemDrive class.
//
//////////////////////////////////////////////////////////////////////

#include "stheader.h"
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
#include "StProgress.h"
#include "StSystemDrive.h"
#include <assert.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStSystemDrive::CStSystemDrive(WORD _langid, string _name):CStDrive( _name )
{
	m_p_scsi					= NULL;
	m_p_updater					= NULL;
	m_sector_size				= 0;
	m_p_fw_component			= NULL;
	m_drive_index				= 0;	
    m_langid                    = _langid;

	m_upgrade_entry.Type		= DriveTypeSystem;
	m_upgrade_entry.Tag			= 0;
	m_upgrade_entry.SizeInBytes = 0;
	m_upgrade_entry.DriveNumber	= CSTDRIVE_INVALID_DRIVE_NUMBER;

	m_current_entry.Type		= DriveTypeSystem;
	m_current_entry.Tag			= 0;
	m_current_entry.SizeInBytes = 0;
	m_current_entry.DriveNumber	= CSTDRIVE_INVALID_DRIVE_NUMBER;
}
/*
CStSystemDrive::CStSystemDrive( const CStSystemDrive& _sysdrive )
{
	*this = _sysdrive;
}

CStSystemDrive& CStSystemDrive::operator=(const CStSystemDrive& _sysdrive)
{
	m_p_scsi				= _sysdrive.m_p_scsi;
	m_p_updater				= _sysdrive.m_p_updater;
	m_sector_size			= _sysdrive.m_sector_size;
	
	m_drive_index			= _sysdrive.m_drive_index;
	m_current_entry			= _sysdrive.m_current_entry;
	m_upgrade_entry			= _sysdrive.m_upgrade_entry;

	m_last_error			= _sysdrive.m_last_error;
	m_system_last_error		= _sysdrive.m_system_last_error;
	m_obj_name				= _sysdrive.m_obj_name;

	if( _sysdrive.m_p_fw_component )
	{
		if( m_p_fw_component )
			delete m_p_fw_component;

		m_p_fw_component		= new CStFwComponent(*_sysdrive.m_p_fw_component);
	}
	return *this;
}
*/
CStSystemDrive::~CStSystemDrive()
{
	if(m_p_fw_component)
		delete m_p_fw_component;
}

void CStSystemDrive::Trash()
{
	if(m_p_fw_component)
	{
		delete m_p_fw_component;
		m_p_fw_component = NULL;
	}
}

//
// Initializes m_p_scsi object
//             m_sector_size
//             m_current_entry
//             m_upgrade_entry
//
ST_ERROR CStSystemDrive::Initialize(CStScsi* _p_scsi, CStUpdater* _p_updater, MEDIA_ALLOCATION_TABLE_ENTRY* _current_entry, WORD _ROMRevID)
{
	ULONG requested_allocation = 0;

	m_p_scsi        = _p_scsi;
	m_p_updater     = _p_updater;
    m_ROMRevID      = _ROMRevID;

	if( _current_entry )
	{
		m_current_entry.DriveNumber = _current_entry->DriveNumber;
		m_current_entry.SizeInBytes = _current_entry->SizeInBytes;
		m_current_entry.Tag			= _current_entry->Tag;
		m_current_entry.Type		= _current_entry->Type;
	}

	m_p_fw_component = new CStFwComponent(GetUpdater(), 
		                                m_drive_index,
                                        m_langid);

	if ( m_p_fw_component && m_p_fw_component->GetLastError() == STERR_NO_FILE_SPECIFIED )
		return STERR_NO_FILE_SPECIFIED;

	if ( !m_p_fw_component || m_p_fw_component->IsResourceLoaded() == FALSE )
		return STERR_FAILED_TO_OPEN_FILE;

	GetUpdater()->GetConfigInfo()->GetRequestedDriveSize(m_drive_index, requested_allocation);

	if( requested_allocation > m_p_fw_component->GetSizeInBytes() )
		m_upgrade_entry.SizeInBytes = requested_allocation;
	else
		m_upgrade_entry.SizeInBytes = (ULONGLONG)(double)(m_p_fw_component->GetSizeInBytes() * GetUpdater()->GetConfigInfo()->GetSystemDrivePaddingFactor());

	if( m_p_fw_component->GetLastError() != STERR_NONE)
	{
		return m_p_fw_component->GetLastError();
	}

	return STERR_NONE;
}

ST_ERROR CStSystemDrive::Open()
{
	ST_ERROR err = STERR_NONE;
	ULONG requested_allocation = 0;

	if( !m_p_fw_component->IsResourceLoaded() )
	{
		err = m_p_fw_component->OpenFile();

		if ( err == STERR_NONE )
		{
			GetUpdater()->GetConfigInfo()->GetRequestedDriveSize(m_drive_index, requested_allocation);

			if( requested_allocation > m_p_fw_component->GetSizeInBytes() )
				m_upgrade_entry.SizeInBytes = requested_allocation;
			else
				m_upgrade_entry.SizeInBytes = (ULONGLONG)(double)(m_p_fw_component->GetSizeInBytes() * GetUpdater()->GetConfigInfo()->GetSystemDrivePaddingFactor());
		}
	}
	return err;
}

ST_ERROR CStSystemDrive::Download( ULONG& _num_iterations )
{
	USES_CONVERSION;

	CStVersionInfo ver;
	ULONG sector_count=0;
	ULONGLONG start_sector = 0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;
	ST_ERROR err = STERR_NONE;

	assert(m_p_fw_component);

	CString str;
	str.Format(_T(" Writing %s to Logical Drive %d, tag: 0x%02x, type %d. "), A2T(m_p_fw_component->GetFileName().c_str()), m_current_entry.DriveNumber, m_current_entry.Tag, m_current_entry.Type);
	GetUpdater()->GetLogger()->Log(str);

	m_p_fw_component->GetProjectVersion(ver);
	err = SetProjectVersion(ver);
	if( err != STERR_NONE )
	{
		GetUpdater()->GetLogger()->Log(_T("   FAIL"));
		return err;
	}
	m_p_fw_component->GetComponentVersion(ver);
	err = SetComponentVersion(ver);
	if( err != STERR_NONE )
	{
		GetUpdater()->GetLogger()->Log(_T("   FAIL"));
		return err;
	}

	err = GetSectorSize(m_sector_size);
	if( err != STERR_NONE )
	{
		GetUpdater()->GetLogger()->Log(_T("   FAIL"));
		return err;
	}
	str.Format(_T("  Sector size for Logical Drive %d is %d bytes."), m_current_entry.DriveNumber, m_sector_size);
	GetUpdater()->GetLogger()->Log(str);

// CLW 3.26.2007 - trying to streamline the mfg tool. only wrote the number of sectors we need to write.
//	sector_count = (ULONG)(m_upgrade_entry.SizeInBytes / m_sector_size);

//	if( m_upgrade_entry.SizeInBytes % m_sector_size )
//	{
//		sector_count ++;	
//	}

	sector_count = (ULONG)m_p_fw_component->GetSizeInSectors(m_sector_size);

	sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}
	str.Format(_T("  Firmware %s is %d bytes (%d sectors)"), A2T(m_p_fw_component->GetFileName().c_str()), (ULONG)m_p_fw_component->GetSizeInBytes(), (ULONG)m_p_fw_component->GetSizeInSectors(m_sector_size));
	GetUpdater()->GetLogger()->Log(str);
	str.Format(_T("  Firmware is loaded from %s."), m_p_fw_component->IsLoadedFromResource() ? _T("Resource") : _T("File"));
	GetUpdater()->GetLogger()->Log(str);

	number_of_iterations = sector_count / sectors_per_write;
	sectors_left_for_last_iteration = sector_count % sectors_per_write;

	_num_iterations = number_of_iterations;
	
	if( sectors_left_for_last_iteration )
		_num_iterations ++;

	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_UPDATE_SYSTEMDRIVE, 
		_num_iterations, m_drive_index);

	for( ULONG iteration = 0; iteration < number_of_iterations; iteration ++ )
	{
		CStByteArray write_data( (size_t)(sectors_per_write * m_sector_size) );
		
		//
		// Copy data from fw_component to data array
		// 
		err = m_p_fw_component->GetData(iteration*write_data.GetCount(), write_data.GetCount(), &write_data);
		if( err != STERR_NONE )
			goto Error_Exit;

		err = WriteData(start_sector, sectors_per_write, write_data);
		if( err != STERR_NONE )
			goto Error_Exit;

		start_sector += sectors_per_write;
		GetUpdater()->GetProgress()->UpdateProgress();
	}

	//
	// run the last iteration
	// Initialize data to 0xFF
	//

	if( sectors_left_for_last_iteration )
	{
		CStByteArray write_data( (size_t)(sectors_left_for_last_iteration * m_sector_size) );

		//
		// Copy remaining data from fw_component to data array
		// 
		err = m_p_fw_component->GetData(number_of_iterations * sectors_per_write * m_sector_size, 
			sectors_left_for_last_iteration * m_sector_size, &write_data);
		if( err != STERR_NONE )
			goto Error_Exit;

		err = WriteData(start_sector, sectors_left_for_last_iteration, write_data);
		if( err != STERR_NONE )
			goto Error_Exit;

		GetUpdater()->GetProgress()->UpdateProgress();
	}

Error_Exit:

	if ( err == STERR_NONE )
		str.Format(_T("  (PASS) Wrote 0x%x(%d) sectors."), sector_count, sector_count);
	else
		str.Format(_T("  (FAIL) Wrote 0x%x(%d) of 0x%x(%d) sectors."), start_sector, start_sector, sector_count, sector_count);
	GetUpdater()->GetLogger()->Log(str);

	return err;
}

ST_ERROR CStSystemDrive::VerifyDownload()
{
	USES_CONVERSION;

	ULONG sector_count=0;
	ULONGLONG start_sector = 0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;
	ST_ERROR err = STERR_NONE;

	assert(m_p_fw_component);

	CString str;
	str.Format(_T(" Verifying %s on Logical Drive %d, tag: 0x%02x, type %d. "), A2T(m_p_fw_component->GetFileName().c_str()), m_current_entry.DriveNumber, m_current_entry.Tag, m_current_entry.Type);
	GetUpdater()->GetLogger()->Log(str);

// CLW 11.26.2007 - trying to streamline verification. only verify the number of sectors we wrote.
	sector_count = (ULONG)m_p_fw_component->GetSizeInSectors(m_sector_size);

	sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}

	number_of_iterations = sector_count / sectors_per_write;
	sectors_left_for_last_iteration = sector_count % sectors_per_write;

#ifdef _DEBUG
	ofstream outfile; 
	string filename;
	if( GetUpdater()->GetCmdLineProcessor()->GenXRFiles() )
	{
		//
		// get filename from configinfo object.
		//
		m_p_updater->GetConfigInfo()->GetSystemDriveName(m_drive_index, filename);
		filename += "r";

		outfile.open(filename.c_str(), ios::binary | ios::out );//no need to worry about close, the destructor will do it anyway.
	}
#endif

	for( ULONG iteration = 0; iteration < number_of_iterations; iteration ++ )
	{
		CStByteArray write_data( (size_t)(sectors_per_write * m_sector_size) );
		CStByteArray read_data( (size_t)(sectors_per_write * m_sector_size) );
		
		//
		// Copy data from fw_component to data array
		// 
		err = m_p_fw_component->GetData(iteration*write_data.GetCount(), write_data.GetCount(), &write_data);
		if( err != STERR_NONE )
			goto Error_Exit;

		err = ReadData(start_sector, sectors_per_write, read_data);
		if( err != STERR_NONE )
			goto Error_Exit;

#ifdef _DEBUG
		if( GetUpdater()->GetCmdLineProcessor()->GenXRFiles() )
		{
			PUCHAR data_read = new UCHAR[read_data.GetCount()];
			read_data.Read((void*)data_read, read_data.GetCount(), 0);
			outfile.write( (const char*)data_read, (streamsize)read_data.GetCount() );
			delete[] data_read;
		}
#endif
		if( read_data != write_data )
		{
#ifdef _DEBUG
	if( GetUpdater()->GetCmdLineProcessor()->GenXRFiles() )
	{
			outfile.close();
	}
#endif
			err = STERR_FAILED_READ_BACK_VERIFY_TEST;
			goto Error_Exit;
		}

		start_sector += sectors_per_write;
		GetUpdater()->GetProgress()->UpdateProgress();
	}

	//
	// run the last iteration
	// Initialize data to 0xFF
	//

	if( sectors_left_for_last_iteration )
	{
		CStByteArray write_data( (size_t)(sectors_left_for_last_iteration * m_sector_size) );
		CStByteArray read_data( (size_t)(sectors_left_for_last_iteration * m_sector_size) );

		//
		// Copy remaining data from fw_component to data array
		// 
		err = m_p_fw_component->GetData(number_of_iterations * sectors_per_write * m_sector_size, 
			sectors_left_for_last_iteration * m_sector_size, &write_data);
		if( err != STERR_NONE )
			goto Error_Exit;

		err = ReadData(start_sector, sectors_left_for_last_iteration, read_data);
		if( err != STERR_NONE )
			goto Error_Exit;
			
#ifdef _DEBUG
		if( GetUpdater()->GetCmdLineProcessor()->GenXRFiles() )
		{
			PUCHAR data_read = new UCHAR[read_data.GetCount()];
			read_data.Read((void*)data_read, read_data.GetCount(), 0);
			outfile.write( (const char*)data_read, (streamsize)read_data.GetCount() );
			delete[] data_read;
			outfile.close();
		}
#endif	
		if( read_data != write_data )
		{
			err = STERR_FAILED_READ_BACK_VERIFY_TEST;
			goto Error_Exit;
		}
		
		GetUpdater()->GetProgress()->UpdateProgress();
	}
Error_Exit:

	if ( err == STERR_NONE )
		str.Format(_T("  (PASS) Verified 0x%x(%d) sectors."), sector_count, sector_count);
	else
		str.Format(_T("  (FAIL) Verified 0x%x(%d) of 0x%x(%d) sectors."), start_sector, start_sector, sector_count, sector_count);
	GetUpdater()->GetLogger()->Log(str);

	return err;
}

ST_ERROR CStSystemDrive::GetCurrentComponentVersion(CStVersionInfo& _ver)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoComponentVersion, sizeof(CStVersionInfo));
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	assert(m_p_scsi);
	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetComponentVersion(_ver);
}

ST_ERROR CStSystemDrive::GetCurrentProjectVersion(CStVersionInfo& _ver)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoProjectVersion, sizeof(CStVersionInfo));
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	assert(m_p_scsi);
	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetProjectVersion(_ver);
}

ST_ERROR CStSystemDrive::GetUpgradeComponentVersion(CStVersionInfo& _ver)
{
	if( !m_p_fw_component )
		return STERR_NONE;
	return m_p_fw_component->GetComponentVersion(_ver);
}

ST_ERROR CStSystemDrive::GetUpgradeProjectVersion(CStVersionInfo& _ver)
{
	if( !m_p_fw_component )
		return STERR_NONE;
	return m_p_fw_component->GetProjectVersion(_ver);
}

ST_ERROR CStSystemDrive::IsWriteProtected(ST_BOOLEAN& _write_protected)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoIsWriteProtected, sizeof(ST_BOOLEAN));
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	assert(m_p_scsi);
	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.IsWriteProtected(_write_protected);
}

ST_ERROR CStSystemDrive::GetSizeOfSerialNumberInBytes(USHORT& _size)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoSizeOfSerialNumberInBytes, 
		sizeof(USHORT));
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	assert(m_p_scsi);
	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetSizeOfSerialNumberInBytes(_size);
}

//
// arr should be initialized to the size of the serial number. 
// 
ST_ERROR CStSystemDrive::GetSerialNumber(CStByteArray& _arr)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoSerialNumber, _arr.GetCount());
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	assert(m_p_scsi);
	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetSerialNumber(_arr);
}

ST_ERROR CStSystemDrive::ReadData(ULONGLONG _sector_start, ULONG _sector_count, CStByteArray& _arr)
{
	ST_ERROR result=STERR_NONE;
	CStReadLogicalDriveSector api(m_current_entry.DriveNumber, m_sector_size, _sector_start, _sector_count);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	assert(m_p_scsi);
	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetData(_arr);
}

ST_ERROR CStSystemDrive::WriteData(ULONGLONG _sector_start, ULONG _sector_count, CStByteArray& _arr)
{
	ST_ERROR result=STERR_NONE;

    CStWriteLogicalDriveSector api(m_current_entry.DriveNumber, m_sector_size, _sector_start, _sector_count);
    if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = api.PutData(_arr);
	if(result != STERR_NONE)
		return result;

	assert(m_p_scsi);
	return m_p_scsi->SendDdiApiCommand(&api);
}

ST_ERROR CStSystemDrive::ReadDrive(CStByteArray* _p_arr)
{
	ULONGLONG sector_count=0;
	ULONGLONG start_sector = 0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;
	ST_ERROR err = STERR_NONE;

	err = GetSizeInSectors(sector_count);
	err = GetSectorSize(m_sector_size);

	if (sector_count == 0 || m_sector_size == 0)
		return STERR_FAILED_TO_SEND_SCSI_COMMAND;

	sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}

	number_of_iterations = (ULONG)(sector_count / sectors_per_write);
	sectors_left_for_last_iteration = (ULONG)(sector_count % sectors_per_write);

	for( ULONG iteration = 0; iteration < number_of_iterations; iteration ++ )
	{
		CStByteArray read_data( (size_t)(sectors_per_write * m_sector_size) );
		
		err = ReadData(start_sector, sectors_per_write, read_data);
		if( err != STERR_NONE )
			goto Error_Exit;

		read_data.CopyTo(_p_arr, (size_t)(start_sector * m_sector_size));

		start_sector += sectors_per_write;
//		GetUpdater()->GetProgress()->UpdateProgress();
	}

	//
	// run the last iteration
	// Initialize data to 0xFF
	//

	if( sectors_left_for_last_iteration )
	{
		CStByteArray read_data( (size_t)(sectors_left_for_last_iteration * m_sector_size) );

		err = ReadData(start_sector, sectors_left_for_last_iteration, read_data);
		if( err != STERR_NONE )
			goto Error_Exit;
			
		read_data.CopyTo(_p_arr, (size_t)(start_sector * m_sector_size));

//		GetUpdater()->GetProgress()->UpdateProgress();
	}
Error_Exit:

	return err;
}

ST_ERROR CStSystemDrive::WriteDrive(CStByteArray* _p_arr)
{
	ULONGLONG sector_count=0;
	ULONGLONG start_sector = 0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;
	ST_ERROR err = STERR_NONE;

//	err = GetSizeInSectors(sector_count);
	err = GetSectorSize(m_sector_size);

	size_t dataSize = _p_arr->GetCount();
	sector_count = ( dataSize / m_sector_size ) + (( dataSize % m_sector_size ) ? 1 : 0);

	sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}

	number_of_iterations = (ULONG)(sector_count / sectors_per_write);
	sectors_left_for_last_iteration = (ULONG)(sector_count % sectors_per_write);

	for( ULONG iteration = 0; iteration < number_of_iterations; iteration ++ )
	{
		CStByteArray write_data( (size_t)(sectors_per_write * m_sector_size) );
		CStByteArray::Copy(_p_arr, (size_t)(start_sector * m_sector_size), &write_data, 0, sectors_per_write * m_sector_size);
		
		err = WriteData(start_sector, sectors_per_write, write_data);
		if( err != STERR_NONE )
			goto Error_Exit;

		start_sector += sectors_per_write;
//		GetUpdater()->GetProgress()->UpdateProgress();
	}

	//
	// run the last iteration
	// Initialize data to 0xFF
	//

	if( sectors_left_for_last_iteration )
	{
		CStByteArray write_data( (size_t)(sectors_left_for_last_iteration * m_sector_size) );
		CStByteArray::Copy(_p_arr, (size_t)(start_sector * m_sector_size), &write_data, 0, sectors_left_for_last_iteration * m_sector_size);

		err = WriteData(start_sector, sectors_left_for_last_iteration, write_data);
		if( err != STERR_NONE )
			goto Error_Exit;
			
//		GetUpdater()->GetProgress()->UpdateProgress();
	}
Error_Exit:

	return err;
}

/*
ST_ERROR CStSystemDrive::SetTag(UCHAR _tag)
{
	ST_ERROR result=STERR_NONE;
	CStSetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoTag);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = api.SetTag(_tag);
	if(result != STERR_NONE)
		return result;

	assert(m_p_scsi);
	return m_p_scsi->SendDdiApiCommand(&api);
}
*/
ST_ERROR CStSystemDrive::SetComponentVersion(CStVersionInfo _ver)
{
	ST_ERROR result=STERR_NONE;
	CStSetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoComponentVersion);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = api.SetComponentVersion(_ver);
	if(result != STERR_NONE)
		return result;

	assert(m_p_scsi);
	return m_p_scsi->SendDdiApiCommand(&api);
}

ST_ERROR CStSystemDrive::SetProjectVersion(CStVersionInfo _ver)
{
	ST_ERROR result=STERR_NONE;
	CStSetLogicalDriveInfo api(m_current_entry.DriveNumber, DriveInfoProjectVersion);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = api.SetProjectVersion(_ver);
	if(result != STERR_NONE)
		return result;

	assert(m_p_scsi);
	return m_p_scsi->SendDdiApiCommand(&api);
}

CStSystemDrivePtrArray::CStSystemDrivePtrArray(size_t _size, WORD _langid, string _name):
	CStArray<CStSystemDrive*>(_size, _name)
{
	CStSystemDrive* drive;
	for(size_t index=0; index<_size; index ++)
	{
		drive = new CStSystemDrive(_langid);
		SetAt(index, drive);
	}
}

CStSystemDrivePtrArray::~CStSystemDrivePtrArray()
{
	CStSystemDrive* drive;
	for(size_t index=0; index<GetCount(); index ++)
	{
		drive = *GetAt(index);
		delete drive;
		SetAt(index, NULL);
	}
}

