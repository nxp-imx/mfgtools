// VisValEdit.cpp : implementation file
//
// ****************************************************
// *                                                  *
// *     CVisValidEdit by stevie_mac@talk21.com       *
// *                                                  *
// ****************************************************
//
// Based on an original idea by joseph. 
// EMail him at - newcomer@flounder.com or visit his excellent
// site at http://www.pgh.net/~newcomer/index.htm

#include "stdafx.h"
#include "VisValEdit.h"
#include <float.h>
#include ".\visvaledit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVisValidEdit

CVisValidEdit::CVisValidEdit()
{
	//brushes
	m_brInvalid.CreateSolidBrush(	RGBDEF_INVALID);
	m_brIncomplete.CreateSolidBrush(RGBDEF_INCOMPLETE);
	m_brEmpty.CreateSolidBrush(		RGBDEF_EMPTY);
	m_brValid.CreateSolidBrush(		RGBDEF_VALID);
	m_brCustom1.CreateSolidBrush(	RGBDEF_CUST1);
	m_brCustom2.CreateSolidBrush(	RGBDEF_CUST2);
	//boolean options
	m_bCustomValidation = FALSE;
	m_bNoValidation = FALSE;
	m_bDoLengthCheck = FALSE;
	m_bAllowEmpty = FALSE;
	m_bAllowAllCharEntry = FALSE;
	m_bCustomValidationCallOK = FALSE;
	m_bCustomErrorIsValid[1]  = FALSE;
	m_bCustomErrorIsValid[2]  = FALSE;
	m_bEnableRangeCheck = TRUE;
	//boolean states
	m_bValid = FALSE;
	m_bValidLength = FALSE;
	m_bValidSyntax = FALSE;
	m_bEnableErrorMessages = TRUE;
	// control_identifier
	m_my_id = 0;
	//default charset
	m_setCurrent = SET_ALLCHAR;
	//create storage for the 'more info' strings - 1 for each set
// VVEset {SET_CUSTOM, SET_DIRPATH, SET_ALLCHAR, SET_ALPHANUM, SET_ALPHA, SET_DECIMAL, SET_HEXADECIMAL, SET_OCTAL, SET_BINARY, SET_FLOAT };
	for (int i = SET_CUSTOM; i <= SET_FLOAT; i++) 
	{
		m_arrMoreStrs.Add(_T("None"));
	}
	//initialising others
	SetValidCharSet(m_setCurrent);
	m_iCurrentErr = BR_EMPTY;
	m_iMaxInputLen = VVE_DEF_MAXLEN;
	m_iMinInputLen = VVE_DEF_MINLEN;
	m_arrErrStrs.Add(VVE_DEFSTRERROR);
	m_arrErrStrs.Add(VVE_DEFSTREMPTY);
	m_arrErrStrs.Add(VVE_DEFSTRVALID);
	m_arrErrStrs.Add(VVE_DEFSTRINVALID);
	m_arrErrStrs.Add(VVE_DEFSTRINCOMPLETE);
	m_arrErrStrs.Add(VVE_DEFSTRSHORT);
	m_arrErrStrs.Add(VVE_DEFSTRLONG);
	m_arrErrStrs.Add(VVE_DEFSTRRANGEERR);
	m_arrErrStrs.Add(VVE_DEFSTRCUST1);
	m_arrErrStrs.Add(VVE_DEFSTRCUST2);


	//init default strings 
	SetMoreInfoString(SET_CUSTOM);
	SetMoreInfoString(SET_ALLCHAR);
	SetMoreInfoString(SET_ALPHANUM);
	SetMoreInfoString(SET_ALPHA);
	SetMoreInfoString(SET_DECIMAL);
	SetMoreInfoString(SET_SDECIMAL);
	SetMoreInfoString(SET_HEXADECIMAL);
	SetMoreInfoString(SET_OCTAL);
	SetMoreInfoString(SET_BINARY);
	SetMoreInfoString(SET_FLOAT);
	SetMoreInfoString(SET_DIRPATH);
}

CVisValidEdit::~CVisValidEdit()
{
	m_brInvalid.DeleteObject();
	m_brIncomplete.DeleteObject();
	m_brEmpty.DeleteObject();
	m_brValid.DeleteObject();
}


BEGIN_MESSAGE_MAP(CVisValidEdit, CEdit)
	//{{AFX_MSG_MAP(CVisValidEdit)
	ON_WM_CHAR()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
	ON_WM_CTLCOLOR_REFLECT()
	ON_CONTROL_REFLECT_EX(EN_CHANGE, OnChange)
    ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CVisValidEdit message handlers

BOOL CVisValidEdit::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch(wParam) 
	{
	case ID_CONTEXT_INFO:
		DisplayMoreInfo();
		break;
	case ID_UNDO:
		Undo();
		break;
	case ID_CUT:
		Cut();
		break;
	case ID_COPY:
		Copy();
		break;
	case ID_PASTE:
		Paste();
		break;
	case ID_DELETE:
		Clear();
		break;
	case ID_SEL_ALL:
		SelectAll();
		break;
	}	
	return CEdit::OnCommand(wParam, lParam);
}


BOOL CVisValidEdit::SetBrushColour(VVEbrush brWhich, COLORREF NewColour)
{
	CBrush brush ;
//	brush.CreateSolidBrush(NewColour);
	switch(brWhich) 
	{
	case BR_ERROR:
	case BR_EMPTY:
		m_brEmpty.DeleteObject();
		m_brEmpty.CreateSolidBrush(NewColour);
		break;
	case BR_VALID:
		m_brValid.DeleteObject();
		m_brValid.CreateSolidBrush(NewColour);
		break;
	case BR_INVALIDSYNTAX:
		m_brInvalid.DeleteObject();
		m_brInvalid.CreateSolidBrush(NewColour);
		break;
	case BR_INCOMPLETE:
	case BR_RANGEERR:
	case BR_LONG:
	case BR_SHORT:
		m_brIncomplete.DeleteObject();
		m_brIncomplete.CreateSolidBrush(NewColour);
		break;
	case BR_CUSTOM_ERR1:
		m_brCustom1.DeleteObject();
		m_brCustom1.CreateSolidBrush(NewColour);
		break;
	case BR_CUSTOM_ERR2:
		m_brCustom2.DeleteObject();
		m_brCustom2.CreateSolidBrush(NewColour);
		break;
	}
	return FALSE;
	
}

COLORREF CVisValidEdit::GetBrushColour(VVEbrush brWhich)
{
	LOGBRUSH  Logbr;
	switch(brWhich) 
	{
	case BR_ERROR:
	case BR_EMPTY:
		if( m_brEmpty.GetLogBrush(&Logbr))
			return Logbr.lbColor;
		else
			return RGBDEF_EMPTY;
	case BR_VALID:
		if( m_brValid.GetLogBrush(&Logbr))
			return Logbr.lbColor;
		else
			return RGBDEF_VALID;
	case BR_INVALIDSYNTAX:
		if( m_brInvalid.GetLogBrush(&Logbr))
			return Logbr.lbColor;
		else
			return RGBDEF_INVALID;
	case BR_INCOMPLETE:
	case BR_LONG:
	case BR_SHORT:
	case BR_RANGEERR:
		if( m_brIncomplete.GetLogBrush(&Logbr))
			return Logbr.lbColor;
		else
			return RGBDEF_INCOMPLETE;
	case BR_CUSTOM_ERR1:
		if( m_brCustom1.GetLogBrush(&Logbr))
			return Logbr.lbColor;
		else
			return RGBDEF_CUST1;
	case BR_CUSTOM_ERR2:
		if( m_brCustom1.GetLogBrush(&Logbr))
			return Logbr.lbColor;
		else
			return RGBDEF_CUST2;
	}
	return ::GetSysColor(COLOR_WINDOW);
}



