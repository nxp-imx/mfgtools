/*
 * Copyright (C) 2012-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "MfgToolLib_Export.h"

UINT ExceptionHandleThread( LPVOID pParam );

#define WM_MSG_EXCEPTION_EVENT		(WM_USER+41)

class CMyExceptionHandler //: public CWinThread
{
	//DECLARE_DYNCREATE(CMyExceptionHandler)
public:
	CMyExceptionHandler();
	virtual ~CMyExceptionHandler();

	typedef enum __except_kind
	{
		DeviceArriveBeforVolumeRemove = 0,
		DeviceArriveButEnumFailed,
		KillExceptionHandlerThread
	} Except_Kind_t;

public:
	HANDLE _hStartEvent;
	HANDLE m_hMapMsgMutex;
	DWORD Open();
	void Close();
	virtual BOOL InitInstance();
	void OnMsgExceptionEvent(WPARAM ExceptionType, LPARAM desc);
	//DECLARE_MESSAGE_MAP()
};
