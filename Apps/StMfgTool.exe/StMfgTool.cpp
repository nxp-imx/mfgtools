// StMfgTool.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <shlwapi.h>
#include "StMfgTool.h"
#include "MainFrm.h"
#include "DefaultProfile.h"
#include "StSplashWnd.h"
#include "../../Libs/DevSupport/DeviceManager.h"

#include "../../Common/updater_res.h"
#include "../../Common/updater_restypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HANDLE g_HIDMutex;

#ifdef RESTRICTED_PC_IDS
typedef LPSTR (CALLBACK* LPFNDLLFUNC1)(BOOL, BOOL, BOOL, BOOL, LPCTSTR);
#define MAX_PC_IDS	10
LPTSTR g_RestrictedPCIds[MAX_PC_IDS] = {
	_T("A61F-440C-224B-79D1"),
	_T("9BE7-79EC-AFED-A5AC"),
	_T("C9CD-DF49-5554-44C5"),
	_T(""),
	_T(""),
	_T(""),
	_T(""),
	_T(""),
	_T(""),
	_T("")};
TCHAR g_ThisPCId[20];

BOOL ValidateHardwareId(void);
BOOL ValidateHardwareId()
{
	HINSTANCE hDLL;               // Handle to DLL
	LPFNDLLFUNC1 lpfnDllFunc1;    // Function pointer
	BOOL bAccess = FALSE;

	hDLL = LoadLibrary(_T(".\\HardwareID.dll"));
	if (hDLL != NULL)
	{
	   lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(hDLL,
                                           "GetHardwareID");
	   if (lpfnDllFunc1)
	   {
		  LPSTR szId;
		  g_ThisPCId[0] = '\0';
		  // call the function
	      szId = lpfnDllFunc1(TRUE, TRUE, TRUE, TRUE, _T("Philips DRM Tool"));
		  if (szId)
		  {
			  USES_CONVERSION;
			  _tcscpy_s(g_ThisPCId, 20, A2T(szId));
			  for (int i = 0; i < MAX_PC_IDS; ++i)
			  {
				  if(!_tcscmp(g_ThisPCId, g_RestrictedPCIds[i]))
				  {
					  bAccess = TRUE;
					  break;
				  }
			  }
		  }
	   }
	   FreeLibrary(hDLL);
	}

	return bAccess;
}
#endif

// CStMfgToolApp

