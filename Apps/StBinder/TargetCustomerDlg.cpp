/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// TargetCustomerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StBinder.h"
#include "TargetCustomerDlg.h"

extern TARGET_CFG_DATA g_ResCfgData;

// CTargetCustomerDlg dialog

IMPLEMENT_DYNAMIC(CTargetCustomerDlg, CPropertyPage)

CTargetCustomerDlg::CTargetCustomerDlg(/*CWnd* pParent =NULL*/)
	: CPropertyPage(CTargetCustomerDlg::IDD)
{
	m_bitmap = NULL;

	m_CompanyImagePathname.Empty();
	m_UpdaterIconPathname.Empty();
}

CTargetCustomerDlg::~CTargetCustomerDlg()
{
	if (m_bitmap)
		DeleteObject(m_bitmap);
}

void CTargetCustomerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TARGET_CUST_COMPANY, m_TargetCustomerName);
	DDX_Control(pDX, IDC_TARGET_CUST_PROD_DESC, m_TargetProdDesc);
	DDX_Control(pDX, IDC_TARGET_CUST_PROD_NAME, m_TargetProdName);
	DDX_Control(pDX, IDC_UPD_MAJOR_VERSION, m_UpdMajorVersion);
	DDX_Control(pDX, IDC_UPD_MINOR_VERSION, m_UpdMinorVersion);
	DDX_Control(pDX, IDC_PROD_MAJOR_VERSION, m_ProdMajorVersion);
	DDX_Control(pDX, IDC_PROD_MINOR_VERSION, m_ProdMinorVersion);
	DDX_Control(pDX, IDC_TARGET_CUST_TITLE, m_TargetAppTitle);
	DDX_Control(pDX, IDC_TARGET_CUST_COPYRIGHT, m_TargetCopyright);
	DDX_Control(pDX, IDC_TARGET_CUST_COMPANYIMAGE, m_TargetCompanyImagePathname);
	DDX_Control(pDX, IDC_TARGET_CUST_UPDATERICON, m_TargetUpdaterIconPathname);
	DDX_Control(pDX, IDC_TARGET_CUST_COMMENT, m_TargetComment);
	DDX_Control(pDX, IDC_COMPANY_ICON, m_CompanyIcon);
	DDX_Control(pDX, IDC_UPDATER_ICON, m_UpdaterIcon);
	DDX_Control(pDX, IDC_COMPANY_BITMAP, m_CompanyBitmap);

}


BEGIN_MESSAGE_MAP(CTargetCustomerDlg, CPropertyPage)
	ON_BN_CLICKED(IDC_TARGET_CUST_BROWSE_COMPANYIMAGE, &CTargetCustomerDlg::OnBnClickedTargetCustBrowseCompanyImage)
	ON_BN_CLICKED(IDC_TARGET_CUST_BROWSE_UPDATERICON, &CTargetCustomerDlg::OnBnClickedTargetCustBrowseUpdaterIcon)
END_MESSAGE_MAP()


