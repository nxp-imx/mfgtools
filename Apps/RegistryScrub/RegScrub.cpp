// RegScrub.cpp : implementation file
// Copyright (c) 2005 SigmaTel, Inc.

#include "stdafx.h"
#include ".\RegScrubList.h"
#include ".\RegScrub.h"

#include <devguid.h>
#include <cfgmgr32.h>

#include <initguid.h> // in a single source file
#include <usbiodef.h> // in that source file a second time to instantiate the GUIDs

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern REMOVEDEVICEITEM	g_AddDeviceEntry;

/*****************************************************************************/
CRegScrub::CRegScrub(CWnd* pCallingWnd /*=NULL*/)
{
   	m_pCallingWnd = pCallingWnd;

	m_EntryRemovalCount = 0;
	m_DeletingKeyStr = (_T(""));
	m_ProcessingKeyStr = (_T(""));

	m_pScrubList = NULL;
//    InitStrings();
}


/*****************************************************************************/
CRegScrub::~CRegScrub()
{
	if (m_pScrubList)
		delete m_pScrubList;
}

/*****************************************************************************/
UINT CRegScrub::GetDeviceCount()
{
	UINT count = 0;

	if (m_pScrubList)
		count = m_pScrubList->GetDeviceCount();

	return count;
}

/*****************************************************************************/
PREMOVEDEVICEITEM CRegScrub::InsertItem(CString orgmfg, CString mfg, CString orgproduct, CString scsiproduct, CString usbvendor, CString usbproduct)
{
	if (m_pScrubList == NULL)
		m_pScrubList = new CRegScrubList;

	return (m_pScrubList->InsertItem( orgmfg, mfg, orgproduct, scsiproduct, usbvendor, usbproduct));
}

/*****************************************************************************/
void CRegScrub::RemoveItem(PREMOVEDEVICEITEM pItem)
{
	if (m_pScrubList)
		m_pScrubList->RemoveItem(pItem);
}


/*****************************************************************************/
DWORD CRegScrub::Clean()
{
	DWORD RetVal=ERROR_SUCCESS;

	m_EntryRemovalCount = 0;	
	m_DeletingKeyStr = (_T(""));
	m_ProcessingKeyStr = (_T(""));

    RetVal=RemoveUSBRegEntries();
	
	if(RetVal==ERROR_SUCCESS)
    {
        RemoveHIDRegEntries();
        RetVal=RemoveUSBStorRegEntries();
    }
 
  	if(RetVal==ERROR_SUCCESS)
	{
        RemoveUSBFlags();
	}

    if(RetVal==ERROR_SUCCESS)
    {
	    m_DeletingKeyStr.Format(_T("Finished"));
		m_ProcessingKeyStr.Format(_T("Finished"));
    }
    else   
    {
		m_DeletingKeyStr.Format(_T("Error"));
		m_ProcessingKeyStr.Format(_T("Error"));
	}

	return(RetVal);
}

/*****************************************************************************/
DWORD CRegScrub::RemoveUSBRegEntries()
{
	BOOL Success;
	CONFIGRET cr;
	TCHAR buf[4096];
	CString USBStr,HWUSBHardwareIDName;
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

    RetVal=ERROR_FILE_NOT_FOUND;

	m_HWUSBDevInfoData.cbSize=sizeof(SP_DEVINFO_DATA);

	for(HWUSBIndex=0;RetVal=SetupDiEnumDeviceInfo(m_HWUSBDevInfo,HWUSBIndex,&m_HWUSBDevInfoData);HWUSBIndex++)
    {   // get the hardware id name
	    HWUSBHardwareIDName=GetDeviceRegistryProperty(SPDRP_HARDWAREID);	        
		HWUSBHardwareIDName.MakeUpper();
		m_ProcessingKeyStr = HWUSBHardwareIDName;
		m_DeletingKeyStr = _T("");
		NotifyParentWnd();
        
		PREMOVEDEVICEITEM pRemoveDeviceList = m_pScrubList->GetHead();

        for(int ProductIDIndex=0; pRemoveDeviceList; ProductIDIndex++)
        {
            USBStr.Format(_T("USB\\Vid_%s&Pid_%s&Rev_"), pRemoveDeviceList->USBVid, pRemoveDeviceList->USBPid);
	        USBStr.MakeUpper();

		    if(HWUSBHardwareIDName.Find(USBStr)!=-1)
		    {//remove enum\USB device and its direct control class link
			    cr=CM_Get_Device_ID(m_HWUSBDevInfoData.DevInst,buf,sizeof(buf),0); 
			    ATLTRACE(_T("	Scrub() found %s.\n"),buf);
			    Success=SetupDiCallClassInstaller(DIF_REMOVE,m_HWUSBDevInfo,&m_HWUSBDevInfoData);
			    if(!Success)
			    {
    				RetVal=GetLastError();
				    ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiCallClassInstaller(DIF_REMOVE) value returned, ErrorCode = %d"), RetVal);
				    break;
                    break;
			    }
			    else
			    {
    				m_EntryRemovalCount++;	
	    			m_DeletingKeyStr=buf;
		    		NotifyParentWnd();
			    	RetVal=ERROR_SUCCESS;
    	    	    ATLTRACE(_T("	Scrub() removed: %s.\n"),buf);
                    break;
    			}
            }
			pRemoveDeviceList = m_pScrubList->GetNext(pRemoveDeviceList);
		}
	}  // end for()

	SetupDiDestroyDeviceInfoList(m_HWUSBDevInfo);

	return RetVal;
}

