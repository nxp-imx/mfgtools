// PortMgrDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "MainFrm.h"
#include "PortMgrDlg.h"
#include "OpCopy.h"
#include "OpRegistry.h"
#include "OpMonitor.h"
#include "OpUpdater.h"
#include "OpUtpUpdate.h"
#include "OpErase.h"
#include "OpLoader.h"
#include "OpOTP.h"

#include "../../Libs/DevSupport/UsbHubMgr.h"
#include "../../Libs/DevSupport/DeviceManager.h"

#define PROGRESS_RGB_BLUE	RGB(9,106,204)
#define PROGRESS_RGB_GREEN	RGB(78,217,11)
#define PROGRESS_RGB_RED	RGB(255,0,0)

extern BOOL g_TestLoop;

// CPortMgrDlg dialog

const PTCHAR CPortMgrDlg::PortEventStrings[] = {_T("PD_EVNT_START"), _T("PD_EVNT_STOP"),
												_T("PD_EVNT_DEVICE_ARRIVAL"), _T("PD_EVNT_DEVICE_REMOVAL"),
												_T("PD_EVNT_VOLUME_ARRIVAL"), _T("PD_EVNT_VOLUME_REMOVAL"),
												_T("PD_EVNT_HUB_ARRIVAL"), _T("PD_EVNT_HUB_REMOVAL"),
												_T("PD_EVNT_CONFIG_CHANGE"), _T("PD_EVNT_OP_COMPLETE"),
												_T("PD_EVNT_START_NOTIFY")};

IMPLEMENT_DYNAMIC(CPortMgrDlg, CDialogBar)
CPortMgrDlg::CPortMgrDlg(CWnd* pParent /*=NULL*/, int _index /*=0*/)
	: CDialogBar()
	, m_port_display_index(_index)
	, m_label(_T(""))
	, m_start(0)
	, m_mode(OPMODE_INVALID)
	, m_p_usb_port(NULL)
	, m_usb_hub_name(_T(""))
	, m_usb_hub_index(-1)
//clw	, m_dev_path(_T(""))
	, m_p_curr_op(NULL)
	, m_p_monitor_op(NULL)
	, m_duration(0)
	, m_curr_op_pos(0)
	, m_p_profile(NULL)
	, m_hDevChangeCallback(NULL)
	, m_hComPort(INVALID_HANDLE_VALUE)
	, m_OpsRunning(FALSE)
{
	m_label.Format(_T("Panel %c"), _T('A')+_index);
	if ( pParent )
		m_bAutoDelete = true;

	m_TaskElapsedTime = 0;
//	m_dev_list.Init(this);
}

CPortMgrDlg::~CPortMgrDlg()
{
	if ( m_hDevChangeCallback )
	{
		gDeviceManager::Instance().Unregister(m_hDevChangeCallback);
		m_hDevChangeCallback = NULL;
	}

	if (m_hComPort != INVALID_HANDLE_VALUE)
		CloseHandle(m_hComPort);
}

#ifdef _DEBUG
void CPortMgrDlg::Dump(CDumpContext& dc) const
{
	CDialogBar::Dump(dc);
	dc << "CPortMgrDlg = " << this;
}
#endif

void CPortMgrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PORT_DRIVE, m_port_drive_ctrl);
	DDX_Control(pDX, ID_PORT_GROUP_BOX, m_port_group_ctrl);
	DDX_Control(pDX, IDC_PORT_DRIVE_TEXT, m_port_drive_text_ctrl);
	DDX_Control(pDX, IDC_PORT_OPERATION_TEXT, m_port_operation_text_ctrl);
	DDX_Control(pDX, IDC_PORT_STATUS_EDIT, m_port_status_ctrl);
	DDX_Control(pDX, IDC_PORT_PROGRESS, m_port_progress_ctrl);
	DDX_Control(pDX, IDC_PORT_TASK_PROGRESS, m_port_task_progress_ctrl);
}

BEGIN_MESSAGE_MAP(CPortMgrDlg, CDialogBar)
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
	ON_MESSAGE(WM_MSG_PD_EVENT, OnMsgStateChange)
	ON_WM_RBUTTONDOWN()
    ON_WM_DESTROY()
	ON_WM_TIMER( )
END_MESSAGE_MAP()

// CPortMgrDlg message handlers
CSize CPortMgrDlg::CalcDynamicLayout(int nLength, DWORD dwMode)
{
	return this->m_sizeDefault;
//	return CDialogBar::CalcDynamicLayout(nLength, dwMode);
}

