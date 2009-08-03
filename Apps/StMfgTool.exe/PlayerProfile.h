#pragma once

#include <afxtempl.h>
#include "Operation.h"
#include "OpInfo.h"

#define MIN_PROFILE_VERSION		1

#define OPINFO_OK 0
#define OPINFO_ERROR 1
#define PROFILE_OK 0
#define PROFILE_WARNING 1
#define PROFILE_ERROR 2


#define PROFILE_MODE_READ_ONLY	0
#define PROFILE_MODE_EDIT		1
#define PROFILE_MODE_NEW		2

// CPlayerProfile command target
class CPlayerProfile : public CObject
{
	friend class CConfigPlayerProfilePage;
	friend class CConfigPlayerOpInfoListCtrl;
	friend class COpInfo;
public:
	CPlayerProfile();
	virtual ~CPlayerProfile();
//	CPlayerProfile &operator=( CPlayerProfile & );
//	typedef enum _tag_e_OP_TYPES{ INVALID = 0, UPDATE, COPY, ERASE, SCRUB, NO_MORE_OPS } e_OP_TYPE;
protected:
	typedef enum _tag_e_INI_FIELDS{ PLAYER = 0, USB_VID, USB_PID, SCSI_MFG, SCSI_PRODUCT, VOLUME_LABEL, USE_VOLUME_LABEL, VERSION } e_INI_FIELD;
	typedef enum _tag_e_DIR_OPS{ NONE = 0, CREATE_BACKUP, RESTORE_BACKUP, DELETE_BACKUP, CREATE_NEW_DIR, DELETE_NEW_DIR } e_DIR_OP;

public:
	typedef CTypedPtrList<CObList, CPlayerProfile::COpInfo*>  COpInfoList;
//	typedef CPlayerProfile::COpInfo * pCOpInfo;

protected:
	CString m_cs_name;				// ex. SigmaTel MSCN Audio Player		
	CString m_cs_usb_vid;			// ex. _T("066F")
	CString m_cs_usb_pid;			// ex. _T("8000")
	CString m_cs_scsi_mfg;			// ex. _T("SigmaTel")
	CString m_cs_scsi_product;		// ex. _T("MSCN")
	COpInfoList m_p_op_info_list;	// ex. { {ERASE, _T("Reset")},
									//       {UPDATE, _T("Test Firmware Files")}, 
									//       {COPY, _T("Test Media Files")},
									//		 {UPDATE, _T("Retail Firmware Files")},
									//       {COPY, _T("Retail Media Files")},
									//       {REG_SCRUB, _T("")} }
	COpInfoList m_p_op_delete_list;	// list of deleted ops

	DWORD m_status;
	CString m_error_msg;
	CString m_cs_profile_root;
	CString m_cs_profile_path;
	CString m_cs_ini_file;
	CString m_cs_volume_label;
	BOOL m_b_use_volume_label;
	CString m_cs_original_name;
	DWORD m_edit_mode;
	int m_iSelectedUpdate;
	BOOL m_bNew;


#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
//	void CleanProfile(void);
	DWORD SetUseVolumeLabel(BOOL _use_label);
	COpInfo* AddOperation(void);
    INT_PTR RemoveOperation(INT_PTR _index);
    DWORD SetIniField(DWORD _field, LPVOID _value);

public:
	DWORD Init( LPCTSTR _name = NULL);
	LPCTSTR GetName(void) { return m_cs_name; }; 
	LPCTSTR GetOriginalName(void) { return m_cs_original_name; };
	BOOL IsValid(void) { return m_status < PROFILE_ERROR; };
	BOOL IsNew(void) { return m_bNew; };
	void GetStatus (DWORD& _dwStatus, CString& _csErrMsg){ _dwStatus = m_status; _csErrMsg = m_error_msg; };
	void RemoveDeletedOps(void);

public:
	COpInfoList* GetOpInfoListPtr(void) { return (&m_p_op_info_list); }
	CString GetUsbVid(void) { return m_cs_usb_vid; };
	CString GetUsbPid(void) { return m_cs_usb_pid; };
	CString GetScsiMfg(void) { return m_cs_scsi_mfg; };
	CString GetScsiProduct(void) { return m_cs_scsi_product; };
	INT_PTR GetNumEnabledOps(void);
	BOOL UseVolumeLabel(void) {	return m_b_use_volume_label; };
	CString GetVolumeLabel(void) { return m_cs_volume_label; };
protected:
    DWORD RenameProfile(CString _new_name, BOOL _overwrite = FALSE);
	DWORD Validate(void);
public:
    CString GetProfileRoot(void) {return m_cs_profile_root; };
	CString GetProfilePath(void) {return m_cs_profile_path; };
	DWORD GetEditMode(void) { return m_edit_mode; };
	CString GetProductVersion();
	void SetSelectedUpdateOp(int _index) { m_iSelectedUpdate = _index; };
	int GetSelectedUpdateOp() { return m_iSelectedUpdate; };

	void SetName (CString _name) { m_cs_name = _name; };
	void SetUsbVid (CString _usbVid) { m_cs_usb_vid = _usbVid; };
	void SetUsbPid (CString _usbPid) { m_cs_usb_pid = _usbPid; };
	void SetScsiMfg (CString _scsiMfg) { m_cs_scsi_mfg = _scsiMfg; };
	void SetScsiProduct (CString _scsiProd) { m_cs_scsi_product = _scsiProd; };
	void SetVolumeLabel (CString _volumeLabel) { m_cs_volume_label = _volumeLabel; };
	void SetUseVolume (BOOL _useVol) { m_b_use_volume_label = _useVol; };

	BOOL m_bLockedProfile;
};

