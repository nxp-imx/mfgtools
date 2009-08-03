// BitBurnerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BitBurner.h"
#include "BitBurnerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum _IDD { IDD = IDD_ABOUTBOX };

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


// CBitBurnerDlg dialog




CBitBurnerDlg::CBitBurnerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBitBurnerDlg::IDD, pParent)
	, _pDevice(NULL)
	, _pOtpPitc(NULL)
	, _pOtpRegs(NULL)
	, _pCurrentReg(NULL)
{
	_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CBitBurnerDlg::~CBitBurnerDlg()
{
	if ( _pOtpRegs )
	{
		delete _pOtpRegs;
		_pOtpRegs = NULL;
	}
}

void CBitBurnerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SELECT_DEVICE_COMBO, _selectDeviceCtrl);
	DDX_Control(pDX, IDC_REGISTER_LIST, _otpRegisterListCtrl);
	DDX_Control(pDX, IDC_PITC_INFO_EDIT, _pitcInfoCtrl);
	DDX_Control(pDX, IDC_PITC_LED, _pitcLedCtrl);
	DDX_Control(pDX, IDC_LOAD_PITC_BUTTON, _pitcDownloadCtrl);
	DDX_Control(pDX, IDC_PROGRESS1, _pitcDownloadProgressCtrl);
	DDX_Control(pDX, IDC_STATUS_EDIT, _cmdStatusCtrl);
	DDX_Control(pDX, IDC_TALKY_LED, _cmdLedCtrl);
	DDX_Control(pDX, IDC_OTP_REG_INFO, _otpRegInfoCtrl);
	DDX_Control(pDX, IDC_CURRENT_EDIT, _CurrentRegisterValueCtrl);
	DDX_Control(pDX, IDC_NEW_EDIT, _NewRegisterValueCtrl);
	DDX_Control(pDX, IDC_LOCK_BUTTON, _lockCtrl);
	DDX_Control(pDX, IDC_WRITE_BUTTON, _writeCtrl);
	DDX_Control(pDX, IDC_WRITE_N_LOCK_BUTTON, _writeAndLockCtrl);
}

BEGIN_MESSAGE_MAP(CBitBurnerDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_SELECT_DEVICE_COMBO, OnCbnSelchangeSelectDeviceCombo)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_REGISTER_LIST, &CBitBurnerDlg::OnLvnItemchangedRegisterList)
    ON_BN_CLICKED(IDC_LOAD_PITC_BUTTON, &CBitBurnerDlg::OnBnClickedLoadPitcButton)
	ON_MESSAGE(HPCM_UPDATE, &CBitBurnerDlg::OnHotPropUpdate)
	ON_BN_CLICKED(IDC_LOCK_BUTTON, &CBitBurnerDlg::OnBnClickedLockButton)
	ON_BN_CLICKED(IDC_WRITE_BUTTON, &CBitBurnerDlg::OnBnClickedWriteButton)
	ON_BN_CLICKED(IDC_WRITE_N_LOCK_BUTTON, &CBitBurnerDlg::OnBnClickedWriteNLockButton)
//    ON_NOTIFY(TTN_NEEDTEXTA, 0, OnToolTipNotify)
//    ON_NOTIFY(TTN_NEEDTEXTW, 0, OnToolTipNotify)
//    ON_NOTIFY(TTN_GETDISPINFO, 0, OnToolTipNotify)
END_MESSAGE_MAP()


