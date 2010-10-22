/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxwin.h"
#include "resource.h"
#include "PlayerProfile.h"
#include "CProfileList.h"
//#include "../../Libs/WinSupport/visvaledit.h"
#include "ConfigPlayerListCtrl.h"

#define COL_OP_TYPE		0
#define COL_OP_NAME		1
#define COL_OP_DETAIL	2
#define COL_OP_OPTIONS	3



// CConfigPlayerProfilePage dialog

class CConfigPlayerProfilePage : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigPlayerProfilePage)

public:
	CConfigPlayerProfilePage(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConfigPlayerProfilePage();
	virtual void OnOK();
	virtual void OnCancel();

// Dialog Data
	enum { IDD = IDD_CONF_PROFILE_DLG2 };

protected:
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

	CPlayerProfile* m_p_player_profile;

	CComboBox m_cb_player_profile_ctrl;
	CString m_cs_old_player_profile;
	CString m_cs_cur_player_profile;
	CConfigPlayerListCtrl m_operations_ctrl;
    CStatic m_status_icon_ctrl;
	CStatic m_status_ctrl;
	HICON m_hi_ok;
	HICON m_hi_warning;
	HICON m_hi_error;

	virtual BOOL OnInitDialog();
	DWORD LoadControlsFromProfile(CPlayerProfile * _pProfile);
	afx_msg void OnCbnSelchangeProductDescCombo();
	afx_msg LRESULT OnUpdateStatus(WPARAM _wparam, LPARAM _p_op_info);
    afx_msg void OnLvnItemChangedOperationsList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMKillfocusOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMSetfocusOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMClickOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnListCtrlContextMenu(UINT nID);
    void Localize(void);
    DWORD SaveProfile(CPlayerProfile *_pProfile);
    void InitListCtrl(CConfigPlayerListCtrl& list);
    DWORD InsertOpsList(CPlayerProfile * _pProfile);
	void InitProfileListCombo();
//	void PerformFileOps(COpInfo * _pOpInfo, CFileList * _pFileList, CString _destFolder, CFileList::FileListAction _action);
	int CheckPath(CString sPath);
	void SetDefaultProfile(void);
public:
	DWORD InitProfileList(void);
	CPlayerProfile* InitProfile(LPCTSTR _name = NULL);
	void SelectProfile(LPCTSTR _name);
	BOOL IsProfileValid(void) { if (m_p_player_profile) return m_p_player_profile->IsValid(); else return FALSE; };
	INT_PTR GetNumEnabledOps(void) { if (m_p_player_profile) return m_p_player_profile->GetNumEnabledOps(); else return 0; };
	LPCTSTR GetProfileName(void) { if (m_p_player_profile) return m_p_player_profile->GetName(); else return NULL; };
	CPlayerProfile* GetProfile(int _index = -1);
	LPCTSTR GetListProfileName(int index); // used to fill dialogbar combobox
protected:
	int			m_iSelectedUpdate;
	CProfileList m_ProfileList;
public:
    afx_msg void OnLvnKeydownOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMDblclkOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	DWORD OpWorkerEnable(void);
    DWORD OpWorkerEdit(void);
    DWORD OpWorkerMove(INT_PTR _up);
};
