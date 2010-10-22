/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// OtherFirmwareLANGID.cpp : implementation file
//

#include "stdafx.h"
#include <winnls.h>
#include "StBinder.h"
#include "default_langid.h"
#include "OtherFirmwareLANGID.h"


// COtherFirmwareLANGID dialog

IMPLEMENT_DYNAMIC(COtherFirmwareLANGID, CDialog)

COtherFirmwareLANGID::COtherFirmwareLANGID(LANGID * _pLangID, CWnd* pParent /*=NULL*/)
	: CDialog(COtherFirmwareLANGID::IDD, pParent)
{
	m_pLangID = _pLangID;
}

COtherFirmwareLANGID::~COtherFirmwareLANGID()
{
}

void COtherFirmwareLANGID::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOCALE_LIST, m_LocaleList);
}


BEGIN_MESSAGE_MAP(COtherFirmwareLANGID, CDialog)
	ON_BN_CLICKED(IDOK, &COtherFirmwareLANGID::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &COtherFirmwareLANGID::OnBnClickedCancel)
	ON_LBN_DBLCLK(IDC_LOCALE_LIST, &COtherFirmwareLANGID::OnLbnDblclkLocaleList)
	ON_LBN_SELCHANGE(IDC_LOCALE_LIST, &COtherFirmwareLANGID::OnLbnSelchangeLocaleList)
END_MESSAGE_MAP()

BOOL COtherFirmwareLANGID::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString resStr;
	int iIndex = 0;

	((CStBinderApp*)AfxGetApp())->GetString(IDS_OK, resStr);
	SetDlgItemText(IDOK, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_CANCEL, resStr);
	SetDlgItemText(IDCANCEL, resStr );

	for (WORD wLangID = 0; wLangID < 256; ++wLangID)
		for (WORD wSublangID = 0; wSublangID < 50; ++wSublangID)
		{
			BOOL bIsDefault = FALSE;
			TCHAR szLocaleName[64];
			LANGID langID = MAKELANGID(wLangID, wSublangID);

			for (int j = 0; j < g_LangIdCount; ++j)
				if ( langID == g_LangIds[j] )
				{
					bIsDefault = TRUE;
					break;
				}

			if ( !bIsDefault && GetLocaleInfo(langID, LOCALE_SLANGUAGE, szLocaleName, 64) )
			{
				TCHAR str[128];

				_stprintf_s (str, 128, L"%s (%x,%x)", szLocaleName, wLangID, wSublangID);
				int iInsertIndex = 	m_LocaleList.AddString(str);
				m_LocaleList.SetItemData( iInsertIndex, (DWORD)langID );
				++iIndex;
			}
		}

	((CStBinderApp*)AfxGetApp())->GetString(IDS_OK, resStr);
	GetDlgItem(IDOK)->SetWindowText(resStr);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_CANCEL, resStr);
	GetDlgItem(IDCANCEL)->SetWindowText(resStr);

	return TRUE;  // return TRUE  unless you set the focus to a control

}

// COtherFirmwareLANGID message handlers

void COtherFirmwareLANGID::OnBnClickedOk()
{
	OnOK();
}

void COtherFirmwareLANGID::OnBnClickedCancel()
{
	*m_pLangID = (LANGID)-1;
	OnCancel();
}

void COtherFirmwareLANGID::OnLbnDblclkLocaleList()
{
	OnLbnSelchangeLocaleList();
	OnBnClickedOk();
}

void COtherFirmwareLANGID::OnLbnSelchangeLocaleList()
{
	int iIndex = m_LocaleList.GetCurSel();
	if ( iIndex != LB_ERR )
		*m_pLangID = (LANGID) m_LocaleList.GetItemData( iIndex );
}
