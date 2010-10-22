/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.ComponentModel;
using System.Runtime.InteropServices;


namespace DevSupport
{
    namespace FormatBuilder
    {
        public class FormatInfo
        {
            //
            // CONSTANTS
            //

            /// <summary> 4,085 - File system determination.</summary>
            const UInt32 KibiByte = 1024;              // 1,024 (1KB)
            /// <summary> 4,085 - File system determination.</summary>
            const UInt32 MebiByte = 1024*1024;         // 1,048,576 (1MB)
            /// <summary> 1,073,741,824 (1GB)</summary>
            const UInt32 GibiByte = 1024*1024*1024;    // 1,073,741,824 (1GB)
            /// <summary> 1024 - Default number of Root Directory entries.</summary>
            const UInt16 Default_RootDirCount = 1024;  // 1024 - Default number of Root Directory entries.
            /// <summary> 2 - Default number of FAT tables.</summary>
            const Byte Default_NumFATS = 2;          // 
            /// <summary> 4,085 - File system determination.</summary>
            /// <remarks> Reference - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.14.</remarks>
            const UInt32 Fat16MinClusters = 4085;
            /// <summary> 65,525 - File system determination.</summary>
            /// <remarks> Reference - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.14.</remarks>
            const UInt32 Fat32MinClusters = 65525;
            /// <summary> 0 - J.C. Might want to locate something in the hidden sectors after the MBR.</summary>
            const UInt32 JCsReservedSectors = 0;

            /// <summary> File System Type enum. Default = 0, Fat12 = 3, Fat16 = 4, Fat32 = 8. Fat?? * 4 = bits per FAT entry.</summary>
            public enum FileSystemType { Default = 0, Fat12 = 3, Fat16 = 4, Fat32 = 8 };

            //
            // FIELDS
            //
            
            /// <summary>The total number of sectors allocated for the data drive.</summary>
            private UInt32         _Sectors;
            /// <summary>Size of a physical media page. 512, 1024, 2048.</summary>
            private UInt16         _SectorSize;
            /// <summary>File System Type. FS_DEFAULT, FS_FAT12, FS_FAT16, FS_FAT32. FS_FATxx * 4 = bits/FAT Table entry.</summary>
            private FileSystemType _FileSystem;
            /// <summary>Volume label. 11-char max. Default = _T("")</summary>
            private String         _VolumeLabel;
            /// <summary>Include MBR upto PBS in image. True will include the MBR and Hidden Sectors between the MBR and PBS. False will create an image that starts with the PBS.</summary>
            private bool           _IncludeMBR;
            /// <summary>Sectors per cluster.</summary>
            private UInt32         _SectorsPerCluster;
            /// <summary>Number of Root Directory entries.</summary>
            private UInt16         _RootEntries;
            /// <summary>Number of FAT Tables. Default = 2.</summary>
            private Byte           _NumFATs;
            /// <summary>Offset from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS).</summary>
            private UInt32         _RelativeSector;

            //
            // PROPERTIES
            //
            
            /// <summary>
            /// readonly The total number of sectors allocated for the data drive.
            /// </summary>
            public UInt32 Sectors { get {return _Sectors; } }
            /// <summary>
            /// readonly The size of a physical media page. 512, 1024, 2048.
            /// </summary>
            public UInt16 SectorSize { get { return _SectorSize; } }
            /// <summary>
            /// The volume label. Labels longer than 11 character may be truncated.
            /// </summary>
            public String VolumeLabel { get { return _VolumeLabel; } set { _VolumeLabel = value; } }
            /// <summary>
            /// Include MBR upto PBS in image. True will include the MBR and Hidden Sectors between the MBR and PBS. False will create an image that starts with the PBS.
            /// </summary>
            public Boolean IncludeMBR { get { return _IncludeMBR; } }
            /// <summary>
            /// The number of sectors per cluster.
            /// </summary>
            public UInt32 SectorsPerCluster { get { return _SectorsPerCluster; } set { _SectorsPerCluster = value; CheckParameters(); } }
            /// <summary>
            /// The File System Type enum. Default = 0, Fat12 = 3, Fat16 = 4, Fat32 = 8. Fat?? * 4 = bits per FAT entry.
            /// </summary>
            public FileSystemType FileSystem { get { return _FileSystem; } set { _FileSystem = value; CheckParameters(); } }
            /// <summary>
            /// The number of Root Directory entries.
            /// </summary>
            public UInt16 RootEntries { get { return _FileSystem == FileSystemType.Fat32 ? (UInt16)0 : _RootEntries; } set { _RootEntries = value; CheckParameters(); } }
            /// <summary>
            /// The number of FAT tables. Default = 2.
            /// </summary>
            public Byte FatTables { get { return _NumFATs; } set { _NumFATs = value; CheckParameters(); } }
            /// <summary>
            /// The Offset in sectors from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS).
            /// </summary>
            public UInt32 RelativeSector { get { return _RelativeSector; } set { _RelativeSector = value; CheckParameters(); }  }
        
            /// <summary>
            /// Constructor for gathering parameters used to create a FAT format image. sectors and sectorSize are required.
            /// fileSystem can be 0 for default and volumeLabel can be null.
            /// </summary>
            /// <param name="sectors">The total number of sectors allocated for the data drive.</param>
            /// <param name="sectorSize">The size of a physical media page. 512, 1024, 2048.</param>
            /// <param name="fileSystem">The File System Type enum. Default = 0, Fat12 = 3, Fat16 = 4, Fat32 = 8. Fat?? * 4 = bits per FAT entry.</param>
            /// <param name="volumeLabel">The volume label. Labels loner than 11 character may be truncated.</param>
            public FormatInfo( UInt32 sectors, UInt16 sectorSize, FileSystemType fileSystem, String volumeLabel, bool includeMBR )
            {
                _Sectors = sectors;
                _SectorSize = sectorSize;
                _FileSystem = fileSystem;
                _IncludeMBR = includeMBR;

                if ( !String.IsNullOrEmpty(volumeLabel) )
                {
                    _VolumeLabel = volumeLabel;
                }
                
                _RootEntries = Default_RootDirCount;
                _NumFATs = Default_NumFATS;
                if ( _IncludeMBR )
                    _RelativeSector = 1/*MBR*/ + JCsReservedSectors;


                // TODO: Probably some minimum number of sectors.
                if ( sectors == 0 )
                {
                    throw new ArgumentOutOfRangeException("sectors", "sectors can not be 0.");
                }

                // We only support 512, 1024, and 2048 sector sizes currently.
                switch ( sectorSize )
                {
                    case 512:
                    case 1024:
                    case 2048:
                    case 4096:
                        break;
                    default:
                        throw new ArgumentOutOfRangeException("sectorSize", "sectorSize must be 512, 1024, 2048 or 4096.");
                }

                // The number of _rootEntries * sizeof(StFormatImage::RootDirEntry) must be a multiple of _sectorSize
                // based on - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.9.
                if ( (_RootEntries * RootDirEntry.Size) % _SectorSize != 0 )
                {
                    throw new ArgumentOutOfRangeException("sectorSize", "The number of RootEntries * sizeof(RootDirEntry) must be a multiple of SectorSize");
                }

                // TODO: We may want to add a  _wastedSectors variable to account for any 'reallocation' we might make based 
                // on a CHS calculation. If we can't get an exact CHS solution, should we reduce the allotted sectors in
                // order to make the CHS work out perfectly? I don't think an exact CHS is vital since we have accurate 
                // LBA variables that support larger media than do the archaic CHS parameters.

                // Guess the filesystem based on Total Size in bytes if it was set to FS_DEFAULT
                if ( _FileSystem == FileSystemType.Default )
                    _FileSystem = GuessFileSystem(sectors, sectorSize);

                // Figure out a default cluster size. 
                if ( _SectorsPerCluster == 0 )
                {
                    // 32KB is the max cluster size. "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.8.
                    // Start with big clusters because it will speed up the device performance since there are fewer clusters/file 
                    // to keep track of. This is probably a good assumption for most of our devices since most media files are larger
                    // than 32KB. Devices that use primarily smaller files may want to specify a smaller cluster size to reduce 
                    // wasted space on the media due to slack.
                    _SectorsPerCluster = 32 * KibiByte / sectorSize;

                    // The starting RelativeSector is 1 for the MBR sector +
                    // JCsReservedSectors in case J.C. wants to locate some data in the hidden sectors following the MBR +
                    // SectorsPerCluster-1 in case we need to remove some sectors to align the UserDataArea on a cluster boundary.
                    if ( _IncludeMBR )
                        _RelativeSector = 1/*MBR sector*/ + JCsReservedSectors + (_SectorsPerCluster - 1);

                    //
                    // TODO: This algorithm may need some work. It probably works ok for default constuctor parameters, but 
                    // may break if the File System and/or _sectorsPerCluster are specified.
                    // 
                    // Since we started with the max cluster size, we can reduce the cluster size to make more clusters until the
                    // number of clusters fits into the ranges specified by "FAT: General Overview of On-Disk Format", Microsoft,
                    // v1.02, 5/5/1999, pg.14.
                    while ( (_SectorsPerCluster != 0) && (_FileSystem != CalcFileSystem(UserDataClusters)) )
                    {
                        // TODO: Really, if UserDataClusters() > _fileSystem.max, we should bail, or change the _fileSystem.
                        // Or not. Setting the error and makng the Client decide how to fix the problem may be sufficient.
                        //
                        // Reduce the sectors/cluster by dividing it by 2. Sectors/cluster is a power of 2 number. This should
                        // double the number of clusters.
                        _SectorsPerCluster >>= 1;

                        // Since we changed the sector/cluster which is used to determine _relativeSector, we need to update
                        // _relativeSector before we call UserDataClusters() again.
                        if (_IncludeMBR)
                            _RelativeSector = 1/*MBR sector*/ + JCsReservedSectors + (_SectorsPerCluster - 1);
                    }
                    // If the above algorithm failed to match the file system, just set an error. Clients of the StFormatInfo
                    // object should check the _lastError value after construction.
                    if (_SectorsPerCluster == 0)
                    {
                        throw new Exception("Could not compute a valid number of Sectors per Cluster.");
                    }
                }

                // Adjust the number of empty sectors between Absolute Sector 0 (MBR) and Logical Sector 0 (PBS)
                // such that the first sector of the User Data area after the FAT(s) (and RootDirectory) is on
                // a cluster boundary.
                AlignUserDataArea();

                //
                // SUCCESS
                //
                return;
            }

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // StFormatInfo::CheckParameters()
            //
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            private void CheckParameters()
            {
                // TODO: Probably some minimum number of sectors.
                if ( _Sectors == 0 ) 
                {
                    throw new Exception("Sectors must be greater than 0.");
                }

                // We only support 512, 1024, and 2048 sector sizes currently.
                switch ( _SectorSize )
                {
                    case 512:
                    case 1024:
                    case 2048:
                    case 4096:
                        break;
                    default:
                        throw new Exception("SectorSize must be 512, 1024, 2048 or 4096.");
                }

                // The number of _rootEntries * sizeof(StFormatImage::RootDirEntry) must be a multiple of _sectorSize
                // based on - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.9.
                if ( (_RootEntries * RootDirEntry.Size) % _SectorSize  != 0 )
                {
                    throw new Exception("The number of RootEntries * sizeof(RootDirEntry) must be a multiple of SectorSize");
                }

                // Guess the filesystem based on Total Size in bytes if it was set to FileSystemType.Default
                if (_FileSystem == FileSystemType.Default)
                {
                    _FileSystem = GuessFileSystem(_Sectors, _SectorSize);
                }

                if ( _SectorsPerCluster == 0 )
                {
                    throw new Exception("Sectors per Cluster must be greater than 0.");
                }

                AlignUserDataArea();

                if ( _FileSystem != CalcFileSystem(UserDataClusters) )
                {
                    throw new Exception(String.Format("Invalid filesystem for {0} clusters.", UserDataClusters));
                }
                
                //
                // SUCCESS
                //
                return;
            }

