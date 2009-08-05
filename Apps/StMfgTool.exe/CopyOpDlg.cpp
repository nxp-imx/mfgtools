// FileOpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "CopyOpDlg.h"

#define FOLDER_IMAGE	0
#define FILE_IMAGE		1

#define COLUMN_FILENAME         0
#define COLUMN_SIZE             1
#define COLUMN_DATE             2


// CCopyOpDlg dialog

IMPLEMENT_DYNAMIC(CCopyOpDlg, CDialog)

CCopyOpDlg::CCopyOpDlg(CWnd* pParent /*=NULL*/, COpInfo* pInfo)
	: CDialog(CCopyOpDlg::IDD, pParent)
    , m_p_op_info(pInfo)
	, m_OutPath(_T(""))
    , m_data_drive_num(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

CCopyOpDlg::~CCopyOpDlg()
{
}

void CCopyOpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_OP_TIMEOUT_EDIT, m_op_timeout_ctrl);
    DDX_Control(pDX, IDC_OP_NAME_EDIT, m_op_name_ctrl);
    DDX_Control(pDX, IDC_OP_FILE_LIST, m_op_file_list_ctrl);
    DDX_Control(pDX, IDC_OP_COPY_DRV_NUM_COMBO, m_op_drv_combo_ctrl);
    DDX_Control(pDX, IDC_STATIC_DRIVE_NUM_EDIT_LABEL, m_static_data_drive_num_label);
	DDX_Control(pDX, IDC_OP_FOLDERTREE, m_op_folders_ctrl);
    DDV_MinMaxByte(pDX, m_data_drive_num, 0, 255);
}


BEGIN_MESSAGE_MAP(CCopyOpDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_EN_CHANGE(IDC_OP_NAME_EDIT, OnEnChangeOpNameEdit)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_NOTIFY(NM_RCLICK, IDC_OP_FILE_LIST, &CCopyOpDlg::OnFileListContextMenu)
	ON_NOTIFY(NM_RCLICK, IDC_OP_FOLDERTREE, &CCopyOpDlg::OnFolderListContextMenu)
	ON_CBN_SELCHANGE(IDC_OP_COPY_DRV_NUM_COMBO, &CCopyOpDlg::OnCbnSelchangeOpCopyDrvNumCombo)
	ON_NOTIFY(TVN_SELCHANGED, IDC_OP_FOLDERTREE, &CCopyOpDlg::OnTvnSelchangedOpFoldertree)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_OP_FOLDERTREE, &CCopyOpDlg::OnTvnEndlabeleditOpFoldertree)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_OP_FOLDERTREE, &CCopyOpDlg::OnTvnBeginlabeleditOpFoldertree)
END_MESSAGE_MAP()


// CCopyOpDlg message handlers

