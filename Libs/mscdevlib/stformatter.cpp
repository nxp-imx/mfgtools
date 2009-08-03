// StFormatter.cpp: implementation of the CStFormatter class.
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
#include "stdatadrive.h"
#include "StProgress.h"
#include "StFormatter.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStFormatter::CStFormatter(CStUpdater* _p_updater, CStDataDrive* _p_data_drive, CStScsi* _p_scsi,
						   string _name):CStBase(_name)
{
	m_p_updater = _p_updater;
	m_p_data_drive = _p_data_drive;
	m_p_scsi = _p_scsi;
	m_sector_size = 0;
	m_platform = CStGlobals::GetPlatform();
}

CStFormatter::~CStFormatter()
{
}

ST_ERROR CStFormatter::ReadCurrentFATInformation()
{
	// save any FAT information before EraseMedia is called.
	return STERR_NONE;
}


ST_ERROR CStFormatter::FormatMedia(BOOL _media_new, UCHAR* _volume_label, int _volume_label_length, USHORT driveToFind )
{
	ST_ERROR err					= STERR_NONE;
	ULONG drive_size_in_sectors		= 0;
	ULONG start_from_sector			= 0;
	READ_CAPACITY_DATA read_capacity;

	//
	// read data_area size in sectors
	//
	err = m_p_data_drive->ReadCapacity(&read_capacity);
	m_p_updater->GetProgress()->UpdateProgress();
	if( err != STERR_NONE )
		return err;

	//
	// read_capacity returns zero based last sector, 
	// add 1 for total number of sectors
	//
	drive_size_in_sectors = read_capacity.LogicalBlockAddress+1;
	m_sector_size = read_capacity.BytesPerBlock;

	//
	// Inititalize sdisk, this will calculate the entire FAT information based on number of sectors
	// and sector_size.
	//
	CStSDisk sdisk( (ULONG)drive_size_in_sectors, m_sector_size, _volume_label, _volume_label_length,
        m_p_updater->GetConfigInfo()->GetPreferredFAT() );
	m_p_updater->GetProgress()->UpdateProgress();
	if( sdisk.GetLastError() != STERR_NONE )
		return sdisk.GetLastError();

	//
	// for XP/2K, to write partition table(MBR) we need to open handle to the physical drive. 
	// use WriteFile and ReadFile APIs as suggested by Microsoft for XP/2K. 
	// for 98/ME, we still use SCSI_PASS_THROUGH using VWIN32 APIs that work quite well for physical volume.
	// the first sector will be the physical zero sector, mostly where MBR is located.
	// unlock and close logical handle first then open physical and lock it.
	//
	if( m_platform != OS_98 && m_platform != OS_ME)
	{
		DISK_GEOMETRY dg;

		err = m_p_scsi->Unlock(FALSE);
		if( err != STERR_NONE )
			return err;
		
		err = m_p_scsi->Close();
		if( err != STERR_NONE )
			return err;

		err = m_p_scsi->OpenPhysicalDrive( driveToFind, m_p_updater->m_DeviceMode == UpdaterDeviceMode ? UPGRADE_MSC : NORMAL_MSC );
		if( err != STERR_NONE )
			return err;

		err = m_p_scsi->Lock(FALSE);
		if( err != STERR_NONE )
			return err;

		err = m_p_scsi->ReadGeometry( &dg );
		if( err != STERR_NONE )
			return err;

		//
		// send ioctl_disk_set_drive_layout with correct partition table information. only required for XP/2K
		// ioctl will cause the disk to be updated with new MBR and also indicate to the operating system 
		// the change in the disk layout that will cause Win2k to update its explorer windows with the new partition
		// information.
		//
		err = m_p_scsi->FormatPartition( &dg, sdisk.GetPartitionBootSector()->BPB_HiddSec );
		if( err != STERR_NONE )
			return err;
	}
	else
	{
		//
		// for 98/ME, we need to acquire format lock in addition to the usual lock.
		//
		err = m_p_scsi->AcquireFormatLock(_media_new);
		if( !_media_new && ( err != STERR_NONE ) )
			return err;
	}

	//
	// for XP/2K, this will overwrite the partition table created using ioctl_disk_set_drive_layout with 
	// MBR calculated using sdisk.
	// for 98/ME, this will write the partition table at zero location.
	//
	err = FormatMBR( sdisk.GetMasterBootRecord(), start_from_sector );
	if( err != STERR_NONE )
		return err;

	//
	// zero the sectors between MBR and PBS
	//
	err = FormatHiddenSectors( sdisk.GetPartitionBootSector()->BPB_HiddSec - 1, start_from_sector );
	if( err != STERR_NONE )
		return err;

	if( m_platform != OS_98 && m_platform != OS_ME )
	{
		//
		// Now that the physical handle is available, calculate the wasted sectors offset after first partition
		//
//		ULONG from_sector = sdisk.GetPartitionBootSector()->BPB_HiddSec + 
//			sdisk.GetMasterBootRecord()->Partitions[0].SectorCount;

		//
		// format the wasted sectors
		//
		/// CLW - Don't screw with the Wasted sectors on the data drive since 
		//		we probably do the calculation wrong and Vista calls us on it. 
		//		Nobody should be accessing the unused sectors at the end of the data drive anyway.
		//
//		err = FormatHiddenSectors(  sdisk.GetWastedSectors(), from_sector );
//		if( err != STERR_NONE )
//			return err;
		
	//
	// for XP/2K/Vista, done with formatting the physical volume with partition table information, unlock and close the 
	// physical volume handle and open the logical volume that will now point to the first sector of the first 
	// partition that is PBS.  For Vista, write the PBS using the physical handle; otherwise, it will fail 
	// writing the PBS with the logical handle.
	//
		if ( m_platform == OS_XP || m_platform == OS_2K )
		{
			err = m_p_scsi->Unlock(FALSE);
			if( err != STERR_NONE )
				return err;

			err = m_p_scsi->Close();
			if( err != STERR_NONE )
				return err;

			err = m_p_scsi->Open();
			if( err != STERR_NONE )
				return err;

			err = m_p_scsi->Lock(FALSE);
			if( err != STERR_NONE )
				return err;

			//
			// for XP/2K, initialize start_from_sector to zero. PBS is now at zero sector of logical volume
			//
			start_from_sector = 0;
		}
	}

	m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_FORMATTING_DATADRIVE, 2 );

	//
	// format partition boot sector
	// 
	// 5.21.2007 - In Vista, PBS was failing to get written every other Format in Release builds. Failure was
	// not seen in Debug builds. Failure seemed to happen in WriteFile() called from CStScsi_Nt::WriteSector
	// with an "Access denied" system error. Adding the below Sleep(100) seems to "fix" it for some reason. ~clw~
	Sleep(100);
	//
	err = FormatPBS( sdisk.GetPartitionBootSector(), start_from_sector );
	if( err != STERR_NONE )
		return err;

	//
	// Now close the physical handle for Vista and get the logical drive handle
	//
	if ( m_platform == OS_VISTA32 || m_platform == OS_VISTA64 || OS_WINDOWS7)
	{
		err = m_p_scsi->Unlock(FALSE);
		if( err != STERR_NONE )
			return err;

		err = m_p_scsi->Close();
		if( err != STERR_NONE )
			return err;

		err = m_p_scsi->Open();
		if( err != STERR_NONE )
			return err;

		err = m_p_scsi->Lock(FALSE);
		if( err != STERR_NONE )
			return err;

		//
		// We just wrote the PBS at sector zero of logical volume, so bump by one.
		//
		start_from_sector = 1;
	}

	if( sdisk.GetFileSystem() == FAT_32 )
	{
		//
		// FAT32 has 3 partition boot sectors. write the other 2 sectors.
		// 
		err = FormatPBS( sdisk.GetPartitionBootSector2(), start_from_sector );
		if( err != STERR_NONE )
			return err;

		err = FormatPBS( sdisk.GetPartitionBootSector3(), start_from_sector );
		if( err != STERR_NONE )
			return err;

		//
		// format the next 3 sectors to zero
		// 
		err = FormatHiddenSectors( 3, start_from_sector );
		if( err != STERR_NONE )
			return err;
		
		//
		// FAT32 has a backup copy of 3 Partition Boot Sectors starting at 6th sector.
		// 
		err = FormatPBS( sdisk.GetPartitionBootSector(), start_from_sector );
		if( err != STERR_NONE )
			return err;

		err = FormatPBS( sdisk.GetPartitionBootSector2(), start_from_sector );
		if( err != STERR_NONE )
			return err;

		err = FormatPBS( sdisk.GetPartitionBootSector3(), start_from_sector );
		if( err != STERR_NONE )
			return err;

		//
		// format the remaining reserved sectors
		// 
		err = FormatHiddenSectors( sdisk.GetPartitionBootSector()->BPB_RsvdSecCnt - 9, start_from_sector );
		if( err != STERR_NONE )
			return err;
	}

	//
	// format the two fat area, FAT_AREA1 and FAT_AREA2 starting next to reserved sectors of partition boot sector.
	//
	err = FormatFATArea( &sdisk, start_from_sector );
	if( err != STERR_NONE )
		return err;
	
	//
	// format the directory structure with volume label
	//
	err = FormatDirectoryStructure( sdisk.GetNumDirectoryEntriesInSectors(), start_from_sector, 
		_volume_label, _volume_label_length, sdisk.GetFileSystem() );
	if( err != STERR_NONE )
		return err;

	//
	// finally for 98/ME, format the rest of the wasted sectors outside partition 1.
	// already did this for 2K/XP when physical volume was available.
	//
	if( m_platform == OS_98 || m_platform == OS_ME )
	{
		//
		// calculate the wasted sectors offset after first partition
		//
		ULONG from_sector = sdisk.GetPartitionBootSector()->BPB_HiddSec + 
			sdisk.GetMasterBootRecord()->Partitions[0].SectorCount;

		//
		// zero the wasted sectors
		//
		err = FormatHiddenSectors(  sdisk.GetWastedSectors(), from_sector );
		if( err != STERR_NONE )
			return err;
	}
	//
	// release format lock that was obtained for 98/ME. 
	// for XP/2K, this will result in dismounting the volume that will cause the drive 
	// contents to get refreshed in operating system cache
	// 
	err = m_p_scsi->ReleaseFormatLock(_media_new);
	if( !_media_new && ( err != STERR_NONE ) )
		return err;		

	return STERR_NONE;
}