            /// <summary>
            /// GuessFileSystem(UInt32 sectors, UInt16 sectorSize)
            /// - Choose initial FileSystem based on the Size of the media. This is totally arbitrary.
            ///   SigmaTel prefers FAT16 for performance reasons. MS surely uses something different.
            /// - This is just an initial guess. It needs to be verified once the number of User Data Clusters has been calculated.
            /// </summary>
            /// <param name="sectors">
            /// UInt32 - Total number of sectors allocated for the System Data Drive
            /// </param>
            /// <param name="sectorSize">
            /// UInt16 - Size in bytes of a physical media page.
            /// </param>
            /// <returns>
            /// FileSystemType - Filesystem enum. FileSystemType.Fat12, FileSystemType.Fat16, FileSystemType.Fat32. Can not 'FAIL'.
            /// </returns>
            private FileSystemType GuessFileSystem(UInt32 sectors, UInt16 sectorSize)
            {
                UInt32 capacity =  sectors * sectorSize;

                // This is totally arbitrary. SigmaTel prefers FAT16 for performance reasons
                // MS surely uses something different.
                if( capacity <= 2*GibiByte  )
                {
                    if ( capacity <= 4*MebiByte  )
                    {
                        // FAT_12 <= 4MiB
                        return FileSystemType.Fat12;
                    } 
                    else
                    {
                        // 4MB < FAT_16 <= 2GiB
                        return FileSystemType.Fat16;
                    }
	            }
                else
                {
		            // 2GB < FAT_32 
                    return FileSystemType.Fat32;
	            }
            }

            /// <summary>
            /// CalcFileSystem(UInt32 clusters)
            /// - Uses count of User Data clusters to determine the FAT filesystem.
            ///   based on - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.14.
            /// </summary>
            /// <param name="clusters">
            /// UInt32 - The number of User Data clusters
            /// </param>
            /// <returns>
            /// FileSystemType - Filesystem enum. FileSystemType.Fat12, FileSystemType.Fat16, FileSystemType.Fat32. (Error) FileSystemType.Default.
            /// </returns>
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            private FileSystemType CalcFileSystem(UInt32 clusters)
            {
                if ( (1 <= clusters) && (clusters < Fat16MinClusters) )
                {
                    return FileSystemType.Fat12;
                }
                else if ( (Fat16MinClusters <= clusters) && (clusters < Fat32MinClusters) )
                {
                    return FileSystemType.Fat16;
                }
                else if ( Fat32MinClusters <= clusters )
                {
                    // TODO: is there a max number of clusters for FAT32? 
                    return FileSystemType.Fat32;
                }

                return FileSystemType.Default;
            }

            /// <summary>
            /// - The number of sectors needed for PBS (PBS2) (PBS3).
            /// - This assumes that there are no reserved sectors following the PBS. This number will be used to populate the 
            ///   BPB_RsvdSecCnt field in the PBS. Value can not be 0. Value should be 1 for FS_FAT12 and FS_FAT16. Typically 32 for
            ///   FS_FAT32, but MS OSes will support any non-zero value.
            /// - Reference - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.8.
            /// - Theory was to simplify the FAT Image creation by making this fairly constant. User Data cluster alignment is done
            ///   by adding sectors between Absolute Sector 0 (MBR) and Logical Sector 0 (PBS).
            /// - Decided to only use 3 sectors for PBS, PBS2 and PBS3 in FAT32 systems instead of the typical 32 to save space. I
            ///   think the reason for the 32 is to have room for boot strap code to reside, but we do not use this area for that.
            ///
            /// - Returns: UInt32 Number of sectors. 3 for FileSystemType.Fat32, 1 otherwise.
            /// </summary>
           public UInt32 NumPbsSectors { get { return (UInt32)(_FileSystem == FileSystemType.Fat32 ? 3 : 1); } }

            /// <summary>
            /// - The size in bytes of a cluster.
            ///   -> NumBytes = Sectors/cluster * bytes/sector
            /// </summary>
            public UInt32 ClusterSize { get { return _SectorsPerCluster * _SectorSize; } }

            /// <summary>
            /// - The number of sectors for the partition.
            ///   -> Total allocated sectors for the data drive - the number of sectors from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS).
            /// </summary>
            public UInt32 PartitionSectors { get { return _Sectors - _RelativeSector; } }

            /// <summary>
            /// - The number of sectors needed for the Root Directory.
            /// - The number of _rootEntries * sizeof(StFormatImage::RootDirEntry) must be a multiple of _sectorSize
            ///   based on - "FAT: General Overview of On-Disk Format", Microsoft,v1.02, 5/5/1999, pg.9.
            /// </summary>
            public UInt32 NumRootDirSectors
            { 
                get
                {
                    // RootDirectory size (bytes)
                    UInt32 rootDirSizeBytes = _RootEntries * (UInt32)RootDirEntry.Size;

                    // The number of RootEntries * RootEntrySize must be a multiple of SectorSize.
                    if (rootDirSizeBytes % _SectorSize != 0)
                    {
                        throw new Exception("The number of RootEntries * RootEntrySize must be a multiple of SectorSize.");
                    }

                    // RootDirectory size (sectors)
                    return rootDirSizeBytes / _SectorSize;
                }
            }

            /// <summary>
            /// - The number of clusters available for actual file storage. Rounded up.
            /// - This calculation is rounded UP since it is used to determine the number of FAT Table entries. We MUST have at least
            ///   enough FAT Table entries for all of the User Data clusters. It is ok if we have more FAT table entries than we have
            ///   clusters.
            /// </summary>
            public UInt32 UserDataClusters
            {
                get
                {
                    // So, from our total allocation of sectors for the data drive we have to subtract
                    // the MBR+HiddenSectors, the PBS sector(s) and the RootDirSectors.
                    UInt64 remainingSectors = _Sectors - _RelativeSector - NumPbsSectors - NumRootDirSectors;

                    // all calculations are done in nibbles (4-bits)
                    UInt64 clusterSizeNibbles = ClusterSize * 2/*nibbles*/;
                    UInt64 fatEntrySizeNibbles = _NumFATs/*FATs*/ * (UInt32)_FileSystem/*nibbles/FAT*/;

                    UInt64 dataClusterTotalNibbles = clusterSizeNibbles + fatEntrySizeNibbles;
                    // Cluster 0 and 1 are unused, but there are entries in the FAT Table for them
                    UInt64 unusedFatEntriesNibbles = 2 * fatEntrySizeNibbles;

                    // clusters = total_nibbles / nibbles_per_cluster
                    UInt64 numUserDataClusters = (remainingSectors * _SectorSize * 2 - unusedFatEntriesNibbles)/dataClusterTotalNibbles;

                    // rounding up
                    if ((remainingSectors * _SectorSize * 2 - unusedFatEntriesNibbles) % dataClusterTotalNibbles != 0 )
                    {
                        ++numUserDataClusters;
                    }

                    return  (UInt32)numUserDataClusters;
                }
            }

            /// <summary>
            /// - The number of sectors needed for 1 FAT Table. Rounded up.
            /// </summary>
            public UInt32 NumFatSectors
            {
                get
                {
                    // numSectorsPerOneFAT = Numberof FatEntries * nibblesPerFatEntry / SectorSizeNibbles
                    // note that clusters 0 & 1 have entries, but are not accounted for in UserClusters()
                    UInt32 numSectorsPerOneFAT = (UInt32)(((UserDataClusters + 2) * (UInt32)_FileSystem) / (_SectorSize * 2));
                    
                    // round up
                    if (((UserDataClusters + 2) * (UInt32)_FileSystem) % (_SectorSize * 2) != 0)
                    {
                        ++numSectorsPerOneFAT;
                    }

                    return numSectorsPerOneFAT;
                }
            }

