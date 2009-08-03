// StDdiApi.h: interface for the CStDdiApi class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STDDIAPI_H__343A6E6E_764F_415F_9746_7B39782BA186__INCLUDED_)
#define AFX_STDDIAPI_H__343A6E6E_764F_415F_9746_7B39782BA186__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef enum _DDI_COMMAND_SET {
	DDI_GET_PROTOCOL_VERSION				= 0x00,					
	DDI_GET_STATUS							= 0x01,							
	DDI_GET_LOGICAL_MEDIA_INFO				= 0x02,							
	DDI_GET_ALLOCATION_TABLE				= 0x05,							
	DDI_SET_ALLOCATE_TABLE					= 0x06,									
	DDI_ERASE_LOGICAL_MEDIA					= 0x07,									
	DDI_GET_LOGICAL_DRIVE_INFO				= 0x12,									
	DDI_READ_LOGICAL_DRIVE_SECTOR			= 0x13,									
	DDI_SET_LOGICAL_DRIVE_INFO				= 0x20,									
	DDI_WRITE_LOGICAL_DRIVE_SECTOR			= 0x23,
	DDI_ERASE_LOGICAL_DRIVE					= 0x2f,
	DDI_GET_CHIP_MAJOR_REV_ID				= 0x30,
	DDI_CHIP_RESET							= 0x31,
	DDI_GET_CHIP_SERIAL_NUMBER_INFO			= 0x32,
    DDI_GET_CHIP_PART_REV_ID                = 0x36,
	DDI_GET_ROM_REV_ID      				= 0x37,
    DDI_GET_JANUS_STATUS                    = 0x40,
    DDI_INITIALIZE_JANUS                    = 0x41,
    DDI_RESET_TO_RECOVERY                   = 0x42,
	DDI_INIT_DATA_STORE						= 0x43,
    DDI_RESET_TO_UPDATER                    = 0x44,
	DDI_GET_DEVICE_PROPERTIES               = 0x45, // ProtocolUpdater2, ProtocolUpdaterJanus2
	DDI_SET_UPDATE_FLAG						= 0x46, // post SDK3.200, 35XX-ONLY
	DDI_FILTER_PING							= 0x9F
} DDI_COMMAND_SET;

#define ST_SCSIOP_READ_COMMAND				0xC0
#define ST_SCSIOP_WRITE_COMMAND				0xC1

#define GEN_DDI_COMMAND_SIZE				16
#define INQUIRY_COMMAND_SIZE				6
#define READ_CAPACITY_COMMAND_SIZE			10
#define READ_COMMAND_SIZE					10
#define WRITE_COMMAND_SIZE					10

#define ST_SCSI_DEFAULT_TIMEOUT     2*60   // 2 minutes
#define ST_SCSI_EXTENDED_TIMEOUT    10*60  // 10 minutes

typedef enum _PROTOCOL_VERSIONS {
    ST_UPDATER					= 1,  // full featured updater (pre SDK3.0)
    ST_LIMITED_HOSTLINK			= 2,  // limited hostlink capabilities
									  // (not all vendor cmds supported; cannot update f/w
	ST_UPDATER_ADVANCED			= 3,  // normal updater without advanced scsi functions
    ST_UPDATER_JANUS			= 4,   // full featured updater with Janus support
	ST_UPDATER_JANUS_ADVANCED	= 5,   // add advanced scsi functions
	ST_UPDATER_RAMLESS_JANUS	= 6,   // requires boot of updater.sb from ROM to do Janus
										// initialization.  Do normal update w/o Janus, ResetToUpdater,
										// and then Janus init.
    ST_UPDATER_JANUS_7	        = 7   // Save/Restore Janus Drive. Janus header not required.

} PROTOCOL_VERSIONS;

typedef enum _ROM_REVISION_TYPE {
    TA1,
    TA2,
    TA3,
    TA3A,
    TA4,
    TA5,
    TA6
} ROM_REVISION_TYPE;

// Device properties types.  This is the third byte in the DDI_GET_DEVICE_PROPERTIES
// command specifying what type if data we want.  Data transfer lengths must be
// set accordingly.
typedef enum
{
  eDevicePhysicalExternalRamSz,  // 4 byte word
  eDeviceVirtualExternalRamSz      // 4 byte word
} STMP_DEVICE_PROPERTIES_TYPE; 


