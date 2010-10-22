/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StFormatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StFormat.h"
#include "StFormatDlg.h"

// messin around
#include "Libs/DevSupport/StFormatImage.h"
#include ".\stformatdlg.h"

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
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// StFormatDlg dialog

StFormatDlg::StFormatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(StFormatDlg::IDD, pParent)
	, _pVolume(NULL)
    , _pDiskParams(NULL)
    , _sectors(0)
    , _sectorSize(0)
    , _dataDriveNumber(0xFF)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

StFormatDlg::~StFormatDlg()
{
    if (_pDiskParams)
    {
        delete _pDiskParams;
        _pDiskParams = NULL;
    }
}
void StFormatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SELECT_DEVICE_COMBO, _selectDeviceCtrl);
	DDX_Control(pDX, IDC_REJECT_AUTOPLAY_CHECK, _reject_autoplay_ctrl);
	DDX_Control(pDX, IDC_STATUS_TEXT, _statusCtrl);
	DDX_Control(pDX, IDC_TOTAL_SECTORS_TEXT, _nandSectorsCtrl);
	DDX_Control(pDX, IDC_SECTOR_SIZE_TEXT, _nandSectorSizeCtrl);
	DDX_Control(pDX, IDC_FILE_SYSTEM_COMBO, _fileSystemCtrl);
	DDX_Control(pDX, IDC_ALLOCATION_SIZE_COMBO, _clusterSizeCtrl);
	DDX_Control(pDX, IDC_ROOT_ENTRIES_COMBO, _rootEntriesCtrl);
	DDX_Control(pDX, IDC_NUM_FATS_COMBO, _numFatsCtrl);
	DDX_Control(pDX, IDC_VOLUME_LABEL_EDIT, _volumeLabelCtrl);
	DDX_Control(pDX, IDC_IMAGE_INFO_EDIT, _imageInfoCtrl);
	DDX_Control(pDX, IDC_FORMAT_BUTTON, _formatCtrl);
    DDX_Control(pDX, IDC_PROGRESS, _progressCtrl);
}


BEGIN_MESSAGE_MAP(StFormatDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_SELECT_DEVICE_COMBO, OnCbnSelchangeSelectDeviceCombo)
	ON_BN_CLICKED(IDC_REJECT_AUTOPLAY_CHECK, OnBnClickedRejectAutoplayCheck)
    ON_CBN_SELCHANGE(IDC_FILE_SYSTEM_COMBO, OnCbnSelchangeFileSystemCombo)
    ON_CBN_SELCHANGE(IDC_ALLOCATION_SIZE_COMBO, OnCbnSelchangeClusterSizeCombo)
    ON_CBN_SELCHANGE(IDC_ROOT_ENTRIES_COMBO, OnCbnSelchangeRootEntriesCombo)
    ON_EN_CHANGE(IDC_VOLUME_LABEL_EDIT, OnEnChangeVolumeLabelEdit)
    ON_CBN_SELCHANGE(IDC_NUM_FATS_COMBO, OnCbnSelchangeNumFatsCombo)
    ON_BN_CLICKED(IDC_FORMAT_BUTTON, OnBnClickedFormatButton)
	ON_BN_CLICKED(IDC_DUMP_BUTTON, &StFormatDlg::OnBnClickedDumpButton)
END_MESSAGE_MAP()


// StFormatDlg message handlers

BOOL StFormatDlg::OnInitDialog()
{
	ATLTRACE(_T(" +StFormatDlg::OnInitDialog()\n"));
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

    // Open the DeviceManager so it will be able to call us back.
	uint32_t hr = gDeviceManager::Instance().Open();
	//
	// Create a DeviceManager::DeviceChangeCallback to objectize the 
	// callback member function. In this example, the Functor 'cmd'
	// is created to objectize CDeviceApiDlg::OnDeviceChangeNotify().
	//
	DeviceManager::DeviceChangeCallback cmd(this, &StFormatDlg::OnDeviceChangeNotify);
	//
    // Register with DeviceManager to call us back for MSC-device changes (default parameters).
    gDeviceManager::Instance().Register(cmd, DeviceManager::ByDeviceClass, DeviceClass::DeviceTypeMsc);

    // Init the RejectAutoPlay checkbox
	_reject_autoplay_ctrl.SetCheck(AfxGetApp()->GetProfileInt(_T("Settings"), _T("RejectAutoPlay"), TRUE));
	OnBnClickedRejectAutoplayCheck();

	// Init the device combo
	FillDeviceCombo();
    FillFileSystemCombo();
    FillRootEntriesCombo();
    FillNumFatsCombo();

    ATLTRACE(_T(" -StFormatDlg::OnInitDialog()\n"));
	return FALSE;  // the api list control has the focus
}

void StFormatDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void StFormatDlg::OnPaint() 
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
HCURSOR StFormatDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// The DeviceChangeCallback function registered with gDeviceManager
bool StFormatDlg::OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo)
{
	// We don't need to rebuild our DeviceManager's List of devices
	// since the DeviceManager should be current by the time we get called.
	// In other words, don't call OnBnClickedRefreshButton(), just clear
	// the DeviceComo and fill it with the current list.
	FillDeviceCombo();

	return false; // do not unregister this callback
}

void StFormatDlg::FillDeviceCombo()
{
    _selectDeviceCtrl.ResetContent();

	std::list<Device*>::iterator device;
	std::list<Device*> deviceList;
	int index=0;

	// Add SCSI devices
	deviceList.clear();
	deviceList = gDeviceManager::Instance()[DeviceClass::DeviceTypeMsc]->Devices();
	for (device = deviceList.begin(); device != deviceList.end(); ++device)
	{
		Volume* vol = dynamic_cast<Volume*>(*device);
		CString str = vol->_logicalDrive.get();

		if ( vol->IsUsb() )
		{
//			std::vector<Device*>::iterator disk = vol->Disks().begin();
//			str.AppendFormat(_T("    %s"), (*disk)->_friendlyName.get().c_str());
			str.AppendFormat(_T("    %s"), vol->Parent()->_friendlyName.get().c_str());
			index = _selectDeviceCtrl.AddString(str);
			_selectDeviceCtrl.SetItemDataPtr(index, vol);
		}
	}

	if ( _selectDeviceCtrl.GetCount() == 1 )
			_selectDeviceCtrl.SetCurSel(0);
	OnCbnSelchangeSelectDeviceCombo();
}

