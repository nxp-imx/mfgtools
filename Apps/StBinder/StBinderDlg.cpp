// StBinderDlg.cpp : implementation file
//

#include "stdafx.h"
#include <winnls.h>
#include "StBinder.h"
#include "StBinderDlg.h"
#include "..\\..\\Libs\\WinSupport\\StSplashWnd.h"
#include "resourcelist.h"
#include "enumresourcetype.h"
#include "stfwversion.h"
#include "default_langid.h"
#include "otherfirmwarelangid.h"
#include "userconfirmation.h"
#include "targetconfigsheet.h"
#include "StCfgResOptions.h"
#include "version.h"
#include "StBinderProfile.h"
#include "LoadProfileDlg.h"
#include "SaveProfileDlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ONE_MB					1024*1024
#define MAX_RESOURCE_GROUPS     8   // 1 for stmp3rec and 7 f/w languages

#define CAPTION_TEXT			_T("StBinder")

#define COLUMN_PATHNAME         0
//#define COLUMN_TAG              1
#define COLUMN_VERSION          1
#define COLUMN_SIZE             2
#define COLUMN_DATE             3

enum _resource_index
{
	recovery_driver_files,
	firmware_language_neutral,
	firmware_chinese_traditional,
	firmware_chinese_simplified,
	firmware_english,
	firmware_french,
	firmware_german,
	firmware_japanese,
	firmware_korean,
    firmware_portuguese,
	firmware_spanish,
	firmware_other
};




#define DEFINED_RESOURCE_GROUPS		11 // matches enum above without "other"

#define STMPRESTYPE					_T("STMPRESTYPE")
#define BOUND_UPDATER_RELEASE		750
#define FORMAT_RESOURCES_RELEASE	780
#define FULL_CFG_RESOURCES_RELEASE	800

#define NUM_RECOVERY_FILENAMES		6
#define NUM_FW_FILENAMES			10

CString csRecoveryFileNames[NUM_RECOVERY_FILENAMES] =
{
    _T("\\stmp3rec.sys"),
    _T("\\stmp3rec.inf"),
    _T("\\stmp3rec.cat"),
    _T("\\stmp3recx64.sys"),
    _T("\\stmp3recx64.inf"),
    _T("\\stmp3recx64.cat")
};

// global resource config data read/written to StUpdater
TARGET_CFG_DATA g_ResCfgData;


// CStBinderDlg dialog



CStBinderDlg::CStBinderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStBinderDlg::IDD, pParent)
    , m_total_file_count(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_SIGMATEL_ICON);
	m_pUpdaterVersion = NULL;
	m_LocalSelectionChanged = FALSE;
	m_Busy = FALSE;
	m_StatusTextId = 0;

	g_ResCfgData.pUpdaterGrpIconHeader = NULL;
	g_ResCfgData.pCompanyGrpIconHeader = NULL;
}

CStBinderDlg::~CStBinderDlg()
{
}


void CStBinderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TARGET, m_target_name);
	DDX_Control(pDX, IDC_BINARIES_LIST, m_ResourceListCtl);
	DDX_Control(pDX, IDC_TOTAL_FILE_COUNT, m_total_files);
	DDX_Control(pDX, IDC_BOUND_RESOURCE_COUNT, m_bound_resources);
	DDX_Control(pDX, IDC_UPDATER_VERSION, m_updater_version);
	DDX_Control(pDX, IDC_DLG_TARGET_BASESDK, m_updater_base_SDK);
	DDX_Control(pDX, IDC_UPDATER_DATE, m_updater_date);
	DDX_Control(pDX, IDC_UPDATER_FILESIZE, m_updater_filesize);
	DDX_Control(pDX, IDC_STATUS_TEXT, m_status_text);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress_ctrl);
	DDX_Control(pDX, IDC_RESOURCETREE, m_ResourceTree);
	DDX_Control(pDX, IDC_FIND_TARGET, m_FindTargetBtn);
	DDX_Control(pDX, IDOK, m_ApplyBtn);
	DDX_Control(pDX, IDCANCEL, m_CloseBtn);
	DDX_Control(pDX, IDC_DLG_TARGET_CONFIGURE, m_configure_btn);
	DDX_Control(pDX, IDC_CFG_LOAD_PROFILE, m_loadprofile_btn);
	DDX_Control(pDX, IDC_CFG_SAVE_PROFILE, m_saveprofile_btn);
}

BEGIN_MESSAGE_MAP(CStBinderDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_FIND_TARGET, OnBnClickedFindTarget)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
    ON_NOTIFY(LVN_INSERTITEM, IDC_BINARIES_LIST, OnLvnInsertitemBinariesList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_BINARIES_LIST, OnLvnKeydownBinariesList)
	ON_NOTIFY(NM_RCLICK, IDC_BINARIES_LIST, &CStBinderDlg::OnNMRclickBinariesList)
	ON_NOTIFY(TVN_SELCHANGED, IDC_RESOURCETREE, &CStBinderDlg::OnTvnSelchangedResourceTree)
	ON_NOTIFY(NM_RCLICK, IDC_RESOURCETREE, &CStBinderDlg::OnNMRclickResourcetree)
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_DLG_TARGET_CONFIGURE, &CStBinderDlg::OnBnClickedDlgTargetConfigure)
	ON_BN_CLICKED(IDC_CFG_SAVE_PROFILE, &CStBinderDlg::OnBnClickedCfgSaveProfile)
	ON_BN_CLICKED(IDC_CFG_LOAD_PROFILE, &CStBinderDlg::OnBnClickedCfgLoadProfile)
END_MESSAGE_MAP()


// CStBinderDlg message handlers

BOOL CStBinderDlg::OnInitDialog()
{
    CString countStr;
	CString resStr, descStr, verStr, copyRightStr ;
	MOUSEMOVECALLBACK mmCallBack;

	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		BOOL bLoad = strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_ABOUT_VERSION, descStr);
	verStr = CString(BINDER_STRING_VERSION);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_ABOUT_COPYRIGHT, copyRightStr);

	CStSplashWnd::ShowSplashScreen(2000, IDB_SPLASH, descStr, verStr, copyRightStr, this);
	Sleep(2000); // sleep while the splash screen is up
	
    GetDlgItem(IDOK)->EnableWindow(FALSE);


	m_updater_version.SetWindowText(_T(""));
	m_updater_base_SDK.SetWindowText(_T(""));
	m_updater_date.SetWindowText(_T(""));
	m_updater_filesize.SetWindowText(_T(""));
	m_status_text.SetWindowText(_T(""));
    m_bound_resources.SetWindowText(_T(""));
	m_configure_btn.EnableWindow(FALSE);
	m_loadprofile_btn.EnableWindow(FALSE);
	m_saveprofile_btn.EnableWindow(FALSE);

    m_total_file_count = 0;
    countStr.Format(_T("%d"), m_total_file_count);
    m_total_files.SetWindowText(countStr);

    m_bound_resource_count = 0;

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_SPECIFY_TARGET, resStr);
	SetDlgItemText(IDC_DLG_SPECIFY_TARGET, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_SPECIFY_RES_GRP, resStr);
	SetDlgItemText(IDC_DLG_SPECIFY_RESOURCE_GRP, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_GRP_UPDATER_INFORMATION, resStr);
	SetDlgItemText(IDC_GRP_UPDATER_INFORMATION, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_TARGET_VERSION, resStr);
	SetDlgItemText(IDC_DLG_TARGET_VERSION_TEXT, resStr );
	GetDlgItem(IDC_DLG_TARGET_VERSION_TEXT)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_TARGET_BASESDK_TEXT, resStr);
	SetDlgItemText(IDC_DLG_TARGET_BASESDK_TEXT, resStr );
	GetDlgItem(IDC_DLG_TARGET_BASESDK_TEXT)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_TARGET_DATE, resStr);
	SetDlgItemText(IDC_DLG_TARGET_DATE_TEXT, resStr );
	GetDlgItem(IDC_DLG_TARGET_DATE_TEXT)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_TARGET_SIZE, resStr);
	SetDlgItemText(IDC_DLG_TARGET_SIZE_TEXT, resStr );
	GetDlgItem(IDC_DLG_TARGET_SIZE_TEXT)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_TARGET_RESOURCES, resStr);
	SetDlgItemText(IDC_DLG_TARGET_RESOURCES_TEXT, resStr );
	GetDlgItem(IDC_DLG_TARGET_RESOURCES_TEXT)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_FILE_COUNT, resStr);
	SetDlgItemText(IDC_DLG_FILE_COUNT_TEXT, resStr );
	GetDlgItem(IDC_DLG_FILE_COUNT_TEXT)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_APPLY_TEXT, resStr);
	SetDlgItemText(IDOK, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_CLOSE_TEXT, resStr);
	SetDlgItemText(IDCANCEL, resStr );

	mmCallBack.id = IDC_FIND_TARGET;
	mmCallBack.pCallerClass = this;
	mmCallBack.pfnCallback = CStBinderDlg::OnControlMouseMove;

	m_FindTargetBtn.SetMouseMoveCallback( mmCallBack );

	mmCallBack.id = IDOK;
	mmCallBack.pCallerClass = this;
	mmCallBack.pfnCallback = CStBinderDlg::OnControlMouseMove;

	m_ApplyBtn.SetMouseMoveCallback( mmCallBack );

	mmCallBack.id = IDCANCEL;
	mmCallBack.pCallerClass = this;
	mmCallBack.pfnCallback = CStBinderDlg::OnControlMouseMove;

	m_CloseBtn.SetMouseMoveCallback( mmCallBack );

	mmCallBack.id = IDC_DLG_TARGET_CONFIGURE;
	mmCallBack.pCallerClass = this;
	mmCallBack.pfnCallback = CStBinderDlg::OnControlMouseMove;

	m_configure_btn.SetMouseMoveCallback( mmCallBack );

	mmCallBack.id = IDC_CFG_LOAD_PROFILE;
	mmCallBack.pCallerClass = this;
	mmCallBack.pfnCallback = CStBinderDlg::OnControlMouseMove;

	m_loadprofile_btn.SetMouseMoveCallback( mmCallBack );

	mmCallBack.id = IDC_CFG_SAVE_PROFILE;
	mmCallBack.pCallerClass = this;
	mmCallBack.pfnCallback = CStBinderDlg::OnControlMouseMove;

	m_saveprofile_btn.SetMouseMoveCallback( mmCallBack );


	PopulateResourceTree();
	// Allow files dragged to the resource group
    //
    // Set the drop mode for the resource tree
    //
    CStResGrpTreeCtrl::DROPTREEMODE dropTreeMode;

    dropTreeMode.pfnCallback = CStBinderDlg::OnTreeFileDropped;
	dropTreeMode.pCallerClass = this;
    m_ResourceTree.SetDropMode(dropTreeMode);
	m_ResourceTree.DragAcceptFiles(TRUE);
	m_ResourceTree.EnableWindow(FALSE);

	mmCallBack.id = IDC_RESOURCETREE;
	mmCallBack.pCallerClass = this;
	mmCallBack.pfnCallback = CStBinderDlg::OnControlMouseMove;

	m_ResourceTree.SetMouseMoveCallback( mmCallBack );

	//
	// Insert columns into the list control
	//
	// What size do we make them?
	//
	// Need to take account of the width of the vertical scrollbar
	// that windows inserts if the number of items exceeds what can
	// be displayed.  This prevents the ugly horizontal scrollbar from appearing.
	//
	RECT listRect;
	m_ResourceListCtl.GetClientRect(&listRect);
	int nListWidth = (listRect.right - listRect.left);
	int nVScrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL); 
	
	int nCol1Width = (int) ((double)nListWidth * 0.45);  // path
