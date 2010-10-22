/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "stbase.h"

////////////////////////////////////////////////////////////////////////////////
//
//	Definitions
//
////////////////////////////////////////////////////////////////////////////////

#define MAX_CYLINDERS	1024
#define MAX_HEADS		256
#define MAX_SECTORS		63
#define MAX_LABEL_SIZE	11

#define DIR_ENTRY_SIZE  32
#define MBR_SECTOR_COUNT 1
#define PBS_SECTOR_COUNT 1

#define FAT_12 0
#define FAT_16 1
#define FAT_32 2

#define PART_BOOTID_BOOTABLE	0x80
#define PART_SYSID_FAT12		0x01
#define PART_SYSID_FAT16		0x06
#define PART_SYSID_FAT32		0x0B
#define PART_SIGNATURE			0xaa55

#define	BOOT_MEDIA_HARD_DISK		0xf8
#define BOOT_SIGNATURE				0xaa55
#define BOOT_SIGNATURE_OFFSET		510

#define SSFDC_ALIGNMENT_8MB     0x0000000F
#define SSFDC_ALIGNMENT_16MB    0x0000001F
#define SSFDC_ALIGNMENT_32MB    SSFDC_ALIGNMENT_16MB
#define SSFDC_ALIGNMENT_64MB    SSFDC_ALIGNMENT_16MB
#define SSFDC_ALIGNMENT_128MB   SSFDC_ALIGNMENT_16MB

#define SSFDC_NUM_SECTORS_8MB	1000*16
#define SSFDC_NUM_SECTORS_16MB	1000*32
#define SSFDC_NUM_SECTORS_32MB	2*1000*32
#define SSFDC_NUM_SECTORS_64MB	4*1000*32
#define SSFDC_NUM_SECTORS_128MB	8*1000*32

#define KILO_BYTES ((ULONG)1024)
#define MEGA_BYTES ((ULONG)1024*(ULONG)1024)
#define GIGA_BYTES ((ULONG)1024*(ULONG)1024*(ULONG)1024)

#define FSI_LEAD_SIGNATURE		0x41615252
#define FSI_STRUC_SIGNATURE		0x61417272
#define FSI_DEFAULT_FREE_COUNT	0xFFFFFFFF
#define FSI_DEFAULT_FREE_NEXT	0xFFFFFFFF
#define FSI_SIGNATURE			0xAA550000

#define ROOT_DIR_ATTRIB_VOL_LABEL	0x8

#pragma pack(push,1)

    // Here is bootstrap code with Sigmatel, Inc. embedded:
    //; Sigmatel device is not a bootable device, therefore no need to check the partition table
    //
    //        CLI                               ; disable interrupts
    //        JMP      MyBootstrap
    //
    //        DB        21h,53h,22h,69h,23h,67h,24h,6dh,25h,61h,26h,54h,27h,65h
    //        DB        28h,6ch,29h,2ch,2ah,49h,2bh,6eh,2ch,63h,2dh,32h,30h,30h,35h
    //MyBootstrap
    //
    //        XOR      AX,AX
    //        MOV     SP,7C00H         ;set the stack pointer
    //        MOV     SS,AX               ; set the stack space
    //        STI                               ; allow interrupts
    //        INT       018H                 ; return to BIOS to boot other drives
    //Hung    JMP      Hung                 ; catcher loop
    //        DB        30h,39h,31h,33h
    //        END
#define BOOTSTRAP_SIZE      48
CONST UCHAR BOOTSTRAP_CODE[] = 
            {0xFA,0xEB,0x1D,0x21,0x53,0x22,0x69,0x23, 0x67,0x24,0x6D,0x25,0x61,0x26,0x54,0x27,
             0x65,0x28,0x6C,0x29,0x2C,0x2A,0x49,0x2B, 0x6E,0x2C,0x63,0x2D,0x32,0x30,0x30,0x35,
             0x33,0xC0,0xBC,0x00,0x7C,0x8E,0xD0,0xFB, 0xCD,0x18,0xEB,0xFE,0x30,0x39,0x31,0x33};
//
// Windows size parameters.
//
typedef struct _WinSizeParams
{
	DWORD		DiskSize;
	UCHAR		SectorPerClusterValue;
} WinSizeParams;