CSize CPortMgrDlg::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	return this->m_sizeDefault;
//	return CDialogBar::CalcFixedLayout(bStretch, bHorz);
}
/*
CPortMgrDlg::CDevList* CPortMgrDlg::GetDriverKeys(void)
{
	CDevList *ptr = NULL;
	CSingleLock sLock(&m_dev_list.m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	GetDriverKeys() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		ptr = &m_dev_list;
		sLock.Unlock();
	}
	else 
		ATLTRACE(_T("ERROR: GetDriverKeys() - Could not get lock."));
	return ptr;
}
*/
LRESULT CPortMgrDlg::OnInitDialog(UINT wParam, LONG lParam)
{
	// have to do this since we are a CDialogBar and not a CDialog
	LRESULT bRet = HandleInitDialog(wParam, lParam);
	if (!UpdateData(FALSE)) {
		TRACE1("Warning: Panel %c - UpdateData failed during dialog init.\n", _T('A')+m_port_display_index);
		return bRet;
	}

	Localize();
	m_port_task_progress_ctrl.ShowWindow(SW_HIDE);

	// assign ourself to our old usb port if it is in the list or
	// unassign ourself if our old usb port is not in the list
	m_usb_hub_name = AfxGetApp()->GetProfileString(m_label, _T("Hub Name"), _T(""));
	m_usb_hub_index = AfxGetApp()->GetProfileInt(m_label, _T("Hub Index"), -1);

	m_useComPortMsgs = AfxGetApp()->GetProfileInt(_T("Options"), _T("Enable COM port status msgs"), 0);

/*
	// Create a DeviceManager::DeviceChangeCallback to objectize the 
	// callback member function. In this example, the Functor 'cmd'
	// is created to objectize CDeviceApiDlg::OnDeviceChangeNotify().
	DeviceManager::DeviceChangeCallback cmd(this, &CPortMgrDlg::OnDeviceChangeNotify);
	//
	// Create an Observer object, fill in the desired criteria, and
	// then pass it to Register(const Observer& observer)
	//
	DeviceManager::Observer callMe;
	callMe.NotifyFn = cmd;
	callMe.NotifyType = DeviceManager::ByPort;
	callMe.Hub = m_usb_hub_name;
	callMe.HubIndex = m_usb_hub_index;
	m_hDeviceManagerCallback = gDeviceManager::Instance().Register(callMe);
	//
    // Register with DeviceManager to call us back for any device change (default parameters).
//	m_hDeviceManagerCallback = gDeviceManager::Instance().Register(cmd);
*/
	m_p_profile = theApp.GetConfigMgr()->GetPlayerProfile();
	SetUsbPort( FindDlgPort() );
	if ( m_p_usb_port && m_p_profile && m_p_profile->IsValid() )
		CreateOps();

	if (m_useComPortMsgs)
	{
		CString csComName;
		DCB ComDCB;
		csComName.Format(_T("\\\\.\\COM%d"), m_port_display_index+1);
		m_hComPort = CreateFile(csComName,
	            GENERIC_READ|GENERIC_WRITE,//access ( read and write)
			    0,    //(share) 0:cannot share the COM port                        
		        0,    //security  (None)                
				OPEN_EXISTING,// creation : open_existing
	            FILE_FLAG_WRITE_THROUGH,// we don't want overlapped operation
		        0// no templates file for COM port...
			    );
		if (m_hComPort != INVALID_HANDLE_VALUE)
		{
			GetCommState(m_hComPort, &ComDCB);
			ComDCB.BaudRate = 9600;
			ComDCB.ByteSize = 8;
			ComDCB.Parity = 0;
			ComDCB.StopBits = 1;
			ComDCB.fBinary = FALSE;
			SetCommState(m_hComPort, &ComDCB);
			ATLTRACE("Panel %c - Opened COM%d\n", _T('A')+m_port_display_index, m_port_display_index+1);
			ATLTRACE("  Rate: %d\n  Bytesize: %d\n  Parity: %d\n  Stopbits: %d\n  Binary: %d\n",
							ComDCB.BaudRate, ComDCB.ByteSize,
							ComDCB.Parity, ComDCB.StopBits,
							ComDCB.fBinary);

		}
		else
			ATLTRACE("Panel %c - Failed to open COM%d rc=0x%x\n", _T('A')+m_port_display_index, m_port_display_index+1, GetLastError());
	}

//	m_port_progress_ctrl.SubclassDlgItem(IDC_PORT_PROGRESS, this);
	if ( gWinVersionInfo().IsWinXPSP1() || gWinVersionInfo().IsWinXPSP2() )
		SetWindowTheme (m_port_progress_ctrl.GetSafeHwnd(), TEXT (" "), TEXT (" "));

	return bRet;
}

INT_PTR CPortMgrDlg::CreateMonitor(void)
{
	ASSERT( m_p_usb_port );

	m_p_curr_op = m_p_monitor_op = new COpMonitor(this, m_p_usb_port);
	if (!m_p_curr_op->CreateThread(/*CREATE_SUSPENDED*/)) {
		ATLTRACE("Warning: Panel %c - CreateMonitor() failed to create a Monitor Op.\n", _T('A')+m_port_display_index);
		delete m_p_curr_op;
		m_p_monitor_op = m_p_curr_op = NULL;
		return false;
	}
	else if ( m_start == false ) {
		m_mode = OPMODE_MONITOR;
		m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_START, 0);
		ATLTRACE("Panel %c - CreateMonitor() Started Monitor Op(%#x, %d).\n", _T('A')+m_port_display_index, m_p_curr_op->m_nThreadID, m_p_curr_op->m_nThreadID);
	}
	return true;
}

