/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "StMfgTool.h"
#include "PlayerProfile.h"
#include "RegCheckDlg.h"


// CRegCheckDlg dialog

IMPLEMENT_DYNAMIC(CRegCheckDlg, CDialog)


/////////////////////////////////////////////////////////////////////////////
CRegCheckDlg::CRegCheckDlg(CConfigMgrSheet *_p_config_mgr, CWnd* pParent /*=NULL*/)
	: CDialog(CRegCheckDlg::IDD, pParent)
	,m_pCfgMgr(_p_config_mgr)
	,m_DeviceCount(0)
	,m_OtherDeviceCount(0)
	,m_FirstPass(TRUE)
{
}

/////////////////////////////////////////////////////////////////////////////
CRegCheckDlg::~CRegCheckDlg(void)
{
}

void CRegCheckDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REGCHK_PROGRESS, m_ProgressCtrl);
	DDX_Text(pDX, IDC_REGCHK_ENTRIES, m_DeviceCount);
	DDX_Text(pDX, IDC_REGCHK_OTHER_ENTRIES, m_OtherDeviceCount);
}

BEGIN_MESSAGE_MAP(CRegCheckDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
BOOL CRegCheckDlg::OnInitDialog()
{  
	CDialog::OnInitDialog();

	Localize();

	m_prog_pos = 0;
	m_ProgressCtrl.SetRange32(0, REGCHK_PROG_RANGE);
	m_ProgressCtrl.SetPos(m_prog_pos);

	m_DelayTimer = SetTimer(ID_TESTTIMER, 500, NULL);

	return TRUE;    // return TRUE unless you set the focus to a control
}

void CRegCheckDlg::Localize()
{
	CString resStr;

	resStr.LoadString(IDS_REGCHK_INFO_TEXT);
	SetDlgItemText(IDC_REGCHK_INFO_TEXT, resStr );

	resStr.LoadString(IDS_REGCHK_ENTRIES_TEXT);
	SetDlgItemText(IDC_REGCHK_ENTRIES_TEXT, resStr );

	resStr.LoadString(IDS_REGCHK_OTHER_TEXT);
	SetDlgItemText(IDC_REGCHK_OTHER_DEVICES_TEXT, resStr );
}

void CRegCheckDlg::UpdateDlgStatus()
{
	UpdateData(FALSE);

	if ( ((m_DeviceCount + m_OtherDeviceCount) % 5) == 0)
	{
		++m_prog_pos;
		if (m_prog_pos == REGCHK_PROG_RANGE)
		{
			m_prog_pos = 0;
		}
		m_ProgressCtrl.SetPos(m_prog_pos);
	}
}

void CRegCheckDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnTimer(nIDEvent);
	KillTimer(m_DelayTimer);

	CountRecoveryRegEntries();

	int i = 0;
	CPlayerProfile *pProfile = NULL;

	while (((pProfile = m_pCfgMgr->GetListProfile(i)) != NULL) &&
			((m_DeviceCount + m_OtherDeviceCount) < REGCHK_WARNING_LIMIT) )
	{
		m_SCSIMfgStr = pProfile->GetScsiMfg();
		m_SCSIProductStr = pProfile->GetScsiProduct();
		m_SCSIProductIDStr = pProfile->GetUsbPid();
		m_SCSIVenderIDStr = pProfile->GetUsbVid();

		DWORD status = CountUSBRegEntries();
		if (status == ERROR_SUCCESS)
		{
			CountUSBStorRegEntries();
		}
		++i;
		if (i == 1)
			m_FirstPass = FALSE;
	}
	Sleep(2000);

	m_ProgressCtrl.SetPos(REGCHK_PROG_RANGE);

	if ((m_DeviceCount + m_OtherDeviceCount) > REGCHK_WARNING_LIMIT)
	{
		CString resStr;
		resStr.LoadString(IDS_REGCHK_WARNING);
		AfxMessageBox(resStr, MB_ICONWARNING | MB_OK);
	}

	CDialog::OnOK();
}




//////////////////////////////////////////////////////////////////////////
// All the following are called from the worker thread.
//////////////////////////////////////////////////////////////////////////
#define NUM_HID_PIDS 3
DWORD CRegCheckDlg::CountRecoveryRegEntries()
{
	CONFIGRET cr;
	TCHAR buf[4096];
	CString HIDStr,HWHIDHardwareIDName;
	DWORD HWHIDIndex,RetVal=ERROR_SUCCESS;
    SP_DEVINFO_DATA HWHIDDevInfoData;
	UINT HidPids[NUM_HID_PIDS] = {0x3700, 0x3770, 0x3780};

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
        
        HIDStr = _T("HID\\VID_066F");
	    if(HWHIDHardwareIDName.Find(HIDStr) == -1)
		{
			if (m_FirstPass)
			{
				++m_OtherDeviceCount;
				UpdateDlgStatus();
			}
			continue;
		}

		for (int i = 0; i < NUM_HID_PIDS; ++i)
		{
	        HIDStr.Format(_T("HID\\VID_066F&PID_%X"), HidPids[i]);

		    if(HWHIDHardwareIDName.Find(HIDStr)!=-1)
		    {

				++m_DeviceCount;
				UpdateDlgStatus();

				// 37ff updater
				m_SCSIMfgStr = _T("_GENERIC");
				m_SCSIProductStr = _T("MSC_Recovery");
				m_SCSIProductIDStr.Format(_T("%04X"), HidPids[i]);
				m_SCSIVenderIDStr =  _T("066F");

				DWORD status = CountUSBRegEntries();
				if (status == ERROR_SUCCESS)
				{
					CountUSBStorRegEntries();
				}
				break;
			}
		}
	}

	SetupDiDestroyDeviceInfoList(m_HWHIDDevInfo);

	return RetVal;
}