            /// <summary>
            /// - Adjusts number of empty sectors between Absolute Sector 0 (MBR) and Logical Sector 0 (PBS)
            ///   such that the first sector of the User Data area after the FAT(s) (and RootDirectory) is on
            ///   a cluster boundary.
            /// - Possibly changes member variable _relativeSector
            /// </summary>
            /// <returns>The Number of sectors from Absolute Sector 0 (MBR) to Logical Sector 0 (PBS)</returns>
            private UInt32 AlignUserDataArea()
            {
                // No room for adjustment if we are not including the MBR and Hidden Sectors.
                if (_IncludeMBR)
                    return _RelativeSector;
    
                // If ( SectorsPerCluster == 1 ) there is no need to align anything
                if ( _SectorsPerCluster == 1 )
                {
                    // The starting RelativeSector is 1 for the MBR sector +
                    // JCsReservedSectors in case J.C. wants to locate some data in the hidden sectors following the MBR.
                    _RelativeSector = 1/*MBR sector*/ + JCsReservedSectors;

                    return _RelativeSector;
                }
                else
                {
                    // The starting RelativeSector is 1 for the MBR sector +
                    // JCsReservedSectors in case J.C. wants to locate some data in the hidden sectors following the MBR +
                    // SectorsPerCluster-1 in case we need to remove some sectors to align the UserDataArea on a cluster boundary.
                    _RelativeSector = 1/*MBR sector*/ + JCsReservedSectors + (_SectorsPerCluster - 1);
                }

                // How many clusters did we start with?
                UInt32 startingClusters = UserDataClusters;

                // How many sectors from absolute 0 to UserData?
                UInt32 offset = _RelativeSector + NumPbsSectors + _NumFATs*NumFatSectors + NumRootDirSectors;
                
                // We want offset to be a multiple of SectorsPerCluster
                UInt32 remainder = offset % _SectorsPerCluster;
                
                if ( remainder != 0 )
                {            
                    // remainder must be <= SectorsPerCluster-1 which we have built into the RelativeSector number
                    // So if we decrease the offset by remainder, the offset should be a multiple of SectorsPerCluster.
                    _RelativeSector -= remainder;

                    // Confirm we are sector aligned
                    Trace.Assert((_RelativeSector + NumPbsSectors + _NumFATs * NumFatSectors + NumRootDirSectors) % _SectorsPerCluster == 0,
                        "Image is not sector aligned.");

                    // Confirm we have one more User Data Cluster that we started with
                    //TODO: is this really right? having the same number of clusters should be ok too?
                    UInt32 endingClusters = UserDataClusters;
                    Trace.Assert(startingClusters == endingClusters || startingClusters + 1 == endingClusters,
                        "The image has the wrong number of clusters.");
                    // Confirm we still have all of our necessary space
                    Trace.Assert(_RelativeSector >= 1/*MBR*/ + JCsReservedSectors, "Image does not have all required elements.");
                }        

                return _RelativeSector;
            }

            public override string ToString()
            {
                String classDesc = base.ToString() + "\n";
                classDesc += String.Format("ClusterSize = 0x{0:X} ({0}) ({1} sectors)\n", ClusterSize, ClusterSize / SectorSize);
                classDesc += String.Format("Number of FAT Tables = {0}\n", FatTables);
                classDesc += String.Format("FileSystem = {0}\n", FileSystem.ToString());
                classDesc += String.Format("Sectors per FAT = {0}\n", NumFatSectors);
                classDesc += String.Format("Sectors for PBS = {0}\n", NumPbsSectors);
                classDesc += String.Format("Sectors for RootDir = {0}\n", NumRootDirSectors);
                classDesc += String.Format("Partition Sectors = {0}\n", PartitionSectors);
                classDesc += String.Format("Sectors = {0}\n", Sectors);
                classDesc += String.Format("Sector size = {0}\n", SectorSize);
                classDesc += String.Format("Sectors per Cluster = {0}\n", SectorsPerCluster);
                classDesc += String.Format("User Data Clusters = {0}\n", UserDataClusters);
                classDesc += String.Format("Volume label = {0}\n", VolumeLabel);
                
                return classDesc;
            }
        }

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
        
	    public class CHS
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
            public const UInt16 MaxCylinders = 1024; // 1024 - Maximum cylinders(or tracks) for computing CHS fields in the MBR.
            public const UInt16 MaxHeads     = 255;  // 255 - Maximum heads/cylinder for computing CHS fields in the MBR. *Note MS bug prevents using 256 heads.
            public const Byte   MaxSectors   = 63;   // 63 - Maximum sectors/track for computing CHS fields in the MBR.

            public Byte SectorsPerTrack { get { return _SectorsPerTrack; } }       // - Returns: uint8_t _sectorsPerTrack
            public UInt16 HeadsPerCylinder { get { return _HeadsPerCylinder; } }   // - Returns: uint16_t _headsPerCylinder
            // CHS member variables
            private UInt16  _HeadsPerCylinder;  // uint16_t - Heads per Cylinder (1-based).
            private Byte   _SectorsPerTrack;    // uint8_t - Sectors per Track (0-based).

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
            public class Packed
            {
                /// <summary>
                /// The size in bytes of the structure (3 bytes).
                /// </summary>
                public const int Size = 3;
                
                // uint8_t - hhhh hhhh - All 8-bits represent Head.
                // 0-based so 0xFF should* translate to 256 maximum representable Heads.
                // * "All versions of MS-DOS (including MS-DOS 7 (Windows 95)) have a bug which prevents
                //    booting on hard disks with 256 heads (FFh), so many modern BIOSes provide mappings
                //    with at most 255 (FEh) heads." (INTER61 Copyright©1989-2000 by Ralf Brown).
                private Byte byte0;
                // uint8_t - CCss ssss - Least significant 6-bits repersent Sector.
                //           1-based so 0x3F translates to 63 for maximim representable number of Sectors.
                private Byte byte1;
                // uint8_t - cccc cccc - 10-bits represents Cylinder. 
                //           ( CC cccc cccc ) 2 most significant bits from byte1 and all 8-bits from byte2.
                //           0-based so 0xFFF translates to 1024 maximum representable Cylinders.
                // Bits are arranged (11 2222 2222) to create a 10-bit number. ex. Cylinder = 0x3FF + 1; MaxSectors(1024).
                private Byte byte2;

                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // CHS.Packed.Packed(UInt32 cylinder, UInt16 head, Byte sector)
                // 
                // - Constructs a 3-byte word representing the Cylinder,Head,Sector address.
                //
                // - Param: const UInt32 Cylinder 
                // - Param: const UInt16 Head
                // - Param: const Byte  Sector
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                public Packed(UInt32 cylinder, UInt16 head, Byte sector)
                {
                    byte0 = (Byte)(head & 0x00FF);
                    byte1 = (Byte)((Byte)((cylinder & 0x00300) >> 2) | (sector & 0x3F));
                    byte2 = ((Byte)(cylinder & 0x00FF));
                }

                public Byte[] ToByteArray()
                {
                    Byte[] chsPackedImage = new Byte[CHS.Packed.Size];
                    chsPackedImage[0] = byte0;
                    chsPackedImage[1] = byte1;
                    chsPackedImage[2] = byte2;

                    Trace.Assert(chsPackedImage.Length == CHS.Packed.Size, "The Byte[] is not the correct size.");

                    return chsPackedImage;
                }
            };

            // CHS member functions
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // CHS(UInt32 totalSectors)
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
            public CHS(UInt32 totalSectors)
            {
                // Initialize member variables
                _HeadsPerCylinder = 1;
                _SectorsPerTrack = 0;

                // Local variables
                UInt32 cylinder = 0;

                // Doesn't really matter how we determine CHS
                // Just try to make something that addresses all the sectors
                for ( _SectorsPerTrack = MaxSectors; _SectorsPerTrack >= 1; --_SectorsPerTrack )
                {
                    for ( _HeadsPerCylinder = MaxHeads; _HeadsPerCylinder >= 1; --_HeadsPerCylinder )
                    {
                        for ( cylinder = 1; cylinder <= MaxCylinders; ++ cylinder )
                        {
                            if ( cylinder * _HeadsPerCylinder * _SectorsPerTrack == totalSectors )
                                break;
                        }
                        if ( cylinder * _HeadsPerCylinder * _SectorsPerTrack == totalSectors )
                            break;

                    }
                    if ( cylinder * _HeadsPerCylinder * _SectorsPerTrack == totalSectors )
                        break;
                }
                
                // If the above algorith failed, just initialize members to max values.
                if ( cylinder * _HeadsPerCylinder * _SectorsPerTrack != totalSectors )
                {
                    _HeadsPerCylinder = MaxHeads;
                    _SectorsPerTrack = MaxSectors;
                }

            } // CHS.CHS(UInt32 totalSectors)

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // CHS.SectorToChs(UInt32 logicalBlockAddress)
            // 
            // - Converts a Logical Block Address to 3-byte CHS struct representing the Cylinder,Head,Sector address.
            //
            // - Param: UInt32 logicalBlockAddress 
            //
            // - Returns: CHS.Packed
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            public CHS.Packed SectorToChs( UInt32 logicalBlockAddress)
            {
                UInt32 cylinder;
                UInt16 head;
                Byte sector;

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
                    cylinder = (UInt32)(logicalBlockAddress / (_SectorsPerTrack * _HeadsPerCylinder));
                    sector = (Byte)(( logicalBlockAddress % _SectorsPerTrack ) + 1);
                    head = (UInt16)(( logicalBlockAddress / _SectorsPerTrack ) % _HeadsPerCylinder);
                }

