/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StUsbMscDev.cpp: implementation of the CStUsbMscDev class.
//
//////////////////////////////////////////////////////////////////////

#include "StHeader.h"
#include "stglobals.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "ddildl_defs.h"
#include "stconfiginfo.h"
#include "StProgress.h"
#include "StUpdater.h"
#include <ntddscsi.h>
//#include <ntdddisk.h>

#include <scsidefs.h>
#include <wnaspi32.h>

#include "StScsi.h"
#include "stscsi_nt.h"
#include "StFwComponent.h"
#include "StDataDrive.h"
#include "StSDisk.h"
#include "StFormatter.h"
#include "StSystemDrive.h"
#include "ddildl_defs.h"
#include "StDdiApi.h"
#include "StHiddenDataDrive.h"
#include "StUsbMscDev.h"
#include "math.h"

extern CStGlobals g_globals;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStUsbMscDev::CStUsbMscDev(CStUpdater *_p_updater, string _name):CStDevice(_p_updater, _name)
{
    UCHAR index_to_bootmanager=0;
	m_p_arr_system_drive = NULL;
	m_p_arr_data_drive = NULL;
	m_chip_id = 0;
    m_ROM_id = 0xFF;
    m_limited_vendor_cmd_support = FALSE;
	m_media_new = FALSE;
//	m_p_data_drive = new CStDataDrive;
//	m_p_data_drive->SetDriveIndex(0);
//	m_p_data_drive->SetTag(DRIVE_TAG_DATA);
	m_drive_letter = L'\0';
	
    CStGlobals::MakeMemoryZero((PUCHAR)&m_table, sizeof(m_table));

    GetUpdater()->GetConfigInfo()->GetBootyDriveIndex(index_to_bootmanager);

	UCHAR index, arrayIndex, tag;
    LOGICAL_DRIVE_TYPE driveType;

    // get total number of drives (system and data)
    GetUpdater()->GetConfigInfo()->GetNumDrives(m_num_drives);

    // data drives
    if( GetUpdater()->GetConfigInfo()->GetNumDataDrives(m_num_data_drives) == STERR_NONE )
	{
		m_p_arr_data_drive = new CStDataDrivePtrArray(m_num_data_drives);

		if( !m_p_arr_data_drive )
		{
			m_last_error = STERR_NO_MEMORY;
			return;
		}

        arrayIndex = 0;
		for(index=0; index<m_num_drives; index++)
		{
            GetUpdater()->GetConfigInfo()->GetDriveType(index, driveType);
            if (driveType == DriveTypeData)
            {
    			CStDataDrive* datadrive = *m_p_arr_data_drive->GetAt(arrayIndex++);
	    		datadrive->SetDriveIndex(index);

		    	GetUpdater()->GetConfigInfo()->GetDriveTag(index, tag);
			    datadrive->SetTag(tag);
            }
		}
	}

    //system drives
	if( GetUpdater()->GetConfigInfo()->GetNumSystemDrives(m_num_system_drives) == STERR_NONE )
	{
		m_p_arr_system_drive = new CStSystemDrivePtrArray(m_num_system_drives, GetUpdater()->GetLanguageId());

		if( !m_p_arr_system_drive )
		{
			m_last_error = STERR_NO_MEMORY;
			return;
		}
		for(UCHAR index=0; index<m_num_system_drives; index++)
		{
			UCHAR tag;
			CStSystemDrive* sysdrive = *m_p_arr_system_drive->GetAt(index);
			sysdrive->SetDriveIndex(index+index_to_bootmanager);

			GetUpdater()->GetConfigInfo()->GetDriveTag(index+index_to_bootmanager, tag);
			sysdrive->SetTag(tag);
		}
	}

	if( GetUpdater()->GetConfigInfo()->GetNumHiddenDataDrives(m_num_hidden_data_drives) == STERR_NONE )
	{
		m_p_arr_hidden_data_drive = new CStHiddenDataDrivePtrArray(m_num_hidden_data_drives,
                                                            GetUpdater()->GetLanguageId());

		if( !m_p_arr_hidden_data_drive )
		{
			m_last_error = STERR_NO_MEMORY;
			return;
		}
		for(UCHAR index=0; index<m_num_hidden_data_drives; index++)
		{
			UCHAR tag;
			CStHiddenDataDrive* sysdrive = *m_p_arr_hidden_data_drive->GetAt(index);
			sysdrive->SetDriveIndex(index+1);

			GetUpdater()->GetConfigInfo()->GetDriveTag(index+1, tag);
			sysdrive->SetTag(tag);
		}
	}

    m_last_error = CreateScsi();

	m_p_arr_ver_info_current = NULL;
	m_p_arr_ver_info_upgrade = NULL;
	m_format_pending = FALSE;
}

CStUsbMscDev::~CStUsbMscDev()
{
	Trash();

	DestroyScsi();

	if( m_p_scsi )
	{
		delete m_p_scsi;
	}

	if(m_p_arr_data_drive)
	{
		delete m_p_arr_data_drive;
	}

	if( m_p_arr_system_drive )
	{
		delete m_p_arr_system_drive;
	}

	if( m_p_arr_hidden_data_drive )
	{
		delete m_p_arr_hidden_data_drive;
	}
}

ST_ERROR CStUsbMscDev::Trash()
{
	if(m_p_arr_data_drive)
	{
		for(long index=0; index<m_num_data_drives; index++)
		{
			CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(index);
			datadrive->Trash();
		}
	}
	if(m_p_arr_system_drive)
	{
		for(long index=0; index<m_num_system_drives; index++)
		{
			CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(index);
			sysdrive->Trash();
		}
	}
	if(m_p_arr_hidden_data_drive)
	{
		for(long index=0; index<m_num_hidden_data_drives; index++)
		{
			CStHiddenDataDrive *sysdrive = *m_p_arr_hidden_data_drive->GetAt(index);
			sysdrive->Trash();
		}
	}
	if( m_p_arr_ver_info_current )
	{
		delete m_p_arr_ver_info_current;
		m_p_arr_ver_info_current = NULL;
	}
	if( m_p_arr_ver_info_upgrade )
	{
		delete m_p_arr_ver_info_upgrade;
		m_p_arr_ver_info_upgrade = NULL;
	}

	m_chip_id = 0;
	return STERR_NONE;
}

void CStUsbMscDev::DestroyScsi()
{
	if( m_p_scsi )
	{
		m_p_scsi->Close();
	}

	for(long count = 0; count < m_num_data_drives; count ++)
	{
		CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(count);
		datadrive->SetScsi( NULL );
	}

	for(long count = 0; count < m_num_system_drives; count ++)
	{
		CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(count);
		sysdrive->SetScsi( NULL );
	}
}

ST_ERROR CStUsbMscDev::CreateScsi()
{
	switch( CStGlobals::GetPlatform() )
	{
		case OS_98:
		case OS_ME:
			m_p_scsi = NULL;
			break;
		case OS_2K:
		case OS_XP:
        case OS_XP64:
        case OS_VISTA32:
		case OS_WINDOWS7:
			m_p_scsi = new CStScsi_Nt(GetUpdater());
			break;
		default:
			m_p_scsi = NULL;
			break;
	}

	if(!m_p_scsi)
	{
		return STERR_UNSUPPORTED_OPERATING_SYSTEM;
	}
	return m_p_scsi->GetLastError();
}

