// OpRegistry.h : header file
//

#pragma once

#include "Operation.h"
#include "PortMgrDlg.h"

class COpRegistry : public COperation
{
	DECLARE_DYNCREATE(COpRegistry)
public:
	COpRegistry(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo *pOpInfo = NULL);
	virtual BOOL InitInstance();
	virtual afx_msg void  OnMsgStateChange(WPARAM nEventType, LPARAM dwData);

protected:
	DECLARE_MESSAGE_MAP()
	virtual ~COpRegistry(void);
	HDEVINFO m_HWUSBDevInfo;
	SP_DEVINFO_DATA m_HWUSBDevInfoData;

	DWORD_PTR Scrub();
	CString GetDeviceRegistryProperty(DWORD Property);
//	CPortMgrDlg::CDevList *m_p_drv_keys;
};