                return new CHS.Packed(cylinder, head, sector);

            } // CHS.SectorToChs(const uint32_t logicalBlockAddress) const

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
        public class MBR
        {
            /// <summary>
            /// The size in bytes of the structure (512 bytes).
            /// </summary>
            public const int Size = 512;
            /// <summary>
            /// The size in bytes of the sector containing the structure.
            /// </summary>
            public readonly int SectorSize;

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // struct MBR.PartitionRecord (16-bytes)
            // 
            // - Defines the MBR.PartitionRecord of which there are 4 in the MBR structure.
            //
            // - Initialized with zeros in the default constructor.
            // - Initialized with StFormatInfo object in the MBR constructor.
            //
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            public class PartitionRecord
            {
                /// <summary>
                /// The size in bytes of the structure (16 bytes).
                /// </summary>
                public const int Size = 16;

                // PartitionRecord Defines
                public enum PartitionStatus : byte
                {
                    Active = 0x80, // 0x80 - Bootable.
                    NotActive = 0x00  // 0x00 - Non-bootable.
                }
                // Partition Types reference: http://www.win.tue.nl/%7Eaeb/partitions/partition_types-1.html
                public enum PartitionType : byte
                {
                    Fat12 = 0x01, // 0x01 - Fat12
                    Fat16 = 0x06, // 0x06 - Fat16
                    Fat32 = 0x0B  // 0x0B - Fat32
                }

                // PartitionRecord Struct
                private PartitionStatus Status;  // Byte - 0=not active, 0x80=active
                private CHS.Packed StartingCHS;  // CHS.Packed - Logical Sector 0 in CHS format
                private PartitionType Type;      // Byte - 01=fat12, 06=fat16, 0B= fat32
                private CHS.Packed LastCHS;      // CHS.Packed - Ending Logical Sector in CHS format
                private UInt32 RelativeSector;   // UInt32 - Logical Sector 0 in LBA format
                private UInt32 SectorCount;      // UInt32 - Logical Sectors from 0 (PBS) to the end of the User Data Region.
                //  * Note: RelativeSector represents the offset in sectors from the MBR ( absolute Sector 0 )
                //           to the PartitionBootSector(PBS) ( logical Sector 0 ). 
                // ** Note: For partitions with less than 16,450,560 sector, the number of sectors repesented by LastCHS
                //          Cylinder * Head * Sector should equal SectorCount - RelativeSector

                public PartitionRecord()
                {
                    StartingCHS = new CHS.Packed(0, 0, 0);
                    LastCHS = new CHS.Packed(0, 0, 0);
                }
                public PartitionRecord(UInt32 totalSectors, UInt32 relativeSector, FormatInfo.FileSystemType fileSystem)
                {
                    // Setting the partition to NON-BOOTABLE so BIOS does not try to boot it during start-up.
                    Status = PartitionRecord.PartitionStatus.NotActive;

                    CHS chs = new CHS(totalSectors);
                    StartingCHS = chs.SectorToChs(relativeSector);

                    if (fileSystem == FormatInfo.FileSystemType.Fat12)
                    {
                        Type = PartitionRecord.PartitionType.Fat12; // 0x01
                    }
                    else if (fileSystem == FormatInfo.FileSystemType.Fat16)
                    {
                        Type = PartitionRecord.PartitionType.Fat16; // 0x06
                    }
                    else
                    {
                        Type = PartitionRecord.PartitionType.Fat32; // 0x0B
                    }

                    LastCHS = chs.SectorToChs(totalSectors - 1);

                    RelativeSector = relativeSector;

                    SectorCount = totalSectors - relativeSector;
                }

                public Byte[] ToByteArray()
                {
                    List<Byte> partitionImage = new List<Byte>(PartitionRecord.Size);
                    partitionImage.Add((Byte)Status);
                    partitionImage.AddRange(StartingCHS.ToByteArray());
                    partitionImage.Add((Byte)Type);
                    partitionImage.AddRange(LastCHS.ToByteArray());
                    partitionImage.AddRange(BitConverter.GetBytes(RelativeSector));
                    partitionImage.AddRange(BitConverter.GetBytes(SectorCount));

                    Trace.Assert(partitionImage.Count == PartitionRecord.Size, "The Byte[] is not the correct size.");
                    
                    return partitionImage.ToArray();
                }
            } // MBR.PartitionRecord 16-bytes

            // static const uint8_t[] - Array containing Sigmatel bootstrap code that instructs the BIOS
            // to not check the partition table since the device is not a bootable device.
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
            //
            // const Byte[] BootstrapCode = {
            //        0xFA,0xEB,0x1D,0x21,0x53,0x22,0x69,0x23, 0x67,0x24,0x6D,0x25,0x61,0x26,0x54,0x27, 
            //        0x65,0x28,0x6C,0x29,0x2C,0x2A,0x49,0x2B, 0x6E,0x2C,0x63,0x2D,0x32,0x30,0x30,0x35,
            //        0x33,0xC0,0xBC,0x00,0x7C,0x8E,0xD0,0xFB, 0xCD,0x18,0xEB,0xFE,0x30,0x39,0x31,0x33 };

            // MBR Struct
            Byte[]            Bootstrap;       // Byte[446] - Bootstrap code space.
            PartitionRecord[] PartitionEntry;  // PartitionRecord[4] - See struct PartitionRecord (4*16)bytes
            UInt16            Signature;       // UInt16 - 0xAA55

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
            public MBR(FormatInfo formatInfo)
            {
                Byte[] BootstrapCode = new Byte[] {
                                    0xFA,0xEB,0x1D,0x21,0x53,0x22,0x69,0x23, 0x67,0x24,0x6D,0x25,0x61,0x26,0x54,0x27, 
                                    0x65,0x28,0x6C,0x29,0x2C,0x2A,0x49,0x2B, 0x6E,0x2C,0x63,0x2D,0x32,0x30,0x30,0x35,
                                    0x33,0xC0,0xBC,0x00,0x7C,0x8E,0xD0,0xFB, 0xCD,0x18,0xEB,0xFE,0x30,0x39,0x31,0x33 };
                
                SectorSize = formatInfo.SectorSize;

                Bootstrap = new Byte[446];
                BootstrapCode.CopyTo(Bootstrap, 0);

                PartitionEntry = new PartitionRecord[4];
                PartitionEntry[0] = new PartitionRecord(formatInfo.Sectors, formatInfo.RelativeSector, formatInfo.FileSystem);
                PartitionEntry[1] = new PartitionRecord();
                PartitionEntry[2] = new PartitionRecord();
                PartitionEntry[3] = new PartitionRecord();

                Signature = 0xAA55;

            } // MBR(FormatInfo formatInfo)

            public Byte[] ToByteArray()
            {
                List<Byte> mbrImage = new List<Byte>(MBR.Size);

                // Add the Bootstap bytes
                mbrImage.AddRange(Bootstrap);

                // Add the PartitionRecord Entries
                foreach (PartitionRecord p in PartitionEntry)
                    mbrImage.AddRange(p.ToByteArray());

                // Add the Signature
                mbrImage.AddRange(BitConverter.GetBytes(Signature));

                Trace.Assert(mbrImage.Count == MBR.Size, "The Byte[] is not the correct size.");

                // Pad the rest of the sector with 0's
                if (SectorSize > MBR.Size)
                {
                    Byte[] zeros = new Byte[SectorSize - MBR.Size];
                    mbrImage.AddRange(zeros);
                }

                Trace.Assert(mbrImage.Count == SectorSize, "The Byte[] is not the correct size.");

                return mbrImage.ToArray();
            }
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
       public class PBS
        { 
            /// <summary>
            /// The size in bytes of the structure (512 bytes).
            /// </summary>
            public const int Size = 512;
            /// <summary>
            /// The size in bytes of the sector containing the structure.
            /// </summary>
            public readonly int SectorSize;
            /// <summary>
            /// The file system Fat12, Fat16, Fat32.
            /// </summary>
            public readonly FormatInfo.FileSystemType FileSystem;
            
           // PBS Defines
           /// <summary>
           /// 0xF8 hard drive, 0xF0 removable media, MUST match FAT[0]
           /// </summary>
           public const Byte MediaTypeFixedDisk = 0xF8;
        
