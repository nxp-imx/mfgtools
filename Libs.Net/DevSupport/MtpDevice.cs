/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
//using System.ComponentModel;
//using System.Diagnostics;
//using System.IO;
//using System.Runtime.InteropServices;
//using System.Reflection;

//using PortableDeviceApiLib;
//using PortableDeviceTypesLib;
//using DevSupport.WPD;
using DevSupport.Api;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A Media Transfer Protocol (MTP) device.
    /// </summary>
    public class MtpDevice : Device, ILiveUpdater, IResetToRecovery
    {
        internal MtpDevice(IntPtr deviceInstance, string path)
            : base(deviceInstance, path)
        {}

        #region ILiveUpdater Members

        public byte[] GetDeviceDataFromFile(string fileName)
        {
            throw new NotImplementedException();
        }

        public int CopyUpdateFileToMedia(string fileName)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region IResetToRecovery Members

        public int ResetToRecovery()
        {
            WpdApi.ResetToRecovery api = new WpdApi.ResetToRecovery();
            return SendCommand(api);
        }

        #endregion
    }
}
/*
MtpDevice::MtpDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
, _pWmdmDevice3(NULL)
, _responseCode(0)
{

	HRESULT hr = S_OK;

	// we need to find out which device we are trying to create. We have a path from the DevNode: Device::_path. 
	// We can compare a partial string of our _path to the CanonicalName of each of the devices enumerated by the WMDM device manager. 
	//
	// skip the first 4 characters //?/ or \\.\
	//
	CStdString devPath = _path.get().GetBuffer() + 4;
	//
	// Take everything off after the last # and convert it to upper case
	devPath = devPath.Left(devPath.ReverseFind(_T('#'))).ToUpper();


	// Go through the WMDM devices and see if our devPath key string lives in any of the WMDM device CanonicalNames
	CComPtr<IWMDMEnumDevice> pMtpDeviceEnum = NULL;
	CComPtr<IWMDeviceManager3> pWmdmDevMgr = gDeviceManager::Instance().GetWmdmDeviceManager();
	if ( pWmdmDevMgr )
	{
		hr = pWmdmDevMgr->Reinitialize();

		hr = pWmdmDevMgr->EnumDevices2(&pMtpDeviceEnum);
	}
	if ( pMtpDeviceEnum )
	{
		hr = pMtpDeviceEnum->Reset();

		while( TRUE )
		{
			IWMDMDevice*  pWmdmDevice = NULL;
			IWMDMDevice3* pWmdmDevice3 = NULL;
			ULONG         ulFetched = 0;
			WCHAR         szCanotonicalName[512] = {0};

			hr = pMtpDeviceEnum->Next( 1, &pWmdmDevice, &ulFetched );
			if( hr != S_OK )
			{
				break;
			}
			if( ulFetched != 1 )
			{
				hr = E_UNEXPECTED;
				break;
			}

			// We need the third interface for vendor commands.
			hr = pWmdmDevice->QueryInterface (IID_IWMDMDevice3, (void**) &pWmdmDevice3);
			if( FAILED(hr) )
			{
				_pWmdmDevice3 = NULL;
				return;
			}

			hr = pWmdmDevice3->GetCanonicalName( szCanotonicalName, sizeof(szCanotonicalName)/sizeof(szCanotonicalName[0]) - 1 );
			if( FAILED(hr) )
			{
				wcscpy_s( szCanotonicalName, 512, L"" );
				return;
			}

			CStdString canonicalName = szCanotonicalName;
			canonicalName.ToUpper();
			if ( canonicalName.Find(devPath) != -1 )
			{
				// FOUND OUR DEVICE. Save the third interface as our device pointer.
				_pWmdmDevice3 = pWmdmDevice3;
				break;
			}
		}
	}
}

MtpDevice::~MtpDevice(void)
{
	if ( _pWmdmDevice3 )
	{
		_pWmdmDevice3->Release();
		_pWmdmDevice3 = NULL;
	}
}
*/
/*
CStdString MtpDevice::GetSerialNumberStr()
{
    CStdString serNoStr = _T("");

	WMDMID SerialNumberStruct;
	BYTE abMAC[] = {0, 0, 0, 0, 0, 0, 0, 0};

    if ( _pWmdmDevice3 )
	{
		HRESULT hr = _pWmdmDevice3->GetSerialNumber(&SerialNumberStruct, abMAC );
		if( SUCCEEDED(hr) )
		{
			serNoStr = (LPCWSTR)SerialNumberStruct.pID;
		}
	}

	return serNoStr;
}
*/
/*
uint32_t MtpDevice::ResetChip()
{
	api::MtpDeviceReset api;

	return SendCommand(api);
}

uint32_t MtpDevice::ResetToRecovery()
{
	api::MtpResetToRecovery api;

	return SendCommand(api);
}

uint32_t MtpDevice::OldResetToRecovery()
{
	uint32_t error;
	api::MtpEraseBootmanager apiEraseBootMgr;
	api::MtpDeviceReset apiReset;

	error = SendCommand(apiEraseBootMgr);

	if ( error == ERROR_SUCCESS )
	{
		error = SendCommand(apiReset);
	}

	return error;
}
*/
/*
BOOL MtpDevice::Open()
{
	m_RecoveryHandle = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if( m_RecoveryHandle == INVALID_HANDLE_VALUE)        
    {
        m_ErrorStatus=GetLastError();
        return FALSE;
    }
 
    m_FileOverlapped.hEvent = 0;
	m_FileOverlapped.Offset = 0;
	m_FileOverlapped.OffsetHigh = 0;

    m_SyncEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	
    if( !m_SyncEvent )
	{
		m_ErrorStatus = GetLastError();
		return FALSE;
	}

    return TRUE;
}

BOOL MtpDevice::Close()
{
    if( m_SyncEvent )
	{
		CloseHandle(m_SyncEvent);
		m_SyncEvent = NULL;
	}

    if(m_RecoveryHandle)
        return CloseHandle(m_RecoveryHandle);
    else
        return 0; // error       
}

void CALLBACK MtpDevice::IoCompletion(DWORD dwErrorCode,           // completion code
								      DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
									  LPOVERLAPPED lpOverlapped)        // pointer to structure with I/O information
{
    if( ((ULONG)(ULONGLONG)lpOverlapped->hEvent != dwNumberOfBytesTransfered) || dwErrorCode )
	{
		*(BOOL *)&lpOverlapped->Offset = 0;
	}
	else
	{
		*(BOOL *)&lpOverlapped->Offset = dwNumberOfBytesTransfered;
	}

    SetEvent(m_SyncEvent);
}


// not used at this time 
CStdString MtpDevice::GetErrorStr()
{
	CStdString msg;

	return msg;
}
*/
/*
uint32_t MtpDevice::SendCommand(StApi& api, uint8_t* additionalInfo)
{

	// If it is not a MTP Api, return error.
//	if ( api.GetType() != API_TYPE_ST_MTP )
//		return ERROR_INVALID_PARAMETER;

    // tell the UI we are beginning a command.
    NotifyStruct nsInfo(api.GetName());
    nsInfo.direction = api.IsWriteCmd() ? Device::NotifyStruct::dataDir_ToDevice : Device::NotifyStruct::dataDir_FromDevice;
    Notify(nsInfo);

	// init parameter if it is used
	if (additionalInfo)
		*additionalInfo = ERROR_SUCCESS;

	// make sure the command itself is ready
	api.PrepareCommand();

    HRESULT hr;
    DWORD dwSize = SIZEOF_REQUIRED_COMMAND_DATA_OUT;
    
	MTP_COMMAND_DATA_IN MtpDataIn(*(_MTP_COMMAND_DATA_IN*)api.GetCdbPtr());
    MTP_COMMAND_DATA_OUT MtpDataOut = {0};

    hr = _pWmdmDevice3->DeviceIoControl ( IOCTL_MTP_CUSTOM_COMMAND,
                                (BYTE *)&MtpDataIn, (DWORD)SIZEOF_REQUIRED_COMMAND_DATA_IN,
                                (BYTE *)&MtpDataOut, (LPDWORD)&dwSize);

	nsInfo.error = hr;
	_responseCode = MtpDataOut.ResponseCode;
	
	if (SUCCEEDED(hr) && _responseCode == MTP_RESPONSE_OK)
	{
		nsInfo.error = ERROR_SUCCESS;
	}
	if (additionalInfo) //else
	{
		*additionalInfo = true;
	}

	if ( !api.IsWriteCmd() )
	{
		api.ProcessResponse((uint8_t*)&MtpDataOut, 0, sizeof(MtpDataOut));
	}
    
	// tell the UI we are done
    nsInfo.inProgress = false;
    Notify(nsInfo);

    return nsInfo.error;

}
//WMDM_E_NOTSUPPORTED
//E_NOTIMPL

CStdString MtpDevice::GetSendCommandErrorStr()
{
	CStdString msg;
	if ( _responseCode == MTP_RESPONSE_OK )
	{
		msg.Format(_T("MTP Response: OK(0x%04X)\r\n"), _responseCode);
	}
	else
	{
		msg.Format(_T("MTP Response: ERROR(0x%04X)\r\n"), _responseCode);
	}

	return msg;
}
*/
