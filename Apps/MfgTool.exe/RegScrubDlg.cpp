// RegScrubDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "RegScrubDlg.h"

// CRegScrubDlg dialog

IMPLEMENT_DYNAMIC(CRegScrubDlg, CDialog)

/////////////////////////////////////////////////////////////////////////////
CRegScrubDlg::CRegScrubDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRegScrubDlg::IDD, pParent)
	, m_RemoveStaticIDEntryCkCtrl(FALSE)
	, m_RemoveRecoveryEntries(FALSE)
{
}

/////////////////////////////////////////////////////////////////////////////
CRegScrubDlg::~CRegScrubDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
void CRegScrubDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBString(pDX, IDC_PROFILE_COMBO, m_ProfileNameStr);
	DDX_Control(pDX, IDC_PROFILE_COMBO, m_ProfileComboBox);
	DDX_Text(pDX, IDC_SCSI_PRODUCT_TEXT, m_SCSIProductStr);
	DDX_Text(pDX, IDC_USB_VID_TEXT, m_SCSIVenderIDStr);
	DDX_Text(pDX, IDC_USB_PID_TEXT, m_SCSIProductIDStr);
	DDX_Text(pDX, IDC_SCSI_MFG_TEXT, m_SCSIMfgStr);
	DDX_Text(pDX, IDC_REG_DELETING, m_DeletingKeyStr);
	DDX_Text(pDX, IDC_REG_PROCESSING, m_ProcessingKeyStr);
	DDX_Text(pDX, IDC_REG_DEVICE_COUNT, m_DeviceCount);
	DDX_Check(pDX, IDC_REMOVE_STATIC_IDS, m_RemoveStaticIDEntryCkCtrl);
	DDX_Check(pDX, IDC_REMOVE_RECOVERY_ENTRIES, m_RemoveRecoveryEntries);
}

BEGIN_MESSAGE_MAP(CRegScrubDlg, CDialog)
	//{{AFX_MSG_MAP(CRegScrubDlg)
	ON_BN_CLICKED(IDC_SCRUB, OnScrub)
	ON_CBN_SELCHANGE(IDC_PROFILE_COMBO, OnCbnSelchangeProductDescCombo)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_REMOVE_STATIC_IDS, OnBnClickedRemoveStaticIds)
	ON_BN_CLICKED(IDC_REMOVE_RECOVERY_ENTRIES, OnBnClickedRemoveRecoveryEntries)
END_MESSAGE_MAP()

// CRegScrubDlg message handlers