            // Struct
            private Byte[]   BS_jmpBoot;       // uint8_t[3] - 0xeb 0x3e 0x90
            private Byte[]   BS_OEMName;      // uint8_t[8]  - "MSWIN4.1"
            private UInt16   BPB_BytsPerSec;     // uint16_t - 512, 1024, 2048, 4096
            private Byte     BPB_SecPerClus;     // uint8_t  - 1, 2, 4, 8, 16, 32, 64, 128
            private UInt16   BPB_RsvdSecCnt;     // uint16_t - FAT12/FAT16: 1, FAT32: 3
            private Byte     BPB_NumFATs;        // uint8_t  - Default = 2
            private UInt16   BPB_RootEntCnt;     // uint16_t - FAT32: 0, FAT12/FAT16: Default = 1024
            private UInt16   BPB_TotSec16;       // uint16_t - Total volume sectors starting with PBS. See BPB_TotSec32.
            private Byte     BPB_Media;          // uint8_t  - 0xF8 hard drive, 0xF0 removable media, MUST match FAT[0]
            private UInt16   BPB_FATSz16;        // uint16_t - FAT12/16: Number of sectors occupied by 1 FAT. FAT32: 0. See BPB_FATSz32.
            private UInt16   BPB_SecPerTrk;      // uint16_t - Sectors per track visible on interrupt 0x13.
            private UInt16   BPB_NumHeads;       // uint16_t - Number of heads visible on interrupt 0x13.
            private UInt32   BPB_HiddSec;        // uint32_t - Sectors before the start of partition boot sector including the MBR.
            private UInt32   BPB_TotSec32;       // uint32_t - Total volume sectors starting with PBS. See BPB_TotSec16.

//           struct PBS_FAT // FAT12, FAT16 ONLY portion of the PBS struct.
//           {
                private Byte     BS_DrvNum;          // uint8_t - 0x80 for hard disks. Interrupt 0x13 drive number.
//                private Byte     BS_Reserved;        // uint8_t - 0
                private Byte     BS_BootSig;         // uint8_t - 0x29 indicates the following 3 fields volid, vollab and filsystype are present.
                private Byte[]   BS_VolID;        // uint8_t - [4]"1234"
                private Byte[]   BS_VolLab;      // uint8_t - [11]-character volume label.
                private Byte[]   BS_FilSysType;   // uint8_t - [8] - FAT12: "FAT12   ", FAT16: "FAT16   "           
//                private Byte[]   BS_Reserved1;  // uint8_t - [448] - 0
                
//           }
//           struct PBS_FAT32 // FAT32 ONLY portion of the PBS struct.
//           {
                private UInt32    BPB_FATSz32;        // uint32_t - Number of sectors occupied by 1 FAT.
                private UInt16    BPB_ExtFlags;       // uint16_t - 0 means the FAT is mirrored at runtime into all FATs.
                private UInt16    BPB_FSVer;          // uint16_t - 0
                private UInt32    BPB_RootClus;       // uint32_t - 2 The first cluster of the root directory. 
                private UInt16    BPB_FSInfo;         // uint16_t - 1 Sector number of FSINFO structure (PBS2) in the reserved area of the FAT32 volume.
                private UInt16    BPB_BkBootSec;      // uint16_t - 0 If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record.
//                private Byte[]     BPB_Reserved;   // uint8_t[12] - 0
//                private Byte     BS_DrvNum;          // uint8_t - 0x80 for hard disks. Interrupt 0x13 drive number.
//                private Byte     BS_Reserved1;       // uint8_t - 0
//                private Byte     BS_BootSig;         // uint8_t - 0x29 indicates the following 3 fields volid, vollab and filsystype are present.
//                private Byte[]     BS_VolID;        // uint8_t[4] - "1234"
//                private Byte[]     BS_VolLab;      // uint8_t[11] - 11-character volume label.
//                private Byte[]     BS_FilSysType;   // uint8_t[8] - "FAT32   "
//                private Byte[]     BS_Reserved3;  // uint8_t[420] - 0
//           }

           private UInt16    BS_Signature;               // uint16_t - 0xAA55

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
            public PBS(FormatInfo formatInfo)
            {
                // Create an ASCII encoding.
                ASCIIEncoding ascii = new ASCIIEncoding();

                // Init these member variables for use in ToArray()
                SectorSize = formatInfo.SectorSize;
                FileSystem = formatInfo.FileSystem;

                // jump instruction to boot code.
                BS_jmpBoot = new Byte[3];
                BS_jmpBoot[0] = 0xeb;
                BS_jmpBoot[1] = 0x3e;
                BS_jmpBoot[2] = 0x90;

	            // MSWIN4.1 is the recommended value from Microsoft white paper on FAT.
                Char[] oemName = { 'M', 'S', 'W', 'I', 'N', '4', '.', '1' };
                BS_OEMName = new Byte[8];
                BS_OEMName = ascii.GetBytes(oemName, 0, 8);

                // Possible values are 512, 1024, 2048, 4096
                // 512 is recommended for maximum compatibility
                // MUST be Page Size of the media.
                BPB_BytsPerSec = formatInfo.SectorSize;

                // Possible values are 1, 2, 4, 8, 16, 32, 64, 128
                // BPB_SecPerClus * BPB_BytsPerSec MUST be <= (32 * 1024)
                BPB_SecPerClus = (Byte)formatInfo.SectorsPerCluster;

                // Count of sectors in the reserved region starting with the PBS sector 0
                // Must not be 0
                // FAT12 & FAT16 should not be anything other than 1
                // FAT32 typically 32
	            BPB_RsvdSecCnt = (UInt16)formatInfo.NumPbsSectors;
            	
                // Typically 2. Should use 2 for maximum compatibility, but 1 may be used to save space.
                BPB_NumFATs = formatInfo.FatTables;

                // Number of 32-byte directory entries in the root directory
                // MUST be 0 for FAT32
                // 512 recommended for max compatibility on FAT12
                // BPB_RootEntCnt * 32 % BPB_BytsPerSec MUST = 0 
                BPB_RootEntCnt = formatInfo.RootEntries;

	            // Count of the total sectors on volume starting with Logical Sector 0 (PBS)
	            // for FS_FAT32, use BPB_TotSec32 and set BPB_TotSec16 to 0
	            // for FS_FAT12, FS_FAT16, if TotSec fits in uint16_t, use BPB_TotSec16 else use BPB_TotSec32
                if ( formatInfo.FileSystem == FormatInfo.FileSystemType.Fat32 )
	            {
		            // FAT_32 case
                    // BPB_TotSec16 = 0; set to zero by default
		            BPB_TotSec32 = formatInfo.PartitionSectors;
	            }
                else
	            {
		            // FAT_12, FAT_16 case
                    if ( formatInfo.PartitionSectors < 0x10000 )
		            {
                        BPB_TotSec16 = (UInt16)formatInfo.PartitionSectors;
                        // BPB_TotSec32 = 0; set to zero by default
		            }
		            else
		            {
                        // BPB_TotSec16 = 0; set to zero by default
			            BPB_TotSec32 = formatInfo.PartitionSectors;
		            }
	            }

	            // 0xF8 hard drive, 0xF0 removable media
                // MUST match 1st byte of FAT Area1 and FAT Area2
                BPB_Media = MediaTypeFixedDisk;
            	
	            // Number of sectors occupied by ONE FAT.
	            // for FS_FAT32 use BPB_FATSz32 and set BPB_FATSz16 to 0
	            // for FS_FAT12, FS_FAT16, use BPB_FATSz16
                if ( formatInfo.FileSystem == FormatInfo.FileSystemType.Fat32 )
	            {
		            // FAT_32 case
                    BPB_FATSz32 = formatInfo.NumFatSectors;
                    // BPB_FATSz16 = 0; set to zero by default
	            }
                else
	            {
		            // FAT_12, FAT_16 case
                    BPB_FATSz16 = (UInt16)formatInfo.NumFatSectors;
	            }

                CHS chs = new CHS(formatInfo.Sectors);
                // sectors per track visible on interrupt 0x13. relevant to media that have a geometry.
                BPB_SecPerTrk = chs.SectorsPerTrack;
	            // number of heads visible on interrupt 0x13. relevant to media that have a geometry.
                BPB_NumHeads = chs.HeadsPerCylinder;
            	
                // sectors before the start of partition boot sector. these sectors includes the MBR.
                BPB_HiddSec = formatInfo.RelativeSector;

                // interrupt 0x13 drive number, 0x00 for floppy, 0x80 for hard disks.
                BS_DrvNum = 0x80;

                // extended boot signature, 0x29 to indicate the following three fields
                // volid, vollab and filsystype are present.
                BS_BootSig = 0x29;

                Char[] volId = { '1', '2', '3', '4' };
                BS_VolID = new Byte[4];
                BS_VolID = ascii.GetBytes(volId, 0, 4);

                // copy the volume label into the name field and pad it to 11-chars with spaces
                BS_VolLab = new Byte[11];
                if (formatInfo.VolumeLabel.Length < 11)
                {
                    ascii.GetBytes(formatInfo.VolumeLabel.PadRight(11, ' '), 0, 11, BS_VolLab, 0);
                }
                else
                {
                    ascii.GetBytes(formatInfo.VolumeLabel, 0, 11, BS_VolLab, 0);
                }

                Char[] fileSysType;
                if (formatInfo.FileSystem == FormatInfo.FileSystemType.Fat12)
 	                fileSysType = new Char[8] { 'F', 'A', 'T', '1', '2', ' ', ' ', ' ' };
                else if (formatInfo.FileSystem == FormatInfo.FileSystemType.Fat16)
 	                fileSysType = new Char[8] { 'F', 'A', 'T', '1', '6', ' ', ' ', ' ' };
                else
 	                fileSysType = new Char[8] { 'F', 'A', 'T', '3', '2', ' ', ' ', ' ' };

                BS_FilSysType = new Byte[8];
                BS_FilSysType = ascii.GetBytes(fileSysType, 0, 8);

                if ( formatInfo.FileSystem == FormatInfo.FileSystemType.Fat32 )
                {
		            // FAT_32 case

                    // This field is only defined for FAT32 media and does not exist on FAT12 and FAT16 media.
		            // Bits 0-3	-- Zero-based number of active FAT. Only valid if mirroring is disabled.
		            // Bits 4-6	-- Reserved.
		            // Bit      7	-- 0 means the FAT is mirrored at runtime into all FATs.
		            //				-- 1 means only one FAT is active; it is the one referenced in bits 0-3.
		            // Bits 8-15 	-- Reserved.
                    // BPB_ExtFlags = 0; set to zero by default
            		
                    // FAT document defines it to be 0.
                    // BPB_FSVer = 0; set to zero by default
                    
		            // This is set to the cluster number of the first cluster of the root directory, 
		            // usually 2 but not required to be 2. 
                    BPB_RootClus = 2;
                    
		            // Sector number of FSINFO structure (PBS2) in the reserved area of the FAT32 volume. Usually 1.
                    BPB_FSInfo = 1;

                    // If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record.
                    // Usually 6. No value other than 6 is recommended.
                    // BPB_BkBootSec = 0; set to zero by default
                }

                BS_Signature = 0xAA55;

            } // PBS(const StFormatInfo& formatInfo)