BOOL CTargetCustomerDlg::OnInitDialog()
{
	CString resStr;

	CPropertyPage::OnInitDialog();

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_CUST_COMPANY_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_CUST_COMPANY_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_CUST_PRODDESC_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_CUST_PROD_DESC_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_CUST_APPTITLE_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_CUST_TITLE_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_CUST_COPYRIGHT_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_CUST_COPYRIGHT_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_CUST_COMPANYIMAGE_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_CUST_COMPANYIMAGE_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_CUST_UPDATERICON_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_CUST_UPDATERICON_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_CUST_COMMENT_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_CUST_COMMENT_TEXT, resStr );

	m_TargetCustomerName.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_TargetCustomerName.SetTextLenMinMax(1,COMPANYNAME_MAX);
	m_TargetCustomerName.SetEnableLengthCheck();
	m_TargetCustomerName.SetMyControlID(0x01);
	resStr = g_ResCfgData.Options.CompanyName;
	resStr.TrimRight(_T(" "));
	m_TargetCustomerName.SetWindowTextW(resStr);

	m_TargetProdDesc.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_TargetProdDesc.SetTextLenMinMax(1,PRODDESC_MAX);
	m_TargetProdDesc.SetEnableLengthCheck();
	m_TargetProdDesc.SetMyControlID(0x02);
	resStr = g_ResCfgData.Options.ProductDesc;
	resStr.TrimRight(_T(" "));
	m_TargetProdDesc.SetWindowTextW(resStr);

	m_TargetProdName.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_TargetProdName.SetTextLenMinMax(1,PRODNAME_MAX);
	m_TargetProdName.SetEnableLengthCheck();
	m_TargetProdName.SetMyControlID(0x03);
	resStr = g_ResCfgData.Options.ProductName;
	resStr.TrimRight(_T(" "));
	m_TargetProdName.SetWindowTextW(resStr);

	m_UpdMajorVersion.SetValidCharSet(CVisValidEdit::SET_DECIMAL);
	m_UpdMajorVersion.SetTextLenMinMax(1,3);
	m_UpdMajorVersion.SetEnableLengthCheck();
	m_UpdMajorVersion.SetMyControlID(0x04);
	m_UpdMajorVersion.SetWindowValue(g_ResCfgData.Options.UpdMajorVersion);

	m_UpdMinorVersion.SetValidCharSet(CVisValidEdit::SET_DECIMAL);
	m_UpdMinorVersion.SetTextLenMinMax(1,3);
	m_UpdMinorVersion.SetEnableLengthCheck();
	m_UpdMinorVersion.SetMyControlID(0x05);
	m_UpdMinorVersion.SetWindowValue(g_ResCfgData.Options.UpdMinorVersion);

	m_ProdMajorVersion.SetValidCharSet(CVisValidEdit::SET_DECIMAL);
	m_ProdMajorVersion.SetTextLenMinMax(1,3);
	m_ProdMajorVersion.SetEnableLengthCheck();
	m_ProdMajorVersion.SetMyControlID(0x06);
	m_ProdMajorVersion.SetWindowValue(g_ResCfgData.Options.ProdMajorVersion);

	m_ProdMinorVersion.SetValidCharSet(CVisValidEdit::SET_DECIMAL);
	m_ProdMinorVersion.SetTextLenMinMax(1,3);
	m_ProdMinorVersion.SetEnableLengthCheck();
	m_ProdMinorVersion.SetMyControlID(0x07);
	m_ProdMinorVersion.SetWindowValue(g_ResCfgData.Options.ProdMinorVersion);

	m_TargetAppTitle.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_TargetAppTitle.SetTextLenMinMax(1,APPTITLE_MAX);
	m_TargetAppTitle.SetEnableLengthCheck();
	m_TargetAppTitle.SetMyControlID(0x08);
	resStr = g_ResCfgData.Options.AppTitle;
	resStr.TrimRight(_T(" "));
	m_TargetAppTitle.SetWindowTextW(resStr);

	m_TargetCopyright.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_TargetCopyright.SetTextLenMinMax(1,COPYRIGHT_MAX);
	m_TargetCopyright.SetEnableLengthCheck();
	m_TargetCopyright.SetMyControlID(0x09);
	resStr = g_ResCfgData.Options.Copyright;
	resStr.TrimRight(_T(" "));
	m_TargetCopyright.SetWindowTextW(resStr);

	m_TargetCompanyImagePathname.SetValidCharSet(CVisValidEdit::SET_DIRPATH);
	m_TargetCompanyImagePathname.SetTextLenMinMax(1,MAX_PATH);
	m_TargetCompanyImagePathname.SetEnableLengthCheck();
	m_TargetCompanyImagePathname.SetMyControlID(0x0A);

	m_TargetUpdaterIconPathname.SetValidCharSet(CVisValidEdit::SET_DIRPATH);
	m_TargetUpdaterIconPathname.SetTextLenMinMax(1,MAX_PATH);
	m_TargetUpdaterIconPathname.SetEnableLengthCheck();
	m_TargetUpdaterIconPathname.SetMyControlID(0x0B);

	m_TargetComment.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_TargetComment.SetTextLenMinMax(1,COMMENT_MAX);
	m_TargetComment.SetEnableLengthCheck();
	m_TargetComment.SetMyControlID(0x0C);
	resStr = g_ResCfgData.Options.Comment;
	resStr.TrimRight(_T(" "));
	m_TargetComment.SetWindowTextW(resStr);
	
	//	Set icons to those currently in the target
	UpdateData(TRUE);
	m_CompanyImagePathname = g_ResCfgData.CompanyImagePathname;
	if (g_ResCfgData.IsBMP)
	{
		if (!m_CompanyImagePathname.IsEmpty())
		{
			m_bitmap = (HBITMAP)::LoadImage( 0, m_CompanyImagePathname, IMAGE_BITMAP, 20, 20,
		                                LR_LOADFROMFILE);
			if (m_bitmap)
			{
				m_CompanyBitmap.SetBitmap(m_bitmap);
				SetDlgItemText(IDC_TARGET_CUST_COMPANYIMAGE, m_CompanyImagePathname);
			}
		}
		else
			m_CompanyBitmap.SetBitmap(g_ResCfgData.CurrentCompanyBitmap);

		m_CompanyBitmap.Invalidate();
		m_CompanyBitmap.ShowWindow(SW_SHOW);
		m_CompanyIcon.ShowWindow(SW_HIDE);
	}
	else
	{
		if ( !m_CompanyImagePathname.IsEmpty() ) 
		{
			m_companyicon = (HICON)::LoadImage ( 0, m_CompanyImagePathname, IMAGE_ICON, 0, 0,
		                                LR_LOADFROMFILE);
			m_CompanyIcon.SetIcon(m_companyicon);
			SetDlgItemText(IDC_TARGET_CUST_COMPANYIMAGE, m_CompanyImagePathname);
		}
		else
			m_CompanyIcon.SetIcon(g_ResCfgData.CurrentCompanyIcon);
		m_CompanyIcon.Invalidate();
		m_CompanyIcon.ShowWindow(SW_SHOW);
		m_CompanyBitmap.ShowWindow(SW_HIDE);
	}

	m_UpdaterIconPathname = g_ResCfgData.UpdaterIconPathname;
	if ( !m_UpdaterIconPathname.IsEmpty() ) 
	{
		m_updatericon = (HICON)::LoadImage ( 0, m_UpdaterIconPathname, IMAGE_ICON, 0, 0,
	                                LR_LOADFROMFILE);
		if (m_updatericon)
		{
			m_UpdaterIcon.SetIcon(m_updatericon);
			SetDlgItemText(IDC_TARGET_CUST_UPDATERICON, m_UpdaterIconPathname);
		}
	}
	else
		m_UpdaterIcon.SetIcon(g_ResCfgData.CurrentUpdaterIcon);

	m_UpdaterIcon.Invalidate();
	UpdateData(FALSE);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// CTargetCustomerDlg message handlers

