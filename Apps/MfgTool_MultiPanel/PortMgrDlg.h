/*
 * Copyright 2012-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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

#pragma once

#include "afxwin.h"
#include "afxcmn.h"

#include "ExProgressCtrl.h"
#include "../MfgToolLib/MfgToolLib_Export.h"

//Progress color
#define PROGRESS_RGB_BLUE	RGB(9,106,204)
#define PROGRESS_RGB_GREEN	RGB(78,217,11)
#define PROGRESS_RGB_RED	RGB(255,0,0)

#define PHASE_COMPLETE_POSITION		0x7FFFFFFF


// CPortMgrDlg dialog

class CPortMgrDlg : public CDialog
{
	DECLARE_DYNAMIC(CPortMgrDlg)

public:
	CPortMgrDlg(CWnd* pParent = NULL, int index = 0);   // standard constructor
	virtual ~CPortMgrDlg();

// Dialog Data
	enum { IDD = IDD_PORT_DLG };

private:
	int m_Index;
	BOOL m_bDeviceConnected;

public:
	int GetIndex() const
	{
		return m_Index;
	}

	BOOL IsDeviceConnected() const
	{
		return m_bDeviceConnected;
	}

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnPortDlgMsgFun(WPARAM wEvent, LPARAM lParam);
	afx_msg LRESULT OnDeviceChangeNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateUIInfo(WPARAM wParam, LPARAM lParam);
	CEdit m_port_status_ctrl;
	CExProgressCtrl m_port_progress_ctrl;
	CExProgressCtrl m_port_task_progress_ctrl;
	int m_PreviousCommandsIndex;
protected:
	virtual void PostNcDestroy();
public:
	CExProgressCtrl m_port_phase_progress_ctrl;
	afx_msg void OnPaint();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

extern void gDeviceChangeNotify(DEVICE_CHANGE_NOTIFY *pnsinfo);
extern void gProgressUpdate(OPERATE_RESULT *puiInfo);
