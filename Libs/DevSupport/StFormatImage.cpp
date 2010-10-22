/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "StFormatImage.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHS IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHS::CHS(const uint32_t totalSectors)
// 
// - Constructs a CHS object and initializes _headsPerCylinder and _sectorsPerTrack member variables
//   using a fairly arbitrary algorithm.
//
// - Param: const uint32_t totalSectors - The total number of sectors from Absolute Sector 0 (MBR) to the end
//                                        of the media.
//
// TODO: This may need to be reworked such that a CHS solution can be achieved that will address all useable
//       sectors. Add a wastedSectors variable such that useableSectors = totalSectors - wastedSectors.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatImage::CHS::CHS(const uint32_t totalSectors)
{
    uint32_t cylinder;

    // Doesn't really matter how we determine CHS
    // Just try to make something that addresses all the sectors
    for ( _sectorsPerTrack = MaxSectors; _sectorsPerTrack >= 1; --_sectorsPerTrack )
    {
        for ( _headsPerCylinder = MaxHeads; _headsPerCylinder >= 1; --_headsPerCylinder )
        {
            for ( cylinder = 1; cylinder <= MaxCylinders; ++ cylinder )
            {
                if ( cylinder * _headsPerCylinder * _sectorsPerTrack == totalSectors )
                    break;
            }
            if ( cylinder * _headsPerCylinder * _sectorsPerTrack == totalSectors )
                break;

        }
        if ( cylinder * _headsPerCylinder * _sectorsPerTrack == totalSectors )
            break;
    }
    
    // If the above algorith failed, just initialize members to max values.
    if ( cylinder * _headsPerCylinder * _sectorsPerTrack != totalSectors )
    {
        _headsPerCylinder = MaxHeads;
        _sectorsPerTrack = MaxSectors;
    }

} // CHS::CHS(const uint32_t totalSectors)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHS::SectorToChs(const uint32_t logicalBlockAddress) const
// 
// - Converts a Logical Block Address to 3-byte CHS struct representing the Cylinder,Head,Sector address.
//
// - Param: const uint32_t logicalBlockAddress 
//
// - Returns: CHS::Packed
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatImage::CHS::Packed StFormatImage::CHS::SectorToChs( const uint32_t logicalBlockAddress) const
{
    uint32_t cylinder, head;
    uint8_t sector;

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
    if ( logicalBlockAddress > 16450560 )
    {
        cylinder = MaxCylinders-2;
        head     = MaxHeads-1;
        sector  = MaxSectors;
    }
    else
    {
        // Translate the logicalBlockAddress to a Cylinder, Head, Sector address.
        //
        // Reference:
        // "This algorithm can be reversed such that an LBA can be converted to a CHS:
        //  cylinder = LBA / (heads_per_cylinder * sectors_per_track)
        //  temp = LBA % (heads_per_cylinder * sectors_per_track)
        //  head = temp / sectors_per_track
        //  sector = temp % sectors_per_track + 1"
        //
        // http://ata-atapi.com/hiwchs.htm
        //
        cylinder = logicalBlockAddress / (_sectorsPerTrack * _headsPerCylinder);
        sector = ( logicalBlockAddress % _sectorsPerTrack ) + 1;
        head = ( logicalBlockAddress / _sectorsPerTrack ) % _headsPerCylinder;
    }

    return Packed(cylinder, head, sector);

} // CHS::SectorToChs(const uint32_t logicalBlockAddress) const

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHS::Packed::Packed(const uint32_t cylinder, const uint16_t head, const uint8_t  sector)
// 
// - Constructs a 3-byte word representing the Cylinder,Head,Sector address.
//
// - Param: const uint32_t Cylinder 
// - Param: const uint16_t Head
// - Param: const uint8_t  Sector
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatImage::CHS::Packed::Packed(const uint32_t cylinder, const uint16_t head, const uint8_t  sector)
    : byte0((uint8_t) ( head & 0x00FF ))
    , byte1((uint8_t) ((( cylinder & 0x00300 ) >> 2) | ( sector & 0x3F )))
    , byte2((uint8_t) (cylinder & 0x00FF))
{};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MBR IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
// static const uint8_t - Array containing Sigmatel bootstrap code that instructs the BIOS
// to not check the partition table since the device is not a bootable device.
//
//        CLI                         ; disable interrupts
//        JMP      MyBootstrap
//
//        DB        21h,53h,22h,69h,23h,67h,24h,6dh,25h,61h,26h,54h,27h,65h
//        DB        28h,6ch,29h,2ch,2ah,49h,2bh,6eh,2ch,63h,2dh,32h,30h,30h,35h
//MyBootstrap
//
//        XOR      AX,AX
//        MOV     SP,7C00H            ; set the stack pointer
//        MOV     SS,AX               ; set the stack space
//        STI                         ; allow interrupts
//        INT       018H              ; return to BIOS to boot other drives
//Hung    JMP      Hung               ; catcher loop
//        DB        30h,39h,31h,33h
//        END
const uint8_t StFormatImage::MBR::BootstrapCode[] = {
        0xFA,0xEB,0x1D,0x21,0x53,0x22,0x69,0x23, 0x67,0x24,0x6D,0x25,0x61,0x26,0x54,0x27, 
        0x65,0x28,0x6C,0x29,0x2C,0x2A,0x49,0x2B, 0x6E,0x2C,0x63,0x2D,0x32,0x30,0x30,0x35,
        0x33,0xC0,0xBC,0x00,0x7C,0x8E,0xD0,0xFB, 0xCD,0x18,0xEB,0xFE,0x30,0x39,0x31,0x33 };

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MBR::MBR(const StFormatInfo& formatInfo)
//
// - Builds a 512-byte MBR structure
//
// - Param: const StFormatInfo& formatInfo - StFormatInfo object used to initialize MBR fields.
//
// *Note: The StFormatInfo parameter is declared const so this class does not have access to non-const functions in 
//        the StFormatInfo object. This is to prevent changing any StFormatInfo information that would invalidate THIS
//        object. Trying to call a non-const StFormatInfo function is from within this object will produce a compile-time
//        error. An example of the error is shown below:
//
//        error C2662: cannot convert 'this' pointer from 'const StFormatInfo' to 'StFormatInfo &'
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatImage::MBR::MBR(const StFormatInfo& formatInfo)
{
    assert ( sizeof(*this) == 512 );
    memset(this, 0, sizeof(*this));

    CHS chs(formatInfo.GetSectors());

    memcpy(Bootstrap, BootstrapCode, sizeof(BootstrapCode));

    // Setting the partition to NON-BOOTABLE so BIOS does not try to boot it during start-up.
    // TODO: verify the drive still enumerates
    PartitionEntry[0].Status = PartitionRecord::Status_NotActive;

    PartitionEntry[0].StartingCHS = chs.SectorToChs(formatInfo.GetRelativeSector());

    if ( formatInfo.GetFileSystem() == StFormatInfo::FS_FAT12)
        PartitionEntry[0].Type = PartitionRecord::Type_Fat12; // 0x01
    else if ( formatInfo.GetFileSystem() == StFormatInfo::FS_FAT16)
        PartitionEntry[0].Type = PartitionRecord::Type_Fat16; // 0x06
    else
        PartitionEntry[0].Type = PartitionRecord::Type_Fat32; // 0x0B
    
    PartitionEntry[0].LastCHS = chs.SectorToChs(formatInfo.GetSectors()-1);

    PartitionEntry[0].RelativeSector = formatInfo.GetRelativeSector();

    PartitionEntry[0].SectorCount = formatInfo.GetSectors() - formatInfo.GetRelativeSector();

    Signature = 0xAA55;

} // MBR(const StFormatInfo& formatInfo)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PBS, PBS2(FSInfo), PBS3 IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PBS::PBS(const StFormatInfo& formatInfo)
//
// - Builds a 512-byte PBS structure
//
// - Param: const StFormatInfo& formatInfo - StFormatInfo object used to initialize PBS fields. 
//
// *Note: The StFormatInfo parameter is declared const so this class does not have access to non-const functions in 
//        the StFormatInfo object. This is to prevent changing any StFormatInfo information that would invalidate THIS
//        object. Trying to call a non-const StFormatInfo function is from within this object will produce a compile-time
//        error. An example of the error is shown below:
//
//        error C2662: cannot convert 'this' pointer from 'const StFormatInfo' to 'StFormatInfo &'
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatImage::PBS::PBS(const StFormatInfo& formatInfo)
{
    assert ( sizeof(*this) == 512 );
    memset(this, 0, sizeof(*this));

    USES_CONVERSION;

	// jump instruction to boot code.
    BS_jmpBoot[0] = 0xeb;
    BS_jmpBoot[1] = 0x3e;
    BS_jmpBoot[2] = 0x90;

	// MSWIN4.1 is the recommended value from Microsoft white paper on FAT.
    BS_OEMName[0] = 'M';
    BS_OEMName[1] = 'S';
    BS_OEMName[2] = 'W';
    BS_OEMName[3] = 'I';
    BS_OEMName[4] = 'N';
    BS_OEMName[5] = '4';
    BS_OEMName[6] = '.';
    BS_OEMName[7] = '1';

    // Possible values are 512, 1024, 2048, 4096
    // 512 is recommended for maximum compatibility
    // MUST be Page Size of the media.
    BPB_BytsPerSec = formatInfo.GetSectorSize();

    // Possible values are 1, 2, 4, 8, 16, 32, 64, 128
    // BPB_SecPerClus * BPB_BytsPerSec MUST be <= (32 * 1024)
    BPB_SecPerClus = formatInfo.GetSectorsPerCluster();

    // Count of sectors in the reserved region starting with the PBS sector 0
    // Must not be 0
    // FAT12 & FAT16 should not be anything other than 1
    // FAT32 typically 32
	BPB_RsvdSecCnt = formatInfo.PbsSectors();
	
    // Typically 2. Should use 2 for maximum compatibility, but 1 may be used to save space.
    BPB_NumFATs = formatInfo.GetNumFatTables();

    // Number of 32-byte directory entries in the root directory
    // MUST be 0 for FAT32
    // 512 recommended for max compatibility on FAT12
    // BPB_RootEntCnt * 32 % BPB_BytsPerSec MUST = 0 
    BPB_RootEntCnt = formatInfo.GetNumRootEntries();

	// Count of the total sectors on volume starting with Logical Sector 0 (PBS)
	// for FS_FAT32, use BPB_TotSec32 and set BPB_TotSec16 to 0
	// for FS_FAT12, FS_FAT16, if TotSec fits in uint16_t, use BPB_TotSec16 else use BPB_TotSec32
    if ( formatInfo.GetFileSystem() == StFormatInfo::FS_FAT32 )
	{
		// FAT_32 case
        BPB_TotSec16 = 0;
		BPB_TotSec32 = formatInfo.PartitionSectors();
	}
    else
	{
		// FAT_12, FAT_16 case
        if ( formatInfo.PartitionSectors() < 0x10000 )
		{
			BPB_TotSec16 = (uint16_t)formatInfo.PartitionSectors();
			BPB_TotSec32 = 0;
		}
		else
		{
			BPB_TotSec16 = 0;
			BPB_TotSec32 = formatInfo.PartitionSectors();
		}
	}

	// 0xF8 hard drive, 0xF0 removable media
    // MUST match 1st byte of FAT Area1 and FAT Area2
    BPB_Media = MediaTypeFixedDisk;
	
	// Number of sectors occupied by ONE FAT.
	// for FS_FAT32 use BPB_FATSz32 and set BPB_FATSz16 to 0
	// for FS_FAT12, FS_FAT16, use BPB_FATSz16
    if ( formatInfo.GetFileSystem() == StFormatInfo::FS_FAT32 )
	{
		// FAT_32 case
        sFat32.BPB_FATSz32 = formatInfo.FatSectors();
        BPB_FATSz16 = 0;
	}
    else
	{
		// FAT_12, FAT_16 case
        BPB_FATSz16 = (uint16_t)formatInfo.FatSectors();
	}

    CHS chs(formatInfo.GetSectors());
    // sectors per track visible on interrupt 0x13. relevant to media that have a geometry.
    BPB_SecPerTrk = chs.SectorsPerTrack();
	// number of heads visible on interrupt 0x13. relevant to media that have a geometry.
    BPB_NumHeads = chs.HeadsPerCylinder();
	
    // sectors before the start of partition boot sector. these sectors includes the MBR.
    BPB_HiddSec = formatInfo.GetRelativeSector();

 	if ( formatInfo.GetFileSystem() != StFormatInfo::FS_FAT32 )
	{
 	    // FAT_12, FAT_16 case

        // interrupt 0x13 drive number, 0x00 for floppy, 0x80 for hard disks.
        sFat.BS_DrvNum = 0x80;
	    
        // extended boot signature, 0x29 to indicate the following three fields
	    // volid, vollab and filsystype are present.
        sFat.BS_BootSig = 0x29;
        memcpy(sFat.BS_VolID, "1234", 4);
        memset(sFat.BS_VolLab, 0, 11);
		memcpy(sFat.BS_VolLab, SSW2A(formatInfo.GetVolumeLabel()), min(formatInfo.GetVolumeLabel().GetLength(), 11));
 	    if ( formatInfo.GetFileSystem() == StFormatInfo::FS_FAT16 )
    		memcpy(sFat.BS_FilSysType, "FAT16   ", 8);
        else
            memcpy(sFat.BS_FilSysType, "FAT12   ", 8);
    }
    else
    {
		// FAT_32 case

        // This field is only defined for FAT32 media and does not exist on FAT12 and FAT16 media.
		// Bits 0-3	-- Zero-based number of active FAT. Only valid if mirroring is disabled.
		// Bits 4-6	-- Reserved.
		// Bit      7	-- 0 means the FAT is mirrored at runtime into all FATs.
		//				-- 1 means only one FAT is active; it is the one referenced in bits 0-3.
		// Bits 8-15 	-- Reserved.
        sFat32.BPB_ExtFlags = 0;
		
        // FAT document defines it to be 0.
        sFat32.BPB_FSVer = 0;
        
		// This is set to the cluster number of the first cluster of the root directory, 
		// usually 2 but not required to be 2. 
        sFat32.BPB_RootClus = 2;
        
		// Sector number of FSINFO structure (PBS2) in the reserved area of the FAT32 volume. Usually 1.
        sFat32.BPB_FSInfo = 1;

        // If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record.
        // Usually 6. No value other than 6 is recommended.
        sFat32.BPB_BkBootSec = 0;

        // interrupt 0x13 drive number, 0x00 for floppy, 0x80 for hard disks.
        sFat32.BS_DrvNum = 0x80;
	    
        // extended boot signature, 0x29 to indicate the following three fields
	    // volid, vollab and filsystype are present.
        sFat32.BS_BootSig = 0x29;
        memcpy(sFat32.BS_VolID, "1234", 4);
        memset(sFat32.BS_VolLab, 0, 11);
        memcpy(sFat32.BS_VolLab, SSW2A(formatInfo.GetVolumeLabel()), min(formatInfo.GetVolumeLabel().GetLength(), 11));
   		memcpy(sFat32.BS_FilSysType, "FAT32   ", 8);
    }

    BS_Signature = 0xAA55;

} // PBS(const StFormatInfo& formatInfo)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Fat12Entry, Fat16Entry, Fat32Entry, FatTable IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// template<>
// FatTable<StFormatImage::Fat12Entry>::operator 'subscript'(size_t position)
//
// - The Fat12Entry specialization of the subscript operator
//
// - A bit of trickery to fix-up the offset to 1.5 * N instead of 2 * N. Here we just get the return reference 
//   addressing the correct 2 bytes. Which 12-bits gets used is determined by a hack in 
//   Fat12Entry& operator=(const Fat12Entry& rhs).
//
// - Param: size_t position - The position of the Fat12Entry in the FatTable object.
//
// - Returns: StFormatImage::Fat12Entry& - a reference to the 2-byte Fat12Entry indicated by position.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
StFormatImage::Fat12Entry& StFormatImage::FatTable<StFormatImage::Fat12Entry>::operator [](size_t position)
{
    uint8_t * pEntry = (uint8_t*)Table + position + (position / 2);
    return (Fat12Entry&)*pEntry;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// template<typename FatEntryType>
// FatTable<StFormatImage::Fat12Entry>::operator 'subscript'(size_t position)
//
// - The 'unspecialized' implementation of the subscript operator for types Fat16Entry and Fat32Entry.
//
// - No trickery need here since the fields are at least byte aligned.
//
// - Param: size_t position - The position of the FatEntryType in the FatTable object.
//
// - Returns: StFormatImage::FatEntryType& - a reference to the 2/4-byte Fat16/32Entry indicated by position.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename FatEntryType>
FatEntryType& StFormatImage::FatTable<FatEntryType>::operator [](size_t position)
{
    return Table[position];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RootDirEntry, RootDir IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RootDirEntry::MakeVolumeLabel(LPCTSTR volumeLabel)
//
// - Converts the volumeLabel to char* if necessary.
// - Fills in DIR_Name with the first 11-characters of volumeLabel.
// - Pads DIR_Name with ' ' if volumeName is less than 11-characters.
// - Sets DIR_Attr to AttrVolumeId;
//
// - Param: LPCTSTR volumeLabel - The volume label.
//
// - Returns: int32_t error - ERROR_SUCCESS = no error, ERROR_INVALID_PARAMETER = entry is not empty.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t StFormatImage::RootDirEntry::MakeVolumeLabel(LPCTSTR volumeLabel)
{
    USES_CONVERSION;

    CStdString dirName = volumeLabel;

    if ( !IsEmpty() )
        return ERROR_INVALID_PARAMETER;

    // clear the entry
    memset(this, 0, sizeof(*this));

    // copy the volume label into the name field and pad it to 11-chars with spaces
    if ( dirName.GetLength() < 11 )
    {
        memcpy(DIR_Name, SSW2A(dirName), dirName.GetLength());
        memset(&DIR_Name[dirName.GetLength()], 0x20, 11 - dirName.GetLength());
    }
    else
        memcpy(DIR_Name, SSW2A(dirName), 11);

    // Set the attribute field to the VolumeLabel attribute.
    DIR_Attr = AttrVolumeId;

    return ERROR_SUCCESS;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StFormatImage IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatImage (const StFormatInfo& formatInfo)
//
// - Param: const StFormatInfo& formatInfo - StFormatInfo object used to initialize most boot image structures such 
//          as MBR, PBS, ... 
//
// *Note: The StFormatInfo parameter is declared const so this class does not have access to non-const functions in 
//        the StFormatInfo object. This is to prevent changing any StFormatInfo information that would invalidate THIS
//        object. Trying to call a const StFormatInfo function is from within this object will produce a compile-time
//        error. An example of the error is shown below:
//
//        error C2662: cannot convert 'this' pointer from 'const StFormatInfo' to 'StFormatInfo &'
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatImage::StFormatImage(const StFormatInfo& formatInfo)
: _theInfo(formatInfo)
, _lastError(ERROR_SUCCESS)
{
    // How many sectors from absolute 0 to UserData?
    uint32_t imageSectors = formatInfo.GetRelativeSector() + formatInfo.PbsSectors() +
        formatInfo.GetNumFatTables()*_theInfo.FatSectors() + formatInfo.RootDirSectors() +
        (formatInfo.GetFileSystem() == StFormatInfo::FS_FAT32 ? formatInfo.GetSectorsPerCluster() : 0);

    // Convert sectors to bytes
    uint32_t bootImageSize = imageSectors * formatInfo.GetSectorSize();

    // Make a vector big enough to hold the whole image
    std::vector<uint8_t> bootImage(bootImageSize);

    // Keep track of where we are
    uint32_t imageOffset = 0;

    // copy the MBR
    memcpy(&bootImage[0], &MBR(formatInfo), sizeof(MBR));
    memset(&bootImage[sizeof(MBR)], 0, formatInfo.GetSectorSize() - sizeof(MBR));
    imageOffset += formatInfo.GetSectorSize();

    // Pad the Hidden sectors after the MBR before the PBS with 0's
    uint32_t hiddenBytes = (formatInfo.GetRelativeSector() - 1/*MBR*/) * formatInfo.GetSectorSize();
    memset(&bootImage[imageOffset], 0, hiddenBytes);
    imageOffset += hiddenBytes;

    // copy the PBS
    memcpy(&bootImage[imageOffset], &PBS(formatInfo), sizeof(PBS));
    memset(&bootImage[imageOffset+sizeof(PBS)], 0, formatInfo.GetSectorSize() - sizeof(PBS));
    imageOffset += formatInfo.GetSectorSize();

    // if FAT_32, copy PBS2 and PBS3
    if ( formatInfo.GetFileSystem() == StFormatInfo::FS_FAT32 )
    {
        memcpy(&bootImage[imageOffset], &PBS2(), sizeof(PBS2));
        memset(&bootImage[imageOffset+sizeof(PBS2)], 0, formatInfo.GetSectorSize() - sizeof(PBS2));
        imageOffset += formatInfo.GetSectorSize();

        memcpy(&bootImage[imageOffset], &PBS3(), sizeof(PBS3));
        memset(&bootImage[imageOffset+sizeof(PBS3)], 0, formatInfo.GetSectorSize() - sizeof(PBS3));
        imageOffset += formatInfo.GetSectorSize();
    }

    // Copy the FAT table(s)
    for ( uint8_t numFats=0; numFats<formatInfo.GetNumFatTables(); ++numFats )
    {
        uint32_t fatBytes = formatInfo.FatSectors() * formatInfo.GetSectorSize();

        switch ( formatInfo.GetFileSystem() )
        {
        case StFormatInfo::FS_FAT12:
            memcpy(&bootImage[imageOffset], FatTable<Fat12Entry>(formatInfo).Table, fatBytes);
            break;
        case StFormatInfo::FS_FAT16:
            memcpy(&bootImage[imageOffset], FatTable<Fat16Entry>(formatInfo).Table, fatBytes);
            break;
        case StFormatInfo::FS_FAT32:
            memcpy(&bootImage[imageOffset], FatTable<Fat32Entry>(formatInfo).Table, fatBytes);
            break;
        default:
            assert(0);
        }
        imageOffset += fatBytes;
    }

    // Root Directory
    //
    // FAT12, FAT16 case
    uint32_t rootDirBytes = formatInfo.GetNumRootEntries() * sizeof(RootDirEntry);
    if ( rootDirBytes == 0 )
    {
        // FAT32 case
        rootDirBytes = formatInfo.GetSectorsPerCluster() * formatInfo.GetSectorSize();
    }

    // RootDirectory size should be a multilpe of Sector size
    assert( rootDirBytes % formatInfo.GetSectorSize() == 0 );
    
    // Create the Root Directory and copy it into theImage
    RootDir rootDir(rootDirBytes / sizeof(RootDirEntry), formatInfo.GetVolumeLabel());
	if ( rootDir.Table == NULL )
	{
		_lastError = ERROR_NOT_ENOUGH_MEMORY;
		_theImage.erase(_theImage.begin(), _theImage.end());
		return;
	}
	memcpy(&bootImage[imageOffset], rootDir.Table, rootDirBytes);
    imageOffset += rootDirBytes;

    // Double check
    assert ( imageOffset == bootImageSize );

    // store the image in our member variable;
    _theImage = bootImage;

}  // StFormatImage (const StFormatInfo& formatInfo)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddFiles (LPCTSTR filePath)
//
// - Param: LPCTSTR filePath - File name or Directory Name to add. Full path of file or directory needs to be specified.
//                             If filePath is a directory, all files and sub-directories will be added to the image. If
//                             filePath is a file, only the file witll be added to the image. Wildcards are not supported.
//
// - Returns: int32_t error - ERROR_SUCCESS = no error.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t StFormatImage::AddFiles(LPCTSTR filePath)
{
	return ERROR_SUCCESS;
}
