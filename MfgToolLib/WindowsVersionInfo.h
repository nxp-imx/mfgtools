/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

// WindowsVersionInfo
#define BUFSIZE 256

class WindowsVersionInfo
{
public:
    typedef enum WIN_FAMILY_VER{ WIN_9X=0, WIN_NT};
    virtual ~WindowsVersionInfo(){};
    BOOL IsAvailable() { return m_is_available; };
    bool IsWin9x() { return ( m_family == WIN_9X ); };
    bool IsWinNT() { return ( m_family == WIN_NT ); };
    bool IsWin2K() { return ( m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion == 0 ); };  
    bool IsWmp10() { return m_bMTPSupported; };
    bool IsWinXPSP1() { return (m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion >= 1 && m_osvi.wServicePackMajor == 1); };
    bool IsWinXPSP2() { return (m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion >= 1 && m_osvi.wServicePackMajor == 2); };
    bool IsWinXPSP3() { return (m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion >= 1 && m_osvi.wServicePackMajor == 3); };
    bool IsVista()    { return (m_osvi.dwMajorVersion == 6 && m_osvi.dwMinorVersion >= 0 /*&& m_osvi.wServicePackMajor == 3*/); };
    bool IsWin7()     { return (m_osvi.dwMajorVersion == 6 && m_osvi.dwMinorVersion >= 1 /*&& m_osvi.wServicePackMajor == 3*/); };

    CString GetDescription() { return m_desc; };

    WindowsVersionInfo()
    {
        m_bMTPSupported = false;
        m_is_available = false;
        m_desc = _T("");
        m_family = (WIN_FAMILY_VER)(-1);
        // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
        // If that fails, try using the OSVERSIONINFO structure.
        ZeroMemory(&m_osvi, sizeof(OSVERSIONINFOEX));
        m_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        if( !(m_bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &m_osvi)) )
        {
            m_osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
            if (! GetVersionEx ( (OSVERSIONINFO *) &m_osvi) ) 
                return;
        }

        switch (m_osvi.dwPlatformId)
        {
            // Test for the Windows NT product family.
            case VER_PLATFORM_WIN32_NT:

                m_family = WIN_NT;

                // Test for the specific product family.
                if ( m_osvi.dwMajorVersion == 6 && m_osvi.dwMinorVersion == 1 )
                    m_desc = _T("Microsoft Windows 7 Family,\r\n");

                if ( m_osvi.dwMajorVersion == 6 && m_osvi.dwMinorVersion == 0 )
                    m_desc = _T("Microsoft Windows Vista Family,\r\n");

                if ( m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion == 2 )
                    m_desc = _T("Microsoft Windows Server 2003 Family,\r\n");

                if ( m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion == 1 )
                    m_desc = _T("Microsoft Windows XP ");

                if ( m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion == 0 )
                    m_desc = _T("Microsoft Windows 2000\r\n");

                if ( m_osvi.dwMajorVersion <= 4 )
                    m_desc = _T("Microsoft Windows NT\r\n");

                // Test for specific product on Windows NT 4.0 SP6 and later.
                if( m_bOsVersionInfoEx )
                {
                    // Test for the workstation type.
                    if ( m_osvi.wProductType == VER_NT_WORKSTATION )
                    {
                    if( m_osvi.dwMajorVersion == 4 )
                        m_desc += _T("Workstation 4.0\r\n");
                    else if( m_osvi.wSuiteMask & VER_SUITE_PERSONAL )
                        m_desc += _T("Home Edition\r\n");
                    else
                        m_desc += _T("Professional\r\n");
                    }
                    
                    // Test for the server type.
                    else if ( m_osvi.wProductType == VER_NT_SERVER )
                    {
                    if( m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion == 2 )
                    {
                        if( m_osvi.wSuiteMask & VER_SUITE_DATACENTER )
                            m_desc += _T("Datacenter Edition\r\n");
                        else if( m_osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            m_desc += _T("Enterprise Edition\r\n");
                        else if ( m_osvi.wSuiteMask == VER_SUITE_BLADE )
                            m_desc += _T("Web Edition\r\n");
                        else
                            m_desc += _T("Standard Edition\r\n");
                    }

                    else if( m_osvi.dwMajorVersion == 5 && m_osvi.dwMinorVersion == 0 )
                    {
                        if( m_osvi.wSuiteMask & VER_SUITE_DATACENTER )
                            m_desc += _T("Datacenter Server\r\n");
                        else if( m_osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            m_desc += _T("Advanced Server\r\n");
                        else
                            m_desc += _T("Server\r\n");
                    }

                    else  // Windows NT 4.0 
                    {
                        if( m_osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                            m_desc += _T("Server 4.0, Enterprise Edition\r\n");
                        else
                            m_desc += _T("Server 4.0\r\n");
                    }
                    }
                }
                else  // Test for specific product on Windows NT 4.0 SP5 and earlier
                {
                    HKEY hKey;
                    TCHAR szProductType[BUFSIZE];
                    DWORD dwBufLen=BUFSIZE;
                    LONG lRet;

                    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                    _T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                    0, KEY_QUERY_VALUE, &hKey );
                    if( lRet != ERROR_SUCCESS )
                    return;

                    lRet = RegQueryValueEx( hKey, _T("ProductType"), NULL, NULL,
                    (LPBYTE) szProductType, &dwBufLen);
                    if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
                    return;

                    RegCloseKey( hKey );

                    if ( CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szProductType, -1, _T("WINNT"), -1) == 0 )
                        m_desc += _T("Workstation ");
                    if ( CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szProductType, -1, _T("LANMANNT"), -1) == 0 )
                        m_desc += _T("Server ");
                    if ( CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szProductType, -1, _T("SERVERNT"), -1) == 0 )
                        m_desc += _T("Advanced Server ");

                    m_desc.AppendFormat( _T("%d.%d\r\n"), m_osvi.dwMajorVersion, m_osvi.dwMinorVersion );
                }

