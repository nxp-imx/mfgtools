/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "AutoPlayReject.h"

// Our IQueryCancelAutoplay implementation CLSID {66A32FE6-229D-427b-A608-D273F40C034C}
const CLSID CAutoPlayReject::m_CancelAutoplayClsId =
			{0x66a32fe6, 0x229d, 0x427b, 0xa6, 0x8, 0xd2, 0x73, 0xf4, 0xc, 0x3, 0x4c};

CString CAutoPlayReject::m_DrvList = _T("DEFGHIJKLMNOPQRSTUVWXYZ");

CAutoPlayReject::CAutoPlayReject()
: m_VistaDefaultDisableAutoplay((DWORD)-1)
{
	HKEY hKey;
	LPTSTR Str; 
	CString  ClsId;
	LONG err;

	OSVERSIONINFOEX osvi;
    memset(&osvi, 0, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx((OSVERSIONINFO*)&osvi);
    
	if ( osvi.dwMajorVersion >= 6 ) // Vista
	{
		DoVista();
	}

    m_ROT = 0;

    StringFromCLSID(m_CancelAutoplayClsId, &Str);
	ClsId = Str;
	CoTaskMemFree(Str);
	ClsId.Remove(_T('{')); ClsId.Remove(_T('}'));

	// add a 32-bit registry key so our IQueryCancelAutoPlay will work
	if(RegCreateKey(HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\CancelAutoplay\\CLSID"),
		&hKey) == ERROR_SUCCESS) 
	{

		if( (err = RegSetValueEx( hKey, ClsId, 0, REG_SZ, 0, 0)) != ERROR_SUCCESS )
		{
			ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Failed to create 32-bit IQueryCancelAutoPlay GUID. ***\n"));
		}

		RegCloseKey(hKey);
	}
	else 
	{
		ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Failed to create 32-bit IQueryCancelAutoPlay GUID.\n ***"));
	}

	// add a 64-bit registry key so our IQueryCancelAutoPlay will work
   	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\CancelAutoplay\\CLSID"),
		0, KEY_WOW64_64KEY | KEY_SET_VALUE,
		&hKey) == ERROR_SUCCESS) 

	{
		if( (err = RegSetValueEx( hKey, ClsId, 0, REG_SZ, 0, 0)) != ERROR_SUCCESS )
		{
			ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Failed to create 64-bit IQueryCancelAutoPlay GUID. ***\n"));
		}

		RegCloseKey(hKey);
	}
	else 
	{
		ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Failed to create 64-bit IQueryCancelAutoPlay GUID.\n ***"));
	}

}

