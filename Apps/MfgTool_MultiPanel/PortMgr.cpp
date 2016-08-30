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

#include "stdafx.h"
#include "PortMgr.h"
#include "MfgTool_MultiPanel.h"
#include "MfgTool_MultiPanelDlg.h"
#include "FslConsole.h"


CPortMgr::CPortMgr(int id)
{
	m_bAutoDelete = FALSE; //Don't destroy the object at thread termination
	m_Index = id;
	m_AllCmdSize = 0;
	m_hThreadStartEvent = NULL;
}

CPortMgr::~CPortMgr()
{
}

BOOL CPortMgr::Open()
{
	m_hThreadStartEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if(m_hThreadStartEvent == NULL)
	{
		CString strMsg;
		strMsg.Format(_T("CPortMgr(%d)::Open---Create m_hThreadStartEvent error."), m_Index);
		OutputInformation(strMsg);
		return FALSE;
	}

	if( CreateThread() != 0 )
	{
		::WaitForSingleObject(m_hThreadStartEvent, INFINITE);
	}
	::CloseHandle(m_hThreadStartEvent);
	m_hThreadStartEvent = NULL;

	return TRUE;
}

void CPortMgr::Close()
{
}

BOOL CPortMgr::InitInstance()
{
	m_AllPhaseNumbers = theApp.m_PhasesInformation.PhaseInfoNumbers;
	m_AllCmdSize = 0;
	for(UINT i=0; i<m_AllPhaseNumbers; i++)
	{
		m_AllCmdSize += theApp.m_PhasesInformation.pPhaseInfo[i].uiPhaseCommandNumbers;
	}

	m_PreviousPhaseIndex = -1;

/*	// Register callbacks
	m_devChangeCallback.DeviceIndex = m_Index;
	m_devChangeCallback.pfunc = gDeviceChangeNotify;
	DWORD error = MfgLib_RegisterCallbackFunction(theApp.m_pLibHandle, DeviceChange, &m_devChangeCallback, &m_hDevChangeCallback);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		CString strMsg;
		strMsg.Format(_T("Register %s callback function failed"), _T("Device Change"));
		OutputInformation(strMsg);
		SetEvent(m_hThreadStartEvent);
		return FALSE;
	}

	m_UIUpdateCallback.DeviceIndex = m_Index;
	m_UIUpdateCallback.pfunc = gProgressUpdate;
	error = MfgLib_RegisterCallbackFunction(theApp.m_pLibHandle, OperateResult, &m_UIUpdateCallback, &m_hUIUpdateCallback);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		CString strMsg;
		strMsg.Format(_T("Register %s callback function failed"), _T("Operate Result"));
		OutputInformation(strMsg);
		SetEvent(m_hThreadStartEvent);
		return FALSE;
	} */

	SetEvent(m_hThreadStartEvent);

	return TRUE;
}

int CPortMgr::ExitInstance()
{
	return CWinThread::ExitInstance();
}

void CPortMgr::StartDownload()
{
	MfgLib_StartOperation(theApp.m_pLibHandle, theApp.m_OperationsInformation.pOperationInfo[m_Index].OperationID);
	if(g_bConsoleApp)
	{
	}
	else
	{
		g_pMainDlg->m_PortMgrDlg_Array[m_Index]->PostMessage(UM_PORT_DLG_MSG, (WPARAM)OP_CMD_START, 0);
	}
}

void CPortMgr::StopDownload()
{
	MfgLib_StopOperation(theApp.m_pLibHandle, theApp.m_OperationsInformation.pOperationInfo[m_Index].OperationID);
	if(g_bConsoleApp)
	{
	}
	else
	{
		g_pMainDlg->m_PortMgrDlg_Array[m_Index]->PostMessage(UM_PORT_DLG_MSG, (WPARAM)OP_CMD_STOP, 0);
	}
}

TCHAR* CPortMgr::GetCurrentDeviceDesc()
{
	MfgLib_GetOperationInformation(theApp.m_pLibHandle, &(theApp.m_OperationsInformation));
	return theApp.m_OperationsInformation.pOperationInfo[m_Index].DeviceDesc;
}