INT_PTR CPortMgrDlg::CreateOps(void)
{
	ASSERT( m_p_usb_port );
	ASSERT( m_p_profile->IsValid() );

	if ( !( m_p_ops = m_p_profile->GetOpInfoListPtr() ) ) {
		ATLTRACE("Warning: Panel %c - CreateOps() failed to get a valid OpInfoList.\n", _T('A')+m_port_display_index);
		return false;
	}

	COperation * pOp;
	m_duration = 0;
	POSITION pos = m_p_ops->GetHeadPosition();
	while ( pos ) {
		COpInfo* pOpInfo = m_p_ops->GetNext(pos);
		if ( pOpInfo->IsEnabled() && pOpInfo->GetStatus() == OPINFO_OK ) {
			switch ( pOpInfo->GetType() ) {
				case COperation::COPY_OP:
					m_op_list.AddTail( pOp = new COpCopy(this, m_p_usb_port, pOpInfo) );
					if (!pOp->CreateThread(/*CREATE_SUSPENDED*/)) {
						delete pOp;
						ATLTRACE("Error: Panel %c - CreateOps() - failed to create a COPY operation.\n", _T('A')+m_port_display_index);
						return false;
					}
					m_duration += pOp->GetDuration();
					ATLTRACE("Panel %c - CreateOps() - created a COPY operation(%#x, %d).\n", _T('A')+m_port_display_index, pOp->m_nThreadID, pOp->m_nThreadID);
					break;
				case COperation::LOADER_OP:
					m_op_list.AddTail( pOp = new COpLoader(this, m_p_usb_port, pOpInfo) );
					if (!pOp->CreateThread(/*CREATE_SUSPENDED*/)) {
						delete pOp;
						ATLTRACE("Error: Panel %c - CreateOps() - failed to create a LOAD operation.\n", _T('A')+m_port_display_index);
						return false;
					}
					m_duration += pOp->GetDuration();
					ATLTRACE("Panel %c - CreateOps() - created a LOAD operation(%#x, %d).\n", _T('A')+m_port_display_index, pOp->m_nThreadID, pOp->m_nThreadID);
					break;
				case COperation::ERASE_OP:
					m_op_list.AddTail( pOp = new COpErase(this, m_p_usb_port, pOpInfo) );
					if (!pOp->CreateThread(/*CREATE_SUSPENDED*/)) {
						delete pOp;
						ATLTRACE("Error: Panel %c - CreateOps() - failed to create an ERASE operation.\n", _T('A')+m_port_display_index);
						return false;
					}
					m_duration += pOp->GetDuration();
					ATLTRACE("Panel %c - CreateOps() - created an ERASE operation(%#x, %d).\n", _T('A')+m_port_display_index, pOp->m_nThreadID, pOp->m_nThreadID);
					break;
				case COperation::REGISTRY_OP:
					m_op_list.AddTail( pOp = new COpRegistry(this, m_p_usb_port, pOpInfo) );
					if (!pOp->CreateThread(/*CREATE_SUSPENDED*/)) {
						delete pOp;
						ATLTRACE("Error: Panel %c - CreateOps() - failed to create a REGISTRY operation.\n", _T('A')+m_port_display_index);
						return false;
					}
					m_duration += pOp->GetDuration();
					ATLTRACE("Panel %c - CreateOps() - created a SCRUB operation(%#x, %d).\n", _T('A')+m_port_display_index, pOp->m_nThreadID, pOp->m_nThreadID);
					break;
				case COperation::UPDATE_OP:
					m_op_list.AddTail( pOp = new COpUpdater(this, m_p_usb_port, pOpInfo) );
					if (!pOp->CreateThread(/*CREATE_SUSPENDED*/)) {
						delete pOp;
						ATLTRACE("Error: Panel %c - CreateOps() - failed to create an UPDATE operation.\n", _T('A')+m_port_display_index);
						return false;
					}
					m_duration += pOp->GetDuration();
					ATLTRACE("Panel %c - CreateOps() - created an UPDATE operation(%#x, %d).\n", _T('A')+m_port_display_index, pOp->m_nThreadID, pOp->m_nThreadID);
					break;
				case COperation::OTP_OP:
					m_op_list.AddTail( pOp = new COpOTP(this, m_p_usb_port, pOpInfo) );
					if (!pOp->CreateThread(/*CREATE_SUSPENDED*/)) {
						delete pOp;
						ATLTRACE("Error: Panel %c - CreateOps() - failed to create an OTP operation.\n", _T('A')+m_port_display_index);
						return false;
					}
					m_duration += pOp->GetDuration();
					ATLTRACE("Panel %c - CreateOps() - created an OTP operation(%#x, %d).\n", _T('A')+m_port_display_index, pOp->m_nThreadID, pOp->m_nThreadID);
					break;
				case COperation::UTP_UPDATE_OP:
					m_op_list.AddTail( pOp = new COpUtpUpdate(this, m_p_usb_port, pOpInfo) );
					if (!pOp->CreateThread(/*CREATE_SUSPENDED*/)) {
						delete pOp;
						ATLTRACE("Error: Panel %c - CreateOps() - failed to create a UTP_UPDATE operation.\n", _T('A')+m_port_display_index);
						return false;
					}
					m_duration += pOp->GetDuration();
					ATLTRACE("Panel %c - CreateOps() - created a UTP_UPDATE operation(%#x, %d).\n", _T('A')+m_port_display_index, pOp->m_nThreadID, pOp->m_nThreadID);
					break;
				case COperation::INVALID_OP:
					ATLTRACE("Warning: Panel %c - CreateOps() - INVALID operation, operation ignored.\n", _T('A')+m_port_display_index);
					break;
				default:
					ATLTRACE("Error: Panel %c - CreateOps() - UNSPECIFIED operation, operation ignored.\n", _T('A')+m_port_display_index);
					break;
			} // switch ( type )
		} // if ( enabled & OK )
	} // while ( list is not empty )
	
	m_port_progress_ctrl.SetRange32(0, (int)(m_duration));
	
	return true;
}

void CPortMgrDlg::DeleteOps(void)
{
	COperation *pOp;
    // go through Operations list and delete all operations
    while ( ! m_op_list.IsEmpty() ) {
		pOp = m_op_list.RemoveTail();
        pOp->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_KILL, 0);
		WaitForSingleObject(pOp->m_hThread, INFINITE);
    }
}

void CPortMgrDlg::UpdateUI(LPCTSTR _status, int _opDelta/*=0*/, int _taskRange/*=-1*/, int _taskPosition/*=0*/)
{
	//
	// Manage Status Text
	//
	if ( _status )
	{
		m_port_status_ctrl.SetWindowText(_status);
	}

	//
	// Manage Operation Progress Bar
	//
	int newOpPosition = m_port_progress_ctrl.GetPos() + _opDelta;
	m_port_progress_ctrl.SetPos(newOpPosition < 0 ? 0 : newOpPosition);

	//
	// Manage Task Progress Bar
	//
///	if ( _taskRange == 0 )
///	{
///		// set the task progress position
///		m_port_task_progress_ctrl.SetPos(_taskPosition);
///	}
	if ( _taskRange > 0 )
	{
		// turn on the task progress bar, set the range and set pos=0
		m_port_task_progress_ctrl.SetRange32(0, _taskRange);
		m_port_task_progress_ctrl.SetPos(_taskPosition);
		m_port_task_progress_ctrl.ShowWindow(SW_SHOW);
	}
	else if ( _taskRange == -1 )
	{
		// turn off the task progress bar
		m_port_task_progress_ctrl.ShowWindow(SW_HIDE);
	}

	//
	// Update the Port Drive Letters
	//
	m_port_drive_ctrl.SetWindowText(m_p_usb_port->GetDriveLetters());

}