CBrush * CVisValidEdit::CustomValidation()
{
//	CBrush * brush = &m_brEmpty;
	CString s;
	CEdit::GetWindowText(s);
	DWORD dwResult = BR_ERROR;
	CWnd * pParent = GetParent();
	NMHDR nm;
	nm.idFrom = GetDlgCtrlID();
	nm.hwndFrom = m_hWnd;
	nm.code = m_iCurrentErr;
	if(::SendMessageTimeout(pParent->GetSafeHwnd(),UWM_VVE_DO_CUSTOMVALIDATION,
				(WPARAM)&nm,
				(LPARAM)(LPCTSTR)s,SMTO_BLOCK,1,&dwResult) == 0) //if == 0 then timeout or failure occured
	{
		m_iCurrentErr = BR_ERROR;
		m_bCustomValidationCallOK = FALSE;
		return &m_brEmpty;
	}
	m_bCustomValidationCallOK = TRUE;
	m_iCurrentErr = (VVEbrush)dwResult;
	switch(m_iCurrentErr)
	{
	case BR_ERROR:
		//SendMessageTimeout can still succeed if WM message is not processed. in this case
		//the return value dwResult will be 0 (BR_ERROR). 
		m_bCustomValidationCallOK = FALSE;
		m_iCurrentErr = BR_ERROR;
		return &m_brEmpty;
	case BR_EMPTY:
		return &m_brEmpty;
	case BR_VALID:
		return &m_brValid;
	case BR_INCOMPLETE:
	case BR_SHORT:
	case BR_LONG:
	case BR_RANGEERR:
		return &m_brIncomplete;//long/short/large/small/incomplete are same brush - only context messages change
	case BR_INVALIDSYNTAX:
		return &m_brInvalid;
	case BR_CUSTOM_ERR1:
		return &m_brCustom1;
	case BR_CUSTOM_ERR2:
		return &m_brCustom2;
	default:
		m_iCurrentErr = BR_ERROR;
		return &m_brEmpty;
	}


//	return brush;
}

//CheckFloat routine compliments of joseph - contact info at top of this file
CBrush * CVisValidEdit::CheckFloat()
{
	CBrush * brush = &m_brEmpty;
	enum {S0, IPART, FPART, ESIGN, EPART};
	WORD state = S0;
	CString s;
	CEdit::GetWindowText(s);
	int iCheckLen = 0;
	BOOL bNewValidOK;
	BOOL bNewValidLengthOK;
	BOOL bNewValidSyntaxOK;

	if(m_bCustomValidation)
	{
		brush = CustomValidation();
	}
	else
	{
		for(int i = 0; brush != &m_brInvalid && i < s.GetLength();)
		{ // scan string /
			TCHAR ch = s[i];
			switch(MAKELONG(state, ch))
			{ 
			case MAKELONG(S0, _T(' ')):
			case MAKELONG(S0, _T('\t')):
				i++;
				continue;
			case MAKELONG(S0, _T('+')):
			case MAKELONG(S0, _T('-')):
				i++;
				m_iCurrentErr = BR_INCOMPLETE;
				brush = &m_brIncomplete;
				state = IPART;
				continue;
			case MAKELONG(S0, _T('0')):
			case MAKELONG(S0, _T('1')):
			case MAKELONG(S0, _T('2')):
			case MAKELONG(S0, _T('3')):
			case MAKELONG(S0, _T('4')):
			case MAKELONG(S0, _T('5')):
			case MAKELONG(S0, _T('6')):
			case MAKELONG(S0, _T('7')):
			case MAKELONG(S0, _T('8')):
			case MAKELONG(S0, _T('9')):
				state = IPART;
				continue;
			case MAKELONG(S0, _T('.')):
				i++;
				state = FPART;
				m_iCurrentErr = BR_INCOMPLETE;
				brush = &m_brIncomplete;
				continue;
			case MAKELONG(S0, _T('E')):
			case MAKELONG(S0, _T('e')):
				i++;
				state = ESIGN;
				m_iCurrentErr = BR_INCOMPLETE;
				brush = &m_brIncomplete;
				continue;
			case MAKELONG(IPART, _T('0')):
			case MAKELONG(IPART, _T('1')):
			case MAKELONG(IPART, _T('2')):
			case MAKELONG(IPART, _T('3')):
			case MAKELONG(IPART, _T('4')):
			case MAKELONG(IPART, _T('5')):
			case MAKELONG(IPART, _T('6')):
			case MAKELONG(IPART, _T('7')):
			case MAKELONG(IPART, _T('8')):
			case MAKELONG(IPART, _T('9')):
				i++;
				m_iCurrentErr = BR_VALID;
				brush = &m_brValid;
				continue;
			case MAKELONG(IPART, _T('.')):
				i++;
				brush = &m_brValid;
				state = FPART;
				continue;
			case MAKELONG(IPART, _T('e')):
			case MAKELONG(IPART, _T('E')):
				i++;
				m_iCurrentErr = BR_INCOMPLETE;
				brush = &m_brIncomplete;
				state = ESIGN;
				continue;
			case MAKELONG(FPART, _T('0')):
			case MAKELONG(FPART, _T('1')):
			case MAKELONG(FPART, _T('2')):
			case MAKELONG(FPART, _T('3')):
			case MAKELONG(FPART, _T('4')):
			case MAKELONG(FPART, _T('5')):
			case MAKELONG(FPART, _T('6')):
			case MAKELONG(FPART, _T('7')):
			case MAKELONG(FPART, _T('8')):
			case MAKELONG(FPART, _T('9')):
				i++;
				m_iCurrentErr = BR_VALID;
				brush = &m_brValid;
				continue;
			case MAKELONG(FPART, _T('e')):
			case MAKELONG(FPART, _T('E')):
				i++;
				m_iCurrentErr = BR_INCOMPLETE;
				brush = &m_brIncomplete;
				state = ESIGN;
				continue;
			case MAKELONG(ESIGN, _T('+')):
			case MAKELONG(ESIGN, _T('-')):
				i++;
				m_iCurrentErr = BR_INCOMPLETE;
				brush = &m_brIncomplete;
				state = EPART;
				continue;
			case MAKELONG(ESIGN, _T('0')):
			case MAKELONG(ESIGN, _T('1')):
			case MAKELONG(ESIGN, _T('2')):
			case MAKELONG(ESIGN, _T('3')):
			case MAKELONG(ESIGN, _T('4')):
			case MAKELONG(ESIGN, _T('5')):
			case MAKELONG(ESIGN, _T('6')):
			case MAKELONG(ESIGN, _T('7')):
			case MAKELONG(ESIGN, _T('8')):
			case MAKELONG(ESIGN, _T('9')):
				state = EPART;
				continue;
			case MAKELONG(EPART, _T('0')):
			case MAKELONG(EPART, _T('1')):
			case MAKELONG(EPART, _T('2')):
			case MAKELONG(EPART, _T('3')):
			case MAKELONG(EPART, _T('4')):
			case MAKELONG(EPART, _T('5')):
			case MAKELONG(EPART, _T('6')):
			case MAKELONG(EPART, _T('7')):
			case MAKELONG(EPART, _T('8')):
			case MAKELONG(EPART, _T('9')):
				i++;
				m_iCurrentErr = BR_VALID;
				brush = &m_brValid;
				continue;
			default:
				m_iCurrentErr = BR_INVALIDSYNTAX;
				brush = &m_brInvalid;
				break;
			} // switch
		} // scan string /
	}
	
	//check contents/////////////////////////////////////////////////////

	//check range
	if(brush == &m_brValid && m_bEnableRangeCheck)//is input valid?
	{
		if (CheckRange(s) != VVE_OK)
		{
			m_iCurrentErr = BR_RANGEERR;
			brush = &m_brIncomplete;
		}
	}
	
	iCheckLen = CheckLen(s);
	bNewValidSyntaxOK = brush == &m_brValid;
	bNewValidLengthOK = iCheckLen == 0;
	//if user wants to allow empty input, then override the checks
	if(m_bAllowEmpty && s.GetLength() == 0)
	{
		bNewValidLengthOK = TRUE;
		bNewValidSyntaxOK = TRUE;
	}
	//set the brush according to check results
	//note though, brush may have been left on m_brIncomplete from tests above
	if(bNewValidSyntaxOK)
	{
		if(iCheckLen == VVE_OK )//all fine
		{
			m_iCurrentErr = BR_VALID;
			brush = &m_brValid;
		}
		else //length ng
		{
			m_iCurrentErr = iCheckLen == VVE_LEN_LONG ? BR_LONG : BR_SHORT;
			brush = &m_brIncomplete;
		}
	}
	//now (almost final check) - as you can correct any syntax but still
	//be too long / short, length checking has greater priority
	if(iCheckLen != VVE_OK && m_bDoLengthCheck)
	{
		m_iCurrentErr = iCheckLen == VVE_LEN_LONG ? BR_LONG : BR_SHORT;
		brush = &m_brIncomplete;
	}
	if( (m_iMinInputLen == 0 || m_bAllowEmpty) && s.GetLength() == 0)
	{
		m_iCurrentErr = BR_EMPTY;
		brush = &m_brEmpty;//only set empty brush if its allowed to go empty
	}
	//log state
	m_bValidSyntax = bNewValidSyntaxOK;
	m_bValidLength = bNewValidLengthOK;
	bNewValidOK = (m_bValidLength && m_bValidSyntax);
	//determine whether or not to post Validation changed msg
	if(m_bValid != bNewValidOK)//has validation state changed
	{
		//log valid state before posting change so that calls to is valid succeed
		m_bValid = bNewValidOK;
		ValidationChanged(bNewValidOK);
	}
	else
		m_bValid = bNewValidOK;//log valid state
	return brush;
	
} 

