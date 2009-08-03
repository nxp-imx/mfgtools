/*********************************************************************
* HotProp Control, version 1.10 (January 14, 2004)
* Copyright (C) 2002-2004 Michal Mecinski.
*
* You may freely use and modify this code, but don't remove
* this copyright note.
*
* THERE IS NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, FOR
* THIS CODE. THE AUTHOR DOES NOT TAKE THE RESPONSIBILITY
* FOR ANY DAMAGE RESULTING FROM THE USE OF IT.
*
* E-mail: mimec@mimec.org
* WWW: http://www.mimec.org
********************************************************************/

#include "stdafx.h"
#include "HotPropCtrl.h"
#include "HPPopList.h"
#include "HPDynEdit.h"

//#include <afxdllx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// CHotPropCtrl

IMPLEMENT_DYNCREATE(CHotPropCtrl, CWnd)
/*
void CHotPropCtrl::PreSubclassWindow()
{
	CWnd::PreSubclassWindow();

	EnableToolTips(TRUE);
}
*/
CHotPropCtrl::CHotPropCtrl()
{
	m_pFirst = m_pLast = NULL;
	m_nHot = -1;
	m_pHot = NULL;
	m_bCapture = FALSE;
	m_bToggle = FALSE;
	m_bLocked = FALSE;
	m_nPop = -1;
	m_nYPos = 0;
	m_pList = NULL;
	m_pEdit = NULL;

	m_pWndToNotify = NULL;
	m_bSingleLine = FALSE;
	m_nItemHeight = 44;
	m_nTitleWidth = 0;

	OSVERSIONINFO osvi;
	memset(&osvi, 0, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);

#if (WINVER >= 0x0400)
	if (osvi.dwMajorVersion >= 4)
		m_font.CreateStockObject(DEFAULT_GUI_FONT);
	else
#endif
		m_font.CreatePointFont(80, _T("MS Sans Serif"));

	m_bFlatStyle = FALSE;
	m_bListAnim = FALSE;

	m_hMSImgLib = NULL;
	m_pfGradientFill = NULL;
}

CHotPropCtrl::~CHotPropCtrl()
{
	RemoveAllProps();

	if (m_hMSImgLib)
		FreeLibrary(m_hMSImgLib);
}


CHotPropCtrl::PropInfo::PropInfo(int nID, LPCTSTR lpszName, int nImage, int nType)
{
	m_nID = nID;
	m_nImage = nImage;
	m_strName = lpszName;
	m_nType = nType;
	m_nValue = -1;
	if (nType==propList || nType==propToggle)
	{
		m_pStrArray = new CStringArray;
		if (nType==propToggle)
		{
			m_pStrArray->Add(_T(""));
			m_pStrArray->Add(_T(""));
		}
	}
	else
		m_pStrArray = NULL;
	m_nState = 0;
	m_nStyle = 0;
	m_nMin = 0;
	m_nMax = nType==propEnum ? 10 : 1000;
	m_nCustom = 0;
	m_pNext = NULL;
}

CHotPropCtrl::PropInfo::~PropInfo()
{
	delete m_pStrArray;
}


