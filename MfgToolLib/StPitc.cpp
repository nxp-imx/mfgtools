/*
 * Copyright 2009-2014, 2016 Freescale Semiconductor, Inc.
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
#if 0
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, strTemp.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
#endif
        _strResponse.AppendFormat(_T("%s"), strTemp.c_str());

    }

    if ( moreInfo )
    {
        _strResponse = api.GetSendCommandErrorStr();

        HidPitcRequestSense senseApi;
        err = _pHidDevice->SendCommand(senseApi, &moreInfo);
        if ( err == ERROR_SUCCESS ) {
            _strResponse.AppendFormat(_T("\r\n%s"), senseApi.ResponseString().c_str());
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
