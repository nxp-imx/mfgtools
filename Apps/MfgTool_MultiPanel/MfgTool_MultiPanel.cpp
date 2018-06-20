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

// MfgTool_MultiPanel.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MfgTool_MultiPanel.h"
#include "MfgTool_MultiPanelDlg.h"
#include "FslConsole.h"
#include "CommonDef.h"

#include <stack>
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG
#pragma comment(lib, "../MfgTool.exe/debug/MfgToolLib.lib")
#else
#pragma comment(lib, "../MfgTool.exe/release/MfgToolLib.lib")
#endif

/************************************************************
* Global configuration file name
************************************************************/
#define DEFAULT_CFG_FILE_NAME					_T("cfg.ini")
#define PHASE_NUMBER							64


int g_totalOps = 0;
int g_successOps = 0;
int g_failedOps = 0;

int g_PortMgrDlgNums = 1;


// CMfgTool_MultiPanelApp

BEGIN_MESSAGE_MAP(CMfgTool_MultiPanelApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CMfgTool_MultiPanelApp construction

CMfgTool_MultiPanelApp::CMfgTool_MultiPanelApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_strProfileName = _T("");
	m_strListName = _T("");
}


// The one and only CMfgTool_MultiPanelApp object

CMfgTool_MultiPanelApp theApp;

BOOL g_bConsoleApp = FALSE;
BOOL WINAPI FslConsoleHandler(DWORD dwEvent);
BOOL FslConsoleMainRouter();

void WhereXY(int *x, int *y)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo( hOut, &bInfo ); 
	COORD pos = bInfo.dwCursorPosition;
	*x = pos.X;
	*y = pos.Y;
}

void GoToXY(int x, int y)
{
	COORD pos = {x,y};
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hOut, pos);
}

void ExitFslConsole()
{
	for(int i=0; i<theApp.m_PortMgr_Array.GetSize(); i++)
	{
		CPortMgr *pPortMgr = (CPortMgr *)(theApp.m_PortMgr_Array.GetAt(i));
		pPortMgr->StopDownload();
	}
}

void OutputToConsole(CString strMsg)
{
	_tprintf(_T("%s\n"), strMsg);
}

void DetachFslConsole()
{
	Sleep(5000);

	if(g_pConsole != NULL)
	{
		g_pConsole->DeleteConsole();
		g_pConsole->Close();
		::PostThreadMessage(g_pConsole->m_nThreadID, WM_QUIT, 0, 0);
		// Wait for the CPortMgr thread to die before returning
		::WaitForSingleObject(g_pConsole->m_hThread, INFINITE);
		delete g_pConsole;
		g_pConsole = NULL;
	}
}

BOOL AttachFslConsole()
{
	g_pConsole = new CFSLConsole;
	if( NULL == g_pConsole )
	{
		return FALSE;
	}
	if(!g_pConsole->CreateConsole())
	{
		delete g_pConsole;
		g_pConsole = NULL;
		return FALSE;
	}
	BOOL bret = g_pConsole->Open();
	if(!bret)
	{
		OutputToConsole(_T("Error: Open FSL console thread failed!"));
		DetachFslConsole();
		return FALSE;
	}
	//Set console hook
	if( SetConsoleCtrlHandler((PHANDLER_ROUTINE)FslConsoleHandler, TRUE)== FALSE )
	{
		// unable to install handler... 
		OutputToConsole(_T("Error: Unable to install console hook handler!"));
		DetachFslConsole();
		return FALSE;
	}

	return TRUE;
}

void OutputInformation(CString strMsg)
{
	if(g_bConsoleApp)
	{
		OutputToConsole(strMsg);
	}
	else
	{
		AfxMessageBox(strMsg);
	}
}

// CMfgTool_MultiPanelApp initialization