BEGIN_MESSAGE_MAP(CStMfgToolApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()


// CStMfgToolApp construction

CStMfgToolApp::CStMfgToolApp()
	: m_p_main_frame(NULL)
	, m_p_config_mgr(NULL)
{
}

BOOL g_TestLoop = FALSE;

// The one and only CStMfgToolApp object
CStMfgToolApp theApp;
BOOL CStMfgToolApp::bClassRegistered = FALSE;


// CStMfgToolApp initialization

BOOL CStMfgToolApp::InitInstance()
{
#ifdef _DEBUG
	afxDump.SetDepth(1);
	//_CrtSetBreakAlloc (262);
#endif

    // If a previous instance of the application is already running,
    // then activate it and return FALSE from InitInstance to
    // end the execution of this instance.

    if(!FirstInstance())
    return FALSE;


    // Register your unique class name that you wish to use
    WNDCLASS wndcls;
    memset(&wndcls, 0, sizeof(WNDCLASS));   // start with NULL
                                            // defaults

    wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndcls.lpfnWndProc = ::DefWindowProc;
    wndcls.hInstance = AfxGetInstanceHandle();
    wndcls.hIcon = LoadIcon(IDR_MAINFRAME); // or load a different
                                            // icon
    wndcls.hCursor = LoadCursor( IDC_ARROW );
    wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wndcls.lpszMenuName = NULL;

    // Specify your own class name for using FindWindow later
    wndcls.lpszClassName = _T("CStMfgToolAppClass");

    // Register the new class and exit if it fails
    if(!AfxRegisterClass(&wndcls))
    {
    ATLTRACE(_T("Class Registration Failed\n"));
    return FALSE;
    }
    bClassRegistered = TRUE;

	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	if (_tcsicmp(this->m_lpCmdLine, L"/TestLoop") == 0 )
		g_TestLoop = TRUE;
/*clw*/
	// Initialize OLE libraries
    // Win2K will hang on GetOpenFileName() dialog if we do the CoInitializeEx, so
    // if MTP is not enabled, we do AfxOleInit instead.
	// TODO: ~clw~ Use gWindowsVersionInfo
	
//SP    if ( IsMTPSupported() )
	OSVERSIONINFO wVersion;
	wVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&wVersion);
	if ((wVersion.dwMajorVersion >= 6) ||
		(wVersion.dwMajorVersion == 5 && wVersion.dwMinorVersion >= 1))
    {
    	HRESULT hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
        if (hr != S_OK)
    	{
	    	AfxMessageBox(IDP_OLE_INIT_FAILED);
		    return FALSE;
    	}
    }
    else  if (!AfxOleInit())
   	{
    	AfxMessageBox(IDP_OLE_INIT_FAILED);
	    return FALSE;
   	}
/**///clw
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization


	SetRegistryKey(_T("Freescale"));

	// initialize the version infomation class
	CString _path;
	GetModuleFileName(NULL, _path.GetBuffer(_MAX_PATH), _MAX_PATH);
	_path.ReleaseBuffer();
	m_version_info.Load(_path);


	//
	// Prepare the DeviceManager object for use. 
	// Note: We must call gDeviceManager::Instance().Close() before the app exits.
	VERIFY(gDeviceManager::Instance().Open() == ERROR_SUCCESS);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;
	m_p_main_frame = pFrame;


	// create the logger
	m_pLogMgrDlg = new CLogMgrDlg();
	if (m_pLogMgrDlg == NULL) {
		TRACE0("Failed to create the log manager\n");
		return FALSE;      // fail to create
	}
	m_pLogMgrDlg->Create();

	// create the configuration manager
	m_p_config_mgr = new CConfigMgrSheet(_T("StMfgTool - Configuration"), pFrame);
	if ( m_p_config_mgr == NULL ) {
		TRACE0("Failed to create the configuration manager\n");
		return FALSE;      // fail to create
	}
	
	// gets the last saved profile
	m_p_config_mgr->InitConfigMgr();

	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

#ifdef RESTRICTED_PC_IDS
	if (!ValidateHardwareId())
	{
		CString resStr;

		resStr.LoadString(IDS_UNAUTHORIZED_SYSTEM);
		::MessageBox(NULL, resStr, _T("StMfgTool"), MB_OK);
		return FALSE;
	}
#endif

	// If the recovery driver is not installed try to load it from resource
    // as it may have been bound to the executable.
    if (!IsRecoveryDriverInstalled())
    {
        InstallRecoveryDriver();
    }

	g_HIDMutex = CreateMutex(NULL, FALSE, NULL);

	// The one and only window has been initialized, so show and update it
//	pFrame->ShowWindow(SW_SHOW);
//	pFrame->UpdateWindow();
	pFrame->ActivateFrame(SW_SHOW);

	SetPriorityClass( GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS );


	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}

BOOL CStMfgToolApp::FirstInstance(void)
{
    CWnd *pWndPrev, *pWndChild;

    // Determine if another window with your class name exists...
    if (pWndPrev = CWnd::FindWindow(_T("CStMfgToolAppClass"),NULL))
    {
        // If so, does it have any popups?
        pWndChild = pWndPrev->GetLastActivePopup();

        // If iconic, restore the main window
        if (pWndPrev->IsIconic())
            pWndPrev->ShowWindow(SW_RESTORE);

        // Bring the main window or its popup to
        // the foreground
        pWndChild->SetForegroundWindow();

        // and you are done activating the previous one.
        return FALSE;
    }
    // First instance. Proceed as normal.
    else
        return TRUE;
}

int CStMfgToolApp::ExitInstance()
{
/*clw
	if(m_p_usb_port_mgr) {
		delete m_p_usb_port_mgr;
		m_p_usb_port_mgr = NULL;
	}
*///clw
	if ( m_p_config_mgr != NULL) {
		delete m_p_config_mgr;
		m_p_config_mgr = NULL;
	}

	if ( m_pLogMgrDlg != NULL) 
	{
		delete m_pLogMgrDlg;
		m_pLogMgrDlg = NULL;
	}

	// gracefully shut down the DeviceManager
	gDeviceManager::Instance().Close();

	if(bClassRegistered)
        ::UnregisterClass(_T("CStMfgToolAppClass"),AfxGetInstanceHandle());

    // Uninitialize ole here and not in the destructor.
    // That can be problematic causing the exit to hang occasionally.
    ::CoFreeUnusedLibraries();
    ::CoFreeAllLibraries();
	CoUninitialize();

    return CWinApp::ExitInstance();
}



