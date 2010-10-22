/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// OpErase.cpp : implementation file
//

#include "stdafx.h"
#include "PortMgrDlg.h"
#include "OpErase.h"

#include "Libs/DevSupport/WindowsVersionInfo.h"


IMPLEMENT_DYNCREATE(COpErase, COperation)

COpErase::COpErase(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo *pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo, ERASE_OP)
, m_OpState(INVALID)
{
	ASSERT(m_pUSBPort);
	ASSERT(m_pOpInfo);
	m_iDuration = 5;
	m_dwStartTime = 0;

}

COpErase::~COpErase()
{
}

BEGIN_MESSAGE_MAP(COpErase, COperation)
	ON_THREAD_MESSAGE(WM_MSG_OPEVENT, OnMsgStateChange)
END_MESSAGE_MAP()

// COpErase message handlers
void COpErase::OnMsgStateChange(WPARAM nEventType, LPARAM dwData)
{
	if ( nEventType == OPEVENT_KILL ) 
	{
		ATLTRACE(_T("%s Updater Event: %s \r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType));
		m_bStart = false;
        AfxEndThread(0);
		return;
    }

	ATLTRACE(_T("%s Erase Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()), GetOpStateString(m_OpState));

	CString taskMsg;

	if((nEventType != OPEVENT_START )&& (nEventType != OPEVENT_STOP) && (m_OpState == INVALID))
		return;

	switch(nEventType)
	{
		case OPEVENT_START:
			m_bStart = true;
			m_iPercentComplete = 0;
//			ATLTRACE(_T("%s Erase Event: OPEVENT_START Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_op_state);
			switch( GetDeviceState() )
			{
				case DISCONNECTED:
					m_OpState = WAITING;
					taskMsg.LoadString(IDS_WAITING_FOR_DEVICE);
					m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));
					break;
				case UPDATER_MODE:
				case MFG_MSC_MODE:
				case MSC_MODE:
					m_OpState = ERASING;
					taskMsg.LoadString(IDS_ERASEOP_ERASING_MEDIA);
					m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

					if( m_dwStartTime == 0 )
						m_dwStartTime = GetTickCount();

					if ( DoErase() == OPSTATUS_SUCCESS) {
							m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
							m_dwStartTime = 0;

							taskMsg.LoadString(IDS_ERASEOP_WAITING_VOLUME_REMOVAL);
							m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(3));
//							ATLTRACE(_T("****** %s Erase Event: OPEVENT_START Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_op_state);
							//m_pPortMgrDlg->UpdateUI(m_sStatus, (INT_PTR)(m_iDuration*double((90-m_iPercentComplete)/100)));
							//m_iPercentComplete = 90;
						}
					else {
						m_OpState = INVALID;
						taskMsg.LoadString(IDS_ERASEOP_ERROR_ERASING);
					}
					break;
				case RECOVERY_MODE:
					if ( m_OpState == COMPLETE )
						break;
				case CONNECTED_UNKNOWN:
				default:
					m_OpState = INVALID;
					taskMsg.LoadString(IDS_ERROR_INVALID_DEVICE);
					break;
			}
			break; // end OPEVENT_START

		case OPEVENT_STOP:
//			ATLTRACE(_T("%s Erase Event: OPEVENT_STOP Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_op_state);
			m_bStart = false;
			if ( m_OpState == ERASING ) {
				taskMsg.LoadString(IDS_ERASEOP_CANT_STOP);
				m_pPortMgrDlg->UpdateUI(taskMsg, 0);
			}
			else {
				m_OpState = COMPLETE;
				taskMsg.LoadString(IDS_STOPPED_BY_USER);
				m_pPortMgrDlg->UpdateUI(taskMsg, 0);
				m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT ,CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);
			}
			break;

		case OPEVENT_DEVICE_ARRIVAL:
			
//			ATLTRACE(_T("%s Erase Event: OPEVENT_DEVICE_ARRIVAL Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_op_state);

/*			switch( GetDeviceState() )			
			{
				case USBMSC_MODE:
				case USBMSC_PLUS_DRIVE_MODE:
					m_OpState = WAITING;
					m_iPercentComplete = 1;
					taskMsg.LoadString(IDS_WAITING_FOR_DRIVE); 
					m_pPortMgrDlg->UpdateUI(taskMsg, m_iPercentComplete);
					break;
				case RECOVERY_MODE:
					if ( m_OpState == ERASING )
						break;
				case DISCONNECTED:
				case CONNECTED_UNKNOWN:
				default:
					m_OpState = INVALID;
					taskMsg.LoadString(IDS_ERROR_INVALID_DEVICE);
					break;
			}
*/			break; // end OPEVENT_DEVICE_ARRIVAL

		case OPEVENT_VOLUME_ARRIVAL:
			if(m_OpState == COMPLETE || m_OpState == ERASING ) // got the drive msg late
				break;
			
//			ATLTRACE(_T("%s Erase Event: OPEVENT_VOLUME_ARRIVAL Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_op_state);
			if (( m_OpState == WAITING) && 
				( GetDeviceState() == UPDATER_MODE ||
				  GetDeviceState() == MFG_MSC_MODE ||
				  GetDeviceState() == MSC_MODE ))
			{
				m_OpState = ERASING;
				m_iPercentComplete = 1;
				taskMsg.LoadString(IDS_ERASEOP_ERASING_MEDIA);
				m_pPortMgrDlg->UpdateUI(taskMsg, m_iPercentComplete);

				if( m_dwStartTime == 0 )
					m_dwStartTime = GetTickCount();

				if ( DoErase() == OPSTATUS_SUCCESS) {
					m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
					m_dwStartTime = 0;

					m_iPercentComplete = 1;
						taskMsg.LoadString(IDS_ERASEOP_WAITING_VOLUME_REMOVAL);
						m_pPortMgrDlg->UpdateUI(taskMsg, m_iPercentComplete);
						//m_pPortMgrDlg->UpdateUI(taskMsg, (INT_PTR)(m_iDuration*double((90-m_iPercentComplete)/100)));
						//m_iPercentComplete = 90;
					}
				else {
					m_OpState = INVALID;
					taskMsg.LoadString(IDS_ERASEOP_ERROR_ERASING);
				}
			}
			else {
				m_OpState = INVALID;
				taskMsg.LoadString(IDS_ERROR_INVALID_DEVICE); 
			}
			break;

		case OPEVENT_DEVICE_REMOVAL:
//			GetDeviceState();
//			ATLTRACE(_T("%s Erase Event: OPEVENT_DEV_REMOVAL Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_op_state);
			if (( m_OpState != COMPLETE ) && ( m_OpState != ERASING )){
				m_OpState = INVALID;
				taskMsg.LoadString(IDS_ERROR_DEVICE_REMOVED_DURING_OP);
			}
			break;

		case OPEVENT_VOLUME_REMOVAL:
//			GetDeviceState();
//			ATLTRACE(_T("%s Erase Event: OPEVENT_VOL_REMOVAL Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_op_state);
			if( m_OpState == ERASING){
				m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
				m_dwStartTime = 0;

				m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT ,CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);
				m_OpState = COMPLETE;
				m_iPercentComplete = 1;
				taskMsg.LoadString(IDS_ERASEOP_COMPLETE);
				m_pPortMgrDlg->UpdateUI(taskMsg, m_iPercentComplete);
//				ATLTRACE(_T("%s Erase Complete... OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), m_op_state);
				//m_pPortMgrDlg->UpdateUI(m_sStatus, (INT_PTR)(m_iDuration*double((100-m_iPercentComplete)/100)));
			}
			else {
				m_OpState = INVALID;
				taskMsg.LoadString(IDS_ERROR_VOLUME_REMOVED_DURING_OP);
			}
			break;

		default:
			// should never get here
			ASSERT (0);
	}

	if(m_OpState == INVALID)
	{
//		ATLTRACE(_T("%s Erase Event: Error %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_op_state);
		m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_ERROR);
		m_bStart = false;
		m_iPercentComplete = 0;
		m_pPortMgrDlg->UpdateUI(_T(""), m_iPercentComplete);
	}
}