// CBitBurnerDlg message handlers
void CBitBurnerDlg::OnToolTipNotify( NMHDR * pNMHDR, LRESULT * pResult )
{
    TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
    UINT_PTR nID =pNMHDR->idFrom;
    if (pTTT->uFlags & TTF_IDISHWND)
    {
        // idFrom is actually the HWND of the tool
        nID = ::GetDlgCtrlID((HWND)nID);
        if(nID)
        {
            CString str;
			str.Format(_T("ctrl: %d"), nID);
			pTTT->lpszText = str.GetBuffer();
//				MAKEINTRESOURCE(nID);
//          pTTT->hinst = AfxGetResourceHandle();
            return;//(TRUE);
        }
    }
    return;//(FALSE);
}
BOOL CBitBurnerDlg::OnInitDialog()
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
	SetIcon(_hIcon, TRUE);		// Set big icon
	SetIcon(_hIcon, FALSE);		// Set small icon

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
	DeviceManager::DeviceChangeCallback cmd(this, &CBitBurnerDlg::OnDeviceChangeNotify);
	//
	// You can provide a subset of the criteria as parameters
	// Register(DeviceChangeCallback notifyFn, DeviceChangeNotifyType notifyType = AnyChange, LPARAM pData = 0)
	// 
	gDeviceManager::Instance().Register(cmd, DeviceManager::ByDeviceClass, DeviceClass::DeviceTypeHid);

	// find any suitable devices
	FillDeviceCombo();

    // fill the register selection control
	CRect rect;
	_otpRegisterListCtrl.GetClientRect(&rect);
	_otpRegisterListCtrl.InsertColumn(0,_T("Register"),LVCFMT_LEFT, rect.Width()-GetSystemMetrics(SM_CXVSCROLL));
	_otpRegisterListCtrl.SetExtendedStyle(/*LVS_EX_CHECKBOXES |*/ LVS_EX_GRIDLINES);
    FillRegisterList();

	// Cancel AutoPlay
//	gDeviceManager::Instance().SetCancelAutoPlay(true);

	// find the placeholder, get its position and destroy it
	CRect rcTreeWnd;
	CWnd* pPlaceholderWnd = GetDlgItem(IDC_PROPERTY_FRAME);
	pPlaceholderWnd->GetWindowRect(&rcTreeWnd);
	ScreenToClient(&rcTreeWnd);
	pPlaceholderWnd->DestroyWindow();

	// create the property window
	_pHotProp = new CHotPropCtrl;
	_pHotProp->CreateEx( WS_EX_STATICEDGE, NULL, NULL, WS_VISIBLE | WS_CHILD, rcTreeWnd, this, IDC_PROPERTY_FRAME);
  
	_pHotProp->RemoveAllProps();

	_pHotProp->EnableFlatStyle(TRUE, FALSE);
	_pHotProp->EnableGradient(TRUE, FALSE);
	_pHotProp->SetSingleLine(true);
	_pHotProp->EnableListAnimation();
	_pHotProp->EnableToolTips(true);

	// PITC LED control
	_pitcLedCtrl.SetColor(RGB(0,0,250), RGB(0,0,150)); // blue
	_pitcLedCtrl.EnableToolTips(true);
	_cmdLedCtrl.EnableToolTips(true);

	EnableToolTips();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CBitBurnerDlg::OnCancel()
{
	if ( _pOtpPitc )
	{
		delete _pOtpPitc;
		_pOtpPitc = NULL;
	}
	
	// gracefully shut down the DeviceManager
	gDeviceManager::Instance().Close();

	CDialog::OnCancel();
}

void CBitBurnerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CBitBurnerDlg::OnPaint()
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
		dc.DrawIcon(x, y, _hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CBitBurnerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(_hIcon);
}

// The DeviceChangeCallback function registered with gDeviceManager
bool CBitBurnerDlg::OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo)
{
	// Clear the DeviceComo and fill it with the current list.
	FillDeviceCombo();

	return false; // don't Unregister me
}

void CBitBurnerDlg::FillDeviceCombo()
{
    _selectDeviceCtrl.ResetContent();

	std::list<Device*>::iterator device;
	std::list<Device*> deviceList;
	int index=0;

	// We're only interested in SigmaTel devices.
//  gDeviceManager::Instance()[DeviceClass::DeviceTypeHid]->AddFilter(0x066F, 0x3700);
//	gDeviceManager::Instance()[DeviceClass::DeviceTypeHid]->Refresh();

	// Add HID devices
    deviceList = gDeviceManager::Instance()[DeviceClass::DeviceTypeHid]->Devices();
	for (device = deviceList.begin(); device != deviceList.end(); ++device)
	{
		HidDevice* hid = dynamic_cast<HidDevice*>(*device);
		CString str = hid->_description.get();

		index = _selectDeviceCtrl.AddString(str);
		_selectDeviceCtrl.SetItemDataPtr(index, hid);
	}

	if ( _selectDeviceCtrl.GetCount() == 1 )
			_selectDeviceCtrl.SetCurSel(0);

	OnCbnSelchangeSelectDeviceCombo();
}