class CStDdiApi : public CStBase 
{

public:

	CStDdiApi(size_t cmd_size = GEN_DDI_COMMAND_SIZE, size_t _data_length=0, string name="CStDdiApi");
	CStDdiApi(const CStDdiApi&);
	virtual ~CStDdiApi();
	CStDdiApi& operator=(const CStDdiApi&);
	virtual	void PrepareCommand() = 0;
	virtual	ST_ERROR ProcessResponse(CStByteArray& arr);
	CStByteArray* GetCommandArray();
	ST_ERROR GetResponseSize(size_t& size);
	size_t GetCommandSize() {return m_cmd_size;}	
	virtual BOOL IsCommandTypeWrite() {return FALSE;}

    ULONG           m_timeout;

protected:
	
	void Trash();
	CStByteArray*	m_p_arr_cmd;
	CStByteArray*	m_p_arr_res;
	size_t			m_cmd_size;
	size_t			m_res_size;
	size_t			m_data_length;

};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetProtocolVersion class.
//
//////////////////////////////////////////////////////////////////////
class CStGetProtocolVersion : public CStDdiApi {

public:

	CStGetProtocolVersion(string name="CStGetProtocolVersion");
	virtual ~CStGetProtocolVersion();

	virtual	void PrepareCommand();

	ST_ERROR GetMajorVersion(UCHAR&);
	ST_ERROR GetMinorVersion(UCHAR&);

};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetChipMajorRevId class.
//
//////////////////////////////////////////////////////////////////////
class CStGetChipMajorRevId : public CStDdiApi {

public:

	CStGetChipMajorRevId(string name="CStGetChipMajorRevId");
	virtual ~CStGetChipMajorRevId();

	virtual	void PrepareCommand();

	ST_ERROR GetChipMajorRevId(USHORT&);

};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetChipPartRevId class.
//
//////////////////////////////////////////////////////////////////////
class CStGetChipPartRevId : public CStDdiApi {

public:

	CStGetChipPartRevId(string name="CStGetChipPartRevId");
	virtual ~CStGetChipPartRevId();

	virtual	void PrepareCommand();

	ST_ERROR GetChipPartRevId(USHORT&);

};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetROMRevId class.
//
//////////////////////////////////////////////////////////////////////
class CStGetROMRevId : public CStDdiApi {

public:

	CStGetROMRevId(string name="CStGetROMRevId");
	virtual ~CStGetROMRevId();

	virtual	void PrepareCommand();

	ST_ERROR GetROMRevId(USHORT&);

};


//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetStatus class.
//
//////////////////////////////////////////////////////////////////////
class CStGetStatus : public CStDdiApi {

public:

	CStGetStatus(string name="CStGetStatus");
	virtual ~CStGetStatus();

	virtual	void PrepareCommand();

	ST_ERROR GetStatus(USHORT&);

};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetLogicalMediaInfo class.
//
//////////////////////////////////////////////////////////////////////
class CStGetLogicalMediaInfo : public CStDdiApi {

public:

	CStGetLogicalMediaInfo(LOGICAL_MEDIA_INFO TypeInfo, size_t res_size=0, 
		string name="CStGetLogicalMediaInfo");
	virtual ~CStGetLogicalMediaInfo();

	virtual	void PrepareCommand();

	ST_ERROR GetNumberOfDrives(USHORT& num);
	ST_ERROR GetPhysicalMediaType(PHYSICAL_MEDIA_TYPE& phys_media_type);
	ST_ERROR IsWriteProtected(ST_BOOLEAN& write_protected);
	ST_ERROR GetSizeInBytes(ULONGLONG& size);
	ST_ERROR GetSizeOfSerialNumber(ULONG& size);
	ST_ERROR GetSerialNumber(CStByteArray& arr);
	ST_ERROR IsSystemMedia(ST_BOOLEAN& media_system);
	// NAND media type specific commands
	ST_ERROR GetNandMfgId(ULONG& nandMfgId);
	ST_ERROR GetNandIdDetails(ULONGLONG& nandIdDetails);
	ST_ERROR GetNandPageSizeInBytes(ULONG& nandPageSize);
	ST_ERROR GetNandChipEnables(ULONG& nandchipenables);

