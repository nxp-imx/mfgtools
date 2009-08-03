#include ".\StHeader.h"
#include ".\StGlobals.h"
#include ".\StByteArray.h"
#include ".\stsdisk.h"
#include <math.h>

// Windows table - This is the Windows Table for FAT16 drives
// Note that this table includes entries for disk sizes larger
// than 512 MB even though typically only the entries for disks < 512 MB
// in size are used.
// The way this table is accessed is to look for the first entry
// in the table for which the disk size is less than or equal
// to the DiskSize field in that table entry.  For this table to
// work properly ReservedSectorCount must be 1, NumberOfFATs must be 2,
// and RootEntriesCount must be 512.  Any of these values being different
// may require the first table entry - DiskSize - value to be
// changed otherwise the cluster count may be too low for FAT16
WinSizeParams DiskSizeTableFAT16 [] = {
    // total sectors, sectorspercluster
	{8400		,	0},		// Disks up to 4.1MB - the 0 value for SecPerCluster trips an error
	{65536		,	1},		// Disks up to 32MB - 0.5k cluster // based on XP formatter behavior
	{131072		,	2},		// Disks up to 64MB - 1k cluster   // based on XP formatter behavior
	{262144		,	4},		// Disks up to 128MB - 2k cluster
	{524288		,	8},		// Disks up to 256MB - 4k cluster
	{1048576	,	16},	// Disks up to 512MB - 8k cluster
	// The entries after this are not used unless FAT16 is forced
	{2097152	,	32},	// Disks up to 1GB - 16k cluster
	{4194304	,	64},	// Disks up to 2GB - 32k cluster
	{0xFFFFFFFF	,	0}		// any disk > 2GB - the 0 value for SecPerCluster trips an error
};

// The way this table is accessed is to look for the first entry
// in the table for which the disk size is less than or equal
// to the DiskSize field in that table entry.  For this table to
// work properly ReservedSectorCount must be 32 and NumberOfFATs 
// must be 2. Any of these values being different may require the 
// first table entry - DiskSize - value to be changed otherwise 
// the cluster count may be too low for FAT32
WinSizeParams DiskSizeTableFAT32 [] = {
//    512	    1024
	{66600	  ,	0},		// Disks up to 32.5MB - the 0 value for SecPerCluster trips an error
	{131072   ,	1},		// Disks up to 64MB - 0.5K Cluster // based on XP formatter behavior
	{262144   ,   /*2*/1},     // Disks up to 128MB - 1K Cluster  // based on XP formatter behavior
	{532480   ,	/*4*/1},// Disks up to 260MB - 2k cluster  // based on XP formatter behavior
	{16777216 ,	8},		// Disks up to 8GB - 4k cluster
	{33554432 ,	16},	// Disks up to 16GB - 8k cluster
	{67108864 ,	32},	// Disks up to 32GB - 16k cluster
	// The entries after this are not used unless FAT16 is forced
	{0xFFFFFFFF	,	64}		// any disk > 32GB - 32k cluster
};

#define ONE_MB  (1024*1024)
#define _256_MB   (256*ONE_MB)
#define ONE_GB 1024*ONE_MB
#define TWO_GB 2*ONE_GB


CStSDisk::CStSDisk( ULONG _sectors, ULONG _sector_size, UCHAR* _vol_label, size_t _len, 
	   UCHAR _fat_type, string _name ):
	CStBase( _name )
{
	m_sectors		= _sectors;
	m_sector_size	= _sector_size;

	for( size_t i=0; i<_len; i++ )
	{
		m_vol_label[i]	= _vol_label[i];
	}
	m_vol_label_length = _len;
	
	m_filesystem	= 0xFF;

    // Selection of filesystem at this point is a guess.  It may be changed once
    // we calculate the total number of clusters.

//    if( ( _sectors * _sector_size ) <= (ULONG)( 32 * MEGA_BYTES ) )
    if( ( (ULONGLONG)m_sectors * (ULONGLONG)m_sector_size ) <= (ULONGLONG)TWO_GB  )
    {
		m_filesystem = FAT_16;
		if( _fat_type && (( m_sectors * m_sector_size ) >= (ULONG)_256_MB) )
		{
			m_filesystem = _fat_type;
		}
	}
	else
	{
		m_filesystem = FAT_32;
	}

	m_last_error	= InitializeDiskParameters();
}

CStSDisk::~CStSDisk(void)
{
}