void CBitBurnerDlg::OnCbnSelchangeSelectDeviceCombo()
{
	if ( _pOtpPitc )
	{
		delete _pOtpPitc;
		_pOtpPitc = NULL;
        _pitcInfoCtrl.SetWindowText(_T(""));
        _pitcLedCtrl.SetColor(RGB(0,0,250), RGB(0,0,150)); // blue
        _pitcLedCtrl.SetOnOff(CDynamicLED::state_Off);
        _pitcDownloadCtrl.EnableWindow(false);
        _pitcDownloadProgressCtrl.SetPos(0);
        _pitcDownloadProgressCtrl.ShowWindow(SW_HIDE);
	}
	if ( _pOtpRegs )
	{
		delete _pOtpRegs;
		_pOtpRegs = NULL;
	}

    _pDevice = NULL;
    _pitcInfoCtrl.SetWindowText(_T(""));
    _pitcLedCtrl.SetColor(RGB(0,0,250), RGB(0,0,150)); // blue
    _pitcLedCtrl.SetOnOff(CDynamicLED::state_Off);
    _pitcDownloadCtrl.EnableWindow(false);
    _pitcDownloadProgressCtrl.SetPos(0);
    _pitcDownloadProgressCtrl.ShowWindow(SW_HIDE);

    _cmdLedCtrl.SetColor(CDynamicLED::color_Red);
    _cmdLedCtrl.SetOnOff(CDynamicLED::state_On);
    _cmdStatusCtrl.SetWindowText(_T("No device."));

	SetWindowTitle(_T(""));

	int selected = _selectDeviceCtrl.GetCurSel();
	if ( selected == -1 )
		return;
/*	{
//		if ( _pDevice )
//            _pDevice->UnregisterCallback();

        _pDevice = NULL;
        _pitcInfoCtrl.SetWindowText(_T(""));
        _pitcLedCtrl.SetColor(RGB(0,0,250), RGB(0,0,150)); // blue
        _pitcLedCtrl.SetOnOff(CDynamicLED::state_Off);
        _pitcDownloadCtrl.EnableWindow(false);
        _pitcDownloadProgressCtrl.SetPos(0);
        _pitcDownloadProgressCtrl.ShowWindow(SW_HIDE);

        _cmdLedCtrl.SetColor(CDynamicLED::color_Red);
        _cmdLedCtrl.SetOnOff(CDynamicLED::state_On);
        _cmdStatusCtrl.SetWindowText(_T("No device."));
	}
	else
	{
//		if ( _pDevice )
//            _pDevice->UnregisterCallback();
*/
        _pDevice = (HidDevice*)_selectDeviceCtrl.GetItemDataPtr(selected);

		_pOtpPitc = new StOtpAccessPitc(_pDevice);

        Device::UI_Callback callback(this, &CBitBurnerDlg::OnTalkyLED);
        _pDevice->RegisterCallback(callback);

        // Display the PITC Info
        if ( _pOtpPitc && _pOtpPitc->GetFwComponent().GetLastError() == ERROR_SUCCESS )
        {
            _pitcInfoCtrl.SetWindowText(_pOtpPitc->GetFwComponent().toString());
            _pitcDownloadCtrl.EnableWindow(true);
            _pitcDownloadProgressCtrl.SetRange32(0, (int)_pOtpPitc->GetFwComponent().size()+65); // TODO: remove this test +65
            _pitcDownloadProgressCtrl.SetStep(1);
            _pitcDownloadProgressCtrl.SetPos(0);
            _pitcDownloadProgressCtrl.ShowWindow(SW_HIDE);

            if ( _pOtpPitc->IsPitcLoaded() )
            {
                _pitcLedCtrl.SetColor(RGB(0,0,250), RGB(0,0,150)); // blue
                _pitcLedCtrl.SetOnOff(CDynamicLED::state_On);
            }
            else
            {
                OnBnClickedLoadPitcButton();
            }

			CString deviceID = _T("");
			switch ( _pOtpPitc->GetChipId() )
			{
				case 0x3780:
				{
					uint32_t regValue = 0;
					if ( _pOtpPitc->OtpRegisterRead(StOtpRegs::HW_OCOTP_SWCAP, &regValue) == ERROR_SUCCESS )
					{
						if ( regValue & 0x00080000 ) //SW_CAP_IMX23
						{
							_pOtpRegs = new StOtpRegsMX23();
							deviceID = _T("i.MX23");
						}
						else
						{
							_pOtpRegs = new StOtpRegs3780();
							deviceID = _T("STMP3780");
						}
					}
					else
					{
						//error
						return;
					}
					break;
				}
				case 0x37B0:
				case 0x3770:
					_pOtpRegs = new StOtpRegs3770();
					deviceID = _T("STMP3770");
					break;
				case 0x3700:
					_pOtpRegs = new StOtpRegs3700();
					deviceID = _T("STMP3700");
					break;
				default:
					// no device
					return;
			}

			SetWindowTitle(deviceID);

        }
        else
        {
 		    CString msg, msg2;
            msg.Format(_T("Error: CreateOtpPitc failed. (%d)\r\n"), _pOtpPitc->GetFwComponent().GetLastError());
		    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, _pOtpPitc->GetFwComponent().GetLastError(), 0, 
                msg2.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
		    msg.Append(msg2);

            _pitcInfoCtrl.SetWindowText(msg);
            _pitcLedCtrl.SetColor(RGB(250,0,0), RGB(150,0,0)); // red
            _pitcLedCtrl.SetOnOff(CDynamicLED::state_On);
            _pitcDownloadCtrl.EnableWindow(false);
            _pitcDownloadProgressCtrl.SetPos(0);
            _pitcDownloadProgressCtrl.ShowWindow(SW_HIDE);
        }

        FillRegisterList();
//	}
}

