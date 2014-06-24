/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "MfgToolLib.h"
#include "CmdOperation.h"
#include "HubClass.h"
#include "UsbHub.h"
#include "MxHidDevice.h"

#include "DeviceManager.h"

extern DEV_CLASS_ARRAY g_devClasses;

//only for debug
extern LARGE_INTEGER g_t1, g_t2, g_t3, g_tc;

UINT CmdListThreadProc(LPVOID pParam);

CCmdOpreation::CCmdOpreation(INSTANCE_HANDLE handle, int WndIndex)
{
	m_bAutoDelete = FALSE; //Don't destroy the object at thread termination
	m_WndIndex = WndIndex;
	m_p_usb_port = NULL;
	m_usb_port_index = 0;
	m_pUTP = NULL;

	m_pLibHandle = handle;

	m_hDeviceChangeCallback = NULL;

	m_hMutex_cb = ::CreateMutex(NULL, FALSE, NULL);
	m_hMutex_cb2 = ::CreateMutex(NULL, FALSE, NULL);
}

CCmdOpreation::~CCmdOpreation()
{
	if (m_pUTP != NULL)
	{
		delete m_pUTP;
		m_pUTP = NULL;
	}

	::CloseHandle(m_hMutex_cb);
	::CloseHandle(m_hMutex_cb2);
}

