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

// PortMgrDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MfgTool_MultiPanel.h"
#include "PortMgrDlg.h"
#include "CommonDef.h"
#include "MfgTool_MultiPanelDlg.h"


// CPortMgrDlg dialog

IMPLEMENT_DYNAMIC(CPortMgrDlg, CDialog)

CPortMgrDlg::CPortMgrDlg(CWnd* pParent /*=NULL*/, int index /* = 0 */)
	: CDialog(CPortMgrDlg::IDD, pParent)
{
	m_Index = index;
	m_bDeviceConnected = FALSE;
}

CPortMgrDlg::~CPortMgrDlg()
{
}

void CPortMgrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PORT_STATUS_EDIT, m_port_status_ctrl);
	DDX_Control(pDX, IDC_PORT_PROGRESS, m_port_progress_ctrl);
	DDX_Control(pDX, IDC_PORT_TASK_PROGRESS, m_port_task_progress_ctrl);
	DDX_Control(pDX, IDC_PORT_PHASE_PROGRESS, m_port_phase_progress_ctrl);
}


BEGIN_MESSAGE_MAP(CPortMgrDlg, CDialog)
	ON_MESSAGE(UM_PORT_DLG_MSG, OnPortDlgMsgFun)
	ON_MESSAGE(UM_PORT_DEVICE_CHANGE, OnDeviceChangeNotify)
	ON_MESSAGE(UM_PORT_UPDATE_INFO, OnUpdateUIInfo)
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CPortMgrDlg message handlers

BOOL CPortMgrDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	CPortMgr *pPortMgr = theApp.m_PortMgr_Array.GetAt(m_Index);
	if((theApp.m_OperationsInformation.pOperationInfo[m_Index].HubName != _T("")) && (theApp.m_OperationsInformation.pOperationInfo[m_Index].HubIndex != 0))
	{
		CString strPanelInfo;
		strPanelInfo.Format(_T("Hub %d--Port %d"), theApp.m_OperationsInformation.pOperationInfo[m_Index].HubIndex, theApp.m_OperationsInformation.pOperationInfo[m_Index].PortIndex);
		(GetDlgItem(ID_PORT_GROUP_BOX))->SetWindowText(strPanelInfo);
	}

	if(theApp.m_OperationsInformation.pOperationInfo[m_Index].bConnected)
	{
		(GetDlgItem(IDC_PORT_STATUS_EDIT))->SetWindowText(theApp.m_OperationsInformation.pOperationInfo[m_Index].DeviceDesc);
		CString strPanelInfo;
		strPanelInfo.Format(_T("Hub %d--Port %d"), theApp.m_OperationsInformation.pOperationInfo[m_Index].HubIndex, theApp.m_OperationsInformation.pOperationInfo[m_Index].PortIndex);
		(GetDlgItem(ID_PORT_GROUP_BOX))->SetWindowText(strPanelInfo);
		m_bDeviceConnected = TRUE;
	}
	else
	{
		(GetDlgItem(IDC_PORT_STATUS_EDIT))->SetWindowText(_T("No Device Connected"));
		m_bDeviceConnected = FALSE;
	}

	m_port_progress_ctrl.SetRange32(0, pPortMgr->m_AllCmdSize);
	m_port_phase_progress_ctrl.SetRange32(0, pPortMgr->m_AllPhaseNumbers);

	m_PreviousCommandsIndex = -1;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CPortMgrDlg::OnPortDlgMsgFun(WPARAM wEvent, LPARAM lParam)
{
	switch(wEvent)
	{
	case OP_CMD_START:
		break;
	case OP_CMD_STOP:
		{
			//Update UI
			m_port_task_progress_ctrl.ShowWindow(SW_HIDE);
			m_port_progress_ctrl.SetPos(0);
			TCHAR *buffer;
			CPortMgr *pPortMgr = theApp.m_PortMgr_Array.GetAt(m_Index);
			buffer = pPortMgr->GetCurrentDeviceDesc();
			m_port_status_ctrl.SetWindowText(buffer);
		}
		break;
	}

	return 0;
}

LRESULT CPortMgrDlg::OnDeviceChangeNotify(WPARAM wParam, LPARAM lParam)
{
	CPortMgr::DEV_CHG_NOTIFY *pNotifyInfo = NULL;

	pNotifyInfo = (CPortMgr::DEV_CHG_NOTIFY *)wParam;

	m_PreviousCommandsIndex = -1;

	if(pNotifyInfo->bDeviceConnected)
	{
		(GetDlgItem(IDC_PORT_STATUS_EDIT))->SetWindowText(pNotifyInfo->DeviceDesc);
		CString strPanelInfo;
		strPanelInfo.Format(_T("Hub %d--Port %d"), pNotifyInfo->HubIndex, pNotifyInfo->PortIndex);
		(GetDlgItem(ID_PORT_GROUP_BOX))->SetWindowText(strPanelInfo);
		CString strDiskDriver;
		strDiskDriver.Format(_T("%C:"), pNotifyInfo->DriverLetter);
		(GetDlgItem(IDC_PORT_DRIVE))->SetWindowText(strDiskDriver);
		m_bDeviceConnected = TRUE;
		if( (pNotifyInfo->Event == MX_DEVICE_ARRIVAL_EVT) || (pNotifyInfo->Event == MX_HUB_ARRIVAL_EVT) )
		{
			m_port_progress_ctrl.SetPos(0);
		}
	}
	else
	{
		(GetDlgItem(IDC_PORT_STATUS_EDIT))->SetWindowText(_T("No Device Connected"));
		(GetDlgItem(IDC_PORT_DRIVE))->SetWindowText(_T(""));
		if(pNotifyInfo->DriverLetter != _T(' '))
		{
			m_port_progress_ctrl.SetPos(0);
		}
		m_port_task_progress_ctrl.ShowWindow(SW_HIDE);
		m_bDeviceConnected = FALSE;
	}

	if(m_bDeviceConnected)
	{
		if( !((g_pMainDlg->m_OpBtnDlg.GetDlgItem(ID_START_STOP_TOGGLE))->IsWindowEnabled()) )
		{
			(g_pMainDlg->m_OpBtnDlg.GetDlgItem(ID_START_STOP_TOGGLE))->EnableWindow(TRUE);
		}
	}

	return 0;
}