/////////////////////////////////////////////////////////////////////////////
void CRegScrubDlg::OnCancel() 
{
	CDialog::OnCancel();
}
/////////////////////////////////////////////////////////////////////////////
BOOL CRegScrubDlg::OnInitDialog()
{  
	CDialog::OnInitDialog();

	Localize();
	m_DeviceCount = 0;
	m_ValidProfile = true;
	m_DeletingKeyStr = (_T(""));
	m_ProcessingKeyStr = (_T(""));
	m_RemoveStaticIDEntryCkCtrl = FALSE;

	m_ProfileNameStr = (_T(""));

	m_SCSIMfgStr = (_T(""));
	m_SCSIProductStr = (_T(""));

	m_SCSIProductIDStr = (_T(""));
	m_SCSIVenderIDStr = (_T(""));

	m_ConfigMgrProfileNameStr = theApp.GetConfigMgr()->GetPlayerProfileName();

	InitProfileComboBox();

	UpdateData(FALSE);

	return TRUE;  // Return TRUE unless you set the focus to a control.
                // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CRegScrubDlg::InitProfileComboBox(void)
{
	int ProfileComboBoxIndex = 0;
	int ProfileComboBoxSelect = -1;
	CPlayerProfile *pProfile = NULL;
	
	m_ProfileComboBox.ResetContent();

	while ( (pProfile = theApp.GetConfigMgr()->GetListProfile(ProfileComboBoxIndex)) != NULL)
	{
		m_ProfileComboBox.AddString(pProfile->GetName());
		m_ProfileComboBox.SetItemDataPtr(ProfileComboBoxIndex, pProfile);
		if (_tcscmp(pProfile->GetName(), m_ConfigMgrProfileNameStr) == 0)
		{
			ProfileComboBoxSelect = ProfileComboBoxIndex;
		}
		ProfileComboBoxIndex++;
	}

	if (ProfileComboBoxSelect >= 0)
	{
		m_ProfileComboBox.SetCurSel(ProfileComboBoxSelect);
		pProfile = (CPlayerProfile *) m_ProfileComboBox.GetItemDataPtr(ProfileComboBoxSelect);

		m_ProfileNameStr = pProfile->GetName();
		m_SCSIMfgStr = pProfile->GetScsiMfg();
		m_SCSIProductStr = pProfile->GetScsiProduct();
		m_SCSIProductIDStr = pProfile->GetUsbPid();
		m_SCSIVenderIDStr = pProfile->GetUsbVid();
	}
}

/////////////////////////////////////////////////////////////////////////////
void CRegScrubDlg::OnCbnSelchangeProductDescCombo()
{
	CPlayerProfile * pProfile = NULL;

    pProfile = (CPlayerProfile *)m_ProfileComboBox.GetItemDataPtr(m_ProfileComboBox.GetCurSel());

	if (pProfile)
	{
		m_ValidProfile = true;
		m_ProfileNameStr = pProfile->GetName();
		m_SCSIMfgStr = pProfile->GetScsiMfg();
		m_SCSIProductStr = pProfile->GetScsiProduct();
		m_SCSIProductIDStr = pProfile->GetUsbPid();
		m_SCSIVenderIDStr = pProfile->GetUsbVid();
		UpdateData(FALSE);
	}
	else
	{
		CString resStr;
		resStr.LoadString(IDS_REGSCRUB_INVALID_PROFILE);
		MessageBeep(0);
		AfxMessageBox(resStr, MB_ICONSTOP | MB_OK);
		m_ValidProfile = false;
		m_ProfileNameStr = (_T(""));
		m_SCSIMfgStr = (_T(""));
		m_SCSIProductStr = (_T(""));
		m_SCSIProductIDStr = (_T(""));
		m_SCSIVenderIDStr = (_T(""));
		UpdateData(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
void CRegScrubDlg::OnBnClickedRemoveStaticIds()
{
	CString Msg;

	UpdateData(TRUE);

	if(m_RemoveStaticIDEntryCkCtrl)
	{
		Msg.LoadString(IDS_REGSCRUB_CONFIRM_PROMPT);

		if( AfxMessageBox(Msg,MB_YESNO | MB_ICONEXCLAMATION) == IDNO )
		{
			m_RemoveStaticIDEntryCkCtrl = FALSE;
			UpdateData(FALSE);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
void CRegScrubDlg::OnBnClickedRemoveRecoveryEntries()
{
	CString Msg;

	UpdateData(TRUE);

	if(m_RemoveRecoveryEntries)
	{
		Msg.LoadString(IDS_REGSCRUB_CONFIRM_PROMPT);

		if( AfxMessageBox(Msg,MB_YESNO | MB_ICONEXCLAMATION) == IDNO )
		{
			m_RemoveRecoveryEntries = FALSE;
			UpdateData(FALSE);
		}
	}

}


/////////////////////////////////////////////////////////////////////////////
void CRegScrubDlg::OnScrub() 
{
	CString Msg;
	DWORD status = ERROR_SUCCESS;
	m_DeviceCount = 0;
	m_DeletingKeyStr.Format(_T(""));
	m_ProcessingKeyStr.Format(_T(""));
	UpdateData(FALSE);

	if(!IsPlatformNT())
	{
		MessageBeep(0);
		Msg.LoadString(IDS_REGSCRUB_WRONG_OS);
		AfxMessageBox(Msg,MB_ICONSTOP);
		return;
	}
	
	if(!m_ValidProfile)
	{
		Msg.LoadString(IDS_REGSCRUB_INVALID_PROFILE);
		MessageBeep(0);
		AfxMessageBox(Msg, MB_ICONSTOP | MB_OK);
		return;
	}

	GetDlgItem(IDC_SCRUB)->EnableWindow(FALSE);

	status = RemoveUSBRegEntries();
	if (status == ERROR_SUCCESS)
	{
		status = RemoveUSBStorRegEntries();
		if (status == ERROR_SUCCESS)
		{
			if (m_RemoveStaticIDEntryCkCtrl)
			{
				// old style
				m_SCSIMfgStr = _T("SIGMATEL");
				m_SCSIProductStr = _T("STMFGTOOL");
				m_SCSIProductIDStr = _T("0000");
				m_SCSIVenderIDStr =  _T("066F");

				status = RemoveUSBRegEntries();
				if (status == ERROR_SUCCESS)
				{
					RemoveUSBStorRegEntries();
				}
			}

			if (status == ERROR_SUCCESS && m_RemoveRecoveryEntries)
			{
				status = RemoveHIDRegEntries();

				if (status == ERROR_SUCCESS)
				{
					// 37ff updater
					m_SCSIMfgStr = _T("_GENERIC");
					m_SCSIProductStr = _T("MSC_Recovery");
					m_SCSIProductIDStr = _T("37FF");
					m_SCSIVenderIDStr =  _T("066F");

					status = RemoveUSBRegEntries();
					if (status == ERROR_SUCCESS)
					{
						RemoveUSBStorRegEntries();
					}
				}
			}
		}
	}
	
	// need to handle ERROR conditions in Remove????RegEntries() ?
	if (status != ERROR_SUCCESS)
	{
		AfxMessageBox(_T("Failed to delete registry entries"), MB_OK);
	}

	OnCbnSelchangeProductDescCombo();

	m_DeletingKeyStr.LoadString(IDS_REGSCRUB_FINISHED);
	m_ProcessingKeyStr.LoadString(IDS_REGSCRUB_FINISHED);
	UpdateData(FALSE);
	GetDlgItem(IDC_SCRUB)->EnableWindow(TRUE);
}


/*****************************************************************************/
#define NUM_HID_PIDS	3
DWORD CRegScrubDlg::RemoveHIDRegEntries()
{
	BOOL Success;
	CONFIGRET cr;
	TCHAR buf[4096];
	CString HIDStr,HWHIDHardwareIDName;
	DWORD HWHIDIndex,RetVal=ERROR_SUCCESS;
    SP_DEVINFO_DATA HWHIDDevInfoData;
	UINT HidPids[NUM_HID_PIDS] = {0x3700, 0x3770, 0x3780};
	BOOL bFoundAtLeastOne = FALSE;

	// Open a handle to the plug and play dev node.  SetupDiGetClassDevs() returns a device information 
	// set that contains info on all installed devices of a specified class.
	m_HWHIDDevInfo=SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_HIDCLASS,NULL,NULL,/*DIGCF_PRESENT|*/0);

    if(m_HWHIDDevInfo==INVALID_HANDLE_VALUE)
	{
		RetVal=GetLastError();
		ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiGetClassDevs() handle value returned, ErrorCode = %d"), RetVal);
		return RetVal; 
	}

    RetVal=ERROR_FILE_NOT_FOUND;

	HWHIDDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);

	for(HWHIDIndex=0;RetVal=SetupDiEnumDeviceInfo(m_HWHIDDevInfo,HWHIDIndex,&HWHIDDevInfoData);HWHIDIndex++)
    {   // get the hardware id name
		cr=CM_Get_Device_ID(HWHIDDevInfoData.DevInst,buf,sizeof(buf),0);
	    HWHIDHardwareIDName=buf;	        
		HWHIDHardwareIDName.MakeUpper();
		m_ProcessingKeyStr = HWHIDHardwareIDName;
		m_DeletingKeyStr = _T("");
        
        HIDStr = _T("HID\\VID_066F");
	    if(HWHIDHardwareIDName.Find(HIDStr) == -1)
			continue;

		for (int i = 0; i < NUM_HID_PIDS; ++i)
		{
	        HIDStr.Format(_T("HID\\VID_066F&PID_%X"), HidPids[i]);

		    if(HWHIDHardwareIDName.Find(HIDStr)!=-1)
		    {//remove enum\USB device and its direct control class link
				if (!bFoundAtLeastOne)
				{
					bFoundAtLeastOne = true;
//					continue;
				}

			    cr=CM_Get_Device_ID(HWHIDDevInfoData.DevInst,buf,sizeof(buf),0); 
				ATLTRACE(_T("	Scrub() found %s.\n"),buf);
			    Success=SetupDiCallClassInstaller(DIF_REMOVE,m_HWHIDDevInfo,&HWHIDDevInfoData);
			    if(!Success)
			    {
   					RetVal=GetLastError();
					ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiCallClassInstaller(DIF_REMOVE) value returned, ErrorCode = %d"), RetVal);
				    break;
			    }
			    else
				{
					DWORD status;

	   				m_DeviceCount++;	
					m_DeletingKeyStr=buf;
					UpdateData(FALSE);

					// 37ff updater
					m_SCSIMfgStr = _T("_GENERIC");
					m_SCSIProductStr = _T("ROM_Recovery");
					m_SCSIProductIDStr.Format(_T("%04X"), HidPids[i]);
					m_SCSIVenderIDStr =  _T("066F");

					status = RemoveUSBRegEntries();
					if (status == ERROR_SUCCESS)
					{
						RemoveUSBStorRegEntries();
					}
					RetVal=ERROR_SUCCESS;
					ATLTRACE(_T("	Scrub() removed: %s.\n"),buf);
					break;
   				}
			}
		}
	}

	SetupDiDestroyDeviceInfoList(m_HWHIDDevInfo);

	return RetVal;
}

/////////////////////////////////////////////////////////////////////////////
DWORD CRegScrubDlg::RemoveUSBRegEntries()
{
	BOOL Success;
	CONFIGRET cr;
	TCHAR buf[4096];
	BOOL bFoundAtLeastOne = FALSE;
	CString USBStr, HWUSBHardwareIDName;
	DWORD HWUSBIndex,RetVal=ERROR_SUCCESS;

	// Open a handle to the plug and play dev node.  SetupDiGetClassDevs() returns a device information 
	// set that contains info on all installed devices of a specified class.
	m_HWUSBDevInfo=SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_USB_DEVICE,NULL,NULL,(/*DIGCF_PRESENT|*/DIGCF_INTERFACEDEVICE));

    if(m_HWUSBDevInfo==INVALID_HANDLE_VALUE)
	{
		RetVal=GetLastError();
		ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiGetClassDevs() handle value returned, ErrorCode = %d"), RetVal);
		return RetVal; 
	}

	m_HWUSBDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);

	USBStr.Format(_T("USB\\Vid_%s&Pid_%s&Rev_"), m_SCSIVenderIDStr, m_SCSIProductIDStr);
	USBStr.MakeUpper();

	RetVal=ERROR_FILE_NOT_FOUND;
	for(HWUSBIndex=0;RetVal=SetupDiEnumDeviceInfo(m_HWUSBDevInfo,HWUSBIndex,&m_HWUSBDevInfoData);HWUSBIndex++)
    {   // get the hardware id name
	    HWUSBHardwareIDName=GetDeviceRegistryProperty(SPDRP_HARDWAREID);	
		HWUSBHardwareIDName.MakeUpper();
		m_ProcessingKeyStr = HWUSBHardwareIDName;
		UpdateData(FALSE);

		if(HWUSBHardwareIDName.Find(USBStr)!=-1)
		{//remove enum\USB device and its direct control class link
			if (!bFoundAtLeastOne)
			{
				bFoundAtLeastOne = true;
//				continue;
			}
			cr=CM_Get_Device_ID(m_HWUSBDevInfoData.DevInst,buf,sizeof(buf),0); 
			ATLTRACE(_T("	Scrub() found %s.\n"),buf);
			Success=SetupDiCallClassInstaller(DIF_REMOVE,m_HWUSBDevInfo,&m_HWUSBDevInfoData);
			if(!Success)
			{
				RetVal=GetLastError();
				ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiCallClassInstaller(DIF_REMOVE) value returned, ErrorCode = %d"), RetVal);
				break;
			}
			else
			{
				m_DeviceCount++;	
				m_DeletingKeyStr=buf;
				UpdateData(FALSE);
				RetVal=ERROR_SUCCESS;
				ATLTRACE(_T("	Scrub() removed: %s.\n"),buf);
			}
		}
	}  // end for()

	SetupDiDestroyDeviceInfoList(m_HWUSBDevInfo);

	return RetVal;
}

/////////////////////////////////////////////////////////////////////////////
DWORD CRegScrubDlg::RemoveUSBStorRegEntries()
{
	HKEY hKey;
	BOOL Success;
	CONFIGRET cr;
	ULONG DataSize=256;
	LONG QueryStatus;
	BOOL bFoundAtLeastOne = FALSE;
	TCHAR buf[4096],ParentIdPrefixValue[256];
	CString USBStorStr,USBStorStr2,HWDiskDrvDevStr;
	HDEVINFO HWDiskDrvDevInfo,HWVolumeDevInfo;
	SP_DEVINFO_DATA HWDiskDrvDevInfoData,HWVolumeDevInfoData;
	DWORD HWDiskDrvIndex,RetVal=ERROR_SUCCESS,TypeValue=REG_SZ;;

	// Open a handle to the plug and play dev node.  SetupDiGetClassDevs() returns a device information 
	// set that contains info on all installed devices of a specified class.
	HWDiskDrvDevInfo=SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_DISKDRIVE,NULL,NULL,/*DIGCF_PRESENT*/0);
	HWVolumeDevInfo=SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_VOLUME,NULL,NULL,/*DIGCF_PRESENT*/0);

    if(HWDiskDrvDevInfo==INVALID_HANDLE_VALUE||HWVolumeDevInfo==INVALID_HANDLE_VALUE)
	{
		RetVal=GetLastError();
		ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiGetClassDevs() handle value returned, ErrorCode = %d"), RetVal);
		return RetVal; 
	}

	HWDiskDrvDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);
	HWVolumeDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);

	USBStorStr.Format(_T("USBSTOR\\Disk&Ven_%s&Prod_%s&Rev_"), m_SCSIMfgStr, m_SCSIProductStr);
	USBStorStr.MakeUpper();	

	for(HWDiskDrvIndex=0; RetVal=SetupDiEnumDeviceInfo(HWDiskDrvDevInfo,HWDiskDrvIndex,&HWDiskDrvDevInfoData);HWDiskDrvIndex++)
	{
		cr=CM_Get_Device_ID(HWDiskDrvDevInfoData.DevInst,buf,sizeof(buf),0); 
		HWDiskDrvDevStr=buf; 
		HWDiskDrvDevStr.MakeUpper();
		m_ProcessingKeyStr = HWDiskDrvDevStr;
		UpdateData(FALSE);
		if(HWDiskDrvDevStr.Find(USBStorStr)!=-1)
		{ //remove enum\USBSTOR device and its direct control class link	
			if (!bFoundAtLeastOne)
			{
				bFoundAtLeastOne = true;
//				continue;
			}

			DataSize=256;
			ParentIdPrefixValue[0]=NULL;
			USBStorStr2.Format(_T("SYSTEM\\CurrentControlSet\\Enum\\%s"),HWDiskDrvDevStr);
			RegOpenKeyEx(HKEY_LOCAL_MACHINE, USBStorStr2, 0, KEY_READ, &hKey);
			QueryStatus = RegQueryValueEx(hKey,_T("ParentIdPrefix"),NULL,&TypeValue,
										 (PUCHAR)ParentIdPrefixValue,&DataSize);
			m_USBStorageStr.Format(_T("STORAGE\\RemovableMedia\\%s&RM"),ParentIdPrefixValue);
			m_USBStorageStr.MakeUpper();

			RemoveRemovableMediaRegEntries();

			// get ParentIdPrefix, search & delete under Enum\Storage\RemovableMedia then do DIF_REMOVE
			Success=SetupDiCallClassInstaller(DIF_REMOVE,HWDiskDrvDevInfo,&HWDiskDrvDevInfoData);
			if(!Success)
			{
				RetVal=GetLastError();
				ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiCallClassInstaller(DIF_REMOVE) value returned, ErrorCode = %d"), RetVal);
				break;
			}
			else
			{
				m_DeviceCount++;	
				m_DeletingKeyStr=HWDiskDrvDevStr;
				UpdateData(FALSE);
				RetVal=ERROR_SUCCESS;
				ATLTRACE(_T("	Scrub() removed: %s.\n"),HWDiskDrvDevStr);
			}
		}
	}		
	
	SetupDiDestroyDeviceInfoList(HWDiskDrvDevInfo);
	
	return RetVal;
}