void gDeviceChangeNotify(DEVICE_CHANGE_NOTIFY *pnsinfo)
{
	CPortMgr::DEV_CHG_NOTIFY notifyInfo;

	switch(pnsinfo->Event)
	{
	case MX_DEVICE_ARRIVAL_EVT:
	case MX_VOLUME_ARRIVAL_EVT:
	case MX_HUB_ARRIVAL_EVT:
		notifyInfo.bDeviceConnected = TRUE;
		break;
	case MX_DEVICE_REMOVAL_EVT:
	case MX_VOLUME_REMOVAL_EVT:
	case MX_HUB_REMOVAL_EVT:
		notifyInfo.bDeviceConnected = FALSE;
		break;
	}
	notifyInfo.DriverLetter = pnsinfo->DriverLetter;
	notifyInfo.Hub = pnsinfo->Hub;
	notifyInfo.HubIndex = pnsinfo->HubIndex;
	notifyInfo.PortIndex = pnsinfo->PortIndex;
	notifyInfo.DeviceDesc = pnsinfo->DeviceDesc;
	notifyInfo.Event = pnsinfo->Event;

	int DeviceIndex = theApp.FindDeviceIndex(pnsinfo->OperationID);
	if(g_bConsoleApp)
	{
		if(!notifyInfo.bDeviceConnected)
		{
			CString strMsg;
			strMsg.Format(_T("Device %d - Waiting for device connect......"), DeviceIndex);
			MSG_TYPE msgType = (MSG_TYPE)((int)DEVICE1_UPDATE_PERCENT + DeviceIndex);
			MSG_CURSOR_POSITION *pMsgPos = NULL;
			std::vector<MSG_CURSOR_POSITION*>::iterator it = g_VolatileMsgPosArray.begin();
			for(; it!=g_VolatileMsgPosArray.end(); it++)
			{
				pMsgPos = (*it);
				if(pMsgPos->type == msgType)
				{
					break;
				}
			}
			if(pMsgPos != NULL)
			{
				BSTR bstr_msg = strMsg.AllocSysString();
				::PostThreadMessage(g_pConsole->m_nThreadID, UM_MODIFY_LINE, (WPARAM)pMsgPos, (LPARAM)bstr_msg);
			}

			CPortMgr *pPortMgr = (CPortMgr *)(theApp.m_PortMgr_Array.GetAt(DeviceIndex));
			pMsgPos = NULL;
			msgType = (MSG_TYPE)((int)DEVICE1_DESCRIPTION + DeviceIndex);
			strMsg.Format(_T("Device %d[Hub %d--Port %d]: No Device Connected"), (DeviceIndex+1), notifyInfo.HubIndex, notifyInfo.PortIndex);
			for(it = g_VolatileMsgPosArray.begin(); it!=g_VolatileMsgPosArray.end(); it++)
			{
				pMsgPos = (*it);
				if(pMsgPos->type == msgType)
				{
					break;
				}
			}
			if(pMsgPos != NULL)
			{
				BSTR bstr_msg = strMsg.AllocSysString();
				::PostThreadMessage(g_pConsole->m_nThreadID, UM_MODIFY_LINE, (WPARAM)pMsgPos, (LPARAM)bstr_msg);
			}
		}
		else
		{
			CString strMsg;
			CString strDevDesc;
			CPortMgr *pPortMgr = (CPortMgr *)(theApp.m_PortMgr_Array.GetAt(DeviceIndex));
			MSG_CURSOR_POSITION *pMsgPos = NULL;
			MSG_TYPE msgType = (MSG_TYPE)((int)DEVICE1_DESCRIPTION + DeviceIndex);
			TCHAR *devDesc;
			devDesc = pPortMgr->GetCurrentDeviceDesc();
			strDevDesc = devDesc;
			strMsg.Format(_T("Device %d[Hub %d--Port %d]: %s"), (DeviceIndex+1), notifyInfo.HubIndex, notifyInfo.PortIndex, strDevDesc);
			std::vector<MSG_CURSOR_POSITION*>::iterator it = g_VolatileMsgPosArray.begin();
			for(it = g_VolatileMsgPosArray.begin(); it!=g_VolatileMsgPosArray.end(); it++)
			{
				pMsgPos = (*it);
				if(pMsgPos->type == msgType)
				{
					break;
				}
			}
			if(pMsgPos != NULL)
			{
				BSTR bstr_msg = strMsg.AllocSysString();
				::PostThreadMessage(g_pConsole->m_nThreadID, UM_MODIFY_LINE, (WPARAM)pMsgPos, (LPARAM)bstr_msg);
			}
		}
	}
	else
	{
		g_pMainDlg->m_PortMgrDlg_Array[DeviceIndex]->SendMessage(UM_PORT_DEVICE_CHANGE, (WPARAM)(&notifyInfo), 0);
	}
}

