/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StFwLoader.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "StFwLoader.h"
#include ".\stfwloader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
// First load StUnicoW.dll to add unicode API support for Win9x/Me.
//
HMODULE __stdcall LoadUnicowsProc(void);

#define MICROSOFT_LAYER_FOR_UNICODE 1

extern "C" HMODULE (__stdcall *_PfnLoadUnicows)(void) = &LoadUnicowsProc;

//extern FARPROC _PfnLoadUnicows = (FARPROC) &LoadUnicowsProc;

HMODULE __stdcall LoadUnicowsProc(void)
{
	//
	// No other API should be called here.
	//
    return(LoadLibraryA("stunicow.dll"));
}

// CStFwLoaderApp

BEGIN_MESSAGE_MAP(CStFwLoaderApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CStFwLoaderApp construction

CStFwLoaderApp::CStFwLoaderApp()
{
	m_error_level = ERROR_SUCCESS;
}


// The one and only CStFwLoaderApp object

CStFwLoaderApp theApp;


// CStFwLoaderApp initialization

BOOL CStFwLoaderApp::InitInstance()
{
//	_CrtSetBreakAlloc(360);

	// Get the application path 
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR buf[_MAX_PATH];
	GetModuleFileName(NULL, buf, _MAX_PATH);
	_tsplitpath_s( buf, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0 );
	m_app_root = drive;
	m_app_root += dir;

	// Make the dialog here in case we have to use some non-GUI
	// functionality in RunSilent()

    // Check for command line arguments or run in GUI mode.
    if (ParseCommandLine())
    {        
		if ( m_cmds.h )
		{
			// Display Help
			AfxMessageBox(m_cmds.u);
		}
		else if ( (m_error_level = CheckFileName()) == ERROR_SUCCESS )
		{
			m_error_level = DownloadFirmware();
		}
	}
	else
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
		SetRegistryKey(_T("SigmaTel"));

		CStFwLoaderDlg dlg;
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
	}
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

uint32_t CStFwLoaderApp::DownloadFirmware()
{
	uint32_t err=ERROR_SUCCESS;
	StFwComponent fw;
	/*CStDeviceRcvry dev;
	
	// Initialize firmware component
	if ( (err = fw.Initialize(m_cmds.f)) != STERR_NONE )
		return err;
	
	// Look for a recovery mode device
	#define SLEEP_TIME	100 // milliseconds
	int loop_cnt = ( m_cmds.w * 1000 ) / SLEEP_TIME;
	for ( int i=0; i<=loop_cnt; ++i )
	{
		int16_t index = 0;
		if ( (err = dev.Initialize(&index)) == STERR_NONE )
			break;
		else
			Sleep(SLEEP_TIME);
	}
	
	// Download firmware
	if ( err != ERROR_SUCCESS )
*/		return err;

//	return dev.Download(&fw);
}

//----------------------------------------------------------------
// ParseCommandLine()
//
// Homegrown command line parser for command line mode.
//----------------------------------------------------------------
bool CStFwLoaderApp::ParseCommandLine()
{
    CString sCmdLine = m_lpCmdLine;
    CString curArg = _T("");
    int pos = 0;
    bool argsFound = false;
    m_cmds.h = false;
	m_cmds.s = false;
	m_cmds.w = 30; // default to wait up to 30 seconds for a device
    m_cmds.f = _T("");
	m_cmds.u.Format(_T("Usage: %s.exe [/f filename] [/w seconds] [/s] [/h]\r\n\nwhere: \r\n\n\t/f filename\t- Specify the firmware file to download. Looks for the file in the application directory if the path is not specified.\r\n\t/w seconds\t- Specifies the length of time to wait for a recovery mode device. Default is 30 seconds.\r\n\t/s\t\t- Silent mode. Suppress message boxes used to display errors.\r\n\t/h or /?\t\t- Displays this help screen.\r\n\nreturns:\r\n\tsuccess:\tERRORLEVEL == 0\r\n\terror:\tERRORLEVEL != 0\r\n"), m_pszAppName);

    curArg = sCmdLine.Tokenize(_T("/"), pos);

    while (pos != -1 && curArg != _T(""))
    {
        bool validArg = ParseArgument(curArg);
        curArg = sCmdLine.Tokenize(_T("/"), pos);

        if (validArg)
            argsFound = true;
    }

    return argsFound;
}

//----------------------------------------------------------------
// ParseArgument()
//
// Parse command line argument and store value.
// /f <filename>    - Firmware file name to download.
// /q               - Quiet mode.
// /h or /?         - Usage screen.
//----------------------------------------------------------------
bool CStFwLoaderApp::ParseArgument(CString arg)
{
    bool validArg = false;

	switch (arg.MakeLower().GetAt(0))
    {
        case _T('f') :
            m_cmds.f = arg.Right(arg.GetLength()-1);
            m_cmds.f.TrimLeft();
            m_cmds.f.TrimRight();
            m_cmds.f.Remove(_T('\"'));
            validArg = true;
            break;
        case _T('s'):
            m_cmds.s = true;
            validArg = true;
            break;
        case _T('w'):
            m_cmds.w = _tstoi(arg.Right(arg.GetLength()-1));
            validArg = true;
            break;
        case _T('h'):
        case _T('?'):
            m_cmds.h = true;
            validArg = true;
            break;
        default:
            break;
    }

    return validArg;
}

uint32_t CStFwLoaderApp::CheckFileName()
{
	uint32_t err = ERROR_SUCCESS;
	CString msg;
	if ( m_cmds.f.IsEmpty() )
	{
		msg.Format(_T("ERROR - You must specify a firmware file to download.\r\n%s.\r\n\n%s"), m_cmds.f, m_cmds.u);
		if (!m_cmds.s)
			AfxMessageBox(msg, MB_ICONERROR);
		err = ERROR_FILE_NOT_FOUND;
	}
	else if ( _taccess(m_cmds.f, 0) == -1) 
	{
		CString full_name = m_app_root + m_cmds.f;
		if ( _taccess(full_name, 0) == -1) 
		{
			msg.Format(_T("ERROR - Can not find %s.\r\n\n%s"), m_cmds.f, m_cmds.u);
			if (!m_cmds.s)
				AfxMessageBox(msg, MB_ICONERROR);
			err = ERROR_FILE_NOT_FOUND;
		}
		else
			m_cmds.f = full_name;
	}
	return err;
}

int CStFwLoaderApp::ExitInstance()
{
	int ret = CWinApp::ExitInstance();

	return (int)m_error_level;
}