ST_ERROR CStSDisk::InitializeDiskParameters()
{
	ST_ERROR err = STERR_NONE;

	m_disk.ulNumSectors		= m_sectors;

	err = InitChs();
	if( err != STERR_NONE )
		return err;

	err = InitPBS();
	if( err != STERR_NONE )
		return err;
/*
wchar_t szMsg[256];
wsprintf(szMsg, L"TotalSectors: %d\nSector Size: %d\nCylinders: %d\nHeads: %d\nSectors/Track: %d\nFile System: %s",
         m_sectors, m_sector_size, m_disk.Chs.Cylinder, m_disk.Chs.Head,
         m_disk.Chs.Sector, m_filesystem == FAT_16 ? L"FAT16" : L"FAT32");
::MessageBox(NULL, szMsg, L"test", MB_OK);
*/
	err = InitMBR();
	if( err != STERR_NONE )
		return err;
/*
wsprintf(szMsg, L"PBS Heads: %d\nMBR start head: %d\nMBR end head: %d",
         m_pbs.BPB_NumHeads, m_mbr.Partitions[0].StartCHSPacked.Head, m_mbr.Partitions[0].EndCHSPacked.Head);
::MessageBox(NULL, szMsg, L"test", MB_OK);
*/
	err = InitNumDirEntriesInSectors();
	if( err != STERR_NONE )
		return err;

	return err;
}