            // Display service pack (if any) and build number.

                if( m_osvi.dwMajorVersion == 4 && 
                    CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, m_osvi.szCSDVersion, -1, _T("Service Pack 6"), -1) == 0 )
                {
                    HKEY hKey;
                    LONG lRet;

                    // Test for SP6 versus SP6a.
                    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                    _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"),
                    0, KEY_QUERY_VALUE, &hKey );
                    if( lRet == ERROR_SUCCESS )
                        m_desc.AppendFormat(_T("Service Pack 6a (Build %d)\r\n"), m_osvi.dwBuildNumber & 0xFFFF );         
                    else // Windows NT 4.0 prior to SP6a
                    {
                        m_desc.AppendFormat(_T("%s (Build %d)\r\n"),
                        m_osvi.szCSDVersion,
                        m_osvi.dwBuildNumber & 0xFFFF);
                    }

                    RegCloseKey( hKey );
                }
                else // Windows NT 3.51 and earlier or Windows 2000 and later
                {
                    m_desc.AppendFormat(_T("%s (Build %d)\r\n"),
                    m_osvi.szCSDVersion,
                    m_osvi.dwBuildNumber & 0xFFFF);
                }

                break;

            // Test for the Windows 95 product family.
            case VER_PLATFORM_WIN32_WINDOWS:

                m_family = WIN_9X;
                
                if (m_osvi.dwMajorVersion == 4 && m_osvi.dwMinorVersion == 0)
                {
                    m_desc = _T("Microsoft Windows 95 ");
                    if ( m_osvi.szCSDVersion[1] == 'C' || m_osvi.szCSDVersion[1] == 'B' )
                        m_desc += _T("OSR2\r\n" );
                    else
                        m_desc += _T("\r\n" );
                } 

                if (m_osvi.dwMajorVersion == 4 && m_osvi.dwMinorVersion == 10)
                {
                    m_desc = _T("Microsoft Windows 98 ");
                    if ( m_osvi.szCSDVersion[1] == 'A' )
                        m_desc += _T("SE\r\n" );
                    else
                        m_desc += _T("\r\n" );
                } 

                if (m_osvi.dwMajorVersion == 4 && m_osvi.dwMinorVersion == 90)
                {
                    m_desc = _T("Microsoft Windows Millennium Edition\r\n");
                } 
                break;

            case VER_PLATFORM_WIN32s:

                m_desc = _T("Microsoft Win32s\r\n");
                break;
        }
           
        // Check the version of Windows Media Player
        //
        // Open the installed versions key and read the data for wmplayer.exe
        // If the version is 10.0 or higher, MTP is supported
        HKEY hMediaPlayerKey;
        CString keyName = _T("Software\\Microsoft\\MediaPlayer\\Setup\\Installed Versions");

        if (RegOpenKey (HKEY_LOCAL_MACHINE, keyName, &hMediaPlayerKey) == ERROR_SUCCESS)
        {
            ULONG typeValue = REG_BINARY;
            USHORT regValues[4] = {0,0,0,0};
            ULONG DataSize = 8;
            TCHAR *ValueName = _T("wmplayer.exe");

            RegQueryValueEx( hMediaPlayerKey,               // handle to key to query
                        ValueName,      // name of value to query
                        NULL,               // reserved
                        &typeValue,         // address of buffer for value type
                        (PUCHAR) regValues,     // address of data buffer
                        &DataSize           // address of data buffer size
                        );

            if (regValues[1] >= 10)
                m_bMTPSupported = TRUE;

            RegCloseKey(hMediaPlayerKey);
            m_desc.AppendFormat(_T("\r\nWindows Media Player Version: %02d.%02d.%02d.%04d"),regValues[1], regValues[0], regValues[3], regValues[2]);
        }

        m_is_available = true;
        return; 
    }

protected:
    OSVERSIONINFOEX m_osvi;
    bool m_bMTPSupported;
    BOOL m_bOsVersionInfoEx;
    BOOL m_is_available;
    WIN_FAMILY_VER m_family;
    CString m_desc;
};

