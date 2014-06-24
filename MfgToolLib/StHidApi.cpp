/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StHidApi.cpp:
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "StHidApi.h"

////////////////////////////////////////////////////////////////////////////////
//
// ST_HID_BLTC_COMMAND: HidInquiry
//
////////////////////////////////////////////////////////////////////////////////

StApi* HidInquiry::Create(CString paramStr)
{
	return new HidInquiry(paramStr);
}

HidInquiry::HidInquiry(CString& paramStr)
	: StApiT<_ST_HID_CDB::_CDBHIDINFO>(API_TYPE_BLTC, ST_READ_CMD, _T("Inquiry"))
	, _pitcStatus(0)
	, _secConfigStatus(0)
{
	// ;param: InfoPage:InfoPage_Chip, InfoPage_PitcStatus, InfoPage_secConfig
	_params[L"InfoPage:"]                    = &_infoPage;
	_infoPage.ValueList[InfoPage_Chip]       = L"InfoPage_Chip";
	_infoPage.ValueList[InfoPage_PitcStatus] = L"InfoPage_PitcStatus";
	_infoPage.ValueList[InfoPage_secConfig]  = L"InfoPage_secConfig";
	// ;param: InfoParam:0xAABBCCDD
	_params[L"InfoParam:"]                    = &_infoParam;

    int ret = ParseParameterString(paramStr, _params);

	PrepareCommand();
}

HidInquiry::HidInquiry(const UCHAR infoPage, const UINT infoParam)
	: StApiT<_ST_HID_CDB::_CDBHIDINFO>(API_TYPE_BLTC, ST_READ_CMD, _T("Inquiry"))
	, _pitcStatus(0)
	, _secConfigStatus(0)
{
	// ;param: InfoPage:InfoPage_Chip, InfoPage_PitcStatus, InfoPage_secConfig
	_params[L"InfoPage:"]                    = &_infoPage;

	_infoPage.Desc = L"InfoPage";
	_infoPage.ValueList[0] = L"Reserved";
	_infoPage.ValueList[InfoPage_Chip]       = L"InfoPage_Chip";
	_infoPage.ValueList[InfoPage_PitcStatus] = L"InfoPage_PitcStatus";
	_infoPage.ValueList[InfoPage_secConfig]  = L"InfoPage_secConfig";
	// ;param: InfoParam:0xAABBCCDD
	_params[L"InfoParam:"]                    = &_infoParam;

	_infoParam.ValueList[0] = L"InfoParam";
	_infoParam.Desc = L"InfoParam";

	_infoPage.Value = _infoPage.Default = infoPage;
	_infoParam.Value = _infoParam.Default = infoParam;

	PrepareCommand();
}

void HidInquiry::ParseCdb()
{
	_infoPage.Value  = _cdb.InfoPage;
    _infoParam.Value = Swap4((UCHAR*)&_cdb.InfoParam);

	PrepareCommand();
}

void HidInquiry::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command   = BLTC_INQUIRY;
	_cdb.InfoPage  = _infoPage.Value;
    _cdb.InfoParam = Swap4((UCHAR*)&_infoParam.Value);

    _tag = 0;
	_xferLength = _infoPage.Value == InfoPage_Chip ? _chipInfo.size() : sizeof(_pitcStatus);

	// initialize return values
    _chipInfo.clear();
	_pitcStatus = 0;

}

void HidInquiry::ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
{
	assert( count >= _xferLength );

	if ( _cdb.InfoPage == InfoPage_Chip )
	{
		USHORT* p = (USHORT*)pData;
		_chipInfo.ChipId = p[0];
		_chipInfo.ChipRevision = p[1];
		_chipInfo.RomVersion = p[2];
		_chipInfo.RomLoaderProtocolVersion = p[3];
	}
	else if ( _cdb.InfoPage == InfoPage_PitcStatus )
	{
		_pitcStatus = *(UINT*)pData; 
	}
	else if ( _cdb.InfoPage == InfoPage_secConfig)
	{
		_secConfigStatus = *(UINT*)pData;
	}
}

