// OpOTPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "OpUclDlg.h"

// COpUclDlg dialog

IMPLEMENT_DYNAMIC(COpUclDlg, CDialog)

COpUclDlg::COpUclDlg(CWnd* pParent /*=NULL*/, COpInfo* pInfo)
	: CDialog(COpUclDlg::IDD, pParent)
    , m_pOpInfo(pInfo)
{
    m_pOpInfo->m_cs_desc = COperation::OperationStrings[m_pOpInfo->m_e_type];
}

COpUclDlg::~COpUclDlg()
{
}

void COpUclDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OP_UTP_UPDATE_UCL_LIST_COMBO, m_combo_ucl_lists);
}


BEGIN_MESSAGE_MAP(COpUclDlg, CDialog)
	ON_BN_CLICKED(IDOK, &COpUclDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &COpUclDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// COpUtpUpdateDlg message handlers

BOOL COpUclDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(m_pOpInfo->GetUclPathname());

	Localize();

	LoadCommandLists(m_pOpInfo->GetUclPathname());

	return TRUE;
	// return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void COpUclDlg::LoadCommandLists(LPCTSTR filename)
{
    USES_CONVERSION;

	CFile commandFile;
	CFileException fileException;

	m_combo_ucl_lists.ResetContent();

	if( commandFile.Open(filename, CFile::modeRead, &fileException) )
	{
		CStringT<char,StrTraitMFC<char> > uclString;
		commandFile.Read(uclString.GetBufferSetLength(commandFile.GetLength()), commandFile.GetLength());
		uclString.ReleaseBuffer();

		if ( m_UclNode.Load(A2T(uclString)) != NULL )
		{
			UCL::CommandLists lists = m_UclNode.GetCommandLists();

			UCL::CommandLists::iterator it = lists.begin();
			for( ; it != lists.end(); ++(it))
			{
				int index = m_combo_ucl_lists.AddString(CString((*it)->GetName()));
				m_combo_ucl_lists.SetItemDataPtr(index, (*it));
			}

			if ( m_combo_ucl_lists.SelectString(0, m_pOpInfo->GetUclInstallSection()) == CB_ERR )
				m_combo_ucl_lists.SetCurSel(0);
		}
		else
		{
			TRACE( _T("Can't parse ucl.xml file.\n"));
		}
	}
	else
	{
		TRACE( _T("Can't open file %s, error = %u\n"), m_pOpInfo->GetUclPathname(), fileException.m_cause );
	}
}

void COpUclDlg::OnBnClickedOk()
{
	m_combo_ucl_lists.GetWindowText(m_pOpInfo->m_UclInstallSection);

	OnOK();
}

void COpUclDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void COpUclDlg::Localize()
{
//	CString resStr;

//	resStr.LoadString(IDS_OTPOP_TEXT);
//	SetDlgItemText(IDC_OTPOP_TEXT, resStr );
}