ST_ERROR CStFormatter::FormatSectors( CStByteArray& _sectors, ULONG& _start_from_sector )
{
	ST_ERROR err = STERR_NONE;
	CStByteArray sector_read( _sectors.GetCount() );

	err = m_p_data_drive->WriteSector( &_sectors, (ULONG)( _sectors.GetCount() / m_sector_size ), 
		_start_from_sector, m_sector_size );
	if( err != STERR_NONE )
		return err;

	m_p_updater->GetProgress()->UpdateProgress();

	err = m_p_data_drive->ReadSector( &sector_read, (ULONG)( _sectors.GetCount() / m_sector_size ),
		_start_from_sector, m_sector_size );
	if( err != STERR_NONE )
		return err;

	m_p_updater->GetProgress()->UpdateProgress();

	if( sector_read != _sectors )
	{
		return STERR_FAILED_READ_BACK_VERIFY_TEST;
	}
	return err;
}

ST_ERROR CStFormatter::FormatMBR( PPARTITION_TABLE _p_part_table, ULONG& _start_from_sector )
{
	CStByteArray sector( m_sector_size );

	m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_FORMATTING_MBR, 2 );
	
	sector.Write((void*) _p_part_table, sizeof( PARTITION_TABLE ), 0 );

	ST_ERROR err = FormatSectors( sector, _start_from_sector );
	if( err != STERR_NONE )
		return err;

	_start_from_sector ++;
	return STERR_NONE;
}