const CString& HidInquiry::ResponseString()
{
	switch (_cdb.InfoPage)
	{
		case InfoPage_Chip:
		{
			_responseStr.Format(_T("Chip ID: 0x%04X\r\n"), GetChipId());
			_responseStr.AppendFormat(_T("Chip Revision: 0x%04X\r\n"), GetChipRevision());
			_responseStr.AppendFormat(_T("ROM Version: 0x%04X\r\n"), GetRomVersion());
			_responseStr.AppendFormat(_T("ROM Loader Protocol Version: 0x%04X\r\n"), GetRomLoaderProtocolVersion());
			break;
		}
		case InfoPage_PitcStatus:
		{
			switch (GetPitcStatus())
			{
				case PITC_STATUS_READY:
					_responseStr = _T("PITC Status: READY(0x00000000)");
					break;
				case PITC_STATUS_NOT_READY:
					_responseStr = _T("PITC Status: NOT_READY(0x00000001)");
					break;
				default:
					_responseStr.Format(_T("PITC Status: UNKNOWN(0x%08X)"), GetPitcStatus());
			}
			break;
		}
		case InfoPage_secConfig:
		{
			switch (GetSecConfig())
			{
				case BLTC_SEC_CONFIG_DISABLE:
					_responseStr = _T("BLTC sec Config Status: DISABLE(0x00000000)");
					break;
				case BLTC_SEC_CONFIG_FAB:
					_responseStr = _T("BLTC sec Config Status: PAB(0x00000001)");
					break;
				case BLTC_SEC_CONFIG_ENGINEERING:
					_responseStr = _T("BLTC sec Config Status: ENGINEERING(0x00000001)");
					break;
				case BLTC_SEC_CONFIG_PRODUCTION:
					_responseStr = _T("BLTC sec Config Status: PRODUCTION(0x00000001)");
					break;
				default:
					_responseStr.Format(_T("BLTC sec Config Status: UNKNOWN(0x%08X)"), GetSecConfig());
			}
			break;
		}
	default:
		_responseStr.Format(_T("Invalid Inquiry InfoPage.(0x%02X)"), _cdb.InfoPage);
	
	}
	return _responseStr;
}

USHORT HidInquiry::GetChipId() const
{ 
	if( _cdb.InfoPage != InfoPage_Chip )
		return 0;
	
	return _chipInfo.ChipId;
}
	
USHORT HidInquiry::GetChipRevision() const
{ 
	if( _cdb.InfoPage != InfoPage_Chip )
		return 0;
	
	return _chipInfo.ChipRevision;
};

USHORT HidInquiry::GetRomVersion() const
{ 
	if( _cdb.InfoPage != InfoPage_Chip )
		return 0;
	
	return _chipInfo.RomVersion;
};

USHORT HidInquiry::GetRomLoaderProtocolVersion() const
{ 
	if( _cdb.InfoPage != InfoPage_Chip )
		return 0;
	
	return _chipInfo.RomLoaderProtocolVersion;
};

UINT HidInquiry::GetPitcStatus() const
{ 
	if( _cdb.InfoPage != InfoPage_PitcStatus )
		return -1;
	
	return _pitcStatus;
};

UINT HidInquiry::GetSecConfig() const
{
	if( _cdb.InfoPage != InfoPage_secConfig )
			return -1;
		
	return _secConfigStatus;
};

//////////////////////////////////////////////////////////////////////
//
// ST_HID_BLTC_COMMAND: HidDownloadFw
//
//////////////////////////////////////////////////////////////////////

HidDownloadFw::HidDownloadFw(const UCHAR * const pData, const size_t size)
	: StApiT<_ST_HID_CDB::_CDBHIDDOWNLOAD>(API_TYPE_BLTC, ST_WRITE_CMD_PLUS_DATA, _T("DownloadFw"))
{
    SetCommandData(pData, size);
}

