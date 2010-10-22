/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// DeviceEnumDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DeviceEnum.h"
#include "DeviceEnumDlg.h"

#include "Libs/DevSupport/Volume.h"
#include "Libs/DevSupport/UsbController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const PTCHAR CDeviceEnumDlg::DeviceTypeDesc[]   = { _T("Recovery"), 
											    	_T("Mass Storage"), 
												    _T("MTP"),
												    _T("HID"),
													_T("USB Controller")};

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


// CDeviceEnumDlg dialog

CDeviceEnumDlg::CDeviceEnumDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDeviceEnumDlg::IDD, pParent)
	, m_device_type_bits(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	memset(&m_classImageList, 0, sizeof(m_classImageList));
}

void CDeviceEnumDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEVICETYPE_LIST, m_dev_type_ctrl);
	DDX_Control(pDX, IDC_DEVICE_TREE, m_device_tree_ctrl);
	DDX_Control(pDX, IDC_USB_FILTER_LIST, m_usb_ids_filter_ctrl);
	DDX_Control(pDX, IDC_WINDOWS_VERSION_TEXT, m_os_version_ctrl);
	DDX_Control(pDX, IDC_REJECT_AUTOPLAY_CHECK, m_reject_autoplay_ctrl);
}

BEGIN_MESSAGE_MAP(CDeviceEnumDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_REFRESH_BUTTON, OnBnClickedRefreshButton)
	ON_NOTIFY(TVN_SELCHANGED, IDC_DEVICE_TREE, OnTvnSelchangedDeviceTree)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_DEVICETYPE_LIST, OnLvnItemchangedDevicetypeList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_USB_FILTER_LIST, OnLvnItemchangedUsbFilterList)
	ON_BN_CLICKED(IDC_REJECT_AUTOPLAY_CHECK, OnBnClickedRejectAutoplayCheck)
END_MESSAGE_MAP()

// CDeviceEnumDlg message handlers

BOOL CDeviceEnumDlg::OnInitDialog()
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

	if ( gWinVersionInfo().IsAvailable() )
		m_os_version_ctrl.SetWindowText( gWinVersionInfo().GetDescription() );
	else
		m_os_version_ctrl.SetWindowText( _T("indeterminate") );


    // Open the DeviceManager so it will be able to call us back.
	uint32_t hr = gDeviceManager::Instance().Open();
	//
	// Create a DeviceManager::DeviceChangeCallback to objectize the 
	// callback member function. In this example, the Functor 'cmd'
	// is created to objectize CDeviceApiDlg::OnDeviceChangeNotify().
	//
	DeviceManager::DeviceChangeCallback cmd(this, &CDeviceEnumDlg::OnDeviceChangeNotify);
	//
    // Register with DeviceManager to call us back for any device change (default parameters).
	gDeviceManager::Instance().Register(cmd);

	// Init the device tree before checking the type list
	InitDeviceTree();
	InitPropertyTree();

	// Init the Device Class Filter List Box
	m_device_type_bits = theApp.GetProfileInt(_T("Class Filters"), _T("Device Classes"), 0x0f);

	CRect rect;
	m_dev_type_ctrl.GetClientRect(&rect);
	m_dev_type_ctrl.InsertColumn(0,_T("Device Type"),LVCFMT_LEFT, rect.Width()-GetSystemMetrics(SM_CXVSCROLL));
	m_dev_type_ctrl.SetExtendedStyle(LVS_EX_CHECKBOXES /*| LVS_EX_GRIDLINES*/);
	TCHAR _char; int item; _char=_T('D'); CString str;
	for ( int i=RECOVERY; i<=USB_CONTROLLER; ++i ) {
		item = m_dev_type_ctrl.InsertItem(i, DeviceTypeDesc[i]);
		DWORD bit = 1<<i;
		m_dev_type_ctrl.SetItemData(item, bit);
		if ( m_device_type_bits & bit )
			m_dev_type_ctrl.SetCheck(item);
	}

	// Init the USB IDs Filter List Box
	m_usb_ids_filter_ctrl.GetClientRect(&rect);
	m_usb_ids_filter_ctrl.InsertColumn(0,_T("USB VID/PID"),LVCFMT_LEFT, rect.Width()-GetSystemMetrics(SM_CXVSCROLL));
	m_usb_ids_filter_ctrl.SetExtendedStyle(LVS_EX_CHECKBOXES /*| LVS_EX_GRIDLINES*/);
	InitFilterListCtrl();
	
	// Init the RejectAutoPlay checkbox
	m_reject_autoplay_ctrl.SetCheck(AfxGetApp()->GetProfileInt(_T("Settings"), _T("RejectAutoPlay"), TRUE));
	OnBnClickedRejectAutoplayCheck();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDeviceEnumDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDeviceEnumDlg::OnPaint() 
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
HCURSOR CDeviceEnumDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CDeviceEnumDlg::InitDeviceTree()
{
#ifdef UNICODE
	//w98 In Win98 we have to tell the control to use Unicode or else we won't 
	//w98 get the TVN_SELCHANGED notification
	if ( gWinVersionInfo().IsWin9x() )
		m_device_tree_ctrl.SendMessage(CCM_SETUNICODEFORMAT, (WPARAM)true);
#endif

	//Set Image List 
	if ( m_classImageList.cbSize == 0 )
	{
		m_classImageList.cbSize = sizeof(m_classImageList);
        BOOL success = gSetupApi().SetupDiGetClassImageList(&m_classImageList);
		m_ImageList.Attach(m_classImageList.ImageList);
		m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_COMPUTER_ICON));
		m_device_tree_ctrl.SetImageList(&m_ImageList, LVSIL_SMALL);
		m_device_tree_ctrl.SetImageList(&m_ImageList, LVSIL_NORMAL);
	}
}

