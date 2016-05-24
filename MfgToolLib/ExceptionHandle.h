/*
 * Copyright 2009-2013, 2016 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
	pthread_t Exception_thread;
	myevent* _hStartEvent;
	myevent * thread_dead;
	pthread_mutex_t *m_hMapMsgMutex;

	std::queue<thread_msg> t_msgQ;
	DWORD Open();
	void Close();
	virtual BOOL InitInstance();
	void OnMsgExceptionEvent(WPARAM ExceptionType, LPARAM desc);
	//DECLARE_MESSAGE_MAP()
};
