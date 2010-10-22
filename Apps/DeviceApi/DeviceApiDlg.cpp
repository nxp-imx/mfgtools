/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// DeviceApiDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceApi.h"
#include "DeviceApiDlg.h"

#include "Libs/DevSupport/Volume.h"
#include "Libs/DevSupport/HidDevice.h"
#include "Libs/DevSupport/MtpDevice.h"

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


// CDeviceApiDlg dialog



CDeviceApiDlg::CDeviceApiDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDeviceApiDlg::IDD, pParent)
	, _pDevice(NULL)
	, _currentApi(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDeviceApiDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SELECT_DEVICE_COMBO, _selectDeviceCtrl);
    DDX_Control(pDX, IDC_RESPONSE_EDIT, _responseCtrl);
    DDX_Control(pDX, IDC_API_LIST, _selectApiCtrl);
    DDX_Control(pDX, IDC_CDB_SIZE_TEXT, _cdbSizeCtrl);
    DDX_Control(pDX, IDC_XFER_SIZE_EDIT, _xferSizeCtrl);
    DDX_Control(pDX, IDC_STATUS_TEXT, _statusCtrl);
    DDX_Control(pDX, IDC_SEND_BUTTON, _sendCtrl);
    DDX_Control(pDX, IDC_READ_RADIO, _readCtrl);
    DDX_Control(pDX, IDC_WRITE_RADIO, _writeCtrl);
    DDX_Control(pDX, IDC_COMMAND_EDIT, _commandCtrl);
    DDX_Control(pDX, IDC_REJECT_AUTOPLAY_CHECK, _reject_autoplay_ctrl);
    DDX_Control(pDX, IDC_TALKY_LED, _cmdLedCtrl);
    DDX_Control(pDX, IDC_PROGRESS1, _cmdProgressCtrl);
}

BEGIN_MESSAGE_MAP(CDeviceApiDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_REFRESH_BUTTON, OnBnClickedRefreshButton)
	ON_CBN_SELCHANGE(IDC_SELECT_DEVICE_COMBO, OnCbnSelchangeSelectDeviceCombo)
	ON_BN_CLICKED(IDC_SEND_BUTTON, OnBnClickedSendButton)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_API_LIST, OnLvnItemchangedApiList)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_WRITE_RADIO, OnBnClickedWriteRadio)
	ON_BN_CLICKED(IDC_READ_RADIO, OnBnClickedReadRadio)
	ON_EN_CHANGE(IDC_XFER_SIZE_EDIT, OnEnChangeXferSizeEdit)
	ON_MESSAGE(WM_MSG_EN_CHANGE, OnEnChangeCommandEdit)
	ON_BN_CLICKED(IDC_REJECT_AUTOPLAY_CHECK, OnBnClickedRejectAutoplayCheck)
END_MESSAGE_MAP()


// CDeviceApiDlg message handlers

BOOL CDeviceApiDlg::OnInitDialog()
{
	ATLTRACE(_T(" +CDeviceApiDlg::OnInitDialog()\n"));
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

	//
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
	// is created to objectize CDeviceApiDlg::OnDeviceChangeNotify().
	//
	DeviceManager::DeviceChangeCallback cmd(this, &CDeviceApiDlg::OnDeviceChangeNotify);
	//
	// Create an Observer object, fill in the desired criteria, and
	// then pass it to Register(const Observer& observer)
	//
	DeviceManager::Observer callMe;
	callMe.NotifyFn = cmd;
	callMe.NotifyType = DeviceManager::ByDeviceClass;
	callMe.DeviceType = DeviceClass::DeviceTypeHid;
	gDeviceManager::Instance().Register(callMe);
	//
	// Alternately you can provide a subset of the criteria as parameters
	// Register(DeviceChangeCallback notifyFn, DeviceChangeNotifyType notifyType = AnyChange, LPARAM pData = 0)
	// 
	// Note: both forms of Register return a HANDLE that can be used to UnRegister(HANDLE)
	// the callback notification
	//
	// gDeviceManager::Instance().Register(cmd, DeviceManager::DeviceChangeNotifyType::ByDeviceClass, DeviceClass::DeviceTypeHid);
	gDeviceManager::Instance().Register(cmd, DeviceManager::ByDeviceClass, DeviceClass::DeviceTypeMsc);
	gDeviceManager::Instance().Register(cmd, DeviceManager::ByDeviceClass, DeviceClass::DeviceTypeMtp);

	// find any suitable devices
	OnBnClickedRefreshButton();
	// fill the APi list box
	FillApiList();
	// enable the context menu on the CDB edit control
	_commandCtrl.ShowMenu(true);

	// Init the RejectAutoPlay checkbox
	_reject_autoplay_ctrl.SetCheck(AfxGetApp()->GetProfileInt(_T("Settings"), _T("RejectAutoPlay"), TRUE));
	OnBnClickedRejectAutoplayCheck();

    // Init Talky LED
    _cmdLedCtrl.SetColor(CDynamicLED::color_Blue);
    _cmdLedCtrl.SetOnOff(CDynamicLED::state_Off);

    ATLTRACE(_T(" -CDeviceApiDlg::OnInitDialog()\n"));
	return FALSE;  // the api list control has the focus
}

void CDeviceApiDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDeviceApiDlg::OnPaint() 
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
HCURSOR CDeviceApiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// The DeviceChangeCallback function registered with gDeviceManager
bool CDeviceApiDlg::OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo)
{
	// We don't need to rebuild our DeviceManager's List of devices
	// since the DeviceManager should be current by the time we get called.
	// In other words, don't call OnBnClickedRefreshButton(), just clear
	// the DeviceComo and fill it with the current list.
	FillDeviceCombo();

	return ERROR_SUCCESS; //don't delete this callback
}

void CDeviceApiDlg::OnBnClickedRefreshButton()
{
	gDeviceManager::Instance()[DeviceClass::DeviceTypeMsc]->Refresh();
	gDeviceManager::Instance()[DeviceClass::DeviceTypeHid]->Refresh();
	FillDeviceCombo();
}

void CDeviceApiDlg::FillDeviceCombo()
{
//	CString selected;
//	Device* selectedDevice = NULL;

//	if ( _selectDeviceCtrl.GetCurSel() != -1 )
//	{
//		_selectDeviceCtrl.GetLBText(_selectDeviceCtrl.GetCurSel(), selected);
//		selectedDevice = (Device*)_selectDeviceCtrl.GetItemDataPtr(_selectDeviceCtrl.GetCurSel());
//	}
    _selectDeviceCtrl.ResetContent();

	std::list<Device*>::iterator device;
	std::list<Device*> deviceList;
	int index=0;

	// Add HID devices
	deviceList = gDeviceManager::Instance()[DeviceClass::DeviceTypeHid]->Devices();
	for (device = deviceList.begin(); device != deviceList.end(); ++device)
	{
		HidDevice* hid = dynamic_cast<HidDevice*>(*device);
		CString str = hid->_description.get();

		index = _selectDeviceCtrl.AddString(str);
		_selectDeviceCtrl.SetItemDataPtr(index, hid);
	}

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

	// Add MTP devices
	deviceList = gDeviceManager::Instance()[DeviceClass::DeviceTypeMtp]->Devices();
	for (device = deviceList.begin(); device != deviceList.end(); ++device)
	{
		MtpDevice* mtp = dynamic_cast<MtpDevice*>(*device);
//		CString str = mtp->_description.get();
		CString str = mtp->_friendlyName.get();

		index = _selectDeviceCtrl.AddString(str);
		_selectDeviceCtrl.SetItemDataPtr(index, mtp);
	}

	if ( _selectDeviceCtrl.GetCount() == 1 )
			_selectDeviceCtrl.SetCurSel(0);
/*	if ( _selectDeviceCtrl.GetCount() != 0 )
	{
		int sel = -1;
		if ( !selected.IsEmpty() )
			sel = _selectDeviceCtrl.FindString(0, selected);
		if ( sel != -1 )
		{
			_selectDeviceCtrl.SetCurSel(sel);
		}
		else
		{
			_selectDeviceCtrl.SetCurSel(0);
		}
	}
*/	
	OnCbnSelchangeSelectDeviceCombo();
}

void CDeviceApiDlg::OnCbnSelchangeSelectDeviceCombo()
{
	int selected = _selectDeviceCtrl.GetCurSel();
	if ( selected == -1 )
		_pDevice = NULL;
	else
		_pDevice = (Device*)_selectDeviceCtrl.GetItemDataPtr(selected);

	UpdateStatus();
}