void CBitBurnerDlg::SetWindowTitle( LPCTSTR deviceID )
{
	CString csTitle;

	if ( *deviceID == _T('') )
	{
		csTitle = _T("BitBurner (No device)");
	}
	else
	{
		csTitle.Format(_T("BitBurner (%s)"), deviceID);
	}

#ifdef INTERNAL_BUILD
// Make it obvious that this is a Confidential Internal build
// that is not to be distributed.
	csTitle.Append(_T(" - FREESCALE CONFIDENTIAL. DO NOT DISTRIBUTE!"));
#endif
	SetWindowText(csTitle);
}

void CBitBurnerDlg::FillRegisterList()
{
    _otpRegisterListCtrl.DeleteAllItems();

	if ( _pOtpRegs == NULL )
		return;

	std::vector<StOtpRegs::OtpRegister>::iterator reg;
	for ( reg = _pOtpRegs->_registers.begin(); reg != _pOtpRegs->_registers.end(); ++reg )
	{
#ifndef INTERNAL_BUILD
		if ( (*reg).Permissions == StOtpRegs::PermissionType_External )
		{
#endif
			int item = _otpRegisterListCtrl.InsertItem((*reg).Offset,(*reg).Name);
			_otpRegisterListCtrl.SetItemData( item, (DWORD_PTR)&(*reg) );

#ifndef INTERNAL_BUILD
		}
#endif
	}
}

