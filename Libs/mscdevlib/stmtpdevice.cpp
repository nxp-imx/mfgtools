/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stheader.h"
//#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <tchar.h>
#include <objbase.h>

#include <atlbase.h>

#include "ddildl_defs.h"

#include <wmsdk.h>
#include <mswmdm.h>
//#include "stheader.h"
#include ".\stmtpdevice.h"
#include <mswmdm_i.c>
#include <sac.h>
#include <scclient.h>
#include "mtpextensions.h"


BYTE abPVK[] = {
        0x00
};
BYTE abCert[] = {
        0x00
};

BYTE abMAC[] = {0, 0, 0, 0, 0, 0, 0, 0};

#define ExitOnFail( hr )    if ( FAILED(hr) ) goto lExit;
#define ExitOnNull( x )    if ( (x) == NULL ) goto lExit;

void StripTrailingBlanks(WCHAR *str);



CStMtpDevice::CStMtpDevice(wchar_t *szMtpManufacturer, USHORT VendorId, USHORT ProductId)
{
    HRESULT hr;
    CComPtr<IComponentAuthenticate> pAuth = NULL;
//    CNotificationHandler * pCallBackObject = NULL;
    IWMDeviceManager3 * pIWMDevMgr3 = NULL;
    IWMDMEnumDevice *pEnumDevice = NULL;

    //
    // Initialize member variables
    //
    m_pSAC        = NULL;
    m_pWMDevMgr   = NULL;
    m_pWmdmDevice = NULL;


    //
    // Acquire the authentication interface of WMDM
    //
    hr = CoCreateInstance(
        CLSID_MediaDevMgr,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IComponentAuthenticate,
        (void**)&pAuth
    );
    ExitOnFail( hr );

    //
    // Create the client authentication object
    //
    m_pSAC = new CSecureChannelClient;
    ExitOnNull( m_pSAC );

    //
    // Select the cert and the associated private key into the SAC
    //
    hr = m_pSAC->SetCertificate(
        SAC_CERT_V1,
        (BYTE *)abCert, sizeof(abCert),
        (BYTE *)abPVK,  sizeof(abPVK)
    );
    ExitOnFail( hr );

    //
    // Select the authentication interface into the SAC
    //
    m_pSAC->SetInterface( pAuth );

    //
    // Authenticate with the V1 protocol
    //
    hr = m_pSAC->Authenticate( SAC_PROTOCOL_V1 );
    ExitOnFail( hr );

    //
    // Authenticate succeeded, so we can use the WMDM functionality.
    // Acquire an interface to the top-level WMDM interface.
    //
    hr = pAuth->QueryInterface( IID_IWMDeviceManager, (void**)&m_pWMDevMgr );
    ExitOnFail( hr );

    hr = m_pWMDevMgr->QueryInterface (IID_IWMDeviceManager3, (void**) &pIWMDevMgr3);

    if (SUCCEEDED (hr))
    {
        hr = pIWMDevMgr3->EnumDevices2(&pEnumDevice);
        pIWMDevMgr3->Release();
    }
    ExitOnFail( hr );

    while( TRUE )
    {
        IWMDMDevice *pWmdmDevice;
        ULONG        ulFetched;
        WCHAR       szManufacturer[128];
        WCHAR       szPathName[MAX_PATH];
//        WCHAR szMsg[128];

        hr = pEnumDevice->Next( 1, &pWmdmDevice, &ulFetched );
        if( hr != S_OK )
        {
            break;
        }
        if( ulFetched != 1 )
        {
            ExitOnFail( hr = E_UNEXPECTED );
        }

        //
        // Get manufacturer name.  The GetName method should return our Model Name from the
        // MTP DeviceInfo as it does the manufacturer name, but it does not.  It returns the
        // FriendlyName from the USB registry entry if one exists; otherwise, it returns the
        // DeviceDesc from the USB registry entry.  This must be a Windows bug.  We find another
        // way to identify our device.
        //
        hr = pWmdmDevice->GetManufacturer( szManufacturer, sizeof(szManufacturer)/sizeof(szManufacturer[0]) - 1 );
        if( FAILED(hr) )
        {
            wcscpy_s( szManufacturer, 128, L"" );
        }
//        swprintf (szMsg, 128, L"Found:\n-%s-%s-\n-%s-%s-", szManufacturer, szDeviceName, CFG_SCSI_MFG_L, CFG_SCSI_PRODUCT_L);
//        swprintf (szMsg, 128, L"Found: -%s-\n\nWant:  -%s-", szManufacturer, szMtpManufacturer);
//        MessageBox(NULL, szMsg, L"found device", MB_OK);

        StripTrailingBlanks( szManufacturer );

        if ( !_wcsicmp(szManufacturer, szMtpManufacturer) )
        {   
			// found our manufacturer
			//
			// We need the third interface for vendor commands.
			//
			IWMDMDevice3* pWmdmDevice3 = NULL;
			hr = pWmdmDevice->QueryInterface (IID_IWMDMDevice3, (void**) &pWmdmDevice3);
			if( FAILED(hr) )
			{
				m_pWmdmDevice = NULL;
				ExitOnFail( hr = E_UNEXPECTED );
			}
            hr = pWmdmDevice3->GetCanonicalName( szPathName, sizeof(szPathName)/sizeof(szPathName[0]) - 1 );
			if( FAILED(hr) )
			{
				wcscpy_s( szPathName, MAX_PATH, L"" );
				m_pWmdmDevice = NULL;
			}

			// See if this is one of our devices.
			CString vidPidString = _T("");
			vidPidString.Format(_T("Vid_%04x&Pid_%04x"), VendorId, ProductId);
			
			CString pathString = szPathName;
			if ( pathString.MakeUpper().Find(vidPidString.MakeUpper()) != -1 )
			{
				// found us
				m_pWmdmDevice = pWmdmDevice3;
				break;
			}
        }
    }

    hr = S_OK;

lExit:
    m_hrInit = hr;
}




