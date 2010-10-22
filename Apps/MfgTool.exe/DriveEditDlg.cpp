/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// DriveEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "DriveEditDlg.h"

	TAGLISTSTRUCT g_TagList[MAX_TAGS] =
	{
		// Type						Display						Tag								Prerequisite Tag				Action
		//------------------------------------------------------------------------------------------------------------------------------------------------
		media::DriveType_System,	_T("(0x00) Player"),		media::DriveTag_Player,			NO_PREREQUISITES,				media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x01) UsbMsc"),		media::DriveTag_UsbMsc,			NO_PREREQUISITES,				media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x01) Hostlink"),		media::DriveTag_Hostlink,		NO_PREREQUISITES,				media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x02) PlayerRsc"),		media::DriveTag_PlayerRsc,		media::DriveTag_Player,			media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x12) PlayerRsc2"),	media::DriveTag_PlayerRsc2,		media::DriveTag_PlayerRsc,		media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x22) PlayerRsc3"),	media::DriveTag_PlayerRsc3,		media::DriveTag_PlayerRsc2,		media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x02) FirmwareRsc"),	media::DriveTag_FirmwareRsc,	media::DriveTag_FirmwareImg,	media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x12) FirmwareRsc2"),	media::DriveTag_FirmwareRsc2,	media::DriveTag_FirmwareRsc,	media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x22) FirmwareRsc3"),	media::DriveTag_FirmwareRsc3,	media::DriveTag_FirmwareRsc2,	media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x06) HostlinkRsc"),	media::DriveTag_HostlinkRsc,	media::DriveTag_Hostlink,		media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x16) HostlinkRsc2"),	media::DriveTag_HostlinkRsc2,	media::DriveTag_HostlinkRsc,	media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x26) HostlinkRsc3"),	media::DriveTag_HostlinkRsc3,	media::DriveTag_HostlinkRsc2,	media::DriveFlag_ImageData,
		media::DriveType_Data,		_T("(0x0A) Data"),			media::DriveTag_Data,			NO_PREREQUISITES,				media::DriveFlag_Format,
		media::DriveType_Data,		_T("(0x1A) Data2"),			media::DriveTag_Data2,			media::DriveTag_Data,			media::DriveFlag_Format,
		media::DriveType_HiddenData,_T("(0x0B) Janus"),			media::DriveTag_DataJanus,		NO_PREREQUISITES,				media::DriveFlag_JanusInit,
		media::DriveType_HiddenData,_T("(0x0C) Settings"),		media::DriveTag_DataSettings,	NO_PREREQUISITES,				media::DriveFlag_NoAction,
		media::DriveType_System,	_T("(0x50) Bootmanger"),	media::DriveTag_Bootmanger,		NO_PREREQUISITES,				media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x50) FirmwareImg"),	media::DriveTag_FirmwareImg,	NO_PREREQUISITES,				media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x60) FirmwareImg2"),	media::DriveTag_FirmwareImg2,	media::DriveTag_FirmwareImg,	media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0x70) FirmwareImg3"),	media::DriveTag_FirmwareImg3,	media::DriveTag_FirmwareImg2,	media::DriveFlag_ImageData,
		media::DriveType_System,	_T("(0xA0) LBA-Boot"),		media::DriveTag_LBABoot,		NO_PREREQUISITES,				media::DriveFlag_ImageData,
		media::DriveType_Invalid,	_T("(0xF0) Invalid"),		media::DriveTag_Invalid,		NO_PREREQUISITES,				media::DriveFlag_NoAction,
	};

// CDriveEditDlg dialog

IMPLEMENT_DYNAMIC(CDriveEditDlg, CDialog)

CDriveEditDlg::CDriveEditDlg(CWnd* pParent /*=NULL*/, BOOL _bNew, COpInfo* _pInfo, int _index)
	: CDialog(CDriveEditDlg::IDD, pParent)
{
	m_bIsNewDrive = _bNew;
	m_pOpInfo = _pInfo;
	m_index = _index; // this index is for the currently selected drivearray


}



CDriveEditDlg::~CDriveEditDlg()
{
}

void CDriveEditDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_str_edit_description);
    DDX_Control(pDX, IDC_COMBO_DRIVE_TYPE, m_combo_drive_type);
    DDX_Control(pDX, IDC_COMBO_DRIVE_TAG, m_combo_drive_tag);
    DDX_Text(pDX, IDC_EDIT_REQUESTED_DRIVE_SIZE, m_dw_edit_requested_drive_size);
    DDV_MinMaxUInt(pDX, m_dw_edit_requested_drive_size, 0, 4294967295);
    DDX_Control(pDX, IDC_BROWSE_FILE, m_ctrl_browse_file);
    DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_ctrl_edit_description);
    DDX_Control(pDX, IDC_EDIT_REQUESTED_DRIVE_SIZE, m_ctrl_edit_requested_drive_size);
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDriveEditDlg, CDialog)
    ON_BN_CLICKED(IDC_BROWSE_FILE, OnBnClickedBrowseFile)
	ON_BN_CLICKED(IDOK, &CDriveEditDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDriveEditDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_DRIVE_TYPE, &CDriveEditDlg::OnCbnSelchangeComboDriveType)
	ON_CBN_SELCHANGE(IDC_COMBO_DRIVE_TAG, &CDriveEditDlg::OnCbnSelchangeComboDriveTag)
END_MESSAGE_MAP()



// CDriveEditDlg message handlers
BOOL CDriveEditDlg::OnInitDialog() 
{

	CDialog::OnInitDialog();

	Localize();

    m_ctrl_browse_file.EnableWindow(TRUE);
    m_ctrl_edit_description.EnableWindow(TRUE);
    m_combo_drive_type.EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(TRUE);
	m_ctrl_edit_requested_drive_size.EnableWindow(TRUE);

	m_ctrl_edit_file_name.SubclassDlgItem(IDC_EDIT_FILE_NAME, this);
	m_ctrl_edit_file_name.SetUseDir(FALSE);
	m_ctrl_edit_file_name.SetReadOnly(FALSE);

    InitDriveTypeCombo();
	InitDriveTagCombo();

	if( m_index != -1 )
	{
		media::LogicalDrive driveDesc = m_pOpInfo->m_drive_array[m_index];
		m_original_filename = m_pOpInfo->m_FileList.GetFileNameAt(driveDesc.FileListIndex);
		SetDlgItems(driveDesc);
	}
	else
		m_original_filename = _T("");

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}



void CDriveEditDlg::SetDlgItems(const media::LogicalDrive& _driveDesc)
{
	m_str_edit_description = _driveDesc.Description.c_str();
	m_dw_edit_requested_drive_size = _driveDesc.RequestedKB;
	UpdateData(FALSE);

	if( _driveDesc.Type != media::DriveType_Invalid )
	{
		if ( _driveDesc.Tag == media::DriveTag_DataJanus || 
			_driveDesc.Tag == media::DriveTag_Data || _driveDesc.Tag == media::DriveTag_Data2 )
		{	// hidden (Janus) and data drives cannot have files associated
		    GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(FALSE);
			m_ctrl_browse_file.EnableWindow(FALSE);
		}
		else
		{
			CString itemStr;

		    GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(TRUE);
			m_ctrl_browse_file.EnableWindow(TRUE);

			itemStr = m_pOpInfo->m_FileList.GetPathNameAt(_driveDesc.FileListIndex);
			if (itemStr)
				SetDlgItemText(IDC_EDIT_FILE_NAME, itemStr);
		}
		m_ctrl_edit_description.EnableWindow(TRUE);
		m_ctrl_edit_requested_drive_size.EnableWindow(TRUE);
	}
	else
	{
		// invalid type
		GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(FALSE);
	    m_ctrl_browse_file.EnableWindow(FALSE);
	    m_ctrl_edit_description.EnableWindow(FALSE);
	    m_ctrl_edit_requested_drive_size.EnableWindow(FALSE);
	}
}

