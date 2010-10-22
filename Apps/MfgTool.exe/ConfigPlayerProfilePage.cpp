/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// ConfigPlayerProfilePage.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigPlayerProfilePage.h"
#include "PlayerProfile.h"
#include "OpUpdateDlg.h"
#include "CopyOpDlg.h"
#include "LoadFileOpDlg.h"
#include "OpOTPDlg.h"
//#include "OpUtpUpdateDlg.h"
//#include "OpMxRomUpdateDlg.h"
#include "OpUclDlg.h"
#include "ConfigUSBPortPage.h"
#include "StMfgTool.h"
#include "DefaultProfile.h"
#include "resource.h"
#include "../../Common/updater_res.h"

// CConfigPlayerProfilePage dialog
IMPLEMENT_DYNAMIC(CConfigPlayerProfilePage, CPropertyPage)


CConfigPlayerProfilePage::CConfigPlayerProfilePage(CWnd * cParent /*=NULL*/)
	: CPropertyPage(CConfigPlayerProfilePage::IDD)
	, m_cs_old_player_profile(_T(""))
	, m_hi_ok(0)
	, m_hi_warning(0)
	, m_hi_error(0)
{
	m_p_player_profile = NULL; //new CPlayerProfile;
}

CConfigPlayerProfilePage::~CConfigPlayerProfilePage()
{
	// cleanup our profile lists if we are exiting without OK
	if ( m_ProfileList.GetCount() )
	{
		m_ProfileList.RemoveAll();
	}
}

void CConfigPlayerProfilePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PLAYER_PROFILE_COMBO, m_cb_player_profile_ctrl);
	DDX_Control(pDX, IDC_OPERATIONS_LIST, m_operations_ctrl);
	DDX_Control(pDX, IDC_STATUS_ICON, m_status_icon_ctrl);
	DDX_Control(pDX, IDC_STATUS_TEXT, m_status_ctrl);
}


BEGIN_MESSAGE_MAP(CConfigPlayerProfilePage, CPropertyPage)
	ON_MESSAGE(WM_UPDATE_STATUS, OnUpdateStatus)
	ON_CBN_SELCHANGE(IDC_PLAYER_PROFILE_COMBO, OnCbnSelchangeProductDescCombo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OPERATIONS_LIST, OnLvnItemChangedOperationsList)
    ON_NOTIFY(NM_KILLFOCUS, IDC_OPERATIONS_LIST, OnNMKillfocusOperationsList)
    ON_NOTIFY(NM_SETFOCUS, IDC_OPERATIONS_LIST, OnNMSetfocusOperationsList)
    ON_NOTIFY(NM_CLICK, IDC_OPERATIONS_LIST, OnNMClickOperationsList)
    ON_NOTIFY(LVN_KEYDOWN, IDC_OPERATIONS_LIST, OnLvnKeydownOperationsList)
    ON_NOTIFY(NM_DBLCLK, IDC_OPERATIONS_LIST, OnNMDblclkOperationsList)
    ON_COMMAND_RANGE(IDM_ENABLED, IDM_NEW_UTP_UPDATE_OP, OnListCtrlContextMenu)
    ON_COMMAND_RANGE(IDM_ENABLED, IDM_NEW_MX_UPDATE_OP, OnListCtrlContextMenu)
//    ON_COMMAND_RANGE(IDM_ENABLED, IDM_NEW_OTP_OP, OnListCtrlContextMenu)
END_MESSAGE_MAP()



#ifdef _DEBUG
void CConfigPlayerProfilePage::Dump(CDumpContext& dc) const
{
	CPropertyPage::Dump(dc);
	dc << "CConfigPlayerProfilePage = " << this;
}
#endif



// CConfigPlayerProfilePage message handlers

BOOL CConfigPlayerProfilePage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_hi_ok = ::LoadIcon(NULL, IDI_INFORMATION);
	m_hi_warning = ::LoadIcon(NULL, IDI_WARNING);
	m_hi_error = ::LoadIcon(NULL, IDI_ERROR);

	Localize();

    // fill combo with profile directories
	if ( m_ProfileList.GetCount() == 0 ) {
		m_operations_ctrl.EnableWindow(FALSE);
	}
    else {
		CString csProfileName;

		InitProfileListCombo();

		if (m_p_player_profile == NULL)
		{
			csProfileName = AfxGetApp()->GetProfileString(_T("Player Profile"), _T("Player Description"));
			if( csProfileName.IsEmpty() )
			{
				m_cb_player_profile_ctrl.GetLBText(0, csProfileName);
			}

			m_p_player_profile = m_ProfileList.Find(csProfileName);
		}
		else
			csProfileName = m_p_player_profile->GetName();

		if( !csProfileName.IsEmpty() )
		{
			m_cb_player_profile_ctrl.SelectString(-1, csProfileName);
        }

        InitListCtrl(m_operations_ctrl);

		m_cs_cur_player_profile = m_cs_old_player_profile = m_p_player_profile->GetName();
    }


    LoadControlsFromProfile(m_p_player_profile);
	OnUpdateStatus(0, 0);

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigPlayerProfilePage::InitProfileListCombo()
{
	int i = 0;

	// Commit the profiles on the active list
	while ( i < m_ProfileList.GetCount() )
	{
		CPlayerProfile *pProfile = m_ProfileList.Get(i++);
		m_cb_player_profile_ctrl.AddString(pProfile->m_cs_name);
	}
}

