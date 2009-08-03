// AddFilterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceEnum.h"
#include "AddFilterDlg.h"
#include ".\addfilterdlg.h"


// AddFilterDlg dialog

IMPLEMENT_DYNAMIC(AddFilterDlg, CDialog)
AddFilterDlg::AddFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AddFilterDlg::IDD, pParent)
{
}

AddFilterDlg::~AddFilterDlg()
{
}

void AddFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USB_VID_EDIT, m_usb_vid);
	DDX_Text(pDX, IDC_USB_PID_EDIT, m_usb_pid);
}


BEGIN_MESSAGE_MAP(AddFilterDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedAdd)
END_MESSAGE_MAP()


// AddFilterDlg message handlers

void AddFilterDlg::OnBnClickedAdd()
{
	UpdateData(true);
	OnOK();
}

BOOL AddFilterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_USB_VID_EDIT)->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
}
