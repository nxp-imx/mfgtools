/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StFwLoaderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StFwLoader.h"
#include "StFwLoaderDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
	CString m_desc_text;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	m_desc_text = _T("StFwLoader allows the user to select a firmware file\r\nand download it to a SigmaTel STMP device that is\r\nconnected to the PC in Recovery-Mode.");
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DESC_EDIT, m_desc_text);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// CStFwLoaderDlg dialog

CStFwLoaderDlg::CStFwLoaderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStFwLoaderDlg::IDD, pParent)
	, m_p_dev(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CStFwLoaderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE_COMBO, m_file_combo_ctrl);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress_ctrl);
	DDX_Control(pDX, IDC_FILE_SIZE, m_file_size_ctrl);
	DDX_Control(pDX, IDC_FILE_DATE, m_date_ctrl);
	DDX_Control(pDX, IDC_FILE_NAME, m_file_name_ctrl);
	DDX_Control(pDX, IDC_PRODUCT_VERSION, m_product_version_ctrl);
	DDX_Control(pDX, IDC_COMPONENT_VERSION, m_component_version_ctrl);
	DDX_Control(pDX, IDC_AUTO_DOWNLOAD_CHECK, m_auto_download_ctrl);
	DDX_Control(pDX, IDC_ON_TOP_CHECK, m_always_on_top_ctrl);
	DDX_Control(pDX, IDC_LOAD_BUTTON, m_load_ctrl);
	DDX_Control(pDX, IDC_STATUS_EDIT, m_status_ctrl);
	DDX_Control(pDX, IDC_REJECT_AUTOPLAY_CHECK, m_reject_autoplay_ctrl);
	DDX_Control(pDX, IDC_EMPTY_LIST_LINK, m_empty_list_ctrl);
	DDX_Control(pDX, IDC_CONNECTED_PICT, m_connection_ctrl);
	DDX_Control(pDX, IDC_TAG, m_tag_ctrl);
}

BEGIN_MESSAGE_MAP(CStFwLoaderDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BROWSE_BUTTON, OnBnClickedBrowse)
	ON_CBN_SELCHANGE(IDC_FILE_COMBO, OnCbnSelchangeFileCombo)
	ON_BN_CLICKED(IDC_AUTO_DOWNLOAD_CHECK, OnBnClickedAutoDownloadCheck)
	ON_BN_CLICKED(IDC_ON_TOP_CHECK, OnBnClickedOnTopCheck)
	ON_BN_CLICKED(IDC_REJECT_AUTOPLAY_CHECK, OnBnClickedRejectAutoplayCheck)
	ON_STN_CLICKED(IDC_EMPTY_LIST_LINK, OnStnClickedEmptyListLink)
	ON_BN_CLICKED(IDC_LOAD_BUTTON, OnBnClickedLoadButton)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

// CStFwLoaderDlg message handlers

BOOL CStFwLoaderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
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

	// Prepare the DeviceManager object for use. 
	// Note: We must call gDeviceManager::Instance().Close() before the app exits.
	VERIFY(gDeviceManager::Instance().Open() == ERROR_SUCCESS);
	//
	// Register with DeviceManager to call us back.
	//
	// Here are two different ways to tell our DeviceManager what changes
	// we are interested in.
	//
	// In either case, we need to create a DeviceManager::DeviceChangeCallback
	// to objectize the callback member function. In this example, the Functor 'cmd'
	// is created to objectize CStFwLoaderDlg::OnDeviceChangeNotify().
	//
	DeviceManager::DeviceChangeCallback cmd(this, &CStFwLoaderDlg::OnDeviceChangeNotify);
	
    gDeviceManager::Instance().Register(cmd, DeviceManager::ByDeviceClass, DeviceClass::DeviceTypeRecovery);

	// Combobox MRU initialization:
    m_file_combo_ctrl.SetMRURegKey ( _T("MRU") );
    m_file_combo_ctrl.SetMRUValueFormat ( _T("File #%d") );