BOOL CCopyOpDlg::OnInitDialog()
{
    ASSERT(m_p_op_info);
    DWORD err = ERROR_SUCCESS;

    CDialog::OnInitDialog();

	Localize();

	m_editing_label = FALSE;

	m_op_name_ctrl.SetValidCharSet(CVisValidEdit::SET_DIRPATH);
    m_op_name_ctrl.SetEmptyValid(FALSE);
    m_op_name_ctrl.SetTextLenMinMax(1);
    m_op_name_ctrl.SetEnableLengthCheck();
	m_op_name_ctrl.SetReadOnly(FALSE);

	if( !m_p_op_info->m_cs_new_ini_section.IsEmpty() )
	    m_op_name_ctrl.SetWindowText(m_p_op_info->m_cs_new_ini_section);
	else
	    m_op_name_ctrl.SetWindowText(m_p_op_info->m_cs_ini_section);

	//Initialize the data drive number control
    m_data_drive_num = m_p_op_info->m_data_drive_num;
	m_op_drv_combo_ctrl.InsertString(0, _T("1"));
	m_op_drv_combo_ctrl.InsertString(1, _T("2"));
	if( m_data_drive_num == 2 )
		m_op_drv_combo_ctrl.SelectString(1, _T("2"));
	else
		m_op_drv_combo_ctrl.SelectString(0, _T("1"));

    UpdateData(FALSE);
    // Initialize the Operation Timeout: control
	CString csTimeout;
	csTimeout.Format(_T("%d"), m_p_op_info->m_timeout);
	m_op_timeout_ctrl.SetWindowText(csTimeout);

	m_original_folder = m_p_op_info->GetPath();

	CBitmap cBmp;
	// Load the small icons; DIR_IMAGE = 0, FILE_IMAGE = 1
	m_image_list.Create(16,16,ILC_COLOR16,2,10);
	cBmp.LoadBitmapW(MAKEINTRESOURCE(IDB_CLOSED_FOLDER));
	m_image_list.Add(&cBmp, (CBitmap *)NULL);
	cBmp.DeleteObject();
	cBmp.LoadBitmapW(MAKEINTRESOURCE(IDB_FIRMWARE_FILE));
	m_image_list.Add(&cBmp, (CBitmap *)NULL);
	m_op_file_list_ctrl.SetImageList(&m_image_list, LVSIL_SMALL);
	m_op_file_list_ctrl.SetImageList(&m_image_list, LVSIL_NORMAL);

	// initialize the file list control size and column names
	RECT listRect;
	CString resStr;
	m_op_file_list_ctrl.GetClientRect(&listRect);
	int nListWidth = (listRect.right - listRect.left);
	int nVScrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL); 
	
	int nCol1Width = (int) ((double)nListWidth * 0.30);  // name
    int nCol2Width = (int) ((double)nListWidth * 0.20);  // size
    int nCol3Width = nListWidth - nCol1Width - nCol2Width - nVScrollBarWidth;  // date

	resStr.LoadStringW(IDS_COLUMN_FILENAME);
	m_op_file_list_ctrl.InsertColumn(COLUMN_FILENAME, resStr, LVCFMT_LEFT, nCol1Width);
	resStr.LoadStringW(IDS_COLUMN_SIZE);
	m_op_file_list_ctrl.InsertColumn(COLUMN_SIZE, resStr, LVCFMT_LEFT, nCol2Width);
	resStr.LoadStringW(IDS_COLUMN_TIMESTAMP);
	m_op_file_list_ctrl.InsertColumn(COLUMN_DATE, resStr, LVCFMT_LEFT, nCol3Width);


	//
	// Set full row selection
	//
	m_op_file_list_ctrl.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    //
    // Set the drop mode for the folder tree
    //
    CStGrpTreeCtrl::DROPTREEMODE dropTreeMode;

    dropTreeMode.pfnCallback = CCopyOpDlg::OnTreeFileDropped;
	dropTreeMode.pCallerClass = this;
    dropTreeMode.iMask = CStGrpTreeCtrl::DL_ACCEPT_FILES | 
		CStGrpTreeCtrl::DL_ACCEPT_FOLDERS |
		CStGrpTreeCtrl::DL_USE_CALLBACK;
	m_op_folders_ctrl.EnableWindow(TRUE);
    m_op_folders_ctrl.SetDropMode(dropTreeMode);
	m_op_folders_ctrl.DragAcceptFiles(TRUE);

	PopulateCopyOpFolderTree();
//	m_op_folders_ctrl.SelectItem(m_hRoot);	

	//
    // Set the drop mode for the file list
    //
    CFileDropListCtrl::DROPLISTMODE dropMode;

    dropMode.iMask = CFileDropListCtrl::DL_ACCEPT_FILES | 
		CFileDropListCtrl::DL_ACCEPT_FOLDERS |
		CFileDropListCtrl::DL_USE_CALLBACK;

	dropMode.csFileExt = _T("");
    dropMode.pfnCallback = CCopyOpDlg::OnListFileDropped;
	dropMode.pCallerClass = this;
	m_op_file_list_ctrl.EnableWindow(TRUE);
    m_op_file_list_ctrl.SetDropMode(dropMode);
	m_op_file_list_ctrl.DragAcceptFiles(TRUE);

	m_op_numobjects = m_p_op_info->m_FileList.GetTotalCount();
	resStr.Format(IDS_NUM_OBJECTS, m_op_numobjects);
	SetDlgItemText (IDC_OP_COPY_NUMOBJECTS, resStr);

	return TRUE;
	// return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CCopyOpDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
		CDialog::OnSysCommand(nID, lParam);
}

void CCopyOpDlg::OnPaint() 
{
	CDialog::OnPaint();
}

