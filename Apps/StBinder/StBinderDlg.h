/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StBinderDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"
#include "stversion.h"
#include "mm_callback.h"
#include "FileDropListCtrl.h"
#include "StResGrpTreeCtrl.h"
#include "StStatusButton.h"
#include "..\\..\\common\\updater_res.h"
#include "..\\..\\common\\updater_restypes.h"


// CStBinderDlg dialog
class CStBinderDlg : public CDialog
{
// Construction
public:
	CStBinderDlg(CWnd* pParent = NULL);	// standard constructor
	~CStBinderDlg(void);

// Dialog Data
	enum { IDD = IDD_STBINDER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

//    afx_msg void OnContextMenu(CWnd*, CPoint point);
  //  void CStBinderDlg::ShowPopupMenu( CPoint& point );

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnBnClickedFindTarget();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnLvnInsertitemBinariesList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnDeleteitemBinariesList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);

    static HRESULT CALLBACK OnListFileDropped(CListCtrl* pList,
										  PVOID pCallerClass,
                                          CString& csPathname,
                                          UINT& iPathType);

    static HRESULT CALLBACK OnTreeFileDropped(CTreeCtrl* pList,
										  PVOID pCallerClass,
                                          HDROP dropInfo);

	static HRESULT CALLBACK OnEnumFwResource(PVOID pCallerClass,
										HMODULE _hModule,
										CString& csResName,
										int _iResId,
										WORD& wLangId);
	static HRESULT CALLBACK OnEnumRecvResource(PVOID pCallerClass,
										HMODULE _hModule,
										int _iResId,
										WORD& wLangId);

    static HRESULT CALLBACK OnControlMouseMove(PVOID pCallerClass,
                                          UINT iResourceId);

	static void InsertFileFromProfile(HTREEITEM _hItem, CString& _csPathname, PVOID pCallerClass);
    static void InsertFileIntoList(HTREEITEM _hItem, CListCtrl* pList, PVOID pCallerClass, CString& _csPathname);
    void InsertBoundResourceIntoList(HTREEITEM _hItem, USHORT _iResId, CString _csResName, DWORD _dwSize, CString szVersion, USHORT _tagId);

	void PopulateResourceTree(void);
	BOOL UpdateTargetInfo();
	USHORT GetNumberOfOpsToPerform();
	CStResGrpTreeCtrl m_ResourceTree;
	CStStatusButton m_FindTargetBtn;
	CStStatusButton m_ApplyBtn;
	CStStatusButton m_CloseBtn;
	HTREEITEM m_hRoot;
	HTREEITEM m_hRecvResources;
	HTREEITEM m_hFirmwareResources;
	HTREEITEM m_hOtherFirmware;
    CEdit m_target_name;
    unsigned int m_total_file_count;
    unsigned int m_bound_resource_count;
    CFileDropListCtrl m_ResourceListCtl;
	CStVersion *m_pUpdaterVersion;
	CStatic m_updater_version;
	CStatic m_updater_base_SDK;
	CStatic m_updater_date;
	CStatic m_updater_filesize;
    CStatic m_total_files;
    CStatic m_bound_resources;
	CStatic m_status_text;
	CImageList m_ImageList;
	BOOL m_LocalSelectionChanged;
	BOOL m_Busy;
	UINT m_StatusTextId;
	CString m_ResListDesc;
	CString m_ResTreeDesc;
	CString m_FindTargetDesc;
	CString m_ApplyDesc;
	CString m_CloseDesc;
	CString m_ConfigureDesc;
	CString m_LoadProfileDesc;
	CString m_SaveProfileDesc;
	CString m_LastUpdaterPath;
	CString m_LastFirmwarePath;
	CString m_LastRecoveryDriverPath;
	int		m_TargetMajorVer;
	int		m_TargetMinorVer;


private:
//	CImageList m_ImageList;
	void OnMenuClickedDelete();
	void OnMenuClickedDeleteResourceGroup();
	void OnMenuClickedExtract();
    void OnMenuClickedBrowse( HTREEITEM hCurrentItem );

	void GetLastPaths();
	void SaveLastPaths();
	int  CheckProfileCount();

public:
	afx_msg void OnTvnSelchangedResourceTree(NMHDR *pNMHDR, LRESULT *pResult);
	void RefreshResourceView(BOOL _bRefreshAllGroups);
	BOOL ApplyResources();
	virtual void UpdateProgress(BOOL _step_it=TRUE);
    BOOL CheckForReplacements(int _iResId, CString _csFileName, BOOL _state);
	int GetResourceID(CString _fname);
    void GetResourceName(int resId, CString& _fname);
	BOOL CheckForDuplicateOrInvalidFilename(HTREEITEM _hItem, CString& _csPathname);
    void CheckForExistingResources();
    BOOL UserConfirmation();
    BOOL DeleteMarkedResources(HANDLE _hResUpdate);
    BOOL ExtractResource(LPTSTR _szPath, HTREEITEM _hItem, int _iResId);

	CProgressCtrl m_progress_ctrl;

	afx_msg void OnLvnKeydownBinariesList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickBinariesList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickResourcetree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void OnBnClickedDlgTargetConfigure();
	CStStatusButton m_configure_btn;
	CStStatusButton m_loadprofile_btn;
	CStStatusButton m_saveprofile_btn;
	afx_msg void OnBnClickedCfgSaveProfile();
	afx_msg void OnBnClickedCfgLoadProfile();
};