void StFormatDlg::OnCbnSelchangeSelectDeviceCombo()
{
    // clear NAND Info
    if (_pDiskParams)
    {
        delete _pDiskParams;
        _pDiskParams = NULL;
        _nandSectorsCtrl.SetWindowText(_T("0"));
        _nandSectorSizeCtrl.SetWindowText(_T("0 bytes"));
    }

    int selected = _selectDeviceCtrl.GetCurSel();
	if ( selected == -1 )
    {
		_pVolume = NULL;
    }
	else
    {
		_pVolume = (Volume*)_selectDeviceCtrl.GetItemDataPtr(selected);
        if ( GetNandParams(&_sectors, &_sectorSize, &_dataDriveNumber) == ERROR_SUCCESS )
		{
			CString txt = FormatSize(_sectors);
			_nandSectorsCtrl.SetWindowText(txt);
			txt = FormatSize(_sectorSize);
			txt += _T(" bytes");
			_nandSectorSizeCtrl.SetWindowText(txt);

			FillClusterSizeCombo(_sectorSize);

			_pDiskParams = new StFormatInfo(_sectors, _sectorSize);
		}
		else
		{
			_nandSectorSizeCtrl.SetWindowText(_T("No media"));
		}

    }

	UpdateStatus();
}
void StFormatDlg::UpdateStatus()
{
	CString temp, str = (_pVolume!=NULL) ? _T("Ready") : _T("No device.");
	_statusCtrl.SetWindowText(str);

    if ( _pDiskParams && _pDiskParams->GetLastError() == ERROR_SUCCESS )
    {
        temp = _pDiskParams->GetFileSystem() == StFormatInfo::FS_FAT12 ? _T("FAT 12") : 
               _pDiskParams->GetFileSystem() == StFormatInfo::FS_FAT16 ?_T("FAT 16")  : 
               _pDiskParams->GetFileSystem() == StFormatInfo::FS_FAT32 ? _T("FAT 32") : _T("Error");
        str.Format(_T("File system: %s\r\n"), temp);

        temp = FormatSize(_pDiskParams->GetRelativeSector());
        str.AppendFormat(_T("MBR: %s sectors\r\n"), temp);

        temp = FormatSize(_pDiskParams->PartitionSectors());
        str.AppendFormat(_T("Partition: %s sectors\r\n"), temp);

        temp = FormatSize(_pDiskParams->UserDataClusters());
        str.AppendFormat(_T("User Data Area: %s clusters\r\n"), temp);
        temp = FormatSize(_pDiskParams->UserDataClusters()* _pDiskParams->GetSectorsPerCluster());
        str.AppendFormat(_T("User Data Area: %s sectors\r\n"), temp);
        temp = FormatSize(_pDiskParams->UserDataClusters() * _pDiskParams->GetSectorSize() * _pDiskParams->GetSectorsPerCluster());
        str.AppendFormat(_T("User Data Area: %s bytes\r\n"), temp);

        str.AppendFormat(_T("Cluster size: %d sectors\r\n"), _pDiskParams->GetSectorsPerCluster());
        temp = FormatSize(_pDiskParams->GetSectorsPerCluster() * _pDiskParams->GetSectorSize());
        str.AppendFormat(_T("Cluster size: %s bytes\r\n"), temp);

        str.AppendFormat(_T("Root entries: %d\r\n"), _pDiskParams->GetNumRootEntries());
        temp = FormatSize(_pDiskParams->GetNumRootEntries() * sizeof(StFormatImage::RootDirEntry) / _pDiskParams->GetSectorSize());
        str.AppendFormat(_T("Root entries: %s sectors\r\n"), temp);
        temp = FormatSize(_pDiskParams->GetNumRootEntries() * sizeof(StFormatImage::RootDirEntry));
        str.AppendFormat(_T("Root entries: %s bytes\r\n"), temp);

        str.AppendFormat(_T("Number of FATs: %d\r\n"), _pDiskParams->GetNumFatTables());
        temp = FormatSize(_pDiskParams->GetNumFatTables() * _pDiskParams->FatSectors());
        str.AppendFormat(_T("Total FAT(s): %s sectors\r\n"), temp);
        
        str.AppendFormat(_T("PBS: %d sectors\r\n"), _pDiskParams->PbsSectors());

        str.AppendFormat(_T("Volume label: %s\r\n"), _pDiskParams->GetVolumeLabel().c_str());

        str.AppendFormat(_T("\r\nStatus: %s"), _pDiskParams->GetLastError()==ERROR_SUCCESS ? _T("OK") : _T("ERROR"));

        _imageInfoCtrl.SetWindowText(str);

    }
    else
    {
        _imageInfoCtrl.SetWindowText(_T(""));
    }

    if ( _pVolume && _pDiskParams && _pDiskParams->GetLastError() == ERROR_SUCCESS )
        _formatCtrl.EnableWindow(true);
    else
        _formatCtrl.EnableWindow(false);

    
    _progressCtrl.SetPos(0);
}

void StFormatDlg::OnCancel()
{
	// gracefully shut down the DeviceManager
	gDeviceManager::Instance().Close();

	CDialog::OnCancel();
}

void StFormatDlg::OnBnClickedRejectAutoplayCheck()
{
	int checked = _reject_autoplay_ctrl.GetCheck();
	AfxGetApp()->WriteProfileInt(_T("Settings"), _T("RejectAutoPlay"), checked);
	gDeviceManager::Instance().SetCancelAutoPlay(checked == TRUE);
}

