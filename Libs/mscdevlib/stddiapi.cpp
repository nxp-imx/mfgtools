/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StDdiApi.cpp: implementation of the CStDdiApi class.
//
//////////////////////////////////////////////////////////////////////

#include "stheader.h"
#include "StBase.h"
#include "StGlobals.h"
#include "StByteArray.h"
#include "ddildl_defs.h"
#include "StVersionInfo.h"
#include "StDdiApi.h"

extern USHORT forcedROMId;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStDdiApi::CStDdiApi(size_t _cmd_size, size_t _data_length, string _name):CStBase(_name)
{
	m_p_arr_cmd = NULL;
	m_p_arr_res = NULL;
	m_cmd_size = _cmd_size;
	m_res_size = 0;
	m_data_length = _data_length;
	m_p_arr_cmd = new CStByteArray(m_cmd_size + m_data_length);
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStDdiApi::CStDdiApi(const CStDdiApi& _api):CStBase( _api )
{
	m_p_arr_cmd = NULL;
	m_p_arr_res = NULL;
	*this = _api;
}

CStDdiApi::~CStDdiApi()
{
	Trash();
}

void CStDdiApi::Trash()
{
	if(m_p_arr_cmd)
	{
		delete m_p_arr_cmd;
		m_p_arr_cmd = NULL;
	}
	if(m_p_arr_res)
	{
		delete m_p_arr_res;
		m_p_arr_res = NULL;
	}
}

CStDdiApi& CStDdiApi::operator=(const CStDdiApi& _api)
{
	Trash();
	
	m_cmd_size		= _api.m_cmd_size;
	m_res_size		= _api.m_res_size;

	if( _api.m_p_arr_cmd )
	{
		m_p_arr_cmd = new CStByteArray( _api.m_p_arr_cmd->GetCount() );
		*m_p_arr_cmd = *_api.m_p_arr_cmd;
	}

	if( _api.m_p_arr_res )
	{
		m_p_arr_res = new CStByteArray( _api.m_p_arr_res->GetCount() );
		*m_p_arr_res = *_api.m_p_arr_res;
	}
	return *this;
}

ST_ERROR CStDdiApi::ProcessResponse(CStByteArray& _arr)
{
	m_res_size = _arr.GetCount();

	if(m_p_arr_res)
	{
		delete m_p_arr_res;
		m_p_arr_res = NULL;
	}

	m_p_arr_res = new CStByteArray(m_res_size);

	if( !m_p_arr_res )
	{
		m_last_error = STERR_NO_MEMORY;
		return m_last_error;
	}

	for(size_t index=0; index<m_res_size; index++)
	{
		UCHAR uch;
		_arr.GetAt(index, uch);
		m_p_arr_res->SetAt(index, uch);
	}
	
	return STERR_NONE;
}

CStByteArray* CStDdiApi::GetCommandArray()
{
	return m_p_arr_cmd;
}

ST_ERROR CStDdiApi::GetResponseSize(size_t& _size)
{
	_size = m_res_size;
	return STERR_NONE;
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetProtocolVersion class.
//
//////////////////////////////////////////////////////////////////////
CStGetProtocolVersion::CStGetProtocolVersion(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = sizeof(UCHAR) * 2;
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStGetProtocolVersion::~CStGetProtocolVersion()
{
}

void CStGetProtocolVersion::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_PROTOCOL_VERSION);
}

ST_ERROR CStGetProtocolVersion::GetMajorVersion(UCHAR& _major)
{
	return m_p_arr_res->GetAt(0, _major);
}

ST_ERROR CStGetProtocolVersion::GetMinorVersion(UCHAR& _minor)
{
	return m_p_arr_res->GetAt(1, _minor);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetChipMajorRevId class.
//
//////////////////////////////////////////////////////////////////////
CStGetChipMajorRevId::CStGetChipMajorRevId(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = sizeof(UCHAR) * 2;
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStGetChipMajorRevId::~CStGetChipMajorRevId()
{
}

void CStGetChipMajorRevId::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_CHIP_MAJOR_REV_ID);
}

ST_ERROR CStGetChipMajorRevId::GetChipMajorRevId(USHORT& _rev)
{
	return m_p_arr_res->Read(_rev, 0);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetPartRevId class.
//
//////////////////////////////////////////////////////////////////////
CStGetChipPartRevId::CStGetChipPartRevId(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = sizeof(UCHAR) * 2;
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStGetChipPartRevId::~CStGetChipPartRevId()
{
}

void CStGetChipPartRevId::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_CHIP_PART_REV_ID);
}

ST_ERROR CStGetChipPartRevId::GetChipPartRevId(USHORT& _rev)
{
	return m_p_arr_res->Read(_rev, 0);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetROMRevId class.
//
//////////////////////////////////////////////////////////////////////
CStGetROMRevId::CStGetROMRevId(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = sizeof(UCHAR) * 2;
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStGetROMRevId::~CStGetROMRevId()
{
}

void CStGetROMRevId::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_ROM_REV_ID);
}

ST_ERROR CStGetROMRevId::GetROMRevId(USHORT& _rev)
{
    if (forcedROMId)
    {
        _rev = forcedROMId;
        return STERR_NONE;
    }
    else
	    return m_p_arr_res->Read(_rev, 0);
}


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetStatus class.
//
//////////////////////////////////////////////////////////////////////
CStGetStatus::CStGetStatus(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = sizeof(USHORT);
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStGetStatus::~CStGetStatus()
{
}

void CStGetStatus::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_STATUS);
}