//
// CHS
//
typedef struct _CHS
{
	USHORT		Head;
	UCHAR		Sector;
	USHORT		Cylinder;
} CHS, *PCHS;

//
// CHS Packed
//
typedef struct _CHS_PACKED
{
	UCHAR		Head;
	UCHAR		Sector;
	UCHAR		Cylinder;
} CHS_PACKED, *PCHS_PACKED;

//
// Fat Partition table
//
typedef struct _PART_ENTRY
{
	UCHAR		BootDescriptor;				// 0=nonboot, 0x80=bootable
	CHS_PACKED	StartCHSPacked;
	UCHAR		FileSystem;					// 1=fat12, 6=fat16
	CHS_PACKED	EndCHSPacked;
	ULONG		FirstSectorNumber;			// relative to beginning of device
	ULONG		SectorCount;
} PART_ENTRY, *PPART_ENTRY;

typedef struct _PARTITION_TABLE
{
    UCHAR       bootstrap[BOOTSTRAP_SIZE];
	UCHAR		ConsistencyCheck[398];		// not used
	PART_ENTRY	Partitions[4];
	USHORT		Signature;					// 0xaa55
} PARTITION_TABLE, *PPARTITION_TABLE;

//
// Fat boot sector
//
typedef struct _BOOT_SECTOR
{
// common to FAT12, FAT16 and FAT32
	UCHAR		BS_jmpBoot[3];				// 0xeb 0xXX 0x90
	UCHAR		BS_OEMName[8];
	USHORT		BPB_BytsPerSec;				// 512
	UCHAR		BPB_SecPerClus;
	USHORT		BPB_RsvdSecCnt;			// 1
	UCHAR		BPB_NumFATs;					// 2
	USHORT		BPB_RootEntCnt;			// 512
	USHORT		BPB_TotSec16;
	UCHAR		BPB_Media;			// 0xf8 (hard disk)
	USHORT		BPB_FATSz16;
	USHORT		BPB_SecPerTrk;			// 32 (no meaning)
	USHORT		BPB_NumHeads;			// 2 (no meaning)
	ULONG		BPB_HiddSec;			// 0
	ULONG		BPB_TotSec32;
	union
	{
		struct 
		{
// common to FAT12 and FAT16
			UCHAR		BS_DrvNum;
			UCHAR		BS_Reserved;
			UCHAR		BS_BootSig;
			UCHAR		BS_VolID[4];
			UCHAR		BS_VolLab[11];
			UCHAR		BS_FilSysType[8];					
			UCHAR		BS_Reserved1[448];
		} Fat16_12;

		struct 
		{
// FAT32 only
			ULONG		BPB_FATSz32;
			USHORT		BPB_ExtFlags;
			USHORT		BPB_FSVer;			// 0
			ULONG		BPB_RootClus;		// 2
			USHORT		BPB_FSInfo;			// 1
			USHORT		BPB_BkBootSec;
			UCHAR		BPB_Reserved[12];
			UCHAR		BS_DrvNum;
			UCHAR		BS_Reserved1;
			UCHAR		BS_BootSig;
			UCHAR		BS_VolID[4];
			UCHAR		BS_VolLab[11];
			UCHAR		BS_FilSysType[8];
			UCHAR		BS_Reserved3[420];
		} Fat32;
	};
// common to FAT12, FAT16 and FAT32
	USHORT	BS_Signature;
} BOOT_SECTOR, *PBOOT_SECTOR;

typedef struct _FAT32_BOOTSECTOR2 {
	ULONG		FSI_LeadSig;
	UCHAR		FSI_Reserved1[480];
	ULONG		FSI_StrucSig;
	ULONG		FSI_Free_Count;
	ULONG		FSI_Nxt_Free;
	UCHAR		FSI_Reserved2[12];
	ULONG		BS_Signature;
} BOOT_SECTOR2, *PBOOT_SECTOR2;

typedef struct _FAT32_BOOTSECTOR3 {
	UCHAR		Reserved_Sector[510];
	USHORT		BS_Signature;
} BOOT_SECTOR3, *PBOOT_SECTOR3;

