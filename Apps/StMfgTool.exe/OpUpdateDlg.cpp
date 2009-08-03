#include "stdafx.h"
#include "OpUpdateDlg.h"
#include "OpInfo.h"
#include "driveeditdlg.h"

// CListCtrlEx
IMPLEMENT_DYNAMIC(CConfigOpListCtrl, CListCtrl)
CConfigOpListCtrl::CConfigOpListCtrl():	m_cs_empty_text(_T(""))
{
	m_cs_empty_text.LoadString(IDS_NO_ITEMS_IN_VIEW);
}
CConfigOpListCtrl::~CConfigOpListCtrl(){}

BEGIN_MESSAGE_MAP(CConfigOpListCtrl, CListCtrl)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CConfigOpListCtrl::OnPaint()
{
	Default();
	if (GetItemCount() <= 0)
	{
		CDC* pDC = GetDC();
		int nSavedDC = pDC->SaveDC();

		CRect rc;
		GetWindowRect(&rc);
		ScreenToClient(&rc);
		CHeaderCtrl* pHC = GetHeaderCtrl();
		if (pHC != NULL)
		{
			CRect rcH;
			pHC->GetItemRect(0, &rcH);
			rc.top += rcH.bottom;
		}
		rc.top += 10;

		COLORREF crText = ::GetSysColor(COLOR_WINDOWTEXT);
		COLORREF crBkgnd = ::GetSysColor(COLOR_WINDOW);

		CBrush brush(crBkgnd);
		pDC->FillRect(rc, &brush);

		pDC->SetTextColor(crText);
		pDC->SetBkColor(crBkgnd);
		pDC->SelectStockObject(ANSI_VAR_FONT);
		pDC->DrawText(m_cs_empty_text, -1, rc, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);
		pDC->RestoreDC(nSavedDC);
		ReleaseDC(pDC);
	}
}

// COpUpdateDlg dialog

IMPLEMENT_DYNAMIC(COpUpdateDlg, CDialog)
COpUpdateDlg::COpUpdateDlg(CWnd* pParent /*=NULL*/, COpInfo* pInfo)
	: CDialog(COpUpdateDlg::IDD, pParent)
    , m_str_edit_file_name(_T(""))
    , m_str_edit_description(_T(""))
    , m_dw_edit_requested_drive_size(0)
    , m_str_edit_drive_tag(_T(""))
{
	m_pOpInfo = pInfo;
	m_current_selection = -1;
	m_pOpInfo->m_e_type = COperation::UPDATE_OP;
	m_savedDriveArray = m_pOpInfo->m_drive_array;

}

COpUpdateDlg::~COpUpdateDlg()
{
}

void COpUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_OP_NAME_EDIT, m_op_name_ctrl);
    DDX_Control(pDX, IDC_DRIVE_LIST, m_drive_list_ctrl);
    DDX_Control(pDX, IDC_OP_OPTIONS_LIST, m_op_options_ctrl);
}


BEGIN_MESSAGE_MAP(COpUpdateDlg, CDialog)
    ON_WM_VKEYTOITEM()
	ON_NOTIFY(NM_RCLICK, IDC_DRIVE_LIST, &COpUpdateDlg::OnContextMenu)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_OP_OPTIONS_LIST, OnLvnItemchangedOpOptionsList)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_UPDATE_BOOT_FW_BROWSE_BTN, &COpUpdateDlg::OnBnClickedBrowseFile)
END_MESSAGE_MAP()


// COpUpdateDlg message handlers
BOOL COpUpdateDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
   
	Localize();

	m_op_name_ctrl.SetValidCharSet(CVisValidEdit::SET_DIRPATH);
    m_op_name_ctrl.SetEmptyValid(FALSE);
    m_op_name_ctrl.SetTextLenMinMax(1);
    m_op_name_ctrl.SetEnableLengthCheck();
    m_op_name_ctrl.SetWindowText(m_pOpInfo->m_cs_ini_section);

	InitOptionsListBox();
	InitFileList();

	int index = m_pOpInfo->GetUpdaterBootFilenameIndex();
	if (index != -1)
	{
		CFileList::PFILEITEM pItem = m_pOpInfo->m_FileList.GetAt(index);

		if (pItem->m_action == CFileList::EXISTS_IN_TARGET)
			SetDlgItemText(IDC_UPDATE_BOOT_FW_EDIT, m_pOpInfo->GetUpdaterBootFilename());
		else
			SetDlgItemText(IDC_UPDATE_BOOT_FW_EDIT, m_pOpInfo->GetUpdaterBootPathname());
	}

	m_updater_fname.SubclassDlgItem(IDC_UPDATE_BOOT_FW_EDIT, this);
	m_updater_fname.SetUseDir(FALSE);
	m_updater_fname.SetReadOnly(TRUE);



    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

