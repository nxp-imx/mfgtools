#include "StFormatInfo.h"
#include "StFormatImage.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::StFormatInfo( const uint32_t sectors, const uint16_t sectorSize,
//                             const FileSystemT fileSystem = FS_DEFAULT, 
//                             LPCTSTR volumeLabel = _T("") )
//
// - This object contains all the information necessary to build all of the BootImage structures, MBR, PBS, PBS2, PBS3
//   FAT Tables, Root Directory, and CHS.
// - Some of the BootImage object take this object as a parameter in their constructor. Namely, StFormatImage which
//   builds an entire data drive image in its constructor.
// - It is intended that all Format parameters be specified in this object and then use this object to as input for
//   constructing the actual Boot/FAT structures instead of trying to modify the Boot/Fat structures after they have
//   been created. It is much easier to change this object and recreate teh image object than to try and mange changes
//   to the image object.
//
// - Param: const uint32_t sectors - Total number of sectors allocated for the System Data Drive. 
// - Param: const uint16_t sectorSize - Size in bytes of a physical media page.
// - Param: const FileSystemT fileSystem - File system enum. Default = FS_DEFAULT. See GuessFileSystem(). 
// - Param: LPCTSTR volumeLabel - Volume label. 11-char max. Default = _T("").
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatInfo::StFormatInfo( const uint32_t sectors, const uint16_t sectorSize, 
                            const FileSystemT fileSystem, LPCTSTR volumeLabel )
                             : _sectors(sectors)
                             , _sectorSize(sectorSize)
                             , _fileSystem(fileSystem)
                             , _volumeLabel(volumeLabel)
                             , _sectorsPerCluster(0)
                             , _rootEntries(Default_RootDirCount)
                             , _numFATs(Default_NumFATS)
                             , _relativeSector(1/*MBR*/ + JCsReservedSectors)
{
    // TODO: Probably some minimum number of sectors.
    if ( _sectors == 0 ) {
        _lastError = ERROR_INVALID_PARAMETER;
        return;
    }

	// We only support 512, 1024, 2048, and 4096 sector sizes currently.
    switch ( _sectorSize )
    {
        case 512:
        case 1024:
        case 2048:
		case 4096:
            break;
        default:
            _lastError = ERROR_INVALID_PARAMETER;
            return;
    }

    // The number of _rootEntries * sizeof(StFormatImage::RootDirEntry) must be a multiple of _sectorSize
    // based on - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.9.
    if ( _rootEntries * sizeof(StFormatImage::RootDirEntry) % _sectorSize )
    {
        _lastError = ERROR_INVALID_PARAMETER;
        return;
    }

    // TODO: We may want to add a  _wastedSectors variable to account for any 'reallocation' we might make based 
    // on a CHS calculation. If we can't get an exact CHS solution, should we reduce the allotted sectors in
    // order to make the CHS work out perfectly? I don't think an exact CHS is vital since we have accurate 
    // LBA variables that support larger media than do the archaic CHS parameters.
    
    // Guess the filesystem based on Total Size in bytes if it was set to FS_DEFAULT
    if ( _fileSystem == FS_DEFAULT )
        _fileSystem = GuessFileSystem(_sectors, _sectorSize);
retry_calc:
    // Figure out a default cluster size. 
    if ( _sectorsPerCluster == 0 )
    {
        // 32KB is the max cluster size. "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.8.
        // Start with big clusters because it will speed up the device performance since there are fewer clusters/file 
        // to keep track of. This is probably a good assumption for most of our devices since most media files are larger
        // than 32KB. Devices that use primarily smaller files may want to specify a smaller cluster size to reduce 
        // wasted space on the media due to slack.
        _sectorsPerCluster = 32*KibiByte / _sectorSize;
        
        // The starting RelativeSector is 1 for the MBR sector +
        // JCsReservedSectors in case J.C. wants to locate some data in the hidden sectors following the MBR +
        // SectorsPerCluster-1 in case we need to remove some sectors to align the UserDataArea on a cluster boundary.
        _relativeSector = 1/*MBR sector*/ + JCsReservedSectors + (_sectorsPerCluster - 1);

        //
        // TODO: This algorithm may need some work. It probably works ok for default constuctor parameters, but 
        // may break if the File System and/or _sectorsPerCluster are specified.
        // 
        // Since we started with the max cluster size, we can reduce the cluster size to make more clusters until the
        // number of clusters fits into the ranges specified by "FAT: General Overview of On-Disk Format", Microsoft,
        // v1.02, 5/5/1999, pg.14.

        while ( _sectorsPerCluster &&  _fileSystem != CalcFileSystem(UserDataClusters()) )
        {
            // TODO: Really, if UserDataClusters() > _fileSystem.max, we should bail, or change the _fileSystem.
            // Or not. Setting the error and makng the Client decide how to fix the problem may be sufficient.
            //
            // Reduce the sectors/cluster by dividing it by 2. Sectors/cluster is a power of 2 number. This should
            // double the number of clusters.
            _sectorsPerCluster >>= 1;

            // Since we changed the sector/cluster which is used to determine _relativeSector, we need to update
            // _relativeSector before we call UserDataClusters() again.
            _relativeSector = 1/*MBR sector*/ + JCsReservedSectors + (_sectorsPerCluster - 1);
        }
        // If the above algorithm failed to match the file system, just set an error. Clients of the StFormatInfo
        // object should check the _lastError value after construction.
        if ( _sectorsPerCluster == 0 )
        {
			if( _fileSystem == FS_FAT32 )
			{
				_fileSystem = FS_FAT16;
				goto retry_calc;
			}

            _lastError = ERROR_INVALID_PARAMETER;
            return;
        }
    }

    // Adjust the number of empty sectors between Absolute Sector 0 (MBR) and Logical Sector 0 (PBS)
    // such that the first sector of the User Data area after the FAT(s) (and RootDirectory) is on
    // a cluster boundary.
    AlignUserDataArea();

    _lastError = ERROR_SUCCESS;
    
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::CheckParams()
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t StFormatInfo::CheckParams()
{
    // TODO: Probably some minimum number of sectors.
    if ( _sectors == 0 ) {
        _lastError = ERROR_INVALID_PARAMETER;
        return ERROR_INVALID_PARAMETER;
    }

    // We only support 512, 1024, 2048, and 4096 sector sizes currently.
    switch ( _sectorSize )
    {
        case 512:
        case 1024:
        case 2048:
		case 4096:
            break;
        default:
            _lastError = ERROR_INVALID_PARAMETER;
            return ERROR_INVALID_PARAMETER;
    }

    // The number of _rootEntries * sizeof(StFormatImage::RootDirEntry) must be a multiple of _sectorSize
    // based on - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.9.
    if ( _rootEntries * sizeof(StFormatImage::RootDirEntry) % _sectorSize )
    {
        _lastError = ERROR_INVALID_PARAMETER;
        return ERROR_INVALID_PARAMETER;
    }

    // Guess the filesystem based on Total Size in bytes if it was set to FS_DEFAULT
    if ( _fileSystem == FS_DEFAULT )
        _fileSystem = GuessFileSystem(_sectors, _sectorSize);

    if ( _sectorsPerCluster == 0 )
    {
        _lastError = ERROR_INVALID_PARAMETER;
        return ERROR_INVALID_PARAMETER;
    }

    AlignUserDataArea();

    if ( _fileSystem != CalcFileSystem(UserDataClusters()) )
    {
        _lastError = ERROR_INVALID_PARAMETER;
        return ERROR_INVALID_PARAMETER;
    }

    _lastError = ERROR_SUCCESS;
    return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::GuessFileSystem(const uint32_t sectors, const uint16_t sectorSize) const
//
// - Choose initial FileSystem based on the Size of the media. This is totally arbitrary.
//   SigmaTel prefers FAT16 for performance reasons. MS surely uses something different.
// - This is just an initial guess. It needs to be verified once the number of User Data Clusters has been calculated.
//
// - Param: const uint32_t sectors - Total number of sectors allocated for the System Data Drive. 
// - Param: const uint16_t sectorSize - Size in bytes of a physical media page.
//
// - Returns: StFormatInfo::FileSystemT Filesystem enum. FS_FAT12, FS_FAT16, FS_FAT32. Can not 'FAIL'.
//
// TODO: Implement a GuessFileSystemMS() and rename this function to GuessFileSystemST()?
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatInfo::FileSystemT StFormatInfo::GuessFileSystem(const uint32_t sectors, const uint16_t sectorSize) const
{
    uint32_t capacity =  sectors * sectorSize;

    // This is totally arbitrary. SigmaTel prefers FAT16 for performance reasons
    // MS surely uses something different.
    if( capacity <= 2*GibiByte  )
    {
        if ( capacity <= 4*MebiByte  )
        {
            // FAT_12 <= 4MiB
            return FS_FAT12;
        } 
        else
        {
            // 4MB < FAT_16 <= 2GiB
            return FS_FAT16;
        }
	}
    else
    {
		// 2GB < FAT_32 
        return FS_FAT32;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::CalcFileSystem(const uint32_t clusters) const
//
// - Uses count of User Data clusters to determine the FAT filesystem.
//   based on - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.14.
//
// - Param: const uint32_t clusters - Number of User Data clusters.
//
// - Returns: StFormatInfo::FileSystemT Filesystem enum. FS_FAT12, FS_FAT16, FS_FAT32, (Error) FS_DEFAULT
//
// TODO: is there a max number of clusters for FAT32?
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StFormatInfo::FileSystemT StFormatInfo::CalcFileSystem(const uint32_t clusters) const
{
    if ( (1 <= clusters) && (clusters < Fat16MinClusters) )
    {
        return FS_FAT12;
    }
    else if ( (Fat16MinClusters <= clusters) && (clusters < Fat32MinClusters) )
    {
        return FS_FAT16;
    }
    else if ( Fat32MinClusters <= clusters )
    {
        // TODO: is there a max number of clusters for FAT32? 
        return FS_FAT32;
    }

    return FS_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::PbsSectors() const
//
// - Calculates the number of sectors needed for PBS (PBS2) (PBS3).
// - This assumes that there are no reserved sectors following the PBS. This number will be used to populate the 
//   BPB_RsvdSecCnt field in the PBS. Value can not be 0. Value should be 1 for FS_FAT12 and FS_FAT16. Typically 32 for
//   FS_FAT32, but MS OSes will support any non-zero value.
// - Reference - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.8.
// - Theory was to simplify the FAT Image creation by making this fairly constant. User Data cluster alignment is done
//   by adding sectors between Absolute Sector 0 (MBR) and Logical Sector 0 (PBS).
// - Decided to only use 3 sectors for PBS, PBS2 and PBS3 in FAT32 systems instead of the typical 32 to save space. I
//   think the reason for the 32 is to have room for boot strap code to reside, but we do not use this area for that.
//
// - Returns: uint32_t Number of sectors. 3 for FS_FAT32, 1 otherwise.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StFormatInfo::PbsSectors() const
{ 
    return _fileSystem == FS_FAT32 ? 3 : 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::ClusterSize() const
//
// - Calculates the size in bytes of a cluster.
//   Sectors/cluster * bytes/sector
//
// - Returns: uint32_t Number of sectors.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StFormatInfo::ClusterSize() const
{ 
    return _sectorsPerCluster * _sectorSize;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::PartitionSectors() const
//
// - Calculates the number of sectors for the partition.
//   Total allocated sectors for the data drive - the number of sectors from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS)
//
// - Returns: uint32_t Number of sectors.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StFormatInfo::PartitionSectors() const
{ 
    return _sectors - _relativeSector; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::RootDirSectors() const
//
// - Calculates the number of sectors needed for the Root Directory.
// - Zero for FAT32.
// - The number of _rootEntries * sizeof(StFormatImage::RootDirEntry) must be a multiple of _sectorSize
//   based on - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.9.
//
// - Returns: uint32_t Number of sectors.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StFormatInfo::RootDirSectors() const
{
    // RootDirectory size (bytes)
    uint32_t RootDirSizeBytes = GetNumRootEntries() * sizeof(StFormatImage::RootDirEntry);

    // The number of RootEntries * RootEntrySize must be a multiple of SectorSize.
    if ( RootDirSizeBytes % _sectorSize != 0)
        throw;

    // RootDirectory size (sectors)
    return RootDirSizeBytes / _sectorSize;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::UserDataClusters() const
//
// - Number of clusters available for actual file storage.
// - This calculation is rounded UP since it is used to determine the number of FAT Table entries. We MUST have at least
//   enough FAT Table entries for all of the User Data clusters. It is ok if we have more FAT table entries than we have
//   clusters.
//
// - Returns: uint32_t Number of clusters. 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StFormatInfo::UserDataClusters() const
{
    // So, from our total allocation of sectors for the data drive we have to subtract
    // the MBR+HiddenSectors, the PBS sector(s) and the RootDirSectors.
    uint64_t RemainingSectors = _sectors - _relativeSector - PbsSectors() - RootDirSectors();

    // all calculations are done in nibbles (4-bits)
    uint64_t ClusterSizeNibbles = ClusterSize() * 2/*nibbles*/;
    uint64_t FatEntrySizeNibbles = _numFATs/*FATs*/ * _fileSystem/*nibbles/FAT*/;

    uint64_t DataClusterTotalNibbles = ClusterSizeNibbles + FatEntrySizeNibbles;
    // Cluster 0 and 1 are unused, but there are entries in the FAT Table for them
    uint64_t UnusedFatEntriesNibbles = 2 * FatEntrySizeNibbles;

    // clusters = total_nibbles / nibbles_per_cluster
    uint64_t numUserDataClusters = (RemainingSectors * _sectorSize * 2 - UnusedFatEntriesNibbles)/DataClusterTotalNibbles;

    // rounding up
    if ( (RemainingSectors * _sectorSize * 2 - UnusedFatEntriesNibbles) % DataClusterTotalNibbles )
        ++numUserDataClusters;

    return  (uint32_t) numUserDataClusters;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::FatSectors() const
//
// - Number of sectors needed for 1 FAT Table. Rounded up.
//
// - Returns: uint32_t Number of sectors.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StFormatInfo::FatSectors() const
{
    // numSectorsPerOneFAT = Numberof FatEntries * nibblesPerFatEntry / SectorSizeNibbles
    // note that clusters 0 & 1 have entries, but are not accounted for in UserClusters()
    uint32_t numSectorsPerOneFAT = ((UserDataClusters() + 2) * _fileSystem) / (_sectorSize * 2);
    
    // round up
    if ( ((UserDataClusters() + 2) * _fileSystem) % (_sectorSize * 2) )
        ++numSectorsPerOneFAT;

    return numSectorsPerOneFAT;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StFormatInfo::AlignUserDataArea()
//
// - Adjusts number of empty sectors between Absolute Sector 0 (MBR) and Logical Sector 0 (PBS)
//   such that the first sector of the User Data area after the FAT(s) (and RootDirectory) is on
//   a cluster boundary.
// - Possibly changes member variable _relativeSector
//
// - Returns: uint32_t _relativeSector : Number of sectors from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StFormatInfo::AlignUserDataArea()
{
    // If ( SectorsPerCluster == 1 ) there is no need to align anything
    if ( _sectorsPerCluster == 1 )
    {
        // The starting RelativeSector is 1 for the MBR sector +
        // JCsReservedSectors in case J.C. wants to locate some data in the hidden sectors following the MBR.
        _relativeSector = 1/*MBR sector*/ + JCsReservedSectors;
        return _relativeSector;
    }
    else
    {
        // The starting RelativeSector is 1 for the MBR sector +
        // JCsReservedSectors in case J.C. wants to locate some data in the hidden sectors following the MBR +
        // SectorsPerCluster-1 in case we need to remove some sectors to align the UserDataArea on a cluster boundary.
        _relativeSector = 1/*MBR sector*/ + JCsReservedSectors + (_sectorsPerCluster - 1);
    }

    // How many clusters did we start with?
    uint32_t startingClusters = UserDataClusters();

    // How many sectors from absolute 0 to UserData?
    uint32_t offset = _relativeSector + PbsSectors() + _numFATs*FatSectors() + RootDirSectors();
    
    // We want offset to be a multiple of SectorsPerCluster
    uint32_t remainder = offset % _sectorsPerCluster;
    
    if ( remainder )
    {            
        // remainder must be <= SectorsPerCluster-1 which we have built into the RelativeSector number
        // So if we decrease the offset by remainder, the offset should be a multiple of SectorsPerCluster.
        _relativeSector -= remainder;

        // Confirm we are sector aligned
        assert ((_relativeSector + PbsSectors() + _numFATs*FatSectors() + RootDirSectors()) % _sectorsPerCluster == 0 );
        // Confirm we have one more User Data Cluster that we started with
        //TODO: is this really right? having the same number of clusters should be ok too?
        uint32_t endingClusters = UserDataClusters();
        assert ( startingClusters == endingClusters || startingClusters+1 == endingClusters );
        // Confirm we still have all of our necessary space
        assert ( _relativeSector >= 1/*MBR*/ + JCsReservedSectors );
    }        

    return _relativeSector;
};

// Sets the file system.
// Param: const FileSystemT fileSystem.  FileSystem enum. FS_DEFAULT, FS_FAT12, FS_FAT16, FS_FAT32.
// Returns: Nothing
int32_t StFormatInfo::SetFileSystem(const FileSystemT fileSystem)
{ 
    _fileSystem = fileSystem; 
    return CheckParams(); 
};

// Sets the number of Root Directory entries.
// Param: const uint16_t rootEntries.  Number of Root Directory entries.
// Returns: Nothing
int32_t StFormatInfo::SetNumRootEntries(const uint16_t rootEntries)
{ 
    _rootEntries = rootEntries;
    return CheckParams(); 
};

// Sets the number of sectors per cluster.
// Param: const uint32_t sectorsPerCluster.  Sectors per cluster.
// Returns: Nothing
int32_t StFormatInfo::SetSectorsPerCluster(const uint32_t sectorsPerCluster)
{ 
    _sectorsPerCluster = sectorsPerCluster;
    return CheckParams(); 
};

// Sets the number of FAT Tables.
// Param: const uint8_t numFatTables.  Number of FAT Tables.
// Returns: Nothing
int32_t StFormatInfo::SetNumFatTables(const uint8_t numFatTables) 
{ 
    _numFATs = numFatTables;  
    return CheckParams(); 
};

// Sets the offset from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS).
// Param: const uint32_t relativeSector. Number of sectors.
// Returns: Nothing
int32_t StFormatInfo::SetRelativeSector(const uint32_t relativeSector) 
{ 
    _relativeSector = relativeSector;  
    return CheckParams(); 
};

