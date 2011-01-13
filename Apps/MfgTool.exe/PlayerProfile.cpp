/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
//
// PlayerProfile.cpp : implementation file
//
#include "stdafx.h"
#include "PlayerProfile.h"
#include "StMfgTool.h"

// CPlayerProfile
CPlayerProfile::CPlayerProfile()
: m_cs_name(_T(""))
, m_cs_usb_vid(_T(""))
, m_cs_usb_pid(_T(""))
, m_cs_scsi_mfg(_T(""))
, m_cs_scsi_product(_T(""))
, m_status(IDS_PROFILE_ERR_NOT_INITIALIZED)
, m_error_msg(_T(""))
, m_cs_profile_root(_T(""))
, m_cs_profile_path(_T(""))
, m_cs_ini_file(_T(""))
, m_cs_volume_label(_T(""))
, m_b_use_volume_label(FALSE)
, m_edit_mode(FALSE)
, m_bNew(TRUE)
, m_bLockedProfile(FALSE)
{
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	CString csTemp;
	GetModuleFileName(NULL, csTemp.GetBuffer(_MAX_PATH), _MAX_PATH);
	_tsplitpath_s( csTemp, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext,_MAX_EXT );
	csTemp.ReleaseBuffer();
	csTemp = _T("Profiles\\");
	m_cs_profile_root.Format(_T("%s%s%s"), drive, dir, csTemp);
    if ( _taccess(m_cs_profile_root, 0) == -1) {
        if ( !CreateDirectory(m_cs_profile_root, NULL) ) {
			    ATLTRACE(_T("ERROR: DirectoryOp():CREATE_NEW_DIR - Could not make %s directory. (%d)\n"), m_cs_profile_root, GetLastError());
		    }
    }
}

CPlayerProfile::~CPlayerProfile()
{
	while (!m_p_op_info_list.IsEmpty()) 
		delete m_p_op_info_list.RemoveTail();

	while (!m_p_op_delete_list.IsEmpty()) 
		delete m_p_op_delete_list.RemoveTail();
}
/*
// Define assignment operator.
CPlayerProfile &CPlayerProfile::operator=( CPlayerProfile &_pp )
{
	m_cs_name = _pp.m_cs_name;
	m_cs_usb_vid = _pp.m_cs_usb_vid;
	m_cs_usb_pid = _pp.m_cs_usb_pid;
	m_cs_scsi_mfg = _pp.m_cs_scsi_mfg;
	m_cs_scsi_product = _pp.m_cs_scsi_product;
	m_status = _pp.m_status;
	m_error_msg = _pp.m_error_msg;
	m_cs_profile_root = _pp.m_cs_profile_root;
	m_cs_profile_path = _pp.m_cs_profile_path;
	m_cs_ini_file = _pp.m_cs_ini_file;
	m_cs_volume_label = _pp.m_cs_volume_label;
	m_b_use_volume_label = _pp.m_b_use_volume_label;
	m_backup_present = _pp.m_backup_present;
	m_cs_backup_path = _pp.m_cs_backup_path;
	m_edit_mode = _pp.m_edit_mode;

	// remove any operations if there are any
	while (!m_p_op_info_list.IsEmpty()) {
		delete m_p_op_info_list.RemoveTail();
	}
	// now copy the operations
	POSITION pos = _pp.m_p_op_info_list.GetHeadPosition();
	while (pos) {
		COpInfo* pOpInfo = new COpInfo(this);
		*pOpInfo = *(_pp.m_p_op_info_list.GetNext(pos));
		m_p_op_info_list.AddTail( pOpInfo );
	}
   return *this;  // Assignment operator returns left side.
}
*/
#ifdef _DEBUG
void CPlayerProfile::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);
	dc << "CPlayerProfile = " << m_cs_name;
}
#endif


