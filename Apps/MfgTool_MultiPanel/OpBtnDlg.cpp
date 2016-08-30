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

// OpBtnDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MfgTool_MultiPanel.h"
#include "OpBtnDlg.h"

#include "../MfgToolLib/MfgToolLib_Export.h"
#include "CommonDef.h"
#include "MfgTool_MultiPanelDlg.h"


// COpBtnDlg dialog

IMPLEMENT_DYNAMIC(COpBtnDlg, CDialog)

COpBtnDlg::COpBtnDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COpBtnDlg::IDD, pParent)
{
	m_bRunning = FALSE;
}

COpBtnDlg::~COpBtnDlg()
{
}

void COpBtnDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(COpBtnDlg, CDialog)
	ON_BN_CLICKED(IDC_BTN_EXIT, &COpBtnDlg::OnBnClickedBtnExit)
	ON_BN_CLICKED(ID_START_STOP_TOGGLE, &COpBtnDlg::OnBnClickedStartStopToggle)
END_MESSAGE_MAP()


// COpBtnDlg message handlers

BOOL COpBtnDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	// first disable Start/Stop button
	

	GetDlgItem(IDC_STATUS_OK_UPDATES)->SetWindowText(_T("0"));
	GetDlgItem(IDC_STATUS_FAILED_UPDATES)->SetWindowText(_T("0"));
	CString strMsg;
	strMsg.Format(_T("%d %%"), 0);
	GetDlgItem(IDC_STATUS_FAILURE_RATE)->SetWindowText(strMsg);

	DoStartStop(m_bRunning);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void COpBtnDlg::OnBnClickedBtnExit()
{
	// TODO: Add your control notification handler code here
	if(m_bRunning)
	{
		AfxMessageBox(_T("Please press <Stop> and allow all operations to finish before exiting the program."));
		return;
	}
	g_pMainDlg->DestroyWindow();
}

void COpBtnDlg::DoStartStop(BOOL bStart)
{
	if (!bStart)
	{	// wanna stop
		for (int i = 0; i<theApp.m_PortMgr_Array.GetSize(); i++)
		{
			CPortMgr *pPortMgr = theApp.m_PortMgr_Array.GetAt(i);
			pPortMgr->StopDownload();
		}
		(GetDlgItem(ID_START_STOP_TOGGLE))->SetWindowText(_T("Start"));
	}
	else
	{	// wanna start
		for (int i = 0; i<theApp.m_PortMgr_Array.GetSize(); i++)
		{
			CPortMgr *pPortMgr = theApp.m_PortMgr_Array.GetAt(i);
			pPortMgr->StartDownload();
		}
		(GetDlgItem(ID_START_STOP_TOGGLE))->SetWindowText(_T("Stop"));
	}
}

void COpBtnDlg::OnBnClickedStartStopToggle()
{
	// TODO: Add your control notification handler code here

	DoStartStop(!m_bRunning);
	m_bRunning = !m_bRunning;
}

BOOL COpBtnDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if(pMsg->message == WM_KEYDOWN) 
	{
		switch(pMsg->wParam)  
		{  
		case VK_RETURN: 
			return true;  
		case VK_ESCAPE:
			return true;  
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}
