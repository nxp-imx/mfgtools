// ApiEngine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ApiEngine.h"

#include "Libs/DevSupport/StApi.h"
#include "Libs/DevSupport/DeviceManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#if !defined(_CONSOLE)
   #error Make it a console application project
#endif

//
// First load StUnicoW.dll to add unicode API support for Win9x/Me.
//
HMODULE __stdcall LoadUnicowsProc(void);

#define MICROSOFT_LAYER_FOR_UNICODE 1

extern "C" HMODULE (__stdcall *_PfnLoadUnicows)(void) = &LoadUnicowsProc;

//extern FARPROC _PfnLoadUnicows = (FARPROC) &LoadUnicowsProc;

HMODULE __stdcall LoadUnicowsProc(void)
{
	//
	// No other API should be called here.
	//
    return(LoadLibraryA("stunicow.dll"));
}

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// Instatiate the application class

DECLARE_CONSOLEAPP

CApiEngineApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Implementation of the application class

CApiEngineApp::CApiEngineApp(void) 
 : CWinApp()
 , _errorLevel(ERROR_SUCCESS)
 , _appRoot(_T(""))
 , _cmdFile(_T(""))
 , _pDevice(NULL)
{
}

CApiEngineApp::~CApiEngineApp(void)
{
}

BOOL CApiEngineApp::InitInstance()
{
//	_CrtSetBreakAlloc(259);
	if (!CWinApp::InitInstance())
	{
		return FALSE;
	}

	SetRegistryKey(_T("SigmaTel"));

	// Get the ini file from the command-line
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	_cmdFile = cmdInfo.m_strFileName;

	// Get the application path 
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR buf[_MAX_PATH];
	GetModuleFileName(NULL, buf, _MAX_PATH);
	_tsplitpath_s(buf, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);

	_appRoot = drive;
	_appRoot += dir;

    return TRUE;
}

int CApiEngineApp::Run()
{
	gDeviceManager::Instance().Open();

    if ( (_errorLevel = CheckFileName()) == ERROR_SUCCESS )
	{
		_errorLevel = ProcessCommandFile();
	}

	gDeviceManager::Instance().Close();

    return CWinApp::Run(); // calls ExitInstance and exits right away when m_pMainWnd=NULL
}

int CApiEngineApp::ExitInstance()
{
    int ret = CWinApp::ExitInstance();
	return _errorLevel;
}

int CApiEngineApp::CheckFileName()
{
	int err = ERROR_SUCCESS;
	CString usage;
	CString msg;
	
	usage.Format(_T("\r\nUsage: %s.exe [filename]\r\n\nwhere: \r\n\n\tfilename\t- Specify the command file to execute.\r\n\nreturns:\r\n\tsuccess: ERRORLEVEL == 0\r\n\terror:   ERRORLEVEL != 0\r\n"), m_pszAppName);

	if ( _cmdFile.IsEmpty() )
	{
		_tprintf(_T("\r\n ERROR - You must specify a command file to execute.\r\n\n%s"), usage);
		err = ERROR_FILE_NOT_FOUND;
	}
	else
	{
		CString full_name = _appRoot + _cmdFile;
		if ( _taccess(full_name, 0) == -1) 
		{
			_tprintf(_T("\r\n ERROR - Can not find %s.\r\n\n%s"), _cmdFile, usage);
			err = ERROR_FILE_NOT_FOUND;
		}
		else if ( _taccess(full_name, 0) == 0)
		{
			_cmdFile = full_name;
			err = ERROR_SUCCESS;
		}
	}
	return err;
}

int CApiEngineApp::ProcessCommandFile()
{
	int err = ERROR_SUCCESS;

	CStringArray cmdSections;
	INT_PTR numCmds = GetCommandSections(&cmdSections);

	for ( int index=0; index < numCmds; ++index )
	{
		err = ProcessCommandSection(cmdSections[index]);
		if ( err )
			break;
	}

	return err;
}