BOOL CMfgTool_MultiPanelApp::InitInstance()
{
	//Get the whole command line
	LPWSTR pszCommandLine = GetCommandLineW();
	CString strCmdLine;

	strCmdLine.Format(_T("%ws"), pszCommandLine);
	BOOL bRight = ParseMyCommandLine(strCmdLine);
	if(!bRight)
	{
		return FALSE;
	}

	m_IsAutoStart = FALSE;

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	AfxEnableControlContainer();

	//Initialize COM for XML
	HRESULT hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
	if (hr != S_OK)
    {
        AfxMessageBox(_T("COM Initialization failed"));
        return FALSE;
    }

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Freescale MultiPanel"));

	LPTSTR *szArglist = NULL;
	int nArgs = 0;
	BOOL bHasCmdLineParameters = FALSE;
	szArglist = CommandLineToArgvW(pszCommandLine, &nArgs); 
	if(nArgs >= 2)
	{
		bHasCmdLineParameters = TRUE;
	}

	DWORD error;
	//Initialize MfgToolLib
	error = MfgLib_Initialize();
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		CString strMsg;
		strMsg.Format(_T("Initialize the library failed, error code: %d"), error);
		OutputInformation(strMsg);
		return FALSE;
	}

	error = MfgLib_CreateInstanceHandle(&m_pLibHandle);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		CString strMsg;
		strMsg.Format(_T("Create the instance handle failed, error code: %d"), error);
		OutputInformation(strMsg);
		return FALSE;
	}

	if(g_bConsoleApp)
	{
		CString strMsgToConsole;
		TCHAR szMsg[MAX_PATH] = {0};
		if( MfgLib_GetLibraryVersion(szMsg, MAX_PATH) == MFGLIB_ERROR_SUCCESS )
		{
			strMsgToConsole.Format(_T("Manufacture Tool (%s)"), szMsg);
		}
		else
		{
			strMsgToConsole.Format(_T("Manufacture Tool (Unknown Version)"));
		}
		OutputToConsole(strMsgToConsole);
	}

	//Get EXE path
	CString _path;
	::GetModuleFileName(NULL, _path.GetBuffer(_MAX_PATH), _MAX_PATH);
    _path.ReleaseBuffer();
	int pos = _path.ReverseFind(_T('\\'));
	_path = _path.Left(pos+1);  //+1 for add '\' at the last

	//Get used port numbers
	CString strPortNumsFilename;
	strPortNumsFilename = _path + _T("UICfg.ini");
	g_PortMgrDlgNums = ::GetPrivateProfileInt(_T("UICfg"), _T("PortMgrDlg"), 1, strPortNumsFilename);
	if(g_PortMgrDlgNums > MAX_BOARD_NUMBERS)
	{
		g_PortMgrDlgNums = DEFAULT_PORT_NUMBERS;
	}

	//Parse global configuration .ini file
	CString strCfgIni = _path + DEFAULT_CFG_FILE_NAME;
	bRight = ParseCfgFile(strCfgIni, !bHasCmdLineParameters);
	if( !bRight )
	{
		if(!bHasCmdLineParameters)
		{
			return FALSE;
		}
	}

	if(bHasCmdLineParameters)
	{
		//parse command line, we should use command line parameters
		CString strParamType;
		for(int i=1; i<nArgs; i++)
		{
			strParamType = szArglist[i];
			if( strParamType.CompareNoCase(_T("-c")) == 0 ) //chip
			{
				m_strProfileName = szArglist[i+1];
			}
			else if( ( strParamType.CompareNoCase(_T("-l")) == 0 ) ) //list
			{
				m_strListName = szArglist[i+1];
			}
			else if( ( strParamType.CompareNoCase(_T("-s")) == 0 ))
			{
				CString str = szArglist[i+1];
				str = str.Trim();
				CString key, value;
				int start = 0;
				key = str.Tokenize(_T("="), start);
				if(start>0)
					value = str.Mid(start);
				if(!key.IsEmpty())
					m_uclKeywords[key] = value;
			}
			else if( ( strParamType.CompareNoCase(_T("-p")) == 0 ) ) //Port numbers
			{
				int portNumbers = _ttoi(szArglist[i+1]);
				if(portNumbers <= MAX_BOARD_NUMBERS)
				{
					g_PortMgrDlgNums = portNumbers;
				}
				else
				{
					CString strMsg;
					strMsg.Format(_T("The used port numbers is too large!!! will use default nember: %d"), g_PortMgrDlgNums);
					OutputInformation(strMsg);
				}
			}
			else if ((strParamType.CompareNoCase(_T("-autostart")) == 0))
			{
				m_IsAutoStart = TRUE;
			}
			else if ((strParamType.CompareNoCase(_T("-u")) == 0)) // parameter specific for hub/port in the format param[hub][port]=value
			{
				CString str = szArglist[i + 1];
				str = str.Trim();

				CString key, param, value;
				auto hub = 0u;
				auto port = 0u;
				int start = 0;
				key = str.Tokenize(_T("="), start);
				if (!key.IsEmpty())
				{
					if (start > 0)
					{
						value = str.Mid(start);
					}

					start = 0;
					param = key.Tokenize(_T("["), start);
					if (start > 0)
					{
						key = key.Mid(start);
						hub = _ttoi(str.Tokenize(_T("]"), start));
						if (start > 0)
						{
							key = str.Tokenize(_T("["), start);
							start = 0;
							port = _ttoi(key.Tokenize(_T("]"), start));

							m_usbPortKeywords[std::make_tuple(hub, port, param)] = value;
						}
					}
				}
			}
		}
		LocalFree(szArglist);
	}

	std::map<CString, CString>::const_iterator it;
	for ( it=m_uclKeywords.begin(); it!=m_uclKeywords.end(); ++it )
    {
		CString key = it->first;
		CString value = it->second;
		MfgLib_SetUCLKeyWord(key.GetBuffer(), value.GetBuffer());
	}

	for (auto it : m_usbPortKeywords)
	{
		CString message;
		auto hub = std::get<0>(it.first);
		auto index = std::get<1>(it.first);
		auto param = std::get<2>(it.first);
		auto value = it.second;
		message.Format(_T("Assign %s=%s to hub=%u,port=%u"), param, value, hub, index);
		OutputInformation(message);

		MfgLib_SetUsbPortKeyWord(hub, index, param.GetBuffer(), value.GetBuffer());
	}
		
	CString strMsg;
	//set profile and list
	error = MfgLib_SetProfileName(m_pLibHandle, m_strProfileName.GetBuffer());
	m_strProfileName.ReleaseBuffer();
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		strMsg.Format(_T("Set Profile name failed"));
		OutputInformation(strMsg);
		return FALSE;
	}
	error = MfgLib_SetListName(m_pLibHandle, m_strListName.GetBuffer());
	m_strListName.ReleaseBuffer();
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		strMsg.Format(_T("Set List name failed"));
		OutputInformation(strMsg);
		return FALSE;
	}
	error = MfgLib_SetMaxBoardNumber(m_pLibHandle, g_PortMgrDlgNums);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		strMsg.Format(_T("The specified board number[%d] is invalid, should be 1~4"), g_PortMgrDlgNums);
		OutputInformation(strMsg);
		return FALSE;
	}

