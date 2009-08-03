// StUpdater.h: interface for the CStUpdater class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STUPDATER_H__CD55EA77_430F_424F_B38F_3709DB36A9E3__INCLUDED_)
#define AFX_STUPDATER_H__CD55EA77_430F_424F_B38F_3709DB36A9E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stglobals.h"
#include "StByteArray.h"
#include "ddildl_defs.h"

typedef enum
{
    FIND_ANY_DEVICE  =   0,
    FIND_MTP_DEVICE  =   1,
    FIND_MSC_DEVICE  =   2,
    FIND_UPD_DEVICE  =   3,
    FIND_REC_DEVICE  =   4
} FIND_DEVICE_TYPE, *PFIND_DEVICE_TYPE;

#define UPDATE_NONE             0x00
#define UPDATE_FIRMWARE			0x01
#define UPDATE_FORMAT_DATA      0x02
#define UPDATE_REMOVE_DRM       0x04
#define UPDATE_ERASE_MEDIA      0x08
#define UPDATE_2DD_CONTENT      0x10
#define UPDATE_INIT_DEVICE      0x20
#define UPDATE_INIT_JANUS		0x40
#define UPDATE_INIT_STORE		0x80
#define UPDATE_SAVE_HDS			0x100

class CStMtpDevice;
class CStUsbMscDev;
class CStRecoveryDev;
class CStHidDevice;
class CStConfigInfo;
class CStProgress;
class CStUpdater : public CStBase
{
public:
	CStUpdater(CStConfigInfo *p_config_info, string name="CStUpdater");
	virtual ~CStUpdater();

	CStConfigInfo* GetConfigInfo();
	void SetConfigInfo(CStConfigInfo*) ;
	ST_ERROR FindDevice(FIND_DEVICE_TYPE deviceType);
    void     CloseDevice();
    BOOL     IsResetToRecoveryRequired();
    BOOL     IsDeviceInRecoveryMode();
	BOOL	 IsDeviceInHidMode();
	BOOL     IsDeviceInUpdaterMode();
	BOOL     IsDeviceInMtpMode();
	BOOL     IsDeviceInMscMode();
	BOOL     IsDeviceInLimitedMscMode();
	BOOL	 IsDeviceReady();
	BOOL	 IsDevicePresent();

    ST_ERROR InitializeDeviceInterface( BOOL bReadyToUpdate );

    ST_ERROR ResetForRecovery();
    BOOL	 OldResetForRecovery();
    ST_ERROR ResetToMscUpdaterMode();
    ST_ERROR FindMTPDevice();
    ST_ERROR FindMSCDevice(FIND_DEVICE_TYPE deviceWeAreLookingFor);
	ST_ERROR FindDeviceToRecover();
    ST_ERROR GetJanusStatus(UCHAR&);
	wchar_t GetDriveLetter();
	ST_ERROR RecoverDevice();
	void	 SetTotalTasks(USHORT _operation);
	ST_ERROR PrepareForUpdate();
	ST_ERROR SaveDrive(int driveTag, CString fileName);
	ST_ERROR RestoreDrive(int driveTag, CString fileName);
    ST_ERROR UpdateDevice(USHORT _operation);
	ST_ERROR CompletedUpdate();
	ST_ERROR CheckForFwResources();
	ST_ERROR IsEraseMediaRequired(BOOL& erase_media_required);
	ST_ERROR DoJustFormat();
	ST_ERROR GetComponentVersions(
		CStVersionInfoPtrArray** p_arr_current_component_vers, 
		CStVersionInfoPtrArray** p_arr_upgrade_component_vers
		);

	ST_ERROR GetProjectVersions(
		CStVersionInfo* p_current_project_ver, 
		CStVersionInfo* p_upgrade_project_ver
		);

	// No need for enumerated MSC device to get the upgrade f/w versions from the files
	CStVersionInfoPtrArray	*m_p_arr_ver_info_upgrade;
	CStVersionInfo			m_ver_info_upgrade_project;
	ST_ERROR GetUpgradeVersions(
		CStVersionInfo* p_upgrade_project_ver,
		CStVersionInfoPtrArray** p_arr_upgrade_component_vers
		);
	ST_ERROR GetProjectUpgradeVersion(
		CStVersionInfo* p_upgrade_project_ver
		);

	ST_ERROR GetChipId(USHORT&);
   	ST_ERROR GetPartId(USHORT&);
    ST_ERROR GetROMId(USHORT&);
	void FreeScsiDevice();

    void WaitForDriveToAppear(DWORD _last_drives_bitmap, BOOL bUpdating );

	void SetProgress( CStProgress* _progress ) { m_p_progress = _progress; }
	CStProgress* GetProgress() { return m_p_progress; }
	CStError* GetErrorObject() { return m_p_error; }
	CStBaseToResource* GetResource() { return m_p_base_resource; }
	void SetResource( CStBaseToResource *_p_base_resource );
	CStBaseToCmdLineProcessor* GetCmdLineProcessor() { return m_p_base_cmdlineprocessor; }
	void SetCmdLineProcessor( CStBaseToCmdLineProcessor* _p_base_cmdlineprocessor );
	CStBaseToLogger* GetLogger() { return m_p_base_logger; }
	void SetLogger( CStBaseToLogger* _p_base_logger );
	ST_ERROR ResetChip();
    void SetLanguageId(WORD _langid);
    WORD GetLanguageId(void);
	void SetStopTrigger(HANDLE hStopTrigger){m_hStopTrigger = hStopTrigger;}
	ST_ERROR GetSerialNumberSize(USHORT& _serialNoSize);
	ST_ERROR GetSerialNumber(CStByteArray * _serialno);
	ST_ERROR GetDataDriveInfo(int _drivenum, LPTSTR _drv, LPTSTR _fs, USHORT _fsnamelen, ULONG& _sectors, USHORT& _secsize);
	ST_ERROR GetMediaType(PHYSICAL_MEDIA_TYPE& _mediatype);
	ST_ERROR GetMediaSizeInBytes(ULONGLONG& _mediasize);
	ST_ERROR GetMediaNandPageSizeInBytes(ULONG& _nandpagesize);
	ST_ERROR GetMediaNandMfgId(ULONG& _nandmfgid);
	ST_ERROR GetMediaNandIdDetails(ULONGLONG& _nandmfgid);
	ST_ERROR GetMediaNandChipEnables(ULONG& _nandchipenables);
	ST_ERROR GetExternalRAMSizeInMB(ULONG& _eRAMsize);
	ST_ERROR GetVirtualRAMSizeInMB(ULONG& _vRAMsize);
	ST_ERROR GetProtocolVersionMajor(UCHAR& _versionMajor);
	ST_ERROR GetProtocolVersionMinor(UCHAR& _versionMinor);

    USHORT      m_DeviceMode;    // Set to indicate what mode the device is in, if present
    WORD        m_langid;
                              
private:

	CStBaseToCmdLineProcessor*	m_p_base_cmdlineprocessor;
	CStBaseToResource*			m_p_base_resource;
	CStBaseToLogger*            m_p_base_logger;
	CStConfigInfo*				m_p_config_info;
	CStUsbMscDev*				m_p_usbmsc_dev;
    CStMtpDevice*               m_p_mtpdev;
    CStRecoveryDev*				m_p_recovery_dev;
	CStHidDevice*				m_p_hid_dev;
	CStProgress*				m_p_progress;
	CStError*					m_p_error;
	HANDLE						m_hStopTrigger;

};

#endif // !defined(AFX_STUPDATER_H__CD55EA77_430F_424F_B38F_3709DB36A9E3__INCLUDED_)
