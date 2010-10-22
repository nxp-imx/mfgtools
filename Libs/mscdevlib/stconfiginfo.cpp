/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StConfigInfo.cpp: implementation of the CStConfigInfo class.
//
//////////////////////////////////////////////////////////////////////


#include "stheader.h"
#include "ddildl_defs.h"

#define INCLUDE_DRIVE_ARRAY
//#include "product.h"
#include "..\..\Customization/default_cfg.h"
#include "..\..\Apps\stupdater.exe\resource.h"
#include "..\..\common\updater_res.h"
#include "..\..\common\updater_restypes.h"

#include "StConfigInfo.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CStConfigInfo::CStConfigInfo(string _name):CStBase(_name)
{
	m_DriveDesc = NULL;
	m_p_resource = NULL;
    m_recover2DD = FALSE;
    m_recover2DDImage = FALSE;
	m_LocalResourceUsage = TRUE;// This is always TRUE unless reset by resource binder
								// to lock out loading of local resources (firmware).

	m_ForceRecvMode = TRUE;
	m_AllowAutoRecovery = TRUE;
	m_FirmwareHeaderFlags = 0;
	m_FormatDataArea = FALSE;
	m_EraseMedia = FALSE;

	// 0 = default FAT per calculation
	// 1 = try to force FAT16
	// 2 = try to force FAT32
	m_preferred_fat = 1;
	m_IsAboutBMP = FALSE;
}

CStConfigInfo::~CStConfigInfo()
{

}

ST_ERROR CStConfigInfo::GetClassVersion(double& _ver)
{
	_ver = 1.0;
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetVendorName(wstring& _wstr, LANGID _lang_id)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_COMPANYNAME, (COMPANYNAME_MAX+1)*sizeof(wchar_t));
	if (pPtr)
		_wstr = (LPCTSTR)pPtr;

	return STERR_NONE;
	UNREFERENCED_PARAMETER( _lang_id );
}

ST_ERROR CStConfigInfo::GetCopyrightString(wstring& _wstr, LANGID _lang_id)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_COPYRIGHT, (COPYRIGHT_MAX+1)*sizeof(wchar_t));
	if (pPtr)
		_wstr = (LPCTSTR)pPtr;

	return STERR_NONE;
	UNREFERENCED_PARAMETER( _lang_id );
}

ST_ERROR CStConfigInfo::ApplicationName(wstring& _wstr, LANGID _lang_id)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_APPTITLE, (APPTITLE_MAX+1)*sizeof(wchar_t));
	if (pPtr)
		_wstr = (LPCTSTR)pPtr;

	return STERR_NONE;
	UNREFERENCED_PARAMETER( _lang_id );
}

ST_ERROR CStConfigInfo::ApplicationDescription(wstring& _wstr, LANGID _lang_id)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_PRODUCTDESC, (PRODDESC_MAX+1)*sizeof(wchar_t));
	if (pPtr)
		_wstr = (LPCTSTR)pPtr;

	return STERR_NONE;
	UNREFERENCED_PARAMETER( _lang_id );
}