//	int nCol2Width = (int) ((double)nListWidth * 0.08);  // tag
    int nCol2Width = (int) ((double)nListWidth * 0.15);  // version
    int nCol3Width = (int) ((double)nListWidth * 0.15);  // size
    int nCol4Width = nListWidth - nCol1Width - nCol2Width - nCol3Width - nVScrollBarWidth;  // date

	((CStBinderApp*)AfxGetApp())->GetString(IDS_COLUMN_PATHNAME, resStr);
	m_ResourceListCtl.InsertColumn(COLUMN_PATHNAME, resStr, LVCFMT_LEFT, nCol1Width);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_COLUMN_VERSION, resStr);
	m_ResourceListCtl.InsertColumn(COLUMN_VERSION, resStr, LVCFMT_LEFT, nCol2Width);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_COLUMN_SIZE, resStr);
	m_ResourceListCtl.InsertColumn(COLUMN_SIZE, resStr, LVCFMT_LEFT, nCol3Width);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_COLUMN_TIMESTAMP, resStr);
	m_ResourceListCtl.InsertColumn(COLUMN_DATE, resStr, LVCFMT_LEFT, nCol4Width);


	//
	// Set full row selection
	//
	m_ResourceListCtl.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    //
    // Set the drop mode
    //
    CFileDropListCtrl::DROPLISTMODE dropMode;

    dropMode.iMask = CFileDropListCtrl::DL_ACCEPT_FILES | 
                      CFileDropListCtrl::DL_FILTER_EXTENSION |
                      CFileDropListCtrl::DL_USE_CALLBACK;
    dropMode.csFileExt = _T(".inf, .cat, .sys");
    //    dropMode.csFileExt = _T(".sb, .rsc, .bin");
    dropMode.pfnCallback = CStBinderDlg::OnListFileDropped;
	dropMode.pCallerClass = this;
    m_ResourceListCtl.SetDropMode(dropMode);
	m_ResourceListCtl.EnableWindow(FALSE);

	mmCallBack.id = IDC_BINARIES_LIST;
	mmCallBack.pCallerClass = this;
	mmCallBack.pfnCallback = CStBinderDlg::OnControlMouseMove;

	m_ResourceListCtl.SetMouseMoveCallback( mmCallBack );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_RESLIST_DESC, m_ResListDesc);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_RESTREE_DESC, m_ResTreeDesc);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_FIND_TARGET_DESC, m_FindTargetDesc);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_CFG_UPDATER_DESC, m_ConfigureDesc);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_CFG_LOADPROFILE_DESC, m_LoadProfileDesc);
	((CStBinderApp*)AfxGetApp())->GetString(IDS_CFG_SAVEPROFILE_DESC, m_SaveProfileDesc);

	GetLastPaths();

