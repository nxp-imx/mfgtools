#pragma once
#include "afxwin.h"
#include "Libs/DevSupport/StApi.h"
#include "OverwriteEdit.h"


// CParamDlg dialog

class CParamDlg : public CDialog
{
	DECLARE_DYNAMIC(CParamDlg)

public:
	CParamDlg(StApi* pApi, CWnd* pParent=NULL);   // standard constructor
	virtual ~CParamDlg();

// Dialog Data
	enum { IDD = IDD_PARAM_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CStatic _apiInfoCtrl;
private:
	StApi *_pApi;
public:
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
public:
	COverwriteEdit _param1Val;
	COverwriteEdit _param2Val;
	COverwriteEdit _param3Val;
	COverwriteEdit _param4Val;
	COverwriteEdit _param5Val;
	COverwriteEdit _param6Val;
};