/*	CString strXmlFilename;
	//strXmlFilename = _path + _T("Profiles") + _T("\\") + m_strProfileName + _T("\\") + _T("OS Firmware") + _T("\\") + _T("ucl2.xml");
	strXmlFilename = _T("ucl2.xml");
	//Set ucl xml file path
	error = MfgLib_SetUCLFile(m_pLibHandle, strXmlFilename.GetBuffer());
	strXmlFilename.ReleaseBuffer();
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		strMsg.Format(_T("Set ucl file path failed"));
		OutputInformation(strMsg);
		return FALSE;
	} */

	//Initialize operation
	error = MfgLib_InitializeOperation(m_pLibHandle);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		strMsg.Format(_T("Iitialize operation failed, please refer to \"MfgTool.log\" for detailed information, error code: %d."), error);
		OutputInformation(strMsg);
		return FALSE;
	}

	//Get Ports Information
	OPERATION_INFOMATION *pOpInformation = NULL;
	pOpInformation = new OPERATION_INFOMATION[g_PortMgrDlgNums];
	if(NULL == pOpInformation)
	{
		strMsg.Format(_T("Lack of Memory!!!."));
		OutputInformation(strMsg);
		return FALSE;
	}
	m_OperationsInformation.pOperationInfo = pOpInformation;
	error = MfgLib_GetOperationInformation(m_pLibHandle, &m_OperationsInformation);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		strMsg.Format(_T("Get Operations information failed."));
		OutputInformation(strMsg);
		return FALSE;
	}
	//Get Phases information
	PHASE_INFORMATION *pPhaseInformation = NULL;
	pPhaseInformation = new PHASE_INFORMATION[PHASE_NUMBER];
	if(NULL == pPhaseInformation)
	{
		strMsg.Format(_T("Lack of Memory!!!."));
		OutputInformation(strMsg);
		return FALSE;
	}
	m_PhasesInformation.pPhaseInfo = pPhaseInformation;
	error = MfgLib_GetPhaseInformation(m_pLibHandle, &m_PhasesInformation, PHASE_NUMBER);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		strMsg.Format(_T("Get Phase information failed."));
		OutputInformation(strMsg);
		return FALSE;
	}

	error = MfgLib_RegisterCallbackFunction(theApp.m_pLibHandle, DeviceChange, gDeviceChangeNotify);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		CString strMsg;
		strMsg.Format(_T("Register %s callback function failed"), _T("Device Change"));
		OutputInformation(strMsg);
		return FALSE;
	}

	error = MfgLib_RegisterCallbackFunction(theApp.m_pLibHandle, OperateResult, gProgressUpdate);
	if(error != MFGLIB_ERROR_SUCCESS)
	{
		CString strMsg;
		strMsg.Format(_T("Register %s callback function failed"), _T("Operate Result"));
		OutputInformation(strMsg);
		return FALSE;
	}

	g_totalOps = 0;
	g_successOps = 0;
	g_failedOps = 0;
	//create port manager
	for(int i=0; i<g_PortMgrDlgNums; i++)
	{
		CPortMgr *pPortMgr = new CPortMgr(i);
		if(pPortMgr != NULL)
		{
			if(!pPortMgr->Open())
			{
				CString strMsg;
				strMsg.Format(_T("CPortMgr(%d) Open failed"), i);
				OutputInformation(strMsg);
				if(g_bConsoleApp)
				{
					CString strMsg;
					strMsg.Format(_T("detailed information please refer to \"MfgTool.log\" file"));
					OutputToConsole(strMsg);
					DetachFslConsole();
				}
				OutputInformation(strMsg);
				return FALSE;
			}
			else
			{
				m_PortMgr_Array.Add(pPortMgr);
			}
		}
		else
		{
			CString strMsg;
			strMsg.Format(_T("CPortMgr(%d) Create failed"), i);
			OutputInformation(strMsg);
			if(g_bConsoleApp)
			{
				CString strMsg;
				strMsg.Format(_T("detailed information please refer to \"MfgTool.log\" file"));
				OutputToConsole(strMsg);
				DetachFslConsole();
			}
			OutputInformation(strMsg);
			return FALSE;
		}
	}

	// go to main router
	if(g_bConsoleApp)
	{
		FslConsoleMainRouter();
	}
	else
	{
		CMfgTool_MultiPanelDlg dlg;
		m_pMainWnd = &dlg;
		dlg.m_IsAutoStart = this->m_IsAutoStart;

		INT_PTR nResponse = dlg.DoModal();
		if (nResponse == IDOK)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with OK
		}
		else if (nResponse == IDCANCEL)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with Cancel
		}
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

