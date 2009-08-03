// ParamDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceApi.h"
#include "ParamDlg.h"


// CParamDlg dialog

IMPLEMENT_DYNAMIC(CParamDlg, CDialog)
CParamDlg::CParamDlg(StApi* pApi, CWnd* pParent)
	: CDialog(CParamDlg::IDD, pParent)
	, _pApi(pApi)
{
}

CParamDlg::~CParamDlg()
{
}

void CParamDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_API_INFO, _apiInfoCtrl);
	DDX_Control(pDX, IDC_PARAM1_VAL, _param1Val);
	DDX_Control(pDX, IDC_PARAM2_VAL, _param2Val);
	DDX_Control(pDX, IDC_PARAM3_VAL, _param3Val);
	DDX_Control(pDX, IDC_PARAM4_VAL, _param4Val);
	DDX_Control(pDX, IDC_PARAM5_VAL, _param5Val);
	DDX_Control(pDX, IDC_PARAM6_VAL, _param6Val);
}


BEGIN_MESSAGE_MAP(CParamDlg, CDialog)
END_MESSAGE_MAP()


// CParamDlg message handlers

BOOL CParamDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CString msg;

	// Dialog caption
	msg.Format(_T("%s Parameters"), _pApi->GetName());
	SetWindowText(msg);

	//Parameters
	Parameter::ParamMap::iterator param;
	CStatic * pTagCtrl;
	CEdit * pValueCtrl;
	CComboBox * pValueListCtrl;

	int index = 0;
	for ( param = _pApi->GetParameters().begin(); param != _pApi->GetParameters().end(); ++param )
	{
		pTagCtrl = (CStatic*)GetDlgItem(IDC_PARAM1_TAG + index);
		pValueCtrl = (CEdit*)GetDlgItem(IDC_PARAM1_VAL + index);
		pValueListCtrl = (CComboBox*)GetDlgItem(IDC_PARAM1_VAL_LIST + index);

		pTagCtrl->ShowWindow(SW_SHOW);
		pTagCtrl->SetWindowText((*param).first);

		StdStringArray strArray = (*param).second->GetValueStrings();
		if ( strArray.empty() )
		{
			pValueCtrl->ShowWindow(SW_SHOW);
			pValueCtrl->SetWindowText((*param).second->ToString());
		}
		else
		{
			StdStringArray::iterator str;
			pValueListCtrl->ShowWindow(SW_SHOW);
			int listIndex = 0;
			for ( str = strArray.begin(); str != strArray.end(); ++str )
			{
				pValueListCtrl->InsertString(listIndex++, (*str));
			}
			pValueListCtrl->SelectString(0,(*param).second->ToString());
		}

		// move to the next set of controls
		index += 3;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CParamDlg::OnOK()
{
	CString str;

	//Parameters
	Parameter::ParamMap::iterator param;
	CEdit * pValueCtrl;
	CComboBox * pValueListCtrl;

	int index = 0;
	for ( param = _pApi->GetParameters().begin(); param != _pApi->GetParameters().end(); ++param )
	{
		pValueCtrl = (CEdit*)GetDlgItem(IDC_PARAM1_VAL + index);
		pValueListCtrl = (CComboBox*)GetDlgItem(IDC_PARAM1_VAL_LIST + index);

		// get the string representation of the value from the proper control
		StdStringArray strArray = (*param).second->GetValueStrings();
		if ( strArray.empty() )
		{
			pValueCtrl->GetWindowText(str);
		}
		else
		{
			pValueListCtrl->GetWindowText(str);
		}

		// save the string as a value in the parameter
		(*param).second->Parse((LPCTSTR)str);

		// move to the next set of controls
		index += 3;
	}
	
	// re-init the api with the new parameters
	_pApi->PrepareCommand();

	CDialog::OnOK();
}