/*****************************************************************************/
DWORD CRegScrub::RemoveUSBStorRegEntries()
{
	HKEY hKey;
	BOOL Success;
	CONFIGRET cr;
	ULONG DataSize=256;
	LONG QueryStatus;
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

	for(HWDiskDrvIndex=0; RetVal=SetupDiEnumDeviceInfo(HWDiskDrvDevInfo,HWDiskDrvIndex,&HWDiskDrvDevInfoData);HWDiskDrvIndex++)
	{
		cr=CM_Get_Device_ID(HWDiskDrvDevInfoData.DevInst,buf,sizeof(buf),0); 
		HWDiskDrvDevStr=buf; 
		HWDiskDrvDevStr.MakeUpper();
		m_ProcessingKeyStr = HWDiskDrvDevStr;
		m_DeletingKeyStr = _T("");
		NotifyParentWnd();

		PREMOVEDEVICEITEM pRemoveDeviceList = m_pScrubList->GetHead();

        for(int ProductIDIndex = 0; pRemoveDeviceList; ProductIDIndex++)
        {
			USBStorStr.Format(_T("USBSTOR\\Disk&Ven_%s&Prod_%s&Rev_"), pRemoveDeviceList->Mfg,  pRemoveDeviceList->Product);
	        USBStorStr.MakeUpper();	

		    if(HWDiskDrvDevStr.Find(USBStorStr)!=-1)
		    { //remove enum\USBSTOR device and its direct control class link	
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
                    m_ProcessingKeyStr=HWDiskDrvDevStr;
    				m_EntryRemovalCount++;	
	    			m_DeletingKeyStr=HWDiskDrvDevStr;
		    		NotifyParentWnd();
			    	RetVal=ERROR_SUCCESS;
    				ATLTRACE(_T("	Scrub() removed: %s.\n"),HWDiskDrvDevStr);
	    		}
                break;
		    }
			pRemoveDeviceList = m_pScrubList->GetNext(pRemoveDeviceList);

        }
	}		
	
	SetupDiDestroyDeviceInfoList(HWDiskDrvDevInfo);
	
	return RetVal;
}

/*****************************************************************************/
DWORD CRegScrub::RemoveRemovableMediaRegEntries()
{
	BOOL Success;
	CONFIGRET cr;
	TCHAR buf[4096];
	CString HWVolumeDevInstStr;
	HDEVINFO HWVolumeDevInfo;
	SP_DEVINFO_DATA HWVolumeDevInfoData;
	DWORD HWVolumeIndex,RetVal=ERROR_SUCCESS;

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
		m_DeletingKeyStr = _T("");
		NotifyParentWnd();
		if(HWVolumeDevInstStr.Find(m_USBStorageStr)!=-1)
		{	//remove enum\Storage\RemovableMedia devnode and its direct control class link
			Success=SetupDiCallClassInstaller(DIF_REMOVE,HWVolumeDevInfo,&HWVolumeDevInfoData);
			if(!Success)
			{
				RetVal=GetLastError();
				ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiCallClassInstaller(DIF_REMOVE) value returned, ErrorCode = %d"), RetVal);
                break;
			}
			else
			{
				m_EntryRemovalCount++;	
				m_DeletingKeyStr=HWVolumeDevInstStr;
				NotifyParentWnd();
				RetVal=ERROR_SUCCESS;
				ATLTRACE(_T("	Scrub() removed: %s.\n"),HWVolumeDevInstStr);
			}
			break;
		}
	}

	SetupDiDestroyDeviceInfoList(HWVolumeDevInfo);

	return RetVal;
}


