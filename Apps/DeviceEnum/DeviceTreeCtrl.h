#pragma once


// CDeviceTreeCtrl

class CDeviceTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CDeviceTreeCtrl)

public:
	CDeviceTreeCtrl();
	virtual ~CDeviceTreeCtrl();

protected:
	CMenu m_eject_menu;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


