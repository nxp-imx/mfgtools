// FileOpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "LoadFileOpDlg.h"

#define FOLDER_IMAGE	0
#define FILE_IMAGE		1

// CLoadFileOpDlg dialog

IMPLEMENT_DYNAMIC(CLoadFileOpDlg, CDialog)

CLoadFileOpDlg::CLoadFileOpDlg(CWnd* pParent /*=NULL*/, COpInfo* pInfo)
	: CDialog(CLoadFileOpDlg::IDD, pParent)
    , m_p_op_info(pInfo)
	, m_OutPath(_T(""))
{

}

CLoadFileOpDlg::~CLoadFileOpDlg()
{
}

void CLoadFileOpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_OP_TIMEOUT_EDIT, m_op_timeout_ctrl);
//    DDX_Control(pDX, IDC_OP_LOAD_FNAME_EDIT, m_op_filename_ctrl);
    DDX_Control(pDX, IDC_OP_NAME_EDIT, m_op_name_ctrl);
}


BEGIN_MESSAGE_MAP(CLoadFileOpDlg, CDialog)
    ON_EN_CHANGE(IDC_OP_NAME_EDIT, OnEnChangeOpNameEdit)
//    ON_EN_CHANGE(IDC_OP_LOAD_FNAME_EDIT, OnEnChangeFileNameEdit)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BROWSE_FILE, &CLoadFileOpDlg::OnBnClickedBrowseFile)
END_MESSAGE_MAP()


// CLoadFileOpDlg message handlers

BOOL CLoadFileOpDlg::OnInitDialog()
{
    ASSERT(m_p_op_info);
    DWORD err = ERROR_SUCCESS;

    CDialog::OnInitDialog();

	Localize();

	m_op_name_ctrl.SetValidCharSet(CVisValidEdit::SET_DIRPATH);
    m_op_name_ctrl.SetEmptyValid(FALSE);
    m_op_name_ctrl.SetTextLenMinMax(1);
    m_op_name_ctrl.SetEnableLengthCheck();
	m_op_name_ctrl.SetReadOnly(FALSE);

	m_op_filename.SubclassDlgItem(IDC_OP_LOAD_FNAME_EDIT, this);
	m_op_filename.SetUseDir(FALSE);
	m_op_filename.SetReadOnly(TRUE);

	if( !m_p_op_info->m_cs_new_ini_section.IsEmpty() )
		SetDlgItemText(IDC_OP_LOAD_FNAME_EDIT, m_p_op_info->m_cs_new_ini_section);
	else
		SetDlgItemText(IDC_OP_LOAD_FNAME_EDIT, m_p_op_info->m_cs_ini_section);


    UpdateData(FALSE);
    // Initialize the Operation Timeout: control
	CString csTimeout;
	csTimeout.Format(_T("%d"), m_p_op_info->m_timeout);
	m_op_timeout_ctrl.SetWindowText(csTimeout);

	m_original_folder = m_p_op_info->GetPath();

	GetDlgItem(IDOK)->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoadFileOpDlg::OnBnClickedOk()
{
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

	CString csFullPathName;
    GetDlgItemText(IDC_OP_LOAD_FNAME_EDIT,csFullPathName);
	m_p_op_info->m_FileList.AddFile(csFullPathName, CFileList::IN_EDIT_ADD);

	if (m_p_op_info->m_FileList.GetCount())
		m_p_op_info->m_status = OPINFO_OK;

	m_p_op_info->m_FileList.CommitEdits();

    // call the default handler
    OnOK();
}

void CLoadFileOpDlg::OnBnClickedCancel()
{
	m_p_op_info->m_FileList.RemoveEdits();

	OnCancel();
}


void CLoadFileOpDlg::OnBnClickedBrowseFile()
{
    CFileDialog *dlg;
	CString resStr;

	resStr.LoadString(IDS_FILE_BROWSE_TYPES);

   	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
							resStr, this);

    if ( IDOK == dlg->DoModal() )
    {
		USHORT index=0;
		CFileList::PFILEITEM pItem;
        CString csFullPathName = dlg->GetPathName();
		index = m_p_op_info->m_FileList.AddFile(csFullPathName, CFileList::IN_EDIT_ADD);
		pItem = m_p_op_info->m_FileList.GetAt(index);
			// set the file name minus the path
		SetDlgItemText(IDC_OP_LOAD_FNAME_EDIT, csFullPathName);
    }

	delete dlg;
}


void CLoadFileOpDlg::OnEnChangeOpNameEdit()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

void CLoadFileOpDlg::OnEnChangeFileNameEdit()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

void CLoadFileOpDlg::Localize()
{
	CString resStr;

	resStr.LoadStringW(IDS_OPERATION_LOAD);

	SetWindowText(resStr);

	resStr.LoadString(IDS_OP_EDIT_STEP1_GRP);
	SetDlgItemText(IDC_STEP1_GROUP, resStr );

	resStr.LoadString(IDS_OP_EDIT_NAME_OPERATION);
	SetDlgItemText(IDC_STEP1_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_STEP2_GRP);
	SetDlgItemText(IDC_STEP2_GROUP, resStr );

	resStr.LoadString(IDS_OP_EDIT_SELECT_FILES);
	SetDlgItemText(IDC_STEP2_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_STEP3_GRP);
	SetDlgItemText(IDC_STEP3_GROUP, resStr );

	resStr.LoadString(IDS_OP_EDIT_SELECT_OPTIONS);
	SetDlgItemText(IDC_STEP3_TEXT, resStr );

	resStr.LoadString(IDS_OP_EDIT_TIMEOUT);
	SetDlgItemText(IDC_OP_TIMEOUT_TEXT, resStr );

	resStr.LoadString(IDS_OK);
	SetDlgItemText(IDOK, resStr );

	resStr.LoadString(IDS_CANCEL);
	SetDlgItemText(IDCANCEL, resStr );
}