//    m_file_combo_ctrl.SetAutoRefreshAfterAdd ( TRUE );
    m_file_combo_ctrl.SetAutoSaveAfterAdd ( TRUE );

    m_file_combo_ctrl.LoadMRU();
    m_file_combo_ctrl.RefreshCtrl();
	m_file_combo_ctrl.SetCurSel(0);
	OnCbnSelchangeFileCombo();
	
	// app options
	m_always_on_top_ctrl.SetCheck(AfxGetApp()->GetProfileInt(_T("Settings"), _T("AlwaysOnTop"), FALSE));
	OnBnClickedOnTopCheck();
	m_auto_download_ctrl.SetCheck(AfxGetApp()->GetProfileInt(_T("Settings"), _T("AutoDownload"), FALSE));
	m_reject_autoplay_ctrl.SetCheck(AfxGetApp()->GetProfileInt(_T("Settings"), _T("RejectAutoPlay"), TRUE));
	OnBnClickedRejectAutoplayCheck();

	// load state
	m_auto_load = m_auto_download_ctrl.GetCheck();
	m_load = false;

	std::list<Device*>::iterator device;
	std::list<Device*> deviceList;
	deviceList = gDeviceManager::Instance()[DeviceClass::DeviceTypeRecovery]->Devices();
    if( !deviceList.empty() )
    {
	    device = deviceList.begin();
        m_p_dev = dynamic_cast<RecoveryDevice*>(*device);
    }

	OnLoadStateChange();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CStFwLoaderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CStFwLoaderDlg::OnPaint() 
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

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CStFwLoaderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// The DeviceChangeCallback function registered with gDeviceManager
bool CStFwLoaderDlg::OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo)
{    
    switch(nsInfo.Event)
    {
        case DeviceManager::DEVICE_ARRIVAL_EVT: 
                if(m_p_dev==NULL)
                {
                    m_p_dev=dynamic_cast<RecoveryDevice*>(nsInfo.Device);
                    OnLoadStateChange();
                }
                break;
        case DeviceManager::DEVICE_REMOVAL_EVT:
                if(m_p_dev==nsInfo.Device)
                {
                    m_p_dev=NULL;
                    OnLoadStateChange();
                }
                break;
     }

	return false; // Do not unregister this callback.
}

void CStFwLoaderDlg::SetCurrentTask(uint32_t /*taskId*/, uint32_t taskRange)
{
	if( InProgress() )
	{
		m_progress_ctrl.SetRange32(0, taskRange);
		m_progress_ctrl.SetStep(1);
	}
}

void CStFwLoaderDlg::UpdateProgress(const Device::NotifyStruct& nsInfo)
{
	if( InProgress() && step )
	{
        m_progress_ctrl.SetPos(m_progress_ctrl.GetPos() + step);
	}
}

void CStFwLoaderDlg::OnBnClickedBrowse()
{
	CFileDialog dlg ( TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
						_T("Firmware files (*.sb)|*.sb|All Files (*.*)|*.*||"), this );

    if ( IDOK == dlg.DoModal() )
    {
		m_file_combo_ctrl.SetWindowText ( dlg.GetPathName() );
 		m_file_combo_ctrl.AddToMRU ( dlg.GetPathName() );
		m_file_combo_ctrl.RefreshCtrl();
		m_file_combo_ctrl.SetCurSel(0);
		OnCbnSelchangeFileCombo();
	}
}

void CStFwLoaderDlg::OnCbnSelchangeFileCombo()
{	
	CString len, name;
	int sel = m_file_combo_ctrl.GetCurSel();
	if ( sel != CB_ERR )
		m_file_combo_ctrl.GetLBText(sel, name);
//	m_file_combo_ctrl.GetWindowText(name);
	
	CWaitCursor cursor;
    if ( m_fw.LoadFromFile(name) == ERROR_SUCCESS )
	{
		cursor.Restore();
		// name
		m_file_name_ctrl.SetWindowText(name);
		// size
        len.Format(_T("%d bytes"), m_fw.size());
		m_file_size_ctrl.SetWindowText(len);
		// versions
		m_product_version_ctrl.SetWindowText(m_fw.GetProductVersion().toString());
		m_component_version_ctrl.SetWindowText(m_fw.GetComponentVersion().toString());
        m_tag_ctrl.SetWindowText(m_fw.GetFileType().ToString());
	}
	else
	{
		cursor.Restore();
		m_file_name_ctrl.SetWindowText(_T("No file selected."));
		m_file_size_ctrl.SetWindowText(_T("0 bytes"));
		m_product_version_ctrl.SetWindowText(_T("000.000.000"));
		m_component_version_ctrl.SetWindowText(_T("000.000.000"));
		m_tag_ctrl.SetWindowText(m_fw.GetFileType().ToString()/*_T("????")*/);
	}

	CFileStatus status;
	if( CFile::GetStatus( name, status ) )
    {
		// date
		CTime time = status.m_mtime;
		m_date_ctrl.SetWindowText( time.Format(_T("%c")) );
	}
	else
		m_date_ctrl.SetWindowText(_T(""));

	// see if we are supposed to do the download
	OnLoadStateChange();
}

void CStFwLoaderDlg::OnStnClickedEmptyListLink()
{
	m_file_combo_ctrl.EmptyMRU();
	m_file_combo_ctrl.RefreshCtrl();
	m_file_combo_ctrl.SetWindowText(_T(""));

	m_file_name_ctrl.SetWindowText(_T("No file selected."));
	m_file_size_ctrl.SetWindowText(_T("0 bytes"));
	m_product_version_ctrl.SetWindowText(_T("000.000.000"));
	m_component_version_ctrl.SetWindowText(_T("000.000.000"));

    m_fw.clear();

	OnLoadStateChange();
}

void CStFwLoaderDlg::OnBnClickedAutoDownloadCheck()
{
	m_auto_load = m_auto_download_ctrl.GetCheck();
	AfxGetApp()->WriteProfileInt(_T("Settings"), _T("AutoDownload"), m_auto_load);
	OnLoadStateChange();
}

