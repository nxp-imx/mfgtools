/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// OpRegistry.cpp : implementation file
//

#include "StdAfx.h"
#include <devguid.h>
#include <cfgmgr32.h>

#include "OpRegistry.h"
#include "PortMgrDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(COpRegistry, COperation)

COpRegistry::COpRegistry(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo *pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo, REGISTRY_OP)
{
	ASSERT(m_pOpInfo);
//	m_p_drv_keys = pPortMgrDlg->GetDriverKeys();
	m_iDuration = 3;
}

COpRegistry::~COpRegistry(void)
{
}
BOOL COpRegistry::InitInstance()
{
	// TODO:  perform and per-thread initialization here
	return TRUE;
}

BEGIN_MESSAGE_MAP(COpRegistry, COperation)
	ON_THREAD_MESSAGE(WM_MSG_OPEVENT, OnMsgStateChange)
END_MESSAGE_MAP()

void COpRegistry::OnMsgStateChange(WPARAM nEventType, LPARAM dwData)
{
	CString taskMsg;

	DWORD ret = OPSTATUS_SUCCESS;

	ATLTRACE(_T("%s Registry Event: %s Msg: %s DevState: %s \r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()));

	switch(nEventType)
	{
        case OPEVENT_KILL:
            AfxEndThread(0);

        case OPEVENT_START:

			m_bStart = true;
			m_iPercentComplete = 0;
			taskMsg.LoadString(IDS_OPREGISTRY_SCRUBBING);
			m_pPortMgrDlg->UpdateUI(taskMsg, m_iPercentComplete);

/*clw		if ( m_p_drv_keys->GetCount() > 0 ) {
				if ( Scrub() == TRUE ) {
					m_iPercentComplete = m_iDuration;
					m_sStatus.LoadString(IDS_OPREGISTRY_SCRUB_COMPLETE);
					ret = OPSTATUS_SUCCESS;
				}
				else {
					m_iPercentComplete = m_iDuration;
					m_sStatus.LoadString(IDS_OPREGISTRY_SCRUB_ERROR);
					ret = OPSTATUS_ERROR;
				}
			}
			else {
				m_iPercentComplete = m_iDuration;
				m_sStatus.LoadString(IDS_OPREGISTRY_SCRUB_NOTHING_TO_DO);
				ATLTRACE(_T("Registry Op - nothing to do.\n"));
				ret = OPSTATUS_SUCCESS;
			}
*///clw
			m_pPortMgrDlg->UpdateUI(taskMsg, m_iPercentComplete);
			m_pPortMgrDlg->PostMessage(WM_MSG_PD_EVENT ,CPortMgrDlg::PD_EVNT_OP_COMPLETE, ret);
			break;

		case OPEVENT_STOP:
			m_bStart = false;
			taskMsg.Empty();
			m_pPortMgrDlg->UpdateUI(taskMsg, m_iPercentComplete);
			m_pPortMgrDlg->PostMessage(WM_MSG_PD_EVENT ,CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);
			break;

		case OPEVENT_DEVICE_ARRIVAL: 
		case OPEVENT_DEVICE_REMOVAL:
		case OPEVENT_VOLUME_ARRIVAL:
		case OPEVENT_VOLUME_REMOVAL:
			break;
		default:
			// should never get here
			ASSERT (0);
	}
	return;
}

DWORD_PTR COpRegistry::Scrub()
{
	HDEVINFO HWDiskDrvDevInfo,HWVolumeDevInfo;
	SP_DEVINFO_DATA HWDiskDrvDevInfoData,HWVolumeDevInfoData;
	LPGUID pDevInterfaceGuid = (LPGUID)&GUID_DEVINTERFACE_USB_DEVICE;
	DWORD HWUSBIndex/*,regDataType*/;
	CString HWUSBDriverName, HWDevInstStr, HWVolumeDevInstStr, HWDiskDrvDevInstStr;
	BOOL RetVal=0, found=0;
	
//	if(m_pUSBPort->GetType()==PORT)
//		pDevInterfaceGuid=(LPGUID)&GUID_DEVINTERFACE_USB_DEVICE;
//	else
//		return RetVal;

	// Open a handle to the plug and play dev node.  SetupDiGetClassDevs() returns a device information 
	// set that contains info on all installed devices of a specified class.
	m_HWUSBDevInfo=SetupDiGetClassDevs(pDevInterfaceGuid,NULL,NULL,(/*DIGCF_PRESENT|*/DIGCF_INTERFACEDEVICE));
	HWDiskDrvDevInfo=SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_DISKDRIVE,NULL,NULL,/*DIGCF_PRESENT*/0);
	HWVolumeDevInfo=SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_VOLUME,NULL,NULL,/*DIGCF_PRESENT*/0);

    if (m_HWUSBDevInfo==INVALID_HANDLE_VALUE||HWDiskDrvDevInfo==INVALID_HANDLE_VALUE||HWVolumeDevInfo==INVALID_HANDLE_VALUE)
	{
		RetVal=GetLastError();
		ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiGetClassDevs() handle value returned, ErrorCode = %d"), RetVal);
		return RetVal; 
	}

	m_HWUSBDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);
	HWDiskDrvDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);
	HWVolumeDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);