//	CStSplashWnd::CloseSplashScreen();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CStBinderDlg::PopulateResourceTree()
{
	HTREEITEM hItem;
	CString resStr;

	DWORD dwStyle = GetWindowLong(m_ResourceTree.GetSafeHwnd(), GWL_STYLE);
	dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
	SetWindowLong(m_ResourceTree.GetSafeHwnd(), GWL_STYLE, dwStyle);

	// Clear the list
	m_ResourceTree.DeleteAllItems();

	// creating image list and put it into the tree control
	//====================================================//
	CBitmap bMap;
	m_ImageList.Create(16,16,ILC_COLOR16,2,10);
	bMap.LoadBitmap(MAKEINTRESOURCE(IDB_CLOSEDFOLDER));
	m_ImageList.Add(&bMap, (CBitmap *)NULL);
	bMap.DeleteObject();

	bMap.LoadBitmap(MAKEINTRESOURCE(IDB_OPENFOLDER));
	m_ImageList.Add(&bMap, (CBitmap *)NULL);
	bMap.DeleteObject();

	m_ResourceTree.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_ResourceTree.SetImageList(&m_ImageList, LVSIL_NORMAL);

	// Create the root level
	((CStBinderApp*)AfxGetApp())->GetString(IDS_RESOURCES, resStr);
	m_hRoot=(HTREEITEM)m_ResourceTree.InsertItem(resStr, 0, 1);
 	m_ResourceTree.SetItemData(m_hRoot, (DWORD_PTR) 0);

	// Add Recovery Mode Driver
	((CStBinderApp*)AfxGetApp())->GetString(IDS_GRP_RECV_DRIVER, resStr);
	m_hRecvResources=(HTREEITEM)m_ResourceTree.InsertItem(resStr, 0, 1, m_hRoot);
	CResourceList *pRecvGroupResList = new CResourceList ( resStr );
	pRecvGroupResList->SetLangID(MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
	m_ResourceTree.SetItemData(m_hRecvResources, (DWORD_PTR)pRecvGroupResList);

	// Add Firmware
	((CStBinderApp*)AfxGetApp())->GetString(IDS_GRP_FIRMWARE, resStr);
	m_hFirmwareResources=(HTREEITEM)m_ResourceTree.InsertItem(resStr, 0, 1, m_hRoot);
 	m_ResourceTree.SetItemData(m_hFirmwareResources, (DWORD_PTR) 0);

	// Under Firmware add each specific language
	for (int i = 0; i < g_LangIdCount; ++i)
	{
		TCHAR szLocaleName[64];
		CString strFmt;

		if ( GetLocaleInfo(g_LangIds[i], LOCALE_SLANGUAGE, szLocaleName, 64) )
		{
			strFmt.Format(_T("%s"), szLocaleName);
			hItem = m_ResourceTree.InsertItem(strFmt, m_hFirmwareResources);
			CResourceList *pGroupResList = new CResourceList( strFmt );
			pGroupResList->SetLangID( g_LangIds[i] );
			m_ResourceTree.SetItemData(hItem, (DWORD_PTR)pGroupResList);
		}
	}

	// Add the "other firmware" entry
	((CStBinderApp*)AfxGetApp())->GetString(IDS_GRP_FIRMWARE_OTHER, resStr);
    m_hOtherFirmware = m_ResourceTree.InsertItem(resStr,m_hFirmwareResources);
	m_ResourceTree.SetItemData(m_hOtherFirmware, (DWORD_PTR)0);

}


void CStBinderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CString descStr, verStr, copyRightStr;
		((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_ABOUT_VERSION, descStr);
		verStr = CString(BINDER_STRING_VERSION);
		((CStBinderApp*)AfxGetApp())->GetString(IDS_DLG_ABOUT_COPYRIGHT, copyRightStr);

		CStSplashWnd::ShowSplashScreen(4000, IDB_SPLASH, descStr, verStr, copyRightStr, this);
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CStBinderDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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

BOOL CStBinderDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Speziellen Code hier einfügen und/oder Basisklasse aufrufen

// ### TO_EDIT:
// Route messages to the splash screen while it is visible
    if (CStSplashWnd::PreTranslateAppMessage(pMsg)) 
		{
			return TRUE;
		}
	
	return CDialog::PreTranslateMessage(pMsg);
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CStBinderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CStBinderDlg::OnBnClickedOk()
{
    GetDlgItem(IDOK)->EnableWindow(FALSE);
    GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
	m_loadprofile_btn.EnableWindow(FALSE);
	m_saveprofile_btn.EnableWindow(FALSE);
	m_configure_btn.EnableWindow(FALSE);

	m_Busy = TRUE;

	ApplyResources();  

	UpdateTargetInfo();

	// clear any current profile
	g_ResCfgData.ProfileName.Empty();

	m_status_text.SetWindowText(_T(""));

	m_Busy = FALSE;

    GetDlgItem(IDOK)->EnableWindow(TRUE);
    GetDlgItem(IDCANCEL)->EnableWindow(TRUE);
	m_configure_btn.EnableWindow(TRUE);
	m_loadprofile_btn.EnableWindow(TRUE);
	m_saveprofile_btn.EnableWindow(TRUE);

	m_progress_ctrl.SetPos( 0 );

}


void CStBinderDlg::OnBnClickedCancel()
{
	HTREEITEM hItem = m_hRecvResources;
    CResourceList *pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

	if ( pGroupResList )
		delete pGroupResList;

	hItem = m_ResourceTree.GetChildItem(m_hFirmwareResources);
	do
	{
		pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);
		if ( pGroupResList )
			delete pGroupResList;

		hItem = m_ResourceTree.GetNextItem(hItem, TVGN_NEXT);
	}
	while ( hItem != NULL );

	if (m_pUpdaterVersion)
	{
		delete (m_pUpdaterVersion);
		m_pUpdaterVersion = NULL;
	}

	if (g_ResCfgData.pUpdaterGrpIconHeader)
	{
		VirtualFree(g_ResCfgData.pUpdaterGrpIconHeader, 0, MEM_RELEASE);
		g_ResCfgData.pUpdaterGrpIconHeader = NULL;
	}
	if (g_ResCfgData.pCompanyGrpIconHeader)
	{
		VirtualFree(g_ResCfgData.pCompanyGrpIconHeader, 0, MEM_RELEASE);
		g_ResCfgData.pCompanyGrpIconHeader = NULL;
	}

	SaveLastPaths();

    OnCancel();
}


void CStBinderDlg::OnBnClickedFindTarget()
{
	CString resStr;


	((CStBinderApp*)AfxGetApp())->GetString(IDS_FILTER_FIRMWARE_UPDATER, resStr);
	resStr.Append(_T(" (*.exe)|*.exe||)"));
	CFileDialog dlg ( TRUE, _T(".\\*.exe"), _T(""), OFN_FILEMUSTEXIST,
						resStr, this );

    dlg.m_pOFN->lpstrInitialDir = m_LastUpdaterPath;

    if ( IDOK == dlg.DoModal() )
    {
		HTREEITEM hItem = m_hRoot;

		// clear any current profile
		g_ResCfgData.ProfileName.Empty();

		m_configure_btn.EnableWindow(FALSE);
		m_loadprofile_btn.EnableWindow(FALSE);
		m_saveprofile_btn.EnableWindow(FALSE);

	    m_ResourceListCtl.DeleteAllItems();

		do
		{
			if ( m_ResourceTree.GetItemData(hItem) > 0 )
			{
			    CResourceList *pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

				if ( pGroupResList )
					pGroupResList->EmptyTheList();
			}
			hItem = m_ResourceTree.GetNextItem(hItem, TVGN_NEXTVISIBLE);
		}
		while ( hItem != NULL );

		m_total_file_count = 0;
		m_bound_resource_count = 0;
	    m_total_files.SetWindowText(_T("0"));

		resStr = dlg.GetPathName();
	    m_target_name.SetWindowText( dlg.GetPathName() );

		int i = resStr.ReverseFind('\\');
		resStr.SetAt(i, '\0');

	    CheckForExistingResources();

		if ( !UpdateTargetInfo() )
		{
		    m_target_name.SetWindowText( _T("") );
		}
		else
		{
			m_ResourceTree.EnableWindow(TRUE);
			m_ResourceListCtl.EnableWindow(TRUE);
			m_LastUpdaterPath = resStr;
			if (CheckProfileCount() > 0)
				m_loadprofile_btn.EnableWindow(TRUE);
			m_saveprofile_btn.EnableWindow(TRUE);
		}
    }

	delete dlg;
}


BOOL CStBinderDlg::UpdateTargetInfo()
{
	TCHAR szTargetName[MAX_PATH];
    CString countStr, csText, csOriginalName, resStr;

	GetDlgItem(IDC_DLG_TARGET_VERSION_TEXT)->EnableWindow(FALSE);
	GetDlgItem(IDC_DLG_TARGET_BASESDK_TEXT)->EnableWindow(FALSE);
	GetDlgItem(IDC_DLG_TARGET_DATE_TEXT)->EnableWindow(FALSE);
	GetDlgItem(IDC_DLG_TARGET_SIZE_TEXT)->EnableWindow(FALSE);
	GetDlgItem(IDC_DLG_TARGET_RESOURCES_TEXT)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_STATUS_UPDATING_TARGET_INFO, resStr);
	m_status_text.SetWindowText(resStr);

	m_updater_version.SetWindowText(_T(""));
	m_updater_base_SDK.SetWindowText(_T(""));
	m_updater_date.SetWindowText(_T(""));
	m_updater_filesize.SetWindowText(_T(""));
	m_target_name.GetWindowText(szTargetName, MAX_PATH);
	csText = _T("");

	if (m_pUpdaterVersion)
	{
		delete (m_pUpdaterVersion);
		m_pUpdaterVersion = NULL;
	}

	m_pUpdaterVersion = new CStVersion(szTargetName);

	m_pUpdaterVersion->StGetOriginalFileName(csOriginalName);

	if ( csOriginalName != _T("StUpdaterApp.exe") )
	{
		((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_ORIG_NAME, resStr);

		MessageBox(resStr, CAPTION_TEXT, MB_OK);
		m_status_text.SetWindowText(_T(""));
		m_target_name.SetWindowText(_T(""));
		return FALSE;
	}

	if ( g_ResCfgData.Options.UpdMajorVersion <= 1 && g_ResCfgData.Options.UpdMinorVersion < FULL_CFG_RESOURCES_RELEASE )
	{
		((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_EARLIER_UPD_VERSION, resStr);
		MessageBox(resStr, CAPTION_TEXT, MB_OK);
		m_status_text.SetWindowText(_T(""));
		m_target_name.SetWindowText(_T(""));
		return FALSE;
	}
	else
		csText.Format(_T("%d.%d.%d.%d"), g_ResCfgData.Options.UpdMajorVersion,
										g_ResCfgData.Options.UpdMinorVersion,
										g_ResCfgData.Options.ProdMajorVersion,
										g_ResCfgData.Options.ProdMinorVersion);


	m_updater_version.SetWindowText(csText);
	m_pUpdaterVersion->StGetFileDate(csText);
	m_updater_date.SetWindowText(csText);
	m_updater_base_SDK.SetWindowText(g_ResCfgData.Options.BaseSDK);
	m_pUpdaterVersion->StGetFileSize(csText);
	m_updater_filesize.SetWindowText(csText);

    countStr.Format(_T("%d"), m_bound_resource_count);
    m_bound_resources.SetWindowText(countStr);

	if ( (m_total_file_count && _tcslen(szTargetName)) || m_LocalSelectionChanged )
	    GetDlgItem(IDOK)->EnableWindow(TRUE);
	else
	    GetDlgItem(IDOK)->EnableWindow(FALSE);

	m_status_text.SetWindowText(_T(""));

	GetDlgItem(IDC_DLG_TARGET_VERSION_TEXT)->EnableWindow(TRUE);
	GetDlgItem(IDC_DLG_TARGET_BASESDK_TEXT)->EnableWindow(TRUE);
	GetDlgItem(IDC_DLG_TARGET_DATE_TEXT)->EnableWindow(TRUE);
	GetDlgItem(IDC_DLG_TARGET_SIZE_TEXT)->EnableWindow(TRUE);
	GetDlgItem(IDC_DLG_TARGET_RESOURCES_TEXT)->EnableWindow(TRUE);
	GetDlgItem(IDC_DLG_FILE_COUNT_TEXT)->EnableWindow(TRUE);
	m_configure_btn.EnableWindow(TRUE);
	m_loadprofile_btn.EnableWindow(TRUE);
	m_saveprofile_btn.EnableWindow(TRUE);

	return TRUE;
}

//
// Callback from CFileDropListCtrl
//
HRESULT CStBinderDlg::OnListFileDropped(CListCtrl* pList,
								     PVOID pCallerClass,
                                     CString& csPathname,
                                     UINT& iPathType)
{
    if(CFileDropListCtrl::DL_FILE_TYPE == iPathType)
    {
		TCHAR szTargetName[MAX_PATH];
		CStBinderDlg *pThisClass = ( CStBinderDlg * )pCallerClass;

		pThisClass->m_target_name.GetWindowText(szTargetName, MAX_PATH);

		if ( _tcslen(szTargetName) )
	        InsertFileIntoList(NULL, pList, pCallerClass, csPathname);
		else
			MessageBeep(-1);
    }
    return S_OK;
UNREFERENCED_PARAMETER (pList);
}

//
// Callback from CResGrpTreeCtrl
//
HRESULT CStBinderDlg::OnTreeFileDropped(CTreeCtrl* pTree,
								     PVOID pCallerClass,
									 HDROP dropInfo)
{
	CStBinderDlg *pThisClass = ( CStBinderDlg * )pCallerClass;
	HTREEITEM hDropItem = pTree->GetDropHilightItem();
	if ( hDropItem && hDropItem != pTree->GetSelectedItem() )
		pTree->SelectItem(hDropItem);
	if ( hDropItem )
	{
		pThisClass->m_ResourceListCtl.TreeDropFiles(dropInfo);
		pThisClass->m_ResourceTree.SetItemState(hDropItem, 0, TVIS_DROPHILITED);
	}

	return S_OK;
}

void CStBinderDlg::OnLvnInsertitemBinariesList(NMHDR *pNMHDR, LRESULT *pResult)
{
    CString countStr;
	TCHAR szTargetName[MAX_PATH];
    *pResult = 0;

   // ++m_total_file_count;
	m_target_name.GetWindowText(szTargetName, MAX_PATH);

	if ( m_total_file_count > 0 && _tcslen(szTargetName) )
	    GetDlgItem(IDOK)->EnableWindow(TRUE);

    countStr.Format(_T("%d"), m_total_file_count);
    m_total_files.SetWindowText(countStr);
}


void CStBinderDlg::OnMenuClickedBrowse( HTREEITEM hCurrentItem )
{
    CFileDialog *dlg;
	CString resStr;

    // for recovery mode resource use the .sys, .inf, and .cat extensions for stmp3rec.*
    if ( hCurrentItem == m_hRecvResources )
	{
		((CStBinderApp*)AfxGetApp())->GetString(IDS_FILTER_RECV_DRIVER, resStr);
		resStr.Append(_T(" (*.sys;*.inf;*.cat)|*.sys;*.inf;*.cat||)"));

    	dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST,
						resStr, this);
	    dlg->m_pOFN->lpstrInitialDir = m_LastRecoveryDriverPath;

	}
    else
	{
        // use firmware file extensions
		((CStBinderApp*)AfxGetApp())->GetString(IDS_FILTER_FIRMWARE, resStr);
		resStr.Append(_T("(*.sb;*.bin;*.rsc)|*.sb;*.bin;*.rsc||)"));

		dlg = new CFileDialog ( TRUE, NULL, NULL, OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST,
						resStr, this );
	    dlg->m_pOFN->lpstrInitialDir = m_LastFirmwarePath;
	}



    if ( IDOK == dlg->DoModal() )
    {
        CString csPathName = dlg->GetPathName();
        POSITION startPosition = dlg->GetStartPosition();
		int count=0;

        while (startPosition)
        {
            CString csFullPathName = dlg->GetNextPathName(startPosition);
            InsertFileIntoList (hCurrentItem, &m_ResourceListCtl, this, csFullPathName);
			++count;
        }

		if (count == 1)
		{	// if only one file specified, the path includes the target
			int i = csPathName.ReverseFind('\\');
			csPathName.SetAt(i, '\0');
		}

		if ( hCurrentItem == m_hRecvResources )
			m_LastRecoveryDriverPath = csPathName;
		else
			m_LastFirmwarePath = csPathName;
    }

	delete dlg;
}

void CStBinderDlg::InsertFileFromProfile(HTREEITEM _hItem,
                                      CString& _csPathname,
									  PVOID pCallerClass)
{
	CStBinderDlg *pThisClass = ( CStBinderDlg * )pCallerClass;
	InsertFileIntoList(_hItem, &pThisClass->m_ResourceListCtl, pThisClass, _csPathname);
}

void CStBinderDlg::InsertFileIntoList(HTREEITEM _hItem,
									  CListCtrl* pList,
									  PVOID pCallerClass,
                                      CString& _csPathname)
{
   DWORD dwFileSize = 0;
   SYSTEMTIME sysLocal = {0};
   CStBinderDlg *pThisClass = ( CStBinderDlg * )pCallerClass;
   HANDLE hFile;
   CString csText, csSize, csTime;
   CString csVersion;
   int nItem;
   int nextIndex = pList->GetItemCount();
   HTREEITEM hInsertGroup = NULL;
   CResourceList *pGroupResList = NULL;

   if ( _hItem == NULL )
   {
	   hInsertGroup = pThisClass->m_ResourceTree.GetDropHilightItem();
	   if ( hInsertGroup == NULL )
		   hInsertGroup = pThisClass->m_ResourceTree.GetSelectedItem();
   }
   else
	   hInsertGroup = _hItem;

   if ( hInsertGroup )
		pGroupResList = (CResourceList *)pThisClass->m_ResourceTree.GetItemData(hInsertGroup);

   if ( !pGroupResList || pThisClass->CheckForDuplicateOrInvalidFilename(_hItem, _csPathname ) )
   {
	   MessageBeep(-1);
	   return;  // already have one of those
   }


    csVersion = _T("");

	// For the executables, we can't get version info when we have an open handle so do it now.
	if ( hInsertGroup == pThisClass->m_hRecvResources )
	{
		TCHAR szPathname[MAX_PATH];
		_tcscpy(szPathname, _csPathname);
		CStVersion *pStmp3RecVersion = new CStVersion(szPathname);

		pStmp3RecVersion->StGetFileVersion(csVersion);

		delete pStmp3RecVersion;
	}

    if ( csVersion.IsEmpty() || csVersion == _T("0.0.0.0") )
        csVersion = _T(" --- ");

    // Get its size
    // returns DWORD in reality
	hFile = CreateFile (
			          _csPathname,
				      GENERIC_READ,// | GENERIC_WRITE,
					  FILE_SHARE_READ,// | FILE_SHARE_WRITE,
	                  NULL, // no SECURITY_ATTRIBUTES structure
		              OPEN_EXISTING, // No special create flags
			          0, // No special attributes
				      NULL); // No template file

    if ( hFile != INVALID_HANDLE_VALUE )
	{
		++pThisClass->m_total_file_count; // this has to go here before we insert into the list
										 // so that the displayed count is updated correctly in
										 // OnLvnInsertitemBinariesList()
		dwFileSize = GetFileSize(hFile, NULL); 
        if ( dwFileSize != INVALID_FILE_SIZE )
	    {
		    csSize.Format(_T("%d"), dwFileSize);
        }

	    BY_HANDLE_FILE_INFORMATION fileInfo;
		if ( GetFileInformationByHandle(hFile, &fileInfo) )
        {
//	        SYSTEMTIME sysTime;

			if ( FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &sysLocal) )
	        {
				WCHAR wszTime[32];
				WCHAR wszDate[32];
				WCHAR wszFmt[32];

				GetLocaleInfo( LOCALE_USER_DEFAULT,				// locale identifier
							LOCALE_STIMEFORMAT,					// information type
							(LPWSTR)wszFmt,		// information buffer
							32);								// size of buffer
				GetTimeFormatW(  LOCALE_USER_DEFAULT, 0, &sysLocal, wszFmt,
									wszTime,         // formatted string buffer
									32);

				GetLocaleInfo( LOCALE_USER_DEFAULT,				// locale identifier
							LOCALE_SSHORTDATE,					// information type
							(LPWSTR)wszFmt,		// information buffer
							32);								// size of buffer
				GetDateFormatW(  LOCALE_USER_DEFAULT, 0, &sysLocal, wszFmt,
									wszDate,         // formatted string buffer
									32);

				csTime.Format(_T("%s  %s"), wszDate, wszTime);

				if ( hInsertGroup != pThisClass->m_hRecvResources )
				{
		            // extract version from file
			        CStFwVersion *pFwVersion = new CStFwVersion(_csPathname, NULL, 0);

					if ( pFwVersion )
					{
						pFwVersion->GetComponentVersion(csVersion);
						delete pFwVersion;
					}
				}
			}
		}

		if ( hInsertGroup == pThisClass->m_ResourceTree.GetSelectedItem() )
		{
			// Insert the filename in column 1
			nItem = pList->InsertItem(nextIndex, _csPathname);
			pList->SetItemText(nItem, COLUMN_SIZE, csSize);
            pList->SetItemText(nItem, COLUMN_DATE, csTime);
			pList->SetItemText(nItem, COLUMN_VERSION, csVersion);
		}
		else
		{
			CString countStr;
			countStr.Format(_T("%d"), pThisClass->m_total_file_count);
			pThisClass->m_total_files.SetWindowText(countStr);
		}

		if ( pGroupResList )
		{
			USHORT index;
	    	int posFname;
		   	int iResId;
		    CString fileName;
			USHORT uTagId = 0;

			// get filename; get resource id
			posFname = _csPathname.ReverseFind(0x5c);

			fileName = _csPathname.Tokenize(_T("\\"), posFname);

			iResId = pThisClass->GetResourceID(fileName);

			if ( iResId < 0 )
				iResId = pGroupResList->NextAvailableResourceID();

			index = pGroupResList->AddResource(_csPathname, dwFileSize, csVersion, uTagId, sysLocal);

			if ( index != NO_INDEX )
			{
		        pGroupResList->SetResId(index, iResId);

				if ( pThisClass->CheckForReplacements(iResId, fileName, TRUE) )
				    pThisClass->RefreshResourceView(FALSE);
			}
	    }

		// fix up the resource group name to prefix the count
		CString csResName;
		pGroupResList->GetResourceGroupName(csResName);
		if ( pGroupResList->GetCount() )
			csText.Format(_T("(%d) %s"), pGroupResList->GetCount(), csResName.GetString());
		else
			csText = csResName;
		pThisClass->m_ResourceTree.SetItemText(hInsertGroup, csText);

	    CloseHandle(hFile);
	}
	else
	{	// file not found
		CString csErrMsg, csErrFmt;
		csErrFmt.LoadStringW(IDS_PROFILE_MISSING_FILE);
		csErrMsg.Format(csErrFmt, _csPathname);
		MessageBeep(-1);
		pThisClass->MessageBox(csErrMsg, CAPTION_TEXT, MB_OK);
	}
}

