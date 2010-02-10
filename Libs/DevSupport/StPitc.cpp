#include "stdafx.h"
#include "StPitc.h"
#include "MxRomDevice.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StPitc Implementation (base class)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StPitc::StPitc(Device * const pDevice, LPCTSTR fileName, StFwComponent::LoadFlag loadFlag, const uint16_t langId)
 : _pDevice(pDevice)
 , _fwComponent(fileName, loadFlag, langId)
 , _strResponse(_T(""))
{
    // TODO: Register w/DeviceManager for callback if device goes away?
}
StPitc::StPitc(Device * const pDevice)
 : _pDevice(pDevice)
 , _fwComponent()
 , _strResponse(_T(""))
{
}

StPitc::~StPitc(void)
{
    // TODO: Unregister w/DeviceManager?
}

uint32_t StPitc::SendPitcCommand(StApi& api)
{
    uint8_t moreInfo = 0;
	uint32_t err; 
    
    err = _pDevice->SendCommand(api, &moreInfo);
	if ( err == ERROR_SUCCESS ) {
		_strResponse = api.ResponseString().c_str();
		if ( _strResponse.IsEmpty() )
			_strResponse = _T("OK");
	}
	else
	{
		_strResponse.Format(_T("Error: SendCommand(%s) failed. (%d)\r\n"), api.GetName(), err);

        CStdString strTemp;
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, strTemp.GetBufferSetLength(MAX_PATH), MAX_PATH, NULL);
        _strResponse.AppendFormat(_T("%s"), strTemp.c_str());
		
	}
	
	if ( moreInfo )
	{
        _strResponse = api.GetSendCommandErrorStr();

        HidPitcRequestSense senseApi;
        err = _pDevice->SendCommand(senseApi, &moreInfo);
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
//  std::vector<uint8_t>
//  uint8_t*, size
uint32_t StPitc::DownloadPitc(Device::UI_Callback callbackFn)
{
    if ( _fwComponent.GetLastError() != ERROR_SUCCESS )
        return _fwComponent.GetLastError();

    HidDownloadFw api(_fwComponent.GetDataPtr(), _fwComponent.size());
    
    HANDLE cb = _pDevice->RegisterCallback(callbackFn);
    
    uint32_t ret = SendPitcCommand(api);
    
    bool check = _pDevice->UnregisterCallback(cb);
    
    return ret;
}

uint32_t StPitc::DownloadMxRomImg(Device::UI_Callback callbackFn, unsigned int RAMKNLAddr, bool bPreload)
{
    if ( _fwComponent.GetLastError() != ERROR_SUCCESS )
        return _fwComponent.GetLastError();
    
	//MxRomDownloadFw api(_fwComponent.GetDataPtr(), _fwComponent.size());

    HANDLE cb = _pDevice->RegisterCallback(callbackFn);

    //MxRomDevice objMxRomDevice();

    BOOL ret = (dynamic_cast<MxRomDevice*>(_pDevice))->DownloadRKL((unsigned char *)_fwComponent.GetDataPtr(), _fwComponent.size(), RAMKNLAddr, bPreload);
    
    bool check = _pDevice->UnregisterCallback(cb);
    
	if(ret)
		return ERROR_SUCCESS;
	else
		return !(ERROR_SUCCESS);
}


bool StPitc::IsPitcLoaded()
{
    bool ret = false;

    if ( _fwComponent.GetLastError() != ERROR_SUCCESS )
        return false;

    HidTestUnitReady apiReady;
    if ( SendPitcCommand(apiReady) == ERROR_SUCCESS )
    {
        HidPitcInquiry apiInquiry(HidPitcInquiry::InfoPage_Pitc);
        if ( SendPitcCommand(apiInquiry) == ERROR_SUCCESS )
        {
            if ( apiInquiry.GetPitcId() == _fwComponent.GetId() )
            {
                return true;
            }
        }
    }

    return false;
}