CBrush * CVisValidEdit::Validate()
{
	if(m_setCurrent == SET_FLOAT && !m_bCustomValidation)
		return CheckFloat();

	CBrush * brush = &m_brValid;
//	BOOL change = FALSE;
	CString s;
	CEdit::GetWindowText(s);
	int iTextLength = s.GetLength();
//	BOOL bChanged = FALSE;
	BOOL bNewValidOK = FALSE;
	BOOL bNewValidSyntaxOK = FALSE;
	BOOL bNewValidLengthOK = FALSE;
	int  iCheckLen;
//check contents
	if(m_bCustomValidation && ! m_bNoValidation)
	{
		brush = CustomValidation();
	}
	else
	{
		if(m_bAllCharValid == FALSE )
		{
			for(int i = 0; i < iTextLength && brush != &m_brInvalid ; i ++)
			{
				if(m_bValidChar[(UCHAR)s.GetAt(i)] == FALSE)
				{
					m_iCurrentErr = BR_INVALIDSYNTAX;
					brush = &m_brInvalid;
				}
			}
			//additional decimal check
			if(m_setCurrent == SET_SDECIMAL)
			{
				CString stemp(s);
				stemp.TrimLeft();
				stemp.TrimRight();
				if(stemp.GetLength())
				{	
					BOOL bSigned = stemp.FindOneOf(_T("+-")) == 0;
					stemp = stemp.Right(stemp.GetLength()-1);
					if( stemp.FindOneOf(_T("+- ")) >= 0)
					{	
						m_iCurrentErr = BR_INVALIDSYNTAX;
						brush = &m_brInvalid;
					}
					if(bSigned && stemp.GetLength() < 1)
					{	
						m_iCurrentErr = BR_INCOMPLETE;
						brush = &m_brIncomplete;
					}
				}
			}
		}
	}
	//bCustErrorValid ...
	//used to determine if the custom error returned from a 
	//custom validation routine is considered valid or not
	BOOL bCustErrorValid = FALSE;

	if(m_bEnableRangeCheck && brush == &m_brValid)//should we check range?
	{	
		if(CheckRange(s) != 0)// is range ok?
		{
			//it is not!
			m_iCurrentErr = BR_RANGEERR;
			brush = &m_brIncomplete;
		}
	}

	//check contents
	iCheckLen = CheckLen(s);
	if(m_bCustomValidation && m_bCustomValidationCallOK)
	{
		//this is a custom validation returned brush so lets see if 
		//user has declared this errorvalue as a validvalue
		if((brush == &m_brCustom1 && m_bCustomErrorIsValid[1])
			|| (brush == & m_brCustom2 && m_bCustomErrorIsValid[2]))
		{
			bNewValidSyntaxOK = TRUE;//ok 
			bCustErrorValid = TRUE;//prevent any futher changes to brush for the rest of this function
		}
		else
			bNewValidSyntaxOK = brush == &m_brValid;//ok 
	}
	else
	{
		//this is a built in validation - set the the ValidOK var accordingly
		bNewValidSyntaxOK = brush == &m_brValid;//ok 
	}
	bNewValidLengthOK = (iCheckLen == VVE_OK);

	//if user wants to allow empty input, then override the checks
	if(m_bAllowEmpty && s.GetLength() == 0)
	{
		bNewValidLengthOK = TRUE;
		bNewValidSyntaxOK = TRUE;
	}
	//default to OK syntax if a custom validation call failed (necessary as brush will == EMPTY)
	if(m_bCustomValidation == TRUE && m_bCustomValidationCallOK == FALSE)
		bNewValidSyntaxOK = TRUE;

	//set the brush according to check results
	if(bNewValidSyntaxOK != FALSE && !bCustErrorValid) //is this valid syntax 
	{
		if(iCheckLen == VVE_OK )//yes, all fine
		{
			m_iCurrentErr = BR_VALID;
			brush = &m_brValid;
		}
		else //length ng
		{
			m_iCurrentErr = iCheckLen == VVE_LEN_LONG ? BR_LONG : BR_SHORT;
			brush = &m_brIncomplete;
		}
	}
	//now (almost final check) - as you can correct any syntax but still
	//be too long / short, length checking has greater priority
	if(iCheckLen != VVE_OK && m_bDoLengthCheck)
	{
		m_iCurrentErr = iCheckLen == VVE_LEN_LONG ? BR_LONG : BR_SHORT;
		brush = &m_brIncomplete;
	}
	if((m_iMinInputLen == 0 || m_bAllowEmpty) && s.GetLength() == 0)//nothing in text box ?
	{
		m_iCurrentErr = BR_EMPTY;
		brush =  &m_brEmpty ;
	}
	if(!m_bAllowEmpty && s.GetLength() == 0)//nothing in text box ?
		bNewValidLengthOK =  FALSE;

	//log states
	m_bValidSyntax = bNewValidSyntaxOK;
	m_bValidLength = bNewValidLengthOK;
	bNewValidOK = (m_bValidLength && m_bValidSyntax);
	//determine whether or not to post Validation changed msg
	if(m_bValid != bNewValidOK)//has validation state changed
	{
		//log valid state before posting change so that calls to is valid succeed
		m_bValid = bNewValidOK;
		ValidationChanged(bNewValidOK);
	}
	else
		m_bValid = bNewValidOK;//log valid state
	return brush;
}