/////////////////////////////////////////////////////////////////////////////
DWORD CRegScrubDlg::RemoveRemovableMediaRegEntries()
{
	BOOL Success;
	CONFIGRET cr;
	TCHAR buf[4096];
	CString HWVolumeDevInstStr;
	HDEVINFO HWVolumeDevInfo;
	SP_DEVINFO_DATA HWVolumeDevInfoData;
	DWORD HWVolumeIndex,RetVal=ERROR_SUCCESS,TypeValue=REG_SZ;;

	HWVolumeDevInfo=SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_VOLUME,NULL,NULL,/*DIGCF_PRESENT*/0);

    if(HWVolumeDevInfo==INVALID_HANDLE_VALUE)
	{
		RetVal=GetLastError();
		ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiGetClassDevs() handle value returned, ErrorCode = %d"), RetVal);
		return RetVal; 
	}

	HWVolumeDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);

	for(HWVolumeIndex=0; RetVal=SetupDiEnumDeviceInfo(HWVolumeDevInfo,HWVolumeIndex,&HWVolumeDevInfoData);HWVolumeIndex++)
	{		
		cr=CM_Get_Device_ID(HWVolumeDevInfoData.DevInst,buf,sizeof(buf),0);
		HWVolumeDevInstStr=buf; 
		HWVolumeDevInstStr.MakeUpper();
		m_ProcessingKeyStr=HWVolumeDevInstStr;
		UpdateData(FALSE);
		if(HWVolumeDevInstStr.Find(m_USBStorageStr)!=-1)
		{	//remove enum\Storage\RemovableMedia devnode and its direct control class link
			Success=SetupDiCallClassInstaller(DIF_REMOVE,HWVolumeDevInfo,&HWVolumeDevInfoData);
			if(!Success)
			{
				RetVal=GetLastError();
				ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiCallClassInstaller(DIF_REMOVE) value returned, ErrorCode = %d"), RetVal);
			}
			else
			{
				m_DeviceCount++;	
				m_DeletingKeyStr=HWVolumeDevInstStr;
				UpdateData(FALSE);
				RetVal=ERROR_SUCCESS;
				ATLTRACE(_T("	Scrub() removed: %s.\n"),HWVolumeDevInstStr);
			}
			break;
		}
	}

	SetupDiDestroyDeviceInfoList(HWVolumeDevInfo);

	return RetVal;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CRegScrubDlg::UpdateData(BOOL bSaveAndValidate)
{	
	BOOL bRetVal;
	
	bRetVal=CWnd::UpdateData(bSaveAndValidate);
	
	UpdateWindow();
	
	return bRetVal;
}