void CStBinderDlg::InsertBoundResourceIntoList(HTREEITEM _hItem, USHORT _iResId, CString _csResName, DWORD _dwSize, CString _szVersion, USHORT _tagId)
{
    CString csResName, csFormattedName;
    CResourceList *pGroupResList;

	if ( _iResId >= IDR_STMP3REC_SYS )
	    // get name from resource id
	    GetResourceName(_iResId, csResName);
	else
		csResName = _csResName;

    csFormattedName.Format(_T("< %s >"), csResName.GetString());

    pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(_hItem);

    if ( pGroupResList != NULL )
	{
	    ++m_bound_resource_count;
		pGroupResList->AddResource(_csResName, csFormattedName, _dwSize, _szVersion, _tagId, _iResId);

		// fix up the resource group name to prefix the count
		pGroupResList->GetResourceGroupName(csResName);
		csFormattedName.Format(_T("(%d) %s"), pGroupResList->GetCount(), csResName.GetString());
		m_ResourceTree.SetItemText(_hItem, csFormattedName);
	}
}


void CStBinderDlg::OnTvnSelchangedResourceTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	RefreshResourceView(FALSE);

	*pResult = 0;
}


void CStBinderDlg::RefreshResourceView(BOOL _bRefreshAllGroups)
{
	HTREEITEM hItem = m_ResourceTree.GetSelectedItem();

    CResourceList *pGroupResList = NULL;
	
	if ( hItem == m_hOtherFirmware )
	{
		// launch dialog to get language IDs
		LANGID langID = 0xFFFF;
		COtherFirmwareLANGID	*dlg = new COtherFirmwareLANGID( &langID );
		CString resStr;

	    dlg->DoModal();

		delete (dlg);

		if ( langID != 0xFFFF )
		{
			TCHAR szNewFwGroup[64];
			CString insertStr;

			GetLocaleInfo(langID, LOCALE_SLANGUAGE, szNewFwGroup, 64);
			insertStr.Format(_T("%s"), szNewFwGroup);
			// insert new item before the "other firmware" item
			hItem = m_ResourceTree.InsertItem(insertStr, m_hFirmwareResources,
											m_ResourceTree.GetPrevSiblingItem(m_hOtherFirmware));
			if ( hItem )
			{
				m_ResourceTree.SetItemState(m_hOtherFirmware, 0, TVIS_DROPHILITED);
				m_ResourceTree.SelectItem(hItem);
				m_ResourceTree.SetItemState(hItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
				CResourceList *pNewGroupResList = new CResourceList( insertStr );
				pNewGroupResList->SetLangID( langID );
				m_ResourceTree.SetItemData(hItem, (DWORD_PTR)pNewGroupResList);

				// save it to the global list
				LANGID *pTmp = (LANGID *) VirtualAlloc(NULL, (g_LangIdCount+1) * sizeof(LANGID), MEM_COMMIT, PAGE_READWRITE);
				if ( pTmp )
				{
					memcpy (pTmp, g_LangIds, g_LangIdCount * sizeof(LANGID));
					VirtualFree (g_LangIds, 0, MEM_RELEASE);
					g_LangIds = pTmp;
					pTmp += g_LangIdCount;
					*pTmp = langID;
					++g_LangIdCount;
				}
			}
		}
		else
			m_ResourceTree.SelectItem( m_hFirmwareResources );
	}


	if ( hItem )
	{
		// clear current list contents
		m_ResourceListCtl.DeleteAllItems();
		// get selected	 group resources
		pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);
	}

    if ( pGroupResList != NULL )
    {
        DWORD dwFileSize;
        SYSTEMTIME sysLocal;
        CString pathName;
        CString csText;
        CString csVersion;
        USHORT uTagId;
		CString csResStr;

        USHORT index = 0;

        while (pGroupResList->GetAtIndex(index, pathName, dwFileSize,csVersion, uTagId, sysLocal))
        {
            int nItem;
                
            if ( pGroupResList->IsMarkedForDeletion(index) )
			{
				((CStBinderApp*)AfxGetApp())->GetString(IDS_RES_DELETE, csResStr);
				pathName.Append(_T(" "));
                pathName.Append(csResStr);
			}

            if ( pGroupResList->IsMarkedForReplacement(index) )
			{
				((CStBinderApp*)AfxGetApp())->GetString(IDS_RES_REPLACE, csResStr);
				pathName.Append(_T(" "));
                pathName.Append(csResStr);
			}

            nItem = m_ResourceListCtl.InsertItem(m_ResourceListCtl.GetItemCount(), pathName);

            if ( csVersion.IsEmpty() )
                csVersion = _T(" --- ");
            m_ResourceListCtl.SetItemText(nItem, COLUMN_VERSION, csVersion);

            csText.Format(_T("%d"), dwFileSize);
            m_ResourceListCtl.SetItemText(nItem, COLUMN_SIZE, csText);

            if ( !pGroupResList->IsBoundResource( index ) )
                csText.Format(_T("%02d/%02d/%d  %02d:%02d"),
                                sysLocal.wDay, sysLocal.wMonth, sysLocal.wYear,
                                sysLocal.wHour, sysLocal.wMinute);
            else
				((CStBinderApp*)AfxGetApp())->GetString(IDS_RES_UNKNOWN, csText);
       
            m_ResourceListCtl.SetItemText(nItem, COLUMN_DATE, csText);

			++index;
        }

	    //
		// Set the drop mode
	    //
		CFileDropListCtrl::DROPLISTMODE dropMode;

	    dropMode.iMask = CFileDropListCtrl::DL_ACCEPT_FILES | 
		                  CFileDropListCtrl::DL_FILTER_EXTENSION |
			              CFileDropListCtrl::DL_USE_CALLBACK;

		if ( hItem == m_hRecvResources )
		    dropMode.csFileExt = _T(".inf, .cat, .sys, .INF, .CAT, .SYS");
		else
		    dropMode.csFileExt = _T(".sb, .rsc, .bin, .SB, .RSC, .BIN");

		dropMode.pCallerClass = this;
		dropMode.pfnCallback = CStBinderDlg::OnListFileDropped;

	    m_ResourceListCtl.SetDropMode(dropMode);

		if ( !_bRefreshAllGroups )
		{
			// fix up the resource group name to prefix the count
			pGroupResList->GetResourceGroupName(csResStr);
			if ( pGroupResList->GetCount() )
				csText.Format(_T("(%d) %s"), pGroupResList->GetCount(), csResStr.GetString());
			else
				csText = csResStr;
			m_ResourceTree.SetItemText(hItem, csText);
		}
    }

	if ( _bRefreshAllGroups )
	{
		hItem = m_hRoot;
		do
		{
			pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);
			if ( pGroupResList )
			{
				CString csResStr, csText;
				pGroupResList->GetResourceGroupName(csResStr);
				if ( pGroupResList->GetCount() )
					csText.Format(_T("(%d) %s"), pGroupResList->GetCount(), csResStr.GetString());
				else
					csText = csResStr;
				m_ResourceTree.SetItemText(hItem, csText);
			}
			hItem = m_ResourceTree.GetNextItem(hItem, TVGN_NEXTVISIBLE);
		}
		while ( hItem != NULL );
	}

}


BOOL CStBinderDlg::CheckForReplacements(int _iResId, CString _csFileName, BOOL _state)
{
    CResourceList *pGroupResList = NULL;
    USHORT index = 0;
    BOOL returnStatus = FALSE;
	HTREEITEM hItem = m_ResourceTree.GetSelectedItem();

	if (hItem)
	    pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

	if ( !pGroupResList )
		return returnStatus;

    while ( index < pGroupResList->GetCount() )
    {
		if ( pGroupResList->IsBoundResource(index) )
		{
			if ( (_iResId != IDR_RESID_UNKNOWN && pGroupResList->GetResId(index) == _iResId ) ||
				( _csFileName == pGroupResList->GetFileName(index) ))
			{
				pGroupResList->SetForReplacement(index, _state);
                returnStatus = TRUE;
	            break;
		    }
		}
        ++index;
    }

    return returnStatus;
}


