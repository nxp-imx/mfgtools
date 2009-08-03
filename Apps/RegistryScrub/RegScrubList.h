#pragma once

#define NUM_SDK_RECV_MFG_ENTRIES	10
#define MAX_OTHER_ENTRIES			5

typedef struct _RemoveDeviceItem
{
	void*				pPrev;
	void*				pNext;
	CString				OrgMfg;
	CString				Mfg;
	CString				OrgProduct;
	CString				Product;
	CString				USBVid;
	CString				USBPid;
} REMOVEDEVICEITEM, *PREMOVEDEVICEITEM;

/*typedef struct _addDeviceItem
{
	TCHAR				Mfg[32];
	TCHAR				Product[32];
	TCHAR				USBVid[16];
	TCHAR				USBPid[16];
} ADDDEVICEITEM, *PADDDEVICEITEM;
*/

class CRegScrubList
{
public:
	CRegScrubList(void);
	~CRegScrubList(void);

	UINT				GetDeviceCount(){return m_RemoveDeviceCount;};
    PREMOVEDEVICEITEM InsertItem (CString OrgMfg, CString mfg, CString OrgProduct, CString scsiproduct, CString usbvendor, CString usbproduct );
	void RemoveItem (PREMOVEDEVICEITEM pItem);
	PREMOVEDEVICEITEM GetHead (void);
	PREMOVEDEVICEITEM GetTail (void);
	PREMOVEDEVICEITEM GetNext (PREMOVEDEVICEITEM pCurrent);
	PREMOVEDEVICEITEM GetPrev (PREMOVEDEVICEITEM pCurrent);

protected:
	UINT				m_RemoveDeviceCount;
	PREMOVEDEVICEITEM	m_pRemoveDeviceListHead;
	PREMOVEDEVICEITEM	m_pRemoveDeviceListTail;

};