            public Byte[] ToByteArray()
            {
                List<Byte> pbsImage = new List<Byte>(PBS.Size);

                pbsImage.AddRange(BS_jmpBoot);
                pbsImage.AddRange(BS_OEMName);
                pbsImage.AddRange(BitConverter.GetBytes(BPB_BytsPerSec));
                pbsImage.Add(BPB_SecPerClus);
                pbsImage.AddRange(BitConverter.GetBytes(BPB_RsvdSecCnt));
                pbsImage.Add(BPB_NumFATs);
                pbsImage.AddRange(BitConverter.GetBytes(BPB_RootEntCnt));
                pbsImage.AddRange(BitConverter.GetBytes(BPB_TotSec16));
                pbsImage.Add(BPB_Media);
                pbsImage.AddRange(BitConverter.GetBytes(BPB_FATSz16));
                pbsImage.AddRange(BitConverter.GetBytes(BPB_SecPerTrk));
                pbsImage.AddRange(BitConverter.GetBytes(BPB_NumHeads));
                pbsImage.AddRange(BitConverter.GetBytes(BPB_HiddSec));
                pbsImage.AddRange(BitConverter.GetBytes(BPB_TotSec32));

                if ( FileSystem != FormatInfo.FileSystemType.Fat32 )
                {
                    pbsImage.Add(BS_DrvNum);
                    pbsImage.Add(new Byte());
                    pbsImage.Add(BS_BootSig);
                    pbsImage.AddRange(BS_VolID);
                    pbsImage.AddRange(BS_VolLab);
                    pbsImage.AddRange(BS_FilSysType);
                    pbsImage.AddRange(new Byte[448]);
                }
                else
                {
                    pbsImage.AddRange(BitConverter.GetBytes(BPB_FATSz32));
                    pbsImage.AddRange(BitConverter.GetBytes(BPB_ExtFlags));
                    pbsImage.AddRange(BitConverter.GetBytes(BPB_FSVer));
                    pbsImage.AddRange(BitConverter.GetBytes(BPB_RootClus));
                    pbsImage.AddRange(BitConverter.GetBytes(BPB_FSInfo));
                    pbsImage.AddRange(BitConverter.GetBytes(BPB_BkBootSec));
                    pbsImage.AddRange(new Byte[12]);
                    pbsImage.Add(BS_DrvNum);
                    pbsImage.Add(new Byte());
                    pbsImage.Add(BS_BootSig);
                    pbsImage.AddRange(BS_VolID);
                    pbsImage.AddRange(BS_VolLab);
                    pbsImage.AddRange(BS_FilSysType);
                    pbsImage.AddRange(new Byte[420]);
                }

                pbsImage.AddRange(BitConverter.GetBytes(BS_Signature));

                Trace.Assert(pbsImage.Count == PBS.Size, "The Byte[] is not the correct size.");
                
                // Pad the rest of the sector with 0's
                if ( SectorSize > PBS.Size )
                {
                    Byte[] zeros = new Byte[SectorSize - PBS.Size];
                    pbsImage.AddRange(zeros);
                }

                Trace.Assert(pbsImage.Count == SectorSize, "The Byte[] is not the correct size.");

                if (FileSystem == FormatInfo.FileSystemType.Fat32)
                {
                    pbsImage.AddRange(new PBS2().ToByteArray(SectorSize));
                    pbsImage.AddRange(new PBS3().ToByteArray(SectorSize));
                }

                return pbsImage.ToArray();
            }

        } // class PBS (512-bytes)
        
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // struct PBS2 (FSInfo) (512-bytes)
        //
        // - Defines the 512-byte PBS2 structure. Only used FAT32 file systems.
        // 
        // - Initialized by: PBS2()
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        public class PBS2
        {
            /// <summary>
            /// The size in bytes of the structure (512 bytes).
            /// </summary>
            public const int Size = 512;

            private UInt32   FSI_LeadSig;            // uint32_t - 0x41615252
            private Byte[]   FSI_Reserved1;          // uint8_t[480] - 0
            private UInt32   FSI_StrucSig;           // uint32_t - 0x61417272
            private UInt32   FSI_Free_Count;         // uint32_t - 0xFFFFFFFF(unnknown) should be <= Volume Cluster Count
            private UInt32   FSI_Nxt_Free;           // uint32_t - 0xFFFFFFFF(unnknown-start looing at 2) should be <= Volume Cluster Count
            private Byte[]   FSI_Reserved2;          // uint8_t[12] - 0
            private UInt32   FSI_TrailSig;           // uint32_t - 0xAA550000

            // PBS2() - Default constructor initializes all member data to constants.
            public PBS2()
            {
                FSI_LeadSig = 0x41615252;
                FSI_Reserved1 = new Byte[480];
                FSI_StrucSig = 0x61417272;
                FSI_Free_Count = 0xFFFFFFFF;
                FSI_Nxt_Free = 0xFFFFFFFF;
                FSI_Reserved2 = new Byte[12];
                FSI_TrailSig = 0xAA550000;
            }

            public Byte[] ToByteArray(int sectorSize)
            {
                List<Byte> pbs2Image = new List<Byte>(PBS2.Size);
                pbs2Image.AddRange(BitConverter.GetBytes(FSI_LeadSig));
                pbs2Image.AddRange(FSI_Reserved1);
                pbs2Image.AddRange(BitConverter.GetBytes(FSI_StrucSig));
                pbs2Image.AddRange(BitConverter.GetBytes(FSI_Free_Count));
                pbs2Image.AddRange(BitConverter.GetBytes(FSI_Nxt_Free));
                pbs2Image.AddRange(FSI_Reserved2);
                pbs2Image.AddRange(BitConverter.GetBytes(FSI_TrailSig));

                Trace.Assert(pbs2Image.Count == PBS2.Size, "The Byte[] is not the correct size.");
                
                // Pad the rest of the sector with 0's
                if ( sectorSize > PBS2.Size )
                {
                    Byte[] zeros = new Byte[sectorSize - PBS2.Size];
                    pbs2Image.AddRange(zeros);
                }

                Trace.Assert(pbs2Image.Count == sectorSize, "The Byte[] is not the correct size.");

                return pbs2Image.ToArray();
            }
        } // class PBS2 (512-bytes)

        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // struct PBS3 (512-bytes)
        //
        // - Defines the 512-byte PBS3 structure. Only used FAT32 file systems.
        // 
        // - Initialized by: PBS3()
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        public class PBS3
        {
            /// <summary>
            /// The size in bytes of the structure (512 bytes).
            /// </summary>
            public const int Size = 512;
            /// <summary>
            /// The signature of the structure.
            /// </summary>
            public const UInt16 Signature = 0xAA55;

            public Byte[] ToByteArray(int sectorSize)
            {
                List<Byte> pbs3Image = new List<Byte>(PBS3.Size);
                pbs3Image.AddRange(new Byte[510]);                      // Reserved
                pbs3Image.AddRange(BitConverter.GetBytes(Signature));

                Trace.Assert(pbs3Image.Count == PBS3.Size, "The Byte[] is not the correct size.");
                
                // Pad the rest of the sector with 0's
                if ( sectorSize > PBS3.Size )
                {
                    Byte[] zeros = new Byte[sectorSize - PBS3.Size];
                    pbs3Image.AddRange(zeros);
                }

                Trace.Assert(pbs3Image.Count == sectorSize, "The Byte[] is not the correct size.");

                return pbs3Image.ToArray();
            }

        } // class PBS3 (512-bytes)

        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // struct Fat12Entry (12-bit field in a 2-byte word)
        //
        // - Defines the 12-bit Fat12Entry structure. Special handling for non-byte aligned data.
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        abstract public class FatxxEntry 
        {
            abstract public Byte[] ToByteArray();
        }

        public class Fat12Entry : FatxxEntry
        {
            public const Byte   Nibbles = 3;                                  // 3 : Nibbles * 4-bits == 12 bit entry size.
            public const UInt16 Entry0 = PBS.MediaTypeFixedDisk + 0x0F00;     // 0x0FF8
            public const UInt16 EOC = 0x0FFF;                                 // 0x0FFF End of cluster chain marker.
            public const UInt16 EocMinimum = 0x0FF8;                          // 0x0FF8-0x0FFF End of cluster chain range.
            public const UInt16 BadCluster = 0x0FF7;                          // 0x0FF7 Bad cluster marker.

            // uint16_t - Each entry is really only 12-bits, but we do not limit our
            // data to 12-bits since we have to read/modify/write the whole
            // 2-bytes so we do not destroy the adjacent entry.
            private UInt16 Entry;