void CDeviceApiDlg::OnBnClickedSendButton()
{
    _sendCtrl.EnableWindow(false);

    _responseCtrl.SetWindowText(_T(""));

    _cmdProgressCtrl.SetRange32(0, _currentApi->GetTransferSize());
    Device::UI_Callback callback(this, &CDeviceApiDlg::OnTalkyLED);
    HANDLE hCallback = _pDevice->RegisterCallback(callback);

	CString msg, msg2;
	uint8_t moreInfo = 0;
	uint32_t err = _pDevice->SendCommand(*_currentApi, &moreInfo);
	if ( err == ERROR_SUCCESS ) {
		msg = _currentApi->ResponseString().c_str();
		if ( msg.IsEmpty() )
			msg = _T("OK");
	}
	else
	{
		msg.Format(_T("Error: SendCommand(%s) failed. 0x%X (%d)\r\n"), _currentApi->GetName(), err, err);

		if ( FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, msg2.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL) )
			msg.Append(msg2);

		msg2.ReleaseBuffer();
	}
	
	if ( moreInfo )
	{
		msg.Append(_T("\r\n"));
		msg.Append(_pDevice->GetSendCommandErrorStr());
	}
	
	_responseCtrl.SetWindowText(msg);

    _pDevice->UnregisterCallback(hCallback);
    _sendCtrl.EnableWindow(true);
}