//clw    ATLTRACE(_T("	Scrub()looking to remove %s.\n"), m_p_drv_keys->GetTail());
	for(HWUSBIndex=0;RetVal=SetupDiEnumDeviceInfo(m_HWUSBDevInfo,HWUSBIndex,&m_HWUSBDevInfoData);HWUSBIndex++)
    {   // get the driver name
	    HWUSBDriverName=GetDeviceRegistryProperty(SPDRP_DRIVER);
		if(!HWUSBDriverName.IsEmpty())
	    {
/*clw			if (m_p_drv_keys->GetTail().CompareNoCase(HWUSBDriverName)==0)
	        {
				cr = CM_Get_Device_ID(m_HWUSBDevInfoData.DevInst, buf, sizeof(buf), 0); 
				HWDevInstStr = buf;
				ATLTRACE(_T("	Scrub() found %s.\n"), HWDevInstStr);
				found = 1;
//				Error = SetupDiEnumDeviceInterfaces
//				Error = SetupDiGetDeviceInterfaceAlias(m_HWUSBDevInfo, &m_HWUSBDevInfoData, &GUID_DEVCLASS_DISKDRIVE, 
				// get USBSTOR devnode from USB devnode
				Error=CM_Get_Child(&HWDiskDrvDevInst,m_HWUSBDevInfoData.DevInst,0);
				if (Error != CR_SUCCESS)
					ATLTRACE(_T("*** ERROR *** get USBSTOR devnode from USB devnode failed: %#x.\n"), Error);
				else {
					cr = CM_Get_Device_ID(HWDiskDrvDevInst, buf, sizeof(buf), 0); 
					HWDiskDrvDevInstStr = buf;
					ATLTRACE(_T("	Scrub() found %s.\n"), HWDiskDrvDevInstStr);
				}

				// get STORAGE devnode from USBSTOR devnode
				Error=CM_Get_Child(&HWVolumeDevInst,HWDiskDrvDevInst,0);
				if (Error != CR_SUCCESS)
					ATLTRACE(_T("*** ERROR *** get STORAGE devnode from USBSTOR devnode failed: %#x.\n"), Error);
				else {
					cr = CM_Get_Device_ID(HWVolumeDevInst, buf, sizeof(buf), 0);
					HWVolumeDevInstStr = buf;
					ATLTRACE(_T("	Scrub() found %s.\n"), HWVolumeDevInstStr);
				}

				for(HWDiskDrvIndex=0; RetVal=SetupDiEnumDeviceInfo(HWDiskDrvDevInfo,HWDiskDrvIndex,&HWDiskDrvDevInfoData);HWDiskDrvIndex++)
					{
						if(HWDiskDrvDevInst==HWDiskDrvDevInfoData.DevInst)
						{ //remove enum\USBSTOR device and its direct control class link
							Error=SetupDiCallClassInstaller(DIF_REMOVE,HWDiskDrvDevInfo,&HWDiskDrvDevInfoData);
							if (Error != TRUE)
								ATLTRACE(_T("*** ERROR *** remove %s failed: %d.\n"), HWDiskDrvDevInstStr, GetLastError());
							else
								ATLTRACE(_T("	Scrub() removed: %s.\n"), HWDiskDrvDevInstStr);
//							++found;
							break;
						}
					}

				for(HWVolumeIndex=0; RetVal=SetupDiEnumDeviceInfo(HWVolumeDevInfo,HWVolumeIndex,&HWVolumeDevInfoData);HWVolumeIndex++)
					{
						if(HWVolumeDevInst==HWVolumeDevInfoData.DevInst)
						{ //remove enum\Storage\RemovableMedia devnode and its direct control class link
							Error=SetupDiCallClassInstaller(DIF_REMOVE,HWVolumeDevInfo,&HWVolumeDevInfoData);
							if (Error != TRUE)
								ATLTRACE(_T("*** ERROR *** remove %s failed: %d.\n"), HWVolumeDevInstStr, GetLastError());
							else
								ATLTRACE(_T("	Scrub() removed: %s.\n"), HWVolumeDevInstStr);
//							++found;
							break;
						}
					}

//				if(m_pUSBPort->GetDriveCount()>1) // must be mmc - lets do the 2nd drive
//				{
				if(CM_Get_Sibling(&HWDiskDrvDevInst2, HWDiskDrvDevInst,0) == CR_SUCCESS ) // must be mmc - lets do the 2nd drive
				{
					cr = CM_Get_Device_ID(HWDiskDrvDevInst2, buf, sizeof(buf), 0); 
					HWDiskDrvDevInstStr = buf;
					ATLTRACE(_T("	Scrub() found 2nd drive: %s.\n"), HWDiskDrvDevInstStr);
					
					Error=CM_Get_Child(&HWVolumeDevInst2,HWDiskDrvDevInst2,0);
					if (Error != CR_SUCCESS)
						ATLTRACE(_T("*** ERROR *** 2nd drive - get STORAGE devnode from USBSTOR devnode failed: %#x.\n"), Error);
					else {
						cr = CM_Get_Device_ID(HWVolumeDevInst2, buf, sizeof(buf), 0);
						HWVolumeDevInstStr = buf;
						ATLTRACE(_T("	Scrub() found 2nd drive: %s.\n"), HWVolumeDevInstStr);
					}
					for(HWDiskDrvIndex=0; RetVal=SetupDiEnumDeviceInfo(HWDiskDrvDevInfo,HWDiskDrvIndex,&HWDiskDrvDevInfoData);HWDiskDrvIndex++)
						{
							if(HWDiskDrvDevInst2==HWDiskDrvDevInfoData.DevInst)
							{ //remove enum\USBSTOR device and its direct control class link
								Error=SetupDiCallClassInstaller(DIF_REMOVE,HWDiskDrvDevInfo,&HWDiskDrvDevInfoData);
								if (Error != TRUE)
									ATLTRACE(_T("*** ERROR *** 2nd drive - remove %s failed: %d.\n"), HWDiskDrvDevInstStr, GetLastError());
								else
									ATLTRACE(_T("	Scrub() removed 2nd drive: %s.\n"), HWDiskDrvDevInstStr);
//								++found;
								break;
							}
						}

					for(HWVolumeIndex=0; RetVal=SetupDiEnumDeviceInfo(HWVolumeDevInfo,HWVolumeIndex,&HWVolumeDevInfoData);HWVolumeIndex++)
						{
							if(HWVolumeDevInst2==HWVolumeDevInfoData.DevInst)
							{ //remove enum\Storage\RemovableMedia devnode and its direct control class link
								Error=SetupDiCallClassInstaller(DIF_REMOVE,HWVolumeDevInfo,&HWVolumeDevInfoData);
								if (Error != TRUE)
									ATLTRACE(_T("*** ERROR *** 2nd drive - remove %s failed: %d.\n"), HWVolumeDevInstStr, GetLastError());
								else
									ATLTRACE(_T("	Scrub() removed 2nd drive: %s.\n"), HWVolumeDevInstStr);
//								++found;
								break;
							}
						}
				}
				else
					ATLTRACE(_T("	Scrub() no second drive.\n"));

				//remove enum\USB devnode and its direct control class link
				Error=SetupDiCallClassInstaller(DIF_REMOVE,m_HWUSBDevInfo,&m_HWUSBDevInfoData);
				if (Error != TRUE)
					ATLTRACE(_T("*** ERROR *** remove %s failed: %d.\n"), HWDevInstStr, GetLastError());
				else
					ATLTRACE(_T("	Scrub() removed: %s.\n"), HWDevInstStr);
				RetVal=TRUE;
//				++found;
				break;
			}
		//	free (HWUSBDriverName);
			//HWUSBDriverName = NULL;
*///clw
		}
		else
			ATLTRACE(_T("*** ERROR *** GetDeviceRegistryProperty(SPDRP_DRIVER) failed to get driver name.\n"));
    }  // end for()
