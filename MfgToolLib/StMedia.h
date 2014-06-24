/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "ParameterT.h"
#include "StFwComponent.h"

namespace media
{
    static const UCHAR DefaultMediaTableEntries = 20; // 20 - Default number of Media Table Entries
	static const UCHAR InvalidDriveNumber = 0xFF;     // 0xFF - Unassigned drive number
	
    enum PhysicalMediaType{
        MediaType_Invalid = -1,
        MediaType_Nand    = 0,
        MediaType_MMC     = 1,
        MediaType_HDD     = 2,
        MediaType_RAM     = 3,
        MediaType_iNAND   = 4
    };

	enum LogicalDriveType {
		DriveType_Invalid = 0xFF,
		DriveType_Data = 0,
		DriveType_System = 1,
		DriveType_HiddenData = 2,
		DriveType_Unknown = 3
	};

	enum LogicalDriveTag{
		DriveTag_Player	      = 0x00,
		DriveTag_UsbMsc       = 0x01,
		DriveTag_Hostlink     = 0x01,
		DriveTag_PlayerRsc    = 0x02,
		DriveTag_PlayerRsc2   = 0x12,
		DriveTag_PlayerRsc3   = 0x22,
		DriveTag_FirmwareRsc  = 0x02,
		DriveTag_FirmwareRsc2 = 0x12,
		DriveTag_FirmwareRsc3 = 0x22,
		DriveTag_Extra        = 0x03,
		DriveTag_ExtraRsc     = 0x04,
		DriveTag_Otg          = 0x05,
		DriveTag_HostlinkRsc  = 0x06,
		DriveTag_HostlinkRsc2 = 0x16,
		DriveTag_HostlinkRsc3 = 0x26,
		DriveTag_Mark         = 0x06,
		DriveTag_IrDA         = 0x07,
		DriveTag_SettingsBin  = 0x07,
		DriveTag_OtgRsc		  = 0x08,
		DriveTag_Data         = 0x0A,
		DriveTag_Data2        = 0x1A, 
		DriveTag_DataJanus    = 0x0B,
		DriveTag_DataSettings = 0x0C,
		DriveTag_Bootmanger   = 0x50,
		DriveTag_FirmwareImg  = 0x50,
		DriveTag_FirmwareImg2 = 0x60,
		DriveTag_FirmwareImg3 = 0x70,
		DriveTag_LBABoot	  = 0xA0,
		DriveTag_Invalid      = 0xF0,
		DriveTag_UpdaterNand  = 0xFE,
		DriveTag_Updater      = 0xFF
	};

	enum LogicalDriveFlag{
		DriveFlag_NoAction     = 0x00,
		DriveFlag_ImageData    = 0x01,
		DriveFlag_FileData     = 0x02,
		DriveFlag_Format       = 0x04,
		DriveFlag_JanusInit    = 0x08,
		DriveFlag_ImageDataRsc = 0x10
	};

    struct MediaAllocationEntry //: public Parameter
    {
        MediaAllocationEntry()
        {
		    _params[_T("Drive Number")] = &DriveNumber;
            _params[_T("Type")] = &Type;
		    _params[_T("Tag")] = &Tag;
		    _params[_T("Size (bytes)")] = &SizeInBytes;

			DriveNumber.Value = 0xFF;
			Type.Value = DriveType_Invalid;
			Tag.Value = DriveTag_Invalid;
			SizeInBytes.Value = 0;
        };

        MediaAllocationEntry(const UCHAR driveNumber, const LogicalDriveType type, const LogicalDriveTag tag, const ULONGLONG size)
        {
		    _params[_T("Drive Number")] = &DriveNumber;
            _params[_T("Type")] = &Type;
		    _params[_T("Tag")] = &Tag;
		    _params[_T("Size (bytes)")] = &SizeInBytes;

			DriveNumber.Value = driveNumber;
			Type.Value = type;
			Tag.Value = tag;
			SizeInBytes.Value = size;
        };

		ParameterT<UCHAR> DriveNumber;
	    ParameterT<LogicalDriveType> Type;
	    ParameterT<LogicalDriveTag> Tag;
	    ParameterT<__int64> SizeInBytes;
        static UINT Size() { return sizeof(UCHAR) + sizeof(UCHAR) + sizeof(UCHAR) + sizeof(ULONGLONG); };

        Parameter::ParamMap _params;
    };