UINT CMfgTool_MultiPanelApp::GetStateCommandSize(int index)
{
	return m_PhasesInformation.pPhaseInfo[index].uiPhaseCommandNumbers;
}

int CMfgTool_MultiPanelApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	// delete all position information
	if(!g_VolatileMsgPosArray.empty())
	{
		std::vector<MSG_CURSOR_POSITION *>::iterator it = g_VolatileMsgPosArray.begin();
		for(; it!=g_VolatileMsgPosArray.end(); it++)
		{
			MSG_CURSOR_POSITION *pMsgPos = (*it);
			delete pMsgPos;
		}
		g_VolatileMsgPosArray.clear();
	}
	// delete all CPortMgr objects
	if(!m_PortMgr_Array.IsEmpty())
	{
		for(int i=0; i<m_PortMgr_Array.GetSize(); i++)
		{
			CPortMgr *pPortMgr = (CPortMgr *)(m_PortMgr_Array.GetAt(i));
			pPortMgr->Close();
			::PostThreadMessage(pPortMgr->m_nThreadID, WM_QUIT, 0, 0);
			// Wait for the CPortMgr thread to die before returning
			::WaitForSingleObject(pPortMgr->m_hThread, INFINITE);
			pPortMgr->m_hThread = NULL;
			delete pPortMgr;
		}
		m_PortMgr_Array.RemoveAll();
	}
	//
	if(g_bConsoleApp)
	{
		DetachFslConsole();
	}
	//
	MfgLib_UnregisterCallbackFunction(theApp.m_pLibHandle, DeviceChange, gDeviceChangeNotify);
	MfgLib_UnregisterCallbackFunction(theApp.m_pLibHandle, OperateResult, gProgressUpdate);
	//
	MfgLib_UninitializeOperation(m_pLibHandle);
	//
	if(m_OperationsInformation.pOperationInfo != NULL)
	{
		delete [] m_OperationsInformation.pOperationInfo;
	}
	//
	if(m_PhasesInformation.pPhaseInfo != NULL)
	{
		delete [] m_PhasesInformation.pPhaseInfo;
	}
	//
	MfgLib_DestoryInstanceHandle(m_pLibHandle);
	//
	MfgLib_Uninitialize();
	// COM unintialize
	CoUninitialize();

	return CWinAppEx::ExitInstance();
}

