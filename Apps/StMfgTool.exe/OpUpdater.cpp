// OpUpdater.cpp : implementation file
//

#include "StdAfx.h"
#include "StMfgTool.h"
#include "PortMgrDlg.h"
#include "stmsg.h"
#include "DefaultProfile.h"

#include "../../Libs/DevSupport/StPitc.h"
#include "../../Libs/DevSupport/RecoveryDevice.h"
#include "../../Libs/DevSupport/StMtpApi.h"
#include "../../Libs/DevSupport/StFormatImage.h"

#include "OpUpdater.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// #define DEBUGOTPINIT 1
// #define DEBUG_LOGICAL_DRIVE_WRITE 1
#define PROFILE 1

extern BOOL g_TestLoop;
extern HANDLE g_HIDMutex;
BOOL g_StopFlagged;  // need a global to reflect the STOP state and break out of request for HID mutex

IMPLEMENT_DYNCREATE(COpUpdater, COperation)

COpUpdater::COpUpdater(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo* pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo, UPDATE_OP)
{
	ASSERT(m_pOpInfo);
	ASSERT(m_pUSBPort);

	m_pProfile = m_pOpInfo->GetProfile();

/*
	reset
	recover
	erase
	allocate
	get_sizes
	write/verify drives * num_drives
*/
	m_iDuration = 5 + (int)m_pOpInfo->GetDriveArray().Size();
/*
	init janus
	init store
*/
	if (m_pOpInfo->GetDriveArray().GetNumDrivesByType(media::DriveType_HiddenData) &&
		m_pOpInfo->IsWinCE() == FALSE )
		m_iDuration += 2;

	m_sDescription.LoadString(IDS_OPUPDATER_UPDATING);
	m_sVersion = GetProjectVersion();
}

COpUpdater::~COpUpdater(void)
{
}

BOOL COpUpdater::InitInstance(void)
{
	m_OpState = WAITING_FOR_DEVICE;
	m_dwStartTime = 0;

	m_bOwnMutex = FALSE;
	g_StopFlagged = FALSE;

	return true;
}

BEGIN_MESSAGE_MAP(COpUpdater, COperation)
	ON_THREAD_MESSAGE(WM_MSG_OPEVENT, OnMsgStateChange)
END_MESSAGE_MAP()

