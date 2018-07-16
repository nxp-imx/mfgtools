/*
 * Copyright 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
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

// MfgToolLib.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "MfgToolLib.h"
#include "LogMgr.h"
#include "WindowsVersionInfo.h"
#include "DeviceManager.h"
#include "UsbHub.h"
#include "HubClass.h"
#include "MxHidDevice.h"
#include "HidDevice.h"
#include "CDCDevice.h"
#include "StPitc.h"
#include "version.h"
#include "ControllerClass.h"
#include "HubClass.h"
#include "MxRomDeviceClass.h"
#include "HidDeviceClass.h"
#include "MxHidDeviceClass.h"
#include "KblHidDeviceClass.h"
#include "DiskDeviceClass.h"
#include "VolumeDeviceClass.h"
#include "CdcDeviceClass.h"

#include <algorithm>
#include <map>
#include <stdexcept>  // std::out_of_range
#include <tuple>
//#include "..\MfgTool.exe\gitversion.h"
#include "UpdateUIInfo.h"
#include "gitversion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//HANDLE g_hDevCanDeleteEvts[MAX_BOARD_NUMBERS];
CString g_strVersion;


static MxHidDeviceClass g_MxHidDeviceClass(NULL);
static MxRomDeviceClass g_MxRomDeviceClass(NULL);
static HidDeviceClass	g_HidDeviceClass(NULL);
static CDCDeviceClass	g_cdcDeviceClass(NULL);
static KblHidDeviceClass g_kblHIdDeviceClass(NULL);


ROM_INFO g_RomInfo []=
{
	{_T("MX23"),	0x00000000, &g_HidDeviceClass,		ROM_INFO_HID | ROM_INFO_HID_MX23},
	{_T("MX50"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX50},
	{_T("MX6Q"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6},
	{_T("MX6D"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 },
	{_T("MX6SL"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 },
	{_T("K32H844P"),0x00008000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6},
	{_T("MX7D"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD},
	{_T("MX6UL"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{_T("MX6ULL"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{_T("MX6SLL"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{_T("MX8MQ"),	0x00910000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{_T("MX7ULP"),	0x2f018000, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{_T("MX8QM"),	0x2000e400, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD | ROM_INFO_HID_MX8_MULTI_IMAGE | ROM_INFO_HID_SYSTEM_ADDR_MAP| ROM_INFO_HID_ECC_ALIGN },
	{_T("MX8QXP"),  0x2000e400, &g_MxHidDeviceClass,	ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD | ROM_INFO_HID_MX8_MULTI_IMAGE | ROM_INFO_HID_ECC_ALIGN },
	{_T("MX8QXPB0"), 0x0,		&g_HidDeviceClass,		ROM_INFO_HID | ROM_INFO_HID_NO_CMD | ROM_INFO_HID_UID_STRING },
};

static ROM_INFO * SearchCompatiableRom(CString str)
{
	for (int i = 0; i < sizeof(g_RomInfo) / sizeof(ROM_INFO); i++)
	{
		if (str.CompareNoCase(g_RomInfo[i].Name) == 0)
			return g_RomInfo + i;
	}
	return NULL;
}
//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CMfgToolLibApp

BEGIN_MESSAGE_MAP(CMfgToolLibApp, CWinApp)
END_MESSAGE_MAP()


// CMfgToolLibApp construction

CMfgToolLibApp::CMfgToolLibApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CMfgToolLibApp object

CMfgToolLibApp theApp;

/************************************************************
* Global variables definition
************************************************************/
CMfgLogMgr *g_pLogMgr;
/*
OP_STATE_ARRAY g_OpStates;
CFG_PARAMETER g_CfgParam;
CUclXml* g_pXmlHandler;
CString g_strPath;
CString g_strUclFilename;
int g_iMaxBoardNum;
std::vector<CCmdOpreation*> g_CmdOperationArray(MAX_BOARD_NUMBERS);
StateCommansMap_t g_StateCommands;
PORT_DEV_INFO g_PortDevInfoArray[MAX_BOARD_NUMBERS];
*/
std::vector<MFGLIB_VARS *> g_LibVarsArray;
//std::vector<USB_PORT_NODE *> g_PortTable;
std::map<CString, CString> g_UclKeywords;
typedef std::tuple<UINT, UINT, CString> UsbPortKey;
std::map<UsbPortKey, CString> g_UsbPortKeywords;

HANDLE g_hOneInstance;
#define UNIQE_NAME	_T("{1AB792D6-EAF2-3267-9A84-9135681127A4}")
#define BLHOST_PATH _T("..\\blhost\\win\\blhost.exe")

// CMfgToolLibApp initialization

BOOL CMfgToolLibApp::InitInstance()
{
	CWinApp::InitInstance();

	TCHAR _path[MAX_PATH] = {0};
	::GetModuleFileName(AfxGetStaticModuleState()->m_hCurrentInstanceHandle, _path, MAX_PATH);

	theApp.m_strDllFullPath = _path;
	int pos = theApp.m_strDllFullPath.ReverseFind(_T('\\'));
	theApp.m_strDllFullPath = theApp.m_strDllFullPath.Left(pos+1);  //+1 for add '\' at the last

	g_pLogMgr = NULL;
	g_hOneInstance = NULL;

	g_strVersion.Format(_T("DLL1 version: %d.%d%s"), FSLMFGTOOL_MAJOR_VERSION, FSLMFGTOOL_MINOR_VERSION, _T(GIT_VERSION));

	return TRUE;
}