//-------------------------------------------------------------------
// InitFileList()
//
// Initialize the file list control with the files contained in the
// drive array.
//-------------------------------------------------------------------
void COpUpdateDlg::InitFileList()
{
	RECT listRect;
	CString resStr;
	m_drive_list_ctrl.GetClientRect(&listRect);
	int nListWidth = (listRect.right - listRect.left);
	int nVScrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL); 
	
	int nCol1Width = (int) ((double)nListWidth * 0.20);  // file name
    int nCol2Width = (int) ((double)nListWidth * 0.30);  // description
    int nCol3Width = (int) ((double)nListWidth * 0.20);  // drive type i.e. "System", "Data", etc...
    int nCol4Width = (int) ((double)nListWidth * 0.10);  // drive tag
														 // requested size
    int nCol5Width = nListWidth - nCol1Width - nCol2Width - nCol3Width - nCol4Width - nVScrollBarWidth;  // date

	resStr.LoadStringW(IDS_COLUMN_FILENAME);
	m_drive_list_ctrl.InsertColumn(0, resStr, LVCFMT_LEFT, nCol1Width);
	resStr.LoadStringW(IDS_OPUPDATE_HEADER_DESC);
	m_drive_list_ctrl.InsertColumn(1, resStr, LVCFMT_LEFT, nCol2Width);
	resStr.LoadStringW(IDS_OPUPDATE_HEADER_TYPE);
	m_drive_list_ctrl.InsertColumn(2, resStr, LVCFMT_LEFT, nCol3Width);
	resStr.LoadStringW(IDS_OPUPDATE_HEADER_TAG);
	m_drive_list_ctrl.InsertColumn(3, resStr, LVCFMT_LEFT, nCol4Width);
	resStr.LoadStringW(IDS_COLUMN_SIZE);
	m_drive_list_ctrl.InsertColumn(4, resStr, LVCFMT_LEFT, nCol5Width);

	m_drive_list_ctrl.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	for (unsigned int i=0; i < m_pOpInfo->m_drive_array.Size(); i++)
	{
		UpdateListItem(TRUE, i, m_pOpInfo->m_drive_array[i]);
	}
}

void COpUpdateDlg::UpdateListItem(BOOL _bInsert, int _index, const media::LogicalDrive& _driveDesc)
{
	CString itemStr;

	itemStr = m_pOpInfo->m_FileList.GetFileNameAt(_driveDesc.FileListIndex);
		
	if ( _bInsert )
		m_drive_list_ctrl.InsertItem(_index, itemStr );
	else
		m_drive_list_ctrl.SetItemText(_index, 0, itemStr);

	m_drive_list_ctrl.SetItemText(_index, 1, _driveDesc.Description.c_str());
	switch (_driveDesc.Type)
	{
		case media::DriveType_Data:
			itemStr = _T("Data");
			break;
		case media::DriveType_System:
			itemStr = _T("System");
			break;
		case media::DriveType_HiddenData:
			itemStr = _T("Hidden");
			break;
		default:
			itemStr = _T("Invalid");
			break;
	}
	m_drive_list_ctrl.SetItemText(_index, 2, itemStr);
	itemStr.Format(_T("0x%02X"), _driveDesc.Tag);
	m_drive_list_ctrl.SetItemText(_index, 3, itemStr);
	itemStr.Format(_T("%d"), _driveDesc.RequestedKB);
	m_drive_list_ctrl.SetItemText(_index, 4, itemStr);
}