void CCopyOpDlg::PopulateCopyOpFolderTree()
{
	CString resStr;

	// Set the style
	DWORD dwStyle = GetWindowLong(m_op_folders_ctrl.GetSafeHwnd(), GWL_STYLE);
	dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
	SetWindowLong(m_op_folders_ctrl.GetSafeHwnd(), GWL_STYLE, dwStyle);

	// Clear the list
	m_op_folders_ctrl.DeleteAllItems();

	// create image list and put it into the tree control
	CBitmap bMap;
	m_folder_image_list.Create(16,16,ILC_COLOR16,2,10);
	bMap.LoadBitmap(MAKEINTRESOURCE(IDB_CLOSED_FOLDER));
	m_folder_image_list.Add(&bMap, (CBitmap *)NULL);
	bMap.DeleteObject();

	bMap.LoadBitmap(MAKEINTRESOURCE(IDB_OPEN_FOLDER));
	m_folder_image_list.Add(&bMap, (CBitmap *)NULL);
	bMap.DeleteObject();

	m_op_folders_ctrl.SetImageList(&m_folder_image_list, LVSIL_SMALL);
	m_op_folders_ctrl.SetImageList(&m_folder_image_list, LVSIL_NORMAL);

	// Create the root level
	m_op_name_ctrl.GetWindowTextW(resStr);
	if (resStr.IsEmpty())
		resStr.LoadStringW(IDS_OP_NAME_FOLDER);

	m_hRoot=(HTREEITEM)m_op_folders_ctrl.InsertItem(resStr, 0, 1);
	m_op_folders_ctrl.SetItemData(m_hRoot, (DWORD_PTR) &m_p_op_info->m_FileList);
	m_op_folders_ctrl.Select(m_hRoot, TVGN_CARET);

	// now populate the tree with folders for the operation
	InsertSubFolders(m_hRoot, (CFileList *)&m_p_op_info->m_FileList);

//	m_op_folders_ctrl.SelectItem(m_hRoot);
}

void CCopyOpDlg::InsertSubFolders(HTREEITEM _hParent, CFileList * _pFileList)
{
	for( int i = 0; i < _pFileList->GetCount(); ++i)
	{
		CFileList::PFILEITEM pItem = _pFileList->GetAt(i);

		if (pItem->m_pSubFolderList)
		{
			HTREEITEM hItem =(HTREEITEM)m_op_folders_ctrl.InsertItem(pItem->m_csFileName, 0, 1, _hParent);
			m_op_folders_ctrl.SetItemData(hItem, (DWORD_PTR) pItem->m_pSubFolderList);
			InsertSubFolders(hItem, pItem->m_pSubFolderList);
		}
	}
}

void CCopyOpDlg::InitializeFileList(CFileList * _pFileList)
{
	int index = 0;

	m_op_file_list_ctrl.DeleteAllItems();

	for (int i = 0; i < _pFileList->GetCount(); ++i)
	{
		CFileList::PFILEITEM pItem = _pFileList->GetAt(i);
		if (pItem->m_action != CFileList::DELETE_FROM_TARGET &&
			pItem->m_action != CFileList::IN_EDIT_DELETE &&
			(pItem->m_action != CFileList::CREATE_DIR || pItem->m_action != CFileList::IN_EDIT_CREATE_DIR) &&
			!(pItem->m_dwAttr &  FILE_ATTRIBUTE_DIRECTORY) )
		{
			AppendFileListItem(pItem);
		}
	}
}


// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCopyOpDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//
// Callback from CResGrpTreeCtrl
//
HRESULT CCopyOpDlg::OnTreeFileDropped(CTreeCtrl* pTree,
								     PVOID pCallerClass,
                                     CString& csPathname,
                                     UINT& iPathType)
{
	HRESULT rc = S_OK;
	CCopyOpDlg *pThisClass = ( CCopyOpDlg * )pCallerClass;
	HTREEITEM hDropItem = pTree->GetDropHilightItem();
	HTREEITEM hCurrent = pTree->GetSelectedItem();
	if ( hDropItem && hDropItem != hCurrent )
	{
		pTree->SelectItem(hDropItem);
	}
	rc = pThisClass->OnListFileDropped(pCallerClass, csPathname, iPathType);
	//pThisClass->m_op_folders_ctrl.SetItemState(hDropItem, 0, TVIS_DROPHILITED);


	return S_OK;
}

//
// Callback from CFileDropListCtrl
//
HRESULT CCopyOpDlg::OnListFileDropped(PVOID pCallerClass,
                                     CString& csPathname,
                                     UINT& iPathType)
{
	CCopyOpDlg *pThisClass = ( CCopyOpDlg * )pCallerClass;
	USHORT index=0;
	CFileList::PFILEITEM pItem;

    if (iPathType == CFileDropListCtrl::DL_FILE_TYPE)
    {
		HTREEITEM hCurrent = pThisClass->m_op_folders_ctrl.GetSelectedItem();
		CFileList *pFileList = (CFileList *)pThisClass->m_op_folders_ctrl.GetItemData(hCurrent);
		
		index = pFileList->AddFile(csPathname, CFileList::IN_EDIT_ADD);
		pItem = pFileList->GetAt(index);
		pThisClass->AppendFileListItem(pItem);
	}
	if (iPathType == CFileDropListCtrl::DL_FOLDER_TYPE)
	{
		pThisClass->InsertFolder(csPathname);
	}

    return S_OK;
}

