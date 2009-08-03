// UserConfirmation.cpp : implementation file
//

#include "stdafx.h"
#include "StBinder.h"
#include "UserConfirmation.h"


// CUserConfirmation dialog

IMPLEMENT_DYNAMIC(CUserConfirmation, CDialog)

CUserConfirmation::CUserConfirmation(PCONFIRM_LIST _pList, CWnd* pParent /*=NULL*/)
	: CDialog(CUserConfirmation::IDD, pParent)
{
	m_resList = _pList;
}

CUserConfirmation::~CUserConfirmation()
{
}

void CUserConfirmation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CONFIRM_LIST, m_resListCtrl);
}


BEGIN_MESSAGE_MAP(CUserConfirmation, CDialog)
	ON_BN_CLICKED(IDOK, &CUserConfirmation::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CUserConfirmation::OnBnClickedCancel)
END_MESSAGE_MAP()


BOOL CUserConfirmation::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString csText;
	RECT listRect;
	m_resListCtrl.GetClientRect(&listRect);
	int nListWidth = (listRect.right - listRect.left);
	int nVScrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL); 
	
	int nCol1Width = (int) ((double)nListWidth * 0.30);  // resource
    int nCol2Width = (int) ((double)nListWidth * 0.70);  // resource group

	m_resListCtrl.InsertColumn(0, L"", LVCFMT_LEFT, nCol1Width);
	m_resListCtrl.InsertColumn(1, L"", LVCFMT_LEFT, nCol2Width);

	if ( m_resList )
	{
		int nItem = 0;
		PCONFIRM_LIST pNext = m_resList;
		while ( pNext )
		{
			m_resListCtrl.InsertItem(nItem, pNext->csResName);
			m_resListCtrl.SetItemText(nItem, 1, pNext->csResGroup);
			++nItem;
			pNext = (PCONFIRM_LIST) pNext->pNext;
		}
	}

	((CStBinderApp*)AfxGetApp())->GetString(IDS_RES_DELETE_REPLACE_PROMPT, csText);
	SetDlgItemText (IDC_CONFIRM_TEXT, csText);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_CONTINUE_PROMPT, csText);
	SetDlgItemText (IDC_CONTINUE, csText);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_OK, csText);
	GetDlgItem(IDOK)->SetWindowText(csText);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_CANCEL, csText);
	GetDlgItem(IDCANCEL)->SetWindowText(csText);

	return TRUE;
}

// CUserConfirmation message handlers

void CUserConfirmation::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}

void CUserConfirmation::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}
