// OpCopy.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "PortMgrDlg.h"
#include "stmsg.h"

#include "../../Libs/DevSupport/StPitc.h"
#include "../../Libs/DevSupport/RecoveryDevice.h"
#include "../../Libs/DevSupport/StMtpApi.h"
#include "../../Libs/DevSupport/Volume.h"

#include "OpCopy.h"

#include "../../Libs/DevSupport/WindowsVersionInfo.h"

extern BOOL g_StopFlagged;  // need a global to reflect the STOP state and break out of request for HID mutex
extern HANDLE g_HIDMutex;


// COpCopy
//UINT DoCopy( LPVOID pParam );

IMPLEMENT_DYNCREATE(COpCopy, COperation)
DWORD CALLBACK CopyProgressRoutine(
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
CMutex COpCopy::m_mutex;

COpCopy::COpCopy(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo *pOpInfo)
: COperation(pPortMgrDlg, pUSBPort, pOpInfo, COPY_OP)
, m_OpState(INVALID)
, m_tot_bytes2copy(0)
, m_curr_file(_T(""))
, m_b_cancel(false)
{
	ASSERT(m_pUSBPort);
	ASSERT(m_pOpInfo);
	m_src_path = m_pOpInfo->GetPath();
	m_iDuration=0;
	m_sDescription.LoadString(IDS_OPCOPY_COPYING); 
	GetCombinedFileSize();
	m_dwStartTime = 0;

}

COpCopy::~COpCopy(void)
{
}

BOOL COpCopy::InitInstance(void)
{

	m_OpState = WAITING_FOR_DEVICE;
	m_dwStartTime = 0;

	m_bOwnMutex = FALSE;
	g_StopFlagged = FALSE;

	return TRUE;
}

void COpCopy::GetCombinedFileSize(void)
{
	m_tot_bytes2copy = 0;
	EnumGetTotalFileSize( &m_pOpInfo->m_FileList );
}

void COpCopy::EnumGetTotalFileSize(CFileList *_pFileList)
{
	for (int i = 0; i < _pFileList->GetCount(); ++i)
	{
		CFileList::PFILEITEM pItem = _pFileList->GetAt(i);

		if (pItem->m_pSubFolderList) // we have a folder/sublist
		{
			EnumGetTotalFileSize(pItem->m_pSubFolderList);
		}
		else // source is a file
		{
			++m_iDuration;
			m_tot_bytes2copy += pItem->m_dwFileSize;
		}
	}
}


BEGIN_MESSAGE_MAP(COpCopy, COperation)
	ON_THREAD_MESSAGE(WM_MSG_OPEVENT, OnMsgStateChange)
END_MESSAGE_MAP()

// COpCopy message handlers
void COpCopy::OnMsgStateChange(WPARAM nEventType, LPARAM dwData)
{
	CString LogStr, taskMsg;


	if ( nEventType == OPEVENT_KILL ) 
	{
		ATLTRACE(_T("%s Updater Event: %s \r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType));
		m_bStart = false;
		m_b_cancel = !m_bStart;
        AfxEndThread(0);
		return;
    }

	ATLTRACE(_T("%s Copy Event: %s Msg: %s DevState: %s OpState: %s\r\n"),m_pPortMgrDlg->GetPanel(), GetEventString(nEventType), dwData, GetDeviceStateString(GetDeviceState()), GetOpStateString(m_OpState));

    if((nEventType != OPEVENT_START) && (nEventType != OPEVENT_STOP) && (m_OpState == INVALID))
		return;
	
	switch(nEventType)
	{
		case OPEVENT_START:
			m_bStart = true;
			m_b_cancel = !m_bStart;
			m_iPercentComplete = 0;
//			ATLTRACE(_T("%s Copy Event: OPEVENT_START Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_OpState);
			switch(GetDeviceState())
			{
				case DISCONNECTED:
					m_OpState = WAITING_FOR_DEVICE;
					taskMsg.LoadString(IDS_WAITING_FOR_DEVICE);
					m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));
					break;
				case MSC_MODE:
				case MTP_MODE:
					g_StopFlagged = FALSE;
					if( m_dwStartTime == 0 )
						m_dwStartTime = GetTickCount();

					if (!m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s MTP/MSC mode request (START)\r\n"), m_pPortMgrDlg->GetPanel());
						GetResetLockMutex(FALSE);
					}

					if (m_bOwnMutex)
						ResetToRecovery();

					m_OpState = WAITING_FOR_DEVICE;

					break;

				case UPDATER_MODE:
				case MFG_MSC_MODE:
					m_OpState = COPYING;
					taskMsg.LoadString(IDS_OPCOPY_STARTED);
					m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));

					if( m_dwStartTime == 0 )
						m_dwStartTime = GetTickCount();

					if ( DoCopy() == OPSTATUS_SUCCESS ) {
//						ATLTRACE(_T("%s Copy Event: Complete %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, GetDeviceState(), m_OpState);

						m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
						m_dwStartTime = 0;

						m_OpState = COMPLETE;
						taskMsg.LoadString(IDS_OPCOPY_COMPLETE);
						m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iDuration));
						m_pPortMgrDlg->PostMessage(WM_MSG_PD_EVENT ,CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);
					}
					else {
						m_OpState = INVALID;
						//m_sStatus =	_T("Error copying files.");
					}
					break;

				case RECOVERY_MODE:
					{
//						if ( !m_Timer && m_TimeOutSeconds )
//							m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

						g_StopFlagged = FALSE;
						if( m_dwStartTime == 0 )
							m_dwStartTime = GetTickCount();

						RecoverDevice();
					
						m_OpState = WAITING_FOR_DEVICE;

						break;
					}

				case HID_MODE:
					{
//						if ( !m_Timer && m_TimeOutSeconds )
//							m_Timer = m_pPortMgrDlg->SetTimer(LOADER_OP, m_TimeOutSeconds * 1000, NULL);

						g_StopFlagged = FALSE;
						if( m_dwStartTime == 0 )
							m_dwStartTime = GetTickCount();

						if (!m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s HID mode request (START)\r\n"), m_pPortMgrDlg->GetPanel());
							GetResetLockMutex(FALSE);
						}

						if (m_bOwnMutex)
						{
							DWORD rc;
							ATLTRACE(_T("MUTEX - %s HID mode has mutex hid recover (START)\r\n"), m_pPortMgrDlg->GetPanel());
							rc = RecoverHidDevice();
							if (rc == ERROR_SUCCESS)
								m_OpState = WAITING_FOR_DEVICE;
							else
							{
								if (rc == ERROR_FILE_NOT_FOUND)
								{
									CString tmp;
									tmp.LoadStringW(IDS_OPINFO_ERR_MISSING_FILE);
									taskMsg.Format(tmp, _T("updater.sb"));
									m_OpState = INVALID;
								}
							}
						}
					}
					break;

				case CONNECTED_UNKNOWN:
				default:
					m_OpState = INVALID;
					taskMsg.LoadString(IDS_ERROR_INVALID_DEVICE);
					break;
			}
			break; // end OPEVENT_START

		case OPEVENT_STOP:
