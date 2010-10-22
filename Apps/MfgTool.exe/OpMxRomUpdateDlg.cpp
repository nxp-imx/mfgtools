/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// OpOTPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "OpMxRomUpdateDlg.h"

// COpMxRomUpdateDlg dialog

IMPLEMENT_DYNAMIC(COpMxRomUpdateDlg, CDialog)

COpMxRomUpdateDlg::COpMxRomUpdateDlg(CWnd* pParent /*=NULL*/, COpInfo* pInfo)
	: CDialog(COpMxRomUpdateDlg::IDD, pParent)
    , m_pOpInfo(pInfo)
	, m_image_files_filter(_T(""))
{
	m_pOpInfo->m_e_type = COperation::MX_UPDATE_OP;
    m_pOpInfo->m_cs_desc = COperation::OperationStrings[m_pOpInfo->m_e_type];
}

COpMxRomUpdateDlg::~COpMxRomUpdateDlg()
{
}

void COpMxRomUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OP_NAME_EDIT, m_vve_op_name_ctrl);
	DDX_Control(pDX, IDC_IMAGE_LIST, m_image_list_ctrl);
}


BEGIN_MESSAGE_MAP(COpMxRomUpdateDlg, CDialog)
	ON_BN_CLICKED(IDOK, &COpMxRomUpdateDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &COpMxRomUpdateDlg::OnBnClickedCancel)
	ON_EN_CHANGE(IDC_RKL_FNAME_EDIT, &COpMxRomUpdateDlg::OnRklFilenameChanged)
	ON_BN_CLICKED(IDC_RKL_BROWSE_BTN, &COpMxRomUpdateDlg::OnBnClickedRklBrowseFile)
	ON_BN_CLICKED(IDC_IMAGE_FILES_BROWSE_BTN, &COpMxRomUpdateDlg::OnBnClickedImageFilesBrowseBtn)
END_MESSAGE_MAP()


// COpMxRomUpdateDlg message handlers

BOOL COpMxRomUpdateDlg::OnInitDialog()
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

	int index = m_pOpInfo->GetRklFilenameIndex();
	if (index != -1)
	{
		CFileList::PFILEITEM pItem = m_pOpInfo->m_FileList.GetAt(index);

		if (pItem->m_action == CFileList::EXISTS_IN_TARGET)
			SetDlgItemText(IDC_RKL_FNAME_EDIT, m_pOpInfo->GetRklFilename());
		else
			SetDlgItemText(IDC_RKL_FNAME_EDIT, m_pOpInfo->GetRklPathname());
	}

	m_rkl_fname.SubclassDlgItem(IDC_RKL_FNAME_EDIT, this);
	m_rkl_fname.SetUseDir(FALSE);
	m_rkl_fname.SetReadOnly(TRUE);

	return TRUE;
	// return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void COpMxRomUpdateDlg::OnBnClickedOk()
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

	m_pOpInfo->m_FileList.CommitEdits();

	OnOK();
}

void COpMxRomUpdateDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void COpMxRomUpdateDlg::Localize()
{
//	CString resStr;

//	resStr.LoadString(IDS_OTPOP_TEXT);
//	SetDlgItemText(IDC_OTPOP_TEXT, resStr );
}

void COpMxRomUpdateDlg::OnBnClickedRklBrowseFile()
{
    CFileDialog *dlg;

   	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
							_T("Update Command Lists (ucl.xml)|ucl.xml|"), this);

    if ( IDOK == dlg->DoModal() )
    {
		int index=0;
        CString csFullPathName = dlg->GetPathName();
		index = m_pOpInfo->GetRklFilenameIndex();
		if ( index == -1 )
			m_pOpInfo->SetRklFilename( csFullPathName, CFileList::IN_EDIT_ADD );
		else
			m_pOpInfo->m_FileList.ChangeFile(index, csFullPathName);

			// set the file name minus the path
		SetDlgItemText(IDC_RKL_FNAME_EDIT, csFullPathName);
    }

	delete dlg;
}


void COpMxRomUpdateDlg::OnRklFilenameChanged()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void COpMxRomUpdateDlg::OnBnClickedImageFilesBrowseBtn()
{
    CFileDialog dlg( TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT,
		m_image_files_filter, this, 0, FALSE);

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
//				m_image_list_ctrl.AddString(itemStr);
			}
			else
				m_pOpInfo->m_FileList.ChangeFile(index, csFullPathName);
        }
    }
}
