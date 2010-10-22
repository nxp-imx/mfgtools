/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "../../Libs/DevSupport/StMedia.h"
#include "opinfo.h"
#include "../../Libs/Winsupport/dropedit.h"
#include "../../Libs/WinSupport/visvaledit.h"

// CDriveEditDlg dialog

#define MAX_DATA_DRIVES			2
#define MAX_HIDDEN_DRIVES		2
#define MAX_BOOTMGR_DRIVES		3
#define MAX_HOSTLINK_RSC_DRIVES	3
#define MAX_PLAYER_RSC_DRIVES	3
#define MAX_LBABOOT_DRIVES		1

#define MAX_TAGS				22
#define NO_PREREQUISITES		(media::LogicalDriveTag)0xFFFFFFFF

typedef struct
{
	media::LogicalDriveType type;
	LPCTSTR  szTag;
	media::LogicalDriveTag tag;
	media::LogicalDriveTag prerequisite;
	media::LogicalDriveFlag Flags;
} TAGLISTSTRUCT, *PTAGLISTSTRUCT;


class CDriveEditDlg : public CDialog
{
	DECLARE_DYNAMIC(CDriveEditDlg)

public:
	CDriveEditDlg(CWnd* pParent = NULL, BOOL _bNew = FALSE, COpInfo* _pInfo = NULL, int _index = -1 );   // standard constructor
	virtual ~CDriveEditDlg();

// Dialog Data
	enum { IDD = IDD_EDIT_DRIVE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    COpInfo *m_pOpInfo;
	int		m_index;
	BOOL	m_bIsNewDrive;
	CString m_original_filename;
//	TAGLISTSTRUCT m_TagList[MAX_TAGS];

    // Control values
    CVisValidEdit m_op_name_ctrl;
//    CString m_str_edit_file_name;
    CString m_str_edit_description;
//    CString m_str_edit_drive_tag;
    DWORD m_dw_edit_requested_drive_size;

    // Controls
//    CEdit m_ctrl_edit_file_name;
    CDropEdit m_ctrl_edit_file_name;
    CButton m_ctrl_browse_file;
    CEdit m_ctrl_edit_description;
    CComboBox m_combo_drive_type;
    CComboBox m_combo_drive_tag;
//    CEdit m_ctrl_edit_drive_tag;
    CEdit m_ctrl_edit_requested_drive_size;

	void SetDlgItems(const media::LogicalDrive& _driveDesc);
	BOOL GetDlgItems(media::LogicalDrive& _driveDesc);

	DECLARE_MESSAGE_MAP()

private:
    void InitDriveTypeCombo();
	void InitDriveTagCombo();
    BOOL ValidateTagStr();
	void Localize();

public:
	BOOL CheckForPrerequisite(media::LogicalDriveTag _tag);
	BOOL CheckForDependents();
    virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedBrowseFile();
	afx_msg void OnCbnSelchangeComboDriveType();
	afx_msg void OnCbnSelchangeComboDriveTag();
};
