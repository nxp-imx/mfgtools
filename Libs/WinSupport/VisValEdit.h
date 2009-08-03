// ****************************************************
// *                                                  *
// *     CVisValidEdit by stevie_mac@talk21.com       *
// *                                                  *
// ****************************************************
//
// Based on an original idea by joseph. 
// EMail him at - newcomer@flounder.com or visit his excellent site
// at http://www.pgh.net/~newcomer/index.htm

#if !defined(AFX_VISVALEDIT1_H__INCLUDED_)
#define AFX_VISVALEDIT1_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FloatingEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVisValidEdit window

// Predefined sets
#define VVE_DEFSETALPHANUM		_T(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
#define VVE_DEFSETALPHA			_T(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
#define VVE_DEFSETDECIMAL		_T("0123456789")
#define VVE_DEFSETSDECIMAL		_T("-+0123456789")
#define VVE_DEFSETHEXADECIMAL	_T("abcdefABCDEF0123456789")
#define VVE_DEFSETOCTAL			_T("01234567")
#define VVE_DEFSETBINARY		_T("01")
#define VVE_DEFSETFLOAT			_T("0123456789+-.Ee")
#define VVE_DEFSETALL			_T("!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ")
//#define VVE_DEFSETDIRPATH		_T(" ~!@#$%{}()+-_.[]abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
// Added "\\/:" and removed "%{}()"
#define VVE_DEFSETDIRPATH		_T(" ~!@#$+-_.\\/:abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")

//default MoreInfo strings
#define VVE_DEFINFO_NOINFO		_T("No additional information available.")
#define VVE_DEFINFO_NOVALDATION _T("All characters are allowed in any combination.")
#define VVE_DEFINFO				VVE_DEFINFO_NOINFO
#define VVE_DEFINFOALL			VVE_DEFINFO_NOVALDATION
#define VVE_DEFINFOALPHANUM		_T("Allowable characters are...\n' \\/*-+.?@';:_~#!\"£$%^&*(),abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
#define VVE_DEFINFOALPHA		_T("Allowable characters are...\n' abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'.")
#define VVE_DEFINFODECIMAL		_T("DECIMAL INPUT\nAllowable characters are...\n'0123456789'\n\nMIN value = 0, MAX value = 4294967295.")
#define VVE_DEFINFOSDECIMAL		_T("SIGNED DECIMAL INPUT\nAllowable characters are...\n'-+0123456789'.\n\nFormat... [SIGN] DIGITS. [] = Optional.\n\nMIN value = -2147483646, MAX value = 2147483647.")
#define VVE_DEFINFOHEXADECIMAL	_T("HEXADECIMAL INPUT\nAllowable characters are...\n'abcdefABCDEF0123456789'.\n\nMAX Hexadecimal value = FFFF,FFFF.")
#define VVE_DEFINFOOCTAL		_T("OCTAL INPUT\nAllowable characters are...\n'01234567'.\n\nMAX Octal input = 37,777,777,777.")
#define VVE_DEFINFOBINARY		_T("BINARY INPUT\nAllowable characters are...\n'01'.\n\nMAX value = 1111 1111 1111 1111 1111 1111 1111 1111.")
#define VVE_DEFINFOFLOAT		_T("FLOATING POINT INPUT\nAllowable characters are...\n'0123456789+-.Ee'.\n\nFormat...[sign] [digits] [.digits] [ {e | E}[sign]digits]\n\nMIN value = 2.2250738585072014 E-308, MAX value = 1.7976931348623158 E+308.")
#define VVE_DEFINFODIRPATH		_T("Allowable characters are...\n' ~!@#$%{}()+-_.[]0123456789.\nabcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ.")

//default context info strings 
#define VVE_DEFSTRERROR			_T("")
#define VVE_DEFSTREMPTY			_T("IS EMPTY")
#define VVE_DEFSTRVALID			_T("VALID SYNTAX")
#define VVE_DEFSTRINVALID		_T("INVALID SYNTAX")
#define VVE_DEFSTRINCOMPLETE	_T("INCOMPETE")
#define VVE_DEFSTRSHORT			_T("TOO SHORT")
#define VVE_DEFSTRLONG			_T("TOO LONG")
#define VVE_DEFSTRRANGEERR		_T("NUMBER OUT OF RANGE")
#define VVE_DEFSTRCUST1			_T("ERROR")
#define VVE_DEFSTRCUST2			_T("ERROR")

// Default colors
#define RGBDEF_EMPTY			::GetSysColor(COLOR_WINDOW)
#define RGBDEF_VALID			RGB(192,255,192) //green
#define RGBDEF_INVALID			RGB(252,132,132) //red
#define RGBDEF_INCOMPLETE		RGB(255,255,128) //yellow
#define RGBDEF_CUST1			RGB(255,196,196) //lt red
#define RGBDEF_CUST2			RGB(190,190,255) //lilac