//-------------------------------------------------------------------
// ConvertDriveToString()
//
// Convert a LogicalDrive data structure to the string equivalent of
// a player.ini UPDATE operation drive entry.
//-------------------------------------------------------------------
CString COpUpdateDlg::ConvertDriveToString(const media::LogicalDrive& driveDesc) const
{
    CString str, csName;

	// filename only - no path
	csName = m_pOpInfo->m_FileList.GetFileNameAt(driveDesc.FileListIndex);

    str.Format(DRIVE_FORMAT_STR,    csName,
                                    driveDesc.Description.c_str(),
                                    driveDesc.Type,
                                    driveDesc.Tag,
                                    driveDesc.RequestedKB);

    return str;
}

//-------------------------------------------------------------------
// ConvertStringToDrive()
//
// Convert a string to a LogicalDrive data structure.
//-------------------------------------------------------------------

media::LogicalDrive COpUpdateDlg::ConvertStringToDrive(CString& strDriveDesc)
{

    media::LogicalDrive  drvDesc;
    CString     tmpStr;
    int         curPos = 0;
    _TCHAR*     stopStr = _T(",");

    if (strDriveDesc.GetAt(0) != _T(','))
    {
        drvDesc.Name = strDriveDesc.Tokenize(_T(","), curPos);
    }
    else
    {
        drvDesc.Name = _T("");
        curPos++;
    }
    if (strDriveDesc.GetAt(curPos) != _T(','))
    {
        drvDesc.Description = strDriveDesc.Tokenize(_T(","), curPos);
    }
    else
    {
        drvDesc.Description = _T("");
        curPos++;
    }
    if (strDriveDesc.GetAt(curPos) != _T(','))
		drvDesc.Type = (media::LogicalDriveType)_tcstoul(strDriveDesc.Tokenize(_T(","), curPos), &stopStr, 10);
    else
        curPos++;
    if (strDriveDesc.GetAt(curPos) != _T(','))
		drvDesc.Tag = (media::LogicalDriveTag)_tcstoul(strDriveDesc.Tokenize(_T(","), curPos), &stopStr, 16);
    else
        curPos++;
    if (strDriveDesc.GetAt(curPos) != _T(','))
        drvDesc.RequestedKB = _tcstoul(strDriveDesc.Tokenize(_T(" "), curPos), &stopStr, 10);

    return drvDesc;
}

//-------------------------------------------------------------------
// ConvertControlsToDriveStr()
//
// Take current values from controls and convert to a drive string
//-------------------------------------------------------------------
CString COpUpdateDlg::ConvertControlsToDriveStr()
{
    CString drvStr;
    _TCHAR* stopStr = _T(" ");
        
    drvStr.Format(DRIVE_FORMAT_STR, m_str_edit_file_name,
                                    m_str_edit_description,
                                    m_combo_drive_type.GetCurSel(),
                                    _tcstoul(m_str_edit_drive_tag, &stopStr, 16),
                                    m_dw_edit_requested_drive_size);

    return drvStr;
}

//-------------------------------------------------------------------
// UpdateControls()
//
// Update controls with values from the selected item in the file list.
//-------------------------------------------------------------------

void COpUpdateDlg::UpdateControls()
{
	media::LogicalDrive drvDesc;
	CString strTmp;
    CString curSelStr;
    int curPos = 0;
	int curSel = m_drive_list_ctrl.GetSelectionMark();
    _TCHAR* stopStr = _T(",");

    if (curSel != -1)
    {
		drvDesc = m_pOpInfo->m_drive_array[curSel];
		m_current_selection = curSel;

		UpdateData(FALSE);
    }

}


//-------------------------------------------------------------------
// RemoveFileListItem()
//
// Remove selected item from the file list control
//-------------------------------------------------------------------
void COpUpdateDlg::RemoveFileListItem()
{
    if (m_current_selection != -1)
	{
	    media::LogicalDrive  drvDescTmp;
		CFileList::PFILEITEM pItem;

		m_drive_list_ctrl.DeleteItem(m_current_selection);
		drvDescTmp = m_drive_array.GetDrive(m_drive_array[m_current_selection].Tag);
		if (drvDescTmp.FileListIndex != -1)
		{
			pItem = m_pOpInfo->m_FileList.GetAt(drvDescTmp.FileListIndex);
			pItem->m_action = CFileList::IN_EDIT_DELETE;
		}

		m_drive_array.Remove(drvDescTmp);
		//??? what about filelist item?  Removing will throw off filelist indexes.  Mark only.
	}
	if (m_drive_list_ctrl.GetItemCount() > 0)
    {
        if (m_current_selection > 0)
        {
			m_drive_list_ctrl.SetSelectionMark(m_current_selection-1);
            UpdateControls();
        }
    }
}