// App command to run the dialog
void CStMfgToolApp::OnAppAbout()
{
	CString desc_text;
	CString version_text;
	CString copyright_text;

	CVersionInfo * _info = theApp.GetVersionInfo();

	if ( _info )
	{
		desc_text = _info->GetProductName();
		version_text.Format(_T("Version: %s"), _info->GetProductVersionAsString());
		copyright_text = _info->GetLegalCopyright();
	}
	else
	{
		desc_text = _T("STMP Manufacturing Tool");
		version_text = _T("Version resource not found");
		copyright_text = _T("Copyright (c) 2008, Freescale Semiconductor, Inc.\nAll Rights Reserved");
	}
	
	CStSplashWnd::ShowSplashScreen(5000, IDB_COMPANY_BMP, desc_text, version_text, copyright_text, m_pMainWnd);

//	CAboutDlg aboutDlg;
//	aboutDlg.DoModal();
}

PLATFORM CStMfgToolApp::GetPlatform()
{
    OSVERSIONINFOEX osvi = {0};
//    WCHAR szMsg[100];
    
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    
    GetVersionEx((OSVERSIONINFO*)&osvi);
	
    //swprintf (szMsg, 100, L"major: %d minor %d", osvi.dwMajorVersion, osvi.dwMinorVersion);
    //MessageBox(NULL, szMsg, L"test", MB_OK);
	if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
	   	return  OS_2K;
    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
	   	return  OS_XP;
    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 2)
	   	return OS_XP64;
    else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
	{
        // Vista32 and Vista64 are returning the same version numbers ( 6.0 )
        // Need to check for something else to differentiate
        HKEY hKey;
        LONG lRet;
        lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
               L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\NtVdm64",
               0, KEY_QUERY_VALUE, &hKey );

        if( lRet == ERROR_SUCCESS )
        {
            RegCloseKey( hKey );
            return OS_VISTA64;
        }
        else
    	    return  OS_VISTA32;  
	}
	
    return OS_UNSUPPORTED;
}


typedef BOOL (WINAPI *LPFN_WOW64DISABLEWOW64FSREDIRECTION) (PVOID *);
typedef BOOL (WINAPI *LPFN_WOW64REVERTWOW64FSREDIRECTION) (PVOID);

BOOL CStMfgToolApp::IsRecoveryDriverInstalled()
{
    TCHAR szWinFolder[32];
    CString szDriverPath;
    BOOL returnStatus = FALSE;
    BOOL Isx64;
	LPFN_WOW64DISABLEWOW64FSREDIRECTION fnWow64DisableWow64FsRedirection;
	LPFN_WOW64REVERTWOW64FSREDIRECTION fnWow64RevertWow64FsRedirection;
    PVOID x64BitRedirector = NULL;

	if (!GetWindowsDirectory (szWinFolder, 32))
		return FALSE;

    if (GetPlatform() == OS_VISTA64 ||
        GetPlatform() == OS_XP64)
        Isx64 = TRUE;
    else
        Isx64 = FALSE;

	if ( Isx64 )
	{
		fnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(
				GetModuleHandle(L"kernel32"),"Wow64DisableWow64FsRedirection");
		fnWow64RevertWow64FsRedirection = (LPFN_WOW64REVERTWOW64FSREDIRECTION)GetProcAddress(
				GetModuleHandle(L"kernel32"),"Wow64RevertWow64FsRedirection");

		if (fnWow64DisableWow64FsRedirection && fnWow64RevertWow64FsRedirection)
		{
		   	fnWow64DisableWow64FsRedirection(&x64BitRedirector);
		}
	}

	if ( Isx64 )
	    szDriverPath.Format(L"%s%s", szWinFolder, L"\\System32\\Drivers\\Stmp3Recx64.sys");
	else
	    szDriverPath.Format(L"%s%s", szWinFolder, L"\\System32\\Drivers\\Stmp3Rec.sys");

	if ( PathFileExists( szDriverPath ) )
    {
	    returnStatus = TRUE;
    }

	if ( Isx64 )
	{
   	    if (fnWow64DisableWow64FsRedirection && fnWow64RevertWow64FsRedirection)
        {
	    	fnWow64RevertWow64FsRedirection(x64BitRedirector);
        }
	}
	
/*
	CString str;
	str.Format(_T("The Player Recovery Device Class drivers are %sinstalled."), returnStatus ? _T(" ") : _T("NOT "));
	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( str );
*/
	return returnStatus;
}



