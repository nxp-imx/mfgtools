/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "StCfgResOptions.h"


// CTargetDriveArrayDlg dialog

class CTargetDriveArrayDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CTargetDriveArrayDlg)

	friend class CDriveEditDlg;
public:
	CTargetDriveArrayDlg();   // standard constructor
	virtual ~CTargetDriveArrayDlg();

// Dialog Data
	enum { IDD = IDD_TARGET_DRIVE_ARRAY };

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	CListCtrl m_drive_list_ctrl;

private:
    void InitFileList(void);
	void UpdateListItem(BOOL _bInsert, int _index, const media::LogicalDrive& _driveDesc);
	void UpdateControls(void);

    void RemoveFileListItem();

	void OnMenuClickedNew(void);
	void OnMenuClickedEdit(void);
	void OnMenuClickedDelete(void);
	void OnMenuClickedMoveUp(void);
	void OnMenuClickedMoveDown(void);
	void OnMenuClickedCopy(void);
	int CanCopy(media::LogicalDriveTag & _newTag);

	int m_current_selection;
	media::LogicalDriveArray m_drive_array;


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnLbnSelchangeDriveList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemActivateDriveList(NMHDR *pNMHDR, LRESULT *pResult);

};
