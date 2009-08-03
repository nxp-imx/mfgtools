// CmdDataDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceApi.h"
#include "CmdDataDlg.h"
#include ".\cmddatadlg.h"


// CCmdDataDlg dialog

IMPLEMENT_DYNAMIC(CCmdDataDlg, CDialog)
CCmdDataDlg::CCmdDataDlg(StApi* pApi, CWnd* pParent /*=NULL*/)
	: CDialog(CCmdDataDlg::IDD, pParent)
	, _pApi(pApi)
{
}

CCmdDataDlg::~CCmdDataDlg()
{
}

void CCmdDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CMD_DATA_EDIT, _cmdDataCtrl);
}


BEGIN_MESSAGE_MAP(CCmdDataDlg, CDialog)
END_MESSAGE_MAP()


// CCmdDataDlg message handlers

BOOL CCmdDataDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString msg;

	// Dialog caption
	msg.Format(_T("%s Cmd Data"), _pApi->GetName());
	SetWindowText(msg);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCmdDataDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialog::OnOK();
}

