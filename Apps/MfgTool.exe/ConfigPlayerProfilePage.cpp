// ConfigPlayerProfilePage.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigPlayerProfilePage.h"
#include "PlayerProfile.h"
#include "OpUpdateDlg.h"
#include "CopyOpDlg.h"
#include "LoadFileOpDlg.h"
#include "OpOTPDlg.h"
#include "OpUtpUpdateDlg.h"
#include "OpMxRomUpdateDlg.h"
#include "ConfigUSBPortPage.h"
#include "StMfgTool.h"
#include "DefaultProfile.h"
#include "resource.h"
#include "../../Common/updater_res.h"

// CConfigPlayerProfilePage dialog
IMPLEMENT_DYNAMIC(CConfigPlayerProfilePage, CPropertyPage)


CConfigPlayerProfilePage::CConfigPlayerProfilePage(CWnd * cParent /*=NULL*/)
	: CPropertyPage(CConfigPlayerProfilePage::IDD)
	, m_b_use_volume_label(FALSE)
	, m_cs_old_player_profile(_T(""))
	, m_pNewPlayerProfile(NULL)
	, m_hi_ok(0)
	, m_hi_warning(0)
	, m_hi_error(0)
	, m_bNewProfileMode(FALSE)
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

	if ( m_DeleteList.GetCount() )
	{
		m_DeleteList.RemoveAll();
	}
}

void CConfigPlayerProfilePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PLAYER_PROFILE_COMBO, m_cb_player_profile_ctrl);
	DDX_Control(pDX, IDC_PLAYER_PROFILE_EDIT, m_vve_player_profile_ctrl);
	DDX_Control(pDX, IDC_USB_VID_TEXT, m_usb_vid_ctrl);
	DDX_Control(pDX, IDC_USB_PID_TEXT, m_usb_pid_ctrl);
	DDX_Control(pDX, IDC_SCSI_MFG_TEXT, m_scsi_mfg_ctrl);
	DDX_Control(pDX, IDC_VOLUME_LABEL_TEXT, m_volume_label_ctrl);
	DDX_Control(pDX, IDC_SCSI_PRODUCT_TEXT, m_scsi_product_ctrl);
	DDX_Control(pDX, IDC_OPERATIONS_LIST, m_operations_ctrl);
	DDX_Control(pDX, IDC_STATUS_ICON, m_status_icon_ctrl);
	DDX_Control(pDX, IDC_STATUS_TEXT, m_status_ctrl);
	DDX_Control(pDX, IDC_NEW, m_new_ctrl);
	DDX_Control(pDX, IDC_DELETE, m_delete_ctrl);
	DDX_Control(pDX, IDC_VOLUME_LABEL_CHECK, m_volume_label_check_ctrl);
}


BEGIN_MESSAGE_MAP(CConfigPlayerProfilePage, CPropertyPage)
	ON_BN_CLICKED(IDC_NEW, OnBnClickedNewSave)
	ON_BN_CLICKED(IDC_DELETE, OnBnClickedDelete)
	ON_MESSAGE(WM_UPDATE_STATUS, OnUpdateStatus)
	ON_CBN_SELCHANGE(IDC_PLAYER_PROFILE_COMBO, OnCbnSelchangeProductDescCombo)
	ON_BN_CLICKED(IDC_VOLUME_LABEL_CHECK, OnBnClickedVolumeLabelCheck)
	ON_MESSAGE(UWM_VVE_VALIDITY_CHANGED, OnValidChanged)
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
///////////////////////////////////////////////////
	m_vve_player_profile_ctrl.SetValidCharSet(CVisValidEdit::SET_DIRPATH);
	m_vve_player_profile_ctrl.SetTextLenMinMax(1,128);
	m_vve_player_profile_ctrl.SetEnableLengthCheck();
	m_vve_player_profile_ctrl.SetMyControlID(0x01);

	m_usb_vid_ctrl.SetValidCharSet(CVisValidEdit::SET_HEXADECIMAL);
	m_usb_vid_ctrl.SetTextLenMinMax(4, 4);
	m_usb_vid_ctrl.SetEnableLengthCheck();
	m_usb_vid_ctrl.SetMyControlID(0x02);
	m_usb_vid_ctrl.SetReadOnly(FALSE);

	m_usb_pid_ctrl.SetValidCharSet(CVisValidEdit::SET_HEXADECIMAL);
	m_usb_pid_ctrl.SetTextLenMinMax(4, 4);
	m_usb_pid_ctrl.SetEnableLengthCheck();
	m_usb_pid_ctrl.SetMyControlID(0x04);
	m_usb_pid_ctrl.SetReadOnly(FALSE);

	m_scsi_mfg_ctrl.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_scsi_mfg_ctrl.SetTextLenMinMax(1, 8);
	m_scsi_mfg_ctrl.SetEnableLengthCheck();
	m_scsi_mfg_ctrl.SetMyControlID(0x08);
	m_scsi_mfg_ctrl.SetReadOnly(FALSE);

	m_scsi_product_ctrl.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_scsi_product_ctrl.SetTextLenMinMax(1, 16);
	m_scsi_product_ctrl.SetEnableLengthCheck();
	m_scsi_product_ctrl.SetMyControlID(0x10);
	m_scsi_product_ctrl.SetReadOnly(FALSE);

	m_volume_label_ctrl.SetValidCharSet(CVisValidEdit::SET_DIRPATH);
	m_volume_label_ctrl.SetTextLenMinMax(0, 11);
	m_volume_label_ctrl.SetEnableLengthCheck();
