// MsFormatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StFormat.h"
#include "MsFormatDlg.h"


// MsFormatDlg dialog

IMPLEMENT_DYNAMIC(MsFormatDlg, CDialog)
MsFormatDlg::MsFormatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(MsFormatDlg::IDD, pParent)
{
}

MsFormatDlg::~MsFormatDlg()
{
}

void MsFormatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(MsFormatDlg, CDialog)
END_MESSAGE_MAP()


// MsFormatDlg message handlers