/************************************************************
* Export Functions implementation
************************************************************/
DWORD MfgLib_Initialize()
{
	// Only an instance
	g_hOneInstance = ::CreateMutex(NULL, FALSE, UNIQE_NAME);
	if(g_hOneInstance == NULL)
	{
		return MFGLIB_ERROR_NO_MEMORY;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS )
	{
		return MFGLIB_ERROR_ALREADY_INITIALIZED;
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_Uninitialize()
{
	if(g_hOneInstance != NULL)
	{
		CloseHandle(g_hOneInstance);
		g_hOneInstance = NULL;
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_CreateInstanceHandle(INSTANCE_HANDLE *pHandle)
{
	if(g_hOneInstance == NULL)
	{
		return MFGLIB_ERROR_NOT_INITIALIZED;
	}

	MFGLIB_VARS *pLibVars = NULL;

	pLibVars = new MFGLIB_VARS;
	if(pLibVars == NULL)
	{
		return MFGLIB_ERROR_NO_MEMORY;
	}
	else
	{
		pLibVars->g_pXmlHandler = NULL;
		for(int i=0; i<MAX_BOARD_NUMBERS; i++)
		{
			pLibVars->g_CmdOperationArray[i] = NULL;
			pLibVars->g_PortDevInfoArray[i].m_bUsed = FALSE;
			pLibVars->g_PortDevInfoArray[i].m_bConnected = FALSE;
			pLibVars->g_PortDevInfoArray[i].hubPath = _T("");
			pLibVars->g_PortDevInfoArray[i].portIndex = 0;
			pLibVars->g_PortDevInfoArray[i].DeviceDesc = _T("");
			pLibVars->g_PortDevInfoArray[i].hubIndex = 0;
			pLibVars->g_cbDevChangeHandle[i] = NULL;
			pLibVars->g_cbOpResultHandle[i] = NULL;
		}
		pLibVars->g_CfgParam.chip = _T("");
		pLibVars->g_CfgParam.list = _T("");
		pLibVars->g_iMaxBoardNum = MAX_BOARD_NUMBERS;

		g_LibVarsArray.push_back(pLibVars);

		*pHandle = (INSTANCE_HANDLE)pLibVars;

		return MFGLIB_ERROR_SUCCESS;
	}
}

DWORD MfgLib_DestoryInstanceHandle(INSTANCE_HANDLE handle)
{
	MFGLIB_VARS *pLibVars = NULL;

	pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	else
	{
		BOOL bFind = FindLibraryHandle(pLibVars);
		if(!bFind)
		{
			return MFGLIB_ERROR_NOT_FIND;
		}
		std::vector<MFGLIB_VARS *>::iterator it;
		it = std::find(g_LibVarsArray.begin(), g_LibVarsArray.end(), pLibVars);
		delete pLibVars;
		g_LibVarsArray.erase(it);
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_SetProfileName(INSTANCE_HANDLE handle, BYTE_t *strName)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	if(strName == NULL)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	pLibVars->g_CfgParam.chip = strName;
	pLibVars->g_strUclFilename = theApp.m_strDllFullPath + _T("Profiles") + _T("\\") + pLibVars->g_CfgParam.chip + _T("\\") + _T("OS Firmware") + _T("\\") + DEFAULT_UCL_XML_FILE_NAME;

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_SetListName(INSTANCE_HANDLE handle, BYTE_t *strName)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	if(strName == NULL)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	pLibVars->g_CfgParam.list = strName;

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_SetUCLFile(INSTANCE_HANDLE handle, BYTE_t *strName)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	if(strName == NULL)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	pLibVars->g_strUclFilename = theApp.m_strDllFullPath + _T("Profiles") + _T("\\") + pLibVars->g_CfgParam.chip + _T("\\") + _T("OS Firmware") + _T("\\") + strName;
	//Open Ucl.xml
	CFile UclXmlFile;
	if( !UclXmlFile.Open(pLibVars->g_strUclFilename, CFile::modeReadWrite, NULL) )
    {
		return MFGLIB_ERROR_FILE_NOT_EXIST;
    }
	UclXmlFile.Close();

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_SetMaxBoardNumber(INSTANCE_HANDLE handle, int boardNum)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	if( (boardNum<=0) || (boardNum>MAX_BOARD_NUMBERS) )
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	pLibVars->g_iMaxBoardNum = boardNum;

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_InitializeOperation(INSTANCE_HANDLE handle)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	DWORD error = MFGLIB_ERROR_SUCCESS;
	//Initialize log
	error = InitLogManager();
	if( (error != MFGLIB_ERROR_SUCCESS) && (error != MFGLIB_ERROR_ALREADY_EXIST) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Initialize log manager failed, error code: %d"), error);
		goto ERROR_END;
	}
	//Parse xml
	error = ParseUclXml(pLibVars);
	if( (error != MFGLIB_ERROR_SUCCESS) && (error != MFGLIB_ERROR_ALREADY_EXIST) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Parse ucl script failed, error code: %d"), error);
		goto ERROR_END;
	}
	//Initialize Device Manager
	error = InitDeviceManager(pLibVars);
	if( (error != MFGLIB_ERROR_SUCCESS) && (error != MFGLIB_ERROR_ALREADY_EXIST) )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Initialize device manager failed, error code: %d"), error);
		goto ERROR_END;
	}
	//scan devices
	AutoScanDevice(pLibVars, pLibVars->g_iMaxBoardNum);
	//Initialize command operation object
	for(int i=0; i<pLibVars->g_iMaxBoardNum; i++)
	{
		error = InitCmdOperation(pLibVars, i);
		if(error != MFGLIB_ERROR_SUCCESS)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Initialize command operation failed, error code: %d"), error);
			goto ERROR_END;
		}
	}

	return MFGLIB_ERROR_SUCCESS;

ERROR_END:
	MfgLib_UninitializeOperation(handle);
	return error;
}

DWORD MfgLib_UninitializeOperation(INSTANCE_HANDLE handle)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	for(int i=0; i<pLibVars->g_iMaxBoardNum; i++)
	{
		DeinitCmdOperation(pLibVars, i);
	}
	DeinitDeviceManager();
	ReleaseUclCommands(pLibVars);
	DeinitLogManager();

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_StartOperation(INSTANCE_HANDLE handle, DWORD OperationID)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	int DeviceIndex;
/*	USB_PORT_NODE *pPortNode = NULL;
	pPortNode = FindPortPhysicalNode(PortID);
	if(pPortNode == NULL)
	{
		return MFGLIB_ERROR_INVALID_PORT_ID;
	}
	for(int i=0; i<pLibVars->g_iMaxBoardNum; i++)
	{
		if( (pLibVars->g_PortDevInfoArray[i].hubPath.CompareNoCase(pPortNode->hubPath) == 0)
			&& (pLibVars->g_PortDevInfoArray[i].portIndex == pPortNode->portIndex) )
		{
			DeviceIndex = i;
			break;
		}
	}
	if(DeviceIndex >= pLibVars->g_iMaxBoardNum)
	{
		return MFGLIB_ERROR_INVALID_PORT_ID;
	}
*/
	DeviceIndex = FindOperationIndex(pLibVars, OperationID);
	if(DeviceIndex >= pLibVars->g_iMaxBoardNum)
	{
		return MFGLIB_ERROR_INVALID_OPERATION_ID;
	}

	if(pLibVars->g_CmdOperationArray[DeviceIndex] != NULL)
	{
		BOOL bret = pLibVars->g_CmdOperationArray[DeviceIndex]->OnStart();
		if(!bret)
		{
			return MFGLIB_ERROR_CONDITION_NOT_READY;
		}
	}
	else
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CmdOperation(%d) is not initialized before starting"), DeviceIndex);
		return MFGLIB_ERROR_INVALID_VALUE;
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_StopOperation(INSTANCE_HANDLE handle, DWORD OperationID)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	int DeviceIndex = -1;
/*	USB_PORT_NODE *pPortNode = NULL;
	pPortNode = FindPortPhysicalNode(PortID);
	if(pPortNode == NULL)
	{
		return MFGLIB_ERROR_INVALID_PORT_ID;
	}
	for(int i=0; i<pLibVars->g_iMaxBoardNum; i++)
	{
		if( (pLibVars->g_PortDevInfoArray[i].hubPath.CompareNoCase(pPortNode->hubPath) == 0)
			&& (pLibVars->g_PortDevInfoArray[i].portIndex == pPortNode->portIndex) )
		{
			DeviceIndex = i;
			break;
		}
	}
	if(DeviceIndex >= pLibVars->g_iMaxBoardNum)
	{
		return MFGLIB_ERROR_INVALID_PORT_ID;
	} */

	DeviceIndex = FindOperationIndex(pLibVars, OperationID);
	if(DeviceIndex >= pLibVars->g_iMaxBoardNum)
	{
		return MFGLIB_ERROR_INVALID_OPERATION_ID;
	}

	if(pLibVars->g_CmdOperationArray[DeviceIndex] != NULL)
	{
		BOOL bret = pLibVars->g_CmdOperationArray[DeviceIndex]->OnStop();
		if(!bret)
		{
			return MFGLIB_ERROR_CONDITION_NOT_READY;
		}
	}
	else
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("CmdOperation(%d) is not initialized before stopping"), DeviceIndex);
		return MFGLIB_ERROR_INVALID_VALUE;
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_GetOperationInformation(INSTANCE_HANDLE handle, OPERATIONS_INFORMATION *pOperationsInfo)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	if( (pOperationsInfo == NULL) || (pOperationsInfo->pOperationInfo == NULL) )
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	pOperationsInfo->OperationInfoNumbers = pLibVars->g_iMaxBoardNum;

	OPERATION_INFOMATION *pInfo = pOperationsInfo->pOperationInfo;
	for(int i=0; i<pLibVars->g_iMaxBoardNum; i++)
	{
		if(pLibVars->g_CmdOperationArray[i] == NULL)
		{
			return MFGLIB_ERROR_INVALID_VALUE;
		}

		pInfo->bConnected = pLibVars->g_PortDevInfoArray[i].m_bConnected;
		pInfo->PortIndex = pLibVars->g_PortDevInfoArray[i].portIndex;
		_tcscpy(pInfo->HubName, pLibVars->g_PortDevInfoArray[i].hubPath.GetBuffer());
		pLibVars->g_PortDevInfoArray[i].hubPath.ReleaseBuffer();
		//_tcscpy(pInfo->DeviceDesc, g_PortDevInfoArray[i].DeviceDesc.GetBuffer());
		//g_PortDevInfoArray[i].DeviceDesc.ReleaseBuffer();
		GetCurrentDeviceDesc(pLibVars, i, pInfo->DeviceDesc, MAX_CHAR_NUMBERS);
		pInfo->HubIndex = pLibVars->g_PortDevInfoArray[i].hubIndex;
		if(pInfo->bConnected)
		{
			pInfo->ConnectedDeviceState = pLibVars->g_CmdOperationArray[i]->GetDeviceState();
		}
		else
		{
			pInfo->ConnectedDeviceState = MX_DISCONNECTED;
		}

/*		USB_PORT_NODE portphynode;
		portphynode.hubPath = pInfo->HubName;
		portphynode.hubIndex = pInfo->HubIndex;
		portphynode.portIndex = pInfo->PortIndex;
		PORT_ID portID = FindPortLogicalID(&portphynode);

		pInfo->PortID = portID;
*/
		pInfo->OperationID = pLibVars->g_CmdOpThreadID[i];
		pInfo = (OPERATION_INFOMATION *)((UCHAR *)(pOperationsInfo->pOperationInfo) + (i+1) * sizeof(OPERATION_INFOMATION));
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_GetPhaseInformation(INSTANCE_HANDLE handle, PHASES_INFORMATION *pPhasesInfo, int MaxInfos)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	if( (pPhasesInfo == NULL) || (pPhasesInfo->pPhaseInfo == NULL) )
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	OP_STATE_ARRAY *pOpStates = GetOpStates(pLibVars);
	if(pOpStates == NULL)
	{
		return MFGLIB_ERROR_PHASE_NOT_PARSED;
	}

	if (pOpStates->size() > MaxInfos)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	pPhasesInfo->PhaseInfoNumbers = pOpStates->size();
	COpState *pPhase = NULL;
	PHASE_INFORMATION *pInfo = pPhasesInfo->pPhaseInfo;
	for(int i=0; i<(int)(pOpStates->size()); i++)
	{
		pPhase = pOpStates->at(i);
		_tcscpy(pInfo->szName, pPhase->strStateName.GetBuffer());
		pPhase->strStateName.ReleaseBuffer();
		//pInfo->relativedState = pPhase->opState;
		pInfo->index = (int)(pPhase->opState);
		pInfo->uiVid = pPhase->uiVid;
		pInfo->uiPid = pPhase->uiPid;
		pInfo->uiPhaseCommandNumbers = GetStateCommandSize(pLibVars, pPhase->opState);

		pInfo = (PHASE_INFORMATION *)((UCHAR *)(pPhasesInfo->pPhaseInfo) + (i+1) * sizeof(PHASE_INFORMATION));
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_GetTotalCommandNumbers(INSTANCE_HANDLE handle, UINT *Number)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	if(Number == NULL)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	StateCommansMap_t::iterator it = pLibVars->g_StateCommands.begin();
	UINT cmdSize = 0;
	for(; it!=pLibVars->g_StateCommands.end(); it++)
	{
		OP_COMMAND_ARRAY _opCmds = (*it).second;
		cmdSize += _opCmds.size();
	}

	*Number = cmdSize;

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_RegisterCallbackFunction(INSTANCE_HANDLE handle, CALLBACK_TYPE cbType, void *pFunc)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	if(pFunc == NULL)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	int i = 0;
	void *pCallback = NULL;
	switch(cbType)
	{
	case DeviceChange:
		for(i=0; i<pLibVars->g_iMaxBoardNum; i++)
		{
			pCallback = new DeviceChangeCallbackStruct;
			((DeviceChangeCallbackStruct *)pCallback)->OperationID = pLibVars->g_CmdOpThreadID[i];
			((DeviceChangeCallbackStruct *)pCallback)->pfunc = (PCALLBACK_DEVICE_CHANGE)pFunc;
			RegisterUIDevChangeCallback((MFGLIB_VARS *)handle, (DeviceChangeCallbackStruct *)pCallback);
			pLibVars->g_cbDevChangeHandle[i] = pCallback;
		}
		break;
	case OperateResult:
		for(i=0; i<pLibVars->g_iMaxBoardNum; i++)
		{
			pCallback = new OperateResultUpdateStruct;
			((OperateResultUpdateStruct *)pCallback)->OperationID = pLibVars->g_CmdOpThreadID[i];
			((OperateResultUpdateStruct *)pCallback)->pfunc = (PCALLBACK_OPERATE_RESULT)pFunc;
			RegisterUIInfoUpdateCallback((MFGLIB_VARS *)handle, (OperateResultUpdateStruct *)pCallback);
			pLibVars->g_cbOpResultHandle[i] = pCallback;
		}
		break;
	default:
		return MFGLIB_ERROR_CALLBACK_INVALID_TYPE;
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_UnregisterCallbackFunction(INSTANCE_HANDLE handle, CALLBACK_TYPE cbType, void *pData)
{
	MFGLIB_VARS *pLibVars = (MFGLIB_VARS *)handle;
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}
	BOOL bFind = FindLibraryHandle(pLibVars);
	if(!bFind)
	{
		return MFGLIB_ERROR_NOT_FIND;
	}

	int i=0;
	switch(cbType)
	{
	case DeviceChange:
		for(i=0; i<pLibVars->g_iMaxBoardNum; i++)
		{
			UnregisterUIDevChangeCallback(pLibVars, i);
			if(pLibVars->g_cbDevChangeHandle[i] != NULL)
			{
				delete pLibVars->g_cbDevChangeHandle[i];
			}
		}
		
		break;
	case OperateResult:
		for(i=0; i<pLibVars->g_iMaxBoardNum; i++)
		{
			UnregisterUIInfoUpdateCallback(pLibVars, i);
			if(pLibVars->g_cbOpResultHandle[i] != NULL)
			{
				delete pLibVars->g_cbOpResultHandle[i];
			}
		}
		break;
	default:
		return MFGLIB_ERROR_CALLBACK_INVALID_TYPE;
	}

	return MFGLIB_ERROR_SUCCESS;
}

DWORD MfgLib_GetLibraryVersion(BYTE_t* version, int maxSize)
{
	CString strTemp;
	strTemp.Format(_T("Lib: %d.%d%s"), FSLMFGTOOL_MAJOR_VERSION, FSLMFGTOOL_MINOR_VERSION, _T(GIT_VERSION));
	if(maxSize < strTemp.GetLength())
	{
		return MFGLIB_ERROR_SIZE_IS_SMALL;
	}
	_tcscpy(version, strTemp.GetBuffer());
	strTemp.ReleaseBuffer();

	return MFGLIB_ERROR_SUCCESS;
}

/************************************************************
* Internal Functions implementation
************************************************************/
DWORD InitLogManager()
{
	if(g_pLogMgr != NULL)
	{
		return MFGLIB_ERROR_ALREADY_EXIST;
	}

	try
	{
		g_pLogMgr = new CMfgLogMgr;
	}
	catch(int &val)
	{
		TRACE(_T("open log file failed(%d)\n"), val);
		val = 0;
		return MFGLIB_ERROR_LOG_FILE_OPEN_FAILED;
	}
	catch(...)
	{
		return MFGLIB_ERROR_NO_MEMORY;
	}
	
	if(NULL == g_pLogMgr)
	{
		return MFGLIB_ERROR_NO_MEMORY;
	}

	return MFGLIB_ERROR_SUCCESS;
}

void DeinitLogManager()
{
	if(g_pLogMgr != NULL)
	{
		delete g_pLogMgr;
	}
}

void LogMsg(DWORD moduleID, DWORD levelID, const wchar_t * format, ... )
{
	TCHAR* buffer;
    va_list args;
    int len;

    va_start(args, format);
    len = _vsctprintf(format, args)+1;
    buffer = (TCHAR*)malloc(len*sizeof(TCHAR));
    std::vswprintf(buffer,len, format, args);
    va_end(args);

    CString str;
    str.Format(_T("ModuleID[%d] LevelID[%d]: %s\n"),moduleID, levelID, buffer);
	
	if(g_pLogMgr != NULL)
	{
		g_pLogMgr->WriteToLogFile(str);
	}

    free(buffer);
}

//parse xml file
DWORD ParseUclXml(MFGLIB_VARS *pLibVars)
{
	if(NULL == pLibVars)
	{
		return MFGLIB_ERROR_INVALID_PARAM;
	}

	if(NULL != pLibVars->g_pXmlHandler)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_WARNING, _T("Ucl xml file has been parsed."));
		return MFGLIB_ERROR_ALREADY_EXIST;
	}

	try
	{
		pLibVars->g_pXmlHandler = new CUclXml;
	}
	catch(...)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Error occurs when constructing CUclXml object."));
		return MFGLIB_ERROR_NO_MEMORY;
	}

	if( NULL == pLibVars->g_pXmlHandler )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Create CUclXml object failed."));
		return MFGLIB_ERROR_NO_MEMORY;
	}

	if(pLibVars->g_CfgParam.chip.IsEmpty())
	{
		return MFGLIB_ERROR_PROFILE_NOT_SET;
	}
	if(pLibVars->g_CfgParam.list.IsEmpty())
	{
		return MFGLIB_ERROR_LIST_NOT_SET;
	}

	//for use atl convert macro, A2T
	USES_CONVERSION;
	//read Ucl.xml
	int pos = pLibVars->g_strUclFilename.ReverseFind(_T('\\'));
	pLibVars->g_strPath = pLibVars->g_strUclFilename.Left(pos+1);  //+1 for add '\' at the last

	CStringT<char,StrTraitMFC<char> > uclString;
	CFile UclXmlFile;
	if(!UclXmlFile.Open(pLibVars->g_strUclFilename, CFile::modeReadWrite, NULL))
	{
		return MFGLIB_ERROR_FILE_OPEN_FAILED;
	}
	UclXmlFile.Read(uclString.GetBufferSetLength((int)UclXmlFile.GetLength()), (unsigned int)UclXmlFile.GetLength());
	uclString.ReleaseBuffer();
	// Load xml file content
	pLibVars->g_pXmlHandler->Load(A2T(uclString));

	DWORD retVal = MFGLIB_ERROR_SUCCESS;
	LPXNode pCfg = pLibVars->g_pXmlHandler->GetChild(_T("CFG"));
	if(pCfg == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't find the \"<CFG>\" item in ucl xml file"));
		delete pLibVars->g_pXmlHandler;
		pLibVars->g_pXmlHandler = NULL;
		return MFGLIB_ERROR_XML_NO_CFG_ITEM;
	}

	XNodes states = pCfg->GetChilds(_T("STATE"));
	if(states.empty())
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't find the \"<STATE>\" item in \"<CFG>\" of the xml file"));
		delete pLibVars->g_pXmlHandler;
		pLibVars->g_pXmlHandler = NULL;
		return MFGLIB_ERROR_XML_NO_STATE_ITEM;
	}

	std::vector<LPXNode>::iterator state = states.begin();
	COpState *pState = NULL;
	CString strTemp;
	// for each STATE
	for(; state!=states.end(); ++state)
	{
		try
		{
			pState = new COpState;
		}
		catch(...)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Error occurs when constructing COpState object"));
			ReleaseUclCommands(pLibVars);
			return MFGLIB_ERROR_NO_MEMORY;
		}
		if(pState == NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Create COpState object failed"));
			ReleaseUclCommands(pLibVars);
			return MFGLIB_ERROR_NO_MEMORY;
		}

		strTemp = (*state)->GetAttrValue(_T("name"));
		pState->strStateName = strTemp;
		if ( strTemp.CompareNoCase(_T("BootStrap")) == 0 )
		{
		   pState->opState = MX_BOOTSTRAP;
		}
		else if ( strTemp.CompareNoCase(_T("Updater")) == 0 )
		{
		   pState->opState = MX_UPDATER;
		}
		else if (strTemp.CompareNoCase(_T("Blhost")) == 0)
		{
			pState->opState = MX_BLHOST;
		}
		else
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Error: Invalid state name: %s"), strTemp);
			pState->opState = MX_DISCONNECTED;
			ReleaseUclCommands(pLibVars);
			return MFGLIB_ERROR_INVALID_STATE_NAME;
		}

		strTemp = (*state)->GetAttrValue(_T("dev"));
		pState->strDevice = strTemp;
