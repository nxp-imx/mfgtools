// CLogMgrDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LogMgrDlg.h"
#include ".\logmgrdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CLogMgrDlg dialog
CLogMgrDlg::CLogMgrDlg()
	: CDialog(CLogMgrDlg::IDD)
	, m_sLogDataStr (_T(""))
{ 
}

CLogMgrDlg::~CLogMgrDlg()
{
	DestroyWindow();
}

void CLogMgrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOG_DATA, m_sLogDataStr);
}

BEGIN_MESSAGE_MAP(CLogMgrDlg, CDialog)
END_MESSAGE_MAP()

// CLogger message handlers

BOOL CLogMgrDlg::Create() 
{ 
	return CDialog::Create(CLogMgrDlg::IDD); 
} 

BOOL CLogMgrDlg::OnInitDialog() 
{
	CString resStr;
	CDialog::OnInitDialog();

	resStr.LoadString(IDS_CANCEL);
	SetDlgItemText(IDCANCEL, resStr );

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

void CLogMgrDlg::LogIt(CString Data)
{
	m_sLogDataStr += Data;
	UpdateData(FALSE);
}

void CLogMgrDlg::OnCancel()
{
	ShowWindow(SW_HIDE);
}