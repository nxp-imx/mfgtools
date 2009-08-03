// StMtpApi.cpp:
//
//////////////////////////////////////////////////////////////////////
#include "StMtpApi.h"

//////////////////////////////////////////////////////////////////////
//
// ST_MTP_COMMAND: MtpResetToRecovery
//
//////////////////////////////////////////////////////////////////////
StApi* MtpResetToRecovery::Create(CStdString paramStr)
{
	return new MtpResetToRecovery();
}

MtpResetToRecovery::MtpResetToRecovery()
	: StApiT<_MTP_COMMAND_DATA_IN>(API_TYPE_ST_MTP, ST_WRITE_CMD, _T("ResetToRecovery"))
{
	PrepareCommand();
}

void MtpResetToRecovery::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.OpCode = MTP_OPCODE_SIGMATEL_FORCERECV;
	_cdb.NextPhase = MTP_NEXTPHASE_NO_DATA;

	_tag = 0;
	_xferLength = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_MTP_COMMAND: MtpEraseBootmanager
//
//////////////////////////////////////////////////////////////////////
StApi* MtpEraseBootmanager::Create(CStdString paramStr)
{
	return new MtpEraseBootmanager();
}

MtpEraseBootmanager::MtpEraseBootmanager()
	: StApiT<_MTP_COMMAND_DATA_IN>(API_TYPE_ST_MTP, ST_WRITE_CMD, _T("EraseBootmanager"))
{
	PrepareCommand();
}

void MtpEraseBootmanager::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.OpCode = MTP_OPCODE_SIGMATEL_ERASEBOOT;
	_cdb.NextPhase = MTP_NEXTPHASE_NO_DATA;

	_tag = 0;
	_xferLength = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_MTP_COMMAND: MtpDeviceReset
//
//////////////////////////////////////////////////////////////////////
StApi* MtpDeviceReset::Create(CStdString paramStr)
{
	return new MtpDeviceReset();
}

MtpDeviceReset::MtpDeviceReset()
	: StApiT<_MTP_COMMAND_DATA_IN>(API_TYPE_ST_MTP, ST_WRITE_CMD, _T("DeviceReset"))
{
	PrepareCommand();
}

void MtpDeviceReset::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.OpCode = MTP_OPCODE_SIGMATEL_RESET;
	_cdb.NextPhase = MTP_NEXTPHASE_NO_DATA;

	_tag = 0;
	_xferLength = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_MTP_COMMAND: MtpResetToUpdater
//
//////////////////////////////////////////////////////////////////////
StApi* MtpResetToUpdater::Create(CStdString paramStr)
{
	return new MtpResetToUpdater();
}

MtpResetToUpdater::MtpResetToUpdater()
	: StApiT<_MTP_COMMAND_DATA_IN>(API_TYPE_ST_MTP, ST_WRITE_CMD, _T("ResetToUpdater"))
{
	PrepareCommand();
}

void MtpResetToUpdater::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.OpCode = MTP_OPCODE_SIGMATEL_RESET_TO_UPDATER;
	_cdb.NextPhase = MTP_NEXTPHASE_NO_DATA;

	_tag = 0;
	_xferLength = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_MTP_COMMAND: MtpGetDriveVersion
//
//////////////////////////////////////////////////////////////////////
StApi* MtpGetDriveVersion::Create(CStdString paramStr)
{
	// TODO: parse string command into drive number
	return new MtpGetDriveVersion();
}

MtpGetDriveVersion::MtpGetDriveVersion(const media::LogicalDriveTag driveTag, const _LOGICAL_DRIVE_INFO versionType)
	: StApiT<_MTP_COMMAND_DATA_IN>(API_TYPE_ST_MTP, ST_READ_CMD, _T("GetDriveVersion"))
{
	_params[L"Drive Tag:"] = &_driveTag;
	_params[L"Version Type:"] = &_versionType;

	_driveTag.Value = driveTag;
	_versionType.Value = versionType;
	
	InitParamTypes();
	
	PrepareCommand();
}

