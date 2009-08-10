#pragma once

#include "Operation.h"
#include "OpInfo.h"

#define PATH_ERROR			-1
#define PATH_NOT_FOUND		0
#define PATH_IS_FILE		1
#define PATH_IS_FOLDER		2
#define DIR_CREATE_PROG_VALUE 25

// COpCopy

class COpCopy : public COperation
{
	DECLARE_DYNCREATE(COpCopy)
	friend DWORD CALLBACK CopyProgressRoutine(
		LARGE_INTEGER TotalFileSize,
		LARGE_INTEGER TotalBytesTransferred,
		LARGE_INTEGER StreamSize,
		LARGE_INTEGER StreamBytesTransferred,
		DWORD dwStreamNumber,
		DWORD dwCallbackReason,
		HANDLE hSourceFile,
		HANDLE hDestinationFile,
		LPVOID lpData
		);

protected:
	DECLARE_MESSAGE_MAP()
	virtual ~COpCopy();
	DWORD DoCopy(void);
	static CMutex m_mutex;
	CString m_src_path;
	CString m_dest_path;
	CString m_curr_file;
	double m_tot_bytes2copy;
	double m_curr_running_file_size;
	double m_prev_running_file_size;
	int m_prev_amnt_done;
	BOOL m_b_cancel;
	enum OpState_t{ INVALID = 0, WAITING_FOR_DEVICE, WAITING_FOR_VOLUME, COPYING, COMPLETE, WAITING_FOR_RECOVERY_MODE, WAITING_FOR_UPDATER_MODE };
	OpState_t m_OpState;

	CString GetOpStateString(OpState_t _state) const;
	int CheckPath(CString sPath);
	void GetCombinedFileSize(void);
	bool IsFileExist(CString sPathName);
	void EnumGetTotalFileSize(CFileList *_pFileList);
	DWORD CopyFilesToTarget(CFileList *_pFileList, CString _destFolder);

	void OnDownloadProgress(const Device::NotifyStruct& nsInfo);
	DWORD ResetToRecovery();
	DWORD RecoverDevice();
	DWORD RecoverHidDevice();
	BOOL  FindUpdaterBinary(CString& _updaterPath);

	DWORD	m_dwStartTime;

BOOL m_bOwnMutex;
BOOL GetResetLockMutex(BOOL _bOpComplete);
BOOL ReleaseResetLockMutex(void);

public:
	COpCopy(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo* pOpInfo = NULL);
	virtual BOOL InitInstance(void);
	afx_msg void OnMsgStateChange(WPARAM nEventType, LPARAM dwData);
};