StApi* HidDownloadFw::Create(CString paramStr)
{
	return new HidDownloadFw(paramStr);
}

HidDownloadFw::HidDownloadFw(LPCTSTR fileName)
	: StApiT<_ST_HID_CDB::_CDBHIDDOWNLOAD>(API_TYPE_BLTC, ST_WRITE_CMD_PLUS_DATA, _T("DownloadFw"))
{
	StFwComponent fw(fileName);

    SetCommandData(fw.GetDataPtr(), fw.size());
}

// _xferLength is set in SetCommandData which calls this function.
void HidDownloadFw::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = BLTC_DOWNLOAD_FW;
	_cdb.Length = Swap4((UCHAR*)&_xferLength);

	_tag = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_HID_BLTC_COMMAND: HidBltcRequestSense
//
//////////////////////////////////////////////////////////////////////

StApi* HidBltcRequestSense::Create(CString paramStr)
{
	return new HidBltcRequestSense();
}

HidBltcRequestSense::HidBltcRequestSense()
	: StApiT<_ST_HID_CDB::_CDBHIDCMD>(API_TYPE_BLTC, ST_READ_CMD, _T("RequestSense"))
	, _senseCode(0)
{
	PrepareCommand();
}

void HidBltcRequestSense::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = BLTC_REQUEST_SENSE;

	_tag = 0;
	_xferLength = sizeof(_senseCode);

	// initialize return variable
	_senseCode = 0;
}

void HidBltcRequestSense::ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
{
	assert( count >= _xferLength );
	
	_senseCode = *(UINT *)pData;
}

const CString& HidBltcRequestSense::ResponseString()
{
	switch (GetSenseCode())
	{
		case BLTC_SENSE_NO_ERRORS:
			_responseStr.Format(_T("Sense Code (0x%08X) - No Errors.\r\n"), BLTC_SENSE_NO_ERRORS);
			break;
		case BLTC_SENSE_INVALID_CBW:
			_responseStr.Format(_T("Sense Code (0x%08X) - Invalid CBW.\r\n"), BLTC_SENSE_INVALID_CBW);
			break;
		case BLTC_SENSE_INVALID_CDB_COMMAND:
			_responseStr.Format(_T("Sense Code (0x%08X) - Invalid CDB command.\r\n"), BLTC_SENSE_INVALID_CDB_COMMAND);
			break;
		case BLTC_SENSE_INVALID_INQUIRY_PAGE:
			_responseStr.Format(_T("Sense Code (0x%08X) - Invalid Inquiry Page.\r\n"), BLTC_SENSE_INVALID_INQUIRY_PAGE);
			break;
		case BLTC_SENSE_FW_DLOAD_INVALID_LENGTH:
			_responseStr.Format(_T("Sense Code (0x%08X) - Firmware Download invalid length.\r\n"), BLTC_SENSE_FW_DLOAD_INVALID_LENGTH);
			break;
		case BLTC_SENSE_ROM_LOADER_INVALID_COMMAND:
			_responseStr.Format(_T("Sense Code (0x%08X) - ROM Loader invalid command.\r\n"), BLTC_SENSE_ROM_LOADER_INVALID_COMMAND);
			break;
		case BLTC_SENSE_ROM_LOADER_DECRYPTION_FAILURE:
			_responseStr.Format(_T("Sense Code (0x%08X) - ROM Loader decryption failure.\r\n"), BLTC_SENSE_ROM_LOADER_DECRYPTION_FAILURE);
			break;
		default:
			_responseStr.Format(_T("Sense Code (0x%08X) - Unknown.\r\n"), GetSenseCode());
			break;
	}
	return _responseStr;
}

UINT HidBltcRequestSense::GetSenseCode() const
{ 
	return _senseCode;
}

//////////////////////////////////////////////////////////////////////
//
// ST_HID_BLTC_COMMAND: HidDeviceReset
//
//////////////////////////////////////////////////////////////////////
StApi* HidDeviceReset::Create(CString paramStr)
{
	return new HidDeviceReset();
}