void CDeviceEnumDlg::OnBnClickedRefreshButton()
{
	// Refresh the Device Classes
	if ( m_device_type_bits & (1<<RECOVERY) )
	{
		gDeviceManager::Instance()[DeviceClass::DeviceTypeRecovery]->Refresh();
	}
	if ( m_device_type_bits & (1<<MASS_STORAGE) )
	{
// TODO: move this functionality to DeviceManager?? DeviceClassVolume??
//		if ( gWinVersionInfo().IsWinNT() )
			gDeviceManager::Instance()[DeviceClass::DeviceTypeMsc]->Refresh();
//		else
//			m_diskDevClass.Refresh();
	}
	if ( m_device_type_bits & (1<<MTP) )
	{
		gDeviceManager::Instance()[DeviceClass::DeviceTypeMtp]->Refresh();
	}
	if ( m_device_type_bits & (1<<HID) )
	{
		gDeviceManager::Instance()[DeviceClass::DeviceTypeHid]->Refresh();
	}
	if ( m_device_type_bits & (1<<USB_CONTROLLER) )
	{
		gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbController]->Refresh();
	}

    // Alternatively, have the DeviceManager Refresh() all the classes
    // gDeviceManager::Instance().Refresh();

	// put the selected devices in the tree
	InsertDevices();
}

bool CDeviceEnumDlg::OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo)
{
	// We don't need to rebuild our DeviceManager's List of devices
	// since the DeviceManager should be current by the time we get called.
	// In other words, don't call OnBnClickedRefreshButton(), just clear
	// the DeviceCombo and fill it with the current list.
	    
	InsertDevices();

	return false; // Do not unregister this callback.
}

