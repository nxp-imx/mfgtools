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

    static const USHORT InvalidId = 0xFFFF;
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
	StFwComponent(LPCTSTR fileName, LoadFlag loadFlag = LoadFlag_FileFirst, USHORT langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
	virtual ~StFwComponent();

    int Load(LPCTSTR fileName, LoadFlag loadFlag, USHORT langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    int LoadFromFile(LPCTSTR fileName);
    int LoadFromResource(LPCTSTR resourceName, USHORT langId);
	int GetResourceID(LPCTSTR _fname);

	void GetData(size_t fromOffset, size_t count, UCHAR * pDest) const;
	const std::vector<UCHAR>& GetData() const { return _data; };
    const UCHAR * const GetDataPtr() const;
    const int GetLastError() const { return _lastError; };
    const UINT size() const;
//	const size_t GetSizeInSectors(const UINT sectorSize) const;
    void clear();
    const CString toString() const;

	struct version_t {
		USHORT Major;
		USHORT c_pad0_1;
		USHORT Minor;
		USHORT c_pad0_2;
		USHORT Revision;
		USHORT c_pad0_3;
	};

	struct FirstBlockHeader
	{
		UINT  RomVersion;
		UINT  ImageSize;
		UINT  CipherTextOffset;
		UINT  UserDataOffset;
		UINT  KeyTransformCode;
		UCHAR   Tag[4];
		version_t ProductVersion;
		version_t ComponentVersion;
		USHORT  DriveTag;
        USHORT  reserved;
	};

    //! An AES-128 cipher block is 16 bytes.
    typedef UCHAR cipher_block_t[16];

    //! An AES-128 key is 128 bits, or 16 bytes.
    typedef UCHAR aes128_key_t[16];

    //! A SHA-1 digest is 160 bits, or 20 bytes.
    typedef UCHAR sha1_digest_t[20];

    //! Unique identifier for a section.
    typedef UINT section_id_t;

    struct BootImageHeader
	{
		sha1_digest_t m_digest;
		UCHAR       Signature[4];
		UCHAR       MajorVersion;
		UCHAR       MinorVersion;
		USHORT      Flags;
		UINT      ImageBlocks;
		UINT      FirstBootTagBlock;
		section_id_t  FirstBootableSectionID;
		USHORT      KeyCount;
		USHORT      KeyDictionaryBlock;
		USHORT      HeaderBlocks;
		USHORT      SectionCount;
		USHORT      SectionHeaderSize;
		UCHAR       padding0[2];
		UCHAR       Signature2[4];
		ULONGLONG      Timestamp;
		version_t     ProductVersion;
		version_t     ComponentVersion;
		USHORT      DriveTag;
		UCHAR       padding1[6];
	};

protected:
    CString _fileName;
    CString _internalName;
    USHORT   _id;
    USHORT   _langId;
    ParameterT<bool> _isResource;

    std::vector<UCHAR> _data;
    ParameterT<FileType> _fileType;
    ParameterT<__time64_t> _fileDate;
    UINT             _startingOffset;
    int              _lastError;

    // info
    StVersionInfo        _productVersion;
	StVersionInfo        _componentVersion;
	USHORT			 _headerFlags;
public:
	const StVersionInfo& GetProductVersion() const;
	const StVersionInfo& GetComponentVersion() const;
    const ParameterT<FileType>& GetFileType();
    const ParameterT<__time64_t>& GetFileDate() const { return _fileDate; };
	const CString GetFileName() const { return _fileName; };
	const CString GetShortFileName() const;
    const bool IsResource() const { return _isResource.Value == true; };
	const USHORT GetId() const { return _id; };
	const USHORT GetFlags() const { return _headerFlags; };
};