ST_ERROR CStFormatter::FormatHiddenSectors( ULONG _hidden_sectors, ULONG& _start_from_sector )
{
	ST_ERROR err = STERR_NONE;
	ULONG sector_count=0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;

	sector_count = _hidden_sectors;

	sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}

	number_of_iterations = sector_count / sectors_per_write;
	sectors_left_for_last_iteration = sector_count % sectors_per_write;
	
	m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_FORMATTING_HIDDEN_SECTORS, (number_of_iterations + 1)*2 );

	for(ULONG loop = 0; loop < number_of_iterations; loop ++)
	{
	    CStByteArray sector( sectors_per_write * m_sector_size );

		err = FormatSectors( sector, _start_from_sector );
		if( err != STERR_NONE )
			return err;

		_start_from_sector += sectors_per_write;
	}

	if( sectors_left_for_last_iteration )
	{
		CStByteArray sector( sectors_left_for_last_iteration * m_sector_size );

		err = FormatSectors( sector, _start_from_sector );
		if( err != STERR_NONE )
			return err;

		_start_from_sector += sectors_left_for_last_iteration;
	}

	return err;
}

ST_ERROR CStFormatter::FormatPBS( PBOOT_SECTOR _p_boot_sector, ULONG& _start_from_sector)
{
	CStByteArray sector( m_sector_size );

	m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_FORMATTING_PBS, 2 );
	
	sector.Write((void*) _p_boot_sector, sizeof( BOOT_SECTOR ), 0 );
	
	ST_ERROR err = FormatSectors( sector, _start_from_sector );
	if( err != STERR_NONE )
		return err;

	_start_from_sector ++;
	return STERR_NONE;
}