HTREEITEM CDeviceEnumDlg::InsertDevices()
{
	// clear the property tree
	CTreeCtrl& propertyTree = m_TreeWnd.GetTreeCtrl();
	if ( propertyTree.GetSafeHwnd() )
		propertyTree.DeleteAllItems();

	// clear the device tree
	m_device_tree_ctrl.DeleteAllItems();
	HTREEITEM computerNode = m_device_tree_ctrl.InsertItem(_T("My Computer"), m_ImageList.GetImageCount()-1, m_ImageList.GetImageCount()-1);

	DeviceClass * pDevClass;
    std::list<Device*>::iterator device;
	std::list<Device*> deviceList;

    if ( m_device_type_bits & (1<<RECOVERY) )
	{
        pDevClass = gDeviceManager::Instance()[DeviceClass::DeviceTypeRecovery];

		HTREEITEM htiUsbClass = m_device_tree_ctrl.InsertItem(
			pDevClass->_classDesc.get(), 
			pDevClass->_classIconIndex.get(),
			pDevClass->_classIconIndex.get(),
			computerNode);
		m_device_tree_ctrl.SetItemData(htiUsbClass, (DWORD_PTR)pDevClass);
	    
        // Add Recovery Class devices
        deviceList.clear();
	    deviceList = pDevClass->Devices();
		for (device = deviceList.begin(); device != deviceList.end(); ++device)
		{
			HTREEITEM htiUsbDevice = m_device_tree_ctrl.InsertItem(
				(*device)->_description.get(), 
				(*device)->_classIconIndex.get(),
				(*device)->_classIconIndex.get(),
				htiUsbClass);
			m_device_tree_ctrl.SetItemData(htiUsbDevice, (DWORD_PTR)(*device));
			m_device_tree_ctrl.EnsureVisible(htiUsbDevice);
		} // for (usb device)
		m_device_tree_ctrl.EnsureVisible(htiUsbClass);
	} // if (recovery class)

	if ( m_device_type_bits & (1<<MASS_STORAGE) )
	{
// TODO: - move this decision inside DeviceManager
//		if ( gWinVersionInfo().IsWinNT() )
//		{
			// WINDOWS NT
	        pDevClass = gDeviceManager::Instance()[DeviceClass::DeviceTypeMsc];

			HTREEITEM htiVolumeClass = m_device_tree_ctrl.InsertItem(
				pDevClass->_classDesc.get(), 
				pDevClass->_classIconIndex.get(),
				pDevClass->_classIconIndex.get(),
				computerNode);
			m_device_tree_ctrl.SetItemData(htiVolumeClass, (DWORD_PTR)pDevClass);

			std::list<Device*>::iterator volume;
			std::list<Device*> volumeList;

			volumeList = pDevClass->Devices();
			for (volume = volumeList.begin(); volume != volumeList.end(); ++volume)
			{
				Volume* vol = dynamic_cast<Volume*>(*volume);

				HTREEITEM htiVolumeDevice = m_device_tree_ctrl.InsertItem(
					vol->_logicalDrive.get(), 
					vol->_classIconIndex.get(),
					vol->_classIconIndex.get(),
					htiVolumeClass);
				m_device_tree_ctrl.SetItemData(htiVolumeDevice, (DWORD_PTR)vol);

				Device* pDisk = vol->Parent();
				if ( pDisk ) 
//				for (disk=vol->Disks().begin(); disk<vol->Disks().end(); ++disk)
				{
					HTREEITEM htiDiskDevice = m_device_tree_ctrl.InsertItem(
						pDisk->_friendlyName.get(),
						pDisk->_classIconIndex.get(),
						pDisk->_classIconIndex.get(),
						htiVolumeDevice);
					m_device_tree_ctrl.SetItemData(htiDiskDevice, (DWORD_PTR)pDisk);

					if ( pDisk->Parent() )
					{
						HTREEITEM htiUsbDevice = m_device_tree_ctrl.InsertItem(
							pDisk->Parent()->_description.get(), 
							pDisk->Parent()->_classIconIndex.get(),
							pDisk->Parent()->_classIconIndex.get(),
							htiDiskDevice);
						m_device_tree_ctrl.SetItemData(htiUsbDevice, (DWORD_PTR)pDisk->Parent());
						m_device_tree_ctrl.EnsureVisible(htiUsbDevice);
					}
					m_device_tree_ctrl.EnsureVisible(htiDiskDevice);
				} // for (disk devices)
				m_device_tree_ctrl.EnsureVisible(htiVolumeDevice);
			} // for (volume devices)
			m_device_tree_ctrl.EnsureVisible(htiVolumeClass);
//		} // WinNT
//		else
//		{
//			// WINDOWS 9X
//			HTREEITEM htiDiskClass = m_device_tree_ctrl.InsertItem(
//				m_diskDevClass._classDesc.get(), 
//				m_diskDevClass._classIconIndex.get(),
//				m_diskDevClass._classIconIndex.get(),
//				computerNode);
//			m_device_tree_ctrl.SetItemData(htiDiskClass, (DWORD_PTR)&m_diskDevClass);
//
//			std::vector<Device*>::iterator disk;
//			std::vector<Device*> diskList = m_diskDevClass.Devices();
//
//			for (disk=diskList.begin(); disk<diskList.end(); ++disk)
//			{
//				HTREEITEM htiDiskDevice = m_device_tree_ctrl.InsertItem(
//					(*disk)->_description.get(), 
//					(*disk)->_classIconIndex.get(),
//					(*disk)->_classIconIndex.get(),
//					htiDiskClass);
//				m_device_tree_ctrl.SetItemData(htiDiskDevice, (DWORD_PTR)(*disk));
//
//				if ( (*disk)->IsUsb() )
//				{
//					HTREEITEM htiUsbDevice = m_device_tree_ctrl.InsertItem(
//						(*disk)->UsbDevice()->_description.get(), 
//						(*disk)->UsbDevice()->_classIconIndex.get(),
//						(*disk)->UsbDevice()->_classIconIndex.get(),
//						htiDiskDevice);
//					m_device_tree_ctrl.SetItemData(htiUsbDevice, (DWORD_PTR)(*disk)->UsbDevice());
//					m_device_tree_ctrl.EnsureVisible(htiUsbDevice);
//				}
//				m_device_tree_ctrl.EnsureVisible(htiDiskDevice);
//			} // for (disk devices)
//			m_device_tree_ctrl.EnsureVisible(htiDiskClass);
//		} // Win98
	}
	if ( m_device_type_bits & (1<<MTP) )
	{
        pDevClass = gDeviceManager::Instance()[DeviceClass::DeviceTypeMtp];

		HTREEITEM htiMtpClass = m_device_tree_ctrl.InsertItem(
			pDevClass->_classDesc.get(), 
			pDevClass->_classIconIndex.get(),
			pDevClass->_classIconIndex.get(),
			computerNode);
		m_device_tree_ctrl.SetItemData(htiMtpClass, (DWORD_PTR)pDevClass);

	    // Add MTP devices
        deviceList.clear();
	    deviceList = pDevClass->Devices();
	    for (device = deviceList.begin(); device != deviceList.end(); ++device)
		{
			HTREEITEM htiMtpDevice = m_device_tree_ctrl.InsertItem(
				(*device)->_description.get(), 
				(*device)->_classIconIndex.get(),
				(*device)->_classIconIndex.get(),
				htiMtpClass);
			m_device_tree_ctrl.SetItemData(htiMtpDevice, (DWORD_PTR)(*device));
			m_device_tree_ctrl.EnsureVisible(htiMtpDevice);
		} // for (mtp device)
		m_device_tree_ctrl.EnsureVisible(htiMtpClass);
	}

	if ( m_device_type_bits & (1<<HID) )
	{
        pDevClass = gDeviceManager::Instance()[DeviceClass::DeviceTypeHid];

		HTREEITEM htiHidClass = m_device_tree_ctrl.InsertItem(
			pDevClass->_classDesc.get(), 
			pDevClass->_classIconIndex.get(),
			pDevClass->_classIconIndex.get(),
			computerNode);
		m_device_tree_ctrl.SetItemData(htiHidClass, (DWORD_PTR)pDevClass);

    	// Add HID devices
        deviceList.clear();
	    deviceList = pDevClass->Devices();
	    for (device = deviceList.begin(); device != deviceList.end(); ++device)
		{
			HTREEITEM htiHidDevice = m_device_tree_ctrl.InsertItem(
				(*device)->_description.get(), 
				(*device)->_classIconIndex.get(),
				(*device)->_classIconIndex.get(),
				htiHidClass);
			m_device_tree_ctrl.SetItemData(htiHidDevice, (DWORD_PTR)(*device));

			if ( (*device)->Parent() )
			{
				HTREEITEM htiUsbDevice = m_device_tree_ctrl.InsertItem(
					(*device)->Parent()->_description.get(), 
					(*device)->Parent()->_classIconIndex.get(),
					(*device)->Parent()->_classIconIndex.get(),
					htiHidDevice);
				m_device_tree_ctrl.SetItemData(htiUsbDevice, (DWORD_PTR)(*device)->Parent());
				m_device_tree_ctrl.EnsureVisible(htiUsbDevice);
			}
		} // for (hid device)
		m_device_tree_ctrl.EnsureVisible(htiHidClass);
	} // if (hid class)

	if ( m_device_type_bits & (1<<USB_CONTROLLER) )
	{
        pDevClass = gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbController];

		HTREEITEM htiUsbControllerClass = m_device_tree_ctrl.InsertItem(
			pDevClass->_classDesc.get(), 
			pDevClass->_classIconIndex.get(),
			pDevClass->_classIconIndex.get(),
			computerNode);
		m_device_tree_ctrl.SetItemData(htiUsbControllerClass, (DWORD_PTR)pDevClass);

    	// Add USB Controller devices
        deviceList.clear();
	    deviceList = pDevClass->Devices();
	    for (device = deviceList.begin(); device != deviceList.end(); ++device)
		{
			HTREEITEM htiUsbController = m_device_tree_ctrl.InsertItem(
				(*device)->_description.get(), 
				(*device)->_classIconIndex.get(),
				(*device)->_classIconIndex.get(),
				htiUsbControllerClass);
			m_device_tree_ctrl.SetItemData(htiUsbController, (DWORD_PTR)(*device));

    		// Add USB Root Hub devices
			usb::Controller* pController = dynamic_cast<usb::Controller*>(*device);
			usb::Hub* pHub = dynamic_cast<usb::Hub*>(pController->GetRootHub());
			if ( pHub )
			{
				HTREEITEM htiUsbRootHub = m_device_tree_ctrl.InsertItem(
					pHub->name().c_str(), 
					pHub->_classIconIndex.get(),
					pHub->_classIconIndex.get(),
					htiUsbController);
				m_device_tree_ctrl.SetItemData(htiUsbRootHub, (DWORD_PTR)pHub);
				m_device_tree_ctrl.EnsureVisible(htiUsbRootHub);

    			// Add USB Ports
				property::Property::PropertyIterator port;
				property::Property::PropertyList* pPortList = pHub->Ports();
				for (port = pPortList->begin(); port != pPortList->end(); ++port)
				{
					CStdString desc = (*port)->name() + _T(": ");

					HTREEITEM htiUsbPort = m_device_tree_ctrl.InsertItem(
						desc, 
// TODO:clw get device icon or "disconnected" icon
//						pHub->_classIconIndex.get(),
//						pHub->_classIconIndex.get(),
						htiUsbRootHub);
					m_device_tree_ctrl.SetItemData(htiUsbPort, (DWORD_PTR)(*port));
					m_device_tree_ctrl.EnsureVisible(htiUsbPort);
				}
			}
		} // for (usb controller node)
		m_device_tree_ctrl.EnsureVisible(htiUsbControllerClass);
	} // if (usb controller class)

	return HTREEITEM();
}

