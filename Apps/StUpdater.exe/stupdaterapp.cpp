// StUpdaterApp.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"

#include "StHeader.h"
#include "StGlobals.h"
#include "ddildl_defs.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "StUpdater.h"
#include "StConfigInfo.h"
#include "StGlobals.h"
//#include <winioctl.h>
#include "StScsi.h"
#include <ntddscsi.h>
//#include <ntdddisk.h>

//#include <scsidefs.h>
//#include <wnaspi32.h>

#include "StScsi_Nt.h"
#include "StFwComponent.h"
#include "StSystemDrive.h"
#include "StDataDrive.h"
#include "StProgress.h"
#include "StHiddenDataDrive.h"
#include "StUsbMscDev.h"
#include "StRecoveryDev.h"
#include "StResource.h"
#include "StMessageDlg.h"
#include "ddildl_defs.h"
#include "StConfigInfo.h"
#include "StCmdLineProcessor.h"
#include "StLogger.h"
#include "StUpdaterApp.h"
#include "StUpdaterDlg.h"
#include <lmcons.h>
#include "StDdiApi.h"
#include "..\\..\\common\\updater_res.h"
#include "..\\..\\common\\updater_restypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CStGlobals g_globals;
static int resetForRecoveryDone = 0;

/////////////////////////////////////////////////////////////////////////////
// CStUpdaterApp

BEGIN_MESSAGE_MAP(CStUpdaterApp, CWinApp)
	//{{AFX_MSG_MAP(CStUpdaterApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStUpdaterApp construction & destruction

CStUpdaterApp::CStUpdaterApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_status_dlg_cancelled = FALSE;
	m_p_resource = NULL;
	m_p_config_info = NULL;
	m_p_cmdline_processor = NULL;
	m_p_logger = NULL;
	m_p_updater = NULL;
	m_p_unsafe_window_closer_thread = NULL;
}

CStUpdaterApp::~CStUpdaterApp()
{
	if( m_p_updater )
		delete m_p_updater;
	if( m_p_logger ) 
		delete m_p_logger;
	if( m_p_cmdline_processor )
		delete m_p_cmdline_processor;
	if( m_p_resource )
		delete m_p_resource;
	if( m_p_config_info )
		delete m_p_config_info;


	// stop the thread that closes the Win 2k 'unsafe removal' dialog
	if (m_p_unsafe_window_closer_thread) 
	{
		VERIFY(SetEvent(m_hEventKill));
		DWORD thread_id = m_p_unsafe_window_closer_thread->m_nThreadID;
		DWORD ret = WaitForSingleObject(m_p_unsafe_window_closer_thread->m_hThread, 10000);
		CloseHandle(m_hEventKill);
		ATLTRACE(_T("== Killed UnsafeWindowCloserThreadProc %s. (%#x, %d)\n"), ret?_T("ERROR"):_T("successfully"), thread_id, thread_id);
	}
}

	/////////////////////////////////////////////////////////////////////////////
// The one and only CStUpdaterApp object

CStUpdaterApp theApp;


/////////////////////////////////////////////////////////////////////////////
// CStUpdaterApp initialization

int g_Status = 0;  // set to non-zero on update error
int CStUpdaterApp::ExitInstance() 
{
	CWinApp::ExitInstance();

	return g_Status;
}

/////////////////////////////////////////////////////////////////////////////
// CStUpdaterApp initialization