ST_ERROR CStUsbMscDev::Initialize(USHORT UpgradeOrNormal)
{
	ST_ERROR err=STERR_NONE;
	ST_ERROR err_get_alloc_table = STERR_NONE;
	ST_ERROR err_init_scsi = STERR_NONE;
	
	Trash(); //make a clean start
	
	m_media_new = FALSE;
	// initialize m_table
	CStGlobals::MakeMemoryZero((PUCHAR)&m_table, sizeof(m_table));

	// if not existing create the scsi and initialize it.
	if( !m_p_scsi )
	{
		m_last_error = err = CreateScsi();
		if( err != STERR_NONE )
		{
			return err;
		}
	}
	
	GetNumberOfDataDrivesPresent(UpgradeOrNormal);

	m_last_error = err_init_scsi = m_p_scsi->Initialize(0, UpgradeOrNormal);

    if (err_init_scsi == STERR_LIMITED_VENDOR_SUPPORT)
    {

        err_init_scsi = STERR_NONE;
        m_limited_vendor_cmd_support = TRUE;
    }
    else
    {
        m_limited_vendor_cmd_support = FALSE;
    }

	if( err_init_scsi == STERR_NONE )
	{
		UCHAR cfg_num_drives = 0;
		//
		// device is found and initialized, query all info from the device.
		//
		m_drive_letter = m_p_scsi->GetDriveLetter();


        if (!m_limited_vendor_cmd_support)  // these will fail with MSC hostlink; don't bother
        {
    		err = GetChipMajorRevId(m_chip_id);  // MSC hostlink at least has this
	    	if(err != STERR_NONE)
		    	return err;
/*
            m_part_id = m_ROM_id = 0;
	    	err = GetChipPartRevId(m_part_id);
		    if(err == STERR_NONE)
*/
    		    err = GetROMRevId(m_ROM_id);
    		//if(err != STERR_NONE)
	    		//return err;

        }

	    GetUpdater()->GetConfigInfo()->GetNumDrives(cfg_num_drives);

		err = InitializeAllocationTable();

        // fail if error or number of drives do not match
		if( err != STERR_NONE || !(m_table.wNumEntries > 4) ) 
		// sdk 2.5xx is the sdk with the smallest drive array size of 4 system drives, hence compare with 4.
		// If there are more than 4 table entries, the media is not new.
		{
			// tolerate GetAllocationTable failure.
			CStGlobals::MakeMemoryZero((PUCHAR)&m_table, sizeof(m_table));
			m_media_new = TRUE;
			err_get_alloc_table = STERR_MEDIA_STATE_UNINITIALIZED;
		}
	}

    m_device_mode = UpgradeOrNormal;  // save it here for later

	err = InitializeDataDrives(err_init_scsi);
	if(err != STERR_NONE)
		return err;

	err = InitializeSystemDrives(err_init_scsi);
	if(err != STERR_NONE)
		return err;

//    if ( !m_limited_vendor_cmd_support) // || GetUpdater()->GetConfigInfo()->ForceGetCurrentVersions() )
//    {
//        // 10-25-06 CLW - Shouldn't fail to find the device just 'cause we failed to get the current versions off
//        // the media.
        /* err =*/ InitializeVersions( err_init_scsi, m_media_new );
//    }
	
	if( err_init_scsi != STERR_NONE )
	{
		return err_init_scsi;
	}

	if( m_media_new )
	{
		return err_get_alloc_table;
	}
	
    return err;
}

ST_ERROR CStUsbMscDev::InitializeAllocationTable()
{
	GetUpdater()->GetLogger()->Log( _T("   Getting the Allocation Table.") );
	ST_ERROR err = GetAllocationTable(&m_table);

	CString str, longSize;
	if ( err == STERR_NONE )
	{
		TCHAR testBuff[512];
		str.Format(_T("    Number of drives: %d"), m_table.wNumEntries);
		for ( int i=0; i<m_table.wNumEntries; ++i )
		{
			_stprintf_s(testBuff, 512, _T("(%I64u)"), m_table.Entry[i].SizeInBytes);
			longSize = testBuff;
			str.AppendFormat( _T("\n    entry[%d] drvNo: %d, tag: %#02x, type: %d, size: %#x"), i, m_table.Entry[i].DriveNumber, m_table.Entry[i].Tag, m_table.Entry[i].Type, m_table.Entry[i].SizeInBytes);
			str += longSize;
		}
	}
	else
	{
		str.Format(_T("    FAILED 0x%x(%d)"), err, err);
	}
	GetUpdater()->GetLogger()->Log( str );


	return err;
}

/*
ST_ERROR CStUsbMscDev::InitializeDataDrive(ST_ERROR _err_init_scsi)
{
	ST_ERROR err = STERR_NONE;
	P_MEDIA_ALLOCATION_TABLE_ENTRY entry = NULL;

	for(USHORT index = 0; index < m_table.wNumEntries; index ++)
	{
		if( m_table.Entry[index].Type == DriveTypeData )
		{
			entry = &m_table.Entry[index];
			break;
		}
	}

	if( _err_init_scsi != STERR_NONE )
		err = m_p_data_drive->Initialize(NULL, GetUpdater(), entry, m_ROM_id);
	else
		err = m_p_data_drive->Initialize(m_p_scsi, GetUpdater(), entry, m_ROM_id);

	if(err != STERR_NONE)
		return err;

	return err;
}
*/
ST_ERROR CStUsbMscDev::InitializeDataDrives( ST_ERROR _err_init_scsi )
{
	ST_ERROR err = STERR_NONE;
	long index_entry_table=0; 

	for(long count = 0; count < m_num_data_drives; count ++)
	{
		CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(count);
		P_MEDIA_ALLOCATION_TABLE_ENTRY entry = NULL;
	
		for( index_entry_table = 0; index_entry_table < m_table.wNumEntries; index_entry_table ++ )
		{
			if( m_table.Entry[index_entry_table].Type == DriveTypeData && 
				m_table.Entry[index_entry_table].Tag == datadrive->GetTag() )
			{
				entry = &m_table.Entry[index_entry_table];
				break;
			}
		}

		if( _err_init_scsi != STERR_NONE )
			err = datadrive->Initialize(NULL, GetUpdater(), entry, m_ROM_id);
		else
			err = datadrive->Initialize(m_p_scsi, GetUpdater(), entry, m_ROM_id);

		if( err != STERR_NONE)
			return err;
	}

	return err;
}

ST_ERROR CStUsbMscDev::InitializeSystemDrives( ST_ERROR _err_init_scsi )
{
	ST_ERROR err = STERR_NONE;
	long index_entry_table=0;
	ULONGLONG size = 0, driveSize = 0; 
	CString str, longSize; TCHAR testBuff[512];
	USES_CONVERSION;

	GetUpdater()->GetLogger()->Log(_T("   Initializing system drives."));

	for(long count = 0; count < m_num_system_drives; count ++)
	{
		CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(count);
		P_MEDIA_ALLOCATION_TABLE_ENTRY entry = NULL;
	
		for( index_entry_table = 0; index_entry_table < m_table.wNumEntries; index_entry_table ++ )
		{
			if( m_table.Entry[index_entry_table].Type == DriveTypeSystem && 
				m_table.Entry[index_entry_table].Tag == sysdrive->GetTag() )
			{
				entry = &m_table.Entry[index_entry_table];
				break;
			}
		}

		if( _err_init_scsi != STERR_NONE )
			err = sysdrive->Initialize(NULL, GetUpdater(), entry, m_ROM_id);
		else
			err = sysdrive->Initialize(m_p_scsi, GetUpdater(), entry, m_ROM_id);

		if( err != STERR_NONE)
			return err;
		
		size = sysdrive->GetFwComponent()->GetSizeInBytes();
		str.Format(_T("    Drive - tag: 0x%02x, file: %s, size: 0x%x"), sysdrive->GetTag(), A2W(sysdrive->GetFwComponent()->GetFileName().c_str()), size);
		_stprintf_s(testBuff, 512, _T("(%I64u)"), size);
		longSize = testBuff;
		str += longSize;
		GetUpdater()->GetLogger()->Log(str);

		if ( driveSize == 0 )
		{
			driveSize = size;
		}
		else if ( driveSize != size )
		{
			GetUpdater()->GetLogger()->Log(_T("    ERROR!"));
//			ASSERT(0);
			err = STERR_FAILED_TO_READ_FILE_DATA;
			return err;
		}
	}

	for(long count = 0; count < m_num_hidden_data_drives; count ++)
	{
		CStHiddenDataDrive *sysdrive = *m_p_arr_hidden_data_drive->GetAt(count);
		P_MEDIA_ALLOCATION_TABLE_ENTRY entry = NULL;
	
		for( index_entry_table = 0; index_entry_table < m_table.wNumEntries; index_entry_table ++ )
		{
			if( m_table.Entry[index_entry_table].Type == DriveTypeHiddenData && 
				m_table.Entry[index_entry_table].Tag == sysdrive->GetTag() )
			{
				entry = &m_table.Entry[index_entry_table];
				break;
			}
		}

		if( _err_init_scsi != STERR_NONE )
			err = sysdrive->Initialize(NULL, GetUpdater(), entry, m_ROM_id);
		else
			err = sysdrive->Initialize(m_p_scsi, GetUpdater(), entry, m_ROM_id);

		if( err != STERR_NONE)
			return err;
	}

    return err;
}