BOOL CStMfgToolApp::InstallRecoveryDriver()
{
        LPVOID pPtr = NULL;
        DWORD dwSize;
        WCHAR szPath[MAX_PATH];
        WCHAR szTempDir[MAX_PATH];
        BOOL Isx64, IsLocal;
        BOOL status = TRUE;

        if (GetPlatform() == OS_VISTA64 ||
            GetPlatform() == OS_XP64)
            Isx64 = TRUE;
        else
            Isx64 = FALSE;

        if (!GetWindowsDirectory(szPath, MAX_PATH))
			return FALSE;

        GetEnvironmentVariable(L"TEMP", szTempDir, MAX_PATH);

        if (!Isx64)
        {
//            wcscat_s(szPath, MAX_PATH, L"\\System32\\Drivers\\stmp3rec.sys");
            wsprintf(szPath, L"%s\\%s", szTempDir, L"StMp3Rec.sys");
            pPtr = StLoadStmp3RecResource(IDR_STMP3REC_SYS, &dwSize, IsLocal);
            if (pPtr)
                StWriteStmp3RecResource(pPtr, dwSize, szPath);

			if( IsLocal ) // For resources loaded for local folder we must free the memory
				free( pPtr );

            wsprintf(szPath, L"%s\\%s", szTempDir, L"StMp3Rec.cat");

            pPtr = StLoadStmp3RecResource(IDR_STMP3REC_CAT, &dwSize, IsLocal);
            if (pPtr)
                StWriteStmp3RecResource(pPtr, dwSize, szPath);

			if( IsLocal )
				free( pPtr );

            wsprintf(szPath, L"%s\\%s", szTempDir, L"stmp3rec.inf");

            pPtr = StLoadStmp3RecResource(IDR_STMP3REC_INF, &dwSize, IsLocal);
        }
        else
        {
/*
            PVOID x64BitRedirector = NULL;
    		LPFN_WOW64DISABLEWOW64FSREDIRECTION fnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(
				GetModuleHandle(L"kernel32"),"Wow64DisableWow64FsRedirection");
    		LPFN_WOW64REVERTWOW64FSREDIRECTION fnWow64RevertWow64FsRedirection = (LPFN_WOW64REVERTWOW64FSREDIRECTION)GetProcAddress(
				GetModuleHandle(L"kernel32"),"Wow64RevertWow64FsRedirection");

    	    if (fnWow64DisableWow64FsRedirection && fnWow64RevertWow64FsRedirection)
	    	{
		    	fnWow64DisableWow64FsRedirection(&x64BitRedirector);
            }
*/
            GetEnvironmentVariable(L"TEMP", szTempDir, MAX_PATH);

//            wcscat_s(szPath, MAX_PATH, L"\\System32\\Drivers\\stmp3recx64.sys");
            wsprintf(szPath, L"%s\\%s", szTempDir, L"StMp3Recx64.sys");

            pPtr = StLoadStmp3RecResource(IDR_STMP3RECX64_SYS, &dwSize, IsLocal);
            if (pPtr)
                StWriteStmp3RecResource(pPtr, dwSize, szPath);

			if( IsLocal )
				free( pPtr );

/*   	    if (fnWow64DisableWow64FsRedirection && fnWow64RevertWow64FsRedirection)
            {
		    	fnWow64RevertWow64FsRedirection(x64BitRedirector);
            }
*/
            wsprintf(szPath, L"%s\\%s", szTempDir, L"StMp3Recx64.cat");

            pPtr = StLoadStmp3RecResource(IDR_STMP3RECX64_CAT, &dwSize, IsLocal);
            if (pPtr)
                StWriteStmp3RecResource(pPtr, dwSize, szPath);

			if( IsLocal )
				free( pPtr );

            wsprintf(szPath, L"%s\\%s", szTempDir, L"stmp3recx64.inf");

            pPtr = StLoadStmp3RecResource(IDR_STMP3RECX64_INF, &dwSize, IsLocal);

        }

        if (pPtr)
        {
            if (StWriteStmp3RecResource(pPtr, dwSize, szPath))
              	if (!SetupCopyOEMInf( szPath, szTempDir, SPOST_PATH,
                                 SP_COPY_NOOVERWRITE,
                       			0,0,0,0))
                    status = FALSE;
        }

		if( IsLocal ) // free the buffer for the .inf
			free( pPtr );

		// The new hardware wizard should now be able to load it when
        // we reset to recovery.

        return status;
}