void COpUpdateDlg::OnLvnItemchangedOpOptionsList(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;
}

DWORD COpUpdateDlg::InitOptionsListBox(void)
{
	CRect rect;
    int item;
	
    if ( (m_op_options_ctrl.GetExtendedStyle() & LVS_EX_CHECKBOXES) != LVS_EX_CHECKBOXES )
    {
		CString resStr;
        m_op_options_ctrl.GetClientRect(&rect);
		resStr.LoadString(IDS_OPTIONS);
	    m_op_options_ctrl.InsertColumn(0, resStr ,LVCFMT_LEFT, rect.Width());
	    m_op_options_ctrl.SetExtendedStyle(LVS_EX_CHECKBOXES); // | LVS_EX_GRIDLINES
		resStr.LoadString(IDS_NO_OPTIONS);
        m_op_options_ctrl.SetEmptyString(resStr);
	}
    m_op_options_ctrl.DeleteAllItems();

    for ( int i=0; 1<<i<UL_PARAM_NO_MORE_OPTIONS; ++i )
    {
		CString resStr;
        if ( 1<<i & UL_PARAM_UPDATE_MASK )
        {
			resStr.LoadString(IDS_OPFLAGS_OPTIONSTRINGS+i);
            item = m_op_options_ctrl.InsertItem(i, resStr);
            m_op_options_ctrl.SetCheck(item, m_pOpInfo->GetOptions() & 1<<i );
        }
    }

    return m_op_options_ctrl.GetItemCount();
}

int COpUpdateDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    switch (nKey)
    {
        case VK_UP:
            break;
        case VK_DOWN:
            break;
        case VK_DELETE:
                RemoveFileListItem();
                break;
        default:
            break;
    }

    return CDialog::OnVKeyToItem(nKey, pListBox, nIndex);
}



void COpUpdateDlg::OnBnClickedCancel()
{
	m_pOpInfo->m_FileList.RemoveEdits();
	m_pOpInfo->m_drive_array = m_savedDriveArray;

	OnCancel();
}

void COpUpdateDlg::OnBnClickedOk()
{
	CString cs_temp;

    m_pOpInfo->m_ul_param = 0;
    for (int i=0; i<m_op_options_ctrl.GetItemCount(); ++i )
    {
        if ( m_op_options_ctrl.GetCheck(i) )
            m_pOpInfo->m_ul_param |= 1<<i;
    }

	if (  m_pOpInfo->m_ul_param & UL_PARAM_WINCE )
		SetDriveFlags(FALSE);
	else
		SetDriveFlags(TRUE);
/*
    m_op_name_ctrl.GetWindowText(cs_temp);
    if ( cs_temp.IsEmpty() )
    {
	    CString resStr, resTitleStr;

		resStr.LoadString(IDS_INVALID_OP_FOLDER);
		resTitleStr.LoadString(IDS_ERROR);

	    MessageBox(resStr, resTitleStr, MB_OK | MB_ICONERROR);
	    return;
    }

	m_pOpInfo->m_cs_ini_section = cs_temp;
*/
    m_op_name_ctrl.GetWindowText(m_pOpInfo->m_cs_NewName);
    if ( m_pOpInfo->m_cs_NewName.IsEmpty() )
    {
	    CString resStr, resTitleStr;

		resStr.LoadString(IDS_INVALID_OP_FOLDER);
		resTitleStr.LoadString(IDS_ERROR);

	    MessageBox(resStr, resTitleStr, MB_OK | MB_ICONERROR);
	    return;
    }
	else
    // see if _new_name directory already exists
    if ( m_pOpInfo->m_cs_ini_section.CompareNoCase(m_pOpInfo->m_cs_NewName) != 0 )
    {
        // the name has changed, so we need to rename the directory
        // we better check if there is already a directory with the new name
        // and let the user choose what to do.
        CString cs_new_path = m_pOpInfo->m_cs_path.Left(m_pOpInfo->m_cs_path.ReverseFind(_T('\\'))+1) + m_pOpInfo->m_cs_NewName;
        if ( _taccess(cs_new_path, 0) == ERROR_SUCCESS )
        {
            // the directory already exists, so ask the user what to do.
		    CString resStr, resTitleStr, resFmtStr;

			resFmtStr.LoadString(IDS_OP_ALREADY_EXISTS);
			resStr.Format(resFmtStr, m_pOpInfo->m_cs_NewName, cs_new_path);
			resTitleStr.LoadString(IDS_CONFIRM_OVERWRITE);

		    if ( IDCANCEL == MessageBox(resStr, resTitleStr, MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) )
            {
			    return;
		    }
		}
	}
	m_pOpInfo->m_cs_new_ini_section = m_pOpInfo->m_cs_NewName;
    m_pOpInfo->m_cs_desc = COperation::OperationStrings[m_pOpInfo->m_e_type];

	m_pOpInfo->m_FileList.CommitEdits();

	m_pOpInfo->Validate();

    OnOK();
}