LRESULT CPortMgrDlg::OnUpdateUIInfo(WPARAM wParam, LPARAM lParam)
{
	OPERATE_RESULT *pProgressInfo = (OPERATE_RESULT *)wParam;

	//update command text
	m_port_status_ctrl.SetWindowText(pProgressInfo->cmdInfo);

	//update commands progress
	int iRange = theApp.GetStateCommandSize(pProgressInfo->CurrentPhaseIndex);
	m_port_progress_ctrl.SetRange32(0, iRange);
	m_port_progress_ctrl.SetPos(pProgressInfo->cmdIndex);
	m_port_progress_ctrl.SetBarColor(PROGRESS_RGB_BLUE);

	//update phase progress
	m_port_phase_progress_ctrl.SetPos(pProgressInfo->CurrentPhaseIndex);
	m_port_phase_progress_ctrl.SetBarColor(PROGRESS_RGB_BLUE);

	if( (pProgressInfo->cmdStatus == COMMAND_STATUS_EXECUTE_COMPLETE) &&
		(pProgressInfo->cmdIndex == theApp.m_PhasesInformation.pPhaseInfo[theApp.m_PhasesInformation.PhaseInfoNumbers-1].uiPhaseCommandNumbers) )
	{	//complete the whole updating process
		m_port_phase_progress_ctrl.SetPos(PHASE_COMPLETE_POSITION);
		m_port_phase_progress_ctrl.SetBarColor(PROGRESS_RGB_GREEN);

		m_port_progress_ctrl.SetBarColor(PROGRESS_RGB_GREEN);

		g_successOps++;
		CString strMsg;
		strMsg.Format(_T("%d"), g_successOps);
		g_pMainDlg->m_OpBtnDlg.GetDlgItem(IDC_STATUS_OK_UPDATES)->SetWindowText(strMsg);
		double dRate = (double)g_failedOps / (g_failedOps+g_successOps);
		dRate = dRate * 100;
		strMsg.Format(_T("%.2f %%"), dRate);
		g_pMainDlg->m_OpBtnDlg.GetDlgItem(IDC_STATUS_FAILURE_RATE)->SetWindowText(strMsg);
	}
	else if(pProgressInfo->cmdStatus == COMMAND_STATUS_EXECUTE_ERROR)
	{
		m_port_progress_ctrl.SetBarColor(PROGRESS_RGB_RED);

		m_port_phase_progress_ctrl.SetPos(PHASE_COMPLETE_POSITION);
		m_port_phase_progress_ctrl.SetBarColor(PROGRESS_RGB_RED);

		g_failedOps++;
		CString strMsg;
		strMsg.Format(_T("%d"), g_failedOps);
		g_pMainDlg->m_OpBtnDlg.GetDlgItem(IDC_STATUS_FAILED_UPDATES)->SetWindowText(strMsg);
		double dRate = (double)g_failedOps / (g_failedOps+g_successOps);
		dRate = dRate * 100;
		strMsg.Format(_T("%.2f %%"), dRate);
		g_pMainDlg->m_OpBtnDlg.GetDlgItem(IDC_STATUS_FAILURE_RATE)->SetWindowText(strMsg);
	}

	if(pProgressInfo->bProgressWithinCommand)
	{
		if(pProgressInfo->TotalWithinCommand > 0)
		{
			m_port_task_progress_ctrl.SetRange32(0, pProgressInfo->TotalWithinCommand);
            m_port_task_progress_ctrl.SetPos(pProgressInfo->DoneWithinCommand);
			m_port_task_progress_ctrl.SetBarColor(PROGRESS_RGB_BLUE);
            m_port_task_progress_ctrl.ShowWindow(SW_SHOW);
		}
		else if(pProgressInfo->TotalWithinCommand == NO_ONE_COMMAND_PROGRESS_RANGE)
		{
			m_port_task_progress_ctrl.ShowWindow(SW_HIDE);
		}
	}
	else
	{
		m_port_task_progress_ctrl.ShowWindow(SW_HIDE);
	}

	return 0;
}

void CPortMgrDlg::PostNcDestroy()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialog::PostNcDestroy();
}

void CPortMgrDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CDialog::OnPaint() for painting messages
	m_port_progress_ctrl.RedrawWindow();
	m_port_task_progress_ctrl.RedrawWindow();
	m_port_phase_progress_ctrl.RedrawWindow();
}

BOOL CPortMgrDlg::PreTranslateMessage(MSG* pMsg)
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

