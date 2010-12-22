/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "Libs/Public/StdString.h"
#include "Common/StdInt.h"
#include <assert.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// class StFormatInfo
///
// TODO: We may want to add a  _wastedSectors variable to account for any 'reallocation' we might make based 
// on a CHS calculation. If we can't get an exact CHS solution, should we reduce the allotted sectors in
// order to make the CHS work out perfectly? I don't think an exact CHS is vital since we have accurate 
// LBA variables that support larger media than do the archaic CHS parameters.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StFormatInfo
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StFormatInfo DEFINES
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    static const uint32_t KibiByte = 1024;              // 1,024 (1KB)
    static const uint32_t MebiByte = 1024*1024;         // 1,048,576 (1MB)
    static const uint32_t GibiByte = 1024*1024*1024;    // 1,073,741,824 (1GB)
    static const uint16_t Default_RootDirCount = 1024;  // 1024 - Default number of Root Directory entries.
    static const uint8_t  Default_NumFATS = 2;      // 2 - Default number of FAT tables.
    static const uint32_t Fat16MinClusters = 4085;  // 4,085 - File system determination. "FAT: General Overview of On-Disk Format", Microsoft, v1.02, 5/5/1999, pg.14.
    static const uint32_t Fat32MinClusters = 65525; // 65,525 - File system determination. "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.14.
    static const uint32_t JCsReservedSectors = 0;   // 0 - J.C. Might want to locate something in the hidden sectors after the MBR.

    // File System Type enum. FS_DEFAULT = 0, FS_FAT12 = 3, FS_FAT16 = 4, FS_FAT32 = 8. FS_FATxx * 4 = bits per FAT entry.
    enum FileSystemT { FS_DEFAULT = 0, FS_FAT12 = 3, FS_FAT16 = 4, FS_FAT32 = 8 };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StFormatInfo STRUCT
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    const uint32_t _sectors;           // const uint32_t - The total number of sectors allocated for the data drive
    const uint16_t _sectorSize;        // const uint16_t - Size of a physical media page. 512, 1024, 2048
    FileSystemT    _fileSystem;        // FileSystemT - File System Type. FS_DEFAULT, FS_FAT12, FS_FAT16, FS_FAT32. FS_FATxx * 4 = bits/FAT Table entry.
    CStdString     _volumeLabel;       // CStdString - Volume label. 11-char max. Default = _T("")
    uint32_t       _sectorsPerCluster; // uint32_t - Sectors per cluster.
    uint16_t       _rootEntries;       // uint16_t - Number of Root Directory entries.
    uint8_t        _numFATs;           // uint8_t - Number of FAT Tables. Default = 2;
    uint32_t       _relativeSector;    // uint32_t - Offset from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS)
    int32_t        _lastError;         // int32_t - Extended error information. ERROR_SUCCESS, ERROR_INVALID_PARAMETER

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StFormatInfo Implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    StFormatInfo( const uint32_t sectors, const uint16_t sectorSize, const FileSystemT fileSystem = FS_DEFAULT, 
                  LPCTSTR volumeLabel = _T("") );

    FileSystemT GuessFileSystem(const uint32_t sectors, const uint16_t sectorSize) const;
    FileSystemT CalcFileSystem(const uint32_t clusters) const;

    uint32_t PbsSectors() const;
    uint32_t ClusterSize() const;
    uint32_t PartitionSectors() const;
    uint32_t RootDirSectors() const;
    uint32_t UserDataClusters() const;
    uint32_t FatSectors() const;

    // Returns: uint32_t _sectors. Total number of sectors allocated for the data drive.
    uint32_t GetSectors() const { return _sectors; };

    // Returns: uint16_t _sectorSize. Size of a physical media page. 512, 1024, 2048.
    uint16_t GetSectorSize() const { return _sectorSize; };

    // Returns: FileSystemT _fileSystem. File System Type. FS_DEFAULT, FS_FAT12, FS_FAT16, FS_FAT32. FS_FATxx * 4 = bits/FAT Table entry.
    FileSystemT GetFileSystem() const { return _fileSystem; };

    // Returns: CStdString _volumeLabel. Volume label. 11-char max. Default = _T("")
    CStdString GetVolumeLabel() const { return _volumeLabel; };

    // Sets the volume label.
    // Param: LPCTSTR volumeLabel.  Volume label. 11-char max.
    // Returns: Nothing
	void SetVolumeLabel(LPCTSTR volumeLabel) { _volumeLabel = volumeLabel; };

    // Returns: uint32_t _sectorsPerCluster. Sectors per cluster.
    uint32_t GetSectorsPerCluster() const { return _sectorsPerCluster;};

    // Returns: uint16_t _rootEntries. Number of Root Directory entries.
    uint16_t GetNumRootEntries() const { return _fileSystem == FS_FAT32 ? 0 : _rootEntries; };

    // Returns: uint8_t _numFATs. Number of FAT Tables. Default = 2.
    uint8_t GetNumFatTables() const { return _numFATs; };

    // Returns: uint32_t _relativeSector. Offset in sectors from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS).
    uint32_t GetRelativeSector() const { return _relativeSector; };

    // Returns: int32_t _lastError. Extended error information. ERROR_SUCCESS, ERROR_INVALID_PARAMETER.
    int32_t GetLastError() const { return _lastError; };

    int32_t SetFileSystem(const FileSystemT fileSystem);
    int32_t SetNumRootEntries(const uint16_t rootEntries);
    int32_t SetNumFatTables(const uint8_t numFatTables);
    int32_t SetRelativeSector(const uint32_t relativeSector);
    int32_t SetSectorsPerCluster(const uint32_t sectorsPerCluster);

private:
    int32_t CheckParams();
    uint32_t AlignUserDataArea();
};