/////////////////////////////////////////////////////////////////////////////
DWORD CRegCheckDlg::CountUSBRegEntries()
{
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

	USBStr.Format(_T("USB\\Vid_%s&Pid_%s&Rev_"),m_SCSIVenderIDStr, m_SCSIProductIDStr);
	USBStr.MakeUpper();

	RetVal=ERROR_FILE_NOT_FOUND;
	for(HWUSBIndex=0;RetVal=SetupDiEnumDeviceInfo(m_HWUSBDevInfo,HWUSBIndex,&m_HWUSBDevInfoData);HWUSBIndex++)
    {   // get the hardware id name
	    HWUSBHardwareIDName=GetDeviceRegistryProperty(SPDRP_HARDWAREID);	
		HWUSBHardwareIDName.MakeUpper();

		if(HWUSBHardwareIDName.Find(USBStr)!=-1)
		{//count enum\USB device and its direct control class link
			++m_DeviceCount;
			UpdateDlgStatus();
			RetVal=ERROR_SUCCESS;
		}
		else
		{
			if (m_FirstPass)
			{
				++m_OtherDeviceCount;
				UpdateDlgStatus();
			}
			continue;
		}

		if ((m_DeviceCount + m_OtherDeviceCount) < REGCHK_WARNING_LIMIT)
		{
			RetVal = !ERROR_SUCCESS;
			break;
		}

	}  // end for()

	SetupDiDestroyDeviceInfoList(m_HWUSBDevInfo);

	return RetVal;
}

/////////////////////////////////////////////////////////////////////////////
DWORD CRegCheckDlg::CountUSBStorRegEntries()
{
	HKEY hKey;
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

		if ((m_DeviceCount + m_OtherDeviceCount) < REGCHK_WARNING_LIMIT)
		{
			RetVal = !ERROR_SUCCESS;
			break;
		}

		if(HWDiskDrvDevStr.Find(USBStorStr)!=-1)
		{ //count enum\USBSTOR device and its direct control class link	

			DataSize=256;
			ParentIdPrefixValue[0]=NULL;
			USBStorStr2.Format(_T("SYSTEM\\CurrentControlSet\\Enum\\%s"),HWDiskDrvDevStr);
			RegOpenKeyEx(HKEY_LOCAL_MACHINE, USBStorStr2, 0, KEY_READ, &hKey);
			QueryStatus = RegQueryValueEx(hKey,_T("ParentIdPrefix"),NULL,&TypeValue,
										 (PUCHAR)ParentIdPrefixValue,&DataSize);
			m_USBStorageStr.Format(_T("STORAGE\\RemovableMedia\\%s&RM"),ParentIdPrefixValue);
			m_USBStorageStr.MakeUpper();

			CountRemovableMediaRegEntries();

			// get ParentIdPrefix, search & delete under Enum\Storage\RemovableMedia then do DIF_REMOVE
			m_DeviceCount += 2;
			UpdateDlgStatus();
			RetVal=ERROR_SUCCESS;
		}
		else
		{
			if (m_FirstPass)
			{
				m_OtherDeviceCount += 2;
				UpdateDlgStatus();
			}
			continue;
		}

	}		
	
	SetupDiDestroyDeviceInfoList(HWDiskDrvDevInfo);
	
	return RetVal;
}

/////////////////////////////////////////////////////////////////////////////
DWORD CRegCheckDlg::CountRemovableMediaRegEntries()
{
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

		if(HWVolumeDevInstStr.Find(m_USBStorageStr)!=-1)
		{	//count enum\Storage\RemovableMedia devnode and its direct control class link
			m_DeviceCount += 2;
			UpdateDlgStatus();
			RetVal=ERROR_SUCCESS;
			break;
		}
		else
		{
			if (m_FirstPass)
			{
				m_OtherDeviceCount += 2;
				UpdateDlgStatus();
			}
			continue;
		}

	}

	SetupDiDestroyDeviceInfoList(HWVolumeDevInfo);

	return RetVal;
}

/////////////////////////////////////////////////////////////////////////////
CString CRegCheckDlg::GetDeviceRegistryProperty(DWORD Property)
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
