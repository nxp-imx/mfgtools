/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// OpUtpUpdate.cpp : implementation file
//

#include "StdAfx.h"
#include "StMfgTool.h"
#include "PortMgrDlg.h"
#include "stmsg.h"
//#include "DefaultProfile.h"

#include "../../Libs/DevSupport/StPitc.h"
#include "../../Libs/DevSupport/RecoveryDevice.h"
#include "../../Libs/DevSupport/StFormatImage.h"
#include "../../Libs/DevSupport/StHidApi.h"
#include "../../Libs/DevSupport/MxRomDevice.h"
#include "../../Libs/DevSupport/MxHidDevice.h"

#include "OpUtpUpdate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// #define DEBUGOTPINIT 1

#import <msxml3.dll> 
using namespace MSXML2;

extern BOOL g_TestLoop;
extern HANDLE g_HIDMutex;
BOOL g_StopFlagged;  // need a global to reflect the STOP state and break out of request for HID mutex

IMPLEMENT_DYNCREATE(COpUtpUpdate, COperation)

COpUtpUpdate::COpUtpUpdate(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo* pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo, UPDATE_OP)
, m_pCmdList(NULL)
, m_p_do_list_thread(NULL)
, m_bProcessingList(FALSE)
, m_pUTP(NULL)
, m_hChangeEvent(INVALID_HANDLE_VALUE)
, m_CurrentDeviceState(UCL::DeviceState::Unknown)
{
	ASSERT(m_pOpInfo);
	ASSERT(m_pUSBPort);

	USES_CONVERSION;

	m_pProfile = m_pOpInfo->GetProfile();

/*
	reset
	recover
	erase
	allocate
	get_sizes
	write/verify drives * num_drives
*/
//	m_iDuration = 5 + (int)m_pOpInfo->GetDriveArray().Size();
/*
	init janus
	init store
*/
//	if (m_pOpInfo->GetDriveArray().GetNumDrivesByType(media::DriveType_HiddenData) &&
//		m_pOpInfo->IsWinCE() == FALSE )
//		m_iDuration += 2;

	m_sDescription.LoadString(IDS_OPUPDATER_UPDATING);
//	m_sVersion = GetProjectVersion();

	CFile commandFile;
	CFileException fileException;
	CString fullFileName = m_pOpInfo->GetPath() + _T("\\ucl.xml");

	if( !commandFile.Open(fullFileName, CFile::modeRead, &fileException) )
	{
		TRACE( _T("Can't open file %s, error = %u\n"), fullFileName, fileException.m_cause );
	}

	CStringT<char,StrTraitMFC<char> > uclString;
	commandFile.Read(uclString.GetBufferSetLength((int)commandFile.GetLength()), (unsigned int)commandFile.GetLength());
	uclString.ReleaseBuffer();

	if ( m_UclNode.Load(A2T(uclString)) != NULL )
	{
		m_pCmdList = m_UclNode.GetCommandList(m_pOpInfo->GetUclInstallSection());
		if ( m_pCmdList )
		{
			m_sDescription = m_pCmdList->GetDescription();
			m_iDuration = m_pCmdList->GetCommandCount() * 2; // bump overall progress before and after every command
		}
		m_DeviceStates = m_UclNode.GetConfiguration()->GetDeviceStates();
	}

	m_habStatus = NoHabCheck;
}

COpUtpUpdate::~COpUtpUpdate(void)
{
	if (m_pUTP != NULL)
	{
		delete m_pUTP;
		m_pUTP = NULL;
	}
    
	if ( m_hChangeEvent != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_hChangeEvent);
		m_hChangeEvent = INVALID_HANDLE_VALUE;
	}
}

BOOL COpUtpUpdate::InitInstance(void)
{
	m_OpState = WAITING_FOR_DEVICE;
	m_dwStartTime = 0;

	m_hChangeEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	g_StopFlagged = FALSE;

	return true;
}