//			ATLTRACE(_T("%s Copy Event: OPEVENT_STOP Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_OpState);
			g_StopFlagged = TRUE;
			m_bStart = false;
			m_b_cancel = !m_bStart; // stops the CopyFileEx() function
			if(m_OpState == COPYING)
			{
				taskMsg.LoadString(IDS_OPCOPY_STOPPED_BY_USER);
				m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));
				//Sleep(1000);
				//m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);
			}
    		m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);
			break;

		case OPEVENT_DEVICE_ARRIVAL:
//			ATLTRACE(_T("%s Copy Event: OPEVENT_DEVICE_ARRIVAL Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_OpState);
			switch(GetDeviceState())
			{
				case UPDATER_MODE:
				case MFG_MSC_MODE:
					m_OpState = WAITING_FOR_VOLUME;
					taskMsg =	_T("Waiting for drive...");
					m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));
					break;

				case MTP_MODE:
				{
					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s MTP mode release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					if (g_StopFlagged)
					{
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						break;
					}

					if ( m_OpState == WAITING_FOR_DEVICE )
					{
//						if ( !m_Timer && m_TimeOutSeconds )
//							m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

						if( m_dwStartTime == 0 )
							m_dwStartTime = GetTickCount();

						if (!m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s MTP mode request (DEV_ARR)\r\n"), m_pPortMgrDlg->GetPanel());
							GetResetLockMutex(FALSE);
						}

						if (!g_StopFlagged)
						{
							ATLTRACE(_T("MUTEX - %s has mutext\r\n"), m_pPortMgrDlg->GetPanel());
							ResetToRecovery();
							m_OpState = WAITING_FOR_DEVICE;
						}
						else
						{
							taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
							break;
						}
					}
					else
					{
							m_OpState = INVALID;
					}					

					break;
				}

				case RECOVERY_MODE:
				{
					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s Recovery mode release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					if (g_StopFlagged)
					{
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						break;
					}

					if ( m_OpState == WAITING_FOR_RECOVERY_MODE ||
						 m_OpState == WAITING_FOR_DEVICE )
					{
//						if ( !m_Timer && m_TimeOutSeconds )
//							m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

						if( m_dwStartTime == 0 )
							m_dwStartTime = GetTickCount();

						if (!m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s Recovery mode request (DEV_ARR)\r\n"), m_pPortMgrDlg->GetPanel());
							GetResetLockMutex(FALSE);
						}

						if (m_bOwnMutex)
						{
							RecoverDevice();
							m_OpState = WAITING_FOR_DEVICE;
						}
					}
					else
					{
						m_OpState = INVALID;
					}					

					break;
				}
				case HID_MODE:
				{
					if (m_bOwnMutex)
					{
						ATLTRACE(_T("MUTEX - %s HID mode device arrival release\r\n"), m_pPortMgrDlg->GetPanel());
						ReleaseResetLockMutex();
					}

					if (g_StopFlagged)
					{
						taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						break;
					}

					if ( m_OpState == WAITING_FOR_RECOVERY_MODE ||
						 m_OpState == WAITING_FOR_DEVICE )
					{
//						if ( !m_Timer && m_TimeOutSeconds )
//							m_Timer = m_pPortMgrDlg->SetTimer(UPDATE_OP, m_TimeOutSeconds * 1000, NULL);

						if( m_dwStartTime == 0 )
							m_dwStartTime = GetTickCount();

						if (!m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s HID mode device arrival request\r\n"), m_pPortMgrDlg->GetPanel());
							GetResetLockMutex(FALSE);
						}

						if (m_bOwnMutex)
						{
							ATLTRACE(_T("MUTEX - %s has mutex\r\n"), m_pPortMgrDlg->GetPanel());

							if (RecoverHidDevice() == ERROR_SUCCESS)
							{
								m_OpState = WAITING_FOR_UPDATER_MODE;
							}
							else
								m_OpState = INVALID;

						}
						else
						{
							taskMsg.LoadString(IDS_STOPPED_BY_USER); // "Operation stopped by user."
						}
					}
					else if (m_OpState != WAITING_FOR_UPDATER_MODE)
					{
						m_OpState = INVALID;
						taskMsg =	_T("Error: Invalid state.");
					}					
					break;
				}

				case DISCONNECTED:
				case CONNECTED_UNKNOWN:
				default:
					m_OpState = INVALID;
					taskMsg =	_T("Error: Invalid device.");
					break;
			}
			break; // end OPEVENT_DEVICE_ARRIVAL

		case OPEVENT_VOLUME_ARRIVAL:
			if(m_OpState == COMPLETE || m_OpState == COPYING ) // got the drive msg late
				break;
