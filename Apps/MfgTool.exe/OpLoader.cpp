// OpUpdater.cpp : implementation file
//

#include "StdAfx.h"
#include "StMfgTool.h"
#include "PortMgrDlg.h"
#include "OpLoader.h"

#include "../../Libs/DevSupport/StPitc.h"
//#include "../../Libs/DevSupport/MtpDevice.h"
#include "../../Libs/DevSupport/RecoveryDevice.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(COpLoader, COperation)

COpLoader::COpLoader(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo* pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo, LOADER_OP)
{
	ASSERT(m_pOpInfo);
	ASSERT(m_pUSBPort);

	m_pProfile = m_pOpInfo->GetProfile();

/*
	find_device            ____
	reset device           X___
	find HID device        XX__
	load                   XXX_
	found MTP device       XXXX
*/
	m_iDuration = 4;

	m_sDescription.LoadString(IDS_OPLOADER_LOADING);// = _T("Loading...");
	m_sVersion = GetProjectVersion();
	m_dwStartTime = 0;

}

COpLoader::~COpLoader(void)
{
}

BOOL COpLoader::InitInstance(void)
{
	m_OpState = WAITING_FOR_DEVICE;
	return true;
}

BEGIN_MESSAGE_MAP(COpLoader, COperation)
	ON_THREAD_MESSAGE(WM_MSG_OPEVENT, OnMsgStateChange)
END_MESSAGE_MAP()