BOOL CStBinderDlg::UserConfirmation()
{
    int iReplacementCount = 0;
    BOOL returnStatus = TRUE;
	PCONFIRM_LIST pConfirmList = NULL, pLast = NULL;
	HTREEITEM hItem = m_hRoot;

	do 
	{
	    CResourceList *pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

		if ( pGroupResList )
		{
   			USHORT index = 0;
            USHORT count = pGroupResList->GetCount();
	        while ( index < count )
		    {
			    if ( pGroupResList->IsBoundResource( index ) &&
				    (pGroupResList->IsMarkedForReplacement( index ) ||
					 pGroupResList->IsMarkedForDeletion( index )) )
                {
	                int iResId;
		            CString csItemName, csGroupText;
					PCONFIRM_LIST pItem;

				    iResId = pGroupResList->GetResId(index);

					if ( iResId >= IDR_STMP3REC_SYS )
						GetResourceName(iResId, csItemName);
					else
						csItemName = pGroupResList->GetFileName(index);

					pGroupResList->GetResourceGroupName(csGroupText);

					pItem = new CONFIRM_LIST;
					if ( pItem )
					{
						pItem->csResGroup = csGroupText;
						pItem->csResName = csItemName;
						pItem->pNext = NULL;

						if ( !pConfirmList )
							pConfirmList = pItem;
						else
							pLast->pNext = pItem;

						pLast = pItem;
					}
           
	                ++iReplacementCount;
		        }

			    ++index;
			}
		}

		hItem = m_ResourceTree.GetNextItem(hItem, TVGN_NEXTVISIBLE);
	}
	while ( hItem != NULL );

    if (iReplacementCount)
    {
		CUserConfirmation	*dlg = new CUserConfirmation( pConfirmList );

	    if ( dlg->DoModal() != IDOK )
			returnStatus = FALSE;

		delete (dlg);
    }

    return returnStatus;
}


BOOL CStBinderDlg::DeleteMarkedResources( HANDLE _hUpdateRes )
{
	HTREEITEM hItem = m_hRoot;

	do
	{
		CResourceList *pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

		if ( pGroupResList )
		{
   			USHORT index = 0;
			USHORT i = 0;
	        USHORT count = pGroupResList->GetCount();
		    while ( i < count )
			{
				if ( pGroupResList->IsBoundResource( index ) &&
					( pGroupResList->IsMarkedForDeletion( index ) ||
                      pGroupResList->IsMarkedForReplacement( index )) )
	            {
		            int iResId = pGroupResList->GetResId(index);
					CString csItemName = pGroupResList->GetFileName(index);
					BOOL result;

					if ( iResId >= IDR_STMP3REC_SYS )
	    				result = UpdateResource(_hUpdateRes,       // update resource handle 
		    							L_STMP_RECV_RESTYPE,           // resource type
			    						MAKEINTRESOURCE(iResId),     // resource name 
										pGroupResList->GetLangID(),  // language
						    			NULL,                   // NULL to remove the resource 
							    		0); // size of resource info. 
					else
						result = UpdateResource(_hUpdateRes,       // update resource handle 
		    							L_STMP_FW_RESTYPE,             // resource type
			    						MAKEINTRESOURCE(iResId),     // resource name 
										pGroupResList->GetLangID(),  // language
					    				NULL,                   // NULL to remove the resource 
						    			0); // size of resource info. 

    				if (result == FALSE)
	                    return FALSE;

		            --m_bound_resource_count;
					//pGroupResList->FreeResourceID(iResId);
				    pGroupResList->SetBoundResource(index, FALSE);
					pGroupResList->RemoveResource(index);
					UpdateProgress(TRUE);

					// don't bump the index because we just took one out.
		        }
				else
					++index;
				++i;
	        }
		}

		hItem = m_ResourceTree.GetNextItem(hItem, TVGN_NEXTVISIBLE);
    }
	while ( hItem != NULL );

    return TRUE;
}

USHORT CStBinderDlg::GetNumberOfOpsToPerform()
{
	USHORT count = 0;
	HTREEITEM hItem = m_hRoot;

	do
	{
	    CResourceList *pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

		if ( pGroupResList )
		{
			int index = 0;

			while ( index < pGroupResList->GetCount() )
			{
			    if ( pGroupResList->IsMarkedForDeletion( index ) ||
					pGroupResList->IsMarkedForReplacement( index ) ||
	               !pGroupResList->IsBoundResource( index ) )
					++count;

				++index;
			}
		}

		hItem = m_ResourceTree.GetNextItem(hItem, TVGN_NEXTVISIBLE);
    }
	while ( hItem != NULL );

	return count;
}


