#pragma once
#include "OpInfo.h"
#include "../../Libs/WinSupport/visvaledit.h"
#include "../../Libs/WinSupport/dropedit.h"
#include "UpdateCommandList.h"

// COpMxRomUpdateDlg dialog

class COpMxRomUpdateDlg : public CDialog
{
	DECLARE_DYNAMIC(COpMxRomUpdateDlg)

public:
	COpMxRomUpdateDlg(CWnd* pParent = NULL, COpInfo* pInfo = NULL);   // standard constructor
	virtual ~COpMxRomUpdateDlg();

// Dialog Data
	enum { IDD = IDD_MXUPDATEOPDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

	CVisValidEdit m_vve_op_name_ctrl;
	CDropEdit     m_rkl_fname;
	CListCtrl     m_image_list_ctrl;
	CString		  m_image_files_filter;

    COpInfo*	m_pOpInfo;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedRklBrowseFile();
	afx_msg void OnRklFilenameChanged();
	afx_msg void OnBnClickedImageFilesBrowseBtn();

private:
	void Localize();
	CString SetFileFilter();
};