/*		if( strTemp.CompareNoCase(_T("MX23")) == 0 )
		{
			pState->opDeviceType = DEV_MX23;
		}
		else if( strTemp.CompareNoCase(_T("MX25")) == 0 )
		{
			pState->opDeviceType = DEV_MX25;
		}
		else if( strTemp.CompareNoCase(_T("MX28")) == 0 )
		{
			pState->opDeviceType = DEV_MX28;
		}
		else if( strTemp.CompareNoCase(_T("MX35")) == 0 )
		{
			pState->opDeviceType = DEV_MX35;
		}
		else if( strTemp.CompareNoCase(_T("MX50")) == 0 )
		{
			pState->opDeviceType = DEV_MX50;
		}
		else if( strTemp.CompareNoCase(_T("MX51")) == 0 )
		{
			pState->opDeviceType = DEV_MX51;
		}
		else if( strTemp.CompareNoCase(_T("MX53")) == 0 )
		{
			pState->opDeviceType = DEV_MX53;
		} */

		if (pState->opState == MX_BOOTSTRAP)
		{
			ROM_INFO *p = SearchCompatiableRom(strTemp);
			if (p)
			{
				pState->romInfo = *p;
			}
			else
			{
				strTemp = (*state)->GetAttrValue(_T("compatible"));
				p = SearchCompatiableRom(strTemp);
				if (p)
				{
					pState->romInfo = *p;
				}
				else
				{
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Can't found compatible ROM type %s-%s"), pState->strDevice, strTemp);
					ReleaseUclCommands(pLibVars);
					return MFGLIB_ERROR_INVALID_DEV_NAME;
				}
			}
		}
		
		strTemp = (*state)->GetAttrValue(_T("vid"));
		pState->uiVid = _tcstoul(strTemp, NULL, 16);

		strTemp = (*state)->GetAttrValue(_T("pid"));
		pState->uiPid = _tcstoul(strTemp, NULL, 16);

		strTemp = (*state)->GetAttrValue(_T("timeout"));
		if(strTemp.IsEmpty())
		{
			pState->dwTimeout = DEFAULT_TIMEOUT_CHANGE_STATE;
		}
		else
		{
			pState->dwTimeout = _tcstol(strTemp, NULL, 10);
		}

		strTemp = (*state)->GetAttrValue(_T("bcdDevice"));
		if (!strTemp.IsEmpty())
		{
			pState->bcdDevice = _tcstoul(strTemp, NULL, 16);
		}
		pLibVars->g_OpStates.push_back(pState);
	}

	//Parse each command
	CCommandList* pCmdList = pLibVars->g_pXmlHandler->GetCmdListNode(pLibVars->g_CfgParam.list);
	if(pCmdList == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Error: Can't find the specified list(%s) in the xml file"), pLibVars->g_CfgParam.list);
		ReleaseUclCommands(pLibVars);
		return MFGLIB_ERROR_XML_NO_MATCHED_LIST;
	}
	CString strCmdType;
	COpCommand* pOpCmd;

	XNodes nodes = pCmdList->GetChilds(_T("CMD"));
	if(nodes.empty())
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Error: Can't find any command in the specified list(%s) in the xml file"), pLibVars->g_CfgParam.list);
		ReleaseUclCommands(pLibVars);
		return MFGLIB_ERROR_XML_NO_CMDS_IN_LIST;
	}
	std::vector<LPXNode>::iterator it = nodes.begin();
	int index = 0;
	for( ; it!=nodes.end(); ++it)
	{
		index++;
		CString strState = (*it)->GetAttrValue(_T("state"));
		CString strCmdType = (*it)->GetAttrValue(_T("type"));
		if(strState.IsEmpty())
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_WARNING, _T("<CMD>%d has no \"state\" attribute"), index);
			retVal = MFGLIB_ERROR_XML_CMD_NO_STATE;
			goto CMD_ERR;
		}
		if(strCmdType.IsEmpty())
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_WARNING, _T("<CMD>%d has no \"type\" attribute"), index);
			retVal = MFGLIB_ERROR_XML_CMD_NO_TYPE;
			goto CMD_ERR;
		}
		if( strCmdType.CompareNoCase(_T("find")) == 0 )
		{
			pOpCmd = new COpCmd_Find;
			if(pOpCmd == NULL)
			{
				retVal = MFGLIB_ERROR_NO_MEMORY;
				goto CMD_ERR;
			}
			pOpCmd->m_pLibVars = (INSTANCE_HANDLE)pLibVars;
			strTemp = (*it)->GetAttrValue(_T("timeout"));
			((COpCmd_Find *)pOpCmd)->SetTimeout(strTemp);
		}
		else if( strCmdType.CompareNoCase(_T("boot")) == 0 )
		{
			pOpCmd = new COpCmd_Boot;
			if(pOpCmd == NULL)
			{
				retVal = MFGLIB_ERROR_NO_MEMORY;
				goto CMD_ERR;
			}
			pOpCmd->m_pLibVars = (INSTANCE_HANDLE)pLibVars;
			strTemp = (*it)->GetAttrValue(_T("file"));
			strTemp = ReplaceKeywords(strTemp);
			retVal = ((COpCmd_Boot*)pOpCmd)->SetFileMapping(strTemp);
			if(retVal != MFGLIB_ERROR_SUCCESS)
			{
				goto CMD_ERR;
			}
		}
		else if( strCmdType.CompareNoCase(_T("init")) == 0 )
		{
			pOpCmd = new COpCmd_Init;
			if(pOpCmd == NULL)
			{
				retVal = MFGLIB_ERROR_NO_MEMORY;
				goto CMD_ERR;
			}
			pOpCmd->m_pLibVars = (INSTANCE_HANDLE)pLibVars;
			strTemp = (*it)->GetAttrValue(_T("file"));
			strTemp = ReplaceKeywords(strTemp);
			retVal = ((COpCmd_Init*)pOpCmd)->SetFileMapping(strTemp);
			if(retVal != MFGLIB_ERROR_SUCCESS)
			{
				goto CMD_ERR;
			}
		}
		else if( strCmdType.CompareNoCase(_T("load")) == 0 )
		{
			pOpCmd = new COpCmd_Load;
			if(pOpCmd == NULL)
			{
				retVal = MFGLIB_ERROR_NO_MEMORY;
				goto CMD_ERR;
			}
			pOpCmd->m_pLibVars = (INSTANCE_HANDLE)pLibVars;
			strTemp = (*it)->GetAttrValue(_T("file"));
			strTemp = ReplaceKeywords(strTemp);
			retVal = ((COpCmd_Load*)pOpCmd)->SetFileMapping(strTemp);
			if(retVal != MFGLIB_ERROR_SUCCESS)
			{
				goto CMD_ERR;
			}

			strTemp = (*it)->GetAttrValue(_T("address"));
			strTemp = ReplaceKeywords(strTemp);
			((COpCmd_Load*)pOpCmd)->SetAddress(strTemp);

			strTemp = (*it)->GetAttrValue(_T("loadSection"));
			strTemp = ReplaceKeywords(strTemp);
			((COpCmd_Load*)pOpCmd)->SetloadSection(strTemp);

			strTemp = (*it)->GetAttrValue(_T("setSection"));
			strTemp = ReplaceKeywords(strTemp);
			((COpCmd_Load*)pOpCmd)->SetsetSection(strTemp);

			strTemp = (*it)->GetAttrValue(_T("HasFlashHeader"));
			strTemp = ReplaceKeywords(strTemp);
			((COpCmd_Load*)pOpCmd)->SetIsHasFlashHeader(strTemp);
		}
		else if( strCmdType.CompareNoCase(_T("jump")) == 0 )
		{
			pOpCmd = new COpCmd_Jump;
			if(pOpCmd == NULL)
			{
				retVal = MFGLIB_ERROR_NO_MEMORY;
				goto CMD_ERR;
			}
			pOpCmd->m_pLibVars = (INSTANCE_HANDLE)pLibVars;

			((COpCmd_Jump*)pOpCmd)->m_bIngoreError = FALSE;
			strTemp = (*it)->GetAttrValue(_T("onError"));
			if (strTemp.CompareNoCase(_T("ignore")) == 0)
			{
				((COpCmd_Jump*)pOpCmd)->m_bIngoreError = TRUE;
			}
		}
		else if( strCmdType.CompareNoCase(_T("push")) == 0 )
		{
			pOpCmd = new COpCmd_Push;
			if(pOpCmd == NULL)
			{
				retVal = MFGLIB_ERROR_NO_MEMORY;
				goto CMD_ERR;
			}
			pOpCmd->m_pLibVars = (INSTANCE_HANDLE)pLibVars;
			strTemp = (*it)->GetAttrValue(_T("file"));
			strTemp = ReplaceKeywords(strTemp);
			if(!strTemp.IsEmpty())
			{
				retVal = ((COpCmd_Push*)pOpCmd)->SetFileMapping(strTemp);
				if(retVal != MFGLIB_ERROR_SUCCESS)
				{
					goto CMD_ERR;
				}
			}
			strTemp = (*it)->GetAttrValue(_T("savedfile"));
			strTemp = ReplaceKeywords(strTemp);
			if(!strTemp.IsEmpty())
			{
				((COpCmd_Push*)pOpCmd)->m_SavedFileName = strTemp;
			}
			strTemp = (*it)->GetAttrValue(_T("onError"));
			if( strTemp.CompareNoCase(_T("ignore")) == 0 )
			{
				((COpCmd_Push*)pOpCmd)->m_bIngoreError = TRUE;
			}
		}
		else if (strCmdType.CompareNoCase(_T("blhost")) == 0)
		{
			pOpCmd = new COpCmd_Blhost;
			if (pOpCmd == NULL)
			{
				retVal = MFGLIB_ERROR_NO_MEMORY;
				goto CMD_ERR;
			}
			pOpCmd->m_pLibVars = (INSTANCE_HANDLE)pLibVars;
			strTemp = (*it)->GetAttrValue(_T("file"));
			strTemp = ReplaceKeywords(strTemp);
			if (!strTemp.IsEmpty())
			{
				retVal = ((COpCmd_Blhost*)pOpCmd)->SetFileMapping(strTemp);
				if (retVal != MFGLIB_ERROR_SUCCESS)
				{
					goto CMD_ERR;
				}
			}
			strTemp = (*it)->GetAttrValue(_T("savedfile"));
			strTemp = ReplaceKeywords(strTemp);
			if (!strTemp.IsEmpty())
			{
				((COpCmd_Blhost*)pOpCmd)->m_SavedFileName = strTemp;
			}
			strTemp = (*it)->GetAttrValue(_T("onError"));
			if (strTemp.CompareNoCase(_T("ignore")) == 0)
			{
				((COpCmd_Blhost*)pOpCmd)->m_bIngoreError = FALSE;
			}

			strTemp = (*it)->GetAttrValue(_T("timeout"));
			if (!strTemp.IsEmpty())
			{
				if (!((COpCmd_Blhost*)pOpCmd)->SetTimeout(strTemp))
				{
					retVal = MFGLIB_ERROR_INVALID_PARAM;
					goto CMD_ERR;
				}
			}
			else
			{
				((COpCmd_Blhost*)pOpCmd)->SetTimeout(5000);
			}
		}
		/*	else if( strCmdType.CompareNoCase(_T("burn")) == 0 )
		{
			pOpCmd = new COpCmd_Burn;
			if(pOpCmd == NULL)
			{
				retVal = MFGLIB_ERROR_NO_MEMORY;
				goto CMD_ERR;
			}
			pOpCmd->m_pLibVars = (INSTANCE_HANDLE)pLibVars;
			strTemp = (*it)->GetAttrValue(_T("file"));
			if(strTemp.IsEmpty())
			{
				retVal = MFGLIB_ERROR_NO_FILENAME;
				goto CMD_ERR;
			}
			((COpCmd_Burn*)pOpCmd)->SetFileName(strTemp);
		} */

		strTemp = (*it)->GetAttrValue(_T("body"));
		strTemp = ReplaceKeywords(strTemp);
		pOpCmd->SetTemplateBodyString(strTemp);
		pOpCmd->SetDescString((*it)->GetText());

		strTemp = (*it)->GetAttrValue(_T("ifdev"));
		pOpCmd->SetIfDevString(strTemp);

		strTemp = (*it)->GetAttrValue(_T("ifhab"));
		pOpCmd->SetIfHabString(strTemp);

		if( strState.CompareNoCase(_T("BootStrap")) == 0 )
		{
			pLibVars->g_StateCommands[MX_BOOTSTRAP].push_back(pOpCmd);
		}
		else if(strState.CompareNoCase(_T("Updater")) == 0)
		{
			pLibVars->g_StateCommands[MX_UPDATER].push_back(pOpCmd);
		}
		else if (strState.CompareNoCase(_T("Blhost")) == 0)
		{
			pLibVars->g_StateCommands[MX_BLHOST].push_back(pOpCmd);
		}
	}
	return MFGLIB_ERROR_SUCCESS;