CString StFormatDlg::FormatSize( ULONG_PTR size, INT_PTR mode )
{
    static CString str, str1;

    switch ( mode )
    {
    case 0: // put commas every 3 digits
        {
            str.Format(_T("%I32u"), size); str1.Empty();
            while (str.GetLength()>3) {
                str1.Insert(0, str.Right(3));
                str1.Insert(0,_T(','));
                str.Truncate(str.GetLength()-3);
	        }
            if (str.GetLength())
                str1.Insert(0,str);
            break;
        }
    case 1: // scale time to 2 decimal places
        {
            // sec / 60 == min, min / 60 == hours
            double time = size;
            if ( size / ( 60*60 ) ) { // hours
                str1.Format(_T("%.2f hours"), time/( 60*60 ) );
            }
            else if ( size / 60 ) { // minutes
                str1.Format(_T("%.1f min."), time/60 );
            }
            else { // seconds
                str1.Format(_T("%d sec."), size );
            }
            break;
        }
    case 2: // GB, MB, KB, bytes
        {
            double scaled_size = size;
            if ( size / (1024*1024*1024)  ) { // GB
                str1.Format(_T("%.5g GB"), scaled_size/(1024*1024*1024) );
            }
            else if ( size / (1024*1024) ) { // MB
                str1.Format(_T("%.5g MB"), scaled_size/(1024*1024) );
            }
            else if ( size / 1024 ) { // KB
                str1.Format(_T("%.5g KB"), scaled_size/1024 );
            }
            else { // bytes
                str1.Format(_T("%d bytes"), size );
            }
            break;
        }
    default:
        break;
    }

    return str1;
}
void StFormatDlg::FillFileSystemCombo()
{
    _fileSystemCtrl.ResetContent();

	int index;

   	index = _fileSystemCtrl.AddString(_T("FAT 16"));
    _fileSystemCtrl.SetItemData(index, StFormatInfo::FS_FAT16);

   	index = _fileSystemCtrl.AddString(_T("FAT 32"));
    _fileSystemCtrl.SetItemData(index, StFormatInfo::FS_FAT32);

	index = _fileSystemCtrl.AddString(_T("FAT 12"));
    _fileSystemCtrl.SetItemData(index, StFormatInfo::FS_FAT12);

	_fileSystemCtrl.SetCurSel(0);
    
    OnCbnSelchangeFileSystemCombo();
}

void StFormatDlg::FillClusterSizeCombo(const uint32_t sectorSize)
{
    _clusterSizeCtrl.ResetContent();

	int index;
    uint32_t size;
    CString txt;

    for ( size=sectorSize; size<=32*StFormatInfo::KibiByte; size*=2 )
    {
	    txt = FormatSize(size, 2);
        index = _clusterSizeCtrl.AddString(txt);
        _clusterSizeCtrl.SetItemData(index, size/sectorSize);
    }

	_clusterSizeCtrl.SetCurSel(1);
}

void StFormatDlg::FillRootEntriesCombo()
{
    _rootEntriesCtrl.ResetContent();

	int index;
    uint32_t entries;
    CString txt;

    for ( entries=512; entries<32*StFormatInfo::KibiByte; entries*=2 )
    {
	    txt = FormatSize(entries, 0);
        index = _rootEntriesCtrl.AddString(txt);
        _rootEntriesCtrl.SetItemData(index, entries);
    }

	_rootEntriesCtrl.SetCurSel(0);

    OnCbnSelchangeRootEntriesCombo();
}

void StFormatDlg::FillNumFatsCombo()
{
    _numFatsCtrl.ResetContent();

	int index;
    uint8_t fats;
    CString txt;

    for ( fats=1; fats<=2; ++fats )
    {
	    txt = FormatSize(fats);
        index = _numFatsCtrl.AddString(txt);
        _numFatsCtrl.SetItemData(index, fats);
    }

	_numFatsCtrl.SetCurSel(1);

    OnCbnSelchangeNumFatsCombo();
}

void StFormatDlg::OnCbnSelchangeFileSystemCombo()
{
    int selected = _fileSystemCtrl.GetCurSel();
    StFormatInfo::FileSystemT fileSystem = (StFormatInfo::FileSystemT)_fileSystemCtrl.GetItemData(selected);

    if (_pDiskParams)
        _pDiskParams->SetFileSystem(fileSystem);

    UpdateStatus();
}

void StFormatDlg::OnCbnSelchangeClusterSizeCombo()
{
    int selected = _clusterSizeCtrl.GetCurSel();
    uint32_t sectorsPerCluster = (uint32_t)_clusterSizeCtrl.GetItemData(selected);

    if (_pDiskParams)
        _pDiskParams->SetSectorsPerCluster(sectorsPerCluster);

    UpdateStatus();
}