void COpUpdateDlg::OnContextMenu(NMHDR *pNMHDR, LRESULT *pResult)
{
	BOOL bCanMoveUp = FALSE;
	BOOL bCanMoveDown = FALSE;
	BOOL bCanEdit = FALSE;
	BOOL bCanDelete = TRUE;
    CMenu menu;
	CMenu* pPopup;
	CWnd* pWndPopupOwner;
	unsigned int iMenuCmd;
	CString csResStr;
	m_current_selection = m_drive_list_ctrl.GetSelectionMark();

	CPoint point;

	::GetCursorPos(&point); //where is the mouse?


    if( m_current_selection != -1 )
	{
		if( m_current_selection <= m_drive_list_ctrl.GetItemCount() )
			bCanEdit = TRUE;
		if( m_current_selection > 0 )
			bCanMoveUp = TRUE;
		if( m_current_selection < m_drive_list_ctrl.GetItemCount() - 1 )
			bCanMoveDown = TRUE;
		if( bCanEdit )
		{
			CDriveEditDlg *pDriveEditor = new CDriveEditDlg(this, FALSE, m_pOpInfo, m_current_selection);
			if( pDriveEditor->CheckForDependents() )
				bCanDelete = FALSE;
			delete pDriveEditor;
		}

	}

	VERIFY(menu.LoadMenu(IDR_UPDATELIST_MENU));

	pPopup = menu.GetSubMenu(0);
	pWndPopupOwner = this;

	if( pPopup )
	{
		csResStr.LoadStringW(IDS_MENU_NEW);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_NEW,
							MF_BYCOMMAND | MF_STRING, IDM_ST_NEW, csResStr);

		csResStr.LoadStringW(IDS_MENU_EDIT);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_EDIT,
							MF_BYCOMMAND | MF_STRING, IDM_ST_EDIT, csResStr);

		csResStr.LoadStringW(IDS_MENU_DELETE);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_DELETE,
							MF_BYCOMMAND | MF_STRING, IDM_ST_DELETE, csResStr);

		csResStr.LoadStringW(IDS_MENU_COPY_DRIVE);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_COPY,
							MF_BYCOMMAND | MF_STRING, IDM_ST_COPY, csResStr);

		csResStr.LoadStringW(IDS_MENU_MOVE_UP);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_MOVE_UP,
							MF_BYCOMMAND | MF_STRING, IDM_ST_MOVE_UP, csResStr);

		csResStr.LoadStringW(IDS_MENU_MOVE_DOWN);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_MOVE_DOWN,
							MF_BYCOMMAND | MF_STRING, IDM_ST_MOVE_DOWN, csResStr);

		if( !bCanEdit )
		{
			pPopup->EnableMenuItem(IDM_ST_EDIT, MF_GRAYED );
			pPopup->EnableMenuItem(IDM_ST_DELETE, MF_GRAYED );
		}
		if( !bCanMoveUp )
			pPopup->EnableMenuItem(IDM_ST_MOVE_UP, MF_GRAYED );
		if( !bCanMoveDown )
			pPopup->EnableMenuItem(IDM_ST_MOVE_DOWN, MF_GRAYED );
		if( !bCanDelete )
			pPopup->EnableMenuItem(IDM_ST_DELETE, MF_GRAYED );


		media::LogicalDriveTag tmpNewTag;
		if( !CanCopy(tmpNewTag) )
			pPopup->EnableMenuItem(IDM_ST_COPY, MF_GRAYED );

	    while (pWndPopupOwner->GetStyle() & WS_CHILD)
	    pWndPopupOwner = pWndPopupOwner->GetParent();

		iMenuCmd = (USHORT) pPopup->TrackPopupMenuEx( TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
												point.x, point.y,
												pWndPopupOwner, NULL);

		if ( iMenuCmd == IDM_ST_NEW )
			OnMenuClickedNew();
		else if ( iMenuCmd == IDM_ST_EDIT )
			OnMenuClickedEdit();
		else if ( iMenuCmd == IDM_ST_DELETE )
			OnMenuClickedDelete();
		else if ( iMenuCmd == IDM_ST_COPY )
			OnMenuClickedCopy();
		else if ( iMenuCmd == IDM_ST_MOVE_UP )
			OnMenuClickedMoveUp();
		else if ( iMenuCmd == IDM_ST_MOVE_DOWN )
			OnMenuClickedMoveDown();
	}
}