CMD_ERR:
	ReleaseUclCommands(pLibVars);
	return retVal;
}

void ReleaseUclCommands(MFGLIB_VARS *pLibVars)
{
	if(!pLibVars->g_OpStates.empty())
	{
		std::vector<COpState*>::iterator it = pLibVars->g_OpStates.begin();
		for(; it!=pLibVars->g_OpStates.end(); it++)
		{
			delete (*it);
		}
		pLibVars->g_OpStates.clear();
	}

	if(!pLibVars->g_StateCommands.empty())
	{
		StateCommansMap_t::iterator it = pLibVars->g_StateCommands.begin();
		for(; it!=pLibVars->g_StateCommands.end(); it++)
		{
			if( !((*it).second.empty()) )
			{
				OP_COMMAND_ARRAY::iterator it2 = (*it).second.begin();
				for(; it2!=(*it).second.end(); it2++)
				{
					delete (*it2);
				}
				(*it).second.clear();
			}
		}
		pLibVars->g_StateCommands.clear();
	}

	if(pLibVars->g_pXmlHandler != NULL)
	{
		delete pLibVars->g_pXmlHandler;
		pLibVars->g_pXmlHandler = NULL;
	}
}
/*
* CCommandList implementation
*/
CString CCommandList::GetName()
{
	return CString(GetAttrValue(_T("name")));
}

CString CCommandList::GetDescription()
{
	return CString(GetAttrValue(_T("desc")));
}

CCommandList* CUclXml::GetCmdListNode(LPCTSTR name)
{
	XNodes lists = GetChilds(_T("LIST"));
	if(lists.empty())
	{
		return NULL;
	}
	
	std::vector<LPXNode>::iterator it = lists.begin();
	for(; it!=lists.end(); ++it)
	{
		CCommandList* pList = (CCommandList*)(*it);
		CString strName = pList->GetName();
		if( strName.CompareNoCase(name) == 0 )
		{
			return pList;
		}
	}
	
	return NULL;
}
/*
* COpCommand implementation
*/
COpCommand::COpCommand()
{
	m_pLibVars = NULL;
}

COpCommand::~COpCommand()
{
}

void COpCommand::SetBodyString(int index, const CString &str)
{
	m_bodyStringMap[index] = str;
}

void COpCommand::SetTemplateBodyString(const CString &str)
{
	m_templateBodyString = str;
}

CString COpCommand::GetBodyString(int index) const
{
	try
	{
		return m_bodyStringMap.at(index);
	}
	catch (const std::out_of_range& oor)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("%s-- index %d not found in m_bodyString."), __func__, index);
		return m_templateBodyString;
	}
}

CString COpCommand::GetTemplateBodyString() const
{
	return m_templateBodyString;
}


void COpCommand::SetDescString(CString &str)
{
	m_descString = str;
}

CString COpCommand::GetDescString()
{
	return m_descString;
}

UINT COpCommand::ExecuteCommand(int index)
{
	return 0;
}

void COpCommand::SetIfDevString(CString &str)
{
	m_ifdev = str;
}

void COpCommand::SetIfHabString(CString &str)
{
	m_ifhab = str;
}

bool COpCommand::IsRun(CString &str)
{
	if (m_ifdev.IsEmpty())
		return true;

	int start = 0;

	CString substr = m_ifdev;
	while (start >= 0)
	{
		substr = m_ifdev.Tokenize(_T(" ,;\n\t"), start);
		if (substr == str)
			return true;
	}

	return false;
}

bool COpCommand::IsRun(HAB_t habState)
{
	if (m_ifhab.IsEmpty())
		return true;

	CString stateStr;
	if (habState == HabDisabled)
	{
		stateStr = _T("Open");
	}
	else if (habState == HabEnabled)
	{
		stateStr = _T("Close");
	}
	else
	{
		stateStr = _T("Unknown");
	}

	int start = 0;
	CString substr = m_ifhab;
	while (start >= 0)
	{
		substr = m_ifhab.Tokenize(_T(" ,;\n\t"), start);
		if (substr == stateStr)
			return true;
	}

	return false;
}

/*
* COpCmd_Find implementation
*/
void COpCmd_Find::SetTimeout(CString &strTO)
{
	m_timeout = _tcstol(LPCTSTR(strTO), NULL, 10);
}

UINT COpCmd_Find::ExecuteCommand(int index)
{
	return 0;
}

/*
* COpCmd_Boot implementation
*/
COpCmd_Boot::COpCmd_Boot()
{
	m_pDataBuf = NULL;
}

COpCmd_Boot::~COpCmd_Boot()
{
	CloseFileMapping();
}

UINT COpCmd_Boot::SetFileMapping(CString &strFile)
{
	int index = strFile.Find(_T('/'));	//find '/'
	if(-1 != index)
	{
		strFile.Replace(_T('/'), _T('\\'));
	}
	m_FileName = ((MFGLIB_VARS *)m_pLibVars)->g_strPath + strFile;

	CFile fwFile;
	if ( fwFile.Open(m_FileName, CFile::modeRead | CFile::shareDenyWrite) == FALSE )
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Boot command-- file %s failed to open.errcode is %d"), m_FileName,GetLastError());
        return MFGLIB_ERROR_FILE_NOT_EXIST;
    }
	m_qwFileSize = fwFile.GetLength();
	m_pDataBuf = (UCHAR*)VirtualAlloc(NULL, (size_t)m_qwFileSize, MEM_COMMIT, PAGE_READWRITE);
	if(m_pDataBuf == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Boot command-- file[%s] data buffer alloc failed"), m_FileName);
		return MFGLIB_ERROR_NO_MEMORY;
	}
	fwFile.Read(m_pDataBuf, (UINT)m_qwFileSize);
	fwFile.Close();

	return MFGLIB_ERROR_SUCCESS;
}

void COpCmd_Boot::CloseFileMapping()
{
	if(m_pDataBuf)
	{
		VirtualFree(m_pDataBuf, 0, MEM_RELEASE);
		m_pDataBuf = NULL;
	}
}

UINT COpCmd_Boot::ExecuteCommand(int index)
{
	CString strMsg;
	strMsg.Format(_T("ExecuteCommand--Boot[WndIndex:%d], File is %s"), index, m_FileName);
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("%s"), strMsg);

	UI_UPDATE_INFORMATION _uiInfo;
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_RUNNING;
	_uiInfo.bUpdateDescription = TRUE;
	CString strDesc = GetDescString();
	_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
	strDesc.ReleaseBuffer();
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDeviceType() == DeviceClass::DeviceTypeMxHid)
	{
		MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		if(pMxHidDevice==NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--No MxHidDevice"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Boot\" body=\"%s\" error, file=\"%s\""), GetBodyString(index), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_INVALID_VALUE;
		}
		if(!pMxHidDevice->RunPlugIn(m_pDataBuf, m_qwFileSize))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--MxHidDevice--Command Boot excute failed"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Boot\" body=\"%s\" error, file=\"%s\""), GetBodyString(index), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
		}
	}
	else if (((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDeviceType() == DeviceClass::DeviceTypeHid)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("run hid command"));
		HidDevice* pHidDevice = dynamic_cast<HidDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		if (pHidDevice == NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--No HidDevice"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Boot\" body=\"%s\" error, file=\"%s\""), GetBodyString(index), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_INVALID_VALUE;
		}

		StPitc myNewFwCommandSupport(pHidDevice, m_pDataBuf, m_qwFileSize);
		StApi *pStApi = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pStApi;
		if (pStApi == NULL)
		{
			pStApi = new HidDownloadFw(m_pDataBuf, m_qwFileSize);
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pStApi = pStApi;
			if (pStApi == NULL)
				return MFGLIB_ERROR_INVALID_VALUE;
		}

		if (myNewFwCommandSupport.DownloadPitc(*pStApi, index))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--HidDevice--Command Boot excute failed"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Boot\" body=\"%s\" error, file=\"%s\""), GetBodyString(index), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
		}
	}
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.bUpdateDescription = FALSE;
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	return MFGLIB_ERROR_SUCCESS;
}

/*
* COpCmd_Init implementation
*/
COpCmd_Init::COpCmd_Init()
{
	m_pDataBuf = NULL;
}

COpCmd_Init::~COpCmd_Init()
{
	CloseFileMapping();
}