CPortMgrDlg::OP_MGR_MODE CPortMgrDlg::ProcessOps(void)
{
	OP_MGR_MODE mode;

	m_op_error = false;

	if ( m_start ) {
		if ( m_mode == OPMODE_MONITOR || m_mode == OPMODE_ALL_OPS_COMPLETE ) {
			m_curr_op_pos = m_op_list.GetHeadPosition();
			m_port_progress_ctrl.SetPos(0);
			m_port_progress_ctrl.SendMessage(PBM_SETBARCOLOR, 0, PROGRESS_RGB_BLUE);
		}
		if ( m_curr_op_pos ) {
			m_p_curr_op = m_op_list.GetNext(m_curr_op_pos);
//clw			m_p_usb_port->Refresh();
			m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_START, 0);
			m_port_operation_text_ctrl.SetWindowText(m_p_curr_op->GetDescription());
			mode = OPMODE_RUNNING;
		}
		else
		{
			m_p_curr_op = NULL;
			mode = OPMODE_ALL_OPS_COMPLETE;
		}
	}
	else { // STOP
		mode = OPMODE_MONITOR;
//clw		m_p_usb_port->Refresh();
		m_p_curr_op = m_p_monitor_op;
		m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_START, 0);
		m_port_operation_text_ctrl.SetWindowText(m_p_curr_op->GetDescription());
	}

	return mode;
}

afx_msg void CPortMgrDlg::OnTimer( UINT_PTR nIDEvent )
{
	if ( m_p_curr_op && nIDEvent == (UINT_PTR)m_p_curr_op->GetOpType() )
		m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_TIMEOUT, 0);
}