void CDeviceEnumDlg::InitPropertyTree()
{
	// find the placeholder, get its position and destroy it
	CRect rcTreeWnd;
	CWnd* pPlaceholderWnd = GetDlgItem(IDC_PROPERTY_TREE);
	pPlaceholderWnd->GetWindowRect(&rcTreeWnd);
	ScreenToClient(&rcTreeWnd);
	pPlaceholderWnd->DestroyWindow();

	// create the multi-column tree window
	m_TreeWnd.CreateEx(WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD | WS_VISIBLE,
		rcTreeWnd, this, IDC_PROPERTY_TREE);

	CTreeCtrl& tree = m_TreeWnd.GetTreeCtrl();
	CHeaderCtrl& header = m_TreeWnd.GetHeaderCtrl();

	DWORD dwStyle = GetWindowLong(tree, GWL_STYLE);
	dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_FULLROWSELECT | TVS_DISABLEDRAGDROP;
	SetWindowLong(tree, GWL_STYLE, dwStyle);

	// TODO: need a Property Type icon image list ( GUID, int, string, ...)
	// tree.SetImageList(&m_ImageList, TVSIL_NORMAL);
	// tree.SetImageList(&m_ImageList, LVSIL_SMALL);

	HDITEM hditem;
	hditem.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
	hditem.fmt = HDF_LEFT | HDF_STRING;
	hditem.cxy = (rcTreeWnd.Width()-GetSystemMetrics(SM_CXVSCROLL))/3;
	hditem.pszText = _T("Property");
	header.InsertItem(0, &hditem);
	hditem.cxy = (rcTreeWnd.Width()-GetSystemMetrics(SM_CXVSCROLL))*2/3;
	hditem.pszText = _T("Value");
	header.InsertItem(1, &hditem);
	m_TreeWnd.UpdateColumns();
}

