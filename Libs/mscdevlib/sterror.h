// StError.h: interface for the CStError class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STERROR_H__7D233368_1994_46FE_929E_DABDE043B00F__INCLUDED_)
#define AFX_STERROR_H__7D233368_1994_46FE_929E_DABDE043B00F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
																		
typedef enum _ST_ERROR{
	STERR_BEGIN													=	  0,
	STERR_NONE													=	  0,
	STERR_NO_MEMORY												=	 -1,
	STERR_INVALID_POS_IN_ARRAY									=	 -2,
	STERR_INVALID_REQUEST										=	 -3,		
	STERR_INVALID_DRIVE_TYPE									=	 -4,
	STERR_FUNCTION_NOT_SUPPORTED								=	 -5,
	STERR_DATA_INCOMPLETE										=	 -6,
	STERR_RES_ARRAY_UNINITIALIZED								=	 -7,
	STERR_INVALID_MEDIA_INFO_REQUEST							=	 -8,
	STERR_INVALID_DRIVE_INFO_REQUEST							=	 -9,
	STERR_PUT_DATA_SIZE_EXCEEDS_ARRAY_SIZE						=	-10,
	STERR_FAILED_TO_OPEN_FILE									=	-11,
	STERR_FAILED_TO_READ_FILE_DATA								=	-12,
	STERR_FAILED_TO_WRITE_FILE_DATA								=	-13,
	STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE				=	-14,
	STERR_FAILED_TO_CREATE_EVENT_OBJECT							=	-15,
	STERR_DEVICE_TIMEOUT										=	-16,
	STERR_FAILED_TO_OPEN_REGISTRY_KEY							=	-17,
	STERR_FAILED_TO_FIND_DRIVE_LETTER_IN_REGISTRY				=	-18,
	STERR_FAILED_TO_LOAD_WNASPI32								=	-19,
	STERR_FAILED_TO_GET_FUNCTION_PTR_IN_WNASPI32_DLL			=	-20,
	STERR_STATE_OF_WNASPI32_NOT_INITIALIZED						=	-21,
	STERR_FAILED_TO_LOCATE_SCSI_DEVICE							=	-22,
	STERR_FAILED_TO_SEND_SCSI_COMMAND							=	-23,
	STERR_INVALID_DEVICE_HANDLE									=	-24,
	STERR_FAILED_DEVICE_IO_CONTROL								=	-25,
	STERR_DEVICE_STATE_UNINITALIZED								=	-26,
	STERR_UNSUPPORTED_OPERATING_SYSTEM							=	-27,
	STERR_FAILED_TO_LOAD_STRING									=	-28,
	STERR_NULL_DRIVE_OBJECT										=	-29,
	STERR_FAILED_TO_FIND_DRIVE_NUMBER							=   -30,
	STERR_FAILED_TO_LOCK_THE_DRIVE								=	-31,
	STERR_FAILED_TO_UNLOCK_THE_DRIVE							=	-32,
	STERR_BAD_CHS_SOLUTION										=	-33,
	STERR_UNABLE_TO_CALCULATE_CHS								=	-34,
	STERR_UNABLE_TO_PACK_CHS									=	-35,	
	STERR_FAILED_TO_READ_SECTOR									=	-36,
	STERR_FAILED_TO_WRITE_SECTOR								=	-37,
	STERR_UNKNOWN_ERROR											=	-38,
	STERR_INVALID_FILE_SYSTEM_REQUEST							=	-39,
	STERR_FAILED_TO_DISMOUNT_THE_DRIVE							=	-40,
	STERR_FAILED_TO_LOAD_ICON									=	-41,
	STERR_MISSING_CMDLINE_PARAMETER_FILENAME					=	-42,
	STERR_MEDIA_STATE_UNINITIALIZED								=	-43,
	STERR_FAILED_TO_LOAD_SETUPAPI_LIB							=	-44,
	STERR_MISSING_API_IN_SETUPAPI_LIB							=	-45,
	STERR_FAILED_TO_LOAD_CFGMGR32_LIB							=	-46,
	STERR_MISSING_API_IN_CFGMGR32_LIB							=	-47,
	STERR_FAILED_TO_GET_DEVICE_INFO_SET							=	-48,
	STERR_FAILED_GET_DEVICE_REGISTRY_PROPERTY					=	-49,
	STERR_ERROR_IN_CFGMGR32_API									=	-50,
	STERR_FAILED_TO_CREATE_MUTEX_OBJECT							=	-51,
	STERR_FAILED_TO_BRING_THE_RUNNING_APPLICATION_TO_FOREGROUND	=	-52,
	STERR_FAILED_TO_LOCATE_SCSI_DEVICE_ON_START					=	-53,
	STERR_FAILED_TO_LOCATE_SCSI_DEVICE_ON_SHOW_VERSIONS			=	-54,
	STERR_FAILED_TO_GET_DRIVE_MAP								=	-55,
	STERR_INVALID_DISK_INFO										=	-56,
	STERR_FAILED_READ_BACK_VERIFY_TEST							=	-57,
	STERR_NO_ADMINISTRATOR										=	-58,
	STERR_FAILED_TO_DELETE_SETTINGS_DOT_DAT_FILE				=	-59,
	STERR_MANUAL_RECOVERY_REQUIRED								=	-60,

// add new ones here and update STERR_END and STERR_ERROR_OUT_OF_RANGE and below ones,
// also update CStResource::map_err_rid
	STERR_END													=	-61,
	STERR_ERROR_OUT_OF_RANGE									=	-62,
// add those errors that do not have a message to display
	STERR_FAILED_TO_LOAD_RESOURCE_DLL							=   -63,
	STERR_ANOTHER_INSTANCE_RUNNING								=	-64,
	STERR_UNKNOWN_VENDOR_SPECIFIC_SENSE_CODE					=	-65,
	STERR_ERROR_IN_SETUPAPI_API									=	-66,
    STERR_ALREADY_IN_RECOVERY_MODE                              =   -67,
    STERR_LIMITED_VENDOR_SUPPORT                                =   -68,
	STERR_NO_FILE_SPECIFIED										=   -69,
	STERR_FAILED_TO_FIND_HID_DEVICE								=   -70,
	STERR_FAILED_TO_LOAD_BMP									=	-71

} ST_ERROR;

class CStBase;

class CStError  
{
public:
	CStError();
	virtual ~CStError();

    void ClearStatus(void);
	void SaveStatus(CStBase* _p_base);
	void SaveStatus(CStBase* _p_base, wstring _more_information);
	void SaveStatus(CStBase* _p_base, UCHAR _drive_index);
	void SaveStatus(CStBase* _p_base, UCHAR _drive_index, wstring _more_information);
	void SaveStatus(ST_ERROR _last_error, long _system_last_error);
	ST_ERROR GetLastError() {return m_last_error;}
	long GetSystemLastError() {return m_system_last_error;}
	string GetObjName() {return m_err_in_obj_name;}
	UCHAR GetDriveIndex() {return m_drive_index;}
	wstring GetMoreErrorInformation(){ return m_more_error_information; }

private:

	string		m_err_in_obj_name;
	ST_ERROR	m_last_error;
	long		m_system_last_error;
	UCHAR		m_drive_index;
	wstring		m_more_error_information;
};

#endif // !defined(AFX_STERROR_H__7D233368_1994_46FE_929E_DABDE043B00F__INCLUDED_)