    typedef std::vector<MediaAllocationEntry> MediaAllocationTable;

	class LogicalDrive
	{
	public:
		LogicalDrive::LogicalDrive( CString _name=_T(""), 
							        CString _desc=_T(""),
							        LogicalDriveType _type=DriveType_Invalid,
							        LogicalDriveTag _tag=DriveTag_Invalid,
									LogicalDriveFlag _flags=DriveFlag_NoAction,
							        UINT _mem=0,
									int _fileindex = -1)
							        : Name(_name)
							        , Description(_desc)
							        , Type(_type)
							        , Tag(_tag)
									, Flags(_flags)
							        , RequestedKB(_mem)
									, DriveNumber(InvalidDriveNumber)
									, SizeInBytes(0)
									, FileListIndex(-1) {};
		CString Name;
		CString Description;
		LogicalDriveType Type;
		LogicalDriveTag Tag;
		LogicalDriveFlag Flags;
		UINT RequestedKB;
		UCHAR DriveNumber;
		ULONGLONG SizeInBytes;
		USHORT FileListIndex;
	
		const UINT SizeInSectors(UINT sectorSize) const
		{
			UINT sectors = (UINT)(SizeInBytes / sectorSize);
			if ( SizeInBytes % sectorSize )
				++sectors;

			return sectors;
		}
	};
	
	class LogicalDriveArray
	{
	public:
//		typedef std::map<LogicalDriveTag, LogicalDrive>::iterator DriveArrayItem;
		LogicalDrive& GetDrive(const LogicalDriveTag tag) { return *(Find(tag)); };
		size_t GetFileDrive(const int _filelistindex)
		{
			size_t position = 0;
			std::vector<LogicalDrive>::iterator drv;
			for ( drv = DriveArray.begin(); drv != DriveArray.end(); ++drv )
			{
				if ( (*drv).FileListIndex == _filelistindex )
					break;
				++position;
			}

			if (drv != DriveArray.end())
				return (position);
			else
				return (-1);
		};

		size_t GetNumDrivesByType(const LogicalDriveType drvType) const
		{ 
			size_t numDrives = 0;

//			std::map<LogicalDriveTag, LogicalDrive>::const_iterator drv;
//			for ( drv = DriveArray.begin(); drv != DriveArray.end(); ++drv )
//			{
//				if ( (*drv).second.Type == drvType )
//				{
//					++numDrives;
//				}
//			}
			std::vector<LogicalDrive>::const_iterator drv;
			for ( drv = DriveArray.begin(); drv != DriveArray.end(); ++drv )
			{
				if ( (*drv).Type == drvType )
				{
					++numDrives;
				}
			}
			return numDrives;
		};

		MediaAllocationTable MakeAllocationTable(StFwComponent::LoadFlag loadFlag)
		{
			/**/
              MediaAllocationTable table;
			  StFwComponent::LoadFlag src;

              // make a placeholder for the first firmware entry at the head of the list
              media::MediaAllocationEntry bootyEntry(InvalidDriveNumber, DriveType_System, DriveTag_FirmwareImg, 0);
              table.push_back(bootyEntry);

              size_t driveIndex;
              for ( driveIndex = 0; driveIndex < (*this).Size(); ++driveIndex )
              {
                    // Skip the updater drive. If you want to put the updater drive on the NAND, use tag DriveTag_UpdaterNand (0xFE).
                    if ( (*this)[driveIndex].Tag == DriveTag_Updater )
                          continue;

					// always force settings to load from file
					if ( (*this)[driveIndex].Tag == media::DriveTag_DataSettings )
						src = StFwComponent::LoadFlag_FileFirst;
					else
						src = loadFlag;

					// Determine how much room to ask for
                    UINT bytesToAllocate = (*this)[driveIndex].RequestedKB * 1024;
                    StFwComponent fw((*this)[driveIndex].Name, src);

                    if ( fw.size() > bytesToAllocate )
                          bytesToAllocate = fw.size();

                     // Create the table entry
                    if ( (*this)[driveIndex].Tag == DriveTag_FirmwareImg )
                    {
                          // Just update the size field for DriveTag_FirmwareImg
                          table.at(0).SizeInBytes = bytesToAllocate;
                    }
                    else
                    {
                          media::MediaAllocationEntry entry(InvalidDriveNumber, (*this)[driveIndex].Type, (*this)[driveIndex].Tag, bytesToAllocate);
                          table.push_back(entry);
                    }
              }

              return table;
			/**/
			/*
			MediaAllocationTable table;

			size_t driveIndex;
			for ( driveIndex = 0; driveIndex < (*this).Size(); ++driveIndex )
			{
				// Skip the updater drive. If you want to put the updater drive on the NAND, use tag DriveTag_UpdaterNand (0xFE).
				if ( (*this)[driveIndex].Tag == DriveTag_Updater )
					continue;

				// Determine how much room to ask for
				UINT bytesToAllocate = (*this)[driveIndex].RequestedKB * 1024;
				StFwComponent fw((*this)[driveIndex].Name, loadFlag);
				if ( fw.size() > bytesToAllocate )
					bytesToAllocate = fw.size();
				
				// Create the table entry
				media::MediaAllocationEntry entry(InvalidDriveNumber, (*this)[driveIndex].Type, (*this)[driveIndex].Tag, bytesToAllocate);
				table.push_back(entry);
			}

			return table;
			*/
		};
		void AddDrive(const LogicalDrive& drive){ DriveArray.push_back(drive); };
		void Clear() { DriveArray.erase(DriveArray.begin(), DriveArray.end()); };
		void Remove (const LogicalDrive& drive) { ( DriveArray.erase(Find(drive.Tag))); };
		void Insert (const int index, const LogicalDrive& drive) {DriveArray.insert(DriveArray.begin() + index, drive); };
//		int GetFileListIndex (const LogicalDrive& drive) { return drive.FileListIndex; };
		size_t Size() const { return DriveArray.size(); };
		LogicalDrive& operator[](size_t position)
		{ 
			return DriveArray[position];
		};  