void CStFwLoaderDlg::OnBnClickedOnTopCheck()
{
	int checked = m_always_on_top_ctrl.GetCheck();
	AfxGetApp()->WriteProfileInt(_T("Settings"), _T("AlwaysOnTop"), checked);

	if ( checked )
	{
		SetWindowPos(&CWnd::wndTopMost,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
	}
	else
	{
		SetWindowPos(&CWnd::wndNoTopMost,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
	}
}

void CStFwLoaderDlg::OnBnClickedRejectAutoplayCheck()
{
	int checked = m_reject_autoplay_ctrl.GetCheck();
	AfxGetApp()->WriteProfileInt(_T("Settings"), _T("RejectAutoPlay"), checked);
    gDeviceManager::Instance().SetCancelAutoPlay(checked == TRUE);	
}

void CStFwLoaderDlg::OnBnClickedLoadButton()
{
	m_load = true;
	OnLoadStateChange();
}

void CStFwLoaderDlg::OnLoadStateChange()
{
	uint32_t err;
	CString dbg_str;

	// connection bitmap just shows if a device is present or not
	if ( m_p_dev == NULL )
		m_connection_ctrl.SetBitmap(::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_UNCONNECTED)));
	else
		m_connection_ctrl.SetBitmap(::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_CONNECTED)));

	// have to have firmware and a device and Load or AutoLoad to download
    if ( m_fw.size() == 0 )
	{
		// can't do anything without firmware
		m_load_ctrl.EnableWindow(false);
		m_progress_ctrl.SetPos(0);
		m_status_ctrl.SetWindowText(_T("Select a firmware file to download."));
	}
	else 
	{
		// Have FIRMWARE
		if ( m_auto_load )
		{
			// AUTOLOAD is selected 
			m_load_ctrl.EnableWindow(false);
			if ( m_p_dev == NULL )
			{
				// No DEVICE
				m_progress_ctrl.SetPos(0);
				m_status_ctrl.SetWindowText(_T("Waiting for device..."));
			}
			else
			{
				// Have DEVICE
				Begin();
				m_status_ctrl.SetWindowText(_T("Downloading firmware..."));
				Device::UI_Callback callback(this, &StFwDownloaderDlg::UpdateProgress);
				if ( (err = m_p_dev->Download(m_fw, this)) == ERROR_SUCCESS)
				{
					m_status_ctrl.SetWindowText(_T("Complete"));
					Relax();
				}
				else
				{
					dbg_str.Format(_T("Error %d"), err);
					m_status_ctrl.SetWindowText(dbg_str);
					m_connection_ctrl.SetBitmap(::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_ERROR)));
				}              
			}
		}
		else
		{
			// No AUTOLOAD
			if ( m_load )
			{
				// Load button is CLICKED
				m_load_ctrl.EnableWindow(false);
				if ( m_p_dev == NULL )
				{
					// No DEVICE
					// button should not be enabled unless there is a device
					ASSERT(0);
				}
				else
				{
					// Have DEVICE
					Begin();
					m_load = false;
					m_status_ctrl.SetWindowText(_T("Downloading firmware..."));				
                    if ( (err = m_p_dev->Download(m_fw, this)) == ERROR_SUCCESS)
					{
						m_status_ctrl.SetWindowText(_T("Complete"));
						Relax();
					}
					else
					{
						dbg_str.Format(_T("Error %d"), err);
						m_status_ctrl.SetWindowText(dbg_str);
						m_connection_ctrl.SetBitmap(::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_ERROR)));
					}                   
				}
			}
			else
			{
				// Load button is NOT CLICKED
				if ( m_p_dev == NULL )
				{
					// turn off the button if we don't have a device
					m_progress_ctrl.SetPos(0);
					m_load_ctrl.EnableWindow(false);
					m_status_ctrl.SetWindowText(_T("Waiting for device..."));
				}
				else
				{
					// turn on the button if we do have a device
					m_progress_ctrl.SetPos(0);
					m_load_ctrl.EnableWindow(true);
					m_status_ctrl.SetWindowText(_T("Ready..."));
				}
			} // end NOT CLICKED
		} // end no AUTOLOAD
	} // end HAVE Firmware
}

void CStFwLoaderDlg::OnDropFiles(HDROP hDropInfo)
{
	CString file_name;
	UINT num = DragQueryFile(hDropInfo, 0, file_name.GetBufferSetLength(_MAX_PATH), _MAX_PATH);
	file_name.ReleaseBuffer();

	m_file_combo_ctrl.SetWindowText ( file_name );
 	m_file_combo_ctrl.AddToMRU ( file_name );
	m_file_combo_ctrl.RefreshCtrl();
	m_file_combo_ctrl.SetCurSel(0);
	OnCbnSelchangeFileCombo();
}

void CStFwLoaderDlg::OnCancel()
{
    gDeviceManager::Instance().Close();
    __super::OnCancel();
}