void CBitBurnerDlg::OnLvnItemchangedRegisterList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	// No change
	if (pNMLV->uOldState == 0 && pNMLV->uNewState == 0)
		return;

    // empty control
    _pHotProp->RemoveAllProps();
	_otpRegInfoCtrl.SetWindowText(_T(""));

	// don't allow writing or locking till we have something to write
	// or know that the register is not locked.
	_lockCtrl.EnableWindow(false);
	_writeCtrl.EnableWindow(false);
	_writeAndLockCtrl.EnableWindow(false);


	// clear current register pointer
	_pCurrentReg = NULL;

	if ( pNMLV->uNewState & LVIS_SELECTED )
	{
		_pCurrentReg = (StOtpRegs3700::OtpRegister*)_otpRegisterListCtrl.GetItemData(pNMLV->iItem);
		theApp.WriteProfileString(_T("Settings"), _T("Selected Register"), _pCurrentReg->Name);

        if ( _pOtpPitc )
		{
			CStdString infoStr = _T("");

			// read the value
            if ( _pOtpPitc->OtpRegisterRead(_pCurrentReg->Offset, &_pCurrentReg->Value) == ERROR_SUCCESS )
			{
				infoStr.Format(_T("0x%08X"), _pCurrentReg->Value);

				_CurrentRegisterValueCtrl.SetWindowText(infoStr);
				_NewRegisterValueCtrl.SetWindowText(infoStr);
			}
			else
			{
				infoStr = _pOtpPitc->GetResponseStr();
				_otpRegInfoCtrl.SetWindowText(infoStr);
				return;
			}

			// display the fields
			uint32_t index;
			for ( index = 0; index < _pCurrentReg->Fields.size(); ++index )
			{
				if ( _pCurrentReg->Fields[index].Length == 1 )
				{
					_pHotProp->AddProp(index, CHotPropCtrl::propToggle, _pCurrentReg->Fields[index].Name, -1);
					_pHotProp->SetPropToggle(index, _pCurrentReg->Fields[index].ValueList.find(0)->second, _pCurrentReg->Fields[index].ValueList.find(1)->second); //_T("Not blown"), _T("Blown")
					_pHotProp->SetProp(index, _pCurrentReg->Fields[index].Format(_pCurrentReg->Value));
				}
				else
				{
					if ( _pCurrentReg->Fields[index].ValueList.empty() )
					{
						uint32_t max = StOtpRegs3700::OtpField::FieldMask[_pCurrentReg->Fields[index].Length];
						_pHotProp->AddProp(index, CHotPropCtrl::propInt, _pCurrentReg->Fields[index].Name, -1);
						_pHotProp->SetPropLimits(index, 0, max);
//						_pHotProp->SetProp(index, _pCurrentReg->Fields[index].Format(_pCurrentReg->Value));
						_pHotProp->SetProp(index, _pCurrentReg->Fields[index].Value(_pCurrentReg->Value));
					}
					else
					{
						_pHotProp->AddProp(index, CHotPropCtrl::propList, _pCurrentReg->Fields[index].Name, -1);
						CStringArray& arrl = _pHotProp->GetPropStringArray(index);
						
						std::map<uint32_t,CStdString>::const_iterator str;
						for ( str = _pCurrentReg->Fields[index].ValueList.begin(); str != _pCurrentReg->Fields[index].ValueList.end(); ++str )
						{
							arrl.Add((*str).second);
						}
						_pHotProp->SetProp(index, _pCurrentReg->Fields[index].Format(_pCurrentReg->Value));
					}
				}
#ifndef INTERNAL_BUILD
				if ( _pCurrentReg->Fields[index].Name.Find(_T("Reserved")) != -1 )
				{
					_pHotProp->LockProp(index, true);
				}
#endif
			} // end for(FIELDS)

			// display the info
			if ( _pOtpPitc->GetOtpRegisterInfo(_pCurrentReg->Offset, &_pCurrentReg->Info) == ERROR_SUCCESS )
			{
				infoStr = _pOtpPitc->GetResponseStr();
				_otpRegInfoCtrl.SetWindowText(infoStr);

				LockRegisterFields(_pCurrentReg);
				_lockCtrl.EnableWindow(!_pCurrentReg->Info.Locked);
			}
			else
			{
				infoStr = _pOtpPitc->GetResponseStr();
				_otpRegInfoCtrl.SetWindowText(infoStr);

				return;
			}

		} // end if (PITC)
	}
    // refresh control
    _pHotProp->Update();
}