BOOL CDriveEditDlg::GetDlgItems(media::LogicalDrive& _driveDesc)
{
	UpdateData(TRUE);

	int idx;
	media::LogicalDriveTag newTag;

	_driveDesc.Description = m_str_edit_description;
	_driveDesc.Type = (media::LogicalDriveType) m_combo_drive_type.GetItemData(m_combo_drive_type.GetCurSel());
	idx = (int) m_combo_drive_tag.GetItemData(m_combo_drive_tag.GetCurSel());
	newTag = g_TagList[idx].tag; 
	_driveDesc.RequestedKB = m_dw_edit_requested_drive_size;

	if ( newTag != media::DriveTag_DataJanus &&
		newTag != media::DriveTag_Data && newTag != media::DriveTag_Data2 )
	{	// hidden (Janus) and data drives cannot have files associated
		CString csFname;

		GetDlgItemText(IDC_EDIT_FILE_NAME, csFname);
		idx = csFname.ReverseFind(_T('\\'));

		if ( !csFname.IsEmpty() )
		{	// adding or changing a file
	        if ( _taccess(csFname, 0) != ERROR_SUCCESS )
			{
				CString errFmtMsg, errMsg;
				
				errFmtMsg.LoadString(IDS_PROFILE_ERR_FILE_NO_EXIST);
				errMsg.Format(errFmtMsg, csFname);
				::MessageBox(NULL, errMsg, _T("MfgTool"), MB_OK);
				return FALSE;
			}

			_driveDesc.Name = csFname.Mid(idx+1);
			if ( _driveDesc.FileListIndex == -1 )
				_driveDesc.FileListIndex = m_pOpInfo->m_FileList.AddFile( csFname, CFileList::IN_EDIT_ADD );
			else
				m_pOpInfo->m_FileList.ChangeFile(_driveDesc.FileListIndex, csFname);
			_driveDesc.Flags = media::DriveFlag_ImageData;
		}
		else
		{	// removing a file
			if ( _driveDesc.Flags == media::DriveFlag_ImageData )
				_driveDesc.Flags = media::DriveFlag_NoAction;

			if ( _driveDesc.FileListIndex != -1 )
			{
				CFileList::PFILEITEM pItem = m_pOpInfo->m_FileList.GetAt(_driveDesc.FileListIndex);
				pItem->m_action = CFileList::IN_EDIT_DELETE;
			}
			_driveDesc.FileListIndex = -1;
			_driveDesc.Name = "";
			_driveDesc.Flags = media::DriveFlag_NoAction;
		}
	}
	else
	{ 	// changing to non-file associated tag; remove any file
		_driveDesc.Flags = g_TagList[idx].Flags;

		if ( _driveDesc.FileListIndex != -1 )
		{
			CFileList::PFILEITEM pItem = m_pOpInfo->m_FileList.GetAt(_driveDesc.FileListIndex);
			pItem->m_action = CFileList::IN_EDIT_DELETE;
		}
		_driveDesc.FileListIndex = -1;
		_driveDesc.Name = "";
	}

	_driveDesc.Tag = newTag;

	return TRUE;
}

void CDriveEditDlg::OnBnClickedOk()
{
	if ( !GetDlgItems( m_pOpInfo->m_drive_array[m_index] ) )
	{
		return;
	}
	m_pOpInfo->m_FileList.CommitEdits();

	OnOK();
}

void CDriveEditDlg::OnBnClickedCancel()
{
	m_pOpInfo->m_FileList.RemoveEdits();

	OnCancel();
}

//-------------------------------------------------------------------
// InitDriveTypeCombo()
//
// Initialize the drive type combo box text items with the currently
// supported drive types
//-------------------------------------------------------------------
void CDriveEditDlg::InitDriveTypeCombo()
{
    m_combo_drive_type.AddString(_T("Data"));
	m_combo_drive_type.SetItemData(0, media::DriveType_Data);
    m_combo_drive_type.AddString(_T("System"));
	m_combo_drive_type.SetItemData(1, media::DriveType_System);
    m_combo_drive_type.AddString(_T("Hidden Data"));
	m_combo_drive_type.SetItemData(2, media::DriveType_HiddenData);
//	m_combo_drive_type.AddString(_T("Non-Volatile"));
//	m_combo_drive_type.SetItemData();
	m_combo_drive_type.AddString(_T("Invalid"));
	m_combo_drive_type.SetItemData(3, media::DriveType_Invalid);

	if( m_index != -1 )
	{
		switch (m_pOpInfo->m_drive_array[m_index].Type)
		{
			case media::DriveType_Data:
				m_combo_drive_type.SelectString(-1, _T("Data"));
				break;
			case media::DriveType_System:
				m_combo_drive_type.SelectString(-1, _T("System"));
				break;
			case media::DriveType_HiddenData:
				m_combo_drive_type.SelectString(-1, _T("Hidden Data"));
				break;
			case media::DriveType_Invalid:
				m_combo_drive_type.SelectString(-1, _T("Invalid"));
				break;
		}
	}
	else
		m_combo_drive_type.SelectString(-1, _T("Invalid"));
}

