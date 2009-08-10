// OpOTPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "OpUtpUpdateDlg.h"

void MyFileDialog::OnTypeChange( )
{
//	CFileDialog::UpdateOFNFromShellDialog();
}


// COpUtpUpdateDlg dialog

IMPLEMENT_DYNAMIC(COpUtpUpdateDlg, CDialog)

COpUtpUpdateDlg::COpUtpUpdateDlg(CWnd* pParent /*=NULL*/, COpInfo* pInfo)
	: CDialog(COpUtpUpdateDlg::IDD, pParent)
    , m_pOpInfo(pInfo)
	, m_required_files_filter(_T(""))
{
	m_pOpInfo->m_e_type = COperation::UTP_UPDATE_OP;
    m_pOpInfo->m_cs_desc = COperation::OperationStrings[m_pOpInfo->m_e_type];
}

COpUtpUpdateDlg::~COpUtpUpdateDlg()
{
}

void COpUtpUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OP_NAME_EDIT, m_vve_op_name_ctrl);
	DDX_Control(pDX, IDC_OP_UTP_UPDATE_UCL_LIST_COMBO, m_combo_ucl_lists);
	DDX_Control(pDX, IDC_LIST1, m_ucl_file_list_ctrl);
}


BEGIN_MESSAGE_MAP(COpUtpUpdateDlg, CDialog)
	ON_BN_CLICKED(IDOK, &COpUtpUpdateDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &COpUtpUpdateDlg::OnBnClickedCancel)
	ON_EN_CHANGE(IDC_UCL_FNAME_EDIT, &COpUtpUpdateDlg::OnUclFilenameChanged)
	ON_BN_CLICKED(IDC_UCL_BROWSE_BTN, &COpUtpUpdateDlg::OnBnClickedUclBrowseFile)
	ON_CBN_SELCHANGE(IDC_OP_UTP_UPDATE_UCL_LIST_COMBO, &COpUtpUpdateDlg::OnCbnSelchangeOpUtpUpdateUclListCombo)
	ON_BN_CLICKED(IDC_REQUIRED_FILES_BROWSE_BTN, &COpUtpUpdateDlg::OnBnClickedRequiredFilesBrowseBtn)
END_MESSAGE_MAP()


// COpUtpUpdateDlg message handlers

BOOL COpUtpUpdateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	Localize();

	m_vve_op_name_ctrl.SetValidCharSet(CVisValidEdit::SET_DIRPATH);
    m_vve_op_name_ctrl.SetEmptyValid(FALSE);
    m_vve_op_name_ctrl.SetTextLenMinMax(1);
    m_vve_op_name_ctrl.SetEnableLengthCheck();
	m_vve_op_name_ctrl.SetReadOnly(FALSE);

	if( !m_pOpInfo->m_cs_new_ini_section.IsEmpty() )
	    m_vve_op_name_ctrl.SetWindowText(m_pOpInfo->m_cs_new_ini_section);
	else
	    m_vve_op_name_ctrl.SetWindowText(m_pOpInfo->m_cs_ini_section);

	int index = m_pOpInfo->GetUclFilenameIndex();
	if (index != -1)
	{
		CFileList::PFILEITEM pItem = m_pOpInfo->m_FileList.GetAt(index);

		if (pItem->m_action == CFileList::EXISTS_IN_TARGET)
			SetDlgItemText(IDC_UCL_FNAME_EDIT, m_pOpInfo->GetUclFilename());
		else
			SetDlgItemText(IDC_UCL_FNAME_EDIT, m_pOpInfo->GetUclPathname());
	}

	m_ucl_fname.SubclassDlgItem(IDC_UCL_FNAME_EDIT, this);
	m_ucl_fname.SetUseDir(FALSE);
	m_ucl_fname.SetReadOnly(TRUE);

	LoadCommandLists(m_pOpInfo->GetUclPathname());

	for (int i=0; i < m_pOpInfo->m_FileList.GetCount(); i++)
	{
		if ( i != m_pOpInfo->m_ucl_fname_list_index )
		{
			CString itemStr = m_pOpInfo->m_FileList.GetFileNameAt(i);
			m_ucl_file_list_ctrl.AddString(itemStr);
		}
	}

	return TRUE;
	// return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void COpUtpUpdateDlg::LoadCommandLists(LPCTSTR filename)
{
    USES_CONVERSION;

	CFile commandFile;
	CFileException fileException;

	m_combo_ucl_lists.ResetContent();

	if( commandFile.Open(filename, CFile::modeRead, &fileException) )
	{
		CStringT<char,StrTraitMFC<char> > uclString;
		commandFile.Read(uclString.GetBufferSetLength(commandFile.GetLength()), commandFile.GetLength());
		uclString.ReleaseBuffer();

		if ( m_UclNode.Load(A2T(uclString)) != NULL )
		{
			UCL::CommandLists lists = m_UclNode.GetCommandLists();

			UCL::CommandLists::iterator it = lists.begin();
			for( ; it != lists.end(); ++(it))
			{
				int index = m_combo_ucl_lists.AddString(CString((*it)->GetName()));
				m_combo_ucl_lists.SetItemDataPtr(index, (*it));
			}

			if ( m_combo_ucl_lists.SelectString(0, m_pOpInfo->GetUclInstallSection()) == CB_ERR )
				m_combo_ucl_lists.SetCurSel(0);

			SetFileFilter();
		}
		else
		{
			TRACE( _T("Can't parse ucl.xml file.\n"));
		}
	}
	else
	{
		TRACE( _T("Can't open file %s, error = %u\n"), m_pOpInfo->GetUclPathname(), fileException.m_cause );
	}
}