ST_ERROR CStSDisk::InitPBS()
{
    UINT uiCountOfClusters = 0;
    UINT FATSz = 0;
    USHORT currentFS = m_filesystem;
//    UINT sector_multiple = m_sector_size / 512;
    UINT pass = 0;

reset_filesystem:

    ++pass; // increment the pass (retry) count

	CStGlobals::MakeMemoryZero( (PUCHAR)&m_pbs, sizeof(BOOT_SECTOR) );

	//
	// jump instruction to boot code.
	//
	m_pbs.BS_jmpBoot[0]			= 0xeb;
    m_pbs.BS_jmpBoot[1]			= 0x3e; 
    m_pbs.BS_jmpBoot[2]			= 0x90;

	//
	// MSWIN4.1 is the recommended value from Microsoft white paper on FAT.
	// 
	m_pbs.BS_OEMName[0]			= 'M';
	m_pbs.BS_OEMName[1]			= 'S';
	m_pbs.BS_OEMName[2]			= 'W';
	m_pbs.BS_OEMName[3]			= 'I';
	m_pbs.BS_OEMName[4]			= 'N';
	m_pbs.BS_OEMName[5]			= '4';
	m_pbs.BS_OEMName[6]			= '.';
	m_pbs.BS_OEMName[7]			= '1';

	//
	// count of bytes per sector.
	//
	m_pbs.BPB_BytsPerSec		= (USHORT)m_sector_size;

/*
	//
	// number of sectors per allocation unit, must be a power of 2 and > 0
	// values greater than 32K for sectors per cluster do not work properly.
	//

	if( m_filesystem == FAT_16 )
	{
		for (int i=0;i<sizeof(DiskSizeTableFAT16)/sizeof(WinSizeParams);i++)
		{
			// Cycle through table until we exceed the size - then use that table entry.
            // Multiply the number of sectors by the multiple of 512 sectors to account for
            // sector sizes > 512 bytes. Likewise, divide the SectorsPerCluster value by the 
            // sector multiple.
			if ( (m_disk.ulNumSectors * sector_multiple) <= DiskSizeTableFAT16[i].DiskSize )
            {
                if (DiskSizeTableFAT16[i].SectorPerClusterValue / sector_multiple)
				    m_pbs.BPB_SecPerClus = DiskSizeTableFAT16[i].SectorPerClusterValue / (UCHAR) sector_multiple;
                else
				    m_pbs.BPB_SecPerClus = DiskSizeTableFAT16[i].SectorPerClusterValue; // this shouldn't happen
				break;
			}
		}
	}
	else //FAT_32
	{
		for (int i=0;i<sizeof(DiskSizeTableFAT32)/sizeof(WinSizeParams);i++)
		{
			// Cycle through table until we exceed the size - then use that table entry.
            // Multiply the number of sectors by the multiple of 512 sectors to account for
            // sector sizes > 512 bytes.  Likewise, divide the SectorsPerCluster value by the 
            // sector multiple.
			if ( (m_disk.ulNumSectors * sector_multiple) <= DiskSizeTableFAT32[i].DiskSize )
            {
// No longer working properly; don't divide
// No longer working properly; don't divide
//                if (DiskSizeTableFAT32[i].SectorPerClusterValue / sector_multiple)
//				    m_pbs.BPB_SecPerClus = DiskSizeTableFAT32[i].SectorPerClusterValue / (UCHAR) sector_multiple;
//                else
				    m_pbs.BPB_SecPerClus = DiskSizeTableFAT32[i].SectorPerClusterValue; // this shouldn't happen
				break;
			}
		}
	}
*/

	//
	// 1 for FAT16,FAT12, 32 for FAT32
	//
	m_pbs.BPB_RsvdSecCnt			= 1;
	if( m_filesystem == FAT_32 )
	{
		m_pbs.BPB_RsvdSecCnt		= 32;
	}

	//
	// most recommended value is 2.
	//
	m_pbs.BPB_NumFATs				= 2;

	// 
	// 512 for FAT16, FAT12 and 0 for FAT32.
	//
	m_pbs.BPB_RootEntCnt			= 1024;		//tt change - force to 512 for Table lookup.
	if( m_filesystem == FAT_32 )
	{
		m_pbs.BPB_RootEntCnt		= 0;
	}

	//
	// 0xF8, must match with 1st byte of FAT Area1 and FAT Area2.
	//
	m_pbs.BPB_Media				= BOOT_MEDIA_HARD_DISK;

	//
	// sectors per track visible on interrupt 0x13. relevant to media that have a geometry.
	//
	m_pbs.BPB_SecPerTrk			= (USHORT)m_disk.Chs.Sector;

	//
	// number of heads visible on interrupt 0x13. relevant to media that have a geometry.
	//
	m_pbs.BPB_NumHeads			= m_disk.Chs.Head;

	//
	// sectors before the start of partition boot sector. these sectors includes the MBR.
	//
	m_pbs.BPB_HiddSec			= m_disk.Chs.Sector;

	//
	// size of the partitionSectors, recommended by Microsoft to start from the second track, track number 1.
	// 
	ULONG partitionSectors		= m_disk.ulNumSectorsAdjusted - m_pbs.BPB_HiddSec;

	//
	// total count of sectors on volume
	//
	// for FAT_16, if TotSec fits in USHORT, use m_pbs.BPB_TotSec16
	//             else use m_pbs.BPB_TotSec32
	// for FAT_32, use m_pbs.BPB_TotSec32 and set m_pbs.BPB_TotSec16 to 0
	// 
	if ( m_filesystem == FAT_16 )
	{
		if ( partitionSectors < 0x10000 )
		{
			m_pbs.BPB_TotSec16 = (USHORT)partitionSectors;
			m_pbs.BPB_TotSec32 = 0;
		}
		else
		{
			m_pbs.BPB_TotSec16 = 0;
			m_pbs.BPB_TotSec32 = partitionSectors;
		}
	}
	else if ( m_filesystem == FAT_32 )// FAT_32 case
	{
		m_pbs.BPB_TotSec16 = 0;
		m_pbs.BPB_TotSec32 = partitionSectors;
	}

	//
	// interrupt 0x13 drive number, 0x00 for floppy, 0x80 for hard disks.
	// extended boot signature, 0x29 to indicate the following three fields
	// volid, vollab and filsystype are present.
	//
	if( m_filesystem != FAT_32 )
	{
		m_pbs.Fat16_12.BS_DrvNum				= 0x80;
		//m_pbs.Fat16_12.BS_Reserved			= already initialized to null
		m_pbs.Fat16_12.BS_BootSig				= 0x29;
		//m_pbs.Fat16_12.BS_VolID				= already initialized to null
		//m_pbs.Fat16_12.BS_VolLab				= see UpdateVolumeLabel()
		//m_pbs.Fat16_12.BS_Reserved1			= already initialized to null
	}
	else
	{
		//
		// This field is only defined for FAT32 media and does not exist on FAT12 and FAT16 media.
		// Bits 0-3	-- Zero-based number of active FAT. Only valid if mirroring is disabled.
		// Bits 4-6	-- Reserved.
		// Bit      7	-- 0 means the FAT is mirrored at runtime into all FATs.
		//				-- 1 means only one FAT is active; it is the one referenced in bits 0-3.
		// Bits 8-15 	-- Reserved.
		//
		m_pbs.Fat32.BPB_ExtFlags = 0;

		//
		// FAT document defines it to be 0.
		//
		m_pbs.Fat32.BPB_FSVer = 0;

		//
		// This field is only defined for FAT32 media and does not exist on FAT12 and FAT16 media. 
		// This is set to the cluster number of the first cluster of the root directory, 
		// usually 2 but not required to be 2. 
		//
		m_pbs.Fat32.BPB_RootClus = 2;

		//
		// This field is only defined for FAT32 media and does not exist on FAT12 and FAT16 media. 
		// Sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1.
		//
		m_pbs.Fat32.BPB_FSInfo = 1;

		//
		// This field is only defined for FAT32 media and does not exist on FAT12 and FAT16 media. 
		// Usually 6. No value other than 6 is recommended.
		//
        m_pbs.Fat32.BPB_BkBootSec = 6;

//		m_pbs.Fat32.BPB_Reserved = already initialized to null
		m_pbs.Fat32.BS_DrvNum = 0x80;
		m_pbs.Fat32.BS_Reserved1 = 0;
		m_pbs.Fat32.BS_BootSig = 0x29;
		//m_pbs.Fat32.BS_VolID				= already initialized to null
		//m_pbs.Fat32.BS_VolLab				= see UpdateVolumeLabel()
	}

	//
	// fat type.
	// 
    if( m_filesystem == FAT_16 )
	{
		memcpy(m_pbs.Fat16_12.BS_FilSysType, "FAT16   ", 8);
	}
	else
	{
		memcpy(m_pbs.Fat32.BS_FilSysType, "FAT32   ", 8);
	}

	//
	// calculation of m_pbs.BPB_SecPerClus
	//
	m_pbs.BPB_SecPerClus = 0;
	ULONG sectorsPerCluster = 0;
	ULONG numClusters = 0;

	if ( m_filesystem == FAT_12 )
	{
		// numClusters < 4085
		// TODO: implement
		// Need to research how to format FAT_12
		return STERR_UNABLE_TO_CALCULATE_CHS;
	}
	else if ( m_filesystem == FAT_16 )
	{
		// 4085 <= numClusters < 65525
        // start with the largest cluster size (32KB) and work smaller.
		for ( sectorsPerCluster = 32768 / m_sector_size; sectorsPerCluster >= 1; sectorsPerCluster>>=1 )
		{
			numClusters = partitionSectors / sectorsPerCluster;
			if ( numClusters < 4085 )
			{
                // not enough clusters for FAT_16 so 
				// half sectorsPerCluster to double numClusters
				continue;
			}
			else
			{
				if ( numClusters < 65525 )
				{
					// numClusters is just right for FAT_16
					m_pbs.BPB_SecPerClus = (UCHAR)sectorsPerCluster;
					break;
				}
				else
				{
					// must be FAT_32
			        m_filesystem = FAT_32;
                    break;
				}
			}
		}
		if ( sectorsPerCluster == 1 && m_pbs.BPB_SecPerClus == 0 )
		{
			// couldn't get numClusters big enough for FAT_16
		    // Must be FAT_12
		    return STERR_UNABLE_TO_CALCULATE_CHS;
		}
	}
	else if ( m_filesystem == FAT_32 )
	{
		// 65525 <= numClusters
        // start with the largest cluster size (32KB) and work smaller.
		for ( sectorsPerCluster = 32768 / m_sector_size; sectorsPerCluster >= 1; sectorsPerCluster>>=1 )
		{
			numClusters = partitionSectors / sectorsPerCluster;
			if ( numClusters < 65525 )
			{
                // not enough clusters for FAT_32 so 
				// half sectorsPerCluster to double numClusters
				continue;
			}
			else
			{
			    // numClusters is OK for FAT_32
			    m_pbs.BPB_SecPerClus = (UCHAR)sectorsPerCluster;
                break;
			}
		}
		if ( sectorsPerCluster == 1 && m_pbs.BPB_SecPerClus == 0 )
		{
			// couldn't get numClusters big enough for FAT_32
		    // try FAT_16
			m_filesystem = FAT_16;
		}
    }
	
    //
    // Now possibly reset the file system selected based on the total number of clusters.
    // If different we need to start over again.
    //
	if ( m_pbs.BPB_SecPerClus == 0 )
	{
		if (currentFS != m_filesystem)
		{
			if (pass == 2) // have we already been around the block once?  If so, we cannot determine the proper values.
				return STERR_UNABLE_TO_CALCULATE_CHS;

			currentFS = m_filesystem;
			goto reset_filesystem;
		}
		else
			return STERR_UNABLE_TO_CALCULATE_CHS;
	}

	//
	// calculation of FAT Area size according to Microsoft FAT document.
	//
	// RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec – 1)) / BPB_BytsPerSec;
	// TmpVal1 = DskSize – (BPB_ResvdSecCnt + RootDirSectors);
	// TmpVal2 = (256 * BPB_SecPerClus) + BPB_NumFATs;
	// If(FATType == FAT32)
    //		TmpVal2 = TmpVal2 / 2;
	// FATSz = (TMPVal1 + (TmpVal2 – 1)) / TmpVal2;
	// If(FATType == FAT32) {
    //		BPB_FATSz16 = 0;
    //		BPB_FATSz32 = FATSz;
	// } else {
    //		BPB_FATSz16 = LOWORD(FATSz);
    //		/* there is no BPB_FATSz32 in a FAT16 BPB */
	// }
	//

	ULONG root_dir_sectors = ( ( m_pbs.BPB_RootEntCnt * 32 ) +
		( m_pbs.BPB_BytsPerSec - 1 ) ) / m_pbs.BPB_BytsPerSec;
	
	ULONG temp1 = partitionSectors - ( m_pbs.BPB_RsvdSecCnt + root_dir_sectors );

	ULONG temp2 = ( 256 * m_pbs.BPB_SecPerClus ) + m_pbs.BPB_NumFATs;

	if( m_filesystem == FAT_32 )
	{
		temp2 = temp2 / 2;
	}
	
	ULONG fat_size = ( temp1 + ( temp2 - 1 ) ) / temp2;
	
	if( m_filesystem == FAT_32 )
	{
		m_pbs.BPB_FATSz16 = 0;
		m_pbs.Fat32.BPB_FATSz32 = fat_size;
	}
	else
	{
		m_pbs.BPB_FATSz16 = LOWORD( fat_size );
//		m_pbs.Fat32.BPB_FATSz32 = 0; //no need.
	}


    //
    // Now possibly reset the file system selected based on the total number of clusters.
    // If different we need to start over again.
    //

    if (m_pbs.BPB_FATSz16 != 0)
        FATSz = m_pbs.BPB_FATSz16;
    else
        FATSz = m_pbs.Fat32.BPB_FATSz32;

    uiCountOfClusters = (m_disk.ulNumSectors - (m_pbs.BPB_RsvdSecCnt + (m_pbs.BPB_NumFATs * FATSz) +  root_dir_sectors)) / m_pbs.BPB_SecPerClus;
//    if (uiCountOfClusters < 65525)
//            m_filesystem = FAT_16;
//        else
//            m_filesystem = FAT_32;

//    if (currentFS != m_filesystem)
//    {
//        if (pass == 2) // have we already been around the block once?  If so, we cannot determine the proper values.
//            return STERR_UNABLE_TO_CALCULATE_CHS;
//
//        currentFS = m_filesystem;
//        goto reset_filesystem;
//    }

    m_pbs.BS_Signature				= BOOT_SIGNATURE;
/*
TCHAR szTmp[128];
_stprintf(szTmp, _T("sectors: %d\nbytespersector: %d\nsectorspercluster: %d\ntotalclusters: %d\nFS: %s"),
          m_disk.ulNumSectors, m_pbs.BPB_BytsPerSec,
          m_pbs.BPB_SecPerClus, uiCountOfClusters,
          m_filesystem == FAT_16 ? _T("FAT") : _T("FAT32"));
MessageBox(NULL, szTmp, _T("test"), MB_OK);
*/
	if( m_filesystem == FAT_32 )
	{
		InitPBS2();
		InitPBS3();
	}

	return STERR_NONE;
}