//-------------------------------------------------------------------
// InitDriveTagCombo()
//
// Initialize the drive tag combo box text items with the currently
// supported drive tags.
//
// Add only tags we don't already have defined in the drive array, but also add the
// currently defined tag if we are editing an existing drive.  Make
// sure any prerequisite drive exists as well.
//-------------------------------------------------------------------
void CDriveEditDlg::InitDriveTagCombo()
{


	media::LogicalDriveType drvType = (media::LogicalDriveType) m_combo_drive_type.GetItemData(m_combo_drive_type.GetCurSel());

	m_combo_drive_tag.ResetContent();

	for( int i = 0; i < MAX_TAGS; ++i)
	{
		int j;
		BOOL bAddTag = TRUE;

		if( g_TagList[i].type != drvType )
			continue;  // not our type

		for( j = 0; j < (int) m_pOpInfo->m_drive_array.Size(); ++j )
		{
			if( m_pOpInfo->m_drive_array[j].Tag == g_TagList[i].tag )
			{
				bAddTag = FALSE; // we already have this one
				break; 
			}
		}

		if( bAddTag )
		{
			// didn't find it, or editing existing drive
			if( g_TagList[i].prerequisite != NO_PREREQUISITES )
			{
				// check that a prerequisite tagged drive exists
				if( !CheckForPrerequisite(g_TagList[i].prerequisite))
					bAddTag = FALSE; // prerequisite not found
			}
		}

		if( j == m_index )
			bAddTag = TRUE; // always add the currently defined tag

		if( bAddTag )
		{
			int index = m_combo_drive_tag.AddString(g_TagList[i].szTag);
			m_combo_drive_tag.SetItemData(index, i);
			if( j == m_index )
				m_combo_drive_tag.SelectString(index-1, g_TagList[i].szTag);
		}
	}

	if (m_combo_drive_tag.GetCount() == 1)
		m_combo_drive_tag.SetCurSel(0);

	// check if there are dependents on this drive.  For example, DriveTag_Data must
	// exist for DriveTag_Data2 so we cannot change DriveTag_Data. 
	if( CheckForDependents() )
	{
	    m_combo_drive_tag.EnableWindow(FALSE);
	    m_combo_drive_type.EnableWindow(FALSE);
	}
	else
	{
	    m_combo_drive_tag.EnableWindow(TRUE);
	    m_combo_drive_type.EnableWindow(TRUE);
	}

	// If this is a settings drive selected and the drive system is changing go ahead and enable the file windows.
	if ( g_TagList[m_combo_drive_tag.GetItemData(m_combo_drive_tag.GetCurSel())].tag == media::DriveTag_DataSettings )
	{
		GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(TRUE);
	    m_ctrl_browse_file.EnableWindow(TRUE);
	}

}

//-------------------------------------------------------------------
// CheckForPrerequisite()
//
// Search for any prerequisite tag.  Such as DriveTag_Data must exist
// before we can add DriveTag_Data2.
//
//--------------------------------------------------------------------
BOOL CDriveEditDlg::CheckForPrerequisite(media::LogicalDriveTag _tag)
{
	BOOL found = TRUE;
	int z;

	// check that a prerequisite tagged drive exists
	for( z = 0; z < (int) m_pOpInfo->m_drive_array.Size(); ++z)
	{
		if( m_pOpInfo->m_drive_array[z].Tag == _tag )
			break;
	}

	if( z == m_pOpInfo->m_drive_array.Size() )
		found = FALSE; // prerequisite not found
	else
		if( z == m_index )
			found = FALSE; // can't overwrite our own preprerequisite

	return found;
}

//-------------------------------------------------------------------
// CheckForDependents()
//
// Search for any dependent tag.  Such as DriveTag_Data2 is dependent
// on DriveTag_Data so we cannot change/remove DriveTag_Data before
// first removing DriveTag_Data2.
//
//--------------------------------------------------------------------
BOOL CDriveEditDlg::CheckForDependents()
{
	media::LogicalDriveTag dependentTag;
	int z;
	BOOL bDependency = FALSE;

	// find a tag that has our current tag as a dependent
	for( z = 0; z < MAX_TAGS; ++z )
	{
		if( g_TagList[z].prerequisite == m_pOpInfo->m_drive_array[m_index].Tag )
		{
			dependentTag = g_TagList[z].tag;
			break;
		}
	}

	// now check if we have that tag in the drive array
	if( z < MAX_TAGS )
	{
		for( z = 0; z < (int) m_pOpInfo->m_drive_array.Size(); ++z )
		{
			if( m_pOpInfo->m_drive_array[z].Tag == dependentTag )
			{
				bDependency = TRUE;
				break;
			}
		}
	}

	return bDependency;
}

