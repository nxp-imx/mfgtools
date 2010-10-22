/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
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
#include "StHiddenDataDrive.h"

CStHiddenDataDrive::CStHiddenDataDrive(string _name ): CStSystemDrive(LANG_NEUTRAL, _name )
{
    memset( &m_HiddenDriveData, 0, sizeof( HIDDEN_DATA_DRIVE_FORMAT ) );

    m_HiddenDriveData.JanusSignature0           = 0x80071119;
    m_HiddenDriveData.JanusSignature1           = 0x19082879;
    m_HiddenDriveData.JanusFormatID             = 0x100;
    m_HiddenDriveData.JanusBootSectorOffset     = 20;
    m_HiddenDriveData.JanusDriveTotalSectors    = 0;//sizeof( HIDDEN_DATA_DRIVE_FORMAT );

	m_upgrade_entry.Type		= DriveTypeHiddenData;
	m_current_entry.Type		= DriveTypeHiddenData;
}

CStHiddenDataDrive::CStHiddenDataDrive( const CStHiddenDataDrive& _sysdrive ) : CStSystemDrive(LANG_NEUTRAL )
{
	*this = _sysdrive;
}

CStHiddenDataDrive& CStHiddenDataDrive::operator=(const CStHiddenDataDrive& _sysdrive)
{
    CStSystemDrive( *((CStSystemDrive *)&_sysdrive) );

	m_HiddenDriveData		= _sysdrive.m_HiddenDriveData;
    
	return *this;
}

CStHiddenDataDrive::~CStHiddenDataDrive(void)
{
}

ST_ERROR CStHiddenDataDrive::Initialize(CStScsi* _p_scsi, CStUpdater* _p_updater, MEDIA_ALLOCATION_TABLE_ENTRY* _current_entry, WORD _ROMRevID)
{
	m_p_scsi    = _p_scsi;
	m_p_updater = _p_updater;
    m_ROMRevID  = _ROMRevID;


	if( _current_entry )
	{
		m_current_entry.DriveNumber = _current_entry->DriveNumber;
		m_current_entry.SizeInBytes = _current_entry->SizeInBytes;
		m_current_entry.Tag			= _current_entry->Tag;
		m_current_entry.Type		= _current_entry->Type;
	}

    // if this is the JANUS drive
    if ( m_upgrade_entry.Tag == DRIVE_TAG_DATA_HIDDEN )
    {
        m_upgrade_entry.SizeInBytes     = m_HiddenDriveData.JanusDriveTotalSectors;
        return STERR_NONE;
    }
    else
    {
        ST_ERROR err = CStSystemDrive::Initialize(_p_scsi, _p_updater, _current_entry, _ROMRevID);
		return err == STERR_NO_FILE_SPECIFIED ? STERR_NONE : err;
    }

}