            // Fat12Entry() - Default constructor initializes Entry to 0.
            public Fat12Entry() { }
            // Fat12Entry(UInt16 entry) - Constructor initializes Entry to entry.
            public Fat12Entry(UInt16 entry)
            {
                Entry = entry;
            }

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
//            Fat12Entry& operator=(const Fat12Entry& rhs)
//            {
//                if ( (uint64_t)this % 2 )   // odd
//                    Entry |= rhs.Entry<<4;
//                else                        // even
//                    Entry |= ( rhs.Entry & 0x0FFF );
//                
//                return *this;
//            }
            override public Byte[] ToByteArray()
            {
                return BitConverter.GetBytes(Entry);
            }

        } // class Fat12Entry (12-bit field in a 2-byte word)
        
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // struct Fat16Entry (2-bytes)
        //
        // - Defines the 2-byte Fat16Entry structure.
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        public class Fat16Entry : FatxxEntry
        {
            public const Byte   Nibbles = 4;                                  // 4 : Nibbles * 4-bits == 16 bit entry size.
            public const UInt16 Entry0 = PBS.MediaTypeFixedDisk + 0xFF00;     // 0xFFF8
            public const UInt16 EOC = 0xFFFF;                                 // 0xFFFF End of cluster chain marker.
            public const UInt16 EocMinimum = 0xFFF8;                          // 0xFFF8-0xFFFF End of cluster chain range.
            public const UInt16 BadCluster = 0xFFF7;                          // 0xFFF7 Bad cluster marker.

            // uint16_t - FAT16 entry
            private UInt16 Entry;
            
            // Fat16Entry() - Default constructor initializes all member data to 0.
            public Fat16Entry(){}
            // Fat16Entry(const uint16_t entry) - Constructor initializes Entry to entry.
            public Fat16Entry(UInt16 entry)
            {
                Entry = entry;
            }

            override public Byte[] ToByteArray()
            {
                return BitConverter.GetBytes(Entry);
            }

        } // class Fat16Entry (2-bytes)
        
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // struct Fat32Entry (4-bytes)
        //
        // - Defines the 4-byte Fat32Entry structure.
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        public class Fat32Entry : FatxxEntry
        {
            public const Byte  Nibbles = 8;                                  // 8 : Nibbles * 4-bits == 32 bit entry size.
            public const UInt32 Entry0 = PBS.MediaTypeFixedDisk + 0x0FFFFF00; // 0x0FFFFFF8
            public const UInt32 EOC = 0x0FFFFFFF;                  // 0x0FFFFFFF End of cluster chain marker.
            public const UInt32 EocMinimum = 0x0FFFFFF8;               // 0x0FFFFFF8-0x0FFFFFFF End of cluster chain range.
            public const UInt32 BadCluster = 0x0FFFFFF7;               // 0x0FFFFFF7 Bad cluster marker.

            // uint32_t - FAT32 entry
            private UInt32 Entry;
            
            // Fat32Entry() - Default constructor initializes Entry to 0.
            public Fat32Entry(){}
            // Fat32Entry(UInt32 entry) - Constructor initializes Entry to entry.
            public Fat32Entry(UInt32 entry)
            {
                Entry = entry;
            }

            override public Byte[] ToByteArray()
            {
                return BitConverter.GetBytes(Entry);
            }

        } // struct Fat32Entry (4-bytes)
        
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // struct FatTable<T>
        //
        // - Templated struct taking the FatEntryType: Fat12Entry, Fat16Entry, Fat32Entry as the template parameter.
        //
        // - Clients of this struct should access the FatEntryType* Table member variable.
        //
        // - Defines the 4-byte FatTable structure.
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//        template<typename FatEntryType>
        public class FatTable
        {
            /// <summary>
            /// The size in bytes of the sector containing the structure.
            /// </summary>
            public readonly int SectorSize;
            /// <summary>
            /// The file system Fat12, Fat16, Fat32.
            /// </summary>
            public readonly FormatInfo.FileSystemType FileSystem;

            // FatEntryType* - pointer to the array of FAT Entries.
            private List<FatxxEntry> Table = new List<FatxxEntry>();

            // - Allocates memory for an array of FatEntryTypes and initializes the first 2 entries to default data and the
            //   remaining entries to 0.
            public FatTable(FormatInfo formatInfo)
            {
                SectorSize = formatInfo.SectorSize;
                FileSystem = formatInfo.FileSystem;

                switch ( FileSystem )
                {
                    case FormatInfo.FileSystemType.Fat12:
                        // Make 2 "used" FAT entries. 
                        // Make "unused" FAT Entries for all UserDataClusters.
                        Table.Add(new Fat12Entry(Fat12Entry.Entry0));
                        Table.Add(new Fat12Entry(Fat12Entry.EOC));
                        for (int index = 2; index < formatInfo.UserDataClusters; ++index)
                            Table[index] = new Fat12Entry();
                        break;
                    
                    case FormatInfo.FileSystemType.Fat16:
                        // Make 2 "used" FAT entries. 
                        // Make "unused" FAT Entries for all UserDataClusters.
                        Table.Add(new Fat16Entry(Fat16Entry.Entry0));
                        Table.Add(new Fat16Entry(Fat16Entry.EOC));
                        for (int index = 2; index < formatInfo.UserDataClusters; ++index)
                            Table.Add(new Fat16Entry());
                        break;
                    
                    case FormatInfo.FileSystemType.Fat32:
                    default:
                        // Make 2 "used" FAT entries. 
                        Table.Add(new Fat32Entry(Fat32Entry.Entry0));
                        Table.Add(new Fat32Entry(Fat32Entry.EOC));
                        // Allocate cluster 2 for the Root Directory
                        Table.Add(new Fat32Entry(Fat32Entry.EOC));
                        // Make "unused" FAT Entries for all UserDataClusters.
                        for (int index = 2; index < formatInfo.UserDataClusters; ++index)
                            Table.Add(new Fat32Entry());
                        break;
                }
            }

            public Byte[] ToByteArray()
            {
                List<Byte> fatEntryList = new List<Byte>(); 

                switch (FileSystem)
                {
                    case FormatInfo.FileSystemType.Fat12:
                        for (int entryIdx = 0; entryIdx < Table.Count - 1; entryIdx += 2)
                        {
                            Byte[] curEntry = Table[entryIdx].ToByteArray();
                            Byte[] nextEntry = new Byte[2];
                            if (entryIdx + 1 <= Table.Count)
                                nextEntry = Table[entryIdx + 1].ToByteArray();

                            fatEntryList.Add(curEntry[0]); // LSB of curEntry
                            fatEntryList.Add((Byte)((curEntry[1] & 0x0F) & ((nextEntry[0] & 0x0F) << 4))); // MSN of curEntry + LSN of nextEntry
                            fatEntryList.Add((Byte)(((nextEntry[0] & 0xF0) >> 4) & ((nextEntry[1] & 0x0F) << 4)));
                        }
                        break;

                    case FormatInfo.FileSystemType.Fat16:
                    case FormatInfo.FileSystemType.Fat32:
                    default:
                        foreach ( FatxxEntry entry in Table)
                            fatEntryList.AddRange(entry.ToByteArray());
                        break;
                }
                
                // Pad the rest of the sector with 0's
                if (fatEntryList.Count % SectorSize != 0)
                {
                    Byte[] zeros = new Byte[SectorSize - fatEntryList.Count % SectorSize];
                    fatEntryList.AddRange(zeros);
                }

                return fatEntryList.ToArray();
            }

        }
        
        /// <summary>
        /// - Defines the 32-byte RootDirEntry structure. Only used in FAT12 and FAT16 file systems.
        /// (32-bytes)
        /// </summary>
        public class RootDirEntry
        {
            //
            // CONSTANTS
            //

            // EntryAttribute defines
            [FlagsAttribute]
            public enum EntryAttribute : byte
            {
                None      = 0x00,
                ReadOnly  = 0x01,    // 0x01 - Read only
                Hidden    = 0x02,    // 0x02 - Hidden
                System    = 0x04,    // 0x04 - System
                VolumeId  = 0x08,    // 0x08 - Volume ID
                Directory = 0x10,    // 0x10 - Directory
                Archive   = 0x20,    // 0x20 - Archive
                LongName  = ReadOnly | Hidden | System | VolumeId    // 0x0F - Long name
            }

            /// <summary>
            /// The size in bytes of the structure (32 bytes).
            /// </summary>
            public const int Size = 32;

            //
            // FIELDS
            //

            /// <summary>
            /// Short name. Byte[11]
            /// </summary>
            private Byte[] Name;
     
            /// <summary>
            /// - File attributes.
            /// The upper 2 bits of the attribute byte are reserved and should always be set to 0 when a file is created
            /// and never modify or look at it after that.
            /// </summary>
            private EntryAttribute Attributes;

            /// <summary>
            /// - Reserved. Set to 0 when file is created and never modify or look at it after that.
            /// </summary>
            private Byte Reserved;
            
            /// <summary>
            /// - Millisecond stamp at file creation time.  This field actually contains a count of tenths of a 
            /// second.  The granularity of the seconds part of the CreationTime is two seconds so this field is a count of 
            /// tenths of a second and it's valid value range is 0-199 inclusive.
            /// </summary>
            private Byte CreationTimeTenth;

            /// <summary>
            /// - Time file was created.
            /// </summary>
            private UInt16 CreationTime;

            /// <summary>
            /// - Date file was created.
            /// </summary>
            private UInt16 CreationDate;

            /// <summary>
            /// - Last access date.  Note that there is no last access time, only a date.  This is the date of last
            /// read or write.  In the case of a write, this should be set to the same date as LastWriteDate.
            /// </summary>
            private UInt16 LastAccessedDate;
            
            /// <summary>
            /// - High word of this entry's first cluster number (always 0 for a FAT12 or FAT16 volume).
            /// </summary>
            private UInt16 FirstClusterHI;

            /// <summary>
            /// - Time of last write.  Note that file creation is considered a write.
            /// </summary>
            private UInt16 LastWriteTime;

            /// <summary>
            /// - Date of last write.  Note that file creation is considered a write.
            /// </summary>
            private UInt16 LastWriteDate;

            /// <summary>
            /// - Low word of this entry's first cluster number.
            /// </summary>
            private UInt16 FirstClusterLO;

            /// <summary>
            /// - 32-bit DWORD holding this file's size in bytes.
            /// </summary>
            private UInt32 DIR_FileSize;

            //
            // METHODS
            //

            public RootDirEntry()
            {
                Name = new Byte[11];
                Clear();
            }

            /// <summary>
            /// Returns true if this Directory Entry is empty.
            /// </summary>
            public Boolean IsEmpty()
            { 
                return ( Name[0] == 0xE5 || Name[0] == 0x00 );
            }

            /// <summary>
            /// Sets all fields to default(0).
            /// </summary>
            private void Clear()
            {
                Name.Initialize();
                Attributes = EntryAttribute.None;
                Reserved = 0;
                CreationTimeTenth = 0;
                CreationTime = 0;
                CreationDate = 0;
                LastAccessedDate = 0;
                FirstClusterHI = 0;
                LastWriteTime = 0;
                LastWriteDate = 0;
                FirstClusterLO = 0;
                DIR_FileSize = 0;
            }

