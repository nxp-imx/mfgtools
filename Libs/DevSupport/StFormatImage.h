/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "../../Common/StdString.h"
#include "../../Common/StdInt.h"

#include "StFormatInfo.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class StFormatImage
// 
// - Builds a std::vector<uint8_t> representing the entire boot image from Absolute Sector 0 (MBR) to the beginning
//   of the User Data Region.
//
// - Defines structures for the CHS, MBR, PBS, PBS2(FSInfo), PBS3, Fat12Entry, Fat16Entry, Fat32Entry, FatTable,
//   RootDirectoryEntry, and RootDirectory structures.
//
// - Initialized by: StFormatImage (const StFormatInfo& formatInfo)
//
//   - Param: const StFormatInfo& formatInfo - StFormatInfo object used to initialize most boot image structures such 
//            as MBR, PBS, ... 
//
// *Note: The StFormatInfo parameter is declared const so this class does not have access to non-const functions in 
//        the StFormatInfo object. This is to prevent changing any StFormatInfo information that would invalidate THIS
//        object. Trying to call a non-const StFormatInfo function is from within this object will produce a compile-time
//        error. An example of the error is shown below:
//
//        error C2662: cannot convert 'this' pointer from 'const StFormatInfo' to 'StFormatInfo &'
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)

class StFormatImage
{
    // StFormatImage defines
public:
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct CHS
    //
    // - Holds the _headsPerCylinder and _sectorsPerTrack member variables and a Packed struct used for constructing a
    //   3-byte word representing a Cylinder,Head,Sector (C,H,S) address.
    // - Several functions ar available for converting LBA addresses to CHS address and member variable access.
    // 
    // - Initialized by: CHS(const uint32_t totalSectors)
    //
    //   - Param: const uint32_t totalSectors - The total number of sectors from Absolute Sector 0 (MBR) to the end
    //                                          of the media.
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
	struct CHS
    {
        // CHS RESEARCH
        //
        // "...all CHS values are limited to: 1024 x 255* x 63 = 16,450,560 sectors"
        //
        // "All versions of MS-DOS (including MS-DOS 7 [Windows 95]) have a bug which prevents booting on hard
        //  disks with 256 heads (FFh), so many modern BIOSes provide mappings with at most 255 (FEh) heads."
        //  [INTER61 Copyright©1989-2000 by Ralf Brown].
        //
        //  If the Cylinder value wasn't limited to 1023, then the Last Sector CHS cylinder value could be computed
        //  as follows: (SectorCount - RelativeSector) / (Head(255*) x Sector(63)) = Cylinder.

        // http://www.win.tue.nl/~aeb/linux/Large-Disk-3.html#ss3.1
        // A disk has sectors numbered 0, 1, 2, ... This is called LBA addressing. 
        // In ancient times, before the advent of IDE disks, disks had a geometry described by 
        // three constants C, H, S: the number of cylinders, the number of heads, the number of sectors per track.
        // The address of a sector was given by three numbers: c, h, s: the cylinder number (between 0 and C-1),
        // the head number (between 0 and H-1), and the sector number within the track (between 1 and S),
        // where for some mysterious reason c and h count from 0, but s counts from 1. This is called CHS addressing. 
        //
        // The correspondence between the linear numbering and this 3D notation is as follows: for a disk 
        // with C cylinders, H heads and S sectors/track position (c,h,s) in 3D or CHS notation is the 
        // same as position c*H*S + h*S + (s-1) in linear or LBA notation. 

