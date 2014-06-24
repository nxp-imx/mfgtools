/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StFwComponent.cpp: implementation of the StFwComponent class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StVersionInfo.h"
#include "StFwComponent.h"
#include "updater_res.h"
#include "updater_restypes.h"

const std::string StFwComponent::FileTypeTag_Sgtl    = "\032sgtl";
const std::string StFwComponent::FileTypeTag_Stmp    = "\032STMP";
const std::string StFwComponent::FileTypeTag_Rsrc    = "\032RSRC";
const std::string StFwComponent::VersionLabel_Product = "Product Version:";
const std::string StFwComponent::VersionLabel_Component = "Component Version:";

StFwComponent::StFwComponent()
: _fileName(_T(""))
, _id(InvalidId)
, _langId(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL))
, _startingOffset(0)
, _fileType(FileType_Invalid)
, _lastError(ERROR_INVALID_DATA)
, _isResource(false)
{ 
    InitParameters();
}

StFwComponent::StFwComponent(LPCTSTR fileName, LoadFlag loadFlag, USHORT langId)
: _fileName(fileName)
, _id(InvalidId)
, _langId(langId)
, _startingOffset(0)
, _fileType(FileType_Invalid)
, _isResource(false)
{
    InitParameters();
    _lastError = Load(fileName, loadFlag, langId);
}

StFwComponent::~StFwComponent()
{
}

void StFwComponent::InitParameters()
{
    _fileType.ValueList[FileType_Invalid]    = _T("FileType_Invalid");
    _fileType.ValueList[FileType_Raw_Binary] = _T("FileType_Raw_Binary");
    _fileType.ValueList[FileType_3500]       = _T("FileType_3500");
    _fileType.ValueList[FileType_3600_Stmp]  = _T("FileType_3600_Stmp");
    _fileType.ValueList[FileType_3600_Rsrc]  = _T("FileType_3600_Rsrc");
    _fileType.ValueList[FileType_3700_Stmp]  = _T("FileType_3700_Stmp");
    _fileType.ValueList[FileType_3700_Rsrc]  = _T("FileType_3700_Rsrc");

    _isResource.ValueList[false] = _T("false");
    _isResource.ValueList[true]  = _T("true");
}

const UINT StFwComponent::size() const
{
    if ( !_data.empty() && ((_data.size() - _startingOffset) > 0) )
        return (UINT)_data.size() - _startingOffset;
    else
        return 0;
}

void StFwComponent::clear()
{
    //_fileName.clear();
	_fileName.Empty();
    //_internalName.clear();
	_internalName.Empty();
    _id = InvalidId;
    _langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    _isResource.Value = false;

    _data.clear();
    _fileType.Value = FileType_Invalid;
    _startingOffset = 0;
    _lastError = ERROR_INVALID_DATA;

    _productVersion.clear();
    _componentVersion.clear();
}

const CString StFwComponent::GetShortFileName() const
{
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	CString shortName;

	_tsplitpath_s(_fileName, NULL, 0, NULL, 0, fname, _MAX_FNAME, ext,_MAX_EXT);

    shortName.Format(_T("%s%s"), fname, ext);

	return shortName;
}

/*
const size_t StFwComponent::GetSizeInSectors(const UINT sectorSize) const
{
	if( sectorSize )
	{
		return 0;
	}

	size_t sectors = size() / sectorSize;

	if( size() % sectorSize )
	{
		sectors += 1;
	}

	return sectors;
}
*/
const StVersionInfo& StFwComponent::GetProductVersion() const
{
	return _productVersion;
}

const StVersionInfo& StFwComponent::GetComponentVersion() const
{
	return _componentVersion;
}

