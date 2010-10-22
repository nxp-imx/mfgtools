/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "OpInfo.h"
#include "../../Libs/WinSupport/visvaledit.h"
#include "../../Libs/WinSupport/colorlistbox.h"
#include "../../Libs/WinSupport/dropedit.h"

#define PATH_ERROR			-1
#define PATH_NOT_FOUND		0
#define PATH_IS_FILE		1
#define PATH_IS_FOLDER		2

// CLoadFileOpDlg dialog

class CLoadFileOpDlg : public CDialog
{
	DECLARE_DYNAMIC(CLoadFileOpDlg)

public:
	CLoadFileOpDlg(CWnd* pParent = NULL, COpInfo* pInfo = NULL);   // standard constructor
	virtual ~CLoadFileOpDlg();

// Dialog Data
	enum { IDD = IDD_LOADOPDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

    CVisValidEdit m_op_name_ctrl;
//    CVisValidEdit m_op_filename_ctrl;
	CDropEdit m_op_filename;
    CEdit m_op_timeout_ctrl;

    COpInfo* m_p_op_info;
	CString m_OutPath;
	CString m_original_folder;

public:
    afx_msg void OnEnChangeOpNameEdit();
    afx_msg void OnEnChangeFileNameEdit();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    virtual BOOL OnInitDialog();

private:
	void Localize();
public:
	afx_msg void OnBnClickedBrowseFile();
};