UINT COpCmd_Init::SetFileMapping(CString &strFile)
{
	int index = strFile.Find(_T('/'));	//find '/'
	if(-1 != index)
	{
		strFile.Replace(_T('/'), _T('\\'));
	}
	m_FileName = ((MFGLIB_VARS *)m_pLibVars)->g_strPath + strFile;

	CFile fwFile;
	if ( fwFile.Open(m_FileName, CFile::modeRead | CFile::shareDenyWrite) == FALSE )
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Init command--file %s failed to open.errcode is %d."), m_FileName,GetLastError());
        return MFGLIB_ERROR_FILE_NOT_EXIST;
    }
	m_qwFileSize = fwFile.GetLength();
	//m_pDataBuf = (UCHAR*)malloc((size_t)m_qwFileSize);
	m_pDataBuf = (UCHAR*)VirtualAlloc(NULL, (size_t)m_qwFileSize, MEM_COMMIT, PAGE_READWRITE);
	if(m_pDataBuf == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Init command--file[%s] data buffer alloc failed."), m_FileName);
		return MFGLIB_ERROR_NO_MEMORY;
	}
	fwFile.Read(m_pDataBuf, (UINT)m_qwFileSize);
	fwFile.Close();

	return MFGLIB_ERROR_SUCCESS;
}

void COpCmd_Init::CloseFileMapping()
{
	if(m_pDataBuf)
	{
		VirtualFree(m_pDataBuf, 0, MEM_RELEASE);
		m_pDataBuf = NULL;
	}
}

UINT COpCmd_Init::ExecuteCommand(int index)
{
	CString strMsg;
	strMsg.Format(_T("ExecuteCommand--Init[WndIndex:%d], File is %s"), index, m_FileName);
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("%s"), strMsg);

	UI_UPDATE_INFORMATION _uiInfo;
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_RUNNING;
	_uiInfo.bUpdateDescription = TRUE;
	CString strDesc = GetDescString();
	_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
	strDesc.ReleaseBuffer();
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDeviceType() == DeviceClass::DeviceTypeMxHid)
	{
		MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		if(pMxHidDevice==NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--No MxHidDevice"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Init\" body=\"%s\" error, file=\"%s\""), GetBodyString(index), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_INVALID_VALUE;
		}
		if(!pMxHidDevice->InitMemoryDevice(m_FileName))
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--MxHidDevice--Command Init excute failed"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Init\" body=\"%s\" error, file=\"%s\""), GetBodyString(index), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
		}
	}

	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.bUpdateDescription = FALSE;
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	return MFGLIB_ERROR_SUCCESS;
}

/*
* COpCmd_Load implementation
*/
COpCmd_Load::COpCmd_Load()
{
	m_iPercentComplete = 0;
	m_pDataBuf = NULL;
}

COpCmd_Load::~COpCmd_Load()
{
	CloseFileMapping();
}

UINT COpCmd_Load::ExecuteCommand(int index)
{
	CString strMsg;
	strMsg.Format(_T("ExecuteCommand--Load[WndIndex:%d], File is %s, address is 0x%X"), index, m_FileName, m_address);
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("%s"), strMsg);

	UI_UPDATE_INFORMATION _uiInfo;
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_RUNNING;
	_uiInfo.bUpdateDescription = TRUE;
	CString strDesc = GetDescString();
	_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
	strDesc.ReleaseBuffer();
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDeviceType() == DeviceClass::DeviceTypeMxHid)
	{
		MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		if(pMxHidDevice==NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--No MxHidDevice"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Load\" error, file=\"%s\""), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_INVALID_VALUE;
		}

		//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("ExecuteCommand--Load, Device[0x%X]"), pMxHidDevice);
	
		int retryCount = 0;
		while(retryCount < 3)
		{
			MxHidDevice::ImageParameter ImageParameter;
			ImageParameter.loadSection = MxHidDevice::StringToMemorySection(m_strloadSection);
			ImageParameter.setSection = MxHidDevice::StringToMemorySection(m_strsetSection);
			ImageParameter.PhyRAMAddr4KRL = m_address;
			//ImageParameter.HasFlashHeader = pCmd->HasFlashHeader();
			ImageParameter.CodeOffset = 0;

			if(!pMxHidDevice->Download(&ImageParameter, m_pDataBuf, m_qwFileSize, index))
			{
				retryCount++;
				continue;
			}
			else
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("PortMgrDlg(%d)--Command Load excute successfully, retry count: %d"), index, retryCount);
				_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
				_uiInfo.bUpdateCommandsProgress = TRUE;
				_uiInfo.bUpdateDescription = FALSE;
				_uiInfo.bUpdatePhaseProgress = FALSE;
				_uiInfo.bUpdateProgressInCommand = FALSE;
				_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
				_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
				return MFGLIB_ERROR_SUCCESS;
			}
		}
		if(retryCount >= 3 )
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--MxHidDevice--Command Load excute failed, retry count: %d"), index, retryCount);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Load\" error, file=\"%s\""), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
		}
	}

	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.bUpdateDescription = FALSE;
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	return MFGLIB_ERROR_SUCCESS;
}

UINT COpCmd_Load::SetFileMapping(CString &strFile)
{
	int index = strFile.Find(_T('/'));	//find '/'
	if(-1 != index)
	{
		strFile.Replace(_T('/'), _T('\\'));
	}
	m_FileName = ((MFGLIB_VARS *)m_pLibVars)->g_strPath + strFile;

	CFile fwFile;
	if ( fwFile.Open(m_FileName, CFile::modeRead) == FALSE )
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Load command--file %s failed to open.errcode is %d"), m_FileName,GetLastError());
        return MFGLIB_ERROR_FILE_NOT_EXIST;
    }
	m_qwFileSize = fwFile.GetLength();
	m_pDataBuf = (UCHAR*)VirtualAlloc(NULL, (size_t)m_qwFileSize, MEM_COMMIT, PAGE_READWRITE);
	//m_pDataBuf = (UCHAR*)malloc((size_t)m_qwFileSize);
	if(m_pDataBuf == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Load command--file[%s] data buffer alloc failed"), m_FileName);
		return MFGLIB_ERROR_NO_MEMORY;
	}
	fwFile.Read(m_pDataBuf, (UINT)m_qwFileSize);
	fwFile.Close();

//	DWORD dwOldProtect;
//	BOOL bFlag = VirtualProtect(m_pDataBuf, (size_t)m_qwFileSize, PAGE_READONLY, &dwOldProtect);
//	if(!bFlag)
//	{
//		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Change the page protect attribute failed!\n"));
//	}

	return 0;
}

void COpCmd_Load::CloseFileMapping()
{
	if(m_pDataBuf)
	{
		VirtualFree(m_pDataBuf, 0, MEM_RELEASE);
		m_pDataBuf = NULL;
	}
}

DWORD COpCmd_Load::GetFileDataSize()
{
	return (DWORD)m_qwFileSize;
}

void COpCmd_Load::SetAddress(CString &strAddr)
{
	if(strAddr.Find(_T("0x")) != -1)
	{
		strAddr = strAddr.Right(strAddr.GetLength()-2);
	}
	m_address = (UINT)_tcstoul(LPCTSTR(strAddr), NULL, 16);
}

void COpCmd_Load::SetloadSection(CString &str)
{
	m_strloadSection = str;
}

void COpCmd_Load::SetsetSection(CString &str)
{
	m_strsetSection = str;
}

void COpCmd_Load::SetIsHasFlashHeader(CString &str)
{
	if(str.CompareNoCase(_T("FALSE")) == 0)
	{
		m_bHasFlashHeader = FALSE;
	}
	else
	{
		m_bHasFlashHeader = TRUE;
	}
}

/*
* COpCmd_Jump implementation
*/
UINT COpCmd_Jump::ExecuteCommand(int index)
{
	CString strMsg;
	strMsg.Format(_T("ExecuteCommand--Jump[WndIndex:%d]"), index);
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("%s"), strMsg);

	UI_UPDATE_INFORMATION _uiInfo;
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_RUNNING;
	_uiInfo.bUpdateDescription = TRUE;
	CString strDesc = GetDescString();
	_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
	strDesc.ReleaseBuffer();
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDeviceType() == DeviceClass::DeviceTypeMxHid)
	{
		MxHidDevice* pMxHidDevice = dynamic_cast<MxHidDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		if(pMxHidDevice==NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--No MxHidDevice"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Jump\" error"));
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_INVALID_VALUE;
		}
		if(!pMxHidDevice->Jump())
		{
			if (!m_bIngoreError)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--MxHidDevice--Command Jump excute failed"), index);
				_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
				_uiInfo.bUpdatePhaseProgress = FALSE;
				_uiInfo.bUpdateProgressInCommand = FALSE;
				_uiInfo.bUpdateCommandsProgress = TRUE;
				_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
				_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
				_uiInfo.bUpdateDescription = TRUE;
				CString strDesc;
				strDesc.Format(_T("\"Jump\" error"));
				_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
				strDesc.ReleaseBuffer();
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
				return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
			}
		}
	}

	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.bUpdateDescription = FALSE;
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	return 0;
}

/*
* COpCmd_Push implementation
*/
COpCmd_Push::COpCmd_Push()
{
	m_FileName = _T("");
	m_qwFileSize = 0;
	m_pDataBuf = NULL;
	m_bIngoreError = FALSE;
}

COpCmd_Push::~COpCmd_Push()
{
	CloseFileMapping();
}

UINT COpCmd_Push::SetFileMapping(CString &strFile)
{
	int index = strFile.Find(_T('/'));	//find '/'
	if(-1 != index)
	{
		strFile.Replace(_T('/'), _T('\\'));
	}
	m_FileName = ((MFGLIB_VARS *)m_pLibVars)->g_strPath + strFile;

	CFile fwFile;
	if ( fwFile.Open(m_FileName, CFile::modeRead | CFile::shareDenyWrite) == FALSE )
    {
        LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Push command--file %s failed to open.errcode is %d"), m_FileName,GetLastError());
        return MFGLIB_ERROR_FILE_NOT_EXIST;
    }

	return 0;
}

void COpCmd_Push::CloseFileMapping()
{
}