HidDeviceReset::HidDeviceReset()
	: StApiT<_ST_HID_CDB::_CDBHIDCMD>(API_TYPE_BLTC, ST_WRITE_CMD, _T("DeviceReset"))
{
	PrepareCommand();
}

void HidDeviceReset::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = BLTC_DEVICE_RESET;

	_tag = 0;
	_xferLength = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_HID_BLTC_COMMAND: HidDevicePowerDown
//
//////////////////////////////////////////////////////////////////////
StApi* HidDevicePowerDown::Create(CString paramStr)
{
	return new HidDevicePowerDown();
}

HidDevicePowerDown::HidDevicePowerDown()
	: StApiT<_ST_HID_CDB::_CDBHIDCMD>(API_TYPE_BLTC, ST_WRITE_CMD, _T("DevicePowerDown"))
{
	PrepareCommand();
}

void HidDevicePowerDown::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = BLTC_DEVICE_POWER_DOWN;

	_tag = 0;
	_xferLength = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_HID_PITC_COMMAND: HidTestUnitReady
//
//////////////////////////////////////////////////////////////////////

StApi* HidTestUnitReady::Create(CString paramStr)
{
	return new HidTestUnitReady();
}

HidTestUnitReady::HidTestUnitReady()
	: StApiT<_ST_HID_CDB::_CDBHIDCMD>(API_TYPE_PITC, ST_READ_CMD, _T("TestUnitReady"))
{
	PrepareCommand();
}

void HidTestUnitReady::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = PITC_TEST_UNIT_READY;

	_tag = 0;
	_xferLength = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_HID_PITC_COMMAND: HidPitcRequestSense
//
//////////////////////////////////////////////////////////////////////

StApi* HidPitcRequestSense::Create(CString paramStr)
{
	return new HidPitcRequestSense();
}

HidPitcRequestSense::HidPitcRequestSense()
	: StApiT<_ST_HID_CDB::_CDBHIDCMD>(API_TYPE_PITC, ST_READ_CMD, _T("RequestSense"))
{
	PrepareCommand();
}

void HidPitcRequestSense::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = PITC_REQUEST_SENSE;

	_tag = 0;
	_xferLength = sizeof(_senseCode);

	// initialize return value
	_senseCode = 0;
}

void HidPitcRequestSense::ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
{
	assert( count >= _xferLength );
	
	_senseCode = *(UINT *)pData;
}

const CString& HidPitcRequestSense::ResponseString()
{
	switch (GetSenseCode())
	{
		case PITC_SENSE_NO_ERRORS:
			_responseStr.Format(_T("Sense Code (0x%08X) - No Errors.\r\n"), PITC_SENSE_NO_ERRORS);
			break;
		case PITC_SENSE_INVALID_CBW:
			_responseStr.Format(_T("Sense Code (0x%08X) - Invalid CBW.\r\n"), PITC_SENSE_INVALID_CBW);
			break;
		case PITC_SENSE_INVALID_CDB_COMMAND:
			_responseStr.Format(_T("Sense Code (0x%08X) - Invalid CDB command.\r\n"), PITC_SENSE_INVALID_CDB_COMMAND);
			break;
		case PITC_SENSE_INVALID_INQUIRY_PAGE:
			_responseStr.Format(_T("Sense Code (0x%08X) - Invalid Inquiry page.\r\n"), PITC_SENSE_INVALID_INQUIRY_PAGE);
			break;
		case PITC_SENSE_NO_SENSE_INFO:
			_responseStr.Format(_T("Sense Code (0x%08X) - No Sense info.\r\n"), PITC_SENSE_NO_SENSE_INFO);
			break;
		case PITC_SENSE_INVALID_OTP_REGISTER:
			_responseStr.Format(_T("Sense Code (0x%08X) - Invalid OTP register.\r\n"), PITC_SENSE_INVALID_OTP_REGISTER);
			break;
		case PITC_SENSE_OTP_INFO_DENIED:
			_responseStr.Format(_T("Sense Code (0x%08X) - OTP Info is protected.\r\n"), PITC_SENSE_OTP_INFO_DENIED);
			break;
		case PITC_SENSE_INVALID_READ_ADDRESS:
		case PITC_SENSE_INVALID_WRITE_ADDRESS:
			_responseStr.Format(_T("Sense Code (0x%08X) - Invalid address.\r\n"), GetSenseCode());
			break;
		case PITC_SENSE_BUFFER_READ_OVERFLOW:
		case PITC_SENSE_BUFFER_WRITE_OVERFLOW:
			_responseStr.Format(_T("Sense Code (0x%08X) - Buffer overflow.\r\n"), GetSenseCode());
			break;
		case PITC_SENSE_ACCESS_READ_DENIED:
		case PITC_SENSE_ACCESS_WRITE_DENIED:
			_responseStr.Format(_T("Sense Code (0x%08X) - Access denied.\r\n"), GetSenseCode());
			break;
		default:
			_responseStr.Format(_T("Sense Code (0x%08X) - Unknown.\r\n"), GetSenseCode());
			break;
	}
	return _responseStr;
}