//
// Validate all changes to the profiles
//
void CConfigPlayerProfilePage::OnOK()
{
	// validate each profile
	for (int i = 0; i < m_ProfileList.GetCount(); ++i )
	{
		CPlayerProfile *pProfile = m_ProfileList.Get(i);
		DWORD attrib = ::GetFileAttributes(pProfile->m_cs_ini_file);
		if ( attrib & FILE_ATTRIBUTE_READONLY ) {
			::SetFileAttributes(pProfile->m_cs_ini_file, attrib & ~FILE_ATTRIBUTE_READONLY);
		}

		pProfile->Validate();
		if ( pProfile->IsValid() )
		{
			SaveProfile(pProfile);
		}
	}

	// if we have no players null the current selection in the registry
	if (m_ProfileList.GetCount() == 0)
	{
	    AfxGetApp()->WriteProfileString(_T("Player Profile"), _T("Player Description"), _T(""));
	}
	else
	{
		// make this the last saved profile in the registry
		AfxGetApp()->WriteProfileString(_T("Player Profile"), _T("Player Description"), m_p_player_profile->GetName());
	}

	CPropertyPage::OnOK();
}

void CConfigPlayerProfilePage::OnCancel()
{
	InitProfileList(); // re-initialize from registry
	CPropertyPage::OnCancel();
}

CPlayerProfile *CConfigPlayerProfilePage::InitProfile(LPCTSTR _name)
{ 
	CPlayerProfile *pProfile = new CPlayerProfile();

	pProfile->Init(_name);

	// tell the Port Dialogs that the profile changed
//    CPortMgrDlg* pDlg;
//	for (int i=0; i < MAX_PORTS; ++i) {
//		if ( ( pDlg = theApp.GetPortDlg(i) ) ) {
//			if(pDlg->GetUsbPort()) {
//				if ( ! pDlg->SendMessage( WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_CONFIG_CHANGE, NULL) )
//					DWORD err = GetLastError();
//			}
//		} // end if ( pDlg )
//	} // end for ( dlgs )

    return pProfile; 
}

//
// Called from DialogBar->ConfigMgr to select the current profile.  No Cfg dialog present.
//
void CConfigPlayerProfilePage::SelectProfile(LPCTSTR _name)
{
	CPlayerProfile *pProfile = m_ProfileList.Find(_name);

	if (pProfile && pProfile != m_p_player_profile)
	{
		m_p_player_profile = pProfile;

		// tell the Port Dialogs that the profile changed
	    CPortMgrDlg* pDlg;
		for (int i=0; i < MAX_PORTS; ++i) {
			if ( ( pDlg = theApp.GetPortDlg(i) ) ) {
				if(pDlg->GetUsbPort()) {
					if ( ! pDlg->SendMessage( WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_CONFIG_CHANGE, NULL) )
						DWORD err = GetLastError();
				}
			} // end if ( pDlg )
		} // end for ( dlgs )
	}
}

CPlayerProfile *CConfigPlayerProfilePage::GetProfile(int _index)
{
	int i = 0;
	CPlayerProfile *pProfile = NULL;

	if (_index < 0)  // default
		pProfile = m_p_player_profile;
	else
		if (_index < m_ProfileList.GetCount())
			pProfile = m_ProfileList.Get(_index);

	return pProfile;
}

LPCTSTR CConfigPlayerProfilePage::GetListProfileName(int _index)
{
	CPlayerProfile *pProfile = GetProfile(_index);
	if (pProfile)
		return pProfile->GetName();
	else
		return NULL;
}

void CConfigPlayerProfilePage::OnCbnSelchangeProductDescCombo()
{
	CString profile_name;
	m_cb_player_profile_ctrl.GetWindowText(profile_name);
	m_cs_cur_player_profile = profile_name;
	m_p_player_profile = m_ProfileList.Find(profile_name);
	LoadControlsFromProfile(m_p_player_profile);
	OnUpdateStatus(0,0);
}

DWORD CConfigPlayerProfilePage::InitProfileList(void)
{
	BOOL fFinished = FALSE;
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	CString csTemp, csProfileRoot, csProfileName;

	GetModuleFileName(NULL, csTemp.GetBuffer(_MAX_PATH), _MAX_PATH);
	_tsplitpath_s( csTemp, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext,_MAX_EXT );
	csTemp.ReleaseBuffer();
	csTemp = _T("Profiles\\");
	csProfileRoot.Format(_T("%s%s%s"), drive, dir, csTemp);

	if (m_p_player_profile)
		csProfileName = m_p_player_profile->GetName();
	else
		csProfileName = AfxGetApp()->GetProfileString(_T("Player Profile"), _T("Player Description"));

	m_ProfileList.RemoveAll();

#ifdef LOCKED_DEFAULT_PROFILE
	SetDefaultProfile();
#else
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFileEx(csProfileRoot + _T("*."),
		FindExInfoStandard, &FindFileData, FindExSearchLimitToDirectories, NULL, 0 );
	if ( hFind != INVALID_HANDLE_VALUE )
	{
		while (!fFinished)
		{ 
			if ( (_tcscmp(FindFileData.cFileName, _T(".")) != 0) &&
				 (_tcscmp(FindFileData.cFileName, _T("..")) != 0) )
			{
				// init the profile
				CPlayerProfile *pProfile = InitProfile(FindFileData.cFileName);
				// add it to the list
				m_ProfileList.Add(pProfile);
			}
			if (!FindNextFile(hFind, &FindFileData))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
				{ 
					fFinished = TRUE; 
				} 
			}
		}
	}
	FindClose(hFind);
#endif

	// reset our currently selected profile
	if (m_ProfileList.GetCount())
	{
		if (!csProfileName.IsEmpty())
			m_p_player_profile = m_ProfileList.Find(csProfileName);

		if (!m_p_player_profile)
			m_p_player_profile = m_ProfileList.Get(0);

//		m_cb_player_profile_ctrl.SelectString(-1, m_p_player_profile->GetName());
	}
	else
		m_p_player_profile = NULL;

	return m_ProfileList.GetCount();
}