CStMtpDevice::~CStMtpDevice(void)
{
	// Release WMDM interfaces
    //

	if (m_pWmdmDevice)
	{	// If the user has disconnected the device, Release will trap.
		m_pWmdmDevice->Release();
		m_pWmdmDevice = NULL;
	}

    if( m_pWMDevMgr )
    {	// If the user has disconnected the device, Release will trap.
        m_pWMDevMgr->Release();
        m_pWMDevMgr = NULL;
    }

    // Release the SAC
    //
    if( m_pSAC )
    {
        delete m_pSAC;
        m_pSAC = NULL;
    }

    return;}


//////////////////////////////////////////////////////////////////////
//
// Class methods
//
//////////////////////////////////////////////////////////////////////



ST_ERROR CStMtpDevice::ResetMtpDeviceForRecovery(void)
{
//	GetUpdater()->GetLogger()->Log( _T("Sending ResetToRecovery(0x97F3) cmd to MTP device.") );

	HRESULT hr;
	ST_ERROR err = STERR_NONE;
//    WCHAR szMsg[64];
    DWORD dwSize = SIZEOF_REQUIRED_COMMAND_DATA_OUT;
    MTP_COMMAND_DATA_IN MtpDataIn = {0};
    MTP_COMMAND_DATA_OUT MtpDataOut = {0};

	// This command resets the device into recovery mode without erasing bootmgr.
    MtpDataIn.OpCode = MTP_OPCODE_SIGMATEL_FORCERECV;     
    MtpDataIn.NextPhase = MTP_NEXTPHASE_NO_DATA;

    hr = m_pWmdmDevice->DeviceIoControl ( IOCTL_MTP_CUSTOM_COMMAND,
                                (BYTE *)&MtpDataIn, (DWORD)SIZEOF_REQUIRED_COMMAND_DATA_IN,
                                (BYTE *)&MtpDataOut, (LPDWORD)&dwSize);

    if (SUCCEEDED (hr) && MtpDataOut.ResponseCode == MTP_RESPONSE_OK)
	{
//		GetLogger()->Log( _T("(PASS)\r\n") );
		err = STERR_NONE;
	    //MessageBox (NULL, L"Reset OK", L"test", MB_OK);
	}
	else
	{
//		GetLogger()->Log( _T("(FAIL)\r\n") );
		err = STERR_FUNCTION_NOT_SUPPORTED;
	}

    return err;
}

