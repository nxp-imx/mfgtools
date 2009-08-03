#include "StdAfx.h"
#include "mm_callback.h"
#include "StStatusButton.h"

CStStatusButton::CStStatusButton(void)
{
}

CStStatusButton::~CStStatusButton(void)
{
}

BEGIN_MESSAGE_MAP(CStStatusButton, CButton)
	//{{AFX_MSG_MAP(CStStatusButton)
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CStStatusButton::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	m_MMCallBack.pfnCallback(m_MMCallBack.pCallerClass, m_MMCallBack.id);

	CButton::OnMouseMove(nFlags, point);
}


void CStStatusButton::SetMouseMoveCallback(MOUSEMOVECALLBACK& pMMCB)
{
	m_MMCallBack.id = pMMCB.id;
	m_MMCallBack.pCallerClass = pMMCB.pCallerClass;
	m_MMCallBack.pfnCallback = pMMCB.pfnCallback;
}