DWORD CCmdOpreation::Open()
{
	m_hKillEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if(m_hKillEvent == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CCmdOpreation::Open---Create m_hKillEvent error\n"));
		return MFGLIB_ERROR_NO_MEMORY;
	}

	m_hDeviceArriveEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if(m_hDeviceArriveEvent == NULL)
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CCmdOpreation::Open---Create m_hDeviceArriveEvent error\n"));
		return MFGLIB_ERROR_NO_MEMORY;
    }
	m_hDeviceRemoveEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if(m_hDeviceRemoveEvent == NULL)
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CCmdOpreation::Open---Create m_hDeviceRemoveEvent error\n"));
		return MFGLIB_ERROR_NO_MEMORY;
    }
	m_hRunEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if(m_hRunEvent == NULL)
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CCmdOpreation::Open---Create m_hRunEvent error\n"));
		return MFGLIB_ERROR_NO_MEMORY;
    }
	m_hStopEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if(m_hStopEvent == NULL)
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CCmdOpreation::Open---Create m_hStopEvent error\n"));
		return MFGLIB_ERROR_NO_MEMORY;
    }
	m_hOneCmdCompleteEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if(m_hOneCmdCompleteEvent == NULL)
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CCmdOpreation::Open---Create m_hOneCmdCompleteEvent error\n"));
		return MFGLIB_ERROR_NO_MEMORY;
    }

	m_hDevCanDeleteEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if(m_hDevCanDeleteEvent == NULL)
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CCmdOpreation::Open---Create m_hDevCanDeleteEvent error\n"));
		return MFGLIB_ERROR_NO_MEMORY;
    }
	((MFGLIB_VARS *)m_pLibHandle)->g_hDevCanDeleteEvts[m_WndIndex] = m_hDevCanDeleteEvent;

	m_hThreadStartEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if(m_hThreadStartEvent == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CCmdOpreation::Open---Create m_hThreadStartEvent error\n"));
		return MFGLIB_ERROR_NO_MEMORY;
	}

	m_bKilled = FALSE;
	m_bRun = FALSE;
	m_bDeviceOn = FALSE;

	m_usb_hub_name = ((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[m_WndIndex].hubPath;
	m_usb_port_index = ((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[m_WndIndex].portIndex;
	SetUsbPort(FindPort());

	if(g_pDeviceManager != NULL)
	{
		DeviceManager::CBStruct _sCB;
		_sCB.pfunc = gOnDeviceChangeNotify;
		_sCB.Hub = m_usb_hub_name;
		_sCB.HubIndex = m_usb_port_index;
		_sCB.cmdOpIndex = m_WndIndex;
		m_hDeviceChangeCallback = g_pDeviceManager->RegisterCallback(&_sCB);
	}

	if( CreateThread() != 0 )
	{
		::WaitForSingleObject(m_hThreadStartEvent, INFINITE);
	}
	::CloseHandle(m_hThreadStartEvent);
	m_hThreadStartEvent = NULL;

	((MFGLIB_VARS *)m_pLibHandle)->g_CmdOpThreadID[m_WndIndex] = this->m_nThreadID;

	// Resume CmdList thread
	m_pThread->ResumeThread();

	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CCmdOpreation[%d]-[Device:%p] thread is Running"), m_WndIndex, m_p_usb_port->GetDevice());

	return MFGLIB_ERROR_SUCCESS;
}

void CCmdOpreation::Close()
{
	//End CmdListThreadProc
	SetEvent(m_hKillEvent);
	WaitForSingleObject(m_pThread->m_hThread, INFINITE);

	::CloseHandle(m_hKillEvent);
	m_hKillEvent = NULL;
	::CloseHandle(m_hDeviceArriveEvent);
	m_hDeviceArriveEvent = NULL;
	::CloseHandle(m_hDeviceRemoveEvent);
	m_hDeviceRemoveEvent = NULL;
	::CloseHandle(m_hRunEvent);
	m_hRunEvent = NULL;
	::CloseHandle(m_hStopEvent);
	m_hStopEvent = NULL;
	::CloseHandle(m_hDevCanDeleteEvent);
	m_hDevCanDeleteEvent = NULL;
	((MFGLIB_VARS *)m_pLibHandle)->g_hDevCanDeleteEvts[m_WndIndex] = NULL;

	if(m_hDeviceChangeCallback != NULL)
	{
		if(g_pDeviceManager != NULL)
		{
			g_pDeviceManager->UnregisterCallback(m_hDeviceChangeCallback);
		}
	}

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CCmdOpreation[%d] thread is Closed"), m_WndIndex);
}

MX_DEVICE_STATE CCmdOpreation::GetDeviceState()
{
	if(NULL == m_p_usb_port->GetDevice())
	{
		return MX_DISCONNECTED;
	}

	MX_DEVICE_STATE state = MX_DISCONNECTED;
	switch((DeviceClass::DEV_CLASS_TYPE)(m_p_usb_port->GetDeviceType()))
	{
	case DeviceClass::DeviceTypeHid:
	case DeviceClass::DeviceTypeMxHid:
	case DeviceClass::DeviceTypeMxRom:
		state = MX_BOOTSTRAP;
		break;
	case DeviceClass::DeviceTypeMsc:
	case DeviceClass::DeviceTypeDisk:
		state = MX_UPDATER;
		break;
	default:
		state = MX_DISCONNECTED;
		break;
	}

	return state;
}

usb::Port* CCmdOpreation::FindPort()
{
	usb::Port * pPort = NULL;
	if ( m_usb_hub_name.IsEmpty() || m_usb_port_index == 0 ) 
	{
        return NULL;
    }

	usb::HubClass *pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
	ASSERT(pHubClass);
	usb::Hub* pHub = pHubClass->FindHubByPath(m_usb_hub_name);
	if ( pHub )
    {
		pPort = pHub->Port(m_usb_port_index);
    }

	return pPort;
}

void CCmdOpreation::SetUsbPort(usb::Port* _port)
{
	if(_port != NULL)
	{
		m_p_usb_port = _port;
		m_usb_hub_name = _port->GetParentHub()->_path.get();
        m_usb_port_index = _port->GetIndex();
		m_p_usb_port->SetWndIndex(m_WndIndex);
		m_p_usb_port->Refresh();
	}
}

BOOL CCmdOpreation::InitInstance()
{
	m_pThread = AfxBeginThread(CmdListThreadProc, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	SetEvent(m_hThreadStartEvent);

	return TRUE;
}

BOOL CCmdOpreation::OnStart()
{
	if(m_hRunEvent && !m_bRun)
    {
		if(m_pUTP != NULL)
		{
			m_pUTP->m_bShouldStop = FALSE;
		}
        SetEvent(m_hRunEvent);
        return TRUE;
    }
    else
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't set m_hRunEvent before it initialized or it already run\n"));
        return FALSE;
    }
}

BOOL CCmdOpreation::OnStop()
{
	if(m_hStopEvent && m_bRun)
    {
		if(m_pUTP != NULL)
		{
			m_pUTP->m_bShouldStop = TRUE;
		}
        SetEvent(m_hStopEvent);
        return TRUE;
    }
    else
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't set m_hStopEvent before it initialized or it already stop\n"));
        return FALSE;
    }
}

BOOL CCmdOpreation::OnDeviceArrive()
{
	if(m_hDeviceArriveEvent)
    {
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CmdOpreation[%d]--set m_hDeviceArriveEvent."), m_WndIndex);
        SetEvent(m_hDeviceArriveEvent);
        return TRUE;
    }
    else
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't set m_hDeviceArriveEvent before it initialized\n"));
        return FALSE;
    }
}

BOOL CCmdOpreation::OnDeviceRemove()
{
	if(m_hDeviceRemoveEvent)
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CmdOpreation[%d]--set m_hDeviceRemoveEvent."), m_WndIndex);
        SetEvent(m_hDeviceRemoveEvent);
        return TRUE;
    }
    else
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't set m_hDeviceRemoveEvent before it initialized\n"));
        return FALSE;
    }
}

