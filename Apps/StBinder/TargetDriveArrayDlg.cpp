// TargetDriveArrayDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StBinder.h"
#include "DriveEditDlg.h"
#include "TargetDriveArrayDlg.h"

extern TARGET_CFG_DATA g_ResCfgData;

// CTargetDriveArrayDlg dialog

IMPLEMENT_DYNAMIC(CTargetDriveArrayDlg, CPropertyPage)

CTargetDriveArrayDlg::CTargetDriveArrayDlg(/*CWnd* pParent =NULL*/)
	: CPropertyPage(CTargetDriveArrayDlg::IDD)
{

}

CTargetDriveArrayDlg::~CTargetDriveArrayDlg()
{
}

void CTargetDriveArrayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TARGET_DRIVEARRAY, m_drive_list_ctrl);
}


BEGIN_MESSAGE_MAP(CTargetDriveArrayDlg, CDialog)
    ON_WM_VKEYTOITEM()
	ON_NOTIFY(NM_RCLICK, IDC_TARGET_DRIVEARRAY, &CTargetDriveArrayDlg::OnContextMenu)
    ON_BN_CLICKED(IDOK, OnOK)
    ON_BN_CLICKED(IDCANCEL, OnCancel)
END_MESSAGE_MAP()


BOOL CTargetDriveArrayDlg::OnInitDialog()
{
	CString resStr;

	CPropertyPage::OnInitDialog();

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DRIVE_ARRAY_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_DRIVEARRAY_TEXT, resStr );

	m_drive_array = g_ResCfgData.Options.DriveArray;
	InitFileList();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// CTargetDriveArrayDlg message handlers

void CTargetDriveArrayDlg::OnOK()
{
	g_ResCfgData.Options.DriveArray = m_drive_array;
	CPropertyPage::OnOK();
}

void CTargetDriveArrayDlg::OnCancel()
{
	CPropertyPage::OnCancel();
}

//-------------------------------------------------------------------
// InitFileList()
//
// Initialize the file list control with the files contained in the
// drive array.
//-------------------------------------------------------------------
void CTargetDriveArrayDlg::InitFileList()
{
	RECT listRect;
	CString resStr;
	int listIndex;
	m_drive_list_ctrl.GetClientRect(&listRect);
	int nListWidth = (listRect.right - listRect.left);
	int nVScrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL); 
	
	int nCol1Width = (int) ((double)nListWidth * 0.17);  // file name
    int nCol2Width = (int) ((double)nListWidth * 0.30);  // description
    int nCol3Width = (int) ((double)nListWidth * 0.16);  // drive type i.e. "System", "Data", etc...
    int nCol4Width = (int) ((double)nListWidth * 0.10);  // drive tag
														 // requested size
    int nCol5Width = nListWidth - nCol1Width - nCol2Width - nCol3Width - nCol4Width - nVScrollBarWidth;  // date

	resStr.LoadStringW(IDS_COLUMN_FILE_NAME);
	m_drive_list_ctrl.InsertColumn(0, resStr, LVCFMT_LEFT, nCol1Width);
	resStr.LoadStringW(IDS_COLUMN_FILE_DESC);
	m_drive_list_ctrl.InsertColumn(1, resStr, LVCFMT_LEFT, nCol2Width);
	resStr.LoadStringW(IDS_COLUMN_FILE_TYPE);
	m_drive_list_ctrl.InsertColumn(2, resStr, LVCFMT_LEFT, nCol3Width);
	resStr.LoadStringW(IDS_COLUMN_FILE_TAG);
	m_drive_list_ctrl.InsertColumn(3, resStr, LVCFMT_LEFT, nCol4Width);
	resStr.LoadStringW(IDS_COLUMN_FILE_RESERVE);
	m_drive_list_ctrl.InsertColumn(4, resStr, LVCFMT_LEFT, nCol5Width);

	m_drive_list_ctrl.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	listIndex = 0;
	for (unsigned int i=0; i < m_drive_array.Size(); ++i)
	{
		if (m_drive_array[i].Tag != media::DriveTag_Updater)
		{
			UpdateListItem(TRUE, listIndex, m_drive_array[i]);
			++listIndex;
		}
	}

}