//clw	if (!found)
//clw		ATLTRACE(_T("*** ERROR *** Scrub() did not find driver %s.\n"), m_p_drv_keys->GetTail());

	SetupDiDestroyDeviceInfoList(m_HWUSBDevInfo);
	SetupDiDestroyDeviceInfoList(HWDiskDrvDevInfo);
	SetupDiDestroyDeviceInfoList(HWVolumeDevInfo);

//clw	m_p_drv_keys->RemoveTail();

	return RetVal;
}

CString COpRegistry::GetDeviceRegistryProperty(DWORD Property)
{
	DWORD BytesReturned;
	PBYTE pRegBuf = NULL;
	CString sRegistryProperty(_T(""));

	// get the required	length of the buffer first
	if(SetupDiGetDeviceRegistryProperty(m_HWUSBDevInfo, &m_HWUSBDevInfoData,
										Property, NULL, NULL, 0, &BytesReturned))
	{
		//m_sTempStr.Format(_T(" *** ERROR *** SetupDiGetDeviceRegistryProperty() failed, ErrorCode = %d\r\n"), GetLastError());
		//m_sPortDataStr += m_sTempStr;
		return sRegistryProperty;
	}

	pRegBuf = (PBYTE) malloc(BytesReturned);

	if(pRegBuf == NULL)
	{
		//m_sTempStr.Format(_T(" *** ERROR *** malloc() failed, ErrorCode = %d\r\n"), GetLastError());
		//m_sPortDataStr += m_sTempStr;
		return sRegistryProperty;
	}

	if(!SetupDiGetDeviceRegistryProperty(m_HWUSBDevInfo, &m_HWUSBDevInfoData, Property,
										  NULL, pRegBuf, BytesReturned, &BytesReturned))
		
	{
		//m_sTempStr.Format(_T(" *** ERROR *** SetupDiGetDeviceRegistryProperty() failed, ErrorCode = %d\r\n"), GetLastError());
		//m_sPortDataStr += m_sTempStr;
	}
	
	sRegistryProperty += (LPTSTR)pRegBuf;

	free(pRegBuf);

	return sRegistryProperty;
}