void COpUpdater::OnMsgStateChange(WPARAM nEventType, LPARAM dwData)
{
	CString LogStr, taskMsg;
	DWORD error = ERROR_SUCCESS;


	if ( nEventType == OPEVENT_KILL ) 
	{
		ATLTRACE(_T("%s Updater Event: %s \r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType));
		m_bStart = false;
		PostQuitMessage(0);
//        AfxEndThread(0);
		return;
    }

//	ATLTRACE(_T("%s Updater Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()), GetOpStateString(m_OpState));

    // Just return if we are in a BAD state and we get something other than START or STOP
    if((nEventType != OPEVENT_START) && (nEventType != OPEVENT_STOP) && (m_OpState == OP_INVALID))
		return;

	switch(nEventType)
	{
		case OPEVENT_START:
		{

			int iTPriority = GetThreadPriority();
			int iPPriority = GetPriorityClass(GetCurrentProcess());
			ATLTRACE(_T("%s Process-Thread priority: 0x%04x-%d \r\n"),m_pPortMgrDlg->GetPanel(), iPPriority, iTPriority);

			m_bStart = true;
			m_iPercentComplete = 0;
			switch( GetDeviceState() )
			{
				case DISCONNECTED:
				{
					taskMsg.LoadString(IDS_WAITING_FOR_DEVICE);           // "Waiting for device..."
					m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));   // STAGE 0 of the Update Operation

					m_OpState = WAITING_FOR_DEVICE;

					break;
				}
				case MSC_MODE:
				case MTP_MODE:
				{
//					if ( !m_Timer && m_TimeOutSeconds )
//						m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

					g_StopFlagged = FALSE;
					if( m_dwStartTime == 0 )
						m_dwStartTime = GetTickCount();

					if (!m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s MTP/MSC mode request (START)\r\n"), m_pPortMgrDlg->GetPanel());
						GetResetLockMutex(FALSE);
					}

					if (m_bOwnMutex)
						error = ResetToRecovery();
					else
						error = ERROR_SUCCESS;

					HandleError(error, NULL, WAITING_FOR_RECOVERY_MODE);

					break;
				}
				case RECOVERY_MODE:
				{
//					if ( !m_Timer && m_TimeOutSeconds )
//						m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

					g_StopFlagged = FALSE;
					if( m_dwStartTime == 0 )
						m_dwStartTime = GetTickCount();

					error = RecoverDevice();
					
					HandleError(error, NULL, WAITING_FOR_MFG_MSC_MODE);

					break;
				}
				case HID_MODE:
				{
//					if ( !m_Timer && m_TimeOutSeconds )
//						m_Timer = m_pPortMgrDlg->SetTimer(LOADER_OP, m_TimeOutSeconds * 1000, NULL);

					g_StopFlagged = FALSE;
					if( m_dwStartTime == 0 )
						m_dwStartTime = GetTickCount();

					if (!m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s HID mode request (START)\r\n"), m_pPortMgrDlg->GetPanel());
						GetResetLockMutex(FALSE);
					}

					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s HID mode has mutex hid recover (START)\r\n"), m_pPortMgrDlg->GetPanel());
						error = InitOtpRegs();
					
						HandleError(error, NULL, OP_RECOVERING);

						if ( error == ERROR_SUCCESS )
						{
							error = RecoverHidDevice();
					
							HandleError(error, NULL, WAITING_FOR_UPDATER_MODE);
						}
					}
					break;
				}				
				case UPDATER_MODE:
				case MFG_MSC_MODE:
				{
					g_StopFlagged = FALSE;
					PerformMscDeviceUpdate(LogStr);
					break;
				}
				case CONNECTED_UNKNOWN:
				{
					taskMsg.LoadString(IDS_OPUPDATER_INVALID_DEVICE); // "Updater Error: Invalid device to update."

					CString logStr = taskMsg;
					logStr.Append(m_pUSBPort->GetUsbDevicePath().c_str());

					HandleError(ERROR_INVALID_DRIVE, taskMsg, OP_INVALID, logStr);

					break;
				}
				default:
				{
					taskMsg.Format(IDS_OPUPDATER_INVALID_STATE, _T("START")); // "Updater Error: Invalid State (START)."

					HandleError(ERROR_INVALID_PARAMETER, taskMsg, OP_INVALID);

					break;
				}
			}			
			break;
		}

		case OPEVENT_STOP:
		{
			g_StopFlagged = TRUE;
			if ( m_OpState == WAITING_FOR_DEVICE || m_OpState == OP_COMPLETE ||
				m_OpState == WAITING_FOR_UPDATER_MODE || m_OpState == WAITING_FOR_MFG_MSC_MODE ||
				m_OpState == WAITING_FOR_SECOND_UPDATER_MODE || m_OpState == OP_INVALID )
			{
				if (m_OpState != OP_COMPLETE)
//					HandleError(ERROR_SUCCESS, NULL, OP_COMPLETE);
//				else
				{
					taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
					HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
				}

			}
			else
			{
				taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
				HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
			}

			if (m_bOwnMutex)
			{
				ATLTRACE(_T("MUTEX - %s release mutex (STOP)\r\n"), m_pPortMgrDlg->GetPanel());
				ReleaseResetLockMutex(); 
			}

			ATLTRACE(_T("STOPPING - %s \r\n"), m_pPortMgrDlg->GetPanel());
			break;
		}

//		case OPEVENT_TIMEOUT:
//		{
//			if (m_bOwnMutex)
//			{
//				ATLTRACE(_T("MUTEX - %s OP TIMEOUT release\r\n"), m_pPortMgrDlg->GetPanel());
//				ReleaseMutex(g_HIDMutex);
//				m_bOwnMutex = FALSE;
//			}
//
//			HandleError(WAIT_TIMEOUT, NULL, OP_INVALID);
//			break;
//		}

		case OPEVENT_DEVICE_ARRIVAL:

			switch(GetDeviceState())
			{		
				case MTP_MODE:
				{
					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s MTP mode release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					if (g_StopFlagged)
					{
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
						break;
					}

					if ( m_OpState == WAITING_FOR_DEVICE )
					{
//						if ( !m_Timer && m_TimeOutSeconds )
//							m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

						if( m_dwStartTime == 0 )
							m_dwStartTime = GetTickCount();

						if (!m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s MTP mode request (DEV_ARR)\r\n"), m_pPortMgrDlg->GetPanel());
							GetResetLockMutex(FALSE);
						}

						if (!g_StopFlagged)
						{
							ATLTRACE(_T("MUTEX - %s has mutext\r\n"), m_pPortMgrDlg->GetPanel());
							error = ResetToRecovery();
							HandleError(error, NULL, WAITING_FOR_RECOVERY_MODE);
						}
						else
						{
							taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
							HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
							break;
						}
					}
					else
					{
						HandleError(ERROR_INVALID_STATE, NULL, OP_INVALID);
					}					

					break;
				}

				case RECOVERY_MODE:
				{
					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s Recovery mode release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					if (g_StopFlagged)
					{
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
						break;
					}

					if ( m_OpState == WAITING_FOR_RECOVERY_MODE ||
						 m_OpState == WAITING_FOR_DEVICE )
					{
//						if ( !m_Timer && m_TimeOutSeconds )
//							m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

						if( m_dwStartTime == 0 )
							m_dwStartTime = GetTickCount();

						if (!m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s Recovery mode request (DEV_ARR)\r\n"), m_pPortMgrDlg->GetPanel());
							GetResetLockMutex(FALSE);
						}

						if (m_bOwnMutex)
						{
							error = RecoverDevice();
							HandleError(error, NULL, WAITING_FOR_MFG_MSC_MODE);
						}
						else
						{
							error = ERROR_PROCESS_ABORTED;
							taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
							HandleError(error, NULL, OP_INVALID);
						}
					}
					else
					{
						HandleError(ERROR_INVALID_STATE, NULL, OP_INVALID);
					}					

					break;
				}
				case HID_MODE:
				{
					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s HID mode device arrival release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					if (g_StopFlagged)
					{
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
						break;
					}

					if ( m_OpState == WAITING_FOR_RECOVERY_MODE ||
						 m_OpState == WAITING_FOR_DEVICE )
					{
//						if ( !m_Timer && m_TimeOutSeconds )
//							m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

						if( m_dwStartTime == 0 )
							m_dwStartTime = GetTickCount();

						if (!m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s HID mode device arrival request\r\n"), m_pPortMgrDlg->GetPanel());
							GetResetLockMutex(FALSE);
						}

						if (m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s has mutex\r\n"), m_pPortMgrDlg->GetPanel());

							error = InitOtpRegs();
				
							HandleError(error, NULL, OP_RECOVERING);

							if ( error == ERROR_SUCCESS )
							{
								error = RecoverHidDevice();
						
								HandleError(error, NULL, WAITING_FOR_UPDATER_MODE);
							}
						}
						else
						{
							taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
							HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
						}
					}
					else if (m_OpState != WAITING_FOR_UPDATER_MODE)
					{
						HandleError(ERROR_INVALID_STATE, NULL, OP_INVALID);
					}					

					break;
				}
			}
			break;
		
		case OPEVENT_DEVICE_REMOVAL:

			if(m_OpState == WAITING_FOR_UPDATER_MODE || 
				m_OpState == WAITING_FOR_SECOND_UPDATER_MODE ||
				m_OpState == WAITING_FOR_MFG_MSC_MODE ||
				m_OpState == WAITING_FOR_RECOVERY_MODE ||
				m_OpState == WAITING_FOR_DEVICE)
			{
				taskMsg.LoadString(IDS_WAITING_FOR_DEVICE);
				m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iPercentComplete));
			}
			else
			{
					HandleError(ERROR_INVALID_STATE, NULL, OP_INVALID);
			}
			break;

		case OPEVENT_VOLUME_ARRIVAL:

			ATLTRACE(_T("%s Updater Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()), GetOpStateString(m_OpState));
//			ATLTRACE(_T("COpUpdater::OnMsgStateChange() - VOLUME_ARRIVAL_EVT(%s)\n"), msg.c_str());
//			if(m_OpState == OP_COMPLETE || m_OpState == OP_RESETTING) // got drive msg late
//			break;

			switch( GetDeviceState() )
			{	
				case UPDATER_MODE:
				case MFG_MSC_MODE:
				{
					if (m_bOwnMutex)
					{
						Sleep(1500); // let the volume settle down
						ATLTRACE(_T("MUTEX - %s UPDATER/MFG mode device arrival release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					if (g_StopFlagged)
					{
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
					}
					else
						PerformMscDeviceUpdate(LogStr);
					break;
				}

				case MSC_MODE:
				{
					if (m_bOwnMutex)
					{
						Sleep(1500); // let the volume settle down
						ATLTRACE(_T("MUTEX - %s MSC mode device state release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					if (g_StopFlagged)
					{
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
						break;
					}

//					if ( !m_Timer && m_TimeOutSeconds )
//						m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

					if (!m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s MSC mode device state request\r\n"), m_pPortMgrDlg->GetPanel());
						GetResetLockMutex(FALSE);
					}

					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s MSC mode device state has mutex\r\n"), m_pPortMgrDlg->GetPanel());
						error = ResetToRecovery();
					}
					else
					{
						error = ERROR_PROCESS_ABORTED;
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						HandleError(error, NULL, OP_INVALID);
					}

					HandleError(error, NULL, WAITING_FOR_RECOVERY_MODE);

					break;
				}

				case CONNECTED_UNKNOWN:
				{
					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s unknown device state release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					taskMsg.LoadString(IDS_OPUPDATER_INVALID_DEVICE); // "Updater Error: Invalid device to update."
					
					CString logStr = taskMsg;
					logStr.Append(m_pUSBPort->GetUsbDevicePath().c_str());

					HandleError(ERROR_INVALID_DRIVE, taskMsg, OP_INVALID, logStr);

					break;
				}
				case DISCONNECTED:
				case RECOVERY_MODE:
				default:
				{
					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s default device state release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					taskMsg.Format(IDS_OPUPDATER_INVALID_STATE, _T("VOL_ARR")); // "Updater Error: Invalid State (VOL_ARR)."

					HandleError(ERROR_INVALID_PARAMETER, taskMsg, OP_INVALID);

					break;
				}
			}
			break;

		case OPEVENT_VOLUME_REMOVAL:
			taskMsg.LoadString(IDS_WAITING_FOR_DEVICE);
			if( m_OpState == WAITING_FOR_SECOND_UPDATER_MODE)
				m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iPercentComplete));
			else
				m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));
			break;

		default:
		{
			if (m_bOwnMutex)
			{
				ATLTRACE(_T("MUTEX - %s default event type release\r\n"), m_pPortMgrDlg->GetPanel());
				ReleaseResetLockMutex();
			}
			taskMsg.Format(IDS_OPUPDATER_INVALID_MSG, _T("DEF")); // "Updater Error: Invalid State (DEF)."

			HandleError(ERROR_INVALID_PARAMETER, taskMsg, OP_INVALID);

			break;
		}
	}
}

//// HandleError()
//
// Mangles member variables m_OpState, m_Timer, m_TimedOut, m_bStart, m_iPercentComplete, m_update_error_count
//
////
void COpUpdater::HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr)
{
	CString newLogStr;

	if ( _logStr )
	{
		if( _error == ERROR_SUCCESS )
		{
			newLogStr = CString(_logStr) + _T("SUCCESS\r\n");
		}
		else
		{
			newLogStr = CString(_logStr) + _T("FAILED\r\n");
		}

		// Log Operation SUCCESS/FAILURE
		BSTR bstr_log_text = newLogStr.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
	}

	if( _error == ERROR_SUCCESS )
	{
		m_OpState = _nextState;

		if ( m_OpState == OP_COMPLETE )
		{
			m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT ,CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);

			if ( m_Timer && m_TimeOutSeconds )
			{
				m_pPortMgrDlg->KillTimer(m_Timer);
				m_Timer = 0;
			}
		}
	}
	else
	{
		m_bStart = false;

		if ( m_Timer && m_TimeOutSeconds )
		{
			m_pPortMgrDlg->KillTimer(m_Timer);
			m_Timer = 0;
		}

		if (_error != ERROR_PROCESS_ABORTED &&!m_bOwnMutex && !g_StopFlagged)
		{
			ATLTRACE(_T("MUTEX - %s Error completion request\r\n"), m_pPortMgrDlg->GetPanel());
			GetResetLockMutex(FALSE);
		}

		if (_error == ERROR_PROCESS_ABORTED)
		{
			ATLTRACE(_T("%s Operation aborted\r\n"), m_pPortMgrDlg->GetPanel());
		}

        CStdString errorMsgStr;
		if ( _errorMsg )
		{
			errorMsgStr.Format(_T("%s"), _errorMsg);
		}
		else
		{
			CStdString systemMsgStr;
			FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, _error, 0, systemMsgStr.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
			errorMsgStr.Format(IDS_OPUPDATER_UPDATE_ERROR, _error, _error, systemMsgStr.c_str());		// "Updater Error 0x%x (%d) - %s"
		}
		m_pPortMgrDlg->UpdateUI(errorMsgStr, ProgressDelta(0));

		// trace error details
		CString traceErrorStr;
		traceErrorStr.Format(_T("%s: %s\r\n"), m_pPortMgrDlg->GetPanel(), errorMsgStr.c_str());
		ATLTRACE(traceErrorStr);
		// log error details
		BSTR bstr_log_text = traceErrorStr.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

		if (_error == ERROR_PROCESS_ABORTED)
		{
			if (m_OpState == OP_COMPLETE)
				m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);
			else
				m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_ABORTED);
		}
		else
			m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_ERROR);

		m_OpState = OP_INVALID;
	}
}

