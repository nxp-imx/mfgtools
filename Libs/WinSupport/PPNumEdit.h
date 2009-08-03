#if !defined(AFX_PPNUMEDIT_H__5B576841_5DBF_11D6_98A4_00C026A7402A__INCLUDED_)
#define AFX_PPNUMEDIT_H__5B576841_5DBF_11D6_98A4_00C026A7402A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PPNumEdit.h : header file
//
#define UDM_PPNUMEDIT				(WM_USER + 102)

#define UDM_PPNUMEDIT_CANCEL		(UDM_PPNUMEDIT)
#define UDM_PPNUMEDIT_ENTER  		(UDM_PPNUMEDIT + 1)
#define UDM_PPNUMEDIT_MOVE  		(UDM_PPNUMEDIT + 2)
#define UDM_PPNUMEDIT_HOTKEY   		(UDM_PPNUMEDIT + 3)

// This structure sent to PPNumEdit parent in a WM_NOTIFY message
typedef struct tagNM_PPNUM_EDIT {
    NMHDR hdr;
	UINT  iEvent;
    UINT  iValue;
} NM_PPNUM_EDIT;

/////////////////////////////////////////////////////////////////////////////
// CPPNumEdit window

class CPPNumEdit : public CEdit
{
// Construction
public:
	enum {	PPNUM_VALUE_DEC = 0, //Type value
			PPNUM_VALUE_HEX,
			PPNUM_VALUE_BIN,
			PPNUM_VALUE_OCT,
			PPNUM_VALUE_ASCII
		};
	enum {	PPNUM_VALUE_CUSTOM = 0, //Size value
			PPNUM_VALUE_BYTE,
			PPNUM_VALUE_WORD,
		};
	enum {	PPNUM_COLOR_VALID_FG = 0, //Index of the colours
			PPNUM_COLOR_VALID_BK,
			PPNUM_COLOR_NOT_VALID_FG,
			PPNUM_COLOR_NOT_VALID_BK
		};

	enum {	PPNUMEDIT_CANCEL_DATA = 0, //The events
			PPNUMEDIT_ENTER_DATA,

			PPNUMEDIT_MOVE_FIRST_DATA,
			PPNUMEDIT_MOVE_END_DATA,
			PPNUMEDIT_MOVE_BEGIN_LINE,
			PPNUMEDIT_MOVE_END_LINE,
			PPNUMEDIT_MOVE_LEFT,
			PPNUMEDIT_MOVE_RIGHT,
			PPNUMEDIT_MOVE_UP,
			PPNUMEDIT_MOVE_DOWN,
			PPNUMEDIT_MOVE_NEXT_FIELD,
			PPNUMEDIT_MOVE_PREV_FIELD,
			PPNUMEDIT_MOVE_PAGE_UP,
			PPNUMEDIT_MOVE_PAGE_DOWN,

			PPNUMEDIT_HOTKEY_HEX,
			PPNUMEDIT_HOTKEY_DEC,
			PPNUMEDIT_HOTKEY_BIN,
			PPNUMEDIT_HOTKEY_OCT,
			PPNUMEDIT_HOTKEY_ASCII,
			
			PPNUMEDIT_MAX_EVENTS
		};

		CPPNumEdit();


// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPPNumEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL	m_bNotificate; //User wants be notificate

	void	SetValue(UINT nValue, UINT nIndexValue = PPNUM_VALUE_HEX, UINT nTypeValue = PPNUM_VALUE_BYTE, UINT nMin = 0, UINT nMax = 0xFF); //Set Edited Value
	UINT	GetValue(); //Gets Current Value

	BOOL	IsValidate(); //Test validate of data
	BOOL    IsChanged(); //Test changed of data

	void	SetDefaultColors(BOOL bRedraw = TRUE); //Sets default colors
	void	SetColor(UINT nIndex, COLORREF crColor, BOOL bRedraw = TRUE); //Sets colors of the control
	virtual ~CPPNumEdit();

	// Generated message map functions
protected:
	CString m_strText; //the string of the current value
	CString m_strOriginal; //the original string of the current value
	UINT m_nValue; //Edited value
	UINT m_nIndexValue; //The index of the value (address, dec, hex, bin, oct, ascii)
	UINT m_nMaxLimit; //The max limit of the value
	UINT m_nMinLimit; //The min limit of the value
	UINT m_nCharsInValue; //How much the chars of the string value;

	BOOL m_bValidValue;

	COLORREF m_crValidBk; //Color of the background
	COLORREF m_crValidFg; //Color of the foreground
	COLORREF m_crNotValidFg;
	COLORREF m_crNotValidBk;

	CBrush m_brValidBk;
	CBrush m_brNotValidBk; //Color background

	LRESULT SendNotify(UINT uNotifyCode);
	BOOL GetNotify();

	//{{AFX_MSG(CPPNumEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnUpdate();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPNUMEDIT_H__5B576841_5DBF_11D6_98A4_00C026A7402A__INCLUDED_)