void COpUpdateDlg::OnMenuClickedNew()
{
	INT_PTR ret;
	media::LogicalDrive newDrive;

	newDrive.DriveNumber = m_drive_list_ctrl.GetItemCount();
	newDrive.RequestedKB = 0;
	newDrive.Type = media::DriveType_Invalid;
	newDrive.Flags = media::DriveFlag_NoAction;
	newDrive.Tag = media::DriveTag_Invalid;
	newDrive.Description = _T("");
	newDrive.Name = _T("");

	m_pOpInfo->m_drive_array.AddDrive(newDrive);

	CDriveEditDlg *pDriveEditor = new CDriveEditDlg(this, TRUE, m_pOpInfo, newDrive.DriveNumber);
    ret = pDriveEditor->DoModal();
    delete pDriveEditor;

	if( ret == IDOK )
		UpdateListItem(TRUE, newDrive.DriveNumber, m_pOpInfo->m_drive_array[newDrive.DriveNumber]);
	else
		m_pOpInfo->m_drive_array.Remove( newDrive );
}

void COpUpdateDlg::OnMenuClickedEdit()
{
	INT_PTR ret;

	CDriveEditDlg *pDriveEditor = new CDriveEditDlg(this, FALSE, m_pOpInfo, m_current_selection);
    ret = pDriveEditor->DoModal();
    delete pDriveEditor;

	if( ret == IDOK )
		UpdateListItem(FALSE, m_current_selection, m_pOpInfo->m_drive_array[m_current_selection]);
}


void COpUpdateDlg::OnMenuClickedCopy()
{
	media::LogicalDrive drv = m_pOpInfo->m_drive_array[m_current_selection];
	media::LogicalDrive newDrive;
	int copyCount;


	copyCount = CanCopy( newDrive.Tag ); // get a new tag

	if( newDrive.Tag != media::DriveTag_Invalid )
	{
		CFileList::PFILEITEM pCopyFileItem = m_pOpInfo->m_FileList.GetAt(drv.FileListIndex);
		newDrive.DriveNumber = m_drive_list_ctrl.GetItemCount();
		newDrive.RequestedKB = drv.RequestedKB;
		newDrive.Type = drv.Type;
		newDrive.Flags = drv.Flags;
		newDrive.Description = drv.Description;
		newDrive.Name = m_pOpInfo->m_FileList.GetPathNameAt(drv.FileListIndex);
		newDrive.FileListIndex = m_pOpInfo->m_FileList.AddFile(newDrive.Name.c_str(), pCopyFileItem->m_action);

		newDrive.Description.Format(_T("%s (%d)"), drv.Description, copyCount);
		m_pOpInfo->m_drive_array.Insert(m_current_selection+1, newDrive);
		UpdateListItem(TRUE, m_current_selection+1, newDrive);

		for(unsigned int i = m_current_selection+2; i < m_pOpInfo->m_drive_array.Size(); ++i )
			m_pOpInfo->m_drive_array[i].DriveNumber = i;

	}
}

