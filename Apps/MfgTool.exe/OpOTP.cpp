// OpOTP.cpp : implementation file
//

#include "StdAfx.h"
#include "StMfgTool.h"
#include "PortMgrDlg.h"
#include "stmsg.h"
#include "DefaultProfile.h"

#include "../../Libs/DevSupport/StPitc.h"
#include "../../Libs/DevSupport/Volume.h"
#include "../../Libs/DevSupport/StMtpApi.h"

#include "OpOTP.h"


extern HANDLE g_HIDMutex;
extern BOOL g_StopFlagged;  // need a global to reflect the STOP state and break out of request for HID mutex

IMPLEMENT_DYNCREATE(COpOTP, COperation)

COpOTP::COpOTP(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo* pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo, UPDATE_OP)
{
	ASSERT(m_pOpInfo);
	ASSERT(m_pUSBPort);

	m_iDuration = 2;

	m_pProfile = m_pOpInfo->GetProfile();
	m_sDescription.LoadString(IDS_OPUPDATER_UPDATING);

}

COpOTP::~COpOTP(void)
{
}

BOOL COpOTP::InitInstance(void)
{
	m_OpState = WAITING_FOR_DEVICE;

	m_bOwnMutex = FALSE;
	g_StopFlagged = FALSE;

	return true;
}

BEGIN_MESSAGE_MAP(COpOTP, COperation)
	ON_THREAD_MESSAGE(WM_MSG_OPEVENT, OnMsgStateChange)
END_MESSAGE_MAP()