ST_ERROR CStGetStatus::GetStatus(USHORT& _sh)
{
	return m_p_arr_res->Read(_sh, 0);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetLogicalMediaInfo class.
//
//////////////////////////////////////////////////////////////////////

CStGetLogicalMediaInfo::CStGetLogicalMediaInfo(
	LOGICAL_MEDIA_INFO	_type_info, 
	size_t				_res_size, 
	string				_name
)	: CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
	m_type = _type_info;
	switch(m_type)
	{
    case MediaInfoNumberOfDrives:
		m_res_size = sizeof(USHORT);
		break;
    case MediaInfoPhysicalMediaType:
		m_res_size = sizeof(UINT);
		break;
    case MediaInfoIsWriteProtected:
		m_res_size = sizeof(ST_BOOLEAN);
		break;
    case MediaInfoSizeInBytes:
		m_res_size = sizeof(ULONGLONG);
		break;
    case MediaInfoSizeOfSerialNumberInBytes:
		m_res_size = sizeof(ULONG);
		break;
    case MediaInfoSerialNumber:  // DO NOT USE!!!  USE DDI_GET_CHIP_SERIAL_NUMBER_INFO
		m_res_size = _res_size;
		break;
	case MediaInfoIsSystemMedia:
		m_res_size = sizeof(ST_BOOLEAN);
		break;
	case MediaInfoNandMfgId:
		m_res_size = sizeof(UINT);
		break;
	case MediaInfoNandIdDetails:
		m_res_size = sizeof(ULONGLONG);
		break;
	case MediaInfoNandPageSizeInBytes:
		m_res_size = sizeof(UINT);
		break;
	case MediaInfoNandChipEnables:
		m_res_size = sizeof(UINT);
		break;
	default:
		this->m_last_error = STERR_INVALID_MEDIA_INFO_REQUEST;
		m_res_size = 0;
		return;
	}

}

CStGetLogicalMediaInfo::~CStGetLogicalMediaInfo()
{
}

void CStGetLogicalMediaInfo::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_LOGICAL_MEDIA_INFO);
	m_p_arr_cmd->SetAt(2, (UCHAR)m_type);
}