int COpUpdateDlg::CanCopy(media::LogicalDriveTag & _newTag)
{
	BOOL bCanMakeCopy = TRUE;
	
	if( m_current_selection == -1 )
		return FALSE;  // no list items

	media::LogicalDrive drv = m_pOpInfo->m_drive_array[m_current_selection];
	media::LogicalDriveTag currentTag;
	int count = 1;

	currentTag = drv.Tag;

tryAgain:

	switch(currentTag)
	{
	case media::DriveTag_FirmwareImg:
		_newTag = media::DriveTag_FirmwareImg2;
		break;
	case media::DriveTag_FirmwareImg2:
		_newTag = media::DriveTag_FirmwareImg3;
		break;
	case media::DriveTag_FirmwareRsc:
		_newTag = media::DriveTag_FirmwareRsc2;
		break;
	case media::DriveTag_FirmwareRsc2:
		_newTag = media::DriveTag_FirmwareRsc3;
		break;
	case media::DriveTag_HostlinkRsc:
		_newTag = media::DriveTag_HostlinkRsc2;
		break;
	case media::DriveTag_HostlinkRsc2:
		_newTag = media::DriveTag_HostlinkRsc3;
		break;
//	case media::DriveTag_PlayerRsc:
//		_newTag = media::DriveTag_PlayerRsc2;
//		break;
//	case media::DriveTag_PlayerRsc2:
//		_newTag = media::DriveTag_PlayerRsc3;
//		break;
	case media::DriveTag_Data:
		_newTag = media::DriveTag_Data2;
		break;
	case media::DriveTag_DataJanus:
	case media::DriveTag_DataSettings:
	default:
		bCanMakeCopy = FALSE;
		break; // no copy
	}

	if(bCanMakeCopy )
	{	// look for an existing drive of this type
		++count; // times we've bumped
		for (unsigned int i=0; i < m_pOpInfo->m_drive_array.Size(); i++)
		{
			media::LogicalDrive searchDrv = m_pOpInfo->m_drive_array[i];

			if( _newTag == searchDrv.Tag )
			{
				currentTag = _newTag;
				goto tryAgain;
			}
		}
	}

	if( !bCanMakeCopy )
	{
		_newTag = media::DriveTag_Invalid;
		count = 0;
	}

	return count;
}


void COpUpdateDlg::OnMenuClickedDelete()
{
	CFileList::PFILEITEM pFileItem = m_pOpInfo->m_FileList.GetAt(m_pOpInfo->m_drive_array[m_current_selection].FileListIndex);

	if( pFileItem )
	{
		if( pFileItem->m_action != CFileList::EXISTS_IN_TARGET )
			pFileItem->m_action = CFileList::IGNORE_FILEITEM; // have to keep it to avoid changing the fileist indexes
//			m_pOpInfo->m_FileList.RemoveFile(m_pOpInfo->m_drive_array[m_current_selection].FileListIndex);
		else
			pFileItem->m_action = CFileList::DELETE_FROM_TARGET;
	}

	m_pOpInfo->m_drive_array.Remove( m_pOpInfo->m_drive_array[m_current_selection] );

	m_drive_list_ctrl.DeleteItem(m_current_selection);
}

void COpUpdateDlg::OnMenuClickedMoveUp(void)
{
	unsigned int i, nItem;
	media::LogicalDrive drv = m_pOpInfo->m_drive_array[m_current_selection];

	for( i = nItem = m_current_selection-1; i < m_pOpInfo->m_drive_array.Size(); ++i )
		m_drive_list_ctrl.DeleteItem (nItem);

	m_pOpInfo->m_drive_array.Remove( m_pOpInfo->m_drive_array[m_current_selection] );
	m_pOpInfo->m_drive_array.Insert( m_current_selection - 1, drv );
	for( i = m_current_selection-1; i < m_pOpInfo->m_drive_array.Size(); ++i )
		m_pOpInfo->m_drive_array[i].DriveNumber = i;

	for( i = m_current_selection-1; i < m_pOpInfo->m_drive_array.Size(); i++ )
	{
		UpdateListItem(TRUE, i, m_pOpInfo->m_drive_array[i]);
	}
}