void MtpGetDriveVersion::InitParamTypes()
{
	_driveTag.ValueList[media::DriveTag_Player]        = L"DriveTag_Player";
	_driveTag.ValueList[media::DriveTag_UsbMsc]        = L"DriveTag_UsbMsc";
	_driveTag.ValueList[media::DriveTag_Hostlink]      = L"DriveTag_Hostlink";
	_driveTag.ValueList[media::DriveTag_PlayerRsc]     = L"DriveTag_PlayerRsc";
	_driveTag.ValueList[media::DriveTag_PlayerRsc2]    = L"DriveTag_PlayerRsc2";
	_driveTag.ValueList[media::DriveTag_PlayerRsc3]    = L"DriveTag_PlayerRsc3";
	_driveTag.ValueList[media::DriveTag_Extra]         = L"DriveTag_Extra";
	_driveTag.ValueList[media::DriveTag_ExtraRsc]      = L"DriveTag_ExtraRsc";
	_driveTag.ValueList[media::DriveTag_Otg]           = L"DriveTag_Otg";
	_driveTag.ValueList[media::DriveTag_HostlinkRsc]   = L"DriveTag_HostlinkRsc";
	_driveTag.ValueList[media::DriveTag_HostlinkRsc2]  = L"DriveTag_HostlinkRsc2";
	_driveTag.ValueList[media::DriveTag_HostlinkRsc3]  = L"DriveTag_HostlinkRsc3";
	_driveTag.ValueList[media::DriveTag_Mark]          = L"DriveTag_Mark";
	_driveTag.ValueList[media::DriveTag_IrDA]          = L"DriveTag_IrDA";
	_driveTag.ValueList[media::DriveTag_SettingsBin]   = L"DriveTag_SettingsBin";
	_driveTag.ValueList[media::DriveTag_OtgRsc]        = L"DriveTag_OtgRsc";
	_driveTag.ValueList[media::DriveTag_Data]          = L"DriveTag_Data";
	_driveTag.ValueList[media::DriveTag_Data2]         = L"DriveTag_Data2";
	_driveTag.ValueList[media::DriveTag_DataJanus]     = L"DriveTag_DataJanus";
	_driveTag.ValueList[media::DriveTag_DataSettings]  = L"DriveTag_DataSettings";
	_driveTag.ValueList[media::DriveTag_Bootmanger]    = L"DriveTag_Bootmanger";
	_driveTag.ValueList[media::DriveTag_UpdaterNand]   = L"DriveTag_UpdaterNand";
	_driveTag.ValueList[media::DriveTag_Updater]       = L"DriveTag_Updater";

	_versionType.ValueList[DriveInfoComponentVersion] = L"DriveInfoComponentVersion";
	_versionType.ValueList[DriveInfoProjectVersion]   = L"DriveInfoProjectVersion";
}

void MtpGetDriveVersion::ParseCdb()
{
	_driveTag.Value = (media::LogicalDriveTag)_cdb.Params[0];
	_versionType.Value = (_LOGICAL_DRIVE_INFO)_cdb.Params[1];

	PrepareCommand();
}

void MtpGetDriveVersion::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.OpCode = MTP_OPCODE_SIGMATEL_GET_DRIVE_VERSION;
	_cdb.NextPhase = MTP_NEXTPHASE_NO_DATA;
	_cdb.NumParams = 2;
	_cdb.Params[0] = _driveTag.Value;
	_cdb.Params[1] = _versionType.Value;

	_tag = 0;
	_xferLength = 0;
}

void MtpGetDriveVersion::ProcessResponse(const uint8_t *const pData, const uint32_t start, const uint32_t count)
{
	assert( count >= _xferLength );

	_version.clear();

	_MTP_COMMAND_DATA_OUT* pBuf = (_MTP_COMMAND_DATA_OUT*)pData;
	_version.SetMajor((uint16_t)pBuf->Params[0]);
	_version.SetMinor((uint16_t)pBuf->Params[1]);
	_version.SetRevision((uint16_t)pBuf->Params[2]);
	
	_responseStr.Format(_T("%s -\r\n %s: %s\r\n"),
		_driveTag.ToString().c_str(),
		_versionType.ToString().c_str(), 
		_version.toString().c_str());
}

//////////////////////////////////////////////////////////////////////
//
// ST_MTP_COMMAND: MtpSetUpdateFlag
//
//////////////////////////////////////////////////////////////////////
StApi* MtpSetUpdateFlag::Create(CStdString paramStr)
{
	return new MtpSetUpdateFlag();
}

MtpSetUpdateFlag::MtpSetUpdateFlag()
	: StApiT<_MTP_COMMAND_DATA_IN>(API_TYPE_ST_MTP, ST_WRITE_CMD, _T("SetUpdateFlag"))
{
	PrepareCommand();
}

void MtpSetUpdateFlag::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.OpCode = MTP_OPCODE_SIGMATEL_SET_UPDATE_FLAG;
	_cdb.NextPhase = MTP_NEXTPHASE_NO_DATA;

	_tag = 0;
	_xferLength = 0;
}

//////////////////////////////////////////////////////////////////////
//
// ST_MTP_COMMAND: MtpSwitchToMsc
//
//////////////////////////////////////////////////////////////////////
StApi* MtpSwitchToMsc::Create(CStdString paramStr)
{
	return new MtpSwitchToMsc();
}

MtpSwitchToMsc::MtpSwitchToMsc()
	: StApiT<_MTP_COMMAND_DATA_IN>(API_TYPE_ST_MTP, ST_WRITE_CMD, _T("SwitchToMsc"))
{
	PrepareCommand();
}

void MtpSwitchToMsc::PrepareCommand()
{
	memset(&_cdb, 0, sizeof(_cdb));
	_cdb.OpCode = MTP_OPCODE_SIGMATEL_SWITCH_TO_MSC;
	_cdb.NextPhase = MTP_NEXTPHASE_NO_DATA;

	_tag = 0;
	_xferLength = 0;
}