int CCmdOpreation::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	return CWinThread::ExitInstance();
}

BOOL CCmdOpreation::CanRun()
{
    if(m_bRun
	    //&& m_bDeviceOn
	    && !m_bKilled
	    && m_p_usb_port
	    && m_p_usb_port->Connected())
	{
		//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOperation[%d]-CanRun return TRUE."), m_WndIndex);
	    return TRUE;
	}
	else
	{
		//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOperation[%d]-CanRun return FALSE."), m_WndIndex);
		return FALSE;
	}
}

//update the phase index
DWORD CCmdOpreation::UpdateUI(UI_UPDATE_INFORMATION* _uiInfo,DWORD dwStateIndex)
{
	_uiInfo->OperationID = ((MFGLIB_VARS *)m_pLibHandle)->g_CmdOpThreadID[m_WndIndex];
	//_uiInfo->bUpdateCommandsProgress = FALSE;
	_uiInfo->bUpdateCommandsProgress = TRUE;
	_uiInfo->CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibHandle)->g_CmdOperationArray[m_WndIndex]->m_dwCmdIndex;
	_uiInfo->CommandStatus = COMMAND_STATUS_EXECUTE_RUNNING;
	_uiInfo->bUpdateDescription = FALSE;
	_uiInfo->bUpdateProgressInCommand = FALSE;
	_uiInfo->bUpdatePhaseProgress = TRUE;
	_uiInfo->CurrentPhaseIndex = dwStateIndex;
	_uiInfo->currentState = (MX_DEVICE_STATE)dwStateIndex;
	//_uiInfo->PhaseStatus = phase_status;
	ExecuteUIUpdate(_uiInfo);
	return 0;
}



DWORD CCmdOpreation::WaitforEvents(DWORD dwTimeOut)
{
    DWORD dwRet = 0;
    HANDLE waitHandles[6] = { 
				m_hKillEvent, 
				m_hDeviceArriveEvent,
				m_hDeviceRemoveEvent,
				m_hRunEvent,  
				m_hStopEvent,
				m_hOneCmdCompleteEvent
	};

	DWORD dwResult = ::WaitForMultipleObjects(6, &waitHandles[0], FALSE, dwTimeOut);
	switch(dwResult)
	{
	case WAIT_OBJECT_0:  // stop command execution thread
		m_bKilled = TRUE;
		dwRet = 0;
		break;
	case WAIT_OBJECT_0+1: // device arrive
		TRACE(_T("WaitforEvents device arrive1\r\n"));
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOpreation[%d]--WaitforEvents device arrive1"), m_WndIndex);
		//bDeviceChange = TRUE;
        //bStateCmdFinished = FALSE; 
		//dwError = MFGLIB_ERROR_SUCCESS;
		m_bDeviceOn = TRUE;
		dwRet = 1;
		break;
	case WAIT_OBJECT_0+2: // device remove
		TRACE(_T("CmdListThreadProc device remove1\r\n"));
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOpreation[%d]--WaitforEvents device remove1"), m_WndIndex);
		//bDeviceChange = TRUE;
        //bStateCmdFinished = TRUE; 
		//dwError = MFGLIB_ERROR_SUCCESS;
		m_bDeviceOn = FALSE;
		//if(pOperation->m_pUTP != NULL)
    	//{
    	//	delete pOperation->m_pUTP;
    	//	pOperation->m_pUTP = NULL;
    	//}
		//SetEvent(pOperation->m_hDevCanDeleteEvent);
		dwRet = 2;
		break;
	case WAIT_OBJECT_0+3: // press the Start button
		m_bRun = TRUE;
		dwRet = 3;
		break;
	case WAIT_OBJECT_0+4: // press the Stop button
		m_bRun = FALSE;
		dwRet = 4;
		break;
	case WAIT_OBJECT_0+5: // one command execution finished
		//pOperation->m_CmdIndex++;
		dwRet = 5;
		break;
	case WAIT_TIMEOUT: // no any event, time out
		//dwTimeout = INFINITE;
		dwRet = 6;
		break;
	default:
		break;
	}
    return dwRet;
}