//
// Disk parameters
//
typedef struct {
	ULONG ulNumSectors;
	CHS Chs;
	ULONG ulWastedSectors;
	ULONG ulNumSectorsAdjusted;
	ST_BOOLEAN bExactChsSolution;
} DISK, *PDISK;

// File attributes:
#define ATTR_READ_ONLY   	0x01
#define ATTR_HIDDEN 	0x02
#define ATTR_SYSTEM 	0x04
#define ATTR_VOLUME_ID 	0x08
#define ATTR_DIRECTORY	0x10
#define ATTR_ARCHIVE  	0x20

#define ROOT_DIR_ATTRIB_OFFSET   11
typedef struct _ROOT_DIR
{
	UCHAR		DIR_Name[11];
	UCHAR		DIR_Attr;
	UCHAR		DIR_NTRes;
	UCHAR		DIR_CrtTimeTenth;
	USHORT		DIR_CrtTime;
	USHORT		DIR_CrtDate;
	USHORT		DIR_LastAccessDate;
	USHORT		DIR_FstClusHI;
	USHORT		DIR_WrtTime;
	USHORT		DIR_WrtDate;
	USHORT		DIR_FstClusLO;
	ULONG		DIR_FileSize;
} ROOT_DIR, *PROOT_DIR;

#pragma pack(pop)

class CStSDisk :
	public CStBase
{
public:

	CStSDisk( ULONG _sectors, ULONG _sector_size, UCHAR* _vol_label, size_t _len, UCHAR _fat_type, string _name="CStDisk");
	virtual ~CStSDisk(void);

	PPARTITION_TABLE GetMasterBootRecord( );

	PBOOT_SECTOR GetPartitionBootSector( );
	PBOOT_SECTOR2 GetPartitionBootSector2( ) { return &m_pbs2; }
	PBOOT_SECTOR3 GetPartitionBootSector3( ) { return &m_pbs3; }

	ULONG GetNumDirectoryEntriesInSectors( );

	ST_ERROR GetFirstFatSector(CStByteArray* _p_sector);

	USHORT GetFileSystem() { return	m_filesystem; }
	ULONG GetWastedSectors();

private:

	ULONG				m_sectors;
	ULONG				m_sector_size;
	UCHAR				m_vol_label[MAX_LABEL_SIZE];
	size_t				m_vol_label_length;
	ROOT_DIR			m_root_dir;
	PARTITION_TABLE		m_mbr;
	BOOT_SECTOR			m_pbs;
	BOOT_SECTOR2		m_pbs2;
	BOOT_SECTOR3		m_pbs3;
	ULONG				m_num_dir_entries_in_sectors;
	DISK				m_disk;
	USHORT				m_filesystem;

	// CHS defines
    static const USHORT MaxCylinders = 1024; // 1024 - Maximum cylinders(or tracks) for computing CHS fields in the MBR.
    static const USHORT MaxHeads     = 255;  // 255 - Maximum heads/cylinder for computing CHS fields in the MBR. *Note MS bug prevents using 256 heads.
    static const UCHAR  MaxSectors   = 63;   // 63 - Maximum sectors/track for computing CHS fields in the MBR.
	
private:

	ST_ERROR InitializeDiskParameters();
	ST_ERROR InitMBR();
	ST_ERROR InitPBS();
	ST_ERROR InitPBS2();
	ST_ERROR InitPBS3();
	ST_ERROR InitRootDirectory();
	ST_ERROR InitNumDirEntriesInSectors();
	ST_ERROR InitChs();

	BOOL IsSsfdcCompliant();
	ST_ERROR FillChsForSsFdcCompliant();
	ST_ERROR UpdateFileSystemParameters();
	ST_ERROR CalcNumSectorsPerFat(ULONG _max_clusters);
	ST_ERROR CalcHiddenSectors();
	ST_ERROR CalcStartEndChs();
	ST_ERROR SectorToChs(PDISK _p_disk, PCHS _p_chs, ULONG _sector);
	ST_ERROR PackChs(CHS _chs, PCHS_PACKED _p_chsPacked);
	ST_ERROR UpdateVolumeLabel();
};
