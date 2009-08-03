
// StBinder.cpp : Defines the class behaviors for the application.
//

#define G_STBINDER 1
#include "stdafx.h"
#include "StBinder.h"
#include "StBinderDlg.h"
#include "default_langid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

LANGID g_DefaultIDs[DEFAULT_LANGIDS] =
{
	0x007F,	// Invariant Language used as failsafe or language neutral
	0x0404,	// Chinese Traditional
	0x0804,	// Chinese Simplified
	0x0409,	// English
	0x000C,	// French
	0x0007, // German
	0x0011,	// Japanese
	0x0012,	// Korean
	0x0016,	// Portuguese
	0x000A	// Spanish
};

// Default and user defined language IDs
USHORT g_LangIdCount;
LANGID *g_LangIds;

// CStBinderApp

BEGIN_MESSAGE_MAP(CStBinderApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CStBinderApp construction

CStBinderApp::CStBinderApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CStBinderApp object

CStBinderApp theApp;


// CStBinderApp initialization

BOOL CStBinderApp::InitInstance()
{
#ifdef _DEBUG
	afxDump.SetDepth(1);
//	_CrtSetBreakAlloc (209);
#endif

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
	SetRegistryKey(_T("Freescale"));

	if (!AfxOleInit())
		return FALSE;
//		AfxMessageBox("Error - AfxOleInit");

	if ( !GetLangIds() )
		return FALSE; // we had a memory failure


	CStBinderDlg dlg;
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

	SaveLangIds();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

BOOL CStBinderApp::GetLangIds()
{
	HKEY hKey;
	DWORD dwDataLen = DEFAULT_LANGIDS * sizeof(LANGID);

	g_LangIdCount = DEFAULT_LANGIDS;
	g_LangIds = (LANGID *)VirtualAlloc(NULL, dwDataLen, MEM_COMMIT, PAGE_READWRITE);
	if ( !g_LangIds )
		return FALSE;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Freescale\\StBinder", 0,
			KEY_READ, &hKey) == ERROR_SUCCESS )
	{
		DWORD dwType;
		LONG queryStatus = RegQueryValueEx( hKey, L"LangIds", NULL, &dwType,
			(LPBYTE)g_LangIds, &dwDataLen);
		if ( queryStatus == ERROR_MORE_DATA )
		{
			VirtualFree (g_LangIds, 0, MEM_RELEASE);
			g_LangIds = (LANGID *) VirtualAlloc(NULL, dwDataLen, MEM_COMMIT, PAGE_READWRITE);
			if ( !g_LangIds )
			{
				RegCloseKey(hKey);
				return FALSE;
			}

			if ( RegQueryValueEx( hKey, L"LangIds", NULL, &dwType,
					(LPBYTE)g_LangIds, &dwDataLen) == ERROR_SUCCESS)
				g_LangIdCount = (USHORT)dwDataLen / sizeof(LANGID);
		}
		else
		{
			if ( queryStatus == ERROR_SUCCESS )
				g_LangIdCount = (USHORT)dwDataLen / sizeof(LANGID);
			else
				memcpy (g_LangIds, g_DefaultIDs, sizeof(g_DefaultIDs));
		}

		RegCloseKey(hKey);
	}
	else
		memcpy (g_LangIds, g_DefaultIDs, sizeof(g_DefaultIDs));

	return TRUE;
}

void CStBinderApp::SaveLangIds()
{
	HKEY hKey;
	DWORD dwDataLen = g_LangIdCount * sizeof(LANGID);

	if (RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Freescale\\StBinder",
			&hKey) == ERROR_SUCCESS )
	{
		RegSetValueEx( hKey, L"LangIds", NULL, REG_BINARY,
						(LPBYTE)g_LangIds, dwDataLen);
		RegCloseKey(hKey);
	}

	if ( g_LangIds )
	{
		VirtualFree( g_LangIds, 0, MEM_RELEASE );
		g_LangIds = NULL;
	}
}

BOOL CStBinderApp::PumpMessages()
{
    MSG msg;
    while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) 
    { 
        if ( !PumpMessage( ) ) 
        { 
            ::PostQuitMessage( 0 ); 
            return FALSE;
        } 
    } 
    // let MFC do its idle processing
    LONG lIdle = 0;
    while ( AfxGetApp()->OnIdle(lIdle++ ) )
        ;  
    // Perform some background processing here 
    // using another call to OnIdle

	return TRUE;
}

BOOL CStBinderApp::GetString(UINT _iResID, CString& _str)
{
	BOOL result=TRUE;

	result = _str.LoadString(_iResID);
	if( !result )
	{
		HINSTANCE hInst = GetModuleHandle(NULL);
		if (hInst)
			result = _str.LoadString( hInst, _iResID, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT));
	}

	return result;
}
