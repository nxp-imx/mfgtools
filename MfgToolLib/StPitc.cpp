/*
 * Copyright (C) 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "StPitc.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StPitc Implementation (base class)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StPitc::StPitc(HidDevice * const pHidDevice, UCHAR *pFileDataBuf, ULONGLONG dwFileSize)
 : _pHidDevice(pHidDevice)
 , _pFileDataBuf(pFileDataBuf)
 , _dwFileSize(dwFileSize)
{
    // TODO: Register w/DeviceManager for callback if device goes away?
}
StPitc::StPitc(HidDevice * const pHidDevice)
 : _pHidDevice(pHidDevice)
 , _strResponse(_T(""))
{
}

StPitc::~StPitc(void)
{
    // TODO: Unregister w/DeviceManager?
}

UINT32 StPitc::SendPitcCommand(StApi& api)
{
    UINT8 moreInfo = 0;
    UINT32 err;

    err = _pHidDevice->SendCommand(api, &moreInfo);
    if ( err == ERROR_SUCCESS ) {
        _strResponse = api.ResponseString();
        if ( _strResponse.IsEmpty() )
            _strResponse = _T("OK");
    }
    else
    {
        _strResponse.Format(_T("Error: SendCommand(%s) failed. (%d)\r\n"), api.GetName(), err);

        CString strTemp;
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, strTemp.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        _strResponse.AppendFormat(_T("%s"), strTemp);

    }

    if ( moreInfo )
    {
        _strResponse = api.GetSendCommandErrorStr();

        HidPitcRequestSense senseApi;
        err = _pHidDevice->SendCommand(senseApi, &moreInfo);
        if ( err == ERROR_SUCCESS ) {
            _strResponse.AppendFormat(_T("\r\n%s"), senseApi.ResponseString());
            err = senseApi.GetSenseCode();
        }
    }

    return err;
}

// possible parameters
//  from resource
//  from file
//  specify resource
//  specify file
//  StFwComponent
//  std::vector<UINT8>
//  UINT8*, size
UINT32 StPitc::DownloadPitc()
{
    HidDownloadFw api(_pFileDataBuf, _dwFileSize);

    UINT32 ret = SendPitcCommand(api);

    return ret;
}

bool StPitc::IsPitcLoaded()
{
    //bool ret = false;

    //if ( _fwComponent.GetLastError() != ERROR_SUCCESS )
    //    return false;

    //HidTestUnitReady apiReady;
    //if ( SendPitcCommand(apiReady) == ERROR_SUCCESS )
    //{
    //    HidPitcInquiry apiInquiry(HidPitcInquiry::InfoPage_Pitc);
    //    if ( SendPitcCommand(apiInquiry) == ERROR_SUCCESS )
    //    {
    //        if ( apiInquiry.GetPitcId() == _fwComponent.GetId() )
    //        {
    //            return true;
    //        }
    //    }
    //}

    return true;
}