/*****************************************************************************
*
*  Function:    IsPlatformNT
*
*  Synopsis:	determines if the platform is NT-based
*
*  Arguments:	
*
*  Returns:		Result of the operation
*
*****************************************************************************/
BOOL CRegScrubDlg::IsPlatformNT()
{ 
    OSVERSIONINFOA osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA((OSVERSIONINFOA*)&osvi);
    if (osvi.dwMajorVersion >= 6)
		return FALSE; // Vista not supported

    return (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId);    
}

/////////////////////////////////////////////////////////////////////////////
CString CRegScrubDlg::GetDeviceRegistryProperty(DWORD Property)
{
	DWORD BytesReturned;
	PBYTE pRegBuf = NULL;
	CString sRegistryProperty(_T(""));

	// get the required	length of the buffer first
	if(SetupDiGetDeviceRegistryProperty(m_HWUSBDevInfo, &m_HWUSBDevInfoData,
										Property, NULL, NULL, 0, &BytesReturned))
	{
		ATLTRACE(_T("\r\n *** ERROR *** SetupDiGetDeviceRegistryProperty() failed, ErrorCode = %d"),  GetLastError());
		return sRegistryProperty;
	}

	pRegBuf = (PBYTE) malloc(BytesReturned);

	if(pRegBuf == NULL)
	{
		ATLTRACE(_T("\r\n *** ERROR *** malloc() failed, ErrorCode = %d"), GetLastError());
		return sRegistryProperty;
	}

	if(!SetupDiGetDeviceRegistryProperty(m_HWUSBDevInfo, &m_HWUSBDevInfoData, Property,
										  NULL, pRegBuf, BytesReturned, &BytesReturned))	
	{
		ATLTRACE(_T("\r\n *** ERROR *** SetupDiGetDeviceRegistryProperty() failed, ErrorCode = %d"),  GetLastError());
	}
	
	sRegistryProperty += (LPTSTR)pRegBuf;

	free(pRegBuf);

	return sRegistryProperty;
}