void CVisValidEdit::DoValidation(BOOL bRedraw )//explicit request for validation & or update
{

	if(m_hWnd && bRedraw)
	{	
		//cause ctlColor to be called (which will call Validate())
		InvalidateRect(NULL);
		UpdateWindow();
	}
	else
		Validate();
}

//returns VVE_LEN_SHORT, VVE_LEN_LONG or VVE_OK
int CVisValidEdit::CheckLen()
{
	if(m_bDoLengthCheck == FALSE || m_bNoValidation == TRUE)
		return VVE_OK;
	CString s;
	CEdit::GetWindowText(s);
	int iLen = s.GetLength();
	if(iLen < m_iMinInputLen)
			return VVE_LEN_SHORT;
	if(iLen > m_iMaxInputLen)
			return VVE_LEN_LONG;
	return VVE_OK;
}

//returns VVE_LEN_SHORT, VVE_LEN_LONG or VVE_OK
int CVisValidEdit::CheckLen( CString& str)
{
	if(m_bDoLengthCheck == FALSE || m_bNoValidation == TRUE)
		return VVE_OK;
	int iLen = str.GetLength();
	if(iLen < m_iMinInputLen)
			return VVE_LEN_SHORT;
	if(iLen > m_iMaxInputLen)
			return VVE_LEN_LONG;
	return VVE_OK;
}


HBRUSH CVisValidEdit::CtlColor(CDC * dc, UINT id)
{
	if(m_bNoValidation || IsWindowEnabled() == FALSE)
		return NULL;
	CBrush * brush = Validate();
	LOGBRUSH br;
	brush->GetLogBrush(&br);
	dc->SetBkColor(br.lbColor);
	UNREFERENCED_PARAMETER(id);
	return (HBRUSH)*brush;
} 



/****************************************************************************
*                           CVisValidEdit::OnChange
*
*   1. Invalidates the entire control thru a call to DoValidation so colors 
*   come out right. Otherwise the "optimizations" of the redraw will leave 
*	the colors banded in odd ways and only update the area around the text, 
*	not the entire	box. 
*	2. Post notification that a change occured lParam being the new state
****************************************************************************/

BOOL CVisValidEdit::OnChange()
{
    DoValidation();//to Validate content & redraw window
	CWnd *pParent = GetParent();
	ASSERT(pParent);
	m_nm.idFrom = GetDlgCtrlID();
	m_nm.hwndFrom = m_hWnd;
	m_nm.code = (UINT) m_iCurrentErr;
	::PostMessage(pParent->GetSafeHwnd(),UWM_VVE_CONTENT_CHANGED,(WPARAM)&m_nm,(LPARAM)m_bValid);//post msg & state
    return FALSE;
} 

/***************************************************************************
*       Allow / disallow character entry
*		user can set warious options which effect the entry of chars
*		see SetAllCharEntry()
****************************************************************************/

void CVisValidEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	
	//if this is a character to process
	if( ! m_bNoValidation  &&  ! m_bAllCharValid && ! m_bAllowAllCharEntry )
		if (   ! (    (nChar < VVE_MIN_CHAR) || (nChar > VVE_MAX_CHAR ))   )
		{
			if(m_bValidChar[nChar] == FALSE) //is this a valid char?
			{
				//its not
				MessageBeep(0);
				return;
			}
		} 
    CEdit::OnChar(nChar, nRepCnt, nFlags);
}

void CVisValidEdit::SetTextLenMinMax(int iMin, int iMax)
{
	m_iMaxInputLen = iMax;
	m_iMinInputLen = iMin;
	if(iMax > VVE_DEF_MAXLEN || iMax < VVE_DEF_MINLEN)
		m_iMaxInputLen = VVE_DEF_MAXLEN;
	if( iMin < VVE_DEF_MINLEN || iMin > VVE_DEF_MAXLEN)
		m_iMinInputLen = VVE_DEF_MINLEN;
	if(m_iMinInputLen > m_iMaxInputLen)
	{	
		int temp = m_iMaxInputLen;
		m_iMaxInputLen = m_iMinInputLen;
		m_iMinInputLen = temp;
	}
	SetLimitText(m_iMaxInputLen);
}
void CVisValidEdit::SetValidChars(LPCTSTR cstrCustomChar)
{
	DWORD dwFillLen = sizeof(BOOL) * UCHAR_MAX;
	FillMemory(m_bValidChar,dwFillLen,FALSE);
	m_bAllCharValid = FALSE;
	for (UINT i = 0; i < _tcslen(cstrCustomChar); i++) 
		m_bValidChar[(UCHAR)cstrCustomChar[i]] = TRUE;
}

void CVisValidEdit::SetValidCharSet(VVEset vveset, LPCTSTR cstrCustomChar)
{
	//set all to FALSE
	DWORD dwFillLen = sizeof(BOOL) * UCHAR_MAX;
	FillMemory(m_bValidChar,dwFillLen,FALSE);
	m_bAllCharValid = FALSE;
	UINT i;
	CString strValid;
	switch(vveset) 
	{
	case SET_CUSTOM: //user choice 
		if(cstrCustomChar == NULL)
			return ;
		for (i = 0; i < _tcslen(cstrCustomChar); i++) 
			m_bValidChar[(TCHAR)cstrCustomChar[i]] = TRUE;
		SetMoreInfoString(vveset);
		m_bEnableRangeCheck = FALSE;
		break;
	case SET_DIRPATH:
		for ( i = 0; i < _tcslen(VVE_DEFSETDIRPATH); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETDIRPATH[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETDIRPATH[i]] = TRUE;
		}
		m_bEnableRangeCheck = FALSE;
		break;
	case SET_ALPHANUM:
		for ( i = 0; i < _tcslen(VVE_DEFSETALPHANUM); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETALPHANUM[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETALPHANUM[i]] = TRUE;
		}
		m_bEnableRangeCheck = FALSE;
		break;
	case SET_ALPHA:
		for ( i = 0; i < _tcslen(VVE_DEFSETALPHA); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETALPHA[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETALPHA[i]] = TRUE;
		}
		m_bEnableRangeCheck = FALSE;
		break;
	case SET_DECIMAL:
		for ( i = 0; i < _tcslen(VVE_DEFSETDECIMAL); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETDECIMAL[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETDECIMAL[i]] = TRUE;
		}
		m_bEnableRangeCheck = TRUE;
		break;	
	case SET_SDECIMAL:
		for ( i = 0; i < _tcslen(VVE_DEFSETSDECIMAL); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETSDECIMAL[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETSDECIMAL[i]] = TRUE;
		}
		m_bEnableRangeCheck = TRUE;
		break;
	case SET_OCTAL:
		for ( i = 0; i < _tcslen(VVE_DEFSETOCTAL); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETOCTAL[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETOCTAL[i]] = TRUE;
		}
		m_bEnableRangeCheck = TRUE;
		break;
	case SET_HEXADECIMAL:
		for ( i = 0; i < _tcslen(VVE_DEFSETHEXADECIMAL); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETHEXADECIMAL[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETHEXADECIMAL[i]] = TRUE;
		}
		m_bEnableRangeCheck = TRUE;
		break;
	case SET_BINARY:
		for ( i = 0; i < _tcslen(VVE_DEFSETBINARY); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETBINARY[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETBINARY[i]] = TRUE;
		}
		m_bEnableRangeCheck = TRUE;
		break;
	case SET_FLOAT:
		for ( i = 0; i < _tcslen(VVE_DEFSETFLOAT); i++) 
		{
			ASSERT((UCHAR)VVE_DEFSETFLOAT[i] < UCHAR_MAX);
			m_bValidChar[(UCHAR)VVE_DEFSETFLOAT[i]] = TRUE;
		}
		m_bEnableRangeCheck = TRUE;
		break;
	case SET_ALLCHAR:
	default:
		m_bEnableRangeCheck = FALSE;
		m_bAllCharValid = TRUE;
	}
	//store the selected set
	m_setCurrent = vveset;

	//update the info string
	if(m_bAllCharValid == TRUE)
		SetMoreInfoString(vveset,VVE_DEFINFOALL);
	else
		SetMoreInfoString(vveset);//set defaults

}