UINT HidPitcRequestSense::GetSenseCode() const
{ 
	return _senseCode;
}

////////////////////////////////////////////////////////////////////////////////
//
// ST_HID_PITC_COMMAND: HidPitcInquiry
//
////////////////////////////////////////////////////////////////////////////////

HidPitcInquiry::HidPitcInquiry(const UCHAR infoPage, const UINT infoParam)
	: StApiT<_ST_HID_CDB::_CDBHIDINFO>(API_TYPE_PITC, ST_READ_CMD, _T("Inquiry"))
	, _pitcInfo()
    , _pitcSenseInfo(_T(""))
	, _otpRegInfo()
    , _persistentInfo()
{
	// ;param: InfoPage:InfoPage_Pitc,InfoPage_PitcSense,InfoPage_OtpReg
	_params[L"InfoPage:"] = &_infoPage;
    _infoPage.Value = infoPage;
	_infoPage.ValueList[InfoPage_Pitc]   = L"InfoPage_Pitc";
	_infoPage.ValueList[InfoPage_PitcSense]   = L"InfoPage_PitcSense";
	_infoPage.ValueList[InfoPage_OtpReg] = L"InfoPage_OtpReg";

    // ;param: InfoParam:0xAABBCCDD
    _params[L"InfoParam:"] = &_infoParam;
    _infoParam.Value = infoParam;

    PrepareCommand();
}

StApi* HidPitcInquiry::Create(CString paramStr)
{
	return new HidPitcInquiry(paramStr);
}

HidPitcInquiry::HidPitcInquiry(CString& paramStr)
	: StApiT<_ST_HID_CDB::_CDBHIDINFO>(API_TYPE_PITC, ST_READ_CMD, _T("Inquiry"))
	, _pitcInfo()
    , _pitcSenseInfo(_T(""))
	, _otpRegInfo()
    , _persistentInfo()
{
	// ;param: InfoPage:InfoPage_Pitc,InfoPage_PitcSense,InfoPage_OtpReg,InfoPage_PersistentReg
	_params[L"InfoPage:"] = &_infoPage;
	_infoPage.ValueList[InfoPage_Pitc]   = L"InfoPage_Pitc";
	_infoPage.ValueList[InfoPage_PitcSense]   = L"InfoPage_PitcSense";
	_infoPage.ValueList[InfoPage_OtpReg] = L"InfoPage_OtpReg";
	_infoPage.ValueList[InfoPage_PersistentReg] = L"InfoPage_PersistentReg";

    // ;param: InfoParam:0xAABBCCDD
    _params[L"InfoParam:"] = &_infoParam;

	int ret = ParseParameterString(paramStr, _params);

    PrepareCommand();
}

