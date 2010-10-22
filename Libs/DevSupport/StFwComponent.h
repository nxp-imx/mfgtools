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
#include "ParameterT.h"
#include "StVersionInfo.h"
#include <fstream>

#define UPDATER_FW_FILENAME		_T("updater.sb")
#define STATIC_ID_FW_FILENAME	_T("stmfgmsc")
#define STATIC_ID_FW_EXTENSION	_T(".sb")

class StFwComponent
{

public:
    enum FileType {
	    FileType_Invalid    = -1,
        FileType_Raw_Binary =  0,
        FileType_3500       =  1,
        FileType_3600_Stmp  =  2,
	    FileType_3600_Rsrc  =  3,
        FileType_3700_Stmp  =  4,
		FileType_3700_Rsrc  =  5
    };

    enum LoadFlag {
        LoadFlag_FileFirst = 0,
        LoadFlag_ResourceFirst,
        LoadFlag_FileOnly,
        LoadFlag_ResourceOnly
    };

    static const uint16_t InvalidId = 0xFFFF;
    static const std::string FileTypeTag_Sgtl;
    static const std::string FileTypeTag_Stmp;
    static const std::string FileTypeTag_Rsrc;
    static const std::string VersionLabel_Product;
    static const std::string VersionLabel_Component;

private:
//    StFwComponent(const StFwComponent& comp);
//	StFwComponent& operator=(const StFwComponent& comp);
    void InitParameters();
    void InitVersionInfo(const FileType fileType);

public:
    StFwComponent();
	StFwComponent(LPCTSTR fileName, LoadFlag loadFlag = LoadFlag_FileFirst, uint16_t langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
	virtual ~StFwComponent();

    int32_t Load(LPCTSTR fileName, LoadFlag loadFlag, uint16_t langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    int32_t LoadFromFile(LPCTSTR fileName);
    int32_t LoadFromResource(LPCTSTR resourceName, uint16_t langId);
	int32_t GetResourceID(LPCTSTR _fname);

	void GetData(size_t fromOffset, size_t count, uint8_t * pDest) const;
	const std::vector<uint8_t>& GetData() const { return _data; };
    const uint8_t * const GetDataPtr() const;
    const int32_t GetLastError() const { return _lastError; };
    const uint32_t size() const;
//	const size_t GetSizeInSectors(const uint32_t sectorSize) const;
    void clear();
    const CStdString toString() const;

	struct version_t {
		uint16_t Major;
		uint16_t c_pad0_1;
		uint16_t Minor;
		uint16_t c_pad0_2;
		uint16_t Revision;
		uint16_t c_pad0_3;
	};

	struct FirstBlockHeader
	{
		uint32_t  RomVersion;
		uint32_t  ImageSize;
		uint32_t  CipherTextOffset;
		uint32_t  UserDataOffset;
		uint32_t  KeyTransformCode;
		uint8_t   Tag[4];
		version_t ProductVersion;
		version_t ComponentVersion;
		uint16_t  DriveTag;
        uint16_t  reserved;
	};

    //! An AES-128 cipher block is 16 bytes.
    typedef uint8_t cipher_block_t[16];

    //! An AES-128 key is 128 bits, or 16 bytes.
    typedef uint8_t aes128_key_t[16];

    //! A SHA-1 digest is 160 bits, or 20 bytes.
    typedef uint8_t sha1_digest_t[20];

    //! Unique identifier for a section.
    typedef uint32_t section_id_t;

    struct BootImageHeader
	{
		sha1_digest_t m_digest;
		uint8_t       Signature[4];
		uint8_t       MajorVersion;
		uint8_t       MinorVersion;
		uint16_t      Flags;
		uint32_t      ImageBlocks;
		uint32_t      FirstBootTagBlock;
		section_id_t  FirstBootableSectionID;
		uint16_t      KeyCount;
		uint16_t      KeyDictionaryBlock;
		uint16_t      HeaderBlocks;
		uint16_t      SectionCount;
		uint16_t      SectionHeaderSize;
		uint8_t       padding0[2];
		uint8_t       Signature2[4];
		uint64_t      Timestamp;
		version_t     ProductVersion;
		version_t     ComponentVersion;
		uint16_t      DriveTag;
		uint8_t       padding1[6];
	};

protected:
    CStdString _fileName;
    CStdString _internalName;
    uint16_t   _id;
    uint16_t   _langId;
    ParameterT<bool> _isResource;

    std::vector<uint8_t> _data;
    ParameterT<FileType> _fileType;
    ParameterT<__time64_t> _fileDate;
    uint32_t             _startingOffset;
    int32_t              _lastError;

    // info
    StVersionInfo        _productVersion;
	StVersionInfo        _componentVersion;
	uint16_t			 _headerFlags;
public:
	const StVersionInfo& GetProductVersion() const;
	const StVersionInfo& GetComponentVersion() const;
    const ParameterT<FileType>& GetFileType();
    const ParameterT<__time64_t>& GetFileDate() const { return _fileDate; };
	const CStdString GetFileName() const { return _fileName; };
	const CStdString GetShortFileName() const;
    const bool IsResource() const { return _isResource.Value == true; };
	const uint16_t GetId() const { return _id; };
	const uint16_t GetFlags() const { return _headerFlags; };
};