//// ResetToRecovery()
//
// ResetToRecovery() is STAGE 1 of the Update Operation
// Valid OpStates are WAITING_FOR_DEVICE, 
//
// changes member variable m_OpState
//
////
DWORD COpUpdater::ResetToRecovery()
{
	DWORD error;
	CString taskMsg;

	// No Task progress bar but bump the Operation progress bar
	taskMsg.LoadString(IDS_OPUPDATER_RESETTING_DEVICE); // "Resetting device..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(1)); // STAGE 1 of the Update Operation

	error = m_pUSBPort->_device->ResetToRecovery();
	if ( error != ERROR_SUCCESS )
	{
		error = m_pUSBPort->_device->OldResetToRecovery();
	}

	if ( error == ERROR_SUCCESS )
	{
		ATLTRACE(_T("%s ResetToRecovery - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	}
	else
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s ResetToRecovery - FAILED.\r\n"), error, m_pPortMgrDlg->GetPanel());
	}

	return error;
}

//// RecoverDevice()
//
// RecoverDevice() is STAGE 2 of the Update Operation
// Valid OpStates are WAITING_FOR_RECOVERY_MODE and WAITING_FOR_DEVICE
//
// changes member variable m_OpState
//
////
DWORD COpUpdater::RecoverDevice()
{
	DWORD error;
	CString taskMsg;
	Device::UI_Callback callback(this, &COpUpdater::OnDownloadProgress);

	taskMsg.LoadString(IDS_OPUPDATER_LOADING_USBMSC);
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));
	
	RecoveryDevice* pRecoveryDevice = dynamic_cast<RecoveryDevice*>(m_pUSBPort->_device);
	
	if ( pRecoveryDevice == NULL )
	{
		error = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s RecoverDevice() - No device.\r\n"), error, m_pPortMgrDlg->GetPanel());
		return error;
	}

	CString recoveryFirmwareFilename = m_pOpInfo->GetPath();
	recoveryFirmwareFilename.AppendFormat(_T("\\%s"), STATIC_ID_FW_FILENAME);
	if ( m_pOpInfo->UseMultipleStaticIdFw() )
	{
		recoveryFirmwareFilename.AppendFormat(_T("%c"), m_pPortMgrDlg->GetPanelIndex()+1);
	}
	recoveryFirmwareFilename.Append(STATIC_ID_FW_EXTENSION);

	StFwComponent fwObject(recoveryFirmwareFilename);

	// Set up the Task progress bar and bump the Operation progress bar
	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, fwObject.GetShortFileName().c_str());		// "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2), (int)fwObject.size());               // STAGE 2 of the Update Operation

	if ( (error = fwObject.GetLastError()) != ERROR_SUCCESS )
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s RecoverDevice() - Firmware object error.\r\n"), error, m_pPortMgrDlg->GetPanel());
		return error;
	}

	//
	// Do the Download
	//
	error = pRecoveryDevice->Download(fwObject, callback);
	if(error != ERROR_SUCCESS) 
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s RecoverDevice() - Failed during Download().\r\n"), error, m_pPortMgrDlg->GetPanel());
		return error;
	}

	// Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);

	ATLTRACE(_T("%s RecoverDevice - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	return ERROR_SUCCESS;
}

//// RecoverhidDevice()
//
// RecoverHidDevice() is STAGE 2 of the Update Operation
// Valid OpStates are WAITING_FOR_RECOVERY_MODE and WAITING_FOR_DEVICE
//
// changes member variable m_OpState
//
////

DWORD COpUpdater::RecoverHidDevice()
{

	DWORD ReturnVal = ERROR_SUCCESS;
	CString taskMsg;
#ifdef DEBUGOTPINIT
BSTR bstr_log_text;
CString _logText;
#endif
	taskMsg.LoadString(IDS_OPLOADER_LOAD_STARTED);
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

	HidDevice* pHidDevice = dynamic_cast<HidDevice*>(m_pUSBPort->_device);

	if ( pHidDevice == NULL )
	{

#ifdef DEBUGOTPINIT
_logText.Format(_T("%s HidRecover - pHiddevice is NULL\r\n"), m_pPortMgrDlg->GetPanel());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
		ReturnVal = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}


	CString fileName = //m_pOpInfo->GetDriveArray().GetDrive(media::DriveTag_Updater).Name;
					m_pOpInfo->GetPath() + _T("\\updater.sb");

	StPitc myNewFwCommandSupport(pHidDevice, (LPCTSTR)fileName,
		m_pOpInfo->GetProfile()->m_bLockedProfile ? 
			StFwComponent::LoadFlag_ResourceOnly :
			StFwComponent::LoadFlag_FileFirst); 

	if ( myNewFwCommandSupport.GetFwComponent().GetLastError() != ERROR_SUCCESS )
	{
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s HidRecover - No FW component\r\n"), m_pPortMgrDlg->GetPanel());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
		ReturnVal = myNewFwCommandSupport.GetFwComponent().GetLastError();
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No FW component. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Set up the Task progress bar and bump the Operation progress bar
	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, myNewFwCommandSupport.GetFwComponent().GetShortFileName().c_str());   // "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2), (int)myNewFwCommandSupport.GetFwComponent().size(), 0);       // STAGE 2 of the Load Operation
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s HidRecover - Downloading...\r\n"), m_pPortMgrDlg->GetPanel());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
	Device::UI_Callback callback(this, &COpUpdater::OnDownloadProgress);
	ReturnVal = myNewFwCommandSupport.DownloadPitc(callback);

	if(ReturnVal != ERROR_SUCCESS) 
	{
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s HidRecover - Download failed rc: %x\r\n"), m_pPortMgrDlg->GetPanel(), ReturnVal);
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, ReturnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);

#ifdef DEBUGOTPINIT
_logText.Format(_T("%s HidRecover - Complete\r\n"), m_pPortMgrDlg->GetPanel());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif

	ATLTRACE(_T("%s RecoverHidDevice - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	return ERROR_SUCCESS;
}

//// InitOtpRegs()
//
// InitOtpRegs() is STAGE 2 of the Update Operation
// Valid OpStates are WAITING_FOR_RECOVERY_MODE and WAITING_FOR_DEVICE
//
// changes member variable m_OpState
//
////
DWORD COpUpdater::InitOtpRegs()
{

	DWORD ReturnVal = ERROR_SUCCESS;
	CString taskMsg;
#ifdef DEBUGOTPINIT
BSTR bstr_log_text;
CString _logText;
#endif

	taskMsg.LoadString(IDS_OPLOADER_LOAD_STARTED);
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

	HidDevice* pHidDevice = dynamic_cast<HidDevice*>(m_pUSBPort->_device);

	if ( pHidDevice == NULL )
	{
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s OtpInit - pHiddevice is NULL\r\n"), m_pPortMgrDlg->GetPanel());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
		ReturnVal = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	CString fileName = //m_pOpInfo->GetDriveArray().GetDrive(media::DriveTag_Updater).Name;
					m_pOpInfo->GetPath() + _T("\\OtpInit.sb");
	StFwComponent::LoadFlag lflag = m_pOpInfo->GetProfile()->m_bLockedProfile ? 
			StFwComponent::LoadFlag_ResourceOnly :
			StFwComponent::LoadFlag_FileFirst; 

	StPitc OtpInitPitc(pHidDevice, (LPCTSTR)fileName, lflag);

	if ( OtpInitPitc.GetFwComponent().GetLastError() != ERROR_SUCCESS )
	{
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s OtpInit ERROR - GetFwComponent() rc: %x\r\n"), m_pPortMgrDlg->GetPanel(), OtpInitPitc.GetFwComponent().GetLastError());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
		// Turn off the Task progress bar
		m_pPortMgrDlg->UpdateUI(NULL);
		ReturnVal = ERROR_SUCCESS; //OtpInitPitc.GetFwComponent().GetLastError();
		ATLTRACE(_T("!!!WARNING!!! (%d): %s No OtpInit FW component. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s OtpInit downloading\r\n"), m_pPortMgrDlg->GetPanel());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif

	// Set up the Task progress bar and bump the Operation progress bar
	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, OtpInitPitc.GetFwComponent().GetShortFileName().c_str());   // "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2), (int)OtpInitPitc.GetFwComponent().size(), 0);       // STAGE 2 of the Update Operation

	Device::UI_Callback callback(this, &COpUpdater::OnDownloadProgress);
	ReturnVal = OtpInitPitc.DownloadPitc(callback);

	if(ReturnVal != ERROR_SUCCESS) 
	{
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s OtpInit ERROR - DownLoadPitc() rc: %x\r\n"), m_pPortMgrDlg->GetPanel(), ReturnVal);
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, ReturnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

#ifdef DEBUGOTPINIT
_logText.Format(_T("%s OtpInit SendCommand()\r\n"), m_pPortMgrDlg->GetPanel());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif

	HidPitcWrite apiOtpInit(0/*address*/,0/*length*/,0/*lock*/,NULL/*pData*/,0/*sizeof(pData*/);
	ReturnVal = OtpInitPitc.SendPitcCommand(apiOtpInit);

	if(ReturnVal != ERROR_SUCCESS) 
	{
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s OtpInit ERROR - SendCommand() rc: %x\r\n"), m_pPortMgrDlg->GetPanel(), ReturnVal);
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
ASSERT(FALSE);
#endif

		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, ReturnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);

	ATLTRACE(_T("%s InitOtpRegs - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());

#ifdef DEBUGOTPINIT
_logText.Format(_T("%s OtpInit Complete\r\n"), m_pPortMgrDlg->GetPanel(), ReturnVal);
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
	return ERROR_SUCCESS;
}

//// UpdateMscDevice()
//
// UpdateMscDevice() begins from STAGE 2 of the Update Operation
// Valid OpStates are MFG_MSC_MODE and UPDATER_MODE
//
// changes member variable m_OpState
//
////
DWORD COpUpdater::UpdateMscDevice(CString& _logText)
{
	DWORD rc = ERROR_SUCCESS;
	CString taskMsg;
	Device::UI_Callback callback(this, &COpUpdater::OnDownloadProgress);
	BSTR bstr_log_text;
	Device::NotifyStruct nsInfo(_T("COpUpdater::UpdateMscDevice"), Device::NotifyStruct::dataDir_Off, 0);

	Volume* pMscDevice = dynamic_cast<Volume*>(m_pUSBPort->_device);
	if ( pMscDevice == NULL )
	{
		rc = ERROR_INVALID_DRIVE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - No device.\r\n"), rc, m_pPortMgrDlg->GetPanel());
		return rc;
	}


	_logText.Format(_T("UPDATE ver(%s) %s\\%s SN:%s\r\n"/* Result: "*/), m_sVersion, m_pProfile->GetUsbVid(), m_pProfile->GetUsbPid(),GetDeviceSerialNumber(pMscDevice));
	bstr_log_text = _logText.AllocSysString();
	((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

    // Lock() opens the device and locks it.
    // Gotta lock the physical or else Windows won't re-read the MBR when we're done.
    HANDLE hVolume = pMscDevice->Lock(Volume::LockType_Physical);
	if ( hVolume == INVALID_HANDLE_VALUE )
	{
		_logText.Format(_T("  UPDATE Result: "));
		return ERROR_LOCK_FAILED;
	}

//StGetChipMajorRevId apiChipId;
//rc = pMscDevice->SendCommand(hVolume, apiChipId, NULL, nsInfo);
//uint16_t _chipId = apiChipId.GetChipMajorRevId() ;

//	if ( m_pOpInfo->EraseMedia() )
//	{
#ifdef PROFILE
	uint8_t moreInfo = 0;
	StGetLogicalMediaInfo apiMediaInfo(MediaInfoSizeInBytes);
	pMscDevice->SendCommand(hVolume, apiMediaInfo, &moreInfo, nsInfo);
	uint64_t size = apiMediaInfo.GetSizeInBytes();
	DWORD startTime = GetTickCount();
	DWORD zeroTime = startTime;
#endif
		// Erase Media
		taskMsg.LoadString(IDS_ERASEOP_ERASING_MEDIA);					// "Erasing media..."
		m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

		if ( (rc = EraseLogicalMedia(pMscDevice, hVolume, nsInfo)) != ERROR_SUCCESS )
		{
			pMscDevice->Unlock(hVolume);
			return rc;
		}
#ifdef PROFILE
		DWORD finishTime = GetTickCount();
		CString profileString;
		profileString.Format(_T("EraseMedia(%#0.2fGB): %#0.3f seconds."), (double)size/(1024.0*1024.0*1024.0), (double)(finishTime - startTime) / 1000.0 );
		bstr_log_text = profileString.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
		_logText.Format(_T("      UPDATE Result: "));
/*
		uint8_t moreInfo = 0;
		StEraseLogicalMedia apiEraseMedia;
		rc = pMscDevice->SendCommand(hVolume, apiEraseMedia, &moreInfo, nsInfo);
		if ( rc != ERROR_SUCCESS  || moreInfo )
		{
			uint32_t numFactoryBadBlocks = (DWORD)pMscDevice->_scsiSenseData.CommandSpecificInformation[0];
			uint8_t numFailedSdkBadBlocks = pMscDevice->_scsiSenseData.SenseKeySpecific[2];
			uint8_t totalLostSdkBadBlocks = pMscDevice->_scsiSenseData.SenseKeySpecific[1];
			
			_logText.Format(_T("  ERASE MEDIA ERROR (%d): numFactoryBadBlocks = %d, numFailedSdkBadBlocks = %d, totalLostSdkBadBlocks = %d\r\n"), rc, numFactoryBadBlocks, numFailedSdkBadBlocks, totalLostSdkBadBlocks);
			bstr_log_text = _logText.AllocSysString();
			((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

			_logText = "      UPDATE Result: ";

			ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to Erase media.\r\n    numFactoryBadBlocks = %d, numFailedSdkBadBlocks = %d, totalLostSdkBadBlocks = %d\r\n"), rc, m_pPortMgrDlg->GetPanel(), numFactoryBadBlocks, numFailedSdkBadBlocks, totalLostSdkBadBlocks);
			pMscDevice->Unlock(hVolume);

			return rc ? rc : ERROR_INVALID_BLOCK;
		}
		_logText.Format(_T("      UPDATE Result: "));
*/
		// Allocate Media
		taskMsg.LoadString(IDS_OPUPDATER_ALLOCATING);					// "Allocating media..."
		m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(3));

		media::MediaAllocationTable table = m_pOpInfo->GetDriveArray().MakeAllocationTable(m_pOpInfo->GetProfile()->m_bLockedProfile ? 
			StFwComponent::LoadFlag_ResourceOnly :
			StFwComponent::LoadFlag_FileFirst);
	    StAllocateLogicalMedia apiAllocateMedia(&table);
		ATLTRACE(_T("%s - Start Allocating media.\r\n"), m_pPortMgrDlg->GetPanel());
#ifdef PROFILE
		startTime = GetTickCount();
#endif
		rc = pMscDevice->SendCommand(hVolume, apiAllocateMedia, NULL, nsInfo);
		if ( rc != ERROR_SUCCESS )
		{
			ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to Allocate media.\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}
		ATLTRACE(_T("%s - Finished Allocating media.\r\n"), m_pPortMgrDlg->GetPanel());
//	} // if erasemedia

	// Update DriveArray with DriveNumbers and ActualSizes
	taskMsg.LoadString(IDS_OPUPDATER_GET_DRIVE_SIZES);				// "Getting partition sizes..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(4));

	StGetLogicalTable apiGetTable;
	rc = pMscDevice->SendCommand(hVolume, apiGetTable, NULL, nsInfo);
	if ( rc != ERROR_SUCCESS )
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to Get Allocation Table.\r\n"), rc, m_pPortMgrDlg->GetPanel());
		pMscDevice->Unlock(hVolume);
		return rc;
	}
	apiGetTable.UpdateDriveArray(m_pOpInfo->GetDriveArray());


	// Erase System drives before writing
	size_t logicalDriveIndex;
	for ( logicalDriveIndex = 0; logicalDriveIndex < m_pOpInfo->GetDriveArray().Size(); ++logicalDriveIndex )
	{
		media::LogicalDrive drive = m_pOpInfo->GetDriveArray()[logicalDriveIndex];
		
		StGetLogicalDriveInfo apiInfo(drive.DriveNumber, DriveInfoType);
		rc = pMscDevice->SendCommand(hVolume, apiInfo, NULL, nsInfo);
		if ( rc != ERROR_SUCCESS )
		{
			ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to Erase System Drive.\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}
		if ( apiInfo.GetType() == media::DriveType_System )
		{
			StEraseLogicalDrive apiErase(drive.DriveNumber);
			rc = pMscDevice->SendCommand(hVolume, apiErase, NULL, nsInfo);
			if ( rc != ERROR_SUCCESS )
			{
				ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to Erase System Drive.\r\n"), rc, m_pPortMgrDlg->GetPanel());
				pMscDevice->Unlock(hVolume);
				return rc;
			}
			ATLTRACE(_T("%s UpdateMscDevice() - Erased drive %d.\r\n"), m_pPortMgrDlg->GetPanel(), drive.DriveNumber);
		}
	}
#ifdef PROFILE
		finishTime = GetTickCount();
		profileString.Format(_T("AllocateMedia(%#0.2fGB): %#0.3f seconds."), (double)size/(1024.0*1024.0*1024.0), (double)(finishTime - startTime) / 1000.0 );
		bstr_log_text = profileString.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
	// Get the sector size
//	StGetLogicalMediaInfo apiInfo(MediaInfoAllocationUnitSizeInBytes);
//	rc = pMscDevice->SendCommand(hVolume, apiInfo, NULL, nsInfo);
//	if ( rc != ERROR_SUCCESS )
//	{
//		ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to get Sector size.\r\n"), rc, m_pPortMgrDlg->GetPanel());
//		pMscDevice->Unlock(hVolume);
//		return rc;
//	}

//	uint32_t sectorSize = apiInfo.GetAllocationUnitSizeInBytes();

	m_pPortMgrDlg->UpdateUI(NULL, ProgressDelta(5));

	//
	// DOWNLOAD DRIVES
	//
#ifdef PROFILE
		startTime = GetTickCount();
#endif
	size_t driveIndex;
	for ( driveIndex = 0; driveIndex < m_pOpInfo->GetDriveArray().Size(); ++driveIndex )
	{
		media::LogicalDrive drive = m_pOpInfo->GetDriveArray()[driveIndex];
		
		// Only if we have a valid drive number
		if ( drive.DriveNumber == media::InvalidDriveNumber )
			continue;

		// Get the sector size
		StGetLogicalDriveInfo apiInfo(drive.DriveNumber, DriveInfoSectorSizeInBytes);
		rc = pMscDevice->SendCommand(hVolume, apiInfo, NULL, nsInfo);
		if ( rc != ERROR_SUCCESS )
		{
			ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to get Sector size.\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}

		uint32_t sectorSize = apiInfo.GetSectorSize();

		// Holder for data to write to the drive
		std::vector<uint8_t> writeData;
		// Holder for verifying data
		std::vector<uint8_t> readData;

		//
		// IMAGE DATA
		//
		if ( drive.Flags & media::DriveFlag_ImageData || drive.Flags & media::DriveFlag_ImageDataRsc )
		{
			// Get the FW component
			StFwComponent fw(drive.Name, drive.Flags & media::DriveFlag_ImageDataRsc ? 
				StFwComponent::LoadFlag_ResourceOnly :
				StFwComponent::LoadFlag_FileFirst);
			if ( (rc = fw.GetLastError()) != ERROR_SUCCESS )
			{
				ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Firmware object error.\r\n"), rc, m_pPortMgrDlg->GetPanel());
				pMscDevice->Unlock(hVolume);
				return rc;
			}

			// TODO: CLW do we still need to do this?
			// Current thinking(7/3/07): No for 3600 and 3700, Yes for 3500
			// This tells the FW to write the project/component versions to the fields in the
			// redundant area. For 3600/3700 the versions are located in the header of the 
			// system drive.
			if ( drive.Type  == media::DriveType_System)
			{
				StVersionInfo verProduct = fw.GetProductVersion();
				StSetLogicalDriveInfo apiProductInfo(drive.DriveNumber, api::DriveInfoProjectVersion, verProduct);
				rc = pMscDevice->SendCommand(hVolume, apiProductInfo, NULL, nsInfo);
				if ( rc != ERROR_SUCCESS )
				{
					ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to set Product version.\r\n"), rc, m_pPortMgrDlg->GetPanel());
					pMscDevice->Unlock(hVolume);
					return rc;
				}

				StVersionInfo verComponent = fw.GetComponentVersion();
				StSetLogicalDriveInfo apiComponentInfo(drive.DriveNumber, api::DriveInfoComponentVersion, verComponent);
				rc = pMscDevice->SendCommand(hVolume, apiComponentInfo, NULL, nsInfo);
				if ( rc != ERROR_SUCCESS )
				{
					ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to set Component version.\r\n"), rc, m_pPortMgrDlg->GetPanel());
					pMscDevice->Unlock(hVolume);
					return rc;
				}
			}

			// put the data in our write holder
			writeData.assign(fw.GetData().begin(), fw.GetData().end());
		}

		//
		// JANUS INIT DATA
		//
		else if ( (drive.Flags & media::DriveFlag_JanusInit) && (drive.Type == media::DriveType_HiddenData) )
		{
			// This writes down the Janus header.
			media::HiddenDataDriveFormat hdd((uint32_t)drive.SizeInBytes/sectorSize);

			// put the data in our write holder
			writeData.resize(sizeof(hdd));
			memcpy_s(&writeData[0], writeData.size(), (uint8_t*)&hdd, sizeof(hdd));
		}
		//
		// FORMAT DATA
		//
		else if ( drive.Flags & media::DriveFlag_Format )
		{
			// Get the size of the data drive
			/*
			StGetLogicalDriveInfo apiDriveInfo(drive.DriveNumber, DriveInfoSizeInSectors);
			rc = pMscDevice->SendCommand(hVolume, apiDriveInfo, NULL, nsInfo);
			if ( rc != ERROR_SUCCESS )
			{
				ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to get Sector size.\r\n"), rc, m_pPortMgrDlg->GetPanel());
				pMscDevice->Unlock(hVolume);
				return rc;
			}
			uint32_t driveSizeInSectors = (uint32_t)apiDriveInfo.GetSizeInSectors();
			*/
			StReadCapacity apiReadCapacity;
			rc = pMscDevice->SendCommand(hVolume, apiReadCapacity, NULL, nsInfo);
			if ( rc != ERROR_SUCCESS )
			{
				ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to get Sector size.\r\n"), rc, m_pPortMgrDlg->GetPanel());
				pMscDevice->Unlock(hVolume);
				return rc;
			}
			READ_CAPACITY_DATA ReadCaps = apiReadCapacity.GetCapacity();
			uint32_t driveSizeInSectors = ReadCaps.LogicalBlockAddress+1;

			// Get the FormatInfo
			StFormatInfo::FileSystemT fs = m_pOpInfo->UseFat32() ? StFormatInfo::FS_FAT32 : StFormatInfo::FS_DEFAULT;
			CString volumeLabel = m_pOpInfo->GetProfile()->UseVolumeLabel() ? m_pOpInfo->GetProfile()->GetVolumeLabel() : _T("\0\0\0\0\0\0\0\0\0\0\0"); 
			StFormatInfo formatParams(driveSizeInSectors, sectorSize, fs, volumeLabel);
			
			// Create the Format Image
			StFormatImage formatImage(formatParams);

			// Add files to image if specified
			if ( drive.Flags & media::DriveFlag_FileData )
			{
				rc = formatImage.AddFiles(drive.Name);
				if ( rc != ERROR_SUCCESS )
				{
					ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to add files to Format Image.\r\n"), rc, m_pPortMgrDlg->GetPanel());
					pMscDevice->Unlock(hVolume);
					return rc;
				}

			}

			// put the data in our write holder
			writeData.assign(formatImage._theImage.begin(), formatImage._theImage.end());
		}

		// If there isn't any data, then there is nothing to do for this drive. Go on to next drive.
		if ( writeData.empty() )
			continue;

		// Write the Data to the Drive.
		if ( drive.Flags & media::DriveFlag_Format )
			taskMsg.LoadString(IDS_PORTMGRDLG_FORMATTING);
		else
			taskMsg.Format(IDS_OPUPDATER_WRITING_FILE, drive.Description.c_str());					// "Writing <filename> ..."	
		m_pPortMgrDlg->UpdateUI(taskMsg, 0, (int)writeData.size());

		rc = pMscDevice->WriteDrive(hVolume, drive.DriveNumber, writeData, writeData.size(), callback);
		if ( rc != ERROR_SUCCESS )
		{
			ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed during WriteDrive().\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}

#ifdef DEBUG_LOGICAL_DRIVE_WRITE
		if ( drive.Flags & media::DriveFlag_ImageData )
		{
			BSTR bstr_log_text;
			CString _logText;
			_logText.Format(_T("%s Wrote %d bytes to logical drive %d\r\n"), m_pPortMgrDlg->GetPanel(), pMscDevice->m_BytesWritten, drive.DriveNumber);
			bstr_log_text = _logText.AllocSysString();
			((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
		}
#endif

		// Read back verify.
		taskMsg.Format(IDS_OPUPDATER_VERIFYING_FILE, drive.Description.c_str());				// "Verifying <filename> ..."
		m_pPortMgrDlg->UpdateUI(taskMsg, 0, (int)writeData.size(), 0);

		rc = pMscDevice->ReadDrive(hVolume, drive.DriveNumber, readData, writeData.size(), callback);
		if ( rc != ERROR_SUCCESS )
		{
			ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed during ReadDrive().\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}

#ifdef DEBUG_LOGICAL_DRIVE_WRITE
//		_logText.Format(_T("%s Read %d bytes from logical drive %d\r\n"), m_pPortMgrDlg->GetPanel(), pMscDevice->m_BytesRead, drive.DriveNumber);
//		bstr_log_text = _logText.AllocSysString();
//		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif

		if ( readData != writeData )
		{
			rc = ERROR_INVALID_DATA; 
			ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Read data not equal to Write data.\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}

		// Turn off task progress and bump op progress
		m_pPortMgrDlg->UpdateUI(NULL, ProgressDelta(m_iPercentComplete+1));
#ifdef PROFILE
		if ( drive.Flags & media::DriveFlag_ImageData )
			size = writeData.size();
#endif
	}
#ifdef PROFILE
		finishTime = GetTickCount();
		profileString.Format(_T("WriteAndVerifyFirmware(%#0.2fMB * 3): %#0.3f seconds."), (double)size/(1024.0*1024.0), (double)(finishTime - startTime) / 1000.0 );
		bstr_log_text = profileString.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

		profileString.Format(_T("TotalUpdateTime: %#0.3f seconds."), (double)(finishTime - zeroTime) / 1000.0 );
		bstr_log_text = profileString.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif

    // Invalidates the cached partition table and re-enumerates the device.
    int32_t err = pMscDevice->Update(hVolume);
    // Frees the device for use and closes our handle
    err = pMscDevice->Unlock(hVolume);

    // Dismount the Logical Volume so Windows will update its filesystem info.
    hVolume = pMscDevice->Lock(Volume::LockType_Logical);
    err = pMscDevice->Dismount(hVolume);
    err = pMscDevice->Unlock(hVolume);

    // Touch all the drives to 'refresh' the one we updated.
    DWORD mask = ::GetLogicalDrives();

	//
	// For Windows CE devices, we are done.
	//
	if ( m_pOpInfo->IsWinCE() )
	{
		ATLTRACE(_T("%s UpdateMscDevice() - Completed for Windows CE firmware.\r\n"), m_pPortMgrDlg->GetPanel());
		return ERROR_SUCCESS;
	}

	StGetProtocolVersion apiGetProtocolVersion;
	rc = pMscDevice->SendCommand(apiGetProtocolVersion);
	if( apiGetProtocolVersion.GetMajorVersion() >= ProtocolUpdaterJanus )
	{
		if( apiGetProtocolVersion.GetMajorVersion() >= ProtocolUpdaterJanus3 )
		{
			CString fileName = m_pOpInfo->GetDriveArray().GetDrive(media::DriveTag_FirmwareImg).Name;
			StFwComponent fw(fileName, m_pOpInfo->GetProfile()->m_bLockedProfile ? 
				StFwComponent::LoadFlag_ResourceOnly :
				StFwComponent::LoadFlag_FileFirst);
			if ( fw.GetFlags() & 0x100 )
			{
				// Reset to updater mode
				StResetToUpdater apiResetToUpdater;
				
				ATLTRACE(_T("MUTEX - %s Update reset to updater request\r\n"), m_pPortMgrDlg->GetPanel());
				GetResetLockMutex(FALSE);
				ATLTRACE(_T("MUTEX - %s Update reset to updater has mutex\r\n"), m_pPortMgrDlg->GetPanel());

				if (m_bOwnMutex)
					pMscDevice->SendCommand(apiResetToUpdater);
				else
					return ERROR_PROCESS_ABORTED;

				return OP_UPDATE_INCOMPLETE;
			}
		}
		else
		{
			rc = CompleteUpdate();
		}
	}

	return rc;
}

DWORD COpUpdater::CompleteUpdate()
{
	DWORD rc = ERROR_SUCCESS;
	CString taskMsg;

	Volume* pMscDevice = dynamic_cast<Volume*>(m_pUSBPort->_device);
	if ( pMscDevice == NULL )
	{
		rc = ERROR_INVALID_DRIVE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - No device.\r\n"), rc, m_pPortMgrDlg->GetPanel());
		return rc;
	}
	//HANDLE hVolume = pMscDevice->Lock( Volume::LockType_Logical );
//	pMscDevice->Dismount( hVolume );
//	pMscDevice->Unlock( hVolume );

	ATLTRACE(_T("%s CompleteUpdate MSC device: %x\r\n"), m_pPortMgrDlg->GetPanel(), pMscDevice);

///////////////////////////////////////////////////////////START
//	rc = VerifyNand(pMscDevice);
//	if ( rc != ERROR_SUCCESS )
//	{
//		ATLTRACE(_T("!!!ERROR!!! (%d): %s CompleteUpdate() - Failed to Verify NAND.\r\n"), rc, m_pPortMgrDlg->GetPanel());
//		return rc;
//	}
///////////////////////////////////////////////////////////END

	CString fileName = m_pOpInfo->GetDriveArray().GetDrive(media::DriveTag_FirmwareImg).Name;
	StFwComponent fw(fileName, m_pOpInfo->GetProfile()->m_bLockedProfile ? 
			StFwComponent::LoadFlag_ResourceOnly :
			StFwComponent::LoadFlag_FileFirst);
	if ( fw.GetFlags() & 0x100 )
	{	//
		// JANUS_INIT
		//
		taskMsg.LoadString(IDS_OPUPDATER_INITIALIZING_DRM);								// "Initializing DRM..."
		m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iPercentComplete+1));
		ATLTRACE(_T("%s Executing JanusInit\r\n"), m_pPortMgrDlg->GetPanel());
		StInitializeJanus apiInitJanus;
			rc = pMscDevice->SendCommand(apiInitJanus);
		if ( rc != ERROR_SUCCESS )
		{
			ATLTRACE(_T("!!!ERROR!!! (%d): %s CompleteUpdate() - Failed to Initialize DRM.\r\n"), rc, m_pPortMgrDlg->GetPanel());
			return rc;
		}
		ATLTRACE(_T("%s CompleteUpdate JanusInit completed.\r\n"), m_pPortMgrDlg->GetPanel());
	}

	//
	// INIT_STORE
	//
	taskMsg.LoadString(IDS_OPUPDATER_INITIALIZING_DATABASE);						// "Initializing database..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iPercentComplete+1));

	StInitializeDataStore apiInitStore(0);
	rc = pMscDevice->SendCommand(apiInitStore);
	if ( rc != ERROR_SUCCESS )
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s CompleteUpdate() - Failed to Initialize Database.\r\n"), rc, m_pPortMgrDlg->GetPanel());
		return rc;
	}

	ATLTRACE(_T("%s CompleteUpdate StoreInit completed.\r\n"), m_pPortMgrDlg->GetPanel());

	return rc;
}

DWORD COpUpdater::EraseLogicalMedia(Volume* pMscDevice, HANDLE hVolume, Device::NotifyStruct& nsInfo)
{
	uint8_t moreInfo = 0;
	BSTR bstr_log_text;
	CString _logText;
	DWORD rc;
	int count = 0;

	bool newCmdNotSupported = false;

	// try the new asyn command
	StEraseLogicalMediaAsync apiEraseMediaAsync;
	while ( 1 )
	{
		DWORD rc = pMscDevice->SendCommand(hVolume, apiEraseMediaAsync, &moreInfo, nsInfo);
		if ( rc != ERROR_SUCCESS )
		{
			return rc;
		}
		else if ( moreInfo == SCSI_SENSE_NO_SENSE )
		{
			// Done. All is good.
			return ERROR_SUCCESS;
		}
		else if ( moreInfo == SCSISTAT_CHECK_CONDITION )
		{
			if ( apiEraseMediaAsync.ScsiSenseData.SenseKey == SCSI_SENSE_ILLEGAL_REQUEST )
			{
				// new Async command is not supported, try the old way.
				newCmdNotSupported = true;
				break;
			}
			else if ( apiEraseMediaAsync.ScsiSenseData.SenseKey == SCSI_SENSE_UNIQUE )
			{
				if ( apiEraseMediaAsync.ScsiSenseData.AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY &&
					 apiEraseMediaAsync.ScsiSenseData.AdditionalSenseCodeQualifier == SCSI_SENSEQ_OPERATION_IN_PROGRESS )
				{
					// Async Erase is working so sleep a bit and try again to see if it is finished.
					ATLTRACE(_T("%s - Waiting for EraseLogicalMedia() to finish. (%d)\r\n"), m_pPortMgrDlg->GetPanel(), count++);
					Sleep(1000);
					continue;
				}
			}
		}

		// if we got here, there is an error
		return ERROR_INVALID_BLOCK;
	}

	if ( newCmdNotSupported )
	{
		// try the old command
		StEraseLogicalMedia apiEraseMedia;
		rc = pMscDevice->SendCommand(hVolume, apiEraseMedia, &moreInfo, nsInfo);
		if ( rc != ERROR_SUCCESS  || moreInfo )
		{
			uint32_t numFactoryBadBlocks = (DWORD)apiEraseMedia.ScsiSenseData.CommandSpecificInformation[0];
			uint8_t numFailedSdkBadBlocks = apiEraseMedia.ScsiSenseData.SenseKeySpecific[2];
			uint8_t totalLostSdkBadBlocks = apiEraseMedia.ScsiSenseData.SenseKeySpecific[1];
			
			_logText.Format(_T("  ERASE MEDIA ERROR (%d): numFactoryBadBlocks = %d, numFailedSdkBadBlocks = %d, totalLostSdkBadBlocks = %d\r\n"), rc, numFactoryBadBlocks, numFailedSdkBadBlocks, totalLostSdkBadBlocks);
			bstr_log_text = _logText.AllocSysString();
			((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

			_logText = "      UPDATE Result: ";

			ATLTRACE(_T("!!!ERROR!!! (%d): %s UpdateMscDevice() - Failed to Erase media.\r\n    numFactoryBadBlocks = %d, numFailedSdkBadBlocks = %d, totalLostSdkBadBlocks = %d\r\n"), rc, m_pPortMgrDlg->GetPanel(), numFactoryBadBlocks, numFailedSdkBadBlocks, totalLostSdkBadBlocks);

			return rc ? rc : ERROR_INVALID_BLOCK;
		}
	}

	return rc;
}

CString COpUpdater::GetProjectVersion()
{
	CString versionStr = _T("");
	
	size_t driveIndex;
	for ( driveIndex = 0; driveIndex < m_pOpInfo->GetDriveArray().Size(); ++driveIndex )
	{
		CString fileName = m_pOpInfo->GetDriveArray()[driveIndex].Name;
		
		if ( !fileName.IsEmpty() )
		{
			StFwComponent::LoadFlag src;

			if ( m_pOpInfo->GetProfile()->m_bLockedProfile &&
				m_pOpInfo->GetDriveArray()[driveIndex].Tag != media::DriveTag_DataSettings )
				src = StFwComponent::LoadFlag_ResourceOnly;
			else
				src = StFwComponent::LoadFlag_FileFirst;

			StFwComponent tempFwObj(fileName, src);

			if ( tempFwObj.GetLastError() == ERROR_SUCCESS )
			{
				versionStr = tempFwObj.GetProductVersion().toString();
				break;
			}
		}
	}

	return versionStr;
}

void COpUpdater::OnDownloadProgress(const Device::NotifyStruct& nsInfo)
{
	if (nsInfo.position)
	{
		m_pPortMgrDlg->UpdateUI(NULL, 0, 0, nsInfo.position);
	}
}

CString COpUpdater::GetOpStateString(OpState_t _state)
{
	CString str;
	switch (_state)
	{
	case OP_INVALID:
		str = "OP_INVALID";
		break;
	case WAITING_FOR_DEVICE:
		str = "WAITING_FOR_DEVICE";
		break;
//	case WAITING_FOR_HID_MODE:
//		str = "WAITING_FOR_HID_MODE";
//		break;
	case WAITING_FOR_RECOVERY_MODE:
		str = "WAITING_FOR_RECOVERY_MODE";
		break;
	case OP_RECOVERING:
		str = "OP_RECOVERING";
		break;
	case WAITING_FOR_UPDATER_MODE:
	case WAITING_FOR_SECOND_UPDATER_MODE:
		str = "WAITING_FOR_UPDATER_MODE";
		break;
	case WAITING_FOR_MFG_MSC_MODE:
		str = "WAITING_FOR_MFG_MSC_MODE";
		break;
	case WAITING_FOR_MSC_MODE:
		str = "WAITING_FOR_MSC_MODE";
		break;
	case WAITING_FOR_MTP_MODE:
		str = "WAITING_FOR_MTP_MODE";
		break;
	case OP_FLASHING:
		str = "OP_FLASHING";
		break;
	case OP_COMPLETE:
		str = "OP_COPMLETE";
		break;
	default:
		str = "OP_UNKNOWN_STATE";
		break;
	}
	return str;
}


#define ONE_MINUTE		60000
#define THIRTY_SECONDS	30000

BOOL COpUpdater::GetResetLockMutex(BOOL _bOpComplete)
{
	int iCurrentPriority = THREAD_PRIORITY_NORMAL;
	DWORD timeout = THIRTY_SECONDS; // start off with this;

	if (!m_bOwnMutex)
	{
		while ( TRUE )
		{
			DWORD rc;
#ifdef SERIALIZE_HID
			rc = WaitForSingleObject(g_HIDMutex, timeout);
#else
			rc = WAIT_OBJECT_0;
#endif

			if (rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED)
			{
				m_bOwnMutex = TRUE;
				break;
			}
			if ( g_StopFlagged && _bOpComplete )
			{
				ReleaseResetLockMutex();
				break;
			}

			iCurrentPriority = GetThreadPriority();

			// boost if needed and go again
			if ( iCurrentPriority == THREAD_PRIORITY_NORMAL)
			{
				ATLTRACE(_T("MUTEX - %s Boosting thread priority to above normal...\r\n"), m_pPortMgrDlg->GetPanel());
				SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);
				timeout = ONE_MINUTE;
			}
			if ( iCurrentPriority == THREAD_PRIORITY_ABOVE_NORMAL)
			{
				ATLTRACE(_T("MUTEX - %s Boosting thread priority to highest...\r\n"), m_pPortMgrDlg->GetPanel());
				SetThreadPriority(THREAD_PRIORITY_HIGHEST);
				timeout = INFINITE;
			}
		}
	}

	if ( iCurrentPriority != THREAD_PRIORITY_NORMAL)
		SetThreadPriority(THREAD_PRIORITY_NORMAL);

	return m_bOwnMutex;
}

BOOL COpUpdater::ReleaseResetLockMutex()
{
	if (m_bOwnMutex)
	{
#ifdef SERIALIZE_HID
		ReleaseMutex(g_HIDMutex);
#endif
		m_bOwnMutex = FALSE;
		Sleep(0);
	}
	return !m_bOwnMutex;
}

DWORD COpUpdater::PerformMscDeviceUpdate(CString& _logText)
{
	DWORD error = ERROR_SUCCESS;

	if (m_OpState == WAITING_FOR_SECOND_UPDATER_MODE)
	{	// SDK5 and later do Janus after resetting to hostlink mode
		error = CompleteUpdate();
		if (error != ERROR_SUCCESS)
		{
			HandleError(error, NULL, OP_INVALID, _logText);
			return error;
		}
	}
	else
	{
		error = UpdateMscDevice(_logText);
		if( error == OP_UPDATE_INCOMPLETE )
		{
			HandleError(ERROR_SUCCESS, NULL, WAITING_FOR_SECOND_UPDATER_MODE);
			return ERROR_SUCCESS;
		}
	}

	if( error == ERROR_SUCCESS )
	{
		CString taskMsg;

		m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
		m_dwStartTime = 0;
		m_OpState = OP_COMPLETE;
/*
		if ( !m_bOwnMutex && !g_StopFlagged )
		{
			taskMsg.LoadString(IDS_WAITING_TO_COMPLETE);						// "Update Complete."
			m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iDuration));	// 100% complete

			ATLTRACE(_T("MUTEX - %s Update complete; request mutex for completion\r\n"), m_pPortMgrDlg->GetPanel());
			GetResetLockMutex(TRUE);
			ATLTRACE(_T("MUTEX - %s Update complete; have mutex for completion\r\n"), m_pPortMgrDlg->GetPanel());
		}
		else if ( m_bOwnMutex && g_StopFlagged )
		{
			ATLTRACE(_T("MUTEX - %s Update complete; STOP signaled releasing mutex\r\n"), m_pPortMgrDlg->GetPanel());
			ReleaseResetLockMutex();
		}
*/
//		taskMsg.LoadString(IDS_OPUPDATER_COMPLETE);						// "Update Complete."
//		m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iDuration));	// 100% complete
	
		ATLTRACE(_T("%s PerformMscDeviceUpdate - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
		HandleError(error, NULL, OP_COMPLETE, _logText);

		if (g_TestLoop && !g_StopFlagged)
		{
			Sleep(3500);
			if ( !m_bOwnMutex && !g_StopFlagged )
				GetResetLockMutex(TRUE);
			if (!g_StopFlagged)
				ResetToRecovery();
			else
				ReleaseResetLockMutex();

			ATLTRACE(_T("%s Update complete reset to recovery ( TestLoop)\r\n"), m_pPortMgrDlg->GetPanel());
		}
	}
	else
		HandleError(error, NULL, OP_INVALID, _logText);

	return error;
}


DWORD COpUpdater::VerifyNand(Volume* pMscDevice)
{
	DWORD rc = ERROR_SUCCESS;
	Device::NotifyStruct nsInfo(_T("COpUpdater::VerifyNand"), Device::NotifyStruct::dataDir_FromDevice, 0);
	Device::UI_Callback callback(this, &COpUpdater::OnDownloadProgress);

	HANDLE hVolume = pMscDevice->Lock( Volume::LockType_Logical );

	size_t driveIndex;
	for ( driveIndex = 0; driveIndex < m_pOpInfo->GetDriveArray().Size(); ++driveIndex )
	{
		media::LogicalDrive drive = m_pOpInfo->GetDriveArray()[driveIndex];
		
		// Only if we have a valid drive number
		if ( drive.DriveNumber == media::InvalidDriveNumber )
			continue;

		// Get the sector size
		StGetLogicalDriveInfo apiInfo(drive.DriveNumber, DriveInfoSectorSizeInBytes);
		rc = pMscDevice->SendCommand(hVolume, apiInfo, NULL, nsInfo);
		if ( rc != ERROR_SUCCESS )
		{
			ATLTRACE(_T("!!!ERROR!!! (%d): %s VerifyNand() - Failed to get Sector size.\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}

		uint32_t sectorSize = apiInfo.GetSectorSize();

		// Holder for data to write to the drive
		std::vector<uint8_t> writeData;
		// Holder for verifying data
		std::vector<uint8_t> readData;

		//
		// IMAGE DATA
		//
		if ( drive.Flags & media::DriveFlag_ImageData || drive.Flags & media::DriveFlag_ImageDataRsc )
		{
			// Get the FW component
			StFwComponent fw(drive.Name, drive.Flags & media::DriveFlag_ImageDataRsc ? 
				StFwComponent::LoadFlag_ResourceOnly :
				StFwComponent::LoadFlag_FileFirst);
			if ( (rc = fw.GetLastError()) != ERROR_SUCCESS )
			{
				ATLTRACE(_T("!!!ERROR!!! (%d): %s VerifyNand() - Firmware object error.\r\n"), rc, m_pPortMgrDlg->GetPanel());
				pMscDevice->Unlock(hVolume);
				return rc;
			}

			// put the data in our write holder
			writeData.assign(fw.GetData().begin(), fw.GetData().end());
		}

		//
		// JANUS INIT DATA
		//
		else if ( (drive.Flags & media::DriveFlag_JanusInit) && (drive.Type == media::DriveType_HiddenData) )
		{
			// This writes down the Janus header.
			media::HiddenDataDriveFormat hdd((uint32_t)drive.SizeInBytes/sectorSize);

			// put the data in our write holder
			writeData.resize(sizeof(hdd));
			memcpy_s(&writeData[0], writeData.size(), (uint8_t*)&hdd, sizeof(hdd));
		}
		//
		// FORMAT DATA
		//
		else if ( drive.Flags & media::DriveFlag_Format )
		{
			// Get the size of the data drive
			/*
			StGetLogicalDriveInfo apiDriveInfo(drive.DriveNumber, DriveInfoSizeInSectors);
			rc = pMscDevice->SendCommand(hVolume, apiDriveInfo, NULL, nsInfo);
			if ( rc != ERROR_SUCCESS )
			{
				ATLTRACE(_T("!!!ERROR!!! (%d): %s VerifyNand() - Failed to get Sector size.\r\n"), rc, m_pPortMgrDlg->GetPanel());
				pMscDevice->Unlock(hVolume);
				return rc;
			}
			uint32_t driveSizeInSectors = (uint32_t)apiDriveInfo.GetSizeInSectors();
			*/
			StReadCapacity apiReadCapacity;
			rc = pMscDevice->SendCommand(hVolume, apiReadCapacity, NULL, nsInfo);
			if ( rc != ERROR_SUCCESS )
			{
				ATLTRACE(_T("!!!ERROR!!! (%d): %s VerifyNand() - Failed to get Sector size.\r\n"), rc, m_pPortMgrDlg->GetPanel());
				pMscDevice->Unlock(hVolume);
				return rc;
			}
			READ_CAPACITY_DATA ReadCaps = apiReadCapacity.GetCapacity();
			uint32_t driveSizeInSectors = ReadCaps.LogicalBlockAddress+1;

			// Get the FormatInfo
			StFormatInfo::FileSystemT fs = m_pOpInfo->UseFat32() ? StFormatInfo::FS_FAT32 : StFormatInfo::FS_DEFAULT;
			CString volumeLabel = m_pOpInfo->GetProfile()->UseVolumeLabel() ? m_pOpInfo->GetProfile()->GetVolumeLabel() : _T("\0\0\0\0\0\0\0\0\0\0\0");
			StFormatInfo formatParams(driveSizeInSectors, sectorSize, fs, volumeLabel);
			
			// Create the Format Image
			StFormatImage formatImage(formatParams);

			// Add files to image if specified
			if ( drive.Flags & media::DriveFlag_FileData )
			{
				rc = formatImage.AddFiles(drive.Name);
				if ( rc != ERROR_SUCCESS )
				{
					ATLTRACE(_T("!!!ERROR!!! (%d): %s VerifyNand() - Failed to add files to Format Image.\r\n"), rc, m_pPortMgrDlg->GetPanel());
					pMscDevice->Unlock(hVolume);
					return rc;
				}

			}

			// put the data in our write holder
			writeData.assign(formatImage._theImage.begin(), formatImage._theImage.end());
		}

		// If there isn't any data, then there is nothing to do for this drive. Go on to next drive.
		if ( writeData.empty() )
			continue;

		// Read back verify.
		CString taskMsg;
		taskMsg.Format(IDS_OPUPDATER_VERIFYING_FILE, drive.Description.c_str());				// "Verifying <filename> ..."
		m_pPortMgrDlg->UpdateUI(taskMsg, 0, (int)writeData.size(), 0);

		rc = pMscDevice->ReadDrive(hVolume, drive.DriveNumber, readData, writeData.size(), callback);
		if ( rc != ERROR_SUCCESS )
		{
			ATLTRACE(_T("!!!ERROR!!! (%d): %s VerifyNand() - Failed during ReadDrive().\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}

		if ( readData != writeData )
		{
			CFile readFile(_T("ReadData.bin"), CFile::modeCreate | CFile::modeWrite);
			readFile.Write(&readData[0], readData.size());
			CFile writeFile(_T("WriteData.bin"), CFile::modeCreate | CFile::modeWrite);
			writeFile.Write(&writeData[0], writeData.size());

			rc = ERROR_INVALID_DATA; 
			ATLTRACE(_T("!!!ERROR!!! (%d): %s VerifyNand() - Read data not equal to Write data.\r\n"), rc, m_pPortMgrDlg->GetPanel());
			pMscDevice->Unlock(hVolume);
			return rc;
		}

		// Turn off task progress
		m_pPortMgrDlg->UpdateUI(NULL, ProgressDelta(m_iPercentComplete));
	}

	pMscDevice->Unlock( hVolume );

	return rc;
}