void HidPitcInquiry::ParseCdb()
{
	_infoPage.Value = _cdb.InfoPage;
    _infoParam.Value = Swap4((UCHAR*)&_cdb.InfoParam);

	PrepareCommand();
}

void HidPitcInquiry::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = PITC_INQUIRY;
	_cdb.InfoPage = _infoPage.Value;
    _cdb.InfoParam = Swap4((UCHAR*)&_infoParam.Value);

	_tag = 0;
	switch ( _infoPage.Value )
    {
        case InfoPage_Pitc:
	        _pitcInfo.clear();
            _xferLength = _pitcInfo.size();
            break;
        case InfoPage_PitcSense:
            //_pitcSenseInfo.clear();
			_pitcSenseInfo.Empty();
            _xferLength = 64; // Spec says this is a 64-byte buffer containing a Unicode string.
            break;
        case InfoPage_OtpReg:
            _otpRegInfo.clear();
            _xferLength = _otpRegInfo.size();
            break;
        case InfoPage_PersistentReg:
            _persistentInfo.clear();
            _xferLength = _persistentInfo.size();
            break;
        default:
            _xferLength = 0;
            break;
    }
}

void HidPitcInquiry::ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
{
	assert( count >= _xferLength );

	switch ( _infoPage.Value )
    {
        case InfoPage_Pitc:
            _pitcInfo.Id = Swap4(pData);

            _pitcInfo.Version.SetMajor(Swap2(pData + sizeof(_pitcInfo.Id)));
            _pitcInfo.Version.SetMinor(Swap2(pData + sizeof(_pitcInfo.Id) + sizeof(USHORT)));
	        _pitcInfo.Version.SetRevision(Swap2(pData + sizeof(_pitcInfo.Id) + sizeof(USHORT) + sizeof(USHORT)));
            break;
        case InfoPage_PitcSense:
            _pitcSenseInfo = *(wchar_t*)pData;
            break;
        case InfoPage_OtpReg:
            _otpRegInfo.Address  = Swap4(pData);
            _otpRegInfo.LockBit  = pData[4];
            _otpRegInfo.OtpBank  = pData[5];
            _otpRegInfo.OtpWord  = pData[6];
            _otpRegInfo.Locked   = pData[7];
            _otpRegInfo.Shadowed = pData[8];
            break;
        case InfoPage_PersistentReg:
            _persistentInfo.Address  = Swap4(pData);
            _persistentInfo.Value  = Swap4(pData + sizeof(_persistentInfo.Address));
            break;
        default:
            break;
    }
}

const CString& HidPitcInquiry::ResponseString()
{
	switch (_cdb.InfoPage)
	{
		case InfoPage_Pitc:
		{
	        _responseStr.Format(_T("PITC ID: 0x%08X\r\n"), GetPitcId());
            _responseStr.AppendFormat(_T("PITC Version: %s\r\n"), GetPitcVersion().toString());
			break;
		}
		case InfoPage_PitcSense:
		{
            _responseStr.Format(_T("PITC Sense: %s(0x%08X)\r\n"), GetPitcSenseString(), _infoParam.Value);
			break;
		}
		case InfoPage_OtpReg:
		{
            _responseStr.Format(_T("OTP Register: %d\r\n"), _infoParam.Value);
            _responseStr.AppendFormat(_T(" Address: 0x%08X\r\n"), GetOtpRegAddress());
            _responseStr.AppendFormat(_T(" Lock Bit: 0x%02X\r\n"), GetOtpRegLockBit());
            _responseStr.AppendFormat(_T(" OTP Bank: 0x%02X\r\n"), GetOtpRegBank());
            _responseStr.AppendFormat(_T(" OTP Word: 0x%02X\r\n"), GetOtpRegWord());
            _responseStr.AppendFormat(_T(" Locked: %s\r\n"), IsOtpRegLocked()?_T("true"):_T("false"));
            _responseStr.AppendFormat(_T(" Shadowed: %s\r\n"), IsOtpRegShadowed()?_T("true"):_T("false"));
			break;
		}
		case InfoPage_PersistentReg:
		{
            _responseStr.Format(_T("Persistent Register: %d\r\n"), _infoParam.Value);
            _responseStr.AppendFormat(_T(" Address: 0x%08X\r\n"), GetPersistentRegAddress());
            _responseStr.AppendFormat(_T(" Value: 0x%08X\r\n"), GetPersistentRegValue());
			break;
		}
	default:
		_responseStr.Format(_T("Invalid Inquiry InfoPage.(0x%02X)"), _cdb.InfoPage);
	
	}
	return _responseStr;
}