void CCopyOpDlg::InsertFolder(CString _path)
{
	HTREEITEM hCurrent;
	CFileList *pFileList;
	CFileList::PFILEITEM pItem;
	int index;
	HTREEITEM hDropItem = m_op_folders_ctrl.GetDropHilightItem();

	hCurrent = m_op_folders_ctrl.GetSelectedItem();
/*	if ( hDropItem && hDropItem != hCurrent )
	{
		m_op_folders_ctrl.SetItemState(hCurrent, 0, TVIS_DROPHILITED);
		m_op_folders_ctrl.SelectItem(hDropItem);
		hCurrent = m_op_folders_ctrl.GetSelectedItem();
		m_op_folders_ctrl.SetItemState(hCurrent, TVIS_DROPHILITED, TVIS_DROPHILITED);
	}
*/
	pFileList = (CFileList *)m_op_folders_ctrl.GetItemData(hCurrent);

	pItem = pFileList->FindFile(_path, index);
	if( pItem == NULL )
	{	// new folder item, add item to parent list, allocate new list and enumerate
		index = pFileList->AddFile(_path, CFileList::IN_EDIT_CREATE_DIR);
		pItem = pFileList->GetAt(index);
		CFileList *pNewFileList = (CFileList *) new CFileList( _path );
		pItem->m_pSubFolderList = pNewFileList;
		EnumerateSourceFolderFiles( pNewFileList );

		// insert new folder into the tree
		HTREEITEM hItem =(HTREEITEM)m_op_folders_ctrl.InsertItem(pItem->m_csFileName, 0, 1, hCurrent);
		m_op_folders_ctrl.SetItemData(hItem, (DWORD_PTR) pItem->m_pSubFolderList);

		// enumerate subfolders inserting into the folder tree
		InsertSubFolders(hItem, pNewFileList);
	}
	else
		MessageBeep(-1);
}

void CCopyOpDlg::OnBnClickedOk()
{
	if( m_editing_label )
	{
		m_editing_label = FALSE;
		return;  // reject; user hit "Enter" to end label edit
	}

    m_p_op_info->m_data_drive_num = m_data_drive_num;

    // Initialize the Operation Timeout: control
	CString csTimeout;
	m_op_timeout_ctrl.GetWindowText(csTimeout);
	m_p_op_info->m_timeout = _tstoi(csTimeout);

    m_p_op_info->m_cs_desc = COperation::OperationStrings[m_p_op_info->m_e_type];

	m_p_op_info->m_ul_param = 0;

    m_op_name_ctrl.GetWindowText(m_p_op_info->m_cs_NewName);
    if ( m_p_op_info->m_cs_NewName.IsEmpty() )
    {
	    CString resStr, resTitleStr;

		resStr.LoadString(IDS_INVALID_OP_FOLDER);
		resTitleStr.LoadString(IDS_ERROR);

	    MessageBox(resStr, resTitleStr, MB_OK | MB_ICONERROR);
	    return;
    }
	else
    // see if _new_name directory already exists
    if ( m_p_op_info->m_cs_ini_section.CompareNoCase(m_p_op_info->m_cs_NewName) != 0 )
    {
        // the name has changed, so we need to rename the directory
        // we better check if there is already a directory with the new name
        // and let the user choose what to do.
        CString cs_new_path = m_p_op_info->m_cs_path.Left(m_p_op_info->m_cs_path.ReverseFind(_T('\\'))+1) + m_p_op_info->m_cs_NewName;
        if ( _taccess(cs_new_path, 0) == ERROR_SUCCESS )
        {
            // the directory already exists, so ask the user what to do.
		    CString resStr, resTitleStr, resFmtStr;

			resFmtStr.LoadString(IDS_OP_ALREADY_EXISTS);
			resStr.Format(resFmtStr, m_p_op_info->m_cs_NewName, cs_new_path);
			resTitleStr.LoadString(IDS_CONFIRM_OVERWRITE);

		    if ( IDCANCEL == MessageBox(resStr, resTitleStr, MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) )
            {
			    return;
		    }
		}
	}
	m_p_op_info->m_cs_new_ini_section = m_p_op_info->m_cs_NewName;
	if (m_p_op_info->m_FileList.GetCount())
		m_p_op_info->m_status = OPINFO_OK;

	m_p_op_info->m_FileList.CommitEdits();

    // call the default handler
    OnOK();
}