INT_PTR COpErase::DoErase(void)
{
/*clw
	CStScsi* p_scsi;

	p_scsi = new CStScsi_Nt(NULL);
	if( p_scsi->Initialize( wstring(m_sDrive) ) != STERR_NONE ) {
		ATLTRACE(_T("DoErase(): ERROR - Failed to initialize SCSI device.\n"));
		return OPSTATUS_ERROR;
	}
	
	CStEraseLogicalMedia api1;
	if( p_scsi->SendDdiApiCommand(&api1) != STERR_NONE ) {
		ATLTRACE(_T("DoErase(): ERROR - Failed to erase logical media.\n"));
		return OPSTATUS_ERROR;
	}

	m_iPercentComplete = 1;
	m_sStatus.LoadString(IDS_ERASEOP_ERASED_RESETTING);
	m_pPortMgrDlg->UpdateUI(m_sStatus, m_iPercentComplete);

	//m_iPercentComplete = 66;
	//m_pPortMgrDlg->UpdateUI(m_sStatus, (INT_PTR)(m_iDuration*double((m_iPercentComplete/100))));
	
	CStChipReset api2;
	if( p_scsi->SendDdiApiCommand(&api2) != STERR_NONE )
	{
		ATLTRACE(_T("DoErase(): ERROR - Failed to reset the device.\n"));
		return OPSTATUS_ERROR;
	}

	if( gWinVersionInfo().IsWin2K() )
	{
		CWnd* wnd = NULL;
		while( wnd == NULL )
		{
			wnd = CWnd::FindWindow(NULL, L"Unsafe Removal of Device");
			Sleep(1000);
		}
		if( wnd )
		{
			wnd->SendMessage( WM_CLOSE );
		}
	}
*///clw
	ATLTRACE(_T("DoErase() function completed successfully.\n"));
	return OPSTATUS_SUCCESS;
}

CString COpErase::GetOpStateString(OpState_t _state) const
{
	CString str;
	switch (_state)
	{
	case INVALID:
		str = "INVALID";
		break;
	case WAITING:
		str = "WAITING";
		break;
	case ERASING:
		str = "ERASING";
		break;
	case COMPLETE:
		str = "COMPLETE";
		break;
	default:
		str = "OP_UNKNOWN_STATE";
		break;
	}
	return str;
}
