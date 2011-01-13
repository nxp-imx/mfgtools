/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// OpMonitor.cpp : implementation file
//

#include "stdafx.h"
#include "PortMgrDlg.h"
#include "OpMonitor.h"

// COpMonitor

IMPLEMENT_DYNCREATE(COpMonitor, COperation)

COpMonitor::COpMonitor(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo *pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo)
{
	ASSERT(m_pUSBPort);
	m_sDescription.LoadString(IDS_OPMONITOR_MONITORING);
}

COpMonitor::~COpMonitor(void)
{
}

BOOL COpMonitor::InitInstance(void)
{
	m_iPercentComplete = -1;
	return true;
}

BEGIN_MESSAGE_MAP(COpMonitor, COperation)
	ON_THREAD_MESSAGE(WM_MSG_OPEVENT, OnMsgStateChange)
END_MESSAGE_MAP()

void COpMonitor::OnMsgStateChange(WPARAM nEventType, LPARAM dwData)
{
	CString LogStr, taskMsg;

//	ATLTRACE(_T("%s Updater Event: %s Msg: %s DevState: %s \r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()));

	switch(nEventType)
	{
        case OPEVENT_KILL:
            AfxEndThread(0);
			break;
		case OPEVENT_START:
			m_bStart = true;
			taskMsg = m_pUSBPort->GetDeviceDescription();
			if ( taskMsg.IsEmpty() )
				taskMsg.LoadString(IDS_NO_DEVICE_CONNECTED);
			m_pPortMgrDlg->UpdateUI(taskMsg, -32768);
			break;
		case OPEVENT_STOP:
			m_bStart = false;
			taskMsg.Empty();
			m_pPortMgrDlg->UpdateUI(taskMsg, -32768);
			m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT ,CPortMgrDlg::PD_EVNT_OP_COMPLETE);
			break;
		case OPEVENT_DEVICE_ARRIVAL: 
		case OPEVENT_VOLUME_ARRIVAL:
			taskMsg = m_pUSBPort->GetDeviceDescription();
			m_pPortMgrDlg->UpdateUI(taskMsg, -32768);
			break;
		case OPEVENT_DEVICE_REMOVAL:
		case OPEVENT_VOLUME_REMOVAL:
			taskMsg = m_pUSBPort->GetDeviceDescription();
			if ( taskMsg.IsEmpty() )
				taskMsg.LoadString(IDS_NO_DEVICE_CONNECTED);
			m_pPortMgrDlg->UpdateUI(taskMsg, -32768);
			break;
		case OPEVENT_TIMEOUT:
			ASSERT(0);
			if ( m_Timer )
			{
				m_pPortMgrDlg->KillTimer(m_Timer);
				m_Timer = 0;
			}
		default:
			// should never get here
			ASSERT (0);
	}
	return;
}