ST_ERROR CStSDisk::InitMBR()
{
	CStGlobals::MakeMemoryZero( (PUCHAR)&m_mbr, sizeof(PARTITION_TABLE) );
	



    memcpy (m_mbr.bootstrap, BOOTSTRAP_CODE, BOOTSTRAP_SIZE); 

	m_mbr.Partitions[0].BootDescriptor		= PART_BOOTID_BOOTABLE;
	m_mbr.Partitions[0].SectorCount			= m_disk.ulNumSectorsAdjusted - m_pbs.BPB_HiddSec;
	m_mbr.Partitions[0].FirstSectorNumber	= m_pbs.BPB_HiddSec;

	if( m_filesystem == FAT_16 )
	{
		m_mbr.Partitions[0].FileSystem = PART_SYSID_FAT16;
	}
	else
	{
		m_mbr.Partitions[0].FileSystem = PART_SYSID_FAT32;
	}

	m_mbr.Signature = BOOT_SIGNATURE;
	
	return CalcStartEndChs();
}


ST_ERROR CStSDisk::InitPBS2()
{
	//
	// all values are set to default as stated in Microsoft FAT document.
	//
	CStGlobals::MakeMemoryZero( (PUCHAR)&m_pbs2, sizeof(BOOT_SECTOR2) );
	m_pbs2.FSI_LeadSig = FSI_LEAD_SIGNATURE;
	m_pbs2.FSI_StrucSig = FSI_STRUC_SIGNATURE;
	m_pbs2.FSI_Free_Count = FSI_DEFAULT_FREE_COUNT;
	m_pbs2.FSI_Nxt_Free = FSI_DEFAULT_FREE_NEXT;
	m_pbs2.BS_Signature = FSI_SIGNATURE;
	return STERR_NONE;
}