void CTargetCustomerDlg::OnOK()
{
	CString csIconPath;
	m_TargetCustomerName.GetWindowTextW(g_ResCfgData.Options.CompanyName,COMPANYNAME_MAX+1);
	m_TargetProdDesc.GetWindowTextW(g_ResCfgData.Options.ProductDesc, PRODDESC_MAX+1);
	m_TargetProdName.GetWindowTextW(g_ResCfgData.Options.ProductName, PRODNAME_MAX+1);
	m_TargetAppTitle.GetWindowTextW(g_ResCfgData.Options.AppTitle,APPTITLE_MAX+1);
	m_TargetCopyright.GetWindowTextW(g_ResCfgData.Options.Copyright,COPYRIGHT_MAX+1);
	m_TargetComment.GetWindowTextW(g_ResCfgData.Options.Comment,COMMENT_MAX+1);

	m_UpdMajorVersion.GetWindowValue(g_ResCfgData.Options.UpdMajorVersion);
	m_UpdMinorVersion.GetWindowValue(g_ResCfgData.Options.UpdMinorVersion);
	m_ProdMajorVersion.GetWindowValue(g_ResCfgData.Options.ProdMajorVersion);
	m_ProdMinorVersion.GetWindowValue(g_ResCfgData.Options.ProdMinorVersion);

	if ( !m_CompanyImagePathname.IsEmpty() )
	{
		g_ResCfgData.CompanyImagePathname = m_CompanyImagePathname;
		if (m_CompanyImagePathname.Find(_T(".bmp"),0) == -1)
			g_ResCfgData.IsBMP = FALSE;
		else
			g_ResCfgData.IsBMP = TRUE;
	}

	if ( !m_UpdaterIconPathname.IsEmpty() ) 
		g_ResCfgData.UpdaterIconPathname = m_UpdaterIconPathname;

	CPropertyPage::OnOK();
}