BOOL CStUpdaterApp::InitInstance()
{
//	_CrtSetBreakAlloc (1132);
    HRESULT hr;
    PLATFORM osPlatform = CStGlobals::GetPlatform();

	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();
//	AfxEnableControlContainer();


	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	ST_ERROR		err = STERR_NONE;

	m_p_config_info = new CStConfigInfo;

	m_p_updater	= new CStUpdater(m_p_config_info);

	SetDefaultLanguage();
	
	m_p_resource = new CStResource(m_p_config_info, m_default_language);

	if( m_p_resource->GetLastError() == STERR_FAILED_TO_LOAD_RESOURCE_DLL )
	{
		HandleError( STERR_FAILED_TO_LOAD_RESOURCE_DLL, 0 );
		return FALSE;
	}

	// YIPPEE!!!  YIPPEE!!!  W98 is dead!!!
	if ( osPlatform == OS_UNSUPPORTED )
	{
		HandleError( STERR_UNSUPPORTED_OPERATING_SYSTEM, 0 );
		return FALSE;
	}

	m_p_config_info->SetResource( m_p_resource );

	m_p_updater->SetResource( m_p_resource );

	m_p_cmdline_processor = new CStCmdLineProcessor( m_p_config_info, this->m_lpCmdLine);
    if( m_p_cmdline_processor->GetLastError() != STERR_NONE )
    {
		HandleError( m_p_cmdline_processor->GetLastError(), 0 );
        return FALSE;
    }

	m_p_updater->SetCmdLineProcessor( m_p_cmdline_processor );

	m_p_logger = new CStLogger( m_p_cmdline_processor->GetLogFilename(), m_p_cmdline_processor->Log() );
	m_p_updater->SetLogger( m_p_logger );

	CString logTxt;
	logTxt.Format(_T("%s, Version: %s"), m_p_resource->GetTitle(), m_p_resource->GetVersionString());
	m_p_logger->Log(logTxt);
	m_p_logger->Log(m_p_config_info->DriveArrayToString());

	if( m_p_cmdline_processor->WantedCommandLineHelp() )
	{
		CStMessageDlg::DisplayMessage(MSG_TYPE_INFO, m_p_cmdline_processor->GetHelpText());

		return FALSE;
	}

	if( m_p_cmdline_processor->ForceEnglish() )
	{
		LANGID log_language_Id = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);

		if( m_default_language != log_language_Id )
		{
			m_p_resource->SetLoggingTo( log_language_Id );
			if( m_p_resource->GetLastError() == STERR_FAILED_TO_LOAD_RESOURCE_DLL )
			{
				HandleError( STERR_FAILED_TO_LOAD_RESOURCE_DLL, 0 );
				return FALSE;
			}
		}
	}

	if( osPlatform == OS_2K || osPlatform == OS_XP || 
        osPlatform == OS_XP64 || osPlatform == OS_VISTA32 || osPlatform == OS_VISTA64 || osPlatform == OS_WINDOWS7)
	{
		if ( !IsUserAdmin() )
		{
			HandleError(STERR_NO_ADMINISTRATOR, 0);
			return FALSE;
		}
	}

	err = CheckForAnotherInstanceRunning( m_p_config_info );
	if( err != STERR_NONE )
	{
		if( err == STERR_ANOTHER_INSTANCE_RUNNING )
		{
			if(	BringTheRunningAppToFront() )
				return FALSE;
			else 
				err = STERR_FAILED_TO_BRING_THE_RUNNING_APPLICATION_TO_FOREGROUND;
		}
		HandleError( err, 0 );
		return FALSE;
	}

	// start the thread that watches for unsafe removal dialogs and closes them
	if( osPlatform == OS_2K || osPlatform == OS_ME )
    {
		m_hEventKill = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_p_unsafe_window_closer_thread = AfxBeginThread( UnsafeWindowCloserThreadProc, this, THREAD_PRIORITY_NORMAL, 0, 0 );
	}


