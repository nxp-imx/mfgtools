#include "stdafx.h"
#include "StMfgTool.h"
#include "OpInfo.h"
#include "Libs/DevSupport/StFwComponent.h"

// COpInfo
//const PTCHAR COpInfo::OptionsStrings[] = {  _T("Use static-id recovery mode firmware"), 
                                            //_T("Use multiple static-id firmware files (multiple data drives only)"),
                                            //_T("FAT32")
                                         //};
const PTCHAR COpInfo::OptionsCmdStrings[] = { _T("STATIC_ID_FW=TRUE"),
                                              _T("USE_MULTIPLE_STATIC_ID_FW=TRUE"),
                                              _T("FILE_SYSTEM=FAT32"),
											  _T("WINDOWS_CE=TRUE"),
											  _T("ERASE_MEDIA=TRUE"),
											  _T("UPDATE_BOOT_FW=stmfgmsc.sb")};

const PTCHAR COpInfo::UpdateFwString = _T("UPDATE_BOOT_FW=");

COpInfo::COpInfo(CPlayerProfile * _profile, LPCTSTR _cmd_line, INT_PTR _index)
	: m_b_enabled(TRUE) // if it is not in the file, turn it on by default
	, m_p_profile(_profile)
	, m_status(IDS_OPINFO_ERR_NOT_INITIALIZED)
	, m_timeout( 0 ) // default operation watchdog timeout to 2 minutes ( 2 * 60 seconds )
	, m_error_msg(_T(""))
	, m_index(_index)
	, m_cs_cmd_line(_cmd_line)
	, m_e_type(COperation::INVALID_OP)
    , m_ul_param(0)
    , m_drv_mask(0)
    , m_data_drive_num(0)
	, m_b_new_op(FALSE)
	, m_cs_new_ini_section(_T(""))
	, m_csOTPValue(_T(""))
	, m_update_boot_fname_list_index(-1)
	, m_ucl_fname_list_index(-1)
	, m_UclInstallSection(_T(""))
{
	ASSERT(m_p_profile);
	if ( m_cs_cmd_line.Find(_T('=')) != -1 ) {
		int pos = 0;
		m_cs_desc = m_cs_cmd_line.Tokenize(_T("="), pos);
		if ( m_cs_cmd_line.GetAt(pos) == _T(',') ) {
			m_cs_ini_section.Empty();	
			m_timeout = _tstoi(m_cs_cmd_line.Tokenize(_T(","), pos));

			if ( m_cs_cmd_line.GetAt(pos-1) == _T(',') ) {
				m_b_enabled = (m_cs_cmd_line.Tokenize(_T("\r\n"), pos)) == _T("1") ? 1 : 0;
			}
		
		}
		else {
			m_cs_ini_section = m_cs_cmd_line.Tokenize(_T(",\0"), pos);
			if ( pos != -1 ) {
				m_cs_path.Format(_T("%s\\%s"), m_p_profile->m_cs_profile_path, m_cs_ini_section);
				
				if ( m_cs_cmd_line.GetAt(pos-1) == _T(',') ) {
					m_timeout = _tstoi(m_cs_cmd_line.Tokenize(_T(","), pos));
				}
				if ( m_cs_cmd_line.GetAt(pos-1) == _T(',') ) {
					m_b_enabled = (m_cs_cmd_line.Tokenize(_T("\r\n"), pos)) == _T("1") ? 1 : 0;
				}
			}
		}
	}

	m_FileList.m_csRootPath = m_cs_path;
}

COpInfo::COpInfo(CPlayerProfile * _profile)
	: m_p_profile(_profile)
	, m_b_enabled(FALSE)
	, m_status(IDS_OPINFO_ERR_NOT_INITIALIZED)
	, m_timeout( 0 ) // default operation watchdog timeout to 2 minutes ( 2 * 60 seconds )
	, m_error_msg(_T(""))
	, m_index(0)
	, m_cs_cmd_line(_T(""))
	, m_e_type(COperation::INVALID_OP)
	, m_cs_desc(_T(""))
	, m_cs_ini_section(_T(""))
	, m_cs_new_ini_section(_T(""))
	, m_cs_path(_T(""))
	, m_ul_param(0)
    , m_drv_mask(0)
    , m_data_drive_num(0)
	, m_update_boot_fname_list_index(-1)
	, m_ucl_fname_list_index(-1)
	, m_UclInstallSection(_T(""))
{
	m_FileList.RemoveAll();

	m_cs_path.Format(_T("%s\\%s"), m_p_profile->m_cs_profile_path, m_cs_ini_section);

	m_FileList.m_csRootPath = m_cs_path;

}

COpInfo::~COpInfo()
{
    m_drive_array.Clear();
	m_FileList.RemoveAll();
}

