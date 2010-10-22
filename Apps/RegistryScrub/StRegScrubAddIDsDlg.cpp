/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StRegScrubAddIDsDlg.cpp : implementation file
//

#include "stdafx.h"
#include ".\regscrublist.h"
#include "SDKRegScrub.h"
#include "StRegScrubAddIDsDlg.h"
#include ".\stregscrubaddidsdlg.h"
#include ".\regscrub.h"

#define MAX_SCSI_MFG_LEN	8
#define MAX_SCSI_PROD_LEN	16

extern REMOVEDEVICEITEM	g_AddDeviceEntry;

// StRegScrubAddIDsDlg dialog

IMPLEMENT_DYNAMIC(CStRegScrubAddIDsDlg, CDialog)
CStRegScrubAddIDsDlg::CStRegScrubAddIDsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStRegScrubAddIDsDlg::IDD, pParent)
{
}

CStRegScrubAddIDsDlg::~CStRegScrubAddIDsDlg()
{
}

void CStRegScrubAddIDsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}



BEGIN_MESSAGE_MAP(CStRegScrubAddIDsDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CStRegScrubAddIDsDlg::OnInitDialog()
{
    CEdit *editCtl;

	CDialog::OnInitDialog();

	editCtl =(CEdit*)GetDlgItem(IDC_OTHER_MFG);
	editCtl->SetLimitText(MAX_SCSI_MFG_LEN);
	editCtl->SetFocus();

//	editCtl->SetCueBannerText(L"Limited to 8 characters");

	editCtl =(CEdit*)GetDlgItem(IDC_OTHER_SCSI_PROD_ID);
	editCtl->SetLimitText(MAX_SCSI_PROD_LEN);
//	editCtl->SetCueBanner(L"Limited to 16 characters");

	editCtl =(CEdit*)GetDlgItem(IDC_OTHER_USB_VENDOR_ID);
	editCtl->SetLimitText(4);
//	editCtl->SetCueBanner(L"Limited to 4 hexidecimal characters");

	editCtl =(CEdit*)GetDlgItem(IDC_OTHER_USB_PROD_ID);
	editCtl->SetLimitText(4);
//	editCtl->SetCueBanner(L"Limited to 4 hexidecimal characters");

	return TRUE;
}
// StRegScrubAddIDsDlg message handlers

void CStRegScrubAddIDsDlg::OnBnClickedOk()
{
	BOOL valid = TRUE;
    CEdit *editCtl;

	editCtl =(CEdit*)GetDlgItem(IDC_OTHER_MFG);
	editCtl->GetWindowText(g_AddDeviceEntry.OrgMfg);
	g_AddDeviceEntry.Mfg = g_AddDeviceEntry.OrgMfg;
	if ( g_AddDeviceEntry.Mfg.IsEmpty() )
	{
		valid = FALSE;
		goto errout;
	}
	else
		FixUpString (g_AddDeviceEntry.Mfg);

	editCtl =(CEdit*)GetDlgItem(IDC_OTHER_SCSI_PROD_ID);
	editCtl->GetWindowText(g_AddDeviceEntry.OrgProduct);
	g_AddDeviceEntry.Product = g_AddDeviceEntry.OrgProduct;
	if ( g_AddDeviceEntry.Product.IsEmpty() )
	{
		valid = FALSE;
		goto errout;
	}
	else
		FixUpString (g_AddDeviceEntry.Product);

	editCtl =(CEdit*)GetDlgItem(IDC_OTHER_USB_VENDOR_ID);
	editCtl->GetWindowText(g_AddDeviceEntry.USBVid);
	if ( g_AddDeviceEntry.USBVid.IsEmpty() || (g_AddDeviceEntry.USBVid.GetLength() < 4) || !ValidateHexString(g_AddDeviceEntry.USBVid) )
	{
		valid = FALSE;
		goto errout;
	}
	else
		g_AddDeviceEntry.USBVid.MakeUpper();

	editCtl =(CEdit*)GetDlgItem(IDC_OTHER_USB_PROD_ID);
	editCtl->GetWindowText(g_AddDeviceEntry.USBPid);
	if ( g_AddDeviceEntry.USBPid.IsEmpty() || (g_AddDeviceEntry.USBPid.GetLength() < 4) || !ValidateHexString(g_AddDeviceEntry.USBPid) )
		valid = FALSE;
	else
		g_AddDeviceEntry.USBPid.MakeUpper();

errout:

	if (!valid)
	{
		editCtl->SetFocus();
		::MessageBeep(MB_ICONASTERISK);
		::MessageBox(NULL, "Invalid entry, please correct", "SDK Registry Scrubber", MB_OK);
	}
	else
		OnOK();
}

BOOL CStRegScrubAddIDsDlg::ValidateHexString(CString hexStr)
{
	CString valid="0123456789ABCDEFabcdef";

	for (int i = 0; i < 4; ++i)
	{
		UCHAR ch = hexStr.GetAt(i);
		if (valid.Find(ch) < 0)
			return FALSE;
	}
	return TRUE;
}

void CStRegScrubAddIDsDlg::FixUpString(CString& str)
{
	UINT i, len;
	UCHAR ch;

	// strip trailing blanks

	i = str.GetLength();
	while (i && str.GetAt(i-1) == ' ')
		--i;

	str.Truncate(i);

	// convert any embedded blanks to underscores

	len = str.GetLength();
	i = 0;

	while (i < len)
	{
		ch = str.GetAt(i);
		if (ch == ' ')
			str.SetAt(i, '_');
		++i;
	}
			
	return;
}