//    hr = ::OleInitialize(NULL);
	hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    if (hr != S_OK)
    {
        TCHAR szMsg[64];
        swprintf_s(szMsg, 64, _T("CoInitializeEx failed hr: %x\n"), hr);
        ATLTRACE(szMsg);
    }

	// reject autoplay for MSC
	if( osPlatform == OS_2K || osPlatform == OS_XP || 
        osPlatform == OS_XP64 || osPlatform == OS_VISTA32 || osPlatform == OS_VISTA64 || osPlatform == OS_WINDOWS7  )
		AutoPlayReject.Register(NULL);


	//
    // Create the dialog to:
    //      search for devices
    //      reset device to recovery mode if needed
    //      recover the device

		SetDefaultDialog();

		switch (m_DefaultDialog)
		{
			case DLG_MINIMAL:
				m_dlg = new CStUpdaterDlg(m_p_updater, (USHORT)0, (CWnd*) 0);
				break;
			case DLG_STANDARD:
				m_dlg = new CStUpdaterDlg(m_p_updater, (USHORT)0, (SHORT)0, (CWnd*)0);
				break;
			case DLG_ADVANCED:
	            m_dlg = new CStUpdaterDlg(m_p_updater); 
				break;
		}


        PumpMessages();

        m_pMainWnd = m_dlg;				
		
        m_p_updater->SetProgress( m_dlg );			

        if( GetCmdLineProcessor()->AutoStart() )
        {
			switch (m_DefaultDialog)
			{
//::MessageBox(NULL, L"Calling dialog...", L"test", MB_OK);
			case DLG_MINIMAL:
				m_dlg->Create(IDD_MIN_STUPDATEDLG);
				break;
			case DLG_STANDARD:
	            m_dlg->Create(IDD_STD_STUPDATERDLG);
				break;
			case DLG_ADVANCED:
	            m_dlg->Create(IDD_ADV_STUPDATERDLG);
				break;
			}

			if( GetCmdLineProcessor()->SilentOperation() )
				m_dlg->ShowWindow(SW_HIDE);
			else
				m_dlg->ShowWindow(SW_SHOW);

    	    m_dlg->OnAutoStart();
	        m_dlg->SetModal(FALSE);
	        while(PumpMessages());
        }
        else
        {
        	m_dlg->SetModal(TRUE);
	        m_dlg->DoModal();
        }

		delete m_dlg;

	    if( err == STERR_FAILED_TO_OPEN_FILE )
    	{
	    	HandleError( err, 0 );
		    m_status_dlg_cancelled = TRUE;
    	}




    // Uninitialize ole here and not in the destructor.
    // That can be problematic causing the exit to hang occasionally.
    ::CoFreeUnusedLibraries();
    ::CoFreeAllLibraries();
//    ::OleUninitialize();
	CoUninitialize();
    PumpMessages();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.

	return FALSE;
}

void CStUpdaterApp::OnStatusDlgCancelled()
{
	m_status_dlg_cancelled = TRUE;
}

BOOL CStUpdaterApp::PumpMessages()
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
void CStUpdaterApp::HandleError( ST_ERROR _err, CWnd* _parent, BOOL _log_only )
{
	CString err_msg(""), err_msg_details(""), err_msg_to_log("");
	MESSAGE_TYPE msg_type = MSG_TYPE_ERROR;
	BOOL noshow = FALSE;

	if( !_log_only && !GetCmdLineProcessor()->SilentOperation() )
	{
		GetErrMsg( _err, err_msg, err_msg_details );
		
		switch ( _err )
		{
			case STERR_MEDIA_STATE_UNINITIALIZED:
			case STERR_DEVICE_STATE_UNINITALIZED:
				// don't show the "Media state uninitialized" dialog box
				// not user friendly
				noshow = TRUE;
				msg_type = MSG_TYPE_INFO;
				break;
			case STERR_FAILED_TO_LOCATE_SCSI_DEVICE_ON_START:
				msg_type = MSG_TYPE_WARNING;
				noshow = FALSE;
				break;
			case STERR_FAILED_TO_LOCATE_SCSI_DEVICE_ON_SHOW_VERSIONS:
//				msg_type = MSG_TYPE_WARNING;
//				noshow = FALSE;
				noshow = TRUE;
				msg_type = MSG_TYPE_INFO;
				break;
			case STERR_NONE:
				noshow = TRUE;
				break;
			default:
				msg_type = MSG_TYPE_ERROR;
				noshow = FALSE;
		}
		
		if( !noshow )
		{
			CStMessageDlg::DisplayMessage(msg_type, err_msg, err_msg_details, _parent);
		}
	}

	GetErrMsg( _err, err_msg, err_msg_details, TRUE /*logging only*/ );

	if( err_msg_details.GetLength() > 0 )
	{
		GetLogger()->Log( err_msg + "\n" + err_msg_details );
	}
	else
	{
		GetLogger()->Log( err_msg );
	}
}