void CDeviceApiDlg::FillApiList()
{
	StApi * pApi = gStApiFactory().CreateApi("HidInquiry", "InfoPage:ChipInfoPage");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("HidDownloadFw", "updater.sb");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("HidBltcRequestSense");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("HidDeviceReset");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("HidDevicePowerDown");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("HidTestUnitReady");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("HidPitcRequestSense");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("HidPitcInquiry");
	_apiList.push_back(pApi);
    pApi = gStApiFactory().CreateApi("HidPitcRead", "Address:0x00000000,Length:0x00000000,DataSize:0x00000001");
	_apiList.push_back(pApi);
    pApi = gStApiFactory().CreateApi("HidPitcWrite", "PitcWriteData.bin");
	_apiList.push_back(pApi);

	pApi = gStApiFactory().CreateApi("MtpEraseBootmanager");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("MtpDeviceReset");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("MtpResetToRecovery");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("MtpResetToUpdater");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("MtpGetDriveVersion");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("MtpSetUpdateFlag");
	_apiList.push_back(pApi);
	pApi = gStApiFactory().CreateApi("MtpSwitchToMsc");
	_apiList.push_back(pApi);

	pApi = new StGetProtocolVersion();
	_apiList.push_back(pApi);
	pApi = new StGetChipMajorRevId();
	_apiList.push_back(pApi);
	pApi = new StGetChipPartRevId();
	_apiList.push_back(pApi);
	pApi = new StGetROMRevId();
	_apiList.push_back(pApi);
	pApi = new StGetStatus();
	_apiList.push_back(pApi);
	pApi = new StGetLogicalMediaInfo(MediaInfoNumberOfDrives, 0);
	_apiList.push_back(pApi);
	pApi = new StGetLogicalDriveInfo(0, DriveInfoSectorSizeInBytes, 0);
	_apiList.push_back(pApi);
	pApi = new StSetLogicalDriveInfo(0, DriveInfoTag, 0); //TODO: uint8_t
	_apiList.push_back(pApi);
	StVersionInfo _versionInfo(0x0123, 0x4567, 0x89ab);
    size_t junk = _versionInfo.size();
	pApi = new StSetLogicalDriveInfo(0, DriveInfoComponentVersion, _versionInfo); //TODO: StVersionInfo
	_apiList.push_back(pApi);
    pApi = new StGetLogicalTable(media::DefaultMediaTableEntries);
	_apiList.push_back(pApi);
/*	
DRIVE_DESC g_arr_drives[CFG_NUM_DRIVES] = {{"", L"Data Drive", DriveTypeData, DRIVE_TAG_DATA, FALSE, 0},
{"", L"Janus Drive", DriveTypeHiddenData, DRIVE_TAG_DATA_HIDDEN, FALSE, 0},
{"settings.bin", L"Persistent Data", DriveTypeHiddenData, DRIVE_TAG_DATA_SETTINGS, FALSE, 0},
{"firmware.sb", L"Application Image 1", DriveTypeSystem, DRIVE_TAG_BOOTMANAGER_S, TRUE, 0},
{"firmware.sb", L"Application Image 2", DriveTypeSystem, DRIVE_TAG_BOOTMANAGER_S+0x10, TRUE, 0},
{"firmware.sb", L"Application Image 3", DriveTypeSystem, DRIVE_TAG_BOOTMANAGER_S+0x20, TRUE, 0},
{"firmware.rsc", L"Application Resource 1", DriveTypeSystem, DRIVE_TAG_RESOURCE_BIN, TRUE, 0},
{"firmware.rsc", L"Application Resource 2", DriveTypeSystem, DRIVE_TAG_RESOURCE_BIN+0x10, TRUE, 0},
{"firmware.rsc", L"Application Resource 3", DriveTypeSystem, DRIVE_TAG_RESOURCE_BIN+0x20, TRUE, 0},
{"NoFile", L"Empty Drive", DriveTypeSystem, DRIVE_TAG_EXTRA_S, FALSE, 2097152},
{"updater.sb", L"Updater", DriveTypeSystem, DRIVE_TAG_UPDATER_S, FALSE, 0}};
*/
	media::MediaAllocationEntry entry(0xFF, media::DriveType_Data, media::DriveTag_Data, 0);
	_driveArray.push_back(entry);
	entry.Type.Value = media::DriveType_HiddenData; entry.Tag.Value = media::DriveTag_DataJanus;
	_driveArray.push_back(entry);
	entry.Type.Value = media::DriveType_HiddenData; entry.Tag.Value = media::DriveTag_DataSettings;
	_driveArray.push_back(entry);
	entry.Type.Value = media::DriveType_System; entry.Tag.Value = media::DriveTag_FirmwareImg; entry.SizeInBytes.Value = 5242880;
	_driveArray.push_back(entry);
	entry.Type.Value = media::DriveType_System; entry.Tag.Value = media::DriveTag_FirmwareImg2; entry.SizeInBytes.Value = 5242880;
	_driveArray.push_back(entry);
	entry.Type.Value = media::DriveType_System; entry.Tag.Value = media::DriveTag_FirmwareImg3; entry.SizeInBytes.Value = 5242880;
	_driveArray.push_back(entry);
//	entry.Type.Value = media::DriveType_System; entry.Tag.Value = media::DriveTag_FirmwareRsc; entry.SizeInBytes.Value = 1191072;
//	_driveArray.push_back(entry);
//	entry.Type.Value = media::DriveType_System; entry.Tag.Value = media::DriveTag_FirmwareRsc2; entry.SizeInBytes.Value = 1191072;
//	_driveArray.push_back(entry);
//	entry.Type.Value = media::DriveType_System; entry.Tag.Value = media::DriveTag_FirmwareRsc3; entry.SizeInBytes.Value = 1191072;
//	_driveArray.push_back(entry);
//	entry.Type.Value = media::DriveType_System; entry.Tag.Value = media::DriveTag_Extra; entry.SizeInBytes.Value = 2147483648;
//	_driveArray.push_back(entry);
	
	pApi = new StAllocateLogicalMedia(&_driveArray); //TODO: MEDIA_ALLOCATION_ARRAY
	_apiList.push_back(pApi);
	
	pApi = new StEraseLogicalMedia();
	_apiList.push_back(pApi);
	pApi = new StReadLogicalDriveSector(3,0x800,0,1);
	_apiList.push_back(pApi);
	uint8_t buffer[256];
	pApi = new StWriteLogicalDriveSector(3,0x800,0,1, &buffer[0], false); //TODO: uint8_t*
	_apiList.push_back(pApi);
	pApi = new StEraseLogicalDrive(0);
	_apiList.push_back(pApi);
	pApi = new StChipReset();
	_apiList.push_back(pApi);
	pApi = new StGetChipSerialNumberInfo(SerialNoInfoSerialNumber);
	_apiList.push_back(pApi);
	pApi = new StGetPhysicalMediaInfo();
	_apiList.push_back(pApi);
	pApi = new StReadPhysicalMediaSector(0,0x800+0x40,0,1);
	_apiList.push_back(pApi);
	pApi = new StResetToRecovery();
	_apiList.push_back(pApi);
	pApi = new StInitializeJanus();
	_apiList.push_back(pApi);
	pApi = new StGetJanusStatus();
	_apiList.push_back(pApi);
	pApi = new StInitializeDataStore(0);
	_apiList.push_back(pApi);
	pApi = new StResetToUpdater();
	_apiList.push_back(pApi);
	pApi = new StGetDeviceProperties();
	_apiList.push_back(pApi);
	pApi = new StSetUpdateFlag();
	_apiList.push_back(pApi);
	pApi = new StFilterPing();
	_apiList.push_back(pApi);
	pApi = new StScsiInquiry();
	_apiList.push_back(pApi);
	pApi = new StReadCapacity();
	_apiList.push_back(pApi);
	pApi = new StRead(0,1,0x800);
	_apiList.push_back(pApi);
	pApi = new StWrite(0,1,0x800,&buffer[0]); //TODO: uint8_t*
	_apiList.push_back(pApi);
	pApi = new StModeSense10();
	_apiList.push_back(pApi);
	pApi = new StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, false, _T("CUSTOM"));
	_apiList.push_back(pApi);

	CRect rect;
	_selectApiCtrl.GetClientRect(&rect);
	_selectApiCtrl.InsertColumn(0,_T("Name"),LVCFMT_LEFT, rect.Width()-GetSystemMetrics(SM_CXVSCROLL));
	_imageList.Create(16, 16, ILC_COLOR16, 0, 4);
	_imageList.SetBkColor(RGB(255,255,255));
	_imageList.Add(AfxGetApp()->LoadIcon(IDI_STSCSI_ICON));
	_imageList.Add(AfxGetApp()->LoadIcon(IDI_SCSI_ICON));
	_imageList.Add(AfxGetApp()->LoadIcon(IDI_STHIDBLTC_ICON));
	_imageList.Add(AfxGetApp()->LoadIcon(IDI_STHIDPITC_ICON));
	_imageList.Add(AfxGetApp()->LoadIcon(IDI_STMTP_ICON));
	_selectApiCtrl.SetImageList(&_imageList, LVSIL_SMALL);
	_selectApiCtrl.SetImageList(&_imageList, LVSIL_NORMAL);

	StApi::StApiList::iterator api; int before=0, after;
	for ( api=_apiList.begin(); api != _apiList.end(); ++api )
	{
		CString name = (*api)->GetName();
		int image = (*api)->GetType();
		after = _selectApiCtrl.InsertItem(before++, name, image);
		_selectApiCtrl.SetItemData(after, (DWORD_PTR)(*api));
	}
	// select the first one in the list by default
	BOOL ret = _selectApiCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
}