ST_ERROR CStUsbMscDev::CheckForFwResources()
{ // Checks for f/w resources and opens them if not open
	ST_ERROR err = STERR_NONE;

	for(long count = 0; count < m_num_system_drives; count ++)
	{
		CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(count);

		err = sysdrive->Open();

		if ( err != STERR_NONE )
			break;
	}

	return err;

}
ST_ERROR CStUsbMscDev::InitializeVersions(ST_ERROR _err_init_scsi, BOOL _media_new)
{
	ST_ERROR err = STERR_NONE;
	CStSystemDrive* sysdrive=NULL;
	UCHAR index=0,player_index=0;

	if( m_p_arr_ver_info_current )
	{
		delete m_p_arr_ver_info_current;
		m_p_arr_ver_info_current = NULL;
	}
	if( m_p_arr_ver_info_upgrade )
	{
		delete m_p_arr_ver_info_upgrade;
		m_p_arr_ver_info_upgrade = NULL;
	}

	m_p_arr_ver_info_current = new CStVersionInfoPtrArray(m_num_system_drives);
	if( !m_p_arr_ver_info_current )
	{
		return STERR_NO_MEMORY;
	}

	m_p_arr_ver_info_upgrade = new CStVersionInfoPtrArray(m_num_system_drives);
	if( !m_p_arr_ver_info_upgrade )
	{
		delete m_p_arr_ver_info_current;
		m_p_arr_ver_info_current = NULL;
		return STERR_NO_MEMORY;
	}

	for( index=0; index<m_num_system_drives; index++)
	{
		CStVersionInfo* ver;

		ver = *m_p_arr_ver_info_current->GetAt(index); 

		sysdrive = *m_p_arr_system_drive->GetAt(index);
		if( !_media_new && ( sysdrive->GetDriveNumber() != CSTDRIVE_INVALID_DRIVE_NUMBER ) && 
			( _err_init_scsi == STERR_NONE ) )
		{
			err = sysdrive->GetCurrentComponentVersion(*ver);
		
			if( err != STERR_NONE )
			{
				//log and return
				return err;
			}
		}
		ver = *m_p_arr_ver_info_upgrade->GetAt(index); 

		err = sysdrive->GetUpgradeComponentVersion(*ver);
		if( err != STERR_NONE )
		{
			//log and return
			return err;
		}
	}

	if (GetUpdater()->GetConfigInfo()->GetPlayerDriveIndex(player_index) == STERR_INVALID_DRIVE_TYPE)
		GetUpdater()->GetConfigInfo()->GetBootyDriveIndex(player_index); // SDK5
	for( index=0; index<m_num_system_drives; index++)
	{
		sysdrive = *m_p_arr_system_drive->GetAt(index);
				
		if( player_index == sysdrive->GetDriveIndex() )
		{
			break;
		}
	}

	if( !_media_new && ( sysdrive->GetDriveNumber() != CSTDRIVE_INVALID_DRIVE_NUMBER ) && 
			( _err_init_scsi == STERR_NONE ) )
	{
		err = sysdrive->GetCurrentProjectVersion(m_ver_info_current_project);
		if( err != STERR_NONE )
		{
			//log and return
			return err;
		}
	}
	err = sysdrive->GetUpgradeProjectVersion(m_ver_info_upgrade_project);

	return err;
}

ST_ERROR CStUsbMscDev::VerifyDevicePresence()
{
	return m_p_scsi->Initialize(0, NORMAL_MSC);
}

ST_ERROR CStUsbMscDev::GetNumberOfDataDrivesPresent(USHORT UpgradeOrNormal)
{
	ST_ERROR err = STERR_NONE;
	m_num_data_drives_present = 0;

	for (USHORT i = 0; i < m_num_data_drives; ++i)
	{
		err = m_p_scsi->Initialize(i, UpgradeOrNormal);
		if ( err == STERR_NONE )
			++m_num_data_drives_present;
	}
	return STERR_NONE;
}



ST_ERROR CStUsbMscDev::IsEraseMediaRequired(BOOL& _erase_media_required)
{
	ST_ERROR 	err = STERR_NONE;
	MEDIA_ALLOCATION_TABLE table;

	_erase_media_required = FALSE;
	
	err = InitializeAllocationTable();
	if( err != STERR_NONE )
	{
		//tolerating getallocationtable failure.
		_erase_media_required = TRUE;
		return STERR_NONE;
//		return err;
	}

	BuildUpgradeTableFromDrives( table );

	if( !IsLessOrEqualToCurrentMediaTable( &table ) )
		_erase_media_required = TRUE;
	
	return err;
}

BOOL CStUsbMscDev::IsLessOrEqualToCurrentMediaTable(P_MEDIA_ALLOCATION_TABLE _p_table)
{
	USHORT num_drives_on_media = 0;
	BOOL found = FALSE;
	UCHAR cfg_num_drives = 0;
	USHORT index;

    GetUpdater()->GetConfigInfo()->GetNumDrives(cfg_num_drives);

	for( index=0; index<m_table.wNumEntries; index ++)
	{	
		if( m_table.Entry[index].Type == DriveTypeSystem || m_table.Entry[index].Type == DriveTypeData ||
            m_table.Entry[index].Type == DriveTypeHiddenData )
		{
			num_drives_on_media ++;
		}
	}
	
	if( _p_table->wNumEntries != num_drives_on_media )
		return FALSE;

    // data drives
	for(index = 0; index < m_num_data_drives; index ++)
	{
		CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(index);

		found = FALSE;

	    for(index=0; index<_p_table->wNumEntries; index ++)
	    {	
		    if( (_p_table->Entry[index].Type == DriveTypeData) && (_p_table->Entry[index].Tag == datadrive->GetTag()) )
		    {
			    found = TRUE;
//			    if( ! datadrive->IsLessOrEqualToCurrent(_p_table->Entry[index]) )
			if (  _p_table->Entry[index].SizeInBytes > datadrive->GetCurrentTableEntry()->SizeInBytes )
			    {
				    return FALSE;
			    }
			    break;
		    }
	    }

	    if( !found )
	    {
		    return FALSE;
	    }
	}
    // system drives
	for(index = 0; index < m_num_system_drives; index ++)
	{
		CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(index);

		found = FALSE;

		for(USHORT index_entry=0; index_entry<_p_table->wNumEntries; index_entry++)
		{	
			if( (_p_table->Entry[index_entry].Type == DriveTypeSystem) && (_p_table->Entry[index_entry].Tag == sysdrive->GetTag()) )
			{
				found = TRUE;
//				if( ! sysdrive->IsLessOrEqualToCurrent(_p_table->Entry[index_entry]) )
				if( sysdrive->GetFwComponent()->GetSizeInBytes() > sysdrive->GetCurrentTableEntry()->SizeInBytes )
				{
					return FALSE;
				}
				break;
			}
		}
		if( !found )
		{
			return FALSE;
		}
	}

	return TRUE;
}

void CStUsbMscDev::BuildUpgradeTableFromDrives(MEDIA_ALLOCATION_TABLE& _table)
{
   	long next_available_index=1;  // 0 is reserved for booty
    long index;

   	_table.wNumEntries = m_num_system_drives + m_num_hidden_data_drives+ m_num_data_drives;

    //
    // data drives
    //
    for(index = 0; index < m_num_data_drives; index ++)
	{
		CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(index);
        //last data drive goes at end of table unless there is only one
		if ( ( m_num_data_drives > 1 ) && ( index == (m_num_data_drives - 1)) )
    		datadrive->FillTableEntryToAllocate(&_table.Entry[_table.wNumEntries - 1]);
        else
    		datadrive->FillTableEntryToAllocate(&_table.Entry[next_available_index++]);
	}

    //
    // Hidden drive
    //
	for(long index = 0; index < m_num_hidden_data_drives; index ++)
	{
		CStHiddenDataDrive *sysdrive = *m_p_arr_hidden_data_drive->GetAt(index);
		sysdrive->FillTableEntryToAllocate(&_table.Entry[next_available_index++]);
	}

    //
    // system drives
    //
    for(long index = 0; index < m_num_system_drives; index ++)
	{
		CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(index);
		if(sysdrive->GetTag() == DRIVE_TAG_BOOTMANAGER_S )
		{
			//
			// booty always on the first index
			//
			sysdrive->FillTableEntryToAllocate(&_table.Entry[0]);
		}
		else
		{
			sysdrive->FillTableEntryToAllocate(&_table.Entry[next_available_index++]);
		}
	}
}

