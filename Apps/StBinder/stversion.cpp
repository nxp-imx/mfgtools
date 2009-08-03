#include "StdAfx.h"
#include "stversion.h"
using namespace std;


CStVersion::CStVersion(LPTSTR _fname)
{
	DWORD dwFileSize = 0;
	SYSTEMTIME sysLocal = {0};
	HANDLE hFile;

	m_csDate.Empty();
	m_csFilesize.Empty();

//	if ( IsInfFile(_fname) )
//	{
//		m_FileVersionInfo.m_strFileVersion = GetInfVersion(_fname);
//	}
//	else
		m_FileVersionInfo.Create( _fname);

	m_csFilename = _fname;

	hFile = CreateFile (
                  m_csFilename,
                  GENERIC_READ,
                  FILE_SHARE_READ,
                  NULL, // no SECURITY_ATTRIBUTES structure
                  OPEN_EXISTING, // No special create flags
                  0, // No special attributes
                  NULL); // No template file

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        m_dwFilesize = GetFileSize(hFile, NULL); 
        if ( m_dwFilesize != INVALID_FILE_SIZE )
        {
            m_csFilesize.Format(L"%d", m_dwFilesize);
        }

        BY_HANDLE_FILE_INFORMATION fileInfo;
        if ( GetFileInformationByHandle(hFile, &fileInfo) )
        {
            SYSTEMTIME sysTime;
            USHORT uTagId = 0;

            if ( FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &sysTime) )
            {
				WCHAR wszDate[32];
				WCHAR wszFmt[32];

				GetLocaleInfo( LOCALE_USER_DEFAULT,				// locale identifier
						LOCALE_SSHORTDATE,					// information type
						(LPWSTR)wszFmt,		// information buffer
						32);								// size of buffer
				GetDateFormatW(  LOCALE_USER_DEFAULT, 0, &sysTime, wszFmt,
								wszDate,         // formatted string buffer
								32);
				m_csDate = wszDate;
			}
		}
		CloseHandle(hFile);
	}
}

/*
BOOL CStVersion::IsInfFile(LPTSTR _fname)
{
}


CString CStVersion::GetInfVersion(LPTSTR _fname)
{
	HINF hInf;

	hInf = SetupOpenInfFile( _fname, NULL, INF_STYLE_WIN4, NULL);
	
	if ( hInf != INVALID_HANDLE_VALUE )
	{
	}
}
*/

CStVersion::~CStVersion()
{}

void CStVersion::StGetProductVersion(CString& _ver)
{
	_ver = m_FileVersionInfo.GetProductVersion();
	_ver.Replace(L", ", L".");
}

void CStVersion::StGetFileVersion(CString& _ver)
{
	_ver = m_FileVersionInfo.GetFileVersion();
	_ver.Replace(L", ", L".");
}

void CStVersion::StGetFileDate(CString& _date)
{
	if ( !m_csDate.IsEmpty() )
		_date = m_csDate;
	else
		_date = L"";
}

void CStVersion::StGetFileSize(CString& _filesize)
{
	if ( !m_csFilesize.IsEmpty() )
		_filesize = m_csFilesize;
	else
		_filesize = L"";
}

void CStVersion::StGetOriginalFileName(CString& _originalfilename)
{
	_originalfilename = m_FileVersionInfo.GetOriginalFileName();
}