INT_PTR CApiEngineApp::GetCommandSections(CStringArray* sections)
{
	CString tempStr, resToken;
	DWORD ret = GetPrivateProfileSection(SEQUENCE_SECTION, tempStr.GetBufferSetLength(_MAX_PATH), _MAX_PATH, _cmdFile);
	int curPos= 0;
	resToken= tempStr.Tokenize(_T("\n"), curPos);
	while (resToken != _T(""))
	{
		sections->Add(resToken);
		resToken= tempStr.Tokenize(_T("\n"),curPos);
	};

	return sections->GetCount();
}

int CApiEngineApp::ProcessCommandSection(LPCTSTR sectionName)
{
	CString name, param, result, status, section = sectionName;
	DWORD ret = GetPrivateProfileString(sectionName, NAME_KEY, _T(""), name.GetBufferSetLength(_MAX_PATH), _MAX_PATH, _cmdFile);
	ret = GetPrivateProfileString(sectionName, PARAMETER_KEY, _T(""), param.GetBufferSetLength(_MAX_PATH), _MAX_PATH, _cmdFile);
	ret = GetPrivateProfileString(sectionName, RESULT_KEY, _T(""), result.GetBufferSetLength(_MAX_PATH), _MAX_PATH, _cmdFile);
	ret = GetPrivateProfileString(sectionName, STATUS_KEY, _T(""), status.GetBufferSetLength(_MAX_PATH), _MAX_PATH, _cmdFile);

	if ( name.Compare(_T("")) == 0 )
	{
		_tprintf(_T("\r\n ERROR - Can not find 'Name' key in '%s' section of %s.\r\n"), sectionName, _cmdFile);
		return ERROR_BADKEY;
	}

	//************************
	// Find a Device
	//************************
	if ( name.CompareNoCase(FIND_DEVICE_VALUE) == 0 )
	{
		// gotta have a parameter to know what device to find
		if ( param.Compare(_T("")) == 0 )
		{
			_tprintf(_T("\r\n ERROR - Can not find 'Parameter' key in '%s' section of %s.\r\n"), sectionName, _cmdFile);
			return ERROR_BADKEY;
		}

		// find a device
		_tprintf(_T(" Waitng for Device...\r\n"));

        _pDevice = gDeviceManager::Instance().FindDevice((LPCTSTR)param);
		if ( _pDevice == NULL )
		{
			_tprintf(_T("\r\n ERROR - Can not FindDevice(%s).\r\n"), param);
			return ERROR_NO_MORE_DEVICES;
		}

        _tprintf(_T(" Found Device: %s.\r\n"), _pDevice->_description.get().c_str());

	}
	//************************
	// Send a Command
	//************************
	else if ( section.Find(_T("Cmd")) != -1 )
	{
		// create the API
		StApi * pApi = gStApiFactory().CreateApi((LPCTSTR)name, (LPCTSTR)param);
		if ( pApi == NULL )
		{
			_tprintf(_T("\r\n ERROR - Can not CreateApi(%s, %s).\r\n"), name, param);
			return ERROR_INVALID_PARAMETER;
		}

		// send the API
		if ( _pDevice == NULL )
		{
			_tprintf(_T("\r\n ERROR - No device. Can not send '%s' command.\r\n"), pApi->GetName());
			SAFE_DELETE( pApi );
			return ERROR_NO_MORE_DEVICES;
		}

		_tprintf(_T(" Send Command: %s.\r\n"), pApi->GetName());

		CString msg, msg2;
		uint8_t moreInfo = 0;
		uint32_t err = _pDevice->SendCommand(*pApi, &moreInfo);
		if ( err == ERROR_SUCCESS ) {
			msg = pApi->ResponseString();
			if ( msg.IsEmpty() )
				msg = _T("OK");
		}
		else
		{
			msg.Format(_T("Error: SendCommand(%s) failed. (%d)\r\n"), pApi->GetName(), err);

			FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, msg2.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
			msg.Append(msg2);
			
		}
		
		if ( moreInfo )
		{
			msg.Append(_T("\r\n"));
			msg.Append(_pDevice->GetSendCommandErrorStr());
		}
		
		_tprintf(_T("  Response: %s.\r\n"), msg);

		// validate the API response

		// delete the API
		SAFE_DELETE( pApi );

	}
	else
	{
		if ( section.CompareNoCase(_T"SyncFile") == 0 )
		{

		}
	}

	return 0;
}