void CDriveEditDlg::OnBnClickedBrowseFile()
{
    CFileDialog *dlg;
	CString resStr;

	resStr.LoadString(IDS_FILE_BROWSE_TYPES);

   	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
							resStr, this);

    if ( IDOK == dlg->DoModal() )
    {
		CString csFullPathName = dlg->GetPathName();
		m_ctrl_edit_file_name.SetWindowTextW(csFullPathName);
    }

	delete dlg;
}


void CDriveEditDlg::OnCbnSelchangeComboDriveType()
{
	media::LogicalDriveType index = (media::LogicalDriveType) m_combo_drive_type.GetItemData(m_combo_drive_type.GetCurSel());

	switch (index)
	{
		case media::DriveType_Data:
		case media::DriveType_HiddenData:
			// check that tag is not Hidden2 to enable filename
			GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(FALSE);
		    m_ctrl_browse_file.EnableWindow(FALSE);
		    m_ctrl_edit_description.EnableWindow(TRUE);
		    m_combo_drive_tag.EnableWindow(TRUE);
		    m_ctrl_edit_requested_drive_size.EnableWindow(TRUE);
			GetDlgItem(IDOK)->EnableWindow(TRUE);
			break;
		case media::DriveType_System:
			GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(TRUE);
		    m_ctrl_browse_file.EnableWindow(TRUE);
		    m_ctrl_edit_description.EnableWindow(TRUE);
		    m_combo_drive_tag.EnableWindow(TRUE);
		    m_ctrl_edit_requested_drive_size.EnableWindow(TRUE);
			GetDlgItem(IDOK)->EnableWindow(TRUE);
			break;
		default:
			// invalid type
			GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(FALSE);
		    m_ctrl_browse_file.EnableWindow(FALSE);
		    m_ctrl_edit_description.EnableWindow(FALSE);
		    m_combo_drive_tag.EnableWindow(FALSE);
		    m_ctrl_edit_requested_drive_size.EnableWindow(FALSE);
			GetDlgItem(IDOK)->EnableWindow(FALSE);
			break;
	}

	InitDriveTagCombo();
}

void CDriveEditDlg::OnCbnSelchangeComboDriveTag()
{
	int index = (int) m_combo_drive_tag.GetItemData(m_combo_drive_tag.GetCurSel());

	switch (g_TagList[index].tag)
	{
		case media::DriveTag_DataSettings:
			GetDlgItem(IDC_EDIT_FILE_NAME)->EnableWindow(TRUE);
		    m_ctrl_browse_file.EnableWindow(TRUE);
			break;
		default: // let the drive type combo drive the rest
			break;
	}

}

void CDriveEditDlg::Localize()
{
	CString resStr;

	resStr.LoadStringW(IDS_OPERATION_UPDATE);
	SetWindowText(resStr);

	resStr.LoadString(IDS_CFG_FNAME_TEXT);
	SetDlgItemText(IDC_CFG_FNAME_TEXT, resStr);

	resStr.LoadString(IDS_CFG_DESC_TEXT);
	SetDlgItemText(IDC_CFG_DESC_TEXT, resStr);

	resStr.LoadString(IDS_CFG_DRIVE_TYPE_TEXT);
	SetDlgItemText(IDC_CFG_DRIVE_TYPE_TEXT, resStr);

	resStr.LoadString(IDS_CFG_DRIVE_TAG_TEXT);
	SetDlgItemText(IDC_CFG_DRIVE_TAG_TEXT, resStr);

	resStr.LoadString(IDS_CFG_DRIVE_SIZE_TEXT);
	SetDlgItemText(IDC_CFG_DRIVE_SIZE_TEXT, resStr);

	resStr.LoadString(IDS_OK);
	SetDlgItemText(IDOK, resStr );

	resStr.LoadString(IDS_CANCEL);
	SetDlgItemText(IDCANCEL, resStr );
}


