// StConfigInfo.h: interface for the StConfigInfo class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_StCONFIGINFO_H)
#define AFX_StCONFIGINFO_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\\..\\common\\updater_restypes.h"

#define MAX_SYSTEM_DRIVES				10
#define MAX_VOLATILE_SYSTEM_DRIVES		10

// F/W header m_flags (USHORT) member has host allocated bits starting at bit 8
#define FW_HEADER_FLAGS_DRM_ENABLED		0x100  


class CStConfigInfo : public CStBase
{
public:
	CStConfigInfo(string name="CStConfigInfo");
	virtual ~CStConfigInfo();

public:
	ST_ERROR GetRequestedDriveSize(UCHAR _drive_index, ULONG& _bytes);
	ST_ERROR GetNonVolatileSystemDriveType(UCHAR _non_volatile_drive_index, UCHAR&);
	ST_ERROR GetNonVolatileSystemDriveTag(UCHAR _non_volatile_drive_index, UCHAR&);
	ST_ERROR GetNonVolatileSystemDriveName(UCHAR _non_volatile_drive_index, string&);
	ST_ERROR GetNonVolatileSystemDriveDescription(UCHAR _drive_index, wstring& strParm);
	ST_ERROR GetNumNonVolatileSystemDrives(UCHAR&);
	ST_ERROR GetSystemDriveName(UCHAR system_drive_index, string& strParm);
	ST_ERROR GetSystemDriveDescription(UCHAR system_drive_index, wstring& strParm);
	ST_ERROR GetDriveType(UCHAR _drive_index, LOGICAL_DRIVE_TYPE& type);
	ST_ERROR GetDriveTag(UCHAR _drive_index, UCHAR& tag);
//	ST_ERROR IsSystemDriveEncrypted(UCHAR system_drive_index, BOOL& encrypted);
	ST_ERROR GetUpdaterDriveIndex(CHAR& _drive_index);
	ST_ERROR GetPlayerDriveIndex(UCHAR& _drive_index);
	ST_ERROR GetNumDrives(UCHAR&);

    ST_ERROR GetBootyDriveIndex(UCHAR& _drive_index);
	CHAR     GetDriveIndex(int _tag);
	ST_ERROR GetNumSystemDrives(UCHAR&);
	ST_ERROR GetNumDataDrives(UCHAR&);
	ST_ERROR GetUSBProductId(USHORT&);
	ST_ERROR GetUSBVendorId(USHORT&);
	ST_ERROR GetSecondaryUSBProductId(USHORT&);
	void SetPreferredFAT(UCHAR);
    UCHAR GetPreferredFAT(void);
	BOOL GetDefaultAutoStartOption();
	BOOL GetDefaultAutoCloseOption();
    void SetRecover2DD(BOOL _recover2DD);
    void SetRecover2DDImage(BOOL _recover2DDImage);
    BOOL Recover2DD() {return m_recover2DD;}
    BOOL Recover2DDImage() {return m_recover2DDImage;}
    void SetRecover2DDImageFileName( wstring& );
    void GetRecover2DDImageFileName(wstring&);
	ST_ERROR ContentFolder(wstring&);
    BOOL UseScsiProductSubstringQualifier();
	ST_ERROR GetMtpModelString(wchar_t *, rsize_t);
	ST_ERROR GetMtpMfgString(wchar_t *, rsize_t);
	ST_ERROR GetSCSIProductString(wstring&);
	ST_ERROR GetSCSIMfgString(wstring&);
	ST_ERROR ApplicationName(wstring&, LANGID _lang_id);
	ST_ERROR ApplicationDescription(wstring&, LANGID _lang_id);
	ST_ERROR ExecutableName(wstring&);
	ST_ERROR GetCopyrightString(wstring&, LANGID _lang_id);
	ST_ERROR GetVendorName(wstring&, LANGID _lang_id);
	ST_ERROR GetClassVersion(double&);
	ST_ERROR GetDefaultVolumeLabel(wstring&);
	BOOL GetDefaultStateForFormatDataArea() { return m_FormatDataArea; };
	BOOL GetDefaultStateForEraseMedia() { return m_EraseMedia; };
	BOOL GetMinDlgFormatWarningMsg() { return m_MinFmtMsg; };
	ST_ERROR GetAboutDlgTitle(wstring& _about_title, LANGID _lang_id);
	void SetResource( CStBaseToResource* _p_resource );
    ST_ERROR GetNumHiddenDataDrives(UCHAR& _num_drives);
	//BOOL HideChipInfo();
    BOOL AllowAutoRecovery();
	//BOOL HasCustomSupport();
	BOOL BuiltFor35xx();
	BOOL BuiltFor36xx();
	BOOL BuiltFor37xx();
	BOOL IsLowNandSolution();
	BOOL IsWinCESolution();
    //BOOL ForceGetCurrentVersions();
	BOOL GetBaseSDKString(wstring&);
	BOOL GetBaseSDKString(CString&);

	USER_DLG_TYPE GetDefaultUserDialog() { return m_DefaultDialog; };
	BOOL ShowBootToPlayerMessage() { return m_ShowBootToPlayerMsg; };
	BOOL GetDefaultLocalResourceUsage() { return m_LocalResourceUsage; };
	//void SetDefaultLocalResourceUsage(BOOL _localResourceUsage);
	double GetSystemDrivePaddingFactor();
	BOOL GetForceRecvMode();
	//void SetDefaultForceRecvMode(BOOL _ForceRecvMode);
	//void SetDefaultEraseMedia(BOOL _erasemedia);
	//void SetDefaultFormatDataArea(BOOL _format);
	void SetFirmwareHeaderFlags(USHORT _flags) { m_FirmwareHeaderFlags = _flags; };
	USHORT GetFirmwareHeaderFlags() { return m_FirmwareHeaderFlags; };
	CString DriveArrayToString();
	CString DriveTypeToString(LOGICAL_DRIVE_TYPE type);
	void ConvertStringToDrive(wchar_t *str, int _index);
	BOOL IsAboutDlgBMP() { return m_IsAboutBMP; };
	USHORT GetBitmapId() { return m_BitmapId; };


private:

    BOOL m_recover2DD;
    BOOL m_recover2DDImage;
    wstring m_recover2dd_image_filename;
	CStBaseToResource* m_p_resource;
	UCHAR m_preferred_fat;
	BOOL m_LocalResourceUsage;
	BOOL m_AllowAutoRecovery;
	BOOL m_ForceRecvMode;
	USHORT m_FirmwareHeaderFlags; // new for 37xx
	BOOL m_EraseMedia;
	BOOL m_FormatDataArea;
	BOOL m_MinFmtMsg;
	BOOL m_ShowBootToPlayerMsg;
	USER_DLG_TYPE m_DefaultDialog;
	BOOL m_WinCE;
	BOOL m_IsLowNand;
	BOOL m_IsAboutBMP;
	USHORT m_BitmapId;

	USHORT	m_DriveDescNumDrives;
	PDRIVE_DESC m_DriveDesc;
};

#endif // !defined(AFX_StCONFIGINFO_H)