UINT CmdListThreadProc(LPVOID pParam)
{
	CCmdOpreation* pOperation = (CCmdOpreation*)pParam;

	BOOL bStateCmdFinished = FALSE;
	DWORD dwTimeout = INFINITE;
	Device* pDevice = NULL;
	MX_DEVICE_STATE currentState = MX_DISCONNECTED;
	DWORD dwError = MFGLIB_ERROR_SUCCESS;
	OP_COMMAND_ARRAY CurrentCommands;
	OP_COMMAND_ARRAY::iterator cmdIt;
	DWORD dwStateIndex = 0;
	CString chip;

	while(!pOperation->m_bKilled)
	{
		bStateCmdFinished = FALSE;
		do
		{
		    DWORD dwResult = pOperation->WaitforEvents(dwTimeout);
			if(dwResult == 2)
		    {
		        pDevice = NULL;
				if(pOperation->m_pUTP != NULL)
        		{
        			delete pOperation->m_pUTP;
        			pOperation->m_pUTP = NULL;
        		}
    			SetEvent(pOperation->m_hDevCanDeleteEvent);
				dwTimeout = INFINITE;
				break;
			}
			if(pOperation->CanRun())
			{
			    if(!pDevice)
			    {
				    //first time to execute
				    pDevice = pOperation->m_p_usb_port->GetDevice();
					if(!pDevice)
					{
						TRACE( _T("DeviceChanged port connected but device not found\n"));
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOpreation[%d]--DeviceChanged port connected but device not found."), pOperation->m_WndIndex);
						dwTimeout = INFINITE;
						break;
					}

					currentState = pOperation->GetDeviceState();
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOperation[%d] device chagned and reset to state %d"), pOperation->m_WndIndex, currentState);
					CurrentCommands = ((MFGLIB_VARS *)(pOperation->m_pLibHandle))->g_StateCommands[currentState];
					dwStateIndex = (DWORD)currentState;
					pOperation->m_currentState = currentState;

					//Update state progress
					//UI_UPDATE_INFORMATION _uiInfo;
					//pOperation->UpdateUI(&_uiInfo,dwStateIndex);
					
					if(CurrentCommands.size() == 0)
					{
						bStateCmdFinished = TRUE;
                        dwTimeout = INFINITE;
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("current state no command, so SetEvent(hDevCanDeleteEvent)"));
						//SetEvent(pDevice->m_hDevCanDeleteEvent);
                        TRACE( _T("DeviceCmd is NULL, just finished\n"));
						//pOperation->UpdateUI(&_uiInfo,dwStateIndex);
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceCmd is NULL, just finished."));
						break;
					}
					cmdIt = CurrentCommands.begin();
					pOperation->m_dwCmdIndex = 1;
                    bStateCmdFinished = FALSE;
					if(pDevice->GetDeviceType() == DeviceClass::DeviceTypeMxHid)
					{
						chip = ((MxHidDevice*)pDevice)->_chiFamilyName;
					}
				}
				
				COpCommand *pCmd = (*cmdIt);
				if(pCmd->IsRun(chip))
				{
					dwError = pCmd->ExecuteCommand(pOperation->m_WndIndex);
				}else
				{
					dwError = MFGLIB_ERROR_SKIP;
				}

				if(dwError == MFGLIB_ERROR_SUCCESS || dwError == MFGLIB_ERROR_SKIP)
				{	//execute command successfully

					if(dwError == MFGLIB_ERROR_SKIP)
					{
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOperation[%d], skip current command, so SetEvent(hDevCanDeleteEvent)"), pOperation->m_WndIndex);
					}

				    cmdIt++;
					
					//In current state(Bootstrap or Updater), all commands are executed
					if(cmdIt == CurrentCommands.end())
					{
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOperation[%d], current state command has been finished and the last command is successful, so SetEvent(hDevCanDeleteEvent)"), pOperation->m_WndIndex);
						dwTimeout = GetOpStatesTimeout(GetOpStates((MFGLIB_VARS *)(pOperation->m_pLibHandle)), currentState);
						dwTimeout = dwTimeout * 1000;
						bStateCmdFinished = TRUE;
						pDevice = NULL;
						UI_UPDATE_INFORMATION _uiInfo;
						//pOperation->UpdateUI(&_uiInfo,dwStateIndex);
					}
					else
					{
					    dwTimeout = 0;
					}
				}
				else if(dwError == MFGLIB_ERROR_SUCCESS_UPDATE_COMPLETE)
				{
					dwTimeout = INFINITE;
					bStateCmdFinished = TRUE;
					pDevice = NULL;
					//Update UI
					//Update state progress
					//UI_UPDATE_INFORMATION _uiInfo;
					//pOperation->UpdateUI(&_uiInfo,dwStateIndex);
				}
				else
				{
					// error
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOperation[%d], current command executed failed, so SetEvent(hDevCanDeleteEvent)"), pOperation->m_WndIndex);
					dwTimeout = INFINITE; 
				}
				pOperation->m_dwCmdIndex++;
			}
		}while(pOperation->CanRun() && !bStateCmdFinished && ((dwError == MFGLIB_ERROR_SUCCESS) || (dwError == MFGLIB_ERROR_SKIP)));
	}

	return 0;
}

