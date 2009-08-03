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

#pragma once


// flags for PropInfo::m_nState
#define PIF_HOTBAR		0x01
#define PIF_HOTITEM		0x02
#define PIF_ONITEM		0x04
#define PIF_PUSHED		0x08
#define PIF_WHITEBG		0x10

// flags for PropInfo::m_nStyle
#define PIS_DISABLED	0x01
#define PIS_LOCKED		0x02
#define PIS_HIDDEN		0x04
#define PIS_BAR			0x08

// notification messages
#define HPCM_UPDATE	(WM_USER+1701)	// nID, nValue
#define HPCM_DRAWCUSTOM	(WM_USER+1705)	// nCustom, pItem

class CHPDynEdit;
#include "HPPopList.h"


class CHotPropCtrl : public CWnd
{
	DECLARE_DYNCREATE(CHotPropCtrl)
public:
	CHotPropCtrl();
	virtual ~CHotPropCtrl();

    enum PropertyType
    {
	    propList, propEnum, propButton, propToggle, propString, propInt
    };

    // Operations
public:
	virtual INT_PTR OnToolHitTest( CPoint point, TOOLINFO* pTI) const;
	// Load image list from the resources, use given color as transparency mask
	void LoadImages(int nID, COLORREF crMask);

	// Access to the image list, useful for drawing custom properties
	CImageList& GetImageList() const;

	// Add new property (requires update)
	void AddProp(int nID, int nType, LPCTSTR lpszName, int nImage);
	void AddProp(int nID, int nType, UINT uNameID, int nImage);
	// Insert new property after the given one (if nAfter<0 - insert on top)
	void InsertProp(int nID, int nAfter, int nType, LPCTSTR lpszName, int nImage);
	void InsertProp(int nID, int nAfter, int nType, UINT uNameID, int nImage);

	// Change the name of property
	void SetPropName(int nID, LPCTSTR lpszName);
	void SetPropName(int nID, UINT uNameID);
	// Set the image index of property
	void SetPropImage(int nID, int nImage);

	// Set max. string length or number of entries in enum list
	void SetPropLimit(int nID, int nMax);
	// Set min. and max integer values
	void SetPropLimits(int nID, int nMin, DWORD nMax);

	// Set the negative and positive strings for a toggle property
	void SetPropToggle(int nID, LPCTSTR lpszFalse, LPCTSTR lpszTrue);
	void SetPropToggle(int nID, UINT uFalseID, UINT uTrueID);

	// Access to the string array of a list property
	CStringArray& GetPropStringArray(int nID) const;

	// Make a property custom drawn
	void SetCustomProp(int nID, int nCustom);

	// Remove a single property (requires update)
	void RemoveProp(int nID);
	// Remove all properties
	void RemoveAllProps();

	// Update control after adding/removing props
	void Update();

	// Set the numeric value of a property (string updated automatically)
	void SetProp(int nID, DWORD nValue);
	// Set the string value of a property
	void SetProp(int nID, LPCTSTR lpszValue);

	// Get the numeric value of a property
	INT_PTR GetProp(int nID) const;
	// Get the string value of a property
	CString GetPropStr(int nID) const;

	// Enable or disable the property
	void EnableProp(int nID, BOOL bEnable=TRUE);
	// Lock or unlock the property
	void LockProp(int nID, BOOL bLock=TRUE);

	// Give the property the bar style
	void SetPropBar(int nID, BOOL bBar=TRUE);

	// Show or hide the property
	void ShowProp(int nID, BOOL bShow=TRUE, BOOL bUpdate=TRUE);
	// Show or hide a range of properties
	void ShowPropsRng(int nFirstID, int nLastID, BOOL bShow=TRUE, BOOL bUpdate=TRUE);

	// Make the specified property visible (unless it's hidden)
	void EnsureVisible(int nID);

	// Lighten the given color
	static COLORREF LightenColor(COLORREF crColor,double dFactor);

	// Activate property with the corresponding shortcut key
	void ProcessKey(int nKey);


	// Set window to send notifications (default is the parent window)
	void SetWndToNotify(CWnd* pWndToNotify);


	// Change control layout
	void SetSingleLine(BOOL bSingleLine=TRUE);

	// Enable/disable flat style
	void EnableFlatStyle(BOOL bFlatStyle=TRUE, BOOL bRedraw=TRUE);
	// Enable/disable gradient fill
	void EnableGradient(BOOL bGradient=TRUE, BOOL bRedraw=TRUE);
	// Enable/disable list animation
	void EnableListAnimation(BOOL bListAnim=TRUE);