void CVisValidEdit::GetValidChars(CString & strValidChars)
{
	int j = 0;
	// 1st check to see if all characters are valid
	if(m_bAllCharValid || m_bNoValidation)
	{
		strValidChars = VVE_DEFSETALL; 
		return;
	}

	//if here then 
	//reset string sent to all spaces
	strValidChars = CString(' ', VVE_MAX_CHAR - VVE_MIN_CHAR +1);
	//fill it with currnet allowable chars
	for (int i = VVE_MIN_CHAR; i < VVE_MAX_CHAR ; i++) 
	{
		if(m_bValidChar[i] != FALSE)
			strValidChars.SetAt(j++, char(i)); 
	}
	//trim str
	strValidChars.TrimRight();
}

void CVisValidEdit::ValidationChanged(BOOL bNewState)
{
	CWnd *pParent = GetParent();
	ASSERT(pParent);
	DWORD dwResult;
	NMHDR nm;
	nm.idFrom = GetDlgCtrlID();
	nm.hwndFrom = m_hWnd;
	nm.code = (UINT) m_iCurrentErr;
	//inform parent of validation state change
	::SendMessageTimeout(pParent->GetSafeHwnd(),UWM_VVE_VALIDITY_CHANGED,
				(WPARAM)&nm,
				(LPARAM) bNewState ,SMTO_BLOCK,1,&dwResult) ;

}



/*
 GetWindowValue(double &dVal)
 returns 

 For	SET_FLOAT:
 FALSE - if string is too large for double 
 
 For	SET_HEXADECIMAL:
		SET_OCTAL:
		SET_BINARY:
		SET_DECIMAL:
 FALSE - if string is too large for ulong 
 
 For	SET_SDECIMAL
		SET_xxx other than above (defaults to SDECIMAL - ULONG)
		default:
 FALSE - if string is too small or large for long

 TRUE - if none of the above
*/
BOOL CVisValidEdit::GetWindowValue(double &dVal)
{
	CString s, strVal;
	GetWindowText(s);
	int iRangeCheck = VVE_OK;
	BOOL bRet;
	TCHAR *pEnd;
	switch(m_setCurrent) 
	{
	//asking for double if you call this function (bum bum) aaaaaany how 
	//i handle it by calling the long implimentation - since 
	//the user is requesting a double, a long/ulong wrapped 
	//in a double will have to do (for now)!
	case SET_HEXADECIMAL:
	case SET_OCTAL:
	case SET_BINARY:
	case SET_DECIMAL:
		{
			iRangeCheck = CheckRange(m_setCurrent,s);
			unsigned long ul;
			bRet = GetWindowValue(ul);
			dVal = (double) ul;
			return bRet;
		}
		break;
	case SET_FLOAT:
		dVal = _tcstod(s,&pEnd);
		iRangeCheck = CheckRange(SET_FLOAT,s);
		break;
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_CUSTOM: 
	case SET_DIRPATH:
	case SET_SDECIMAL:
	default: 
		{
			iRangeCheck = CheckRange(SET_SDECIMAL,s);
			long l;
			bRet = GetWindowValue(l);
			dVal = (double) l;
			return bRet;
		}
		break;
	}
	return (iRangeCheck == VVE_OK || m_bEnableRangeCheck == FALSE ) 
		&& (m_bValidSyntax || m_bNoValidation == TRUE);
}

//returns FALSE if convertion is unverified
//(this may happen if control contains invalid characters 
//or the value is too large for container)
BOOL CVisValidEdit::GetWindowValue(long &lVal)
{
	CString s, strVal;
	GetWindowText(s);
	int iRangeCheck = VVE_OK;
	TCHAR *pEnd = NULL;
	switch(m_setCurrent) 
	{
	case SET_DECIMAL:
		iRangeCheck = CheckRange(s);
		lVal = (long)_tcstoul(s,&pEnd,10); 
		break;
	case SET_OCTAL:
		iRangeCheck = CheckRange(s);
		lVal = (long)_tcstoul(s,&pEnd,8);
		break;
	case SET_HEXADECIMAL:
		iRangeCheck = CheckRange(s);
		lVal = (long)_tcstoul(s,&pEnd,16);
		break;
	case SET_BINARY:
		iRangeCheck = CheckRange(s);
		lVal = (long)_tcstoul(s,&pEnd,2);
		break;
	case SET_FLOAT:
		{
			//the check for float is done in 
			//GetWindowValue(double)
			double d;
			BOOL bRet = GetWindowValue(d);
			lVal = (long) d;
			if(d > LONG_MAX || d < LONG_MIN)
				return FALSE;
			return bRet;
		}
	case SET_SDECIMAL:
	case SET_DIRPATH:
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_CUSTOM: 
		//as we are rerieving a value, check as a ULONG.
		//this way, a negitive value can be got from a
		//number > LONG_MAX but less than ULONG_MAX
		iRangeCheck = CheckRange(SET_DECIMAL,s);
		lVal = (long)_tcstoul(s,&pEnd,10);
		break;
	}
	return (iRangeCheck == VVE_OK || m_bEnableRangeCheck == FALSE ) 
		&& (m_bValidSyntax || m_bNoValidation == TRUE);
}

//returns FALSE if convertion is unverified
//(this may happen if control contains invalid characters 
//or the value is too large for container)
BOOL CVisValidEdit::GetWindowValue(unsigned long &lVal)
{
	CString s, strVal;
	GetWindowText(s);
	int iRangeCheck = VVE_NO_LARGE;
	TCHAR *pEnd = NULL;
	switch(m_setCurrent) 
	{
	case SET_DECIMAL:
		iRangeCheck = CheckRange(SET_DECIMAL,s);
		lVal = _tcstoul(s,&pEnd,10); 
		break;
	case SET_OCTAL:
		iRangeCheck = CheckRange(SET_OCTAL,s);
		lVal = _tcstoul(s,&pEnd,8);
		break;
	case SET_HEXADECIMAL:
		iRangeCheck = CheckRange(SET_HEXADECIMAL,s);
		lVal = _tcstoul(s,&pEnd,16);
		break;
	case SET_BINARY:
		iRangeCheck = CheckRange(SET_BINARY,s);
		lVal = _tcstoul(s,&pEnd,2);
		break;
	case SET_FLOAT:
		{
			double d;
			BOOL bRet = GetWindowValue(d);
			lVal = (unsigned long) d;
			if(d > ULONG_MAX)
				return FALSE;
			return bRet;
		}
	case SET_SDECIMAL:
	case SET_DIRPATH:
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_CUSTOM: 
		iRangeCheck = CheckRange(SET_DECIMAL,s);//force ULONG checking
		lVal = _tcstoul(s,&pEnd,10);
		break;
	}
	return (iRangeCheck == VVE_OK || m_bEnableRangeCheck == FALSE ) 
		&& (m_bValidSyntax || m_bNoValidation == TRUE);
}

