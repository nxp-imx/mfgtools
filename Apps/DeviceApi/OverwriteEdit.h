#pragma once

#define WM_MSG_EN_CHANGE	WM_USER+100

// COverwriteEdit

class COverwriteEdit : public CEdit
{
	DECLARE_DYNAMIC(COverwriteEdit)

public:
	COverwriteEdit();
	virtual ~COverwriteEdit();
	CMenu _paramMenu;
	void ShowMenu(bool show=true) { _bShowMenu = show; };

protected:
	bool _bOverwrite;
	bool _bShowMenu;
	CString _theString;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