void CTargetDriveArrayDlg::UpdateListItem(BOOL _bInsert, int _index, const media::LogicalDrive& _driveDesc)
{
	CString itemStr;

	itemStr = _driveDesc.Name.c_str();
		
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

int CTargetDriveArrayDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
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

//-------------------------------------------------------------------
// UpdateControls()
//
// Update controls with values from the selected item in the file list.
//-------------------------------------------------------------------

void CTargetDriveArrayDlg::UpdateControls()
{
	media::LogicalDrive drvDesc;
	CString strTmp;
    CString curSelStr;
    int curPos = 0;
	int curSel = m_drive_list_ctrl.GetSelectionMark();
    _TCHAR* stopStr = _T(",");

    if (curSel != -1)
    {
		drvDesc = m_drive_array[curSel];
		m_current_selection = curSel;

		UpdateData(FALSE);
    }

}

//-------------------------------------------------------------------
// RemoveFileListItem()
//
// Remove selected item from the file list control
//-------------------------------------------------------------------
void CTargetDriveArrayDlg::RemoveFileListItem()
{
    if (m_current_selection != -1)
	{
	    media::LogicalDrive  drvDescTmp;

		m_drive_list_ctrl.DeleteItem(m_current_selection);
		drvDescTmp = m_drive_array.GetDrive(m_drive_array[m_current_selection].Tag);

		m_drive_array.Remove(drvDescTmp);
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

void CTargetDriveArrayDlg::OnContextMenu(NMHDR *pNMHDR, LRESULT *pResult)
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
			CDriveEditDlg *pDriveEditor = new CDriveEditDlg(this, FALSE, m_current_selection);
			if( pDriveEditor->CheckForDependents() )
				bCanDelete = FALSE;
			delete pDriveEditor;
		}

	}

	VERIFY(menu.LoadMenu(IDR_DRIVE_MENU));

	pPopup = menu.GetSubMenu(0);
	pWndPopupOwner = this;

	if( pPopup )
	{
		csResStr.LoadStringW(IDS_DRIVE_MENU_NEW);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_DRIVE_NEW,
							MF_BYCOMMAND | MF_STRING, IDM_DRIVE_NEW, csResStr);

		csResStr.LoadStringW(IDS_DRIVE_MENU_EDIT);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_DRIVE_EDIT,
							MF_BYCOMMAND | MF_STRING, IDM_DRIVE_EDIT, csResStr);

		csResStr.LoadStringW(IDS_DRIVE_MENU_DELETE);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_DRIVE_DELETE,
							MF_BYCOMMAND | MF_STRING, IDM_DRIVE_DELETE, csResStr);

		csResStr.LoadStringW(IDS_DRIVE_MENU_COPY_DRIVE);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_DRIVE_COPY,
							MF_BYCOMMAND | MF_STRING, IDM_DRIVE_COPY, csResStr);

		csResStr.LoadStringW(IDS_DRIVE_MENU_MOVE_UP);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_DRIVE_MOVE_UP,
							MF_BYCOMMAND | MF_STRING, IDM_DRIVE_MOVE_UP, csResStr);

		csResStr.LoadStringW(IDS_DRIVE_MENU_MOVE_DOWN);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_DRIVE_MOVE_DOWN,
							MF_BYCOMMAND | MF_STRING, IDM_DRIVE_MOVE_DOWN, csResStr);

		if( !bCanEdit )
		{
			pPopup->EnableMenuItem(IDM_DRIVE_EDIT, MF_GRAYED );
			pPopup->EnableMenuItem(IDM_DRIVE_DELETE, MF_GRAYED );
		}
		if( !bCanMoveUp )
			pPopup->EnableMenuItem(IDM_DRIVE_MOVE_UP, MF_GRAYED );
		if( !bCanMoveDown )
			pPopup->EnableMenuItem(IDM_DRIVE_MOVE_DOWN, MF_GRAYED );
		if( !bCanDelete )
			pPopup->EnableMenuItem(IDM_DRIVE_DELETE, MF_GRAYED );


		media::LogicalDriveTag tmpNewTag;
		if( !CanCopy(tmpNewTag) )
			pPopup->EnableMenuItem(IDM_DRIVE_COPY, MF_GRAYED );

	    while (pWndPopupOwner->GetStyle() & WS_CHILD)
	    pWndPopupOwner = pWndPopupOwner->GetParent();

		iMenuCmd = (USHORT) pPopup->TrackPopupMenuEx( TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
												point.x, point.y,
												pWndPopupOwner, NULL);

		if ( iMenuCmd == IDM_DRIVE_NEW )
			OnMenuClickedNew();
		else if ( iMenuCmd == IDM_DRIVE_EDIT )
			OnMenuClickedEdit();
		else if ( iMenuCmd == IDM_DRIVE_DELETE )
			OnMenuClickedDelete();
		else if ( iMenuCmd == IDM_DRIVE_COPY )
			OnMenuClickedCopy();
		else if ( iMenuCmd == IDM_DRIVE_MOVE_UP )
			OnMenuClickedMoveUp();
		else if ( iMenuCmd == IDM_DRIVE_MOVE_DOWN )
			OnMenuClickedMoveDown();
	}
}

