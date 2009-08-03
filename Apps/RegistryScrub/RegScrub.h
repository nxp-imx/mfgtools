// RegScrub.h : header file
// Copyright (c) 2005 SigmaTel, Inc.

#pragma once

#include <Setupapi.h>

#define WM_MSG_UPDATE_UI WM_USER+100



// CRegScrub
class CRegScrub
{
// Construction
public:
	CRegScrub(CWnd* pParent = NULL);	// standard constructor
	virtual ~CRegScrub();

    DWORD Clean(void);

	UINT GetDeviceCount();
	UINT GetEntryRemovalCount(){return m_EntryRemovalCount;};
	CString GetDeletingKeyStr(){return m_DeletingKeyStr;};
	CString GetProcessingKeyStr(){return m_ProcessingKeyStr;};
    PREMOVEDEVICEITEM InsertItem(CString orgmfg, CString mfg, CString orgproduct, CString scsiproduct, CString usbvendor, CString usbproduct);
	void RemoveItem(PREMOVEDEVICEITEM pItem);


// Implementation
protected:
	CWnd*				m_pCallingWnd;

	CRegScrubList*		m_pScrubList;

	HDEVINFO			m_HWUSBDevInfo;
	SP_DEVINFO_DATA		m_HWUSBDevInfoData;

	HDEVINFO			m_HWHIDDevInfo;

	UINT				m_EntryRemovalCount;
	CString				m_DeletingKeyStr;
	CString				m_ProcessingKeyStr;

	CString				m_HWDevInstStr;
	CString				m_USBStorageStr;


    BOOL IsPlatformNT(void);
    
   	void NotifyParentWnd(void);

    DWORD RemoveUSBFlags();
    DWORD RemoveUSBRegEntries();
	DWORD RemoveUSBStorRegEntries();
	DWORD RemoveRemovableMediaRegEntries();
    DWORD RemoveHIDRegEntries();

    CString GetDeviceRegistryProperty(DWORD Property);
};