ST_ERROR CStConfigInfo::ExecutableName(wstring& _wstr)
{
	_wstr = L"StUpdaterApp.exe";
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetSCSIMfgString(wstring& _wstr)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_SCSIMFG, (SCSIMFG_MAX+1)*sizeof(wchar_t));
	if (pPtr)
		_wstr = (LPCTSTR)pPtr;

	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetSCSIProductString(wstring& _wstr)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_SCSIPROD, (SCSIPROD_MAX+1)*sizeof(wchar_t));
	if (pPtr)
		_wstr = (LPCTSTR)pPtr;

	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetMtpMfgString(wchar_t * _wstr, rsize_t _bufsize)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_MTPMFG, (DWORD) _bufsize*sizeof(wchar_t));
	if (pPtr)
		wcscpy_s(_wstr, _bufsize, (LPCTSTR)pPtr);

	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetMtpModelString(wchar_t * _wstr, rsize_t _bufsize)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_MTPPROD, (DWORD)_bufsize*sizeof(wchar_t));
	if (pPtr)
		wcscpy_s(_wstr, _bufsize, (LPCTSTR)pPtr);

	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetUSBProductId(USHORT& _ID)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_USBPROD, sizeof(USHORT));
	if (pPtr)
		_ID = *((USHORT *)pPtr);

	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetSecondaryUSBProductId(USHORT& _ID)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_USBPROD_SECONDARY, sizeof(USHORT));
	if (pPtr)
		_ID = *((USHORT *)pPtr);

	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetUSBVendorId(USHORT& _ID)
{
	if( !m_p_resource )
		return STERR_FAILED_TO_LOAD_STRING;

	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_USBVENDOR, sizeof(USHORT));
	if (pPtr)
		_ID = *((USHORT *)pPtr);

	return STERR_NONE;
}

void CStConfigInfo::SetPreferredFAT(UCHAR _fattype)
{
	// 0 = default FAT per calculation
	// 1 = try to force FAT16
	// 2 = try to force FAT32
	m_preferred_fat = _fattype;
}

UCHAR CStConfigInfo::GetPreferredFAT()
{
	// 0 = default FAT per calculation
	// 1 = try to force FAT16
	// 2 = try to force FAT32
	return m_preferred_fat;
}


BOOL CStConfigInfo::GetDefaultAutoStartOption()
{
	LPVOID pPtr = NULL;
	BOOL ret = FALSE;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_AUTOSTART, sizeof(BOOL));
	if (pPtr)
		ret = *((BOOL *)pPtr);

	return ret;
}

BOOL CStConfigInfo::GetDefaultAutoCloseOption()
{
	LPVOID pPtr = NULL;
	BOOL ret = FALSE;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_AUTOCLOSE, sizeof(BOOL));
	if (pPtr)
		ret = *((BOOL *)pPtr);

	return ret;
}

void CStConfigInfo::SetRecover2DD(BOOL _recover2DD)
{
    m_recover2DD = _recover2DD;
}

void CStConfigInfo::SetRecover2DDImage(BOOL _recover2DDImage)
{
    m_recover2DDImage = _recover2DDImage;
}

void CStConfigInfo::SetRecover2DDImageFileName(wstring& _fName)
{
    m_recover2dd_image_filename = _fName;
}

void CStConfigInfo::GetRecover2DDImageFileName(wstring& _fName)
{
    _fName = m_recover2dd_image_filename;
}

ST_ERROR CStConfigInfo::ContentFolder(wstring& _wstr)
{
	_wstr = L"recover"; //CFG_CONTENT_FOLDER_L;
	return STERR_NONE;
}

BOOL CStConfigInfo::UseScsiProductSubstringQualifier()
{
	return FALSE; //CFG_USE_SUBSTRING_QUAL;
}