DWORD COpInfo::ValidateDrvInfo(media::LogicalDrive* _drv)
{
    CString csTemp;

    // type //
    if ( _drv->Type != media::DriveType_Data &&
         _drv->Type != media::DriveType_System  &&
         _drv->Type != media::DriveType_HiddenData ) {

        csTemp.LoadString(IDS_OPINFO_ERR_INVALID_DRV_TYPE);
        m_error_msg.Format(csTemp, m_FileList.GetFileNameAt(_drv->FileListIndex));
		return ( OPINFO_ERROR );
    }
    // name //
    if ( _drv->Type == media::DriveType_System ) {
        if ( _taccess(_drv->Name.c_str(), 0) == -1 ) {
			csTemp.LoadString(IDS_OPINFO_ERR_MISSING_FILE);
            m_error_msg.Format(csTemp, m_FileList.GetFileNameAt(_drv->FileListIndex));
			return ( OPINFO_ERROR );
		}
    }
    // description //
    if ( _drv->Type == media::DriveType_System  || _drv->Type == media::DriveType_HiddenData ) {
        if ( _drv->Description.empty() ) {
			csTemp.LoadString(IDS_OPINFO_ERR_NO_FILE_DESC);
            m_error_msg.Format(csTemp, m_FileList.GetFileNameAt(_drv->FileListIndex));
			return ( OPINFO_ERROR );
        }
    }
    // tag // can't really validate tags
    switch ( _drv->Tag ) {
		case media::DriveTag_Bootmanger:   // all
            m_drv_mask |= UPDATE_DRIVE_BOOTMANAGER;
            break;
		case media::DriveTag_Player:       // all
            m_drv_mask |= UPDATE_DRIVE_PLAYER;
            break;
		case media::DriveTag_PlayerRsc:    // all
		case media::DriveTag_PlayerRsc2:    // all
            m_drv_mask |= UPDATE_DRIVE_PLAYER_RES;
            break;
		case media::DriveTag_PlayerRsc3:    // all
            m_drv_mask |= UPDATE_ALL_SDK2612_DRIVES;
            break;
		case media::DriveTag_Data:            // all
            m_drv_mask |= UPDATE_DRIVE_DATA;
            break;

		case media::DriveTag_UsbMsc:        // mtp & usbmsc
//        case media::DriveTag_Hostlink:
            m_drv_mask |= UPDATE_DRIVE_USBMSC_HOST;
            break;

		case media::DriveTag_HostlinkRsc:     // mtp
			m_drv_mask |= UPDATE_DRIVE_HOST_RES;
            break;
		case media::DriveTag_HostlinkRsc2:
        case media::DriveTag_HostlinkRsc3:
			m_drv_mask |= UPDATE_ALL_SDK3120_DRIVES;
            break;
		case media::DriveTag_DataJanus:     // mtp
            m_drv_mask |= UPDATE_DRIVE_DATA_HIDDEN;
            break;
		case media::DriveTag_Updater:       // mtp
            m_drv_mask |= UPDATE_DRIVE_UPDATER;
            break;

		case media::DriveTag_Extra:         // optional
            m_drv_mask |= UPDATE_DRIVE_EXTRA;
            break;
		case media::DriveTag_ExtraRsc:   // optional
            m_drv_mask |= UPDATE_DRIVE_RES_1;
            break;
		case media::DriveTag_Otg:       // optional
            m_drv_mask |= UPDATE_DRIVE_OTG;
            break;
        default:
            break;
    }
    // additional_memory // can't really validate additional_memory

	// get player product version
	if( _drv->Tag == media::DriveTag_Player || _drv->Tag == media::DriveTag_FirmwareImg)
	{
		CString csPathName;

		csPathName.Format(L"%s", _drv->Name.c_str());
		StFwComponent tempFwObj(csPathName, m_p_profile->m_bLockedProfile ? 
			StFwComponent::LoadFlag_ResourceOnly : StFwComponent::LoadFlag_FileFirst);
		if ( tempFwObj.GetLastError() == ERROR_SUCCESS )
		{
			m_versionStr = tempFwObj.GetProductVersion().toString();
		}
	}

    return OPINFO_OK;
}

#ifdef _DEBUG
void COpInfo::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);
	dc << "OpsList = " << m_cs_ini_section;
}
#endif