CAutoPlayReject::~CAutoPlayReject()
{
	OSVERSIONINFOEX osvi;
    memset(&osvi, 0, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx((OSVERSIONINFO*)&osvi);

    if ( osvi.dwMajorVersion >= 6 ) // Vista
	{
		if (m_VistaDefaultDisableAutoplay != (DWORD) -1)
		{
			HKEY hKey;

			if(RegOpenKeyEx(HKEY_CURRENT_USER,
					_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers"),
					0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) 
			{
				if ( RegSetValueEx( hKey, _T("DisableAutoplay"),
						0, REG_DWORD, (const BYTE *)&m_VistaDefaultDisableAutoplay, sizeof(DWORD)) != ERROR_SUCCESS )
				{
					ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Failed to reset DisableAutoplay key. ***\n"));
				}
				else
					ATLTRACE(_T("*** CAutoPlayReject: Reset DisableAutoplay key. ***\n"));
			}
			RegCloseKey(hKey);
		}
	}

	Unregister();
}


HRESULT CAutoPlayReject::Register(LPCTSTR DrvList)
{
	if(DrvList)
		m_DrvList = DrvList;

    IMoniker* pmoniker;
    
	ATLTRACE(_T("*** CAutoPlayReject: Registering {66A32FE6-229D-427b-A608-D273F40C034C} ***\n"));

    // Create the moniker that we'll put in the ROT
    HRESULT hr = CreateClassMoniker(m_CancelAutoplayClsId, &pmoniker);
    
	if(SUCCEEDED(hr)) 
	{
        IRunningObjectTable* prot;
		/* hr = CoInitialize(0);
		if(hr == S_FALSE) // JWE just in case we are already initialized as in SDI & MDI (AfxOleInit())
			CoUninitialize();
		*/
        hr = GetRunningObjectTable(0, &prot);
        if(SUCCEEDED(hr)) 
		{
			CQueryCancelAutoplay* pQCA = new CQueryCancelAutoplay();
            if(pQCA) 
			{
	            IUnknown* punk;
                hr = pQCA->QueryInterface(IID_IUnknown, (void**)&punk);
                if (SUCCEEDED(hr)) 
				{
                    // Register...
                    hr = prot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, punk, pmoniker, &m_ROT);
                    if(SUCCEEDED(hr)) 
					{
						m_Registered = true;
						ATLTRACE(_T("*** CAutoPlayReject: Registered ***\n"));
                    }
					else 
						ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Registration failed ***\n"));
                    punk->Release();
                }
				else 
					ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Registration failed ***\n"));
                pQCA->Release();
            }
            else 
			{
                hr = E_OUTOFMEMORY;
				ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Registration failed ***\n"));
            }
            prot->Release();
        }
		else 
			ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Registration failed ***\n"));    
		pmoniker->Release();
    }
    return hr;
}

HRESULT CAutoPlayReject::Unregister(void)
{
    IRunningObjectTable *prot;

	if(m_Registered)
	{
		ATLTRACE(_T("*** CAutoPlayReject: Unregistering. ***\n"));
		if (SUCCEEDED(GetRunningObjectTable(0, &prot))) 
		{
			// Remove our instance from the ROT
			prot->Revoke(m_ROT);
			prot->Release();
		// jwe	CoUninitialize();
			m_Registered = false;
			ATLTRACE(_T("*** CAutoPlayReject: Unregistered. ***\n"));
		}
		else
			return S_FALSE;
	}

    return S_OK;
}

STDMETHODIMP CAutoPlayReject::CQueryCancelAutoplay::AllowAutoPlay(LPCWSTR pszPath,
    DWORD dwContentType, LPCWSTR pszLabel, DWORD dwSerialNumber)
{
    HRESULT hr = S_OK;
	CString drv_path = pszPath;

	// Is it the drive we want to cancel Autoplay for?
	if(drv_path.FindOneOf(CAutoPlayReject::GetDrvList()) != -1 ) {
		ATLTRACE("*** CAutoPlayReject: Rejected AutoPlay Drive: %s ***\n", pszPath);
        hr = S_FALSE;
    }

    return hr;
UNREFERENCED_PARAMETER(dwContentType);
UNREFERENCED_PARAMETER(pszLabel);
UNREFERENCED_PARAMETER(dwSerialNumber);
}

void CAutoPlayReject::DoVista()
{
	DWORD dwType;
	DWORD dwNewValue = 1;
	DWORD dwLen = sizeof(DWORD);
	HKEY hKey;

	m_VistaDefaultDisableAutoplay = (DWORD) -1;

	if(RegOpenKeyEx(HKEY_CURRENT_USER,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers"),
		0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) 
	{
		if ( RegQueryValueEx(hKey,  _T("DisableAutoplay"),
				0, &dwType, (LPBYTE) &m_VistaDefaultDisableAutoplay, &dwLen) != ERROR_SUCCESS )
		{
			ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Failed to read DisableAutoplay key. ***\n"));
		}
		else
		{
			if (!m_VistaDefaultDisableAutoplay)
			{
				if ( RegSetValueEx( hKey, _T("DisableAutoplay"),
						0, REG_DWORD, (const BYTE *)&dwNewValue, sizeof(DWORD)) != ERROR_SUCCESS )
				{
					ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Failed to set DisableAutoplay key. ***\n"));
				}
				else
					ATLTRACE(_T("*** CAutoPlayReject: Set DisableAutoplay key. ***\n"));
			}
			else
				ATLTRACE(_T("*** CAutoPlayReject: DisableAutoplay is default. ***\n"));
		}
		RegCloseKey(hKey);
	}
	else
		ATLTRACE(_T("*** CAutoPlayReject: [ERROR] Failed to open HKCU key. ***\n"));
}

///////////////////////////////////////////////////////////////////////////////
// COM boiler plate code...
//
STDMETHODIMP CAutoPlayReject::CQueryCancelAutoplay::QueryInterface(REFIID riid, void** ppv)
{
    IUnknown* punk = NULL;
    HRESULT hr = S_OK;

    if (IID_IUnknown == riid)
    {
        punk = static_cast<IUnknown*>(this);
        punk->AddRef();
    }
    else
    {
        if (IID_IQueryCancelAutoPlay == riid)
        {
            punk = static_cast<IQueryCancelAutoPlay*>(this);
            punk->AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    *ppv = punk;

    return hr;
}

STDMETHODIMP_(ULONG) CAutoPlayReject::CQueryCancelAutoplay::AddRef()
{
    return ::InterlockedIncrement((LONG*)&_cRef);
}

STDMETHODIMP_(ULONG) CAutoPlayReject::CQueryCancelAutoplay::Release()
{
    ULONG cRef = ::InterlockedDecrement((LONG*)&_cRef);

    if(!cRef)
    {
        delete this;
    }

    return cRef;
}