void StFormatDlg::OnCbnSelchangeRootEntriesCombo()
{
    int selected = _rootEntriesCtrl.GetCurSel();
    uint16_t rootEntries = (uint16_t)_rootEntriesCtrl.GetItemData(selected);

    if (_pDiskParams)
        _pDiskParams->SetNumRootEntries(rootEntries);

    UpdateStatus();
}

void StFormatDlg::OnCbnSelchangeNumFatsCombo()
{
    int selected = _numFatsCtrl.GetCurSel();
    uint8_t numFats = (uint8_t)_numFatsCtrl.GetItemData(selected);

    if (_pDiskParams)
        _pDiskParams->SetNumFatTables(numFats);

    UpdateStatus();
}

void StFormatDlg::OnEnChangeVolumeLabelEdit()
{
    CString label;
    _volumeLabelCtrl.GetWindowText(label);

    if ( label.GetLength() > 11 )
    {
        _volumeLabelCtrl.SetWindowText(label.Left(11));
        _volumeLabelCtrl.SetSel(12,12);
    }
    else
    {
        _pDiskParams->SetVolumeLabel(label);
        UpdateStatus();
    }
}

//TODO: move this to StFormat.cpp ??
int32_t StFormatDlg::GetNandParams(uint32_t* pNumSectors, uint32_t* pSectorSize, uint8_t* pDriveNumber)
{
    // TODO: if (pDevice->GetType == TypeMsc)
    StReadCapacity api1;
    // TODO: check return value!
    _pVolume->SendCommand(api1);
	if ( api1.ScsiSenseStatus != SCSISTAT_GOOD )
		return api1.ScsiSenseStatus;

    READ_CAPACITY_DATA cap = api1.GetCapacity();
    *pSectorSize = cap.BytesPerBlock;
	*pNumSectors = cap.LogicalBlockAddress + 1;

    StGetLogicalTable api2;
    // TODO: check return value!
    _pVolume->SendCommand(api2);
	if ( api2.ScsiSenseStatus != SCSISTAT_GOOD )
		return api2.ScsiSenseStatus;

	media::LogicalDriveArray arr = api2.GetEntryArray();
    
//    *pNumSectors = (uint32_t) arr[0].SizeInBytes / *pSectorSize;
	if ( arr.Size() )
	{
		*pDriveNumber = arr.GetDrive(media::DriveTag_Data).DriveNumber;
	}

    return ERROR_SUCCESS;
}