void CStUpdaterApp::GetErrMsg( ST_ERROR _err, CString& _err_msg, CString& _err_msg_ex, BOOL _logging_only )
{
	CStResource * p_resource = GetResource();

	switch( _err )
	{
	case STERR_FAILED_TO_OPEN_FILE:
		{
			p_resource->GetErrorMessage( _err, _err_msg, GetUpdater()->GetErrorObject()->GetDriveIndex(), _logging_only );

			break;
		}
	case STERR_FAILED_TO_SEND_SCSI_COMMAND:
		{
			p_resource->GetErrorMessage(_err, _err_msg, _logging_only);
			p_resource->GetResourceString(IDS_LOGGING_SCSI_SENSE_DATA, _err_msg_ex, _logging_only);
			_err_msg_ex += CString( GetUpdater()->GetErrorObject()->GetMoreErrorInformation().c_str() );
			break;
		}
	case STERR_ERROR_IN_CFGMGR32_API:
		{
			p_resource->GetResourceString(IDS_STERR_UNABLE_TO_REFRESH_DEVICE, _err_msg, _logging_only);
			p_resource->GetErrorMessage(_err, _err_msg_ex, _logging_only);
			_err_msg_ex += CString( GetUpdater()->GetErrorObject()->GetMoreErrorInformation().c_str() );

			break;
		}
	case STERR_ERROR_IN_SETUPAPI_API:
		{
			p_resource->GetResourceString(IDS_STERR_UNABLE_TO_REFRESH_DEVICE, _err_msg, _logging_only);
			p_resource->GetResourceString(IDS_LOGGING_LAST_SYSTEM_ERROR, _err_msg_ex, _logging_only);

			break;
		}
	case STERR_FAILED_TO_LOAD_STRING:
	case STERR_FAILED_TO_LOAD_ICON:
	case STERR_FAILED_TO_LOAD_CFGMGR32_LIB:
	case STERR_MISSING_API_IN_CFGMGR32_LIB:
	case STERR_FAILED_TO_LOAD_SETUPAPI_LIB:
	case STERR_MISSING_API_IN_SETUPAPI_LIB:
	case STERR_FAILED_TO_GET_DEVICE_INFO_SET:
	case STERR_FAILED_GET_DEVICE_REGISTRY_PROPERTY:
	case STERR_FAILED_TO_DELETE_SETTINGS_DOT_DAT_FILE:
	case STERR_FAILED_TO_WRITE_SECTOR:
		{
			p_resource->GetErrorMessage(_err, _err_msg, _logging_only);
			p_resource->GetResourceString(IDS_LOGGING_LAST_SYSTEM_ERROR, _err_msg_ex, _logging_only);

			break;
		}
	case STERR_FAILED_DEVICE_IO_CONTROL:
		{
			p_resource->GetResourceString(IDS_LOGGING_LAST_SYSTEM_ERROR, _err_msg, _logging_only);

			break;
		}
	case STERR_FAILED_TO_LOAD_RESOURCE_DLL: // resource dll is absent, just display the system last error msg.
		{
			CString msg;
			p_resource->GetResourceString(IDS_UNABLE_TO_LOAD_RESOURCE_DLL, msg, _logging_only);
			_err_msg.Format( msg, GetUpdater()->GetErrorObject()->GetMoreErrorInformation().c_str() );
			p_resource->GetResourceString(IDS_LOGGING_LAST_SYSTEM_ERROR, _err_msg_ex, _logging_only);

			break;
		}
	default:
		{
			if( p_resource->GetErrorMessage(_err, _err_msg) == STERR_ERROR_OUT_OF_RANGE )
			{
				p_resource->GetResourceString(IDS_ERR_UNDEFINED, _err_msg, _logging_only);
			}

			break;
		}
	}
}

ST_ERROR CStUpdaterApp::CheckForAnotherInstanceRunning(CStConfigInfo* _p_config_info)
{
	CString MutexName;
	wstring app_name;
	wstring scsi_ven_str, scsi_prod_str;

	// Prevent multiple instances of the upgrader and the formatter
	_p_config_info->ExecutableName(app_name);
	_p_config_info->GetSCSIMfgString(scsi_ven_str);
	_p_config_info->GetSCSIProductString(scsi_prod_str);
	MutexName = CString(CString(app_name.c_str()) + CString(scsi_ven_str.c_str()) + CString(scsi_prod_str.c_str()));
	if( !CreateMutex(
			NULL,
			FALSE,  
			MutexName
		) )
	{
		return STERR_FAILED_TO_CREATE_MUTEX_OBJECT;
	}

	if( CStGlobals::GetLastError() == ERROR_ALREADY_EXISTS )
	{
		return STERR_ANOTHER_INSTANCE_RUNNING;
	}

	//
	// return STERR_NONE if no instance is running,
	//
	return STERR_NONE;
}