void gProgressUpdate(OPERATE_RESULT *puiInfo)
{
	OPERATE_RESULT ProgressInfo;

	ProgressInfo.bProgressWithinCommand = puiInfo->bProgressWithinCommand;
	ProgressInfo.cmdIndex = puiInfo->cmdIndex;
	ProgressInfo.cmdStatus = puiInfo->cmdStatus;
	_tcscpy(ProgressInfo.cmdInfo, puiInfo->cmdInfo);
	ProgressInfo.DoneWithinCommand = puiInfo->DoneWithinCommand;
	ProgressInfo.TotalWithinCommand = puiInfo->TotalWithinCommand;
	ProgressInfo.OperationID = puiInfo->OperationID;
	ProgressInfo.CurrentPhaseIndex = puiInfo->CurrentPhaseIndex;
	//ProgressInfo.phaseStatus = puiInfo->phaseStatus;

	int DeviceIndex = theApp.FindDeviceIndex(ProgressInfo.OperationID);
	if(g_bConsoleApp)
	{
		CPortMgr *pPortMgr = theApp.m_PortMgr_Array.GetAt(DeviceIndex);
//		if((pPortMgr->m_PreviousPhaseIndex != puiInfo->CurrentPhaseIndex) || (puiInfo->bUpdateProgressInCommand))
//		{
//			pPortMgr->m_PreviousPhaseIndex = puiInfo->CurrentPhaseIndex;
//			return;
//		}
		CString strMsg;
		CString strPhase;
		UINT uiPhaseCmdSize = 0;
		switch(ProgressInfo.CurrentPhaseIndex)
		{
		case 0:
			strPhase = _T("Bootstrap phase: ");
			break;
		case 1:
			strPhase = _T("Updater phase: ");
			break;
		}
		uiPhaseCmdSize = theApp.GetStateCommandSize(ProgressInfo.CurrentPhaseIndex);
		//if(ProgressInfo.CommandsProgressIndex == OPERATE_COMPLETE)
		if( (ProgressInfo.cmdStatus == COMMAND_STATUS_EXECUTE_COMPLETE) &&
			(ProgressInfo.cmdIndex == theApp.m_PhasesInformation.pPhaseInfo[theApp.m_PhasesInformation.PhaseInfoNumbers-1].uiPhaseCommandNumbers) )
		{
			g_successOps++;
			strMsg.Format(_T("Device %d - All has completed successfully"), (DeviceIndex+1));
		}
		//else if(ProgressInfo.CommandsProgressIndex == OPERATE_ERROR)
		else if(ProgressInfo.cmdStatus == COMMAND_STATUS_EXECUTE_ERROR)
		{
			g_failedOps++;
			strMsg.Format(_T("Device %d - %sFailed"), (DeviceIndex+1), strPhase);
		}
		else
		{
			double dPercent = (double)(ProgressInfo.cmdIndex) / uiPhaseCmdSize;
			int iPercent = (int)(dPercent * 100);
			strMsg.Format(_T("Device %d - %s%d%%"), (DeviceIndex+1), strPhase, iPercent);
		}
		MSG_TYPE msgType = (MSG_TYPE)((int)DEVICE1_UPDATE_PERCENT + DeviceIndex);
		MSG_CURSOR_POSITION *pMsgPos = NULL;
		std::vector<MSG_CURSOR_POSITION*>::iterator it = g_VolatileMsgPosArray.begin();
		for(; it!=g_VolatileMsgPosArray.end(); it++)
		{
			pMsgPos = (*it);
			if(pMsgPos->type == msgType)
			{
				break;
			}
		}
		if(pMsgPos != NULL)
		{
			BSTR bstr_msg = strMsg.AllocSysString();
			::PostThreadMessage(g_pConsole->m_nThreadID, UM_MODIFY_LINE, (WPARAM)pMsgPos, (LPARAM)bstr_msg);
		}

		//if((ProgressInfo.CommandsProgressIndex == OPERATE_COMPLETE) || (ProgressInfo.CommandsProgressIndex == OPERATE_ERROR))
		if( ((ProgressInfo.cmdStatus == COMMAND_STATUS_EXECUTE_COMPLETE) &&
			(ProgressInfo.cmdIndex == theApp.m_PhasesInformation.pPhaseInfo[theApp.m_PhasesInformation.PhaseInfoNumbers-1].uiPhaseCommandNumbers)) ||
			(ProgressInfo.cmdStatus == COMMAND_STATUS_EXECUTE_ERROR) )
		{
			strMsg.Format(_T("success: %d -- failed: %d"), g_successOps, g_failedOps);
			msgType = SUCCESS_AND_FAIL_COUNT;
			pMsgPos = NULL;
			it = g_VolatileMsgPosArray.begin();
			for(; it!=g_VolatileMsgPosArray.end(); it++)
			{
				pMsgPos = (*it);
				if(pMsgPos->type == msgType)
				{
					break;
				}
			}
			if(pMsgPos != NULL)
			{
				BSTR bstr_msg = strMsg.AllocSysString();
				::PostThreadMessage(g_pConsole->m_nThreadID, UM_MODIFY_LINE, (WPARAM)pMsgPos, (LPARAM)bstr_msg);
				Sleep(0);
			}
		}
	}
	else
	{
		g_pMainDlg->m_PortMgrDlg_Array[DeviceIndex]->SendMessage(UM_PORT_UPDATE_INFO, (WPARAM)(&ProgressInfo), 0);
	}
}