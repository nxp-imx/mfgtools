#pragma once
#include "PlayerProfile.h"
#include "FileList.h"
#include "Libs/DevSupport/StMedia.h"

#define UL_PARAM_USE_STATIC_FW		        0x00000001
#define UL_PARAM_USE_MULTIPLE_STATIC_ID_FW  0x00000002
#define UL_PARAM_FAT32				        0x00000004
#define UL_PARAM_WINCE						0x00000008
//#define UL_PARAM_ERASE_MEDIA				0x00000008
#define UL_PARAM_NO_MORE_OPTIONS	        0x0000000F
#define UL_PARAM_UPDATE_MASK		        0x0000000F
#define UL_PARAM_COPY_MASK		            0x00000000

#define UPDATE_DRIVE_BOOTMANAGER    0x00000001
#define UPDATE_DRIVE_PLAYER         0x00000002
#define UPDATE_DRIVE_PLAYER_RES     0x00000004
#define UPDATE_DRIVE_DATA           0x00000008
#define UPDATE_DRIVE_USBMSC_HOST    0x00000010
#define UPDATE_ALL_SDK25XX_DRIVES   0x0000001F

#define UPDATE_DRIVE_HOST_RES       0x00000020
#define UPDATE_DRIVE_DATA_HIDDEN    0x00000040
#define UPDATE_DRIVE_UPDATER        0x00000080
#define UPDATE_ALL_SDK26XX_DRIVES   0x000000BF
#define UPDATE_ALL_SDK2612_DRIVES   0x00000800
#define UPDATE_ALL_SDK3120_DRIVES   0x00001000
#define UPDATE_ALL_SDK3XXX_DRIVES   0x000000FF
#define UPDATE_ALL_SDK4XXX_DRIVES   0x000000FF

#define UPDATE_DRIVE_EXTRA          0x00000100
#define UPDATE_DRIVE_RES_1          0x00000200
#define UPDATE_DRIVE_OTG            0x00000400
#define UPDATE_CUSTOM_DRIVES_MASK   0xFFFFFF00

class COpInfo : public CObject
{
friend class CConfigPlayerOpInfoListCtrl;
friend class CXListCtrl;
friend class CConfigPlayerProfilePage;
friend class CCopyOpDlg;
friend class CLoadFileOpDlg;
friend class COpUpdateDlg;
friend class COpOTPDlg;
friend class COpUtpUpdateDlg;
private:
	static const PTCHAR OptionsCmdStrings[];
	static const PTCHAR UpdateFwString;
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
private:
	CPlayerProfile * m_p_profile;
	BOOL m_b_enabled;
	DWORD m_status;
	DWORD m_timeout; // operation watchdog timeout in seconds
	CString m_error_msg;
	INT_PTR m_index;
	CString m_cs_cmd_line;
	COperation::OpTypes m_e_type;
	CString m_cs_desc;
	CString	m_cs_ini_section;
//	CString m_cs_profile_path;
	CString m_cs_path;
//	CString m_cs_ini_file;
	ULONG m_ul_param;
    ULONG m_drv_mask;
    UCHAR m_data_drive_num;     // drive number for COPY ops.
	CString m_versionStr;
	BOOL m_b_new_op;
	CString m_cs_NewName;
	CString m_cs_new_ini_section;
//	CString m_cs_original_name;
	int m_update_boot_fname_list_index;
	int m_ucl_fname_list_index;
	CString m_UclInstallSection;

    // DriveArray s_usbmsc_drive_array;
    // DriveArray s_mtp_drive_array;
public:
	COpInfo(CPlayerProfile * _profile, LPCTSTR _cmd_line, INT_PTR _index);
	COpInfo(CPlayerProfile * _profile);
	virtual ~COpInfo();
//	COpInfo &operator=( COpInfo & );
	DWORD ReadProfileOpAndValidate(void);
	DWORD Validate(void);
	DWORD ValidateDrvInfo(media::LogicalDrive* _drv);
	DWORD OnEnable(BOOL _enable);
	LPCTSTR GetLastErrorMsg(void) {	return LPCTSTR(m_error_msg); };
//	DWORD SetDesc(LPCTSTR _desc);
//	DWORD SetPath(LPCTSTR _ini_section);
//	DWORD SetDetail(LPCTSTR _detail);
    void SetIndex(INT_PTR _index) { m_index = _index; };
	DWORD GetStatus(void) { return m_status; };
	DWORD GetTimeout() { return m_timeout; };
	COperation::OpTypes GetType(void) const{ return m_e_type; };
	BOOL IsEnabled(void) { return m_b_enabled; };
	BOOL UseFat32(void) { return ((m_ul_param & UL_PARAM_FAT32) == UL_PARAM_FAT32); };
    BOOL UseStaticIdFw(void) { return (m_ul_param & UL_PARAM_USE_STATIC_FW); };
    BOOL UseMultipleStaticIdFw(void) { return (m_ul_param & UL_PARAM_USE_MULTIPLE_STATIC_ID_FW); };
	BOOL IsWinCE(void) { return (m_ul_param & UL_PARAM_WINCE); };
//	BOOL EraseMedia(void) { return (m_ul_param & UL_PARAM_ERASE_MEDIA); };
	CString GetPath(void) { return m_cs_path; };
	CString GetUsbVid(void);
	CString GetUsbPid(void);
    INT_PTR GetIndex(void) { return m_index; };
    CString GetIniSection(void) { return m_cs_ini_section; };
	media::LogicalDriveArray& GetDriveArray(void) { return m_drive_array; };
	CPlayerProfile* GetProfile(void) { return m_p_profile; };
    ULONG GetOptions(void) { return m_ul_param; };
	CString GetDesc(void) { return m_cs_desc; };
    UCHAR GetDataDriveNum(void) { return m_data_drive_num; }
	BOOL IsNewOp(void) { return m_b_new_op; };
	CString GetUpdaterBootFilename(void);
	CString GetUpdaterBootPathname(void);
	void SetUpdaterBootFilename(CString _fName, CFileList::FileListAction _action);
	void RemoveUpdaterBootFilename(void);
	CString GetUclFilename(void);
	CString GetUclPathname(void);
	void SetUclFilename(CString _fName, CFileList::FileListAction _action);
	void RemoveUclFilename(void);
	int GetUpdaterBootFilenameIndex(void) { return m_update_boot_fname_list_index; };
	int GetUclFilenameIndex(void) { return m_ucl_fname_list_index; };
	CString GetOTPRegisterValue(void) { return m_csOTPValue; };
	CString GetUclInstallSection() { return m_UclInstallSection; };

protected:
    INT_PTR ReplaceIniLine(LPCTSTR _section, LPCTSTR _string, INT_PTR _line);
    DWORD WriteIniSection(LPCTSTR _section, LPCTSTR _old_section = NULL);
	DWORD ValidateFileList(CFileList * _pFileList);
	DWORD EnumerateFolderFiles(CFileList * p_FileList);

public:
    DWORD Remove(void);
	CString GetProductVersion() { return m_versionStr; };
	void SetProductVersion( CString _csVers) { m_versionStr = _csVers; };

	CFileList	m_FileList;
	media::LogicalDriveArray m_drive_array;
	CString		m_csOTPValue; // OTP register op only

};