void COpOTP::OnMsgStateChange(WPARAM nEventType, LPARAM dwData)
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
			ATLTRACE(_T("Start OTP Operation - %s\r\n"), m_pPortMgrDlg->GetPanel());

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
				case UPDATER_MODE:
				case MFG_MSC_MODE:
				{
					g_StopFlagged = FALSE;

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
				case HID_MODE:
				{
					g_StopFlagged = FALSE;

					if (!m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s HID mode request (START)\r\n"), m_pPortMgrDlg->GetPanel());
						GetResetLockMutex(FALSE);
					}

					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s HID mode has mutex hid recover (START)\r\n"), m_pPortMgrDlg->GetPanel());
						error = UpdateSettingsField();
				
						HandleError(error, NULL, OP_COMPLETE);
					}
					break;
				}				
				case CONNECTED_UNKNOWN:
				{
					taskMsg.LoadString(IDS_OPUPDATER_INVALID_DEVICE); // "Updater Error: Invalid device to update."

					HandleError(ERROR_INVALID_DRIVE, taskMsg, OP_INVALID);

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
						if (!m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s HID mode device arrival request\r\n"), m_pPortMgrDlg->GetPanel());
							GetResetLockMutex(FALSE);
						}

						if (m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s has mutex\r\n"), m_pPortMgrDlg->GetPanel());

							error = UpdateSettingsField();
				
							HandleError(error, NULL, OP_COMPLETE);
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
//			ATLTRACE(_T("OpOTP::OnMsgStateChange() - VOLUME_ARRIVAL_EVT(%s)\n"), msg.c_str());
//			if(m_OpState == OP_COMPLETE || m_OpState == OP_RESETTING) // got drive msg late
//			break;

			switch( GetDeviceState() )
			{	
				case UPDATER_MODE:
				case MFG_MSC_MODE:
				case MSC_MODE:
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
					{
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

					HandleError(ERROR_INVALID_DRIVE, taskMsg, OP_INVALID);

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
void COpOTP::HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr)
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
		}
	}
	else
	{
		m_bStart = false;


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
			errorMsgStr.Format(IDS_OPOTP_OP_FAILED, _error, _error, systemMsgStr.c_str());		// "Updater Error 0x%x (%d) - %s"
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
DWORD COpOTP::ResetToRecovery()
{
	DWORD error;
	CString taskMsg;

	// No Task progress bar but bump the Operation progress bar
	taskMsg.LoadString(IDS_OPUPDATER_RESETTING_DEVICE); // "Resetting device..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(1)); // STAGE 1 of the Update Operation

	error = m_pUSBPort->_device->ResetToRecovery();

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


void COpOTP::OnDownloadProgress(const Device::NotifyStruct& nsInfo)
{
	if (nsInfo.position)
	{
		m_pPortMgrDlg->UpdateUI(NULL, 0, 0, nsInfo.position);
	}
}

DWORD COpOTP::UpdateSettingsField()
{
	uint8_t fieldNum;
	uint8_t otpFieldValue = 0;
	int settingValue;
	DWORD result = ERROR_SUCCESS;

	HidDevice* pHidDevice = dynamic_cast<HidDevice*>(m_pUSBPort->_device);
	CString taskMsg;

	// No Task progress bar but bump the Operation progress bar
	taskMsg.LoadString(IDS_OPOTP_WRITING_REG); 

	_stscanf_s ( m_pOpInfo->m_csOTPValue, _T("%2x"), &settingValue, sizeof(int) );

	StOtpAccessPitc *pOtpPitc = new StOtpAccessPitc(pHidDevice);
	result = pOtpPitc->GetFwComponent().GetLastError();
	if ( result != ERROR_SUCCESS )
		return result;

	Device::UI_Callback callback(this, &COpOTP::OnDownloadProgress);
	result = pOtpPitc->DownloadPitc(callback);
	if ( result != ERROR_SUCCESS )
		goto exit;

	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(1)); 
	// step through the 4 registers 8 bits at time to find the first open field
	for ( fieldNum = 0; fieldNum < 16; ++fieldNum )
	{
		result = ReadCustomerOtpField(pOtpPitc, fieldNum, otpFieldValue);
		if ( result != ERROR_SUCCESS )
			goto exit;
		
		if ( otpFieldValue == 0 )
		{
			// if the field is blank, break out and write the value to the field.
		    break;
		}
		else if ( otpFieldValue == settingValue )
		{
			// we’re done if the field already contains the right value
			result = ERROR_SUCCESS;
			goto exit;
		}
		else
		{
			// otherwise, invalidate the field and check the next one.
			if (otpFieldValue != 0xFF)
			{
				result = WriteCustomerOtpField(pOtpPitc, fieldNum, 0xFF);
				if ( result != ERROR_SUCCESS )
					goto exit;
			}
		}
	}

	if ( fieldNum < 16 )
	{
	    result = WriteCustomerOtpField(pOtpPitc, fieldNum, (uint8_t)settingValue);
	}
	else
	{
		result = -1;  // No unused fields
	}

exit:
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2)); 

	if (pOtpPitc)
		delete pOtpPitc;

	return result;
}

DWORD COpOTP::ReadCustomerOtpField(StOtpAccessPitc * const pOtpPitc, const uint8_t _fieldNum, uint8_t& _fieldValue)
{
	uint8_t reg = _fieldNum / 4;
	uint32_t regValue;

	uint32_t err = pOtpPitc->OtpRegisterRead(reg, &regValue);

	if ( err == ERROR_SUCCESS )
		_fieldValue = (uint8_t)((regValue >> ((_fieldNum % 4) * 8)) & 0xFF);
	
	return err;
}

DWORD COpOTP::WriteCustomerOtpField(StOtpAccessPitc * const pOtpPitc, const uint8_t _fieldNum, uint8_t _fieldValue)
{
	uint8_t reg = _fieldNum / 4;
	uint32_t regValue;

	uint32_t err = pOtpPitc->OtpRegisterRead(reg, &regValue);
	if ( err != ERROR_SUCCESS )
		return err;

	regValue |= _fieldValue << ((_fieldNum % 4) * 8);

	return pOtpPitc->OtpRegisterWrite(reg, regValue, false); // don't lock the register
}

CString COpOTP::GetOpStateString(OpState_t _state)
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

BOOL COpOTP::GetResetLockMutex(BOOL _bOpComplete)
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

BOOL COpOTP::ReleaseResetLockMutex()
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