int CMfgTool_MultiPanelApp::FindDeviceIndex(DWORD operationID)
{
	int iret = 1000;

	for(int i=0; i<m_OperationsInformation.OperationInfoNumbers; i++)
	{
		if(operationID == m_OperationsInformation.pOperationInfo[i].OperationID)
		{
			iret = i;
		}
	}

	return iret;
}

BOOL CMfgTool_MultiPanelApp::ParseMyCommandLine(CString strCmdLine)
{
	//AfxMessageBox(strCmdLine);
	CString strMsg;
	CString strParameters;
	CString strParamType;
	int i = 0;
	TCHAR chQuotes = _T('\"');
	TCHAR chBar = _T('-');
	TCHAR chSpace = _T(' ');

	g_bConsoleApp = FALSE;
	//first, remove the exe path
	int index = strCmdLine.ReverseFind(_T('\\'));
	if(index != -1)
	{
		strCmdLine = strCmdLine.Right(strCmdLine.GetLength() - index);
	}
	//AfxMessageBox(strCmdLine);
	for(i=0; i<strCmdLine.GetLength(); i++)
	{
		if(strCmdLine.GetAt(i) == chSpace)
		{
			break;
		}
	}
	strParameters = strCmdLine.Right(strCmdLine.GetLength()-i);
	strParameters.TrimLeft();
	//AfxMessageBox(strParameters);
	if(strParameters.IsEmpty())
	{
		return TRUE;
	}

	//justify console or ui
	if(strParameters[0] != chBar)
	{
		strMsg.Format(_T("Error: Unexpected command format, please check your parameters"));
		AfxMessageBox(strMsg);
		return FALSE;
	}
	for(i=1; i<strParameters.GetLength(); i++)
	{
		if(strParameters.GetAt(i) == chSpace)
		{
			break;
		}
	}
	strParamType = strParameters.Left(i);
	if( strParamType.CompareNoCase(_T("-noui")) == 0 )
	{
		g_bConsoleApp = TRUE;
		strParameters = strParameters.Right(strParameters.GetLength()-i);
		strParameters.TrimLeft();
		if(!AttachFslConsole())
		{
			return FALSE;
		}
	}
	else
	{
		g_bConsoleApp = FALSE;
	}

	while(!strParameters.IsEmpty())
	{
		if(strParameters[0] != chBar)
		{
			strMsg.Format(_T("Error: Unexpected command format, please check your parameters"));
			OutputInformation(strMsg);
			return FALSE;
		}
		for(i=1; i<strParameters.GetLength(); i++)
		{
			if(strParameters.GetAt(i) == chSpace)
			{
				break;
			}
		}
		strParamType = strParameters.Left(i);
		strParameters = strParameters.Right(strParameters.GetLength()-i);
		strParameters.TrimLeft();

		if ((strParamType.CompareNoCase(_T("-c")) == 0) || (strParamType.CompareNoCase(_T("-l")) == 0) || (strParamType.CompareNoCase(_T("-s")) == 0) || (strParamType.CompareNoCase(_T("-u")) == 0))
		{
			if(strParameters[0] != chQuotes)
			{
				strMsg.Format(_T("Error: Unexpected command format, please check your parameters"));
				OutputInformation(strMsg);
				return FALSE;
			}
			for(i=1; i<strParameters.GetLength(); i++)
			{
				if(strParameters.GetAt(i) == chQuotes)
				{
					break;
				}
			}
			if(i >= strParameters.GetLength())
			{
				strMsg.Format(_T("Error: Unexpected command format, please check your parameters"));
				OutputInformation(strMsg);
				return FALSE;
			}
			else
			{
				strParameters = strParameters.Right(strParameters.GetLength()-i-1);
				strParameters.TrimLeft();
			}
		}
		else if( strParamType.CompareNoCase(_T("-p"))==0 )
		{
			for(i=1; i<strParameters.GetLength(); i++)
			{
				if(strParameters.GetAt(i) == chSpace)
				{
					break;
				}
			}
			CString strPortNumbers;
			strPortNumbers = strParameters.Mid(0, i);
			if( (strPortNumbers!=_T("1")) && (strPortNumbers!=_T("2"))
				&& (strPortNumbers!=_T("3")) && (strPortNumbers!=_T("4")) )
			{
				strMsg.Format(_T("Error: Invalid port numbers, should be 1--4"));
				OutputInformation(strMsg);
				return FALSE;
			}
			else
			{
				strParameters = strParameters.Right(strParameters.GetLength()-i);
				strParameters.TrimLeft();
			}
		}
		else if( strParamType.CompareNoCase(_T("-noui"))==0 )
		{
			strMsg.Format(_T("Error: -noui should be the second parameter"));
			OutputInformation(strMsg);
			return FALSE;
		}
		else if (strParamType.CompareNoCase(_T("-autostart")) == 0)
		{
			continue;
		}
		else
		{
			strMsg.Format(_T("Error: Invalid Parameter Type"));
			OutputInformation(strMsg);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CMfgTool_MultiPanelApp::IsSectionExist(CString strSection, CString strFilename)
{
	TCHAR ac_Result[100];
	DWORD dwRet;

	dwRet = GetPrivateProfileString((LPCTSTR)strSection, NULL, _T(""), ac_Result, 90, (LPCTSTR)strFilename);
	if(dwRet > 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CMfgTool_MultiPanelApp::ParseCfgFile(CString strFilename, BOOL bMsgBox)
{
	CString strMsg;

	CFile fileTestExist;
	if( !(fileTestExist.Open(strFilename, CFile::modeRead, NULL)) )
	{
		if(bMsgBox)
		{
			strMsg.Format(_T("%s is not existed"), strFilename);
			AfxMessageBox(strMsg);
		}
		return FALSE;
	}
	fileTestExist.Close();

	BOOL IsExisted;
	//see if [profiles] exists
	IsExisted = IsSectionExist(_T("profiles"), strFilename);
	if(!IsExisted)
	{
		if(bMsgBox)
		{
			strMsg.Format(_T("Can't find the [profiles] section in the %s."), strFilename);
			AfxMessageBox(strMsg);
		}
		return FALSE;
	}
	//see if [LIST] exists
	IsExisted = IsSectionExist(_T("LIST"), strFilename);
	if(!IsExisted)
	{
		if(bMsgBox)
		{
			strMsg.Format(_T("Can't find the [LIST] section in the %s."), strFilename);
			AfxMessageBox(strMsg);
		}
		return FALSE;
	}

	TCHAR buffer[MAX_PATH];
	::GetPrivateProfileString(_T("profiles"), _T("chip"), _T(""), buffer, MAX_PATH, strFilename);
	m_strProfileName = buffer;
	m_strProfileName.TrimLeft();
	m_strProfileName.TrimRight();
	if(m_strProfileName.IsEmpty())
	{
		if(bMsgBox)
		{
			strMsg.Format(_T("In the [profiles] section the \"chip\" key or its value is not existed in the %s."), strFilename);
			AfxMessageBox(strMsg);
		}
		return FALSE;
	}

	::GetPrivateProfileString(_T("LIST"), _T("name"), _T(""), buffer, MAX_PATH, strFilename);
	m_strListName = buffer;
	m_strListName.TrimLeft();
	m_strListName.TrimRight();
	if(m_strListName.IsEmpty())
	{
		if(bMsgBox)
		{
			strMsg.Format(_T("In the [LIST] section the \"name\" key or its value is not existed in the %s."), strFilename);
			AfxMessageBox(strMsg);
		}
		return FALSE;
	}

	if(IsSectionExist(_T("variable"), strFilename))
	{
		TCHAR *p = new TCHAR[4096];
		GetPrivateProfileSection(_T("variable"), p, 4096, strFilename);
		TCHAR *p1 = p;
		while(*p1 != 0)
		{
				CString str = p1;
				CString key, value;
				int start = 0;
				key = str.Tokenize(_T("="), start);
				if(start>=0)
					value = str.Mid(start);
				if(!key.IsEmpty())
					m_uclKeywords[key.Trim()] = value.Trim();
				p1 += str.GetLength() +1;
		}
		delete p;
	}

	return TRUE;
}

BOOL WINAPI FslConsoleHandler(DWORD dwEvent)
{
	switch(dwEvent)
	{
	case CTRL_C_EVENT:
		//MessageBox(NULL, _T("CTRL+C received!"), _T("CEvent"), MB_OK);
		ExitFslConsole();
		theApp.PostThreadMessage(WM_QUIT, 0, 0);
		return FALSE;
	case CTRL_BREAK_EVENT:
		//MessageBox(NULL, _T("CTRL+BREAK received!"), _T("CEvent"), MB_OK);
		ExitFslConsole();
		break;
	case CTRL_CLOSE_EVENT:
		//MessageBox(NULL, _T("Program being closed!"), _T("CEvent"), MB_OK);
		ExitFslConsole();
		theApp.PostThreadMessage(WM_QUIT, 0, 0);
		return FALSE;
	case CTRL_LOGOFF_EVENT:
		//MessageBox(NULL, _T("User is logging off!"), _T("CEvent"), MB_OK);
		ExitFslConsole();
		break;
	case CTRL_SHUTDOWN_EVENT:
		//MessageBox(NULL, _T("User is shuting down!"), _T("CEvent"), MB_OK);
		ExitFslConsole();
		break;
	}

	return TRUE;
}

void StartForConsole()
{
	int i = 0;
	for(i=0; i<theApp.m_PortMgr_Array.GetSize(); i++)
	{
		CString strMsg;
		CString strDevDesc;
		CPortMgr *pPortMgr = (CPortMgr *)(theApp.m_PortMgr_Array.GetAt(i));
		if(theApp.m_OperationsInformation.pOperationInfo[i].bConnected)
		{
			strDevDesc = theApp.m_OperationsInformation.pOperationInfo[i].DeviceDesc;
		}
		else
		{
			strDevDesc = _T("No Device Connected");
		}
		strMsg.Format(_T("Device %d[Hub %d--Port %d]: %s"), (i+1), theApp.m_OperationsInformation.pOperationInfo[i].HubIndex, theApp.m_OperationsInformation.pOperationInfo[i].PortIndex, strDevDesc);
		MSG_CURSOR_POSITION *pMsgPos = new MSG_CURSOR_POSITION;
		WhereXY(&(pMsgPos->x), &(pMsgPos->y));
		pMsgPos->type = (MSG_TYPE)((int)DEVICE1_DESCRIPTION + i);
		pMsgPos->length = strMsg.GetLength();
		g_VolatileMsgPosArray.push_back(pMsgPos);
		OutputToConsole(strMsg);
	}
	_tprintf(_T("Start updating......\n"));

	for(i=0; i<theApp.m_PortMgr_Array.GetSize(); i++)
	{
		CPortMgr *pPortMgr = (CPortMgr *)(theApp.m_PortMgr_Array.GetAt(i));
		MSG_CURSOR_POSITION *pMsgPos = new MSG_CURSOR_POSITION;
		WhereXY(&(pMsgPos->x), &(pMsgPos->y));
		CString strMsg;
		CString strPhase;
		if(theApp.m_OperationsInformation.pOperationInfo[i].bConnected)
		{
			MX_DEVICE_STATE devState = theApp.m_OperationsInformation.pOperationInfo[i].ConnectedDeviceState;
			switch(devState)
			{
			case MX_BOOTSTRAP:
				strPhase = _T("Bootstrap phase: ");
				break;
			case MX_UPDATER:
				strPhase = _T("Updater phase: ");
				break;
			case MX_BLHOST:
				strPhase = _T("Blhost phase: ");
				break;
			}
			strMsg.Format(_T("Device %d - %s%d%%"), (i+1), strPhase, 0);
		}
		else
		{
			strPhase = _T("Waiting for device connect......");
			strMsg.Format(_T("Device %d - %s"), (i+1), strPhase);
		}
		pMsgPos->type = (MSG_TYPE)((int)DEVICE1_UPDATE_PERCENT + i);
		pMsgPos->length = strMsg.GetLength();
		g_VolatileMsgPosArray.push_back(pMsgPos);
		OutputToConsole(strMsg);
	}

	{
		CString strMsg;
		strMsg.Format(_T("success: %d -- failed: %d"), g_successOps, g_failedOps);
		MSG_CURSOR_POSITION *pMsgPos = new MSG_CURSOR_POSITION;
		WhereXY(&(pMsgPos->x), &(pMsgPos->y));
		pMsgPos->type = SUCCESS_AND_FAIL_COUNT;
		pMsgPos->length = strMsg.GetLength();
		g_VolatileMsgPosArray.push_back(pMsgPos);
		OutputToConsole(strMsg);

		pMsgPos = new MSG_CURSOR_POSITION;
		WhereXY(&(pMsgPos->x), &(pMsgPos->y));
		pMsgPos->type = LAST_CURSOR_POSITION;
		pMsgPos->length = 0;
		g_VolatileMsgPosArray.push_back(pMsgPos);
	}

	for(i=0; i<g_PortMgrDlgNums; i++)
	{
		CPortMgr *pPortMgr = (CPortMgr *)(theApp.m_PortMgr_Array.GetAt(i));
		pPortMgr->StartDownload();
	}
}

BOOL FslConsoleMainRouter()
{
	StartForConsole();

	int nRetCode = 1;
	MSG msg;
	while(1)
	{
		nRetCode = GetMessage( &msg, ( HWND )NULL, 0, 0 );
		if ( nRetCode == 0 || nRetCode == -1 )
		{
			break;
		}
		TranslateMessage( &msg );
        DispatchMessage ( &msg );
	}

	return TRUE;
}

void ModifySpecifiedLine(MSG_CURSOR_POSITION *pMsgPos, CString strMsgNew)
{
	GoToXY(pMsgPos->x, pMsgPos->y);

	for(int i=0; i<pMsgPos->length; i++)
	{
		_tprintf(_T(" "));
	}
	//_tprintf(_T("\r"));
	GoToXY(pMsgPos->x, pMsgPos->y);

	_tprintf(_T("%s"), strMsgNew);

	pMsgPos->length = strMsgNew.GetLength();
}
