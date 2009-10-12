// OpUtpUpdate.cpp : implementation file
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
#include "../../Libs/DevSupport/StHidApi.h"

#include "OpUtpUpdate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// #define DEBUGOTPINIT 1

#import <msxml3.dll> 
using namespace MSXML2;

extern BOOL g_TestLoop;
extern HANDLE g_HIDMutex;
extern BOOL g_StopFlagged;  // need a global to reflect the STOP state and break out of request for HID mutex

IMPLEMENT_DYNCREATE(COpUtpUpdate, COperation)

COpUtpUpdate::COpUtpUpdate(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo* pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo, UPDATE_OP)
, m_pCmdList(NULL)
, m_p_do_list_thread(NULL)
, m_bProcessingList(FALSE)
, m_pUTP(NULL)
, m_hChangeEvent(INVALID_HANDLE_VALUE)
, m_CurrentDeviceMode(UCL::DeviceDesc::Unknown)
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
	commandFile.Read(uclString.GetBufferSetLength(commandFile.GetLength()), commandFile.GetLength());
	uclString.ReleaseBuffer();

	if ( m_UclNode.Load(A2T(uclString)) != NULL )
	{
		m_pCmdList = m_UclNode.GetCommandList(m_pOpInfo->GetUclInstallSection());
		if ( m_pCmdList )
		{
			m_sDescription = m_pCmdList->GetDescription();
			m_iDuration = m_pCmdList->GetCommandCount() * 2; // bump overall progress before and after every command
		}
		m_DeviceDescs = m_UclNode.GetConfiguration()->GetDeviceDescs();
	}
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