void CDeviceEnumDlg::OnTvnSelchangedDeviceTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	// clear the tree
	CTreeCtrl& propertyTree = m_TreeWnd.GetTreeCtrl();
	propertyTree.DeleteAllItems();

	// get the conainter of properties
	Property* pPropertyContainer = dynamic_cast<Property*>((Property*)pNMTreeView->itemNew.lParam);
	if ( pPropertyContainer == NULL )
		return;

	// get the property list from the container
	Property::PropertyList* pList = pPropertyContainer->getList();

	// add the properties to the tree
	InsertPropertyItems(pList, TVI_ROOT);

	*pResult = 0;
}

HTREEITEM CDeviceEnumDlg::InsertPropertyItems(Property::PropertyList* pList, HTREEITEM parent)
{
	HTREEITEM hti;
	Property::PropertyIterator item;
	CTreeCtrl& propertyTree = m_TreeWnd.GetTreeCtrl();

	for ( item = pList->begin(); item != pList->end(); ++item)
    {
		Property* property = *item;
		// build a string with the property name and value(as a string).
		CStdString str = property->name() + _T("\t") + property->ToString();
		hti = propertyTree.InsertItem(str, parent);

		// see if the property is actually a PropertyContainer
		if ( property->isContainer() )
			InsertPropertyItems(property->getList(), hti);
    }

	return hti;
}
void CDeviceEnumDlg::OnLvnItemchangedDevicetypeList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->uOldState == 0 && pNMLV->uNewState == 0)
		return;	// No change

	BOOL bPrevState = (BOOL)(((pNMLV->uOldState & 
				LVIS_STATEIMAGEMASK)>>12)-1);   // Old check box state
	if (bPrevState < 0)	// On startup there's no previous state 
		bPrevState = 0; // so assign as false (unchecked)

	// New check box state
	BOOL bChecked=(BOOL)(((pNMLV->uNewState & LVIS_STATEIMAGEMASK)>>12)-1);   
	if (bChecked < 0) // On non-checkbox notifications assume false
		bChecked = 0; 

	if (bPrevState == bChecked) // No change in check box
		return;
	
	// Now bChecked holds the new check box state
	if ( bChecked ) {
		m_device_type_bits |= m_dev_type_ctrl.GetItemData(pNMLV->iItem);
	}
	else {
		m_device_type_bits &= ~m_dev_type_ctrl.GetItemData(pNMLV->iItem);
	}
	theApp.WriteProfileInt(_T("Class Filters"), _T("Device Classes"), m_device_type_bits);

	// put the selected devices in the tree
	InsertDevices();

	*pResult = 0;
}