//returns FALSE if convertion is unverified
//(this may happen if control contains invalid characters 
//or the value is too large for container)
BOOL CVisValidEdit::GetWindowValue(unsigned short &usVal)
{
	CString s, strVal;
	GetWindowText(s);
	int iRangeCheck = VVE_NO_LARGE;
	unsigned long lVal;
	TCHAR *pEnd = NULL;
	switch(m_setCurrent) 
	{
	case SET_DECIMAL:
		iRangeCheck = CheckRange(SET_DECIMAL,s);
		lVal = _tcstoul(s,&pEnd,10); 
		break;
	case SET_OCTAL:
		iRangeCheck = CheckRange(SET_OCTAL,s);
		lVal = _tcstoul(s,&pEnd,8);
		break;
	case SET_HEXADECIMAL:
		iRangeCheck = CheckRange(SET_HEXADECIMAL,s);
		lVal = _tcstoul(s,&pEnd,16);
		break;
	case SET_BINARY:
		iRangeCheck = CheckRange(SET_BINARY,s);
		lVal = _tcstoul(s,&pEnd,2);
		break;
	case SET_FLOAT:
		{
			double d;
			BOOL bRet = GetWindowValue(d);
			lVal = (unsigned long) d;
			usVal = (USHORT)(lVal & 0x0000FFFF);
			if(d > USHRT_MAX)
				return FALSE;
			return bRet;
		}
	case SET_SDECIMAL:
	case SET_DIRPATH:
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_CUSTOM: 
		iRangeCheck = CheckRange(SET_DECIMAL,s);//force ULONG checking
		lVal = _tcstoul(s,&pEnd,10);
		break;
	}

	usVal = (USHORT)(lVal & 0x0000FFFF);

	return (iRangeCheck == VVE_OK || m_bEnableRangeCheck == FALSE ) 
		&& (m_bValidSyntax || m_bNoValidation == TRUE);
}




//returns TRUE if the entered value is validated (according to settings & set selected)
//eg1, if mode is SET_ALPHA & you set any value, validation will fail
//eg2, if mode is SET_BINARY & MAX_LEN == 5, SetWindowValue(12345) would insert 
//the test 11000000111001 into the edit but the return would fail 
//eg3, if mode is SET_CUSTOM, SetWindowValue would default to decimal input & validation
//would depend on your character set 
//retuns false otherwise
BOOL CVisValidEdit::SetWindowValue(unsigned long lSetVal)
{
	TCHAR szTemp[LONG_STRMAX] = _T("");//long enough for representation of a long as a string 
	switch(m_setCurrent) 
	{
	case SET_DECIMAL:
		_ultot_s(lSetVal,szTemp, LONG_STRMAX, 10);
		SetWindowText(szTemp);
		break;
	case SET_SDECIMAL:
		_ltot_s(lSetVal,szTemp,LONG_STRMAX, 10);
		SetWindowText(szTemp);
		break;
	case SET_OCTAL:
		_ultot_s(lSetVal,szTemp, LONG_STRMAX, 8);
		SetWindowText(szTemp);
		break;
	case SET_HEXADECIMAL:
		_ultot_s(lSetVal,szTemp, LONG_STRMAX, 16);
		SetWindowText(szTemp);
		break;
	case SET_BINARY:
		_ultot_s(lSetVal,szTemp, LONG_STRMAX, 2);
		SetWindowText(szTemp);
		break;
	case SET_FLOAT:
		return SetWindowValue((double)lSetVal);
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_CUSTOM: 
	case SET_DIRPATH:
		_ultot_s(lSetVal,szTemp, LONG_STRMAX, 10);
		SetWindowText(szTemp);
	}
	//SetWindowText(strVal);
	//causes a validation thru OnChange
	//so m_bValidSyntax will be correct
	return m_bValidSyntax;
	
}

BOOL CVisValidEdit::SetWindowValue(unsigned short usSetVal)
{
	unsigned long ulSetVal = (unsigned long) usSetVal;
	TCHAR szTemp[USHORT_STRMAX] = _T("");//long enough for representation of a long as a string 
	switch(m_setCurrent) 
	{
	case SET_DECIMAL:
		_ultot_s(ulSetVal,szTemp, USHORT_STRMAX, 10);
		SetWindowText(szTemp);
		break;
	case SET_SDECIMAL:
		_ultot_s(ulSetVal,szTemp,USHORT_STRMAX, 10);
		SetWindowText(szTemp);
		break;
	case SET_OCTAL:
		_ultot_s(ulSetVal,szTemp, USHORT_STRMAX, 8);
		SetWindowText(szTemp);
		break;
	case SET_HEXADECIMAL:
		_ultot_s(ulSetVal,szTemp, USHORT_STRMAX, 16);
		SetWindowText(szTemp);
		break;
	case SET_BINARY:
		_ultot_s(ulSetVal,szTemp, USHORT_STRMAX, 2);
		SetWindowText(szTemp);
		break;
	case SET_FLOAT:
		return SetWindowValue((double)ulSetVal);
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_CUSTOM: 
	case SET_DIRPATH:
		_ultot_s(ulSetVal,szTemp, USHORT_STRMAX, 10);
		SetWindowText(szTemp);
	}
	//SetWindowText(strVal);
	//causes a validation thru OnChange
	//so m_bValidSyntax will be correct
	return m_bValidSyntax;
	
}

BOOL CVisValidEdit::SetWindowValue(long lSetVal)
{
	TCHAR szTemp[LONG_STRMAX] = _T("");//long enough for representation of a long as a string 
	switch(m_setCurrent) 
	{
	case SET_DECIMAL:
		_ultot_s(lSetVal,szTemp,LONG_STRMAX,10);
		SetWindowText(szTemp);
		break;
	case SET_SDECIMAL:
		_ltot_s(lSetVal,szTemp,LONG_STRMAX,10);
		SetWindowText(szTemp);
		break;
	case SET_OCTAL:
		_ltot_s(lSetVal,szTemp,LONG_STRMAX,8);
		SetWindowText(szTemp);
		break;
	case SET_HEXADECIMAL:
		_ltot_s(lSetVal,szTemp,LONG_STRMAX,16);
		SetWindowText(szTemp);
		break;
	case SET_BINARY:
		_ltot_s(lSetVal,szTemp,LONG_STRMAX,2);
		SetWindowText(szTemp);
		break;
	case SET_FLOAT:
		return SetWindowValue((double)lSetVal);
		break;
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_CUSTOM: 
	case SET_DIRPATH:
		_ltot_s(lSetVal,szTemp,LONG_STRMAX,10);
		SetWindowText(szTemp);//causes a validation thru OnChange
	}
	//SetWindowText(strVal);
	//causes a validation thru OnChange
	//so m_bValidSyntax will be correct
	return m_bValidSyntax;
}