void CCopyOpDlg::OnBnClickedCancel()
{
	// Remove any IN_EDIT action files
	m_p_op_info->m_FileList.RemoveEdits();

	OnCancel();

}




void CCopyOpDlg::OnEnChangeOpNameEdit()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}


void CCopyOpDlg::OnEnChangeEditDataDriveNum()
{
    UpdateData();
}



bool CCopyOpDlg::IsFileExist(CString sPathName)
{
	HANDLE hFile;

	hFile = CreateFile(sPathName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
		return false;
	
	CloseHandle(hFile);
	return true;
}



void CCopyOpDlg::OnFileListContextMenu(NMHDR *pNMHDR, LRESULT *pResult)
{
    CMenu menu;
	CMenu* pPopup;
	CWnd* pWndPopupOwner;
	unsigned int iMenuCmd;
	CString csResStr;
	CPoint point;

	::GetCursorPos(&point); //where is the mouse?


	VERIFY(menu.LoadMenu(IDR_FILEOPLIST_MENU));

	pPopup = menu.GetSubMenu(0);
	pWndPopupOwner = this;

	if( pPopup )
	{
		csResStr.LoadStringW(IDS_OP_EDIT_ADD_FILE);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_NEW,
							MF_BYCOMMAND | MF_STRING, IDM_ST_NEW, csResStr);

		csResStr.LoadStringW(IDS_MENU_DELETE);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_DELETE,
							MF_BYCOMMAND | MF_STRING, IDM_ST_DELETE, csResStr);

		if (!m_p_op_info->m_FileList.GetTotalCount())
			pPopup->EnableMenuItem(IDM_ST_DELETE, MF_GRAYED ); // nothing to delete

	    while (pWndPopupOwner->GetStyle() & WS_CHILD)
	    pWndPopupOwner = pWndPopupOwner->GetParent();

		iMenuCmd = (USHORT) pPopup->TrackPopupMenuEx( TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
												point.x, point.y,
												pWndPopupOwner, NULL);

		if ( iMenuCmd == IDM_ST_NEW )
			OnMenuClickedAdd();
		else if ( iMenuCmd == IDM_ST_DELETE )
			OnMenuClickedDelete();

		m_op_numobjects = m_p_op_info->m_FileList.GetTotalCount();
		csResStr.Format(IDS_NUM_OBJECTS, m_op_numobjects);
		SetDlgItemText (IDC_OP_COPY_NUMOBJECTS, csResStr);
	}
}


void CCopyOpDlg::OnFolderListContextMenu(NMHDR *pNMHDR, LRESULT *pResult)
{
    CMenu menu;
	CMenu* pPopup;
	CWnd* pWndPopupOwner;
	unsigned int iMenuCmd;
	CString csResStr;

	CPoint point;

	::GetCursorPos(&point); //where is the mouse?


	VERIFY(menu.LoadMenu(IDR_FOLDEROPLIST_MENU));

	pPopup = menu.GetSubMenu(0);
	pWndPopupOwner = this;

	if( pPopup )
	{
		csResStr.LoadStringW(IDS_NEW_FOLDER);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_NEW,
							MF_BYCOMMAND | MF_STRING, IDM_ST_NEW, csResStr);

		csResStr.LoadStringW(IDS_ADD_FOLDER);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_ADD_DIR,
							MF_BYCOMMAND | MF_STRING, IDM_ST_ADD_DIR, csResStr);

		csResStr.LoadStringW(IDS_MENU_DELETE);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_ST_DELETE,
							MF_BYCOMMAND | MF_STRING, IDM_ST_DELETE, csResStr);


	    while (pWndPopupOwner->GetStyle() & WS_CHILD)
	    pWndPopupOwner = pWndPopupOwner->GetParent();

		iMenuCmd = (USHORT) pPopup->TrackPopupMenuEx( TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
												point.x, point.y,
												pWndPopupOwner, NULL);

		if ( iMenuCmd == IDM_ST_NEW )
			OnMenuClickedNewDirectory();
		else if ( iMenuCmd == IDM_ST_DELETE )
			OnMenuClickedDeleteDirectory();
		else if ( iMenuCmd == IDM_ST_ADD_DIR )
			OnMenuClickedAddDirectory();

		m_op_numobjects = m_p_op_info->m_FileList.GetTotalCount();
		csResStr.Format(IDS_NUM_OBJECTS, m_op_numobjects);
		SetDlgItemText (IDC_OP_COPY_NUMOBJECTS, csResStr);
	}
}


