// StUpdaterDlg.h : header file
//

#if !defined(AFX_STUPDATERDLG_H__B2BF0998_1066_4F49_862A_51F7A8F66E6F__INCLUDED_)
#define AFX_STUPDATERDLG_H__B2BF0998_1066_4F49_862A_51F7A8F66E6F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "afxwin.h"
#include "..\..\Libs\Winsupport\ColorStaticST.h"
#include "..\..\Libs\Winsupport\AutoPlayReject.h"

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
class CColorStaticST;

class CAboutDlg : public CDialog
{
public:
	CAboutDlg(CStConfigInfo* _p_config_info, CWnd* pParent = NULL);
 
// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };

	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CStConfigInfo*		m_p_config_info;
};



/////////////////////////////////////////////////////////////////////////////
// CStUpdaterDlg dialog

class CStUpdaterDlg : public CDialog, public CStProgress
{
// Construction
public:
	CStUpdaterDlg(CStUpdater* updater, CWnd* pParent = NULL);	// full dialog constructor
	CStUpdaterDlg(CStUpdater* updater, USHORT dummy, CWnd* pParent = NULL);	// minimal dialog constructor
	CStUpdaterDlg(CStUpdater* updater, USHORT dummy, SHORT dummy2, CWnd* pParent = NULL);	// standard dialog constructor
public:
	~CStUpdaterDlg(void);
	static DWORD WINAPI ConnectionProgressThread( LPVOID pParam );
	static DWORD WINAPI ConnectionMonitorThread( LPVOID pParam );
	static DWORD WINAPI UpdateTaskProgressThread( LPVOID pParam );
	virtual void SetTotalTasks(ULONG _total_tasks);
	virtual void SetCurrentTask(TASK_TYPE _task, ULONG _range);
    virtual void SetCurrentTask(TASK_TYPE _task, ULONG _range, UCHAR _drive_index);
	virtual void UpdateProgress(BOOL _step_it=TRUE);
	virtual void UpdateGrandProgress();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual ST_ERROR Begin();
	virtual void Relax();
	void SetModal(BOOL);
	void CheckForDevice();
	virtual void OnCancel();
	void ClearInfo();
	void GetDeviceInfo();
	void GetMediaInfo();

	void GetDataDriveInfo();
	virtual void UpdateDeviceMode();
	BOOL OnAutoStart();

	ST_ERROR DownloadFirmware();
	ST_ERROR CheckOperationToPerform(USHORT& _operation);
	void WhatResourcesAreBound();
	BOOL IsRecoveryDriverInstalled();
	LPVOID StLoadStmp3RecResource(USHORT resID, PDWORD pdwSize, BOOL& isLocal);
	BOOL StWriteStmp3RecResource(LPVOID _pPtr, ULONG _dwSize, LPTSTR _pathName);
	
	CStUpdater* GetUpdater() { return m_p_updater; }

// Dialog Data
	//{{AFX_DATA(CStUpdaterDlg)
	enum { IDD = IDD_ADV_STUPDATERDLG };
	enum { IDD2 = IDD_MIN_STUPDATEDLG };
	enum { IDD3 = IDD_STD_STUPDATERDLG };



	CProgressCtrl	m_pb_overall;
	CProgressCtrl	m_pb_task;
	CButton	m_format_data_area_btn;
	CButton m_full_media_erase_btn;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStUpdaterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	BOOL			m_modal;
    HANDLE          m_progress_thread_stop;
	HANDLE			m_monitor_thread_handle;
    HANDLE          m_monitor_stop;
    HANDLE          m_monitor_trigger;
	HANDLE			m_app_closing;
	HANDLE			m_task_progress_mutex;
	HICON			m_hIcon;
	CStUpdater*		m_p_updater;
	USHORT			m_pos_total_progress_bar;
	wchar_t			m_volume_label[MAX_PATH];
	USHORT       	m_operation;
	CFont			m_warning_font;
	CColorStaticST  m_warning_ctrl;
    UINT_PTR        m_init_timer;
	LONG			m_thread_block;
    BOOL            m_advanced_dlg;
	BOOL			m_minimal_dlg;
    BOOL            m_stmp3rec_installed;
	HANDLE			m_hEventKill;
	unsigned int	m_TotalBoundResourceCount;
	unsigned int	m_BoundLangResourceCount;
	LANGID			*m_pBoundIds;
	BOOL			m_bCapacityOver256MB;

public:
	// Generated message map functions
	//{{AFX_MSG(CStUpdaterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnStart();
	afx_msg void OnFormatDataArea();
	afx_msg void OnFullMediaErase();
	afx_msg void OnClose();
	afx_msg void OnDownloadDetails();
	afx_msg void OnShowVersionDetails();
//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	void SetupDisplay();
	void SaveVolumeLabel();
	void RestoreVolumeLabel();
	ST_ERROR DisplayProjectVersion();
    ST_ERROR InstallRecoveryDriver();
	static UINT BugabooWindowCloserThreadProc( LPVOID pParam );
	CWinThread* m_p_bugaboo_window_closer_thread;
	void GetLanguageString( CString& str, LANGID langId);
	void UpdateInfoAfterUpdate();
	void SetOptionsEnabled(BOOL _state);
	void LogDetails(BOOL _full);
	void ClearLogDetails();

public:
	typedef struct
	{
		CString UpdaterInfoVersion;
		CString UpdaterInfoSDKBase;
		CString UpdaterInfoVidPid;
		CString UpdaterInfoBoundResources;
		CString DevInfoChipId;
		CString DevInfoROMRevision;
		CString DevInfoExtRAMSize;
		CString DevInfoVirtualRAMSize;
		CString DevInfoSerialNumber;
		CString DevInfoMode;
		CString DevInfoProtocolVer;
		CString MediaInfoMediaType;
		CString MediaInfoNandChipSelects;
		CString MediaInfoNandMfgId;
		CString MediaInfoNandCellType;
		CString MediaInfoNandIdDetails;
		CString MediaInfoCapacity;
		CString MediaInfoNandPageSize;
		CString DataInfoDriveLetter;
		CString DataInfoCapacity;
		CString DataInfoFreespace;
		CString DataInfoFileSystem;
		CString DataInfoSectorSize;
		CString DataInfoSectorCount;
	} LOGGING_INFO;
	LOGGING_INFO	m_LogInfo;

	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwpData);
    CComboBox m_language_list;
    CComboBox m_filesystem_list;
	afx_msg void OnCbnSelchangeLanguageId();
	afx_msg void OnCbnSelchangeFileSystemId();

	BOOL IsAdvDialog(){return m_advanced_dlg;};
	BOOL IsMinDialog(){return m_minimal_dlg;};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STUPDATERDLG_H__B2BF0998_1066_4F49_862A_51F7A8F66E6F__INCLUDED_)