ST_ERROR CStGetLogicalMediaInfo::GetNumberOfDrives(USHORT& _num)
{
	if(m_type == MediaInfoNumberOfDrives)
	{
		return m_p_arr_res->Read(_num, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalMediaInfo::GetPhysicalMediaType(PHYSICAL_MEDIA_TYPE& _phys_media_type)
{
	UCHAR byType;
	if(m_type == MediaInfoPhysicalMediaType)
	{
		m_p_arr_res->GetAt(0, byType);
		_phys_media_type = (PHYSICAL_MEDIA_TYPE)byType;
		return STERR_NONE;
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}


ST_ERROR CStGetLogicalMediaInfo::IsWriteProtected(ST_BOOLEAN& _write_protected)
{
	if(m_type == MediaInfoNumberOfDrives)
	{
		return m_p_arr_res->Read(_write_protected, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalMediaInfo::GetSizeInBytes(ULONGLONG& _size)
{
	if(m_type == MediaInfoSizeInBytes)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}


ST_ERROR CStGetLogicalMediaInfo::GetSizeOfSerialNumber(ULONG& _size)
{
	if(m_type == MediaInfoSizeOfSerialNumberInBytes)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	else
		return STERR_INVALID_MEDIA_INFO_REQUEST;
}

//
// parameter arr should be initilized first with MediaInfoSizeOfSerialNumberInBytes to hold the serial number. 
//
ST_ERROR CStGetLogicalMediaInfo::GetSerialNumber(CStByteArray& _arr)
{
	if(m_type == MediaInfoSerialNumber)
	{
		UCHAR uch;
		for(size_t index = 0; index < m_p_arr_res->GetCount(); index ++ )
		{
			m_p_arr_res->GetAt(index, uch);
			_arr.SetAt(index, uch);
		}
		return STERR_NONE;
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalMediaInfo::IsSystemMedia(ST_BOOLEAN& _media_system)
{
	if(m_type == MediaInfoIsSystemMedia)
	{
		return m_p_arr_res->Read(_media_system, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalMediaInfo::GetNandPageSizeInBytes(ULONG& _pagesize)
{
	if(m_type == MediaInfoNandPageSizeInBytes)
	{
		return m_p_arr_res->Read(_pagesize, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalMediaInfo::GetNandMfgId(ULONG& _nandMfgId)
{
	if(m_type == MediaInfoNandMfgId)
	{
		return m_p_arr_res->Read(_nandMfgId, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalMediaInfo::GetNandIdDetails(ULONGLONG& _nandIdDetails)
{
	if(m_type == MediaInfoNandIdDetails)
	{
		ULONGLONG t;
		ST_ERROR err;
		err = m_p_arr_res->Read((PVOID) &t, sizeof(ULONGLONG), 0);
		_nandIdDetails = t;
		return err;
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalMediaInfo::GetNandChipEnables(ULONG& _nandChipEnables)
{
	if(m_type == MediaInfoNandChipEnables)
	{
		return m_p_arr_res->Read(_nandChipEnables, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetLogicalDriveInfo class.
//
//////////////////////////////////////////////////////////////////////

CStGetLogicalDriveInfo::CStGetLogicalDriveInfo(
	UCHAR				_drive_num, 
	LOGICAL_DRIVE_INFO	_type, 
	size_t				_res_size, 
	string				_name
)	: CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_type = _type;
	m_drive_number = _drive_num;
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;

	switch (m_type)
	{
    case DriveInfoSectorSizeInBytes:
		m_res_size = sizeof(ULONG);
		break;
    case DriveInfoEraseSizeInBytes:
		m_res_size = sizeof(ULONG);
		break;
    case DriveInfoSizeInBytes:
		m_res_size = sizeof(ULONGLONG);
		break;
    case DriveInfoSizeInMegaBytes:
		m_res_size = sizeof(ULONG);
		break;
    case DriveInfoSizeInSectors:
		m_res_size = sizeof(ULONGLONG);
		break;
    case DriveInfoType:
		m_res_size = sizeof(LOGICAL_DRIVE_TYPE);
		break;
    case DriveInfoTag:
		m_res_size = sizeof(UCHAR);
		break;
    case DriveInfoComponentVersion:
		m_res_size = sizeof(CStVersionInfo);
		break;
    case DriveInfoProjectVersion:
		m_res_size = sizeof(CStVersionInfo);
		break;
    case DriveInfoIsWriteProtected:
		m_res_size = sizeof(ST_BOOLEAN);
		break;
    case DriveInfoSizeOfSerialNumberInBytes:
		m_res_size = sizeof(USHORT);
		break;
    case DriveInfoSerialNumber:
		m_res_size = _res_size;
		break;
	default:
		this->m_last_error = STERR_INVALID_DRIVE_INFO_REQUEST;
		m_res_size = 0;
		return;
	}
}

CStGetLogicalDriveInfo::~CStGetLogicalDriveInfo()
{
}

void CStGetLogicalDriveInfo::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_LOGICAL_DRIVE_INFO);
	m_p_arr_cmd->SetAt(2, (UCHAR)m_drive_number);
	m_p_arr_cmd->SetAt(3, (UCHAR)m_type);
}

ST_ERROR CStGetLogicalDriveInfo::GetSectorSize(ULONG& _size)
{
	if(m_type == DriveInfoSectorSizeInBytes)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetEraseSizeInBytes(ULONG& _size)
{
	if(m_type == DriveInfoEraseSizeInBytes)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetSizeInBytes(ULONGLONG& _size)
{
	if(m_type == DriveInfoSizeInBytes)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetSizeInMegaBytes(ULONG& _size)
{
	if(m_type == DriveInfoSizeInMegaBytes)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetSizeInSectors(ULONGLONG& _size)
{
	if(m_type == DriveInfoSizeInSectors)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetType(LOGICAL_DRIVE_TYPE& _logical_drive_type)
{
	UCHAR byType;
	if(m_type == DriveInfoType)
	{
		m_p_arr_res->GetAt(0, byType);
		_logical_drive_type = (LOGICAL_DRIVE_TYPE)byType;
		return STERR_NONE;
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetTag(UCHAR& _tag)
{
	if(m_type == DriveInfoTag)
	{
		return m_p_arr_res->GetAt(0, _tag);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetComponentVersion(CStVersionInfo& _ver)
{
	if(m_type == DriveInfoComponentVersion)
	{
		USHORT sh;
		
		m_p_arr_res->Read(sh, 0);
		_ver.SetHigh(sh);

		m_p_arr_res->Read(sh, sizeof(USHORT));
		_ver.SetMid(sh);

		m_p_arr_res->Read(sh, sizeof(USHORT) * 2);
		_ver.SetLow(sh);

		return STERR_NONE;
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetProjectVersion(CStVersionInfo& _ver)
{
	if(m_type == DriveInfoProjectVersion)
	{
		USHORT sh;
		
		m_p_arr_res->Read(sh, 0);
		_ver.SetHigh(sh);

		m_p_arr_res->Read(sh, sizeof(USHORT));
		_ver.SetMid(sh);

		m_p_arr_res->Read(sh, sizeof(USHORT) * 2);
		_ver.SetLow(sh);

		return STERR_NONE;
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::IsWriteProtected(ST_BOOLEAN& _write_protected)
{
	if(m_type == DriveInfoIsWriteProtected)
	{
		return m_p_arr_res->Read(_write_protected, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetSizeOfSerialNumberInBytes(USHORT& _size)
{
	if(m_type == DriveInfoSizeOfSerialNumberInBytes)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

ST_ERROR CStGetLogicalDriveInfo::GetSerialNumber(CStByteArray& _arr)
{
	if(m_type == DriveInfoSerialNumber)
	{
		UCHAR uch;
		for(size_t index = 0; index < m_p_arr_res->GetCount(); index ++ )
		{
			m_p_arr_res->GetAt(index, uch);
			_arr.SetAt(index, uch);
		}
		return STERR_NONE;
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetLogicalTable class.
//
//////////////////////////////////////////////////////////////////////

CStGetLogicalTable::CStGetLogicalTable(
	UCHAR	_num_tables_to_read, 
	string	_name
)	: CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_num_tables_to_read = _num_tables_to_read;
	m_res_size = sizeof(USHORT) + m_num_tables_to_read * sizeof(MEDIA_ALLOCATION_TABLE_ENTRY);
	CStGlobals::MakeMemoryZero((PUCHAR)&m_table, sizeof(m_table));
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStGetLogicalTable::~CStGetLogicalTable()
{
}

void CStGetLogicalTable::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_ALLOCATION_TABLE);
	m_p_arr_cmd->SetAt(2, m_num_tables_to_read);
}

ST_ERROR CStGetLogicalTable::ProcessResponse(CStByteArray& _arr)
{
	ST_ERROR err = CStDdiApi::ProcessResponse(_arr);
	if( err != STERR_NONE )
		return err;

	err = m_p_arr_res->Read(m_table.wNumEntries, 0);
	if( err != STERR_NONE )
		return err;
	
	for(UCHAR index=0; index<m_table.wNumEntries; index++)
	{
		err = ReadTableEntry(index, m_table.Entry[index]);
		if( err != STERR_NONE )
			return err;
	}
	return err;
}

ST_ERROR CStGetLogicalTable::GetTable(MEDIA_ALLOCATION_TABLE& _table)
{
	_table.wNumEntries = m_table.wNumEntries;
	for(USHORT index=0; index<m_table.wNumEntries; index++)
	{
		_table.Entry[index].DriveNumber = m_table.Entry[index].DriveNumber;
		_table.Entry[index].SizeInBytes = m_table.Entry[index].SizeInBytes;
		_table.Entry[index].Tag = m_table.Entry[index].Tag;
		_table.Entry[index].Type = m_table.Entry[index].Type;
	}

	return STERR_NONE;
}

ST_ERROR CStGetLogicalTable::GetNumDrives(USHORT& _num_drives)
{
	_num_drives = m_table.wNumEntries;
	return STERR_NONE;
}

ST_ERROR CStGetLogicalTable::GetDriveType(UCHAR _drive_number, LOGICAL_DRIVE_TYPE& _type)
{
	BOOL drive_found = FALSE;

	for(USHORT index=0; index<m_table.wNumEntries; index++)
	{
		if( m_table.Entry[index].DriveNumber == _drive_number )
		{	
			drive_found = TRUE;
			_type = (LOGICAL_DRIVE_TYPE)m_table.Entry[index].Type;
			break;
		}
	}

	if( !drive_found )
	{
		return STERR_FAILED_TO_FIND_DRIVE_NUMBER;
	}
	return STERR_NONE;
}

ST_ERROR CStGetLogicalTable::GetDriveTag(UCHAR _drive_number, UCHAR& _tag)
{
	BOOL drive_found = FALSE;

	for(USHORT index=0; index<m_table.wNumEntries; index++)
	{
		if( m_table.Entry[index].DriveNumber == _drive_number )
		{	
			drive_found = TRUE;
			_tag = m_table.Entry[index].Tag;
			break;
		}
	}

	if( !drive_found )
	{
		return STERR_FAILED_TO_FIND_DRIVE_NUMBER;
	}
	return STERR_NONE;
}

ST_ERROR CStGetLogicalTable::GetDriveSizeInBytes(UCHAR _drive_number, ULONGLONG& _size)
{
	BOOL drive_found = FALSE;

	for(USHORT index=0; index<m_table.wNumEntries; index++)
	{
		if( m_table.Entry[index].DriveNumber == _drive_number )
		{	
			drive_found = TRUE;
			_size = m_table.Entry[index].SizeInBytes;
			break;
		}
	}

	if( !drive_found )
	{
		return STERR_FAILED_TO_FIND_DRIVE_NUMBER;
	}
	return STERR_NONE;
}

ST_ERROR CStGetLogicalTable::ReadTableEntry(UCHAR _index, MEDIA_ALLOCATION_TABLE_ENTRY& _entry)
{
	MEDIA_ALLOCATION_TABLE table;
	ST_ERROR err = STERR_NONE;
	long from_index = sizeof(table.wNumEntries) + (_index * sizeof(MEDIA_ALLOCATION_TABLE_ENTRY));

	err = m_p_arr_res->GetAt(from_index, _entry.DriveNumber);
	if( err != STERR_NONE )
		return err;
	from_index += sizeof(_entry.DriveNumber);
	
	err = m_p_arr_res->GetAt(from_index, _entry.Type);
	if( err != STERR_NONE )
		return err;
	from_index += sizeof(_entry.Type);
	
	err = m_p_arr_res->GetAt(from_index, _entry.Tag);
	if( err != STERR_NONE )
		return err;
	from_index += sizeof(_entry.Tag);

	return (m_p_arr_res->Read(_entry.SizeInBytes, from_index));
	UNREFERENCED_PARAMETER(table);
}

ST_ERROR CStGetLogicalTable::GetDriveNumber(UCHAR _index, UCHAR& _drive_number)
{
	MEDIA_ALLOCATION_TABLE table;
	size_t from_index = sizeof(table.wNumEntries) + (_index * sizeof(MEDIA_ALLOCATION_TABLE_ENTRY));

	return m_p_arr_res->Read((void*) &_drive_number, sizeof(MEDIA_ALLOCATION_TABLE_ENTRY), from_index);
	UNREFERENCED_PARAMETER(table);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStAllocateLogicalMedia class.
//
//////////////////////////////////////////////////////////////////////
CStAllocateLogicalMedia::CStAllocateLogicalMedia(P_MEDIA_ALLOCATION_TABLE _p_table, string _name):
	CStDdiApi(GEN_DDI_COMMAND_SIZE, _p_table->wNumEntries * sizeof(MEDIA_ALLOCATION_TABLE_ENTRY), _name)
{
	m_p_media_alloc_table = _p_table;
	m_res_size = 0; 
	m_data_length = _p_table->wNumEntries * sizeof(MEDIA_ALLOCATION_TABLE_ENTRY);
    m_timeout = ST_SCSI_EXTENDED_TIMEOUT/*ST_SCSI_DEFAULT_TIMEOUT*/;
}

CStAllocateLogicalMedia::~CStAllocateLogicalMedia()
{
}

void CStAllocateLogicalMedia::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_SET_ALLOCATE_TABLE);
	m_p_arr_cmd->SetAt(2, (UCHAR)m_p_media_alloc_table->wNumEntries);
	for(UCHAR index = 0; index < m_p_media_alloc_table->wNumEntries; index ++ )
	{
		WriteTableEntry(index, m_p_media_alloc_table->Entry[index]);
	}
}

ST_ERROR CStAllocateLogicalMedia::WriteTableEntry(UCHAR _drive_number, MEDIA_ALLOCATION_TABLE_ENTRY _entry)
{
	ST_ERROR err = STERR_NONE;
	size_t from_index = GEN_DDI_COMMAND_SIZE + (_drive_number * (sizeof(_entry.Type) 
		+ sizeof(_entry.Tag) + sizeof(_entry.SizeInBytes)));
/*
	err = m_p_arr_cmd->SetAt( from_index, _entry.DriveNumber );
	if( err != STERR_NONE )
		return err;

	from_index += sizeof(_entry.DriveNumber);
*/	err = m_p_arr_cmd->SetAt( from_index, _entry.Type );
	if( err != STERR_NONE )
		return err;

	from_index += sizeof(_entry.Type);
	err = m_p_arr_cmd->SetAt( from_index, _entry.Tag );
	if( err != STERR_NONE )
		return err;

	from_index += sizeof(_entry.Tag);
	err = m_p_arr_cmd->Write( _entry.SizeInBytes, from_index );

	return err;
}


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStEraseLogicalMedia class.
//
//////////////////////////////////////////////////////////////////////
CStEraseLogicalMedia::CStEraseLogicalMedia(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = 0;
    m_bPreserveHiddenDrive = FALSE;
    m_timeout = ST_SCSI_EXTENDED_TIMEOUT/*ST_SCSI_DEFAULT_TIMEOUT*/;
}

CStEraseLogicalMedia::~CStEraseLogicalMedia()
{
}

void CStEraseLogicalMedia::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_ERASE_LOGICAL_MEDIA);
	m_p_arr_cmd->SetAt(2, (UCHAR)m_bPreserveHiddenDrive);
}


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStReadLogicalDriveSector class.
//
//////////////////////////////////////////////////////////////////////
CStReadLogicalDriveSector::CStReadLogicalDriveSector(
	UCHAR		_drive_num, 
	ULONG		_sector_size, 
	ULONGLONG	_sector_start, 
	ULONG		_sector_count, 
	string		_name
)	: CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_sector_count = _sector_count;
	m_sector_start = _sector_start;
	m_drive_number = _drive_num;
	m_res_size = (size_t)(m_sector_count * _sector_size); 
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStReadLogicalDriveSector::~CStReadLogicalDriveSector()
{
}

void CStReadLogicalDriveSector::PrepareCommand()
{	
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_READ_LOGICAL_DRIVE_SECTOR);
	m_p_arr_cmd->SetAt(2, m_drive_number);
	m_p_arr_cmd->Write(m_sector_start, 3);
	m_p_arr_cmd->Write(m_sector_count, 3+sizeof(m_sector_start));
}

ST_ERROR CStReadLogicalDriveSector::GetData(CStByteArray& _arr)
{
	if( GetLastError() != STERR_NONE )
		return GetLastError();

	for(size_t index = 0; (index < _arr.GetCount() || index < m_p_arr_res->GetCount()); index ++ )
	{
		UCHAR uch;
		m_p_arr_res->GetAt(index, uch);
		_arr.SetAt(index, uch);
	}
	return STERR_NONE;
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStWriteLogicalDriveSector class.
//
//////////////////////////////////////////////////////////////////////
CStWriteLogicalDriveSector::CStWriteLogicalDriveSector(
	UCHAR		_drive_num, 
	ULONG		_sector_size, 
	ULONGLONG	_sector_start, 
	ULONG		_sector_count, 
	string		_name
)	: CStDdiApi((size_t)GEN_DDI_COMMAND_SIZE, (size_t)_sector_count * _sector_size, _name)
{
	m_sector_count = _sector_count;
	m_sector_start = _sector_start;
	m_drive_number = _drive_num;
	m_res_size = 0; 
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStWriteLogicalDriveSector::~CStWriteLogicalDriveSector()
{
}

void CStWriteLogicalDriveSector::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);

    m_p_arr_cmd->SetAt(1, DDI_WRITE_LOGICAL_DRIVE_SECTOR);

	m_p_arr_cmd->SetAt(2, m_drive_number);
	m_p_arr_cmd->Write(m_sector_start, 3);
	m_p_arr_cmd->Write(m_sector_count, 3+sizeof(m_sector_start));
}

ST_ERROR CStWriteLogicalDriveSector::PutData(CStByteArray& _arr)
{
	if(_arr.GetCount() > (m_p_arr_cmd->GetCount() - GEN_DDI_COMMAND_SIZE))
	{
		return STERR_PUT_DATA_SIZE_EXCEEDS_ARRAY_SIZE;
	}
	for(size_t index=0, index_to_cmd = GEN_DDI_COMMAND_SIZE; index<_arr.GetCount();
		index++, index_to_cmd++)
	{
		UCHAR uch;
		_arr.GetAt(index, uch);
		m_p_arr_cmd->SetAt(index_to_cmd, uch);
	}
	return STERR_NONE;
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStSetLogicalDriveInfo class.
//
//////////////////////////////////////////////////////////////////////
CStSetLogicalDriveInfo::CStSetLogicalDriveInfo(
	UCHAR				_drive_num, 
	LOGICAL_DRIVE_INFO	_type,
	string				_name
)	: CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_type = _type;
	m_drive_number = _drive_num;

	m_res_size = 0; 
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStSetLogicalDriveInfo::~CStSetLogicalDriveInfo()
{
}

void CStSetLogicalDriveInfo::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_SET_LOGICAL_DRIVE_INFO);
	m_p_arr_cmd->SetAt(2, m_drive_number);
	m_p_arr_cmd->SetAt(3, (UCHAR)m_type);
}

ST_ERROR CStSetLogicalDriveInfo::SetTag(UCHAR _tag)
{
	PrepareCommand();
	m_p_arr_cmd->SetAt(4, (UCHAR)_tag);
	return STERR_NONE;
}

ST_ERROR CStSetLogicalDriveInfo::SetComponentVersion(CStVersionInfo _ver)
{
	PrepareCommand();
	m_p_arr_cmd->Write(_ver.GetHigh(), 4);
	m_p_arr_cmd->Write(_ver.GetMid(), 4 + sizeof(USHORT));
	m_p_arr_cmd->Write(_ver.GetLow(), 4 + sizeof(USHORT) * 2 );
	return STERR_NONE;
}

ST_ERROR CStSetLogicalDriveInfo::SetProjectVersion(CStVersionInfo _ver)
{
	PrepareCommand();
	m_p_arr_cmd->Write(_ver.GetHigh(), 4);
	m_p_arr_cmd->Write(_ver.GetMid(), 4 + sizeof(USHORT));
	m_p_arr_cmd->Write(_ver.GetLow(), 4 + sizeof(USHORT) * 2 );
	return STERR_NONE;
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStEraseLogicalDrive class.
//
//////////////////////////////////////////////////////////////////////
CStEraseLogicalDrive::CStEraseLogicalDrive(
	UCHAR	_drive_num, 
	string	_name
)	: CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_drive_number = _drive_num;
	m_res_size = 0; 
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStEraseLogicalDrive::~CStEraseLogicalDrive()
{
}

void CStEraseLogicalDrive::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_ERASE_LOGICAL_DRIVE);
	m_p_arr_cmd->SetAt(2, m_drive_number);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStFilterPing class.
//
//////////////////////////////////////////////////////////////////////
CStFilterPing::CStFilterPing(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = 2; 
	m_p_arr_res = new CStByteArray(m_res_size);
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStFilterPing::~CStFilterPing()
{
	if (m_p_arr_res)
	{
		delete m_p_arr_res;
		m_p_arr_res = NULL;
	}

}

void CStFilterPing::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_FILTER_PING);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStScsiInquiry class.
//
//////////////////////////////////////////////////////////////////////
CStScsiInquiry::CStScsiInquiry(string _name):CStDdiApi(INQUIRY_COMMAND_SIZE, 0, _name)
{
	m_res_size = sizeof(INQUIRYDATA); 
	m_p_arr_res = new CStByteArray(m_res_size);
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStScsiInquiry::~CStScsiInquiry()
{
	if (m_p_arr_res)
	{
		delete m_p_arr_res;
		m_p_arr_res = NULL;
	}

}

void CStScsiInquiry::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, SCSIOP_INQUIRY);
	m_p_arr_cmd->SetAt(4, sizeof(INQUIRYDATA));
}

ST_ERROR CStScsiInquiry::GetInquiryData(PINQUIRYDATA _p_inq_data)
{
	return m_p_arr_res->Read((void*)_p_inq_data, sizeof(INQUIRYDATA), 0);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStReadCapacity class.
//
//////////////////////////////////////////////////////////////////////
CStReadCapacity::CStReadCapacity(string _name):CStDdiApi(READ_CAPACITY_COMMAND_SIZE, 0, _name)
{
	m_res_size = sizeof(READ_CAPACITY_DATA); 
	m_p_arr_res = new CStByteArray(m_res_size);
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStReadCapacity::~CStReadCapacity()
{
	if (m_p_arr_res)
	{
		delete m_p_arr_res;
		m_p_arr_res = NULL;
	}
}

void CStReadCapacity::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, SCSIOP_READ_CAPACITY);
}

ST_ERROR CStReadCapacity::GetCapacity(PREAD_CAPACITY_DATA _p_read_capacity)
{
	m_p_arr_res->Read(_p_read_capacity->LogicalBlockAddress, 0);
	return m_p_arr_res->Read(_p_read_capacity->BytesPerBlock, sizeof(_p_read_capacity->LogicalBlockAddress));
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStRead class.
//
//////////////////////////////////////////////////////////////////////
CStRead::CStRead(ULONG _start_sector, ULONG _sectors_to_read, ULONG _sector_size, string _name):
	CStDdiApi(READ_COMMAND_SIZE, 0, _name)
{
	m_start_sector		= _start_sector;
	m_sectors_to_read	= _sectors_to_read;
	m_sector_size		= _sector_size;

	m_res_size			= _sectors_to_read * _sector_size; 

	m_p_arr_res			= new CStByteArray(m_res_size);
    m_timeout           = ST_SCSI_DEFAULT_TIMEOUT;
}

CStRead::~CStRead()
{
	if (m_p_arr_res)
	{
		delete m_p_arr_res;
		m_p_arr_res = NULL;
	}
}

void CStRead::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, SCSIOP_READ);
	m_p_arr_cmd->Write(m_start_sector, 2);
	m_p_arr_cmd->Write((USHORT)m_sectors_to_read, 7);
}

ST_ERROR CStRead::GetData(CStByteArray& _arr)
{
	for(size_t index = 0; (index < _arr.GetCount() || index < m_p_arr_res->GetCount()); index ++ )
	{
		UCHAR uch;
		m_p_arr_res->GetAt(index, uch);
		_arr.SetAt(index, uch);
	}

	return STERR_NONE;
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStWrite class.
//
//////////////////////////////////////////////////////////////////////
CStWrite::CStWrite(ULONG _start_sector, ULONG _sectors_to_write, ULONG _sector_size, string _name)
	:CStDdiApi((size_t)WRITE_COMMAND_SIZE, _sectors_to_write * _sector_size, _name)
{
	m_start_sector		= _start_sector;
	m_sectors_to_write	= _sectors_to_write;
	m_sector_size		= _sector_size;

	m_res_size			= 0; 

	m_p_arr_res			= NULL;//new CStByteArray(m_res_size);
    m_timeout           = ST_SCSI_DEFAULT_TIMEOUT;
}

CStWrite::~CStWrite()
{
}

void CStWrite::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, SCSIOP_WRITE);
	m_p_arr_cmd->Write(m_start_sector, 2);
	m_p_arr_cmd->Write((USHORT)m_sectors_to_write, 7);
}

ST_ERROR CStWrite::PutData(CStByteArray& _arr)
{
	if(_arr.GetCount() > (m_p_arr_cmd->GetCount() - WRITE_COMMAND_SIZE))
	{
		return STERR_PUT_DATA_SIZE_EXCEEDS_ARRAY_SIZE;
	}
	for(size_t index=0, index_to_cmd = WRITE_COMMAND_SIZE; index<_arr.GetCount(); 
		index++, index_to_cmd++)
	{
		UCHAR uch;
		_arr.GetAt(index, uch);
		m_p_arr_cmd->SetAt(index_to_cmd, uch);
	}
	return STERR_NONE;
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStChipReset class.
//
//////////////////////////////////////////////////////////////////////
CStChipReset::CStChipReset(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = 0; 
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStChipReset::~CStChipReset()
{
}

void CStChipReset::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_CHIP_RESET);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStResetToRecovery class.
//
//////////////////////////////////////////////////////////////////////
CStResetToRecovery::CStResetToRecovery(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = 0; 
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStResetToRecovery::~CStResetToRecovery()
{
}

void CStResetToRecovery::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_RESET_TO_RECOVERY);
}


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStResetToUpdater class.
//
//////////////////////////////////////////////////////////////////////
CStResetToUpdater::CStResetToUpdater(SerialNumberFlag flag, string _name)
: CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = 0; 
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
    m_serialNumberFlag = flag;
}

CStResetToUpdater::~CStResetToUpdater()
{
}

void CStResetToUpdater::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_RESET_TO_UPDATER);
    m_p_arr_cmd->Write((ULONG)m_serialNumberFlag, 2);
}


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStInitializeJanus class.
//
//////////////////////////////////////////////////////////////////////
CStInitializeJanus::CStInitializeJanus(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = 0;
    m_timeout = ST_SCSI_EXTENDED_TIMEOUT;
}

CStInitializeJanus::~CStInitializeJanus()
{
}

void CStInitializeJanus::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_INITIALIZE_JANUS);
}


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetJanusStatus class.
//
//////////////////////////////////////////////////////////////////////
CStGetJanusStatus::CStGetJanusStatus(string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = sizeof(UCHAR);
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
}

CStGetJanusStatus::~CStGetJanusStatus()
{
}

void CStGetJanusStatus::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_JANUS_STATUS);
}

ST_ERROR CStGetJanusStatus::GetJanusStatus(UCHAR& _rev)
{
	return m_p_arr_res->Read(_rev, 0);
}

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStInitializeDataStore class.
//
//////////////////////////////////////////////////////////////////////
CStInitializeDataStore::CStInitializeDataStore(UCHAR ucDataStore,string _name):CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_res_size = 0;
	m_data_store = ucDataStore;
    m_timeout = ST_SCSI_EXTENDED_TIMEOUT;
}

CStInitializeDataStore::~CStInitializeDataStore()
{
}

void CStInitializeDataStore::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_WRITE_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_INIT_DATA_STORE);
	m_p_arr_cmd->SetAt(2, m_data_store); // initialize zero based store number
}


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetDeviceProperties class.
//
//////////////////////////////////////////////////////////////////////
CStGetDeviceProperties::CStGetDeviceProperties(
	STMP_DEVICE_PROPERTIES_TYPE	_type_property, 
	string					_name)
	: CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
    m_timeout = ST_SCSI_DEFAULT_TIMEOUT;
	m_type = _type_property;
	switch(m_type)
	{
		case eDevicePhysicalExternalRamSz:
		case eDeviceVirtualExternalRamSz:
			// 4 byte word returned
			m_res_size = sizeof(ULONG);
			break;
	}

}

CStGetDeviceProperties::~CStGetDeviceProperties()
{
}

void CStGetDeviceProperties::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_DEVICE_PROPERTIES);
	m_p_arr_cmd->SetAt(2, (UCHAR)m_type);
}



ST_ERROR CStGetDeviceProperties::GetDevicePropExternalRAMSize(ULONG& _pERAMSize)
{
	return m_p_arr_res->Read(_pERAMSize, 0, TRUE);
}

ST_ERROR CStGetDeviceProperties::GetDevicePropVirtualRAMSize(ULONG& _pVRAMSize)
{
	return m_p_arr_res->Read(_pVRAMSize, 0, TRUE);
}


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.cpp: implementation of the CStGetChipSerialNumber class.
//
//////////////////////////////////////////////////////////////////////
CStGetChipSerialNumber::CStGetChipSerialNumber(	SERIAL_NO_INFO	_type_info, size_t _res_size, string _name)
:CStDdiApi(GEN_DDI_COMMAND_SIZE, 0, _name)
{
	m_type = _type_info;
	switch(m_type)
	{
    case SerialNoInfoSizeOfSerialNumberInBytes:
		m_res_size = sizeof(USHORT);
		break;
    case SerialNoInfoSerialNumber:
		m_res_size = _res_size;
		break;
	default:
		this->m_last_error = STERR_INVALID_MEDIA_INFO_REQUEST;
		m_res_size = 0;
	}
}

CStGetChipSerialNumber::~CStGetChipSerialNumber()
{
}

void CStGetChipSerialNumber::PrepareCommand()
{
	m_p_arr_cmd->SetAt(0, ST_SCSIOP_READ_COMMAND);
	m_p_arr_cmd->SetAt(1, DDI_GET_CHIP_SERIAL_NUMBER_INFO);
	m_p_arr_cmd->SetAt(2, (UCHAR)m_type);
}

ST_ERROR CStGetChipSerialNumber::GetSizeOfSerialNumber(USHORT& _size)
{
	if(m_type == SerialNoInfoSizeOfSerialNumberInBytes)
	{
		return m_p_arr_res->Read(_size, 0);
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

//
// parameter arr should be initilized first with SerialNoInfoSizeOfSerialNumberInBytes to hold the serial number. 
//
ST_ERROR CStGetChipSerialNumber::GetChipSerialNumber(CStByteArray * _arr)
{
	if(m_type == SerialNoInfoSerialNumber)
	{
		UCHAR uch;
		for(size_t index = 0; index < m_p_arr_res->GetCount(); index ++ )
		{
			m_p_arr_res->GetAt(index, uch);
			_arr->SetAt(index, uch);
		}
		return STERR_NONE;
	}
	return STERR_INVALID_MEDIA_INFO_REQUEST;
}