ST_ERROR CStConfigInfo::GetNumDrives(UCHAR& _num_drives)
{
    _num_drives = (UCHAR) m_DriveDescNumDrives; //CFG_NUM_DRIVES;
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetNumSystemDrives(UCHAR& _num_drives)
{
	_num_drives = 0;
	for( USHORT index=0; index< m_DriveDescNumDrives /*CFG_NUM_DRIVES*/; index++ )
	{
		if( ( m_DriveDesc[index].uchType == DriveTypeSystem ) && 
            m_DriveDesc[index].uchTag != DRIVE_TAG_UPDATER_S )
		{
			_num_drives ++;
		}
	}
	
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetNumDataDrives(UCHAR& _num_drives)
{
	_num_drives = 0;
	for( USHORT index=0; index< m_DriveDescNumDrives /*CFG_NUM_DRIVES*/; index++ )
	{
		if( m_DriveDesc[index].uchType == DriveTypeData )
		{
			_num_drives ++;
		}
	}
	
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetNumHiddenDataDrives(UCHAR& _num_drives)
{
	_num_drives = 0;
	for( USHORT index=0; index < m_DriveDescNumDrives /*CFG_NUM_DRIVES*/; index++ )
	{
		if( m_DriveDesc[index].uchType == DriveTypeHiddenData ) 
		{
			_num_drives ++;
		}
	}
	
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetDriveType(UCHAR _drive_index, LOGICAL_DRIVE_TYPE& _typeParm)
{
	if( (USHORT)_drive_index >= ( m_DriveDescNumDrives+1 ) /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}

	_typeParm = (LOGICAL_DRIVE_TYPE)m_DriveDesc[_drive_index].uchType;
	return STERR_NONE;		
}

ST_ERROR CStConfigInfo::GetDriveTag(UCHAR _drive_index, UCHAR& _tag)
{
	if( (USHORT) _drive_index >= ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}

	_tag = m_DriveDesc[_drive_index].uchTag;
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetSystemDriveName(UCHAR _system_drive_index, string& _str)
{
USES_CONVERSION;
	if( _system_drive_index >= ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}

	if( m_DriveDesc[_system_drive_index].uchType != DriveTypeSystem &&
        m_DriveDesc[_system_drive_index].uchType != DriveTypeHiddenData )
	{
		return STERR_INVALID_DRIVE_TYPE;
	}

	_str = W2A(m_DriveDesc[_system_drive_index].wchName);
	return STERR_NONE;
}
/*
ST_ERROR CStConfigInfo::IsSystemDriveEncrypted(UCHAR _system_drive_index, BOOL& _encrypted)
{
	if( _system_drive_index >= m_DriveDescNumDrives  )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}

	if( m_DriveDesc[_system_drive_index].uchType != DriveTypeSystem &&
        m_DriveDesc[_system_drive_index].uchType != DriveTypeHiddenData )
	{
		return STERR_INVALID_DRIVE_TYPE;
	}
	_encrypted = m_DriveDesc[_system_drive_index].bEncrypted;
	return STERR_NONE;
}
*/
ST_ERROR CStConfigInfo::GetSystemDriveDescription(UCHAR _system_drive_index, wstring& _wstr)
{
	if( _system_drive_index >= ( m_DriveDescNumDrives + 1 )  /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}

	if( m_DriveDesc[_system_drive_index].uchType != DriveTypeSystem &&
        m_DriveDesc[_system_drive_index].uchType != DriveTypeHiddenData )
	{
		return STERR_INVALID_DRIVE_TYPE;
	}
	
	_wstr = m_DriveDesc[_system_drive_index].wchDescription;
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetUpdaterDriveIndex(CHAR& _system_drive_index)
{
	ST_ERROR err = STERR_NONE;
	
	for( CHAR index=0; index < ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/; ++index )
	{
		// UPDATER.SB is either written to the NAND (DRIVE_TAG_UPDATER_NAND_S) or
		// not (DRIVE_TAG_UPDATER_S). Both tags can not be present in a given drive array.
		if( m_DriveDesc[index].uchTag == DRIVE_TAG_UPDATER_S ||
			m_DriveDesc[index].uchTag == DRIVE_TAG_UPDATER_NAND_S )
		{
			_system_drive_index = index;
			err = STERR_NONE;
			break;
		}
	}
	return err;
}



ST_ERROR CStConfigInfo::GetPlayerDriveIndex(UCHAR& _system_drive_index)
{
	ST_ERROR err = STERR_INVALID_DRIVE_TYPE;
	
	for( UCHAR index=0; index < ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/; ++index )
	{
		if( m_DriveDesc[index].uchTag == DRIVE_TAG_STMPSYS_S )
		{
			_system_drive_index = index;
			err = STERR_NONE;
			break;
		}
	}
	return err;
}

ST_ERROR CStConfigInfo::GetBootyDriveIndex(UCHAR& _drive_index)
{
	ST_ERROR err = STERR_INVALID_DRIVE_TYPE;
	
	for( UCHAR index=0; index < ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/; ++index )
	{
		if( m_DriveDesc[index].uchTag == DRIVE_TAG_BOOTMANAGER_S )
		{
			_drive_index = index;
			err = STERR_NONE;
			break;
		}
	}
	return err;
}

CHAR CStConfigInfo::GetDriveIndex(int _tag)
{
	CHAR drv_idx = -1;
	
	for( UCHAR index=0; index < ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/; ++index )
	{
		if( m_DriveDesc[index].uchTag == _tag )
		{
			drv_idx = index;
			break;
		}
	}
	return drv_idx;
}

ST_ERROR CStConfigInfo::GetNumNonVolatileSystemDrives(UCHAR& _num_drives)
{
	_num_drives = 0;
	for( int index=0; index < ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/; index++ )
	{
		if( m_DriveDesc[index].uchType == DriveTypeNonVolatile )
		{
			_num_drives ++;
		}
	}

	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetNonVolatileSystemDriveName(UCHAR _non_volatile_drive_index, string& _str)
{
USES_CONVERSION;
	if( _non_volatile_drive_index >= ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}
	if( m_DriveDesc[_non_volatile_drive_index].uchType != DriveTypeNonVolatile )
	{
		return STERR_INVALID_DRIVE_TYPE;
	}
	_str = W2A(m_DriveDesc[_non_volatile_drive_index].wchName);
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetNonVolatileSystemDriveDescription(UCHAR _non_volatile_drive_index, wstring& _wstr)
{
	if( _non_volatile_drive_index >= ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}
	if( m_DriveDesc[_non_volatile_drive_index].uchType != DriveTypeNonVolatile )
	{
		return STERR_INVALID_DRIVE_TYPE;
	}
	_wstr = m_DriveDesc[_non_volatile_drive_index].wchDescription;
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetNonVolatileSystemDriveType(UCHAR _non_volatile_drive_index, UCHAR& _drive_type)
{
	if( _non_volatile_drive_index >= ( m_DriveDescNumDrives  + 1 ) /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}
	if( m_DriveDesc[_non_volatile_drive_index].uchType != DriveTypeNonVolatile )
	{
		return STERR_INVALID_DRIVE_TYPE;
	}
	_drive_type = m_DriveDesc[_non_volatile_drive_index].uchType;
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetNonVolatileSystemDriveTag(UCHAR _non_volatile_drive_index, UCHAR& _drive_tag)
{
	if( _non_volatile_drive_index >= ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}
	if( m_DriveDesc[_non_volatile_drive_index].uchType != DriveTypeNonVolatile )
	{
		return STERR_INVALID_DRIVE_TYPE;
	}
	_drive_tag = m_DriveDesc[_non_volatile_drive_index].uchTag;
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetRequestedDriveSize(UCHAR _drive_index, ULONG& _bytes)
{
	if( _drive_index >= ( m_DriveDescNumDrives + 1 ) /*CFG_NUM_DRIVES*/ )
	{
		return STERR_INVALID_DRIVE_INFO_REQUEST;
	}

	_bytes = m_DriveDesc[_drive_index].ulRequestedKB * 1024;
	
	return STERR_NONE;
}

ST_ERROR CStConfigInfo::GetDefaultVolumeLabel(wstring& _vol_label)
{
	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_DEFAULTLABEL, DRIVELABEL_MAX);
	if (pPtr)
		_vol_label = (LPCTSTR)pPtr;

	return STERR_NONE;
}


ST_ERROR CStConfigInfo::GetAboutDlgTitle(wstring& _about_title, LANGID _lang_id)
{
	LPVOID pPtr = NULL;
	wstring w_format;
	wchar_t wstr[120];

	m_p_resource->GetResourceString(IDS_ABOUTBOX, w_format);
	_about_title = L"About ";
	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_APPTITLE, APPTITLE_MAX);
	if (pPtr)
	{
		swprintf(wstr, 120, w_format.c_str(), (LPCTSTR)pPtr);
		_about_title = wstr;
	}
		//_about_title = (LPCTSTR)pPtr;

	return STERR_NONE;
	UNREFERENCED_PARAMETER( _lang_id );
}



void CStConfigInfo::SetResource( CStBaseToResource* _p_resource )
{
	LPVOID pPtr = NULL;

	m_p_resource = _p_resource;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_AUTORECOVERY, sizeof(BOOL));
	if (pPtr)
		m_AllowAutoRecovery = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_FORCERECOVERY, sizeof(BOOL));
	if (pPtr)
		m_ForceRecvMode = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_REBOOTMSG, sizeof(BOOL));
	if (pPtr)
		m_ShowBootToPlayerMsg = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_USELOCALRES, sizeof(BOOL));
	if (pPtr)
		m_LocalResourceUsage = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_WINCE, sizeof(BOOL));
	if (pPtr)
		m_WinCE = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_LOW_NAND_SOLUTION, sizeof(BOOL));
	if (pPtr)
		m_IsLowNand = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_ABOUT_BMP, sizeof(BOOL));
	if (pPtr)
		m_IsAboutBMP = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_BITMAP_ID, sizeof(USHORT));
	if (pPtr)
		m_BitmapId = *((USHORT *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_DLGTYPE, sizeof(USER_DLG_TYPE));
	if (pPtr)
		m_DefaultDialog = *((USER_DLG_TYPE *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_FORMATDATA, sizeof(BOOL));
	if (pPtr)
		m_FormatDataArea = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_ERASEMEDIA, sizeof(BOOL));
	if (pPtr)
		m_EraseMedia = *((BOOL *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_DEFAULTFAT, sizeof(USHORT));
	if (pPtr)
		m_preferred_fat = *((UCHAR *) pPtr);

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_DRVARRAY_NUM_DRIVES, sizeof(USHORT));
	if (pPtr)
		m_DriveDescNumDrives = *((USHORT *) pPtr);

	if (m_DriveDesc)
		VirtualFree(m_DriveDesc, 0, MEM_RELEASE);

	// Add one more for updater
	m_DriveDesc = (PDRIVE_DESC) VirtualAlloc(NULL, (m_DriveDescNumDrives+1)*sizeof(DRIVE_DESC), MEM_COMMIT, PAGE_READWRITE);
	if (m_DriveDesc)
	{
		int iDataDriveCount = 0;
		for (USHORT i = 0; i < m_DriveDescNumDrives; ++i)
		{
			wchar_t *resStr = (wchar_t *) m_p_resource->GetDefaultCfgResource(IDR_CFG_DRVARRAY_ONE+i, sizeof(DRIVE_DESC));
			ConvertStringToDrive(resStr, i);
			if (m_DriveDesc[i].uchType == DriveTypeData)
				++iDataDriveCount;
		}
		// Add updater
		if (iDataDriveCount <= 1)
			ConvertStringToDrive(L"updater.sb,Updater,1,0xFF,0", m_DriveDescNumDrives);
		else
		{
			m_recover2DD = TRUE;
			ConvertStringToDrive(L"recover2DD.sb,Updater,1,0xFF,0", m_DriveDescNumDrives);
		}
			
	}

	m_FirmwareHeaderFlags = 0;
}

//-------------------------------------------------------------------
// ConvertStringToDrive()
//
// Convert a string to a DRIVE_DESC data structure.
//-------------------------------------------------------------------

void CStConfigInfo::ConvertStringToDrive(wchar_t *str, int _index)
{
    CString     tmpStr, strDriveDesc;
    int         curPos = 0;
    _TCHAR*     stopStr = _T(",");

	strDriveDesc = str;
    if (strDriveDesc.GetAt(0) != _T(','))
    {
        wcscpy_s(m_DriveDesc[_index].wchName, MAX_PATH, strDriveDesc.Tokenize(_T(","), curPos));
    }
    else
    {
        m_DriveDesc[_index].wchName[0] = '\0';
        curPos++;
    }
    if (strDriveDesc.GetAt(curPos) != _T(','))
    {
        wcscpy_s(m_DriveDesc[_index].wchDescription, MAX_PATH, strDriveDesc.Tokenize(_T(","), curPos));
    }
    else
    {
        m_DriveDesc[_index].wchDescription[0] = '\0';
        curPos++;
    }
    if (strDriveDesc.GetAt(curPos) != _T(','))
		m_DriveDesc[_index].uchType = (UCHAR) _tcstoul(strDriveDesc.Tokenize(_T(","), curPos), &stopStr, 10);
    else
        curPos++;

    if (strDriveDesc.GetAt(curPos) != _T(','))
		m_DriveDesc[_index].uchTag = (UCHAR) _tcstoul(strDriveDesc.Tokenize(_T(","), curPos), &stopStr, 16);
    else
        curPos++;

//    if (strDriveDesc.GetAt(curPos) != _T(','))
//		m_DriveDesc[_index].bEncrypted = (BOOLEAN) _tcstoul(strDriveDesc.Tokenize(_T(","), curPos), &stopStr, 10);
//    else
//        curPos++;

    if (strDriveDesc.GetAt(curPos) != _T(','))
        m_DriveDesc[_index].ulRequestedKB = _tcstoul(strDriveDesc.Tokenize(_T(" "), curPos), &stopStr, 10);

	// check for 2nd data drive and image file name
	if ( m_DriveDesc[_index].uchTag == (DRIVE_TAG_DATA + 0x10) && _tcslen(m_DriveDesc[_index].wchName) )
	{
		m_recover2dd_image_filename = m_DriveDesc[_index].wchName;
		m_recover2DDImage = TRUE;
	}

}

//BOOL CStConfigInfo::HideChipInfo()
//{
//	return CFG_HIDE_CHIP_VERSION;
//}

BOOL CStConfigInfo::AllowAutoRecovery()
{
	return m_AllowAutoRecovery;
}


//BOOL CStConfigInfo::HasCustomSupport()
//{
//	return CFG_HAS_CUSTOM_SUPPORT;
//}



BOOL CStConfigInfo::BuiltFor35xx()
{
    BOOL ret = FALSE;
	CString sdk;
	GetBaseSDKString(sdk);
	if ( ( sdk.Find(L"SDK3.")  != -1 ) )
	{	
		 ret = TRUE;
	}
	return ret;
}

BOOL CStConfigInfo::BuiltFor36xx()
{
    BOOL ret = FALSE;
	CString sdk;
	GetBaseSDKString(sdk);
	if ( ( sdk.Find(L"SDK4.")  != -1 ) )
	{	
		 ret = TRUE;
	}
	return ret;
}

BOOL CStConfigInfo::BuiltFor37xx()
{
    BOOL ret = FALSE;
	CString sdk;
	GetBaseSDKString(sdk);
	if ( ( sdk.Find(L"SDK5.")  != -1 ) )
	{	
		 ret = TRUE;
	}
	return ret;
}
BOOL CStConfigInfo::IsLowNandSolution()
{
	return m_IsLowNand;
}

BOOL CStConfigInfo::IsWinCESolution()
{
	return m_WinCE;
}


//BOOL CStConfigInfo::ForceGetCurrentVersions()
//{
//	return CFG_FORCE_GET_CURR_VERSIONS;
//}

BOOL CStConfigInfo::GetBaseSDKString(CString& _sdkstring)
{
	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_BASE_SDK, BASESDK_MAX);
	if (pPtr)
		_sdkstring = (LPCTSTR)pPtr;

	return STERR_NONE;
/*
	CString resStr;
	resStr.LoadStringW(IDS_BASE_SDK);
	_sdkstring = resStr;
	return TRUE;
*/
}

BOOL CStConfigInfo::GetBaseSDKString(wstring& _sdkstring)
{
	LPVOID pPtr = NULL;

	pPtr = m_p_resource->GetDefaultCfgResource(IDR_CFG_BASE_SDK, BASESDK_MAX);
	if (pPtr)
		_sdkstring = (LPCTSTR)pPtr;

	return STERR_NONE;
/*
	CString resStr;
	resStr.LoadStringW(IDS_BASE_SDK);
	_sdkstring = resStr;
	return TRUE;
*/
}

/*
void CStConfigInfo::SetDefaultLocalResourceUsage(BOOL _localResourceUsage)
{
	m_LocalResourceUsage = _localResourceUsage;
}
*/

double CStConfigInfo::GetSystemDrivePaddingFactor()
{
	return  1.0 + ((double)CFG_SYSTEM_DRIVE_PADDING_PERCENTAGE/100.0);
}

BOOL CStConfigInfo::GetForceRecvMode()
{
	return m_ForceRecvMode;
}

/*
void CStConfigInfo::SetDefaultForceRecvMode(BOOL _ForceRecvMode)
{
	m_ForceRecvMode = _ForceRecvMode;

	if( m_ForceRecvMode )
		m_AllowAutoRecovery = TRUE;
	else
		m_AllowAutoRecovery = CFG_AUTO_RECOVERY;
}


void CStConfigInfo::SetDefaultEraseMedia(BOOL _EraseMedia)
{
	m_EraseMedia = _EraseMedia;
}

void CStConfigInfo::SetDefaultFormatDataArea(BOOL _Format)
{
	m_FormatDataArea = _Format;
}
*/
CString CStConfigInfo::DriveArrayToString()
{
	USES_CONVERSION;

	CString returnTxt, descTxt, lineTxt, sizeTxt;

	lineTxt.Format(_T("gDriveArray[%d]\n\n"), m_DriveDescNumDrives /*CFG_NUM_DRIVES*/);
	returnTxt = lineTxt;

	for ( int i=0; i<m_DriveDescNumDrives /*CFG_NUM_DRIVES*/; ++i )
	{
		DRIVE_DESC drive = m_DriveDesc[i];
		
		if ( drive.ulRequestedKB )
			sizeTxt.Format(_T("0x%X KB"), drive.ulRequestedKB);
		else
			if ( drive.uchType == DriveTypeData )
				sizeTxt = _T("0");
			else if ( drive.uchType == DriveTypeHiddenData )
				if ( drive.wchName[0] == '\0' )
					sizeTxt = _T("0");
				else
					sizeTxt = _T("file_size");
			else
				sizeTxt = _T("file_size");

		
		lineTxt.Format(_T("[0x%02X] %s \"%s\", <%s>, <%s>\n"), 
			drive.uchTag,
			DriveTypeToString((LOGICAL_DRIVE_TYPE)drive.uchType),
			W2T(drive.wchDescription),
			W2T(drive.wchName),
			sizeTxt);
		
		returnTxt.Append(lineTxt);
	}

	return returnTxt;
}

CString CStConfigInfo::DriveTypeToString(LOGICAL_DRIVE_TYPE type)
{
	CString retStr;

	switch ( type )
	{
		case DriveTypeInvalid:
			retStr = _T("Invalid_t,    ");
			break;
		case DriveTypeData:
			retStr = _T("Data_t,       ");
			break;
		case DriveTypeSystem:
			retStr = _T("System_t,     ");
			break;
		case DriveTypeHiddenData:
			retStr = _T("HiddenData_t, ");
			break;
		case DriveTypeNonVolatile:
			retStr = _T("NonVolatile_t,");
			break;
		case DriveTypeUnknown:
		default:
			retStr = _T("Unknown_t,    ");
	}

	return retStr;
}
