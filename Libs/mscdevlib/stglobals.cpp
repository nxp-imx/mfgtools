// StGlobals.cpp: implementation of the CStGlobals class.
//
//////////////////////////////////////////////////////////////////////

#include "stheader.h"
#include "StGlobals.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStGlobals::CStGlobals(string _name):CStBase(_name)
{


    // Open the installed versions key and read the data for wmplayer.exe
    // If the version is 10.0 or higher, MTP is supported
    m_bMTPSupported = FALSE;
	PLATFORM winVersion = GetPlatform(); 
    if (winVersion == OS_XP || winVersion == OS_XP64 || winVersion == OS_VISTA32 || winVersion == OS_VISTA64 || winVersion == OS_WINDOWS7)
    {
        HKEY hMediaPlayerKey;
        TCHAR *KeyName = L"Software\\Microsoft\\MediaPlayer\\Setup\\Installed Versions";

	    if (RegOpenKey (HKEY_LOCAL_MACHINE, KeyName,
                    &hMediaPlayerKey) == ERROR_SUCCESS)
        {
            ULONG typeValue = REG_BINARY;
            USHORT regValues[4] = {0,0,0,0};
            ULONG DataSize = 8;
            TCHAR *ValueName = L"wmplayer.exe";

            RegQueryValueEx( hMediaPlayerKey,				// handle to key to query
				        ValueName,		// name of value to query
				        NULL,				// reserved
				        &typeValue,			// address of buffer for value type
				        (PUCHAR) regValues,	    // address of data buffer
				        &DataSize			// address of data buffer size
			    	    );

            if (regValues[1] >= 10)
                m_bMTPSupported = TRUE;

            RegCloseKey(hMediaPlayerKey);
        }
    }
}

CStGlobals::~CStGlobals()
{

}

long CStGlobals::GetLastError()
{ 
    return ::GetLastError();
}

PLATFORM CStGlobals::GetPlatform()
{
    OSVERSIONINFOEX osvi = {0};
//    WCHAR szMsg[100];
    
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    
    GetVersionEx((OSVERSIONINFO*)&osvi);
	
    //swprintf (szMsg, 100, L"major: %d minor %d", osvi.dwMajorVersion, osvi.dwMinorVersion);
    //MessageBox(NULL, szMsg, L"test", MB_OK);
	if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
	   	return  OS_2K;
    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
	   	return  OS_XP;
    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 2)
	   	return OS_XP64;
    else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
	{
        // Vista32 and Vista64 are returning the same version numbers ( 6.0 )
        // Need to check for something else to differentiate
        HKEY hKey;
        LONG lRet;
        lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
               L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\NtVdm64",
               0, KEY_QUERY_VALUE, &hKey );

        if( lRet == ERROR_SUCCESS )
        {
            RegCloseKey( hKey );
            return OS_VISTA64;
        }
        else
    	    return  OS_VISTA32;  
	}
    else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
		return OS_WINDOWS7;
	
    return OS_UNSUPPORTED;
}


ST_ERROR CStGlobals::SpacesToUnderScores(string& _str)
{
	for( size_t i=0; i<_str.length(); i++ )
	{
		if( _str[i] == ' ')
		{
			_str[i] = '_';
		}
	}
	return STERR_NONE;
}


size_t CStGlobals::Max(size_t _a, size_t _b)
{
	return max(_a, _b);
}

ST_ERROR CStGlobals::MakeMemoryZero(PUCHAR _buf, size_t _size)
{
	memset(_buf, 0, _size);
	return STERR_NONE;
}

HANDLE CStGlobals::CreateEvent(
	LPSECURITY_ATTRIBUTES _lpEventAttributes, 
	BOOL _bManualReset, 
	BOOL _bInitialState, 
	LPCWSTR _lpName
)
{
	return ::CreateEventW(_lpEventAttributes, _bManualReset, _bInitialState, _lpName);
}

HANDLE CStGlobals::CreateFile(
	LPCWSTR _lpFileName, 
	DWORD _dwDesiredAccess, 
	DWORD _dwShareMode,
	LPSECURITY_ATTRIBUTES _lpSecurityAttributes, 
	DWORD _dwCreationDisposition, 
	DWORD _dwFlagsAndAttributes,
	HANDLE _hTemplateFile
)
{
	return ::CreateFileW(_lpFileName, _dwDesiredAccess, _dwShareMode, _lpSecurityAttributes, 
		_dwCreationDisposition, _dwFlagsAndAttributes, _hTemplateFile);
}

BOOL CStGlobals::DeviceIoControl(
	HANDLE _hDevice, 
	DWORD _dwIoControlCode, 
	LPVOID _lpInBuffer, 
	DWORD _nInBufferSize, 
	LPVOID _lpOutBuffer, 
	DWORD _nOutBufferSize, 
	LPDWORD _lpBytesReturned, 
	LPOVERLAPPED _lpOverlapped
)
{
	return ::DeviceIoControl( _hDevice, _dwIoControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, 
		_nOutBufferSize, _lpBytesReturned, _lpOverlapped);
}


BOOL CStGlobals::WriteFileEx(
	HANDLE _hFile, 
	LPCVOID _lpBuffer, 
	DWORD _nNumberOfBytesToWrite,
	LPOVERLAPPED _lpOverlapped, 
	LPOVERLAPPED_COMPLETION_ROUTINE _lpCompletionRoutine
)
{
	return ::WriteFileEx(_hFile, _lpBuffer, _nNumberOfBytesToWrite, _lpOverlapped, _lpCompletionRoutine);
}

DWORD CStGlobals::WaitForSingleObjectEx(
	HANDLE _hHandle,
	DWORD _dwMilliseconds,
	BOOL _bAlertable
)
{
	return ::WaitForSingleObjectEx(_hHandle, _dwMilliseconds, _bAlertable);
}

BOOL CStGlobals::SetEvent(HANDLE _hEvent)
{
	return ::SetEvent(_hEvent);
}

BOOL CStGlobals::ResetEvent(HANDLE _hEvent)
{
	return ::ResetEvent(_hEvent);
}

BOOL CStGlobals::CancelIo(HANDLE _hFile)
{
	return ::CancelIo(_hFile);
}