LPVOID CStMfgToolApp::StLoadStmp3RecResource(USHORT resID, PDWORD pdwSize, BOOL& isLocal)
{
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;

	isLocal = FALSE;

	// get resource name
	CString csFname;
	PVOID fileBuf;
	HANDLE hFile;
	LARGE_INTEGER fileSize;

	switch ( resID )
	{
	case IDR_STMP3REC_SYS:
		csFname = L"stmp3rec.sys";
		break;
	case IDR_STMP3REC_INF:
		csFname = L"stmp3rec.inf";
		break;
	case IDR_STMP3REC_CAT:
		csFname = L"stmp3rec.cat";
		break;
	case IDR_STMP3RECX64_SYS:
		csFname = L"stmp3recx64.sys";
		break;
	case IDR_STMP3RECX64_INF:
		csFname = L"stmp3recx64.inf";
		break;
	case IDR_STMP3RECX64_CAT:
		csFname = L"stmp3recx64.cat";
		break;
	}

	hFile = CreateFile (
		 			  csFname,
			    		GENERIC_READ,
						FILE_SHARE_READ ,
						NULL, // no SECURITY_ATTRIBUTES structure
					    OPEN_EXISTING, // No special create flags
						0, // No special attributes
						NULL); // No template file		

	if ( hFile != INVALID_HANDLE_VALUE )
	{
		GetFileSizeEx( hFile, &fileSize );
		fileBuf = malloc(fileSize.LowPart);
		if ( fileBuf )
		{
			DWORD dwBytesRead;
			BOOL ret;
			ret = ReadFile( hFile, fileBuf, fileSize.LowPart, &dwBytesRead, FALSE);
			if ( ret && dwBytesRead == fileSize.LowPart )
			{
				CloseHandle(hFile);
				*pdwSize = fileSize.LowPart;
				isLocal = TRUE;
				return fileBuf;
			}

			free( fileBuf );
		}
		CloseHandle( hFile );
	}

	hResInfo = FindResourceEx( NULL,
						L_STMP_RECV_RESTYPE,
						MAKEINTRESOURCE(resID),
						MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
	if ( hResInfo )
    {
		hRes = LoadResource(NULL, hResInfo);
		if ( hRes )
			pPtr = LockResource(hRes);

        *pdwSize = SizeofResource(NULL, hResInfo);
    }

    return pPtr;
}

BOOL CStMfgToolApp::StWriteStmp3RecResource(LPVOID _pPtr, ULONG _dwSize, LPTSTR _pathName)
{
    BOOL returnStatus = FALSE;
    HANDLE hFile;


    hFile = ::CreateFile(
            _pathName,						    // file name
            GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                               // lpSecurityAttributes
            CREATE_ALWAYS,                      // dwCreationDistribution
            FILE_ATTRIBUTE_NORMAL,              // dwFlagsAndAttributes
            NULL                                // hTemplateFile
    );

	if( hFile != INVALID_HANDLE_VALUE )
    {
        DWORD dwBytesWritten;

        WriteFile(hFile, _pPtr, _dwSize, &dwBytesWritten, NULL);

        CloseHandle (hFile);

        if (dwBytesWritten == _dwSize)
            returnStatus = TRUE;
    }

    return returnStatus;
}

/*clw
BOOL CStMfgToolApp::IsMTPSupported()
{
    BOOL bReturn = FALSE;
    OSVERSIONINFOEX osvi = {0};
    
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx((OSVERSIONINFO*)&osvi);
	
	if ( //(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) ||  // 2K
         (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) ||  // XP
         (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 2) )   // XP64
    {
        HKEY hMediaPlayerKey;
        TCHAR *KeyName = L"Software\\Microsoft\\MediaPlayer\\Setup\\Installed Versions";

	    if (RegOpenKey (HKEY_LOCAL_MACHINE, KeyName,
                    &hMediaPlayerKey) == ERROR_SUCCESS)
        {
            ULONG typeValue = REG_BINARY;
            USHORT regValues[4] = {0,0,0,0};
            ULONG DataSize = 8;
            TCHAR *ValueName = L"wmplayer.exe";

            RegQueryValueEx( hMediaPlayerKey,				// handle to key to query
				        ValueName,		// name of value to query
				        NULL,				// reserved
				        &typeValue,			// address of buffer for value type
				        (PUCHAR) regValues,	    // address of data buffer
				        &DataSize			// address of data buffer size
			    	    );

            if (regValues[1] >= 10)
                bReturn = TRUE;

            RegCloseKey(hMediaPlayerKey);
        }
    }
    return bReturn;
}
*///clw