void CTargetDriveArrayDlg::OnMenuClickedNew()
{
	INT_PTR ret;
	media::LogicalDrive newDrive;

	newDrive.DriveNumber = m_drive_array.Size(); //m_drive_list_ctrl.GetItemCount();
	newDrive.RequestedKB = 0;
	newDrive.Type = media::DriveType_Invalid;
	newDrive.Flags = media::DriveFlag_NoAction;
	newDrive.Tag = media::DriveTag_Invalid;
	newDrive.Description = _T("");
	newDrive.Name = _T("");

	m_drive_array.AddDrive(newDrive);

	CDriveEditDlg *pDriveEditor = new CDriveEditDlg(this, TRUE, newDrive.DriveNumber);
    ret = pDriveEditor->DoModal();
    delete pDriveEditor;

	if( ret == IDOK )
		UpdateListItem(TRUE, newDrive.DriveNumber, m_drive_array[newDrive.DriveNumber]);
	else
		m_drive_array.Remove( newDrive );
}

void CTargetDriveArrayDlg::OnMenuClickedEdit()
{
	INT_PTR ret;

	CDriveEditDlg *pDriveEditor = new CDriveEditDlg(this, FALSE, m_current_selection);
    ret = pDriveEditor->DoModal();
    delete pDriveEditor;

	if( ret == IDOK )
		UpdateListItem(FALSE, m_current_selection, m_drive_array[m_current_selection]);
}


void CTargetDriveArrayDlg::OnMenuClickedCopy()
{
	media::LogicalDrive drv = m_drive_array[m_current_selection];
	media::LogicalDrive newDrive;
	int copyCount;


	copyCount = CanCopy( newDrive.Tag ); // get a new tag

	if( newDrive.Tag != media::DriveTag_Invalid )
	{
		newDrive.DriveNumber = m_drive_list_ctrl.GetItemCount();
		newDrive.RequestedKB = drv.RequestedKB;
		newDrive.Type = drv.Type;
		newDrive.Flags = drv.Flags;
		newDrive.Description = drv.Description;
		newDrive.Name = drv.Name;
		newDrive.FileListIndex = -1;

		newDrive.Description.Format(_T("%s (%d)"), drv.Description, copyCount);
		m_drive_array.Insert(m_current_selection+1, newDrive);
		UpdateListItem(TRUE, m_current_selection+1, newDrive);

		for(unsigned int i = m_current_selection+2; i < m_drive_array.Size(); ++i )
			m_drive_array[i].DriveNumber = i;

	}
}

int CTargetDriveArrayDlg::CanCopy(media::LogicalDriveTag & _newTag)
{
	BOOL bCanMakeCopy = TRUE;
	
	if( m_current_selection == -1 )
		return FALSE;  // no list items

	media::LogicalDrive drv = m_drive_array[m_current_selection];
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
	case media::DriveTag_LBABoot:
		_newTag = media::DriveTag_LBABoot;
		bCanMakeCopy = FALSE;
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
		for (unsigned int i=0; i < m_drive_array.Size(); i++)
		{
			media::LogicalDrive searchDrv = m_drive_array[i];

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


void CTargetDriveArrayDlg::OnMenuClickedDelete()
{
	m_drive_array.Remove( m_drive_array[m_current_selection] );

	m_drive_list_ctrl.DeleteItem(m_current_selection);
}

void CTargetDriveArrayDlg::OnMenuClickedMoveUp(void)
{
	unsigned int i, nItem;
	media::LogicalDrive drv = m_drive_array[m_current_selection];

	for( i = nItem = m_current_selection-1; i < m_drive_array.Size(); ++i )
		m_drive_list_ctrl.DeleteItem (nItem);

	m_drive_array.Remove( m_drive_array[m_current_selection] );
	m_drive_array.Insert( m_current_selection - 1, drv );
	for( i = m_current_selection-1; i < m_drive_array.Size(); ++i )
		m_drive_array[i].DriveNumber = i;

	for( i = m_current_selection-1; i < m_drive_array.Size(); i++ )
	{
		if (m_drive_array[i].Tag != media::DriveTag_Updater)
			UpdateListItem(TRUE, i, m_drive_array[i]);
	}
}

void CTargetDriveArrayDlg::OnMenuClickedMoveDown(void)
{
	unsigned int i, nItem;
	media::LogicalDrive drv = m_drive_array[m_current_selection];

	for( i = nItem = m_current_selection; i < m_drive_array.Size(); ++i )
		m_drive_list_ctrl.DeleteItem (nItem);

	m_drive_array.Remove( m_drive_array[m_current_selection] );
	m_drive_array.Insert( m_current_selection + 1, drv );
	for( i = m_current_selection; i < m_drive_array.Size(); ++i )
		m_drive_array[i].DriveNumber = i;

	for( i = m_current_selection; i < m_drive_array.Size(); i++ )
	{
		if (m_drive_array[i].Tag != media::DriveTag_Updater)
			UpdateListItem(TRUE, i, m_drive_array[i]);
	}
}