void CDeviceEnumDlg::OnLvnItemchangedUsbFilterList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->uOldState == 0 && pNMLV->uNewState == 0)
		return;	// No change

	BOOL bPrevState = (BOOL)(((pNMLV->uOldState & 
				LVIS_STATEIMAGEMASK)>>12)-1);   // Old check box state
	if (bPrevState < 0)	// On startup there's no previous state 
		bPrevState = 0; // so assign as false (unchecked)

	// New check box state
	BOOL bChecked=(BOOL)(((pNMLV->uNewState & LVIS_STATEIMAGEMASK)>>12)-1);   
	if (bChecked < 0) // On non-checkbox notifications assume false
		bChecked = 0; 

	if (bPrevState == bChecked) // No change in check box
		return;
	
	// Now bChecked holds the new check box state
	CString name = m_usb_ids_filter_ctrl.GetItemText(pNMLV->iItem, 0);
	theApp.WriteProfileInt(_T("USB Filters"), name, bChecked);

	UpdateUsbFilters();

	*pResult = 0;
}

void CDeviceEnumDlg::InitFilterListCtrl()
{
	m_usb_ids_filter_ctrl.DeleteAllItems();

	HKEY hKey; DWORD index=0; TCHAR name[MAX_PATH]; DWORD name_size; DWORD data; DWORD data_size = sizeof(DWORD);
	if ( RegOpenKey(HKEY_CURRENT_USER, _T("Software\\SigmaTel\\DeviceEnum\\USB Filters"), &hKey) == ERROR_SUCCESS )
	{
		while (1)
		{
			name_size = MAX_PATH;
			LONG error = RegEnumValue(hKey, index, &name[0], &name_size, NULL, NULL, (LPBYTE)&data, &data_size);
			if (error == ERROR_NO_MORE_ITEMS)
				break;
			
			int item = m_usb_ids_filter_ctrl.InsertItem(index, name);
			if ( data )
			{
				m_usb_ids_filter_ctrl.SetCheck(item);
			}

			++index;
		}

		RegCloseKey(hKey);
	}
}