void StFormatDlg::OnBnClickedFormatButton()
{
    const StFormatImage myImage(*_pDiskParams);
    uint8_t moreInfo;
    int32_t err;

	// Dummy info
	Device::NotifyStruct dummyInfo(_T("Not used"), Device::NotifyStruct::dataDir_Off, 0);

	ULONG sector_count=0;
	ULONGLONG start_sector = 0;
	ULONG sectors_per_write = 0, number_of_iterations = 0;
	ULONG sectors_left_for_last_iteration = 0;

	sector_count = (uint32_t)myImage._theImage.size()/_sectorSize;

    sectors_per_write = 64*StFormatInfo::KibiByte / _sectorSize;
	if( !sectors_per_write )
	{
		sectors_per_write = 1;
	}

	number_of_iterations = sector_count / sectors_per_write;
	sectors_left_for_last_iteration = sector_count % sectors_per_write;

	int steps = number_of_iterations;
	if( sectors_left_for_last_iteration )
		steps ++;
    _progressCtrl.SetRange32(0, steps+2);
    _progressCtrl.SetStep(1);
    _progressCtrl.SetPos(0);


    // Lock() opens the device and locks it.
    // Gotta lock the physical or else Windows won't re-read the MBR when we're done.
//	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_LOCKING_DRIVES, 2);
//	GetUpdater()->GetProgress()->UpdateProgress();
    HANDLE hVolume = _pVolume->Lock(Volume::LockType_Physical);
//	GetUpdater()->GetProgress()->UpdateProgress();
    _progressCtrl.StepIt();
    if( hVolume == INVALID_HANDLE_VALUE )
    {
        ATLTRACE(_T("*** Failed to Lock Physical Drive."));
        return;
    }

    // Erase the entire data drive allocation
//	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_ERASE_DATADRIVE, 1);

	StEraseLogicalDrive apiErase(_dataDriveNumber);
    err = _pVolume->SendCommand(hVolume, apiErase, &moreInfo, dummyInfo);

    _progressCtrl.StepIt();
//  GetUpdater()->GetProgress()->UpdateProgress();
    if( err != ERROR_SUCCESS )
    {
        ATLTRACE(_T("*** Failed EraseLogicalDrive."));
        return;
    }

//	GetUpdater()->GetProgress()->SetCurrentTask(TASK_TYPE_FORMATTING_DATADRIVE,	number_of_iterations);
	for( ULONG iteration = 0; iteration < number_of_iterations; iteration ++ )
	{
		// Get a pointer to the correct data in the image
        uint8_t* pData = (uint8_t*)&myImage._theImage[iteration * sectors_per_write * _sectorSize];
        // Api constructor copies the data into its own buffer
        StWriteLogicalDriveSector api(_dataDriveNumber, _sectorSize, start_sector, (uint32_t)sectors_per_write, pData);
        // Write the data to the device
        int32_t err = _pVolume->SendCommand(hVolume, api, &moreInfo, dummyInfo);
		if( err != ERROR_SUCCESS )
        {
            ATLTRACE(_T("*** failed after %d writes.\n"), iteration);
            // Unlock() Closes the device too.
            err = _pVolume->Unlock(hVolume);
        	return;
        }
		start_sector += sectors_per_write;
//		GetUpdater()->GetProgress()->UpdateProgress();
        _progressCtrl.StepIt();
	}

	// run the last iteration
	if( sectors_left_for_last_iteration )
	{
		// Get a pointer to the correct data in the image
        uint8_t* pData = (uint8_t*)&myImage._theImage[number_of_iterations * sectors_per_write * _sectorSize];
        // Api constructor copies the data into its own buffer
        StWriteLogicalDriveSector api(_dataDriveNumber, _sectorSize, start_sector, (uint32_t)sectors_left_for_last_iteration, pData);
        // Write the data to the device
        int32_t err = _pVolume->SendCommand(hVolume, api, &moreInfo, dummyInfo);
		if( err != ERROR_SUCCESS )
        {
            ATLTRACE(_T("*** failed on last write.\n"));
            // Unlock() Closes the device too.
            err = _pVolume->Unlock(hVolume);
        	return;
        }
//		GetUpdater()->GetProgress()->UpdateProgress();
        _progressCtrl.StepIt();
	}
    ATLTRACE(_T("*** Completed %d writes.\n"), number_of_iterations);

    // Invalidates the cached partition table and re-enumerates the device.
    err = _pVolume->Update(hVolume);
    // Frees the device for use and closes our handle
    err = _pVolume->Unlock(hVolume);

    // Dismount the Logical Volume so Windows will update its filesystem info.
    hVolume = _pVolume->Lock(Volume::LockType_Logical);
    err = _pVolume->Dismount(hVolume);
    err = _pVolume->Unlock(hVolume);

    // Touch all the drives to 'refresh' the one we updated.
    DWORD mask = ::GetLogicalDrives();

// found this code laying around... wonder if this will make windows re-examine the mbr and file system?
//    //big-time-hack : to fix the refresh problem under winxp. 
//	CString drive(GetUpdater()->GetDriveLetter());
//	drive += ":\\";
//	SHChangeNotify(SHCNE_MEDIAINSERTED | SHCNE_UPDATEDIR | SHCNE_FREESPACE, SHCNF_PATH|SHCNF_FLUSH, drive, 0);

    return;
}



void StFormatDlg::OnBnClickedDumpButton()
{
    CString fileName = _T("stformat.img");

	if ( _pDiskParams )
	{
		const StFormatImage myImage(*_pDiskParams);
		CFile dumpFile(fileName, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary );
		dumpFile.Write(&myImage._theImage[0], (UINT)myImage._theImage.size());
		dumpFile.Close();
	}
}