void CCopyOpDlg::OnMenuClickedAdd()
{
	CString resStr;
    CFileDialog *dlg;
	HTREEITEM hParent;
	CFileList *pFileList;

	hParent = m_op_folders_ctrl.GetSelectedItem();
	pFileList = (CFileList *)m_op_folders_ctrl.GetItemData(hParent);

	resStr.LoadString(IDS_FILE_BROWSE_TYPES);
   	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST,
							resStr, this);

    if ( IDOK == dlg->DoModal() )
    {
        CString csPathName = dlg->GetPathName();
        POSITION startPosition = dlg->GetStartPosition();
		USHORT index=0;
		CFileList::PFILEITEM pItem;

        while (startPosition)
        {
            CString csFullPathName = dlg->GetNextPathName(startPosition);
			index = pFileList->AddFile(csFullPathName, CFileList::IN_EDIT_ADD);
			pItem = pFileList->GetAt(index);
			AppendFileListItem(pItem);
        }
    }

	delete dlg;
}


void CCopyOpDlg::OnMenuClickedDelete()
{
    CString cs_temp;
	CFileList::PFILEITEM pItem;

    // Get the indexes of all the selected items.
    int num_sel_files = m_op_file_list_ctrl.GetSelectedCount();

    if ( num_sel_files == 0 ) {
		CString resStr, resTitleStr;
		resStr.LoadString(IDS_NOTHING_TO_REMOVE);
		resTitleStr.LoadString(IDS_OP_EDIT_FILE_REMOVE);
        MessageBox(resStr, resTitleStr, MB_ICONINFORMATION | MB_OK);
        return;
    }

	POSITION pos = m_op_file_list_ctrl.GetFirstSelectedItemPosition();

	while (pos)
	{
		int nItem = m_op_file_list_ctrl.GetNextSelectedItem(pos);
		pItem = m_p_op_info->m_FileList.GetAt(nItem);
		if (pItem)
		{

			if (pItem->m_action == CFileList::EXISTS_IN_TARGET)
			{
				m_op_file_list_ctrl.DeleteItem(nItem);
				pItem->m_action = CFileList::IN_EDIT_DELETE;
			}
			else
			{
				m_p_op_info->m_FileList.RemoveFile(nItem);
				m_op_file_list_ctrl.DeleteItem(nItem);
			}
		}
    }
}


void CCopyOpDlg::OnMenuClickedNewDirectory()
{
	CString resStr;
	HTREEITEM parentItem = m_op_folders_ctrl.GetSelectedItem();
	HTREEITEM newItem;

	resStr.LoadStringW(IDS_NEW_DIR);
	newItem = m_op_folders_ctrl.InsertItem(resStr, 0, 1, parentItem);
	UpdateData(FALSE);
	m_op_folders_ctrl.SetFocus();
	m_op_folders_ctrl.SelectItem(newItem);
	m_op_folders_ctrl.EditLabel(newItem);
}

void CCopyOpDlg::OnMenuClickedAddDirectory()
{
    TCHAR path[MAX_PATH];
    BROWSEINFO bi = { 0 };
	CString resStr;

	m_OutPath=m_p_op_info->m_cs_path  + _T("\\");

	// jwe Could of used this for both dirs & files
	//bi.ulFlags=BIF_BROWSEINCLUDEFILES;
	bi.ulFlags = BIF_USENEWUI;

	resStr.LoadString(IDS_SELECT_DIRECTORY);
	bi.lpszTitle = resStr;
    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );

    if ( pidl != 0 )
    {
		HTREEITEM hCurrent;
		CFileList *pFileList;
		CFileList::PFILEITEM pItem;
		int index;
        // get the name of the folder and put it in path
        SHGetPathFromIDList ( pidl, path );

		hCurrent = m_op_folders_ctrl.GetSelectedItem();
		pFileList = (CFileList *)m_op_folders_ctrl.GetItemData(hCurrent);

		pItem = pFileList->FindFile(path, index);
		if( pItem == NULL )
		{	// new folder item, add item to parent list, allocate new list and enumerate
			index = pFileList->AddFile(path, CFileList::IN_EDIT_CREATE_DIR);
			pItem = pFileList->GetAt(index);
			CFileList *pNewFileList = (CFileList *) new CFileList( path );
			pItem->m_pSubFolderList = pNewFileList;
			EnumerateSourceFolderFiles( pNewFileList );

			// insert new folder into the tree
			HTREEITEM hItem =(HTREEITEM)m_op_folders_ctrl.InsertItem(pItem->m_csFileName, 0, 1, hCurrent);
			m_op_folders_ctrl.SetItemData(hItem, (DWORD_PTR) pItem->m_pSubFolderList);

			// enumerate subfolders inserting into the folder tree
			InsertSubFolders(hItem, pNewFileList);
		}
		else
			MessageBeep(-1);
    }
}