BEGIN_MESSAGE_MAP(CHotPropCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_TIMER()
	ON_WM_CAPTURECHANGED()
	ON_MESSAGE(HPEM_ENDEDIT, OnEndEdit)
	ON_MESSAGE(HPPM_ENDPOP, OnEndPop)
	ON_MESSAGE(HPPM_DRAWITEM, OnDrawPopItem)
END_MESSAGE_MAP()

//////4///////////////////////////////////////////////////////////////////////
// CHotPropCtrl public methods

PTCHAR testString = _T("Test String!");

INT_PTR CHotPropCtrl::OnToolHitTest( CPoint point, TOOLINFO* pTI) const
{
	CPoint pt = point;
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	pTI->uFlags = TTF_NOTBUTTON|TTF_CENTERTIP;
	pTI->uId = 	(point.y + m_nYPos) / m_nItemHeight;
	pTI->hwnd = GetParent()->GetSafeHwnd();

	return pTI->uId;
}

void CHotPropCtrl::AddProp(int nID, int nType, LPCTSTR lpszName, int nImage)
{
	PropInfo* pProp = new PropInfo(nID, lpszName, nImage, nType);

	if (m_pLast)	// insert in the end of the list
		m_pLast = m_pLast->m_pNext = pProp;
	else
		m_pLast = m_pFirst = pProp;
}

void CHotPropCtrl::AddProp(int nID, int nType, UINT uNameID, int nImage)
{
	CString strName;
	strName.LoadString(uNameID);
	AddProp(nID, nType, strName, nImage);
}

void CHotPropCtrl::InsertProp(int nID, int nAfter, int nType, LPCTSTR lpszName, int nImage)
{
	PropInfo* pProp = new PropInfo(nID, lpszName, nImage, nType);

	PropInfo* pPred;
	if (nAfter>=0)
	{
		FindProp(nAfter, pPred);
		if (pPred)
		{
			if (pPred->m_pNext)		// insert after pPred
				pProp->m_pNext = pPred->m_pNext;
			else
				m_pLast = pPred;
			pPred->m_pNext = pProp;
			return;
		}
	}
	if (m_pFirst)	// insert in the beginning
		pProp->m_pNext = m_pFirst;
	else
		m_pLast = pProp;
	m_pFirst = pProp;
}				

void CHotPropCtrl::InsertProp(int nID, int nAfter, int nType, UINT uNameID, int nImage)
{
	CString strName;
	strName.LoadString(uNameID);
	InsertProp(nID, nAfter, nType, strName, nImage);
}

void CHotPropCtrl::SetPropName(int nID, LPCTSTR lpszName)
{
	PropInfo* pProp;
	int nProp = FindVisProp(nID, pProp);
	if (pProp)
	{
		pProp->m_strName = lpszName;

		int nLastWidth = m_nTitleWidth;
		CalculateTitleWidth();

		if (m_nTitleWidth != nLastWidth)
			Invalidate(FALSE);
		else
			InvalidateProp(nProp);	// redraw immediately
	}
}

void CHotPropCtrl::SetPropName(int nID, UINT uNameID)
{
	CString strName;
	strName.LoadString(uNameID);
	SetPropName(nID, strName);
}

void CHotPropCtrl::SetPropImage(int nID, int nImage)
{
	PropInfo* pProp;
	int nProp = FindVisProp(nID, pProp);
	if (pProp)
	{
		pProp->m_nImage = nImage;
		InvalidateProp(nProp);
	}
}

void CHotPropCtrl::SetPropLimit(int nID, int nMax)
{
	SetPropLimits(nID, 0, nMax);
}

void CHotPropCtrl::SetPropLimits(int nID, int nMin, DWORD nMax)
{
	PropInfo* pProp;
	int nProp = FindProp(nID, pProp);
	if (nProp>=0)
	{
		pProp->m_nMin = nMin;	// just change the values
		pProp->m_nMax = nMax;
	}
}

void CHotPropCtrl::SetPropToggle(int nID, LPCTSTR lpszFalse, LPCTSTR lpszTrue)
{
	PropInfo* pProp;
	int nProp = FindProp(nID, pProp);
	if (nProp>=0)
	{
		ASSERT(pProp->m_nType==propToggle);

		pProp->m_pStrArray->SetAt(0, lpszFalse);
		pProp->m_pStrArray->SetAt(1, lpszTrue);
	}
}

void CHotPropCtrl::SetPropToggle(int nID, UINT uFalseID, UINT uTrueID)
{
	CString strFalse, strTrue;
	strFalse.LoadString(uFalseID);
	strTrue.LoadString(uTrueID);
	
	SetPropToggle(nID, strFalse, strTrue);
}

CStringArray& CHotPropCtrl::GetPropStringArray(int nID) const
{
	PropInfo* pProp;
	FindProp(nID, pProp);

	ASSERT(pProp && pProp->m_pStrArray);	// it has to exist

	return *pProp->m_pStrArray;
}

void CHotPropCtrl::SetCustomProp(int nID, int nCustom)
{
	PropInfo* pProp;
	FindProp(nID, pProp);
	if (pProp)
	{
		pProp->m_nCustom = nCustom;
	}
}

void CHotPropCtrl::RemoveProp(int nID)
{
	PropInfo* pPrev;
	PropInfo* pProp;

	if (FindProp(nID, pPrev, pProp)>=0)
	{
		PreRemove();	// make sure it's safe to remove

		if (pPrev)
			pPrev->m_pNext = pProp->m_pNext;
		else
			m_pFirst = pProp->m_pNext;
		if (m_pLast == pProp)
			m_pLast = pPrev;

		delete pProp;
	}		
}

void CHotPropCtrl::RemoveAllProps()
{
	PreRemove();

	while (m_pFirst)	// delete everything
	{
		PropInfo* pNext = m_pFirst->m_pNext;
		delete m_pFirst;
		m_pFirst = pNext;
	}
	m_pLast = NULL;
	m_nTitleWidth = 0;
}


void CHotPropCtrl::Update()
{
	CalculateTitleWidth();
	Invalidate(FALSE);	// force to redraw everything

	CRect rcClient;
	GetClientRect(&rcClient);
	OnSize(0, rcClient.right, rcClient.bottom);	// update scroll info
	SetScrollPos(SB_VERT, m_nYPos);

	if (m_bCapture)
	{
		CPoint m_ptCur;
		GetCursorPos(&m_ptCur);
		ScreenToClient(&m_ptCur);

		OnMouseMove(0, m_ptCur);	// update hot item under the cursor
	}
}


void CHotPropCtrl::SetProp(int nID, DWORD dwValue)
{
	PropInfo* pProp;
	int nProp = FindVisProp(nID, pProp);
	if (pProp)
	{
		pProp->m_nValue = dwValue;
		if (dwValue<0)	// display nothing
			pProp->m_strValue.Empty();
		else if (pProp->m_pStrArray)	// value from the string array
			pProp->m_strValue = pProp->m_pStrArray->GetAt(dwValue);
		else	// integer value
		{
			int dataSize = 0; 
			DWORD tempMax = pProp->m_nMax;
			while ( tempMax & 0x0F )
			{
				++dataSize;
				tempMax >>= 4;
			}
			CString strFormat;
			strFormat.Format(_T("0x%%0%dX"), dataSize);
			pProp->m_strValue.Format(strFormat, dwValue);
		}
		InvalidateProp(nProp);	// redraw immediately
	}
}

void CHotPropCtrl::SetProp(int nID, LPCTSTR lpszValue)
{
	PropInfo* pProp;
	int nProp = FindVisProp(nID, pProp);
	if (pProp)
	{
		pProp->m_nValue = 0;
		pProp->m_strValue = lpszValue;
		InvalidateProp(nProp);
	}
}


INT_PTR CHotPropCtrl::GetProp(int nID) const
{
	PropInfo* pProp;
	if (FindProp(nID, pProp)>=0)
		return pProp->m_nValue;
	return -1;
}

CString CHotPropCtrl::GetPropStr(int nID) const
{
	PropInfo* pProp;
	if (FindProp(nID, pProp)>=0)
		return pProp->m_strValue;
	return (LPCTSTR)NULL;
}


void CHotPropCtrl::LoadImages(int nID, COLORREF crMask)
{
	m_imgList.Create(nID, 16, 0, crMask);	// initialize the image list
	m_imgList.SetBkColor(RGB(255,255,255));
}

CImageList& CHotPropCtrl::GetImageList() const
{
	return (CImageList&)m_imgList;
}


void CHotPropCtrl::ProcessKey(int nKey)
{
	int nProp = 0;
	for (PropInfo* pProp=m_pFirst; pProp; pProp=pProp->m_pNext)
	{
		if (pProp->m_nStyle & PIS_HIDDEN)
			continue;

		if (pProp->m_nStyle & (PIS_LOCKED | PIS_DISABLED))
		{
			nProp++;
			continue;
		}

		CString strName = pProp->m_strName;

		for (int i=0; i<strName.GetLength(); i++)
		{
			TCHAR ch = strName[i];
			if (ch==_T('&'))
			{
				ch = strName[++i];
				if (toupper(ch)==toupper(nKey))	// check if the shortcut matches
				{
					if (pProp->m_nType!=propToggle && nProp!=m_nHot)
					{
						if (m_nHot>=0)
						{
							m_pHot->m_nState = 0;
							InvalidateProp(m_nHot);
						}
						m_nHot = nProp;	// make this property the hot one
						m_pHot = pProp;
					}

					EnsureVisible(pProp->m_nID);

					switch (pProp->m_nType)
					{
					case propList:
					case propEnum:
						m_pHot->m_nState = PIF_HOTBAR | PIF_HOTITEM | PIF_ONITEM;
						CreateList(TRUE);	// display the list
						break;

					case propButton:
						NotifyView(pProp);
						break;

					case propToggle:
						pProp->m_nValue = !pProp->m_nValue;		// toggle the value
						pProp->m_strValue = pProp->m_pStrArray->GetAt(pProp->m_nValue);

						NotifyView(pProp);	// post the message immediately
			
						InvalidateProp(nProp);
						break;

					case propString:
					case propInt:
						m_pHot->m_nState = PIF_HOTBAR | PIF_HOTITEM | PIF_ONITEM;
						CreateEdit(CPoint(0,0));	// create the dynamic edit control
						break;
					}
					return;
				}
			}
		}
		nProp++;
	}
}

void CHotPropCtrl::EnableProp(int nID, BOOL bEnable)
{
	PropInfo* pProp;
	int nProp = FindVisProp(nID, pProp);
	if (pProp)
	{
		if (bEnable)
			pProp->m_nStyle &= ~PIS_DISABLED;
		else
			pProp->m_nStyle |= PIS_DISABLED;
		InvalidateProp(nProp);
	}
}

void CHotPropCtrl::LockProp(int nID, BOOL bLock)
{
	PropInfo* pProp;
	int nProp = FindVisProp(nID, pProp);
	if (pProp)
	{
		if (bLock)
			pProp->m_nStyle |= PIS_LOCKED;
		else
			pProp->m_nStyle &= ~PIS_LOCKED;
		InvalidateProp(nProp);
	}
}

void CHotPropCtrl::SetPropBar(int nID, BOOL bBar)
{
	PropInfo* pProp;
	int nProp = FindVisProp(nID, pProp);
	if (pProp)
	{
		if (bBar)
			pProp->m_nStyle |= PIS_BAR;
		else
			pProp->m_nStyle &= ~PIS_BAR;
		InvalidateProp(nProp);
	}
}

void CHotPropCtrl::ShowProp(int nID, BOOL bShow, BOOL bUpdate)
{
	PropInfo* pProp;
	FindProp(nID, pProp);
	if (pProp)
	{
		int nStyle = pProp->m_nStyle;
		if (bShow)
			nStyle &= ~PIS_HIDDEN;
		else
			nStyle |= PIS_HIDDEN;

		if (nStyle != pProp->m_nStyle)
		{
			pProp->m_nStyle = nStyle;
			if (bUpdate)
				Update();
		}
	}
}

void CHotPropCtrl::ShowPropsRng(int nFirstID, int nLastID, BOOL bShow, BOOL bUpdate)
{
	BOOL bChange = FALSE;
	PropInfo* pProp = m_pFirst;

	while (pProp)	// search the list
	{
		if (pProp->m_nID >= nFirstID && pProp->m_nID <= nLastID)
		{
			int nStyle = pProp->m_nStyle;
			if (bShow)
				nStyle &= ~PIS_HIDDEN;
			else
				nStyle |= PIS_HIDDEN;

			if (nStyle != pProp->m_nStyle)
			{
				pProp->m_nStyle = nStyle;
				bChange = TRUE;
			}
		}
		pProp = pProp->m_pNext;
	}
	if (bChange && bUpdate)
		Update();
}


COLORREF CHotPropCtrl::LightenColor(COLORREF crColor,double dFactor)
{
	BYTE r = GetRValue(crColor);
	BYTE g = GetGValue(crColor);
	BYTE b = GetBValue(crColor);
	r = (BYTE)((dFactor*(255-r)) + r);
	g = (BYTE)((dFactor*(255-g)) + g);
	b = (BYTE)((dFactor*(255-b)) + b);
	return RGB(r,g,b);
}

void CHotPropCtrl::EnsureVisible(int nID)
{
	PropInfo* pProp;
	int nProp = FindVisProp(nID, pProp);
	if (nProp>=0)
	{
		if (nProp*m_nItemHeight < m_nYPos)	// scroll up
		{
			m_nYPos = nProp*m_nItemHeight;
			SetScrollPos(SB_VERT, m_nYPos);
			Invalidate(FALSE);
		}
		else
		{
			CRect rcClient;
			GetClientRect(&rcClient);	// scroll down
			if ((nProp+1)*m_nItemHeight > m_nYPos + rcClient.bottom)
			{
				m_nYPos = (nProp+1)*m_nItemHeight - rcClient.bottom;
				SetScrollPos(SB_VERT, m_nYPos);
				Invalidate(FALSE);
			}
		}
	}
}


void CHotPropCtrl::SetWndToNotify(CWnd* pWndToNotify)
{
	m_pWndToNotify = pWndToNotify;
}


void CHotPropCtrl::SetSingleLine(BOOL bSingleLine)
{
	m_bSingleLine = bSingleLine;
	m_nItemHeight = (m_bSingleLine ? 24 : 44);

	Update();
}


void CHotPropCtrl::EnableFlatStyle(BOOL bFlatStyle, BOOL bRedraw)
{
	m_bFlatStyle = bFlatStyle;
	if (bRedraw)
		Invalidate(FALSE);
}

void CHotPropCtrl::EnableGradient(BOOL bGradient, BOOL bRedraw)
{
	m_pfGradientFill = NULL;

	if (bGradient)
	{
		if (!m_hMSImgLib)
			m_hMSImgLib = LoadLibrary(_T("msimg32.dll"));
	
		if (m_hMSImgLib)
			m_pfGradientFill = (GRADIENTFILL_PROC)GetProcAddress(m_hMSImgLib, "GradientFill");
	}

	if (bRedraw)
		Invalidate(FALSE);
}

void CHotPropCtrl::EnableListAnimation(BOOL bListAnim)
{
	m_bListAnim = bListAnim;
}


/////////////////////////////////////////////////////////////////////////////
// CHotPropCtrl private methods

int CHotPropCtrl::FindProp(int nID, CHotPropCtrl::PropInfo*& pProp) const
{
	int nCnt = 0;
	pProp = m_pFirst;

	while (pProp)	// search the list
	{
		if (pProp->m_nID == nID)
			return nCnt;

		pProp = pProp->m_pNext;
		nCnt++;
	}

	return -1;	// not found
}

int CHotPropCtrl::FindProp(int nID, CHotPropCtrl::PropInfo*& pPrev, CHotPropCtrl::PropInfo*& pProp) const
{
	int nCnt = 0;
	pPrev = NULL;
	pProp = m_pFirst;

	while (pProp)
	{
		if (pProp->m_nID == nID)
			return nCnt;

		pPrev = pProp;	// remember the previous prop
		pProp = pProp->m_pNext;
		nCnt++;
	}

	return -1;
}

int CHotPropCtrl::FindVisProp(int nID, CHotPropCtrl::PropInfo*& pProp) const
{
	int nCnt = 0;
	pProp = m_pFirst;

	while (pProp)
	{
		if (!(pProp->m_nStyle & PIS_HIDDEN))
		{
			if (pProp->m_nID == nID)
				return nCnt;
			nCnt++;
		}
		else if (pProp->m_nID == nID)
			return -1;
		pProp = pProp->m_pNext;
	}
	return -1;
}


void CHotPropCtrl::PreRemove()
{
	if (m_bLocked)
	{
		m_bLocked = FALSE;	// destroy dynamic controls
		if (m_pEdit)
			m_pEdit->DestroyWindow();
		if (m_pList)
			m_pList->DestroyWindow();
		m_pEdit = NULL;
		m_pList = NULL;
	}

	ReleaseCapture();	// this will set m_pHot to NULL
}

void CHotPropCtrl::DrawProp(CDC& dc, CHotPropCtrl::PropInfo *pProp, const CRect &rcRect)
{
	int cy = rcRect.top;
	int nState = pProp->m_nState;
	int nStyle = pProp->m_nStyle;

	BOOL bSym = (nState & (PIF_ONITEM | PIF_HOTBAR))	// display arrow on the right
		&& (pProp->m_nType <= propEnum || pProp->m_nType == propToggle);
	int nPush = nState & PIF_PUSHED ? 1 : 0;

	if  (pProp->m_nImage>=0)
	{
		if (nStyle & PIS_DISABLED)
			DrawDisabled(dc, 3, cy+2, pProp->m_nImage);	// draw grayed image
		else
			m_imgList.Draw(&dc, pProp->m_nImage, CPoint(3, cy+2), ILD_TRANSPARENT);
	}

	CRect rcTemp = rcRect;

	if ((nStyle & (PIS_LOCKED | PIS_DISABLED)) == PIS_LOCKED)
	{
		if (m_bSingleLine)
			rcTemp.right = m_nTitleWidth;

		dc.SelectStockObject(BLACK_PEN);	// draw a black padlock on the bar
		dc.SelectStockObject(NULL_BRUSH);
		int cx = rcTemp.right - 12;
		dc.Ellipse(cx-2, cy+5, cx+2, cy+12);
		dc.FillSolidRect(cx-3, cy+9, 6, 5, 0);

		if (m_bSingleLine)
			rcTemp.DeflateRect(24, 0, 18, 0);
		else
			rcTemp.DeflateRect(24, 0, 18, 24);
	}
	else
	{
		if (m_bSingleLine)
		{
			rcTemp.right = m_nTitleWidth;
			rcTemp.DeflateRect(24, 0, 4, 0);
		}
		else
			rcTemp.DeflateRect(24, 0, 4, 24);
	}

	if (nStyle & PIS_DISABLED)		// grayed text
	{
		if (m_bFlatStyle)
			dc.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
		else
		{
			dc.SetTextColor(GetSysColor(COLOR_3DHILIGHT));
			dc.DrawText(pProp->m_strName, &(rcTemp+CPoint(1,1)), DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
			dc.SetTextColor(GetSysColor(COLOR_3DSHADOW));
		}
		dc.DrawText(pProp->m_strName, &rcTemp, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
	}
	else
	{
		dc.SetTextColor(0);
		dc.DrawText(pProp->m_strName, &rcTemp, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
	}

	rcTemp = rcRect;
	if (m_bSingleLine)
		rcTemp.right = m_nTitleWidth;
	else
		rcTemp.bottom -= 24;

	BOOL bBar = (nState & PIF_HOTBAR) && !(nStyle & PIS_DISABLED);

	if (m_bFlatStyle)
	{
		if (nStyle & PIS_BAR)
			dc.DrawEdge(&rcTemp, EDGE_RAISED, BF_RECT);
		else if (bBar)
			dc.DrawEdge(&rcTemp, BDR_RAISEDINNER, BF_RECT);
	}
	else
		dc.DrawEdge(&rcTemp, (bBar || nStyle & PIS_BAR) ? EDGE_RAISED : BDR_RAISEDINNER, BF_RECT);

	int nCut = bSym ? 24 : 12;

	if (nState & PIF_HOTITEM)
	{
		rcTemp = rcRect;

		COLORREF crHigh = GetSysColor(COLOR_HIGHLIGHT);

		if (pProp->m_nType <= propToggle)
		{
			if (m_bSingleLine)
				rcTemp.DeflateRect(m_nTitleWidth + 4, 2, 4, 3);
			else
				rcTemp.DeflateRect(8, 20+2, 8, 2);

			if (m_bFlatStyle)
			{
				dc.FillSolidRect(&rcTemp, LightenColor(crHigh, nState & PIF_PUSHED ? 0.5 : 0.7));
				dc.Draw3dRect(&rcTemp, crHigh, crHigh);
			}
			else
			{
				dc.DrawEdge(&rcTemp, nState & PIF_PUSHED	// border around the value
					? BDR_SUNKENOUTER : BDR_RAISEDINNER, BF_RECT);
			}
		}
		else
		{
			if (m_bSingleLine)
				rcTemp.DeflateRect(m_nTitleWidth + 4, 2, 4, 3);
			else
				rcTemp.DeflateRect(20, 20+2, 20, 2);

			if (nState & PIF_WHITEBG)	// white while editing
				dc.FillSolidRect(&rcTemp, RGB(255,255,255));	// draw sunken border
			else if (m_bFlatStyle)
				dc.FillSolidRect(&rcTemp, LightenColor(crHigh, 0.7));

			if (m_bFlatStyle)
			{
				COLORREF crFrame = nState & PIF_WHITEBG
					? LightenColor(crHigh, 0.4) : crHigh;
				dc.Draw3dRect(&rcTemp, crFrame, crFrame);
			}
			else
				dc.DrawEdge(&rcTemp, BDR_SUNKENOUTER, BF_RECT);
		}
	}

	if (!(nStyle & PIS_DISABLED || nState & PIF_WHITEBG))
	{
		rcTemp = rcRect;	// the property value
		if (pProp->m_nCustom)
		{
			if (m_bSingleLine)
				rcTemp.DeflateRect(m_nTitleWidth + 8+nPush, 4+nPush, 24, 4-nPush);
			else
				rcTemp.DeflateRect(24+nPush, 20+4+nPush, 24, 4-nPush);

			DrawCustomItem(dc, rcTemp, pProp, pProp->m_nValue);
		}
		else
		{
			if (m_bSingleLine)
				rcTemp.DeflateRect(m_nTitleWidth + 8+nPush, nPush, nCut, -nPush);
			else
				rcTemp.DeflateRect(24+nPush, 20+nPush, nCut, -nPush);

			dc.DrawText(pProp->m_strValue, &rcTemp, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
		}
	}
	
	if (bSym)
	{
		dc.SetTextColor(GetSysColor(nState & PIF_HOTITEM ? COLOR_HIGHLIGHT : COLOR_WINDOWTEXT));

		CFont font;
		font.CreatePointFont(pProp->m_nType==propToggle ? 80 : 100, _T("Marlett"));
		CFont* pFont = dc.SelectObject(&font);

		rcTemp = rcRect;

		if (m_bSingleLine)
			rcTemp.DeflateRect(m_nTitleWidth + 20, 2+nPush, 7-nPush, 2-nPush);
		else
			rcTemp.DeflateRect(20, 22+nPush, 11-nPush, 2-nPush);
			
		dc.DrawText(pProp->m_nType==propToggle ? _T("v") : _T("u"),	// draw popup/toggle arrow
			1, &rcTemp, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);

		dc.SelectObject(pFont);
	}
}

void CHotPropCtrl::DrawDisabled(CDC& dc, int x, int y, int nImage)
{
	CDC dcBmp;
	dcBmp.CreateCompatibleDC(&dc);

	struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[2];
	} bmInfo = {{
		sizeof(BITMAPINFOHEADER),
		16, 16, 1, 1, BI_RGB, 0, 0, 0, 0, 0
	}, {
	  { 0x00, 0x00, 0x00, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }
	}};

	VOID *pBits;	// create a black&white bitmap
	HBITMAP hBitmap = CreateDIBSection(dcBmp.m_hDC, (LPBITMAPINFO)&bmInfo,
		DIB_RGB_COLORS, &pBits, NULL, 0);

	SelectObject(dcBmp.m_hDC, hBitmap);

	// draw the image into the bitmap
	m_imgList.Draw(&dcBmp, nImage, CPoint(0, 0), ILD_NORMAL);

	CBrush brush, *pOld;
	if (m_bFlatStyle)
	{
		brush.CreateSolidBrush(GetSysColor(COLOR_GRAYTEXT));
		pOld = dc.SelectObject(&brush);
		dc.BitBlt(x, y, 16, 16, &dcBmp, 0, 0, 0xB8074A);
	}
	else
	{
		brush.CreateSolidBrush(GetSysColor(COLOR_3DHILIGHT));
		pOld = dc.SelectObject(&brush);
		dc.BitBlt(x+1, y+1, 16, 16, &dcBmp, 0, 0, 0xB8074A);	// render with the hilight color
		brush.DeleteObject();

		brush.CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
		dc.SelectObject(&brush);
		dc.BitBlt(x, y, 16, 16, &dcBmp, 0, 0, 0xB8074A);	// render with the shadow color
	}
	dc.SelectObject(pOld);

	DeleteObject(hBitmap);
}

void CHotPropCtrl::InvalidateProp(int nProp)
{
	if (nProp >= 0)
	{
		CRect rcRect;
		GetClientRect(&rcRect);
		rcRect.top = nProp*m_nItemHeight - m_nYPos;
		rcRect.bottom = rcRect.top + m_nItemHeight;	// calc the rectangle of the property
		InvalidateRect(&rcRect, FALSE);
	}
}


void CHotPropCtrl::CreateList(BOOL bKey)
{
	CRect rcList;
	GetClientRect(&rcList);
	rcList.top = m_nHot * m_nItemHeight - m_nYPos;
	rcList.bottom = rcList.top + m_nItemHeight;

	if (m_bSingleLine)
		rcList.DeflateRect(m_nTitleWidth + 4, 2, 4, 3);
	else
		rcList.DeflateRect(8, 22, 8, 2);
		
	ClientToScreen(&rcList);
	INT_PTR nSize = m_pHot->m_pStrArray ? m_pHot->m_pStrArray->GetSize() : m_pHot->m_nMax;
	m_pList = new CHPPopList(rcList, m_pHot->m_nValue, bKey, nSize,
		m_bListAnim, m_bFlatStyle, &m_font, this);

	if (m_pList && m_pList->m_hWnd)
	{
		m_bLocked = TRUE;

		ReleaseCapture();
		
		m_pHot->m_nState |= PIF_PUSHED;
		InvalidateProp(m_nHot);
	}
}

void CHotPropCtrl::CreateEdit(CPoint point)
{
	CRect rcEdit;
	GetClientRect(&rcEdit);
	rcEdit.top = m_nHot * m_nItemHeight - m_nYPos;
	rcEdit.bottom = rcEdit.top + m_nItemHeight;

	if (m_bSingleLine)
		rcEdit.DeflateRect(m_nTitleWidth + 8, 5, 10, 5);
	else
		rcEdit.DeflateRect(24, 25, 23, 6);
		
	if (m_pHot->m_nType==propString)
		m_pEdit = new CHPDynEdit(rcEdit, m_pHot->m_strValue,
			m_pHot->m_nMax, point, 0, &m_font, this);	// string input
	else
	{
		int dataSize = 2; // start with 2 for the "0x" characters 
		DWORD tempMax = m_pHot->m_nMax;
		while ( tempMax & 0x0F )
		{
			++dataSize;
			tempMax >>= 4;
		}
		m_pEdit = new CHPDynEdit(rcEdit,	m_pHot->m_strValue,
			dataSize, point, /*ES_NUMBER*/0, &m_font, this);	// integer input // modified by CLW. Need to be able to use Hex values.
	}

	if (m_pEdit && m_pEdit->m_hWnd)
	{
		m_bLocked = TRUE;

		ReleaseCapture();
		
		m_pHot->m_nState |= PIF_WHITEBG;
		InvalidateProp(m_nHot);
	}
}


void CHotPropCtrl::NotifyView(CHotPropCtrl::PropInfo* pInfo)
{
	if (!pInfo)
		pInfo = m_pHot;

	CWnd* pView = m_pWndToNotify;
	if (!pView)
 		pView = GetParent();

	if (pView)
		pView->PostMessage(HPCM_UPDATE, pInfo->m_nID, pInfo->m_nValue);
}


void CHotPropCtrl::DrawCustomItem(CDC& dc, CRect& rcRect, CHotPropCtrl::PropInfo* pProp, UINT_PTR nValue)
{
	if (CWnd* pView = ((CFrameWnd*)AfxGetMainWnd())->GetActiveView())
	{
		CustomItem citem;
		citem.m_pDC = &dc;
		citem.m_rcRect = rcRect;
		citem.m_nID = pProp->m_nID;
		citem.m_nValue = nValue;
		citem.m_lpszValue = pProp->m_pStrArray ? pProp->m_pStrArray->GetAt(nValue) : pProp->m_strValue;
		pView->SendMessage(HPCM_DRAWCUSTOM, pProp->m_nCustom, (LPARAM)&citem);
	}
}

void CHotPropCtrl::GradFillH(CDC& dc, int x, int y, int cx, int cy, COLORREF crLeft, COLORREF crRight)
{
	if (!m_pfGradientFill)
		return;
	
	TRIVERTEX vertex[2] ;
	GRADIENT_RECT gradrect;

	vertex[0].x = x;
	vertex[0].y = y;
	vertex[0].Red = GetRValue(crLeft) << 8;
	vertex[0].Green = GetGValue(crLeft) << 8;
	vertex[0].Blue = GetBValue(crLeft) << 8;
	vertex[0].Alpha = 0;

	vertex[1].x = x + cx;
	vertex[1].y = y + cy;
	vertex[1].Red = GetRValue(crRight) << 8;
	vertex[1].Green = GetGValue(crRight) << 8;
	vertex[1].Blue = GetBValue(crRight) << 8;
	vertex[1].Alpha = 0;

	gradrect.UpperLeft  = 0;
	gradrect.LowerRight = 1;

	(m_pfGradientFill)(dc.m_hDC, vertex, 2, &gradrect, 1, GRADIENT_FILL_RECT_H);
}


/////////////////////////////////////////////////////////////////////////////
// CHotPropCtrl message handlers

BOOL CHotPropCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	CWnd::PreCreateWindow(cs);

	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
		LoadCursor(NULL, IDC_ARROW), NULL, NULL);

	return TRUE;
}

void CHotPropCtrl::PostNcDestroy()
{
	delete this;
}


void CHotPropCtrl::OnPaint()
{
	CPaintDC dc(this);

	CRect rcClient;
	GetClientRect(&rcClient);

	COLORREF crBack = GetSysColor(COLOR_BTNFACE);	// bg color
	COLORREF crDark = LightenColor(crBack, -0.5);
	COLORREF crMed = LightenColor(crBack, 0.1);
	COLORREF crLight = LightenColor(crBack, 0.5);

	dc.SelectObject(&m_font);
	dc.SetBkMode(TRANSPARENT);


	int y = -m_nYPos;
	for (PropInfo* pProp=m_pFirst; pProp; pProp=pProp->m_pNext)
	{
		if (pProp->m_nStyle & PIS_HIDDEN)
			continue;

		if (m_bSingleLine)
		{
			if (m_pfGradientFill)
				GradFillH(dc, 0, y, m_nTitleWidth, 24, crMed, crDark);
			else
				dc.FillSolidRect(0, y, m_nTitleWidth, 24, crBack);
			dc.FillSolidRect(m_nTitleWidth, y, rcClient.right - m_nTitleWidth, 24, crLight);
			dc.FillSolidRect(m_nTitleWidth, y+m_nItemHeight-1, rcClient.right - m_nTitleWidth, 1, crDark);
		}
		else
		{
			if (m_pfGradientFill)
				GradFillH(dc, 0, y, rcClient.right, 20, crMed, crDark);
			else
				dc.FillSolidRect(0, y, rcClient.right, 20, crBack);
			dc.FillSolidRect(0, y+20, rcClient.right, 24, crLight);
		}
		
		DrawProp(dc, pProp, CRect(0, y, rcClient.right, y+m_nItemHeight));	// draw the property

		y += m_nItemHeight;
	}

	rcClient.top = y;
	if (rcClient.top < rcClient.bottom)
	{
		if (m_pfGradientFill)
		{
			if (m_bSingleLine)
			{
				GradFillH(dc, 0, y, m_nTitleWidth, rcClient.bottom - y, crMed, crDark);
				dc.FillSolidRect(m_nTitleWidth, y, rcClient.right - m_nTitleWidth, rcClient.bottom - y, crBack);
			}
			else
				GradFillH(dc, 0, y, rcClient.right, rcClient.bottom - y, crMed, crDark);
		}
		else		
			dc.FillSolidRect(&rcClient, crBack);	// area below all properties

		if (!m_bFlatStyle)
		{
			if (m_bSingleLine)
				rcClient.right = m_nTitleWidth;
			dc.DrawEdge(&rcClient, BDR_RAISEDINNER, BF_RECT);
		}
	}
}


void CHotPropCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bLocked)
		return;

	m_nPop = -1;

	CRect rcClient;
	GetClientRect(&rcClient);

	if (m_bToggle)	// toggle mode (LMB pressed)
	{
		int nState = PIF_HOTBAR | PIF_ONITEM | m_pHot->m_nState & PIF_HOTITEM;

		point.y -= m_nHot * m_nItemHeight - m_nYPos;

		if (m_bSingleLine)
		{
			rcClient.bottom = m_nItemHeight;
			rcClient.left = m_nTitleWidth;
			rcClient.DeflateRect(4, 2, 4, 3);
		}
		else
		{
			rcClient.top = 20;
			rcClient.bottom = m_nItemHeight;
			rcClient.DeflateRect(8, 2);
		}

		if (rcClient.PtInRect(point))
			nState |= PIF_PUSHED;

		if (nState != m_pHot->m_nState)
		{
			m_pHot->m_nState = nState;
			InvalidateProp(m_nHot);
		}

		return;
	}

	if (!m_bCapture)
	{
		SetCapture();	// capture it so we know when the cursor leaves our control
		m_bCapture = TRUE;
	}
	else
	{
		CPoint ptScr = point;
		ClientToScreen(&ptScr);	// check if cursor above the control
		if (!rcClient.PtInRect(point) || WindowFromPoint(ptScr)!=this)
		{
			ReleaseCapture();	// this will restore the hot item to normal state
			return;
		}
	}

	int nProp = (point.y + m_nYPos) / m_nItemHeight;

	if (nProp!=m_nHot && m_nHot>=0)
	{
		m_pHot->m_nState = 0;
		InvalidateProp(m_nHot);	// restore old hot item
		m_nHot = -1;
		m_pHot = NULL;
	}

	int i=0;
	for (PropInfo* pProp=m_pFirst; pProp; pProp=pProp->m_pNext)
	{
		if (pProp->m_nStyle & PIS_HIDDEN)
			continue;
		if (nProp == i++)
		{
			int nState = PIF_HOTBAR | pProp->m_nState & PIF_HOTITEM;
			m_nHot = nProp;
			m_pHot = pProp;

			point.y -= nProp * m_nItemHeight - m_nYPos;

			if (m_bSingleLine)
			{
				rcClient.bottom = m_nItemHeight;
				rcClient.left = m_nTitleWidth;
				rcClient.DeflateRect(4, 2, 4, 3);
			}
			else
			{
				rcClient.top = 20;
				rcClient.bottom = m_nItemHeight;
			
				if (pProp->m_nType <= propToggle)
					rcClient.DeflateRect(8, 2);
				else
					rcClient.DeflateRect(20, 2);
			}
			
			if (rcClient.PtInRect(point) && !(pProp->m_nStyle & (PIS_LOCKED | PIS_DISABLED)))
			{
				nState |= PIF_ONITEM;	// cursor over the property value
				SetTimer(1, 150, NULL);	// redraw it after 150 ms if the mouse stops moving
			}
			else
				nState &= ~PIF_HOTITEM;	// FIX: 1.5

			if (nState != pProp->m_nState)
			{
				pProp->m_nState = nState;
				InvalidateProp(nProp);
			}
			break;
		}
	}
}

void CHotPropCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bLocked || m_nPop>=0)
	{
		int nPop = m_nPop;
		
		SetFocus();	// this will destroy the dynamic control
		OnMouseMove(nFlags, point);	// update the hot item

		if (m_nHot==nPop)
			return;	// prevent from unfolding the same list again
	}

	if (m_nHot>=0)
	{
		if (m_pHot->m_nState & PIF_ONITEM)
		{
			m_pHot->m_nState |= PIF_HOTITEM;

			switch (m_pHot->m_nType)
			{
			case propList:
			case propEnum:
				CreateList(FALSE);	// unfold the list
				break;

			case propButton:
			case propToggle:
				m_bToggle = TRUE;	// enter the toggle mode
				m_pHot->m_nState |= PIF_PUSHED;
				InvalidateProp(m_nHot);
				break;

			case propString:
			case propInt:
				CreateEdit(point);	// create dynamic edit control
				break;
			}
		}
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CHotPropCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bToggle)
	{
		if (m_pHot->m_nState & PIF_PUSHED)
		{
			if (m_pHot->m_nType==propToggle)
			{
				m_pHot->m_nValue = !m_pHot->m_nValue;	// change the value
				m_pHot->m_strValue = m_pHot->m_pStrArray->GetAt(m_pHot->m_nValue);
			}
			NotifyView();
		}
		m_bToggle = FALSE;

		OnMouseMove(nFlags, point);	// update the hot item
	}
}