BOOL CStMtpDevice::OldResetMtpDeviceForRecovery(void)
{
//	GetLogger()->Log( _T("Sending EraseBootManager(0x97F2) cmd to MTP device.") );

	HRESULT hr;
//    WCHAR szMsg[64];
    DWORD dwSize = SIZEOF_REQUIRED_COMMAND_DATA_OUT;
    MTP_COMMAND_DATA_IN MtpDataIn = {0};
    MTP_COMMAND_DATA_OUT MtpDataOut = {0};

    MtpDataIn.OpCode = MTP_OPCODE_SIGMATEL_ERASEBOOT;     
    MtpDataIn.NextPhase = MTP_NEXTPHASE_NO_DATA;

    MtpDataOut.CommandReadDataSize = 0;

    hr = m_pWmdmDevice->DeviceIoControl ( IOCTL_MTP_CUSTOM_COMMAND,
                                (BYTE *)&MtpDataIn, (DWORD)SIZEOF_REQUIRED_COMMAND_DATA_IN,
                                (BYTE *)&MtpDataOut, (LPDWORD)&dwSize);
/*
    if (SUCCEEDED (hr) && MtpDataOut.ResponseCode == MTP_RESPONSE_OK)
    {
//		GetLogger()->Log( _T("(PASS)\r\n") );
//		MessageBox (NULL, L"Erase boot OK", L"test", MB_OK);
    }
    else
    {
//		GetLogger()->Log( _T("(FAIL)\r\n") );
//      wsprintf (szMsg, L"Erase boot failed;\n\n DevIoControl = %x\nMtpResponse = %x", hr, MtpDataOut.ResponseCode);
//      MessageBox (NULL, szMsg, L"test", MB_OK);
    }
*/
//	GetLogger()->Log( _T("Sending ChipReset(0x97F1) cmd to MTP device.") );

	MtpDataIn.OpCode = MTP_OPCODE_SIGMATEL_RESET;     
    MtpDataIn.NextPhase = MTP_NEXTPHASE_NO_DATA;

    hr = m_pWmdmDevice->DeviceIoControl ( IOCTL_MTP_CUSTOM_COMMAND,
                                (BYTE *)&MtpDataIn, (DWORD)SIZEOF_REQUIRED_COMMAND_DATA_IN,
                                (BYTE *)&MtpDataOut, (LPDWORD)&dwSize);
/*
    if (SUCCEEDED (hr) && MtpDataOut.ResponseCode == MTP_RESPONSE_OK)
    {
//		GetLogger()->Log( _T("(PASS)\r\n") );
//      MessageBox (NULL, L"Reset OK", L"test", MB_OK);
    }
    else
    {
//		GetLogger()->Log( _T("(FAIL)\r\n") );
//      wsprintf (szMsg, L"Reset failed;\n\n DevIoControl = %x\nMtpResponse = %x", hr, MtpDataOut.ResponseCode);
//      MessageBox (NULL, szMsg, L"test", MB_OK);
    }
*/
    return TRUE;
}

ST_ERROR CStMtpDevice::ResetMtpDeviceToMscUpdaterMode(void)
{
    if ( m_pWmdmDevice == NULL )
		return STERR_FUNCTION_NOT_SUPPORTED;
	
//	GetLogger()->Log( _T("Sending ResetToUpdater(0x97F4) cmd to MTP device.") );

	HRESULT hr;
	ST_ERROR err = STERR_NONE;
//    WCHAR szMsg[64];
    DWORD dwSize = SIZEOF_REQUIRED_COMMAND_DATA_OUT;
    MTP_COMMAND_DATA_IN MtpDataIn = {0};
    MTP_COMMAND_DATA_OUT MtpDataOut = {0};

	// This command resets the device into MSC Updater mode without erasing bootmgr.
    MtpDataIn.OpCode = MTP_OPCODE_SIGMATEL_RESET_TO_UPDATER;     
    MtpDataIn.NextPhase = MTP_NEXTPHASE_NO_DATA;
    MtpDataIn.NumParams = 1;
    MtpDataIn.Params[0] = 0; // CStResetToUpdater::SER_NUM_NONE;

    hr = m_pWmdmDevice->DeviceIoControl ( IOCTL_MTP_CUSTOM_COMMAND,
                                (BYTE *)&MtpDataIn, (DWORD)SIZEOF_REQUIRED_COMMAND_DATA_IN,
                                (BYTE *)&MtpDataOut, (LPDWORD)&dwSize);

    if (SUCCEEDED (hr) && MtpDataOut.ResponseCode == MTP_RESPONSE_OK)
	{
//		GetLogger()->Log( _T("(PASS)\r\n") );
		err = STERR_NONE;
	    //MessageBox (NULL, L"Reset OK", L"test", MB_OK);
	}
	else
	{
//		GetLogger()->Log( _T("(FAIL)\r\n") );
		err = STERR_FUNCTION_NOT_SUPPORTED;
	}
    return err;
}
// This command gets the version of the updater.sb file on the nand if present.
ST_ERROR CStMtpDevice::MtpGetUpdaterNandVersion(CStVersionInfo& updaterVersion)
{
//	GetLogger()->Log( _T("Sending GetComponentVersionOfUpdaterOnNand(0x97F5, drvTag:0xFE) cmd to MTP device.") );

	HRESULT hr;
	ST_ERROR err = STERR_NONE;
    DWORD dwSize = SIZEOF_REQUIRED_COMMAND_DATA_OUT;
    MTP_COMMAND_DATA_IN MtpDataIn = {0};
    MTP_COMMAND_DATA_OUT MtpDataOut = {0};

    MtpDataIn.OpCode = MTP_OPCODE_SIGMATEL_GET_DRIVE_VERSION;     
    MtpDataIn.NextPhase = MTP_NEXTPHASE_NO_DATA;
	MtpDataIn.NumParams = 2;
	MtpDataIn.Params[0] = DRIVE_TAG_UPDATER_NAND_S;
	MtpDataIn.Params[1] = DriveInfoComponentVersion;

    hr = m_pWmdmDevice->DeviceIoControl ( IOCTL_MTP_CUSTOM_COMMAND,
                                (BYTE *)&MtpDataIn, (DWORD)SIZEOF_REQUIRED_COMMAND_DATA_IN,
                                (BYTE *)&MtpDataOut, (LPDWORD)&dwSize);

    if (SUCCEEDED (hr) && MtpDataOut.ResponseCode == MTP_RESPONSE_OK)
	{
//		CString msg; msg.Format(_T("PASS ver: %d.%d.%d\r\n") MtpDataOut.Params[0], MtpDataOut.Params[1], MtpDataOut.Params[2]);
//		GetLogger()->Log(msg);
		err = STERR_NONE;
		updaterVersion.SetHigh(MtpDataOut.Params[0]);
		updaterVersion.SetMid(MtpDataOut.Params[1]);
		updaterVersion.SetLow(MtpDataOut.Params[2]);
	}
	else
	{
//		GetLogger()->Log( _T("(FAIL)\r\n") );
		err = STERR_FUNCTION_NOT_SUPPORTED;
	}

    return err;
}