ST_ERROR CStFormatter::FormatPBS( PBOOT_SECTOR2 _p_boot_sector, ULONG& _start_from_sector)
{
	CStByteArray sector( m_sector_size );

	m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_FORMATTING_PBS, 2 );
	
	sector.Write((void*) _p_boot_sector, sizeof( BOOT_SECTOR ), 0 );

	ST_ERROR err = FormatSectors( sector, _start_from_sector );
	if( err != STERR_NONE )
		return err;

	_start_from_sector ++;
	return STERR_NONE;
}

ST_ERROR CStFormatter::FormatPBS( PBOOT_SECTOR3 _p_boot_sector, ULONG& _start_from_sector)
{
	CStByteArray sector( m_sector_size );

	m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_FORMATTING_PBS, 2 );

	sector.Write((void*) _p_boot_sector, sizeof( BOOT_SECTOR ), 0 );

	ST_ERROR err = FormatSectors( sector, _start_from_sector );
	if( err != STERR_NONE )
		return err;

	_start_from_sector ++;
	return STERR_NONE;
}

ST_ERROR CStFormatter::FormatFATArea( CStSDisk* _p_sdisk, ULONG& _start_from_sector )
{
	ST_ERROR err = STERR_NONE;
	ULONG sector_count=0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;

	if( _p_sdisk->GetFileSystem() == FAT_32 )
	{
		sector_count = _p_sdisk->GetPartitionBootSector()->Fat32.BPB_FATSz32;
	}
	else
	{
		sector_count = _p_sdisk->GetPartitionBootSector()->BPB_FATSz16;
	}

	sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}

	number_of_iterations = sector_count / sectors_per_write;
	sectors_left_for_last_iteration = sector_count % sectors_per_write;

	for(UCHAR fat = 1; fat <= _p_sdisk->GetPartitionBootSector()->BPB_NumFATs; fat ++ )
	{
		m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_FORMATTING_FAT_AREA, 
			(number_of_iterations + 1)*2, fat );
		
		for(ULONG loop = 0; loop < number_of_iterations; loop ++ )
		{
			CStByteArray sector( sectors_per_write * m_sector_size );
		
			if( loop == 0 )
			{
				//
				// only the first sectors of FAT 1 and 2 are non-zeroes.
				//
				_p_sdisk->GetFirstFatSector( &sector );
			}
			
			err = FormatSectors( sector, _start_from_sector );
			if( err != STERR_NONE )
				return err;

			_start_from_sector += sectors_per_write;
		}

		if( sectors_left_for_last_iteration )
		{
			CStByteArray sector( sectors_left_for_last_iteration * m_sector_size );
		
			if( !number_of_iterations )
			{
				//
				// only the first sectors of FAT 1 and 2 are non-zeroes.
				//
				_p_sdisk->GetFirstFatSector( &sector );
			}

			err = FormatSectors( sector, _start_from_sector );
			if( err != STERR_NONE )
				return err;

			_start_from_sector += sectors_left_for_last_iteration;
		}
	}

	return err;
}
	