UINT COpCmd_Push::ExecuteCommand(int index)
{
	CString strMsg;
	if (m_FileName.IsEmpty())
		strMsg.Format(_T("ExecuteCommand--Push[WndIndex:%d], Body is %s"), index, GetBodyString(index));
	else
		strMsg.Format(_T("ExecuteCommand--Push[WndIndex:%d], Body is %s, File is %s"), index, GetBodyString(index), m_FileName);

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("%s"), strMsg);

	UI_UPDATE_INFORMATION _uiInfo;
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_RUNNING;
	_uiInfo.bUpdateDescription = TRUE;
	CString strDesc = GetDescString();
	_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
	strDesc.ReleaseBuffer();
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	CString csCmdBody = GetBodyString(index);
	CString csCmdText = GetDescString();
	DWORD retValue = MFGLIB_ERROR_SUCCESS;

	if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pUTP == NULL)
	{
		((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pUTP = new UpdateTransportProtocol((Volume*)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice()));
	}

	if(m_FileName.IsEmpty())
	{
	/*	if(csCmdBody.Find(_T("GenNewUID")) != -1)
        {
            int Begin, End, i=0;
			CString strTemp;
            //Replace $() with real value
            //Search all variables which has a format of $()
            do{
                Begin = csCmdBody.Find(_T("$("));
                End = csCmdBody.Find(_T(")"));           
                
                if(Begin != -1 && End != -1)
                {
                    //Extract Current key word
					strTemp = _T("$(");
        			Begin += strTemp.GetLength();
                    CString CurKeyWord = csCmdBody.Mid(Begin, (End-Begin));
                    
                    //Find current key word value in m_UniqueID.KeyWord list
                    for(i=0;i<MAX_CNT_KEYWORD;i++)
                    {
                        if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].KeyWordName.CompareNoCase(CurKeyWord) == 0)
                        {
                            //Replace $() with real value
        					CString csCurKeyWordValue;
        					csCurKeyWordValue.Format(_T("0x%x"),((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].CurrentValue);
                            CString csTemp;
							csTemp.Format(_T("$(%s)"), CurKeyWord);
                            csCmdBody.Replace(csTemp, csCurKeyWordValue);
                            break;
                        }
                    }
                    if(i >= MAX_CNT_KEYWORD)
                        return MFGLIB_ERROR_WRITE_LOG;
                }             
            }while(Begin != -1 && End != -1);                 
        }
	*/
		if(m_SavedFileName.IsEmpty())
		{
			retValue = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pUTP->UtpCommand(csCmdBody);
			if( (csCmdText.CompareNoCase(_T("Done")) == 0) )
			{
				_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
				_uiInfo.bUpdatePhaseProgress = TRUE;
				_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
				_uiInfo.bUpdateProgressInCommand = FALSE;
				_uiInfo.bUpdateDescription = FALSE;
				_uiInfo.bUpdateCommandsProgress = TRUE;
				_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
				_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
				return MFGLIB_ERROR_SUCCESS_UPDATE_COMPLETE;
			}
			else if((retValue != ERROR_SUCCESS) && (!m_bIngoreError))
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--MSCDevice--Command Push(no file) excute failed"), index);
				_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
				_uiInfo.bUpdatePhaseProgress = FALSE;
				_uiInfo.bUpdateProgressInCommand = FALSE;
				_uiInfo.bUpdateDescription = FALSE;
				_uiInfo.bUpdateCommandsProgress = TRUE;
				_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
				_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
				return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
			}
			if(m_bIngoreError)
			{
				_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
				_uiInfo.bUpdateCommandsProgress = TRUE;
				_uiInfo.bUpdateDescription = FALSE;
				_uiInfo.bUpdatePhaseProgress = TRUE;
				_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
				_uiInfo.bUpdateProgressInCommand = FALSE;
				_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
				_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
				return MFGLIB_ERROR_SUCCESS;
 			}
		}
		else
		{	//read command
			if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pUTP == NULL)
			{
				_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
				_uiInfo.bUpdatePhaseProgress = FALSE;
				_uiInfo.bUpdateProgressInCommand = FALSE;
				_uiInfo.bUpdateDescription = FALSE;
				_uiInfo.bUpdateCommandsProgress = TRUE;
				_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
				_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
				return -65520;
			}
			retValue = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pUTP->UtpRead(csCmdBody, m_SavedFileName, index);
			if((retValue != ERROR_SUCCESS) && !m_bIngoreError )
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--MSCDevice--Command Push(file) excute failed"), index);
				_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
				_uiInfo.bUpdatePhaseProgress = FALSE;
				_uiInfo.bUpdateProgressInCommand = FALSE;
				_uiInfo.bUpdateCommandsProgress = TRUE;
				_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
				_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
				_uiInfo.bUpdateDescription = TRUE;
				CString strDesc;
				strDesc.Format(_T("\"Push\" error, file=\"%s\""), m_FileName);
				_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
				strDesc.ReleaseBuffer();
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
				return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
			}
			else if(m_bIngoreError)
			{
				_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
				_uiInfo.bUpdateCommandsProgress = TRUE;
				_uiInfo.bUpdateDescription = FALSE;
				_uiInfo.bUpdatePhaseProgress = TRUE;
				_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
				_uiInfo.bUpdateProgressInCommand = FALSE;
				_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
				_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
				return MFGLIB_ERROR_SUCCESS;
			}
		}
	}
	else
	{
		if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pUTP == NULL)
		{
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateDescription = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return -65520;
		}
		retValue = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_pUTP->UtpWrite(csCmdBody, m_FileName, index);
		if((retValue != ERROR_SUCCESS) && !m_bIngoreError )
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--MSCDevice--Command Push(file) excute failed"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Push\" error, file=\"%s\""), m_FileName);
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
		}
		else if(m_bIngoreError)
		{
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.bUpdateDescription = FALSE;
			_uiInfo.bUpdatePhaseProgress = TRUE;
			_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_SUCCESS;
		}
	}

	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.bUpdateDescription = FALSE;
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	return retValue;
}

/*
* COpCmd_Blhost implementation
*/
COpCmd_Blhost::COpCmd_Blhost()
{
	m_FileName = _T("");
	m_qwFileSize = 0;
	m_pDataBuf = NULL;
	m_bIngoreError = FALSE;
}

COpCmd_Blhost::~COpCmd_Blhost()
{
	CloseFileMapping();
}

UINT COpCmd_Blhost::SetFileMapping(CString &strFile)
{
	int index = strFile.Find(_T('/'));	//find '/'
	if (-1 != index)
	{
		strFile.Replace(_T('/'), _T('\\'));
	}
	m_FileName = ((MFGLIB_VARS *)m_pLibVars)->g_strPath + strFile;

	CFile fwFile;
	if (fwFile.Open(m_FileName, CFile::modeRead | CFile::shareDenyWrite) == FALSE)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Push command--file %s failed to open.errcode is %d"), m_FileName, GetLastError());
		return MFGLIB_ERROR_FILE_NOT_EXIST;
	}

	return 0;
}

bool COpCmd_Blhost::SetTimeout(const CString strValue)
{
	if (strValue.IsEmpty())
		return false;

	if (strValue[0] == '-')
		return false;

	TCHAR *p;
	UINT64 temp;
	temp = _tcstoull(strValue, &p, 0);
	if (temp > UINT32_MAX)
	{
		return false;
	}
	m_timeout = static_cast<UINT32>(temp);
	return (p != NULL) && (*p == 0);
}

bool COpCmd_Blhost::SetTimeout(const uint32_t value)
{
	m_timeout = value;
	return true;
}

void COpCmd_Blhost::CloseFileMapping()
{
}

UINT COpCmd_Blhost::ExecuteCommand(int index)
{
	CString strMsg;
	strMsg.Format(_T("ExecuteCommand--Blhost[WndIndex:%d], Body is %s"), index, GetBodyString(index));
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("%s"), strMsg);

	UI_UPDATE_INFORMATION _uiInfo;
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_RUNNING;
	_uiInfo.bUpdateDescription = TRUE;
	CString strDesc = GetDescString();
	_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
	strDesc.ReleaseBuffer();
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	CString csCmdBody = GetBodyString(index);
	CString csCmdText = GetDescString();

	if (csCmdText.CompareNoCase(_T("Done")) == 0)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("%s [WndIndex:%d]"), csCmdText, index);
		_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
		_uiInfo.bUpdatePhaseProgress = TRUE;
		_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
		_uiInfo.bUpdateProgressInCommand = FALSE;
		_uiInfo.bUpdateDescription = FALSE;
		_uiInfo.bUpdateCommandsProgress = TRUE;
		_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
		_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
		((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
		return MFGLIB_ERROR_SUCCESS_UPDATE_COMPLETE;
	}

	csCmdText.AppendFormat(_T(" [WndIndex:%d] "), index);
	CString peripheral = _T("--usb ");
	DWORD retValue = MFGLIB_ERROR_SUCCESS;
	if (((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDeviceType() == DeviceClass::DeviceTypeKBLCDC)
	{
		CDCDevice* pCDCDevice = dynamic_cast<CDCDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		if (pCDCDevice == NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--No CDCDevice"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Execute\" error"));
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_INVALID_VALUE;
		}
		peripheral = CString("-p ") + CString(pCDCDevice->m_commport);
	}
	else
	{
		MxHidDevice* pHidDevice = dynamic_cast<MxHidDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		peripheral.Append(pHidDevice->_path.get());
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("PortMgrDlg(%d)--Path=%s"), index, pHidDevice->_path.get());
    }

	CString csTimeout;
	csTimeout.Format(_T("%d"), m_timeout);

	retValue = (ExecuteBlhostCommand(CString(peripheral + _T(" -t ") + csTimeout + _T(" -- ") + csCmdBody), csCmdText));
	if ( retValue == ERROR_SUCCESS)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, csCmdText, index);
		_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
		_uiInfo.bUpdatePhaseProgress = TRUE;
		_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
		_uiInfo.bUpdateProgressInCommand = TRUE;
		_uiInfo.bUpdateDescription = TRUE;
		_uiInfo.bUpdateCommandsProgress = TRUE;
		_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
		_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
		((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
		return MFGLIB_ERROR_SUCCESS;
	}
	else if ((retValue != ERROR_SUCCESS) && (!m_bIngoreError))
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, csCmdText, index);
		_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
		_uiInfo.bUpdatePhaseProgress = FALSE;
		_uiInfo.bUpdateProgressInCommand = FALSE;
		_uiInfo.bUpdateDescription = FALSE;
		_uiInfo.bUpdateCommandsProgress = TRUE;
		_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
		_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
		((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
		return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
	}
	if (m_bIngoreError)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, csCmdText, index);
		_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
		_uiInfo.bUpdateCommandsProgress = TRUE;
		_uiInfo.bUpdateDescription = FALSE;
		_uiInfo.bUpdatePhaseProgress = TRUE;
		_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
		_uiInfo.bUpdateProgressInCommand = FALSE;
		_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
		_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
		((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
		return MFGLIB_ERROR_SUCCESS;
	}

	return retValue;
}

UINT COpCmd_Blhost::GetSecureState(int index, HAB_t *secureState)
{
	CString strMsg;
	strMsg.Format(_T("ExecuteCommand--Blhost[WndIndex:%d], Body is get-property 17"), index);
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("%s"), strMsg);

	UI_UPDATE_INFORMATION _uiInfo;
	_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
	_uiInfo.bUpdatePhaseProgress = TRUE;
	_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
	_uiInfo.bUpdateProgressInCommand = FALSE;
	_uiInfo.bUpdateCommandsProgress = TRUE;
	_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
	_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_RUNNING;
	_uiInfo.bUpdateDescription = TRUE;
	CString strDesc = _T("Get Secure State");
	_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
	strDesc.ReleaseBuffer();
	((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

	CString csCmdBody = _T("get-property 17");
	CString csCmdText = _T("Get Property 17(Secure State)");

	csCmdText.AppendFormat(_T(" [WndIndex:%d] "), index);
	CString peripheral = _T("--usb ");
	DWORD retValue = MFGLIB_ERROR_SUCCESS;
	if (((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDeviceType() == DeviceClass::DeviceTypeKBLCDC)
	{
		CDCDevice* pCDCDevice = dynamic_cast<CDCDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		if (pCDCDevice == NULL)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("PortMgrDlg(%d)--No CDCDevice"), index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = FALSE;
			_uiInfo.bUpdateProgressInCommand = FALSE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
			_uiInfo.bUpdateDescription = TRUE;
			CString strDesc;
			strDesc.Format(_T("\"Execute\" error"));
			_tcscpy(_uiInfo.strDescription, strDesc.GetBuffer());
			strDesc.ReleaseBuffer();
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
			return MFGLIB_ERROR_INVALID_VALUE;
		}
		peripheral = CString("-p ") + CString(pCDCDevice->m_commport);
	}
	else
	{
		MxHidDevice* pHidDevice = dynamic_cast<MxHidDevice*>(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_p_usb_port->GetDevice());
		peripheral.Append(pHidDevice->_path.get());
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("PortMgrDlg(%d)--Path=%s"), index, pHidDevice->_path.get());
	}

	CString csTimeout;
	csTimeout.Format(_T("%d"), m_timeout);

	retValue = (ExecuteBlhostCommand(CString(peripheral + _T(" -t ") + csTimeout + _T(" -- ") + csCmdBody), csCmdText));

	if (retValue == ERROR_SUCCESS)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, csCmdText, index);
		_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
		_uiInfo.bUpdatePhaseProgress = TRUE;
		_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
		_uiInfo.bUpdateProgressInCommand = TRUE;
		_uiInfo.bUpdateDescription = TRUE;
		_uiInfo.bUpdateCommandsProgress = TRUE;
		_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
		_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
		((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

		BLHOST_RESULT blhostResult;
		Parse_blhost_output_for_response(csCmdText, &blhostResult);
		if (_ttoi(blhostResult.Response))
		{
			*secureState = HAB_t::HabEnabled;
		}
		else
		{
			*secureState = HAB_t::HabDisabled;
		}
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Secure State = %s"), (*secureState == HAB_t::HabEnabled) ? _T("SECURE") : _T("UNSECURE"));

		return MFGLIB_ERROR_SUCCESS;
	}
	else
	{
		BLHOST_RESULT blhostResult;
		Parse_blhost_output_for_error_code(csCmdText, &blhostResult);
		if (blhostResult.error_code == KBL_Status_UnknownProperty)
		{

			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, csCmdText, index);
			_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
			_uiInfo.bUpdatePhaseProgress = TRUE;
			_uiInfo.CurrentPhaseIndex = (int)(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_currentState);
			_uiInfo.bUpdateProgressInCommand = TRUE;
			_uiInfo.bUpdateDescription = TRUE;
			_uiInfo.bUpdateCommandsProgress = TRUE;
			_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
			_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_COMPLETE;
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);

			*secureState = HAB_t::HabDisabled;

			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Not support to check secure state, treat as UNSECURE"));

			return MFGLIB_ERROR_SUCCESS;
		}

		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, csCmdText, index);
		_uiInfo.OperationID = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOpThreadID[index];
		_uiInfo.bUpdatePhaseProgress = FALSE;
		_uiInfo.bUpdateProgressInCommand = FALSE;
		_uiInfo.bUpdateDescription = FALSE;
		_uiInfo.bUpdateCommandsProgress = TRUE;
		_uiInfo.CommandsProgressIndex = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_dwCmdIndex;
		_uiInfo.CommandStatus = COMMAND_STATUS_EXECUTE_ERROR;
		((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->ExecuteUIUpdate(&_uiInfo);
		return MFGLIB_ERROR_CMD_EXECUTE_FAILED;
	}

	return retValue;
}