void CConfigPlayerProfilePage::SetDefaultProfile()
{
#ifdef LOCKED_DEFAULT_PROFILE
	CPlayerProfile *pDefaultProfile = new CPlayerProfile();
	pDefaultProfile->m_cs_name				= DEFAULT_PROFILE_NAME;	// ex. SigmaTel MSCN Audio Player		
	pDefaultProfile->m_cs_usb_vid			= DEFAULT_PROFILE_VID;	// ex. _T("066F")
	pDefaultProfile->m_cs_usb_pid			= DEFAULT_PROFILE_PID;	// ex. _T("8000")
	pDefaultProfile->m_cs_scsi_mfg			= DEFAULT_PROFILE_SCSI_MFG;	// ex. _T("SigmaTel")
	pDefaultProfile->m_cs_scsi_product		= DEFAULT_PROFILE_SCSI_PROD;		// ex. _T("MSCN")
	pDefaultProfile->m_status				= PROFILE_OK;
	pDefaultProfile->m_error_msg			= _T("");
	pDefaultProfile->m_cs_profile_path		= pDefaultProfile->m_cs_profile_root + pDefaultProfile->m_cs_name;
	pDefaultProfile->m_cs_ini_file			= _T("");
	pDefaultProfile->m_cs_volume_label		= DEFAULT_PROFILE_LABEL;
	pDefaultProfile->m_b_use_volume_label	= DEFAULT_PROFILE_USE_LABEL;
	pDefaultProfile->m_edit_mode			= FALSE;
	pDefaultProfile->m_iSelectedUpdate		= 0;
	pDefaultProfile->m_bNew					= FALSE;
	pDefaultProfile->m_bLockedProfile		= TRUE;

	// generate a default ops list

	// generate a new update operation
	COpInfo* pUpdateOpInfo = new COpInfo(pDefaultProfile, DEFAULT_PROFILE_UPDATE_OP, 0);
	media::LogicalDrive dd;

	pUpdateOpInfo->m_status = OPINFO_OK;
    pUpdateOpInfo->m_drive_array.Clear();
	pUpdateOpInfo->m_FileList.RemoveAll();

	pUpdateOpInfo->m_e_type = COperation::UPDATE_OP;
	pUpdateOpInfo->m_ul_param = DEFAULT_PROFILE_PARAM;
	pUpdateOpInfo->m_b_enabled = TRUE;

	pUpdateOpInfo->m_FileList.AddFile(_T("Updater.sb"), CFileList::RESOURCE_FILE, IDR_DEFAULT_PROFILE_RESID_0);
	pUpdateOpInfo->m_update_boot_fname_list_index = IDR_DEFAULT_PROFILE_RESID_0; // updater is the 1st file/resource

#ifdef DEFAULT_PROFILE_DRV1_FILE
// data drive
	dd.Name				= DEFAULT_PROFILE_DRV1_FILE;
	dd.FileListIndex	= -1;
	dd.Description		= DEFAULT_PROFILE_DRV1_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV1_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV1_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV1_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV1_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

#ifdef DEFAULT_PROFILE_DRV2_FILE
	// Janus drive
	dd.Name				= DEFAULT_PROFILE_DRV2_FILE;
	dd.FileListIndex	= -1;
	dd.Description		= DEFAULT_PROFILE_DRV2_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV2_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV2_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV2_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV2_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

#ifdef DEFAULT_PROFILE_DRV3_FILE
	// settings drive
	dd.Name				= pDefaultProfile->m_cs_profile_path + _T("\\") + DEFAULT_PROFILE_DRV3_FILE;
	dd.FileListIndex	= pUpdateOpInfo->m_FileList.AddFile(dd.Name.c_str(), CFileList::EXISTS_IN_TARGET);
	dd.Description		= DEFAULT_PROFILE_DRV3_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV3_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV3_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV3_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV3_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

#ifdef DEFAULT_PROFILE_DRV4_FILE
	// firmware.sb #1
	dd.Name				= DEFAULT_PROFILE_DRV4_FILE;
	dd.FileListIndex	= pUpdateOpInfo->m_FileList.AddFile(DEFAULT_PROFILE_DRV4_FILE, CFileList::RESOURCE_FILE, IDR_DEFAULT_PROFILE_RESID_1);
	dd.Description		= DEFAULT_PROFILE_DRV4_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV4_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV4_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV4_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV4_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

#ifdef DEFAULT_PROFILE_DRV5_FILE
	// firmware.sb #2
	dd.Name				= DEFAULT_PROFILE_DRV5_FILE;
	dd.FileListIndex	= pUpdateOpInfo->m_FileList.AddFile(DEFAULT_PROFILE_DRV5_FILE, CFileList::RESOURCE_FILE, IDR_DEFAULT_PROFILE_RESID_1);
	dd.Description		= DEFAULT_PROFILE_DRV5_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV5_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV5_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV5_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV5_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

#ifdef DEFAULT_PROFILE_DRV6_FILE
	// firmware.sb #3
	dd.Name				= DEFAULT_PROFILE_DRV6_FILE;
	dd.FileListIndex	= pUpdateOpInfo->m_FileList.AddFile(DEFAULT_PROFILE_DRV6_FILE, CFileList::RESOURCE_FILE, IDR_DEFAULT_PROFILE_RESID_1);
	dd.Description		= DEFAULT_PROFILE_DRV6_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV6_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV6_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV6_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV6_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

#ifdef DEFAULT_PROFILE_DRV7_FILE
	// firmware.rsc #1
	dd.Name				= DEFAULT_PROFILE_DRV7_FILE;
	dd.FileListIndex	= pUpdateOpInfo->m_FileList.AddFile(DEFAULT_PROFILE_DRV7_FILE, CFileList::RESOURCE_FILE, IDR_DEFAULT_PROFILE_RESID_2);
	dd.Description		= DEFAULT_PROFILE_DRV7_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV7_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV7_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV7_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV7_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

#ifdef DEFAULT_PROFILE_DRV8_FILE
	// firmware.rsc #2
	dd.Name				= DEFAULT_PROFILE_DRV8_FILE;
	dd.FileListIndex	= pUpdateOpInfo->m_FileList.AddFile(DEFAULT_PROFILE_DRV8_FILE, CFileList::RESOURCE_FILE, IDR_DEFAULT_PROFILE_RESID_2);
	dd.Description		= DEFAULT_PROFILE_DRV8_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV8_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV8_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV8_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV8_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

#ifdef DEFAULT_PROFILE_DRV9_FILE
	// firmware.rsc #3
	dd.Name				= DEFAULT_PROFILE_DRV9_FILE;
	dd.FileListIndex	= pUpdateOpInfo->m_FileList.AddFile(DEFAULT_PROFILE_DRV9_FILE, CFileList::RESOURCE_FILE, IDR_DEFAULT_PROFILE_RESID_2);
	dd.Description		= DEFAULT_PROFILE_DRV9_DESC;
	dd.Type				= (media::LogicalDriveType)DEFAULT_PROFILE_DRV9_TYPE;
	dd.Tag				= (media::LogicalDriveTag)DEFAULT_PROFILE_DRV9_TAG;
	dd.Flags			= (media::LogicalDriveFlag)DEFAULT_PROFILE_DRV9_FLAGS;
	dd.RequestedKB		= DEFAULT_PROFILE_DRV9_REQKB;
	pUpdateOpInfo->m_drive_array.AddDrive(dd);
#endif

	for( size_t i = 0; i < pUpdateOpInfo->m_drive_array.Size(); ++i)
	{
		media::LogicalDrive drvDescTmp = pUpdateOpInfo->m_drive_array.GetDrive(media::DriveTag_FirmwareImg);
		if( drvDescTmp.FileListIndex != -1 )
		{
			CString csPathName, csVersion;

			csPathName.Format(L"%s", drvDescTmp.Name.c_str());
			StFwComponent tempFwObj(csPathName, StFwComponent::LoadFlag_ResourceOnly);
			if ( tempFwObj.GetLastError() == ERROR_SUCCESS )
			{
				csVersion = tempFwObj.GetProductVersion().toString();
				pUpdateOpInfo->SetProductVersion(csVersion);
			}
			break;
		}
	}


	pDefaultProfile->m_p_op_info_list.AddTail( pUpdateOpInfo );

	// generate a new copy operation
	CString copyOpStr = DEFAULT_PROFILE_COPY_OP;
	if ( !copyOpStr.IsEmpty() )
	{
		COpInfo* pCopyOpInfo = new COpInfo(pDefaultProfile, copyOpStr, 0);

		pCopyOpInfo->m_status = OPINFO_OK;
		pCopyOpInfo->m_drive_array.Clear();
		pCopyOpInfo->m_FileList.RemoveAll();

		pCopyOpInfo->m_e_type = COperation::COPY_OP;
		pCopyOpInfo->m_b_enabled = TRUE;

		if ( pCopyOpInfo->EnumerateFolderFiles(&pCopyOpInfo->m_FileList) != OPINFO_OK )
			return;

		pDefaultProfile->m_p_op_info_list.AddTail( pCopyOpInfo );
	}

	m_ProfileList.Add(pDefaultProfile);
#endif
}