// CPlayerProfile member functions
DWORD CPlayerProfile::Init( LPCTSTR _name /* = NULL */)
{
	CString csTemp;
	DWORD ret;
	int pos = 0;
	int iProfileVer;

	m_iSelectedUpdate = -1;

	// init return values
	m_status = PROFILE_OK;
	m_error_msg.LoadString(IDS_PROFILE_OK);

	// get the last player profile used from the registry
	if (_name) {
		m_cs_name = _name;
		if ( m_cs_name.IsEmpty() ) {
			m_cs_name = AfxGetApp()->GetProfileString(_T("Player Profile"), _T("Player Description"));
		}
	}
//	else { 
//		m_cs_name = AfxGetApp()->GetProfileString(_T("Player Profile"), _T("Player Description"));
//	}

	if ( m_cs_name.IsEmpty() ) {
		// if there wasn't a saved player profile, open profile editor
		m_status = PROFILE_ERROR;
		m_error_msg.LoadString(IDS_PROFILE_ERR_NO_SELECTION);
		return ( m_status );
	}
	// if there was a saved player profile, see if the directory
	// <AppDir>\Profiles\<Player Description> exists
	m_cs_profile_path = m_cs_profile_root + m_cs_name;
	if ( _taccess(m_cs_profile_path, 0) == -1 ) {
		// if the player profile directory doesn't exist, open profile editor
		m_status = PROFILE_ERROR;
		csTemp.LoadString(IDS_PROFILE_ERR_MISSING_DIRECTORY);
		m_error_msg.Format(csTemp, m_cs_profile_path);
		return ( m_status );
	}
	else
		m_bNew = FALSE;  // an existing profile, at least a folder

	// if the <Player Description> dir exists, see if the .ini file exists
	m_cs_ini_file = m_cs_profile_path + _T("\\player.ini");
	if ( _taccess(m_cs_ini_file, 0) == -1 ) {
		// if the .ini file doesn't exist, open profile editor
		m_status = PROFILE_ERROR;
		csTemp.LoadString(IDS_PROFILE_ERR_MISSING_INI_FILE);
		m_error_msg.Format(csTemp, m_cs_ini_file);
		return ( m_status );
	}

	// parse the .ini file for config information
	ret = GetPrivateProfileString(_T("PROFILE"), _T("VERSION"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_cs_ini_file);
    csTemp.ReleaseBuffer();
	if ( ret != 0 )
		iProfileVer = _wtoi(csTemp.GetBuffer());
	if ( ret == 0 || iProfileVer < MIN_PROFILE_VERSION)
	{
		m_status = PROFILE_ERROR;
		m_error_msg.LoadStringW(IDS_INVALID_PLAYER_PROFILE);
		return (m_status);
	}
/*
	ret = GetPrivateProfileString(_T("PROFILE"), _T("USB_VID"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_cs_ini_file);
    csTemp.ReleaseBuffer();
	if ( ret != 6 ) {
		if ( ret > 2 && ret < 6 )
            m_cs_usb_vid = csTemp.Mid(2).MakeUpper();
        else
            m_cs_usb_vid.Empty();
        if ( m_status < PROFILE_ERROR ) {
            m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_USB_VENDOR_ID);
        }
	} else {
		m_cs_usb_vid = csTemp.Mid(2).MakeUpper();
    }
	ret = GetPrivateProfileString(_T("PROFILE"), _T("USB_PID"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_cs_ini_file);
    csTemp.ReleaseBuffer();
	if ( ret != 6 ) {
		if ( ret > 2 && ret < 6 )
            m_cs_usb_pid = csTemp.Mid(2).MakeUpper();
        else
            m_cs_usb_pid.Empty();
        if ( m_status < PROFILE_ERROR ) {
            m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_USB_PROD_ID);
        }
	} else {
		m_cs_usb_pid = csTemp.Mid(2).MakeUpper();
    }
	ret = GetPrivateProfileString(_T("PROFILE"), _T("SCSI_MFG"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_cs_ini_file);
    csTemp.ReleaseBuffer();
	m_cs_scsi_mfg = csTemp;
	if ( ret == 0 || ret > 8 ) {
        if ( m_status < PROFILE_ERROR ) {
    		m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_SCSI_VENDOR_ID);
        }
	}
	ret = GetPrivateProfileString(_T("PROFILE"), _T("SCSI_PRODUCT"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_cs_ini_file);
    csTemp.ReleaseBuffer();
	m_cs_scsi_product = csTemp;
	if ( ret == 0 || ret > 16 ) {
        if ( m_status < PROFILE_ERROR ) {
    		m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_SCSI_PROD_ID);
        }
	}
	ret = GetPrivateProfileString(_T("PROFILE"), _T("VOLUME_LABEL"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_cs_ini_file);
    csTemp.ReleaseBuffer();
    pos = 0;
    m_cs_volume_label = csTemp.Tokenize(_T(","), pos);
    if ( pos != -1 && csTemp.GetAt(pos-1) == _T(',') ) {
		m_b_use_volume_label = (csTemp.Tokenize(_T(", \t\0"), pos)) == _T("1") ? TRUE : FALSE;
    }
    if ( m_cs_volume_label.GetLength() > 11 && m_status < PROFILE_WARNING ) {
    		m_status = PROFILE_WARNING;
			m_error_msg.LoadString(IDS_PROFILE_WRN_INVALID_VOLUME_LABLE);
    }
	m_b_use_volume_label &= ( !m_cs_volume_label.IsEmpty() && m_cs_volume_label.GetLength() <= 11 );
*/
	// remove any operations if there are any
	while (!m_p_op_info_list.IsEmpty()) {
		delete m_p_op_info_list.RemoveTail();
	}
	// now get the operations from the file
	int num_ops = 0, num_valid_ops = 0;
	CString csCmdLine;
	GetPrivateProfileSection(_T("OPERATIONS"), csTemp.GetBufferSetLength(1024), 1024, m_cs_ini_file);
	pos = 0;
    while (csTemp.GetAt(pos) != _T('\0')) {
		csCmdLine = csTemp.Tokenize(_T("\r\n"), pos);
		COpInfo* pOpInfo = new COpInfo(this, (LPCTSTR)csCmdLine, num_ops);
		m_p_op_info_list.AddTail( pOpInfo );
		if ( pOpInfo->ReadProfileOpAndValidate() == OPINFO_OK ) {
			++num_valid_ops;
		}
		else {
			pOpInfo->OnEnable(false);
		}
		++num_ops;
	}

	if ( num_valid_ops == 0 )
	{
        if ( m_status < PROFILE_ERROR )
		{
            m_status = PROFILE_ERROR;
		    m_error_msg.LoadString(IDS_PROFILE_ERR_NO_VALID_OPERATION);
        }
	}
	else if ( num_ops != num_valid_ops )
	{
        if ( m_status < PROFILE_WARNING )
		{
		    m_status = PROFILE_WARNING;
		    m_error_msg.LoadString(IDS_PROFILE_WRN_INVALID_OPERATION);
        }
	}

	return m_status;
}

INT_PTR CPlayerProfile::GetNumEnabledOps(void)
{
	INT_PTR num_enabled_ops = 0;
	POSITION pos = m_p_op_info_list.GetHeadPosition();
	while ( pos ) {
		if ( m_p_op_info_list.GetNext(pos)->IsEnabled() )
			++num_enabled_ops;
	}
	return num_enabled_ops;
}

DWORD CPlayerProfile::SetUseVolumeLabel(BOOL _use_label)
{
	// if we are in edit mode just update the class variable
	// don't try to save it to the player.ini file.
//	if ( m_edit_mode ) {
		m_b_use_volume_label = _use_label;  
		return PROFILE_OK;
//	}
/*SP	
	// check to see if the ini file is write protected
	DWORD attrib = ::GetFileAttributes(m_cs_ini_file);
	if ( attrib & FILE_ATTRIBUTE_READONLY ) {
		CString msg;
		msg.Format(IDS_PROFILE_MSG_OVERWRITE_PROMPT, m_cs_ini_file);
		if ( IDYES == AfxMessageBox(msg, MB_YESNO| MB_ICONQUESTION) ) {
			::SetFileAttributes(m_cs_ini_file, attrib & ~FILE_ATTRIBUTE_READONLY);
		}
		else {
			m_error_msg.Format(IDS_PROFILE_ERR_WRITE_FAILURE, m_cs_ini_file);
			return PROFILE_WARNING;
		}
	}
	
	CString csCmdLineNew;
	csCmdLineNew.Format(_T("%s,%d"), m_cs_volume_label, _use_label);
	if (!WritePrivateProfileString(_T("PROFILE"), _T("VOLUME_LABEL"), csCmdLineNew, m_cs_ini_file)) {
		m_error_msg.Format(IDS_PROFILE_ERR_WRITE_FAILURE, m_cs_ini_file);
		return PROFILE_WARNING;
	}
	else {
		// save the new state
		m_b_use_volume_label = _use_label;  
		return PROFILE_OK;
	}
SP*/
}

DWORD CPlayerProfile::SetIniField(DWORD _field, LPVOID _value)
{
	CString cs_temp;

    // check to see if the ini file is write protected
	DWORD attrib = ::GetFileAttributes(m_cs_ini_file);
	if ( attrib & FILE_ATTRIBUTE_READONLY ) {
		CString msg;
		msg.Format(IDS_PROFILE_MSG_OVERWRITE_PROMPT, m_cs_ini_file);
		if ( IDYES == AfxMessageBox(msg, MB_YESNO| MB_ICONQUESTION) ) {
			::SetFileAttributes(m_cs_ini_file, attrib & ~FILE_ATTRIBUTE_READONLY);
		}
		else {
			m_error_msg.Format(IDS_PROFILE_ERR_WRITE_FAILURE, m_cs_ini_file);
			return PROFILE_WARNING;
		}
	}
    else if ( attrib == INVALID_FILE_ATTRIBUTES ) {
		m_error_msg.Format(IDS_PROFILE_ERR_FILE_NO_EXIST, m_cs_ini_file);
		return PROFILE_ERROR;
    }
	
	switch ( _field )
    {
	case VERSION:
		cs_temp = (LPCSTR)_value;
		WritePrivateProfileString(_T("PROFILE"), _T("VERSION"), cs_temp, m_cs_ini_file);
		break;
    case PLAYER:
        cs_temp = (LPCTSTR)_value;
        WritePrivateProfileString(_T("PROFILE"), _T("PLAYER"), cs_temp, m_cs_ini_file);
        break;
/*
	case USB_VID:
        m_cs_usb_vid = (LPCTSTR)_value;
	    cs_temp.Format(_T("0x%s"), m_cs_usb_vid);
	    WritePrivateProfileString(_T("PROFILE"), _T("USB_VID"), cs_temp, m_cs_ini_file);
        break;
    case USB_PID:
        m_cs_usb_pid = (LPCTSTR)_value;
	    cs_temp.Format(_T("0x%s"), m_cs_usb_pid);
	    WritePrivateProfileString(_T("PROFILE"), _T("USB_PID"), cs_temp, m_cs_ini_file);
        break;
    case SCSI_MFG:
        m_cs_scsi_mfg = (LPCTSTR)_value;
	    WritePrivateProfileString(_T("PROFILE"), _T("SCSI_MFG"), m_cs_scsi_mfg, m_cs_ini_file);
        break;
    case SCSI_PRODUCT:
        m_cs_scsi_product = (LPCTSTR)_value;
	    WritePrivateProfileString(_T("PROFILE"), _T("SCSI_PRODUCT"), m_cs_scsi_product, m_cs_ini_file);
        break;
    case VOLUME_LABEL:
        m_cs_volume_label = (LPCTSTR)_value;
	    if ( m_cs_volume_label.IsEmpty() )
            cs_temp.Empty();
        else
            cs_temp.Format(_T("%s,%d"), m_cs_volume_label, m_b_use_volume_label);
	    if (!WritePrivateProfileString(_T("PROFILE"), _T("VOLUME_LABEL"), cs_temp, m_cs_ini_file)) {
		    m_error_msg.Format(IDS_PROFILE_ERR_WRITE_FAILURE, m_cs_ini_file);
	    }
        break;
    case USE_VOLUME_LABEL:
        m_b_use_volume_label = *(BOOL*)_value;
	    if ( m_cs_volume_label.IsEmpty() )
            cs_temp.Empty();
        else
            cs_temp.Format(_T("%s,%d"), m_cs_volume_label, m_b_use_volume_label);
	    if (!WritePrivateProfileString(_T("PROFILE"), _T("VOLUME_LABEL"), cs_temp, m_cs_ini_file)) {
		    m_error_msg.Format(IDS_PROFILE_ERR_WRITE_FAILURE, m_cs_ini_file);
	    }
        break;
*/
	}

	//Init(m_cs_name);
	return m_status;
}

void CPlayerProfile::RemoveDeletedOps()
{
	if (!m_p_op_delete_list.IsEmpty())
	{
	    COpInfo * pOpInfo;
	    POSITION pos = m_p_op_delete_list.GetHeadPosition();
		while (pos)
		{
			pOpInfo = m_p_op_delete_list.GetNext(pos);
		    pOpInfo->Remove();
		    delete pOpInfo;
		}
		m_p_op_delete_list.RemoveAll();
	}
}

COpInfo* CPlayerProfile::AddOperation(void)
{
	COpInfo* pOpInfo = new COpInfo(this);
	if ( pOpInfo ) {
		pOpInfo->ReadProfileOpAndValidate();
        pOpInfo->SetIndex( m_p_op_info_list.GetCount() );
		m_p_op_info_list.AddTail( pOpInfo );
	}
	return pOpInfo;
}

INT_PTR CPlayerProfile::RemoveOperation(INT_PTR _index)
{
    INT_PTR i = 0;
    COpInfo * pOpInfo;
    COpInfoList temp_list;
    POSITION pos = m_p_op_info_list.GetHeadPosition();
    while (pos) {
        pOpInfo = m_p_op_info_list.GetNext(pos);
        if ( pOpInfo->GetIndex() == _index ) {
			m_p_op_delete_list.AddTail(pOpInfo);
	//	    pOpInfo->Remove();
    //        delete pOpInfo;
        }
        else {
            temp_list.AddTail(pOpInfo);
            pOpInfo->SetIndex(i);
            ++i;
        }
    }
    m_p_op_info_list.RemoveAll();
    m_p_op_info_list.AddTail(&temp_list);

	return m_p_op_info_list.GetCount();
}

DWORD CPlayerProfile::Validate(void)
{
	CString csHexChars, csIllegalChars;
//	int i;
	csHexChars = _T("abcdefABCDEF0123456789");
	csIllegalChars = _T("<>:\"/\\|*?");
	// init return values
	m_status = PROFILE_OK;
	m_error_msg.LoadString(IDS_PROFILE_OK);

	if ( m_bLockedProfile)
		return m_status; // skip a locked profile

	// Profile Description:
	if ( m_cs_name.IsEmpty() )
	{
		// no name, can't check directoy
		if (m_status < PROFILE_ERROR)
		{
			m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_DESCRIPTION);
		}
	}
	else if ( m_cs_name.FindOneOf(csIllegalChars) != -1 )
	{
		if (m_status < PROFILE_ERROR)
		{
			m_status = PROFILE_ERROR;
			m_error_msg.Format(IDS_PROFILE_ERR_INVALID_CHARS, csIllegalChars);
		}
	}
	else
	{
		m_cs_profile_path = m_cs_profile_root + m_cs_name;
		m_cs_ini_file = m_cs_profile_path + _T("\\player.ini");
	}
/*
	// USB Vendor ID:
	if ( m_cs_usb_vid.GetLength() != 4 ) {
		if (m_status < PROFILE_ERROR) {
			m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_USB_VENDOR_ID);
		}
	} else {
		for ( i=0; i<m_cs_usb_vid.GetLength(); ++i ) {
			if ( csHexChars.Find(m_cs_usb_vid.GetAt(i)) == -1 ) {
				if (m_status < PROFILE_ERROR) {
					m_status = PROFILE_ERROR;
					m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_USB_VENDOR_ID);
				}
			}
		}
	}
	// USB Product ID:
	if ( m_cs_usb_pid.GetLength() != 4 ) {
		if (m_status < PROFILE_ERROR) {
			m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_USB_PROD_ID);
		}
	} else {
		for ( i=0; i<m_cs_usb_pid.GetLength(); ++i ) {
			if ( csHexChars.Find(m_cs_usb_pid.GetAt(i)) == -1 ) {
				if (m_status < PROFILE_ERROR) {
					m_status = PROFILE_ERROR;
					m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_USB_PROD_ID);
				}
			}
		}
	}
	// SCSI Vendor ID:
	if ( m_cs_scsi_mfg.IsEmpty() || m_cs_scsi_mfg.GetLength() > 8 ) {
		if (m_status < PROFILE_ERROR) {
			m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_SCSI_VENDOR_ID);
		}
	}
	// SCSI Product ID:
	if ( m_cs_scsi_product.IsEmpty() || m_cs_scsi_product.GetLength() > 16 ) {
		if (m_status < PROFILE_ERROR) {
			m_status = PROFILE_ERROR;
			m_error_msg.LoadString(IDS_PROFILE_ERR_INVALID_SCSI_PROD_ID);
		}
	}
	// Volume Label:
	if ( m_cs_volume_label.GetLength() > 11 ) {
		if (m_status < PROFILE_WARNING) {
			m_status = PROFILE_WARNING;
			m_error_msg.LoadString(IDS_PROFILE_WRN_INVALID_VOLUME_LABLE);
		}
	}
	m_b_use_volume_label &= ( !m_cs_volume_label.IsEmpty() && m_cs_volume_label.GetLength() <= 11 );
*/
	// Operations:
	int num_ops = 0, num_valid_ops = 0;
	COpInfo* p_info;
//	GetPrivateProfileSection(_T("OPERATIONS"), csTemp.GetBufferSetLength(1024), 1024, m_cs_ini_file);
	POSITION pos = m_p_op_info_list.GetHeadPosition();
    while (pos) {
		p_info = m_p_op_info_list.GetNext(pos);
		if ( p_info->GetStatus() == OPINFO_OK ) {
			++num_valid_ops;
		}
		else {
			p_info->OnEnable(false);
		}
		++num_ops;
	}
	if ( num_valid_ops == 0 ) {
        if ( m_status < PROFILE_ERROR ) {
            m_status = PROFILE_ERROR;
		    m_error_msg.LoadString(IDS_PROFILE_ERR_NO_VALID_OPERATION);
        }
	}
	else if ( ( num_ops-1 != num_valid_ops && m_edit_mode != PROFILE_MODE_READ_ONLY ) ||  // take off 1 for the Add... ( new operation )
              (   num_ops != num_valid_ops && m_edit_mode == PROFILE_MODE_READ_ONLY ) ) {
        
        if ( m_status < PROFILE_WARNING ) {
		    m_status = PROFILE_WARNING;
		    m_error_msg.LoadString(IDS_PROFILE_WRN_INVALID_OPERATION);
        }
	}
	return m_status;
}

CString CPlayerProfile::GetProductVersion()
{
	COpInfo* p_info;
	POSITION pos = m_p_op_info_list.GetHeadPosition();
    while (pos) {
		p_info = m_p_op_info_list.GetNext(pos);
		if ( p_info->GetType() == COperation::UPDATE_OP && p_info->IsEnabled())
			return p_info->GetProductVersion();

	}

	return NULL;
}