//	m_volume_label_ctrl.SetEmptyValid();
	m_volume_label_ctrl.SetMyControlID(0x20);
	m_volume_label_ctrl.SetReadOnly(FALSE);

///////////////////////////////////////////////////
	// BEGIN REAL WORK

    // fill combo with profile directories
	if ( m_ProfileList.GetCount() == 0 ) {
        m_delete_ctrl.EnableWindow(FALSE);
		m_operations_ctrl.EnableWindow(FALSE);
		m_vve_player_profile_ctrl.EnableWindow(FALSE);
		m_usb_vid_ctrl.EnableWindow(FALSE);
		m_usb_pid_ctrl.EnableWindow(FALSE);
		m_scsi_mfg_ctrl.EnableWindow(FALSE);
		m_scsi_product_ctrl.EnableWindow(FALSE);
		m_volume_label_check_ctrl.EnableWindow(FALSE);
		m_volume_label_ctrl.EnableWindow(FALSE);
	}
    else {
		CString csProfileName;
        m_delete_ctrl.EnableWindow(TRUE);

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

		m_vve_player_profile_ctrl.SetWindowText(m_cs_cur_player_profile);
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

	// save current profile changes
	if (!SaveProfileStrings(m_p_player_profile))
	{
		if ( m_pNewPlayerProfile )
		{
			delete m_pNewPlayerProfile;
			m_pNewPlayerProfile = NULL;
		}
		m_bNewProfileMode = FALSE;
		return;
	}

	// validate each profile
	for (int i = 0; i < m_ProfileList.GetCount(); ++i )
	{
		CPlayerProfile *pProfile = m_ProfileList.Get(i);
		DWORD attrib = ::GetFileAttributes(pProfile->m_cs_ini_file);
		if ( attrib & FILE_ATTRIBUTE_READONLY ) {
			::SetFileAttributes(pProfile->m_cs_ini_file, attrib & ~FILE_ATTRIBUTE_READONLY);
		}

		pProfile->Validate();
		if ( !pProfile->IsValid() )
		{
			// select the offending profile
			m_cb_player_profile_ctrl.SelectString(-1, pProfile->m_cs_name);
			OnUpdateStatus(0,0);
			return;
		}
	}

	// Delete any profiles on the delete list
	while ( m_DeleteList.GetCount() )
	{
	    SHFILEOPSTRUCT FileOp;
	    CString csDirToDelete;

		CPlayerProfile *pProfile = m_DeleteList.Get(0);
		m_DeleteList.Remove(0);

		csDirToDelete = pProfile->GetProfilePath();
		delete pProfile;
	    csDirToDelete.AppendChar(_T('\0'));
	    FileOp.hwnd = this->m_hWnd;
	    FileOp.wFunc = FO_DELETE;
	    FileOp.pFrom = csDirToDelete;
	    FileOp.pTo = NULL;
	    FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_SILENT;
	    int err;
    		
	    // delete the profiles\<player_profile> directory
	    err = SHFileOperation( &FileOp );
	}

	// if we have no players null the current selection in the registry
	if (m_ProfileList.GetCount() == 0)
	    AfxGetApp()->WriteProfileString(_T("Player Profile"), _T("Player Description"), _T(""));

	// Commit the profiles on the active list
	int i = 0;
	while ( i < m_ProfileList.GetCount() )
	{
		CPlayerProfile *pProfile = m_ProfileList.Get(i++);
		if (pProfile)
			SaveProfile(pProfile);
	}

	CPropertyPage::OnOK();

}