//			ATLTRACE(_T("%s Copy Event: OPEVENT_VOLUME_ARRIVAL Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_OpState);
			if ( GetDeviceState() == UPDATER_MODE ||
				 GetDeviceState() == MFG_MSC_MODE ||
				 GetDeviceState() == MSC_MODE ) {
				m_OpState = COPYING;
				taskMsg =	_T("Copy started...");
				m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));

				if( m_dwStartTime == 0 )
					m_dwStartTime = GetTickCount();

				if ( DoCopy() == OPSTATUS_SUCCESS ) {
					taskMsg =	_T("Copy complete.");

					m_pPortMgrDlg->OpCompleteTick(GetTickCount() - m_dwStartTime);
					m_dwStartTime = 0;

					m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iDuration));
					m_pPortMgrDlg->PostMessage(WM_MSG_PD_EVENT ,CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_SUCCESS);
				}
				else {
					m_OpState = INVALID;
					taskMsg =	_T("Error: Copy failed.");
					//m_sStatus =	_T("Error copying files.");
				}
			}
			else {
				m_OpState = INVALID;
				taskMsg =	_T("Error: Invalid device.");
			}
			break;

		case OPEVENT_VOLUME_REMOVAL:
//			ATLTRACE(_T("%s Copy Event: OPEVENT_VOL_REMOVAL Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_OpState);
			break;
		case OPEVENT_DEVICE_REMOVAL: 
			// probably already broke by the time it gets here