ST_ERROR CStSDisk::InitPBS3()
{
	//
	// all values are set to default as stated in Microsoft FAT document.
	//
	CStGlobals::MakeMemoryZero( (PUCHAR)&m_pbs3, sizeof(BOOT_SECTOR3) );
	m_pbs3.BS_Signature = BOOT_SIGNATURE;
	return STERR_NONE;
}

ST_ERROR CStSDisk::InitNumDirEntriesInSectors()
{
	m_num_dir_entries_in_sectors = (m_pbs.BPB_RootEntCnt * DIR_ENTRY_SIZE) / 
		m_pbs.BPB_BytsPerSec;

	if( m_filesystem == FAT_32 )
	{
		m_num_dir_entries_in_sectors = m_pbs.BPB_SecPerClus;
	}
	return STERR_NONE;
}

ST_ERROR CStSDisk::UpdateVolumeLabel()
{
	if( m_vol_label_length > MAX_LABEL_SIZE )
		m_vol_label_length = MAX_LABEL_SIZE;

	if( m_filesystem == FAT_32 )
		memcpy( m_pbs.Fat32.BS_VolLab, m_vol_label, m_vol_label_length );
	else
		memcpy( m_pbs.Fat16_12.BS_VolLab, m_vol_label, m_vol_label_length );

	return STERR_NONE;
}