BOOL CStBinderDlg::ApplyResources()
{
	HANDLE hUpdateRes;  // update target resource handle 
	TCHAR szTargetName[MAX_PATH];
	BOOL result = TRUE;
	LANGID *pLangIds;
	unsigned int languageResources = 0;
	USHORT count = 0;
	CString resStr;
	PSTFWRESINFO pFwResInfo;
	int iInfoIndex = 0;
	HTREEITEM hItem = m_hRoot;
	CStCfgResOptions *pResOptions = NULL;

	pLangIds = new LANGID[g_LangIdCount];

  // give a chance to back out
    if ( !UserConfirmation() )
	{
		delete []pLangIds;
        return FALSE;
	}

	((CStBinderApp*)AfxGetApp())->GetString(IDS_STAT_UPDATING_RESOURCES, resStr);
	m_status_text.SetWindowText(resStr);

	// Set the range on the progress bar
	// #ops + one for the closing of the executable
	m_progress_ctrl.SetRange( 0, (short)GetNumberOfOpsToPerform()+1 );
	m_progress_ctrl.SetStep( 1 );
	m_progress_ctrl.SetPos( 0 );

	// Open the target executable file and prepare to
	// load resources.
	m_target_name.GetWindowText(szTargetName, MAX_PATH);

	hUpdateRes = BeginUpdateResource(szTargetName, FALSE); 
	if (hUpdateRes == NULL) 
	{ 
		((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_TARGET_OPEN_FAILED, resStr);
	    MessageBox(resStr, CAPTION_TEXT, MB_OK);
		delete []pLangIds;
		return FALSE;
	} 

	// Apply any file version changes
	m_pUpdaterVersion->ApplyVersionChanges(hUpdateRes, (LPVOID) &g_ResCfgData);

	// Apply other configuration changes
	pResOptions = new CStCfgResOptions();
	if ( pResOptions )
	{
		pResOptions->WriteConfigOptions(hUpdateRes);
	}

	// release the icon headers
	if (g_ResCfgData.pUpdaterGrpIconHeader)
	{
		VirtualFree(g_ResCfgData.pUpdaterGrpIconHeader, 0, MEM_RELEASE);
		g_ResCfgData.pUpdaterGrpIconHeader = NULL;
	}
	if (g_ResCfgData.pCompanyGrpIconHeader)
	{
		VirtualFree(g_ResCfgData.pCompanyGrpIconHeader, 0, MEM_RELEASE);
		g_ResCfgData.pCompanyGrpIconHeader = NULL;
	}

    // Run through each resource group and delete any bound resource
    // marked for deletion.

    DeleteMarkedResources(hUpdateRes);

	pFwResInfo = new STFWRESINFO[ m_total_file_count + m_bound_resource_count];
	if (!pFwResInfo)
	{
		((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_MEM_ALLOC_FAILED, resStr);
	    MessageBox(resStr, CAPTION_TEXT, MB_OK);
		delete []pLangIds;
		return FALSE;
	}

	// Run through each resource group and apply any resources
	// specified by the user.  Add the bound resources bitfield
	// to the updater after applying all resources.

	do
	{
		CResourceList *pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

		if ( pGroupResList )
		{
	        DWORD dwFileSize;
		    SYSTEMTIME sysLocal;
			CString pathName;
	        CString csText;
	        CString csVersion;
		    USHORT uTagId;
			USHORT index = 0;

			// Add the f/w language ID to that resource
			if ( hItem != m_hRecvResources && pGroupResList->GetCount() )
			{
				LANGID *pIds = pLangIds + (languageResources);

				*pIds = pGroupResList->GetLangID();
				++languageResources;
			}

			while (pGroupResList->GetAtIndex(index, pathName, dwFileSize, csVersion, uTagId, sysLocal))
		    {
			    PVOID pResBuffer;

                if ( pGroupResList->IsBoundResource( index ) )
	            {
					if ( hItem != m_hRecvResources )
					{
						CString csFwResName;
						pFwResInfo[iInfoIndex].wLangId = pGroupResList->GetLangID();
						pFwResInfo[iInfoIndex].iResId = pGroupResList->GetResId(index);
						csFwResName = pGroupResList->GetFileName(index);
						_tcscpy (pFwResInfo[iInfoIndex].szResourceName, csFwResName);
						++iInfoIndex;
					}
		            ++index;
			        ++count;
                    continue;
	            }
    			
		        pResBuffer = VirtualAlloc(0,dwFileSize, MEM_COMMIT, PAGE_READWRITE);

				if ( pResBuffer )
	    		{
					CString csResName;
                    int iResId;
					LANGID wLangId;
					//int iFwResId;

					// update the status area in the controlbar
					((CStBinderApp*)AfxGetApp())->GetString(IDS_STATUS_WRITING_RESOURCE, resStr);
					resStr.AppendFormat(_T(" %s"), pathName.GetString());
					m_status_text.SetWindowText(resStr);

    				HANDLE hFile = CreateFile (
		    					  pathName,
			    				  GENERIC_READ,
							      FILE_SHARE_READ,
								  NULL, // no SECURITY_ATTRIBUTES structure
					              OPEN_EXISTING, // No special create flags
							      0, // No special attributes
								  NULL); // No template file

				    if ( hFile != INVALID_HANDLE_VALUE )
					{
			   			// read in the data from the file
						DWORD dwBytesRead;
						BOOL bRead = ReadFile( hFile, pResBuffer, dwFileSize, &dwBytesRead, FALSE);
						CloseHandle(hFile);
						if ( dwBytesRead != dwFileSize )
						{
							((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_FAILED_TO_READ_DATA, resStr);
							resStr.AppendFormat(_T("\n\n%s"), pathName.GetString());
						    MessageBox(resStr, CAPTION_TEXT, MB_OK);
							EndUpdateResource ( hUpdateRes, TRUE );
							VirtualFree (pResBuffer, 0, MEM_RELEASE);
							delete []pFwResInfo;
							delete []pLangIds;
							return FALSE;
						}
					}
					else
					{
						((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_FAILED_TO_OPEN_FILE, resStr);
						resStr.AppendFormat(_T("\n\n%s"), pathName.GetString());
					    MessageBox(resStr, CAPTION_TEXT, MB_OK);
						EndUpdateResource ( hUpdateRes, TRUE );
						VirtualFree (pResBuffer, 0, MEM_RELEASE);
						delete []pFwResInfo;
						delete []pLangIds;
						return FALSE;
					}


		            iResId = pGroupResList->GetResId(index);
					csResName = pGroupResList->GetFileName(index);
					wLangId = pGroupResList->GetLangID();

					if ( hItem == m_hRecvResources )
						result = UpdateResource (hUpdateRes,       // update resource handle 
									L_STMP_RECV_RESTYPE,                   // resource type
									MAKEINTRESOURCE(iResId),     // resource name 
									wLangId,  // language
									pResBuffer,                   // ptr to resource info 
									dwFileSize); // size of resource info. 
					else
					{
						//iFwResId = pGroupResList->NextAvailableResourceID();
						//pGroupResList->SetResId(index, iFwResId);
						result = UpdateResource(hUpdateRes,       // update resource handle 
									L_STMP_FW_RESTYPE,                   // resource type
									MAKEINTRESOURCE(iResId),     // resource name 
									wLangId,  // language
									pResBuffer,                   // ptr to resource info 
									dwFileSize); // size of resource info.
					}
					
					VirtualFree ( pResBuffer, 0, MEM_RELEASE );

					if (result == FALSE) 
					{ 
						((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_FAILED_RES_WRITE, resStr);
						resStr.AppendFormat(_T("\n\n%s"), pathName.GetString());
						MessageBox (resStr, CAPTION_TEXT, MB_OK); 
						EndUpdateResource ( hUpdateRes, TRUE );
						delete []pFwResInfo;
						delete []pLangIds;
						return FALSE;
					} 
					else
					{
						CString csFormattedName;
	                    // convert the listing to a < bound resource >
	                    pGroupResList->SetBoundResource(index, TRUE);
		                GetResourceName(iResId, csResName);
			            csFormattedName.Format(_T("< %s >"), csResName.GetString());
				        pGroupResList->SetPathname (index, csFormattedName);
						++count;
						++m_bound_resource_count;
	                    --m_total_file_count;

						if ( hItem != m_hRecvResources )
						{
							pFwResInfo[iInfoIndex].wLangId = wLangId;
							pFwResInfo[iInfoIndex].iResId = iResId;
						//	pGroupResList->ReserveResourceID(iFwResId);
							_tcscpy (pFwResInfo[iInfoIndex].szResourceName, csResName);
							++iInfoIndex;
						}
		            }
				}
				else
				{
					((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_MEM_ALLOC_FAILED, resStr);
				    MessageBox(resStr, CAPTION_TEXT, MB_OK);
					EndUpdateResource ( hUpdateRes, TRUE );
					delete[] pFwResInfo;
					delete[] pLangIds;
					return FALSE;
				}
			
				UpdateProgress(TRUE);
				Sleep(1000);
				++index;
			}
		}
	
		hItem = m_ResourceTree.GetNextItem(hItem, TVGN_NEXTVISIBLE);
    }
	while ( hItem != NULL );

	// update the status area in the controlbar
	((CStBinderApp*)AfxGetApp())->GetString(IDS_STATUS_UPDATING_RESOURCE_CFG, resStr);
	m_status_text.SetWindowText(resStr);

	if ( count )
	{
		UpdateResource(hUpdateRes,       // update resource handle 
								L_STMP_RESINFO_TYPE,                   // resource type
								MAKEINTRESOURCE(IDR_BOUND_RESOURCE_COUNT),   // resource name 
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
								&m_bound_resource_count,      // ptr to resource info 
								sizeof(m_bound_resource_count)); // size of resource info. 

		UpdateResource(hUpdateRes,       // update resource handle 
								L_STMP_RESINFO_TYPE,                   // resource type
								MAKEINTRESOURCE(IDR_BOUND_LANG_RESOURCE_COUNT),   // resource name 
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
								&languageResources,      // ptr to resource info 
								sizeof(languageResources)); // size of resource info. 

		UpdateResource(hUpdateRes,       // update resource handle 
								L_STMP_RESINFO_TYPE,                   // resource type
								MAKEINTRESOURCE(IDR_BOUND_LANG_IDS),   // resource name 
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
								pLangIds,      // ptr to resource info 
								sizeof(LANGID)*languageResources); // size of resource info. 

		UpdateResource(hUpdateRes,       // update resource handle 
								L_STMP_RESINFO_TYPE,                   // resource type
								MAKEINTRESOURCE(IDR_BOUND_RESOURCE_TABLE),   // resource name 
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
								pFwResInfo,      // ptr to resource info 
								sizeof(STFWRESINFO)*iInfoIndex); // size of resource info. 

		UpdateResource(hUpdateRes,       // update resource handle 
								L_STMP_RESINFO_TYPE,                   // resource type
								MAKEINTRESOURCE(IDR_BOUND_RESOURCE_TABLE_LEN),   // resource name 
								MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL),  // language
								&iInfoIndex,      // ptr to resource info 
								sizeof(iInfoIndex)); // size of resource info. 
	}


	// update the status area in the controlbar
	((CStBinderApp*)AfxGetApp())->GetString(IDS_STATUS_CLOSING_TARGET, resStr);
	m_status_text.SetWindowText(resStr);

	EndUpdateResource ( hUpdateRes, FALSE );
	UpdateProgress(TRUE);
    RefreshResourceView(TRUE);

	delete[] pFwResInfo;
	delete[] pLangIds;

	if (pResOptions)
		delete pResOptions;


	m_LocalSelectionChanged = FALSE;

	return TRUE;
}


void CStBinderDlg::UpdateProgress( BOOL _step_it )
{
	if( _step_it )
	{
		m_progress_ctrl.StepIt();
	}
	((CStBinderApp*)AfxGetApp())->PumpMessages();
}


int CStBinderDlg::GetResourceID(CString _fname)
{
	int resId;

	if ( _fname.CompareNoCase( _T("stmp3rec.sys") ) == 0 )
		resId = IDR_STMP3REC_SYS;
	else if ( _fname.CompareNoCase( _T("stmp3rec.inf") ) == 0 )
		resId = IDR_STMP3REC_INF;
	else if ( _fname.CompareNoCase( _T("stmp3rec.cat") ) == 0 )
		resId = IDR_STMP3REC_CAT;
	else if ( _fname.CompareNoCase( _T("stmp3recx64.sys") ) == 0 )
		resId = IDR_STMP3RECX64_SYS;
	else if ( _fname.CompareNoCase( _T("stmp3recx64.inf") ) == 0 )
		resId = IDR_STMP3RECX64_INF;
	else if ( _fname.CompareNoCase( _T("stmp3recx64.cat") ) == 0 )
		resId = IDR_STMP3RECX64_CAT;
	else
		resId = IDR_RESID_UNKNOWN;

	return resId;
}

void CStBinderDlg::GetResourceName(int resId, CString& _fname)
{

	if ( resId == IDR_STMP3REC_SYS )
        _fname = _T("stmp3rec.sys");
	else if ( resId == IDR_STMP3REC_INF )
        _fname = _T("stmp3rec.inf");
	else if ( resId == IDR_STMP3REC_CAT )
        _fname = _T("stmp3rec.cat");
	else if ( resId == IDR_STMP3RECX64_SYS )
        _fname = _T("stmp3recx64.sys");
	else if ( resId == IDR_STMP3RECX64_INF )
        _fname = _T("stmp3recx64.inf");
    else if ( resId == IDR_STMP3RECX64_CAT )
        _fname = _T("stmp3recx64.cat");
//	else
//		((CStBinderApp*)AfxGetApp())->GetString(IDS_RES_UNKNOWN, _fname);

	return;
}


BOOL CStBinderDlg::CheckForDuplicateOrInvalidFilename(HTREEITEM _hItem, CString& _csPathname)
{
	BOOL result = FALSE;
	CString csAddPathName, csExistingPathName, csVersion;
    USHORT uIndex;
    BOOL bFileNameIsValid = FALSE;
	DWORD dwFileSize;
	USHORT uTagId;

	csAddPathName = _csPathname.Right( _csPathname.GetLength() -  _csPathname.ReverseFind('\\'));

	csAddPathName.MakeLower();

    // csAddPathName has a leading '\' so to simplify things the string constants have one too 

    // what resource group is selected?
	if (!_hItem)
		_hItem = m_ResourceTree.GetSelectedItem();

    if ( _hItem == m_hRecvResources ) 
    {
        for (int i = 0; i < NUM_RECOVERY_FILENAMES; ++i)
            if ( csAddPathName == csRecoveryFileNames[i])
            {
                bFileNameIsValid = TRUE;
                break;
            }

	    if ( !bFileNameIsValid )
	        return TRUE;
    }

	CResourceList *pGroupResList = (CResourceList *) m_ResourceTree.GetItemData(_hItem);

    uIndex = 0;
	while (pGroupResList->GetAtIndex(uIndex, csExistingPathName, dwFileSize, csVersion, uTagId))
	{
		if (_csPathname.CompareNoCase(csExistingPathName) == 0)
		{
			result = TRUE;
			break;
		}
		/*
		TCHAR szListPathName[MAX_PATH];

		LPTSTR pszListFileName;

		if (m_ResourceListCtl.GetItemText(iIndex, 0, szListPathName, MAX_PATH))
		{
			pszListFileName = _tcsrchr(szListPathName, '\\');
			if ( pszListFileName )
			{
				if (  csAddPathName.CompareNoCase( pszListFileName) == 0)
				{
					result = TRUE;
					break;
				}
			}
		}
		else
			break;
			*/
		++uIndex;
	}

	return result;
}


void CStBinderDlg::OnLvnKeydownBinariesList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	//LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
	switch(pLVKeyDow->wVKey)
	{
		case VK_DELETE: 
			{
                OnMenuClickedDelete();
			}
	}

	*pResult = 0;
}


void CStBinderDlg::OnMenuClickedDelete()
{
    CString countStr, csResName;
	int iSelectedCount = m_ResourceListCtl.GetSelectedCount();
    CResourceList *pGroupResList;
	HTREEITEM hItem = m_ResourceTree.GetSelectedItem();

	pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

	if ( pGroupResList )
	{
		int iIndex = m_ResourceListCtl.GetNextItem(-1, LVNI_SELECTED);
		while ( iSelectedCount > 0 )
    	{
		    if ( iIndex >= 0 )
   			{
			    if (pGroupResList->IsBoundResource( (USHORT)iIndex) == FALSE)
				{
					int iResId = pGroupResList->GetResId(iIndex);
					CString csFileName = pGroupResList->GetFileName(iIndex);

					m_ResourceListCtl.DeleteItem(iIndex);

		            // check if this was a replacement for existing bound resource
			        // remove the "- replace" if so.
				    CheckForReplacements(iResId, csFileName, FALSE);
	        		--m_total_file_count;
                }

	            pGroupResList->RemoveResource( (USHORT)iIndex);
				if ( GetDlgItem(IDOK)->IsWindowEnabled() == FALSE )
				    GetDlgItem(IDOK)->EnableWindow(TRUE);
            }

			if (pGroupResList->IsBoundResource( (USHORT)iIndex) == FALSE)
				iIndex = -1; //start from the top since we just deleted it

	    	iIndex = m_ResourceListCtl.GetNextItem(iIndex, LVNI_SELECTED);
			--iSelectedCount;
	    }

		m_ResourceListCtl.SetSelectionMark(0);

		countStr.Format(_T("%d"), m_total_file_count);
		m_total_files.SetWindowText(countStr);

	    countStr.Format(_T("%d"), m_bound_resource_count);
	    m_bound_resources.SetWindowText(countStr);

		pGroupResList->GetResourceGroupName( csResName );
		if ( pGroupResList->GetCount() )
			countStr.Format(_T("(%d) %s"), pGroupResList->GetCount(), csResName.GetString());
		else
			countStr = csResName;
		m_ResourceTree.SetItemText(hItem, countStr);

		RefreshResourceView(FALSE);
	}
}


void CStBinderDlg::OnMenuClickedDeleteResourceGroup()
{
	HTREEITEM hCurrentItem = m_ResourceTree.GetDropHilightItem();
	HTREEITEM hPrevSibling;
    CResourceList *pGroupResList;
	LANGID wLangID = 0;

	if ( !hCurrentItem )
		hCurrentItem = m_ResourceTree.GetSelectedItem();

	if ( !hCurrentItem )
		return;

    pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hCurrentItem);

	if ( pGroupResList )
	{
		wLangID = pGroupResList->GetLangID();
		delete pGroupResList;
	}

	hPrevSibling = m_ResourceTree.GetPrevSiblingItem( hCurrentItem );
	if ( hPrevSibling )
		m_ResourceTree.SelectItem( hPrevSibling );
	else
		m_ResourceTree.SelectItem( m_hRoot );

	m_ResourceTree.DeleteItem(hCurrentItem);

	BOOL bFound = FALSE;
	for (int i = 0; i < g_LangIdCount; ++i)
	{
		if ( g_LangIds[i] == wLangID )
			bFound = TRUE;

		if ( bFound )
			g_LangIds[i] = g_LangIds[i+1];
	}
	if ( bFound )
		--g_LangIdCount;
}


void CStBinderDlg::OnMenuClickedExtract()
{
	int iSelectedCount = m_ResourceListCtl.GetSelectedCount();
    CResourceList *pGroupResList;
	HTREEITEM hItem = m_ResourceTree.GetSelectedItem();

    pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hItem);

	if ( pGroupResList )
	{
		TCHAR szPathName[MAX_PATH];
        TCHAR *pPtr;
	    int iIndex = -1;

	 //       _tcscpy(szPathName, _T("."));
	    pPtr = &szPathName[0];

		while ( iSelectedCount > 0 )
	    {
	        CString csPath;

		    iIndex = m_ResourceListCtl.GetNextItem(iIndex, LVNI_SELECTED);
   			if ( iIndex >= 0 )
	    	{
	            if (pGroupResList->IsBoundResource( (USHORT)iIndex))
		        {
			        OPENFILENAME Ofn;
				    CString csResourceName, csFilter;
					TCHAR szFilter[100];
                    int iResId = pGroupResList->GetResId( iIndex );
					CString resStr;

					if ( iResId >= IDR_STMP3REC_SYS )
			            GetResourceName(iResId, csResourceName);
					else
						csResourceName = pGroupResList->GetFileName( iIndex );

                    ZeroMemory(&Ofn, sizeof(Ofn));
	                ZeroMemory(szFilter, sizeof(szFilter));

		            Ofn.lStructSize = sizeof(OPENFILENAME); 
			        if ( hItem == m_hRecvResources )
						((CStBinderApp*)AfxGetApp())->GetString(IDS_GRP_RECV_DRIVER, resStr);
					else
						((CStBinderApp*)AfxGetApp())->GetString(IDS_GRP_FIRMWARE, resStr);

                    _tcscpy(szFilter, resStr);
	                pPtr = szFilter + _tcslen(szFilter) + 1;
		            _tcscpy (pPtr, csResourceName);
			        pPtr += csResourceName.GetLength() + 1;
					*pPtr = '\0';

					Ofn.lpstrFilter = szFilter;
                    Ofn.nFilterIndex = 1;
	                _tcscpy(szPathName, csResourceName);
		            Ofn.lpstrFile= szPathName; 
			        Ofn.nMaxFile = sizeof(szPathName); 
				    Ofn.Flags = OFN_SHOWHELP | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_PATHMUSTEXIST; 
 
                    if ( GetSaveFileName(&Ofn) )
					{
		    		    if ( ExtractResource(szPathName, hItem, iResId) == FALSE )
						{
							((CStBinderApp*)AfxGetApp())->GetString(IDS_ERR_EXTRACT_FAILED, resStr);

					        MessageBox(resStr, CAPTION_TEXT, MB_OK);
						}
					}
				}
			}
			--iSelectedCount;
		}

        iSelectedCount = 0;
	}
}


void CStBinderDlg::CheckForExistingResources()
{
	TCHAR szTargetName[MAX_PATH];
    HMODULE hModule;
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;
	LANGID *pLangIDs = NULL;
	unsigned int langResCount = 0;
    ULONG ulBoundResources = 0L;

	m_target_name.GetWindowText(szTargetName, MAX_PATH);

    hModule = LoadLibraryEx (szTargetName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hModule)
    {
		CStCfgResOptions *pResOptions = new CStCfgResOptions();
		if ( pResOptions )
		{
			pResOptions->GetConfigOptions(hModule);
			delete pResOptions;
		}


    	hResInfo = FindResourceEx( hModule,
	    					L_STMP_RESINFO_TYPE,
		    				MAKEINTRESOURCE(IDR_BOUND_RESOURCE_COUNT),
			    			MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
    	if ( hResInfo )
        {
		    hRes = LoadResource(hModule, hResInfo);
    		if ( hRes )
	    		pPtr = LockResource(hRes);
        	if ( pPtr )
	        	ulBoundResources = *((ULONG *)pPtr);
        }

		if ( ulBoundResources )
		{
			CEnumResourceType	*pEnumResources;
				
			// Fixed resource IDs can be enumerated; resource names doesn't work for
			// language specific resources.
			pEnumResources = new CEnumResourceType (this,
										CStBinderDlg::OnEnumRecvResource,
										hModule,
										L_STMP_RECV_RESTYPE);
			if (pEnumResources)
			{
				pEnumResources->Begin();
				delete pEnumResources;
			}

			pEnumResources = new CEnumResourceType (this,
									CStBinderDlg::OnEnumFwResource,
									hModule,
									L_STMP_FW_RESTYPE);
			if (pEnumResources)
			{
				pEnumResources->Begin();
				delete pEnumResources;
			}
		}

        FreeLibrary (hModule);
    }
}

HRESULT CStBinderDlg::OnEnumRecvResource(PVOID pCallerClass,
									 HMODULE _hModule,
									int _iResId,
									WORD& wLangId)
{
	CStBinderDlg *pThisClass = (CStBinderDlg *) pCallerClass;
   	HRSRC hResInfo;
	LPVOID pPtr = NULL;
    CString csVersion = _T("");
	DWORD dwSize = 0;

   	hResInfo = FindResourceEx( _hModule,
	    				L_STMP_RECV_RESTYPE,
		    			MAKEINTRESOURCE(_iResId),
			    		MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL));
    if ( hResInfo )
    {
        dwSize = SizeofResource(_hModule, hResInfo);

	    if ( _iResId == IDR_STMP3REC_SYS || _iResId == IDR_STMP3RECX64_SYS )
		{
			// extract the version
			TCHAR szTmpPath[MAX_PATH];

			GetTempPath(MAX_PATH, szTmpPath);
			_tcscat(szTmpPath, _T("stmp.sys") );

			pThisClass->ExtractResource(szTmpPath, pThisClass->m_hRecvResources, _iResId);
			CStVersion *pStmp3RecVersion = new CStVersion(szTmpPath);

			if (pStmp3RecVersion)
			{
				pStmp3RecVersion->StGetFileVersion(csVersion);
				::DeleteFile(szTmpPath);
				delete (pStmp3RecVersion);
			}
		}

        pThisClass->InsertBoundResourceIntoList(pThisClass->m_hRecvResources, _iResId, NULL, dwSize, csVersion, NO_TAGID);
    }

	return S_OK;
}

HRESULT CStBinderDlg::OnEnumFwResource(PVOID pCallerClass,
									 HMODULE _hModule,
									CString& csResName,
									int _iResId,
									WORD& wLangId)
{
	CStBinderDlg *pThisClass = (CStBinderDlg *) pCallerClass;
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;
	BOOL bMatched = FALSE;
	CResourceList *pGroupResList;
	HTREEITEM hItem = pThisClass->m_ResourceTree.GetChildItem(pThisClass->m_hFirmwareResources);

	while ( hItem != NULL )
	{
		pGroupResList = (CResourceList *)pThisClass->m_ResourceTree.GetItemData(hItem);
		if ( pGroupResList )
		{
			LANGID groupID =  pGroupResList->GetLangID();

			if ( groupID == wLangId )
			{
				bMatched = TRUE;
				break;
			}
		}

		hItem = pThisClass->m_ResourceTree.GetNextItem(hItem, TVGN_NEXT);
	}

	if ( !bMatched )
	{
		// Add new resource group
		TCHAR szNewFwGroup[64];
		CString strFmt;

		GetLocaleInfo(wLangId, LOCALE_SLANGUAGE, szNewFwGroup, 64);
		strFmt.Format(_T("%s"), szNewFwGroup);
		hItem = pThisClass->m_ResourceTree.InsertItem(strFmt, pThisClass->m_hFirmwareResources,
											pThisClass->m_ResourceTree.GetPrevSiblingItem(pThisClass->m_hOtherFirmware));
		pGroupResList = new CResourceList( strFmt );
		pGroupResList->SetLangID( wLangId );
		pThisClass->m_ResourceTree.SetItemData(hItem, (DWORD_PTR)pGroupResList);

		// save it to the global list
		LANGID *pTmp = (LANGID *) VirtualAlloc(NULL, (g_LangIdCount+1) * sizeof(LANGID), MEM_COMMIT, PAGE_READWRITE);
		if ( pTmp )
		{
			memcpy (pTmp, g_LangIds, g_LangIdCount * sizeof(LANGID));
			VirtualFree( g_LangIds, 0, MEM_RELEASE);
			g_LangIds = pTmp;
			pTmp += g_LangIdCount;
			*pTmp = wLangId;
			++g_LangIdCount;
		}
	}

	hResInfo = FindResourceEx( _hModule,
			    				L_STMP_FW_RESTYPE,
								MAKEINTRESOURCE(_iResId),
								wLangId);

	if ( hResInfo )
	{
		CString csVersion = _T("");
		DWORD dwSize = 0;
		CString csDummy = _T("");

		dwSize = SizeofResource(_hModule, hResInfo);

		// extract the version
	   	hRes = LoadResource(_hModule, hResInfo);
	    if ( hRes )
		    pPtr = LockResource(hRes);

		// extract version and tagid from file
	    CStFwVersion *pFwVersion = new CStFwVersion(csDummy, (PUCHAR)pPtr, dwSize);

		pFwVersion->GetComponentVersion(csVersion);

		delete pFwVersion;

        pThisClass->InsertBoundResourceIntoList(hItem, _iResId, csResName, dwSize, csVersion, NO_TAGID);
	}

	return S_OK;
}


BOOL CStBinderDlg::ExtractResource(LPTSTR _szPathname, HTREEITEM _hItem, int _iResId)
{
	TCHAR szTargetName[MAX_PATH];
    HMODULE hModule;
   	HRSRC hResInfo;
	HGLOBAL hRes;
	LPVOID pPtr = NULL;

	CResourceList *pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(_hItem);

	m_target_name.GetWindowText(szTargetName, MAX_PATH);

    hModule = LoadLibraryEx (szTargetName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hModule)
    {
    	hResInfo = FindResourceEx( hModule,
			(_iResId >= IDR_STMP3REC_SYS) ? L_STMP_RECV_RESTYPE : L_STMP_FW_RESTYPE,
		    				MAKEINTRESOURCE(_iResId),
							pGroupResList->GetLangID());
    	if ( hResInfo )
        {
		    hRes = LoadResource(hModule, hResInfo);
    		if ( hRes )
	    		pPtr = LockResource(hRes);

            if ( pPtr )
            {
                DWORD dwSize = SizeofResource(hModule, hResInfo);
                HANDLE hFile;

                // create file and write contents
                hFile = CreateFile (
                                      _szPathname,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, // no SECURITY_ATTRIBUTES structure
                                      OPEN_ALWAYS, // No special create flags
                                      0, // No special attributes
                                      NULL); // No template file

                if ( hFile != INVALID_HANDLE_VALUE )
                {
                    DWORD dwBytesWritten;
                    if ( WriteFile(hFile, pPtr, dwSize, &dwBytesWritten, NULL) )
                    {
                        FreeLibrary (hModule);
                        CloseHandle(hFile);
                        return TRUE;
                    }

                    CloseHandle(hFile);
                }
            }
        }

        FreeLibrary (hModule);

    }

    return FALSE;
}


void CStBinderDlg::OnNMRclickBinariesList(NMHDR *pNMHDR, LRESULT *pResult)
{
    CMenu menu;
	CMenu* pPopup;
	CWnd* pWndPopupOwner;
	unsigned int iMenuCmd;
	CString csResStr;
	CPoint point;
	HTREEITEM hCurrentItem = m_ResourceTree.GetSelectedItem();
	int iSelectedCount = m_ResourceListCtl.GetSelectedCount();

	if ( !hCurrentItem  || hCurrentItem == m_hRoot || hCurrentItem == m_hFirmwareResources)
		return;

	::GetCursorPos(&point); //where is the mouse?

	VERIFY(menu.LoadMenu(IDR_LISTMENU));

	pPopup = menu.GetSubMenu(0);
	pWndPopupOwner = this;

	if( pPopup )
	{
		((CStBinderApp*)AfxGetApp())->GetString(IDS_EXTRACT, csResStr);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_EXTRACT,
							MF_BYCOMMAND | MF_STRING, IDM_EXTRACT, csResStr);

		((CStBinderApp*)AfxGetApp())->GetString(IDS_DELETE, csResStr);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_DELETE,
							MF_BYCOMMAND | MF_STRING, IDM_DELETE, csResStr);

		((CStBinderApp*)AfxGetApp())->GetString(IDS_BROWSE, csResStr);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_BROWSE,
							MF_BYCOMMAND | MF_STRING, IDM_BROWSE, csResStr);

		if ( iSelectedCount )
		{
			CResourceList *pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hCurrentItem);
	        int iIndex = -1;
			int iCheckCount = iSelectedCount;
			BOOL bEnableExtract = FALSE;

	        while ( iCheckCount > 0 )
		    {
			    iIndex = m_ResourceListCtl.GetNextItem(iIndex, LVNI_SELECTED);
    			if ( iIndex >= 0 )
		    	{
		            if (pGroupResList->IsBoundResource( (USHORT)iIndex))
						bEnableExtract = TRUE;
					++iIndex;
					--iCheckCount;
				}
			}


//			pPopup->EnableMenuItem(IDM_BROWSE, MF_GRAYED );

			if ( !bEnableExtract )
				pPopup->EnableMenuItem(IDM_EXTRACT, MF_GRAYED );
		}
		else
		{
			pPopup->EnableMenuItem(IDM_EXTRACT, MF_GRAYED );
			pPopup->EnableMenuItem(IDM_DELETE, MF_GRAYED );
		}

	    while (pWndPopupOwner->GetStyle() & WS_CHILD)
		    pWndPopupOwner = pWndPopupOwner->GetParent();

		iMenuCmd = (USHORT) pPopup->TrackPopupMenuEx( TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
												point.x, point.y,
												pWndPopupOwner, NULL);

		if ( iMenuCmd == IDM_DELETE )
			OnMenuClickedDelete();
		else if ( iMenuCmd == IDM_EXTRACT )
			OnMenuClickedExtract();
		else if ( iMenuCmd == IDM_BROWSE )
			OnMenuClickedBrowse( hCurrentItem );
	}
	*pResult = 0;
}