ST_ERROR CStUsbMscDev::ReadTableAndSetupDrivesToUpgrade()
{
	P_MEDIA_ALLOCATION_TABLE_ENTRY entry = NULL;
	ST_ERROR err = STERR_NONE;
	UCHAR index;

	err = InitializeAllocationTable();
	if( err != STERR_NONE )
	{
		return err;
	}
	//
	// update the newly allocated table into upgrade entries
	//

    // data drives
    for(index = 0; index < m_num_data_drives; index ++)
	{
		CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(index);
		
		entry = NULL;
		for( long index_entry_table=0; index_entry_table < m_table.wNumEntries; index_entry_table ++ )
		{
			if( m_table.Entry[index_entry_table].Type == DriveTypeData && 
				m_table.Entry[index_entry_table].Tag == datadrive->GetTag() )
			{
				entry = &m_table.Entry[index_entry_table];
				break;
			}
		}
		datadrive->SetUpgradeTableEntry(entry);
		datadrive->SetCurrentTableEntry(entry);
	}
    // system drives
	for(index = 0; index < m_num_hidden_data_drives; index ++)
	{
		CStHiddenDataDrive *sysdrive = *m_p_arr_hidden_data_drive->GetAt(index);
		
		entry = NULL;
		for( long index_entry_table=0; index_entry_table < m_table.wNumEntries; index_entry_table ++ )
		{
			if( m_table.Entry[index_entry_table].Type == DriveTypeHiddenData && 
				m_table.Entry[index_entry_table].Tag == sysdrive->GetTag() )
			{
				entry = &m_table.Entry[index_entry_table];
				break;
			}
		}
		sysdrive->SetUpgradeTableEntry(entry);
		sysdrive->SetCurrentTableEntry(entry);
	}

	for(index = 0; index < m_num_system_drives; index ++)
	{
		CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(index);
		
		entry = NULL;
		for( long index_entry_table=0; index_entry_table < m_table.wNumEntries; index_entry_table ++ )
		{
			if( m_table.Entry[index_entry_table].Type == DriveTypeSystem && 
				m_table.Entry[index_entry_table].Tag == sysdrive->GetTag() )
			{
				entry = &m_table.Entry[index_entry_table];
				break;
			}
		}
		sysdrive->SetUpgradeTableEntry(entry);
		sysdrive->SetCurrentTableEntry(entry);
	}
	return err;
}



ST_ERROR CStUsbMscDev::GetReadyToDownload()
{
	ST_ERROR err = STERR_NONE;

	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_INITIALIZE, 1);
	err = m_p_scsi->Open();
	GetUpdater()->GetProgress()->UpdateProgress();

	if( err != STERR_NONE )
		return err;
    // add one for re-opening the first drive
	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_LOCKING_DRIVES, m_num_data_drives_present+1);
	m_p_scsi->Close();

    for(UCHAR index = 0; index < m_num_data_drives_present; index ++)
	{

		CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(index);
        if ( ( DRIVE_TAG_DATA + (index * 0x10) ) != datadrive->GetTag() )
            continue;
		err = m_p_scsi->Initialize(index, m_device_mode);
    	if( err != STERR_NONE )
	    {
		    return err;
    	}

		err = m_p_scsi->Open();
    	if( err != STERR_NONE )
	    {
		    return err;
    	}

		err = m_p_scsi->Lock(m_media_new);
    	if( err != STERR_NONE )
	    {
		    return err;
    	}
    	m_p_scsi->Unlock(m_media_new);
    	m_p_scsi->Close();
    	GetUpdater()->GetProgress()->UpdateProgress();
	}
    // now re-open and lock the first drive
	m_p_scsi->Initialize(0, m_device_mode);
	m_p_scsi->Open();
	err = m_p_scsi->Lock(m_media_new);
	GetUpdater()->GetProgress()->UpdateProgress();
	if( err != STERR_NONE )
	{
		return err;
	}

	return err;
}


void CStUsbMscDev::CloseScsi()
{
	if (m_p_scsi)
	{
		m_p_scsi->Unlock(m_media_new);
		m_p_scsi->Close();
		DestroyScsi();
	}
}

ST_ERROR CStUsbMscDev::CleanupAfterDownload()
{
	ST_ERROR err = STERR_NONE;

	
	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_UNLOCKING_DRIVES, 1);
	err = m_p_scsi->Unlock(m_media_new);
	GetUpdater()->GetProgress()->UpdateProgress();
	if( err != STERR_NONE )
		return err;


	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_CLOSING_DEVICE, 1);
	err = m_p_scsi->Close();
	GetUpdater()->GetProgress()->UpdateProgress();

	DestroyScsi();

	return err;
}



ST_ERROR CStUsbMscDev::DownloadFirmware()
{
	ST_ERROR err = STERR_NONE;
	ULONG total_iterations = 0;
//    UCHAR index_to_bootmanager=0;

	//
	// the call to ReadTableAndSetupDrivesToUpgrade is moved from fulldownload(), 
	// this is important and should be called for both quick and full download
	// otherwise player or usbmsc may fail to work after a quick download.
	//
	
	err = ReadTableAndSetupDrivesToUpgrade();
	GetUpdater()->GetProgress()->UpdateProgress();
	if( err != STERR_NONE )
	{
		return err;
	}

	for(UCHAR index=0; index<m_num_system_drives; index++)
	{
		ULONG iterations=0;
		CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(index);

		GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_ERASE_SYSTEMDRIVE, 1, sysdrive->GetDriveIndex());

		err = sysdrive->EraseDrive();

		GetUpdater()->GetProgress()->UpdateProgress();
		
		if(err != STERR_NONE)
		{
			return err;		
		}

		err = sysdrive->Download( iterations );

		if(err != STERR_NONE)
		{
			return err;		
		}

		total_iterations += iterations;
	}

	InitializeVersions(STERR_NONE);

	if( err != STERR_NONE )
		return err;

	// verify download
	GetUpdater()->GetProgress()->SetCurrentTask( TASK_TYPE_VERIFY_QUICK, total_iterations );


	for(UCHAR index=0; index<m_num_system_drives; index++)
	{
		CStSystemDrive *sysdrive = *m_p_arr_system_drive->GetAt(index);

		err = sysdrive->VerifyDownload();

		if(err != STERR_NONE)
		{
			return err;		
		}
	}

	return err;
}

ST_ERROR CStUsbMscDev::FormatDataDrive()
{
    ST_ERROR err = STERR_NONE;
	CStDataDrive *datadrive = NULL;

	m_format_pending = FALSE;

    for(UCHAR index = 0; index < m_num_data_drives_present; index ++)
	{
		datadrive = *m_p_arr_data_drive->GetAt(index);
        if ( ( DRIVE_TAG_DATA + (index * 0x10) ) != datadrive->GetTag() )
            continue;

		if (index > 0)
        {   // drive is already opened and locked before calling into here  
    		m_p_scsi->Close();
            m_p_scsi->Initialize(index, m_device_mode);
		    m_p_scsi->Open();
		    m_p_scsi->Lock(m_media_new);
		}

        CStFormatter formatter( GetUpdater(), datadrive, m_p_scsi );

		err = formatter.ReadCurrentFATInformation();

		if( err != STERR_NONE )
		{
			return err;
		}

		GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_ERASE_DATADRIVE, 2);
		GetUpdater()->GetProgress()->UpdateProgress();
		err = datadrive->EraseDrive();
		GetUpdater()->GetProgress()->UpdateProgress();

		if( err != STERR_NONE )
			return err;

		CString str;
		str.Format(_T(" Formatting Volume %c, Data Drive[%d] on Logical Drive %d, tag: 0x%02x"), datadrive->GetDriveLetter(), index, datadrive->GetDriveNumber(), datadrive->GetTag());
		GetUpdater()->GetLogger()->Log(str);

		USES_CONVERSION;
		wstring volume_label;
		GetUpdater()->GetConfigInfo()->GetDefaultVolumeLabel(volume_label);
		UCHAR *szStr = (UCHAR *)W2A(volume_label.c_str());
	    err = formatter.FormatMedia( m_media_new, szStr, /*(UCHAR*)volume_label.data(),*/ (int)volume_label.length(), index );
		if ( err == STERR_NONE )
			GetUpdater()->GetLogger()->Log(_T("  PASS"));
		else
			GetUpdater()->GetLogger()->Log(_T("  FAIL"));

        if ( index < (m_num_data_drives_present-1) )
        {   // leave the last data drive locked for cleanup task
	        err = m_p_scsi->Unlock(m_media_new);
    	    err = m_p_scsi->Close();
        }

	}
	m_format_pending = FALSE;

    return err;
}

ST_ERROR CStUsbMscDev::FormatHiddenDrive()
{
    ST_ERROR err = STERR_NONE;
	ULONG iterations = 0;
	UCHAR versionMajor;

    for(UCHAR index=0; index<m_num_hidden_data_drives; index++)
    {
	    CStHiddenDataDrive *sysdrive = *m_p_arr_hidden_data_drive->GetAt(index);
		
		if (sysdrive->GetTag() == DRIVE_TAG_DATA_HIDDEN)
		{
			// skip formatting the hidden drive if DRM is not enabled for SDK5 and later
			GetUpdater()->GetProtocolVersionMajor(versionMajor);
			if (versionMajor >= ST_UPDATER_RAMLESS_JANUS &&
				!(GetUpdater()->GetConfigInfo()->GetFirmwareHeaderFlags() & FW_HEADER_FLAGS_DRM_ENABLED) )
				continue;
		}

        GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_ERASE_HIDDENDRIVE, 1, sysdrive->GetDriveIndex());

        err = sysdrive->EraseDrive();

        GetUpdater()->GetProgress()->UpdateProgress();
    		
        if(err != STERR_NONE)
        {
	        return err;		
        }

        err = sysdrive->Download( iterations );

        if(err != STERR_NONE)
        {
	        return err;		
        }
    }
    return err;
}