// user messages
#define UWM_VVE_DO_CUSTOMVALIDATION (WM_APP + 1)
#define UWM_VVE_VALIDITY_CHANGED (WM_APP + 2)
#define UWM_VVE_CONTENT_CHANGED (WM_APP + 3)

//context constants
#define ID_CONTEXT_INFO (WM_APP + 4)
#define ID_UNDO (WM_APP + 5)
#define ID_CUT (WM_APP + 6)
#define ID_COPY (WM_APP + 7)
#define ID_PASTE (WM_APP + 8)
#define ID_DELETE (WM_APP + 9)
#define ID_SEL_ALL (WM_APP + 10)

//other constants
#define LONG_STRMAX 33 /*enough for a string containing a binary/hexadecimal/octal equiv of a long/ulong*/
#define USHORT_STRMAX 6

class CVisValidEdit : public CEdit
{
// Construction & enums
public:
	//return constants - each has a matching brush (BR_INCOMPLETE, BR_SHORT, BR_LONG, BR_RANGEERR are same brush)
	enum VVEbrush {BR_ERROR = 0, BR_EMPTY, BR_VALID, BR_INVALIDSYNTAX, BR_INCOMPLETE, BR_SHORT, BR_LONG, BR_RANGEERR, BR_CUSTOM_ERR1, BR_CUSTOM_ERR2};
	//built in validation sets
	enum VVEset {SET_CUSTOM, SET_DIRPATH, SET_ALLCHAR, SET_ALPHANUM, SET_ALPHA, SET_DECIMAL, SET_SDECIMAL, SET_HEXADECIMAL, SET_OCTAL, SET_BINARY, SET_FLOAT };
	//internal constants
	enum {VVE_LEN_SHORT = -1, VVE_NO_SMALL = -1, VVE_OK, VVE_LEN_LONG,  VVE_NO_LARGE = 1, VVE_MIN_CHAR = 32, VVE_MAX_CHAR = 255};
	//internal defaults
	enum {VVE_DEF_MINLEN = 0 , VVE_DEF_MAXLEN = 0xffff};
	CVisValidEdit();

// Attributes
public:
	BOOL IsValid() 
	{return m_bNoValidation ? TRUE : m_bValid; };
	BOOL IsValidLength() 
	{return (m_bNoValidation || m_bAllowEmpty || !m_bDoLengthCheck) ? TRUE : m_bValidLength; };
	BOOL IsValidSyntax() 
	{return m_bNoValidation ? TRUE : m_bValidSyntax; };

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVisValidEdit)
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	int  CheckRange();
	int  CheckRange(LPCTSTR lpszVal)
	{	return CheckRange(m_setCurrent,lpszVal);};
	static int CheckRange(VVEset vveset, LPCTSTR lpszVal);
	static int CheckRange(VVEset vveset, double dVal);
	void DisplayMoreInfo(BOOL bAddOnExtraInfo = TRUE);
	void GetMoreInfoString(VVEset vveset, CString &sInfoString, BOOL bAddOnExtraInfo = TRUE );
	void SetMoreInfoString(VVEset vveset, LPCTSTR sNewInfoString = NULL /*NULL = reset to default*/);
	void SetContextInfo(VVEbrush ErrBrush, LPCTSTR sErrMesg){m_arrErrStrs.SetAt(ErrBrush,sErrMesg);};
	void GetContextInfo(VVEbrush ErrBrush, CString &sErrMesg){sErrMesg = m_arrErrStrs.GetAt(ErrBrush);};
	void GetCurContextInfo(CString &sErrMesg){sErrMesg = m_arrErrStrs.GetAt(m_iCurrentErr);};
	void SetEnableContextInfo(BOOL bEnable = TRUE){m_bEnableErrorMessages = bEnable;};
	BOOL GetEnableContextInfo(){return m_bEnableErrorMessages;};
	void SetEnableRangeChecking(BOOL bEnable = TRUE){m_bEnableRangeCheck = bEnable;};
	BOOL GetEnableRangeChecking(){return m_bEnableRangeCheck;};
	void SetCustomErrorIsValid(int iCustomWhich, BOOL bSetIsValid = TRUE)
	{	if(iCustomWhich > 2 || iCustomWhich < 1)
			return ;
		m_bCustomErrorIsValid[iCustomWhich] = bSetIsValid;
	};
	BOOL GetCustomErrorIsValid(int iCustomWhich, BOOL &bCustomIsValid )
	{	if(iCustomWhich > 2 || iCustomWhich < 1)
			return FALSE;
		bCustomIsValid = m_bCustomErrorIsValid[iCustomWhich];
		return TRUE;;
	};
	void DoValidation(BOOL bRedraw = TRUE);//explicit request
	void SetEnableLengthCheck(BOOL bEnable = TRUE){m_bDoLengthCheck = bEnable;};
	BOOL GetEnableLengthCheck(){return m_bDoLengthCheck;};
	BOOL SetBrushColour(VVEbrush brWhich, COLORREF NewColour);
	COLORREF GetBrushColour(VVEbrush brWhich);
	BOOL SetWindowValue(double fSetVal);
	BOOL SetWindowValue(long lSetVal);
	BOOL SetWindowValue(unsigned long lSetVal);
	BOOL SetWindowValue(unsigned short lSetVal);

	BOOL GetWindowValue(double &fVal);//returns FALSE if string is too large for double
	BOOL GetWindowValue(long &lVal);//returns FALSE if string is too small/large for long 
	BOOL GetWindowValue(unsigned long &lVal);//returns FALSE if string is too large for ulong
	BOOL GetWindowValue(unsigned short &uslVal);//returns FALSE if string is too large for ushort

	BOOL GetAllCharEntry(){return m_bAllowAllCharEntry;};
	void SetAllCharEntry(BOOL bAllowAllCharsIn = TRUE){m_bAllowAllCharEntry = bAllowAllCharsIn;};
	BOOL GetEmptyValid(){return m_bAllowEmpty;};
	void SetEmptyValid(BOOL bEmptyOK = TRUE){m_bAllowEmpty = bEmptyOK;};
	void SetDisableValidation(BOOL bNoValidation = TRUE){m_bNoValidation = bNoValidation;};
	BOOL GetDisableValidation(){return m_bNoValidation;};
	void SetCustomValidation(BOOL bUseCustom = TRUE){m_bCustomValidation = bUseCustom;};
	BOOL GetCustomValidation(){return m_bCustomValidation;};
	void SetValidCharSet(VVEset vveset, LPCTSTR cstrCustomChar = NULL);
	void SetValidSet(VVEset vveset){m_setCurrent = vveset;};
	VVEset GetValidSet(){return m_setCurrent;};
	void SetValidChars(LPCTSTR cstrCustomChar);
	void GetValidChars(CString & strValidChars);
	void SetTextLenMinMax(int iMin = VVE_DEF_MINLEN, int iMax = VVE_DEF_MAXLEN);
	int	 GetTextMinLen(){return m_iMinInputLen;};
	int  GetTextMaxLen(){return m_iMaxInputLen;};
	void SetMyControlID(INT_PTR _id){m_my_id = _id;};
	INT_PTR GetMyControlID(){return m_my_id;};

	

	virtual ~CVisValidEdit();

	// Generated message map functions