void CCopyOpDlg::OnMenuClickedDeleteDirectory()
{
    CString cs_temp;
	CFileList::PFILEITEM pItem;

    // Get the indexes of all the selected items.
    int num_sel_files = m_op_file_list_ctrl.GetSelectedCount();

    if ( num_sel_files == 0 ) {
		CString resStr, resTitleStr;
		resStr.LoadString(IDS_NOTHING_TO_REMOVE);
		resTitleStr.LoadString(IDS_OP_EDIT_FILE_REMOVE);
        MessageBox(resStr, resTitleStr, MB_ICONINFORMATION | MB_OK);
        return;
    }

	POSITION pos = m_op_file_list_ctrl.GetFirstSelectedItemPosition();

	while (pos)
	{
		int nItem = m_op_file_list_ctrl.GetNextSelectedItem(pos);
		pItem = m_p_op_info->m_FileList.GetAt(nItem);
		if (pItem)
		{

			if (pItem->m_action == CFileList::EXISTS_IN_TARGET)
			{
				m_op_file_list_ctrl.DeleteItem(nItem);
				pItem->m_action = CFileList::IN_EDIT_DELETE;
			}
			else
			{
				m_p_op_info->m_FileList.RemoveFile(nItem);
				m_op_file_list_ctrl.DeleteItem(nItem);
			}
		}
    }
}

int CCopyOpDlg::CheckPath(CString sPath)
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

void CCopyOpDlg::Localize()
{
	CString resStr;

	if ( m_p_op_info->GetType() == COperation::COPY_OP )
		resStr.LoadStringW(IDS_OPERATION_COPY);
	if ( m_p_op_info->GetType() == COperation::LOADER_OP )
		resStr.LoadStringW(IDS_OPERATION_LOAD);

	SetWindowText(resStr);

	resStr.LoadString(IDS_OP_EDIT_STEP1_GRP);
	SetDlgItemText(IDC_STEP1_GROUP, resStr );

	resStr.LoadString(IDS_OP_EDIT_NAME_OPERATION);
	SetDlgItemText(IDC_STEP1_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_STEP2_GRP);
	SetDlgItemText(IDC_STEP2_GROUP, resStr );

	resStr.LoadString(IDS_OP_COPY_STEP2);
	SetDlgItemText(IDC_STEP2_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_STEP3_GRP);
	SetDlgItemText(IDC_STEP3_GROUP, resStr );

	resStr.LoadString(IDS_OP_EDIT_SELECT_OPTIONS);
	SetDlgItemText(IDC_STEP3_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_TIMEOUT);
	SetDlgItemText(IDC_OP_TIMEOUT_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_DRIVE_NUM);
	SetDlgItemText(IDC_STATIC_DRIVE_NUM_EDIT_LABEL, resStr );

	resStr.LoadString(IDS_OK);
	SetDlgItemText(IDOK, resStr );

	resStr.LoadString(IDS_CANCEL);
	SetDlgItemText(IDCANCEL, resStr );
}



void CCopyOpDlg::OnCbnSelchangeOpCopyDrvNumCombo()
{
	m_data_drive_num = m_op_drv_combo_ctrl.GetCurSel()+1;
}

void CCopyOpDlg::OnTvnSelchangedOpFoldertree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	if (pNMTreeView->itemOld.hItem)
		m_op_folders_ctrl.SetItemState(pNMTreeView->itemOld.hItem, 0, TVIS_DROPHILITED);
	m_op_folders_ctrl.SetItemState(pNMTreeView->itemNew.hItem, TVIS_DROPHILITED, TVIS_DROPHILITED);

	CFileList * pFileList = (CFileList *) m_op_folders_ctrl.GetItemData(pNMTreeView->itemNew.hItem);
	m_op_file_list_ctrl.DeleteAllItems();

	if( pFileList )
		InitializeFileList( pFileList );

	*pResult = 0;
}