LRESULT CHotPropCtrl::OnEndEdit(WPARAM bDone, LPARAM)
{
	m_bLocked = FALSE;

	if (m_nHot<0)
	{
		m_pEdit = NULL;	// editing was interrupted
		return 0;
	}

	if (bDone)
	{
		m_pEdit->GetWindowText(m_pHot->m_strValue);

		if (m_pHot->m_nType==propInt)	// update the value
		{
// modified by CLW to support Hex numbers
			TCHAR *pEnd = NULL;
			DWORD dwVal = _tcstoul(m_pHot->m_strValue.GetBuffer(), &pEnd, 0);
			if (dwVal < (DWORD)m_pHot->m_nMin)	// bounds checking
				dwVal = m_pHot->m_nMin;
			if (dwVal > m_pHot->m_nMax)
				dwVal = m_pHot->m_nMax;

			m_pHot->m_nValue = dwVal;
//			m_pHot->m_strValue.Format(_T("0x%08X (%d)"), dwVal, dwVal);
			int dataSize = 0; 
			DWORD tempMax = m_pHot->m_nMax;
			while ( tempMax & 0x0F )
			{
				++dataSize;
				tempMax >>= 4;
			}
			CString strFormat;
			strFormat.Format(_T("0x%%0%dX"), dataSize);
			m_pHot->m_strValue.Format(strFormat, dwVal);
		}
		else
			m_pHot->m_nValue = 0;

		NotifyView();
	}

	m_pHot->m_nState = 0;
	InvalidateProp(m_nHot);
	m_nHot = -1;
	m_pHot = NULL;

	m_pEdit = NULL;

	return 0;
}

