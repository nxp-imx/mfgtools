/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// SplashWnd.h : header file
//
// Based on the Visual C++ splash screen component.
//////////////////////////////////////////////////////////////////////

#if !defined(SPLASHWND_H__FBDD806E_8C73_43AB_BB39_105116D6627D__INCLUDED_)
#define		 SPLASHWND_H__FBDD806E_8C73_43AB_BB39_105116D6627D__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
//   Splash Screen class

class CStSplashWnd : public CWnd
{
protected:
	CStSplashWnd();
	virtual ~CStSplashWnd();

	static BOOL m_bShowSplashWnd; // TRUE if the splash screen is enabled.
	static CStSplashWnd* m_pSplashWnd;     // Points to the splash screen.
	CBitmap		 m_bitmap;         // Splash screen image.
	UINT m_VersionAreaHeight;

public:
	// close the SplashScreen:
	static void CloseSplashScreen();
	static BOOL ShowSplashScreen(UINT uTimeOut, UINT uBitmapID, CString csDesc, CString csVers, CString csCopyRight, CWnd* pParentWnd = NULL);

	// -> Input:   bEnable - TRUE to enable the splash screen.
	// -> Remarks: This member function is called to enable the splash screen
	//             component. It is typically called from your CWinApp derived
	//             class InitInstance() method, after the call to ParseCommandLine.
	//
	//             Example:
	//
	//			   CCommandLineInfo cmdInfo;
	//			   ParseCommandLine(cmdInfo);
	//			   CStSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
	//
	static void EnableSplashScreen(BOOL bEnable = TRUE);

	// -> Input:   A pointer to a MSG structure that contains the message to process.
	// -> Returns: TRUE if successful; otherwise FALSE;
	// -> Remarks: This member function is by your CWinApp derived class from the
	//             overridden CWinApp::PreTranslateMessage. This will route messages
	//             to the splash screen while it is active.
	//
	//             Example:
	//
	//			   BOOL CDialogApp::PreTranslateMessage(MSG* pMsg) 
	//	           {
	//                 // Route messages to the splash screen while it is visible
	//                 if (CStSplashWnd::PreTranslateAppMessage(pMsg)) {
	//                     return TRUE;
	//                 }
	//		
	//                 return CWinApp::PreTranslateMessage(pMsg);
	//             }
	//
	static BOOL PreTranslateAppMessage(MSG* pMsg);

	CString m_csDesc;
	CString m_csVers;
	CString m_csCopyRight;


protected:

	// -> Remarks: This member function is called internally by the CStSplashWnd class
	//             to destroy the splash window once the timer has run out.
	void HideSplashScreen();

	//{{AFX_VIRTUAL(CStSplashWnd)
	public:
	virtual void PostNcDestroy();
	protected:
	//}}AFX_VIRTUAL

protected:


	//{{AFX_MSG(CStSplashWnd)
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void InitFonts();
	CFont	m_cfStatusText;
	COLORREF m_crTextCol;
	BOOL m_bShow;
	CSize m_csTextExt;
	int		m_iX;
	int		m_iY;

};/* class CStSplashWnd */

/////////////////////////////////////////////////////////////////////////////


#endif // SPLASHWND_H__FBDD806E_8C73_43AB_BB39_105116D6627D__INCLUDED_