DWORD COpInfo::ReadProfileOpAndValidate(void)
{
	CString csTemp, csFile;

	for (int i=0; i < COperation::NO_MORE_OPS; ++i ) {
		if ( m_cs_desc.CompareNoCase(COperation::OperationStrings[i]) == 0) {
			m_e_type = (COperation::OpTypes)i;
			break;
		}
	}
	if ( m_e_type  == COperation::INVALID_OP ) {
		m_error_msg.LoadString(IDS_OPINFO_ERR_INVALID_OPERATION);
		return m_status = OPINFO_ERROR;
	}

	m_status = OPINFO_OK;
    m_drive_array.Clear();
	m_FileList.RemoveAll();

	switch ( m_e_type ) {
		case COperation::UPDATE_OP:
		{
			// see if <AppDir>\Profiles\<Player Description>\<Operation Description> exists
			if ( !m_b_new_op && _taccess(m_cs_path, 0) == -1 ) {
				csTemp.LoadString(IDS_OPINFO_ERR_MISSING_DIRECTORY);
				m_error_msg.Format(csTemp, m_cs_path);
				return (  m_status = OPINFO_ERROR );
			}

			// is there a static-id firmware entry?
			GetPrivateProfileString(m_cs_ini_section, _T("STATIC_ID_FW"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
			csTemp.ReleaseBuffer();
			if ( !csTemp.IsEmpty() )
			{
				if (csTemp.CompareNoCase(_T("TRUE")) == 0)
					m_ul_param |= UL_PARAM_USE_STATIC_FW;
			}

			// is there a file_system entry?
			GetPrivateProfileString(m_cs_ini_section, _T("FILE_SYSTEM"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
			csTemp.ReleaseBuffer();
			if ( !csTemp.IsEmpty() )
			{
				if (csTemp.CompareNoCase(_T("FAT32")) == 0)
					m_ul_param |= UL_PARAM_FAT32;
			}
			// is there a multiple static-id firmware files entry?
			GetPrivateProfileString(m_cs_ini_section, _T("USE_MULTIPLE_STATIC_ID_FW"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
			csTemp.ReleaseBuffer();
			if ( !csTemp.IsEmpty() )
			{
				if (csTemp.CompareNoCase(_T("TRUE")) == 0)
					m_ul_param |= UL_PARAM_USE_MULTIPLE_STATIC_ID_FW;
			}

			// is there a Windows CE firmware files entry?
			GetPrivateProfileString(m_cs_ini_section, _T("WINDOWS_CE"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
			csTemp.ReleaseBuffer();
			if ( !csTemp.IsEmpty() )
			{
				if (csTemp.CompareNoCase(_T("TRUE")) == 0)
					m_ul_param |= UL_PARAM_WINCE;
			}


/*
			// is there an erase media entry?  We do erase by default.
			GetPrivateProfileString(m_cs_ini_section, _T("ERASE_MEDIA"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
			csTemp.ReleaseBuffer();
			if ( !csTemp.IsEmpty() )
			{
				if (csTemp.CompareNoCase(_T("TRUE")) == 0)
					m_ul_param |= UL_PARAM_ERASE_MEDIA;
			}
*/
			GetPrivateProfileString(m_cs_ini_section, _T("UPDATE_BOOT_FW"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
			csTemp.ReleaseBuffer();
			if ( !csTemp.IsEmpty() )
			{
				CString csUpdateBootFilename;
				csUpdateBootFilename.Format(_T("%s\\%s"), m_cs_path, csTemp);
				if ( _taccess(csUpdateBootFilename, 0) != -1 )
				{
					SetUpdaterBootFilename( csUpdateBootFilename, CFileList::EXISTS_IN_TARGET );
				}
			}

			// get firmware file names
			int n_copied_chars, pos = 0;
			n_copied_chars = GetPrivateProfileSection(m_cs_ini_section, csFile.GetBufferSetLength(1024), 1024, m_p_profile->m_cs_ini_file);
			csFile.ReleaseBuffer(n_copied_chars+1);
			if ( n_copied_chars != 0 && n_copied_chars != (1024-2) ) {
				// parse section
				// Need to store the file names for the profile editor
				while (csFile.GetAt(pos) != _T('\0')) {
					DWORD dwStatus;
					CString csField;
                    int lpos = 0;
                    CString csLine = csFile.Tokenize(_T("\r\n"), pos);
					// already handled the STATIC_ID_FW case
                    if ( csLine.Find(_T("STATIC_ID_FW")) != -1 )
						continue;
					// already handled the FILE_SYSTEM case
                    if ( csLine.Find(_T("FILE_SYSTEM")) != -1 )
						continue;
					// already handled the USE_MULTIPLE_STATIC_ID case
                    if ( csLine.Find(_T("USE_MULTIPLE_STATIC_ID_FW")) != -1 )
						continue;
					// already handled the Windows CE case
                    if ( csLine.Find(_T("WINDOWS_CE")) != -1 )
						continue;
					// already handled the ERASE_MEDIA case
//                    if ( csLine.Find(_T("ERASE_MEDIA")) != -1 )
//						continue;
					// already handled the UPDATE_BOOT_FW case
                    if ( csLine.Find(_T("UPDATE_BOOT_FW")) != -1 )
						continue;
					media::LogicalDrive dd;
                    // file name
                    if ( csLine.GetAt(0) != _T(',') ) { 
                        csField = csLine.Tokenize(_T(","), lpos);
                        // skip the file if it is the static id manufacturing firmware
                        if ( (csField.MakeLower().Find(STATIC_ID_FW_FILENAME) != -1) ||
							(csField.MakeLower().Find(UPDATER_FW_FILENAME) != -1) )
						{
							CString csUpdateBootFilename;
							csUpdateBootFilename.Format(_T("%s\\%s"), m_cs_path, csField);
							if ( _taccess(csUpdateBootFilename, 0) != -1 )
							{
								SetUpdaterBootFilename( csUpdateBootFilename, CFileList::EXISTS_IN_TARGET );
							}
                            continue;
						}
						// Add to string list for the profile editor

						csTemp.Format(_T("%s\\%s"), m_cs_path, csField);
						dd.Name = csTemp;
						dd.FileListIndex = m_FileList.AddFile(csTemp, CFileList::EXISTS_IN_TARGET);
                    }
                    else ++lpos;
                    if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
                    // file description
                    if ( csLine.GetAt(lpos) != _T(',') ) { 
                        dd.Description = csLine.Tokenize(_T(","), lpos);
                    }
                    else ++lpos;
                    if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
                    // drive type
                    if ( csLine.GetAt(lpos) != _T(',') ) { 
						dd.Type = (media::LogicalDriveType)_tstoi(csLine.Tokenize(_T(","), lpos));
                    }
                    else ++lpos;
                    if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
                    // drive tag
                    if ( csLine.GetAt(lpos) != _T(',') ) {
                        CString str = csLine.Tokenize(_T(","), lpos);
                        _stscanf_s(str, _T("%x"), &dd.Tag);
                    }
                    else ++lpos;
                    if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
                    // drive flags
                    if ( csLine.GetAt(lpos) != _T(',') ) {
                        CString str = csLine.Tokenize(_T(","), lpos);
						_stscanf_s(str, _T("%x"), &dd.Flags);
                    }
                    else ++lpos;
                    if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
                    // requested_drive_size_kb
                    if ( csLine.GetAt(lpos) != _T(',') ) { 
                        dd.RequestedKB = _tstoi(csLine.Tokenize(_T(","), lpos));
                    }
validate:
					if (dd.Type != media::DriveType_Invalid)
					{
						m_drive_array.AddDrive(dd);

	                    if ( (dwStatus = ValidateDrvInfo(&dd) ) != OPINFO_OK )
		                {
							m_status = dwStatus;
				            //return ( m_status );
					    }
					}
				}

			} // end if (got section)
			break;
		}
		case COperation::COPY_OP:
		case COperation::LOADER_OP:
		{
			// if an existing op, check that the folder and file(s) exist
			if ( !m_b_new_op )
			{
				// see if <AppDir>\Profiles\<Player Description>\<Operation Description> exists
				if ( _taccess(m_cs_path, 0) == -1 ) {
					csTemp.LoadString(IDS_OPINFO_ERR_MISSING_DIRECTORY);
					m_error_msg.Format(csTemp, m_cs_path);
					return ( m_status = OPINFO_ERROR );
				}

				if (m_FileList.GetCount())
				{
					if (ValidateFileList(&m_FileList) != OPINFO_OK)
						return m_status;
				}
				else
				{
					if ( EnumerateFolderFiles(&m_FileList) != OPINFO_OK )
						return m_status;
				}
			}
		}
		break;

		case COperation::OTP_OP:
		{
			// see if <AppDir>\Profiles\<Player Description>\<Operation Description> exists
			if ( !m_b_new_op && _taccess(m_cs_path, 0) == -1 ) {
				csTemp.LoadString(IDS_OPINFO_ERR_MISSING_DIRECTORY);
				m_error_msg.Format(csTemp, m_cs_path);
				return (  m_status = OPINFO_ERROR );
			}

			GetPrivateProfileString(m_cs_ini_section, _T("VALUE"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
			// can we validate this somehow?
			m_csOTPValue = csTemp;
			break;
		}

		case COperation::ERASE_OP:
			break;
		case COperation::REGISTRY_OP:
			break;

		case COperation::UTP_UPDATE_OP:
		case COperation::MX_UPDATE_OP:
		{
			// see if <AppDir>\Profiles\<Player Description>\<Operation Description> exists
			if ( !m_b_new_op && _taccess(m_cs_path, 0) == -1 ) {
				csTemp.LoadString(IDS_OPINFO_ERR_MISSING_DIRECTORY);
				m_error_msg.Format(csTemp, m_cs_path);
				return (  m_status = OPINFO_ERROR );
			}

			if ( !m_b_new_op )
			{
				GetPrivateProfileString(m_cs_ini_section, _T("UCL_INSTALL_SECTION"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
				csTemp.ReleaseBuffer();
				if ( csTemp.IsEmpty() )
				{
					m_error_msg = _T("Operation error: UCL_INSTALL_SECTION= value was not found in player.ini file.");
					return ( m_status = OPINFO_ERROR );
				}
				m_UclInstallSection = csTemp;

				if ( EnumerateFolderFiles(&m_FileList) != OPINFO_OK )
					return m_status;
				
				CString csUclFilename;
				csUclFilename.Format(_T("%s\\%s"), m_cs_path, _T("ucl.xml"));
				SetUclFilename( csUclFilename, CFileList::EXISTS_IN_TARGET );
				if ( m_ucl_fname_list_index == -1 )
				{
					m_error_msg.Format(_T("Operation error: Can not find ucl.xml in %s."), m_cs_path);
					return ( m_status = OPINFO_ERROR );
				}
			}
			break;
		}
/*		case COperation::MX_UPDATE_OP:
		{
			// see if <AppDir>\Profiles\<Player Description>\<Operation Description> exists
			if ( !m_b_new_op && _taccess(m_cs_path, 0) == -1 ) {
				csTemp.LoadString(IDS_OPINFO_ERR_MISSING_DIRECTORY);
				m_error_msg.Format(csTemp, m_cs_path);
				return (  m_status = OPINFO_ERROR );
			}

			if ( !m_b_new_op )
			{
				GetPrivateProfileString(m_cs_ini_section, _T("RKL_FW"), _T(""), csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
				csTemp.ReleaseBuffer();
				if ( !csTemp.IsEmpty() )
				{
					CString csRklFilename;
					csRklFilename.Format(_T("%s\\%s"), m_cs_path, csTemp);
					if ( _taccess(csRklFilename, 0) != -1 )
					{
						SetRklFilename( csRklFilename, CFileList::EXISTS_IN_TARGET );
					}
				}

				GetPrivateProfileString(m_cs_ini_section, NULL, NULL, csTemp.GetBufferSetLength(_MAX_PATH), _MAX_PATH, m_p_profile->m_cs_ini_file);
				// get firmware file names
				int n_copied_chars, pos = 0;
				n_copied_chars = GetPrivateProfileSection(m_cs_ini_section, csFile.GetBufferSetLength(1024), 1024, m_p_profile->m_cs_ini_file);
				csFile.ReleaseBuffer(n_copied_chars+1);
				if ( n_copied_chars != 0 && n_copied_chars != (1024-2) ) {
					// parse section
					// Need to store the file names for the profile editor
					while (csFile.GetAt(pos) != _T('\0')) {
						DWORD dwStatus;
						CString csField;
						int lpos = 0;
						CString csLine = csFile.Tokenize(_T("\r\n"), pos);
						// already handled the STATIC_ID_FW case
						if ( csLine.Find(_T("RKL_FW")) != -1 )
							continue;
						media::MxImageObject imageObj;
						// file name
						if ( csLine.GetAt(0) != _T(',') ) { 
							csField = csLine.Tokenize(_T(","), lpos);
							// skip the file if it is the static id manufacturing firmware
							if ( (csField.MakeLower().Find(STATIC_ID_FW_FILENAME) != -1) ||
								(csField.MakeLower().Find(UPDATER_FW_FILENAME) != -1) )
							{
								CString csUpdateBootFilename;
								csUpdateBootFilename.Format(_T("%s\\%s"), m_cs_path, csField);
								if ( _taccess(csUpdateBootFilename, 0) != -1 )
								{
									SetUpdaterBootFilename( csUpdateBootFilename, CFileList::EXISTS_IN_TARGET );
								}
								continue;
							}
							// Add to string list for the profile editor

							csTemp.Format(_T("%s\\%s"), m_cs_path, csField);
							dd.Name = csTemp;
							dd.FileListIndex = m_FileList.AddFile(csTemp, CFileList::EXISTS_IN_TARGET);
						}
						else ++lpos;
						if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
						// file description
						if ( csLine.GetAt(lpos) != _T(',') ) { 
							dd.Description = csLine.Tokenize(_T(","), lpos);
						}
						else ++lpos;
						if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
						// drive type
						if ( csLine.GetAt(lpos) != _T(',') ) { 
							dd.Type = (media::LogicalDriveType)_tstoi(csLine.Tokenize(_T(","), lpos));
						}
						else ++lpos;
						if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
						// drive tag
						if ( csLine.GetAt(lpos) != _T(',') ) {
							CString str = csLine.Tokenize(_T(","), lpos);
							_stscanf_s(str, _T("%x"), &dd.Tag);
						}
						else ++lpos;
						if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
						// drive flags
						if ( csLine.GetAt(lpos) != _T(',') ) {
							CString str = csLine.Tokenize(_T(","), lpos);
							_stscanf_s(str, _T("%x"), &dd.Flags);
						}
						else ++lpos;
						if ( lpos == -1 || lpos > csLine.GetLength() ) goto validate;
						// requested_drive_size_kb
						if ( csLine.GetAt(lpos) != _T(',') ) { 
							dd.RequestedKB = _tstoi(csLine.Tokenize(_T(","), lpos));
						}
	validate:
						if (dd.Type != media::DriveType_Invalid)
						{
							m_drive_array.AddDrive(dd);

							if ( (dwStatus = ValidateDrvInfo(&dd) ) != OPINFO_OK )
							{
								m_status = dwStatus;
								//return ( m_status );
							}
						}
					}

				} // end if (got section)
			}
			break;
		}
*/		default:
			m_error_msg.LoadString(IDS_OPINFO_ERR_INVALID_OPERATION);
			return m_status = OPINFO_ERROR;
	} // end switch(type)
	if (m_status == OPINFO_OK)
		m_error_msg.LoadString(IDS_OPINFO_OK);
	return m_status;
}



DWORD COpInfo::Validate(void)
{
	CString csTemp, csFile;

	for (int i=0; i < COperation::NO_MORE_OPS; ++i ) {
		if ( m_cs_desc.CompareNoCase(COperation::OperationStrings[i]) == 0) {
			m_e_type = (COperation::OpTypes)i;
			break;
		}
	}
	if ( m_e_type  == COperation::INVALID_OP ) {
		m_error_msg.LoadString(IDS_OPINFO_ERR_INVALID_OPERATION);
		return m_status = OPINFO_ERROR;
	}

	switch ( m_e_type ) {
		case COperation::UPDATE_OP:
		case COperation::COPY_OP:
		case COperation::LOADER_OP:
		case COperation::UTP_UPDATE_OP:
		case COperation::MX_UPDATE_OP:
		{
			// if an existing op, check that the folder and file(s) exist
			if ( !m_b_new_op )
			{
				// see if <AppDir>\Profiles\<Player Description>\<Operation Description> exists
				if ( _taccess(m_cs_path, 0) == -1 ) {
					csTemp.LoadString(IDS_OPINFO_ERR_MISSING_DIRECTORY);
					m_error_msg.Format(csTemp, m_cs_path);
					return ( m_status = OPINFO_ERROR );
				}
			}
			else
			{
				// if this is a new uncommitted op, check for existance of source files
				if (ValidateFileList(&m_FileList) == OPINFO_ERROR)
					return m_status;
			}

			break;
		}
		case COperation::OTP_OP:
			break;
		case COperation::ERASE_OP:
			break;
		case COperation::REGISTRY_OP:
			break;
		default:
			m_error_msg.LoadString(IDS_OPINFO_ERR_INVALID_OPERATION);
			return m_status = OPINFO_ERROR;
	} // end switch(type)
	m_error_msg.LoadString(IDS_OPINFO_OK);
	return m_status = OPINFO_OK;
}

DWORD COpInfo::OnEnable(BOOL _enable)
{
	m_b_enabled = _enable;

	return m_status;
}

CString COpInfo::GetUsbVid(void)
{ 
	return m_p_profile->GetUsbVid(); 
}

CString COpInfo::GetUsbPid(void)
{ 
	return m_p_profile->GetUsbPid();
}

INT_PTR COpInfo::ReplaceIniLine(LPCTSTR _section, LPCTSTR _string, INT_PTR _line)
{
	INT_PTR index = _line;
    CString csSection;
    CStringArray csArray;
    // get the section of the ini file
	GetPrivateProfileSection(_section, csSection.GetBufferSetLength(1024), 1024, m_p_profile->m_cs_ini_file);
   
	int pos = 0;
    while (csSection.GetAt(pos) != _T('\0')) {
		csArray.Add( csSection.Tokenize(_T("\r\n"), pos) );
	}
	if ( _line < csArray.GetCount() )
        csArray.SetAt(_line, _string);
    else
        index = csArray.Add(_string);
    
    csSection.Empty();
    for ( int i=0; i<csArray.GetCount(); ++i ) {
        csSection.Append(csArray.GetAt(i));
        csSection.AppendChar(_T('\0'));
    }
    csSection.AppendChar(_T('\0'));
    // write out the ini file section
	WritePrivateProfileSection(_section, csSection, m_p_profile->m_cs_ini_file);
    
    return index;
}

DWORD COpInfo::WriteIniSection(LPCTSTR _section)
{
    CString csSectionNames;
    CString csSection, csTemp;
	TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT]; 

	// delete the section
//    WritePrivateProfileSection(_section, NULL, m_p_profile->m_cs_ini_file);

	switch ( m_e_type ) {
		case COperation::UPDATE_OP:
//			if( UseFat32() ) {
			for ( int i=0; 1<<i<UL_PARAM_NO_MORE_OPTIONS; ++i ) {
//				csSection.Append(COpInfo::OptionsCmdStrings[1]);
				if (m_ul_param & 1<<i) {
					csSection.Append(COpInfo::OptionsCmdStrings[i]);
					csSection.AppendChar(_T('\0'));
				}
			}

			if (m_update_boot_fname_list_index != -1) {
				csSection.Append(COpInfo::UpdateFwString);
				csSection.Append(m_FileList.GetFileNameAt(m_update_boot_fname_list_index));
				csSection.AppendChar(_T('\0'));
			}


			for ( size_t i=0; i<m_drive_array.Size(); ++i ) {
			    drive[0] = dir[0] = fname[0] = ext[0] = _T('');
				_tsplitpath_s(m_drive_array[i].Name.c_str(), drive, dir, fname, ext);
                csTemp.Format(_T("%s%s"), fname, ext);
                csSection.AppendFormat( _T("%s,%s,%d,0x%02X,%d,%d"), 
                    csTemp.IsEmpty() ? _T("") : csTemp, 
					m_drive_array[i].Description.c_str(), 
                    m_drive_array[i].Type, 
                    m_drive_array[i].Tag,
					m_drive_array[i].Flags,
                    m_drive_array[i].RequestedKB );
                csSection.AppendChar(_T('\0'));
            }
			break;

		case COperation::COPY_OP:
			// Add drive number for copy when multiple data drives are present
			csSection.AppendFormat(_T("DRIVE_NUM=%d"), m_data_drive_num);
			csSection.AppendChar(_T('\0'));
			break; // we don't write the COPY_OP file list; all files under the operation folder are copied.
		case COperation::OTP_OP:
			csSection.AppendFormat(_T("VALUE=%s"), m_csOTPValue);
			csSection.AppendChar(_T('\0'));
			break;
		case COperation::UTP_UPDATE_OP:
		case COperation::MX_UPDATE_OP:
			csSection.AppendFormat(_T("UCL_INSTALL_SECTION=%s"), m_UclInstallSection);
			csSection.AppendChar(_T('\0'));
			break;
		case COperation::ERASE_OP:
		case COperation::REGISTRY_OP:
		case COperation::LOADER_OP:
		default:
				for (int i = 0; i < m_FileList.GetCount(); ++i)
                {
					csSection.Append(m_FileList.GetFileNameAt(i));
                    csSection.AppendChar(_T('\0'));
                }
				break;

	} // end switch(type)

	// write out the ini file section
    csSection.AppendChar(_T('\0'));
    WritePrivateProfileSection(_section, csSection, m_p_profile->m_cs_ini_file);

    return 0;
}

DWORD COpInfo::Remove(void)
{
    CString csTemp, csSection, csSectionNames;
    CStringArray csArray;
    int pos;
    
    // fix up the OPERATIONS section
    if ( !m_cs_cmd_line.IsEmpty() ) {
        GetPrivateProfileSection(_T("OPERATIONS"), csSection.GetBufferSetLength(1024), 1024, m_p_profile->m_cs_ini_file);
 	    pos = 0;
        while (csSection.GetAt(pos) != _T('\0')) {
		    csTemp = csSection.Tokenize(_T("\r\n"), pos);
            if ( csTemp.Find(m_cs_cmd_line) == -1 )
                csArray.Add( csTemp );
	    }
        csSection.Empty();
        for ( int i=0; i<csArray.GetCount(); ++i ) {
            csSection.Append(csArray.GetAt(i));
            csSection.AppendChar(_T('\0'));
        }
	    WritePrivateProfileSection(_T("OPERATIONS"), csSection, m_p_profile->m_cs_ini_file);
    }
    if ( !m_cs_ini_section.IsEmpty() ) {
        // delete the [<Operation_Name>] section
        GetPrivateProfileSectionNames(csSectionNames.GetBufferSetLength(1024), 1024, m_p_profile->m_cs_ini_file);
        BOOL found = FALSE;
	    pos = 0;
        while (csSectionNames.GetAt(pos) != _T('\0')) {
		    csTemp = csSectionNames.Tokenize(_T("\r\n"), pos);
		    if ( csTemp.CompareNoCase(m_cs_ini_section) == 0 ) {
			    found = TRUE;
			    break;
		    }
	    }
	    if ( found ) {
            WritePrivateProfileSection(m_cs_ini_section, NULL, m_p_profile->m_cs_ini_file);
        }
    }
    // delete the op directory if it exists
    if ( _taccess(m_cs_path, 0) == 0 ) {
        CString cs_from = m_cs_path;
        cs_from.AppendChar(_T('\0'));
        SHFILEOPSTRUCT FileOp;
        FileOp.hwnd = theApp.GetConfigMgr()->GetSafeHwnd();
        FileOp.hNameMappings = NULL;
        FileOp.lpszProgressTitle = NULL;
	    CWaitCursor wait;

	    FileOp.wFunc = FO_DELETE;
	    FileOp.pFrom = cs_from;
	    FileOp.pTo = NULL;
	    FileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
	    if ( SHFileOperation(&FileOp) != ERROR_SUCCESS ) {
		    ATLTRACE(_T("COpInfo::Remove() - Could not delete operation directory. (%d)\n"), GetLastError());
	    }
	    else {
		    // SUCCESS
		    ATLTRACE(_T("COpInfo::Remove() - SUCCESS.\n"));
	    }
    }

    return 0;
}

DWORD COpInfo::ValidateFileList(CFileList * _pFileList)
{
	m_status = OPINFO_OK;

	for (int i = 0; i < _pFileList->GetCount(); ++i)
	{
		CString csTemp;
		if ( _taccess(_pFileList->GetPathNameAt(i), 0) == -1)
		{
			csTemp.LoadString(IDS_OPINFO_ERR_MISSING_FILE);
			m_error_msg.Format(csTemp, m_FileList.GetPathNameAt(i));
			return ( m_status = OPINFO_ERROR ); 
		}
		
		CFileList::PFILEITEM pItem = _pFileList->GetAt(i);

		if (pItem->m_pSubFolderList)
			if (ValidateFileList((CFileList *)pItem->m_pSubFolderList) == OPINFO_ERROR)
				break;
	}

	return m_status;
}

DWORD COpInfo::EnumerateFolderFiles(CFileList *pFileList)
{
	HANDLE hFind;
	BOOL fFinished = FALSE;
	WIN32_FIND_DATA FindFileData;
	CString csTemp;

	csTemp = pFileList->m_csRootPath;
	csTemp.Append(_T("\\*.*"));
	hFind = FindFirstFile(csTemp, &FindFileData);
	if ( hFind != INVALID_HANDLE_VALUE ) {
		while (!fFinished)
		{ 
			if ( (_tcscmp(FindFileData.cFileName, _T(".")) != 0) &&
				(_tcscmp(FindFileData.cFileName, _T("..")) != 0) )
			{
				int index;
				CString csNewFile;

				csNewFile.Format(_T("%s\\%s"), pFileList->m_csRootPath, FindFileData.cFileName);
				index = pFileList->AddFile(csNewFile, CFileList::EXISTS_IN_TARGET);

				CFileList::PFILEITEM pItem = pFileList->GetAt(index);

				if ( pItem->m_dwAttr & FILE_ATTRIBUTE_DIRECTORY )
				{
					// if a folder, allocate new list and enumerate
					DWORD rc;

					pItem->m_pSubFolderList = new CFileList( csNewFile );
					rc = EnumerateFolderFiles( pItem->m_pSubFolderList );
					if (rc != OPINFO_OK)
					{
						FindClose(hFind);
						return m_status = rc;
					}
				}
			}

			if (!FindNextFile(hFind, &FindFileData))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES) 
					fFinished = TRUE; 
				else
				{
					m_error_msg.LoadString(IDS_OPINFO_ERR_SYSTEM_ERROR);
					FindClose(hFind);
					return ( m_status = OPINFO_ERROR ); 
				} 
			}
		} // done making a file list
		if (m_FileList.GetCount() == 0)
		{
			CString csErr;
			csErr.LoadString(IDS_OPINFO_ERR_EMPTY_DIRECTORY);
			m_error_msg.Format(csErr, m_cs_path);
			FindClose(hFind);
			return ( m_status = OPINFO_ERROR );
		}
	} // end if (HANDLE)
	FindClose(hFind);

	return m_status;
}

CString COpInfo::GetUpdaterBootFilename(void)
{
	if (m_update_boot_fname_list_index != -1)
		return m_FileList.GetFileNameAt(m_update_boot_fname_list_index);
	else return NULL;
}

CString COpInfo::GetUpdaterBootPathname(void)
{
	if (m_update_boot_fname_list_index != -1)
		return m_FileList.GetPathNameAt(m_update_boot_fname_list_index);
	else return NULL;
}

void COpInfo::SetUpdaterBootFilename(CString _fName, CFileList::FileListAction _action)
{
	if (m_update_boot_fname_list_index != -1)
		RemoveUpdaterBootFilename();

	m_update_boot_fname_list_index = m_FileList.AddFile(_fName, _action);
}

void COpInfo::RemoveUpdaterBootFilename(void)
{
	if (m_update_boot_fname_list_index != -1)
	{
		m_FileList.RemoveFile(m_update_boot_fname_list_index);
		m_update_boot_fname_list_index = -1;
	}
}

CString COpInfo::GetUclFilename(void)
{
	if (m_ucl_fname_list_index != -1)
		return m_FileList.GetFileNameAt(m_ucl_fname_list_index);
	else return NULL;
}
/*
CString COpInfo::GetRklFilename(void)
{
	if (m_rkl_fname_list_index != -1)
		return m_FileList.GetFileNameAt(m_rkl_fname_list_index);
	else return NULL;
}
*/
CString COpInfo::GetUclPathname(void)
{
	if (m_ucl_fname_list_index != -1)
		return m_FileList.GetPathNameAt(m_ucl_fname_list_index);
	else return NULL;
}
/*
CString COpInfo::GetRklPathname(void)
{
	if (m_rkl_fname_list_index != -1)
		return m_FileList.GetPathNameAt(m_rkl_fname_list_index);
	else return NULL;
}
*/
void COpInfo::SetUclFilename(CString _fName, CFileList::FileListAction _action)
{
	int index = -1;
	if ( m_FileList.FindFile(_fName, index) != NULL )
	{
		m_ucl_fname_list_index = index;
	}
	else
	{
		if (m_ucl_fname_list_index != -1)
			RemoveUclFilename();

		m_ucl_fname_list_index = m_FileList.AddFile(_fName, _action);
	}
}
/*
void COpInfo::SetRklFilename(CString _fName, CFileList::FileListAction _action)
{
	int index = -1;
	if ( m_FileList.FindFile(_fName, index) != NULL )
	{
		m_rkl_fname_list_index = index;
	}
	else
	{
		if (m_rkl_fname_list_index != -1)
			RemoveRklFilename();

		m_rkl_fname_list_index = m_FileList.AddFile(_fName, _action);
	}
}
*/
void COpInfo::RemoveUclFilename(void)
{
	if (m_ucl_fname_list_index != -1)
	{
		m_FileList.RemoveFile(m_ucl_fname_list_index);
		m_ucl_fname_list_index = -1;
	}
}
/*
void COpInfo::RemoveRklFilename(void)
{
	if (m_rkl_fname_list_index != -1)
	{
		m_FileList.RemoveFile(m_rkl_fname_list_index);
		m_rkl_fname_list_index = -1;
	}
}
*/