void CConfigPlayerProfilePage::OnCancel()
{
	if (m_bNewProfileMode && m_pNewPlayerProfile)
	{
		delete m_pNewPlayerProfile;
		m_pNewPlayerProfile = NULL;
		m_bNewProfileMode = FALSE;
	}

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

void CConfigPlayerProfilePage::OnBnClickedNewSave()
{
	CString resStr;

	if ( m_ProfileList.GetCount() == 0 )
	{
        m_delete_ctrl.EnableWindow(FALSE);
		m_operations_ctrl.EnableWindow(TRUE);
		m_vve_player_profile_ctrl.EnableWindow(TRUE);
		m_usb_vid_ctrl.EnableWindow(TRUE);
		m_usb_pid_ctrl.EnableWindow(TRUE);
		m_scsi_mfg_ctrl.EnableWindow(TRUE);
		m_scsi_product_ctrl.EnableWindow(TRUE);
		m_volume_label_check_ctrl.EnableWindow(TRUE);
		m_volume_label_ctrl.EnableWindow(TRUE);
	}

	if( !m_bNewProfileMode )
	{
		// create a new profile
		// hide combo box and show profile name edit control
		// clear other edit fields and ops list
		// enable Delete button if not already enabled
		// tag the profile as new, and add to profile list
		// remove all ops from the list and disable it
		m_bNewProfileMode = TRUE;

		m_operations_ctrl.RemoveAllGroups();
		m_operations_ctrl.EnableWindow(FALSE);
		((CPropertySheet*)GetParent())->GetDlgItem(IDOK)->EnableWindow(FALSE);

		m_pNewPlayerProfile = new CPlayerProfile();
		m_pNewPlayerProfile->Init(NULL);

		resStr.LoadString(IDS_SAVE);
		m_new_ctrl.SetWindowText( resStr );

//		m_vve_player_profile_ctrl.SetWindowText(m_pNewPlayerProfile->m_cs_name);

		m_cb_player_profile_ctrl.AddString(L" ");
		m_cb_player_profile_ctrl.SelectString(0, L" ");
		m_cb_player_profile_ctrl.EnableWindow(FALSE);

		LoadControlsFromProfile(m_pNewPlayerProfile);
	}
	else
	{
		// save a new profile
		// the profile was tagged as new when created with no name
		// validate the profile
		// add to profile list
		// enable the combobox list and hide the name edit ctrl
		// select the new profile

		if (!SaveProfileStrings(m_pNewPlayerProfile))
			return;

		m_pNewPlayerProfile->Validate();
		if( m_vve_player_profile_ctrl.IsValid() )
		{
			m_ProfileList.Add(m_pNewPlayerProfile);

			m_cb_player_profile_ctrl.EnableWindow(TRUE);
			m_cb_player_profile_ctrl.DeleteString(m_cb_player_profile_ctrl.SelectString(0, L" "));
			m_cb_player_profile_ctrl.AddString(m_pNewPlayerProfile->m_cs_name);

			m_cs_cur_player_profile = m_pNewPlayerProfile->m_cs_name;
			m_p_player_profile = m_pNewPlayerProfile;
			m_cb_player_profile_ctrl.SelectString(0, m_pNewPlayerProfile->m_cs_name);
			OnCbnSelchangeProductDescCombo();

			resStr.LoadString(IDS_NEW);
			m_new_ctrl.SetWindowText( resStr );

			m_operations_ctrl.EnableWindow(TRUE);
			if ( m_ProfileList.GetCount() == 1 ) // first added?
			{
		        InitListCtrl(m_operations_ctrl);
			}

			m_bNewProfileMode = FALSE;
			((CPropertySheet*)GetParent())->GetDlgItem(IDOK)->EnableWindow(TRUE);
		}
		else
		{
			OnUpdateStatus(0,0);
		}
	}
}



void CConfigPlayerProfilePage::OnBnClickedDelete()
{
	// delete a profile
	CString resStr, resTitleStr;
    resStr.Format(IDS_DELETE_CONFIRM_MSG, m_p_player_profile->m_cs_name);
	resTitleStr.LoadString(IDS_DELETE_CONFIRM_TITLE);
	int iCurrentIndex = m_cb_player_profile_ctrl.GetCurSel();

	if ( IDCANCEL == MessageBox(resStr, resTitleStr, MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1) )
	{
		return;
	}
    else
	{
		if ( m_bNewProfileMode )
		{	// abandon new profile
			delete m_pNewPlayerProfile;
			m_pNewPlayerProfile = NULL;

			resStr.LoadString(IDS_NEW);
			m_new_ctrl.SetWindowText( resStr );

			m_cb_player_profile_ctrl.SetCurSel(0);
			OnCbnSelchangeProductDescCombo();

			m_bNewProfileMode = FALSE;

			return;
		}

		// take the profile out of the active list and put it in the delete list
		CPlayerProfile *pProfile = m_ProfileList.Get(iCurrentIndex);

		m_ProfileList.Remove(iCurrentIndex);

		m_DeleteList.Add(pProfile);
	}

	m_cb_player_profile_ctrl.DeleteString(iCurrentIndex);
	
	if ( m_cb_player_profile_ctrl.GetCount() == 0 )
	{
        m_delete_ctrl.EnableWindow(FALSE);

		// no profiles left to show, so clear all the controls
	    // make this the last saved profile in the registry
		m_cs_cur_player_profile.Empty();
        OnCbnSelchangeProductDescCombo();
		LoadControlsFromProfile(m_p_player_profile);
		OnUpdateStatus(0,0);
    }
    else
	{
        m_delete_ctrl.EnableWindow(TRUE);

		if ( iCurrentIndex > 0 )
			m_cb_player_profile_ctrl.SetCurSel( iCurrentIndex -1 );
		else
			m_cb_player_profile_ctrl.SetCurSel( 0 );

		OnCbnSelchangeProductDescCombo();
		LoadControlsFromProfile(m_p_player_profile);
		OnUpdateStatus(0,0);
    }
}

void CConfigPlayerProfilePage::OnCbnSelchangeProductDescCombo()
{
	// save any changes to current profile
	if (!SaveProfileStrings(m_p_player_profile))
	{
		m_cb_player_profile_ctrl.SelectString(-1, m_cs_cur_player_profile);
	}
	else
	{
		CString profile_name;
		m_cb_player_profile_ctrl.GetWindowText(profile_name);
		m_cs_cur_player_profile = profile_name;
		m_p_player_profile = m_ProfileList.Find(profile_name);
		LoadControlsFromProfile(m_p_player_profile);
		OnUpdateStatus(0,0);
	}
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
//	pDefaultProfile->m_cs_original_name		= _T("");
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

	if( _pProfile->m_bLockedProfile )
	{
		m_usb_vid_ctrl.EnableWindow(FALSE);
		m_usb_pid_ctrl.EnableWindow(FALSE);
		m_scsi_mfg_ctrl.EnableWindow(FALSE);
		m_scsi_product_ctrl.EnableWindow(FALSE);
		m_volume_label_ctrl.EnableWindow(FALSE);
		m_volume_label_check_ctrl.EnableWindow(FALSE);
        m_delete_ctrl.EnableWindow(FALSE);
		m_new_ctrl.EnableWindow(FALSE);
		m_vve_player_profile_ctrl.EnableWindow(FALSE);
	}

	m_vve_player_profile_ctrl.SetWindowText(_pProfile->GetName());
	m_usb_vid_ctrl.SetWindowText(_pProfile->m_cs_usb_vid);
	m_usb_pid_ctrl.SetWindowText(_pProfile->m_cs_usb_pid);
	m_scsi_mfg_ctrl.SetWindowText(_pProfile->m_cs_scsi_mfg);
	m_scsi_product_ctrl.SetWindowText(_pProfile->m_cs_scsi_product);
	m_volume_label_ctrl.SetWindowText(_pProfile->m_cs_volume_label);
	if ( ( _pProfile->m_cs_volume_label.IsEmpty() || _pProfile->m_cs_volume_label.GetLength() > 11 ) )
	{
		m_volume_label_check_ctrl.SetCheck(_pProfile->m_b_use_volume_label);
		m_volume_label_check_ctrl.EnableWindow(FALSE);
        m_volume_label_ctrl.EnableWindow(FALSE);
	}
    else {
        m_volume_label_ctrl.EnableWindow(TRUE);
		m_volume_label_check_ctrl.EnableWindow(!_pProfile->m_cs_volume_label.IsEmpty());
		m_volume_label_check_ctrl.SetCheck(_pProfile->m_b_use_volume_label);
	}
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

BOOL CConfigPlayerProfilePage::SaveProfileStrings(CPlayerProfile *_pProfile)
{
	CString cs_temp;
	BOOL b_temp;

	if (!_pProfile || (m_p_player_profile && m_p_player_profile->m_bLockedProfile) )
		return FALSE;

	m_usb_vid_ctrl.GetWindowText(cs_temp);
	_pProfile->SetUsbVid (cs_temp);

	m_usb_pid_ctrl.GetWindowText(cs_temp);
	_pProfile->SetUsbPid (cs_temp);

	m_scsi_mfg_ctrl.GetWindowText(cs_temp);
	_pProfile->SetScsiMfg (cs_temp);

	m_scsi_product_ctrl.GetWindowText(cs_temp);
	_pProfile->SetScsiProduct(cs_temp);

	m_volume_label_ctrl.GetWindowText(cs_temp);
	_pProfile->SetVolumeLabel(cs_temp);

	b_temp = m_volume_label_check_ctrl.GetCheck();
	_pProfile->SetUseVolume(b_temp);

//	if (m_bNewProfileMode)
//	{
		m_vve_player_profile_ctrl.GetWindowText(cs_temp);

	    if ( cs_temp.IsEmpty() ) {
		    CString resStr, resTitleStr;
			resStr.LoadString(IDS_CFG_PROFILE_INVALID_DESC_MSG);
			resTitleStr.LoadString(IDS_ERROR);
		    MessageBox(resStr, resTitleStr, MB_OK | MB_ICONERROR);
			return FALSE;
		}

		_pProfile->SetName(cs_temp);
//	}

	return TRUE;
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


void CConfigPlayerProfilePage::OnBnClickedVolumeLabelCheck()
{
	CString cs_temp;
	m_volume_label_ctrl.GetWindowText(cs_temp);
	if ( m_p_player_profile )
	    m_p_player_profile->m_b_use_volume_label = m_volume_label_check_ctrl.GetCheck();
}

//validation has changed 
//wParam - pointer to a notification message structure MNHDR (code contains enumerated value of current error brush see VVEbrush)
//lParam - The new valid state. 0 = Invalid, other = Valid
LRESULT CConfigPlayerProfilePage::OnValidChanged(WPARAM wParam, LPARAM lParam)
{
/*    ASSERT(m_p_player_profile->m_edit_mode != PROFILE_MODE_READ_ONLY); */
    LPNMHDR pNM = (LPNMHDR) wParam;
    CString cs_temp;
	BOOL changed = FALSE;
//	CVisValidEdit::VVEbrush iBrush = (CVisValidEdit::VVEbrush)pNM->code ;

	if (!m_p_player_profile)
	{
		return 0;
	}
	INT_PTR ctrl_id = GetDlgItem((int)pNM->idFrom)->GetDlgCtrlID();
	switch ( ctrl_id )
    {
        case IDC_PLAYER_PROFILE_EDIT:
            changed = TRUE;
            break;
        case IDC_USB_VID_TEXT:
            m_usb_vid_ctrl.GetWindowText(cs_temp);
            if ( cs_temp.Compare( m_p_player_profile->m_cs_usb_vid ) != 0 ) {
                m_p_player_profile->m_cs_usb_vid = cs_temp;
                changed = TRUE;
            }
            break;
        case IDC_USB_PID_TEXT:
            m_usb_pid_ctrl.GetWindowText(cs_temp);
            if ( cs_temp.Compare( m_p_player_profile->m_cs_usb_pid ) != 0 ) {
                m_p_player_profile->m_cs_usb_pid = cs_temp;
                changed = TRUE;
            }
            break;
        case IDC_SCSI_MFG_TEXT:
            m_scsi_mfg_ctrl.GetWindowText(cs_temp);
            if ( cs_temp.Compare( m_p_player_profile->m_cs_scsi_mfg ) != 0 ) {
                m_p_player_profile->m_cs_scsi_mfg = cs_temp;
                changed = TRUE;
            }
            break;
        case IDC_SCSI_PRODUCT_TEXT:
            m_scsi_product_ctrl.GetWindowText(cs_temp);
            if ( cs_temp.Compare( m_p_player_profile->m_cs_scsi_product ) != 0 ) {
				m_p_player_profile->m_cs_scsi_product = cs_temp;
                changed = TRUE;
            }
            break;
        case IDC_VOLUME_LABEL_TEXT:
            m_volume_label_ctrl.GetWindowText(cs_temp);
            if ( cs_temp.Compare( m_p_player_profile->m_cs_volume_label ) != 0 ) {
				m_p_player_profile->m_cs_volume_label = cs_temp;
                changed = TRUE;

                if ( m_p_player_profile->m_cs_volume_label.IsEmpty() ) {
		            m_p_player_profile->m_b_use_volume_label = FALSE;
                    m_volume_label_check_ctrl.SetCheck(m_p_player_profile->m_b_use_volume_label);
		            m_volume_label_check_ctrl.EnableWindow(FALSE);
	            }
                else {
                    m_volume_label_ctrl.EnableWindow(TRUE);
		            m_volume_label_check_ctrl.EnableWindow(!m_p_player_profile->m_cs_volume_label.IsEmpty());
		            m_volume_label_check_ctrl.SetCheck(m_p_player_profile->m_b_use_volume_label);
	            }
            }
            break;
        default:
            break;
    }
    if ( changed && m_p_player_profile ) {
        m_p_player_profile->Validate();
		if( !m_vve_player_profile_ctrl.IsValid() ) {
            m_p_player_profile->m_status = PROFILE_ERROR;
			m_p_player_profile->m_error_msg.LoadString(IDS_CFG_PROFILE_INVALID_DESC);
		}
//		LoadControlsFromProfile();
        OnUpdateStatus(0,0);
    }
	return 0;
}

DWORD CConfigPlayerProfilePage::SaveProfile(CPlayerProfile *_pProfile)
{
    CString cs_temp, cs_temp_path;
    BOOL b_temp;

	if( _pProfile->m_bLockedProfile )
		return 1;

	if( _pProfile->IsNew() )
	{
		// create profile folder
		if ( !CreateDirectory(_pProfile->GetProfilePath(), NULL) )
		{
            DWORD err = GetLastError();
            ATLTRACE(_T("ERROR: SaveProfile() - Could not create profile directory %s.(%d)\n"), _pProfile->GetName(), err);
            return err;
        }

		// create player.ini
		CFile file(_pProfile->m_cs_ini_file, CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyWrite);
	    file.Close();

		_pProfile->m_cs_original_name = _pProfile->m_cs_name;

		_pProfile->m_bNew = FALSE;

	}

    // see if _new_name directory already exists
    if ( _pProfile->m_cs_name.CompareNoCase(_pProfile->m_cs_original_name) != 0 ) {
        // the name has changed, so we need to rename the directory
        // we better check if there is already a directory with the new name
        // and let the user choose what to do.
        if ( _taccess(_pProfile->m_cs_name, 0) != ERROR_SUCCESS )
		{
            _pProfile->RenameProfile(_pProfile->m_cs_name, TRUE);
	    }
    }

	cs_temp.Format(L"%d", MIN_PROFILE_VERSION);
	_pProfile->SetIniField( CPlayerProfile::VERSION, cs_temp.GetBuffer());

	cs_temp = _pProfile->GetUsbVid();
	_pProfile->SetIniField( CPlayerProfile::USB_VID, cs_temp.GetBuffer());

	cs_temp = _pProfile->GetUsbPid();
    _pProfile->SetIniField( CPlayerProfile::USB_PID, cs_temp.GetBuffer());

	cs_temp = _pProfile->GetScsiMfg();
    _pProfile->SetIniField( CPlayerProfile::SCSI_MFG, cs_temp.GetBuffer());

	cs_temp = _pProfile->GetScsiProduct();
    _pProfile->SetIniField( CPlayerProfile::SCSI_PRODUCT, cs_temp.GetBuffer());

	cs_temp = _pProfile->GetVolumeLabel();
    _pProfile->SetIniField( CPlayerProfile::VOLUME_LABEL, cs_temp.GetBuffer());

	b_temp = _pProfile->UseVolumeLabel();
    _pProfile->SetIniField( CPlayerProfile::USE_VOLUME_LABEL, &b_temp);

	cs_temp = _pProfile->GetName();
    _pProfile->SetIniField( CPlayerProfile::PLAYER, cs_temp.GetBuffer());

	// Remove any deleted ops
	_pProfile->RemoveDeletedOps();

	// save all operations for this profile
	COpInfo* pOpInfo;
	POSITION pos = _pProfile->m_p_op_info_list.GetHeadPosition();
    while (pos)
	{
		pOpInfo = _pProfile->m_p_op_info_list.GetNext(pos);
		if (pOpInfo)
		{
		    // see if _new_name directory already exists
			if ( !pOpInfo->m_cs_NewName.IsEmpty() && 
				pOpInfo->m_cs_ini_section.CompareNoCase(pOpInfo->m_cs_NewName) != 0 )
		    {
		        // the name has changed, so we need to rename the directory
		        // we better check if there is already a directory with the new name
		        // and let the user choose what to do.
				cs_temp_path = pOpInfo->m_cs_path.Left(pOpInfo->m_cs_path.ReverseFind(_T('\\'))+1) + pOpInfo->m_cs_NewName;
		        if ( _taccess(cs_temp_path, 0) == ERROR_SUCCESS )
		        {
				    // the directory already exists, so ask the user what to do.
				    CString resStr, resTitleStr, resFmtStr;

					resFmtStr.LoadString(IDS_OP_ALREADY_EXISTS);
					resStr.Format(resFmtStr, pOpInfo->m_cs_NewName, cs_temp_path);
					resTitleStr.LoadString(IDS_CONFIRM_OVERWRITE);

				    if ( IDCANCEL == MessageBox(resStr, resTitleStr, MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) )
					{
					    return 0;
				    }
				    else
						CreateOperationFolder(pOpInfo, TRUE); // overwrite existing directory
		        }
				else // the new directory does not exist so just rename it
		            CreateOperationFolder(pOpInfo);
		    }

			// fix up the player.ini file
			pOpInfo->m_cs_cmd_line.Format(_T("%s=%s,%d,%d"), pOpInfo->m_cs_desc, pOpInfo->m_cs_ini_section, pOpInfo->m_timeout, pOpInfo->m_b_enabled);
			pOpInfo->ReplaceIniLine(_T("OPERATIONS"), pOpInfo->m_cs_cmd_line, pOpInfo->m_index);
			pOpInfo->WriteIniSection(pOpInfo->m_cs_ini_section);

		    // now copy/delete the files

			PerformFileOps(pOpInfo, &pOpInfo->m_FileList, pOpInfo->m_cs_path, CFileList::DELETE_FROM_TARGET);
			PerformFileOps(pOpInfo, &pOpInfo->m_FileList, pOpInfo->m_cs_path, CFileList::CREATE_DIR);
			PerformFileOps(pOpInfo, &pOpInfo->m_FileList, pOpInfo->m_cs_path, CFileList::COPY_TO_TARGET);
		}
	}

	return 1;
}


void CConfigPlayerProfilePage::PerformFileOps(COpInfo * _pOpInfo, CFileList * _pFileList, CString _destFolder, CFileList::FileListAction _action)
{
	CFileList::PFILEITEM pItem;
	SHFILEOPSTRUCT FileOp;

	FileOp.hwnd = GetSafeHwnd();
	FileOp.hNameMappings = NULL;
    FileOp.lpszProgressTitle = NULL;

	for (int i = 0; i < _pFileList->GetCount(); ++i)
	{
		pItem = _pFileList->GetAt(i);

		if (pItem->m_action == CFileList::EXISTS_IN_TARGET && !pItem->m_pSubFolderList)
			continue; // skip - no changes

		if (pItem->m_pSubFolderList) // we have a folder/sublist
		{
			CString newFolder = _destFolder + _T("\\") + pItem->m_csFileName;
			if (_action == CFileList::CREATE_DIR)
			{
				// create the folder now
				if (!CreateDirectory(newFolder, NULL))
					ATLTRACE(_T("PerformFileOps() CreateDirectory() error(%d)\n"), GetLastError());
				else		// change to EXISTS_IN_TARGET
					pItem->m_action = pItem->m_currentAction = CFileList::EXISTS_IN_TARGET;
			}
			PerformFileOps(_pOpInfo, pItem->m_pSubFolderList, newFolder, _action);
		}

		if (pItem->m_action == _action)
		{
			switch (_action)
			{
			case CFileList::DELETE_FROM_TARGET:
				{
				BOOL bDeleteFile = TRUE;
				// Need to check if any other drive references the same file.
				// If not, delete it from the folder.
				for( unsigned int drvIndex = 0; drvIndex < (unsigned int)_pOpInfo->m_drive_array.Size(); ++drvIndex )
					if( pItem->m_csFilePathName.CompareNoCase(_pOpInfo->m_drive_array[drvIndex].Name) == 0)
						bDeleteFile = FALSE;
				
				if( bDeleteFile )
				{
					// Delete file
					//it wants an additional NULL at the end
					pItem->m_csFilePathName.AppendChar(_T('\0'));
					FileOp.wFunc = FO_DELETE;
					FileOp.pFrom = pItem->m_csFilePathName;
				    FileOp.pTo = NULL;
					FileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
					if ( SHFileOperation(&FileOp) != ERROR_SUCCESS )
					{
						ATLTRACE(_T("PerformFileOps() - Could not delete %s. (%d)\n"), pItem->m_csFilePathName, GetLastError());
					}
				}
				break;
				}

			case CFileList::COPY_TO_TARGET:
				FileOp.wFunc = FO_COPY;
				pItem->m_csFilePathName.AppendChar(_T('\0'));
				FileOp.pFrom = pItem->m_csFilePathName;
		        FileOp.pTo = _destFolder;
				FileOp.fFlags = 0;
				FileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;

				if ( SHFileOperation(&FileOp) != ERROR_SUCCESS )
				{	
					ATLTRACE(_T("PerformFileOps() - Could not copy %s. (%d)\n"), pItem->m_csFilePathName, GetLastError());
				}
				else
					pItem->m_action = pItem->m_currentAction = CFileList::EXISTS_IN_TARGET;
				break;
			}
		}
	}
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

DWORD CConfigPlayerProfilePage::CreateOperationFolder(COpInfo* pOpInfo, BOOL _overwrite)
{
    CString cs_new_path = pOpInfo->m_cs_path.Left(pOpInfo->m_cs_path.ReverseFind(_T('\\'))+1) + pOpInfo->m_cs_NewName;

    SHFILEOPSTRUCT FileOp;
	CString cs_from, new_cmd_line;
    DWORD err = ERROR_SUCCESS;
	FileOp.hwnd = GetSafeHwnd();
    FileOp.hNameMappings = NULL;
    FileOp.lpszProgressTitle = NULL;
	CWaitCursor wait;

    if ( _overwrite )
    {
        cs_from = pOpInfo->m_cs_path;
		cs_from.AppendChar(_T('\0'));
		FileOp.wFunc = FO_RENAME;
		FileOp.pFrom = cs_from;
		FileOp.pTo = cs_new_path;
		FileOp.fFlags = FOF_NOCONFIRMATION;
		if ( SHFileOperation( &FileOp ) != ERROR_SUCCESS )
        {
            err = GetLastError();
            ATLTRACE(_T("ERROR: CreateOperationFolder() - Could not rename %s. (%d)\n"), cs_from, err);
            return err;
        }
    }
	else
	{
		if ( !CreateDirectory(cs_new_path, NULL) )
		{
            err = GetLastError();
            ATLTRACE(_T("ERROR: CreateOperationFolder() - Could not create directory %s.(%d)\n"), cs_new_path, err);
            return err;
        }
	}

    
    // save the new member variables
    pOpInfo->m_cs_path = cs_new_path;
    pOpInfo->m_cs_ini_section = pOpInfo->m_cs_NewName;
	pOpInfo->m_cs_cmd_line = new_cmd_line;
    
    return ERROR_SUCCESS;
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
	case VK_DELETE:
		OpWorkerRemove();
		break;
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

DWORD CConfigPlayerProfilePage::OpWorkerNew(COperation::OpTypes _opType)
{
	INT_PTR ret = IDCANCEL;
	int index = m_operations_ctrl.GetItemCount();
	COpInfo *pInfo = m_p_player_profile->AddOperation();

	pInfo->m_e_type = _opType;
	pInfo->m_b_new_op = TRUE;

	if (_opType == COperation::COPY_OP)
	{
		CCopyOpDlg *pOpEditor = new CCopyOpDlg(this, pInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
	else if ( _opType == COperation::LOADER_OP)
	{
		CLoadFileOpDlg *pOpEditor = new CLoadFileOpDlg(this, pInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
	else if (_opType == COperation::UPDATE_OP)
	{
		COpUpdateDlg *pOpEditor = new COpUpdateDlg(this, pInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
	else if (_opType == COperation::OTP_OP)
	{
		COpOTPDlg *pOpEditor = new COpOTPDlg(this, pInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
	else if (_opType == COperation::UTP_UPDATE_OP || _opType == COperation::MX_UPDATE_OP)
	{
		COpUtpUpdateDlg *pOpEditor = new COpUtpUpdateDlg(this, pInfo);
	    ret = pOpEditor->DoModal();
	    delete pOpEditor;
	}
//	else if (_opType == COperation::MX_UPDATE_OP)
//	{
//		COpMxRomUpdateDlg *pOpEditor = new COpMxRomUpdateDlg(this, pInfo);
//	    ret = pOpEditor->DoModal();
//	    delete pOpEditor;
//	}

    if (ret == IDOK)
	{
	    pInfo->Validate();
		InsertOpsList(m_p_player_profile);
		m_operations_ctrl.SetItemData(index, (DWORD_PTR)pInfo);
	    m_operations_ctrl.SetItemState( index, LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
	}
	else
	{
		pInfo->m_cs_path.Empty();
		m_p_player_profile->RemoveOperation(index);
	}

	return m_operations_ctrl.GetItemCount();
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
		COpUtpUpdateDlg *pOpEditor = new COpUtpUpdateDlg(this, pOpInfo);
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

DWORD CConfigPlayerProfilePage::OpWorkerRemove(void)
{
    POSITION pos = m_operations_ctrl.GetFirstSelectedItemPosition();
    if (pos == NULL)
        return -1;
	int index = m_operations_ctrl.GetNextSelectedItem(pos);
	COpInfo * pInfo = (COpInfo*)m_operations_ctrl.GetItemData(index);
	
	CString resStr, resTitleStr;
	resStr.Format(IDS_CFG_PROFILE_DELETE_CONFIRM, pInfo->GetDesc(), pInfo->GetIniSection());
	resTitleStr.LoadString(IDS_CFG_PROFILE_DELETE_TITLE);
	if ( IDCANCEL == MessageBox(resStr, resTitleStr, MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) ) {
		return -1;
	}

	m_p_player_profile->RemoveOperation(index);
    m_p_player_profile->Validate();
	InsertOpsList(m_p_player_profile);

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
	case IDM_NEW_COPY_OP:
		OpWorkerNew(COperation::COPY_OP);
		break;
	case IDM_NEW_LOAD_OP:
	    OpWorkerNew(COperation::LOADER_OP);
		break;
	case IDM_NEW_UPDATE_OP:
	    OpWorkerNew(COperation::UPDATE_OP);
		break;
	case IDM_NEW_OTP_OP:
	    OpWorkerNew(COperation::OTP_OP);
		break;
	case IDM_NEW_UTP_UPDATE_OP:
		OpWorkerNew(COperation::UTP_UPDATE_OP);
		break;
	case IDM_NEW_MX_UPDATE_OP:
		OpWorkerNew(COperation::MX_UPDATE_OP);
		break;
	case IDM_ST_EDIT:
        OpWorkerEdit();
		break;
	case IDM_ST_DELETE:
		OpWorkerRemove();
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