void StFwComponent::InitVersionInfo(const StFwComponent::FileType fileType)
{
    _productVersion.clear();
    _componentVersion.clear();

    switch ( fileType )
    {
    case FileType_3500:
        {
            // 3500-style files have 2 lines of plain text containing ProductVersion: and ComponentVersion:.
            // We will will limit the search for the VersionLabel_Product and VersionLabel_Component
            // to the first 1024 bytes of the file.
            std::string versionLabel((char*)&_data[0], 1024);
            UINT major=0, minor=0, revision=0;
            
            // check for VersionLabel_Product
            size_t index = versionLabel.find(VersionLabel_Product);
            if ( index != std::string::npos )
            {
                index += VersionLabel_Product.size();
                if( sscanf_s(&versionLabel[index], "%u.%u.%u", &major, &minor, &revision) != EOF )
                {
	                _productVersion.SetMajor(major);
	                _productVersion.SetMinor(minor);
	                _productVersion.SetRevision(revision);
                }
            }
            // check for VersionLabel_Component
            index = versionLabel.find(VersionLabel_Component);
            if ( index != std::string::npos )
            {
                index += VersionLabel_Component.size();
                if( sscanf_s(&versionLabel[index], "%u.%u.%u", &major, &minor, &revision) != EOF )
                {
	                _componentVersion.SetMajor(major);
	                _componentVersion.SetMinor(minor);
	                _componentVersion.SetRevision(revision);
                }
            }
            _headerFlags = 0;
        }
        break;
    case FileType_3600_Stmp:
    case FileType_3600_Rsrc:
        {
            FirstBlockHeader* pHeader = (FirstBlockHeader*)&_data[0];
            // 3600-style firmware files have a FirstBlockHeader at the beginning of the file.
            _productVersion.SetMajor(pHeader->ProductVersion.Major, true);
            _productVersion.SetMinor(pHeader->ProductVersion.Minor, true);
            _productVersion.SetRevision(pHeader->ProductVersion.Revision, true);
            _componentVersion.SetMajor(pHeader->ComponentVersion.Major, true);
            _componentVersion.SetMinor(pHeader->ComponentVersion.Minor, true);
            _componentVersion.SetRevision(pHeader->ComponentVersion.Revision, true);
            _headerFlags = 0;
            _id = pHeader->DriveTag;
            break;
        }
    case FileType_3700_Stmp:
        {
            // 3700-style firmware files have a BootImageHeader at the beginning of the file.
            BootImageHeader* pHeader = (BootImageHeader*)&_data[0];
            _productVersion.SetMajor(pHeader->ProductVersion.Major, true);
            _productVersion.SetMinor(pHeader->ProductVersion.Minor, true);
            _productVersion.SetRevision(pHeader->ProductVersion.Revision, true);
            _componentVersion.SetMajor(pHeader->ComponentVersion.Major, true);
            _componentVersion.SetMinor(pHeader->ComponentVersion.Minor, true);
            _componentVersion.SetRevision(pHeader->ComponentVersion.Revision, true);
			_headerFlags = pHeader->Flags;
            break;
        }
    case FileType_Invalid:
    case FileType_Raw_Binary:
    default:
        break;
    }
}

