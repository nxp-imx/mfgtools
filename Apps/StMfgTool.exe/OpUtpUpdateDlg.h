#pragma once
#include "OpInfo.h"
#include "../../Libs/WinSupport/visvaledit.h"
#include "../../Libs/WinSupport/dropedit.h"
#include "UpdateCommandList.h"

class MyFileDialog : public CFileDialog
{
public:
	MyFileDialog( BOOL bOpen, LPCTSTR ext, LPCTSTR filename, DWORD flags,  LPCTSTR filter, CWnd* parent, DWORD size, BOOL bVista )
		: CFileDialog( bOpen, ext, filename, flags, filter, parent, size, bVista)
	{}

	void OnTypeChange();
};

// COpUtpUpdateDlg dialog

class COpUtpUpdateDlg : public CDialog
{
	DECLARE_DYNAMIC(COpUtpUpdateDlg)

public:
	COpUtpUpdateDlg(CWnd* pParent = NULL, COpInfo* pInfo = NULL);   // standard constructor
	virtual ~COpUtpUpdateDlg();

// Dialog Data
	enum { IDD = IDD_UTPUPDATEOPDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

	CVisValidEdit m_vve_op_name_ctrl;
	CDropEdit     m_ucl_fname;
    CComboBox     m_combo_ucl_lists;
	CListBox      m_ucl_file_list_ctrl;
	CString		  m_required_files_filter;
	UCL			  m_UclNode;

    COpInfo*	m_pOpInfo;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedUclBrowseFile();
	afx_msg void OnUclFilenameChanged();
	afx_msg void OnCbnSelchangeOpUtpUpdateUclListCombo();

private:
	void Localize();
	void LoadCommandLists(LPCTSTR filename);
	CString SetFileFilter();
public:
	afx_msg void OnBnClickedRequiredFilesBrowseBtn();
};