	LOGICAL_MEDIA_INFO m_type;

};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetLogicalTable class.
//
//////////////////////////////////////////////////////////////////////
class CStGetLogicalTable : public CStDdiApi {

public:

	CStGetLogicalTable(UCHAR num_tables_to_read=MAX_MEDIA_TABLE_ENTRIES, 
		string name="CStGetLogicalTable");
	virtual ~CStGetLogicalTable();

	virtual	void PrepareCommand();
	ST_ERROR GetNumDrives(USHORT&);
	ST_ERROR GetDriveType(UCHAR drive_number, LOGICAL_DRIVE_TYPE& type);
	ST_ERROR GetDriveTag(UCHAR drive_number, UCHAR& tag);
	ST_ERROR GetDriveSizeInBytes(UCHAR drive_number, ULONGLONG& size_in_bytes);
	ST_ERROR GetDriveNumber(UCHAR index, UCHAR& DriveNumber);
	ST_ERROR GetTable(MEDIA_ALLOCATION_TABLE& _table);
	virtual	ST_ERROR ProcessResponse(CStByteArray& arr);
	
private:

	UCHAR					m_num_tables_to_read;
	MEDIA_ALLOCATION_TABLE	m_table;

	ST_ERROR ReadTableEntry(UCHAR drive_number, MEDIA_ALLOCATION_TABLE_ENTRY& entry);
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStAllocateLogicalMedia class.
//
//////////////////////////////////////////////////////////////////////
class CStAllocateLogicalMedia : public CStDdiApi {

public:

	CStAllocateLogicalMedia(P_MEDIA_ALLOCATION_TABLE pSetTable, 
		string name="CStAllocateLogicalMedia");
	virtual ~CStAllocateLogicalMedia();

	virtual	void PrepareCommand();

	virtual BOOL IsCommandTypeWrite() {return TRUE;}

private:

	P_MEDIA_ALLOCATION_TABLE m_p_media_alloc_table;

	ST_ERROR WriteTableEntry(UCHAR drive_number, MEDIA_ALLOCATION_TABLE_ENTRY entry);
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStEraseLogicalMedia class.
//
//////////////////////////////////////////////////////////////////////
class CStEraseLogicalMedia : public CStDdiApi {

public:

	CStEraseLogicalMedia(string name="CStEraseLogicalMedia");
	virtual ~CStEraseLogicalMedia();

	virtual	void PrepareCommand();
    virtual void SetPreserveHiddenDrive( BOOL preserve ) {m_bPreserveHiddenDrive = preserve;}
	virtual BOOL IsCommandTypeWrite() {return TRUE;}

private:
    BOOL    m_bPreserveHiddenDrive;
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetLogicalDriveInfo class.
//
//////////////////////////////////////////////////////////////////////
class CStGetLogicalDriveInfo : public CStDdiApi {

public:

	CStGetLogicalDriveInfo(UCHAR drive_num, LOGICAL_DRIVE_INFO type, size_t res_size, 
		string name="CStGetLogicalDriveInfo");
	virtual ~CStGetLogicalDriveInfo();

	virtual	void PrepareCommand();
    ST_ERROR GetSectorSize(ULONG& size);
    ST_ERROR GetEraseSizeInBytes(ULONG& size);
    ST_ERROR GetSizeInBytes(ULONGLONG& size);
    ST_ERROR GetSizeInMegaBytes(ULONG& size);
    ST_ERROR GetSizeInSectors(ULONGLONG& size);
    ST_ERROR GetType(LOGICAL_DRIVE_TYPE& type);
    ST_ERROR GetTag(UCHAR& tag);
    ST_ERROR GetComponentVersion(CStVersionInfo& ver);
    ST_ERROR GetProjectVersion(CStVersionInfo& ver);
    ST_ERROR IsWriteProtected(ST_BOOLEAN& write_protected);
    ST_ERROR GetSizeOfSerialNumberInBytes(USHORT&);
    ST_ERROR GetSerialNumber(CStByteArray& arr);

private:

	UCHAR				m_drive_number;
	LOGICAL_DRIVE_INFO	m_type;
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStSetLogicalDriveInfo class.
//
//////////////////////////////////////////////////////////////////////
class CStSetLogicalDriveInfo : public CStDdiApi {

public:

	CStSetLogicalDriveInfo(UCHAR drive_num, LOGICAL_DRIVE_INFO type, 
		string name="CStSetLogicalDriveInfo");
	virtual ~CStSetLogicalDriveInfo();

	virtual	void PrepareCommand();
    ST_ERROR SetTag(UCHAR tag);
    ST_ERROR SetComponentVersion(CStVersionInfo ver);
    ST_ERROR SetProjectVersion(CStVersionInfo ver);

	virtual BOOL IsCommandTypeWrite() {return TRUE;}

	UCHAR				m_drive_number;
	LOGICAL_DRIVE_INFO	m_type;
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStReadLogicalDriveSector class.
//
//////////////////////////////////////////////////////////////////////
class CStReadLogicalDriveSector : public CStDdiApi {

public:

	CStReadLogicalDriveSector(UCHAR drive_num, ULONG sector_size, 
		ULONGLONG sector_start, ULONG sector_count, string name="CStReadLogicalDriveSector");
	virtual ~CStReadLogicalDriveSector();

	virtual	void PrepareCommand();
	ST_ERROR GetData(CStByteArray& arr);

private:
	
	ULONGLONG	m_sector_start;
	ULONG		m_sector_count;
	UCHAR		m_drive_number;
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStWriteLogicalDriveSector class.
//
//////////////////////////////////////////////////////////////////////
class CStWriteLogicalDriveSector : public CStDdiApi {

public:

	CStWriteLogicalDriveSector(UCHAR drive_num, ULONG sector_size, ULONGLONG sector_start, 
		ULONG sector_count, string name="CStWriteLogicalDriveSector");
	virtual ~CStWriteLogicalDriveSector();

	virtual	void PrepareCommand();
	ST_ERROR PutData(CStByteArray& arr);

	virtual BOOL IsCommandTypeWrite() {return TRUE;}
private:
	
	ULONG		m_sector_count;
	ULONGLONG	m_sector_start;
	UCHAR		m_drive_number;
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStEraseLogicalDrive class.
//
//////////////////////////////////////////////////////////////////////
class CStEraseLogicalDrive : public CStDdiApi {

public:

	CStEraseLogicalDrive(UCHAR drive_num, string name="CStEraseLogicalDrive");
	virtual ~CStEraseLogicalDrive();

	virtual	void PrepareCommand();

	virtual BOOL IsCommandTypeWrite() {return TRUE;}
	UCHAR m_drive_number;
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStFilterPing class.
//
//////////////////////////////////////////////////////////////////////
class CStFilterPing : public CStDdiApi {

public:

	CStFilterPing(string name="CStFilterPing");
	virtual ~CStFilterPing();

	virtual	void PrepareCommand();
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for CStScsiInquiry class.
//
//////////////////////////////////////////////////////////////////////
class CStScsiInquiry : public CStDdiApi {

public:

	CStScsiInquiry(string name="CStScsiInquiry");
	virtual ~CStScsiInquiry();

	virtual	void PrepareCommand();
	ST_ERROR GetInquiryData(PINQUIRYDATA _p_inq_data);
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for CStReadCapacity class.
//
//////////////////////////////////////////////////////////////////////
class CStReadCapacity : public CStDdiApi {

public:

	CStReadCapacity(string name="CStReadCapacity");
	virtual ~CStReadCapacity();

	virtual	void PrepareCommand();
	ST_ERROR GetCapacity(PREAD_CAPACITY_DATA _p_read_capacity);
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for CStRead class.
//
//////////////////////////////////////////////////////////////////////
class CStRead : public CStDdiApi {

public:

	CStRead(ULONG _start_sector, ULONG _sectors_to_read, ULONG _sector_size, string name="CStRead");
	virtual ~CStRead();

	virtual	void PrepareCommand();
	ST_ERROR GetData(CStByteArray&);

private:

	ULONG m_sector_size;
	ULONG m_sectors_to_read;
	ULONG m_start_sector;
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for CStWrite class.
//
//////////////////////////////////////////////////////////////////////
class CStWrite : public CStDdiApi {

public:

	CStWrite(ULONG _start_sector, ULONG _sectors_to_write, ULONG _sector_size, string name="CStWrite");
	virtual ~CStWrite();

	virtual	void PrepareCommand();
	ST_ERROR PutData(CStByteArray&);