void CDeviceApiDlg::OnLvnItemchangedApiList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->uOldState == 0 && pNMLV->uNewState == 0)
		return;	// No change

	_currentApi = (StApi*)_selectApiCtrl.GetItemData(pNMLV->iItem);

	UpdateCommandParams();

	*pResult = 0;

}

void CDeviceApiDlg::UpdateStatus()
{
    _sendCtrl.EnableWindow(_pDevice != NULL);
    _cmdLedCtrl.SetOnOff(_pDevice != NULL ? CDynamicLED::state_On : CDynamicLED::state_Off);
	_cmdLedCtrl.SetColor(CDynamicLED::color_Blue);
	CString str = (_pDevice!=NULL) ? _T("Ready") : _T("No device.");
	_statusCtrl.SetWindowText(str);
}

void CDeviceApiDlg::UpdateCommandParams()
{
	CString str;

	// Command Data
	FormatCDB();
	
	// CDB size
	if ( IsCustomCommand() )
		str = _T("16 bytes (max)");
	else
		str.Format(_T("%d bytes"), _currentApi->GetCdbSize());
	_cdbSizeCtrl.SetWindowText(str);

	// Read/Write
	if ( IsCustomCommand() )
	{
		// allow the user to select read or write for custom commands
		_readCtrl.EnableWindow(true);
		_writeCtrl.EnableWindow(true);
	}
	else
	{
		_readCtrl.EnableWindow(false);
		_writeCtrl.EnableWindow(false);
	}
	_readCtrl.SetCheck(!_currentApi->IsWriteCmd());
	_writeCtrl.SetCheck(_currentApi->IsWriteCmd());

	// Response size
	if ( IsCustomCommand() )
	{
		// allow the user to specify the response size for custom commands
		_xferSizeCtrl.SetReadOnly(false);
	}
	else
	{
		_xferSizeCtrl.SetReadOnly(true);
	}
	str.Format(_T("0x%X"), _currentApi->GetTransferSize());
	_xferSizeCtrl.SetWindowText(str);

	// Response
	_responseCtrl.SetWindowText(_T(""));

	// Status
	UpdateStatus();
}

bool CDeviceApiDlg::IsCustomCommand()
{
	CString name = _currentApi->GetName();
    if ( name.CompareNoCase(_T("CUSTOM")) == 0 )
		return true;
	else
		return false;
}