DWORD CConfigPlayerProfilePage::LoadControlsFromProfile(CPlayerProfile * _pProfile)
{
	if( !_pProfile )
		return 0;

	// fill the operations list control
    InsertOpsList(_pProfile);

    return 0;
}

//
// Insert ops into the list control
//
DWORD CConfigPlayerProfilePage::InsertOpsList(CPlayerProfile * _pProfile)
{
	CPlayerProfile::COpInfoList* p_ops_list = _pProfile->GetOpInfoListPtr();
    ASSERT(p_ops_list);

    CString csDetails, csOptions, csToolTip, csText, resStr;
	COpInfo *pOpInfo;
	COLORREF bk_color;

	m_operations_ctrl.DeleteAllItems();
    
    int i, iItem, iSubItem, iState;
	iItem = iSubItem = 0;

	m_operations_ctrl.LockWindowUpdate();	// ***** lock window updates while filling list *****

	POSITION pos = p_ops_list->GetHeadPosition();
	for (iItem = 0; iItem < p_ops_list->GetCount(); iItem++) {  // insert the items and subitems into the list view.
		pOpInfo = p_ops_list->GetNext(pos);

        for (iSubItem = 0; iSubItem <= COL_OP_OPTIONS; iSubItem++) {
            switch ( iSubItem )
            {
            case COL_OP_TYPE:
                // color  
                bk_color = 
                    pOpInfo->GetStatus() == OPINFO_OK ? RGBDEF_VALID : RGBDEF_INCOMPLETE;
                iState = pOpInfo->m_e_type == COperation::INVALID_OP ? 0 : 
                    pOpInfo->m_status ? 3 : pOpInfo->m_b_enabled+1;
                
				// Only allow a single UPDATE_OP to be selected
				if ( pOpInfo->m_e_type == COperation::UPDATE_OP && iState == 2 )
				{
					if( _pProfile->GetSelectedUpdateOp() < 0 ||
						m_p_player_profile->GetSelectedUpdateOp() == iItem )
					{
						_pProfile->SetSelectedUpdateOp(iItem);
					}
					else
					{
						iState = 1;
						pOpInfo->OnEnable(FALSE);
					}
				}

                m_operations_ctrl.InsertItem(iItem, pOpInfo->m_cs_desc, -1, bk_color);
                m_operations_ctrl.SetItemData(iItem, (DWORD_PTR)pOpInfo);
			    m_operations_ctrl.SetItemState(iItem/*, iSubItem*/, INDEXTOSTATEIMAGEMASK(iState), LVIS_STATEIMAGEMASK);
                break;
            case COL_OP_NAME:
                // color 
                bk_color = 
                    pOpInfo->GetStatus() == OPINFO_OK ? RGBDEF_VALID : RGBDEF_INCOMPLETE;
                // tool tip
                if ( pOpInfo->GetProfile()->m_cs_name.IsEmpty() ) //|| pOpInfo->m_cs_ini_section.IsEmpty() )
					csToolTip.LoadString(IDS_TOOLTIP_NO_PATH);
                else
                    csToolTip.Format(IDS_TOOLTIP_PROFILE_LOCATION, pOpInfo->GetProfile()->GetName(), pOpInfo->m_cs_ini_section);
                // set item
				if (pOpInfo->m_cs_new_ini_section.IsEmpty())
	                m_operations_ctrl.SetItemText(iItem, iSubItem, pOpInfo->m_cs_ini_section, -1 , bk_color);
				else
	                m_operations_ctrl.SetItemText(iItem, iSubItem, pOpInfo->m_cs_new_ini_section, -1 , bk_color);
                m_operations_ctrl.SetItemToolTipText(iItem, iSubItem, csToolTip);
                break;
            case COL_OP_DETAIL:
                // color  
                bk_color = 
                    pOpInfo->GetStatus() == OPINFO_OK ? RGBDEF_VALID : RGBDEF_INCOMPLETE;
                // text
                csText.Empty();

                for ( i=0; i<pOpInfo->m_FileList.GetCount(); ++i)
				{
					CFileList::PFILEITEM pItem = pOpInfo->m_FileList.GetAt(i);
					if (pItem->m_action != CFileList::IN_EDIT_DELETE &&
						pItem->m_action != CFileList::DELETE_FROM_TARGET )
					{
	                    csText.Append(pOpInfo->m_FileList.GetFileNameAt(i));
		                if( i+1 < pOpInfo->m_FileList.GetCount() )
   					        csText.Append(_T("; "));
					}
				}

				resStr.LoadString(IDS_TOOLTIP_NO_FILES);
	            m_operations_ctrl.SetItemToolTipText(iItem, iSubItem,
		            pOpInfo->m_FileList.GetCount() ? csText : resStr);
                // set item
                m_operations_ctrl.SetItemText(iItem, iSubItem, csText, -1 , bk_color);
                break;
            case COL_OP_OPTIONS:
                // color 
                bk_color = 
                    pOpInfo->GetStatus() == OPINFO_OK ? RGBDEF_VALID : RGBDEF_INCOMPLETE;
                // text
		        csText.Empty();
				if ( pOpInfo->m_e_type == COperation::OTP_OP)
				{
					csText.Append(pOpInfo->m_csOTPValue);
				}
				else if ( pOpInfo->m_e_type == COperation::UTP_UPDATE_OP ||
					      pOpInfo->m_e_type == COperation::MX_UPDATE_OP)
				{
					csText.Append(pOpInfo->m_UclInstallSection);
				}
				else
				{
	                for ( int i=0; 1<<i<UL_PARAM_NO_MORE_OPTIONS; ++i ) {
				        if ( pOpInfo->m_ul_param & 1<<i ) {
							CString resStr;
							resStr.LoadString(IDS_OPFLAGS_OPTIONSTRINGS+i);
					        if ( !csText.IsEmpty() )
						        csText.AppendFormat(_T("; %s"), resStr);
							else
								csText.Append(resStr);
	                    }
					}
                }
                m_operations_ctrl.SetItemText(iItem, iSubItem, csText, -1 , bk_color);
                break;
            default:
                ASSERT(0);
                break;
            }
        }
    }
	m_operations_ctrl.UnlockWindowUpdate();	// ***** unlock window updates *****

	if( _pProfile->m_bLockedProfile )
		m_operations_ctrl.EnableWindow(FALSE);

	return iItem;
}