void COpCmd_Blhost::Parse_blhost_output_for_error_code(CString& jsonStream, BLHOST_RESULT* pblhostResult)
{
	int index = 0;
	index = jsonStream.Find(_T("\"value\" : "), index) - 1 + sizeof("\"value\" : ");
	pblhostResult->error_code = _ttoi(jsonStream.Mid(index, jsonStream.Find(_T("\r\n"), index) - index));
}

void COpCmd_Blhost::Parse_blhost_output_for_response(CString& jsonStream, BLHOST_RESULT* pblhostResult)
{
	int index = 0;
	index = jsonStream.Find(_T("\"response\" : [ "), index) - 1 + sizeof("\"response\" : [ ");
	pblhostResult->Response = jsonStream.Mid(index, jsonStream.Find(_T(" ]"), index) - index);
}

DWORD COpCmd_Blhost::ExecuteBlhostCommand(CString csArguments, CString& out_text)
{
	CString csExecute(BLHOST_PATH);

	if (!PathFileExists(csExecute))
	{
		out_text.AppendFormat(_T("Cannot find %s"), csExecute);
		return ERROR_FILE_NOT_FOUND;
	}

	csExecute.Append(_T(" -j ") + csArguments);

	SECURITY_ATTRIBUTES secAttr;
	secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAttr.bInheritHandle = TRUE;
	secAttr.lpSecurityDescriptor = NULL;

	HANDLE outReadPipe, outWritePipe;
	//Create pipes to write and read data
	if (!CreatePipe(&outReadPipe, &outWritePipe, &secAttr, NULL))
	{
		return GetLastError();
	}

	STARTUPINFO sInfo;
	ZeroMemory(&sInfo, sizeof(sInfo));
	PROCESS_INFORMATION pInfo;
	ZeroMemory(&pInfo, sizeof(pInfo));
	sInfo.cb = sizeof(STARTUPINFO);
	sInfo.dwFlags |= STARTF_USESTDHANDLES;
	sInfo.hStdInput = NULL;
	sInfo.hStdOutput = outWritePipe;
	sInfo.hStdError = outWritePipe;

	//Create the process here.
	if (!CreateProcess(NULL, csExecute.GetBuffer(), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &sInfo, &pInfo))
	{
		return GetLastError();
	}
	CloseHandle(pInfo.hProcess);
	CloseHandle(pInfo.hThread);
	CloseHandle(outWritePipe);
	
	//now read the output pipe here.
	CStringA ctrResultText, ctrResultBuffer;
	DWORD dwLength;
	while (ReadFile(outReadPipe, ctrResultBuffer.GetBuffer(Result_Buffer_Size), Result_Buffer_Size, &dwLength, NULL) && (dwLength != 0))
	{
		ctrResultBuffer.ReleaseBuffer();
		ctrResultText += ctrResultBuffer.Left(dwLength);
	}
	if (ctrResultText.IsEmpty())
	{
		return ERROR_GENERIC_COMMAND_FAILED;
	}
	else
	{
		out_text += ctrResultText;
	}

	BLHOST_RESULT blhostResult;
	Parse_blhost_output_for_error_code(out_text, &blhostResult);

	BOOL isReceiveSBFile = FALSE;
	if (csArguments.Find(_T("receive-sb-file")) != -1)
	{
		isReceiveSBFile = TRUE;
	}
	if ((blhostResult.error_code != KBL_Status_Success) && \
		((!isReceiveSBFile) || (isReceiveSBFile && (blhostResult.error_code != KBL_Status_AbortDataPhase))))
	{
		return ERROR_GEN_FAILURE;
	}

	return ERROR_SUCCESS;
}


/*
* COpCmd_Burn implementation

COpCmd_Burn::COpCmd_Burn()
{
	m_FileName = _T("");
}

COpCmd_Burn::~COpCmd_Burn()
{
}

void COpCmd_Burn::SetFileName(CString strFile)
{
	m_FileName = ((MFGLIB_VARS *)m_pLibVars)->g_strPath + strFile;
}

UINT COpCmd_Burn::ExecuteCommand(int index)
{
	if(m_FileName.IsEmpty())
	{
		return MFGLIB_ERROR_FILE_NOT_EXIST;
	}
	
	CString csCmdBody = GetBodyString();
	CString strTemp;
	int Index, Begin, End, i=0;
	//Search keyword: "Key" and exstract all key words
	if(csCmdBody.Find(_T("readRange:")) == 0)
	{
		Index = csCmdBody.Find(_T("section")); 
		while(Index != -1)
		{
			strTemp = _T("section");
			csCmdBody = csCmdBody.Right(csCmdBody.GetLength()-Index-strTemp.GetLength());
			Begin = csCmdBody.Find(_T("="));
			End = csCmdBody.Find(_T(","));
			if(Begin != -1 && End != -1)
			{
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].KeyWordName = csCmdBody.Mid(Begin+1, (End-Begin-1));
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].RangeBegin = GetPrivateProfileInt(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].KeyWordName, _T("START"), 1, m_FileName); 
				((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].RangeEnd = GetPrivateProfileInt(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].KeyWordName, _T("END"), 1, m_FileName );
				i++;
			}
			else
			{
				return MFGLIB_ERROR_FORMAT_MISMATCH;
			}
			Index = csCmdBody.Find(_T("section"));
		}
	}
	else if(csCmdBody.Find(_T("readValue:")) == 0)
	{
		Index = csCmdBody.Find(_T("section="));
		if(Index != -1)
		{
			strTemp = _T("section=");
			Begin = Index + strTemp.GetLength();
			End = csCmdBody.Find(_T(","));
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.Section = csCmdBody.Mid(Begin, (End-Begin));
			csCmdBody = csCmdBody.Right(csCmdBody.GetLength()-End-1);
		}
		else
		{
			return MFGLIB_ERROR_FORMAT_MISMATCH;
		}
		//Search all variables which has a format of "key"
		Begin = csCmdBody.Find(_T("key="));
		End = csCmdBody.Find(_T(","));
		while(Begin != -1 && End != -1)
		{
			strTemp = _T("key=");
			Begin += strTemp.GetLength();
			CString CurKeyWord = csCmdBody.Mid(Begin, (End-Begin));
			//Verify current key word in m_UniqueID.KeyWord list
			for(i=0;i<MAX_CNT_KEYWORD;i++)
			{
				if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].KeyWordName.CompareNoCase(CurKeyWord) == 0)
				{
					//Read current key word value
					((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].CurrentValue = GetPrivateProfileInt(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.Section, CurKeyWord, 1, m_FileName);
					//Is the value in the range specified? 
					if( (((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].CurrentValue < ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].RangeBegin) || 
                        (((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].CurrentValue >= ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].RangeEnd) )
					{
						return MFGLIB_ERROR_BEYOND_RANGE;
					}
					break;
				}
			}
			csCmdBody = csCmdBody.Right(csCmdBody.GetLength()-End-1);
			Begin = csCmdBody.Find(_T("key="));
			End = csCmdBody.Find(_T(","));
		}
	}
	else if(csCmdBody.Find(_T("write:")) == 0)
	{
		//First of all, find the name of m_UniqueID.Section we are writing to
		Index = csCmdBody.Find(_T("section="));
		if(Index != -1)
		{
			strTemp = _T("section=");
			Begin = Index + strTemp.GetLength();
			End = csCmdBody.Find(_T(","));
			((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.Section = csCmdBody.Mid(Begin, (End-Begin));
			csCmdBody = csCmdBody.Right(csCmdBody.GetLength()-End-1);
		}
		//Search all variables which has a format of $()
		Begin = csCmdBody.Find(_T("$("));
        End = csCmdBody.Find(_T(")+=")); 
		while(Begin != -1 && End != -1)
		{
			//Extract Current key word
			strTemp = _T("$(");
			Begin += strTemp.GetLength();
			CString CurKeyWord = csCmdBody.Mid(Begin, (End-Begin));
			//Extract current key value
			strTemp = _T(")+=");
			Begin = End + strTemp.GetLength();
			End = csCmdBody.Find(_T(","));
			DWORD IncreaseValue = _tcstoul(csCmdBody.Mid(Begin, (End-Begin)), NULL, 16);
			//Find current key word in m_UniqueID.KeyWord list
			for(i=0;i<MAX_CNT_KEYWORD;i++)
			{
				if(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].KeyWordName.CompareNoCase(CurKeyWord) == 0)
				{
					//Read current key word value
					((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].CurrentValue = GetPrivateProfileInt(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.Section, CurKeyWord, 1, m_FileName);
					//Change current value according to the value specified in ucl
					((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].NextValue = ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].CurrentValue + IncreaseValue;
					//Is the value in the range specified?
					if( (((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].NextValue < ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].RangeBegin) || 
                        (((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].NextValue >= ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].RangeEnd) )
					{
						return MFGLIB_ERROR_BEYOND_RANGE;
					}
					//Write the value to file specified in ucl
					CString csNextKeyWordValue;
					csNextKeyWordValue.Format(_T("0x%0x"), ((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.keyWord[i].NextValue);
					WritePrivateProfileString(((MFGLIB_VARS *)m_pLibVars)->g_CmdOperationArray[index]->m_UniqueID.Section, CurKeyWord, csNextKeyWordValue, m_FileName);
					break;
				}
			}
			csCmdBody = csCmdBody.Right(csCmdBody.GetLength()-End-1);
			Begin = csCmdBody.Find(_T("$("));
			End = csCmdBody.Find(_T(")+=")); 
		}
	}

	return MFGLIB_ERROR_SUCCESS;
}
*/
// WndIndex is port dlg index(based 0), every PortMgrDlg has its own CCmdOpreation(g_CmdOperationArray[WndIndex])
DWORD InitCmdOperation(MFGLIB_VARS *pLibVars, int WndIndex)
{
	if(NULL != pLibVars->g_CmdOperationArray[WndIndex])
	{
		delete pLibVars->g_CmdOperationArray[WndIndex];
		pLibVars->g_CmdOperationArray[WndIndex] = NULL;
	}
	pLibVars->g_CmdOperationArray[WndIndex] = new CCmdOpreation((INSTANCE_HANDLE)pLibVars, WndIndex);
	if(pLibVars->g_CmdOperationArray[WndIndex] == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Create CCmdOpreation[%d] object failed"), WndIndex);
		return MFGLIB_ERROR_NO_MEMORY;
	}

	ReplaceAllUsbPortKeyWords(pLibVars, WndIndex);

	return (pLibVars->g_CmdOperationArray[WndIndex]->Open());
}

void DeinitCmdOperation(MFGLIB_VARS *pLibVars, int WndIndex)
{
	if(pLibVars->g_CmdOperationArray[WndIndex] != NULL)
	{
		pLibVars->g_CmdOperationArray[WndIndex]->Close();
		::PostThreadMessage(pLibVars->g_CmdOperationArray[WndIndex]->m_nThreadID, WM_QUIT, 0, 0);
		// Wait for the CCmdOpreation thread to die before returning
		WaitForSingleObject(pLibVars->g_CmdOperationArray[WndIndex]->m_hThread, INFINITE);
		pLibVars->g_CmdOperationArray[WndIndex]->m_hThread = NULL;

		delete pLibVars->g_CmdOperationArray[WndIndex];
		pLibVars->g_CmdOperationArray[WndIndex] = NULL;

		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("CCmdOpreation[%d] thread is Closed"), WndIndex);
	}
}