BOOL CStUpdaterApp::BringTheRunningAppToFront()
{
	CString title1, title2, title3;
	CWnd *pWndPrev, *pWndChild;

	title1 = GetResource()->GetTitle();
	GetResource()->GetResourceString(IDS_CAPTION_INITIALIZING, title2);
	GetResource()->GetResourceString(IDS_CAPTION_RECOVERY, title3);

	// Determine if a window with the class name exists...
	pWndPrev = CWnd::FindWindow(NULL, title1);
	if( pWndPrev == NULL )
	{
		pWndPrev = CWnd::FindWindow(NULL, title2);
	}
	if( pWndPrev == NULL )
	{
		pWndPrev = CWnd::FindWindow(NULL, title3);
	}

	if( pWndPrev == NULL )
		return FALSE;
	
	// If so, does it have any popups?
	pWndChild = pWndPrev->GetLastActivePopup();

	// If iconic, restore the main window
	if (pWndPrev->IsIconic())
		pWndPrev->ShowWindow(SW_RESTORE);

	// Bring the main window or its popup to the foreground
	pWndChild->SetForegroundWindow();

	// and you are done activating the other application
	return TRUE;
}




BOOL CStUpdaterApp::IsUserAdmin(void)
/*++ 
Routine Description: This routine returns TRUE if the caller's
process is a member of the Administrators local group. Caller is NOT
expected to be impersonating anyone and is expected to be able to
open its own process and process token. 
Arguments: None. 
Return Value: 
   TRUE - Caller has Administrators local group. 
   FALSE - Caller does not have Administrators local group. --

Taken from MS VS2008 documentation.
*/ 
{
BOOL b;
SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
PSID AdministratorsGroup; 
b = AllocateAndInitializeSid(
    &NtAuthority,
    2,
    SECURITY_BUILTIN_DOMAIN_RID,
    DOMAIN_ALIAS_RID_ADMINS,
    0, 0, 0, 0, 0, 0,
    &AdministratorsGroup); 
if(b) 
{
    if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) 
    {
         b = FALSE;
    } 
    FreeSid(AdministratorsGroup); 
}

return(b);
}


void CStUpdaterApp::SetDefaultLanguage()
{
	m_default_language = GetUserDefaultLangID();

	/*
	switch( PRIMARYLANGID( m_default_language ) )
	{
	case LANG_ENGLISH:
	case LANG_CHINESE:
	case LANG_JAPANESE:
	case LANG_KOREAN:
	case LANG_GERMAN:
	case LANG_SPANISH:
	case LANG_FRENCH:
    case LANG_PORTUGUESE:
		break;
	default:
		m_default_language = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
	}
	*/
}

UINT CStUpdaterApp::UnsafeWindowCloserThreadProc( LPVOID pParam )
	{
	CStUpdaterApp * p_updaterapp_class = (CStUpdaterApp*)pParam;
	CWnd* wnd = NULL;
	ATLTRACE(_T("== UnsafeWindowCloserThreadProc (%#x, %d)\n"), p_updaterapp_class->m_p_unsafe_window_closer_thread->m_nThreadID, p_updaterapp_class->m_p_unsafe_window_closer_thread->m_nThreadID);
	
//	SetThreadName ((DWORD)-1, "UnsafeWindowCloserThread");

	// get the localized title of the "Unsafe Removal of Device" dialog 
	// so we can close it.
	CString title = GetDlgCaption(_T("hotplug.dll"), 330);
	if(!title.IsEmpty())
	{	
		while( WaitForSingleObject(p_updaterapp_class->m_hEventKill, 0) == WAIT_TIMEOUT) {
			wnd = CWnd::FindWindow(NULL, title);
			if( wnd ) {
				wnd->SendMessage( WM_CLOSE );
				ATLTRACE("== Closed an Unsafe Removal window.\n");
				wnd = NULL;
			}
			Sleep(25);
		}
	}
	
	return 0;
}