void COpUpdateDlg::OnMenuClickedMoveDown(void)
{
	unsigned int i, nItem;
	media::LogicalDrive drv = m_pOpInfo->m_drive_array[m_current_selection];

	for( i = nItem = m_current_selection; i < m_pOpInfo->m_drive_array.Size(); ++i )
		m_drive_list_ctrl.DeleteItem (nItem);

	m_pOpInfo->m_drive_array.Remove( m_pOpInfo->m_drive_array[m_current_selection] );
	m_pOpInfo->m_drive_array.Insert( m_current_selection + 1, drv );
	for( i = m_current_selection; i < m_pOpInfo->m_drive_array.Size(); ++i )
		m_pOpInfo->m_drive_array[i].DriveNumber = i;

	for( i = m_current_selection; i < m_pOpInfo->m_drive_array.Size(); i++ )
	{
		UpdateListItem(TRUE, i, m_pOpInfo->m_drive_array[i]);
	}
}

void COpUpdateDlg::OnBnClickedBrowseFile()
{
    CString copy_buffer;
    CFileDialog *dlg;
	CString resStr;

	resStr.LoadString(IDS_FILE_BROWSE_TYPES);


   	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
							resStr, this);

    if ( IDOK == dlg->DoModal() )
    {
		int index=0;
        CString csFullPathName = dlg->GetPathName();
		index = m_pOpInfo->GetUpdaterBootFilenameIndex();
		if ( index == -1 )
			m_pOpInfo->SetUpdaterBootFilename( csFullPathName, CFileList::IN_EDIT_ADD );
		else
			m_pOpInfo->m_FileList.ChangeFile(index, csFullPathName);

			// set the file name minus the path
		SetDlgItemText(IDC_UPDATE_BOOT_FW_EDIT, csFullPathName);
    }

	delete dlg;
}

void COpUpdateDlg::SetDriveFlags(BOOL _setState)
{
	for( unsigned int i = 0; i < m_pOpInfo->m_drive_array.Size(); ++i )
	{
		if (m_pOpInfo->m_drive_array[i].Type == media::DriveType_HiddenData && 
			m_pOpInfo->m_drive_array[i].Tag == media::DriveTag_DataJanus)
		{
			if ( _setState )
				m_pOpInfo->m_drive_array[i].Flags = media::DriveFlag_JanusInit;
			else
				m_pOpInfo->m_drive_array[i].Flags = media::DriveFlag_NoAction;
		}

		if (m_pOpInfo->m_drive_array[i].Type == media::DriveType_Data)
		{
			if ( _setState )
				m_pOpInfo->m_drive_array[i].Flags = media::DriveFlag_Format;
			else
				m_pOpInfo->m_drive_array[i].Flags =  media::DriveFlag_NoAction;
		}
	}
}
void COpUpdateDlg::Localize()
{
	CString resStr;

	resStr.LoadStringW(IDS_OPERATION_UPDATE);
	SetWindowText(resStr);

	resStr.LoadString(IDS_OP_EDIT_STEP1_GRP);
	SetDlgItemText(IDC_STEP1_GROUP, resStr );

	resStr.LoadString(IDS_OP_EDIT_NAME_OPERATION);
	SetDlgItemText(IDC_UPDATE_STEP1_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_STEP2_GRP);
	SetDlgItemText(IDC_STEP2_GROUP, resStr );

	resStr.LoadString(IDS_DEFINE_TABLE_TEXT);
	SetDlgItemText(IDC_UPDATE_STEP2_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_STEP3_GRP);
	SetDlgItemText(IDC_STEP3_GROUP, resStr );

	resStr.LoadString(IDS_UPDATE_STEP3_TEXT);
	SetDlgItemText(IDC_UPDATE_STEP3_TEXT, resStr);

	resStr.LoadString(IDS_OP_EDIT_STEP4_GRP);
	SetDlgItemText(IDC_STEP4_GROUP, resStr );

	resStr.LoadString(IDS_OP_EDIT_SELECT_OPTIONS);
	SetDlgItemText(IDC_UPDATE_STEP4_TEXT, resStr );

	resStr.LoadString(IDS_OK);
	SetDlgItemText(IDOK, resStr );

	resStr.LoadString(IDS_CANCEL);
	SetDlgItemText(IDCANCEL, resStr );
}



