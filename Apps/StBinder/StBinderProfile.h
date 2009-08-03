#pragma once
#include "mm_callback.h"
#include "StResGrpTreeCtrl.h"
#include "resourcelist.h"
#include "StBinderDlg.h"

class StBinderProfile
{
public:
	StBinderProfile(CString _ProfileName);
	~StBinderProfile(void);

	BOOL ReadProfileRegData(CStBinderDlg *_binderDlg);
	BOOL WriteProfileRegData(CStBinderDlg *_binderDlg);

	CString m_csProfileName;
	CString m_error_msg;
	HKEY	m_hProfilesKey;
	CStringArray m_csProfileArray;

protected:
typedef enum _tag_e_REG_VALUE{ COMPANY_DESC = 0,
								PRODUCT_DESC,
								PRODUCT_NAME,
								APP_TITLE,
								COPYRIGHT,
								COMMENT,
								MTP_MFG,
								MTP_PROD,
								USB_VID,
								USB_PID,
								USB_PID2,
								SCSI_MFG,
								SCSI_PRODUCT,
								AUTO_RECV,
								FORCE_RECV,
								REBOOT_2PLAYER,
								LOW_NAND,
								USE_LOCAL,
								DLG_TYPE,
								FORMAT_DATA,
								ERASE_MEDIA,
								DATA_ERASE_WARNING,
								DEFAULT_FS,
								AUTO_START,
								AUTO_CLOSE,
								COMPANY_IMAGE,
								UPDATER_ICON,
								VERSION,
								DRIVE1,
								DRIVE2,
								DRIVE3,
								DRIVE4,
								DRIVE5,
								DRIVE6,
								DRIVE7,
								DRIVE8,
								DRIVE9,
								DRIVE10,
								DRIVE11,
								DRIVE12
								} e_REG_VALUE;



};