CString CStUpdaterApp::GetDlgCaption(LPCTSTR _module, UINT _id)
{
	LPVOID lpResource = NULL;
	HGLOBAL hgResource = NULL;
	CString strDlgTitle(_T(""));

	// Load the module
	HMODULE hDll = LoadLibrary(_module);
	if (hDll == NULL)
	{
		goto CleanExit;
	}

	// Find resource handle
	HRSRC hrDlg = FindResource(hDll, MAKEINTRESOURCE(_id), RT_DIALOG);
	if (hrDlg == NULL)
	{
		goto CleanExit;
	}

	// Load it
	hgResource = LoadResource(hDll, hrDlg);
	if (hgResource == NULL)
	{
		goto CleanExit;
	}

	// Lock it
	lpResource = LockResource(hgResource);
	if (lpResource == NULL)
	{
		goto CleanExit;
	}	

	// The dialog's menu is the first
	// item after the dialog template (1.)
	WORD * wPointer = (WORD *)((PBYTE)lpResource + sizeof(DLGTEMPLATE));

	if ((*wPointer) == 0x0000)
	{
		// No menu resource
		wPointer++;
	}
	else
	{
		if ((*wPointer) == 0xffff)
		{
			// There's one other element which specifies
			// the ordinal value of the menu
			wPointer++;
//			WORD wMenuOrd = (*wPointer);
			wPointer++;
		}
		else
		{
			// It's a string specifying the
			// name of a menu resource
			CString strMenuResource ((LPCWSTR)wPointer);
			while ((*wPointer++) != 0x0000);
		}
	}

	// Next comes the window class
	if ((*wPointer) == 0x0000)
	{
		// The system uses the predefined dialog
		// box class for the dialog box
		wPointer++;
	}
	else
	{
		if ((*wPointer) == 0xffff)
		{
			// One other element which specifies the
			// ordinal value of a predefined system window class
			wPointer++;
//			WORD wClassOrd = (*wPointer);
			wPointer++;
		}
		else
		{
			// It's a string specifying the name of a registered window class
			CString strWndClass ((LPCWSTR)wPointer);
			while ((*wPointer++) != 0x0000);
		}
	}

	// Now comes the title of the dialog box
	if ((*wPointer) == 0x0000)
	{
		// There is no title
		wPointer++;
	}
	else
	{
		strDlgTitle = ((LPCWSTR)wPointer);
		while ((*wPointer++) != 0x0000);
	}
	
	// Cleanup
CleanExit:
	if (hgResource)
	{
		UnlockResource(hgResource);
		FreeResource(hgResource);
	}
	if (hDll)
		FreeLibrary(hDll);
	
	return strDlgTitle;
}


//
// SetDefaultDialog()
//
// Sets the dialog to be used.  First sets the default build value form product.h.
// Checks for the resource specification made by the binder application.
// Checks for a command line override.
//
void CStUpdaterApp::SetDefaultDialog()
{
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;

	m_DefaultDialog = m_p_config_info->GetDefaultUserDialog();

   	hResInfo = FindResourceEx( NULL,
    					L_STMP_RESINFO_TYPE,
	    				MAKEINTRESOURCE(IDR_DEFAULT_DIALOG),
		    			MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
   	if ( hResInfo )
    {
	    hRes = LoadResource(NULL, hResInfo);
    	if ( hRes )
	   		pPtr = LockResource(hRes);
       	if ( pPtr )
		{
	       	m_DefaultDialog = *((USER_DLG_TYPE *)pPtr);
			if ( m_DefaultDialog == DLG_DEFAULT )
				m_DefaultDialog = m_p_config_info->GetDefaultUserDialog();
		}
    }

	if ( GetCmdLineProcessor()->MinimalDialog() )
		m_DefaultDialog = DLG_MINIMAL;

	if ( GetCmdLineProcessor()->StandardDialog() )
		m_DefaultDialog = DLG_STANDARD;

    if( GetCmdLineProcessor()->AdvancedDialog() )
		m_DefaultDialog = DLG_ADVANCED;
}

void SetThreadName( DWORD dwThreadID, char* threadName)
{
   Sleep(10);
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = threadName;
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try
   {
      RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }
}