///////////////////////////////////////////////////////////////////////////////
// OnLvnItemChangedOperationsList
//
// This method shows how to handle LVN_ITEMCHANGED messages from ListCtrl
//
void CConfigPlayerProfilePage::OnLvnItemChangedOperationsList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int nItem = -1;
	int nSubItem = -1;
	if (pNMLV)
	{
		nItem = pNMLV->iItem;
		nSubItem = pNMLV->iSubItem;
	}
//	TRACE(_T("in CConfigPlayerProfilePage::OnItemChanged:  %d, %d\n"), nItem, nSubItem);

	if (pNMLV && (pNMLV->uNewState == (UINT)(LVIS_FOCUSED|LVIS_SELECTED)))
	{
		TRACE(_T("item has changed:  %d, %d\n"), nItem, nSubItem);
        LPARAM info = m_operations_ctrl.GetItemData(nItem);
        OnUpdateStatus(0, info);
	}
	if (pNMLV && (pNMLV->uNewState == 0))
        OnUpdateStatus(0, 0);

	*pResult = 0;
}



LRESULT CConfigPlayerProfilePage::OnUpdateStatus(WPARAM _wparam, LPARAM _p_op_info)
{
	if ( _p_op_info ) {
		COpInfo *pOpInfo = (COpInfo*)_p_op_info;
		m_status_ctrl.SetWindowText(pOpInfo->GetLastErrorMsg());
		if (pOpInfo->GetStatus())
			m_status_icon_ctrl.SetIcon(m_hi_error);
		else 
			m_status_icon_ctrl.SetIcon(m_hi_ok);
	}
	else {
		// make the status icon and text display the status of the 
		// <profile> instead of for an <operation>
		if ( m_p_player_profile )
		{
			m_status_ctrl.SetWindowText(m_p_player_profile->m_error_msg);
			switch(m_p_player_profile->m_status) {
				case PROFILE_OK:
					m_status_icon_ctrl.SetIcon(m_hi_ok);
					break;
				case PROFILE_WARNING:
					m_status_icon_ctrl.SetIcon(m_hi_warning);
					break;
				case PROFILE_ERROR:
					m_status_icon_ctrl.SetIcon(m_hi_error);
					break;
			}
		}
	}
	return 0;
}