	// Structure passed with the HPCM_DRAWCUSTOM message
	struct CustomItem
	{
		CDC* m_pDC; // device context to use
		CRect m_rcRect; // item rectangle
		int m_nID; // identifier of the property
		INT_PTR m_nValue; // numeric value
		LPCTSTR m_lpszValue; // string value
	};

protected:
//	virtual void PreSubclassWindow( );
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PostNcDestroy();

protected:
	struct PropInfo
	{
		PropInfo(int nID, LPCTSTR lpszName, int nImage, int nType);
		~PropInfo();

		int	m_nID;		// property identifier
		int m_nImage;		// image index (-1 = none)
		CString m_strName;		// name of property
		int m_nType;		// type (propToggle, propList, propInt or propString)
		int m_nValue;		// current numeric value
		CString m_strValue;		// displayed string value
		CStringArray* m_pStrArray;		// for toggle (true/false strings) and list
		int m_nMin;		// min. integer value
		DWORD m_nMax;		// max. integer or string length
		BYTE m_nState;		// PIF flags
		BYTE m_nStyle;		// PIS flags
		int m_nCustom;	// non-zero if owner drawn
		PropInfo* m_pNext;	// simple linked list
	};

	PropInfo* m_pFirst;		// list head and tail
	PropInfo* m_pLast;

	CImageList m_imgList;		// image list for properties

	CFont m_font;	// default gui font

	int m_nHot;		// "hot" property (under the mouse) index
	PropInfo* m_pHot;	// hot prop pointer

	BOOL m_bCapture;	// mouse is captured
	BOOL m_bToggle;		// toggle property pressed
	BOOL m_bLocked;		// dynedit/poplist present
	int m_nPop;		// poplist property

	CHPDynEdit* m_pEdit;	// dynamic edit control
	CHPPopList* m_pList;	// dynami poplist control

	int m_nYPos;	// current vertical position
	int m_nYMax;	// max. vertical position
	int m_nHeight;		// total height of all properties

	BOOL m_bFlatStyle;	// use flat style
	BOOL m_bListAnim;	// animate pop-up list

	BOOL m_bSingleLine;	// double/single line mode

	int m_nItemHeight;	// height of inividual item
	int m_nTitleWidth;	// width of the title bar in single line mode

	CWnd* m_pWndToNotify;	// window to send notifications to

	typedef BOOL (WINAPI *GRADIENTFILL_PROC)(HDC hdc, PTRIVERTEX pVertex, ULONG dwNumVertex, PVOID pMesh, ULONG dwNumMesh, ULONG dwMode);

	HINSTANCE m_hMSImgLib;					// msimg32.dll
	GRADIENTFILL_PROC m_pfGradientFill;	// GradientFill function pointer

protected:
	// Get the index and pointer to prop with given ID
	int FindProp(int nID, PropInfo*& pProp) const;
	// Get previous property also for removing from the list
	int FindProp(int nID, PropInfo*& pPrev, PropInfo*& pProp) const;

	// Get the visible index and pointer to prop
	int FindVisProp(int nID, PropInfo*& pProp) const;

	// Invalidate the area of the property at specified index
	void InvalidateProp(int nProp);
	// Draw the property within given rectangle
	void DrawProp(CDC& dc, PropInfo* pProp, const CRect& rcRect);

	// Draw disabled (grayed) image from the image list
	void DrawDisabled(CDC& dc, int x, int y, int nImage);

	// Prepare to delete a property safely
	void PreRemove();

	// Create dynamic edit and pass the mouse position for correct caret placement
	void CreateEdit(CPoint point);
	// Create dynamic poplist, if bKey=TRUE then highlight current item
	void CreateList(BOOL bKey);

	// Post a HPCM_UPDATE message to current view (if pInfo is null, use current hot prop)
	virtual void NotifyView(PropInfo* pInfo=NULL);

	// Post a HPCM_DRAWCUSTOM message to current view
	virtual void DrawCustomItem(CDC& dc, CRect& rcRect, PropInfo* pProp, UINT_PTR nValue);

	// Fill rectangle with a horizontal gradient
	void GradFillH(CDC& dc, int x, int y, int cx, int cy, COLORREF crLeft, COLORREF crRight);

	// Calculate longest title width in single line mode
	void CalculateTitleWidth();

protected:
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg LRESULT OnEndEdit(WPARAM bDone, LPARAM);
	afx_msg LRESULT OnEndPop(WPARAM nRes, LPARAM);
	afx_msg LRESULT OnDrawPopItem(WPARAM nItem, LPARAM pDrawInfo);
	DECLARE_MESSAGE_MAP()
};
