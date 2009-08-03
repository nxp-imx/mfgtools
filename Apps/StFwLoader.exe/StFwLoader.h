// StFwLoader.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "StFwLoaderDlg.h"

// CStFwLoaderApp:
// See StFwLoader.cpp for the implementation of this class
//

class CStFwLoaderApp : public CWinApp
{
public:
	CStFwLoaderApp();

	virtual BOOL InitInstance();
	virtual int ExitInstance();

private:
	CString m_app_root;
	uint32_t m_error_level;

	struct CmdArgs
	{
		bool    h;      // Help
		bool    s;      // Silent mode.
		CString f;      // Firmware filename.
		int     w;      // Wait for device (seconds)
		CString u;      // Usage string
	} m_cmds;

    bool ParseCommandLine();
    bool ParseArgument(CString arg);

    uint32_t CheckFileName();
	uint32_t DownloadFirmware();

	DECLARE_MESSAGE_MAP()
};

extern CStFwLoaderApp theApp;