LRESULT CPortMgrDlg::OnMsgStateChange(WPARAM _event_type, LPARAM _msg_string)
{
	CString msg = (LPCTSTR)_msg_string;
	// **** WARNING **** SysFreeString((BSTR)_msg_string); 
	// must be call after processing the following events:
	// PD_EVNT_HUB_ARRIVAL, PD_EVNT_HUB_REMOVAL, PD_EVNT_DEVICE_ARRIVAL, 
	// PD_EVNT_DEVICE_REMOVAL, PD_EVNT_VOLUME_ARRIVAL, PD_EVNT_VOLUME_REMOVAL

	if (!m_p_usb_port)
		return true;

//	ATLTRACE(_T("%s %s Msg: %s\r\n"),GetPanel(), PortEventStrings[_event_type], msg);

	switch(_event_type)
	{
		case PD_EVNT_START:
			m_start = true;
			// stop the monitoing op
			m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_STOP, 0);
			break;

		case PD_EVNT_STOP:
			m_start = false;
			m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_STOP, 0);
			if ( m_mode == OPMODE_ALL_OPS_COMPLETE )
 				m_mode = ProcessOps();
			break;

		case PD_EVNT_CONFIG_CHANGE:
			// should have gotten a stop msg before we get this message
			ASSERT ( m_start == FALSE );
			DeleteOps();
			m_p_profile = theApp.GetConfigMgr()->GetPlayerProfile();
			if ( m_p_usb_port && m_p_profile && m_p_profile->IsValid() )
				CreateOps();
			break;

		case PD_EVNT_OP_COMPLETE:
			m_port_operation_text_ctrl.SetWindowText(_T(""));
			if ( ( m_curr_op_pos || m_mode == OPMODE_MONITOR ) && ( _msg_string == COperation::OPSTATUS_SUCCESS ) )
			{
				m_mode = ProcessOps();
				if(m_mode == OPMODE_ALL_OPS_COMPLETE) {
					CString resStr;

					// Need to delay a little for device to flush it's internal cache
					// Idle time flush is 2 seconds
					if (!g_TestLoop)
					{
						resStr.LoadString(IDS_WAITING_TO_COMPLETE);						// "Waiting to Complete."
						m_port_status_ctrl.SetWindowText(resStr);
						m_port_progress_ctrl.SetPos((int)m_duration);
						Sleep(3500);
					}
					//
					// Send ScsiStartStopUnit to flush device's cache.
					//
/*					Volume* pMscDevice = dynamic_cast<Volume*>(m_p_usb_port->_device);
					if ( pMscDevice != NULL )
					{
						StScsiStartStopUnit apiStartStop;
						pMscDevice->SendCommand(apiStartStop);
					}
*/
					((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_UPDATE_STATUS, TRUE, (LPARAM)m_TaskElapsedTime );
					m_TaskElapsedTime = 0;

					resStr.LoadString(IDS_PORTMGRDLG_COMPLETE); // "Operations Complete"
					m_port_status_ctrl.SetWindowText(resStr);
					m_port_progress_ctrl.SetPos((int)m_duration);
					m_port_progress_ctrl.SendMessage(PBM_SETBARCOLOR, 0, PROGRESS_RGB_GREEN);
					if (m_hComPort != INVALID_HANDLE_VALUE)
					{
						LPTSTR token = _T("OK\r\n");
						DWORD dwBytesWritten = (DWORD)_tcslen(token);
						if (!WriteFile (m_hComPort, token,dwBytesWritten,&dwBytesWritten , NULL))
						{
							ATLTRACE("Panel %c - Failed to write OK to COM%d rc=0x%x\n", _T('A')+m_port_display_index, m_port_display_index+1, GetLastError());
						}
						else
							ATLTRACE("Panel %c - Wrote OK to COM%d\n", _T('A')+m_port_display_index, m_port_display_index+1);
					}
				}
			}
			else {
				if(m_start)
				{
					m_mode = OPMODE_ALL_OPS_COMPLETE;
					if(_msg_string == COperation::OPSTATUS_SUCCESS)// OPSTATUS_SUCCESS
					{
						CString resStr;

						// Need to delay a little for device to flush it's internal cache
						// Idle time flush is 2 seconds
						if (!g_TestLoop)
						{
							resStr.LoadString(IDS_WAITING_TO_COMPLETE);						// "Waiting to Complete."
							m_port_status_ctrl.SetWindowText(resStr);
							m_port_progress_ctrl.SetPos((int)m_duration);
							Sleep(3500);
						}
						//
						// Send ScsiStartStopUnit to flush device's cache.
						//
/*						Volume* pMscDevice = dynamic_cast<Volume*>(m_p_usb_port->_device);
						if ( pMscDevice != NULL )
						{
							StScsiStartStopUnit apiStartStop;
							pMscDevice->SendCommand(apiStartStop);
						}
*/
						((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_UPDATE_STATUS, TRUE, (LPARAM)m_TaskElapsedTime );
						m_TaskElapsedTime = 0;
						m_OpsRunning = false;

						resStr.LoadString(IDS_PORTMGRDLG_COMPLETE);  // "Operations Complete"
						m_port_status_ctrl.SetWindowText(resStr);
						m_port_progress_ctrl.SetPos((int)m_duration);
						m_port_progress_ctrl.SendMessage(PBM_SETBARCOLOR, 0, PROGRESS_RGB_GREEN);
						if (m_hComPort != INVALID_HANDLE_VALUE)
						{
							LPTSTR token = _T("OK\r\n");
							DWORD dwBytesWritten = (DWORD)_tcslen(token);
							if (!WriteFile (m_hComPort, token,dwBytesWritten,&dwBytesWritten , NULL))
							{
								ATLTRACE("Panel %c - Failed to write OK to COM%d rc=0x%x\n", _T('A')+m_port_display_index, m_port_display_index+1, GetLastError());
							}
							else
								ATLTRACE("Panel %c - Wrote OK to COM%d\n", _T('A')+m_port_display_index, m_port_display_index+1);

						}
					}
					else if (_msg_string == COperation::OPSTATUS_ERROR)
					{

						((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_UPDATE_STATUS, FALSE, (LPARAM)m_TaskElapsedTime );
						m_TaskElapsedTime = 0;

						m_op_error = true;
						m_OpsRunning = false;
						m_port_progress_ctrl.SetPos((int)m_duration);
						m_port_progress_ctrl.SendMessage(PBM_SETBARCOLOR, 0,PROGRESS_RGB_RED);
						if (m_hComPort != INVALID_HANDLE_VALUE)
						{
							LPTSTR token = _T("NG\r\n");
							DWORD dwBytesWritten = (DWORD)_tcslen(token);
							if (!WriteFile (m_hComPort, token,dwBytesWritten,&dwBytesWritten , NULL))
							{
								ATLTRACE("Panel %c - Failed to write NG to COM%d rc=0x%x\n", _T('A')+m_port_display_index, m_port_display_index+1, GetLastError());
							}
							else
								ATLTRACE("Panel %c - Wrote NG to COM%d\n", _T('A')+m_port_display_index, m_port_display_index+1);

						}

					}
					else if (_msg_string == COperation::OPSTATUS_ABORTED)
					{
						m_TaskElapsedTime = 0;

						m_op_error = false;
						m_OpsRunning = false;
						m_port_progress_ctrl.SetPos((int)0);
						m_port_progress_ctrl.SendMessage(PBM_SETBARCOLOR, 0,PROGRESS_RGB_BLUE);
						if (m_hComPort != INVALID_HANDLE_VALUE)
						{
							LPTSTR token = _T("NG\r\n");
							DWORD dwBytesWritten = (DWORD)_tcslen(token);
							if (!WriteFile (m_hComPort, token,dwBytesWritten,&dwBytesWritten , NULL))
							{
								ATLTRACE("Panel %c - Failed to write NG (Abort) to COM%d rc=0x%x\n", _T('A')+m_port_display_index, m_port_display_index+1, GetLastError());
							}
							else
								ATLTRACE("Panel %c - Wrote NG to COM%d\n", _T('A')+m_port_display_index, m_port_display_index+1);

						}

					}
				}
				else
				{
					if (m_mode == OPMODE_RUNNING)
					{
						if (_msg_string != COperation::OPSTATUS_ABORTED)
							((CMainFrame*)theApp.GetMainWnd())->PostMessage(WM_MSG_UPDATE_STATUS, (_msg_string == COperation::OPSTATUS_SUCCESS), (LPARAM)m_TaskElapsedTime );
						m_TaskElapsedTime = 0;
					}
					m_mode = ProcessOps();
				}
			}
			break;
		case PD_EVNT_START_NOTIFY:
			if (m_hComPort != INVALID_HANDLE_VALUE && !m_OpsRunning)
			{
				LPTSTR token = _T("ST\r\n");
				DWORD dwBytesWritten = (DWORD)_tcslen(token);
				if (!WriteFile (m_hComPort, token, dwBytesWritten, &dwBytesWritten , NULL))
				{
					ATLTRACE("Panel %c - Failed to write ST to COM%d rc=0x%x\n", _T('A')+m_port_display_index, m_port_display_index+1, GetLastError());
				}
				else
					ATLTRACE("Panel %c - Wrote ST to COM%d\n", _T('A')+m_port_display_index, m_port_display_index+1);
			}
			m_OpsRunning = TRUE;
			break;
		default:
			ASSERT(0);// should never get here
	}
	return true;
}

bool CPortMgrDlg::OnDeviceChangeNotify(const DeviceClass::NotifyStruct& nsInfo)
{
	ATLTRACE(_T("%s PortMgr DevChange Event: %s DevType: %s OpMgr Mode: %s\r\n"),GetPanel(), DeviceManager::EventToString(nsInfo.Event), DeviceClass::DeviceTypeToString(nsInfo.Type).c_str(), OpModeToString(m_mode));
	bool ret = TRUE;

	switch(nsInfo.Event)
	{
		case DeviceManager::DEVICE_ARRIVAL_EVT:
			if ( m_mode != OPMODE_ALL_OPS_COMPLETE )
				ret = m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_DEVICE_ARRIVAL, 0);
			else
				m_mode = ProcessOps();
			break;

		case DeviceManager::VOLUME_ARRIVAL_EVT:
			if ( m_mode != OPMODE_ALL_OPS_COMPLETE )
			{
				ret = m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_VOLUME_ARRIVAL, 0);
				ret = m_p_curr_op->PumpMessage();
			}
			else
				m_port_drive_ctrl.SetWindowText(m_p_usb_port->GetDriveLetters());
			break;

		case DeviceManager::DEVICE_REMOVAL_EVT:
			if ( m_mode != OPMODE_ALL_OPS_COMPLETE )
			{
				ret = m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_DEVICE_REMOVAL, 0);
				m_p_curr_op->PumpMessage();
			}
			else if (!m_op_error)
				m_mode = ProcessOps();
			break;

		case DeviceManager::VOLUME_REMOVAL_EVT:
			if ( m_mode != OPMODE_ALL_OPS_COMPLETE )
			{
				ret = m_p_curr_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_VOLUME_REMOVAL, 0);
				m_p_curr_op->PumpMessage();
			}
			else
				m_mode = ProcessOps();