ST_ERROR CStUsbMscDev::ReadDrive(DWORD driveTag, CStByteArray* _p_arr)
{
	CStSystemDrive * pDrive = NULL;
	for(UCHAR index=0; index<m_num_system_drives; index++)
	{
		CStSystemDrive* sysdrive = *m_p_arr_system_drive->GetAt(index);
		if ( sysdrive->GetTag() == driveTag )
		{
			pDrive = sysdrive;
			break;
		}
	}

	if ( pDrive == NULL )
	{
		for(UCHAR index=0; index<m_num_hidden_data_drives; index++)
		{
			CStHiddenDataDrive* sysdrive = *m_p_arr_hidden_data_drive->GetAt(index);
			if ( sysdrive->GetTag() == driveTag )
			{
				pDrive = sysdrive;
				break;
			}
		}
	}

	if ( pDrive == NULL )
		return STERR_INVALID_DRIVE_TYPE;

	return pDrive->ReadDrive(_p_arr);
}

ST_ERROR CStUsbMscDev::WriteDrive(DWORD driveTag, CStByteArray* _p_arr)
{
	CStSystemDrive * pDrive = NULL;
	for(UCHAR index=0; index<m_num_system_drives; index++)
	{
		CStSystemDrive* sysdrive = *m_p_arr_system_drive->GetAt(index);
		if ( sysdrive->GetTag() == driveTag )
		{
			pDrive = sysdrive;
			break;
		}
	}

	if ( pDrive == NULL )
	{
		for(UCHAR index=0; index<m_num_hidden_data_drives; index++)
		{
			CStHiddenDataDrive* sysdrive = *m_p_arr_hidden_data_drive->GetAt(index);
			if ( sysdrive->GetTag() == driveTag )
			{
				pDrive = sysdrive;
				break;
			}
		}
	}

	if ( pDrive == NULL )
		return STERR_INVALID_DRIVE_TYPE;

	return pDrive->WriteDrive(_p_arr);
}

BOOL CStUsbMscDev::HasSettingsFile()
{
	BOOL retValue = FALSE;
	for(UCHAR index=0; index<m_num_hidden_data_drives; index++)
	{
		CStHiddenDataDrive* sysdrive = *m_p_arr_hidden_data_drive->GetAt(index);
		if ( sysdrive->GetTag() == DRIVE_TAG_DATA_SETTINGS )
		{
			CStFwComponent *pFw = sysdrive->GetFwComponent();
			if ( pFw->GetLastError() == STERR_NONE )
				retValue = TRUE;
			break;
		}
	}

	return retValue;
}

ULONGLONG CStUsbMscDev::GetDriveSize(DWORD driveTag)
{
	for(int index=0; index<m_num_drives; index++)
	{
		if ( m_table.Entry[index].Tag == driveTag )
			return m_table.Entry[index].SizeInBytes;
	}

	return 0;
}

ST_ERROR CStUsbMscDev::SetAllocationTable()
{
	MEDIA_ALLOCATION_TABLE table;
    ST_ERROR err;

	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_ALLOCATING_TABLES, 4);

	BuildUpgradeTableFromDrives(table);
	GetUpdater()->GetProgress()->UpdateProgress();

	CString str;
	str.Format(_T("  Number of drives: %d"), table.wNumEntries);
	for ( int i=0; i<table.wNumEntries; ++i )
	{
		str.AppendFormat( _T("\n  entry[%d] tag: 0x%02x, type: %d, size: 0x%x(%I64d)"), i, table.Entry[i].Tag, table.Entry[i].Type, table.Entry[i].SizeInBytes, table.Entry[i].SizeInBytes);
	}
	GetUpdater()->GetLogger()->Log( str );
	err = SetAllocationTable(table);
	GetUpdater()->GetProgress()->UpdateProgress();

	return err;
}

ST_ERROR CStUsbMscDev::GetNumberOfDrives(USHORT& _num_drives)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalMediaInfo api(MediaInfoNumberOfDrives);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;


	return api.GetNumberOfDrives(_num_drives);
}

ST_ERROR CStUsbMscDev::GetPhysicalMediaType(PHYSICAL_MEDIA_TYPE& _type)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalMediaInfo api(MediaInfoPhysicalMediaType);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetPhysicalMediaType(_type);
}

ST_ERROR CStUsbMscDev::IsMediaWriteProtected(ST_BOOLEAN& _write_protected)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalMediaInfo api(MediaInfoIsWriteProtected);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.IsWriteProtected(_write_protected);
}

ST_ERROR CStUsbMscDev::GetMediaSizeInBytes(ULONGLONG& _size)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalMediaInfo api(MediaInfoSizeInBytes);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetSizeInBytes(_size);
}

ST_ERROR CStUsbMscDev::GetMediaNandPageSizeInBytes(ULONG& _pagesize)
{
	ST_ERROR result=STERR_NONE;

	if (m_p_scsi->m_ProtocolVersion != ST_UPDATER_ADVANCED && 
		m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS_ADVANCED )
		return STERR_FUNCTION_NOT_SUPPORTED;

	CStGetLogicalMediaInfo api(MediaInfoNandPageSizeInBytes);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetNandPageSizeInBytes(_pagesize);
}

ST_ERROR CStUsbMscDev::GetMediaNandMfgId(ULONG& _nandMfgId)
{
	ST_ERROR result=STERR_NONE;

	if (m_p_scsi->m_ProtocolVersion != ST_UPDATER_ADVANCED && 
		m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS_ADVANCED )
		return STERR_FUNCTION_NOT_SUPPORTED;

	CStGetLogicalMediaInfo api(MediaInfoNandMfgId);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;
	return api.GetNandMfgId(_nandMfgId);
}

ST_ERROR CStUsbMscDev::GetMediaNandIdDetails(ULONGLONG& _nandIdDetails)
{
	ST_ERROR result=STERR_NONE;

	if (m_p_scsi->m_ProtocolVersion != ST_UPDATER_ADVANCED && 
		m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS_ADVANCED )
		return STERR_FUNCTION_NOT_SUPPORTED;

	CStGetLogicalMediaInfo api(MediaInfoNandIdDetails);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;
	return api.GetNandIdDetails(_nandIdDetails);
}

ST_ERROR CStUsbMscDev::GetMediaNandChipEnables(ULONG& _nandChipEnables)
{
	ST_ERROR result=STERR_NONE;

	if (m_p_scsi->m_ProtocolVersion != ST_UPDATER_ADVANCED && 
		m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS_ADVANCED )
		return STERR_FUNCTION_NOT_SUPPORTED;

	CStGetLogicalMediaInfo api(MediaInfoNandChipEnables);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;
	return api.GetNandChipEnables(_nandChipEnables);
}

ST_ERROR CStUsbMscDev::GetSizeOfSerialNumber(USHORT& _size)
{
	ST_ERROR result=STERR_NONE;

	if (m_p_scsi->m_ProtocolVersion != ST_UPDATER_ADVANCED && 
		m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS_ADVANCED )
		return STERR_FUNCTION_NOT_SUPPORTED;

	CStGetChipSerialNumber api(SerialNoInfoSizeOfSerialNumberInBytes);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;
	
	return api.GetSizeOfSerialNumber(_size);
}

ST_ERROR CStUsbMscDev::GetSerialNumber(CStByteArray * _arr)
{
	ST_ERROR result=STERR_NONE;

	if (m_p_scsi->m_ProtocolVersion != ST_UPDATER_ADVANCED && 
		m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS_ADVANCED )
		return STERR_FUNCTION_NOT_SUPPORTED;

	CStGetChipSerialNumber api(SerialNoInfoSerialNumber, _arr->GetCount());
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;
	return api.GetChipSerialNumber(_arr);
}

ST_ERROR CStUsbMscDev::EraseLogicalMedia(BOOL bPreserveHiddenDrive)
{
	ST_ERROR result;
	CStEraseLogicalMedia api;
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

    api.SetPreserveHiddenDrive(bPreserveHiddenDrive);
	result = m_p_scsi->SendDdiApiCommand(&api);
	if (result == STERR_NONE)
		m_media_new = TRUE;
	return result;
}



ST_ERROR CStUsbMscDev::GetChipMajorRevId(USHORT& _rev)
{
	ST_ERROR result=STERR_NONE;
	CStGetChipMajorRevId api;
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetChipMajorRevId(_rev);
}

ST_ERROR CStUsbMscDev::GetChipPartRevId(USHORT& _rev)
{
	ST_ERROR result=STERR_NONE;
	CStGetChipPartRevId api;
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetChipPartRevId(_rev);
}