PPARTITION_TABLE CStSDisk::GetMasterBootRecord( )
{
	return &m_mbr;
}

PBOOT_SECTOR CStSDisk::GetPartitionBootSector( )
{
	return &m_pbs;
}

ULONG CStSDisk::GetNumDirectoryEntriesInSectors()
{
	return m_num_dir_entries_in_sectors;
}

ST_ERROR CStSDisk::GetFirstFatSector(CStByteArray* _p_sector)
{
	ST_ERROR err = STERR_NONE;

	switch( m_filesystem )
	{
	case FAT_32:
			_p_sector->SetAt(11, 0x0F );
			_p_sector->SetAt(10, 0xFF );
			_p_sector->SetAt( 9, 0xFF );
			_p_sector->SetAt( 8, 0xFF );
			_p_sector->SetAt( 7, 0x0F );
			_p_sector->SetAt( 6, 0xFF );
			_p_sector->SetAt( 5, 0xFF );
			_p_sector->SetAt( 4, 0xFF );
			_p_sector->SetAt( 3, 0x0F );
			_p_sector->SetAt( 2, 0xFF );
			_p_sector->SetAt( 1, 0xFF );
			_p_sector->SetAt( 0, BOOT_MEDIA_HARD_DISK );
			break;
	case FAT_16:
			_p_sector->SetAt( 3, 0xFF );
			_p_sector->SetAt( 2, 0xFF );
			_p_sector->SetAt( 1, 0xFF );
			_p_sector->SetAt( 0, BOOT_MEDIA_HARD_DISK );
			break;
	default:
		err = STERR_INVALID_FILE_SYSTEM_REQUEST;
	}

	return err;
}