void CDeviceApiDlg::FormatCDB()
{
	// get the cursor position
	int start, end;
	_commandCtrl.GetSel(start, end);

	CString str;
	uint32_t size = _currentApi->GetCdbSize();
	uint8_t* byte = (uint8_t*)_currentApi->GetCdbPtr();
	uint32_t index;
	for ( index=0; index<size; ++index )
	{
		str.AppendFormat(_T("%02X "), byte[index]);
		if ( (index+1)%8 == 0 ) {
			str.Delete(str.GetLength()-1);
			str.Append(_T("\r\n"));
		}
	}
	if ( (index)%8 == 0 )
		str.Delete(str.GetLength()-2, 2);

	_commandCtrl.SetWindowText(str);

	// put the cursor back
	_commandCtrl.SetSel(start, end);
}

void CDeviceApiDlg::ParseCDB()
{
	CString str;
	_commandCtrl.GetWindowText(str);
	str.Remove(_T(' '));
	str.Remove(_T('\r'));
	str.Remove(_T('\n'));
	str.Remove(_T('\t'));

	uint32_t buffer;
	uint8_t * cdbBuf = (uint8_t *)_currentApi->GetCdbPtr();
	for (int i=0; i<str.GetLength(); i+=2)
	{
		_stscanf_s(str.Mid(i, 2).GetBuffer(), _T("%02x"), &buffer);
		*cdbBuf = (uint8_t)buffer;
		++cdbBuf;
	}
}

void CDeviceApiDlg::OnDestroy()
{
	while ( _apiList.size() > 0 )
	{
		StApi * api = _apiList.back();
		_apiList.pop_back();
		delete api;
	}

	CDialog::OnDestroy();
}

void CDeviceApiDlg::OnBnClickedWriteRadio()
{
	_currentApi->SetWriteCmd(true);
}

void CDeviceApiDlg::OnBnClickedReadRadio()
{
	_currentApi->SetWriteCmd(false);
}

void CDeviceApiDlg::OnEnChangeXferSizeEdit()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	CString str;
	_xferSizeCtrl.GetWindowText(str);
	uint32_t size;
	_stscanf_s(str.GetBuffer(), _T("%x"), &size);
	
	_currentApi->SetTransferSize(size);
}

LRESULT CDeviceApiDlg::OnEnChangeCommandEdit(WPARAM, LPARAM)
{
	ParseCDB();
	_currentApi->ParseCdb();
	UpdateCommandParams();
	return 0;
}

void CDeviceApiDlg::OnCancel()
{
	// gracefully shut down the DeviceManager
	gDeviceManager::Instance().Close();

	CDialog::OnCancel();
}

void CDeviceApiDlg::OnBnClickedRejectAutoplayCheck()
{
	int checked = _reject_autoplay_ctrl.GetCheck();
	AfxGetApp()->WriteProfileInt(_T("Settings"), _T("RejectAutoPlay"), checked);
	gDeviceManager::Instance().SetCancelAutoPlay(checked == TRUE);
}

void CDeviceApiDlg::OnTalkyLED(const Device::NotifyStruct& nsInfo)
{
    if ( nsInfo.inProgress )
    {
//        switch ( nsInfo.direction )
//        {
//            case Device::NotifyStruct::dataDir_ToDevice:
//                _cmdLedCtrl.SetColor(CDynamicLED::color_Yellow);
//                _cmdLedCtrl.SetOnOff(CDynamicLED::state_Blink);
//                break;
//            case Device::NotifyStruct::dataDir_FromDevice:
		if ( _cmdLedCtrl.GetOnOff() != CDynamicLED::state_Blink )
		{
			_cmdLedCtrl.SetColor(CDynamicLED::color_Green);
            _cmdLedCtrl.SetOnOff(CDynamicLED::state_Blink);
		}
//                break;
//            case Device::NotifyStruct::dataDir_Off:
//            default:
//                _cmdLedCtrl.SetOnOff(CDynamicLED::state_Off);
//                break;
//        }

        _statusCtrl.SetWindowText(nsInfo.status);
        _cmdProgressCtrl.SetPos(nsInfo.position);
    }
    else
    {
		if ( nsInfo.error == ERROR_SUCCESS )
		{
			_cmdLedCtrl.SetColor(CDynamicLED::color_Blue);
			_cmdLedCtrl.SetOnOff(CDynamicLED::state_On);

			_statusCtrl.SetWindowText(_T("Ready"));
			_cmdProgressCtrl.SetPos(0);
		}
		else
		{
			_cmdLedCtrl.SetColor(CDynamicLED::color_Yellow);
			_cmdLedCtrl.SetOnOff(CDynamicLED::state_On);

			_statusCtrl.SetWindowText(_T("Error"));
			_cmdProgressCtrl.SetPos(0);
		}
    }
}