void CCopyOpDlg::AppendFileListItem(CFileList::PFILEITEM pItem)
{
	int image;
	int index = m_op_file_list_ctrl.GetItemCount();
	CString csTemp;

	if (pItem->m_dwAttr & FILE_ATTRIBUTE_DIRECTORY)
		image = FOLDER_IMAGE;
	else
		image = FILE_IMAGE;


	if ( !(pItem->m_dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
	{
		SYSTEMTIME sysTime;
//SHFILEINFO sfi;
//SHGetFileInfo( pItem->m_csFilePathName, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);



		if (pItem->m_action == CFileList::COPY_TO_TARGET || pItem->m_action == CFileList::IN_EDIT_ADD)
			m_op_file_list_ctrl.InsertItem(index, pItem->m_csFilePathName, image);
		else  // EXISTS_IN_TARGET
			m_op_file_list_ctrl.InsertItem(index, pItem->m_csFileName, image);

		m_op_file_list_ctrl.SetItemData(index, 0);
		csTemp.Format(_T("%d"), pItem->m_dwFileSize);
		m_op_file_list_ctrl.SetItemText(index, COLUMN_SIZE, csTemp);

	    if ( FileTimeToSystemTime(&pItem->m_timestamp, &sysTime) )
		{
			WCHAR wszTime[32];
			WCHAR wszDate[32];
			WCHAR wszFmt[32];

			GetLocaleInfo( LOCALE_USER_DEFAULT,				// locale identifier
							LOCALE_STIMEFORMAT,					// information type
							(LPWSTR)wszFmt,		// information buffer
							32);								// size of buffer
			GetTimeFormatW(  LOCALE_USER_DEFAULT, 0, &sysTime, wszFmt,
									wszTime,         // formatted string buffer
									32);

			GetLocaleInfo( LOCALE_USER_DEFAULT,				// locale identifier
							LOCALE_SSHORTDATE,					// information type
							(LPWSTR)wszFmt,		// information buffer
							32);								// size of buffer
			GetDateFormatW(  LOCALE_USER_DEFAULT, 0, &sysTime, wszFmt,
									wszDate,         // formatted string buffer
									32);

			csTemp.Format(L"%s  %s", wszDate, wszTime);
               m_op_file_list_ctrl.SetItemText(index, COLUMN_DATE, csTemp);
		}
	}
//	else
//		m_op_file_list_ctrl.SetItemData(index, (DWORD_PTR)pItem->m_pSubFolderList);
}

void CCopyOpDlg::OnTvnEndlabeleditOpFoldertree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	if( pTVDispInfo->item.hItem != m_hRoot && pTVDispInfo->item.pszText )
	{
		CFileList *pFileList;
		int index;
		CFileList::PFILEITEM pItem;
		CString csNewPath;

		m_op_folders_ctrl.SetFocus();
		m_op_folders_ctrl.SelectItem( pTVDispInfo->item.hItem );
		pFileList = (CFileList *)m_op_folders_ctrl.GetItemData(m_op_folders_ctrl.GetParentItem(pTVDispInfo->item.hItem));
		csNewPath.Format(_T("%s\\%s"), pFileList->m_csRootPath, pTVDispInfo->item.pszText);
		index = pFileList->AddFile( csNewPath, CFileList::IN_EDIT_CREATE_DIR);
		pItem = pFileList->GetAt( index );
		pItem->m_pSubFolderList = new CFileList( csNewPath );
		m_op_folders_ctrl.SetItemData( pTVDispInfo->item.hItem, (DWORD_PTR)pItem->m_pSubFolderList);
		*pResult = TRUE;	
	}
	else
		*pResult = 0;
}

void CCopyOpDlg::OnTvnBeginlabeleditOpFoldertree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LRESULT result = 0;

	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	if( pTVDispInfo->item.hItem != m_hRoot )
		m_editing_label = TRUE;
	else
		result = TRUE;
	
	*pResult = result;
}


void CCopyOpDlg::EnumerateSourceFolderFiles(CFileList *pFileList)
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
				
				if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
					index = pFileList->AddFile(csNewFile, CFileList::IN_EDIT_CREATE_DIR);
				else
					index = pFileList->AddFile(csNewFile, CFileList::IN_EDIT_ADD);

				CFileList::PFILEITEM pItem = pFileList->GetAt(index);

				if ( pItem->m_dwAttr & FILE_ATTRIBUTE_DIRECTORY )
				{
					// if a folder, allocate new list and enumerate
					pItem->m_pSubFolderList = new CFileList( csNewFile );
					EnumerateSourceFolderFiles( pItem->m_pSubFolderList );
				}
			}

			if (!FindNextFile(hFind, &FindFileData))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES) 
					fFinished = TRUE; 
				else
				{
					FindClose(hFind);
					return; 
				} 
			}
		} // done making a file list
	} // end if (HANDLE)
	FindClose(hFind);

	return;
}