//				m_port_drive_ctrl.SetWindowText(m_p_usb_port->GetDriveLetters());
			break;
		case DeviceManager::HUB_ARRIVAL_EVT:
		case DeviceManager::HUB_REMOVAL_EVT:
			// BAD BAD BAD BAD BAD
			// should have gotten a stop msg before we get this message
			ASSERT ( m_start == FALSE );
			// I REALLY don't want to get here. I think we should be getting an 
			// PD_EVNT_CONFIG_CHANGE msg instead, and make sure that we have a port
			// before we (re)Create the ops.
			
			// see if we are still assigned
			// if this changed we better tell the ops
			if ( FindDlgPort() == NULL )
			{
				// ret = DeviceManager::retUnregisterCallback;
				ret = 0; // will be !'d at return
				SetUsbPort( NULL );
			}
			break;

		default:
			;// Process other WM_DEVICECHANGE notifications for other 
			;// devices or reasons.
	}
	return !ret; // 0 means success
}

BOOL CPortMgrDlg::FitsProfile(CString dev_path)
{
	BOOL ret = FALSE;
	if ( m_p_profile && m_p_profile->IsValid() ) {
		dev_path.MakeUpper();
		if ((dev_path.Find(m_p_profile->GetUsbVid().MakeUpper(), 0) != -1) &&
			(dev_path.Find(m_p_profile->GetUsbPid().MakeUpper(), 0) != -1))
			ret = TRUE;
	}
	return ret;
}

void CPortMgrDlg::SetUsbPort(usb::Port* _port)
{
	CString panel_text;

	if (_port) {
		m_p_usb_port = _port;
		m_usb_hub_name = _port->_parentHub->_path.get();
		m_usb_hub_index = _port->_index.get();
		// save registry record
		AfxGetApp()->WriteProfileString(m_label, _T("Hub Name"), m_usb_hub_name);
		AfxGetApp()->WriteProfileInt(m_label, _T("Hub Index"), (int)m_usb_hub_index);
		
		// register for device chnage callbacks on this hub/port
		//
		// First unregister if we are already registered
		if ( m_hDevChangeCallback )
		{
			gDeviceManager::Instance().Unregister(m_hDevChangeCallback);
			m_hDevChangeCallback = NULL;
		}
		// Create a DeviceManager::DeviceChangeCallback to objectize the 
		// callback member function. In this example, the Functor 'cmd'
		// is created to objectize CDeviceApiDlg::OnDeviceChangeNotify().
		DeviceManager::DeviceChangeCallback cmd(this, &CPortMgrDlg::OnDeviceChangeNotify);
		//
		// Create an Observer object, fill in the desired criteria, and
		// then pass it to Register(const Observer& observer)
		//
		DeviceManager::Observer callMe;
		callMe.NotifyFn = cmd;
		callMe.NotifyType = DeviceManager::ByPort;
		callMe.Hub = m_usb_hub_name;
		callMe.HubIndex = m_usb_hub_index;
		m_hDevChangeCallback = gDeviceManager::Instance().Register(callMe);

		// update my gui
		m_port_group_ctrl.EnableWindow(TRUE);
		m_port_drive_ctrl.EnableWindow(TRUE);
		m_port_drive_text_ctrl.EnableWindow(TRUE);
		m_port_status_ctrl.EnableWindow(TRUE);
		m_port_progress_ctrl.EnableWindow(TRUE);
		// increment the usb configmrg enabled port count
//		if ( theApp.GetConfigMgr()->AddRefPort() == 0 ) {
			// we didn't get a port from the config mgr
			// so we need to disable ourself.
			// Setting m_p_usb_port = NULL prevents the
			// configmgr from decrementing its count
			//ASSERT(0);
//			m_p_usb_port = NULL;
//			SetUsbPort(NULL);
//			return;
//		}

		VERIFY(CreateMonitor());

//clw		m_dev_path = m_p_usb_port->GetDevicePath();

		m_port_drive_ctrl.SetWindowText(m_p_usb_port->GetDriveLetters());
		panel_text.LoadString(IDS_PORTMGRDLG_MONITORING);
		m_port_operation_text_ctrl.SetWindowText(panel_text);
		
		panel_text.Format( IDS_PORTMGRDLG_HUB_PORT_TEXT, _T('A')+m_port_display_index, 
			_port->_parentHub->_index.get(),
			_port->_index.get() );
		
		m_port_group_ctrl.SetWindowText(panel_text);
		// todo: clw init the device list if there is a device connected
//clw		if ( !m_dev_path.IsEmpty() ) {
//clw			if ( FitsProfile(m_dev_path) )
//clw				m_dev_list.AddTail(m_p_usb_port->GetDriverKeyName());
//clw		}
	}
	else {
		// if this port was assigned, decrement the 
		// usb configmrg enabled port count
		if ( m_p_usb_port )
		{
//			theApp.GetConfigMgr()->ReleasePort();
			m_p_usb_port = NULL;
		}
		m_usb_hub_name.Empty();
		m_usb_hub_index = -1;
		// save registry record
		AfxGetApp()->WriteProfileString(m_label, _T("Hub Name"), m_usb_hub_name);
		AfxGetApp()->WriteProfileInt(m_label, _T("Hub Index"), (int)m_usb_hub_index);
		// Unregister with gDeviceManager for callbacks on this port
///c		if ( m_hDevChangeCallback )
///c		{
///c			gDeviceManager::Instance().Unregister(m_hDevChangeCallback);
///c			m_hDevChangeCallback = NULL;
///c		}
		// update my gui
		panel_text.Format(IDS_PORTMGRDLG_PANEL_TEXT, _T('A')+m_port_display_index);
		m_port_group_ctrl.SetWindowText(panel_text);
		m_port_group_ctrl.EnableWindow(FALSE);
		m_port_drive_ctrl.EnableWindow(FALSE);
		m_port_drive_text_ctrl.EnableWindow(FALSE);
		m_port_operation_text_ctrl.SetWindowText(_T(""));
		m_port_status_ctrl.EnableWindow(FALSE);
		m_port_progress_ctrl.EnableWindow(FALSE);

		if ( m_p_monitor_op ) {
			ASSERT ( m_p_monitor_op == m_p_curr_op );
            m_p_monitor_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_KILL, 0);
			m_p_monitor_op = m_p_curr_op = NULL;
		}

//clw		m_dev_path.Empty();
		m_port_drive_ctrl.SetWindowText(_T(""));
		m_port_status_ctrl.SetWindowText(_T(""));

		// todo: clw delete the device list??
	}