	virtual BOOL IsCommandTypeWrite() {return TRUE;}

private:

	ULONG m_sector_size;
	ULONG m_sectors_to_write;
	ULONG m_start_sector;
};
#endif // !defined(AFX_STDDIAPI_H__343A6E6E_764F_415F_9746_7B39782BA186__INCLUDED_)

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStChipReset class.
//
//////////////////////////////////////////////////////////////////////
class CStChipReset : public CStDdiApi {

public:

	CStChipReset(string name="CStChipReset");
	virtual ~CStChipReset();

	virtual	void PrepareCommand();
	
	virtual BOOL IsCommandTypeWrite() {return TRUE;}
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStResetToRecovery class.
//
//////////////////////////////////////////////////////////////////////
class CStResetToRecovery : public CStDdiApi {

public:

	CStResetToRecovery(string name="CStResetToRecovery");
	virtual ~CStResetToRecovery();

	virtual	void PrepareCommand();
	
	virtual BOOL IsCommandTypeWrite() {return TRUE;}
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStResetToUpdater class.
//
//////////////////////////////////////////////////////////////////////

class CStResetToUpdater : public CStDdiApi {

public:
    enum SerialNumberFlag { SER_NUM_DEFAULT = 0xFFFFFFFF, SER_NUM_NONE = 0, SER_NUM_ONE, SER_NUM_TWO, SER_NUM_THREE, SER_NUM_FOUR };

    CStResetToUpdater(SerialNumberFlag flag = SER_NUM_NONE, string name="CStResetToUpdater");
	virtual ~CStResetToUpdater();

	virtual	void PrepareCommand();
	
	virtual BOOL IsCommandTypeWrite() {return TRUE;}
private:
    SerialNumberFlag m_serialNumberFlag;
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStInitializeJanus class.
//
//////////////////////////////////////////////////////////////////////
class CStInitializeJanus : public CStDdiApi {

public:

	CStInitializeJanus(string name="CStInitializeJanus");
	virtual ~CStInitializeJanus();

	virtual	void PrepareCommand();

	virtual BOOL IsCommandTypeWrite() {return TRUE;}
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetJanusStatus.
//
//////////////////////////////////////////////////////////////////////
class CStGetJanusStatus : public CStDdiApi {

public:

	CStGetJanusStatus(string name="CStGetJanusStatus");
	virtual ~CStGetJanusStatus();

	virtual	void PrepareCommand();

	ST_ERROR GetJanusStatus(UCHAR&);
};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStInitializeDataStore class.
//
//////////////////////////////////////////////////////////////////////
class CStInitializeDataStore : public CStDdiApi {

public:

	CStInitializeDataStore(UCHAR ucDataStore, string name="CStInitializeDataStore");
	virtual ~CStInitializeDataStore();

	virtual	void PrepareCommand();

	virtual BOOL IsCommandTypeWrite() {return TRUE;}

private:
	UCHAR m_data_store;
};




//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetDeviceInfo class.
//
//////////////////////////////////////////////////////////////////////
class CStGetDeviceProperties : public CStDdiApi {

public:
	CStGetDeviceProperties(STMP_DEVICE_PROPERTIES_TYPE TypeProp,
		string name="CStGetDeviceInfo");
	virtual ~CStGetDeviceProperties();

	virtual	void PrepareCommand();

	ST_ERROR GetDevicePropExternalRAMSize(ULONG&);
	ST_ERROR GetDevicePropVirtualRAMSize(ULONG&);

	STMP_DEVICE_PROPERTIES_TYPE m_type;

};

//////////////////////////////////////////////////////////////////////
//
// StDdiApi.h: interface for the CStGetChipSerialNumber class.
//
//////////////////////////////////////////////////////////////////////
class CStGetChipSerialNumber : public CStDdiApi {

public:

	CStGetChipSerialNumber(SERIAL_NO_INFO _type_info, size_t _res_size=0, string name="CStGetChipSerialNumber");
	virtual ~CStGetChipSerialNumber();

	virtual	void PrepareCommand();

	ST_ERROR GetSizeOfSerialNumber(USHORT& size);
	ST_ERROR GetChipSerialNumber(CStByteArray * arr);

	SERIAL_NO_INFO m_type;
};