// StUpdaterDlg.cpp : implementation file
//

#include "stdafx.h"
#include <shlwapi.h>
#include "StHeader.h"
#include "StGlobals.h"
#include "ddildl_defs.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "StProgress.h"
#include "StUpdater.h"
#include "StConfigInfo.h"
#include "StCmdlineProcessor.h"
#include "StLogger.h"
#include <winioctl.h>
#include "StScsi.h"
#include "StError.h"
#include <ntddscsi.h>
//#include <ntdddisk.h>

//#include <scsidefs.h>
//#include <wnaspi32.h>
#include <dbt.h>
#include <winnls.h>
#include "StScsi_Nt.h"
#include "StFwComponent.h"
#include "StSystemDrive.h"
#include "StDataDrive.h"
#include "StHiddenDataDrive.h"
#include "StUsbMscDev.h"
#include "stddiapi.h"
#include "StRecoveryDev.h"
#include "StResource.h"
#include "StUpdaterApp.h"
#include "StDriveRefresher.h"
#include "StUpdaterDlg.h"
#include "StFwVersionDlg.h"
#include "StMessageDlg.h"
#include "CustomSupport.h"
#include "StDeviceInfo.h"
#include "StNandMediaInfo.h"
#include ".\stupdaterdlg.h"
#include "..\\..\\Libs\\WinSupport\\AutoPlayReject.h"
#include "..\\..\\Libs\\WinSupport\\StSplashWnd.h"
#include "..\\..\\Libs\\WinSupport\\ColorStaticST.h"


#include "..\\..\\common\\updater_res.h"
#include "..\\..\\common\\updater_restypes.h"

#include "shlobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WAIT_FOR_EVENT_TIME				1000
#define PROGRESS_OVERALL_TIMER			3000
#define PROGRESS_OVERALL_RANGE			16 //20
#define PROGRESS_OVERALL_RANGE_FAT		32
#define PROGRESS_OVERALL_RANGE_FAT32	37
#define PROGRESS_TASK_RANGE				10
#define VOLUME_LABEL_LENGTH				11

#define FAT_12 0
#define FAT_16 1
#define FAT_32 2

#ifdef ONE_MB
#define ONE_MB  (1024*1024)
#endif
#ifndef _256_MB
#define _256_MB   (256*ONE_MB)
#endif
#ifndef ONE_GB
#define ONE_GB ((ULONG)(1024*ONE_MB))
#endif
#ifndef TWO_GB
#define TWO_GB (2*ONE_GB)
#endif

extern CStGlobals g_globals;
extern int g_Status;



CAboutDlg::CAboutDlg(CStConfigInfo* _p_config, CWnd* _pParent /*=NULL*/) : CDialog(CAboutDlg::IDD, _pParent)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
	m_p_config_info = _p_config;
}



void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	wstring wstr;
	HICON h_icon=NULL;
	LANGID language = ((CStUpdaterApp*)AfxGetApp())->GetLangId();

	m_p_config_info->GetAboutDlgTitle( wstr, language );
	SetWindowText( wstr.c_str() );

	//set description
	m_p_config_info->ApplicationDescription( wstr, language );
	GetDlgItem(IDC_ABOUTDLG_DESC)->SetWindowText( wstr.c_str() );

	//set copyright string
	m_p_config_info->GetCopyrightString( wstr, language );
	GetDlgItem(IDC_ABOUTDLG_COPYRIGHT)->SetWindowText( wstr.c_str() );
	
	//set version
	GetDlgItem(IDC_ABOUTDLG_VERSION)->SetWindowText( 
		((CStUpdaterApp*)AfxGetApp())->GetResource()->GetAboutVersionString() );

	//set icon
	((CStUpdaterApp*)AfxGetApp())->GetResource()->LoadIcon( IDI_COMPANY_ICON, h_icon );
	((CStatic*)GetDlgItem(IDC_ABOUTDLG_COMPANY_IMAGE))->SetIcon( h_icon );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



/////////////////////////////////////////////////////////////////////////////
// CStUpdaterDlg ADVANCED dialog

CStUpdaterDlg::CStUpdaterDlg(CStUpdater* _p_updater, CWnd* _pParent /*=NULL*/)
	: CDialog(CStUpdaterDlg::IDD, _pParent)
{
	//{{AFX_DATA_INIT(CStUpdaterDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	((CStUpdaterApp*)AfxGetApp())->GetResource()->LoadIcon(IDI_UPDATER_ICON, m_hIcon);
	m_p_updater = _p_updater;
	m_pos_total_progress_bar = 0;
	m_modal = FALSE;
	m_volume_label[0] = _T('\0');
	m_thread_block = 1; // starts off blocked
    m_monitor_thread_handle = INVALID_HANDLE_VALUE;
	m_monitor_stop = INVALID_HANDLE_VALUE;
	m_monitor_trigger = INVALID_HANDLE_VALUE;
	m_task_progress_mutex = INVALID_HANDLE_VALUE;
    m_advanced_dlg = TRUE;
	m_minimal_dlg = FALSE;
	m_p_bugaboo_window_closer_thread = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CStUpdaterDlg MINIMAL dialog

CStUpdaterDlg::CStUpdaterDlg(CStUpdater* _p_updater, USHORT dummy, CWnd* _pParent /*=NULL*/)
	: CDialog(CStUpdaterDlg::IDD2, _pParent)
{
	//{{AFX_DATA_INIT(CStUpdaterDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	((CStUpdaterApp*)AfxGetApp())->GetResource()->LoadIcon(IDI_UPDATER_ICON, m_hIcon);
	m_p_updater = _p_updater;
	m_pos_total_progress_bar = 0;
	m_modal = FALSE;
	m_volume_label[0] = _T('\0');
	m_thread_block = 1; // starts off blocked
    m_monitor_thread_handle = INVALID_HANDLE_VALUE;
	m_monitor_stop = INVALID_HANDLE_VALUE;
	m_monitor_trigger = INVALID_HANDLE_VALUE;
	m_task_progress_mutex = INVALID_HANDLE_VALUE;
	m_p_bugaboo_window_closer_thread = NULL;
    m_advanced_dlg = FALSE;
	m_minimal_dlg = TRUE;
UNREFERENCED_PARAMETER(dummy);
}

/////////////////////////////////////////////////////////////////////////////
// CStUpdaterDlg STANDARD dialog

CStUpdaterDlg::CStUpdaterDlg(CStUpdater* _p_updater, USHORT dummy, SHORT dummy2, CWnd* _pParent /*=NULL*/)
	: CDialog(CStUpdaterDlg::IDD3, _pParent)
{
	//{{AFX_DATA_INIT(CStUpdaterDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	((CStUpdaterApp*)AfxGetApp())->GetResource()->LoadIcon(IDI_UPDATER_ICON, m_hIcon);
	m_p_updater = _p_updater;
	m_pos_total_progress_bar = 0;
	m_modal = FALSE;
	m_volume_label[0] = _T('\0');
	m_thread_block = 1; // starts off blocked
    m_monitor_thread_handle = INVALID_HANDLE_VALUE;
	m_monitor_stop = INVALID_HANDLE_VALUE;
	m_monitor_trigger = INVALID_HANDLE_VALUE;
	m_task_progress_mutex = INVALID_HANDLE_VALUE;
	m_p_bugaboo_window_closer_thread = NULL;
    m_advanced_dlg = FALSE;
	m_minimal_dlg = FALSE;
UNREFERENCED_PARAMETER(dummy);
UNREFERENCED_PARAMETER(dummy2);
}

CStUpdaterDlg::~CStUpdaterDlg(void)
{
}

void CStUpdaterDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CStUpdaterDlg)
	if ( !m_minimal_dlg )
	{
		DDX_Control(pDX, IDC_FORMAT_DATA_AREA, m_format_data_area_btn);
	    DDX_Control(pDX, IDC_TOTAL_PROGRESS, m_pb_overall);
	}

	if ( m_advanced_dlg )
	{
		DDX_Control(pDX, IDC_FULL_MEDIA_ERASE, m_full_media_erase_btn);
		DDX_Control(pDX, IDC_LANGUAGE_ID, m_language_list);
		DDX_Control(pDX, IDC_DNLD_FS, m_filesystem_list);
	}
    DDX_Control(pDX, IDC_PROGRESS, m_pb_task);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CStUpdaterDlg, CDialog)
	//{{AFX_MSG_MAP(CStUpdaterDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, OnStart)
	ON_BN_CLICKED(IDC_FORMAT_DATA_AREA, OnFormatDataArea)
	ON_BN_CLICKED(IDC_FULL_MEDIA_ERASE, OnFullMediaErase)
	ON_BN_CLICKED(IDOK, OnClose)
	ON_BN_CLICKED(IDC_DOWNLOAD_DETAILS, OnDownloadDetails)
	ON_BN_CLICKED(IDC_DETAILS, OnShowVersionDetails)
	ON_CBN_SELCHANGE(IDC_LANGUAGE_ID, OnCbnSelchangeLanguageId)
	ON_CBN_SELCHANGE(IDC_DNLD_FS, OnCbnSelchangeFileSystemId)
	//}}AFX_MSG_MAP
	ON_WM_DEVICECHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStUpdaterDlg message handlers

