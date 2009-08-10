#pragma once
#include "../../Libs/WinSupport/visvaledit.h"
#include "../../Libs/WinSupport/ColorListbox.h"
#include "../../Libs/WinSupport/dropedit.h"
#include "../../Libs/DevSupport/StMedia.h"
#include "Operation.h"
#include "resource.h"

#define DEFAULT_DRIVE_STR   _T(",,0,0xCD,0")
#define DRIVE_FORMAT_STR    _T("%s,%s,%u,0x%02X,%d")

// Control enable flags
#define DRIVE_NAME_ENABLE			0x01
#define DRIVE_DESC_ENABLE			0x02
#define DRIVE_TYPE_ENABLE			0x04
#define DRIVE_TAG_ENABLE			0x08
#define DRIVE_REQUESTED_SIZE_ENABLE	0x20
#define DATA_DRIVE_ENABLE_MASK  DRIVE_REQUESTED_SIZE_ENABLE
#define SYS_DRIVE_ENABLE_MASK   (DRIVE_NAME_ENABLE | DRIVE_DESC_ENABLE | DRIVE_REQUESTED_SIZE_ENABLE)


// CListCtrlEx
class CConfigOpListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CConfigOpListCtrl)
public:
	CConfigOpListCtrl();
	virtual ~CConfigOpListCtrl();
	afx_msg void OnPaint();
    void SetEmptyString(LPCTSTR _str){m_cs_empty_text=_str;};
protected:
	DECLARE_MESSAGE_MAP()
    CString m_cs_empty_text;
};

// COpUpdateDlg dialog

class COpUpdateDlg : public CDialog
{
	DECLARE_DYNAMIC(COpUpdateDlg)

public:
	COpUpdateDlg(CWnd* pParent = NULL, COpInfo* pInfo = NULL);      // standard constructor
	virtual ~COpUpdateDlg();

// Dialog Data
	enum { IDD = IDD_UPDATEOPDLG };

	media::LogicalDriveArray m_drive_array;				            // Local copy of current drive array configuration

private:
    void InitFileList();
//    void InitDriveTypeCombo();
    DWORD InitOptionsListBox(void);
    CString ConvertDriveToString(const media::LogicalDrive& driveDesc) const;
    media::LogicalDrive ConvertStringToDrive(CString& strDriveDesc);
    CString ConvertControlsToDriveStr();
    void UpdateControls();
    void SetDriveEditControlsEnabled();
    void UpdateFileListItem(int _index);
    void RemoveFileListItem();
    BOOL ValidateTagStr();
	void Localize();
	void UpdateListItem(BOOL _bInsert, int index, const media::LogicalDrive& driveDesc);
	void OnMenuClickedNew(void);
	void OnMenuClickedEdit(void);
	void OnMenuClickedDelete(void);
	void OnMenuClickedMoveUp(void);
	void OnMenuClickedMoveDown(void);
	void OnMenuClickedCopy(void);
	int CanCopy(media::LogicalDriveTag & _newTag);
	void SetDriveFlags(BOOL _state);


protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CDropEdit m_updater_fname;

    // Control values
    CVisValidEdit m_op_name_ctrl;
    CString m_str_edit_file_name;
    CString m_str_edit_description;
    CString m_str_edit_drive_tag;
    DWORD m_dw_edit_requested_drive_size;

    // Controls
    CButton m_ctrl_browse_file;
    CEdit m_ctrl_edit_description;
    CComboBox m_combo_drive_type;
    CEdit m_ctrl_edit_drive_tag;
    CEdit m_ctrl_edit_requested_drive_size;
	CListCtrl m_drive_list_ctrl;
    CConfigOpListCtrl m_op_options_ctrl;
	COpInfo * m_pOpInfo;
	int m_current_selection;
	media::LogicalDriveArray m_savedDriveArray;

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnLbnSelchangeDriveList();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    virtual BOOL OnInitDialog();
//    afx_msg void OnEnChangeEditFileName();
//    afx_msg void OnEnChangeEditDescription();
//    afx_msg void OnEnChangeEditDriveTag();
//    afx_msg void OnEnChangeEditRequestedDriveSize();
//    afx_msg void OnBnClickedBrowseFile();
//    afx_msg void OnBnClickedAddDrive();
//    afx_msg void OnCbnSelchangeComboDriveType();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnLvnItemchangedOpOptionsList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemActivateDriveList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBrowseFile();
};
