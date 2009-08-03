// SDKRegScrub.cpp : Defines the class behaviors for the application.
// Copyright (c) 2005 SigmaTel, Inc.

#include "stdafx.h"
#include "regscrublist.h"
#include "SDKRegScrub.h"
#include "SDKRegScrubDlg.h"

#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSDKRegScrubApp

BEGIN_MESSAGE_MAP(CSDKRegScrubApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CSDKRegScrubApp construction
/*****************************************************************************/
CSDKRegScrubApp::CSDKRegScrubApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CSDKRegScrubApp object

CSDKRegScrubApp theApp;

// CSDKRegScrubApp initialization
/*****************************************************************************/
BOOL CSDKRegScrubApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

//    CSplash Splash;
//    Splash.Create(m_pMainWnd);
//    Splash.ShowWindow(SW_SHOW);
//    Splash.UpdateWindow();
//    Sleep(1250);
//    Splash.DestroyWindow(); // kill the splash screen

	CSDKRegScrubDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