        // CHS defines
        static const uint16_t MaxCylinders = 1024; // 1024 - Maximum cylinders(or tracks) for computing CHS fields in the MBR.
        static const uint16_t MaxHeads     = 255;  // 255 - Maximum heads/cylinder for computing CHS fields in the MBR. *Note MS bug prevents using 256 heads.
        static const uint8_t  MaxSectors   = 63;   // 63 - Maximum sectors/track for computing CHS fields in the MBR.

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // struct CHS::Packed (3-bytes)
        // 
        // - Represents the Cylinder,Head,Sector (C,H,S) address in a 3-byte word.
        //
        // - Initialized by: Packed(const uint32_t cylinder, const uint16_t head, const uint8_t  sector)
        //   - Param: const uint32_t Cylinder 
        //   - Param: const uint16_t Head
        //   - Param: const uint8_t  Sector
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        struct Packed
        { 
        private:
            // uint8_t - hhhh hhhh - All 8-bits represent Head.
            // 0-based so 0xFF should* translate to 256 maximum representable Heads.
            // * "All versions of MS-DOS (including MS-DOS 7 (Windows 95)) have a bug which prevents
            //    booting on hard disks with 256 heads (FFh), so many modern BIOSes provide mappings
            //    with at most 255 (FEh) heads." (INTER61 Copyright©1989-2000 by Ralf Brown).
            uint8_t byte0;
            // uint8_t - CCss ssss - Least significant 6-bits repersent Sector.
            //           1-based so 0x3F translates to 63 for maximim representable number of Sectors.
            uint8_t byte1;
            // uint8_t - cccc cccc - 10-bits represents Cylinder. 
            //           ( CC cccc cccc ) 2 most significant bits from byte1 and all 8-bits from byte2.
            //           0-based so 0xFFF translates to 1024 maximum representable Cylinders.
            // Bits are arranged (11 2222 2222) to create a 10-bit number. ex. Cylinder = 0x3FF + 1; MaxSectors(1024).
            uint8_t byte2;
        public:
            // Packed() - Default constructor initializes 3-bytes to 0.
            Packed() : byte0(0), byte1(0), byte2(0) {};
            Packed(const uint32_t cylinder, const uint16_t head, const uint8_t  sector);
        };

        // CHS member variables
    private:
        uint16_t  _headsPerCylinder;  // uint16_t - Heads per Cylinder (1-based).
        uint8_t   _sectorsPerTrack;   // uint8_t - Sectors per Track (0-based).
    
