#pragma once
#include "afxcmn.h"

typedef struct _resourceList {
	void		*pNext;
	CString		csResName;
	CString		csResGroup;
} CONFIRM_LIST, *PCONFIRM_LIST;


// CUserConfirmation dialog

class CUserConfirmation : public CDialog
{
	DECLARE_DYNAMIC(CUserConfirmation)

public:
	CUserConfirmation(PCONFIRM_LIST _pList, CWnd* pParent = NULL);   // standard constructor
	virtual ~CUserConfirmation();

// Dialog Data
	enum { IDD = IDD_USERCONFIRM };

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl		m_resListCtrl;
	PCONFIRM_LIST	m_resList;

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