void CBitBurnerDlg::LockRegisterFields(const StOtpRegs3700::OtpRegister *const pRegister) const
{
	uint32_t index;
	for ( index = 0; index < pRegister->Fields.size(); ++index )
	{
		// lock the fields if the register is locked and not shadowed
		if ( pRegister->Info.Locked && !pRegister->Info.Shadowed )
		{
			_pHotProp->LockProp(index, true);
		}

		// lock the field if the length of the field is 1 and the bit is Blown and the register is not shadowed.
		if ( pRegister->Fields[index].Length == 1 &&
			 pRegister->Fields[index].Value(pRegister->Value) == 1 &&
			 !pRegister->Info.Shadowed )
		{
			_pHotProp->LockProp(index, true);
		}
	}
}

// notification from the FieldControl that something changed
LRESULT CBitBurnerDlg::OnHotPropUpdate(WPARAM nID, LPARAM nValue)
{
    assert(_pCurrentReg);

	CString valStr;
	TCHAR *pEnd = NULL;

	// get the current register value
	uint32_t currentRegValue = _pCurrentReg->Value;
	
	// get the current "new" register value from the control
	_NewRegisterValueCtrl.GetWindowText(valStr);
	uint32_t newRegValue = _tcstoul(valStr.GetBuffer(), &pEnd, 16);
	
	// clear the field in the register
	newRegValue &= ~(StOtpRegs3700::OtpField::FieldMask[_pCurrentReg->Fields[nID].Length] << _pCurrentReg->Fields[nID].Offset);
	// OR in the new value for the field
	newRegValue |= (StOtpRegs3700::OtpField::FieldMask[_pCurrentReg->Fields[nID].Length] & nValue) << _pCurrentReg->Fields[nID].Offset;
	
	// put the modified value in the NewRegisterValue control
	valStr.Format(_T("0x%08X"), newRegValue);
	_NewRegisterValueCtrl.SetWindowText(valStr);

	if ( newRegValue != currentRegValue )
	{
		// allow writing if it is not locked or if it is shadowed
		_writeCtrl.EnableWindow(!_pCurrentReg->Info.Locked || _pCurrentReg->Info.Shadowed);
		// allow write and lock if it is not locked
		_writeAndLockCtrl.EnableWindow(!_pCurrentReg->Info.Locked);
	}
	else
	{
		// don't allow writing if the value hasn't changed
		_writeCtrl.EnableWindow(false);
		_writeAndLockCtrl.EnableWindow(false);
	}

	return ERROR_SUCCESS;
}

void CBitBurnerDlg::OnBnClickedLoadPitcButton()
{
//    _pitcLedCtrl.SetColor(RGB(200,200,0), RGB(150,150,0)); // yellow
//    _pitcLedCtrl.SetBlink(300);
    _pitcDownloadCtrl.EnableWindow(false);
    _pitcDownloadProgressCtrl.SetPos(0);
    _pitcDownloadProgressCtrl.ShowWindow(SW_SHOW);

    Device::UI_Callback callback(this, &CBitBurnerDlg::OnDownloadPitcProgress);
    
    int32_t error = _pOtpPitc->DownloadPitc(callback);

    if ( error == ERROR_SUCCESS )
    {
        _pitcLedCtrl.SetColor(RGB(0,0,250), RGB(0,0,150)); // blue
        _pitcLedCtrl.SetOnOff(CDynamicLED::state_On);
        _pitcDownloadCtrl.EnableWindow(true);
        _pitcDownloadProgressCtrl.SetPos(0);
        _pitcDownloadProgressCtrl.ShowWindow(SW_HIDE);
    }
    else
    {
        _pitcLedCtrl.SetColor(RGB(250,0,0), RGB(150,0,0)); // red
        _pitcLedCtrl.SetOnOff(CDynamicLED::state_On);
        _pitcDownloadCtrl.EnableWindow(true);
//        _pitcDownloadProgressCtrl.SetPos(0);
//        _pitcDownloadProgressCtrl.ShowWindow(SW_HIDE);
//   todo: maybe hide the progress bar and put up the error.
    }
}