//	m_port_progress_ctrl.SetPos(0);
}

usb::Port * CPortMgrDlg::FindDlgPort(void)
{
	// initialize the return value
	usb::Port * pPort = NULL;

	// m_p_usb_port should always be NULL here
//	if ( m_p_usb_port ) {
//		theApp.GetConfigMgr()->ReleasePort();
//		m_p_usb_port = NULL;
//	}

	// check the search criteria
	if ( m_usb_hub_name.IsEmpty() || m_usb_hub_index == -1 ) {
		return pPort;
	}

	// Get the usbHubMgr
	usb::HubMgr* pHubMgr = dynamic_cast<usb::HubMgr*>(gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbHub]);
	assert( pHubMgr );

	// See if our hub is in the gDeviceManagers list of USB Hubs
	usb::Hub* pHub = pHubMgr->FindHubByPath(m_usb_hub_name);
	if ( pHub )
	{
		pPort = pHub->Port(m_usb_hub_index);
	}

	return pPort;
}

void CPortMgrDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CDialogBar::OnRButtonDown(nFlags, point);
}

// gradient defines (if not already defined)
#ifndef COLOR_GRADIENTACTIVECAPTION
#define COLOR_GRADIENTACTIVECAPTION     27
#define COLOR_GRADIENTINACTIVECAPTION   28
#define SPI_GETGRADIENTCAPTIONS         0x1008
#endif

void CPortMgrDlg::OnNcPaint()
{
	// TODO: Add your message handler code here
	// Do not call CDialogBar::OnNcPaint() for painting messages
    // get window DC that is clipped to the non-client area
    CWindowDC dc(this);
	dc.SetBkColor(RGBDEF_INCOMPLETE);
//void CSizingControlBarCF::NcPaintGripper(CDC* pDC, CRect rcClient)
//{
//    if (!HasGripper())
//        return;

    // compute the caption rectangle
    BOOL bHorz = FALSE; //IsHorzDocked();
    CRect rcGrip; GetClientRect(&rcGrip);
//    CRect rcBtn = m_biHide.GetRect();
    if (bHorz)
    {   // right side gripper
        rcGrip.left -= 12 + 1;
        rcGrip.right = rcGrip.left + 11;
//        rcGrip.top = rcBtn.bottom + 3;
    }
    else
    {   // gripper at top
        rcGrip.top -= 12 + 1;
        rcGrip.bottom = rcGrip.top + 11;
//clw        rcGrip.right = rcBtn.left - 3;
    }
    rcGrip.InflateRect(bHorz ? 1 : 0, bHorz ? 0 : 1);

    // draw the caption background
    //CBrush br;
    COLORREF clrCptn = 1/*m_bActive*/ ?
        ::GetSysColor(COLOR_ACTIVECAPTION) :
        ::GetSysColor(COLOR_INACTIVECAPTION);

    // query gradient info (usually TRUE for Win98/Win2k)
    BOOL bGradient = FALSE;
    ::SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &bGradient, 0);
    
    if (!bGradient)
        dc.FillSolidRect(&rcGrip, clrCptn); // solid color
    else
    {
        // gradient from left to right or from bottom to top
        // get second gradient color (the right end)
        COLORREF clrCptnRight =1/* m_bActive*/ ?
            ::GetSysColor(COLOR_GRADIENTACTIVECAPTION) :
            ::GetSysColor(COLOR_GRADIENTINACTIVECAPTION);

        // this will make 2^6 = 64 fountain steps
        int nShift = 6;
        int nSteps = 1 << nShift;

        for (int i = 0; i < nSteps; i++)
        {
            // do a little alpha blending
            int nR = (GetRValue(clrCptn) * (nSteps - i) +
                      GetRValue(clrCptnRight) * i) >> nShift;
            int nG = (GetGValue(clrCptn) * (nSteps - i) +
                      GetGValue(clrCptnRight) * i) >> nShift;
            int nB = (GetBValue(clrCptn) * (nSteps - i) +
                      GetBValue(clrCptnRight) * i) >> nShift;

            COLORREF cr = RGB(nR, nG, nB);

            // then paint with the resulting color
            CRect r2 = rcGrip;
            if (bHorz)
            {
                r2.bottom = rcGrip.bottom - 
                    ((i * rcGrip.Height()) >> nShift);
                r2.top = rcGrip.bottom - 
                    (((i + 1) * rcGrip.Height()) >> nShift);
                if (r2.Height() > 0)
                    dc.FillSolidRect(r2, cr);
            }
            else
            {
                r2.left = rcGrip.left + 
                    ((i * rcGrip.Width()) >> nShift);
                r2.right = rcGrip.left + 
                    (((i + 1) * rcGrip.Width()) >> nShift);
                if (r2.Width() > 0)
                    dc.FillSolidRect(r2, cr);
            }
        }
    }
}

