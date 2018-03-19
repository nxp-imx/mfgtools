/*
 * Copyright 2009-2013, 2016 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
		   _params[_T("Drive Number")]=&DriveNumber;
	           _params[_T("Type")] = &Type;
		   _params[_T("Tag")] = &Tag;
		   _params[_T("Size (bytes)")]=&SizeInBytes;

			DriveNumber.Value = 0xFF;
			Type.Value = DriveType_Invalid;
			Tag.Value = DriveTag_Invalid;
			SizeInBytes.Value = 0;
        };

        MediaAllocationEntry(const UCHAR driveNumber, const LogicalDriveType type, const LogicalDriveTag tag, const ULONGLONG size)
        {
		    _params[_T("Drive Number")]=&DriveNumber;
	            _params[_T("Type")] = &Type;
		    _params[_T("Tag")] = &Tag;
		    _params[_T("Size (bytes)")]=&SizeInBytes;

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
		LogicalDrive( CString _name=_T(""),
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
		};
		void AddDrive(const LogicalDrive& drive){ DriveArray.push_back(drive); };
		void Clear() { DriveArray.erase(DriveArray.begin(), DriveArray.end()); };
		void Remove (const LogicalDrive& drive) { ( DriveArray.erase(Find(drive.Tag))); };
		void Insert (const int index, const LogicalDrive& drive) {DriveArray.insert(DriveArray.begin() + index, drive); };

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


/*/////////////// TODO: //////////////////////
    LOGICAL_DRIVE_TYPE GetDriveType(MediaDriveIdx _index);
    UCHAR GetDriveTag(MediaDriveIdx _index);
    LPCTSTR GetSystemDriveName(MediaDriveIdx _index);
////////////////////////////////////////////*/