const UCL::DeviceState::DeviceState_t COpUtpUpdate::GetDeviceState()
{
	CString csVidPid, devPath;
	UCL::DeviceState::DeviceState_t state = UCL::DeviceState::Unknown;

	switch ( (DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() )
	{
		case DeviceClass::DeviceTypeHid:
		case DeviceClass::DeviceTypeMxHid:
		case DeviceClass::DeviceTypeRecovery:
		{
			devPath = m_pUSBPort->GetUsbDevicePath(); devPath.MakeUpper();

			if ( m_DeviceStates[UCL::DeviceState::Recovery] == NULL )
			{
				state = UCL::DeviceState::ConnectedUnknown;
				break;
			}

			csVidPid.Format(_T("Vid_%s&Pid_%s"), m_DeviceStates[UCL::DeviceState::Recovery]->GetVid(), m_DeviceStates[UCL::DeviceState::Recovery]->GetPid());
			csVidPid.MakeUpper();

			if ( devPath.Find(csVidPid) != -1 )
				state = UCL::DeviceState::Recovery;
			else
				state = UCL::DeviceState::ConnectedUnknown;
			
			break;
		}
		case DeviceClass::DeviceTypeMxRom:
		{
			devPath = m_pUSBPort->GetUsbDevicePath(); devPath.MakeUpper();

			if ( m_DeviceStates[UCL::DeviceState::BootStrap] == NULL )
			{
				state = UCL::DeviceState::ConnectedUnknown;
				break;
			}
			
			csVidPid.Format(_T("Vid_%s&Pid_%s"), m_DeviceStates[UCL::DeviceState::BootStrap]->GetVid(), m_DeviceStates[UCL::DeviceState::BootStrap]->GetPid());
			csVidPid.MakeUpper();

			if ( devPath.Find(csVidPid) != -1 )
			{
				MxRomDevice* pMxRomDevice = dynamic_cast<MxRomDevice*>(m_pUSBPort->_device);
			
				int len = 0, type = 0;
				CString model = _T("");
				if ( pMxRomDevice->GetRKLVersion(model, len, type) == ERROR_SUCCESS )
				{
					if ( len == 0 && type == 0 )
						state = UCL::DeviceState::BootStrap;
					else
						state = UCL::DeviceState::RamKernel;
				}
			}
			else
			{
				state = UCL::DeviceState::ConnectedUnknown;
			}
			break;
		}
		case DeviceClass::DeviceTypeMsc:
		{
			CString devPath = m_pUSBPort->GetUsbDevicePath();
			devPath.MakeUpper();

			CString csVidPid;
			// Check for USER MSC mode
			if ( m_DeviceStates.find(UCL::DeviceState::UserMsc) != m_DeviceStates.end() && 
				 m_DeviceStates[UCL::DeviceState::UserMsc] != NULL)
			{
				csVidPid.Format(_T("Vid_%s&Pid_%s"), m_DeviceStates[UCL::DeviceState::UserMsc]->GetVid(), m_DeviceStates[UCL::DeviceState::UserMsc]->GetPid());
				csVidPid.MakeUpper();

				if ( devPath.Find(csVidPid) != -1 )
				{
					state = UCL::DeviceState::UserMsc;
				}
			}
			if ( state != UCL::DeviceState::UserMsc )
			{
				// Check for UPDATER mode
				if ( m_DeviceStates.find(UCL::DeviceState::Updater) != m_DeviceStates.end() &&
					 m_DeviceStates[UCL::DeviceState::Updater] != NULL )
				{
					csVidPid.Format(_T("Vid_%s&Pid_%s"), m_DeviceStates[UCL::DeviceState::Updater]->GetVid(), m_DeviceStates[UCL::DeviceState::Updater]->GetPid());
					csVidPid.MakeUpper();

					if ( devPath.Find(csVidPid) != -1 )
					{
						state = UCL::DeviceState::Updater;
					}
				}
			}
			if ( state != UCL::DeviceState::UserMsc && state != UCL::DeviceState::Updater )
			{
				state = UCL::DeviceState::ConnectedUnknown;
			}
			break;
		}
		/*case DeviceClass::DeviceTypeMtp:
		{
			devPath = m_pUSBPort->GetUsbDevicePath(); devPath.MakeUpper();

			if ( m_DeviceStates[UCL::DeviceState::UserMtp] == NULL )
			{
				state = UCL::DeviceState::ConnectedUnknown;
				break;
			}

			csVidPid.Format(_T("Vid_%s&Pid_%s"), m_DeviceStates[UCL::DeviceState::UserMtp]->GetVid(), m_DeviceStates[UCL::DeviceState::UserMtp]->GetPid());
			csVidPid.MakeUpper();

			if ( devPath.Find(csVidPid) != -1 )
			{
				state = UCL::DeviceState::UserMtp;
			}
			else
			{
				state = UCL::DeviceState::ConnectedUnknown;
			}

			break;
		}*/
		case DeviceClass::DeviceTypeNone:
			state = UCL::DeviceState::Disconnected;
			break;
		default:
			state = UCL::DeviceState::ConnectedUnknown;
			break;
	}

	return state;
}

BEGIN_MESSAGE_MAP(COpUtpUpdate, COperation)
	ON_THREAD_MESSAGE(WM_MSG_OPEVENT, OnMsgStateChange)
END_MESSAGE_MAP()

void COpUtpUpdate::OnMsgStateChange(WPARAM nEventType, LPARAM dwData)
{
	CString LogStr, taskMsg;
	DWORD error = ERROR_SUCCESS;


	if ( nEventType == OPEVENT_KILL ) 
	{
		ATLTRACE(_T("%s UtpUpdate Event: %s \r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType));
		m_bStart = false;
		PostQuitMessage(0);
//        AfxEndThread(0);
		return;
    }

//	ATLTRACE(_T("%s Updater Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()), GetOpStateString(m_OpState));

    // Just return if we are in a BAD state and we get something other than START or STOP
    if((nEventType != OPEVENT_START) && (nEventType != OPEVENT_STOP) && (m_OpState == OP_INVALID))
	{
		ATLTRACE(_T("%s UtpUpdate Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, UCL::DeviceState::DeviceStateToString(m_CurrentDeviceState), GetOpStateString(m_OpState));
		ATLTRACE(_T("Just return if we are in a BAD state and we get something other than START or STOP\r\n"));
		VERIFY(::SetEvent(m_hChangeEvent));
		return;
	}

	switch(nEventType)
	{
		case OPEVENT_START:
		{

			int iTPriority = GetThreadPriority();
			int iPPriority = GetPriorityClass(GetCurrentProcess());
			ATLTRACE(_T("%s Process-Thread priority: 0x%04x-%d \r\n"),m_pPortMgrDlg->GetPanel(), iPPriority, iTPriority);

			m_bStart = true;
			m_iPercentComplete = 0;

			m_CurrentDeviceState = GetDeviceState();
			
			if ( m_CurrentDeviceState == UCL::DeviceState::Updater )
				m_pUTP = new UpdateTransportProtocol((Volume*)m_pUSBPort->_device);

			if ( m_CurrentDeviceState != UCL::DeviceState::Disconnected && m_CurrentDeviceState != UCL::DeviceState::ConnectedUnknown)
			{
				// Start the thread to process the list
				m_p_do_list_thread = AfxBeginThread( DoListThreadProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED );
				m_p_do_list_thread->ResumeThread();
			}
			else if (m_CurrentDeviceState == UCL::DeviceState::Disconnected)
			{
				m_pPortMgrDlg->UpdateUI(_T("Please connect device."));
				m_OpState = WAITING_FOR_DEVICE;
			}			
			else if (m_CurrentDeviceState == UCL::DeviceState::ConnectedUnknown)
				m_pPortMgrDlg->UpdateUI(_T("Unrecognized device."));

			break;
		}

		case OPEVENT_STOP:
		{
			g_StopFlagged = TRUE;
			/*if ( m_OpState == WAITING_FOR_DEVICE || m_OpState == OP_COMPLETE ||
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
			}*/

			if (m_OpState != OP_COMPLETE)
			{
				taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
				HandleError(ERROR_PROCESS_ABORTED, taskMsg, OP_INVALID);
			}

			m_OpState = OP_COMPLETE;	
			ATLTRACE(_T("STOPPING - %s \r\n"), m_pPortMgrDlg->GetPanel());
			break;
		}

		case OPEVENT_DEVICE_ARRIVAL:
		case OPEVENT_DEVICE_REMOVAL:
		case OPEVENT_VOLUME_ARRIVAL:
		case OPEVENT_VOLUME_REMOVAL:
		{
//			ATLTRACE(_T("%s UtpUpdate Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()), GetOpStateString(m_OpState));
//			Debug.WriteLine(String.Format("*** MainWindow.DeviceManager_DeviceChanged(): {0}: {1}, {2}({3})", e.Event, e.DeviceId, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

//            OutputWindow.Text += "<" + e.Event + ": " + e.DeviceId + ">\r\n";

            UCL::DeviceState::DeviceState_t oldState = m_CurrentDeviceState;
			m_CurrentDeviceState = GetDeviceState();
			ATLTRACE(_T("%s UtpUpdate Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, UCL::DeviceState::DeviceStateToString(m_CurrentDeviceState), GetOpStateString(m_OpState));

			if (oldState == UCL::DeviceState::Disconnected)
            {
                switch (nEventType)
                {
                    case OPEVENT_DEVICE_ARRIVAL:
                        break;

                    case OPEVENT_VOLUME_ARRIVAL:
						if ( m_CurrentDeviceState == UCL::DeviceState::Updater )
						{
							if (m_pUTP != NULL)
							{
								delete m_pUTP;
								m_pUTP = NULL;
							}

							m_pUTP = new UpdateTransportProtocol((Volume*)m_pUSBPort->_device);
						}
						break;
                }

				if ( m_bProcessingList == FALSE && 
				   ( m_CurrentDeviceState != UCL::DeviceState::Disconnected ||
				     m_CurrentDeviceState != UCL::DeviceState::ConnectedUnknown ) )
				{
					// Start the thread to process the list
					m_p_do_list_thread = AfxBeginThread( DoListThreadProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED );
					m_p_do_list_thread->ResumeThread();
				}
				else
				{
					if (m_CurrentDeviceState == UCL::DeviceState::Disconnected)
						m_pPortMgrDlg->UpdateUI(_T("Please connect device."));
					else if (m_CurrentDeviceState == UCL::DeviceState::ConnectedUnknown)
						m_pPortMgrDlg->UpdateUI(_T("Unrecognized device."));
				}
//                if (CurrentDevice != null)
//                    CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);
            }
            else
            {
                if (nEventType == OPEVENT_DEVICE_REMOVAL || nEventType == OPEVENT_VOLUME_REMOVAL)
                {
///                    if (m_pCurrentDevice != NULL)
///                    {
///                        CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;
///                        CurrentDevice.Dispose();
///                        CurrentDevice = null;
///                    }
                    if (/*CurrentUpdateAction == UpdateAction.Working &&*/ m_pUTP != NULL) // ugly
                    {
                        delete m_pUTP;
                        m_pUTP = NULL;
                    }

                    m_CurrentDeviceState = UCL::DeviceState::Disconnected;
///                    DeviceDescStatusLabel.Text = DeviceState.Disconnected.ToString();

///                    if (CurrentUpdateAction == UpdateAction.Ready)
///                        CurrentUpdateAction = UpdateAction.NotStarted;
/*
                    if (CurrentUpdateAction == UpdateAction.Ready)
                    {
                        CurrentUpdateAction = UpdateAction.NotStarted;
                        UpdateStatus();
                    }
*/
                }
            }

            if (oldState != m_CurrentDeviceState)
			{
				// Signal anyone waiting on a device change to go see what changed.
				if ( m_hChangeEvent != INVALID_HANDLE_VALUE )
				{
					ATLTRACE(_T("%s UtpUpdate Event: %s Msg: %s DevState: %s OpState: %s SET_EVENT\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, UCL::DeviceState::DeviceStateToString(m_CurrentDeviceState), GetOpStateString(m_OpState));
					VERIFY(::SetEvent(m_hChangeEvent));
				}
			}
			else
			{
				ATLTRACE(_T("%s UtpUpdate Event: %s Msg: %s DevState: %s OpState: %s NO SET_EVENT!!!\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, UCL::DeviceState::DeviceStateToString(m_CurrentDeviceState), GetOpStateString(m_OpState));
			}
///            UpdateStatus();
            
			break;
		}
		
		default:
		{
			taskMsg.Format(IDS_OPUPDATER_INVALID_MSG, _T("DEF")); // "Updater Error: Invalid State (DEF)."

			HandleError(ERROR_INVALID_PARAMETER, taskMsg, OP_INVALID);

			break;
		}
	}
}

BOOL COpUtpUpdate::WaitForDeviceChange(int seconds)
{
 	BOOL retValue = FALSE;

	// Create an event that we can wait on so we will know if
    // the DeviceManager got a device arrival
//    if ( m_hChangeEvent == INVALID_HANDLE_VALUE ) 
//		m_hChangeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    HANDLE hTimer = CreateWaitableTimer(NULL, true, _T("DeviceChangeTimer"));
    LARGE_INTEGER waitTime;
    waitTime.QuadPart = seconds * (-10000000);
    SetWaitableTimer(hTimer, &waitTime, 0, NULL, NULL, false);

    HANDLE waitHandles[2] = { m_hChangeEvent, hTimer };

	DWORD waitResult = MsgWaitForMultipleObjects(2, &waitHandles[0], false, INFINITE, QS_ALLINPUT);
    if ( waitResult == WAIT_OBJECT_0 )
    {
        // Device Change Event
       	VERIFY(::ResetEvent(m_hChangeEvent));
        retValue = TRUE;
    }
    else if ( waitResult == WAIT_OBJECT_0 + 1 )
    {
        // Timeout
        retValue = FALSE;
    }
    else if ( waitResult == WAIT_OBJECT_0 + 2 )
    {
        // got a message that we need to handle while we are waiting.
        MSG msg ; 
        // Read all of the messages in this next loop, 
        // removing each message as we read it.
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
        { 
        ATLTRACE2(_T("   COpUtpUpdate::WaitForDeviceChange() - Got a message(%0x).\r\n"), msg.message);
            // If it is a quit message, exit.
///            if (msg.message == WM_QUIT)  
///            {
///                done = true;
/////                            break;
///            }
            // Otherwise, dispatch the message.
            DispatchMessage(&msg); 
        } // End of PeekMessage while loop.
        //retValue = FALSE;
		retValue = TRUE;
    }
    else
    {
        // unreachable, but catch it just in case.
        retValue = FALSE;
		ATLTRACE2(_T("   COpUtpUpdate::WaitForDeviceChange() Invalid waitReturn: %d.\r\n"), waitResult);
    }

	// clean up
//    CloseHandle(m_hChangeEvent);
//    m_hChangeEvent = INVALID_HANDLE_VALUE;

	return retValue;
}

//// HandleError()
//
// Mangles member variables m_OpState, m_Timer, m_TimedOut, m_bStart, m_iPercentComplete, m_update_error_count
//
////
void COpUtpUpdate::HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr)
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
/*
DWORD COpUtpUpdate::ResetToRecovery()
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
*/
//// RecoverDevice()
//
// RecoverDevice() is STAGE 2 of the Update Operation
// Valid OpStates are WAITING_FOR_RECOVERY_MODE and WAITING_FOR_DEVICE
//
// changes member variable m_OpState
//
////
/*
DWORD COpUtpUpdate::RecoverDevice()
{
	DWORD error;
	CString taskMsg;
	Device::UI_Callback callback(this, &COpUtpUpdate::OnDownloadProgress);

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
*/
//// RecoverhidDevice()
//
// RecoverHidDevice() is STAGE 2 of the Update Operation
// Valid OpStates are WAITING_FOR_RECOVERY_MODE and WAITING_FOR_DEVICE
//
// changes member variable m_OpState
//
////
/*
DWORD COpUtpUpdate::DoMxRomLoad(UCL::Command* pCmd)
{

	DWORD returnVal = ERROR_SUCCESS;
	CString taskMsg;

///	taskMsg.LoadString(IDS_OPLOADER_LOAD_STARTED);
///	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

	MxRomDevice* pMxRomDevice = dynamic_cast<MxRomDevice*>(m_pUSBPort->_device);
//	TRACE(_T("%s"),m_pUSBPort->_device->name.c_str());
	if ( pMxRomDevice == NULL )
	{
		returnVal = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No MxRom device. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return returnVal;
	}

	CString fileName = m_pOpInfo->GetPath() + _T("\\") + pCmd->GetFile();
	StFwComponent fwObject((LPCTSTR)fileName);
	if ( fwObject.GetLastError() != ERROR_SUCCESS )
	{
		returnVal = fwObject.GetLastError();
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No FW component. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return returnVal;
	}

	// Set up the Task progress bar and bump the Operation progress bar
///	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, myNewFwCommandSupport.GetFwComponent().GetShortFileName().c_str());   // "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(NULL, m_iPercentComplete, fwObject.size(), 0);       // STAGE 2 of the Load Operation
	Device::UI_Callback callback(this, &COpUtpUpdate::OnDownloadProgress);

	returnVal = pMxRomDevice->DownloadImage(pCmd->GetAddress(), MxRomDevice::MemSectionOTH, MxRomDevice::MemSectionAPP, fwObject, callback);

	if(returnVal != ERROR_SUCCESS) 
	{
		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, returnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load i.MXxx device. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return returnVal;
	}

	// Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);

	ATLTRACE(_T("%s Recover i.MXDevice - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	return ERROR_SUCCESS;
}
*/
DWORD COpUtpUpdate::DoMxHidLoad(UCL::Command* pCmd)
{
    DWORD ReturnVal = ERROR_SUCCESS;
	CString taskMsg;

	CString filename = m_pOpInfo->GetPath() + _T("\\") + pCmd->GetFile();

	MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(m_pUSBPort->_device);
	TRACE(("%s\r\n"),typeid(m_pUSBPort->_device).name());

	if ( pMxHidDevice == NULL )
	{
		ReturnVal = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No MxHID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	//load firmware to _data:Firmware.nb0
	//MxFwComponent fwComponent(filename);
	StFwComponent fwComponent((LPCTSTR)filename);
	if ( fwComponent.GetLastError() != ERROR_SUCCESS )
	{
		ReturnVal = fwComponent.GetLastError();
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No FW component. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	m_pPortMgrDlg->UpdateUI(NULL, m_iPercentComplete, (int)fwComponent.size(), 0);       // STAGE 2 of the Load Operation
	Device::UI_Callback callback(this, &COpUtpUpdate::OnDownloadProgress);

	MxHidDevice::ImageParameter ImageParameter;
	ImageParameter.loadSection = MxHidDevice::StringToMemorySection(pCmd->GetLoadSection());
	ImageParameter.setSection = MxHidDevice::StringToMemorySection(pCmd->GetSetSection());
	ImageParameter.PhyRAMAddr4KRL = pCmd->GetAddress();
	//ImageParameter.HasFlashHeader = pCmd->HasFlashHeader();
	ImageParameter.CodeOffset = pCmd->GetCodeOffset();

    ReturnVal = pMxHidDevice->Download( &ImageParameter, &fwComponent, callback);

	if(ReturnVal != TRUE) 
	{
		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, ReturnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return !(ERROR_SUCCESS);
	}

    // Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);

	ATLTRACE(_T("%s RecoverHidDevice - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	return ERROR_SUCCESS;
}

DWORD COpUtpUpdate::CheckHabType(CString strHabType)
{
	HAB_type habValue = HabUnknown;

    if(memcmp(strHabType,_T(""),sizeof(_T("")))==0)
        return NoHabCheck;

	if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeHid)
	{
		HidDevice* pHidDevice = dynamic_cast<HidDevice*>(m_pUSBPort->_device);
		if ( pHidDevice )
		{
			if(pHidDevice->GetHabType() >= 1)
                habValue = HabEnable;
            else
                habValue = (HAB_type)pHidDevice->GetHabType();
            switch(habValue)
            {
                case HabDisable:
                    if (memcmp(strHabType,_T("HabDisable"),sizeof(_T("HabDisable")))==0)
                        m_habStatus = HabMatch;
                    else
                    {
                        if (m_habStatus == HabUnknownStatus)
                            m_habStatus = HabDismatch;
                        else if (m_habStatus == HabMatch)
                            m_habStatus = HabChecked;
						else
							m_habStatus = HabUnknownStatus;
                    }
                    break;
                case HabEnable:
                    if (memcmp(strHabType,_T("HabEnable"),sizeof(_T("HabEnable")))==0)
                        m_habStatus = HabMatch;
                    else
                    {
                        if (m_habStatus == HabUnknownStatus)
                            m_habStatus = HabDismatch;
                        else if (m_habStatus == HabMatch)
                            m_habStatus = HabChecked;
						else
							m_habStatus = HabUnknownStatus;
                    }
                    break;
                default:
                    m_habStatus = HabDismatch;
                    return m_habStatus;
            }
		}
		else
			m_habStatus = HabDismatch;
	}
	else if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxRom)
	{
		MxRomDevice* pMxRomDevice = dynamic_cast<MxRomDevice*>(m_pUSBPort->_device);
		if ( pMxRomDevice )
		{
			return NoHabCheck;
		}
	}
	else if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxHid)
	{
		MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(m_pUSBPort->_device);
		if ( pMxHidDevice )
		{
			return NoHabCheck;
		}
	}

	return m_habStatus;
}

DWORD COpUtpUpdate::DoHidLoad(UCL::Command* pCmd)
{

	DWORD ReturnVal = ERROR_SUCCESS;
	CString taskMsg;

	HidDevice* pHidDevice = dynamic_cast<HidDevice*>(m_pUSBPort->_device);
	TRACE(("%s\r\n"),typeid(m_pUSBPort->_device).name());

	//Check board hab type
    HAB_status habstatus = (HAB_status)CheckHabType(pCmd->GetIFCondition());
    switch(habstatus)
    {
        case HabChecked:
        case HabUnknownStatus:
            return ERROR_SUCCESS;
        case HabDismatch:
            return ERROR_INVALID_HANDLE;
        case HabMatch:
        default:
            break;
    }

	CString filename = m_pOpInfo->GetPath() + _T("\\") + pCmd->GetFile();

///	taskMsg.LoadString(IDS_OPLOADER_LOAD_STARTED);
///	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

	if ( pHidDevice == NULL )
	{
		ReturnVal = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	StPitc myNewFwCommandSupport(pHidDevice, (LPCTSTR)filename,
		m_pOpInfo->GetProfile()->m_bLockedProfile ? 
			StFwComponent::LoadFlag_ResourceOnly :
			StFwComponent::LoadFlag_FileFirst); 

	if ( myNewFwCommandSupport.GetFwComponent().GetLastError() != ERROR_SUCCESS )
	{
		ReturnVal = myNewFwCommandSupport.GetFwComponent().GetLastError();
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No FW component. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Set up the Task progress bar and bump the Operation progress bar
///	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, myNewFwCommandSupport.GetFwComponent().GetShortFileName().c_str());   // "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(NULL, m_iPercentComplete, (int)myNewFwCommandSupport.GetFwComponent().size(), 0);       // STAGE 2 of the Load Operation
	Device::UI_Callback callback(this, &COpUtpUpdate::OnDownloadProgress);
	ReturnVal = myNewFwCommandSupport.DownloadPitc(callback);

	if(ReturnVal != ERROR_SUCCESS) 
	{
		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, ReturnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);

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
/*
DWORD COpUtpUpdate::InitOtpRegs()
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

	Device::UI_Callback callback(this, &COpUtpUpdate::OnDownloadProgress);
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

	HidPitcWrite apiOtpInit(0,0,0,NULL,0); // address, length, lock, pData, sizeof(pData)
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
*/
CString COpUtpUpdate::GetProjectVersion()
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

void COpUtpUpdate::OnDownloadProgress(const Device::NotifyStruct& nsInfo)
{
	if (nsInfo.position)
	{
		m_pPortMgrDlg->UpdateUI(NULL, 0, nsInfo.maximum, nsInfo.position);
	}
}

CString COpUtpUpdate::GetOpStateString(OpState_t _state)
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

UINT DoListThreadProc( LPVOID pParam )
{
    UINT retValue = 0;

	COpUtpUpdate* pOperation = (COpUtpUpdate*)pParam;
	pOperation->m_bProcessingList = TRUE;
	UCL::Command* pCmd = NULL;
	pOperation->m_dwStartTime = GetTickCount();

	CString logText;
	BSTR bstr_log_text;

///    if (String.IsNullOrEmpty(list.Description))
///        StatusTextBox.Text = String.Empty;
///    else
///    {
///        StatusTextBox.Text = list.Description;
///        OutputWindow.Text += "(s) " + StatusTextBox.Text + "\r\n";
///    }
///    OutputWindow.Text += "LIST: " + list.Name + "\r\n";
///
///    // Set Overall Progress Bar - increment before and after command.
///    OverallProgressBar.Maximum = list.Commands.Length * 2;
///    OverallProgressBar.Value = 0;

	if ( pOperation->m_pCmdList == NULL )
	{
		CString msg; msg.Format(_T("No <CMD/>s. Can not find \"%s\" <LIST/> in ucl.xml file."), pOperation->m_pOpInfo->GetUclInstallSection());
		pOperation->HandleError(-65535, msg, COpUtpUpdate::OP_INVALID);
		return -65535;
	}

	logText.Format(_T("%s Start processing %s <LIST/>.\r\n"), pOperation->m_pPortMgrDlg->GetPanel(), pOperation->m_pOpInfo->GetUclInstallSection());
	bstr_log_text = logText.AllocSysString();
	((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

	for ( size_t i = 0; i < pOperation->m_pCmdList->GetCommandCount(); ++i )
	{
		pOperation->m_pPortMgrDlg->UpdateUI(NULL, pOperation->ProgressDelta(i*2+1));

		pCmd = pOperation->m_pCmdList->GetCommand(i);

		retValue = pOperation->DoCommand(pCmd);
        if (retValue != 0)
        {
			if (pCmd->GetOnError() == "ignore")
            {
                retValue = 0;
				pOperation->m_pPortMgrDlg->UpdateUI(NULL, pOperation->ProgressDelta(i*2+2));
                continue;
            }
            else
                break;
        }

		pOperation->m_pPortMgrDlg->UpdateUI(NULL, pOperation->ProgressDelta(i*2+2));
	}

	if( retValue == ERROR_SUCCESS )
	{
		CString taskMsg;

		pOperation->m_pPortMgrDlg->OpCompleteTick(GetTickCount() - pOperation->m_dwStartTime);
		pOperation->m_dwStartTime = 0;
		pOperation->m_OpState = COpUtpUpdate::OP_COMPLETE;

		ATLTRACE(_T("%s DoListThreadProc() - SUCCESS.\r\n"),pOperation->m_pPortMgrDlg->GetPanel());
		pOperation->HandleError(retValue, NULL, COpUtpUpdate::OP_COMPLETE);

		if (g_TestLoop && !g_StopFlagged)
		{
			Sleep(3500);
			if (!g_StopFlagged)
			{
				pOperation->m_bProcessingList = FALSE;
				UCL::Command cmd;
				cmd.Load(_T("<CMD type=\"drop\" body=\"!3\" timeout=\"0\">"));
				pOperation->DoDrop(&cmd);

				ATLTRACE(_T("%s Update complete reset to recovery ( TestLoop)\r\n"), pOperation->m_pPortMgrDlg->GetPanel());
			}
		}
	}
	else
		pOperation->HandleError(retValue, NULL, COpUtpUpdate::OP_INVALID);

	logText.Format(_T("%s Finished processing %s <LIST/> : %s code=%d.\r\n"), pOperation->m_pPortMgrDlg->GetPanel(), pOperation->m_pOpInfo->GetUclInstallSection(), retValue == ERROR_SUCCESS ? _T("SUCCESS") : _T("FAIL"), retValue);
	bstr_log_text = logText.AllocSysString();
	((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

	pOperation->m_bProcessingList = FALSE;
	return retValue;
}

DWORD COpUtpUpdate::DoCommand(UCL::Command* pCmd)
{
	DWORD retValue = ERROR_SUCCESS;
	Device::UI_Callback callback(this, &COpUtpUpdate::OnDownloadProgress);

	CString logText;
	BSTR bstr_log_text;

//    if (!String.IsNullOrEmpty(cmd.Description))
//    {
//        TaskTextBox.Text = cmd.Description;
//        OutputWindow.Text += " (t) " + TaskTextBox.Text + "\r\n";
//    }
//    OutputWindow.Text += " CMD: " + cmd.ToString() + "\r\n";
	LPCTSTR pStatus = pCmd->value.IsEmpty() ? NULL : (LPCTSTR)pCmd->value;
	m_pPortMgrDlg->UpdateUI(pStatus, ProgressDelta(m_iPercentComplete));

	logText.Format(_T("%s Start <CMD/> %s.\r\n"), m_pPortMgrDlg->GetPanel(), pCmd->ToString());
	bstr_log_text = logText.AllocSysString();
	((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

	ATLTRACE(_T("-----------------------------------------------------------------\r\n"));
    ATLTRACE(_T("%s Start <CMD/> %s.\r\n"), m_pPortMgrDlg->GetPanel(), pCmd->ToString());

	if ( pCmd->GetType() == _T("boot") )
	{
		// Reset device to ROM and load file.
        retValue = DoBoot(pCmd);
	}
	else if ( pCmd->GetType() == _T("init") )
	{
        retValue = DoInit(pCmd);
	}
	else if ( pCmd->GetType() == _T("load") )
	{
        if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxRom)
        {
		    retValue = DoMxRomLoad(pCmd);
        }
        else
            retValue = DoMxHidLoad(pCmd);
	}
	else if ( pCmd->GetType() == _T("jump") )
	{
        if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxRom)
        {
    		MxRomDevice* pMxRomDevice = dynamic_cast<MxRomDevice*>(m_pUSBPort->_device);
    		if ( pMxRomDevice )
    		{
    			if ( (retValue = pMxRomDevice->Jump()) == TRUE )
    				retValue = ERROR_SUCCESS;
    			else
    				retValue = ERROR_INVALID_HANDLE;
    		}
    		else
    			retValue = ERROR_INVALID_HANDLE;
        }
        else if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxHid)
        {
            MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(m_pUSBPort->_device);
    		if ( pMxHidDevice )
    		{
    			if ( (retValue = pMxHidDevice->Jump()) == TRUE )
    				retValue = ERROR_SUCCESS;
    			else
    				retValue = ERROR_INVALID_HANDLE;
    		}
    		else
    			retValue = ERROR_INVALID_HANDLE;
        }       
	}
	else if ( pCmd->GetType() == _T("reset") )
	{
		MxRomDevice* pMxRomDevice = dynamic_cast<MxRomDevice*>(m_pUSBPort->_device);
		if ( pMxRomDevice )
		{
			if ( (retValue = pMxRomDevice->Reset()) == TRUE )
				retValue = ERROR_SUCCESS;
			else
				retValue = ERROR_INVALID_HANDLE;
		}
		else
			retValue = ERROR_INVALID_HANDLE;
	}
	else if ( pCmd->GetType() == _T("burn") )
	{
        retValue = DoBurn(pCmd);
	}
	else if ( pCmd->GetType() == _T("show") )
	{
        retValue = DoShow(pCmd);
	}
	else if ( pCmd->GetType() == _T("format") )
	{
	}
	else if ( pCmd->GetType() == _T("push") )
	{
        // register for the progress events
///        CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

		if ( pCmd->GetFile() == _T("") )
        {
            // send command and wait for device to finish.
///            DoUtpCommandAsync utpCmdDelegate = new DoUtpCommandAsync(UTProtocol.UtpCommand);
///            IAsyncResult arUtpCmd = utpCmdDelegate.BeginInvoke(cmd.CommandString, null, null);
///
///            // Wait for the asynchronous operation to finish.
///            while (!arUtpCmd.IsCompleted)
///            {
///                // Keep UI messages moving, so the form remains 
///                // responsive during the asynchronous operation.
///                Application.DoEvents();
///            }
///
///            // Get the result of the operation.
///            retValue = utpCmdDelegate.EndInvoke(arUtpCmd);
/////                        retValue = UTProtocol.UtpCommand(cmd.CommandString);
			retValue = m_pUTP->UtpCommand(pCmd->GetBody());
        }
        else
        {
///            retValue = UTProtocol.UtpWrite(cmd.CommandString, cmd.Filename);
			CString fullFileName = m_pOpInfo->GetPath() + _T("\\") + pCmd->GetFile();
			retValue = m_pUTP->UtpWrite(pCmd->GetBody(), fullFileName, callback);
        }
///
///        // unregister for the progress events
///        CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;
	}
	else if ( pCmd->GetType() == _T("pull") )
	{
///        // register for the progress events
///        CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);
///
		CString panelIndex;
		panelIndex.Format(_T("%d"), m_pPortMgrDlg->GetPanelIndex());
		CString fullFileName = m_pOpInfo->GetPath() + _T("\\") + pCmd->GetFile();
		int indexOfExtension = fullFileName.ReverseFind(_T('.'));
		if ( indexOfExtension != -1 )
		{
			fullFileName.Insert(indexOfExtension, panelIndex); 
		}
		else
		{
			fullFileName += panelIndex;
		}
		
		retValue = m_pUTP->UtpRead(pCmd->GetBody(), fullFileName, callback);
///
///        // unregister for the progress events
///        CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;
///
	}
	else if ( pCmd->GetType() == _T("drop") )
	{
        // send command and wait for device to disconnect from the bus.
        retValue = DoDrop(pCmd);
	}
	else if ( pCmd->GetType() == _T("find") )
	{
        // See if specified device is connected and wait for it
        // if it is not.
        retValue = DoFind(pCmd);
	}
	
///    // unregister for the progress events
/////            if ( CurrentDevice != null )
/////                CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

	logText.Format(_T("%s Finished <CMD/> %s %s code=%d.\r\n"), m_pPortMgrDlg->GetPanel(), pCmd->ToString(), retValue == ERROR_SUCCESS ? _T("SUCCESS") : _T("FAIL"), retValue);
	bstr_log_text = logText.AllocSysString();
	((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

    ATLTRACE(_T("%s Finished <CMD/> %s %s code=%d.\r\n"), m_pPortMgrDlg->GetPanel(), pCmd->ToString(), retValue == ERROR_SUCCESS ? _T("SUCCESS") : _T("FAIL"), retValue);
	ATLTRACE(_T("=================================================================\r\n"));
	return retValue;
}

DWORD COpUtpUpdate::DoDrop(UCL::Command* pCmd)
{
    // Send the command.
	DWORD retValue = m_pUTP->UtpDrop(pCmd->GetBody());
/*
    if (retValue == 0)
    {
        // Wait for device to disconnect from the bus.
		while (m_CurrentDeviceState != UCL::DeviceState::Disconnected)
        {
			if ( pCmd->GetTimeout() )
			{
				// wait seconds for device to change state.
				if (!WaitForDeviceChange(pCmd->GetTimeout()))
				{
		///                SetText(OutputWindow, String.Format(" ERROR: Timeout. {0} never disconnected.\r\n", CurrentDevice.ToString()), "add");
					retValue = -65536;
					break;
				}
			}
        }
    }
*/
    return retValue;

} // DoDrop()

DWORD COpUtpUpdate::DoFind(UCL::Command* pCmd)
{
    DWORD retValue = 0;

	UCL::DeviceState::DeviceState_t newDevState = UCL::DeviceState::StringToDeviceState(pCmd->GetBody());
    UCL::DeviceDesc* pNewDevDesc = NULL;
	
	if (m_DeviceStates.find(newDevState) != m_DeviceStates.end())
        pNewDevDesc = m_DeviceStates[newDevState];

///    SetText(OutputWindow, String.Format("Looking for {0}-mode device.\r\n", newDevState), "add");

    if (m_pUSBPort->_device != NULL)
    {
        if (m_CurrentDeviceState == newDevState)
            return 0;
    }

   	VERIFY(::ResetEvent(m_hChangeEvent));
    // Wait for device to connect to the bus.
    while (m_CurrentDeviceState != newDevState)
    {
        // wait for device to change state.
		ATLTRACE(_T("%s WAITING FOR DEV_CHANGE: NewMode: %s CurrMode: %s \r\n"),m_pPortMgrDlg->GetPanel(), UCL::DeviceState::DeviceStateToString(newDevState), UCL::DeviceState::DeviceStateToString(m_CurrentDeviceState));
		if (!WaitForDeviceChange(pCmd->GetTimeout()))
        {
///            SetText(OutputWindow, String.Format(" ERROR: Timeout. Never found {0}.\r\n", newDevDesc.ToString()), "add");
			TRACE(_T(" ERROR: Timeout. Never found ") + pNewDevDesc->ToString()); TRACE(_T("\r\n"));
			TRACE(_T(" Current device mode: ") + UCL::DeviceState::DeviceStateToString(m_CurrentDeviceState)); TRACE(_T("\r\n"));
            retValue = -65536;
            break;
        }
		else if ( m_CurrentDeviceState != UCL::DeviceState::Disconnected && m_CurrentDeviceState != newDevState )
		{
			// If the device is connected and it is not in the expected mode, then don't wait any longer.
			TRACE(_T(" ERROR: Device is connected in an unexpected mode.\r\n"));
			TRACE(_T("  Current device mode: ") + UCL::DeviceState::DeviceStateToString(m_CurrentDeviceState)); TRACE(_T("\r\n"));
			TRACE(_T("  Expected device mode: ") + UCL::DeviceState::DeviceStateToString(newDevState)); TRACE(_T("\r\n"));
            retValue = -65534;
			break;
		}
    }

	ATLTRACE(_T("%s DONE WAITING FOR DEV_CHANGE: NewMode: %s CurrMode: %s \r\n"),m_pPortMgrDlg->GetPanel(), UCL::DeviceState::DeviceStateToString(newDevState), UCL::DeviceState::DeviceStateToString(m_CurrentDeviceState));
/*
    if (m_CurrentDeviceState == newDevState)
        retValue = 0;
    else
        retValue = -65536;
*/
    return retValue;

} // DoFind()

DWORD COpUtpUpdate::DoBoot(UCL::Command* pCmd)
{
    DWORD retValue = 0;

	CString logText;
	BSTR bstr_log_text;

///    if (String.IsNullOrEmpty(Thread.CurrentThread.Name))
///        Thread.CurrentThread.Name = "DoBoot";

    // Reset Device to Recovery-mode
	// NOT YET IMPLEMENTED ON MX/WINCE/LINUX DEVICES
    /*
	retValue = DoResetToRecovery();
    if (retValue != 0)
    {
		logText.Format(_T("%s DoBoot() - Failed to reset device to Recovery mode. (err=%d)\r\n"), m_pPortMgrDlg->GetPanel(), retValue);
		bstr_log_text = logText.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
		return retValue;
    }
	*/

    // Look for device in "Recovery" mode specified in pCmd->GetBody()
    retValue = DoFind(pCmd);
    if (retValue != 0)
    {
		logText.Format(_T("%s DoBoot() - Failed to find device in Recovery mode. (err=%d)\r\n"), m_pPortMgrDlg->GetPanel(), retValue);
		bstr_log_text = logText.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
		return retValue;
    }

	//if current device is i.MXxx device
	if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxRom)
	{
		retValue = DoMxRomLoad(pCmd);
	}
    else if ((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxHid)
    {
        retValue = DoPlugin(pCmd);
    }
	else
	{//HID device
		retValue = DoHidLoad(pCmd);
	}

    if (retValue != 0)
    {
		logText.Format(_T("%s DoBoot() - Failed to load %s to Recovery mode device. (err=%d)\r\n"), m_pPortMgrDlg->GetPanel(), pCmd->GetFile(), retValue);
		bstr_log_text = logText.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
    }

	return retValue;

} // DoBoot()

DWORD COpUtpUpdate::DoResetToRecovery()
{
    DWORD retValue = 0;

    //
    // Send Reset Command
    //
    switch (m_CurrentDeviceState)
    {
	case UCL::DeviceState::Recovery:
            // Already in Recovery mode, so we're done.
///            SetText(OutputWindow, "Already in Recovery-mode.\r\n", "add");
            break;
		case UCL::DeviceState::UserMsc:
        case UCL::DeviceState::UserMtp:
        case UCL::DeviceState::User:
        case UCL::DeviceState::Updater:
///        if (CurrentDevice is IResetToRecovery)
///            {
///                SetText(OutputWindow, "Sending ResetToRecovery command.\r\n", "add");
			if (m_pUSBPort->_device->ResetToRecovery() != ERROR_SUCCESS)
            {
///                    SetText(OutputWindow, CurrentDevice.ErrorString + "\r\n", "add");
                retValue = -65536;
            }
///            }
            break;
        case UCL::DeviceState::Disconnected:
        case UCL::DeviceState::Unknown:
        default:
///            SetText(OutputWindow, String.Format(" ERROR: Device \"{0}\" does not support ResetToRecovery command.\r\n", CurrentDevice == null ? "No Device" : CurrentDevice.ToString()), "add");
            retValue = -65536;
            break;
    }

    return retValue;
}

DWORD COpUtpUpdate::DoBurn(UCL::Command* pCmd)
{
    DWORD retValue = 0;

	HidPitcWrite apiOtpInit(0/*address*/,0/*length*/,0/*flags*/);

	uint8_t moreInfo = 0;
    
    retValue = m_pUSBPort->_device->SendCommand(apiOtpInit, &moreInfo);
    if ( retValue == ERROR_SUCCESS )
	{
		if ( moreInfo )
		{
			HidPitcRequestSense senseApi;
			retValue = m_pUSBPort->_device->SendCommand(senseApi, &moreInfo);
			if ( retValue == ERROR_SUCCESS ) {
				retValue = senseApi.GetSenseCode();
			}
		}
	}
    
	return retValue;
}

DWORD COpUtpUpdate::DoInit(UCL::Command* pCmd)
{
	DWORD returnVal = ERROR_SUCCESS;

    //if current device is i.MXxx device
    if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxRom)
    {
    	MxRomDevice* pMxRomDevice = dynamic_cast<MxRomDevice*>(m_pUSBPort->_device);
        //	TRACE(_T("%s"),m_pUSBPort->_device->name.c_str());
    	if ( pMxRomDevice == NULL )
    	{
    		returnVal = ERROR_INVALID_HANDLE;
    		ATLTRACE(_T("!!!ERROR!!! (%d): %s No MxRom device. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
    		return returnVal;
    	}

    	CString fullFileName;
    	fullFileName.Format(_T("%s//%s"), m_pOpInfo->GetPath(), pCmd->GetFile());

    	if( !pMxRomDevice->InitMemoryDevice(fullFileName) ) 
    	{
    		returnVal = ERROR_INVALID_HANDLE;
        //	taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, 0);
    		ATLTRACE(_T("!!!ERROR!!! %s Failed to initialize i.MXxx memory. OpState: %s\r\n"), m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
    		return returnVal;
    	}
    }
    else// For MX508
    {//HID device-Mx508
        MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(m_pUSBPort->_device);
    	if ( pMxHidDevice == NULL )
    	{
    		returnVal = ERROR_INVALID_HANDLE;
    		ATLTRACE(_T("!!!ERROR!!! (%d): %s No MxRom device. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
    		return returnVal;
    	}

    	CString fullFileName;
    	fullFileName.Format(_T("%s//%s"), m_pOpInfo->GetPath(), pCmd->GetFile());

    	if( !pMxHidDevice->InitMemoryDevice(fullFileName) ) 
    	{
    		returnVal = ERROR_INVALID_HANDLE;
    		ATLTRACE(_T("!!!ERROR!!! %s Failed to initialize i.MXxx memory. OpState: %s\r\n"), m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
    		return returnVal;
    	}
	}

	return returnVal;
}

DWORD COpUtpUpdate::DoPlugin(UCL::Command* pCmd)
{
	DWORD returnVal = ERROR_SUCCESS;

    //if current device is i.MXxx device
    if((DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() == DeviceClass::DeviceTypeMxRom)
    {
    	return returnVal;
    }
    else// For MX508
    {//HID device-Mx508
        MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(m_pUSBPort->_device);
    	if ( pMxHidDevice == NULL )
    	{
    		returnVal = ERROR_INVALID_HANDLE;
    		ATLTRACE(_T("!!!ERROR!!! (%d): %s No MxRom device. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
    		return returnVal;
    	}

    	CString fullFileName;
    	fullFileName.Format(_T("%s//%s"), m_pOpInfo->GetPath(), pCmd->GetFile());

    	if( !pMxHidDevice->RunPlugIn(fullFileName) ) 
    	{
    		returnVal = ERROR_INVALID_HANDLE;
    		ATLTRACE(_T("!!!ERROR!!! %s Failed to initialize i.MXxx memory. OpState: %s\r\n"), m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
    		return returnVal;
    	}
	}

	return returnVal;
}

DWORD COpUtpUpdate::DoMxRomLoad(UCL::Command* pCmd)
{

	DWORD returnVal = ERROR_SUCCESS;
	CString taskMsg;

///	taskMsg.LoadString(IDS_OPLOADER_LOAD_STARTED);
///	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

	MxRomDevice* pMxRomDevice = dynamic_cast<MxRomDevice*>(m_pUSBPort->_device);
//	TRACE(_T("%s"),m_pUSBPort->_device->name.c_str());
	if ( pMxRomDevice == NULL )
	{
		returnVal = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No MxRom device. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return returnVal;
	}

	CString fileName = m_pOpInfo->GetPath() + _T("\\") + pCmd->GetFile();
	StFwComponent fwObject((LPCTSTR)fileName);
	if ( fwObject.GetLastError() != ERROR_SUCCESS )
	{
		returnVal = fwObject.GetLastError();
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No FW component. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return returnVal;
	}

	// Set up the Task progress bar and bump the Operation progress bar
///	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, myNewFwCommandSupport.GetFwComponent().GetShortFileName().c_str());   // "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(NULL, m_iPercentComplete, fwObject.size(), 0);       // STAGE 2 of the Load Operation
	Device::UI_Callback callback(this, &COpUtpUpdate::OnDownloadProgress);

	MxRomDevice::ImageParameter ImageParameter;
	ImageParameter.loadSection = MxRomDevice::StringToMemorySection(pCmd->GetLoadSection());
	ImageParameter.setSection = MxRomDevice::StringToMemorySection(pCmd->GetSetSection());
	ImageParameter.PhyRAMAddr4KRL = pCmd->GetAddress();
	ImageParameter.HasFlashHeader = pCmd->HasFlashHeader();
	ImageParameter.CodeOffset = pCmd->GetCodeOffset();

	returnVal = pMxRomDevice->DownloadImage(&ImageParameter, fwObject, callback);

	if(returnVal != TRUE) 
	{
		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, returnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load i.MXxx device. OpState: %s\r\n"), returnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return returnVal;
	}

	// Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);
	ATLTRACE(_T("RAM kernel: %s is downloaded to the device of %s.\r\n"),pCmd->GetFile(), m_pPortMgrDlg->GetPanel());
	return ERROR_SUCCESS;
}

/*
DWORD COpUtpUpdate::DoLoad(CString filename)
{
    DWORD retValue = 0;

	if ( m_CurrentDeviceState == UCL::DeviceState::Recovery )
    {
///        SetText(OutputWindow, String.Format("Loading {0}.\r\n", filename), "add");
///
///        // register for the progress events
///        CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

        if (((IRecoverable)CurrentDevice).LoadRecoveryDevice(filename) != Win32.ERROR_SUCCESS)
        {
            SetText(OutputWindow, CurrentDevice.ErrorString + "\r\n", "add");
            retValue = -65536;
        }

        // unregister for the progress events
        CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;
    }
    else
    {
        SetText(OutputWindow, String.Format("ERROR: Device {0} is not in the correct mode for loading {1}.\r\n", CurrentDevice, filename), "add");
        retValue = -65536;
    }

//            SetText(OutputWindow, String.Format("DONE Loading {0}.\r\n", filename), "add");
    return retValue;
}
*/
DWORD COpUtpUpdate::DoShow(UCL::Command* pCmd)
{
    DWORD retValue = 0;
/*    DeviceInfo devInfo = null;

    // Get the device information.
    try
    {
        XmlSerializer xmlr = new XmlSerializer(typeof(DeviceInfo));
        FileStream fs = new FileStream(cmd.Filename, FileMode.Open);
        devInfo = (DeviceInfo)xmlr.Deserialize(fs);
        fs.Close();
    }
    catch (Exception err)
    {
        DialogResult result = MessageBox.Show(this, err.Message);
    }
    
    if ( devInfo != null )
        CurrentVersionTextBox.Text = devInfo.FW_Version;
*/
    return retValue;
}

