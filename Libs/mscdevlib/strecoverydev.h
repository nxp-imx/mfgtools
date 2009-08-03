// StRecoveryDev.h: interface for the CStRecoveryDev class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRECOVERYDEV_H__E57DC981_5DB4_4194_8912_CB96EB83F056__INCLUDED_)
#define AFX_STRECOVERYDEV_H__E57DC981_5DB4_4194_8912_CB96EB83F056__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdevice.h"

#include <basetyps.h>
#include <setupapi.h>
//#include <initguid.h>
#include "../DevSupport/RecoveryDeviceGuid.h"

#pragma warning( push )
#pragma warning( disable : 4201 )

#include <usb200.h>
#pragma warning( pop )

class CStFwComponent;
class CStRecoveryDev : public CStDevice  
{
public:
	CStRecoveryDev(CStUpdater* pUpdater, string name= "CStRecoveryDev");
	virtual ~CStRecoveryDev();

	ST_ERROR Initialize();
	ST_ERROR DownloadUsbMsc();

	static VOID CALLBACK WriteCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,  
		LPOVERLAPPED lpOverlapped);       

	static HANDLE	m_sync_event;	

private:
	ST_ERROR Write(UCHAR* buf, ULONG size);
	ST_ERROR Trash();
	ST_ERROR Close();
	ST_ERROR Open(const wchar_t* _p_device_name);
	HANDLE			m_recovery_drive_handle;
	CStFwComponent*	m_p_fw_component;
	ST_OVERLAPPED	m_overlapped;
	wstring			m_device_name;

	HANDLE OpenOneDevice (IN HDEVINFO HardwareDeviceInfo,
		IN PSP_INTERFACE_DEVICE_DATA DeviceInfoData,
		IN wchar_t *devName, rsize_t bufsize);
	HANDLE OpenUsbDevice( LPGUID  pGuid, wchar_t *outNameBuf, rsize_t bufsize);
	BOOL GetUsbDeviceFileName( LPGUID  pGuid, wchar_t *outNameBuf, rsize_t bufsize);
};

#endif // !defined(AFX_STRECOVERYDEV_H__E57DC981_5DB4_4194_8912_CB96EB83F056__INCLUDED_)