void CBitBurnerDlg::OnDownloadPitcProgress(const Device::NotifyStruct& nsInfo)
{
    _pitcDownloadProgressCtrl.SetPos(nsInfo.position);
}

void CBitBurnerDlg::OnTalkyLED(const Device::NotifyStruct& nsInfo)
{
/*    static COLORREF oldOffColor, oldOnColor;

    if ( nsInfo.inProgress )
    {
        _pitcLedCtrl.GetColor(oldOnColor, oldOffColor);
        _pitcLedCtrl.SetColor(RGB(200,200,0), RGB(150,150,0)); // yellow
        _pitcLedCtrl.SetBlink(300);
    }
    else
    {
        _pitcLedCtrl.SetColor(oldOnColor, oldOffColor);
        _pitcLedCtrl.SetBlink(0);
    }
*/
    ATLTRACE2(_T("!!!!!!!CBitBurnerDlg::OnTalkyLED() inProgress(%d)\r\n"), nsInfo.inProgress);
    
    if ( nsInfo.error )
    {
        _cmdLedCtrl.SetColor(CDynamicLED::color_Red);
        _cmdLedCtrl.SetOnOff(CDynamicLED::state_On);

        _cmdStatusCtrl.SetWindowText(nsInfo.status);
    }
    else if ( nsInfo.inProgress )
    {
        switch ( nsInfo.direction )
        {
            case Device::NotifyStruct::dataDir_ToDevice:
                _cmdLedCtrl.SetColor(CDynamicLED::color_Yellow);
                _cmdLedCtrl.SetOnOff(CDynamicLED::state_Blink);
                break;
            case Device::NotifyStruct::dataDir_FromDevice:
                _cmdLedCtrl.SetColor(CDynamicLED::color_Green);
                _cmdLedCtrl.SetOnOff(CDynamicLED::state_Blink);
                break;
            case Device::NotifyStruct::dataDir_Off:
            default:
                _cmdLedCtrl.SetOnOff(CDynamicLED::state_Off);
                break;
        }

        _cmdStatusCtrl.SetWindowText(nsInfo.status);
    }
    else
    {
        _cmdLedCtrl.SetColor(CDynamicLED::color_Blue);
        _cmdLedCtrl.SetOnOff(CDynamicLED::state_On);

        _cmdStatusCtrl.SetWindowText(_T("Ready"));
    }
}

void CBitBurnerDlg::OnBnClickedLockButton()
{
	assert( _pCurrentReg );
	assert(_pOtpPitc);

	CString infoStr;

	if ( _pOtpPitc->OtpRegisterLock(_pCurrentReg->Offset) == ERROR_SUCCESS )
	{
			// display the info
			if ( _pOtpPitc->GetOtpRegisterInfo(_pCurrentReg->Offset, &_pCurrentReg->Info) == ERROR_SUCCESS )
			{
				infoStr = _pOtpPitc->GetResponseStr();
				_otpRegInfoCtrl.SetWindowText(infoStr);

				LockRegisterFields(_pCurrentReg);
				_lockCtrl.EnableWindow(!_pCurrentReg->Info.Locked);
			}
			else
			{
				infoStr = _pOtpPitc->GetResponseStr();
				_otpRegInfoCtrl.SetWindowText(infoStr);

				return;
			}
	}
	else
	{
		infoStr = _pOtpPitc->GetResponseStr();
		_otpRegInfoCtrl.SetWindowText(infoStr);

		return;
	}
}