        // CHS member functions
    public:
        CHS(const uint32_t totalSectors);
        Packed SectorToChs( const uint32_t logicalBlockAddress) const;
        uint8_t SectorsPerTrack() { return _sectorsPerTrack; };       // - Returns: uint8_t _sectorsPerTrack
        uint16_t HeadsPerCylinder() { return _headsPerCylinder; };    // - Returns: uint16_t _headsPerCylinder
    };

    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct MBR (512-bytes)
    //
    // - Defines the 512-byte MBR structure
    // 
    // - Initialized by: MBR(const StFormatInfo& formatInfo)
    //
    //   - Param: const StFormatInfo& formatInfo - StFormatInfo object used to initialize MBR fields.
    //
    // *Note: The StFormatInfo parameter is declared const so this class does not have access to non-const functions in 
    //        the StFormatInfo object. This is to prevent changing any StFormatInfo information that would invalidate THIS
    //        object. Trying to call a non-const StFormatInfo function is from within this object will produce a compile-time
    //        error. An example of the error is shown below:
    //
    //        error C2662: cannot convert 'this' pointer from 'const StFormatInfo' to 'StFormatInfo &'
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct MBR
    {
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // struct MBR::PartitionRecord (16-bytes)
        // 
        // - Defines the MBR::PartitionRecord of which there are 4 in the MBR structure.
        //
        // - Initialized with zeros in the default constructor.
        // - Initialized with StFormatInfo object in the MBR constructor.
        //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        struct PartitionRecord
        {
            // PartitionRecord Defines
            static const uint8_t Status_Active    = 0x80; // 0x80 - Bootable.
            static const uint8_t Status_NotActive = 0x00; // 0x00 - Non-bootable.
            // Partition Types reference: http://www.win.tue.nl/%7Eaeb/partitions/partition_types-1.html
            static const uint8_t Type_Fat12 = 0x01; // 0x01 - Fat12
            static const uint8_t Type_Fat16 = 0x06; // 0x06 - Fat16
            static const uint8_t Type_Fat32 = 0x0B; // 0x0B - Fat32

            // PartitionRecord Sruct
            uint8_t             Status;             // uint8_t - 0=not active, 0x80=active
            CHS::Packed         StartingCHS;        // CHS::Packed - Logical Sector 0 in CHS format
            uint8_t             Type;               // uint8_t - 01=fat12, 06=fat16, 0B= fat32
            CHS::Packed         LastCHS;            // CHS::Packed - Ending Logical Sector in CHS format
            uint32_t            RelativeSector;     // uint32_t - Logical Sector 0 in LBA format
            uint32_t            SectorCount;        // uint32_t - Logical Sectors from 0 (PBS) to the end of the User Data Region.
            //  * Note: RelativeSector represents the offset in sectors from the MBR ( absolute Sector 0 )
            //           to the PartitionBootSector(PBS) ( logical Sector 0 ). 
            // ** Note: For partitions with less than 16,450,560 sector, the number of sectors repesented by LastCHS
            //          Cylinder * Head * Sector should equal SectorCount - RelativeSector
            
            // PartitionRecord() - Default constructor inits all member data to 0.
            PartitionRecord()
            {
                assert ( sizeof(*this) == 16 );
                memset(this, 0, sizeof(*this));
            };

        }; // MBR::PartitionRecord 16-bytes

        // static const uint8_t[] - Array containing Sigmatel bootstrap code that instructs the BIOS
        // to not check the partition table since the device is not a bootable device.
        static const uint8_t BootstrapCode[];

        // MBR Struct
    private:
        uint8_t                 Bootstrap[446];     // uint8_t[446] - Bootstrap code space.
        PartitionRecord         PartitionEntry[4];  // PartitionRecord[4] - See struct PartitionRecord (4*16)bytes
        uint16_t                Signature;          // uint16_t - 0xAA55

        // MBR Functions
    public:
        MBR(const StFormatInfo& formatInfo);

    }; // MBR 512-bytes
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct PBS Partition Boot Sector (512-bytes)
    //
    // - Defines the 512-byte PBS structure. Necessary for FAT12, FAT16, and FAT32 file systems.
    // 
    // - Initialized by: PBS(const StFormatInfo& formatInfo)
    //
    //   - Param: const StFormatInfo& formatInfo - StFormatInfo object used to initialize PBS fields.
    //
    // *Note: The StFormatInfo parameter is declared const so this class does not have access to non-const functions in 
    //        the StFormatInfo object. This is to prevent changing any StFormatInfo information that would invalidate THIS
    //        object. Trying to call a non-const StFormatInfo function is from within this object will produce a compile-time
    //        error. An example of the error is shown below:
    //
    //        error C2662: cannot convert 'this' pointer from 'const StFormatInfo' to 'StFormatInfo &'
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   struct PBS
    { 
        // PBS Defines
        static const uint8_t MediaTypeFixedDisk = 0xF8; // 0xF8 hard drive, 0xF0 removable media, MUST match FAT[0]
    
    private: 
        // Struct
        uint8_t     BS_jmpBoot[3];      // uint8_t[3] - 0xeb 0x3e 0x90
        uint8_t     BS_OEMName[8];      // uint8_t  - "MSWIN4.1"
        uint16_t    BPB_BytsPerSec;     // uint16_t - 512, 1024, 2048, 4096
        uint8_t     BPB_SecPerClus;     // uint8_t  - 1, 2, 4, 8, 16, 32, 64, 128
        uint16_t    BPB_RsvdSecCnt;     // uint16_t - FAT12/FAT16: 1, FAT32: 3
        uint8_t     BPB_NumFATs;        // uint8_t  - Default = 2
        uint16_t    BPB_RootEntCnt;     // uint16_t - FAT32: 0, FAT12/FAT16: Default = 1024
        uint16_t    BPB_TotSec16;       // uint16_t - Total volume sectors starting with PBS. See BPB_TotSec32.
        uint8_t     BPB_Media;          // uint8_t  - 0xF8 hard drive, 0xF0 removable media, MUST match FAT[0]
        uint16_t    BPB_FATSz16;        // uint16_t - FAT12/16: Number of sectors occupied by 1 FAT. FAT32: 0. See BPB_FATSz32.
        uint16_t    BPB_SecPerTrk;      // uint16_t - Sectors per track visible on interrupt 0x13.
        uint16_t    BPB_NumHeads;       // uint16_t - Number of heads visible on interrupt 0x13.
        uint32_t    BPB_HiddSec;        // uint32_t - Sectors before the start of partition boot sector including the MBR.
        uint32_t    BPB_TotSec32;       // uint32_t - Total volume sectors starting with PBS. See BPB_TotSec16.
        union {
            struct PBS_FAT{
                uint8_t     BS_DrvNum;          // uint8_t - 0x80 for hard disks. Interrupt 0x13 drive number.
                uint8_t     BS_Reserved;        // uint8_t - 0
                uint8_t     BS_BootSig;         // uint8_t - 0x29 indicates the following 3 fields volid, vollab and filsystype are present.
                uint8_t     BS_VolID[4];        // uint8_t - "1234"
                uint8_t     BS_VolLab[11];      // uint8_t - 11-character volume label.
                uint8_t     BS_FilSysType[8];   // uint8_t - FAT12: "FAT12   ", FAT16: "FAT16   "           
                uint8_t     BS_Reserved1[448];  // uint8_t - 0
            }sFat;                              // FAT12, FAT16 ONLY portion of the PBS struct.
            struct PBS_FAT32{
                uint32_t    BPB_FATSz32;        // uint32_t - Number of sectors occupied by 1 FAT.
                uint16_t    BPB_ExtFlags;       // uint16_t - 0 means the FAT is mirrored at runtime into all FATs.
                uint16_t    BPB_FSVer;          // uint16_t - 0
                uint32_t    BPB_RootClus;       // uint32_t - 2 The first cluster of the root directory. 
                uint16_t    BPB_FSInfo;         // uint16_t - 1 Sector number of FSINFO structure (PBS2) in the reserved area of the FAT32 volume.
                uint16_t    BPB_BkBootSec;      // uint16_t - 0 If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record.
                uint8_t     BPB_Reserved[12];   // uint8_t - 0
                uint8_t     BS_DrvNum;          // uint8_t - 0x80 for hard disks. Interrupt 0x13 drive number.
                uint8_t     BS_Reserved1;       // uint8_t - 0
                uint8_t     BS_BootSig;         // uint8_t - 0x29 indicates the following 3 fields volid, vollab and filsystype are present.
                uint8_t     BS_VolID[4];        // uint8_t - "1234"
                uint8_t     BS_VolLab[11];      // uint8_t - 11-character volume label.
                uint8_t     BS_FilSysType[8];   // uint8_t - "FAT32   "
                uint8_t     BS_Reserved3[420];  // uint8_t - 0
            }sFat32;                            // FAT32 ONLY portion of the PBS struct.
        };
        uint16_t    BS_Signature;               // uint16_t - 0xAA55

        // Constructor
    public:
        PBS(const StFormatInfo& formatInfo);

    }; // struct PBS (512-bytes)
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct PBS2 (FSInfo) (512-bytes)
    //
    // - Defines the 512-byte PBS2 structure. Only used FAT32 file systems.
    // 
    // - Initialized by: PBS2()
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct PBS2
    {
        // Struct
    private:
        uint32_t    FSI_LeadSig;            // uint32_t - 0x41615252
        uint8_t     FSI_Reserved1[480];     // uint8_t[480] - 0
        uint32_t    FSI_StrucSig;           // uint32_t - 0x61417272
        uint32_t    FSI_Free_Count;         // uint32_t - 0xFFFFFFFF(unnknown) should be <= Volume Cluster Count
        uint32_t    FSI_Nxt_Free;           // uint32_t - 0xFFFFFFFF(unnknown-start looing at 2) should be <= Volume Cluster Count
        uint8_t     FSI_Reserved2[12];      // uint8_t[12] - 0
        uint32_t    FSI_TrailSig;           // uint32_t - 0xAA550000

    public:
        // PBS2() - Default constructor initializes all member data to constants.
        PBS2()
        {
            assert ( sizeof(*this) == 512 );
            memset(this, 0, sizeof(*this));
            
            FSI_LeadSig = 0x41615252;
            FSI_StrucSig = 0x61417272;
            FSI_Free_Count = 0xFFFFFFFF;
            FSI_Nxt_Free = 0xFFFFFFFF;
            FSI_TrailSig = 0xAA550000;
        };

    }; // struct PBS2 (512-bytes)

    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct PBS3 (512-bytes)
    //
    // - Defines the 512-byte PBS3 structure. Only used FAT32 file systems.
    // 
    // - Initialized by: PBS3()
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct PBS3
    {
        // Struct
    private:
        uint8_t     Reserved[510];  // uint8_t[510] - 0
        uint16_t    Signature;      // uint16_t - 0xAA55
        
    public:
        // PBS3() - Default constructor initializes all member data to constants.
        PBS3()
        {
            assert ( sizeof(*this) == 512 );
            memset(this, 0, sizeof(*this));

            Signature = 0xAA55;
        }

    }; // struct PBS3 (512-bytes)

    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct Fat12Entry (12-bit field in a 2-byte word)
    //
    // - Defines the 12-bit Fat12Entry structure. Special handling for non-byte aligned data.
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct Fat12Entry
    {
        static const uint8_t  Nibbles = 3;                                  // 3 : Nibbles * 4-bits == 12 bit entry size.
        static const uint16_t Entry0 = PBS::MediaTypeFixedDisk + 0x0F00;    // 0x0FF8
        static const uint16_t EOC = 0x0FFF;                                 // 0x0FFF End of cluster chain marker.
        static const uint16_t EocMinimum = 0x0FF8;                          // 0x0FF8-0x0FFF End of cluster chain range.
        static const uint16_t BadCluster = 0x0FF7;                          // 0x0FF7 Bad cluster marker.

        // uint16_t - Each entry is really only 12-bits, but we do not limit our
        // data to 12-bits since we have to read/modify/write the whole
        // 2-bytes so we do not destroy the adjacent entry.
        uint16_t    Entry;
        
        // Fat12Entry() - Default constructor initializes all member data to 0.
        Fat12Entry() : Entry(0) {};
        // Fat12Entry(const uint12_t entry) - Constructor initializes Entry to entry.
        explicit Fat12Entry(const uint16_t entry) : Entry(entry) {};

        //
        // Fat12Entry& operator=(const Fat12Entry& rhs)
        //
        // Play some addressing tricks so we modify the correct 12-bits in a 2-byte word.
        //
        // This feels pretty hackish, but here is the reasoning. In
        // Fat12Entry& StFormatImage::FatTable<StFormatImage::Fat12Entry>::operator 'sub-script'(size_t position)
        // we fixed-up the reference to 1.5 * N instead of 2 * N in the Table of Fat12Entries.
        // ASSUMING THE TABLE of Fat12Entry(s) WOULD START ON AN EVEN ADDRESS or that any single instantiation
        // would be aligned on an EVEN address, we simply see if the THIS pointer is EVEN or ODD. If it is
        // EVEN, we modify the lower 12 bits of our 2-bytes. If the THIS pointer is ODD, we modify
        // the upper 12-bits of our 2-byte word.
        //
        Fat12Entry& operator=(const Fat12Entry& rhs)
        {
            if ( (uint64_t)this % 2 )   // odd
                Entry |= rhs.Entry<<4;
            else                        // even
                Entry |= ( rhs.Entry & 0x0FFF );
            
            return *this;
        };

    }; // struct Fat12Entry (12-bit field in a 2-byte word)
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct Fat16Entry (2-bytes)
    //
    // - Defines the 2-byte Fat16Entry structure.
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct Fat16Entry
    {
        static const uint8_t  Nibbles = 4;                                  // 4 : Nibbles * 4-bits == 16 bit entry size.
        static const uint16_t Entry0 = PBS::MediaTypeFixedDisk + 0xFF00;    // 0xFFF8
        static const uint16_t EOC = 0xFFFF;                                 // 0xFFFF End of cluster chain marker.
        static const uint16_t EocMinimum = 0xFFF8;                          // 0xFFF8-0xFFFF End of cluster chain range.
        static const uint16_t BadCluster = 0xFFF7;                          // 0xFFF7 Bad cluster marker.

        // uint16_t - FAT16 entry
        uint16_t    Entry;
        
        // Fat16Entry() - Default constructor initializes all member data to 0.
        Fat16Entry() : Entry(0) {};
        // Fat16Entry(const uint16_t entry) - Constructor initializes Entry to entry.
        explicit Fat16Entry(const uint16_t entry) : Entry(entry) {};

    }; // struct Fat16Entry (2-bytes)
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct Fat32Entry (4-bytes)
    //
    // - Defines the 4-byte Fat32Entry structure.
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct Fat32Entry
    {
        static const uint8_t  Nibbles = 8;                                  // 8 : Nibbles * 4-bits == 32 bit entry size.
        static const uint32_t Entry0  = PBS::MediaTypeFixedDisk + 0x0FFFFF00; // 0x0FFFFFF8
        static const uint32_t EOC     = 0x0FFFFFFF;                  // 0x0FFFFFFF End of cluster chain marker.
        static const uint32_t EocMinimum = 0x0FFFFFF8;               // 0x0FFFFFF8-0x0FFFFFFF End of cluster chain range.
        static const uint32_t BadCluster = 0x0FFFFFF7;               // 0x0FFFFFF7 Bad cluster marker.

        // uint32_t - FAT32 entry
        uint32_t    Entry;
        
        // Fat32Entry() - Default constructor initializes Entry to 0.
        Fat32Entry() : Entry(0) {};
        // Fat32Entry(const uint32_t entry) - Constructor initializes Entry to entry.
        explicit Fat32Entry(const uint32_t entry) : Entry(entry) {};

    }; // struct Fat32Entry (4-bytes)
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct FatTable<T>
    //
    // - Templated struct taking the FatEntryType: Fat12Entry, Fat16Entry, Fat32Entry as the template parameter.
    //
    // - Clients of this struct should access the FatEntryType* Table member variable.
    //
    // - Defines the 4-byte FatTable structure.
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    template<typename FatEntryType>
    struct FatTable
    {
        // FatEntryType* - pointer to the array of FAT Entries.
        FatEntryType* Table;
        
