// StUsbMscDev.h: interface for the CStUsbMscDev class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STUSBMSCDEV_H__9A26AB99_5F86_4149_A02E_AAF9B9557463__INCLUDED_)
#define AFX_STUSBMSCDEV_H__9A26AB99_5F86_4149_A02E_AAF9B9557463__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdevice.h"

#define NORMAL_MSC      0
#define UPGRADE_MSC     1

#define SAVE_HDS        0
#define RESTORE_HDS     1

#define NUM_HDS_FILES   6

class CStUsbMscDev : public CStDevice  
{
public:
	CStUsbMscDev(CStUpdater* pCStUpdater, string name="CStUsbMscDev");
	virtual ~CStUsbMscDev();

	ST_ERROR Initialize(USHORT UpgradeOrNormal);
	ST_ERROR CheckForFwResources();
	ST_ERROR VerifyDevicePresence();
	ST_ERROR IsEraseMediaRequired(BOOL&);
	ST_ERROR GetReadyToDownload();

    ST_ERROR DownloadFirmware();
    ST_ERROR FormatDataDrive();
    ST_ERROR FormatHiddenDrive();
    ST_ERROR SetAllocationTable();
	ST_ERROR EraseLogicalMedia(BOOL bPreserveHiddenDrive);

	void CloseScsi();
	ST_ERROR CleanupAfterDownload();
	ST_ERROR DoJustFormat();
	void DestroyScsi();
	USHORT GetChipId() { return m_chip_id; }
	USHORT GetPartId() { return m_part_id; }
    USHORT GetROMId() { return m_ROM_id; }
	ST_ERROR GetUpdaterDriveVersion(CStVersionInfo& updaterVersion);
	wchar_t	GetDriveLetter();

	ST_ERROR GetNumberOfDrives(USHORT& num_drives);
	ST_ERROR GetNumberOfDataDrivesPresent(USHORT UpgradeOrNormal);
    ST_ERROR GetPhysicalMediaType(PHYSICAL_MEDIA_TYPE& type);
    ST_ERROR IsMediaWriteProtected(ST_BOOLEAN&);
    ST_ERROR GetMediaSizeInBytes(ULONGLONG&);
	ST_ERROR GetMediaNandPageSizeInBytes(ULONG& _pagesize);
	ST_ERROR GetMediaNandMfgId(ULONG& _nandMfgId);
	ST_ERROR GetMediaNandIdDetails(ULONGLONG& _nandIdDetails);
    ST_ERROR GetSizeOfSerialNumber(USHORT& size);
    ST_ERROR GetSerialNumber(CStByteArray *);
	ST_ERROR GetMediaNandChipEnables( ULONG& _nandChipEnables);
	ST_ERROR GetCurrentComponentVersions(CStVersionInfoPtrArray **p_arr_ver_info_current);
	ST_ERROR GetCurrentProjectVersion(CStVersionInfo* p_ver_info_current);
	ST_ERROR GetUpgradeComponentVersions(CStVersionInfoPtrArray **p_arr_ver_info_current);
	ST_ERROR GetUpgradeProjectVersion(CStVersionInfo* p_ver_info_current);
	ST_ERROR GetChipMajorRevId(USHORT& rev);
	ST_ERROR GetChipPartRevId(USHORT& rev);
	ST_ERROR GetROMRevId(USHORT& rev);
	ST_ERROR GetExternalRAMSizeInMB(ULONG& _eRAMSize);
	ST_ERROR GetVirtualRAMSizeInMB(ULONG& _vRAMSize);
	ST_ERROR GetAllocationTable(P_MEDIA_ALLOCATION_TABLE _p_table);
	ST_ERROR SetAllocationTable(MEDIA_ALLOCATION_TABLE _table);
	ST_ERROR ReadDrive(DWORD driveTag, CStByteArray* _p_arr);
	ST_ERROR WriteDrive(DWORD driveTag, CStByteArray* _p_arr);
	BOOL     HasSettingsFile();
	ULONGLONG GetDriveSize(DWORD driveTag);
	BOOL     IsLessOrEqualToCurrentMediaTable(P_MEDIA_ALLOCATION_TABLE _p_table);
    ULONG    GetTotalTasks(USHORT _operation);
    ST_ERROR ResetMSCDeviceForRecovery();
	ST_ERROR OldResetMSCDeviceForRecovery();
    ST_ERROR ResetMscDeviceToMscUpdaterMode();
	ST_ERROR ResetChip();
    BOOL     AreVendorCmdsLimited();
    ST_ERROR Transfer2DDContent();
    ST_ERROR SaveRestoreHDS(UCHAR);
    unsigned DeleteFiles (wstring srcFolder, wstring szQualifier);
    unsigned CopyContents (wstring srcFolder, wstring destFolder, BOOL bCountOnly, BOOL bCopySubFolders, wstring szQualifier, DWORD dwAttrSpec);
	ST_ERROR GetDataDriveInfo(int _drivenum, ULONG& _sectors, USHORT& _secsize);

	UCHAR	 GetProtocolVersion(void);
	UCHAR	 GetProtocolVersionMajor(void);
	UCHAR	 GetProtocolVersionMinor(void);
    ST_ERROR GetJanusStatus(UCHAR&);
	ST_ERROR JanusInitialization();
	static DWORD WINAPI JanusInitThread( LPVOID pParam );
	ST_ERROR DataStoreInitialization();
	static DWORD WINAPI DataStoreInitThread( LPVOID pParam );

private:

	CStScsi					*m_p_scsi;
//	CStDataDrive			*m_p_data_drive;
	CStDataDrivePtrArray    *m_p_arr_data_drive;
    CStSystemDrivePtrArray	*m_p_arr_system_drive;
	CStHiddenDataDrivePtrArray	*m_p_arr_hidden_data_drive;

	CStVersionInfoPtrArray	*m_p_arr_ver_info_current;
	CStVersionInfoPtrArray	*m_p_arr_ver_info_upgrade;
	
	CStVersionInfo			m_ver_info_current_project;
	CStVersionInfo			m_ver_info_upgrade_project;

	BOOL					m_format_pending;
	BOOL					m_media_new;
    BOOL                    m_limited_vendor_cmd_support;

	MEDIA_ALLOCATION_TABLE	m_table;

    UCHAR                   m_num_drives;
	UCHAR					m_num_system_drives;
	UCHAR					m_num_data_drives;
	UCHAR					m_num_hidden_data_drives;
	UCHAR					m_num_data_drives_present;
	USHORT					m_chip_id;
    USHORT                  m_part_id;
	USHORT					m_ROM_id;
	wchar_t					m_drive_letter;
    USHORT                  m_device_mode;

	void BuildUpgradeTableFromDrives(MEDIA_ALLOCATION_TABLE& _table);

	ST_ERROR ReadTableAndSetupDrivesToUpgrade();
	ST_ERROR LockLogicalVolume();
	ST_ERROR UnLockLogicalVolume();
	ST_ERROR CreateScsi();
	ST_ERROR InitializeDataDrives(ST_ERROR _err_init_scsi);
	ST_ERROR InitializeSystemDrives(ST_ERROR _err_init_scsi);
	ST_ERROR InitializeVersions(ST_ERROR _err_init_scsi, BOOL _media_new=FALSE);
	ST_ERROR InitializeAllocationTable();
	ST_ERROR Trash();
};

#endif // !defined(AFX_STUSBMSCDEV_H__9A26AB99_5F86_4149_A02E_AAF9B9557463__INCLUDED_)