const USHORT HidPitcInquiry::GetPitcId() const
{ 
	assert(_cdb.InfoPage == InfoPage_Pitc);

    return _pitcInfo.Id;
}
	
const StVersionInfo& HidPitcInquiry::GetPitcVersion() const
{ 
	assert(_cdb.InfoPage == InfoPage_Pitc);

	return _pitcInfo.Version;
};

const CString HidPitcInquiry::GetPitcSenseString() const
{
    assert(_cdb.InfoPage == InfoPage_PitcSense);

    return _pitcSenseInfo;
}

const HidPitcInquiry::OtpRegInfoPage HidPitcInquiry::GetOtpRegInfoPage() const
{ 
	assert(_cdb.InfoPage == InfoPage_OtpReg);

    return _otpRegInfo;
};

const UINT HidPitcInquiry::GetOtpRegAddress() const
{ 
	assert(_cdb.InfoPage == InfoPage_OtpReg);

    return _otpRegInfo.Address;
};

const UCHAR HidPitcInquiry::GetOtpRegLockBit() const
{ 
	assert(_cdb.InfoPage == InfoPage_OtpReg);

    return _otpRegInfo.LockBit;
};

const UCHAR HidPitcInquiry::GetOtpRegBank() const
{ 
	assert(_cdb.InfoPage == InfoPage_OtpReg);

    return _otpRegInfo.OtpBank;
};

const UCHAR HidPitcInquiry::GetOtpRegWord() const
{ 
	assert(_cdb.InfoPage == InfoPage_OtpReg);

    return _otpRegInfo.OtpWord;
};

const UCHAR HidPitcInquiry::IsOtpRegLocked() const
{ 
	assert(_cdb.InfoPage == InfoPage_OtpReg);

    return _otpRegInfo.Locked;
};

const UCHAR HidPitcInquiry::IsOtpRegShadowed() const
{ 
	assert(_cdb.InfoPage == InfoPage_OtpReg);

    return _otpRegInfo.Shadowed;
};

const UINT HidPitcInquiry::GetPersistentRegAddress() const
{ 
	assert(_cdb.InfoPage == InfoPage_PersistentReg);

    return _persistentInfo.Address;
};

const UINT HidPitcInquiry::GetPersistentRegValue() const
{ 
	assert(_cdb.InfoPage == InfoPage_PersistentReg);

    return _persistentInfo.Value;
};


//////////////////////////////////////////////////////////////////////
//
// ST_HID_PITC_COMMAND: HidPitcRead
//
//////////////////////////////////////////////////////////////////////

HidPitcRead::HidPitcRead(const UINT address, const UINT length, const UINT flags, const UINT dataSize)
: StApiT<_ST_HID_CDB::_CDBHIDREADWRITE>(API_TYPE_PITC, ST_READ_CMD, _T("Read"))
{
    _params[L"Address:"] = &_address;
    _params[L"Length:"] = &_length;
    _params[L"Flags:"] = &_flags;
    _params[L"DataSize:"] = &_dataSize;

    _address.Value = address;
    _length.Value = length;
    _flags.Value = flags;
	_dataSize.Value = dataSize;

	PrepareCommand();
}

StApi* HidPitcRead::Create(CString paramStr)
{
	return new HidPitcRead(paramStr);
}