/************************************************************************
SetWindowValue(double lSetVal)
As this function is primaraly for use with SET_FLOAT, some conversions are
made when this is not the case

dSetVal is cast to a ULONG when setting an edit cotrol with one of
the following sets selected...
	SET_BINARY
	SET_HEXADECIMAL
	SET_OCTAL
	SET_DECIMAL
dSetVal is cast to a LONG when setting an edit cotrol with one of
the following sets selected...
	SET_ALPHA:
	SET_ALPHANUM:
	SET_ALLCHAR:
	SET_CUSTOM: 
	SET_SDECIMAL:
/************************************************************************/
BOOL CVisValidEdit::SetWindowValue(double dSetVal)
{
/*  future use - may consider converting 64 bit binary - currently,
	calls to Get/SetWindowValue(double) demote binary, hex, octal to 32bit
	USAGE would be - declare this HexStr

	static _TCHAR HexStr[][5] =	{
			_T("0000"), _T("0001"), _T("0010"), _T("0011"), //0,1,2,3
			_T("0100"), _T("0101"), _T("0110"), _T("0111"), //4,5,6,7
			_T("1000"), _T("1001"),                         //8,9
			_T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), //for chars between 9 ~ A
			_T("1010"), _T("1011"),                         //A,B
			_T("1100"), _T("1101"), _T("1110"), _T("1111")  //C,D,E,F
	};
	
	convert to hex string
	strVal.Format(_T("%X"),(long)lSetVal);
	
	CString temp = "";
	//whip thru hex chars & convert to 0001 0010 etc..
	for ( i = 0 ; i < strVal.GetLength() ; i++ ) 
		temp += HexStr[strVal[i] - '0'];
*/
	
	CString strVal = _T(""); 
	TCHAR szTemp[LONG_STRMAX] = _T("");//long enough for representation of a long as a string 
	BOOL bRangeOK = TRUE;
	switch(m_setCurrent) 
	{
	case SET_DECIMAL:
		strVal.Format(_T("%u"),(unsigned long)dSetVal);
/*		if(dSetVal < 0)
			bRangeOK = (LONG_MIN < (__int64)dSetVal && LONG_MAX > (__int64)dSetVal);
		else
			bRangeOK = (ULONG_MAX > (unsigned __int64)dSetVal);  */
		SetWindowText(strVal);
		bRangeOK = CheckRange(m_setCurrent,strVal) ==VVE_OK ;
		break;
	case SET_OCTAL:
		strVal.Format(_T("%o"),(unsigned long)dSetVal);
/*		if(dSetVal < 0)
			bRangeOK = (LONG_MIN < (__int64)dSetVal && LONG_MAX > (__int64)dSetVal);
		else
			bRangeOK = (ULONG_MAX > (unsigned __int64)dSetVal);  */
		SetWindowText(strVal);
		bRangeOK = CheckRange(m_setCurrent,strVal) ==VVE_OK ;
		break;
	case SET_HEXADECIMAL:
		strVal.Format(_T("%x"),(unsigned long)dSetVal);
/*		if(dSetVal < 0)
			bRangeOK = (LONG_MIN < (__int64)dSetVal && LONG_MAX > (__int64)dSetVal);
		else
			bRangeOK = (ULONG_MAX > (unsigned __int64)dSetVal);  */
		SetWindowText(strVal);
		bRangeOK = CheckRange(m_setCurrent,strVal) ==VVE_OK ;
		break;
	case SET_BINARY:
		_ultot_s((unsigned long)dSetVal,szTemp,LONG_STRMAX,2);
/*		if(dSetVal < 0)
			bRangeOK = (LONG_MIN < (__int64)dSetVal && LONG_MAX > (__int64)dSetVal);
		else
			bRangeOK = (ULONG_MAX > (unsigned __int64)dSetVal);  */
		SetWindowText(szTemp);
		bRangeOK = CheckRange(m_setCurrent,szTemp) ==VVE_OK ;
		break;
	case SET_FLOAT:
		strVal.Format(_T("%0.16G"), dSetVal);
		SetWindowText(strVal);
		bRangeOK = CheckRange(m_setCurrent,dSetVal) == VVE_OK;
		break;
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_CUSTOM: 
	case SET_DIRPATH:
	case SET_SDECIMAL:
	default:
		strVal.Format(_T("%d"),(long)dSetVal);
		SetWindowText(strVal);//causes Validate to be called thru OnChange
		bRangeOK = CheckRange(SET_SDECIMAL,dSetVal) ==VVE_OK ;
		break;
	}
	return m_bValidSyntax && bRangeOK;
}


void CVisValidEdit::OnContextMenu(CWnd* pWnd, CPoint pt) 
{
	UNREFERENCED_PARAMETER(pWnd);
	const MSG *m = GetCurrentMessage();
	if(m_bEnableErrorMessages == FALSE || m_bNoValidation == TRUE)
	{
		CEdit::DefWindowProc(m->message , m->wParam, m->lParam);
		return ;
	}
	SetFocus();
	CMenu cntxtMenu;
	CMenu * pm;
	pm = GetSystemMenu(TRUE);
	BOOL bSel = HIWORD(GetSel()) != LOWORD(GetSel());
	cntxtMenu.CreatePopupMenu();
	cntxtMenu.AppendMenu(MF_STRING | MF_ENABLED, ID_CONTEXT_INFO, m_arrErrStrs.GetAt(m_iCurrentErr) );
	cntxtMenu.AppendMenu(MF_SEPARATOR , MF_SEPARATOR   ,	_T("") );
//	cntxtMenu.AppendMenu(MF_SEPARATOR , MF_SEPARATOR   ,	_T("") );
	cntxtMenu.AppendMenu(MF_STRING | (CanUndo() ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)), ID_UNDO ,	_T("&Undo"));
	cntxtMenu.AppendMenu(MF_SEPARATOR , MF_SEPARATOR   ,	_T("") );
	cntxtMenu.AppendMenu(MF_STRING | (bSel ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)) , ID_CUT   , _T("Cu&t") );
	cntxtMenu.AppendMenu(MF_STRING | (bSel ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)), ID_COPY  , _T("&Copy"));
	cntxtMenu.AppendMenu(MF_STRING | MF_ENABLED, ID_PASTE , _T("&Paste"));
	cntxtMenu.AppendMenu(MF_STRING | (bSel ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)), ID_DELETE , _T("&Delete"));
	cntxtMenu.AppendMenu(MF_SEPARATOR , MF_SEPARATOR   ,	 _T("") );
	cntxtMenu.AppendMenu(MF_STRING | (LineLength() ? MF_ENABLED : (MF_GRAYED | MF_DISABLED)), ID_SEL_ALL, _T("&Select All"));
	cntxtMenu.TrackPopupMenu(TPM_LEFTALIGN , pt.x,
							  pt.y, this, NULL); 
	cntxtMenu.DestroyMenu();
		
}


#define BLANKALLOW   (m_bAllowEmpty == TRUE ? _T("is") : _T("is not"))
void CVisValidEdit::GetMoreInfoString(VVEset vveset, CString &sInfoString, BOOL bAddOnExtraInfo)
{
	if(m_bNoValidation == TRUE)
		sInfoString = VVE_DEFINFO_NOVALDATION;
	else
	{
		CString msg;
		if(bAddOnExtraInfo)
		{
			if(m_bDoLengthCheck == TRUE)

				sInfoString.Format(_T("%s\nZero length input %s allowed.\nMinimum length = %i, Maximum length = %i."), m_arrMoreStrs.GetAt(vveset), BLANKALLOW , m_iMinInputLen, m_iMaxInputLen);
			else
				sInfoString.Format(_T("%s\nZero length input %s allowed.\nText input is limited to %i %s."),m_arrMoreStrs.GetAt(vveset), BLANKALLOW, GetLimitText(), (GetLimitText() > 1 ? "characters" : "character") );
		}
		else
			sInfoString = m_arrMoreStrs.GetAt(vveset);	 
	}
}
#undef BLANKALLOW