DWORD InitDeviceManager(MFGLIB_VARS *pLibVars)
{
	if(g_pDeviceManager != NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_WARNING, _T("DeviceManager object has been existed"));
		return MFGLIB_ERROR_ALREADY_EXIST;
	}

	try
	{
		g_pDeviceManager = new DeviceManager((INSTANCE_HANDLE)pLibVars);
	}
	catch(...)
	{
		return MFGLIB_ERROR_NO_MEMORY;
	}

	if(g_pDeviceManager == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("Create DeviceManager object failed"));
		return MFGLIB_ERROR_NO_MEMORY;
	}
	
	DWORD error = MFGLIB_ERROR_SUCCESS;
	error = g_pDeviceManager->Open();

	return error;
}

void DeinitDeviceManager()
{
	if(g_pDeviceManager != NULL)
	{
		g_pDeviceManager->Close();
		delete g_pDeviceManager;
		g_pDeviceManager = NULL;
	}
}

void AutoScanDevice(MFGLIB_VARS *pLibVars, int iPortUsedNums)
{
	usb::Port*  pPortMappings[MAX_BOARD_NUMBERS] = {NULL};
	BYTE portIndex=0;
	CString strPath;
	CString strFiliter;
	OP_STATE_ARRAY *pOpStates = GetOpStates(pLibVars);
	OP_STATE_ARRAY::iterator stateIt = pOpStates->begin();
	UINT uEnablePorts = 0;

	// Get the list of the USB Hubs
	DEVICES_ARRAY HubList = g_devClasses[DeviceClass::DeviceTypeUsbHub]->Devices();
	std::list<Device*>::iterator hub;
	//poll each hub
	for ( hub = HubList.begin(); hub != HubList.end(); ++hub )
	{
		usb::Hub* pHub = dynamic_cast<usb::Hub*>(*hub);
		//poll each port in each hub
		for ( portIndex = 1; portIndex <= pHub->GetNumPorts(); ++portIndex )
		{
			usb::Port* pPort = pHub->Port(portIndex);
			if( pPort->GetDevice() != NULL )  //the port has connected a device
			{
				switch(pPort->GetDevice()->GetDeviceType())
				{
				case DeviceClass::DeviceTypeMsc:
					pPortMappings[uEnablePorts] = pPort;
					pLibVars->g_PortDevInfoArray[uEnablePorts].m_bConnected = TRUE;
					uEnablePorts++;
					if(iPortUsedNums == uEnablePorts)
					{
						goto SCAN_END;
					}
					break;
				case DeviceClass::DeviceTypeHid:
				case DeviceClass::DeviceTypeMxHid:
				case DeviceClass::DeviceTypeMxRom:
				case DeviceClass::DeviceTypeKBLCDC:
				case DeviceClass::DeviceTypeKBLHID:
					{
						strPath = pPort->GetDevice()->_hardwareIds.get();
						strPath.MakeUpper();
						stateIt = pOpStates->begin();
						for(; stateIt!=pOpStates->end(); stateIt++)
						{
							int bcdDevice = (*(stateIt))->bcdDevice;

							if(bcdDevice >= 0)
								strFiliter.Format(_T("vid_%04x&pid_%04x&rev_%04x"), (*stateIt)->uiVid, (*stateIt)->uiPid, bcdDevice);
							else
								strFiliter.Format(_T("vid_%04x&pid_%04x"), (*stateIt)->uiVid, (*stateIt)->uiPid);

							strFiliter.MakeUpper();
							if(strPath.Find(strFiliter) != -1)
							{
								pPortMappings[uEnablePorts] = pPort;
								pLibVars->g_PortDevInfoArray[uEnablePorts].m_bConnected = TRUE;
								uEnablePorts++;
								if(iPortUsedNums == uEnablePorts)
								{
									goto SCAN_END;
								}
							}
						}
					}
					break;
				}
			}
		}
	}

SCAN_END:
	//write out new port info, the port that have connected device
	//for(UINT i=0; i<uEnablePorts; i++)
	for(UINT i=0; i<MAX_BOARD_NUMBERS; i++)
	{
		CString csPanel, strTemp;
		csPanel.Format(_T("Panel%dInfo"), i);
		if(pPortMappings[i])
		{
			pLibVars->g_PortDevInfoArray[i].hubPath = pPortMappings[i]->GetName();
			pLibVars->g_PortDevInfoArray[i].portIndex = pPortMappings[i]->GetIndex();
			pLibVars->g_PortDevInfoArray[i].m_bUsed = TRUE;
			pLibVars->g_PortDevInfoArray[i].hubIndex = pPortMappings[i]->GetParentHub()->_index.get();
			Device *pDevice = pPortMappings[i]->GetDevice();
			if(pDevice->GetDeviceType() == DeviceClass::DeviceTypeMsc)
			{
				//strTemp = ((Volume*)pDevice)->_friendlyName.get();
				strTemp = _T("USB Mass Storage Device");
			}
			else
			{
				strTemp = pDevice->_description.get();
			}
			pLibVars->g_PortDevInfoArray[i].DeviceDesc = strTemp;
			g_pDeviceManager->m_bHasConnected[i] = TRUE;
		}
		else
		{
			g_pDeviceManager->m_bHasConnected[i] = FALSE;
		} 
	}
}

UINT GetStateCommandSize(MFGLIB_VARS *pLibVars, MX_DEVICE_STATE currentState)
{
	OP_COMMAND_ARRAY _opCmds = pLibVars->g_StateCommands[currentState];

	return _opCmds.size();
}

void RegisterUIInfoUpdateCallback(MFGLIB_VARS *pLibVars, OperateResultUpdateStruct *pCB)
{
	int DeviceIndex;

	DeviceIndex = FindOperationIndex(pLibVars, pCB->OperationID);

	pLibVars->g_CmdOperationArray[DeviceIndex]->mRegisterUIInfoUpdateCallback(pCB);
}

void UnregisterUIInfoUpdateCallback(MFGLIB_VARS *pLibVars, int DeviceIndex)
{
	pLibVars->g_CmdOperationArray[DeviceIndex]->mUnregisterUIInfoUpdateCallback();
}

void RegisterUIDevChangeCallback(MFGLIB_VARS *pLibVars, DeviceChangeCallbackStruct *pCB)
{
	int DeviceIndex;

	DeviceIndex = FindOperationIndex(pLibVars, pCB->OperationID);

	pLibVars->g_CmdOperationArray[DeviceIndex]->mRegisterUIDevChangeCallback(pCB);
}

void UnregisterUIDevChangeCallback(MFGLIB_VARS *pLibVars, int DeviceIndex)
{
	pLibVars->g_CmdOperationArray[DeviceIndex]->mUnregisterUIDevChangeCallback();
}

DWORD GetCurrentDeviceDesc(MFGLIB_VARS *pLibVars, int DeviceIndex, TCHAR* desc, int maxSize)
{
	CString strTemp = _T("No Device Connected");
	if(pLibVars->g_CmdOperationArray[DeviceIndex]->m_p_usb_port == NULL)
	{
		if(maxSize < strTemp.GetLength())
		{
			return MFGLIB_ERROR_SIZE_IS_SMALL;
		}
		_tcscpy(desc, strTemp.GetBuffer());
		strTemp.ReleaseBuffer();
		return MFGLIB_ERROR_SUCCESS;
	}
	Device *pDevice = pLibVars->g_CmdOperationArray[DeviceIndex]->m_p_usb_port->GetDevice();
	if(pDevice == NULL)
	{
		if(maxSize < strTemp.GetLength())
		{
			return MFGLIB_ERROR_SIZE_IS_SMALL;
		}
		_tcscpy(desc, strTemp.GetBuffer());
		strTemp.ReleaseBuffer();
		return MFGLIB_ERROR_SUCCESS;
	}
	else
	{
		CString strDesc;
		if(pDevice->_deviceClass->_deviceClassType == DeviceClass::DeviceTypeMsc)
		{
			strDesc = _T("USB Mass Storage Device");
		}
		else
		{
			strDesc = pDevice->_description.get();
		}
		
		if(strDesc.GetLength() > maxSize)
		{
			return MFGLIB_ERROR_SIZE_IS_SMALL;
		}
		else
		{
			_tcscpy(desc, strDesc.GetBuffer());
			desc[strDesc.GetLength()+1] = 0;
			return MFGLIB_ERROR_SUCCESS;
		}
	}
}

OP_STATE_ARRAY *GetOpStates(MFGLIB_VARS *pLibVars)
{
	if(pLibVars->g_OpStates.empty())
	{
		return NULL;
	}

	return &(pLibVars->g_OpStates);
}

DWORD GetOpStatesTimeout(OP_STATE_ARRAY *pOpStateArray, MX_DEVICE_STATE _state)
{
    std::vector<COpState*>::iterator stateIt = pOpStateArray->begin();
	for(; stateIt!=pOpStateArray->end(); stateIt++)
	{
		if(_state == (*stateIt)->opState)
		{
			return (*stateIt)->dwTimeout;
		}
	}
	return DEFAULT_TIMEOUT_CHANGE_STATE;
}

BOOL FindLibraryHandle(MFGLIB_VARS *pLibVars)
{
	std::vector<MFGLIB_VARS *>::iterator findit;

	findit = std::find(g_LibVarsArray.begin(), g_LibVarsArray.end(), pLibVars);
	if(findit == g_LibVarsArray.end())
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
/*
USB_PORT_NODE *FindPortPhysicalNode(PORT_ID portID)
{
	USB_PORT_NODE *pPortNode = NULL;

	for(int i=0; i<(int)(g_PortTable.size()); i++)
	{
		pPortNode = g_PortTable[i];
		if(portID == pPortNode->portID)
		{
			return pPortNode;
		}
	}

	return NULL;
}

PORT_ID FindPortLogicalID(USB_PORT_NODE *pPortNode)
{
	for(int i=0; i<(int)(g_PortTable.size()); i++)
	{
		if(g_PortTable[i]->hubPath.CompareNoCase(pPortNode->hubPath) == 0)
		{
			if(g_PortTable[i]->portIndex == pPortNode->portIndex)
			{
				return g_PortTable[i]->portID;
			}
		}
	}

	return 0;
}
*/
int FindOperationIndex(MFGLIB_VARS *pLibVars, DWORD operationID)
{
	int iret = -1;

	for(int i=0; i<pLibVars->g_iMaxBoardNum; i++)
	{
		if(pLibVars->g_CmdOpThreadID[i] == operationID)
		{
			iret = i;
			break;
		}
	}

	return iret;
}

DWORD MfgLib_SetUCLKeyWord(CHAR_t *key, CHAR_t *value)
{
	if(value == NULL)
		g_UclKeywords.erase(key);
	g_UclKeywords[key] = CString(value);
	return 0;
}

DWORD MfgLib_SetUsbPortKeyWord(UINT hub, UINT index, CHAR_t *key, CHAR_t *value)
{
	auto usb_key = std::make_tuple(hub, index, CString(key));
	if (value == NULL)
		g_UsbPortKeywords.erase(usb_key);
	g_UsbPortKeywords[usb_key] = CString(value);
	return 0;
}

CString ReplaceKeywords(CString str)
{
	std::map<CString, CString>::const_iterator it;
	for ( it=g_UclKeywords.begin(); it!=g_UclKeywords.end(); ++it )
    {
		CString key = _T("%");
		key +=	it->first;
		key += _T("%");		
		CString value = it->second;
		str.Replace(key, value);		
	}
	return str;
}
void ReplaceAllUsbPortKeyWords(MFGLIB_VARS *pLibVars, int WndIndex)
{
	auto& hub = pLibVars->g_PortDevInfoArray[WndIndex].hubIndex;
	auto& port = pLibVars->g_PortDevInfoArray[WndIndex].portIndex;
	if (hub && port)
	{
		for (auto& stateIt : pLibVars->g_StateCommands)
		{
			for (auto& it : stateIt.second)
			{
				it->SetBodyString(WndIndex, ReplaceUsbPortKeywords(it->GetTemplateBodyString(), hub, port));
			}
		}
	}
}
CString ReplaceUsbPortKeywords(CString str, UINT hub, UINT port)
{
	for (auto it : g_UsbPortKeywords)
	{
		if (std::get<0>(it.first) == hub && std::get<1>(it.first) == port)
		{
			CString key = _T("%");
			key += std::get<2>(it.first);
			key += _T("%");
			CString value = it.second;
			str.Replace(key, value);
		}
	}
	return str;
}