const UCL::DeviceDesc::DeviceMode COpUtpUpdate::GetDeviceMode()
{
	UCL::DeviceDesc::DeviceMode mode = UCL::DeviceDesc::Unknown;

	switch ( (DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() )
	{
		case DeviceClass::DeviceTypeHid:
		case DeviceClass::DeviceTypeRecovery:
			mode = UCL::DeviceDesc::Recovery;
			break;
		case DeviceClass::DeviceTypeMsc:
		{
			CString devPath = m_pUSBPort->GetUsbDevicePath();
			devPath.MakeUpper();

			CString csVidPid;
			// Check for USER MSC mode
			if ( m_DeviceDescs.find(UCL::DeviceDesc::UserMsc) != m_DeviceDescs.end() )
			{
				csVidPid.Format(_T("Vid_%s&Pid_%s"), m_DeviceDescs[UCL::DeviceDesc::UserMsc]->GetVid(), m_DeviceDescs[UCL::DeviceDesc::UserMsc]->GetPid());
				csVidPid.MakeUpper();

				if ( devPath.Find(csVidPid) != -1 )
				{
					mode = UCL::DeviceDesc::UserMsc;
				}
			}
			if ( mode != UCL::DeviceDesc::UserMsc )
			{
				// Check for UPDATER mode
				if ( m_DeviceDescs.find(UCL::DeviceDesc::Updater) != m_DeviceDescs.end() )
				{
					csVidPid.Format(_T("Vid_%s&Pid_%s"), m_DeviceDescs[UCL::DeviceDesc::Updater]->GetVid(), m_DeviceDescs[UCL::DeviceDesc::Updater]->GetPid());
					csVidPid.MakeUpper();

					if ( devPath.Find(csVidPid) != -1 )
					{
						mode = UCL::DeviceDesc::Updater;
					}
				}
			}
			if ( mode != UCL::DeviceDesc::UserMsc && mode != UCL::DeviceDesc::Updater )
			{
				mode = UCL::DeviceDesc::ConnectedUnknown;
			}
			break;
		}
		case DeviceClass::DeviceTypeMtp:
			mode = UCL::DeviceDesc::UserMtp;
			break;
		case DeviceClass::DeviceTypeNone:
			mode = UCL::DeviceDesc::Disconnected;
			break;
		default:
			mode = UCL::DeviceDesc::ConnectedUnknown;
			break;
	}

	return mode;
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

			m_CurrentDeviceMode = GetDeviceMode();
			
			if ( m_CurrentDeviceMode == UCL::DeviceDesc::Updater )
				m_pUTP = new UpdateTransportProtocol((Volume*)m_pUSBPort->_device);

			if ( m_CurrentDeviceMode != UCL::DeviceDesc::Disconnected && m_CurrentDeviceMode != UCL::DeviceDesc::ConnectedUnknown)
			{
				// Start the thread to process the list
				m_p_do_list_thread = AfxBeginThread( DoListThreadProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED );
				m_p_do_list_thread->ResumeThread();
			}
			else if (m_CurrentDeviceMode == UCL::DeviceDesc::Disconnected)
			{
				m_pPortMgrDlg->UpdateUI(_T("Please connect device."));
				m_OpState = WAITING_FOR_DEVICE;
			}			
			else if (m_CurrentDeviceMode == UCL::DeviceDesc::ConnectedUnknown)
				m_pPortMgrDlg->UpdateUI(_T("Unrecognized device."));

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

            UCL::DeviceDesc::DeviceMode oldMode = m_CurrentDeviceMode;
			m_CurrentDeviceMode = GetDeviceMode();
			ATLTRACE(_T("%s UtpUpdate Event: %s Msg: %s DevMode: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, UCL::DeviceDesc::DeviceModeToString(m_CurrentDeviceMode), GetOpStateString(m_OpState));

			if (oldMode == UCL::DeviceDesc::Disconnected)
            {
                switch (nEventType)
                {
                    case OPEVENT_DEVICE_ARRIVAL:
                        break;

                    case OPEVENT_VOLUME_ARRIVAL:
						if ( m_CurrentDeviceMode == UCL::DeviceDesc::Updater )
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
				   ( m_CurrentDeviceMode != UCL::DeviceDesc::Disconnected ||
				     m_CurrentDeviceMode != UCL::DeviceDesc::ConnectedUnknown ) )
				{
					// Start the thread to process the list
					m_p_do_list_thread = AfxBeginThread( DoListThreadProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED );
					m_p_do_list_thread->ResumeThread();
				}
				else
				{
					if (m_CurrentDeviceMode == UCL::DeviceDesc::Disconnected)
						m_pPortMgrDlg->UpdateUI(_T("Please connect device."));
					else if (m_CurrentDeviceMode == UCL::DeviceDesc::ConnectedUnknown)
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

                    m_CurrentDeviceMode = UCL::DeviceDesc::Disconnected;
///                    DeviceDescStatusLabel.Text = DeviceMode.Disconnected.ToString();

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

            if (oldMode != m_CurrentDeviceMode)
			{
				// Signal anyone waiting on a device change to go see what changed.
				if ( m_hChangeEvent != INVALID_HANDLE_VALUE )
				{
					VERIFY(::SetEvent(m_hChangeEvent));
					ATLTRACE(_T("%s UtpUpdate Event: %s Msg: %s DevMode: %s OpState: %s SET_EVENT\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, UCL::DeviceDesc::DeviceModeToString(m_CurrentDeviceMode), GetOpStateString(m_OpState));
				}
			}
			else
			{
				ATLTRACE(_T("%s UtpUpdate Event: %s Msg: %s DevMode: %s OpState: %s NO SET_EVENT!!!\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, UCL::DeviceDesc::DeviceModeToString(m_CurrentDeviceMode), GetOpStateString(m_OpState));
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
        retValue = FALSE;
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
DWORD COpUtpUpdate::DoLoad(CString filename) // RecoverHidDevice()
{

	DWORD ReturnVal = ERROR_SUCCESS;
	CString taskMsg;
#ifdef DEBUGOTPINIT
BSTR bstr_log_text;
CString _logText;
#endif
///	taskMsg.LoadString(IDS_OPLOADER_LOAD_STARTED);
///	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

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

	StPitc myNewFwCommandSupport(pHidDevice, (LPCTSTR)filename,
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
///	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, myNewFwCommandSupport.GetFwComponent().GetShortFileName().c_str());   // "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(NULL, m_iPercentComplete, (int)myNewFwCommandSupport.GetFwComponent().size(), 0);       // STAGE 2 of the Load Operation
#ifdef DEBUGOTPINIT
_logText.Format(_T("%s HidRecover - Downloading...\r\n"), m_pPortMgrDlg->GetPanel());
bstr_log_text = _logText.AllocSysString();
((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
#endif
	Device::UI_Callback callback(this, &COpUtpUpdate::OnDownloadProgress);
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

	for ( size_t i = 0; i < pOperation->m_pCmdList->GetChildCount(); ++i )
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

    if ( pCmd->GetType() == _T("boot") )
	{
		// Reset device to ROM and load file.
        retValue = DoBoot(pCmd);
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
		while (m_CurrentDeviceMode != UCL::DeviceDesc::Disconnected)
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

	UCL::DeviceDesc::DeviceMode newDevMode = UCL::DeviceDesc::StringToDeviceMode(pCmd->GetBody());
    UCL::DeviceDesc* pNewDevDesc = NULL;
	
	if (m_DeviceDescs.find(newDevMode) != m_DeviceDescs.end())
        pNewDevDesc = m_DeviceDescs[newDevMode];

///    SetText(OutputWindow, String.Format("Looking for {0}-mode device.\r\n", newDevMode), "add");

    if (m_pUSBPort->_device != NULL)
    {
        if (m_CurrentDeviceMode == newDevMode)
            return 0;
    }

    // Wait for device to connect to the bus.
    while (m_CurrentDeviceMode != newDevMode)
    {
        // wait for device to change state.
		ATLTRACE(_T("%s WAITING FOR DEV_CHANGE: NewMode: %s CurrMode: %s \r\n"),m_pPortMgrDlg->GetPanel(), UCL::DeviceDesc::DeviceModeToString(newDevMode), UCL::DeviceDesc::DeviceModeToString(m_CurrentDeviceMode));
		if (!WaitForDeviceChange(pCmd->GetTimeout()))
        {
///            SetText(OutputWindow, String.Format(" ERROR: Timeout. Never found {0}.\r\n", newDevDesc.ToString()), "add");
			TRACE(_T(" ERROR: Timeout. Never found ") + pNewDevDesc->ToString()); TRACE(_T("\r\n"));
			TRACE(_T(" Current device mode: ") + UCL::DeviceDesc::DeviceModeToString(m_CurrentDeviceMode)); TRACE(_T("\r\n"));
            retValue = -65536;
            break;
        }
		else if ( m_CurrentDeviceMode != UCL::DeviceDesc::Disconnected && m_CurrentDeviceMode != newDevMode )
		{
			// If the device is connected and it is not in the expected mode, then don't wait any longer.
			TRACE(_T(" ERROR: Device is connected in an unexpected mode.\r\n"));
			TRACE(_T("  Current device mode: ") + UCL::DeviceDesc::DeviceModeToString(m_CurrentDeviceMode)); TRACE(_T("\r\n"));
			TRACE(_T("  Expected device mode: ") + UCL::DeviceDesc::DeviceModeToString(newDevMode)); TRACE(_T("\r\n"));
            retValue = -65534;
			break;
		}
    }
	ATLTRACE(_T("%s DONE WAITING FOR DEV_CHANGE: NewMode: %s CurrMode: %s \r\n"),m_pPortMgrDlg->GetPanel(), UCL::DeviceDesc::DeviceModeToString(newDevMode), UCL::DeviceDesc::DeviceModeToString(m_CurrentDeviceMode));
/*
    if (m_CurrentDeviceMode == newDevMode)
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
    retValue = DoResetToRecovery();
    if (retValue != 0)
    {
		logText.Format(_T("%s DoBoot() - Failed to reset device to Recovery mode. (err=%d)\r\n"), m_pPortMgrDlg->GetPanel(), retValue);
		bstr_log_text = logText.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
		return retValue;
    }

    // Look for device in Recovery-mode
    retValue = DoFind(pCmd);
    if (retValue != 0)
    {
		logText.Format(_T("%s DoBoot() - Failed to find device in Recovery mode. (err=%d)\r\n"), m_pPortMgrDlg->GetPanel(), retValue);
		bstr_log_text = logText.AllocSysString();
		((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOG_OP_EVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
		return retValue;
    }

	CString fullFileName = m_pOpInfo->GetPath() + _T("\\") + pCmd->GetFile();
	retValue = DoLoad(fullFileName);
    if (retValue != 0)
    {
		logText.Format(_T("%s DoBoot() - Failed to load %s to Recovery mode device. (err=%d)\r\n"), m_pPortMgrDlg->GetPanel(), fullFileName,retValue);
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
    switch (m_CurrentDeviceMode)
    {
	case UCL::DeviceDesc::Recovery:
            // Already in Recovery mode, so we're done.
///            SetText(OutputWindow, "Already in Recovery-mode.\r\n", "add");
            break;
		case UCL::DeviceDesc::UserMsc:
        case UCL::DeviceDesc::UserMtp:
        case UCL::DeviceDesc::User:
        case UCL::DeviceDesc::Updater:
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
        case UCL::DeviceDesc::Disconnected:
        case UCL::DeviceDesc::Unknown:
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
/*
DWORD COpUtpUpdate::DoLoad(CString filename)
{
    DWORD retValue = 0;

	if ( m_CurrentDeviceMode == UCL::DeviceDesc::Recovery )
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