DWORD CConfigPlayerProfilePage::SaveProfile(CPlayerProfile *_pProfile)
{
    CString cs_temp, cs_temp_path;

	if( _pProfile->m_bLockedProfile )
		return 1;

	cs_temp.Format(L"%d", MIN_PROFILE_VERSION);
	_pProfile->SetIniField( CPlayerProfile::VERSION, cs_temp.GetBuffer());

	cs_temp = _pProfile->GetName();
    _pProfile->SetIniField( CPlayerProfile::PLAYER, cs_temp.GetBuffer());

	// save all operations for this profile
	COpInfo* pOpInfo;
	POSITION pos = _pProfile->m_p_op_info_list.GetHeadPosition();
    while (pos)
	{
		pOpInfo = _pProfile->m_p_op_info_list.GetNext(pos);
		if (pOpInfo)
		{
			// fix up the player.ini file
			pOpInfo->m_cs_cmd_line.Format(_T("%s=%s,%d,%d"), pOpInfo->m_cs_desc, pOpInfo->m_cs_ini_section, pOpInfo->m_timeout, pOpInfo->m_b_enabled);
			pOpInfo->ReplaceIniLine(_T("OPERATIONS"), pOpInfo->m_cs_cmd_line, pOpInfo->m_index);
			pOpInfo->WriteIniSection(pOpInfo->m_cs_ini_section);
		}
	}

	return 1;
}


int CConfigPlayerProfilePage::CheckPath(CString sPath)
{
	DWORD dwAttr = GetFileAttributes(sPath);
	if (dwAttr == 0xffffffff) 
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) 
			return PATH_NOT_FOUND;
		return PATH_ERROR;
	}

	if (dwAttr & FILE_ATTRIBUTE_DIRECTORY) 
		return PATH_IS_FOLDER;
	
	return PATH_IS_FILE;
}

void CConfigPlayerProfilePage::InitListCtrl(CConfigPlayerListCtrl& list)
{
	CRect rect;
	// insert 3 columns (REPORT mode)
	list.GetClientRect(&rect);
//	int scrollbar_width = GetSystemMetrics(SM_CXVSCROLL) + 4;
//	rect.right -= scrollbar_width;
	if (list.GetHeaderCtrl()->GetItemCount() == 0) {
		CString resStr;
		resStr.LoadString(IDS_CFG_PROFILE_COL_OP);
		list.InsertColumn(COL_OP_TYPE, resStr, LVCFMT_LEFT, rect.Width() * 2/12, 0);
		resStr.LoadString(IDS_CFG_PROFILE_COL_FOLDER);
		list.InsertColumn(COL_OP_NAME, resStr, LVCFMT_LEFT, rect.Width() * 3/12, 1);
		resStr.LoadString(IDS_CFG_PROFILE_COL_FILES);
		list.InsertColumn(COL_OP_DETAIL, resStr, LVCFMT_LEFT, rect.Width() * 5/12, 2);
		resStr.LoadString(IDS_CFG_PROFILE_COL_OPTIONS);
		list.InsertColumn(COL_OP_OPTIONS, resStr, LVCFMT_LEFT, rect.Width() * 2/12, 3);
	}
	// fix up some settings on the operations list control
    long lStyleOld = GetWindowLong(list.GetSafeHwnd(), GWL_STYLE);
	lStyleOld &= ~(LVS_TYPEMASK);  // turn off all the style (view mode) bits
	lStyleOld |= LVS_REPORT;        // Set the new style for the control
	SetWindowLong(list.GetSafeHwnd(), GWL_STYLE, lStyleOld);
	list.SetExtendedStyle(LVS_EX_CHECKBOXES  | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP
		| LVS_EX_GRIDLINES | LVS_EX_ONECLICKACTIVATE | LVS_EX_LABELTIP );
	// create the image list for the check boxes in the list control
	list.SetImageList( &list.m_il_state, TVSIL_STATE );
	list.EnableToolTips();
}


void CConfigPlayerProfilePage::OnNMKillfocusOperationsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnUpdateStatus(0,0);
    *pResult = 0;
}

void CConfigPlayerProfilePage::OnNMSetfocusOperationsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	POSITION pos = m_operations_ctrl.GetFirstSelectedItemPosition();
	int item = -1;
	if (pos != NULL)
		item = m_operations_ctrl.GetNextSelectedItem(pos);

    if ( item != -1 ) {
        LPARAM pOpInfo = m_operations_ctrl.GetItemData(item);
        OnUpdateStatus(0,pOpInfo);
    }
    *pResult = 0;
}