HBRUSH CPortMgrDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
//	HBRUSH hbr = CDialogBar::OnCtlColor(pDC, pWnd, nCtlColor);

	switch ( m_mode )
	{
		case OPMODE_INVALID:
			m_br_background.DeleteObject();
			m_br_background.CreateSolidBrush(RGBDEF_INVALID);
			pDC->SetBkColor(RGBDEF_INVALID);
			break;
		case OPMODE_RUNNING:
			m_br_background.DeleteObject();
			m_br_background.CreateSolidBrush(RGBDEF_INCOMPLETE);
			pDC->SetBkColor(RGBDEF_INCOMPLETE);
			break;
		case OPMODE_ALL_OPS_COMPLETE:
			m_br_background.DeleteObject();
			m_br_background.CreateSolidBrush(RGBDEF_VALID);
			pDC->SetBkColor(RGBDEF_VALID);
			break;
		case OPMODE_MONITOR:
		default:
			return CDialogBar::OnCtlColor(pDC, pWnd, nCtlColor);
	}
	return (HBRUSH)m_br_background;
}
/*
POSITION CPortMgrDlg::CDevList::AddTail( LPCTSTR newElement )
{
	POSITION pos = 0;
	CString str = newElement; str.MakeUpper();
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	CPortMgrDlg::CDevList::AddTail() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		if ( m_list.Find(str) == NULL ) {
			pos = m_list.AddTail(str);
			ATLTRACE(_T("Panel %c - CDevList::AddTail() - %s.\n"), _T('A')+m_parent->m_port_display_index, str);
		}
		sLock.Unlock();
	}
	else {
		ATLTRACE(_T("ERROR: CPortMgrDlg::CDevList::AddTail() - Could not get lock."));
		ASSERT(0);
	}
	return pos;
}

POSITION CPortMgrDlg::CDevList::Find( LPCTSTR searchValue, POSITION startAfter )
{
	POSITION pos = 0;
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	CPortMgrDlg::CDevList::Find() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		pos = m_list.Find(searchValue, startAfter);
		sLock.Unlock();
	}
	else {
		ATLTRACE(_T("ERROR: CPortMgrDlg::CDevList::Find() - Could not get lock."));
		ASSERT(0);
	}
	return pos;
}

CString CPortMgrDlg::CDevList::GetTail(void)
{
	CString str;
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	CPortMgrDlg::CDevList::GetTail() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		str = m_list.GetTail();
		sLock.Unlock();
	}
	else {
		ATLTRACE(_T("ERROR: CPortMgrDlg::CDevList::GetTail() - Could not get lock."));
		ASSERT(0);
	}
	return str;
}
CString CPortMgrDlg::CDevList::RemoveTail(void)
{
	CString str;
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	CPortMgrDlg::CDevList::RemoveTail() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		str = m_list.RemoveTail();
		sLock.Unlock();
	}
	else {
		ATLTRACE(_T("ERROR: CPortMgrDlg::CDevList::RemoveTail() - Could not get lock."));
		ASSERT(0);
	}
	return str;
}
INT_PTR CPortMgrDlg::CDevList::GetCount(void)
{
	INT_PTR num = 0;
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	CPortMgrDlg::CDevList::GetCount() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		num = m_list.GetCount();
		sLock.Unlock();
	}
	else {
		ATLTRACE(_T("ERROR: CPortMgrDlg::CDevList::GetCount() - Could not get lock."));
		ASSERT(0);
	}
	return num;
}
*/
void CPortMgrDlg::KillMonitor()
{
	if (m_p_monitor_op) {
        m_p_monitor_op->PostThreadMessage(WM_MSG_OPEVENT, COperation::OPEVENT_KILL, 0);
			m_p_monitor_op = NULL;

	}
}

void CPortMgrDlg::CloseDlg()
{
	OnDestroy();
}

void CPortMgrDlg::OnDestroy()
{
    CDialogBar::OnDestroy();

	// delete the rest of the ops
	DeleteOps();
}

void CPortMgrDlg::Localize()
{
	CString resStr;

	resStr.LoadString(IDS_PORTMGRDLG_DRIVES_TEXT);
	SetDlgItemText(IDC_PORT_DRIVE_TEXT, resStr);  		

	resStr.LoadString(IDS_PORTMGRDLG_PORT_GRP_TEXT);
	SetDlgItemText(ID_PORT_GROUP_BOX, resStr);  

//	resStr.LoadString(IDS_PORTMGRDLG_CLEAR_ERR_TEXT);
//	SetDlgItemText(IDC_CLEAR_ERROR_BUTTON, resStr);  	

}

void CPortMgrDlg::OpCompleteTick(DWORD _dwElapsedTime)
{
	m_TaskElapsedTime += _dwElapsedTime;
}