// needs _data
// modifies member variables _startingOffset and _fileType
const ParameterT<StFwComponent::FileType>& StFwComponent::GetFileType()
{
    if ( _data.empty() )
    {
        _startingOffset = 0;
        _fileType.Value = FileType_Invalid;
        return _fileType;
    }

	// 3700-style firmware files have a BootImageHeader at the beginning of the file. 
	// The signature is unfortunately placed at the same offset for 36xx and 37xx.  So for
	// 37xx we have two signatures.
    // "STMP" should be located in the m_signature field and "sgtl" in the second signature. 
    //
    // The FileTypeTag_Stmp needs to be incremented by 1 to skip the 0x1a leading byte for all but 3500-stlye
    // firmware files.
    if ( _data.size() >= sizeof(BootImageHeader) )
	{
		std::string fileTag37((char*)((BootImageHeader*)&_data[0])->Signature, FileTypeTag_Stmp.size()-1);
		std::string fileTag37_2((char*)((BootImageHeader*)&_data[0])->Signature2, FileTypeTag_Stmp.size()-1);
	    
		// check 3700-stye STMP
		if ( (FileTypeTag_Stmp.find(fileTag37) != std::string::npos ) &&
				(FileTypeTag_Sgtl.find(fileTag37_2) != std::string::npos ) )
		{
			_startingOffset = 0;
			_fileType.Value = FileType_3700_Stmp;
			return _fileType;
		}

		// check 3700-stye RSRC
		if ( (FileTypeTag_Rsrc.find(fileTag37) != std::string::npos ) &&
				(FileTypeTag_Sgtl.find(fileTag37_2) != std::string::npos ) )
		{
			_startingOffset = 0;
			_fileType.Value = FileType_3700_Rsrc;
			return _fileType;
		}
	}

	// 3600-style firmware files have a FirstBlockHeader at the beginning of the file.
    // "STMP" or "RSRC" should be located in the tag field.
    //
    // The FileTypeTag_xxxx needs to be incremented by 1 to skip the 0x1a leading byte for all but 3500-stlye
    // firmware files.
    if ( _data.size() >= sizeof(FirstBlockHeader) )
	{
		std::string fileTag36((char*)((FirstBlockHeader*)&_data[0])->Tag, FileTypeTag_Stmp.size()-1);
	    
		// check 3600-stye STMP
		if ( FileTypeTag_Stmp.find(fileTag36) != std::string::npos ) 
		{
			_startingOffset = 0;
			_fileType.Value = FileType_3600_Stmp;
			return _fileType;
		}

		// check 3600-stye RSRC
		if ( FileTypeTag_Rsrc.find(fileTag36) != std::string::npos )
		{
			_startingOffset = 0;
			_fileType.Value = FileType_3600_Rsrc;
			return _fileType;
		}
	}

    // 3500-style files have 2 lines of plain text defining the ProductVersion and ComponentVersion.
    // All information up to and including the (0x1A)STMP tag must be stripped off of the file. Thus, for 
    // 3500-style files, _startingOffset will be non-zero. For all other types, _startingOffset will be 0.
    //
    // The full FileTypeTag_Stmp should be present for 3500-style files.
    // We will will limit the search for the FileTypeTag_Stmp to the first 1024 bytes of the file 
	// or the size of the data which ever is smaller.
    std::string fileTag35((char*)&_data[0], min(_data.size(), 1024));
    
    // check 3500-stye STMP
    UINT index = (UINT)fileTag35.find(FileTypeTag_Stmp);
    if ( index != std::string::npos )
    {
        _startingOffset = index + FileTypeTag_Stmp.size();
        _fileType.Value = FileType_3500;
        return _fileType;
    }

    _startingOffset = 0;
    _fileType.Value = FileType_Raw_Binary;
    return _fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////
// if the fromOffset and count are invalid then it must fill pDest with 0xFF.
void StFwComponent::GetData(size_t fromOffset, size_t count, UCHAR * pDest) const
{
    // 3500-style files have 2 lines of plain text defining the ProductVersion and ComponentVersion.
    // All information before the STMP or RSRC tag must be stripped off of the file. Thus, for 
    // 3500-style files, _startingOffset will be non-zero. For all other types, _startingOffset will be 0.
    size_t startOffset = fromOffset + _startingOffset;

	size_t index;
    
    if( startOffset > _data.size() )
	{
		// fromOffset is out of range. Fill pDest with 0xff.
		for( index = 0; index < count; index++ )
		{
			pDest[index] = 0xFF;
		}
	}

    if( count > _data.size() - startOffset ) 
	{
		// data is less than asked for so fill the remaining with 0xff
		for( index = 0; index < ( _data.size() - startOffset ); index++ )
		{
			pDest[index] = _data[index + startOffset];
		}

		for( ; index < count; index ++ )
		{
			pDest[index] = 0xFF;
		}
	}
    else
    {
	    // found fromOffset and count in range so return the right data.
	    for( index = 0; index < count; index++ )
	    {
			pDest[index] = _data[index + startOffset];
	    }
    }
}

// return a pointer to the actual firmware data
const UCHAR * const StFwComponent::GetDataPtr() const
{ 
    return _data.empty() ? NULL : &_data[0] + _startingOffset; 
}

int StFwComponent::Load(LPCTSTR fileName, LoadFlag loadFlag, USHORT langId)
{
	switch (loadFlag)
    {
        case LoadFlag_FileFirst:
            _lastError = LoadFromFile(fileName);
            if ( _lastError != ERROR_SUCCESS )
				_lastError = LoadFromResource(fileName, langId);
            break;
        
        case LoadFlag_ResourceFirst:
            _lastError = LoadFromResource(fileName, langId);
            if ( _lastError != ERROR_SUCCESS )
                _lastError = LoadFromFile(fileName);
			break;
        
        case LoadFlag_FileOnly:
            _lastError = LoadFromFile(fileName);
			break;
        
        case LoadFlag_ResourceOnly:
            _lastError = LoadFromResource(fileName, langId);
			break;
        
        default:
            _lastError = ERROR_INVALID_DATA;
			break;
    }
	return _lastError;
}

// modifies member variables _fileName, _data, _startingOffset, _fileDate, and _fileType
int StFwComponent::LoadFromFile(LPCTSTR fileName)
{
    clear();
    
    _fileName = fileName;
    _isResource = false;

	// open the file in binary mode. return error on fail to open or read.
	std::ifstream file(fileName, std::ios::in | std::ios::binary, _SH_DENYNO);
    if( file.fail() )
        return ERROR_OPEN_FAILED;

    // Get the file size
    UINT fileSize = file.rdbuf()->pubseekoff(0, std::ios::end, std::ios::in);
    file.rdbuf()->pubseekpos(0, std::ios::in);

	if ( fileSize == 0 )
	{
			file.close();
			return ERROR_INVALID_DATA;
	}
	
	// Size our vector to accomodate the file size.
    _data.resize(fileSize);

	// Put the data from the file in our vector
	if ( file.rdbuf()->sgetn((char*)&_data[0], _data.size()) != fileSize )
	{
		file.close();
		return ERROR_INVALID_DATA;
	}

    file.close();

    InitVersionInfo( GetFileType().Value );

    // Get the last modified date/time
    struct _stat buf = {0};
    if ( _tstat(_fileName, &buf) == ERROR_SUCCESS )
        _fileDate = buf.st_mtime;
    else
        _fileDate = 0;


    return ERROR_SUCCESS;
}

// modifies member variables _fileName, _data, _startingOffset, _fileDate, and _fileType
int StFwComponent::LoadFromResource(LPCTSTR resourceName, USHORT langId)
{
    clear();

    _fileName = resourceName;
	_langId = langId;
    _isResource = true;
	int iResId = GetResourceID(resourceName);

    HRSRC hResInfo = FindResourceEx( GetModuleHandle(NULL), L_STMP_FW_RESTYPE, MAKEINTRESOURCE(iResId), langId );
    if ( hResInfo == NULL )
        return ERROR_OPEN_FAILED;
    
	HGLOBAL hRes = LoadResource(GetModuleHandle(NULL), hResInfo);
	if ( hRes == NULL )
        return GetLastError();

	UCHAR * pResourceData = (UCHAR*)LockResource(hRes);
    if ( pResourceData == NULL )
        return ERROR_LOCK_FAILED;

    // Get the file size
    UINT resourceSize = SizeofResource(GetModuleHandle(NULL), hResInfo);

    // Size our vector to accomodate the file size.
    _data.resize(resourceSize);

	// Put the data from the file in our vector
    if ( memcpy_s((UCHAR*)&_data[0], _data.size(), pResourceData, resourceSize) != ERROR_SUCCESS )
        return ERROR_CANNOT_COPY;

    InitVersionInfo( GetFileType().Value );

    // no valid date/time property
    _fileDate = 0;    

    return ERROR_SUCCESS;
}

const CString StFwComponent::toString() const
{
    CString theInfo;

    theInfo.Format(_T("Name: %s\r\n"), _fileName);
    theInfo.AppendFormat(_T("Internal name: %s\r\n"), _internalName);
    theInfo.AppendFormat(_T("Size: %d bytes\r\n"), size());

    CString dateStr; 
    struct tm modTime;
    if ( _fileDate.Value != 0 )
    {
        _gmtime64_s(&modTime, &_fileDate.Value);
        _tcsftime( dateStr.GetBufferSetLength(_MAX_PATH), _MAX_PATH, _T("%c"), &modTime);
        dateStr.ReleaseBuffer();
    }
    else
    {
        dateStr = _T("N/A");
    }
    theInfo.AppendFormat(_T("Date modified: %s\r\n"), dateStr);
    
    theInfo.AppendFormat(_T("Type: %s\r\n"), _fileType.ToString());
    theInfo.AppendFormat(_T("Resource/File: %s\r\n"), IsResource() ? _T("Resource") : _T("File"));
    theInfo.AppendFormat(_T("Tag/Id: 0x%04X\r\n"), _id);
    theInfo.AppendFormat(_T("Product version: %s\r\n"), _productVersion.toString());
    theInfo.AppendFormat(_T("Component version: %s\r\n"), _componentVersion.toString());
    theInfo.AppendFormat(_T("Language: 0x%04X\r\n"), _langId);

    return theInfo; 
}



int StFwComponent::GetResourceID(LPCTSTR _fname)
{
	int resId = IDR_RESID_UNKNOWN;;
	LPCTSTR pFileName;

	pFileName = _tcsrchr(_fname, '\\');
	if (pFileName)
		++pFileName;
	else
		pFileName = _fname;

	if (_tcscmp(pFileName, _T("updater.sb")) == 0)
		return IDR_DEFAULT_PROFILE_RESID_0;
	if (_tcscmp(pFileName, _T("firmware.sb")) == 0)
		return IDR_DEFAULT_PROFILE_RESID_1;
	if (_tcscmp(pFileName, _T("firmware.rsc")) == 0)
		return IDR_DEFAULT_PROFILE_RESID_2;
	if (_tcscmp(pFileName, _T("OtpInit.sb")) == 0)
		return IDR_DEFAULT_PROFILE_RESID_3;
	if (_tcscmp(pFileName, _T("OtpAccessPitc.3700.sb")) == 0)
		return IDR_OTPACCESS3700_RESID;
	if (_tcscmp(pFileName, _T("OtpAccessPitc.3770.sb")) == 0)
		return IDR_OTPACCESS3770_RESID;
	if (_tcscmp(pFileName, _T("OtpAccessPitc.3780.sb")) == 0)
		return IDR_OTPACCESS3780_RESID;
/**
	// look it up in the resource table
	PSTFWRESINFO	pFwResInfo = NULL;
	int				iFwResInfoCount;
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;

    hResInfo = FindResourceEx( NULL,
	    					L_STMP_RESINFO_TYPE,
		    				MAKEINTRESOURCE(IDR_BOUND_RESOURCE_TABLE_LEN),
			    			MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
	if ( hResInfo )
    {
	    hRes = LoadResource( NULL, hResInfo);
   		if ( hRes )
	   		pPtr = LockResource(hRes);
	   	if ( pPtr )
		{
       		iFwResInfoCount = *((int *)pPtr);

		    hResInfo = FindResourceEx( NULL,
								L_STMP_RESINFO_TYPE,
								MAKEINTRESOURCE(IDR_BOUND_RESOURCE_TABLE),
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
		    if ( hResInfo )
		    {
				hRes = LoadResource( NULL, hResInfo);
	    		if ( hRes )
   					pFwResInfo = (PSTFWRESINFO)LockResource(hRes);

				if ( pFwResInfo )
				{
					for (int i = 0; i < iFwResInfoCount; ++i)
						if( (pFwResInfo[i].wLangId == langID) &&
							(_tcscmp( _fname, pFwResInfo[i].szResourceName) == 0) )
							resId = pFwResInfo[i].iResId;
				}
			}
		}
   }
*/
	return resId;
}
