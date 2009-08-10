#pragma once
#include "OpInfo.h"
#include "../../Libs/WinSupport/visvaledit.h"
//#include "mm_callback.h"
#include "FileDropListCtrl.h"
#include "StGrpTreeCtrl.h"

#define PATH_ERROR			-1
#define PATH_NOT_FOUND		0
#define PATH_IS_FILE		1
#define PATH_IS_FOLDER		2

// CCopyOpDlg dialog

class CCopyOpDlg : public CDialog
{
	DECLARE_DYNAMIC(CCopyOpDlg)

public:
	CCopyOpDlg(CWnd* pParent = NULL, COpInfo* pInfo = NULL);   // standard constructor
	virtual ~CCopyOpDlg();

// Dialog Data
	enum { IDD = IDD_COPYOPDLG };

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnPaint();
	HICON m_hIcon;

    virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	bool IsFileExist(CString sPathName);

    CVisValidEdit m_op_name_ctrl;
	CEdit m_op_timeout_ctrl;
	CComboBox m_op_drv_combo_ctrl;

    COpInfo* m_p_op_info;
	CString m_OutPath;
    CStatic m_static_data_drive_num_label;
    unsigned char m_data_drive_num;
	CString m_original_folder;
	CImageList m_image_list;
	CImageList m_folder_image_list;
	HTREEITEM m_hRoot;
	int m_op_numobjects;
	BOOL m_editing_label;

public:
    void SetOpInfo(COpInfo* _info) { m_p_op_info = _info; };
  //  DWORD InitFileListBox(void);
    afx_msg void OnEnChangeOpNameEdit();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnEnChangeEditDataDriveNum();
	afx_msg void OnFileListContextMenu(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnFolderListContextMenu(NMHDR *pNMHDR, LRESULT *pResult);

    static HRESULT CALLBACK OnTreeFileDropped(CTreeCtrl* pList,
											PVOID pCallerClass,
											CString& csPathname,
											UINT& iPathType);

	static HRESULT CALLBACK OnListFileDropped(PVOID pCallerClass,
                                     CString& csPathname,
                                     UINT& iPathType);

	CFileDropListCtrl m_op_file_list_ctrl;
	CStGrpTreeCtrl m_op_folders_ctrl;

private:
	void PopulateCopyOpFolderTree(void);
	void InsertSubFolders(HTREEITEM _hParent, CFileList * _pFileList);
	void OnMenuClickedAdd(void);
	void OnMenuClickedDelete(void);
	void OnMenuClickedNewDirectory(void);
	void OnMenuClickedAddDirectory(void);
	void OnMenuClickedDeleteDirectory(void);
	int CheckPath(CString sPath);
	void InitializeFileList(CFileList * _pFileList);
	void Localize();
	void AppendFileListItem(CFileList::PFILEITEM pItem);
	void EnumerateSourceFolderFiles(CFileList *pFileList);
	void InsertFolder(CString _path);

public:
	afx_msg void OnCbnSelchangeOpCopyDrvNumCombo();
	afx_msg void OnTvnSelchangedOpFoldertree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnEndlabeleditOpFoldertree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnBeginlabeleditOpFoldertree(NMHDR *pNMHDR, LRESULT *pResult);
};