BOOL CStUpdaterDlg::PreTranslateMessage(MSG* pMsg) 
{
	// Route messages to the splash screen while it is visible
    if (CStSplashWnd::PreTranslateAppMessage(pMsg)) 
	{
		return TRUE;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CStUpdaterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CWaitCursor wait;

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
//::MessageBox( NULL, L"InitDialog...", L"test", MB_OK);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		wstring wstr_about_menu;
		LANGID language = ((CStUpdaterApp*)AfxGetApp())->GetLangId();

		((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetAboutDlgTitle( wstr_about_menu, language );
		
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, CString("&" + CString(wstr_about_menu.c_str())));
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	ClearLogDetails();

    m_stmp3rec_installed = IsRecoveryDriverInstalled();

	WhatResourcesAreBound();

    // If the recovery driver is not installed try to load it from resource
    // as it may have been bound to the executable.
    if (!m_stmp3rec_installed)
    {
        InstallRecoveryDriver();
    }

   	SetupDisplay();
//::MessageBox(NULL, L"Display done...", L"test", MB_OK);

    m_app_closing = CreateEvent( NULL, TRUE, FALSE, NULL );

	GetUpdater()->SetStopTrigger(m_app_closing);

    // Get the device in update mode if possible.  
//	if( !((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->AutoStart() )
//    {
        DWORD dwThreadId;

        // this event maintains or kills the monitor thread
        // initially set to FALSE.  Set in OnClose or OnStart
        // to kill the thread.
        m_monitor_stop = CreateEvent( NULL, TRUE, FALSE, NULL );

        // this event wakes up the thread to check connections
        // initially set TRUE to do initial check for devices
        // and is auto reset.
        m_monitor_trigger = CreateEvent( NULL, FALSE, TRUE, NULL );

    	m_monitor_thread_handle = CreateThread(
	                	NULL,                        // default security attributes 
                		0,                           // use default stack size  
                		CStUpdaterDlg::ConnectionMonitorThread,  // thread function 
                		this,						 // argument to thread function 
                		0,                           // use default creation flags 
                		&dwThreadId);  
//    }

	return TRUE;  // return TRUE  unless you set the focus to a control
}


void CStUpdaterDlg::SetupDisplay()
{
	CString str;
	BOOL format_data_area = FALSE;
	BOOL erase_media = FALSE;
	CStResource * p_resource = ((CStUpdaterApp*)AfxGetApp())->GetResource();
	LANGID fwLangId = GetUserDefaultLangID();

//::MessageBox( NULL, L"Setting up display...", L"test", MB_OK);

	//set default states for the check boxes.
	format_data_area = GetUpdater()->GetConfigInfo()->GetDefaultStateForFormatDataArea();
	erase_media = GetUpdater()->GetConfigInfo()->GetDefaultStateForEraseMedia();


	// set Title;
	
	SetWindowText(p_resource->GetTitle());

	//set other window texts.

	p_resource->GetResourceString(IDS_UPDLG_CURR_FW_VER, str);
	SetDlgItemText(IDC_TEXT_CURRENT_FIRMWARE, str);

	p_resource->GetResourceString(IDS_UPDLG_UPGRADE_FW_VER, str);
	SetDlgItemText(IDC_TEXT_UPGRADE_FIRMWARE, str);

	p_resource->GetResourceString(IDS_UPDLG_START_BTN, str);
	SetDlgItemText(IDC_START, str);
    GetDlgItem(IDC_START)->EnableWindow(FALSE);

	p_resource->GetResourceString(IDS_UPDLG_CLOSE_BTN, str);
	SetDlgItemText(IDOK, str);

	// Set waring text.
    p_resource->GetResourceString(IDS_DISCONNECT_WARNING, str);
	SetDlgItemText(IDC_WARNING, str);
    // Create the colored and blinking <Danger>
	m_warning_ctrl.SubclassDlgItem(IDC_WARNING, this);
    // Set a special font
	CFont * p_font = m_warning_ctrl.GetFont();
    LOGFONT l_font;
	p_font->GetLogFont(&l_font);
    l_font.lfHeight = -12;
	l_font.lfWeight = FW_SEMIBOLD;
    if ( m_warning_font.CreateFontIndirect(&l_font) )
	{
	    m_warning_ctrl.SetFont(&m_warning_font);// RGB(206,17,5)
    }
	// Set background blinking colors
    // Medium and light red
//	m_warning_ctrl.SetBlinkBkColors(RGB(128, 0, 0), RGB(255,0,0));
    m_warning_ctrl.SetBlinkTextColors(RGB(128,0,0), RGB(255,0,0));
//	m_warning_ctrl.SetTextColor(RGB(255,0,0)/*RGB(255,255,255)*/);

	if ( !m_minimal_dlg )
	{
    	p_resource->GetResourceString(IDS_UPDLG_FORMAT_DATA_AREA, str);
	    SetDlgItemText(IDC_FORMAT_DATA_AREA, str);

		// Download options groupbox
    	p_resource->GetResourceString(IDS_DOWNLOAD_OPTIONS_GRP, str);
	    SetDlgItemText(IDC_OPTIONS, str);

		if( format_data_area || erase_media ||
			((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->FormatDataArea() ||
			((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->EraseMedia() )
	    {
		    format_data_area = TRUE;
    		m_format_data_area_btn.SetCheck(BST_CHECKED);
		    if ( m_advanced_dlg )
				m_filesystem_list.EnableWindow(TRUE);
	    }

		if( m_advanced_dlg &&
			(erase_media ||
			((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->EraseMedia()) )
	    {
		    erase_media = TRUE;
    		m_full_media_erase_btn.SetCheck(BST_CHECKED);
	    }

		// Version info groupbox

    	p_resource->GetResourceString(IDS_FWVERDLG_TITLE, str);
	    SetDlgItemText(IDC_FW_DETAILS_GRP, str);

    	p_resource->GetResourceString(IDS_UPDLG_DETAILS_BTN, str);
	    SetDlgItemText(IDC_DETAILS, str);

	}

	if ( m_advanced_dlg )
    {
		int indexItem = 0;
		wstring wstr;
		CString csStr;
		USHORT vendorid, productid, secondary_productid;
		LANGID cmdline_langid = ((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->ForceFWLanguage();
		
    	GetDlgItem(IDC_DOWNLOAD_DETAILS)->ShowWindow(FALSE);

		ClearInfo();

		// updater details groupbox
    	p_resource->GetResourceString(IDS_UPDATER_INFO_GRP, str);
	    SetDlgItemText(IDC_UPDATER_INFO, str);

    	p_resource->GetResourceString(IDS_UPDATER_VERSION, str);
	    SetDlgItemText(IDC_TEXT_VERSION, str);

		str = ((CStUpdaterApp*)AfxGetApp())->GetResource()->GetVersionString();
		SetDlgItemText(IDC_UPDATER_VERSION, str);
		m_LogInfo.UpdaterInfoVersion = str;

    	p_resource->GetResourceString(IDS_UPDATER_BASE_SDK, str);
	    SetDlgItemText(IDC_TEXT_BASE_SDK, str);

		((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetBaseSDKString(wstr);
		SetDlgItemText(IDC_UPDATER_BASE_SDK, wstr.c_str());
		m_LogInfo.UpdaterInfoSDKBase = wstr.c_str();

		((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetUSBVendorId(vendorid);
		((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetUSBProductId(productid);
		((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetSecondaryUSBProductId(secondary_productid);
		if ( !secondary_productid )
			csStr.Format(L"%04X_%04X", vendorid, productid);
		else
			csStr.Format(L"%04X_%04X\\%04X", vendorid, productid, secondary_productid);
		SetDlgItemText(IDC_USB_VIDPID, csStr);
		m_LogInfo.UpdaterInfoVidPid = csStr;

    	p_resource->GetResourceString(IDS_UPDATER_BOUND_RES_COUNT, str);
	    SetDlgItemText(IDC_TEXT_BOUND_RES, str);

		str.Format(L"%u", m_TotalBoundResourceCount); 
		SetDlgItemText(IDC_BOUND_RES_COUNT, str);
		m_LogInfo.UpdaterInfoBoundResources = str;

		// Device details groupbox

    	p_resource->GetResourceString(IDS_DEVICE_INFO_GRP, str);
	    SetDlgItemText(IDC_DEVICE_INFO_GRP, str);

		p_resource->GetResourceString(IDS_DEVINFO_CHIPID, str);
	    SetDlgItemText(IDC_TEXT_DEVINFO_CHIPID, str);

    	p_resource->GetResourceString(IDS_DEVINFO_ROM_REVISION, str);
	    SetDlgItemText(IDC_TEXT_DEVINFO_ROM_REVISION, str);

    	p_resource->GetResourceString(IDS_DEVINFO_EXT_RAM_SIZE, str);
	    SetDlgItemText(IDC_TEXT_DEVINFO_EXT_RAM_SIZE, str);

    	p_resource->GetResourceString(IDS_DEVINFO_VRAM_SIZE, str);
	    SetDlgItemText(IDC_TEXT_DEVINFO_VRAM_SIZE, str);

    	p_resource->GetResourceString(IDS_MEDIA_SERIAL_NUM, str);
	    SetDlgItemText(IDC_TEXT_DEVINFO_SERIALNO, str);

    	p_resource->GetResourceString(IDS_DEVINFO_MODE, str);
	    SetDlgItemText(IDC_TEXT_DEVINFO_MODE, str);

    	p_resource->GetResourceString(IDS_PROTOCOL_VER, str);
	    SetDlgItemText(IDC_TEXT_PROTOCOL_VER, str);

		// Media details groupbox - All but these two are filled in by media class
    	p_resource->GetResourceString(IDS_MEDIA_DETAILS_GRP, str);
	    SetDlgItemText(IDC_MEDIA_DETAILS_GRP, str);

    	p_resource->GetResourceString(IDS_MEDIA_TYPE, str);
	    SetDlgItemText(IDC_TEXT_MEDIA_TYPE, str);

    	GetDlgItem(IDC_TEXT_MEDIA1)->ShowWindow(FALSE);
    	GetDlgItem(IDC_TEXT_MEDIA2)->ShowWindow(FALSE);
    	GetDlgItem(IDC_TEXT_MEDIA3)->ShowWindow(FALSE);
    	GetDlgItem(IDC_TEXT_MEDIA4)->ShowWindow(FALSE);
    	GetDlgItem(IDC_TEXT_MEDIA5)->ShowWindow(FALSE);
    	GetDlgItem(IDC_TEXT_MEDIA6)->ShowWindow(FALSE);

		// Data area details groupbox
    	p_resource->GetResourceString(IDS_DATA_AREA_DETAILS_GRP, str);
	    SetDlgItemText(IDC_DATA_AREA_DETAILS_GRP, str);

    	p_resource->GetResourceString(IDS_FREESPACE, str);
	    SetDlgItemText(IDC_TEXT_DATA_FREESPACE, str);

    	p_resource->GetResourceString(IDS_FILE_SYSTEM, str);
	    SetDlgItemText(IDC_TEXT_FILE_SYSTEM, str);

    	p_resource->GetResourceString(IDS_SECTOR_COUNT, str);
	    SetDlgItemText(IDC_TEXT_SECTOR_COUNT, str);

    	p_resource->GetResourceString(IDS_SECTOR_SIZE, str);
	    SetDlgItemText(IDC_TEXT_SECTOR_SIZE, str);

    	p_resource->GetResourceString(IDS_FULL_MEDIA_ERASE, str);
	    SetDlgItemText(IDC_FULL_MEDIA_ERASE, str);


    	p_resource->GetResourceString(IDS_LANGUAGE, str);
	    SetDlgItemText(IDC_LANGUAGE_TEXT, str);


		if(	((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->EraseMedia() )
	    {
    		m_full_media_erase_btn.SetCheck(BST_CHECKED);
	    }

    	p_resource->GetResourceString(IDS_FILE_SYSTEM, str);
	    SetDlgItemText(IDC_DNLD_FS_TEXT, str);

		// FAT and FAT32 on the checkbox text are fine, no translation needed

		p_resource->GetResourceString(IDS_DEFAULT, str);

		m_filesystem_list.AddString(str);
		m_filesystem_list.AddString(L"FAT");
		m_filesystem_list.AddString(L"FAT32");
		m_filesystem_list.SetItemData(0, 0);
		m_filesystem_list.SetItemData(1, 1);
		m_filesystem_list.SetItemData(2, 2);

		m_filesystem_list.SetCurSel(
						GetUpdater()->GetConfigInfo()->GetPreferredFAT() );

		if ( !m_format_data_area_btn.GetCheck() )
		{
			m_filesystem_list.EnableWindow(FALSE);
		}

    	m_pb_overall.SetRange( 0, PROGRESS_OVERALL_RANGE );
	    m_pb_overall.SetStep( 1 );

		if ( m_BoundLangResourceCount )
		{
	    	p_resource->GetResourceString(IDS_LANGUAGE, str);
		    SetDlgItemText(IDC_LANGUAGE_TEXT, str);
			int defaultSelect = 0;
			LANGID langId = 0;

			for (unsigned int i = 0; i < m_BoundLangResourceCount; ++i)
			{
				langId = *(m_pBoundIds + i);
				int iInsertIndex;

				// get the localized language name
				GetLanguageString( str, langId);

		        iInsertIndex = m_language_list.AddString(str);
				m_language_list.SetItemData(iInsertIndex, langId);

				if ( cmdline_langid )
				{
					if (langId == cmdline_langid)
					{
						defaultSelect = iInsertIndex;
					}
				}
				else
					if ( PRIMARYLANGID(langId) == PRIMARYLANGID( GetUserDefaultLangID()) )
					{
						defaultSelect = iInsertIndex;
					}

				++indexItem;
			}

			if ( langId ) // have at least one item inserted?
			{
		        m_language_list.SetCurSel(defaultSelect);
				fwLangId = (LANGID) m_language_list.GetItemData(defaultSelect);
			}

		}
		else
		{	// make the language combobox go away
	    	m_language_list.ShowWindow(FALSE);
            m_language_list.EnableWindow(FALSE);
	    	GetDlgItem(IDC_LANGUAGE_TEXT)->ShowWindow(FALSE);
		}

		//SetTextColor(::GetDC(GetDlgItem(IDC_FW_DETAILS_GRP)->GetSafeHwnd()), RGB(128,0,0));

		SetOptionsEnabled( FALSE );

    } // Advanced dialog controls

	GetUpdater()->SetLanguageId( (WORD) fwLangId );


}

void CStUpdaterDlg::CheckForDevice()
{
	ST_ERROR err;
    HANDLE hProgressThread;
    DWORD dwThreadId;


	if ( !m_minimal_dlg )
	    GetDlgItem(IDC_DETAILS)->EnableWindow(FALSE);

    // Start a thread to update the task progress bar while we initialize.

    m_progress_thread_stop = CreateEvent( NULL, FALSE, FALSE, NULL );

    hProgressThread = CreateThread(
	                	NULL,                        // default security attributes 
                		0,                           // use default stack size  
                		CStUpdaterDlg::ConnectionProgressThread,  // thread function 
                		this,						 // argument to thread function 
                		0,                           // use default creation flags 
                		&dwThreadId);  

	// start the thread that watches for auto formating of uninitialized media
	if( CStGlobals::GetPlatform() == OS_VISTA32 || CStGlobals::GetPlatform() == OS_VISTA64 ||  CStGlobals::GetPlatform() == OS_WINDOWS7 )
    {
		m_hEventKill = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_p_bugaboo_window_closer_thread = AfxBeginThread( BugabooWindowCloserThreadProc, this, THREAD_PRIORITY_NORMAL, 0, 0 );
	}

	// Initialize device interface. If it is an MTP device and FAILS the ResetToRecovery
    // command (STMP35xx) leave it alone until we actually start the download.

	err = GetUpdater()->InitializeDeviceInterface ( FALSE );

	// Kill the thread that monitors for the Vista unformatted media message.
	if (m_p_bugaboo_window_closer_thread) 
	{
		VERIFY(SetEvent(m_hEventKill));
		Sleep(0);
		if (WaitForSingleObject(m_p_bugaboo_window_closer_thread->m_hThread, 3000) == WAIT_OBJECT_0)
			CloseHandle(m_hEventKill);
		m_p_bugaboo_window_closer_thread = NULL;
	}

    SetEvent ( m_progress_thread_stop );
	Sleep(0);
	if ( WaitForSingleObject( hProgressThread, 3000 ) == WAIT_OBJECT_0)
	{
		CloseHandle( m_progress_thread_stop );
		m_progress_thread_stop = INVALID_HANDLE_VALUE;
	}

	if ( WaitForSingleObject( m_app_closing, 0) != WAIT_OBJECT_0 )
	{	// only if we are not closing the app
		if ( m_current_task != TASK_TYPE_DETECTING_DEVICE &&
			 m_current_task != TASK_TYPE_INIT_DEVICE_INTERFACE)
		{
		    SetCurrentTask( TASK_TYPE_NONE, 0 );
			m_pb_task.SetPos( 0 );
		    if ( !m_minimal_dlg )
				m_pb_overall.SetPos( 0 );
		}

	    if ( err != STERR_NONE )
			((CStUpdaterApp*) AfxGetApp())->HandleError( err, this );

		err = DisplayProjectVersion();

		if ( !GetUpdater()->IsDeviceReady() )
		{
			SetDlgItemText(IDC_CURRENT_FIRMWARE, L"");
			SetOptionsEnabled( FALSE );
			GetDlgItem(IDC_START)->EnableWindow(FALSE);
			if ( m_current_task == TASK_TYPE_DETECTING_DEVICE ||
				 m_current_task == TASK_TYPE_INIT_DEVICE_INTERFACE )
			{
				SetCurrentTask (TASK_TYPE_NO_DEVICE, 0);
			}
			ClearInfo();
		}
		else if ( err == STERR_NONE )
		{
			GetDeviceInfo();
			GetMediaInfo();
			GetDataDriveInfo(); // must do after getting media info
			SetOptionsEnabled(TRUE);
			GetDlgItem(IDC_START)->EnableWindow(TRUE);
			SetCurrentTask (TASK_TYPE_READY, 0);
			GetUpdater()->GetLogger()->Log( _T("START button enabled.") );
		}

		// Now close it till we need it.
		if ( GetUpdater()->IsDeviceReady() ||
			GetUpdater()->IsDeviceInRecoveryMode() ||
			GetUpdater()->IsDeviceInHidMode() )
			GetUpdater()->CloseDevice();

	    if ( !m_minimal_dlg )
		    GetDlgItem(IDC_DETAILS)->EnableWindow(TRUE);

		((CStUpdaterApp*)AfxGetApp())->PumpMessages();
	}
}

//
// This thread simply updates the progress bar during the non-update operations such as
// looking for devices and initializing recovery mode.
//
DWORD WINAPI CStUpdaterDlg::ConnectionProgressThread( LPVOID pParam )
{
	CStUpdaterDlg* _p_updDlg = (CStUpdaterDlg*)pParam;
    BOOL bKeepYourselfAlive = TRUE;     

	CoInitializeEx (NULL, COINIT_MULTITHREADED);

    _p_updDlg->SetTotalTasks(1);
   	_p_updDlg->SetCurrentTask (TASK_TYPE_DETECTING_DEVICE, 10);

    while ( bKeepYourselfAlive )
    {
            // we have been triggered
            // are we supposed to die?
            if ( WaitForSingleObject ( _p_updDlg->m_progress_thread_stop, 600) == WAIT_OBJECT_0 ||
				WaitForSingleObject ( _p_updDlg->m_app_closing, 0) == WAIT_OBJECT_0 )
            {
                bKeepYourselfAlive = FALSE;   // yes, die
				_p_updDlg->m_pb_task.SetPos( 0 );	// clear the progress bar
                continue; 
            }
           
	        _p_updDlg->m_pb_task.StepIt();
            _p_updDlg->UpdateProgress(TRUE);
    }

	CoUninitialize();

    return 0;
}

DWORD WINAPI CStUpdaterDlg::ConnectionMonitorThread( LPVOID pParam )
{
	CStUpdaterDlg* _p_updDlg = (CStUpdaterDlg*)pParam;
    BOOL bKeepYourselfAlive = TRUE;     

	CoInitializeEx (NULL, COINIT_MULTITHREADED);

//	SetThreadName ((DWORD)-1, "ConnectionMonitorThread");
	
	while ( bKeepYourselfAlive )
    {
        if ( WaitForSingleObject ( _p_updDlg->m_monitor_trigger, INFINITE) == WAIT_OBJECT_0 )
        {
            // we have been triggered
            // are we supposed to die?
            if ( WaitForSingleObject ( _p_updDlg->m_monitor_stop, 0) == WAIT_OBJECT_0 )
            {
                bKeepYourselfAlive = FALSE;   // yes, die
                continue; 
            }
            // go check for connection status
            _p_updDlg->CheckForDevice();

			InterlockedDecrement(&_p_updDlg->m_thread_block);
        }
    }

	CoUninitialize();
	return 0;
}



BOOL CStUpdaterDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwpData)
{
	switch (nEventType)
	{
		case DBT_DEVICEARRIVAL:
		case DBT_DEVICEREMOVECOMPLETE:
			break;
		case DBT_DEVICEQUERYREMOVE:
//			::MessageBox(NULL, L"DBT_DEVICEQUERYREMOVE", L"test", MB_OK);
			break;
		case DBT_DEVICEQUERYREMOVEFAILED:
//			::MessageBox(NULL, L"DBT_DEVICEQUERYREMOVEFAILED", L"test", MB_OK);
			break;
//		case DBT_DEVICEREMOVECOMPLETE:
//			::MessageBox(NULL, L"DBT_DEVICEREMOVECOMPLETE", L"test", MB_OK);
//			break;
		case DBT_DEVICEREMOVEPENDING:
//			::MessageBox(NULL, L"DBT_DEVICEREMOVEPENDING", L"test", MB_OK);
			break;
		case DBT_DEVICETYPESPECIFIC:
			::MessageBox(NULL, L"DBT_DEVICETYPESPECIFIC", L"test", MB_OK);
			break;
		case DBT_CONFIGCHANGED:
//			::MessageBox (NULL, L"DBT_CONFIG_CHANGED", L"test", MB_OK);
			break;
		case DBT_DEVNODES_CHANGED:
///			::MessageBox (NULL, L"DBT_DEVNODES_CHANGED", L"test", MB_OK);
			// Initialize device interface, but don't reset the device to do so.  If it is
			// in MTP mode leave it alone until we actually start the download.
//			if ( !m_init_timer && !m_timer_block && !((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->AutoStart() )
			if ( m_monitor_thread_handle != INVALID_HANDLE_VALUE )
                // && !((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->AutoStart() )
			{
				if (InterlockedCompareExchange(&m_thread_block, 1, 0) == 0)
				{
                    if ( !m_minimal_dlg )
			            GetDlgItem(IDC_DETAILS)->EnableWindow(FALSE);
			        GetDlgItem(IDC_START)->EnableWindow(FALSE);
					SetOptionsEnabled( FALSE );
	                CWaitCursor wait;
		            Sleep(3000); // have to wait for device to be stable, and Autoplay
			        SetEvent ( m_monitor_trigger );
				}
			}
			break;
		}

	return TRUE;
UNREFERENCED_PARAMETER(dwpData);
}

void CStUpdaterDlg::OnSysCommand(UINT _nID, LPARAM _lParam)
{
	if ((_nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg *pAboutDlg = NULL;
		if ( (GetUpdater()->GetConfigInfo())->IsAboutDlgBMP() )
		{
//			pAboutDlg = new CAboutDlg(GetUpdater()->GetConfigInfo(), (USHORT)0, (CWnd*) 0);
//			pAboutDlg->DoModal();
			// Set bitmap
			//CString dummy = _T("");
			CString csDesc, csVers, csCopyRight;

			wstring wsDesc, wsCopyRight;
			LANGID language = ((CStUpdaterApp*)AfxGetApp())->GetLangId();

			GetUpdater()->GetConfigInfo()->ApplicationDescription( wsDesc, language );
			csDesc = wsDesc.c_str();

			GetUpdater()->GetConfigInfo()->GetCopyrightString( wsCopyRight, language );
			csCopyRight = wsCopyRight.c_str();

			csVers = ((CStUpdaterApp*)AfxGetApp())->GetResource()->GetAboutVersionString();
	
			USHORT bitmapId = GetUpdater()->GetConfigInfo()->GetBitmapId();
//			CStSplashWnd::ShowSplashScreen(5000, IDI_COMPANY_BMP, dummy, (LPTSTR)wsDesc.c_str(), csVers, (LPTSTR)wsCopyRight.c_str(), this);
			CStSplashWnd::ShowSplashScreen(5000, bitmapId, csDesc, csVers, csCopyRight, this);

		}
		else // use icon
		{
			pAboutDlg = new CAboutDlg(GetUpdater()->GetConfigInfo());
			pAboutDlg->DoModal();
//			CAboutDlg dlgAbout(GetUpdater()->GetConfigInfo());
//			dlgAbout.DoModal();
		}
		if (pAboutDlg)
			delete pAboutDlg;
	}
    else
	{
		CDialog::OnSysCommand(_nID, _lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CStUpdaterDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CStUpdaterDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CStUpdaterDlg::OnAutoStart()
{
	USHORT maxWait = 600;
	while ( !GetDlgItem(IDC_START)->IsWindowEnabled() && maxWait )
	{
		Sleep(100);
		((CStUpdaterApp*)AfxGetApp())->PumpMessages();
		--maxWait;
	}

	if ( maxWait != 0 && GetDlgItem(IDC_START)->IsWindowEnabled() )
	{
		OnStart();
		return TRUE;
	}
	else
		return FALSE;
}

void CStUpdaterDlg::OnStart() 
{
    HANDLE hProgressThread;
    DWORD dwThreadId, dwProgThreadId;
    CWaitCursor wait;

	GetUpdater()->GetLogger()->Log( _T("<User clicked START.>") );
//	if ( !GetUpdater()->IsDeviceInUpdaterMode() && !GetUpdater()->IsDeviceInMscMode() )
//		goto outtahere;


    // kill the connection monitor
	if ( m_monitor_thread_handle != INVALID_HANDLE_VALUE )
	{
	    SetEvent ( m_monitor_stop );
	    SetEvent ( m_monitor_trigger );

		if ( WaitForSingleObjectEx( m_monitor_thread_handle, 3000, FALSE ) == WAIT_OBJECT_0 )
			m_monitor_thread_handle = INVALID_HANDLE_VALUE;
	}

	//
	// if format data area is checked and default setting for the check-box is also checked and 
	// if default action to take is not enabled then confirm from user to carry out the firmware 
	// download operation as it results in loosing all media contents. 
	// Default action is to continue with firmware download operation without consulting the user.
	//
	if( !m_minimal_dlg && m_format_data_area_btn.GetCheck() )
	{
		if ( m_p_updater->GetConfigInfo()->GetDefaultStateForFormatDataArea() )
		{
			if( ((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->DefaultAction() == FALSE )
			{
				int ret_code = IDYES;
				CString str;

				((CStUpdaterApp*)AfxGetApp())->GetResource()->GetResourceString(IDS_MSG_DEFAULT_FORMAT_DATA_AREA, str);
				ret_code = CStMessageDlg::DisplayMessage(MSG_TYPE_QUESTION, str, this);

				if(ret_code == IDNO)
				{
					return;
				}
			}
		}
	}

	if (m_minimal_dlg &&
		((CStUpdaterApp*)AfxGetApp())->GetUpdater()->GetConfigInfo()->GetMinDlgFormatWarningMsg() &&
		( ((CStUpdaterApp*)AfxGetApp())->GetUpdater()->GetConfigInfo()->GetDefaultStateForFormatDataArea() ||
		  ((CStUpdaterApp*)AfxGetApp())->GetUpdater()->GetConfigInfo()->GetDefaultStateForEraseMedia() ) )
	{
		int ret_code = IDYES;
		CString str;

		((CStUpdaterApp*)AfxGetApp())->GetResource()->GetResourceString(IDS_MSG_USER_SELECTED_FORMAT_DATA_AREA, str);
		ret_code = CStMessageDlg::DisplayMessage(MSG_TYPE_QUESTION, str, this);

		if(ret_code == IDNO)
		{
			return;
		}
	}

	LogDetails(TRUE); // full list of details in log

	ST_ERROR err = Begin();
	if( err != STERR_NONE && err != STERR_MEDIA_STATE_UNINITIALIZED )
	{
		g_Status = err;
		return;
	}

    if ( err == STERR_MEDIA_STATE_UNINITIALIZED )
        m_operation = UPDATE_ERASE_MEDIA;
	else
		m_operation = UPDATE_NONE;

	// task progress bar control handles
	m_task_progress_mutex = CreateMutex(NULL, FALSE, NULL);
	m_progress_thread_stop = CreateEvent( NULL, FALSE, FALSE, NULL );

    hProgressThread = CreateThread(
	                	NULL,                        // default security attributes 
                		0,                           // use default stack size  
                		CStUpdaterDlg::UpdateTaskProgressThread,  // thread function 
                		this,						 // argument to thread function 
                		0,                           // use default creation flags 
                		&dwProgThreadId);  

   	err = DownloadFirmware();

    SetEvent ( m_progress_thread_stop );
	Sleep(0);

	if ( hProgressThread)
		if ( WaitForSingleObject( hProgressThread, 3000 ) == WAIT_OBJECT_0)
		{
			CloseHandle( m_progress_thread_stop );
			m_progress_thread_stop = INVALID_HANDLE_VALUE;
		}

	CloseHandle ( m_task_progress_mutex );
	m_task_progress_mutex = INVALID_HANDLE_VALUE;


	if(err != STERR_NONE)
	{
		g_Status = err;
		((CStUpdaterApp*) AfxGetApp())->HandleError( err, this );
	}
	
	if(err == STERR_MANUAL_RECOVERY_REQUIRED)
	{
		GetUpdater()->GetLogger()->Log( _T("Waiting for user to put device in recovery-mode manually...") );
	    ResetEvent ( m_monitor_stop );
	    ResetEvent ( m_monitor_trigger );

		SetCurrentTask (TASK_TYPE_NO_DEVICE, 0);

    	m_monitor_thread_handle = CreateThread(
			           	NULL,                        // default security attributes 
		         		0,                           // use default stack size  
                		CStUpdaterDlg::ConnectionMonitorThread,  // thread function 
                		this,						 // argument to thread function 
                		0,                           // use default creation flags 
                		&dwThreadId);  


	}
//	else
//	{
		// update data drive info and disable all after update
//		UpdateInfoAfterUpdate();
//	}

    if ( m_operation != UPDATE_NONE )
	    m_p_updater->ResetChip();

	Relax();

   	if( err == STERR_NONE )
	{

		SetCurrentTask(TASK_TYPE_COMPLETED, 1);
		UpdateProgress();

		USHORT chipId = 0xffff;
		err = GetUpdater()->GetChipId(chipId);
        if ( chipId < 0x3600 && 
            !((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->AutoQuit() &&
            m_p_updater->GetConfigInfo()->ShowBootToPlayerMessage() &&
            ( m_operation & UPDATE_ERASE_MEDIA || m_operation & UPDATE_REMOVE_DRM || m_operation & UPDATE_FORMAT_DATA ) )
        {
	    	CString str;

            ((CStUpdaterApp*)AfxGetApp())->GetResource()->GetResourceString(IDS_MTP_STORE_INIT_REQUIRED, str);
       	    CStMessageDlg::DisplayMessage(MSG_TYPE_INFO, str, this);
        }

	}
	else if (err != STERR_MANUAL_RECOVERY_REQUIRED)
	{
		SetCurrentTask(TASK_TYPE_ERROR_OCCURED, 1);
		UpdateProgress();
	}

	if( ((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->AutoQuit() )
	{
		OnClose();
	}

}

void CStUpdaterDlg::OnFormatDataArea() 
{
    if ( m_minimal_dlg )
        return;

	// TODO: Add your control notification handler code here
	if(m_format_data_area_btn.GetCheck())
	{
		int ret_code;
		CString str;

		if( m_advanced_dlg )
		{	// don't bug the "advanced" user with this silly message
			ret_code = IDYES;
		}
		else
		{
			((CStUpdaterApp*)AfxGetApp())->GetResource()->GetResourceString(IDS_MSG_USER_SELECTED_FORMAT_DATA_AREA, str);

			ret_code = CStMessageDlg::DisplayMessage(MSG_TYPE_QUESTION, str, this);
		}

		if(ret_code == IDNO)
		{
			m_format_data_area_btn.SetCheck(BST_UNCHECKED);
			m_full_media_erase_btn.SetCheck(BST_UNCHECKED);
		}
		else if ( m_advanced_dlg )
			m_filesystem_list.EnableWindow(TRUE);
	}
	else if ( m_advanced_dlg )
	{
		m_filesystem_list.EnableWindow(FALSE);
		m_full_media_erase_btn.SetCheck(BST_UNCHECKED);
	}
}

void CStUpdaterDlg::OnFullMediaErase() 
{
    if ( !m_advanced_dlg )
        return;

	// TODO: Add your control notification handler code here
	if(m_full_media_erase_btn.GetCheck())
	{
		if(!m_format_data_area_btn.GetCheck())
		{
			int ret_code;
			CString str;

			if( m_advanced_dlg )
				ret_code = IDYES;
			else
			{
				((CStUpdaterApp*)AfxGetApp())->GetResource()->GetResourceString(IDS_MSG_USER_SELECTED_FORMAT_DATA_AREA, str);

				ret_code = CStMessageDlg::DisplayMessage(MSG_TYPE_QUESTION, str, this);
			}

			if(ret_code == IDNO)
			{
				m_full_media_erase_btn.SetCheck(BST_UNCHECKED);
			}
			else  // set the format data area checkbox also
			{
				m_format_data_area_btn.SetCheck(BST_CHECKED);
				m_filesystem_list.EnableWindow(TRUE);
			}
		}
	}
}




void CStUpdaterDlg::OnClose() 
{
	GetUpdater()->GetLogger()->Log(_T("<User clicked CLOSE.>"));

	// kill the connection monitor
	if ( m_monitor_thread_handle != INVALID_HANDLE_VALUE )
	{
		SetEvent ( m_app_closing );
	    SetEvent ( m_monitor_stop );
	    SetEvent ( m_monitor_trigger );
		MsgWaitForMultipleObjects(1, &m_monitor_thread_handle, TRUE, 10000, QS_ALLEVENTS);
		//WaitForSingleObjectEx( m_monitor_thread_handle, 15000, FALSE );
	}

	if ( m_task_progress_mutex != INVALID_HANDLE_VALUE )
		CloseHandle ( m_task_progress_mutex );
	if ( m_progress_thread_stop != INVALID_HANDLE_VALUE )
		CloseHandle ( m_progress_thread_stop );
	if ( m_app_closing != INVALID_HANDLE_VALUE )
		CloseHandle ( m_app_closing );

	if( m_modal )
		CDialog::OnOK();
	else
		DestroyWindow();
}

void CStUpdaterDlg::OnShowVersionDetails() 
{
    if ( m_minimal_dlg )
        return;

	CStVersionInfoPtrArray* p_arr_current_component_vers = NULL;
	CStVersionInfoPtrArray* p_arr_upgrade_component_vers = NULL;
    BOOL tried_to_get_versions = FALSE;

    GetDlgItem(IDC_DETAILS)->EnableWindow(FALSE);

	ST_ERROR err = GetUpdater()->FindMSCDevice(FIND_ANY_DEVICE);
	GetUpdater()->FreeScsiDevice();

	if( err != STERR_NONE && err != STERR_MEDIA_STATE_UNINITIALIZED )
	{
		if( err == STERR_FAILED_TO_LOCATE_SCSI_DEVICE )
		{
			err = STERR_FAILED_TO_LOCATE_SCSI_DEVICE_ON_SHOW_VERSIONS;
		}

		((CStUpdaterApp*) AfxGetApp())->HandleError( err, this );
	}
    else
    {
        if ( !GetUpdater()->IsDeviceInLimitedMscMode()) // || GetUpdater()->GetConfigInfo()->ForceGetCurrentVersions() )
        {
	        err = GetUpdater()->GetComponentVersions( &p_arr_current_component_vers, &p_arr_upgrade_component_vers );
            tried_to_get_versions = TRUE;
        }
    }
	
	if( err != STERR_NONE || !tried_to_get_versions )
	{	// no device connected
		// at least get versions for the upgrade binaries
		CStVersionInfo p_upgrade_project_vers;

		p_arr_current_component_vers = NULL;

		err = GetUpdater()->GetUpgradeVersions(
			&p_upgrade_project_vers, 
			&p_arr_upgrade_component_vers);

		if (err != STERR_NONE)
			((CStUpdaterApp*) AfxGetApp())->HandleError( err, this );
	}


	// We no longer need the device, so close it
	GetUpdater()->CloseDevice();

	CStFwVersionDlg dlg(
		p_arr_current_component_vers, 
		p_arr_upgrade_component_vers, 
		m_p_updater->GetConfigInfo()
	);
	
	dlg.DoModal();

	delete dlg;

	if ( p_arr_current_component_vers )
		delete p_arr_current_component_vers;
	if ( p_arr_upgrade_component_vers )
		delete p_arr_upgrade_component_vers;

	GetDlgItem(IDC_DETAILS)->EnableWindow(TRUE);

}

void CStUpdaterDlg::ClearInfo()
{
    if ( !m_advanced_dlg )
        return;

	SetDlgItemText(IDC_DEVINFO_CHIPID, L"");
	SetDlgItemText(IDC_DEVINFO_ROM_REVISION, L"");
	SetDlgItemText(IDC_DEVINFO_MODE, L"");
	SetDlgItemText(IDC_DEVINFO_EXT_RAM_SIZE, L"");
	SetDlgItemText(IDC_DEVINFO_VRAM_SIZE, L"");
	SetDlgItemText(IDC_DEVINFO_SERIALNO, L"");
	SetDlgItemText(IDC_PROTOCOL_VER, L"");
    GetDlgItem(IDC_TEXT_DEVINFO_CHIPID)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DEVINFO_ROM_REVISION)->EnableWindow(FALSE);
    GetDlgItem(IDC_TEXT_DEVINFO_MODE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DEVINFO_EXT_RAM_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DEVINFO_VRAM_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DEVINFO_SERIALNO)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_PROTOCOL_VER)->EnableWindow(FALSE);

	SetDlgItemText(IDC_MEDIA_TYPE, L"");
	SetDlgItemText(IDC_MEDIA1, L"");
	SetDlgItemText(IDC_MEDIA2, L"");
	SetDlgItemText(IDC_MEDIA3, L"");
	SetDlgItemText(IDC_MEDIA4, L"");
	SetDlgItemText(IDC_MEDIA5, L"");
	SetDlgItemText(IDC_MEDIA6, L"");
	GetDlgItem(IDC_TEXT_MEDIA_TYPE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA1)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA2)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA3)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA4)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA5)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA6)->EnableWindow(FALSE);
   	GetDlgItem(IDC_TEXT_MEDIA1)->ShowWindow(FALSE);
   	GetDlgItem(IDC_TEXT_MEDIA2)->ShowWindow(FALSE);
   	GetDlgItem(IDC_TEXT_MEDIA3)->ShowWindow(FALSE);
   	GetDlgItem(IDC_TEXT_MEDIA4)->ShowWindow(FALSE);
   	GetDlgItem(IDC_TEXT_MEDIA5)->ShowWindow(FALSE);
   	GetDlgItem(IDC_TEXT_MEDIA6)->ShowWindow(FALSE);

	SetDlgItemText(IDC_DRIVE_LETTER, L"");
	SetDlgItemText(IDC_DRIVE_CAPACITY, L"");
	SetDlgItemText(IDC_DATA_FREESPACE, L"");
	SetDlgItemText(IDC_FILE_SYSTEM, L"");
	SetDlgItemText(IDC_SECTOR_SIZE, L"");
	SetDlgItemText(IDC_SECTOR_COUNT, L"");
    GetDlgItem(IDC_TEXT_DATA_FREESPACE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_FILE_SYSTEM)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_SECTOR_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_SECTOR_COUNT)->EnableWindow(FALSE);
}

void CStUpdaterDlg::GetDeviceInfo()
{
	CStDeviceInfo *pDeviceInfo = new CStDeviceInfo(this);

	pDeviceInfo->GetDeviceInfo();

    if ( m_advanced_dlg )
	{
		pDeviceInfo->SetDeviceInfo();

		if (GetDlgItem(IDC_TEXT_DEVINFO_MODE)->IsWindowEnabled() == FALSE )
		    GetDlgItem(IDC_TEXT_DEVINFO_MODE)->EnableWindow(TRUE);

		UpdateDeviceMode();
	}

	delete pDeviceInfo;
}

void CStUpdaterDlg::GetMediaInfo()
{
	PHYSICAL_MEDIA_TYPE MediaType;
	ST_ERROR err = STERR_NONE;
	CString str;

/***/
	if ( !m_advanced_dlg )
		return;

	// Get media type
  	err = GetUpdater()->GetMediaType(MediaType);
	
	if(err == STERR_NONE)
	{
		switch (MediaType)
		{
			case MediaTypeNand:
			{
				CStNandMediaInfo *pNandInfo = new CStNandMediaInfo(this,
								((CStUpdaterApp*)AfxGetApp())->GetResource());
				if( pNandInfo )
				{
					err = pNandInfo->GetMediaInfo();

					str = L"NAND";

					if ( m_advanced_dlg )
						pNandInfo->SetNandInfo();

					if( pNandInfo->GetCapacity() > (256*ONE_MB) )
						m_bCapacityOver256MB = TRUE;
					else
						m_bCapacityOver256MB = FALSE;

					delete pNandInfo;
				}
				break;
			}

			case MediaTypeHDD:
				str = L"HDD";
				break;

			case MediaTypeiNAND:
				str = L"iNAND";
				break;

			case MediaTypeMMC:
				str = L"MMC/SD";
				break;

			case MediaTypeRAM:
				str = L"RAM";
				break;

			default:
				str = L"Unknown";
				break;
		}

		if ( m_advanced_dlg )
		{
		    GetDlgItem(IDC_TEXT_MEDIA_TYPE)->EnableWindow(TRUE);
			SetDlgItemText(IDC_MEDIA_TYPE, str);
		}

		m_LogInfo.MediaInfoMediaType = str;
	}
	else
	{
		if ( m_advanced_dlg )
		{
		    GetDlgItem(IDC_TEXT_MEDIA_TYPE)->EnableWindow(FALSE);
			SetDlgItemText(IDC_MEDIA_TYPE, L"");
		}
	}

	return;
}




void CStUpdaterDlg::GetDataDriveInfo()
{
	USHORT secsize = 0;
	ULONG sectors = 0;
	TCHAR szFileSystem[16] = {0};
	TCHAR szDrive[8] ={0};
	CString str;
	ULARGE_INTEGER ulCapacity;
	ULARGE_INTEGER freeSpace;
	double dCapInMB, dFreeSpaceInMB;
	int retry = 0;

	if ( GetUpdater()->IsDeviceInMtpMode() ||
		GetUpdater()->IsDeviceInRecoveryMode() ||
		GetUpdater()->IsDeviceInHidMode() )
		return;

	ulCapacity.QuadPart = 0;
	freeSpace.QuadPart = 0;

try_again:
	if (retry < 3)
	{
		GetUpdater()->GetDataDriveInfo (0, szDrive, szFileSystem, 16, sectors, secsize);

		if (szDrive[0])
		{
			str.Format(L"%s\\", szDrive);
			GetDiskFreeSpaceEx( str, NULL, &ulCapacity, &freeSpace );
		}
		else
		{
			Sleep(2000);
			++retry;
			goto try_again;
		}
	}

	dCapInMB = (double)(ulCapacity.QuadPart / ONE_MB);
	dFreeSpaceInMB = (double)(freeSpace.QuadPart / ONE_MB);


	if ( m_advanced_dlg && dCapInMB != 0)
	{
		if (freeSpace.QuadPart > (ULONG)TWO_GB)
		{
			m_filesystem_list.DeleteString(1); // no FAT16
			if (GetUpdater()->GetConfigInfo()->GetPreferredFAT() != FAT_32)
				GetUpdater()->GetConfigInfo()->SetPreferredFAT(FAT_32);
		}
		else
		{
			m_filesystem_list.DeleteString(2);
			if( dCapInMB >= 256 || (dCapInMB == 0 && m_bCapacityOver256MB) ||
				((sectors * secsize) >= (256 * ONE_MB)) )
			{
				m_filesystem_list.AddString(L"FAT32");
				m_filesystem_list.SetItemData(2, 2);
			}
			else
			{
				if (GetUpdater()->GetConfigInfo()->GetPreferredFAT() == FAT_32)
					GetUpdater()->GetConfigInfo()->SetPreferredFAT(FAT_16);
			}
		}
		m_filesystem_list.SetCurSel(
						GetUpdater()->GetConfigInfo()->GetPreferredFAT() );

	    GetDlgItem(IDC_TEXT_DATA_FREESPACE)->EnableWindow(TRUE);
		GetDlgItem(IDC_TEXT_FILE_SYSTEM)->EnableWindow(TRUE);
	    GetDlgItem(IDC_TEXT_SECTOR_SIZE)->EnableWindow(TRUE);
		GetDlgItem(IDC_TEXT_SECTOR_COUNT)->EnableWindow(TRUE);
	}

	if ( m_advanced_dlg  && dCapInMB != 0)
		SetDlgItemText(IDC_DRIVE_LETTER, szDrive);
	m_LogInfo.DataInfoDriveLetter = szDrive;

	if ( m_advanced_dlg  && dCapInMB != 0)
		SetDlgItemText(IDC_FILE_SYSTEM, szFileSystem);
	m_LogInfo.DataInfoFileSystem = szFileSystem;

	str.Format(L"%d", secsize);
	if ( m_advanced_dlg  && dCapInMB != 0)
		SetDlgItemText(IDC_SECTOR_SIZE, str);
	m_LogInfo.DataInfoSectorSize = str;

	str.Format(L"0x%X", sectors);
	if ( m_advanced_dlg  && dCapInMB != 0)
		SetDlgItemText(IDC_SECTOR_COUNT, str);
	m_LogInfo.DataInfoSectorCount = str;

	str.Format(L"%g(MB)", dCapInMB);
	if ( m_advanced_dlg  && dCapInMB != 0)
		SetDlgItemText(IDC_DRIVE_CAPACITY, str);
	m_LogInfo.DataInfoCapacity = str;

	str.Format(L"%g(MB)", dFreeSpaceInMB);
	if ( m_advanced_dlg  && dCapInMB != 0)
		SetDlgItemText(IDC_DATA_FREESPACE, str);
	m_LogInfo.DataInfoFreespace = str;
}

void CStUpdaterDlg::UpdateInfoAfterUpdate()
{
	TCHAR szDrive[6]= _T("");
	TCHAR szFS[16] = _T("");
	ULARGE_INTEGER ulCapacity;
	ULARGE_INTEGER freeSpace;
	double dCapInMB = 0;
	double dFreeSpaceInMB = 0;
	CString str;
	ULONG sectors;
	USHORT secsize;

	if ( GetUpdater()->IsDeviceInMtpMode() ||
		GetUpdater()->IsDeviceInRecoveryMode() ||
		GetUpdater()->IsDeviceInHidMode() )
		return;

	ulCapacity.QuadPart = 0;
	freeSpace.QuadPart = 0;

	GetUpdater()->GetDataDriveInfo (0, szDrive, szFS, 16, sectors, secsize);
	if (szDrive[0])
	{
		_tcscat_s(szDrive, 6, L"\\");

	    if( GetDiskFreeSpaceEx( szDrive, NULL, &ulCapacity, &freeSpace ) )
		{
			dCapInMB = (double)(ulCapacity.QuadPart / (1024 * 1024));
			dFreeSpaceInMB = (double)(freeSpace.QuadPart / (1024 * 1024));
		}
	}

	if ( m_advanced_dlg )
		SetDlgItemText(IDC_FILE_SYSTEM, szFS);
	m_LogInfo.DataInfoFileSystem = szFS;

	str.Format(L"%g(MB)", dCapInMB);
	if ( m_advanced_dlg )
		SetDlgItemText(IDC_DRIVE_CAPACITY, str);
	m_LogInfo.DataInfoCapacity = str;

	str.Format(L"%g(MB)", dFreeSpaceInMB);
	if ( m_advanced_dlg )
		SetDlgItemText(IDC_DATA_FREESPACE, str);
	m_LogInfo.DataInfoFreespace = str;

	str.Format(L"%d", secsize);
	if ( m_advanced_dlg )
		SetDlgItemText(IDC_SECTOR_SIZE, str);
	m_LogInfo.DataInfoSectorSize = str;

	str.Format(L"0x%x", sectors);
	if ( m_advanced_dlg )
		SetDlgItemText(IDC_SECTOR_COUNT, str);
	m_LogInfo.DataInfoSectorCount = str;

	LogDetails(FALSE); // log only data area changes

	if ( !m_advanced_dlg )
		return;

    GetDlgItem(IDC_TEXT_DEVINFO_CHIPID)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DEVINFO_ROM_REVISION)->EnableWindow(FALSE);
    GetDlgItem(IDC_TEXT_DEVINFO_MODE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DEVINFO_EXT_RAM_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DEVINFO_VRAM_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DEVINFO_SERIALNO)->EnableWindow(FALSE);
    GetDlgItem(IDC_TEXT_PROTOCOL_VER)->EnableWindow(FALSE);
    GetDlgItem(IDC_DEVINFO_CHIPID)->EnableWindow(FALSE);
	GetDlgItem(IDC_DEVINFO_ROM_REVISION)->EnableWindow(FALSE);
    GetDlgItem(IDC_DEVINFO_MODE)->EnableWindow(FALSE);
	GetDlgItem(IDC_DEVINFO_EXT_RAM_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_DEVINFO_VRAM_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_DEVINFO_SERIALNO)->EnableWindow(FALSE);
    GetDlgItem(IDC_PROTOCOL_VER)->EnableWindow(FALSE);

	GetDlgItem(IDC_TEXT_MEDIA_TYPE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA1)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA2)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA3)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA4)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA5)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_MEDIA6)->EnableWindow(FALSE);
	GetDlgItem(IDC_MEDIA_TYPE)->EnableWindow(FALSE);
	GetDlgItem(IDC_MEDIA1)->EnableWindow(FALSE);
	GetDlgItem(IDC_MEDIA2)->EnableWindow(FALSE);
	GetDlgItem(IDC_MEDIA3)->EnableWindow(FALSE);
	GetDlgItem(IDC_MEDIA4)->EnableWindow(FALSE);
	GetDlgItem(IDC_MEDIA5)->EnableWindow(FALSE);
	GetDlgItem(IDC_MEDIA6)->EnableWindow(FALSE);

	GetDlgItem(IDC_DRIVE_LETTER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DRIVE_CAPACITY)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_DATA_FREESPACE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_FILE_SYSTEM)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_SECTOR_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_TEXT_SECTOR_COUNT)->EnableWindow(FALSE);
    GetDlgItem(IDC_DATA_FREESPACE)->EnableWindow(FALSE);
	GetDlgItem(IDC_FILE_SYSTEM)->EnableWindow(FALSE);
	GetDlgItem(IDC_SECTOR_SIZE)->EnableWindow(FALSE);
	GetDlgItem(IDC_SECTOR_COUNT)->EnableWindow(FALSE);
}

ST_ERROR CStUpdaterDlg::DownloadFirmware()
{
	ST_ERROR					err = STERR_NONE;
	CString						msg;

	//
    // Check for what operation to perform. 
	//
   	err = CheckOperationToPerform(m_operation);
    if( err != STERR_NONE )
   	{
    	((CStUpdaterApp*) AfxGetApp())->HandleError( err, this );
	    return err;
   	}

    if ( m_operation == UPDATE_NONE )
        return STERR_NONE;  // user aborted on invalid Janus data

	CString drive(GetUpdater()->GetDriveLetter());
	// Let the shell know we are gone temporarily so that it moves its focus out.
	drive += ":\\";
	SHChangeNotify(SHCNE_MEDIAREMOVED, SHCNF_PATH|SHCNF_FLUSH, drive, 0);
	Sleep(1000); //pause for a while

	GetUpdater()->SetTotalTasks(m_operation);

	GetUpdater()->PrepareForUpdate();

    err = GetUpdater()->UpdateDevice( m_operation );

//	((CStUpdaterApp*) AfxGetApp())->HandleError( err, this );

	//big-time-hack : to fix the refresh problem under winxp. 
	SHChangeNotify(SHCNE_MEDIAINSERTED | SHCNE_UPDATEDIR | SHCNE_FREESPACE, SHCNF_PATH|SHCNF_FLUSH, drive, 0);
//	Sleep(1000); //pause for a while.

	//
	// mount any dismounted volumes.
	//
//	GetLogicalDrives();

	DisplayProjectVersion();

	if (err == STERR_NONE)
	{
		//On a non-formatting download delete the settings.dat file.
		if ( !(m_operation & UPDATE_FORMAT_DATA) )
		{
			WIN32_FIND_DATA	file_data;
			HANDLE hFind;

			CString settings_file(GetUpdater()->GetDriveLetter());

			settings_file += ":\\SETTINGS.DAT";	

			hFind = FindFirstFile( settings_file, &file_data );
			if (hFind != INVALID_HANDLE_VALUE) 
			{
				FindClose(hFind);
				SetFileAttributes( settings_file, FILE_ATTRIBUTE_NORMAL );
				if( !DeleteFile( settings_file ) )
				{
					GetUpdater()->GetErrorObject()->SaveStatus( STERR_FAILED_TO_DELETE_SETTINGS_DOT_DAT_FILE, 
						CStGlobals::GetLastError() );
					((CStUpdaterApp*) AfxGetApp())->HandleError( STERR_FAILED_TO_DELETE_SETTINGS_DOT_DAT_FILE, this );
				}
			}
		}

		if ( m_operation & UPDATE_ERASE_MEDIA || m_operation & UPDATE_REMOVE_DRM ) 
		{
			UCHAR versionMajor;

			GetUpdater()->GetProtocolVersionMajor(versionMajor);
			if (versionMajor >= ST_UPDATER_RAMLESS_JANUS &&
				(GetUpdater()->GetConfigInfo()->GetFirmwareHeaderFlags() & FW_HEADER_FLAGS_DRM_ENABLED) )
			{

				UCHAR versionMinor = 0;

				GetUpdater()->GetProtocolVersionMinor(versionMinor);
				// reset to updater and perform Janus init
				//GetUpdater()->InitializeDeviceInterface( TRUE );
				GetUpdater()->ResetToMscUpdaterMode();
				while (!GetUpdater()->IsDeviceInUpdaterMode())
				{
					Sleep(500);
					((CStUpdaterApp*)AfxGetApp())->PumpMessages();
					if ( WaitForSingleObject( m_app_closing, 0) == WAIT_OBJECT_0 )
					{
						err = STERR_NONE;
						break;
					}

					err = GetUpdater()->InitializeDeviceInterface( TRUE );
				}
				GetUpdater()->GetProtocolVersionMinor(versionMinor);
				if ( versionMinor == 1 )
				{
					m_operation = UPDATE_INIT_JANUS | UPDATE_INIT_STORE;
					err = GetUpdater()->UpdateDevice( m_operation );
				}
			}
		}

		// update data drive info and disable all after update
		UpdateInfoAfterUpdate();

		// Cleanup after update
		GetUpdater()->CompletedUpdate();
	} // successful update

    return err;
}

void CStUpdaterDlg::SetTotalTasks(ULONG _total_tasks)
{
	m_total_tasks = _total_tasks;
	if( InProgress () )
	{
        if ( !m_minimal_dlg )
        {
    		m_pb_overall.SetRange(0, (short)_total_tasks);
	    	m_pb_overall.SetStep(1);
        }
	}
}

void CStUpdaterDlg::SetCurrentTask(TASK_TYPE _task, ULONG _range)
{
	m_current_task = _task;
	m_task_range = _range;

	if( WaitForSingleObject ( m_app_closing, 0) == WAIT_TIMEOUT )
	{
		CString str_task;
//		m_pb_task.SetRange(0, (short)_range);
//		m_pb_task.SetStep(1);
		if ( m_task_progress_mutex != INVALID_HANDLE_VALUE )
		{
			if ( WaitForSingleObject ( m_task_progress_mutex, 1000) == WAIT_OBJECT_0 )
			{
				m_pb_task.SetPos( 0 );
				ReleaseMutex( m_task_progress_mutex );
			}
		}
		else
		{
			m_pb_task.SetRange(0, (short)_range);
			m_pb_task.SetStep(1);
			m_pb_task.SetPos( 0 );
		}

		((CStUpdaterApp*)AfxGetApp())->GetResource()->GetTaskName(_task, str_task);
		SetDlgItemText(IDC_TEXT_PROGRESS, str_task);

		CString logStr = _T("*");
		logStr.Append(str_task);
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

		((CStUpdaterApp*)AfxGetApp())->PumpMessages();

		if ( !m_minimal_dlg &&
                _task != TASK_TYPE_INIT_DEVICE_INTERFACE &&
                _task != TASK_TYPE_DETECTING_DEVICE &&
			    _task != TASK_TYPE_READY &&
			    _task != TASK_TYPE_NO_DEVICE )
			UpdateGrandProgress();

		if( !m_minimal_dlg  && ( _task == TASK_TYPE_COMPLETED ) || ( _task == TASK_TYPE_ERROR_OCCURED ) )
		{
			int upper, lower;

			m_pb_overall.GetRange( lower, upper );
			m_pb_overall.SetPos( upper );	
		}
	}
}


//
// This thread simply updates the progress bar during the update operations.  Large NANDs
// were making the task progress appear to stop during long operations like EraseMedia.
// Now we just bump it along at regular intervals, and zero it for new tasks.
//
DWORD WINAPI CStUpdaterDlg::UpdateTaskProgressThread( LPVOID pParam )
{
	CStUpdaterDlg* _p_updDlg = (CStUpdaterDlg*)pParam;
    BOOL bKeepYourselfAlive = TRUE;     

	CoInitializeEx (NULL, COINIT_MULTITHREADED);

	_p_updDlg->m_pb_task.SetRange(0, 12);
	_p_updDlg->m_pb_task.SetStep(1);
	_p_updDlg->m_pb_task.SetPos( 0 );

    while ( bKeepYourselfAlive )
    {
        // we have been triggered
        // are we supposed to die?
        if ( WaitForSingleObject ( _p_updDlg->m_progress_thread_stop, 500) == WAIT_OBJECT_0 ||
			WaitForSingleObject ( _p_updDlg->m_app_closing, 0) == WAIT_OBJECT_0 )
        {
            bKeepYourselfAlive = FALSE;   // yes, die
			if ( WaitForSingleObject ( _p_updDlg->m_task_progress_mutex, 1000) == WAIT_OBJECT_0 )
			{
			    _p_updDlg->m_pb_task.SetPos(0);
				ReleaseMutex( _p_updDlg->m_task_progress_mutex );
			}
 
            continue; 
        }
          
		if ( WaitForSingleObject ( _p_updDlg->m_task_progress_mutex, 1000) == WAIT_OBJECT_0 )
		{
	        _p_updDlg->m_pb_task.StepIt();
			ReleaseMutex( _p_updDlg->m_task_progress_mutex );
		}
    }

	CoUninitialize();
    return 0;
}

void CStUpdaterDlg::UpdateProgress( BOOL _step_it )
{
//	if( InProgress () && _step_it )
//	if( _step_it )
//	{
//		m_pb_task.StepIt();
//	}
	if ( WaitForSingleObject( m_app_closing, 0) != WAIT_OBJECT_0 )
		((CStUpdaterApp*)AfxGetApp())->PumpMessages();
UNREFERENCED_PARAMETER(_step_it);
}

unsigned total = 0;
void CStUpdaterDlg::UpdateGrandProgress()
{
	if ( WaitForSingleObject( m_app_closing, 0) != WAIT_OBJECT_0 )
	{
		++total;
		if ( !m_minimal_dlg )
			m_pb_overall.StepIt();
	}
}

void CStUpdaterDlg::UpdateDeviceMode()
{
    if ( m_advanced_dlg && (WaitForSingleObject( m_app_closing, 0) != WAIT_OBJECT_0) )
	{
		UCHAR versionMajor, versionMinor;

		if ( GetUpdater()->IsDeviceInMscMode() || GetUpdater()->IsDeviceInUpdaterMode() )
		{
			CString str;
		    GetDlgItem(IDC_TEXT_PROTOCOL_VER)->EnableWindow(TRUE);
			GetUpdater()->GetProtocolVersionMajor(versionMajor);
			GetUpdater()->GetProtocolVersionMinor(versionMinor);
			str.Format(L"%d.%d", versionMajor, versionMinor);
			SetDlgItemText(IDC_PROTOCOL_VER, str);
			str.Format(L"Protocol version: %d.%d", versionMajor, versionMinor);
			((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( str );
		}
		else
		{
			SetDlgItemText(IDC_PROTOCOL_VER, L"");
		    GetDlgItem(IDC_TEXT_PROTOCOL_VER)->EnableWindow(FALSE);
		}

		SetDlgItemText(IDC_DEVINFO_MODE, L"");
	    GetDlgItem(IDC_TEXT_DEVINFO_MODE)->EnableWindow(TRUE);
		if ( GetUpdater()->IsDeviceInMtpMode() )
			SetDlgItemText(IDC_DEVINFO_MODE, L"MTP");
		else if ( GetUpdater()->IsDeviceInRecoveryMode() )
			SetDlgItemText(IDC_DEVINFO_MODE, L"RCV");
		else if ( GetUpdater()->IsDeviceInHidMode() )
			SetDlgItemText(IDC_DEVINFO_MODE, L"HID");
		else if ( GetUpdater()->IsDeviceInLimitedMscMode() )
			SetDlgItemText(IDC_DEVINFO_MODE, L"LIM");
		else if ( GetUpdater()->IsDeviceInMscMode() )
			SetDlgItemText(IDC_DEVINFO_MODE, L"MSC");
		else if ( GetUpdater()->IsDeviceInUpdaterMode() )
			SetDlgItemText(IDC_DEVINFO_MODE, L"UPD");
		else
		    GetDlgItem(IDC_TEXT_DEVINFO_MODE)->EnableWindow(FALSE);
	}
}


#define JANUS_OK             0x00
#define JANUS_CORRUPT        0x01

ST_ERROR CStUpdaterDlg::CheckOperationToPerform(USHORT& _operation) 
{
	CStResource *p_resource	= ((CStUpdaterApp*)AfxGetApp())->GetResource();
	ST_ERROR err = STERR_NONE;
	int ret_code;
	CString str;
	BOOL erase_media_required=FALSE;
    UCHAR janus_status = JANUS_CORRUPT;
	UCHAR protocol_version;

    // if device is in not in MSC or updater mode we must download updater.sb before download can begin
	err = GetUpdater()->InitializeDeviceInterface ( TRUE );

	if ( err || WaitForSingleObject( m_app_closing, 0) == WAIT_OBJECT_0 )
	{ // we've been closed
	    _operation = UPDATE_NONE;
		return STERR_NONE;
	}

	// clear the status bars
	m_pb_task.SetPos( 0 );
    if ( !m_minimal_dlg )
    	m_pb_overall.SetPos( 0 );

	if (err == STERR_MANUAL_RECOVERY_REQUIRED)
		return err;

	if ( (err != STERR_NONE && err != STERR_MEDIA_STATE_UNINITIALIZED) || !GetUpdater()->IsDeviceReady() )
		return STERR_FAILED_TO_LOCATE_SCSI_DEVICE_ON_START;

	err = GetUpdater()->CheckForFwResources();

	if ( err != STERR_NONE )
		return STERR_FAILED_TO_OPEN_FILE;

    if ( m_operation & UPDATE_ERASE_MEDIA ) // Media un-initialized?
		m_operation |= UPDATE_FIRMWARE | UPDATE_REMOVE_DRM | UPDATE_FORMAT_DATA;
	else
	{
		_operation = UPDATE_FIRMWARE;

		if ( m_minimal_dlg &&
			((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetDefaultStateForFormatDataArea() )
			m_operation |= UPDATE_FORMAT_DATA;

		if ( m_minimal_dlg &&
			((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetDefaultStateForEraseMedia() )
			_operation |= UPDATE_REMOVE_DRM | UPDATE_ERASE_MEDIA | UPDATE_FORMAT_DATA;

		// check for cmdline override
		if( ((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->EraseMedia() ||
				( m_advanced_dlg && m_full_media_erase_btn.GetCheck() ) )
			_operation |= UPDATE_REMOVE_DRM | UPDATE_ERASE_MEDIA;
	    else
		{
    		// check janus status
	        GetUpdater()->GetJanusStatus (janus_status);
		    // If the janus data is corrupt, we don't want to restore the HDS.
			if (janus_status != JANUS_OK)
	        {
//                WCHAR szMsg[100];
//                wsprintf(szMsg, L"Janus status: %01x", janus_status);
//                ::MessageBox(NULL, szMsg, L"test", MB_OK);

		        _operation |= UPDATE_REMOVE_DRM | UPDATE_ERASE_MEDIA;

				if( m_minimal_dlg || ( !m_minimal_dlg && !m_format_data_area_btn.GetCheck()) )
				{
				    p_resource->GetResourceString(IDS_MSG_DEFAULT_REMOVE_DRM_DATA, str);
					ret_code = CStMessageDlg::DisplayMessage(MSG_TYPE_QUESTION, str, this);

			    	if( ret_code == IDYES )
				    {	// User has opted to continue.
                        if ( !m_minimal_dlg )
						{
				            m_format_data_area_btn.SetCheck(BST_CHECKED);
	                        if ( m_advanced_dlg )
							{
//								m_filesystem_list.EnableWindow(TRUE);
					            m_full_media_erase_btn.SetCheck(BST_CHECKED);
							}
						}
		            }
			        else
				    {
					    _operation = UPDATE_NONE;
						return err;
					}
				}
		    }
			else
	        {
		    	err = GetUpdater()->IsEraseMediaRequired(erase_media_required);
			    if( err != STERR_NONE )
       			    return err;

	            if ( erase_media_required )
		            _operation |= UPDATE_ERASE_MEDIA;
			}
		}
	}

	if( _operation & UPDATE_ERASE_MEDIA || _operation & UPDATE_REMOVE_DRM )
	{
		//
		// If fw reported media needs to be erased or DRM data removed then inform the user and 
		// get approval to continue with a data area format. If not approved quit.
		//
		if( !m_minimal_dlg && !m_format_data_area_btn.GetCheck() )
        {
			if( ((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->DefaultAction() )
			{
				ret_code = IDYES;
			}
			else
			{
				p_resource->GetResourceString(IDS_MSG_DOWNLOAD_RESULTS_IN_FORMATTING_DATA_AREA, str);

				ret_code = CStMessageDlg::DisplayMessage(MSG_TYPE_QUESTION, str, this);
			}
			
			if( ret_code == IDYES )
			{
				//
				// User has opted to do a format.
				//
				m_format_data_area_btn.SetCheck(BST_CHECKED);
//				if ( m_advanced_dlg )
//					m_filesystem_list.EnableWindow(TRUE);

                _operation |= UPDATE_FORMAT_DATA;

			}
			else
			{
				//
				// User has opted NOT to continue with the download. In this case we can't do anything.
				//
                _operation = UPDATE_NONE;
			}
		}
    	else 
	    {
            _operation |= UPDATE_FORMAT_DATA;
		}
	}
	else
	{   // everything OK, check if user has selected to format
		if( !m_minimal_dlg && m_format_data_area_btn.GetCheck() )
            _operation |= UPDATE_FORMAT_DATA;
	}


    if ( _operation & UPDATE_FORMAT_DATA )
        if ( ((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->Recover2DD() )
            _operation |= UPDATE_2DD_CONTENT;

	// check that the version is prior to SDK5.  If so, we do the Janus and Store init
	// as part of the update.  If SDK5 or later it requires the Janus and Store initialization
	// to be done after the update and reset to updater.
	GetUpdater()->GetProtocolVersionMajor( protocol_version );

	if ( protocol_version < ST_UPDATER_RAMLESS_JANUS )
	{
		if ( _operation & UPDATE_REMOVE_DRM || _operation & UPDATE_ERASE_MEDIA )
			_operation |= UPDATE_INIT_JANUS + UPDATE_SAVE_HDS;

		if ( _operation & UPDATE_FORMAT_DATA )
			_operation |= UPDATE_INIT_STORE + UPDATE_SAVE_HDS;
	}
	else
		if ( GetUpdater()->GetConfigInfo()->GetFirmwareHeaderFlags() & FW_HEADER_FLAGS_DRM_ENABLED )
			_operation |= UPDATE_SAVE_HDS;

	return err;
}

void CStUpdaterDlg::SaveVolumeLabel()
{
	CString drive(GetUpdater()->GetDriveLetter());

	drive += ":\\";	

	::GetVolumeInformation(
			drive,        
			m_volume_label,     
			MAX_PATH,         
			NULL,  
			NULL,
			NULL,     
			NULL, 
			0
		);
}

void CStUpdaterDlg::RestoreVolumeLabel()
{
	CString drive(GetUpdater()->GetDriveLetter());
	CString label( m_volume_label );
	wstring		str_label;

	drive += ":\\";	


	if( GetUpdater()->GetConfigInfo()->GetDefaultVolumeLabel(str_label) == STERR_NONE )
	{
		if( str_label.length() > VOLUME_LABEL_LENGTH )
			label.Format(L"%-*.*s", VOLUME_LABEL_LENGTH, VOLUME_LABEL_LENGTH, str_label.c_str() );
		else
			label = CString( str_label.c_str() );
	}

	label.TrimRight();
	::SetVolumeLabel(drive, label);
}

ST_ERROR CStUpdaterDlg::Begin()
{
	CStProgress::Begin();

	// It could be a while and the status of the device might have changed so refresh is a good idea before we begin.
	CStResource *				p_resource	= ((CStUpdaterApp*)AfxGetApp())->GetResource();
	ST_ERROR					err = STERR_NONE;
	CString						msg;
	
	p_resource->GetResourceString( IDS_LOGGING_DOWNLOAD_PROCESS_BEGIN, msg );
	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( msg );

	CString str_task;

	((CStUpdaterApp*)AfxGetApp())->GetResource()->GetTaskName(TASK_TYPE_NONE, str_task);
	SetDlgItemText( IDC_TEXT_PROGRESS, str_task );

	m_pb_task.SetPos( 0 );

	LANGID langId = ((CStUpdaterApp*)AfxGetApp())->GetCmdLineProcessor()->ForceFWLanguage();

	if ( langId )
		GetUpdater()->SetLanguageId( (WORD) langId );

    m_warning_ctrl.ShowWindow(SW_SHOW);
    // Start background blinking
//	m_warning_ctrl.StartBkBlink(TRUE, CColorStaticST::ST_FLS_NORMAL);
    // Start text blinking
	m_warning_ctrl.StartTextBlink(TRUE, CColorStaticST::ST_FLS_FAST);

    if ( !m_minimal_dlg )
    {
	    m_format_data_area_btn.EnableWindow(FALSE);

	    if ( m_advanced_dlg )
		    m_full_media_erase_btn.EnableWindow(FALSE);

    	m_pb_overall.SetPos( 0 );

        GetDlgItem(IDC_DETAILS)->EnableWindow(FALSE);

		if ( m_advanced_dlg && m_language_list.IsWindowEnabled() )
			GetUpdater()->SetLanguageId( (WORD) m_language_list.GetItemData( m_language_list.GetCurSel() ));
    }

	SetOptionsEnabled( FALSE );
	GetDlgItem(IDC_START)->EnableWindow(FALSE);
	GetDlgItem(IDOK)->EnableWindow(FALSE);

	return err;
}

void CStUpdaterDlg::Relax()
{
	m_warning_ctrl.ShowWindow(SW_HIDE);
    m_warning_font.DeleteObject();

	if ( !m_minimal_dlg )
    {
    	GetDlgItem(IDC_DETAILS)->EnableWindow(TRUE);
    }

	GetDlgItem(IDOK)->EnableWindow(TRUE);

	// set the focus to the CStUpdaterDlg window so the shortcut keys will still work -CLW
	SetFocus();

	//big-time-hack : to fix the refresh problem under winxp. 
//	CString drive(GetUpdater()->GetDriveLetter());
//	drive += ":\\";
//	SHChangeNotify(SHCNE_MEDIAINSERTED | SHCNE_UPDATEDIR | SHCNE_FREESPACE, SHCNF_PATH|SHCNF_FLUSH, drive, 0);
//	Sleep(1000); //pause for a while.

	CStProgress::Relax();
}

ST_ERROR CStUpdaterDlg::DisplayProjectVersion()
{
	CString cur_ver_str = "0.0.0";
	CString upgrade_ver_str = "0.0.0";
	
	//versions
	CStVersionInfo current_project_vers;
	CStVersionInfo upgrade_project_vers;
	
	ST_ERROR err = m_p_updater->GetProjectVersions(
		&current_project_vers, 
		&upgrade_project_vers
	);
	
	if(err != STERR_NONE || GetUpdater()->IsDeviceInLimitedMscMode())
	{
		err = m_p_updater->GetProjectUpgradeVersion(
					&upgrade_project_vers);

		if ( err != STERR_NONE )
		{
			((CStUpdaterApp*) AfxGetApp())->HandleError( err, this );
			return err;
		}
	}

/*	if ( m_p_updater->GetConfigInfo()->HasCustomSupport() )
	{
		CCustomSupport support;
		cur_ver_str = support.GetVersionString(&current_project_vers);
		upgrade_ver_str = support.GetVersionString(&upgrade_project_vers);
	}
	else
*/
//	{
//		cur_ver_str.Format(_T("%d.%d"), current_project_vers.GetHigh(), current_project_vers.GetMid());
//		upgrade_ver_str.Format(_T("%d.%d"), upgrade_project_vers.GetHigh(), upgrade_project_vers.GetMid());
		cur_ver_str = current_project_vers.GetVersionString().c_str();
		upgrade_ver_str = upgrade_project_vers.GetVersionString().c_str();
//	}
	SetDlgItemText(IDC_CURRENT_FIRMWARE, cur_ver_str);
	SetDlgItemText(IDC_UPGRADE_FIRMWARE, upgrade_ver_str);
	((CStUpdaterApp*) AfxGetApp())->PumpMessages();

	return err;
}

void CStUpdaterDlg::OnCancel()
{
	if( InProgress() )
		return;

	if( m_modal )
		CDialog::OnCancel();
	else
		DestroyWindow();
}

void CStUpdaterDlg::SetModal(BOOL _state)
{
	m_modal = _state;
}

void CStUpdaterDlg::SetCurrentTask(TASK_TYPE _task, ULONG _range, UCHAR _index)
{
	//
	// to keep the other functions simple the update and erase system drive tasks are found out here
	// based on the _drive_index parameter. Other routines may simply pass the correct index to 
	// m_p_arr_system_drive and for _task they may pass either TASK_TYPE_ERASE_SYSTEMDRIVE1 if its an 
	// erase task otherwise TASK_TYPE_UPDATE_SYSTEMDRIVE1 if its an update.
	//

    UCHAR booty_index=0;

    ((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetBootyDriveIndex(booty_index);

	switch( _task )
	{
	case TASK_TYPE_UPDATE_HIDDENDRIVE :
		_task = (TASK_TYPE)((int)TASK_TYPE_UPDATE_HIDDENDRIVE + _index);
		break;
	case TASK_TYPE_ERASE_HIDDENDRIVE :
		_task = (TASK_TYPE)((int)TASK_TYPE_ERASE_HIDDENDRIVE + _index);
		break;
	case TASK_TYPE_UPDATE_SYSTEMDRIVE :
		_task = (TASK_TYPE)((int)TASK_TYPE_UPDATE_SYSTEMDRIVE + (_index - booty_index + 1));
        break;
    case TASK_TYPE_ERASE_SYSTEMDRIVE :
		_task = (TASK_TYPE)((int)TASK_TYPE_ERASE_SYSTEMDRIVE + (_index - booty_index + 1));
		break;
	case TASK_TYPE_FORMATTING_FAT_AREA :
		_task = (TASK_TYPE)((int)TASK_TYPE_FORMATTING_FAT_AREA + _index);
		break;
	}
	
	return SetCurrentTask( _task, _range );	
}


void CStUpdaterDlg::WhatResourcesAreBound()
{
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;

    m_TotalBoundResourceCount = 0;
	m_BoundLangResourceCount = 0;
	m_pBoundIds = NULL;

	hResInfo = FindResourceEx( NULL,
					L_STMP_RESINFO_TYPE,
					MAKEINTRESOURCE(IDR_BOUND_RESOURCE_COUNT),
					MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
	if ( hResInfo )
	{
		hRes = LoadResource(NULL, hResInfo);
		if ( hRes )
			pPtr = LockResource(hRes);
		if ( pPtr && *((unsigned int *)pPtr) ) 
			m_TotalBoundResourceCount = *((unsigned int *)pPtr);
	}

	if ( m_TotalBoundResourceCount )
	{
		hResInfo = FindResourceEx( NULL,
						L_STMP_RESINFO_TYPE,
						MAKEINTRESOURCE(IDR_BOUND_LANG_RESOURCE_COUNT),
						MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
		if ( hResInfo )
		{
			hRes = LoadResource(NULL, hResInfo);
			if ( hRes )
				pPtr = LockResource(hRes);
			if ( *((unsigned int *)pPtr) ) 
				m_BoundLangResourceCount = *((unsigned int *)pPtr);
		}

		if ( m_BoundLangResourceCount )
		{
			hResInfo = FindResourceEx( NULL,
							L_STMP_RESINFO_TYPE,
							MAKEINTRESOURCE(IDR_BOUND_LANG_IDS),
							MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
			if ( hResInfo )
			{
				hRes = LoadResource(NULL, hResInfo);
				if ( hRes )
					m_pBoundIds = (LANGID *)LockResource(hRes);
			}
		}
	}
}

typedef BOOL (WINAPI *LPFN_WOW64DISABLEWOW64FSREDIRECTION) (PVOID *);
typedef BOOL (WINAPI *LPFN_WOW64REVERTWOW64FSREDIRECTION) (PVOID);

BOOL CStUpdaterDlg::IsRecoveryDriverInstalled()
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

    if (CStGlobals::GetPlatform() == OS_VISTA64 ||
        CStGlobals::GetPlatform() == OS_XP64)
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
	
	CString str;
	str.Format(_T("The Player Recovery Device Class drivers are %sinstalled."), returnStatus ? _T(" ") : _T("NOT "));
	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( str );

	return returnStatus;
}



ST_ERROR CStUpdaterDlg::InstallRecoveryDriver()
{
        LPVOID pPtr = NULL;
        DWORD dwSize;
        WCHAR szPath[MAX_PATH];
        WCHAR szTempDir[MAX_PATH];
        BOOL Isx64, IsLocal;
        ST_ERROR status = STERR_FAILED_TO_OPEN_FILE;

        if (CStGlobals::GetPlatform() == OS_VISTA64 ||
            CStGlobals::GetPlatform() == OS_XP64)
            Isx64 = TRUE;
        else
            Isx64 = FALSE;

        if (!GetWindowsDirectory(szPath, MAX_PATH))
			return STERR_NO_MEMORY;

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
              	if (SetupCopyOEMInf( szPath, szTempDir, SPOST_PATH,
                                 SP_COPY_NOOVERWRITE,
                       			0,0,0,0))
                    status = STERR_NONE;
        }

		if( IsLocal ) // free the buffer for the .inf
			free( pPtr );

		// The new hardware wizard should now be able to load it when
        // we reset to recovery.

        return status;
}




LPVOID CStUpdaterDlg::StLoadStmp3RecResource(USHORT resID, PDWORD pdwSize, BOOL& isLocal)
{
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;

	isLocal = FALSE;

	if ( ((CStUpdaterApp*) AfxGetApp())->GetUpdater()->GetConfigInfo()->GetDefaultLocalResourceUsage() )
	{
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

BOOL CStUpdaterDlg::StWriteStmp3RecResource(LPVOID _pPtr, ULONG _dwSize, LPTSTR _pathName)
{
    BOOL returnStatus = FALSE;
    HANDLE hFile;


    hFile = CStGlobals::CreateFile(
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


void CStUpdaterDlg::OnCbnSelchangeLanguageId()
{
	if ( m_advanced_dlg )
		GetUpdater()->SetLanguageId( (WORD) m_language_list.GetItemData( m_language_list.GetCurSel() ));
}

void CStUpdaterDlg::OnCbnSelchangeFileSystemId()
{
	if ( m_advanced_dlg )
		GetUpdater()->GetConfigInfo()->SetPreferredFAT(
		(UCHAR)m_filesystem_list.GetItemData(m_filesystem_list.GetCurSel())
						);
}


void CStUpdaterDlg::OnDownloadDetails()
{
}

UINT CStUpdaterDlg::BugabooWindowCloserThreadProc( LPVOID pParam )
{
	CStUpdaterDlg * p_updaterdlg_class = (CStUpdaterDlg*)pParam;
	CWnd* wnd = NULL;
	
//	SetThreadName ((DWORD)-1, "BugabooWindowCloserThread");

	// get the localized title of the "Unsafe Removal of Device" dialog 
	// so we can close it.
	//CString title = GetDlgCaption(_T("hotplug.dll"), 330);
	CString title = L"Microsoft Windows";
	if(!title.IsEmpty())
	{	
		while( WaitForSingleObject(p_updaterdlg_class->m_hEventKill, 0) == WAIT_TIMEOUT) {
			wnd = CWnd::FindWindow(NULL, title);
			if( wnd ) {
				wnd->SendMessage( WM_CLOSE );
				ATLTRACE("== Closed an auto format window.\n");
				wnd = NULL;
			}
			Sleep(25);
		}
	}
	
	return 0;
}

void CStUpdaterDlg::GetLanguageString( CString& str, LANGID _langId )
{
	//CStResource * p_resource = ((CStUpdaterApp*)AfxGetApp())->GetResource();
	TCHAR szNewFwGroup[64];

	GetLocaleInfo(_langId, LOCALE_SLANGUAGE, szNewFwGroup, 64);

	str = szNewFwGroup;
}


void CStUpdaterDlg::SetOptionsEnabled( BOOL _state )
{
    if ( m_minimal_dlg )
		return;

	GetDlgItem(IDC_FORMAT_DATA_AREA)->EnableWindow( _state );

	if ( m_advanced_dlg )
	{
		GetDlgItem(IDC_FULL_MEDIA_ERASE)->EnableWindow( _state );
		GetDlgItem(IDC_DNLD_FS_TEXT)->EnableWindow( _state );
		GetDlgItem(IDC_DNLD_FS)->EnableWindow( _state );
		GetDlgItem(IDC_LANGUAGE_TEXT)->EnableWindow( _state );
	    GetDlgItem(IDC_LANGUAGE_ID)->EnableWindow( _state );
	}

}

void CStUpdaterDlg::LogDetails(BOOL _full)
{
	CString rscStr, logStr;
	CStResource * p_resource = ((CStUpdaterApp*)AfxGetApp())->GetResource();

	if (!(((CStUpdaterApp*) AfxGetApp())->GetCmdLineProcessor())->Log())
		return;

	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( _T("Dumping Updater/Device details.") );

	if ( _full )
	{
	  	p_resource->GetResourceString(IDS_UPDATER_VERSION, rscStr);
		logStr.Format( _T(" %s%s"), rscStr,m_LogInfo.UpdaterInfoVersion);
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

		p_resource->GetResourceString(IDS_UPDATER_BASE_SDK, rscStr);
		logStr.Format( _T(" %s %s"), rscStr, m_LogInfo.UpdaterInfoSDKBase );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

		logStr.Format( _T(" USB VID_PID:  %s"), m_LogInfo.UpdaterInfoVidPid );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   		p_resource->GetResourceString(IDS_UPDATER_BOUND_RES_COUNT, rscStr);
		logStr.Format( _T(" %s%s"), rscStr, m_LogInfo.UpdaterInfoBoundResources );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

		// Device info group
		p_resource->GetResourceString(IDS_DEVINFO_CHIPID, rscStr);
		logStr.Format( _T(" %s%s"), rscStr, m_LogInfo.DevInfoChipId );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

	   	p_resource->GetResourceString(IDS_DEVINFO_ROM_REVISION, rscStr);
		logStr.Format( _T(" %s%s"), rscStr, m_LogInfo.DevInfoROMRevision );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   		p_resource->GetResourceString(IDS_DEVINFO_EXT_RAM_SIZE, rscStr);
		logStr.Format( _T(" %s%s"), rscStr, m_LogInfo.DevInfoExtRAMSize );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

	   	p_resource->GetResourceString(IDS_DEVINFO_VRAM_SIZE, rscStr);
		logStr.Format( _T(" %s%s"), rscStr, m_LogInfo.DevInfoVirtualRAMSize );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   		p_resource->GetResourceString(IDS_MEDIA_SERIAL_NUM, rscStr);
		logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.DevInfoSerialNumber );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );
		
		p_resource->GetResourceString(IDS_DEVINFO_MODE, rscStr);
		logStr.Format( _T(" %s%s"), rscStr, m_LogInfo.DevInfoMode );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

		// Media details group
   		p_resource->GetResourceString(IDS_MEDIA_TYPE, rscStr);
		logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.MediaInfoMediaType );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   		p_resource->GetResourceString(IDS_MEDIA_CHIPSELECTS, rscStr);
		logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.MediaInfoNandChipSelects );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   		p_resource->GetResourceString(IDS_MEDIA_MFG, rscStr);
		logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.MediaInfoNandMfgId );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   		p_resource->GetResourceString(IDS_MEDIA_CELLTYPE, rscStr);
		logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.MediaInfoNandCellType );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

	   	p_resource->GetResourceString(IDS_MEDIA_MODEL, rscStr);
		logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.MediaInfoNandIdDetails );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   		p_resource->GetResourceString(IDS_MEDIA_CAPACITY, rscStr);
		logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.MediaInfoCapacity );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

	   	p_resource->GetResourceString(IDS_MEDIA_PAGE_SIZE, rscStr);
		logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.MediaInfoNandPageSize );
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );



		// Download options groupbox

		if ( !m_minimal_dlg )
		{
			m_format_data_area_btn.GetWindowTextW(rscStr);
			if ( m_format_data_area_btn.GetCheck() )
				rscStr.Append( L": Yes" );
			else
				rscStr.Append( L": No");
		}
		else
		{
			if ( GetUpdater()->GetConfigInfo()->GetDefaultStateForFormatDataArea() )
				rscStr.Append(L": Yes");
			else
				rscStr.Append(L": No");
		}
		
		logStr.Format( _T(" %s"), rscStr); 
		((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

		if ( m_advanced_dlg )
		{
			m_full_media_erase_btn.GetWindowTextW(rscStr);
			if ( m_full_media_erase_btn.GetCheck() )
				rscStr.Append( L": Yes" );
			else
				rscStr.Append( L": No");
			
			logStr.Format( _T(" %s"), rscStr); 
			((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

			if ( m_language_list.IsWindowEnabled() )
			{
				CString other;
   				p_resource->GetResourceString(IDS_LANGUAGE, rscStr);
				rscStr.Append( L": ");
				m_language_list.GetWindowTextW(other);
				rscStr.Append( other );
				logStr.Format( _T(" %s"), rscStr); 
				((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );
			}
		}
	}

	// Data area details group
	logStr.Format( _T(" %s  %s"), m_LogInfo.DataInfoDriveLetter, m_LogInfo.DataInfoCapacity);
	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   	p_resource->GetResourceString(IDS_FREESPACE, rscStr);
	logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.DataInfoFreespace );
	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

	p_resource->GetResourceString(IDS_FILE_SYSTEM, rscStr);
	logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.DataInfoFileSystem );
	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   	p_resource->GetResourceString(IDS_SECTOR_SIZE, rscStr);
	logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.DataInfoSectorSize );
	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );

   	p_resource->GetResourceString(IDS_SECTOR_COUNT, rscStr);
	logStr.Format( _T(" %s  %s"), rscStr, m_LogInfo.DataInfoSectorCount );
	((CStUpdaterApp*) AfxGetApp())->GetLogger()->Log( logStr );
}

void CStUpdaterDlg::ClearLogDetails()
{
	m_LogInfo.UpdaterInfoVersion.Empty();
	m_LogInfo.UpdaterInfoSDKBase.Empty();
	m_LogInfo.UpdaterInfoVidPid.Empty();
	m_LogInfo.UpdaterInfoBoundResources.Empty();
	m_LogInfo.DevInfoChipId.Empty();
	m_LogInfo.DevInfoROMRevision.Empty();
	m_LogInfo.DevInfoExtRAMSize.Empty();
	m_LogInfo.DevInfoVirtualRAMSize.Empty();
	m_LogInfo.DevInfoSerialNumber.Empty();
	m_LogInfo.DevInfoMode.Empty();
	m_LogInfo.MediaInfoMediaType.Empty();
	m_LogInfo.MediaInfoNandChipSelects.Empty();
	m_LogInfo.MediaInfoNandMfgId.Empty();
	m_LogInfo.MediaInfoNandCellType.Empty();
	m_LogInfo.MediaInfoNandIdDetails.Empty();
	m_LogInfo.MediaInfoCapacity.Empty();
	m_LogInfo.MediaInfoNandPageSize.Empty();
	m_LogInfo.DataInfoDriveLetter.Empty();
	m_LogInfo.DataInfoCapacity.Empty();
	m_LogInfo.DataInfoFreespace.Empty();
	m_LogInfo.DataInfoFileSystem.Empty();
	m_LogInfo.DataInfoSectorSize.Empty();
	m_LogInfo.DataInfoSectorCount.Empty();
}