	private:
		std::vector<LogicalDrive> DriveArray;
		std::vector<LogicalDrive>::iterator Find(const LogicalDriveTag tag)
		{
			std::vector<LogicalDrive>::iterator drv;
			for ( drv = DriveArray.begin(); drv != DriveArray.end(); ++drv )
			{
				if ( (*drv).Tag == tag )
				{
					break;
				}
			}
			return drv;
		}
	};

	#pragma pack (push, 1)

	struct HiddenDataDriveFormat
	{
		HiddenDataDriveFormat(const UINT numSectors)
			: JanusSignature0(0x80071119)
			, JanusSignature1(0x19082879)
			, JanusFormatID(0x100)
			, JanusBootSectorOffset(20)
			, JanusDriveTotalSectors(numSectors)
		{
			memset(First16Bytes, 0, sizeof(First16Bytes));
		};

		UCHAR First16Bytes[16];
		const UINT JanusSignature0;
		const UINT JanusSignature1;
		const USHORT JanusFormatID;
		const USHORT JanusBootSectorOffset;
		const UINT JanusDriveTotalSectors;
	};

	struct MxImageObject
	{
		CString ImageFilename;
		UINT Address;
	};

	#pragma pack (pop, 1)

} // end namespace media


/*








#include "StBase.h"
#include "StFwComponent.h"
#include "Libs/Public/StdString.h"

#define DRIVE_TAG_STMPSYS_S         0x00
#define DRIVE_TAG_USBMSC_S          0x01
#define DRIVE_TAG_HOSTLINK_S        0x01
#define DRIVE_TAG_RESOURCE_BIN      0x02
#define DRIVE_TAG_EXTRA_S           0x03
#define DRIVE_TAG_RESOURCE1_BIN     0x04
#define DRIVE_TAG_OTGHOST_S         0x05
#define DRIVE_TAG_HOSTRSC_BIN       0x06
#define DRIVE_TAG_DATA              0x0A
#define DRIVE_TAG_DATA_HIDDEN       0x0B
#define DRIVE_TAG_BOOTMANAGER_S     0x50
#define DRIVE_TAG_UPDATER_S         0xFF
#define DRIVE_TAG_UNKOWN            0xEF

#define NUM_MEDIA_DRIVE_STR_ELEMENTS 6

class CStMedia :
    public CStBase
{
public:
    CStMedia(CString _drive_array_str = L"");
    virtual ~CStMedia(void);

    ///////////////////////////////////////////////////////////////////////////////
    // Typedefs
    ///////////////////////////////////////////////////////////////////////////////

    typedef enum PHYSICAL_MEDIA_TYPE {
        MediaTypeNand = 0,
        MediaTypeMMC = 1,
        MediaTypeHDD = 2,
        MediaTypeRAM = 3
    }* P_PHYSICAL_MEDIA_TYPE;

    typedef enum LOGICAL_DRIVE_TYPE {
            DriveTypeInvalid = -1,
            DriveTypeData = 0,
            DriveTypeSystem = 1,
            DriveTypeHiddenData = 2,
	        DriveTypeNonVolatile = 3,
            DriveTypeUnknown = 4
    }* P_LOGICAL_DRIVE_TYPE;

    #pragma pack (1)
     
    typedef struct MEDIA_ALLOCATION_TABLE_ENTRY {
        UCHAR DriveNumber;
        UCHAR Type;
        UCHAR Tag;
        ULONGLONG SizeInBytes;
    }* P_MEDIA_ALLOCATION_TABLE_ENTRY;

    typedef struct MEDIA_ALLOCATION_TABLE {
        USHORT wNumEntries;
        MEDIA_ALLOCATION_TABLE_ENTRY Entry[MAX_MEDIA_TABLE_ENTRIES];
    }MEDIA_ALLOCATION_TABLE, *P_MEDIA_ALLOCATION_TABLE;

    typedef struct _HiddenDataDriveFormat {

        BYTE First16Bytes[16];
        ULONG JanusSignature0;
        ULONG JanusSignature1;
        USHORT JanusFormatID;
        USHORT JanusBootSectorOffset;
        ULONG JanusDriveTotalSectors;

    } HIDDEN_DATA_DRIVE_FORMAT, *P_HIDDEN_DATA_DRIVE_FORMAT;



    typedef vector<CString > StdStrArray;




    #pragma pack ()

    // Drive description class declaration
    class CStMediaDrive : public CStBase
    {
        public:
	        CStMediaDrive( CString _name=_T(""), 
                        CString _desc=_T(""),
                        LOGICAL_DRIVE_TYPE _type=DriveTypeInvalid,
                        UCHAR _tag=0,
                        bool _crypt=FALSE,
                        ULONG _mem=0 );
	        CStMediaDrive(wstring strDriveDesc);
	        CStMediaDrive(StdStrArray& _driver_desc_strs);
            ~CStMediaDrive();

            CString m_name;
            CString m_pathname;
            CString m_desc;
            LOGICAL_DRIVE_TYPE m_type;
            UCHAR m_tag;
            bool m_encrypted;
            ULONG m_additional_memory;
            
            void InitFwComponent();
            CString ToString();
            DWORD Validate();

            StFwComponent* GetFwComponent() { return &m_fw_component; }

        private:
            CString DriveTypeToStr(LOGICAL_DRIVE_TYPE type);
            StFwComponent m_fw_component;
    };

    typedef vector<CStMediaDrive>        MediaDriveArray;
    typedef MediaDriveArray::size_type   MediaDriveIdx;

private:
    void InitializeObjects(CString _drive_array_str = L"");

    MediaDriveArray m_media_drive_array;

public:
    size_t GetNumDrives() { return m_media_drive_array.size(); };
    size_t GetNumSystemDrives();
    size_t GetNumDataDrives();
    size_t GetNumHiddenDataDrives();

    LPCTSTR GetDrivePathName(MediaDriveIdx _index);
    LPCTSTR GetDrivePathName(UCHAR _tag);
    LPCTSTR GetDriveDescription(MediaDriveIdx _index);
    LPCTSTR GetDriveDescription(UCHAR _tag);
    UCHAR GetTag(MediaDriveIdx _index);

    void AddDrive(wstring _media_drive_str);
    void AddDrive(CString _name,
                  CString _desc,
                  LOGICAL_DRIVE_TYPE _type,
                  UCHAR _tag,
                  bool _crypt,
                  ULONG _mem );

    // Firmware component interface
    StFwComponent* GetFwComponent(MediaDriveIdx _sys_drv_index);
    StFwComponent* GetFwComponent(UCHAR _tag);

    // Media allocation table built from MediaDriveArray
    void GetMediaAllocationTable(P_MEDIA_ALLOCATION_TABLE p_media_allocation_table=NULL);
};
*/
/*/////////////// TODO: //////////////////////
    LOGICAL_DRIVE_TYPE GetDriveType(MediaDriveIdx _index);
    UCHAR GetDriveTag(MediaDriveIdx _index);
    LPCTSTR GetSystemDriveName(MediaDriveIdx _index);
////////////////////////////////////////////*/