//			ATLTRACE(_T("%s Copy Event: OPEVENT_DEV_REMOVAL Msg: %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_OpState);
			if(m_OpState == WAITING_FOR_UPDATER_MODE ||
				m_OpState == WAITING_FOR_RECOVERY_MODE ||
				m_OpState == WAITING_FOR_DEVICE)
			{
				taskMsg.LoadString(IDS_WAITING_FOR_DEVICE);
				m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(m_iPercentComplete));
			}
			else
			{
				m_OpState = INVALID;
				taskMsg =	_T("Error: Device removal during copy");
			}
			break;

		default:
			// should never get here
			ASSERT (0);
	}

	if(m_OpState == INVALID)
	{
//		ATLTRACE(_T("%s Copy Event: Error %s DevState: %d OpState: %d\r\n"),m_pPortMgrDlg->GetPanel(), dwData, m_DeviceState, m_OpState);
		m_bStart = false;
		m_b_cancel = !m_bStart; // stops the CopyFileEx() function
		if (taskMsg.IsEmpty())
			taskMsg = _T("ERROR");
		m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(0));
		m_pPortMgrDlg->SendMessage(WM_MSG_PD_EVENT, CPortMgrDlg::PD_EVNT_OP_COMPLETE, OPSTATUS_ERROR);
	}
}


DWORD CALLBACK CopyProgressRoutine(
  LARGE_INTEGER TotalFileSize,
  LARGE_INTEGER TotalBytesTransferred,
  LARGE_INTEGER StreamSize,
  LARGE_INTEGER StreamBytesTransferred,
  DWORD dwStreamNumber,
  DWORD dwCallbackReason,
  HANDLE hSourceFile,
  HANDLE hDestinationFile,
  LPVOID lpData
  )
{
	COpCopy * pOp = (COpCopy*)lpData;
	ASSERT(pOp);
	CString dbg_str;

	CSingleLock sLock(&pOp->m_mutex);
//	if ( sLock.IsLocked() )
//	{
//		dbg_str.Format( _T(" - Had to wait for lock.\r\n") );
//		ATLTRACE(_T(" - Had to wait for lock.\r\n"));
//	}
//	int retries;
//	for ( retries = 100; !sLock.Lock(50) && retries; --retries ) {
//		dbg_str.Format(_T(" - Waiting for lock. Try: %d, Hub %d, Port %d\r\n"), 100-retries, hub_index, port_index);
//		ATLTRACE(_T(" - Waiting for lock.\r\n"));
//		Sleep(50);
//	}
	if ( !sLock.Lock(50) /*retries == 0*/ )
	{
//		ATLTRACE(_T(" - ERROR: Could not get lock.\r\n"));
		ASSERT(0);
		return PROGRESS_STOP;
	}

//	dbg_str.Format(_T("%s - Copying %s: %d"),pOp->m_pPortMgrDlg->GetPanel(), pOp->m_curr_file, dwCallbackReason);
//	ATLTRACE(dbg_str);

	if ( dwCallbackReason == CALLBACK_CHUNK_FINISHED ) {
		double prog;
		CString status;
		prog = ((pOp->m_prev_running_file_size + TotalBytesTransferred.QuadPart)/pOp->m_tot_bytes2copy)*pOp->m_iDuration;
		status.Format(IDS_OPCOPY_COPYING_FILE,  pOp->m_curr_file);
		pOp->m_pPortMgrDlg->UpdateUI(status, (int)prog-pOp->m_prev_amnt_done);
		pOp->m_prev_amnt_done = (int)prog;
	}
	else if ( dwCallbackReason == CALLBACK_STREAM_SWITCH ) {
		pOp->m_prev_running_file_size += pOp->m_curr_running_file_size;
		pOp->m_curr_running_file_size = (double)TotalFileSize.QuadPart;
	}

	MSG msg;
	while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )  
	{ 
        pOp->PumpMessage( );
    }

	sLock.Unlock();