void CDeviceEnumDlg::UpdateUsbFilters(void)
{
	CStringArray filterPairs;

	HKEY hKey; DWORD index=0; TCHAR name[MAX_PATH]; DWORD name_size; DWORD data; DWORD data_size = sizeof(DWORD);
	if ( RegOpenKey(HKEY_CURRENT_USER, _T("Software\\SigmaTel\\DeviceEnum\\USB Filters"), &hKey) == ERROR_SUCCESS )
	{
		while (1)
		{
			name_size = MAX_PATH;
			LONG error = RegEnumValue(hKey, index, &name[0], &name_size, NULL, NULL, (LPBYTE)&data, &data_size);
			if (error == ERROR_NO_MORE_ITEMS)
				break;
			
			if ( data )
				filterPairs.Add(name);

			++index;
		}

		RegCloseKey(hKey);
	}

    gDeviceManager::Instance().ClearFilters();

	for (INT_PTR idx=0; idx < filterPairs.GetCount(); ++idx)
	{
		CString vid, pid; int pos=0;
		vid = filterPairs[idx].Tokenize(_T("/"), pos);
		if ( pos != -1 )
			pid = filterPairs[idx].Tokenize(_T("/"), pos);
		
		gDeviceManager::Instance().AddFilter(vid,pid);
	}

	// TODO: Need to destroy the list and recreate the list since devices
	// are not added to the list unless they meet the filter criteria. Revisit
	// CreateDevice() and decide if this is really what we want to do.
	OnBnClickedRefreshButton();
}

void CDeviceEnumDlg::OnCancel()
{
	gDeviceManager::Instance().Close();
	CDialog::OnCancel();
}

void CDeviceEnumDlg::OnBnClickedRejectAutoplayCheck()
{
	int checked = m_reject_autoplay_ctrl.GetCheck();
	AfxGetApp()->WriteProfileInt(_T("Settings"), _T("RejectAutoPlay"), checked);
	gDeviceManager::Instance().SetCancelAutoPlay(checked == TRUE);
}
