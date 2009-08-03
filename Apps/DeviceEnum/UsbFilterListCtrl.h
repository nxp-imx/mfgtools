#pragma once


// CUsbFilterListCtrl

class CUsbFilterListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CUsbFilterListCtrl)

public:
	CUsbFilterListCtrl();
	virtual ~CUsbFilterListCtrl();
	CMenu m_usb_filter_menu;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