void CStBinderDlg::OnNMRclickResourcetree(NMHDR *pNMHDR, LRESULT *pResult)
{
    CMenu menu;
	CMenu* pPopup;
	CWnd* pWndPopupOwner;
	CString csResStr;
	unsigned int iMenuCmd;
	CPoint point;
	CResourceList *pGroupResList;
	HTREEITEM hCurrentItem = m_ResourceTree.GetDropHilightItem();

	*pResult = 0;

	if ( !hCurrentItem )
		hCurrentItem = m_ResourceTree.GetSelectedItem();

	if ( hCurrentItem == m_hRoot ||
		 hCurrentItem == m_hFirmwareResources ||
		 hCurrentItem == m_hOtherFirmware )
		return;

	::GetCursorPos(&point); //where is the mouse?

	VERIFY(menu.LoadMenu(IDR_GROUPMENU));

	pPopup = menu.GetSubMenu(0);
	pWndPopupOwner = this;

	if ( pPopup )
	{
		((CStBinderApp*)AfxGetApp())->GetString(IDS_DELETE, csResStr);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_DELETE,
							MF_BYCOMMAND | MF_STRING, IDM_DELETE, csResStr);

		((CStBinderApp*)AfxGetApp())->GetString(IDS_BROWSE, csResStr);
		ModifyMenu(pPopup->GetSafeHmenu(), IDM_BROWSE,
							MF_BYCOMMAND | MF_STRING, IDM_BROWSE, csResStr);

		pGroupResList = (CResourceList *)m_ResourceTree.GetItemData(hCurrentItem);

		if ( pGroupResList )
		{
			if ( pGroupResList->GetCount() )
				pPopup->EnableMenuItem(IDM_DELETE, MF_GRAYED );

		    while (pWndPopupOwner->GetStyle() & WS_CHILD)
			    pWndPopupOwner = pWndPopupOwner->GetParent();

			iMenuCmd = (USHORT) pPopup->TrackPopupMenuEx( TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
												point.x, point.y,
												pWndPopupOwner, NULL);

			if ( iMenuCmd == IDM_DELETE )
			{
				OnMenuClickedDeleteResourceGroup();
			}
			else if ( iMenuCmd == IDM_BROWSE )
			{
				// select the "browse for" item if is not currently selected
				if ( hCurrentItem != m_ResourceTree.GetSelectedItem() )
					m_ResourceTree.SelectItem( hCurrentItem );

				OnMenuClickedBrowse( hCurrentItem );
			}
		}
	}
}





void CStBinderDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	if ( !m_Busy )
	{
		m_status_text.SetWindowText(_T(""));
		m_StatusTextId = 0;
	}
	CDialog::OnMouseMove(nFlags, point);
}

//
// Callback for control mouse move
//
HRESULT CStBinderDlg::OnControlMouseMove(PVOID pCallerClass,
                                     UINT iResourceId)
{
	CStBinderDlg *pThisClass = (CStBinderDlg *) pCallerClass;

	if ( iResourceId != pThisClass->m_StatusTextId )
	{
		pThisClass->m_StatusTextId = iResourceId;

		switch ( iResourceId )
		{
		case IDC_BINARIES_LIST:
			pThisClass->m_status_text.SetWindowText(pThisClass->m_ResListDesc);
			break;
		case IDC_RESOURCETREE:
			pThisClass->m_status_text.SetWindowText(pThisClass->m_ResTreeDesc);
			break;
		case IDC_FIND_TARGET:
			pThisClass->m_status_text.SetWindowText(pThisClass->m_FindTargetDesc);
			break;
		case IDOK:
			pThisClass->m_status_text.SetWindowText(pThisClass->m_ApplyDesc);
			break;
		case IDCANCEL:
			pThisClass->m_status_text.SetWindowText(pThisClass->m_CloseDesc);
			break;
		case IDC_DLG_TARGET_CONFIGURE:
			pThisClass->m_status_text.SetWindowText(pThisClass->m_ConfigureDesc);
			break;
		case IDC_CFG_LOAD_PROFILE:
			pThisClass->m_status_text.SetWindowText(pThisClass->m_LoadProfileDesc);
			break;
		case IDC_CFG_SAVE_PROFILE:
			pThisClass->m_status_text.SetWindowText(pThisClass->m_SaveProfileDesc);
			break;
			
		default:
			pThisClass->m_status_text.SetWindowText(_T(""));
			break;
		}
	}

	return S_OK;
}

void CStBinderDlg::GetLastPaths()
{
	HKEY hKey;
	TCHAR szPath[MAX_PATH];
	DWORD dwDataLen = MAX_PATH;

	// Default to current folder
	m_LastRecoveryDriverPath = _T(".");
	m_LastFirmwarePath = _T(".");
	m_LastUpdaterPath = _T(".");

	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Freescale\\StBinder"), 0,
			KEY_READ, &hKey) == ERROR_SUCCESS )
	{
		DWORD dwType;
		LONG queryStatus = RegQueryValueEx( hKey, _T("LastUpdater"), NULL, &dwType,
			(LPBYTE)szPath, &dwDataLen);
		if ( queryStatus == ERROR_SUCCESS )
			m_LastUpdaterPath = szPath;

		dwDataLen = MAX_PATH;
		queryStatus = RegQueryValueEx( hKey, _T("LastFirmware"), NULL, &dwType,
			(LPBYTE)szPath, &dwDataLen);
		if ( queryStatus == ERROR_SUCCESS )
			m_LastFirmwarePath = szPath;

		dwDataLen = MAX_PATH;
		queryStatus = RegQueryValueEx( hKey, _T("LastRecvDriver"), NULL, &dwType,
			(LPBYTE)szPath, &dwDataLen);
		if ( queryStatus == ERROR_SUCCESS )
			m_LastRecoveryDriverPath = szPath;

		RegCloseKey(hKey);
	}
}

void CStBinderDlg::SaveLastPaths()
{
	HKEY hKey;
	DWORD dwDataLen;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Freescale\\StBinder"), 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS )
	{
		if (m_LastUpdaterPath.Compare(_T(".")))
		{	// it has changed
			dwDataLen = (m_LastUpdaterPath.GetLength()+1) * sizeof(TCHAR);
			RegSetValueEx( hKey, _T("LastUpdater"), NULL, REG_BINARY,
							(LPBYTE)m_LastUpdaterPath.GetString(), dwDataLen);
		}

		if (m_LastFirmwarePath.Compare(_T(".")))
		{	// it has changed
			dwDataLen = (m_LastFirmwarePath.GetLength()+1) * sizeof(TCHAR);
			RegSetValueEx( hKey, _T("LastFirmware"), NULL, REG_BINARY,
							(LPBYTE)m_LastFirmwarePath.GetString(), dwDataLen);
		}

		if (m_LastRecoveryDriverPath.Compare(_T(".")))
		{	// it has changed
			dwDataLen = (m_LastRecoveryDriverPath.GetLength()+1) * sizeof(TCHAR);
			RegSetValueEx( hKey, _T("LastRecvDriver"), NULL, REG_BINARY,
							(LPBYTE)m_LastRecoveryDriverPath.GetString(), dwDataLen);
		}

		RegCloseKey(hKey);
	}
}


void CStBinderDlg::OnBnClickedDlgTargetConfigure()
{
		// create the configuration manager
	TCHAR szCaption[MAX_PATH];

	_tcscpy(szCaption, _T("StBinder"));

	if (!g_ResCfgData.ProfileName.IsEmpty())
	{
		_tcscat(szCaption, _T(" - "));
		_tcscat(szCaption, g_ResCfgData.ProfileName);
	}
	CTargetConfigSheet *pTargetConfigSheet = new CTargetConfigSheet(szCaption, this);
	if ( pTargetConfigSheet  ) 
	{
		if (pTargetConfigSheet->DoModal() == IDOK)
			if ( !GetDlgItem(IDOK)->IsWindowEnabled() )
				GetDlgItem(IDOK)->EnableWindow(TRUE);

		delete pTargetConfigSheet;
	}
}

void CStBinderDlg::OnBnClickedCfgSaveProfile()
{
	CSaveProfileDlg dlg(this);
	dlg.DoModal();
}

void CStBinderDlg::OnBnClickedCfgLoadProfile()
{
	CLoadProfileDlg dlg(this);
	if (dlg.DoModal() == IDOK)
		GetDlgItem(IDOK)->EnableWindow(TRUE);

}

int CStBinderDlg::CheckProfileCount()
{
	LONG lStatus = -1;
	HKEY hProfilesKey = NULL;

	lStatus = RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Freescale\\StBinder\\Profiles"), 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hProfilesKey, NULL);

	DWORD dwIndex = 0;
	while (lStatus == ERROR_SUCCESS)
	{
		TCHAR nameStr[256];
		DWORD dwSize = 256*sizeof(TCHAR);

		lStatus = RegEnumKeyEx(hProfilesKey, dwIndex, nameStr, &dwSize, NULL, NULL, NULL, NULL );

		if (lStatus == ERROR_SUCCESS)
		{
			++dwIndex;
		}
	}
	if (hProfilesKey)
		RegCloseKey(hProfilesKey);

	return (int)dwIndex;
}