void COpUtpUpdateDlg::OnBnClickedOk()
{
    m_vve_op_name_ctrl.GetWindowText(m_pOpInfo->m_cs_NewName);
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
	m_combo_ucl_lists.GetWindowText(m_pOpInfo->m_UclInstallSection);

	m_pOpInfo->m_FileList.CommitEdits();

	OnOK();
}

void COpUtpUpdateDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void COpUtpUpdateDlg::Localize()
{
//	CString resStr;

//	resStr.LoadString(IDS_OTPOP_TEXT);
//	SetDlgItemText(IDC_OTPOP_TEXT, resStr );
}

void COpUtpUpdateDlg::OnBnClickedUclBrowseFile()
{
    CFileDialog *dlg;

   	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
							_T("Update Command Lists (ucl.xml)|ucl.xml|"), this);

    if ( IDOK == dlg->DoModal() )
    {
		int index=0;
        CString csFullPathName = dlg->GetPathName();
		index = m_pOpInfo->GetUclFilenameIndex();
		if ( index == -1 )
			m_pOpInfo->SetUclFilename( csFullPathName, CFileList::IN_EDIT_ADD );
		else
			m_pOpInfo->m_FileList.ChangeFile(index, csFullPathName);

			// set the file name minus the path
		SetDlgItemText(IDC_UCL_FNAME_EDIT, csFullPathName);
    }

	delete dlg;
}


void COpUtpUpdateDlg::OnUclFilenameChanged()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	LoadCommandLists(m_pOpInfo->GetUclPathname());

	// TODO:  Add your control notification handler code here
}

CString COpUtpUpdateDlg::SetFileFilter()
{
	m_required_files_filter = _T("Required Files ");
	CString fileList;
	
	UCL::CommandList* pList = (UCL::CommandList*)m_combo_ucl_lists.GetItemDataPtr(m_combo_ucl_lists.GetCurSel());
	
	UCL::CommandList::Commands cmds = pList->GetCommands();
	UCL::CommandList::Commands::iterator it = cmds.begin();
	for ( ; it != cmds.end(); ++it )
	{
		if ( (*it)->GetType() == _T("push") )
		{
			CString filename = (*it)->GetFile();
			if ( !filename.IsEmpty() )
			{
				fileList.AppendFormat(_T("%s;"), filename);
			}
		}
		else if ( (*it)->GetType() == _T("boot") )
		{
			CString filename = (*it)->GetFile();
			if ( !filename.IsEmpty() )
			{
				fileList.AppendFormat(_T("%s;"), filename);
			}
		}
	}

	if ( !fileList.IsEmpty() )
		fileList.TrimRight(_T(';'));

	m_required_files_filter.AppendFormat(_T("(%s)|%s|"), fileList, fileList);
	m_required_files_filter += _T("All Files (*.*)|*.*||");

	return m_required_files_filter;
}
void COpUtpUpdateDlg::OnCbnSelchangeOpUtpUpdateUclListCombo()
{
	SetFileFilter();
}

void COpUtpUpdateDlg::OnBnClickedRequiredFilesBrowseBtn()
{
    MyFileDialog dlg( TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT,
		m_required_files_filter, this, 0, FALSE);

	if ( IDOK == dlg.DoModal() )
    {
        CString csPathName = dlg.GetPathName();
        POSITION startPosition = dlg.GetStartPosition();
		CFileList::PFILEITEM pItem;
		int index=0;

        while (startPosition)
        {
            CString csFullPathName = dlg.GetNextPathName(startPosition);

			pItem = m_pOpInfo->m_FileList.FindFile(csFullPathName, index);
			if ( pItem == NULL )
			{
				index = m_pOpInfo->m_FileList.AddFile( csFullPathName, CFileList::IN_EDIT_ADD );
				CString itemStr = m_pOpInfo->m_FileList.GetFileNameAt(index);
				m_ucl_file_list_ctrl.AddString(itemStr);
			}
			else
				m_pOpInfo->m_FileList.ChangeFile(index, csFullPathName);
        }
    }
}