void CVisValidEdit::SetMoreInfoString(VVEset vveset, LPCTSTR sNewInfoString)
{
	if(sNewInfoString == NULL)//NULL = reset string to default
	{
		switch(vveset) 
		{
		case SET_DECIMAL:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFODECIMAL);			
			break;
		case SET_SDECIMAL:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFOSDECIMAL);			
			break;
		case SET_OCTAL:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFOOCTAL);			
			break;
		case SET_HEXADECIMAL:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFOHEXADECIMAL);			
			break;
		case SET_BINARY:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFOBINARY);			
			break;
		case SET_FLOAT:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFOFLOAT);			
			break;
		case SET_ALPHA:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFOALPHA);
			break;
		case SET_ALPHANUM:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFOALPHANUM);			
			break;
		case SET_ALLCHAR:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFOALL);			
			break;
		case SET_DIRPATH:
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFODIRPATH);			
			break;
		case SET_CUSTOM: 
			m_arrMoreStrs.SetAt(vveset,VVE_DEFINFO_NOINFO);			
			break;
		default:
			return;
		}
	}
	else 
	{		
		m_arrMoreStrs.SetAt(vveset,sNewInfoString);
	}

}

void CVisValidEdit::DisplayMoreInfo(BOOL bAddOnExtraInfo)
{
	CString msg;
	GetMoreInfoString(m_setCurrent,msg,bAddOnExtraInfo);
	MessageBox(msg, 
		   _T("Information"),
			MB_ICONINFORMATION|MB_OK|MB_DEFBUTTON1);
}

//returns VVE_OK, VVE_NO_SMALL or VVE_NO_LARGE
int CVisValidEdit::CheckRange()
{
	CString s;
	GetWindowText(s);
	return CheckRange(GetValidSet(),s);
}

//returns VVE_OK, VVE_NO_SMALL or VVE_NO_LARGE
int CVisValidEdit::CheckRange(VVEset vveset, LPCTSTR lpszVal)
{

	CString s(lpszVal);
	switch(vveset) 
	{
	case SET_DECIMAL:
		{
			s.TrimLeft(_T("0-+ "));
			//1st, if the char string is > 10 (+1 for sign) characters, then it is too large for
			//a long
			if(s.SpanIncluding(VVE_DEFSETSDECIMAL).GetLength() > 11)//include all valid chars (& sign)
				return VVE_NO_LARGE;
			__int64 i64 =  _tstoi64(lpszVal);//get the window value as a 64bit int
			if(i64 < 0)
				return VVE_NO_SMALL;
			else if(i64 > ULONG_MAX) //now a comparison to ULONG max can be made
				return VVE_NO_LARGE;
		}
		break;
	case SET_OCTAL: 
		//as 37777777777 is largest octal val representable in an long/ulong
		//lengh if 1st verified, then string is compared to 37777777777 comparison 
		//is simply returned as VVE_OK == 0, VVE_NO_LARGE = 1
		s.TrimLeft(_T("0"));
		s.SpanIncluding(VVE_DEFSETOCTAL);//remove rubbish after ok chars
		//if s > _T("37777777777") then 1 is returned (implicit VVE_NO_LARGE)
		if(s.GetLength() > 10)
			return s.Compare(_T("37777777777")) == 1;
		break;
	case SET_HEXADECIMAL://simple length check as FFFFFFFF is max val
		s.TrimLeft(_T("0 "));//remove leading spaces & 0s
		s.SpanIncluding(VVE_DEFSETHEXADECIMAL);//clean up
		if(s.GetLength() > 8)
			return VVE_NO_LARGE;
		break;
	case SET_BINARY://simple length check as 1111 1111 1111 1111 1111 1111 1111 1111 is max val
		s.TrimLeft(_T("0 "));//remove leading spaces & 0s
		s.SpanIncluding(VVE_DEFSETBINARY);//remove rubbish after ok chars
		if(s.GetLength() > 32)
			return VVE_NO_LARGE;
		break;
	case SET_FLOAT:
		{
			TCHAR *end;
			s.TrimLeft(_T("0 "));
			double d = _tcstod(s,&end);
			CString ErrTest(end);
			if(ErrTest.Find(_T("INF")) >= 0)
				return VVE_NO_LARGE;
			if( _isnan(d) || _finite(d) == FALSE || d == 0)
			{
				//s may be "+0" so check this - if any digit other than 0 is in the string, 
				//then 0 s != a string == "0" signed or not!
				if(d == 0)
					if(s.FindOneOf(_T("123456789")) < 0)
						return VVE_OK;//must be zero string!
				return (d==0 ? VVE_NO_SMALL : VVE_NO_LARGE);			
			}
		}
		break;
	case SET_CUSTOM: 
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_DIRPATH:
	case SET_SDECIMAL:
	default://all other sets default to signed decimal LONG for range check
		{
			s.TrimLeft(_T("0-+ "));
			//1st, if the char string is > 11 characters, then it is too large for
			//a long
			if(s.SpanIncluding(VVE_DEFSETSDECIMAL).GetLength() > 11)//include all valid chars 
				return VVE_NO_LARGE;
			__int64 i64 =  _tstoi64(lpszVal);//get the window value as a 64bit int
			if(i64 < LONG_MIN)// compare to LONG min
				return VVE_NO_SMALL;
			else if(i64 > LONG_MAX) //now a comparison to LONG max can be made
				return VVE_NO_LARGE;
		}
		break;
	}
	return VVE_OK;
}

//returns VVE_OK, VVE_NO_SMALL or VVE_NO_LARGE
int CVisValidEdit::CheckRange(VVEset vveset, double dVal)
{

	if( _isnan(dVal) || _finite(dVal) == FALSE)
		return VVE_NO_LARGE;//default check on double - just return NO_LARGE

	switch(vveset) 
	{
	case SET_DECIMAL:
	case SET_OCTAL: 
	case SET_HEXADECIMAL:
	case SET_BINARY:
			if(dVal < 0)
				return VVE_NO_SMALL;
			else if(dVal > ULONG_MAX) //now a comparison to ULONG max can be made
				return VVE_NO_LARGE;
		break;
	case SET_FLOAT:
			;//must be ok, it came in a double
		break;
	case SET_CUSTOM: 
	case SET_ALPHA:
	case SET_ALPHANUM:
	case SET_ALLCHAR:
	case SET_DIRPATH:
	case SET_SDECIMAL:
	default://all other sets default to signed decimal LONG for range check
			if(dVal < LONG_MIN)// compare to LONG min
				return VVE_NO_SMALL;
			else if(dVal > LONG_MAX) //now a comparison to LONG max can be made
				return VVE_NO_LARGE;
		break;
	}
	return VVE_OK;
}

void CVisValidEdit::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    lpwndpos->x = m_rect.left;
    lpwndpos->y = m_rect.top;
    lpwndpos->cx = m_rect.right;
    lpwndpos->cy = m_rect.bottom;
    CEdit::OnWindowPosChanging(lpwndpos);
}