void CRegScrubDlg::Localize()
{
	CString resStr;

	resStr.LoadString(IDS_REGSCRUB_CLEAN_TEXT);
	SetDlgItemText(IDC_SCRUB, resStr );

	resStr.LoadString(IDS_REGSCRUB_REMOVE_STATIC_TEXT);
	SetDlgItemText(IDC_REMOVE_STATIC_IDS, resStr );

	resStr.LoadString(IDS_REGSCRUB_PROFILE_DESC_TEXT);
	SetDlgItemText(IDC_PLAYER_PROFILE_TEXT, resStr );

	resStr.LoadString(IDS_REGSCRUB_SCSI_VID_TEXT);
	SetDlgItemText(IDC_USB_VID_DESC_TEXT, resStr );

	resStr.LoadString(IDS_REGSCRUB_USB_PID_TEXT);
	SetDlgItemText(IDC_USB_PID_DESC_TEXT, resStr );

	resStr.LoadString(IDS_REGSCRUB_SCSI_VID_TEXT);
	SetDlgItemText(IDC_SCSI_MFG_DESC_TEXT, resStr );

	resStr.LoadString(IDS_REGSCRUB_SCSI_PID_TEXT);
	SetDlgItemText(IDC_SCSI_PRODUCT_DESC_TEXT, resStr );

	resStr.LoadString(IDS_REGSCRUB_SCANNING_STATUS_TEXT);
	SetDlgItemText(IDC_SCANNING_TEXT, resStr );

	resStr.LoadString(IDS_REGSCRUB_DELETING_STATUS_TEXT);
	SetDlgItemText(IDC_DELETING_TEXT, resStr );

	resStr.LoadString(IDS_REGSCRUB_ENTRIES_REM_STATUS_TEXT);
	SetDlgItemText(IDC_REMOVED_TEXT, resStr );

	resStr.LoadString(IDS_REGSCRUB_REMOVE_RECOVERY);
	SetDlgItemText(IDC_REMOVE_RECOVERY_ENTRIES, resStr );
}