ST_ERROR CStSDisk::InitChs()
{
	ULONG ulSize, ulWastedSectors;
	UCHAR ucSectors, ucSectors2, ucOptimalSectors=0;
	USHORT usHeads, usHeads2, usOptimalHeads=0, usCylinders, usCylinders2, usOptimalCylinders=0;
	BOOL not_done=TRUE;
    BOOL bGetEvenSectors = TRUE;
	// Number of bits available for CHS:
	//
	// Standard      Cylinders   Heads   Sectors   Total
	// --------------------------------------------------
	//  IDE/ATA        16          4        8       28
	//  Int13/MBR      10          8        6       24
	//  Combination    10          4        6       20
	//
	// In decimal we get
	//
	// Standard      Cylinders   Heads   Sectors            Total
	// ----------------------------------------------------------------
	//  IDE/ATA        65536      16       256       268435456 =  128GB
	//  Int13/MBR      1024       256       63*       16515072 = 8064MB
	//  Combination    1024       16        63         1032192 =  504MB
	//
	// * There is no sector "0" in CHS (there is in LBA, though)
	//
	// All drives with more than 16,515,072 sectors will get bogus CHS
	//	parameters.

	if(m_disk.ulNumSectors >= (ULONG)16515072)
	{
		// Create bogus, non-zero parameters.  Params are non-zero because
		//	some 3rd party media readers may fail to recognize the media.
		m_disk.Chs.Cylinder	= MaxCylinders;
		m_disk.Chs.Head		= MaxHeads;
		m_disk.Chs.Sector	= MaxSectors;

		m_disk.ulNumSectorsAdjusted = m_disk.ulNumSectors;
		m_disk.ulWastedSectors = m_disk.ulNumSectors - m_disk.ulNumSectorsAdjusted;

		return STERR_NONE;
	}

start_calculation:

	usCylinders = 1;
	usHeads = 1;
	ucSectors = 1;
	ulWastedSectors = 0x7FFFFFFF;
	
	while(not_done)
	{
		ulSize = (ULONG)usCylinders * (ULONG)usHeads * (ULONG)ucSectors;
		if(ulSize < m_disk.ulNumSectors)
		{
			// Not enough
			ucSectors++;
			if(ucSectors > MAX_SECTORS)
			{
				ucSectors = 1;
				usHeads++;
				if(usHeads > MAX_HEADS)
				{
					usHeads = 1;
					usCylinders++;
					if(usCylinders > MAX_CYLINDERS)
					{
						break;
					}
				}
			}
		}
		else
		{
			if ( (ulSize == m_disk.ulNumSectors)  &&
                 ( (bGetEvenSectors && !(ucSectors % 2)) || !bGetEvenSectors) )
			{
				// Found an exact solution so it's time to stop
				m_disk.bExactChsSolution = TRUE;
				usOptimalCylinders = usCylinders;
				usOptimalHeads = usHeads;
				ucOptimalSectors = ucSectors;
				break;
			}
			else
			{
				// Found a solution.  We're over by some amount so we need
				//	to back up
				usCylinders2 = usCylinders;
				usHeads2 = usHeads;
				ucSectors2 = ucSectors;

				ucSectors2--;
				if(ucSectors2 == 0)
				{
					ucSectors2 = MAX_SECTORS;
					usHeads2--;
					if(usHeads2 == 0)
					{
						usHeads2 = MAX_HEADS;
						usCylinders2--;
						if(usCylinders2 == 0)
						{
							//printf("ERROR - bad CHS solution\r\n");
							return STERR_BAD_CHS_SOLUTION;
						}
					}
				}
				
				// Only keep it if it's optimal
				if ( ((m_disk.ulNumSectors - ((ULONG)usCylinders2 * 
					(ULONG)usHeads2 * (ULONG)ucSectors2)) < ulWastedSectors)  && 
                    ( (bGetEvenSectors && !(ucSectors2 % 2)) || !bGetEvenSectors) )
				{
					ulWastedSectors = m_disk.ulNumSectors -
						((ULONG)usCylinders2 * (ULONG)usHeads2 *
						(ULONG)ucSectors2);
					usOptimalCylinders = usCylinders2;
					usOptimalHeads = usHeads2;
					ucOptimalSectors = ucSectors2;
				}

				// Keep searching
				ucSectors++;
				if(ucSectors > MAX_SECTORS)
				{
					ucSectors = 1;
					usHeads++;
					if(usHeads > MAX_HEADS)
					{
						usHeads = 1;
						usCylinders++;
						if(usCylinders > MAX_CYLINDERS)
						{
							break;
						}
					}
				}
			}
		}
	}

    if (ucOptimalSectors == 0 && bGetEvenSectors) // We didn't find a solution for an even number of sectors/track
    {
        bGetEvenSectors = FALSE;
        goto start_calculation;     // try again for odd number of sectors/track
    }

	m_disk.Chs.Cylinder = usOptimalCylinders;
	m_disk.Chs.Head = usOptimalHeads;
	m_disk.Chs.Sector = ucOptimalSectors;

	m_disk.ulNumSectorsAdjusted = ((ULONG)usOptimalCylinders * (ULONG)usOptimalHeads * (ULONG)ucOptimalSectors);

	m_disk.ulWastedSectors = m_disk.ulNumSectors - m_disk.ulNumSectorsAdjusted;

	return STERR_NONE;
}