LRESULT CHotPropCtrl::OnEndPop(WPARAM nRes, LPARAM)
{
	m_bLocked = FALSE;
	m_pList = NULL;

	if ((int)nRes>=0)
	{
		m_pHot->m_nValue = (int)nRes;	// change prop value
		if (m_pHot->m_pStrArray && m_pHot->m_pStrArray->GetSize() )
			m_pHot->m_strValue = m_pHot->m_pStrArray->GetAt(nRes);
		else
		{
//			m_pHot->m_strValue.Format(_T("0x%08X (%d)"), nRes, nRes);
			int dataSize = 0; 
			DWORD tempMax = m_pHot->m_nMax;
			while ( tempMax & 0x0F )
			{
				++dataSize;
				tempMax >>= 4;
			}
			CString strFormat;
			strFormat.Format(_T("0x%%0%dX"), dataSize);
			m_pHot->m_strValue.Format(strFormat, nRes);
		}
		NotifyView();
	}

	m_nPop = m_nHot;	// prevent from unfolding again when clicked

	m_pHot->m_nState = 0;
	InvalidateProp(m_nHot);
	m_nHot = -1;
	m_pHot = NULL;

	return 0;
}


LRESULT CHotPropCtrl::OnDrawPopItem(WPARAM nItem, LPARAM pInfo)
{
	CHPPopList::DrawInfo* pDrawInfo = (CHPPopList::DrawInfo*)pInfo;

	if (m_pHot->m_nCustom)
	{
		CRect rcTemp = pDrawInfo->m_rcRect;
		rcTemp.right -= 12;
		DrawCustomItem(*pDrawInfo->m_pDC, rcTemp, m_pHot, nItem);
	}
	else	// standard item
	{
		CString strItem;
		if (m_pHot->m_pStrArray)
			strItem = m_pHot->m_pStrArray->GetAt(nItem);
		else
			strItem.Format(_T("%d"), nItem);

		pDrawInfo->m_pDC->DrawText(strItem, &pDrawInfo->m_rcRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
	}
	return 0;
}											


void CHotPropCtrl::OnSize(UINT nType, int cx, int cy)
{
	int nProps = 0;
	for (PropInfo* pProp=m_pFirst; pProp; pProp=pProp->m_pNext)
		if (!(pProp->m_nStyle & PIS_HIDDEN))
			nProps++;	// count visible props

	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	sinfo.nMin = 0;
	sinfo.nMax = nProps*m_nItemHeight;
	sinfo.nPage = cy+1;
	sinfo.nPos = m_nYPos;

	SetScrollInfo(SB_VERT, &sinfo, FALSE);

	m_nHeight = max(1, cy/m_nItemHeight) * m_nItemHeight;

	m_nYMax = nProps*m_nItemHeight - cy;	// calc new position
	if (m_nYMax < 0)
		m_nYMax = 0;
	if (m_nYPos > m_nYMax)
		m_nYPos = m_nYMax;

	CalculateTitleWidth();

	CWnd::OnSize(nType, cx, cy);
}


void CHotPropCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int nYLast = m_nYPos;

	switch (nSBCode)	// change vertical position
	{
	case SB_PAGEUP:
		m_nYPos -= m_nHeight;
		break;

	case SB_PAGEDOWN:
		m_nYPos += m_nHeight;
		break;

	case SB_LINEUP:
		m_nYPos -= m_nItemHeight;
		break;

	case SB_LINEDOWN:
		m_nYPos += m_nItemHeight;
		break;
	
	case SB_THUMBTRACK:
		m_nYPos = nPos;
		break;
	}

	if (m_nYPos < 0)
		m_nYPos = 0;
	else if (m_nYPos > m_nYMax)
		m_nYPos = m_nYMax;

	ScrollWindow(0, nYLast - m_nYPos);

	SetScrollPos(SB_VERT, m_nYPos);
}