ST_ERROR CStUsbMscDev::GetROMRevId(USHORT& _rev)
{
	ST_ERROR result=STERR_NONE;
	CStGetROMRevId api;
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetROMRevId(_rev);
}

ST_ERROR CStUsbMscDev::GetExternalRAMSizeInMB(ULONG& _RAMSize)
{
	if (m_p_scsi->m_ProtocolVersion != ST_UPDATER_ADVANCED && 
		m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS_ADVANCED )
		return STERR_FUNCTION_NOT_SUPPORTED;

	ST_ERROR result=STERR_NONE;
	CStGetDeviceProperties api(eDevicePhysicalExternalRamSz);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetDevicePropExternalRAMSize(_RAMSize);
}

ST_ERROR CStUsbMscDev::GetVirtualRAMSizeInMB(ULONG& _RAMSize)
{
	ST_ERROR result=STERR_NONE;

	if (m_p_scsi->m_ProtocolVersion != ST_UPDATER_ADVANCED && 
		m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS_ADVANCED )
		return STERR_FUNCTION_NOT_SUPPORTED;

	CStGetDeviceProperties api(eDeviceVirtualExternalRamSz);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;

	return api.GetDevicePropVirtualRAMSize(_RAMSize);
}

ST_ERROR CStUsbMscDev::GetAllocationTable(P_MEDIA_ALLOCATION_TABLE _p_table)
{
	ST_ERROR result=STERR_NONE;
	CStGetLogicalTable api;

	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	result = m_p_scsi->SendDdiApiCommand(&api);
	if(result != STERR_NONE)
		return result;
	
	return api.GetTable(*_p_table);
}

ST_ERROR CStUsbMscDev::SetAllocationTable(MEDIA_ALLOCATION_TABLE _table)
{
	CStAllocateLogicalMedia api(&_table);

	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	return m_p_scsi->SendDdiApiCommand(&api);
}

ST_ERROR CStUsbMscDev::GetCurrentComponentVersions(CStVersionInfoPtrArray** _p_arr_ver_info_current)
{
	//*_p_arr_ver_info_current = m_p_arr_ver_info_current;

	*_p_arr_ver_info_current = new CStVersionInfoPtrArray(m_num_system_drives);
	if( !*_p_arr_ver_info_current )
	{
		return STERR_NO_MEMORY;
	}

	for(UCHAR index=0; index<m_num_system_drives; index++)
	{
		CStVersionInfo* ver = *(*_p_arr_ver_info_current)->GetAt(index);

		*ver = *(*m_p_arr_ver_info_current->GetAt(index)); 
	}
	return STERR_NONE;
}

ST_ERROR CStUsbMscDev::GetCurrentProjectVersion(CStVersionInfo* _p_ver_info_current)
{
	*_p_ver_info_current = m_ver_info_current_project;
	return STERR_NONE;
}

ST_ERROR CStUsbMscDev::GetUpgradeComponentVersions(CStVersionInfoPtrArray** _p_arr_ver_info_upgrade)
{
	//*_p_arr_ver_info_upgrade = m_p_arr_ver_info_upgrade;

	*_p_arr_ver_info_upgrade = new CStVersionInfoPtrArray(m_num_system_drives);
	if( !*_p_arr_ver_info_upgrade )
	{
		return STERR_NO_MEMORY;
	}

	for( UCHAR index=0; index<m_num_system_drives; index++)
	{
		CStVersionInfo* ver = *(*_p_arr_ver_info_upgrade)->GetAt(index);

		*ver = *(*m_p_arr_ver_info_upgrade->GetAt(index)); 
	}

	return STERR_NONE;
}

ST_ERROR CStUsbMscDev::GetUpgradeProjectVersion(CStVersionInfo* _p_ver_info_current)
{
	*_p_ver_info_current = m_ver_info_upgrade_project;
	return STERR_NONE;
}

wchar_t	CStUsbMscDev::GetDriveLetter()
{
	return m_drive_letter;
}


ULONG CStUsbMscDev::GetTotalTasks(USHORT _operation)
{
	ULONG total_tasks = ( ( m_num_system_drives * 2 ) + 1 + 3); // download firmware
	UCHAR versionMajor;

	// skip formatting the hidden drive if DRM is not enabled for SDK5 and later
	GetUpdater()->GetProtocolVersionMajor(versionMajor);

//	total_tasks += 1; // init

    if ( _operation & UPDATE_INIT_DEVICE )  // download updater.sb
        total_tasks += 1;

    total_tasks += ( m_num_data_drives * 3 );  // close data drives

    if ( _operation & UPDATE_ERASE_MEDIA )
        total_tasks += 3;  //allocating table, erase data drive,

    if ( _operation & UPDATE_FORMAT_DATA )
	{
        total_tasks += (CStFormatter::GetTotalTasks() * m_num_data_drives_present );

		if ( GetUpdater()->GetConfigInfo()->GetPreferredFAT() == FAT_32 )
			total_tasks += 7 * m_num_data_drives_present;

		if ( m_p_scsi->m_ProtocolVersion >= ST_UPDATER_JANUS )
		{
			if (versionMajor >= ST_UPDATER_RAMLESS_JANUS &&
			!(GetUpdater()->GetConfigInfo()->GetFirmwareHeaderFlags() & FW_HEADER_FLAGS_DRM_ENABLED) )
				; // DRM not enabled
			else
			{
				total_tasks += 3; // format hidden, InitJanus, InitDataStore
			}
		}

		if ( _operation & UPDATE_SAVE_HDS )
		    total_tasks += 2;  // HDS save/restore

		if ( _operation & UPDATE_2DD_CONTENT )
			total_tasks += 1;
	}

	return total_tasks;
}
ST_ERROR CStUsbMscDev::DoJustFormat()
{
    ST_ERROR err = STERR_NONE;

    for(UCHAR index = 0; index < m_num_data_drives_present; index ++)
	{
		CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(index);
        if ( ( DRIVE_TAG_DATA + (index * 0x10) ) != datadrive->GetTag() )
            continue;

		if (index > 0)
        {   // drive is already opened and locked before calling into here 
    		m_p_scsi->Close();
	    	m_p_scsi->Initialize(index, m_device_mode);
		    m_p_scsi->Open();
		    m_p_scsi->Lock(m_media_new);
		}

        CStFormatter format(GetUpdater(), datadrive, m_p_scsi);
	    err = format.FormatMedia( FALSE, NULL, 0, index );

        if ( index < (m_num_data_drives_present-1) )
        {   // leave the last one locked for cleanup task
            m_p_scsi->Unlock(m_media_new);
        	m_p_scsi->Close();
        }

        if (err != STERR_NONE)
            break;
    }
    
    return err;
}

//
// Reset a device currently running in MSC mode to recovery mode
//
ST_ERROR CStUsbMscDev::ResetMSCDeviceForRecovery()
{
    ST_ERROR returnStatus;

	if ( m_p_scsi->m_ProtocolVersion >= ST_UPDATER_ADVANCED )
	{
		CStResetToRecovery api;
		if(api.GetLastError() != STERR_NONE)
			return api.GetLastError();

	    returnStatus = m_p_scsi->SendDdiApiCommand(&api);

		// don't try to close the handle later
	    if (returnStatus == STERR_NONE)
		    m_p_scsi->ClearHandle();
	}
	else
		returnStatus = STERR_FUNCTION_NOT_SUPPORTED;

	return returnStatus; 
}

//
// Reset a device currently running in MSC mode to recovery mode
//
ST_ERROR CStUsbMscDev::OldResetMSCDeviceForRecovery()
{
    ST_ERROR err;
	UCHAR index_to_bootmanager=0;
	CStSystemDrive *sysdrive;

	// returns 2
	// sysdrive gets player system drive
	// ????
	//GetUpdater()->GetConfigInfo()->GetBootyDriveIndex(index_to_bootmanager);

	sysdrive = *m_p_arr_system_drive->GetAt(index_to_bootmanager);

	err = sysdrive->EraseDrive();

    if (err == STERR_NONE)
	    ResetChip();

    // don't try to close the handle later
    m_p_scsi->ClearHandle();

    return err;
}

//
// Reset a device currently running in MSC mode to MSC Updater mode
//
ST_ERROR CStUsbMscDev::ResetMscDeviceToMscUpdaterMode()
{
    ST_ERROR returnStatus;

	if ( m_p_scsi->m_ProtocolVersion >= ST_UPDATER_ADVANCED )
	{
		CStResetToUpdater api;
		if(api.GetLastError() != STERR_NONE)
			return api.GetLastError();

	    returnStatus = m_p_scsi->SendDdiApiCommand(&api);

		// don't try to close the handle later
	    if (returnStatus == STERR_NONE)
		    m_p_scsi->ClearHandle();
	}
	else
		returnStatus = STERR_FUNCTION_NOT_SUPPORTED;

	return returnStatus; 
}