ST_ERROR CStHiddenDataDrive::Download( ULONG& _num_iterations )
{
	CString str;
    ST_ERROR err = STERR_NONE;
	ULONG sector_count=0;
    ULONGLONG start_sector = 0;

	// if this is the JANUS drive
    if ( m_upgrade_entry.Tag == DRIVE_TAG_DATA_HIDDEN )
    {
		str.Format(_T(" Writing JANUS header info to Logical Drive %d."), m_current_entry.DriveNumber);
		GetUpdater()->GetLogger()->Log(str);

	    ULONGLONG drive_size_in_sectors=0;
	    ULONG sectors_per_write = 0, number_of_iterations = 0;
	    ULONG sectors_left_for_last_iteration = 0;

	    err = GetSectorSize(m_sector_size);
	    if( err != STERR_NONE )
	    {
			GetUpdater()->GetLogger()->Log(_T("   FAIL"));
		    return err;
	    }

	    err = GetSizeInSectors(drive_size_in_sectors);
	    if( err != STERR_NONE )
		{
			GetUpdater()->GetLogger()->Log(_T("   FAIL"));
		    return err;
		}

	    sector_count = (ULONG)drive_size_in_sectors;
        
        // update hidden data drive sectors.
        m_HiddenDriveData.JanusDriveTotalSectors = sector_count;
    	
	    sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	    if( !sectors_per_write )
	    {
		    sectors_per_write = 1;
	    }

	    // Firmware recomputes number of sectors of hidden drive after
	    // allocation.  So, a better method is needed to erase hidden
	    // drive than to rely on old number.  For now, just go through
	    // one iteration.
	    number_of_iterations = sector_count / sectors_per_write;
	    sectors_left_for_last_iteration = sector_count % sectors_per_write;

	    _num_iterations = number_of_iterations;
    	
	    if( sectors_left_for_last_iteration )
		    _num_iterations ++;

	    GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_UPDATE_HIDDENDRIVE, 
		    _num_iterations, m_drive_index);

	    for( ULONG iteration = 0; iteration < number_of_iterations; iteration ++ )
	    {
		    CStByteArray write_data( (size_t)(sectors_per_write * m_sector_size) );
            write_data.InitializeElementsTo( 0xFF );
    		
            if( iteration == 0 )
            {
		        //
		        // Copy data from m_HiddenDriveData to data array
		        // 
                write_data.Write( (void*)&m_HiddenDriveData, sizeof( m_HiddenDriveData ), 0 );
            }

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

            write_data.InitializeElementsTo( 0xFF );

            if( number_of_iterations == 0 )
            {
		        //
		        // Copy data from m_HiddenDriveData to data array
		        // 
                write_data.Write( (void*)&m_HiddenDriveData, sizeof( m_HiddenDriveData ), 0 );
            }

		    err = WriteData(start_sector, sectors_left_for_last_iteration, write_data);
		    if( err != STERR_NONE )
				goto Error_Exit;

		    GetUpdater()->GetProgress()->UpdateProgress();
	    }

	    return err;
    }
    else
    {
        return CStSystemDrive::Download(_num_iterations);
    }
Error_Exit:

	if ( err == STERR_NONE )
		str.Format(_T("  (PASS) Wrote 0x%x(%d) sectors."), sector_count, sector_count);
	else
		str.Format(_T("  (FAIL) Wrote 0x%x(%d) of 0x%x(%d) sectors."), start_sector, start_sector, sector_count, sector_count);
	GetUpdater()->GetLogger()->Log(str);

	return err;
}

ST_ERROR CStHiddenDataDrive::GetCurrentComponentVersion(CStVersionInfo&)
{
    return STERR_NONE;
}

ST_ERROR CStHiddenDataDrive::GetCurrentProjectVersion(CStVersionInfo&)
{
    return STERR_NONE;
}

ST_ERROR CStHiddenDataDrive::GetUpgradeComponentVersion(CStVersionInfo&)
{
    return STERR_NONE;
}

ST_ERROR CStHiddenDataDrive::GetUpgradeProjectVersion(CStVersionInfo&)
{
    return STERR_NONE;
}

ST_ERROR CStHiddenDataDrive::SetComponentVersion(CStVersionInfo)
{
    return STERR_NONE;
}

ST_ERROR CStHiddenDataDrive::SetProjectVersion(CStVersionInfo)
{
    return STERR_NONE;
}

CStHiddenDataDrivePtrArray::CStHiddenDataDrivePtrArray(size_t _size, WORD _langid, string _name):
	CStArray<CStHiddenDataDrive*>(_size, _name)
{
	CStHiddenDataDrive* drive;
	for(size_t index=0; index<_size; index ++)
	{
		drive = new CStHiddenDataDrive (/*_langid*/);
		SetAt(index, drive);
	}
UNREFERENCED_PARAMETER(_langid);
}

CStHiddenDataDrivePtrArray::~CStHiddenDataDrivePtrArray()
{
	CStHiddenDataDrive* drive;
	for(size_t index=0; index<GetCount(); index ++)
	{
		drive = *GetAt(index);
		delete drive;
		SetAt(index, NULL);
	}
}