void CConfigPlayerProfilePage::OnNMClickOperationsList(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    LVHITTESTINFO lvhti;
    lvhti.pt = pNMIA->ptAction;
	*pResult = 0;

    m_operations_ctrl.SubItemHitTest(&lvhti);
    if ( lvhti.iItem == -1 )
        return;
	
    TRACE(_T("in CConfigPlayerProfilePage::OnNMClickOperationsList:  %d, %d\n"), lvhti.iItem, lvhti.iSubItem);
	
    if (lvhti.flags & LVHT_ONITEMSTATEICON ){
	    m_operations_ctrl.SetItemState( lvhti.iItem, LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
		OpWorkerEnable();
		*pResult = 1;
	}

//    if (lvhti.flags & LVHT_ONITEMLABEL  && 
//        lvhti.iItem == m_operations_ctrl.GetItemCount()-1 ){
//            OpWorkerNew();
//    }
}

void CConfigPlayerProfilePage::OnLvnKeydownOperationsList(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
    *pResult = 0;

    switch ( pLVKeyDow->wVKey )
	{
	case VK_SPACE:
		OpWorkerEnable();
		*pResult = 1; // disable default handler gong through all 3 states of enable bitmap
		break;
	default:
		break;
	}
}

DWORD CConfigPlayerProfilePage::OpWorkerEnable(void)
{
    POSITION pos = m_operations_ctrl.GetFirstSelectedItemPosition();
    if (pos == NULL)
        return -1;
	int index = m_operations_ctrl.GetNextSelectedItem(pos);
	COpInfo * pInfo = (COpInfo*)m_operations_ctrl.GetItemData(index);
	if ( index == m_operations_ctrl.GetItemCount() )
        return -1;
	
	int checked = m_operations_ctrl.GetItemState( index, TVIS_STATEIMAGEMASK )>>12;
	switch (checked) {
		case 1: // unchecked -> checked
			if (pInfo->OnEnable(TRUE) != OPINFO_OK) {
				m_operations_ctrl.SetItemState(index, INDEXTOSTATEIMAGEMASK(3), LVIS_STATEIMAGEMASK);
			}
			else {
                // 2 for checked
				m_operations_ctrl.SetItemState(index, INDEXTOSTATEIMAGEMASK(pInfo->IsEnabled()+1), LVIS_STATEIMAGEMASK);
			}
			// disable any selected UPDATE_OP
			if( pInfo->m_e_type == COperation::UPDATE_OP )
			{
				if( m_p_player_profile->GetSelectedUpdateOp() >= 0 )
				{
					COpInfo *pOpInfoUpd = (COpInfo*)m_operations_ctrl.GetItemData(m_p_player_profile->GetSelectedUpdateOp());
					pOpInfoUpd->OnEnable(FALSE);
					m_operations_ctrl.SetItemState(m_p_player_profile->GetSelectedUpdateOp(),
							INDEXTOSTATEIMAGEMASK(pOpInfoUpd->IsEnabled()+1), LVIS_STATEIMAGEMASK);
				}
				m_p_player_profile->SetSelectedUpdateOp(index);				
			}
			break;

		case 2: // checked -> unchecked // disabled (unchecked)
			if (pInfo->OnEnable(FALSE) != OPINFO_OK) {
				m_operations_ctrl.SetItemState(index, INDEXTOSTATEIMAGEMASK(3), LVIS_STATEIMAGEMASK);
			}
			else {
				// 1 for unchecked
                m_operations_ctrl.SetItemState(index, INDEXTOSTATEIMAGEMASK(pInfo->IsEnabled()+1), LVIS_STATEIMAGEMASK);
			}
			m_p_player_profile->SetSelectedUpdateOp(-1);
			break;

		case 3: // was disabled, trying to enable, message to user?
			m_operations_ctrl.SetItemState(index, INDEXTOSTATEIMAGEMASK(3), LVIS_STATEIMAGEMASK);
			break;

		default:
            ASSERT(0);
			m_operations_ctrl.SetItemState(index, INDEXTOSTATEIMAGEMASK(3), LVIS_STATEIMAGEMASK);
			break;
	} // end switch ( checked )
	
	return checked;
}

DWORD CConfigPlayerProfilePage::OpWorkerEdit()
{
	INT_PTR ret;
    POSITION pos = m_operations_ctrl.GetFirstSelectedItemPosition();
    if (pos == NULL)
        return -1;
	int index = m_operations_ctrl.GetNextSelectedItem(pos);
	COpInfo * pOpInfo = (COpInfo*)m_operations_ctrl.GetItemData(index);
	
	pOpInfo->m_cs_NewName.Empty();
//	pOpInfo->m_cs_new_ini_section.Empty();

	if (pOpInfo->GetType() == COperation::COPY_OP)
	{
		CCopyOpDlg *pOpEditor = new CCopyOpDlg(this, pOpInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
	else if (pOpInfo->GetType() == COperation::LOADER_OP)
	{
		CLoadFileOpDlg *pOpEditor = new CLoadFileOpDlg(this, pOpInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
	else if (pOpInfo->GetType() == COperation::UPDATE_OP)
	{
		COpUpdateDlg *pOpEditor = new COpUpdateDlg(this, pOpInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
	else if (pOpInfo->GetType() == COperation::OTP_OP)
	{
		COpOTPDlg *pOpEditor = new COpOTPDlg(this, pOpInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
	else if (pOpInfo->GetType() == COperation::UTP_UPDATE_OP || pOpInfo->GetType() == COperation::MX_UPDATE_OP)
	{
		COpUclDlg *pOpEditor = new COpUclDlg(this, pOpInfo); //new COpUtpUpdateDlg(this, pOpInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
//	else if (pOpInfo->GetType() == COperation::MX_UPDATE_OP)
//	{
//		COpMxRomUpdateDlg *pOpEditor = new COpMxRomUpdateDlg(this, pOpInfo);
//	    ret = pOpEditor->DoModal();
//	    delete pOpEditor;
//	}

//    pInfo->Validate();
    InsertOpsList(m_p_player_profile);
    m_p_player_profile->Validate();

	return m_operations_ctrl.GetItemCount();
}

DWORD CConfigPlayerProfilePage::OpWorkerMove(INT_PTR _up)
{
	BOOL bCanMoveUp, bCanMoveDown, bCanEdit;
    POSITION new_pos, old_pos = m_operations_ctrl.GetFirstSelectedItemPosition();
    if (old_pos == NULL)
        return -1;
	int index = m_operations_ctrl.GetNextSelectedItem(old_pos);
	COpInfo * pOpInfo = (COpInfo*)m_operations_ctrl.GetItemData(index);
    bCanMoveDown = index < ( m_operations_ctrl.GetItemCount() );
	bCanEdit = index != m_operations_ctrl.GetItemCount();
	bCanMoveUp = index > 0 && bCanEdit;

    // copy ops list to temp list and empty ops list
    CPlayerProfile::COpInfoList temp_list, *p_ops_list = m_p_player_profile->GetOpInfoListPtr();
    temp_list.AddTail(p_ops_list);
    p_ops_list->RemoveAll();

    // read OPERATIONS section of ini file into an array
/*
    CString cs_buffer;
    CStringArray cs_old_arr, cs_new_arr;
    GetPrivateProfileSection(_T("OPERATIONS"), cs_buffer.GetBufferSetLength(1024), 1024, m_p_player_profile->m_cs_ini_file);
	int ini_pos = 0;
    while (cs_buffer.GetAt(ini_pos) != _T('\0')) {
		cs_old_arr.Add( cs_buffer.Tokenize(_T("\r\n"), ini_pos) );
	}
*/
    // move operation up 1
    if ( _up ) {
        ASSERT(bCanMoveUp);
        old_pos = temp_list.GetHeadPosition();
        while (old_pos) {
            pOpInfo = temp_list.GetNext(old_pos);
            if ( pOpInfo->GetIndex() != index ) {
                new_pos = p_ops_list->AddTail(pOpInfo);
//                cs_new_arr.Add( cs_old_arr[0] );
//                cs_old_arr.RemoveAt(0);
            }
            else {
                p_ops_list->InsertBefore(new_pos, pOpInfo);
//                cs_new_arr.InsertAt( index-1, cs_old_arr[0] );
//                cs_old_arr.RemoveAt(0);
            }
        }
    }
    // move operation down 1
    else {
        ASSERT(bCanMoveDown);
        old_pos = temp_list.GetTailPosition();
        while (old_pos) {
            pOpInfo = temp_list.GetPrev(old_pos);
            if ( pOpInfo->GetIndex() != index ) {
                new_pos = p_ops_list->AddHead(pOpInfo);
//                cs_new_arr.InsertAt( 0, cs_old_arr[cs_old_arr.GetCount()] );
//                cs_old_arr.RemoveAt(cs_old_arr.GetCount());
            }
            else {
                p_ops_list->InsertAfter(new_pos, pOpInfo);
//                cs_new_arr.InsertAt( 1, cs_old_arr[cs_old_arr.GetCount()] );
//                cs_old_arr.RemoveAt(cs_old_arr.GetCount());
            }
        }
    }
    // reset the opinfo m_index members
    index = 0;
    new_pos = p_ops_list->GetHeadPosition();
    while (new_pos)
	{
        pOpInfo = p_ops_list->GetNext(new_pos);
        pOpInfo->SetIndex(index);
		if ( pOpInfo->m_e_type == COperation::UPDATE_OP && pOpInfo->IsEnabled() )
		{
			m_p_player_profile->SetSelectedUpdateOp(index);
		}

        ++index;
    }
    
    // format and write ini file
/*
    cs_buffer.ReleaseBuffer();
    cs_buffer.Empty();
    for ( int i=0; i<cs_new_arr.GetCount(); ++i ) {
        cs_buffer.Append(cs_new_arr.GetAt(i));
        cs_buffer.AppendChar(_T('\0'));
    }
	WritePrivateProfileSection(_T("OPERATIONS"), cs_buffer, m_p_player_profile->m_cs_ini_file);
*/
    // update the GUI
    InsertOpsList(m_p_player_profile);

	return m_operations_ctrl.GetItemCount();
}

void CConfigPlayerProfilePage::OnNMDblclkOperationsList(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    LVHITTESTINFO lvhti;
    lvhti.pt = pNMIA->ptAction;
	
    m_operations_ctrl.SubItemHitTest(&lvhti);
    if ( lvhti.iItem == -1 ) {
        *pResult = 0;
        return;
    }
	
    TRACE(_T("in CConfigPlayerProfilePage::OnNMDblclkOperationsList:  %d, %d\n"), lvhti.iItem, lvhti.iSubItem);

    *pResult = 0;
}

void CConfigPlayerProfilePage::OnListCtrlContextMenu(UINT nID)
{
    TRACE(_T("in CConfigPlayerProfilePage::OnListCtrlContextMenu: %d\n"), nID);
	switch ( nID )
	{
	case IDM_ENABLED:
		OpWorkerEnable();
		break;
	case IDM_ST_EDIT:
        OpWorkerEdit();
		break;
	case IDM_ST_MOVE_UP:
        OpWorkerMove(TRUE);
		break;
	case IDM_ST_MOVE_DOWN:
		OpWorkerMove(FALSE);
        break;
	default:
		ASSERT(0);
	}
}

void CConfigPlayerProfilePage::Localize()
{
	CString resStr;

	resStr.LoadString(IDS_CFG_PROFILE_TITLE);
	SetWindowText(resStr);

	resStr.LoadString(IDS_CFG_PROFILE_LABEL);
	SetDlgItemText(IDC_VOLUME_LABEL_CHECK, resStr );

	resStr.LoadString(IDS_NEW);
	SetDlgItemText(IDC_NEW, resStr );

	resStr.LoadString(IDS_EDIT);
	SetDlgItemText(IDC_EDIT, resStr );

	resStr.LoadString(IDS_DELETE);
	SetDlgItemText(IDC_DELETE, resStr );

	resStr.LoadString(IDS_CFG_PROFILE_DESC);
	SetDlgItemText(IDC_PLAYER_PROFILE_TEXT, resStr );

	resStr.LoadString(IDS_CFG_PROFILE_USB_VENDOR);
	SetDlgItemText(IDC_USB_VID_DESC_TEXT, resStr );

	resStr.LoadString(IDS_CFG_PROFILE_USB_PRODUCT);
	SetDlgItemText(IDC_USB_PID_DESC_TEXT, resStr );

	resStr.LoadString(IDS_CFG_PROFILE_SCSI_VENDOR);
	SetDlgItemText(IDC_SCSI_MFG_DESC_TEXT, resStr );

	resStr.LoadString(IDS_CFG_PROFILE_SCSI_PRODUCT);
	SetDlgItemText(IDC_SCSI_PRODUCT_DESC_TEXT, resStr );

	resStr.LoadString(IDS_CFG_PROFILE_OPERATIONS);
	SetDlgItemText(IDC_OPERATION_DESC_TEXT, resStr );

	resStr.LoadString(IDS_CFG_PROFILE_GRP);
	SetDlgItemText(IDC_PROFILE_GRP, resStr );
}