void CBitBurnerDlg::OnBnClickedWriteButton()
{
	assert( _pCurrentReg );
	assert(_pOtpPitc);

	CString infoStr;
	uint32_t newValue = 0;
	TCHAR *pEnd = NULL;

	// Get the current "new" register value from the control
	_NewRegisterValueCtrl.GetWindowText(infoStr);
	uint32_t newRegValue = _tcstoul(infoStr.GetBuffer(), &pEnd, 0);

	// Write it down to the device
	if ( _pOtpPitc->OtpRegisterWrite(_pCurrentReg->Offset, newRegValue, false) == ERROR_SUCCESS )
	{
			// read the value back from the device
            if ( _pOtpPitc->OtpRegisterRead(_pCurrentReg->Offset, &_pCurrentReg->Value) == ERROR_SUCCESS )
			{
				infoStr.Format(_T("0x%08X"), _pCurrentReg->Value);

				_CurrentRegisterValueCtrl.SetWindowText(infoStr);
				
				// tell the user if we read back something different than we wrote
				if ( _pCurrentReg->Value != newRegValue )
				{
					infoStr.Format(_T("Read back verify failed.\r\n\r\n\tWrote: 0x%08X\r\n\t Read: 0x%08X"), newRegValue, _pCurrentReg->Value);
					MessageBox(infoStr, _T("Warning"),MB_OK);
				}
			}
			else
			{
				infoStr = _pOtpPitc->GetResponseStr();
				_otpRegInfoCtrl.SetWindowText(infoStr);
				return;
			}
	}
	else
	{
		infoStr = _pOtpPitc->GetResponseStr();
		_otpRegInfoCtrl.SetWindowText(infoStr);

		return;
	}
}

void CBitBurnerDlg::OnBnClickedWriteNLockButton()
{
	assert( _pCurrentReg );
	assert(_pOtpPitc);
	
	CString infoStr;
	uint32_t newValue = 0;
	TCHAR *pEnd = NULL;

	// Get the current "new" register value from the control
	_NewRegisterValueCtrl.GetWindowText(infoStr);
	uint32_t newRegValue = _tcstoul(infoStr.GetBuffer(), &pEnd, 0);

	// Write it down to the device
	if ( _pOtpPitc->OtpRegisterWrite(_pCurrentReg->Offset, newRegValue, true) == ERROR_SUCCESS )
	{
			// read the value back from the device
            if ( _pOtpPitc->OtpRegisterRead(_pCurrentReg->Offset, &_pCurrentReg->Value) == ERROR_SUCCESS )
			{
				infoStr.Format(_T("0x%08X"), _pCurrentReg->Value);

				_CurrentRegisterValueCtrl.SetWindowText(infoStr);
				
				// tell the user if we read back something different than we wrote
				if ( _pCurrentReg->Value != newRegValue )
				{
					infoStr.Format(_T("Read back verify failed.\r\n\r\n\tWrote: 0x%08X\r\n\t Read: 0x%08X"), newRegValue, _pCurrentReg->Value);
					MessageBox(infoStr, _T("Warning"),MB_OK);
				}

				// Update the register info to reflect the changed lock bit.
				HidPitcInquiry::OtpRegInfoPage infoPage;
				if ( _pOtpPitc->GetOtpRegisterInfo(_pCurrentReg->Offset, &_pCurrentReg->Info) == ERROR_SUCCESS )
				{
					infoStr = _pOtpPitc->GetResponseStr();
					_otpRegInfoCtrl.SetWindowText(infoStr);

					LockRegisterFields(_pCurrentReg);
					_lockCtrl.EnableWindow(!_pCurrentReg->Info.Locked);
				}
				else
				{
					infoStr = _pOtpPitc->GetResponseStr();
					_otpRegInfoCtrl.SetWindowText(infoStr);

					return;
				}
			}
			else
			{
				infoStr = _pOtpPitc->GetResponseStr();
				_otpRegInfoCtrl.SetWindowText(infoStr);
				return;
			}
	}
	else
	{
		infoStr = _pOtpPitc->GetResponseStr();
		_otpRegInfoCtrl.SetWindowText(infoStr);

		return;
	}
}

BOOL CBitBurnerDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	return CDialog::PreTranslateMessage(pMsg);
}
