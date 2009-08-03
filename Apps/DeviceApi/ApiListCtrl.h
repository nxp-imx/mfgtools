#pragma once


// CApiListCtrl

class CApiListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CApiListCtrl)

public:
	CApiListCtrl();
	virtual ~CApiListCtrl();
	CMenu _paramMenu;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


