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
#include "OpOTPDlg.h"


// COpOTPDlg dialog

IMPLEMENT_DYNAMIC(COpOTPDlg, CDialog)

COpOTPDlg::COpOTPDlg(CWnd* pParent /*=NULL*/, COpInfo* pInfo)
	: CDialog(COpOTPDlg::IDD, pParent)
    , m_pOpInfo(pInfo)
{
	m_pOpInfo->m_e_type = COperation::OTP_OP;
    m_pOpInfo->m_cs_desc = COperation::OperationStrings[m_pOpInfo->m_e_type];
}

COpOTPDlg::~COpOTPDlg()
{
}

void COpOTPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_OP_NAME_EDIT, m_vve_op_name_ctrl);
	DDX_Control(pDX, IDC_OTPOP_VALUE, m_vve_OTP_register_value);
}


BEGIN_MESSAGE_MAP(COpOTPDlg, CDialog)
	ON_BN_CLICKED(IDOK, &COpOTPDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &COpOTPDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// COpOTPDlg message handlers

BOOL COpOTPDlg::OnInitDialog()
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

	m_vve_OTP_register_value.SetValidCharSet(CVisValidEdit::SET_HEXADECIMAL);
	m_vve_OTP_register_value.SetTextLenMinMax(2, 2);
	m_vve_OTP_register_value.SetEnableLengthCheck();
	m_vve_OTP_register_value.SetMyControlID(0x02);
	m_vve_OTP_register_value.SetReadOnly(FALSE);

	m_vve_OTP_register_value.SetWindowTextW(m_pOpInfo->m_csOTPValue);

	return TRUE;
	// return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void COpOTPDlg::OnBnClickedOk()
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

	m_vve_OTP_register_value.GetWindowTextW(m_pOpInfo->m_csOTPValue);
	OnOK();
}

void COpOTPDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void COpOTPDlg::Localize()
{
	CString resStr;

	resStr.LoadString(IDS_OTPOP_TEXT);
	SetDlgItemText(IDC_OTPOP_TEXT, resStr );
}
