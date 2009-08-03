#pragma once
#include "afxwin.h"
#include "afxcmn.h"

class CStStatusButton :
	public CButton
{
public:
	CStStatusButton(void);
public:
	virtual ~CStStatusButton(void);

	void SetMouseMoveCallback(MOUSEMOVECALLBACK& mmcb);
	MOUSEMOVECALLBACK	m_MMCallBack;

protected:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
