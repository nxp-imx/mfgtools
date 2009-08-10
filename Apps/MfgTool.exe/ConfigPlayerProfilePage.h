#pragma once
#include "afxwin.h"
#include "resource.h"
#include "PlayerProfile.h"
#include "CProfileList.h"
#include "../../Libs/WinSupport/visvaledit.h"
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
	enum { IDD = IDD_CONF_PROFILE_DLG };

protected:
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

	CPlayerProfile *m_p_player_profile;

	CPlayerProfile *m_pNewPlayerProfile;

	CComboBox m_cb_player_profile_ctrl;
	CVisValidEdit m_vve_player_profile_ctrl;
//	CString m_cs_player_profile;
	CString m_cs_old_player_profile;
	CString m_cs_cur_player_profile;
//	CString m_cs_usb_vid;
	CVisValidEdit m_usb_vid_ctrl;
//	CString m_cs_usb_pid;
	CVisValidEdit m_usb_pid_ctrl;
//	CString m_cs_scsi_mfg;
	CVisValidEdit m_scsi_mfg_ctrl;
//	CString m_cs_scsi_product;
	CVisValidEdit m_scsi_product_ctrl;
	CVisValidEdit m_volume_label_ctrl;
//	CString m_cs_volume_label;
	CButton m_volume_label_check_ctrl;
	BOOL m_b_use_volume_label;
	CConfigPlayerListCtrl m_operations_ctrl;
    CStatic m_status_icon_ctrl;
	CStatic m_status_ctrl;
	HICON m_hi_ok;
	HICON m_hi_warning;
	HICON m_hi_error;
    CStringArray m_csa_supported_ops;

	virtual BOOL OnInitDialog();
	DWORD LoadControlsFromProfile(CPlayerProfile * _pProfile);
	afx_msg void OnBnClickedNewSave();
	afx_msg void OnBnClickedDelete();
	afx_msg void OnCbnSelchangeProductDescCombo();
	afx_msg LRESULT OnUpdateStatus(WPARAM _wparam, LPARAM _p_op_info);
    afx_msg void OnLvnItemChangedOperationsList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMKillfocusOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMSetfocusOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMClickOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnListCtrlContextMenu(UINT nID);
    void Localize(void);
	afx_msg LRESULT OnValidChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedVolumeLabelCheck();
    DWORD SaveProfile(CPlayerProfile *_pProfile);
    void InitListCtrl(CConfigPlayerListCtrl& list);
    DWORD InsertOpsList(CPlayerProfile * _pProfile);
	BOOL SaveProfileStrings(CPlayerProfile *_pProfile);
    DWORD CreateOperationFolder(COpInfo* pOpInfo, BOOL _overwrite = FALSE);
	void InitProfileListCombo();
//	void DoFolderCopy(CString sSourceFolder, CString sDestFolder);
	void PerformFileOps(COpInfo * _pOpInfo, CFileList * _pFileList, CString _destFolder, CFileList::FileListAction _action);
	int CheckPath(CString sPath);
	void SetDefaultProfile(void);
public:
	DWORD InitProfileList(void);
	CPlayerProfile *InitProfile(LPCTSTR _name = NULL);
	void SelectProfile(LPCTSTR _name);
	BOOL IsProfileValid(void) { if (m_p_player_profile) return m_p_player_profile->IsValid(); else return FALSE; };
	INT_PTR GetNumEnabledOps(void) { if (m_p_player_profile) return m_p_player_profile->GetNumEnabledOps(); else return 0; };
	LPCTSTR GetProfileName(void) { if (m_p_player_profile) return m_p_player_profile->GetName(); else return NULL; };
	CPlayerProfile* GetProfile(int _index = -1);
	BOOL IsNewProfileMode(void) { return m_bNewProfileMode; };
	LPCTSTR GetListProfileName(int index); // used to fill dialogbar combobox
protected:
	BOOL		m_bNewProfileMode;
	CButton		m_new_ctrl;
	CButton		m_delete_ctrl;
	int			m_iSelectedUpdate;
	CProfileList m_DeleteList;
	CProfileList m_ProfileList;
public:
    afx_msg void OnLvnKeydownOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMDblclkOperationsList(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	DWORD OpWorkerEnable(void);
    DWORD OpWorkerNew(COperation::OpTypes _opType);
    DWORD OpWorkerEdit(void);
	DWORD OpWorkerRemove(void);
    DWORD OpWorkerMove(INT_PTR _up);
};