ULONG CStSDisk::GetWastedSectors()
{
	return m_disk.ulWastedSectors;
}

ST_ERROR CStSDisk::CalcStartEndChs()
{
	CHS StartChs;
	CHS EndChs;

	// Calculate the start CHS for the MBR partition entry.  Use the MBR
	//	start sector number adjusted from LBA 0-based to a 1-based address.
    if(SectorToChs(&m_disk, &StartChs, (m_sector_size == 512 ? m_mbr.Partitions[0].FirstSectorNumber+1 : m_mbr.Partitions[0].FirstSectorNumber)) !=
//    if(SectorToChs(&m_disk, &StartChs, m_mbr.Partitions[0].FirstSectorNumber+1) !=
		STERR_NONE)
	{
		return STERR_UNABLE_TO_CALCULATE_CHS;
	}

	if(PackChs(StartChs, &(m_mbr.Partitions[0].StartCHSPacked)) != STERR_NONE)
	{
		return STERR_UNABLE_TO_PACK_CHS;
	}

	// Calculate the end CHS for the MBR partition entry.  Use the MBR
	//	start sector number, the SectorCount will adjust for LBA 0-based.
	if(SectorToChs(&m_disk, &EndChs, (m_mbr.Partitions[0].FirstSectorNumber+
		m_mbr.Partitions[0].SectorCount)) != STERR_NONE)
	{
		return STERR_UNABLE_TO_CALCULATE_CHS;
	}

	if(PackChs(EndChs, &(m_mbr.Partitions[0].EndCHSPacked)) != STERR_NONE)
	{
		return STERR_UNABLE_TO_PACK_CHS;
	}

	return STERR_NONE;
}

ST_ERROR CStSDisk::SectorToChs(PDISK _p_disk, PCHS _p_chs, ULONG _sector)
{
	ULONG ulTemp=0;
	BOOL not_done = TRUE;
	_p_chs->Cylinder = 0;
	_p_chs->Head = 0;
	_p_chs->Sector = 1;

    // If logicalBlockAddress is bigger than 1024 x 255* x 63 == 16,450,560, use some 'commonly accepted' bogus values.
    // *note: MS bug prevents using 256 heads.
    //
    // Reference:
    // "Then there is the problem of what to write in (c,h,s) if the numbers do not fit. The main strategies 
    // seem to be:
    // 1. Mark (c,h,s) as invalid by writing some fixed value.
    //    ...
    //    1b. Write (1022,254,63) for any nonrepresentable CHS."
    //
    // http://www.win.tue.nl/~aeb/partitions/partition_types-2.html
    //
    if ( _sector > 16450560 )
    {
        _p_chs->Cylinder = MaxCylinders-2;
        _p_chs->Head     = MaxHeads-1;
        _p_chs->Sector   = MaxSectors;
    }
    else
    {
		while(not_done)
		{
			ulTemp = (_p_chs->Cylinder * _p_disk->Chs.Head * _p_disk->Chs.Sector) 
				+ (_p_chs->Head * _p_disk->Chs.Sector) 
				+ _p_chs->Sector;

			if( ulTemp == _sector)
			{
				break;
			}
			
			_p_chs->Sector++;
			if(_p_chs->Sector > _p_disk->Chs.Sector)
			{
				_p_chs->Sector = 1;
				_p_chs->Head++;
				if(_p_chs->Head == _p_disk->Chs.Head)
				{
					_p_chs->Head = 0;
					_p_chs->Cylinder++;
					if(_p_chs->Cylinder == _p_disk->Chs.Cylinder)
					{
						return STERR_UNABLE_TO_CALCULATE_CHS;
					}
				}
			}

		}
	}
	return STERR_NONE;
}

ST_ERROR CStSDisk::PackChs(CHS _chs, PCHS_PACKED _p_chsPacked)
{
	_p_chsPacked->Cylinder	= (UCHAR)(_chs.Cylinder & 0x00FF);
	_p_chsPacked->Head		= (UCHAR)_chs.Head;
	_p_chsPacked->Sector	= _chs.Sector | ((UCHAR)((_chs.Cylinder & 0x0300)>>2));

	return STERR_NONE;
}