void CCmdOpreation::mRegisterUIDevChangeCallback(DeviceChangeCallbackStruct *pCB)
{
	WaitForSingleObject(m_hMutex_cb, INFINITE);
	m_callbacks[m_WndIndex] = pCB;
	ReleaseMutex(m_hMutex_cb);
}

void CCmdOpreation::mUnregisterUIDevChangeCallback()
{
	WaitForSingleObject(m_hMutex_cb, INFINITE);
	m_callbacks.erase(m_WndIndex);
	ReleaseMutex(m_hMutex_cb);
}

void CCmdOpreation::mRegisterUIInfoUpdateCallback(OperateResultUpdateStruct *pCB)
{
	WaitForSingleObject(m_hMutex_cb2, INFINITE);
	m_callbacks2[m_WndIndex] = pCB;
	ReleaseMutex(m_hMutex_cb2);
}

void CCmdOpreation::mUnregisterUIInfoUpdateCallback()
{
		WaitForSingleObject(m_hMutex_cb2, INFINITE);
		m_callbacks2.erase(m_WndIndex);
		ReleaseMutex(m_hMutex_cb2);
}

void CCmdOpreation::OnDeviceChangeNotify(DeviceClass::NotifyStruct *pnsinfo)
{
    TRACE(_T("OnDeviceChangeNotify bbb\r\n"));
	std::map<int, DeviceChangeCallbackStruct*>::iterator cbIt = m_callbacks.begin();
	CString strDesc;

	switch(pnsinfo->Event)
	{
	case DeviceManager::DEVICE_ARRIVAL_EVT:
		m_ni.Event = MX_DEVICE_ARRIVAL_EVT;
		strDesc = pnsinfo->Device->_description.get();
		_tcscpy(m_ni.DeviceDesc, strDesc.GetBuffer());
		strDesc.ReleaseBuffer();
		break;
	case DeviceManager::DEVICE_REMOVAL_EVT:
		m_ni.Event = MX_DEVICE_REMOVAL_EVT;
		strDesc = pnsinfo->Device->_description.get();
		_tcscpy(m_ni.DeviceDesc, strDesc.GetBuffer());
		strDesc.ReleaseBuffer();
		break;
	case DeviceManager::VOLUME_ARRIVAL_EVT:
		m_ni.Event = MX_VOLUME_ARRIVAL_EVT;
		m_ni.DriverLetter = pnsinfo->DriverLetter;
		//strDesc = ((Volume*)(pnsinfo->Device))->_friendlyName.get();
		strDesc = _T("USB Mass Storage Devcie");
		_tcscpy(m_ni.DeviceDesc, strDesc.GetBuffer());
		strDesc.ReleaseBuffer();
		//QueryPerformanceFrequency(&g_tc);
		//QueryPerformanceCounter(&g_t1);
//		if(m_pUTP != NULL)
//		{
//			delete m_pUTP;
//			m_pUTP = NULL;
//		}
		//QueryPerformanceCounter(&g_t2);
		//d1 = (double)(g_t2.QuadPart-g_t1.QuadPart) / (double)g_tc.QuadPart;
		//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Delete UTP times(%f)"), d1);
		break;
	case DeviceManager::VOLUME_REMOVAL_EVT:
		m_ni.Event = MX_VOLUME_REMOVAL_EVT;
		m_ni.DriverLetter = _T('');
		//m_ni.DeviceDesc = ((Volume*)(pnsinfo->Device))->_friendlyName.get();
		strDesc = _T("USB Mass Storage Device");
		_tcscpy(m_ni.DeviceDesc, strDesc.GetBuffer());
		strDesc.ReleaseBuffer();
		break;
	case DeviceManager::HUB_ARRIVAL_EVT:
		m_ni.Event = MX_HUB_ARRIVAL_EVT;
		break;
	case DeviceManager::HUB_REMOVAL_EVT:
		m_ni.Event = MX_HUB_REMOVAL_EVT;
		break;
	}
	_tcscpy(m_ni.Hub, pnsinfo->Hub.GetBuffer());
	pnsinfo->Hub.ReleaseBuffer();
	m_ni.HubIndex = pnsinfo->HubIndex;
	m_ni.PortIndex = pnsinfo->PortIndex;
	m_ni.OperationID = ((MFGLIB_VARS *)m_pLibHandle)->g_CmdOpThreadID[pnsinfo->cmdOpIndex];
	m_ni.DriverLetter = pnsinfo->DriverLetter;

	if( (pnsinfo->Event == DeviceManager::DEVICE_ARRIVAL_EVT) || (pnsinfo->Event == DeviceManager::VOLUME_ARRIVAL_EVT) )
	{
		((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[m_WndIndex].m_bConnected = TRUE;
	}
	else if( (pnsinfo->Event == DeviceManager::DEVICE_REMOVAL_EVT) || (pnsinfo->Event == DeviceManager::VOLUME_REMOVAL_EVT) )
	{
		((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[m_WndIndex].m_bConnected = FALSE;
	}
	((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[m_WndIndex].DeviceDesc = m_ni.DeviceDesc;

	m_usb_hub_name = pnsinfo->Hub;
	m_usb_port_index = pnsinfo->PortIndex;
	SetUsbPort(FindPort());

	if( (pnsinfo->Event != DeviceManager::HUB_ARRIVAL_EVT) && (pnsinfo->Event != DeviceManager::HUB_REMOVAL_EVT) )
	{
		WaitForSingleObject(m_hMutex_cb, INFINITE);
		if( !m_callbacks.empty() )
		{
			for(; cbIt!=m_callbacks.end(); cbIt++)
			{
				(*cbIt).second->pfunc(&m_ni);
			}
		}
		ReleaseMutex(m_hMutex_cb);
	}

	switch(pnsinfo->Event)
	{
	case DeviceManager::VOLUME_ARRIVAL_EVT:
	case DeviceManager::VOLUME_REMOVAL_EVT:
    case DeviceManager::DEVICE_ARRIVAL_EVT:
    case DeviceManager::DEVICE_REMOVAL_EVT:
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOpreation[%d]--OnDeviceChangeNotify, Volume Arrive/Remove or Device Arrive/Remove"), m_WndIndex);
		TRACE(_T("OnDeviceChangeNotify\r\n"));
		if(m_p_usb_port != NULL)
        {
            TRACE(_T("OnDeviceChangeNotify11\r\n"));
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOpreation[%d]--OnDeviceChangeNotify, m_p_usb_port is not NULL, so only refresh"), m_WndIndex);
            m_p_usb_port->Refresh();
			TRACE(_T("OnDeviceChangeNotify12\r\n"));
			//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("OnDeviceChangeNotify12"));
        }
		if( (pnsinfo->Event == DeviceManager::VOLUME_ARRIVAL_EVT) || (pnsinfo->Event == DeviceManager::DEVICE_ARRIVAL_EVT) )
		{
			//QueryPerformanceCounter(&g_t2);
			TRACE(_T("OnDeviceChangeNotify3\r\n"));
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOpreation[%d]--OnDeviceChangeNotify, Volume/Device Arrive"), m_WndIndex);
			OnDeviceArrive();
		}
		else
		{
		    TRACE(_T("OnDeviceChangeNotify4\r\n"));
		    LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CmdOpreation[%d]--OnDeviceChangeNotify, Volume/Device Remove"), m_WndIndex);
			OnDeviceRemove();
		}
		break;
	case DeviceManager::HUB_ARRIVAL_EVT:
    case DeviceManager::HUB_REMOVAL_EVT:
		if ( FindPort() == NULL )
        {
            SetUsbPort( NULL );
            // it means the hub which our port attached to is removed, so we need stop all the image updater action
            if(m_bRun)
            {
                OnStop();
            }        
        }
		break;
	default:
		break;
	}
}

void gOnDeviceChangeNotify(INSTANCE_HANDLE handle, DeviceClass::NotifyStruct *pNsInfo)
{
	CCmdOpreation *pOperation = ((MFGLIB_VARS *)handle)->g_CmdOperationArray[pNsInfo->cmdOpIndex];

	pOperation->OnDeviceChangeNotify(pNsInfo);
}

void CCmdOpreation::ExecuteUIUpdate(UI_UPDATE_INFORMATION *pInfo)
{
	OperateResultUpdateStruct* pCallback;
	std::map<int, OperateResultUpdateStruct*>::iterator cbIt;

	//assign UI update information
	if(pInfo->bUpdateCommandsProgress)
	{
		m_uiInfo.cmdIndex = pInfo->CommandsProgressIndex;
		m_uiInfo.cmdStatus = pInfo->CommandStatus;
	}
	if(pInfo->bUpdateDescription)
	{
		_tcscpy(m_uiInfo.cmdInfo, pInfo->strDescription);
	}
	m_uiInfo.bProgressWithinCommand = pInfo->bUpdateProgressInCommand;
	if(m_uiInfo.bProgressWithinCommand)
	{
		m_uiInfo.DoneWithinCommand = pInfo->ProgressIndexInCommand;
		m_uiInfo.TotalWithinCommand = pInfo->ProgressRangeInCommand;
	}
	if(pInfo->bUpdatePhaseProgress)
	{
		m_uiInfo.CurrentPhaseIndex = pInfo->CurrentPhaseIndex;
		//m_uiInfo.phaseStatus = pInfo->PhaseStatus;
	}
	m_uiInfo.OperationID = pInfo->OperationID;

	WaitForSingleObject(m_hMutex_cb2, INFINITE);
	if( !m_callbacks2.empty() )
	{
		for(cbIt=m_callbacks2.begin(); cbIt!=m_callbacks2.end(); cbIt++)
		{
			pCallback = (*cbIt).second;
			if(pInfo->OperationID == pCallback->OperationID)
			{
				pCallback->pfunc(&m_uiInfo);
			}
		}
	}
	ReleaseMutex(m_hMutex_cb2);
}