void CTargetCustomerDlg::OnCancel()
{
	CPropertyPage::OnCancel();
}

void CTargetCustomerDlg::OnBnClickedTargetCustBrowseCompanyImage()
{
    CFileDialog *dlg;

   	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
							_T("(*.ico;*.bmp)|*.ico;*.bmp||\0"), this);

	if ( IDOK == dlg->DoModal() )
    {
		m_CompanyImagePathname = dlg->GetPathName();
		SetDlgItemText(IDC_TARGET_CUST_COMPANYIMAGE, m_CompanyImagePathname);

		if (m_CompanyImagePathname.Find(_T(".bmp"),0) == -1)
		{
			m_companyicon = (HICON)::LoadImage ( 0, m_CompanyImagePathname, IMAGE_ICON, 0, 0,
		                                LR_LOADFROMFILE);

			if (m_companyicon)
			{
				m_CompanyBitmap.ShowWindow(SW_HIDE);
				m_CompanyIcon.ShowWindow(SW_SHOW);
				UpdateData(TRUE);
				m_CompanyIcon.SetIcon(m_companyicon);
				m_CompanyIcon.Invalidate();
				UpdateData(FALSE);
			}
			else
				MessageBeep(MB_ICONQUESTION);
		}
		else
		{
		    HANDLE hFile;

			hFile = CreateFile (
				          m_CompanyImagePathname,
					      GENERIC_READ,// | GENERIC_WRITE,
						  FILE_SHARE_READ,// | FILE_SHARE_WRITE,
						  NULL, // no SECURITY_ATTRIBUTES structure
			              OPEN_EXISTING, // No special create flags
				          0, // No special attributes
					      NULL); // No template file

		    if ( hFile != INVALID_HANDLE_VALUE )
			{
					if (m_bitmap)
						DeleteObject(m_bitmap);

					m_bitmap = (HBITMAP)::LoadImage( 0, m_CompanyImagePathname, IMAGE_BITMAP, 20, 20,
		                                LR_LOADFROMFILE);
					if (m_bitmap)
					{
						m_CompanyBitmap.ShowWindow(SW_SHOW);
						m_CompanyIcon.ShowWindow(SW_HIDE);
						UpdateData(TRUE);
						m_CompanyBitmap.SetBitmap(m_bitmap);
						m_CompanyBitmap.Invalidate();
						UpdateData(FALSE);
					}
				CloseHandle(hFile);
			}
			else
			{
				m_CompanyImagePathname.Empty();
				MessageBeep(MB_ICONQUESTION);
			}
		}
    }

	delete dlg;
}

void CTargetCustomerDlg::OnBnClickedTargetCustBrowseUpdaterIcon()
{
    CFileDialog *dlg;

   	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
							_T("(*.ico)|*.ico||\0"), this);

    if ( IDOK == dlg->DoModal() )
    {
		m_UpdaterIconPathname = dlg->GetPathName();
		m_updatericon = (HICON)::LoadImage ( 0, m_UpdaterIconPathname, IMAGE_ICON, 0, 0,
                                    LR_LOADFROMFILE);
		UpdateData(TRUE);
		m_UpdaterIcon.SetIcon(m_updatericon);
		m_UpdaterIcon.Invalidate();
		UpdateData(FALSE);
		SetDlgItemText(IDC_TARGET_CUST_UPDATERICON, m_UpdaterIconPathname);
    }

	delete dlg;
}