ST_ERROR CStUsbMscDev::ResetChip()
{
    ST_ERROR returnStatus;

	GetUpdater()->GetLogger()->Log(_T("Sending the ChipReset(0x31) command)."));

	CStChipReset api;
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

    returnStatus = m_p_scsi->SendDdiApiCommand(&api);

    // don't try to close the handle later
    if (returnStatus == STERR_NONE)
        m_p_scsi->ClearHandle();

	return returnStatus; 
}


BOOL CStUsbMscDev::AreVendorCmdsLimited()
{
    return m_limited_vendor_cmd_support;
}


ST_ERROR CStUsbMscDev::Transfer2DDContent()
{
    ST_ERROR err = STERR_NONE;
    wchar_t drvLetter;

    if (m_num_data_drives_present <= 1)
        return err;

    // what is the drive letter of the second data drive?
    // close out the existing scsi and then open the second drive
    m_p_scsi->Unlock(m_media_new);
    m_p_scsi->Close();

   	m_p_scsi->Initialize(1, m_device_mode);
    m_p_scsi->Open();
    err = m_p_scsi->Lock(m_media_new);
    m_p_scsi->Unlock(m_media_new);
    drvLetter = m_p_scsi->GetDriveLetter();
  	m_p_scsi->Close();

    if ( err != STERR_NONE )
        return STERR_FAILED_TO_LOCK_THE_DRIVE;

    if ( GetUpdater()->GetConfigInfo()->Recover2DDImage() )
    {
        CStDataDrive *datadrive = *m_p_arr_data_drive->GetAt(1);

        CStFormatter format(GetUpdater(), datadrive, m_p_scsi);
	    err = format.WriteRecover2DDImage();
    }
    else
    {
        wstring srcFilePath, destFilePath;
        wstring contentFolder;
        ULONG numItems = 0;
        wchar_t currentDir[MAX_PATH];

        GetUpdater()->GetConfigInfo()->ContentFolder(contentFolder);

        // find out how many files to copy
        _wgetcwd (currentDir, MAX_PATH);
        if ( wcslen(currentDir) > 3)  // is it NOT a root?
            wcscat_s (currentDir, L"\\");
        srcFilePath = currentDir + contentFolder;

		wsprintf (currentDir, L"%c:\\", drvLetter);

        destFilePath = currentDir;

        // get the count of items to copy
        numItems = CopyContents (srcFilePath, destFilePath, TRUE, TRUE, L"*.*", 0);

        // now set the range based on the number of files
  	    GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_TRANSFER_2DD_CONTENT, numItems);

        // copy the contents
        if (CopyContents (srcFilePath, destFilePath, FALSE, TRUE, L"*.*", 0) != numItems)
            err = STERR_FAILED_TO_WRITE_FILE_DATA;
        else
        {
            // open the drive, lock+unlock, and close
            m_p_scsi->Open();
            err = m_p_scsi->Lock(m_media_new);
            m_p_scsi->Unlock(m_media_new);
  	        m_p_scsi->Close();
        }
    }

    // leave with the first drive in the m_p_scsi; the
    // drive letter is used later to delete settings.dat
   	m_p_scsi->Initialize(0, m_device_mode);
    m_p_scsi->Open();
    m_p_scsi->Close();

    return err;
}


ST_ERROR CStUsbMscDev::SaveRestoreHDS(UCHAR operation)
{
    ST_ERROR err = STERR_NONE;
    wchar_t drvLetter;
    wstring srcFilePath, destFilePath;
    wstring contentFolder;
    ULONG numItems = 0;
    wchar_t currentDir[MAX_PATH] = {0};

	if ( m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS )
        return STERR_NONE;
    // add one for re-opening the first drive
	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_LOCKING_DRIVES, m_num_data_drives_present+1);

    // what is the drive letter of the data drive?
    // make sure we have a handle by closing out the existing scsi
    // and then open again
    m_p_scsi->Unlock(m_media_new);
    m_p_scsi->Close();

   	m_p_scsi->Initialize(0, m_device_mode);
    m_p_scsi->Open();
    err = m_p_scsi->Lock(m_media_new);  // make sure nothing else is using it
	if ( err != STERR_NONE )
		return err;
    m_p_scsi->Unlock(m_media_new);
    drvLetter = m_p_scsi->GetDriveLetter();
  	m_p_scsi->Close();

    // find out how many files to copy
	GetCurrentDirectoryW ( MAX_PATH, currentDir );


    if ( wcslen(currentDir) > 3)  // is it NOT a root?
    {
        wcscat_s (currentDir, L"\\");
    }


    if ( operation == SAVE_HDS )
    {
        wcscat_s (currentDir, L"HDS");
        destFilePath = currentDir;

        wsprintf (currentDir, L"%c:\\", drvLetter);
        srcFilePath = currentDir;

        // delete any current stuff leftover by error
        DeleteFiles ( destFilePath, L"*.hds" );
        // Remove the temp HDS folder
        RemoveDirectory( destFilePath.c_str() );

        // create a temp HDS folder
        if ( !CreateDirectory (destFilePath.c_str(), NULL) )
		{
//			WCHAR szMsg[MAX_PATH];
//			wsprintf(szMsg, L"SaveRestoreHDS() createdir failed: %s", destFilePath);
//			MessageBox(NULL, szMsg, L"test", MB_OK);
		}
  	    GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_SAVE_HDS, NUM_HDS_FILES);
    }
    else
    {
        wcscat_s (currentDir, L"HDS");
        srcFilePath = currentDir;

        wsprintf (currentDir, L"%c:\\", drvLetter);
        destFilePath = currentDir;

		GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_RESTORE_HDS, NUM_HDS_FILES);
    }


    // copy the contents
    numItems = CopyContents (srcFilePath, destFilePath, FALSE, TRUE, L"*.hds", FILE_ATTRIBUTE_HIDDEN);

    if ( operation == RESTORE_HDS )
    {
//	    if ( numItems != NUM_HDS_FILES )
//		    err = STERR_FAILED_TO_WRITE_FILE_DATA;

       // open the drive, lock+unlock, and close
       m_p_scsi->Open();
       err = m_p_scsi->Lock(m_media_new);
       m_p_scsi->Unlock(m_media_new);
  	   m_p_scsi->Close();

       // delete the saved files
       DeleteFiles ( srcFilePath, L"*.hds" );
       // Remove the temp HDS folder
       RemoveDirectory( srcFilePath.c_str() );
    }
   
    // leave with the first drive in the m_p_scsi; the
    // drive letter is used later to delete settings.dat
   	m_p_scsi->Initialize(0, m_device_mode);
    m_p_scsi->Open();
	if ( operation == RESTORE_HDS )
	{
		m_p_scsi->Dismount();
		m_p_scsi->Close();
	}
	else
	    err = m_p_scsi->Lock(m_media_new);  // make sure nothing else is using it

    return err;
}


unsigned CStUsbMscDev::DeleteFiles (wstring srcFolder, wstring szQualifier)
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    unsigned count = 0;
	wchar_t saveDir[MAX_PATH];

	GetCurrentDirectoryW ( MAX_PATH, saveDir );


    if ( !SetCurrentDirectoryW(srcFolder.c_str()) )
//	if ( _wchdir (srcFolder.c_str()) )
	{
//		WCHAR szMsg[64];
//		wsprintf(szMsg, L"DeleteFiles() chdir failed: %s", srcFolder);
//		MessageBox(NULL, szMsg, L"test", MB_OK);
		return 0;
	}


	hFind = FindFirstFile (szQualifier.c_str(), &FindData);

    if (hFind != INVALID_HANDLE_VALUE) 
    {
        do
        {
		    DWORD dwAttrs = GetFileAttributes(FindData.cFileName); 
            if ( dwAttrs & FILE_ATTRIBUTE_DIRECTORY )
				continue;

	        DeleteFile ( FindData.cFileName );
		    ++count;
        }
        while (FindNextFile (hFind, &FindData));

        FindClose(hFind);
    }


	_wchdir ( saveDir );

    return count;
}