void CHotPropCtrl::OnTimer(UINT nIDEvent)
{
	if (nIDEvent==1)	// for delayed hot tracking
	{
		if (m_pHot && (m_pHot->m_nState & (PIF_ONITEM | PIF_HOTITEM))==PIF_ONITEM)
		{
			m_pHot->m_nState |= PIF_HOTITEM;
			InvalidateProp(m_nHot);		// redraw when the mouse stops for a short time
		}
		KillTimer(1);
	}
}


void CHotPropCtrl::OnCaptureChanged(CWnd *pWnd)
{
	m_bCapture = FALSE;
	m_bToggle = FALSE;

	if (!m_bLocked && m_nHot>=0)
	{
		m_pHot->m_nState = 0;	// restore the hot prop to normal state
		InvalidateProp(m_nHot);
		m_nHot = -1;
		m_pHot = NULL;
	}
}

void CHotPropCtrl::CalculateTitleWidth()
{
	m_nTitleWidth = 0;

	if (!m_bSingleLine)
		return;

	int nLongestName = 0;

	CDC* pDC = GetDC();
	CFont* pOldFont = pDC->SelectObject(&m_font);

	PropInfo* pProp = m_pFirst;
	while (pProp)	// search the list
	{
		CSize szText = pDC->GetTextExtent(pProp->m_strName);
		nLongestName = max(nLongestName, szText.cx);

		pProp = pProp->m_pNext;
	}

	pDC->SelectObject(pOldFont);

	// add 12 so lock symbol won't truncate the longest title
	m_nTitleWidth = nLongestName + 36 + 12;

	CRect rcClient;
	GetClientRect(&rcClient);
//	m_nTitleWidth = min(m_nTitleWidth, rcClient.right / 2);
}