protected:
	void   SelectAll(){  if(m_hWnd){SetFocus();	SetSel(0,-1);}  };
	void   ValidationChanged(BOOL bNewState);//post new state to parent
	CBrush m_brEmpty;
 	CBrush m_brValid; 
	CBrush m_brInvalid;
	CBrush m_brIncomplete;
	CBrush m_brCustom1;
	CBrush m_brCustom2;
	CBrush * Validate();
	CBrush * CheckAlphaNum()	{return Validate();};
	CBrush * CheckDecimal()		{return Validate();};
	CBrush * CheckAlpha()		{return Validate();};
	CBrush * CheckHexadecimal() {return Validate();};
	CBrush * CheckOctal()		{return Validate();};
	CBrush * CheckBinary()		{return Validate();};      
	CBrush * CheckFloat();        
	CBrush * CustomValidation();
	int    CheckLen();//returns VVE_LEN_SHORT , VVE_LEN_LONG or VVE_LEN_OK
	int    CheckLen(CString& str);//returns VVE_LEN_SHORT , VVE_LEN_LONG or VVE_LEN_OK
	
	//monitoring
	BOOL m_bValid;
	BOOL m_bValidSyntax;
	BOOL m_bValidLength;
	BOOL m_bCustomValidationCallOK;
	VVEbrush m_iCurrentErr;//used for context info
	//allwable characters
	BOOL m_bValidChar[UCHAR_MAX];
	BOOL m_bAllCharValid;
	//input lenth
	int m_iMinInputLen;
	int m_iMaxInputLen;
	//currently selected set / brush
	VVEset m_setCurrent;
	//Options
	BOOL m_bEnableErrorMessages;
	BOOL m_bCustomValidation;
	BOOL m_bNoValidation;
	BOOL m_bDoLengthCheck;
	BOOL m_bAllowEmpty;
	BOOL m_bAllowAllCharEntry;//allows invalid chars into control 
	BOOL m_bCustomErrorIsValid[3];
	BOOL m_bEnableRangeCheck;
	// setable id
	INT_PTR m_my_id;
    //error messages
	CStringArray m_arrErrStrs;//context info strings
	CStringArray m_arrMoreStrs;
	// Notification message used for POSTING changes
	NMHDR m_nm;
	//{{AFX_MSG(CVisValidEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
    afx_msg HBRUSH CtlColor(CDC * dc, UINT id);
	afx_msg BOOL OnChange();
	DECLARE_MESSAGE_MAP()
public:
	// the rectangle
    RECT m_rect;
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
};



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VISVALEDIT1_H__INCLUDED_)