//	dbg_str.Format(_T(" - Unlocked.\r\n") );
//	ATLTRACE(dbg_str);

	return PROGRESS_CONTINUE;
}

DWORD COpCopy::DoCopy(void)
{
	CString log_text;
    BSTR bstr_log_text;
	CString file, src_file, dst_file;
    UCHAR data_drive_letter_index = m_pOpInfo->GetDataDriveNum() + (m_pOpInfo->GetDataDriveNum()*2);

	m_prev_amnt_done = 0;
	m_prev_running_file_size = m_curr_running_file_size = 0;
    log_text.Format(_T("COPY %s\\%s %s DRIVE %d  : "), m_pProfile->GetUsbVid(), 
                                                       m_pProfile->GetUsbPid(), 
                                                       _T(""), //GetChipSN() 
                                                       m_pOpInfo->GetDataDriveNum());
    
    // Can't index an unvailable drive, return error if drive number is too high.
    if (data_drive_letter_index >= m_pUSBPort->GetDriveLetters().GetLength())
    {
		log_text.Append(_T("ERROR, invalid drive number\n"));
		ATLTRACE(log_text);
        // Log event
        bstr_log_text = log_text.AllocSysString();
        ((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
        return OPSTATUS_ERROR;
    }

    m_dest_path.Format(_T("%c:"), m_pUSBPort->GetDriveLetters().GetAt(data_drive_letter_index));

	if ( CopyFilesToTarget(&m_pOpInfo->m_FileList, m_dest_path) != OPSTATUS_SUCCESS )
	{
		log_text.Append(_T("ERROR\n"));
		ATLTRACE(log_text);
        // Log event
        bstr_log_text = log_text.AllocSysString();
        ((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);
		if( gWinVersionInfo().IsWin2K() )
			Sleep(7500);
		return OPSTATUS_ERROR;	
	}

	log_text.Append(_T("SUCCESS\n"));
    // Log event
    bstr_log_text = log_text.AllocSysString();
    ((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_LOGEVENT, CEventLogger::LOGEVENT_APPEND, (LPARAM)bstr_log_text);

    return OPSTATUS_SUCCESS;
}



DWORD COpCopy::CopyFilesToTarget(CFileList *_pFileList, CString _destFolder)
{
	SHFILEOPSTRUCT FileOp;

	FileOp.hwnd = ((CMainFrame*)theApp.GetMainWnd())->GetSafeHwnd();
	FileOp.hNameMappings = NULL;
    FileOp.lpszProgressTitle = NULL;

	int count = _pFileList->GetCount();
	for (int i = 0; i < count; ++i)
	{
		CString status;
		DWORD err;
		CFileList::PFILEITEM pItem = _pFileList->GetAt(i);

		m_curr_file = pItem->m_csFileName; 

		if (pItem->m_pSubFolderList) // we have a folder/sublist
		{
			CString newFolder = _destFolder + _T("\\") + pItem->m_csFileName;

			// create the folder now
			status.Format(IDS_OPCOPY_CREATING_DIR, pItem->m_csFileName);
			m_pPortMgrDlg->UpdateUI(status, 0);
			if (!CreateDirectory(newFolder, NULL))
			{
				err = GetLastError();
				status.Format(IDS_OPCOPY_CREATE_DIR_ERR, err);
				ATLTRACE(status + _T("\n"));
				m_pPortMgrDlg->UpdateUI(status, 0);
		        return OPSTATUS_ERROR;
			}

			err = CopyFilesToTarget(pItem->m_pSubFolderList, newFolder);
			if ( err != OPSTATUS_SUCCESS )
				return err;
		}
		else
		{
			m_curr_running_file_size = pItem->m_dwFileSize;
			FileOp.wFunc = FO_COPY;
			pItem->m_csFilePathName.AppendChar(_T('\0'));
			FileOp.pFrom = pItem->m_csFilePathName;
	        FileOp.pTo = _destFolder;
			FileOp.fFlags = 0;
			status.Format(IDS_OPCOPY_COPYING_FILE,  m_curr_file);
			if ( SHFileOperation(&FileOp) != ERROR_SUCCESS )
			{	
				err = GetLastError();
				status.Format(IDS_OPCOPY_COPY_ERR, err);
				ATLTRACE(status + _T("\n"));
				return OPSTATUS_ERROR;
			}
			m_prev_running_file_size += m_curr_running_file_size;
			double prog = (m_prev_running_file_size/m_tot_bytes2copy)*m_iDuration;
			m_pPortMgrDlg->UpdateUI(status, (int)prog-m_prev_amnt_done);
		    m_prev_amnt_done = (int)prog;
		}
	}

	return  OPSTATUS_SUCCESS;
}


bool COpCopy::IsFileExist(CString sPathName)
{
	HANDLE hFile;

	hFile = CreateFile(sPathName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
		return false;
	
	CloseHandle(hFile);
	return true;
}

int COpCopy::CheckPath(CString sPath)
{
	DWORD dwAttr = GetFileAttributes(sPath);
	if (dwAttr == 0xffffffff) 
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) 
			return PATH_NOT_FOUND;
		return PATH_ERROR;
	}

	if (dwAttr & FILE_ATTRIBUTE_DIRECTORY) 
		return PATH_IS_FOLDER;
	
	return PATH_IS_FILE;
}

BOOL COpCopy::FindUpdaterBinary(CString& _updaterPath)
{
	BOOL result = FALSE;
	
	_updaterPath.Format(_T("%s\\updater.sb"), ((CPlayerProfile*)m_pOpInfo->GetProfile())->GetProfilePath());

	if (IsFileExist(_updaterPath))
	{
		result = TRUE;
	}
	return result;
}

DWORD COpCopy::ResetToRecovery()
{
	DWORD error;
	CString taskMsg;

	// No Task progress bar but bump the Operation progress bar
	taskMsg.LoadString(IDS_OPUPDATER_RESETTING_DEVICE); // "Resetting device..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(1)); // STAGE 1 of the Copy Operation

	error = m_pUSBPort->_device->ResetToRecovery();
	if ( error != ERROR_SUCCESS )
	{
		error = m_pUSBPort->_device->OldResetToRecovery();
	}

	if ( error == ERROR_SUCCESS )
	{
		ATLTRACE(_T("%s ResetToRecovery - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	}
	else
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s ResetToRecovery - FAILED.\r\n"), error, m_pPortMgrDlg->GetPanel());
	}

	return error;
}


void COpCopy::OnDownloadProgress(const Device::NotifyStruct& nsInfo)
{
	if (nsInfo.position)
	{
		m_pPortMgrDlg->UpdateUI(NULL, 0, 0, nsInfo.position);
	}
}
//// RecoverDevice()
//
// RecoverDevice() is STAGE 2 of the Update Operation
// Valid OpStates are WAITING_FOR_RECOVERY_MODE and WAITING_FOR_DEVICE
//
// changes member variable m_OpState
//
////
DWORD COpCopy::RecoverDevice()
{
	DWORD error;
	CString taskMsg;
	Device::UI_Callback callback(this, &COpCopy::OnDownloadProgress);

	taskMsg.LoadString(IDS_OPUPDATER_LOADING_USBMSC);
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));
	
	RecoveryDevice* pRecoveryDevice = dynamic_cast<RecoveryDevice*>(m_pUSBPort->_device);
	
	if ( pRecoveryDevice == NULL )
	{
		error = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s RecoverDevice() - No device.\r\n"), error, m_pPortMgrDlg->GetPanel());
		return error;
	}

	CString recoveryFirmwareFilename = m_pOpInfo->GetPath();
	recoveryFirmwareFilename.AppendFormat(_T("\\%s"), STATIC_ID_FW_FILENAME);
	if ( m_pOpInfo->UseMultipleStaticIdFw() )
	{
		recoveryFirmwareFilename.AppendFormat(_T("%c"), m_pPortMgrDlg->GetPanelIndex()+1);
	}
	recoveryFirmwareFilename.Append(STATIC_ID_FW_EXTENSION);

	StFwComponent fwObject(recoveryFirmwareFilename);

	// Set up the Task progress bar and bump the Operation progress bar
	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, fwObject.GetShortFileName().c_str());		// "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2), (int)fwObject.size());               // STAGE 2 of the Update Operation

	if ( (error = fwObject.GetLastError()) != ERROR_SUCCESS )
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s RecoverDevice() - Firmware object error.\r\n"), error, m_pPortMgrDlg->GetPanel());
		return error;
	}

	//
	// Do the Download
	//
	error = pRecoveryDevice->Download(fwObject, callback);
	if(error != ERROR_SUCCESS) 
	{
		ATLTRACE(_T("!!!ERROR!!! (%d): %s RecoverDevice() - Failed during Download().\r\n"), error, m_pPortMgrDlg->GetPanel());
		return error;
	}

	// Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);

	ATLTRACE(_T("%s RecoverDevice - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	return ERROR_SUCCESS;
}

//// RecoverhidDevice()
//
// RecoverHidDevice() is STAGE 2 of the Copy Operation
// Valid OpStates are WAITING_FOR_RECOVERY_MODE and WAITING_FOR_DEVICE
//
// changes member variable m_OpState
//
////

DWORD COpCopy::RecoverHidDevice()
{

	DWORD ReturnVal = ERROR_SUCCESS;
	CString taskMsg, fileName;

	taskMsg.LoadString(IDS_OPLOADER_LOAD_STARTED);
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2));

	HidDevice* pHidDevice = dynamic_cast<HidDevice*>(m_pUSBPort->_device);

	if ( pHidDevice == NULL )
	{
		ReturnVal = ERROR_INVALID_HANDLE;
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}


	if (!FindUpdaterBinary(fileName))
	{
		ReturnVal = ERROR_FILE_NOT_FOUND;
		ATLTRACE(_T("!!!ERROR!!! (%d): Unable to load %s. OpState: %s\r\n"), ReturnVal, fileName, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

//	CString fileName = //m_pOpInfo->GetDriveArray().GetDrive(media::DriveTag_Updater).Name;
//					m_pOpInfo->GetPath() + _T("\\updater.sb");

	StPitc myNewFwCommandSupport(pHidDevice, (LPCTSTR)fileName,
		m_pOpInfo->GetProfile()->m_bLockedProfile ? 
			StFwComponent::LoadFlag_ResourceOnly :
			StFwComponent::LoadFlag_FileFirst); 

	if ( myNewFwCommandSupport.GetFwComponent().GetLastError() != ERROR_SUCCESS )
	{
		ReturnVal = myNewFwCommandSupport.GetFwComponent().GetLastError();
		ATLTRACE(_T("!!!ERROR!!! (%d): %s No FW component. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Set up the Task progress bar and bump the Operation progress bar
	taskMsg.Format(IDS_OPUPDATER_LOADING_FILE, myNewFwCommandSupport.GetFwComponent().GetShortFileName().c_str());   // "Loading <filename> ..."
	m_pPortMgrDlg->UpdateUI(taskMsg, ProgressDelta(2), (int)myNewFwCommandSupport.GetFwComponent().size(), 0);       // STAGE 2 of the Load Operation

	Device::UI_Callback callback(this, &COpCopy::OnDownloadProgress);
	ReturnVal = myNewFwCommandSupport.DownloadPitc(callback);

	if(ReturnVal != ERROR_SUCCESS) 
	{
		taskMsg.Format(IDS_OPLOADER_LOAD_ERROR, ReturnVal);
		ATLTRACE(_T("!!!ERROR!!! (%d): %s Failed to load HID device. OpState: %s\r\n"), ReturnVal, m_pPortMgrDlg->GetPanel(), GetOpStateString(m_OpState));
		return ReturnVal;
	}

	// Turn off the Task progress bar
	m_pPortMgrDlg->UpdateUI(NULL);


	ATLTRACE(_T("%s RecoverHidDevice - SUCCESS.\r\n"),m_pPortMgrDlg->GetPanel());
	return ERROR_SUCCESS;
}



#define ONE_MINUTE		60000
#define THIRTY_SECONDS	30000

BOOL COpCopy::GetResetLockMutex(BOOL _bOpComplete)
{
	int iCurrentPriority = THREAD_PRIORITY_NORMAL;
	DWORD timeout = THIRTY_SECONDS; // start off with this;

	if (!m_bOwnMutex)
	{
		while ( TRUE )
		{
			DWORD rc;
#ifdef SERIALIZE_HID
			rc = WaitForSingleObject(g_HIDMutex, timeout);
#else
			rc = WAIT_OBJECT_0;
#endif

			if (rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED)
			{
				m_bOwnMutex = TRUE;
				break;
			}
			if ( g_StopFlagged && _bOpComplete )
			{
				ReleaseResetLockMutex();
				break;
			}

			iCurrentPriority = GetThreadPriority();

			// boost if needed and go again
			if ( iCurrentPriority == THREAD_PRIORITY_NORMAL)
			{
				ATLTRACE(_T("MUTEX - %s Boosting thread priority to above normal...\r\n"), m_pPortMgrDlg->GetPanel());
				SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);
				timeout = ONE_MINUTE;
			}
			if ( iCurrentPriority == THREAD_PRIORITY_ABOVE_NORMAL)
			{
				ATLTRACE(_T("MUTEX - %s Boosting thread priority to highest...\r\n"), m_pPortMgrDlg->GetPanel());
				SetThreadPriority(THREAD_PRIORITY_HIGHEST);
				timeout = INFINITE;
			}
		}
	}

	if ( iCurrentPriority != THREAD_PRIORITY_NORMAL)
		SetThreadPriority(THREAD_PRIORITY_NORMAL);

	return m_bOwnMutex;
}

BOOL COpCopy::ReleaseResetLockMutex()
{
	if (m_bOwnMutex)
	{
#ifdef SERIALIZE_HID
		ReleaseMutex(g_HIDMutex);
#endif
		m_bOwnMutex = FALSE;
		Sleep(0);
	}
	return !m_bOwnMutex;
}

CString COpCopy::GetOpStateString(OpState_t _state) const
{
	CString str;
	switch (_state)
	{
	case INVALID:
		str = "INVALID";
		break;
	case WAITING_FOR_DEVICE:
		str = "WAITING_FOR_DEVICE";
		break;
	case WAITING_FOR_VOLUME:
		str = "WAITING_FOR_VOLUME";
		break;
	case COPYING:
		str = "COPYING";
		break;
	case COMPLETE:
		str = "COMPLETE";
		break;
	default:
		str = "OP_UNKNOWN_STATE";
		break;
	}
	return str;
}