ST_ERROR CStFormatter::FormatDirectoryStructure( ULONG _dir_entries, ULONG& _start_from_sector,
											UCHAR* _volume_label, int _volume_label_length, USHORT /*_filesystem*/ )
{
	ST_ERROR err = STERR_NONE;
	ULONG sector_count=0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;

	sector_count = _dir_entries;

	sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}

	number_of_iterations = sector_count / sectors_per_write;
	sectors_left_for_last_iteration = sector_count % sectors_per_write;

	m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_FORMATTING_DIRECTORY_STRUCTURE, 
		(number_of_iterations + 1) * 2 );

	for(ULONG loop = 0; loop < number_of_iterations; loop ++)
	{
	    CStByteArray sector( sectors_per_write * m_sector_size );

		if( loop == 0 )
		{
			sector.Write(_volume_label, _volume_label_length, 0);
			sector.SetAt(ROOT_DIR_ATTRIB_OFFSET, (UCHAR)ATTR_VOLUME_ID);
		}

		err = FormatSectors( sector, _start_from_sector );
		if( err != STERR_NONE )
			return err;

		_start_from_sector += sectors_per_write;
	}

	if( sectors_left_for_last_iteration )
	{
	    CStByteArray sector( sectors_left_for_last_iteration * m_sector_size );

		if( number_of_iterations == 0 )
		{
			sector.Write(_volume_label, _volume_label_length, 0);
			sector.SetAt(ROOT_DIR_ATTRIB_OFFSET, (UCHAR)ATTR_VOLUME_ID);
		}
	
		err = FormatSectors( sector, _start_from_sector );
		if( err != STERR_NONE )
			return err;
		
		_start_from_sector += sectors_left_for_last_iteration;
	}
	return err;
}

ST_ERROR CStFormatter::GetVolumeLabel( UCHAR* _volume_label, int _volume_label_length, int& size_of_actual_label )
{
	ST_ERROR						err						= STERR_NONE;
	ULONG							drive_size_in_sectors	= 0;
	READ_CAPACITY_DATA				read_capacity;
	BOOL							label_found				= FALSE;	
	ULONG							sector_count			= 0;

	size_of_actual_label = 0;
	err = m_p_data_drive->ReadCapacity(&read_capacity);
	m_p_updater->GetProgress()->UpdateProgress();
	if( err != STERR_NONE )
		return err;

	drive_size_in_sectors = read_capacity.LogicalBlockAddress+1;
	m_sector_size = read_capacity.BytesPerBlock;

	CStSDisk sdisk( (ULONG)drive_size_in_sectors, m_sector_size, _volume_label, _volume_label_length,
		m_p_updater->GetConfigInfo()->GetPreferredFAT() );
	if( sdisk.GetLastError() != STERR_NONE )
		return sdisk.GetLastError();

	if( sdisk.GetFileSystem() == FAT_32 )
	{
		sector_count = sdisk.GetPartitionBootSector()->Fat32.BPB_FATSz32;
	}
	else
	{
		sector_count = sdisk.GetPartitionBootSector()->BPB_FATSz16;
	}
	int directory_structure_offset = sdisk.GetPartitionBootSector()->BPB_HiddSec /*MBR included*/ 
		+ 1 /*PBS*/ + 
		( sector_count * sdisk.GetPartitionBootSector()->BPB_NumFATs );

	for( ULONG j=0; j<sdisk.GetNumDirectoryEntriesInSectors(); j++ )
	{
	    CStByteArray sector( m_sector_size );

		err = m_p_data_drive->ReadSector( &sector, 1, directory_structure_offset, m_sector_size );
		if( err != STERR_NONE )
			return err;
			
		for( size_t byte=0; byte<(size_t)m_sector_size; byte+=DIR_ENTRY_SIZE )
		{
			UCHAR attrib=0;
			sector.GetAt( byte + ROOT_DIR_ATTRIB_OFFSET, attrib );
			if( attrib == ATTR_VOLUME_ID )
			{
				for( int index=0; index<_volume_label_length; index ++ )
				{
					_volume_label[index] = *sector.GetAt( byte + index );
				}
				
				size_of_actual_label = _volume_label_length;
				label_found = TRUE;
				break;
			}
		}

		if( label_found )
			break;

		directory_structure_offset ++;
	}
	
	return err;
}