HidPitcRead::HidPitcRead(CString paramStr)
: StApiT<_ST_HID_CDB::_CDBHIDREADWRITE>(API_TYPE_PITC, ST_READ_CMD, _T("Read"))
, _address(0)
, _length(0)
, _flags(0)
, _dataSize(0)
{
	// ;param: Address:0xAABBCCDD,Length:0xAABBCCDD,Flags:0xAABBCCDD,DataSize:0xAABBCCDD
    _params[L"Address:"] = &_address;
    _params[L"Length:"] = &_length;
    _params[L"Flags:"] = &_flags;
    _params[L"DataSize:"] = &_dataSize;

	UINT ret = ParseParameterString(paramStr, _params);

    PrepareCommand();
}

void HidPitcRead::ParseCdb()
{
    _address.Value = Swap4((UCHAR*)&_cdb.Address);
    _length.Value = Swap4((UCHAR*)&_cdb.Length);
	_flags.Value = Swap4((UCHAR*)&_cdb.Flags);

	PrepareCommand();
}

void HidPitcRead::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = PITC_READ;
    _cdb.Address = Swap4((UCHAR*)&_address.Value);
    _cdb.Length = Swap4((UCHAR*)&_length.Value);
	_cdb.Flags = Swap4((UCHAR*)&_flags.Value);

	_tag = 0;
	_xferLength = _length.Value * _dataSize.Value;

	if ( _responseDataPtr != NULL )
	{
		free(_responseDataPtr);
		_responseDataPtr = NULL;
	}

	_responseDataPtr = (UCHAR*)malloc(_xferLength);
}

void HidPitcRead::ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
{
    UINT bytesToCopy = min(count, _xferLength - start);
	
	if ( _responseDataPtr != NULL )
		memcpy(_responseDataPtr + start, pData, bytesToCopy);
}

const CString& HidPitcRead::ResponseString()
{
	FormatReadResponse(_responseDataPtr, 16, 4);

    return _responseStr;
}

//////////////////////////////////////////////////////////////////////
//
// ST_HID_PITC_COMMAND: HidPitcWrite
//
//////////////////////////////////////////////////////////////////////

HidPitcWrite::HidPitcWrite(const UINT address, const UINT length, const UINT flags, const UCHAR * const pData, const UINT dataSize)
	: StApiT<_ST_HID_CDB::_CDBHIDREADWRITE>(API_TYPE_PITC, ST_WRITE_CMD_PLUS_DATA, _T("Write"))
{
    _params[L"Address:"] = &_address;
    _params[L"Length:"] = &_length;
    _params[L"Flags:"] = &_flags;

    _address.Value = address;
    _length.Value = length;
    _flags.Value = flags;

    SetCommandData(pData, dataSize);
}

StApi* HidPitcWrite::Create(CString paramStr)
{
	return new HidPitcWrite(paramStr);
}

HidPitcWrite::HidPitcWrite(LPCTSTR fileName)
	: StApiT<_ST_HID_CDB::_CDBHIDREADWRITE>(API_TYPE_PITC, ST_WRITE_CMD_PLUS_DATA, _T("Write"))
{
    _params[L"Address:"] = &_address;
    _params[L"Length:"] = &_length;
    _params[L"Flags:"] = &_flags;

	StFwComponent fileData(fileName);
	SetCommandData(fileData.GetDataPtr(), fileData.size());
}

void HidPitcWrite::ParseCdb()
{
    _address.Value = Swap4((UCHAR*)&_cdb.Address);
    _length.Value = Swap4((UCHAR*)&_cdb.Length);
	_flags.Value = Swap4((UCHAR*)&_cdb.Flags);

	PrepareCommand();
}

// _xferLength is set in SetCommandData which calls this function.
void HidPitcWrite::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.Command = PITC_WRITE;
    _cdb.Address = Swap4((UCHAR*)&_address.Value);
    _cdb.Length = Swap4((UCHAR*)&_length.Value);
	_cdb.Flags = Swap4((UCHAR*)&_flags.Value);

	_tag = 0;
}