// This command sets the ResetToUpdater persistent bit on the STMP35xx. (post SDK3.200)
ST_ERROR CStMtpDevice::MtpSetResetToUpdaterFlag(void)
{
    HRESULT hr;
	ST_ERROR err = STERR_NONE;
    DWORD dwSize = SIZEOF_REQUIRED_COMMAND_DATA_OUT;
    MTP_COMMAND_DATA_IN MtpDataIn = {0};
    MTP_COMMAND_DATA_OUT MtpDataOut = {0};

    MtpDataIn.OpCode = MTP_OPCODE_SIGMATEL_SET_UPDATE_FLAG;     
    MtpDataIn.NextPhase = MTP_NEXTPHASE_NO_DATA;

    hr = m_pWmdmDevice->DeviceIoControl ( IOCTL_MTP_CUSTOM_COMMAND,
                                (BYTE *)&MtpDataIn, (DWORD)SIZEOF_REQUIRED_COMMAND_DATA_IN,
                                (BYTE *)&MtpDataOut, (LPDWORD)&dwSize);

    if (SUCCEEDED (hr) && MtpDataOut.ResponseCode == MTP_RESPONSE_OK)
	{
		err = STERR_NONE;
	}
	else
		err = STERR_FUNCTION_NOT_SUPPORTED;

    return err;
}

void StripTrailingBlanks(WCHAR *str)
{
	WCHAR *p = str + (wcslen(str) - 1);

	while (p != str)
	{
		if (*p != ' ')
		{
			*(p + 1)= '\0';
			break;
		}
		--p;
	}
}

/*
HRESULT CStMtpDevice::_RegisterForNotifications()
{
    HRESULT hr = S_OK;

    CComPtr<IConnectionPointContainer> spICPC;
    CComPtr<IConnectionPoint> spICP;

    if (SUCCEEDED (hr = m_pWMDevMgr->QueryInterface(IID_IConnectionPointContainer, (void**) & spICPC)))
    {
        if (SUCCEEDED (hr = spICPC->FindConnectionPoint(IID_IWMDMNotification, &spICP)))
        {
            DWORD dwCookie;
            if (SUCCEEDED (hr = spICP->Advise(m_pICallbackObject, &dwCookie)))
            {
                m_dwNotificationCookie = dwCookie;
            }
        }
    }

    return hr;
}

HRESULT CStMtpDevice::_UnregisterForNotifications()
{
    HRESULT hr = S_FALSE;

    if (-1 != m_dwNotificationCookie)
    {
        CComPtr<IConnectionPointContainer> spICPC;
        CComPtr<IConnectionPoint> spICP;

        if (SUCCEEDED (hr = m_pWMDevMgr->QueryInterface(IID_IConnectionPointContainer, (void**) & spICPC)))
        {
            if (SUCCEEDED (hr = spICPC->FindConnectionPoint(IID_IWMDMNotification, &spICP)))
            {
                if (SUCCEEDED (hr = spICP->Unadvise(m_dwNotificationCookie)))
                {
                    m_dwNotificationCookie = -1;
                    hr = S_OK;
                }
            }
        }
    }

    return hr;
}
*/