            /// <summary>
            /// - Converts the volumeLabel to char* if necessary.
            /// - Fills in DIR_Name with the first 11-characters of volumeLabel.
            /// - Pads DIR_Name with ' ' if volumeName is less than 11-characters.
            /// - Sets DIR_Attr to AttrVolumeId;
            /// </summary>
            /// <param name="volumeLabel">The volume label.</param>
            /// <exception cref="System.Exception"> Directory Entry is not empty.</exception> 
            public void MakeVolumeLabel(String volumeLabel)
            {
                if ( !IsEmpty() )
                {
                    throw new Exception("Directory Entry is not empty.");
                }

                // clear the entry
                Clear();

                // Create an ASCII encoding.
                ASCIIEncoding ascii = new ASCIIEncoding();

                // copy the volume label into the name field and pad it to 11-chars with spaces
                if (volumeLabel.Length < 11)
                {
                    ascii.GetBytes(volumeLabel.PadRight(11, ' '), 0, 11, Name, 0);
                }
                else
                {
                    ascii.GetBytes(volumeLabel, 0, 11, Name, 0);
                }
                
                // Set the attribute field to the VolumeLabel attribute.
                Attributes = EntryAttribute.VolumeId;

                // SUCCESS
                return;
            }

            public Byte[] ToByteArray()
            {
                List<Byte> entryImage = new List<Byte>(RootDirEntry.Size);

                entryImage.AddRange(Name);
                entryImage.Add((Byte)Attributes);
                entryImage.Add(Reserved);
                entryImage.Add(CreationTimeTenth);
                entryImage.AddRange(BitConverter.GetBytes(CreationTime));
                entryImage.AddRange(BitConverter.GetBytes(CreationDate));
                entryImage.AddRange(BitConverter.GetBytes(LastAccessedDate));
                entryImage.AddRange(BitConverter.GetBytes(FirstClusterHI));
                entryImage.AddRange(BitConverter.GetBytes(LastWriteTime));
                entryImage.AddRange(BitConverter.GetBytes(LastWriteDate));
                entryImage.AddRange(BitConverter.GetBytes(FirstClusterLO));
                entryImage.AddRange(BitConverter.GetBytes(DIR_FileSize));

                Trace.Assert(entryImage.Count == RootDirEntry.Size, "The Byte[] is not the correct size.");

                return entryImage.ToArray();
            }

        } // RootDirEntry (32-bytes)

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
        public class RootDir
        {
            /// <summary>
            /// The size in bytes of the sector containing the structure.
            /// </summary>
            public readonly int SectorSize;

            // RootDirEntry* - pointer to the array of Root Directory Entries.
            private List<RootDirEntry> RooDirEntryTable = new List<RootDirEntry>();

            //
            // RootDir(const StFormatInfo& formatInfo, const LPCSTR volumeLabel)
            //
            // - Allocates memory for an array of RootDirEntries and initializes them to 0.
            // - Sets the first 
            public RootDir(FormatInfo formatInfo)
            {
                SectorSize = formatInfo.SectorSize;
                
                UInt32 numRootEntries = formatInfo.RootEntries;
                if (numRootEntries == 0)
                {
                    // FAT32 case; make a cluster of rootdir entries
                    numRootEntries = formatInfo.SectorsPerCluster * formatInfo.SectorSize / RootDirEntry.Size;
                }
                
                // Add default(0) entries
                for (int entryIdx = 0; entryIdx < numRootEntries; ++entryIdx)
                    RooDirEntryTable.Add(new RootDirEntry());

                // Create a 'file' in the root directory named "volume label' and set the AttrVolumeId attribute.
                if ( formatInfo.VolumeLabel != null )
			    {
                    RooDirEntryTable[0].MakeVolumeLabel(formatInfo.VolumeLabel);
			    }
            }

            public int FirstAvailableEntry()
            {
                for (int entry = 0; entry < RooDirEntryTable.Count; ++entry)
                {
                    if ( RooDirEntryTable[entry].IsEmpty() )
                        return entry;
                }

                // TODO: I think this is a valid RootDirectory number. Probably have to make this more robust when we port
                // this function to the generalized FirstAvailableCluster() that will work in the User Data area including 
                // 'root directory' on FAT32 volumes.
                throw new Exception("Could not find an empty Root Directory Entry.");
                return 0xFFFF;
            }

            public Byte[] ToByteArray()
            {
                List<Byte> rootDirImage = new List<Byte>(RooDirEntryTable.Count * RootDirEntry.Size);

                // Add the Root Directory Entries
                foreach (RootDirEntry entry in RooDirEntryTable)
                    rootDirImage.AddRange(entry.ToByteArray());

                // Pad the rest of the sector with 0's
                if (rootDirImage.Count % SectorSize != 0)
                {
                    Byte[] zeros = new Byte[rootDirImage.Count % SectorSize];
                    rootDirImage.AddRange(zeros);
                }

                Trace.Assert(rootDirImage.Count % SectorSize == 0, "The Byte[] is not the correct size.");

                return rootDirImage.ToArray();
            }

        } // class RootDir

        public class FormatImage
        {
            public Byte[] TheImage { get { return _TheImage.ToArray(); } }
            public String[] MapFile { get { return _MapFile.ToArray(); } }

            private List<Byte> _TheImage = new List<Byte>();
            private List<String> _MapFile = new List<String>();

            public FormatImage(FormatInfo formatInfo)
            {
                Int64 mapStart= 0;
                _MapFile.Add(formatInfo.ToString() + "\n");
                _MapFile.Add(String.Format("Start\t\tLength (sectors)\tName\t\t\tSectorBoundary\tClusterBoundary"));
                
                try
                {
                    if (formatInfo.IncludeMBR)
                    {
                        // Start with the MBR
                        MBR mbrSector = new MBR(formatInfo);

                        mapStart = _TheImage.Count;
                        _TheImage.AddRange(mbrSector.ToByteArray());
                        _MapFile.Add(String.Format("0x{0:X}\t\t\t0x{1:X} ({2})\t\t\t{3}\t\t\t\t\t{4}\t\t\t{5}", mapStart, _TheImage.Count - mapStart, (_TheImage.Count - mapStart) / formatInfo.SectorSize, "MBR", mapStart % formatInfo.SectorSize == 0, mapStart % formatInfo.ClusterSize == 0));

                        // Pad the Hidden sectors after the MBR before the PBS with 0's
                        Byte[] hiddenBytes = new Byte[(formatInfo.RelativeSector - 1/*MBR*/) * formatInfo.SectorSize];

                        mapStart = _TheImage.Count;
                        _TheImage.AddRange(hiddenBytes);
                        _MapFile.Add(String.Format("0x{0:X}\t\t0x{1:X} ({2})\t\t\t{3}\t\t{4}\t\t\t{5}", mapStart, _TheImage.Count - mapStart, (_TheImage.Count - mapStart) / formatInfo.SectorSize, "HiddenSectors", mapStart % formatInfo.SectorSize == 0, mapStart % formatInfo.ClusterSize == 0));
                    }
                    // Next comes the PBS
                    PBS pbsSectors = new PBS(formatInfo);

                    mapStart = _TheImage.Count;
                    _TheImage.AddRange(pbsSectors.ToByteArray());
                    _MapFile.Add(String.Format("0x{0:X}\t\t0x{1:X} ({2})\t\t\t{3}\t\t\t\t\t{4}\t\t\t{5}", mapStart, _TheImage.Count - mapStart, (_TheImage.Count - mapStart) / formatInfo.SectorSize, "PBS", mapStart % formatInfo.SectorSize == 0, mapStart % formatInfo.ClusterSize == 0));

                    // Make the FAT table(s)
                    FatTable fatTable = new FatTable(formatInfo);
                    for (int fatTableIdx = 0; fatTableIdx < formatInfo.FatTables; ++fatTableIdx)
                    {
                        mapStart = _TheImage.Count;
                        _TheImage.AddRange(fatTable.ToByteArray());
                        _MapFile.Add(String.Format("0x{0:X}\t\t0x{1:X} ({2})\t\t\t{3}\t\t\t{4}\t\t\t{5}", mapStart, _TheImage.Count - mapStart, (_TheImage.Count - mapStart) / formatInfo.SectorSize, String.Format("FAT Table {0}", fatTableIdx + 1), mapStart % formatInfo.SectorSize == 0, mapStart % formatInfo.ClusterSize == 0));
                    }
                    
                    // Create the Root Directory and copy it into theImage
                    RootDir rootDir = new RootDir(formatInfo);
	                 
                    mapStart = _TheImage.Count;
                    _TheImage.AddRange(rootDir.ToByteArray());
                    _MapFile.Add(String.Format("0x{0:X}\t\t0x{1:X} ({2})\t\t\t{3}\t\t{4}\t\t\t{5}", mapStart, _TheImage.Count - mapStart, (_TheImage.Count - mapStart) / formatInfo.SectorSize, "Root Directory", mapStart % formatInfo.SectorSize == 0, mapStart % formatInfo.ClusterSize == 0));

                    mapStart = _TheImage.Count;
                    _MapFile.Add(String.Format("0x{0:X}\t\t0x{1:X} ({2})\t{3}\t\t{4}\t\t\t{5}", mapStart, (formatInfo.UserDataClusters * formatInfo.ClusterSize) - mapStart, ((formatInfo.UserDataClusters * formatInfo.ClusterSize) - mapStart) / formatInfo.SectorSize, "User Clusters", mapStart % formatInfo.SectorSize == 0, mapStart % formatInfo.ClusterSize == 0));
                }
                catch(Exception e)
                {
                    Trace.Assert(false, e.Message);
                }
            }
            
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // AddFiles (LPCTSTR filePath)
            //
            // - Param: LPCTSTR filePath - File name or Directory Name to add. Full path of file or directory needs to be specified.
            //                             If filePath is a directory, all files and sub-directories will be added to the image. If
            //                             filePath is a file, only the file witll be added to the image. Wildcards are not supported.
            //
            // - Returns: int32_t error - ERROR_SUCCESS = no error.
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            public void AddFiles(String filePath)
            {
	            return;
            }        
        
        } // class FormatImage

    } // namespace FormatBuilder

} // namespace DevSupport