unsigned CStUsbMscDev::CopyContents (wstring srcFolder,
                                     wstring destFolder,
                                     BOOL bCountOnly,
                                     BOOL bCopySubFolders,
                                     wstring szQualifier,
                                     DWORD dwAttrSpec)
{

    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    DWORD dwAttrs; 
    wchar_t destFileName[MAX_PATH];
    wchar_t srcFileName[MAX_PATH];
	wchar_t saveDir[MAX_PATH];
    wstring nextSrcFolder, nextDestFolder;
    unsigned count = 0;

	GetCurrentDirectoryW ( MAX_PATH, saveDir );

    _wchdir (srcFolder.c_str());

    hFind = FindFirstFile (szQualifier.c_str(), &FindData);

    if (hFind != INVALID_HANDLE_VALUE) 
    {
        do
        {
            if ( !wcscmp(FindData.cFileName, L".") || !wcscmp(FindData.cFileName, L"..") )
                continue;

            dwAttrs = GetFileAttributes(FindData.cFileName); 
            if ( (dwAttrs & FILE_ATTRIBUTE_DIRECTORY) && bCopySubFolders ) 
            { 
                    nextSrcFolder = srcFolder + L"\\" + FindData.cFileName;
	    			if (destFolder.length() == 3) // not a root?
		    			nextDestFolder = destFolder + FindData.cFileName;
			    	else
				    	nextDestFolder = destFolder + L"\\" + FindData.cFileName;
                    if (!bCountOnly)
                        CreateDirectory (nextDestFolder.c_str(), NULL);
                    count += CopyContents (nextSrcFolder, nextDestFolder, bCountOnly, bCopySubFolders, szQualifier, dwAttrSpec);
                    _wchdir (L".."); // back up
            }
			else
			{
                if ( dwAttrSpec && (dwAttrs & dwAttrSpec) )
                {  // everything we're interested in is hidden
	                if (!bCountOnly)
		            {
			            swprintf (srcFileName, MAX_PATH, L"%s\\%s", srcFolder.c_str(), FindData.cFileName);
    					if (destFolder.length() == 3) // not a root?
	    	                swprintf (destFileName, MAX_PATH, L"%s%s", destFolder.c_str(), FindData.cFileName);
		    			else
			    		    swprintf (destFileName, MAX_PATH, L"%s\\%s", destFolder.c_str(), FindData.cFileName);
			            CopyFile (srcFileName, destFileName, TRUE);
				        GetUpdater()->GetProgress()->UpdateProgress();
    	            }
	                ++count;
                }
			}
        }
        while (FindNextFile (hFind, &FindData));
    }
    FindClose(hFind);

	_wchdir ( saveDir );
    return count;
}


ST_ERROR CStUsbMscDev::GetDataDriveInfo(int _drivenum, ULONG& _sectors, USHORT& _secsize)
{
    ST_ERROR err = STERR_NONE;
	CStDataDrive *datadrive = NULL;
	READ_CAPACITY_DATA read_capacity;

    if ( _drivenum >= m_num_data_drives_present )
		return STERR_FAILED_TO_LOCATE_SCSI_DEVICE;

	datadrive = *m_p_arr_data_drive->GetAt(_drivenum);

    if ( ( DRIVE_TAG_DATA + (_drivenum * 0x10) ) != datadrive->GetTag() )
		return STERR_FAILED_TO_LOCATE_SCSI_DEVICE;

	err = datadrive->ReadCapacity(&read_capacity);
	if ( err != STERR_NONE )
		return err;

	//
	// read_capacity returns zero based last sector, 
	// add 1 for total number of sectors
	//
	_sectors = read_capacity.LogicalBlockAddress+1;
	_secsize = (USHORT)read_capacity.BytesPerBlock;

	return STERR_NONE;
}

UCHAR CStUsbMscDev::GetProtocolVersion()
{
	if (m_p_scsi)
		return m_p_scsi->m_ProtocolVersion;
	return 0;
}

UCHAR CStUsbMscDev::GetProtocolVersionMajor()
{
	if (m_p_scsi)
		return m_p_scsi->m_ProtocolVersionMajor;
	return 0;
}

UCHAR CStUsbMscDev::GetProtocolVersionMinor()
{
	if (m_p_scsi)
		return m_p_scsi->m_ProtocolVersionMinor;
	return 0;
}

ST_ERROR CStUsbMscDev::GetJanusStatus(UCHAR& _status)
{
    ST_ERROR returnStatus;

	if (m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS ||
		m_p_scsi->m_ProtocolVersion >= ST_UPDATER_RAMLESS_JANUS)
	{
		_status = JANUS_OK;
		return STERR_NONE;
	}

	CStGetJanusStatus api;
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

    returnStatus = m_p_scsi->SendDdiApiCommand(&api);

    if ( returnStatus == STERR_NONE )
        api.GetJanusStatus( _status );

	return returnStatus; 
}



ST_ERROR CStUsbMscDev::JanusInitialization()
{
	ST_ERROR	ret = STERR_NONE;
	int			loop_count = 0;
	DWORD		dwThreadId=0;
	HANDLE		thread_handle=INVALID_HANDLE_VALUE;
	DWORD		wait_result;

	if (m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS )
	{
		return STERR_NONE;
	}

    GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_WAIT_FOR_INIT, 10);

    GetUpdater()->GetProgress()->UpdateProgress();

	thread_handle = CreateThread(
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		CStUsbMscDev::JanusInitThread,      // thread function 
		this,						 // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);  
	
	if (thread_handle)
	{
		wait_result = CStGlobals::WaitForSingleObjectEx( thread_handle, 0, FALSE );
		while( wait_result != WAIT_OBJECT_0 )
		{	
	        GetUpdater()->GetProgress()->UpdateProgress();

			++loop_count;
			wait_result = CStGlobals::WaitForSingleObjectEx( thread_handle, 1000, FALSE );
		}	

		CloseHandle(thread_handle);
	}
	return ret;
}

DWORD WINAPI CStUsbMscDev::JanusInitThread(LPVOID pParam)
{
	CStUsbMscDev* _p_MscDev = (CStUsbMscDev*)pParam;

	CStInitializeJanus api;

	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	_p_MscDev->m_p_scsi->SendDdiApiCommand(&api);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	return 0;
}



ST_ERROR CStUsbMscDev::DataStoreInitialization()
{
	ST_ERROR	ret = STERR_NONE;
	int			loop_count = 0;
	DWORD		dwThreadId=0;
	HANDLE		thread_handle=INVALID_HANDLE_VALUE;
	DWORD		wait_result;

	if (m_p_scsi->m_ProtocolVersion < ST_UPDATER_JANUS )
	{
		return STERR_NONE;
	}

    GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_WAIT_FOR_INIT, 10);

    GetUpdater()->GetProgress()->UpdateProgress();

	thread_handle = CreateThread(
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		CStUsbMscDev::DataStoreInitThread,      // thread function 
		this,						 // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);  
	
	if (thread_handle)
	{
		wait_result = CStGlobals::WaitForSingleObjectEx( thread_handle, 0, FALSE );
		while( wait_result != WAIT_OBJECT_0 )
		{	
	        GetUpdater()->GetProgress()->UpdateProgress();

			++loop_count;
			wait_result = CStGlobals::WaitForSingleObjectEx( thread_handle, 1000, FALSE );
		}	

		CloseHandle(thread_handle);
	}
	return ret;
}

DWORD WINAPI CStUsbMscDev::DataStoreInitThread(LPVOID pParam)
{
	CStUsbMscDev* _p_MscDev = (CStUsbMscDev*)pParam;

	CStInitializeDataStore api(0);

	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	_p_MscDev->m_p_scsi->SendDdiApiCommand(&api);
	if(api.GetLastError() != STERR_NONE)
		return api.GetLastError();

	return 0;
}

ST_ERROR CStUsbMscDev::GetUpdaterDriveVersion(CStVersionInfo& updaterVersion)
{
    ST_ERROR returnStatus;

	// First get the allocation table from the media.
	CStGetLogicalTable api1;
	if(api1.GetLastError() != STERR_NONE)
		return api1.GetLastError();

	returnStatus = m_p_scsi->SendDdiApiCommand(&api1);
	if(returnStatus != STERR_NONE)
		return returnStatus;

	MEDIA_ALLOCATION_TABLE driveArray;
	api1.GetTable(driveArray);

	// Second, see if there is an updater.sb file on the NAND
	UCHAR driveNumber = 0xFF;
	int index;
	for ( index=0; index < driveArray.wNumEntries; ++index )
	{
		if ( driveArray.Entry[index].Tag == DRIVE_TAG_UPDATER_NAND_S )
		{
			driveNumber = driveArray.Entry[index].DriveNumber;
			break;
		}
	}

	if ( driveNumber == 0xFF )
	{
		return STERR_FAILED_TO_GET_DEVICE_INFO_SET;
	}

	// Third, get the version of the udapter.sb file on the nand.
	CStGetLogicalDriveInfo api2(driveNumber, DriveInfoComponentVersion, 0);
	if(api2.GetLastError() != STERR_NONE)
		return api2.GetLastError();

	returnStatus = m_p_scsi->SendDdiApiCommand(&api2);
	if(returnStatus != STERR_NONE)
		return returnStatus;

	return api2.GetComponentVersion(updaterVersion);
}

