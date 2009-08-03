#include "StdAfx.h"
#include "resource.h"
#include "mm_callback.h"
#include "StResGrpTreeCtrl.h"

CStResGrpTreeCtrl::CStResGrpTreeCtrl(void)
{
}

CStResGrpTreeCtrl::~CStResGrpTreeCtrl(void)
{
}


BEGIN_MESSAGE_MAP(CStResGrpTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CStResGrpTreeCtrl)
	ON_WM_DROPFILES()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////



BOOL CStResGrpTreeCtrl::SetDropMode(CStResGrpTreeCtrl::DROPTREEMODE& dropMode)
{
	m_dropMode = dropMode;

	// Register Tree control as a drop target	
	m_CTreeDropTarget.Register(this);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//	Handle the WM_DROPFILES message
//

void CStResGrpTreeCtrl::OnDropFiles(HDROP dropInfo)
{

	if(m_dropMode.pfnCallback)
		m_dropMode.pfnCallback(this, m_dropMode.pCallerClass, dropInfo);

	//Remove highlighting
	SendMessage(TVM_SELECTITEM, TVGN_DROPHILITE,0);
}


void CStResGrpTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	m_MMCallBack.pfnCallback(m_MMCallBack.pCallerClass, m_MMCallBack.id);

	CTreeCtrl::OnMouseMove(nFlags, point);
}


void CStResGrpTreeCtrl::SetMouseMoveCallback(MOUSEMOVECALLBACK& pMMCB)
{
	m_MMCallBack.id = pMMCB.id;
	m_MMCallBack.pCallerClass = pMMCB.pCallerClass;
	m_MMCallBack.pfnCallback = pMMCB.pfnCallback;
}

