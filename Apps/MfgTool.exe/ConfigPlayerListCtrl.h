#pragma once
#include "PlayerProfile.h"

#define WM_UPDATE_STATUS WM_USER	+ 11
#define COL_OP_TYPE		0
#define COL_OP_NAME		1
#define COL_OP_DETAIL	2
#define COL_OP_OPTIONS	3

#define IDM_ENABLED (WM_APP + 11)
/*
#define IDM_NEW		(WM_APP + 12)
#define IDM_EDIT	(WM_APP + 13)
#define IDM_DELETE	(WM_APP + 14)
#define IDM_MOVE_UP	(WM_APP + 15)
#define IDM_MOVE_DOWN	(WM_APP + 16)
*/

#define IDM_NEW_UPDATE_OP	  (WM_APP + 17)
#define IDM_NEW_COPY_OP		  (WM_APP + 18)
#define IDM_NEW_LOAD_OP		  (WM_APP + 19)
#define IDM_NEW_OTP_OP		  (WM_APP + 20)
#define IDM_NEW_UTP_UPDATE_OP (WM_APP + 21)


///////////////////////////////////////////////////////////////////////////////
// CConfigOpInfoListCtrl data

struct OPLISTCTRLDATA
{
	// ctor
	OPLISTCTRLDATA()
	{
		bEnabled             = TRUE;
		crText               = ::GetSysColor(COLOR_WINDOWTEXT);
		crBackground         = ::GetSysColor(COLOR_WINDOW);
		nStateImage          = -1;
		strToolTip           = _T("");
		dwItemData           = 0;
	}

	BOOL		bEnabled;				// TRUE = enabled, FALSE = disabled (gray text)
	int			nStateImage;			// index in state image list, else -1
	CString		strToolTip;				// tool tip text for cell
	COLORREF	crText;
	COLORREF	crBackground;
	DWORD		dwItemData;				// pointer to app's data
};
// CListCtrlEx
class CConfigPlayerListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CConfigPlayerListCtrl)

public:
	CConfigPlayerListCtrl();
	virtual ~CConfigPlayerListCtrl();
	void SetEditMode(BOOL _edit_mode);
	CImageList m_il_state;
    DWORD_PTR GetItemData(int nItem);
    BOOL SetItemData(int nItem, DWORD_PTR dwData);
	int		InsertItem(int nItem, LPCTSTR lpszItem);
	int		InsertItem(int nItem, 
					   LPCTSTR lpszItem, 
					   COLORREF crText, 
					   COLORREF crBackground);
	int		InsertItem(const LVITEM* pItem);
	int		SetItem(const LVITEM* pItem);
    BOOL SetItemText(int nItem, int nSubItem, LPCTSTR lpszText,	COLORREF crText, COLORREF crBackground);
	BOOL SetItemToolTipText(int nItem, int nSubItem, LPCTSTR lpszToolTipText);
	BOOL	DeleteAllItems();
	BOOL	DeleteItem(int nItem);
	CString GetItemToolTipText(int nItem, int nSubItem);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnNMCustomDraw(NMHDR *pNMHDR, LRESULT *pResult);
	virtual afx_msg BOOL OnToolTipText(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
	virtual BOOL OnToolHitTest(CPoint point, TOOLINFO * pTI) const;
    afx_msg void OnDestroy();
    int GetColumns(void);
    void UpdateSubItem(int nItem, int nSubItem);
public:
//    afx_msg int OnCharToItem(UINT nChar, CListBox* pListBox, UINT nIndex);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