ULONG CStFormatter::GetTotalTasks()
{
	// tasks:
	// formatting data drive
	// formatting MBR
	// formatting hidden sectors
	// formatting PBS
	// formatting reserved sectors
	// formatting FAT Area
	// formatting directory structure
	// formatting wasted sectors
	return 10;
}

ST_ERROR CStFormatter::WriteRecover2DDImage()
{
	ST_ERROR err = STERR_NONE;
	ULONG sector_count=0;
    ULONG start_from_sector = 0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;
	wifstream image_file;
    int hImgFile = 0;
	wstring	filename;
	READ_CAPACITY_DATA read_capacity;

	//
	// read data_area size in sectors
	//
	err = m_p_data_drive->ReadCapacity(&read_capacity);

	m_sector_size = read_capacity.BytesPerBlock;

	if( m_platform != OS_98 && m_platform != OS_ME )
	{
		err = m_p_scsi->OpenPhysicalDrive( 1, m_p_updater->m_DeviceMode == UpdaterDeviceMode ? UPGRADE_MSC : NORMAL_MSC );
		if( err != STERR_NONE )
			return err;

		err = m_p_scsi->Lock(FALSE);
		if( err != STERR_NONE )
			return err;
	}
	else 
	{
		//
		// for 98/ME, we need to acquire format lock in addition to the usual lock.
		//
        m_p_scsi->Open();
        err = m_p_scsi->Lock(FALSE);
		if( err != STERR_NONE )
			return err;

		err = m_p_scsi->AcquireFormatLock(FALSE);
		if( err != STERR_NONE )
			return err;
	}

	m_p_updater->GetConfigInfo()->GetRecover2DDImageFileName(filename);
    _wsopen_s (&hImgFile, filename.c_str(), _O_BINARY | _O_RDONLY, _SH_DENYWR, _S_IREAD);

    if (hImgFile < 0)
        return 	STERR_FAILED_TO_OPEN_FILE;

    

    sector_count = _lseek (hImgFile, 0, SEEK_END) / m_sector_size;

	sectors_per_write = MAX_DOWNLOAD_SIZE_IN_BYTES / m_sector_size;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}

	number_of_iterations = sector_count / sectors_per_write;
	sectors_left_for_last_iteration = sector_count % sectors_per_write;

	m_p_updater->GetProgress()->SetCurrentTask( TASK_TYPE_TRANSFER_2DD_CONTENT, 
		(number_of_iterations + 1) * 2 );

    _lseek(hImgFile, 0, SEEK_SET);

    CStByteArray imageData( sectors_per_write * m_sector_size );
    char *cBuf = new char [ sectors_per_write * m_sector_size ];

	for(ULONG loop = 0; loop < number_of_iterations; loop ++)
    {
        _read (hImgFile, cBuf, (sectors_per_write * m_sector_size) );
        imageData.Write(cBuf, sectors_per_write * m_sector_size, 0);

		err = FormatSectors( imageData, start_from_sector );
		if( err != STERR_NONE )
			return err;

		start_from_sector += sectors_per_write;
	}

	delete[] cBuf;
    _close (hImgFile);

	err = m_p_scsi->ReleaseFormatLock(FALSE);
	if( err != STERR_NONE )
		return err;

	err = m_p_scsi->Unlock(FALSE);
	if( err != STERR_NONE )
		return err;

	err = m_p_scsi->Close();
	if( err != STERR_NONE )
		return err;

    return STERR_NONE;
}