        //
        // FatTable(const StFormatInfo& formatInfo) : Table(NULL)
        //
        // - Allocates memory for an array of FatEntryTypes and initializes the first 2 entries to default data and the
        //   remaining entries to 0.
        FatTable(const StFormatInfo& formatInfo) : Table(NULL)
        {
            
            // allocate memory for the ALL sectors used by the FAT Table and init it to 0.
            Table = (FatEntryType*) malloc(formatInfo.FatSectors() * formatInfo.GetSectorSize());
            memset(Table, 0, formatInfo.FatSectors() * formatInfo.GetSectorSize());

            // Init the first 2 FAT Entries
            // We use (*this)[] instead of Table[] so our FatTable::operator[] will get called. The
            // FatTable<Fat12Entry>::operator[] is specialized so that it will address the correct Fat12Entry&
            // The general FatTable<FatEntryType>::operator[] simply returns Table[position].
            (*this)[0] = FatEntryType(FatEntryType::Entry0);
            (*this)[1] = FatEntryType(FatEntryType::EOC);

            // If this is a FAT32 FatTable, allocate cluster 2 for the Root Directory
            if ( formatInfo.GetFileSystem() == StFormatInfo::FS_FAT32 )
                (*this)[2] = FatEntryType(FatEntryType::EOC);

        };