/*****************************************************************************/
DWORD CRegScrub::RemoveHIDRegEntries()
{
	BOOL Success;
	CONFIGRET cr;
	TCHAR buf[4096];
	CString HIDStr,HWHIDHardwareIDName;
	DWORD HWHIDIndex,RetVal=ERROR_SUCCESS;
    SP_DEVINFO_DATA HWHIDDevInfoData;
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
		NotifyParentWnd();
        
		PREMOVEDEVICEITEM pRemoveDeviceList = m_pScrubList->GetHead();

        for(int ProductIDIndex=0; pRemoveDeviceList; ProductIDIndex++)
        {
            HIDStr.Format(_T("HID\\Vid_%s&Pid_%s"), pRemoveDeviceList->USBVid, pRemoveDeviceList->USBPid);
	        HIDStr.MakeUpper();

		    if(HWHIDHardwareIDName.Find(HIDStr)!=-1)
		    {//remove enum\USB device and its direct control class link
			    cr=CM_Get_Device_ID(HWHIDDevInfoData.DevInst,buf,sizeof(buf),0); 
			    ATLTRACE(_T("	Scrub() found %s.\n"),buf);
			    Success=SetupDiCallClassInstaller(DIF_REMOVE,m_HWHIDDevInfo,&HWHIDDevInfoData);
			    if(!Success)
			    {
    				RetVal=GetLastError();
				    ATLTRACE(_T("\r\n *** ERROR *** Invalid SetupDiCallClassInstaller(DIF_REMOVE) value returned, ErrorCode = %d"), RetVal);
				    break;
                    break;
			    }
			    else
			    {
    				m_EntryRemovalCount++;	
	    			m_DeletingKeyStr=buf;
		    		NotifyParentWnd();
			    	RetVal=ERROR_SUCCESS;
    	    	    ATLTRACE(_T("	Scrub() removed: %s.\n"),buf);
                    break;
    			}
            }
			pRemoveDeviceList = m_pScrubList->GetNext(pRemoveDeviceList);
		}
	}  // end for()

	SetupDiDestroyDeviceInfoList(m_HWHIDDevInfo);

	return RetVal;
}

/*****************************************************************************/
CString CRegScrub::GetDeviceRegistryProperty(DWORD Property)
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

/*****************************************************************************/
DWORD CRegScrub::RemoveUSBFlags()
{
    HKEY hUSBKey;
   	DWORD RetVal=ERROR_SUCCESS;
    CString USBStr;
    CString BaseKeyStr="SYSTEM\\CurrentControlSet\\Control\\UsbFlags";

	ULONG DataSize;
	TCHAR EnumKey[256];
    CString EnumKeyStr; 

	//TCHAR devStr[128];
	int UsbFlagsIndex = 0;
	LONG RetStatus;
	FILETIME LastTime;


//    m_USBVendorIDStr="066F";

    //m_SCSIRecoveryIDStrs
    
    //m_USBProductIDStrs
  
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, BaseKeyStr, 0, KEY_ALL_ACCESS, &hUSBKey)==ERROR_SUCCESS)
    {
        do
        {
            DataSize = 256;
            RetStatus = RegEnumKeyEx(hUSBKey, UsbFlagsIndex, EnumKey, &DataSize, NULL, NULL, NULL,&LastTime);
            UsbFlagsIndex++;
            if (RetStatus == ERROR_SUCCESS)
            {
                m_ProcessingKeyStr=BaseKeyStr+"\\"+EnumKey;
                m_ProcessingKeyStr.MakeUpper();
                m_DeletingKeyStr = _T("");
                NotifyParentWnd();
                EnumKeyStr=EnumKey;
                if(EnumKeyStr.GetLength()>=8)
                    EnumKeyStr.SetAt(8,NULL);

                EnumKeyStr.MakeUpper();

				PREMOVEDEVICEITEM pRemoveDeviceList = m_pScrubList->GetHead();

                for(int ProductIDIndex=0; pRemoveDeviceList; ProductIDIndex++)
                {
                    USBStr.Format(_T("%s%s"), pRemoveDeviceList->USBVid, pRemoveDeviceList->USBPid);
                    if(EnumKeyStr==USBStr)
                    {
                        RetStatus=RegDeleteKey(HKEY_LOCAL_MACHINE, BaseKeyStr+"\\"+EnumKey);
                        m_EntryRemovalCount++;	
                        m_DeletingKeyStr=BaseKeyStr+"\\"+EnumKey;
                        m_DeletingKeyStr.MakeUpper();
                        NotifyParentWnd();
                        --UsbFlagsIndex; // adjust index
                        break;
                    }
					pRemoveDeviceList = m_pScrubList->GetNext(pRemoveDeviceList);
                }
            }
        } while  (RetStatus == ERROR_SUCCESS);
        RegCloseKey (hUSBKey);
    }                   
         
  
    return RetVal;
}

/*****************************************************************************/



/*****************************************************************************/
BOOL CRegScrub::IsPlatformNT(void)
{ 
    OSVERSIONINFOA osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA((OSVERSIONINFOA*)&osvi);
    return (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId);    
}

/*****************************************************************************/
void CRegScrub::NotifyParentWnd(void)
{
	if(m_pCallingWnd)
		m_pCallingWnd->SendMessage(WM_MSG_UPDATE_UI);
}