void COpLoader::OnMsgStateChange(WPARAM nEventType, LPARAM dwData)
{
	CString LogStr, taskMsg;
	DWORD error = ERROR_SUCCESS;


    if ( nEventType == OPEVENT_KILL ) 
	{
		ATLTRACE(_T("%s Updater Event: %s \r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType));
		m_bStart = false;
//		PostQuitMessage(0);
        AfxEndThread(0);
		return;
    }

	ATLTRACE(_T("%s Loader Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()), GetOpStateString(m_OpState));

    // Just return if we are in a BAD state and we get something other than START or STOP
	if((nEventType != OPEVENT_START) && (nEventType != OPEVENT_STOP) && (m_OpState == OP_INVALID))
	{
		return;
	}

	switch(nEventType)
	{
		case OPEVENT_START:
		{
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

				case HID_MODE:
				{
					if ( !m_Timer && m_TimeOutSeconds )
						m_Timer = m_pPortMgrDlg->SetTimer(LOADER_OP, m_TimeOutSeconds * 1000, NULL);

					if( m_dwStartTime == 0 )
						m_dwStartTime = GetTickCount();

					error = LoadHidDevice();
					
					if ( error == OPSTATUS_SUCCESS ) {
						m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
						m_dwStartTime = 0;
					}

					HandleError(error, NULL, WAITING_FOR_MTP_MODE);

					break;
				}

				case RECOVERY_MODE:
				{
					if ( !m_Timer && m_TimeOutSeconds )
						m_Timer = m_pPortMgrDlg->SetTimer(LOADER_OP, m_TimeOutSeconds * 1000, NULL);

					if( m_dwStartTime == 0 )
						m_dwStartTime = GetTickCount();

					error = LoadRecoveryDevice();
					
					if ( error == OPSTATUS_SUCCESS ) {
						m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
						m_dwStartTime = 0;
					}

					HandleError(error, NULL, WAITING_FOR_MTP_MODE);

					break;
				}

				case MSC_MODE:
				case MTP_MODE:
				{
					error = ResetToRecovery();

					HandleError(error, NULL, WAITING_FOR_HID_MODE);

					break;
				}
				case UPDATER_MODE:
				case MFG_MSC_MODE:
				case CONNECTED_UNKNOWN:
				{
					taskMsg.LoadString(IDS_OPLOADER_INVALID_DEVICE); // "Loader Error: Invalid device to load."

					HandleError(ERROR_INVALID_DRIVE, taskMsg, OP_INVALID);

					break;
				}
				default:
				{
					taskMsg.Format(IDS_OPLOADER_INVALID_STATE, _T("START")); // "Updater Error: Invalid State (START)."

					HandleError(ERROR_INVALID_PARAMETER, taskMsg, OP_INVALID);

					break;
				}
			}			
			break;
		}
		case OPEVENT_STOP:
		{
			taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."

			HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);

			break;
		}

		case OPEVENT_TIMEOUT:
		{
			HandleError(WAIT_TIMEOUT, NULL, OP_INVALID);
			break;
		}

		case OPEVENT_DEVICE_ARRIVAL:

			switch(GetDeviceState())
			{		
				case HID_MODE:
				{
					if ( m_OpState == WAITING_FOR_HID_MODE ||
						 m_OpState == WAITING_FOR_DEVICE )
					{
						if ( !m_Timer && m_TimeOutSeconds )
							m_Timer = m_pPortMgrDlg->SetTimer(LOADER_OP, m_TimeOutSeconds * 1000, NULL);

						error = LoadHidDevice();
						
						if ( error == OPSTATUS_SUCCESS ) {
							m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
							m_dwStartTime = 0;
						}

						HandleError(ERROR_SUCCESS/*error*/, NULL, WAITING_FOR_MTP_MODE);
					}
					else
					{
						HandleError(ERROR_INVALID_STATE, NULL, OP_INVALID);
					}					

					break;
				}

				case MTP_MODE:
				{
					if ( m_OpState == WAITING_FOR_DEVICE )
					{
						error = ResetToRecovery();
						
						HandleError(error, NULL, WAITING_FOR_HID_MODE);
					}
					else if ( m_OpState == WAITING_FOR_MTP_MODE )
					{
						// SUCCESS:
						LogStr = MakeLogString();
						HandleError(ERROR_SUCCESS, NULL, OP_COMPLETE, LogStr);
					}
					else
					{
						HandleError(ERROR_INVALID_STATE, NULL, OP_INVALID);
					}
					break;
				}

				case RECOVERY_MODE:
				case UPDATER_MODE:
				case MFG_MSC_MODE:
				case MSC_MODE:
				case CONNECTED_UNKNOWN:					
				{
					taskMsg.LoadString(IDS_OPLOADER_INVALID_DEVICE); // "Error: Invalid device to load."

					HandleError(ERROR_INVALID_DRIVE, taskMsg, OP_INVALID);

					break;
				}
				case DISCONNECTED:
				default:
				{
					taskMsg.Format(IDS_OPLOADER_INVALID_STATE, _T("DEV_ARR_D")); // "Updater Error: Invalid State (DEV_ARR_D)."

					HandleError(ERROR_INVALID_PARAMETER, taskMsg, OP_INVALID);

					break;
				}
			}
			break;
		case OPEVENT_DEVICE_REMOVAL:
		case OPEVENT_VOLUME_ARRIVAL:
		case OPEVENT_VOLUME_REMOVAL:
		default:
//			m_OpState = OP_INVALID;
//			taskMsg.Format(IDS_OPLOADER_INVALID_MSG, _T("DEF"));// =	_T("Loader Error: Invalid Msg State.");
			break;
	}
}

DWORD COpLoader::LoadHidDevice()
{

	DWORD ReturnVal = ERROR_SUCCESS;
	CString taskMsg;

	taskMsg.LoadString(IDS_OPLOADER_LOAD_STARTED);
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

	HidDevice* pHidDevice = dynamic_cast<HidDevice*>(m_pUSBPort->_device);

	if ( pHidDevice == NULL )
	{
		ReturnVal = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	CString fileName = m_pOpInfo->GetPath() + _T('\\');
//	fileName.Append(m_pOpInfo->GetFileArray()[0]);
	fileName.Append(m_pOpInfo->m_FileList.GetFileNameAt(0));

	StPitc myNewFwCommandSupport(pHidDevice, (LPCTSTR)fileName); 
	if ( myNewFwCommandSupport.GetFwComponent().GetLastError() != ERROR_SUCCESS )
	{
		ReturnVal = myNewFwCommandSupport.GetFwComponent().GetLastError();
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No FW component. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Set up the Task progress bar and bump the Operation progress bar
	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, myNewFwCommandSupport.GetFwComponent().GetShortFileName().c_str());   // "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2), (int)myNewFwCommandSupport.GetFwComponent().size(), 0);       // STAGE 2 of the Load Operation

	Device::UI_Callback callback(this, &COpLoader::OnDownloadProgress);
	ReturnVal = myNewFwCommandSupport.DownloadPitc(callback);

	if(ReturnVal != ERROR_SUCCESS) 
	{
		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, ReturnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Turn off the Task progress bar and bump the Operation progress bar
	taskMsg = _T("Load complete. Waiting for MTP device...");
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(3), -1);

	ATLTRACE(_T("%s LoadHidDevice - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	return ERROR_SUCCESS;
}

DWORD COpLoader::LoadRecoveryDevice()
{
	DWORD error;
	CString taskMsg;
	Device::UI_Callback callback(this, &COpLoader::OnDownloadProgress);

	taskMsg.LoadString(IDS_OPUPDATER_LOADING_USBMSC);
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));
	
	RecoveryDevice* pRecoveryDevice = dynamic_cast<RecoveryDevice*>(m_pUSBPort->_device);
	
	if ( pRecoveryDevice == NULL )
	{
		error = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s RecoverDevice() - No device.\r\n"), error, m_pPortMgrDlg->GetPanel());
		return error;
	}

	CString recoveryFirmwareFilename = m_pOpInfo->GetPath() + _T('\\');
	recoveryFirmwareFilename.Append(m_pOpInfo->m_FileList.GetFileNameAt(0));

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

//// ResetToRecovery()
//
// ResetToRecovery() is STAGE 1 of the Load Operation
// Valid OpStates are WAITING_FOR_DEVICE, 
//
// changes member variable m_OpState
//
////
DWORD COpLoader::ResetToRecovery()
{
	DWORD error;
	CString taskMsg;

	// No Task progress bar but bump the Operation progress bar
	taskMsg.LoadString(IDS_OPUPDATER_RESETTING_DEVICE); // "Resetting device..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(1)); // STAGE 1 of the Load Operation

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


//// HandleError()
//
// Mangles member variables m_OpState, m_Timer, m_TimedOut, m_bStart, m_iPercentComplete, m_update_error_count
//
////
void COpLoader::HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr)
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
		m_OpState = OP_INVALID;
		m_bStart = false;

		if ( m_Timer && m_TimeOutSeconds )
		{
			m_pPortMgrDlg->KillTimer(m_Timer);
			m_Timer = 0;
		}

        CString errorMsgStr;
		if ( _errorMsg )
		{
			errorMsgStr = _errorMsg;
		}
		else
		{
			CString systemMsgStr;
			FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, _error, 0, systemMsgStr.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
			errorMsgStr.Format(IDS_OPUPDATER_UPDATE_ERROR, _error, _error, systemMsgStr);		// "Updater Error 0x%x (%d) - %s"
		}
		m_pPortMgrDlg->UpdateUI(errorMsgStr, ProgressDelta(0));

		// trace error details
		CString traceErrorStr;
		traceErrorStr.Format(_T("\t%s: %s\r\n"), m_pPortMgrDlg->GetPanel(), errorMsgStr);
		ATLTRACE(traceErrorStr);
		// log error details
		BSTR bstr_log_text = traceErrorStr.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

		m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_ERROR);
	}
}

CString COpLoader::GetProjectVersion()
{
	CString versionStr = _T("");
	
	int32_t fileIndex;
	for ( fileIndex = 0; fileIndex < m_pOpInfo->m_FileList.GetCount(); ++fileIndex )
	{
		CString fileName = m_pOpInfo->m_FileList.GetPathNameAt(fileIndex);
		StFwComponent tempFwObj(fileName, StFwComponent::LoadFlag_FileOnly);
		if ( tempFwObj.GetLastError() == ERROR_SUCCESS )
		{
			versionStr = tempFwObj.GetProductVersion().toString();
			break;
		}
	}

	return versionStr;
}

CString COpLoader::MakeLogString() const
{
	CString logStr = _T("");

	/*MtpDevice* pMtpDevice = dynamic_cast<MtpDevice*>(m_pUSBPort->_device);

	if ( pMtpDevice == NULL )
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s COpLoader::MakeLogString() - No MTP device.\r\n"), ERROR_INVALID_HANDLE, m_pPortMgrDlg->GetPanel());
		return logStr;
	}

	logStr.Format(_T("UPDATE ver(%s) %s\\%s %s : "), m_sVersion, m_pProfile->GetUsbVid(), m_pProfile->GetUsbPid(), GetDeviceSerialNumber(pMtpDevice));*/

	return logStr;
}

CString COpLoader::GetOpStateString(OpState_t _state) const
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
	case WAITING_FOR_HID_MODE:
		str = "WAITING_FOR_HID_MODE";
		break;
	case OP_LOADING:
		str = "OP_LOADING";
		break;
	case WAITING_FOR_MTP_MODE:
		str = "WAITING_FOR_MTP_MODE";
		break;
	case OP_COMPLETE:
		str = "OP_COMPLETE";
		break;
	default:
		str = "OP_UNKNOWN_STATE";
		break;
	}
	return str;
}

void COpLoader::OnDownloadProgress(const Device::NotifyStruct& nsInfo)
{
	if (nsInfo.position)
	{
		m_pPortMgrDlg->UpdateUI(NULL, 0, 0, nsInfo.position);
	}
}