        // ~FatTable() - Destroys the allocated Table.
        ~FatTable()
        {
            if (Table)
                free(Table);
        }

        // operator - Specialized for Fat12Entry access
        FatEntryType& operator[](size_t position);

    }; // 4-bytes
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct RootDirEntry (32-bytes)
    //
    // - Defines the 32-byte RootDirEntry structure. Only used in FAT12 and FAT16 file systems.
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct RootDirEntry
    {
        // DIR_Attr defines
        static const uint8_t    AttrReadOnly  = 0x01;    // 0x01 - Read only
        static const uint8_t    AttrHidden    = 0x02;    // 0x02 - Hidden
        static const uint8_t    AttrSystem    = 0x04;    // 0x04 - System
        static const uint8_t    AttrVolumeId  = 0x08;    // 0x08 - Volume ID
        static const uint8_t    AttrDirectory = 0x10;    // 0x10 - Directory
        static const uint8_t    AttrArchive   = 0x20;    // 0x20 - Archive
        static const uint8_t    AttrLongName  = AttrReadOnly | AttrHidden | AttrSystem | AttrVolumeId;    // 0x0F - Long name

        uint8_t     DIR_Name[11]; // uint8_t(11) - Short name.
 
        // uint8_t - File attributes.
        // The upper 2 bits of the attribute byte are reserved and should always be set to 0 when a file is created
        // and never modify or look at it after that.
        uint8_t     DIR_Attr;
        // uint8_t - Reserved. Set to 0 when file is created and never modify or look at it after that.
        uint8_t     DIR_NTRes;
        // uint8_t - Millisecond stamp at file creation time.  This field actually contains a count of tenths of a 
        // second.  The granularity of the seconds part of the DIR_CrtTime is two seconds so this field is a count of 
        // tenths of a second and it's valid value range is 0-199 inclusive.
        uint8_t     DIR_CrtTimeTenth;
        // uint16_t - Time file was created.
        uint16_t    DIR_CrtTime;
        // uint16_t - Date file was created.
        uint16_t    DIR_CrtDate;
        // uint16_t - Last access date.  Note that there is no last access time, only a date.  This is the date of last
        // read or write.  In the case of a write, this should be set to the same date as DIR_WrtDate.
        uint16_t    DIR_LstAccDate;
        // uint16_t - High word of this entry's first cluster number (always 0 for a FAT12 or FAT16 volume).
        uint16_t    DIR_FstClusHI;
        // uint16_t - Time of last write.  Note that file creation is considered a write.
        uint16_t    DIR_WrtTime;
        // uint16_t - Date of last write.  Note that file creation is considered a write.
        uint16_t    DIR_WrtDate;
        // uint16_t - Low word of this entry's first cluster number.  
        uint16_t    DIR_FstClusLO;
        // uint32_t - 32-bit DWORD holding this file's size in bytes.
        uint32_t    DIR_FileSize;
    

        // RootDirEntry() - Default constructor initializes data to 0.
        RootDirEntry()
        {
            assert ( sizeof(*this) == 32 );
            memset(this, 0, sizeof(*this));
        };

        int32_t MakeVolumeLabel(LPCTSTR volumeLabel);

        bool IsEmpty() { return ( DIR_Name[0] == 0xE5 || DIR_Name[0] == 0x00 ); };

    }; // 32-bytes

    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct RootDir
    //
    // - Clients of this struct should access the RootDirEntry* Table member variable.
    //
    // - Initialized by: RootDir(const uin16_t numEntries, const LPCSTR volumeLabel)
    //
    // - Param: const uin16_t numEntries - the number* of Root Directory entries
    //
    //   *Note that numEntries * sizeof(RootDirEntry) MUST be a multiple of formatInfo.GetSectorSize()
    //
    // - Defines the 6-byte RootDir structure. Only used in FAT12 and FAT16 file systems.
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    struct RootDir
    {
        // RootDirEntry* - pointer to the array of Root Directory Entries.
        RootDirEntry* Table;

        const uint16_t _numEntries;

        //
        // RootDir(const StFormatInfo& formatInfo, const LPCSTR volumeLabel)
        //
        // - Allocates memory for an array of RootDirEntries and initializes them to 0.
        // - Sets the first 
        RootDir(const uint16_t numEntries, const LPCTSTR volumeLabel)
            : Table(NULL)
            , _numEntries(numEntries)
        {
            
            // allocate memory for the Root Directory Entries.
            Table = (RootDirEntry*) malloc(_numEntries * sizeof(RootDirEntry));
            if ( Table == NULL )
			{
				return;
			}
			// got memory so set it to 0
			memset(Table, 0, _numEntries * sizeof(RootDirEntry));

			// Create a 'file' in the root directory named "volume label' and set the AttrVolumeId attribute.
			if ( volumeLabel )
			{
				Table[0].MakeVolumeLabel(volumeLabel);
			}
        };

        // ~RootDir() - Destroys the allocated Table.
        ~RootDir()
        {
            if (Table)
                free(Table);
        }

        uint16_t FirstAvailableEntry()
        {
            for ( uint16_t entry=0; entry < _numEntries; ++entry )
            {
                if ( Table[entry].IsEmpty() )
                    return entry;
            }

            // TODO: I think this is a valid RootDirectory number. Probably have to make this more robust when we port
            // this function to the generalized FirstAvailableCluster() that will work in the User Data area including 
            // 'root directory' on FAT32 volumes.
            throw;
            return 0xFFFF;
        }

    }; // 6-bytes

#pragma pack(pop)

public:
    StFormatImage(const StFormatInfo& formatInfo);
	int32_t AddFiles(LPCTSTR filePath);
    // Not currently used.
    // const StFormatInfo - A const copy of the StFormatInfo object used to initialize this StFormatImage object.
    const StFormatInfo _theInfo;

//private:
    // std::vector<uint8_t> - An array of bytes representing the entire boot image from Absolute Sector 0 (MBR) to the 
    //                        beginning of the User Data Region.
    std::vector<uint8_t> _theImage;

private:
	int32_t _lastError;
};
