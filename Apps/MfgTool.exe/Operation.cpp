// Operation.cpp : implementation file
//

#include "StdAfx.h"
#include "StMfgTool.h"
#include "PlayerProfile.h"
#include "OpInfo.h"
#include "Operation.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(COperation, CWinThread)

const PTCHAR COperation::OperationStrings[]     = { _T("UPDATE"), 
											    	_T("COPY"), 
												    _T("LOAD"),
													_T("OTP"),
													_T("UTP_UPDATE"),
													_T("INVALID OPERATION"),
												    _T("ERASE"), 
												    _T("REGISTRY SCRUB") };


COperation::COperation(CPortMgrDlg *pPortMgrDlg, usb::Port *pUSBPort, COpInfo *pOpInfo, OpTypes type )
{
	m_pPortMgrDlg = pPortMgrDlg;
	m_pUSBPort = pUSBPort;
	m_pOpInfo = pOpInfo;
	m_OpType = type;
	//	m_part = 0;
//	if (GetType() != MONITOR_OP)
	if ( pOpInfo )
	{
		m_pProfile = pOpInfo->GetProfile();
		m_TimeOutSeconds = pOpInfo->GetTimeout();
		if ( m_TimeOutSeconds < 2 )
			m_TimeOutSeconds = 0;
	}
	else
	{
		m_pProfile = NULL;
		m_TimeOutSeconds = 0;
	}
//	m_DeviceState = DISCONNECTED;
	m_bStart = false;
	if ( m_TimeOutSeconds < 2 ) // don't let timer start if the timeout is less than 2 seconds.
		m_Timer = 1;
	else
		m_Timer = 0;

	m_iDuration = 0;
	m_iPercentComplete = 0;

//	m_sStatus = _T("");
//	m_sDrive = _T("");
//	m_sDevicePath = _T("");
	m_sDescription = _T("");
}

COperation::~COperation(void)
{
	if ( m_Timer && m_TimeOutSeconds )
	{
		m_pPortMgrDlg->KillTimer(m_Timer);
		m_Timer = 0;
	}
}

BEGIN_MESSAGE_MAP(COperation, CWinThread)
END_MESSAGE_MAP()
/*
const COperation::OpTypes COperation::GetType() const
{
	ASSERT ( m_pOpInfo );
	return m_pOpInfo->GetType();
}
*/
const COperation::DeviceState_t COperation::GetDeviceState() const
{
	DeviceState_t devState = DISCONNECTED;

	switch ( (DeviceClass::DeviceType)m_pUSBPort->GetDeviceType() )
	{
		case DeviceClass::DeviceTypeHid:
			devState = HID_MODE;
			break;
		case DeviceClass::DeviceTypeRecovery:
			devState = RECOVERY_MODE;
			break;
		case DeviceClass::DeviceTypeMsc:
		{
			CString devPath = m_pUSBPort->GetUsbDevicePath();
			CString csVidPid;

			devPath.MakeUpper();
			csVidPid.Format(_T("Vid_%s&Pid_%s"), m_pProfile->GetUsbVid(), m_pProfile->GetUsbPid());
			csVidPid.MakeUpper();

			if ( m_pProfile &&
				 (devPath.Find(csVidPid) != -1) )		// profile ids
			{
				devState = MSC_MODE;
			}
			else if ( (devPath.Find(SIGMATEL_VID, 0) != -1) &&				// updater.sb ids
		              (devPath.Find(SIGMATEL_UPDATER_MSC_PID, 0) != -1) )
			{
				devState = UPDATER_MODE;
			}
			else if ( (devPath.Find(SIGMATEL_VID, 0) != -1) &&				// stmfgmsc.sb ids
		              ((devPath.Find(SIGMATEL_MFG_MSC_PID, 0) != -1) ||
					  (devPath.Find(_T("PID_37FF"), 0) != -1)) )
			{
				devState = MFG_MSC_MODE;
			}
			else															// something we don't care about
			{																// should have been filtered out anyways
				devState = CONNECTED_UNKNOWN;
			}
			break;
		}
		case DeviceClass::DeviceTypeMtp:
			devState = MTP_MODE;
			break;
		case DeviceClass::DeviceTypeNone:
			devState = DISCONNECTED;
			break;
		default:
			devState = CONNECTED_UNKNOWN;
			break;
	}

//	ATLTRACE(_T("GetDeviceState: %s \r\n"), GetDeviceStateString(devState));

	return devState;
}


CString COperation::GetDeviceSerialNumber(Device* pDevice) const
{
	CString serialNumber = _T("");
	
	if ( pDevice->UsbDevice() == NULL )
		return serialNumber;

	CString devPath = pDevice->UsbDevice()->_path.get();
	devPath.MakeUpper();
	
	if ( (devPath.Find(SIGMATEL_VID) != -1) &&
		((devPath.Find(SIGMATEL_37XX_UPDATER_PID) != -1) ||
	     (devPath.Find(SIGMATEL_MFG_MSC_PID) != -1)) ) {
		// if it our special stmfgmsc.sb firmware, 
		// send a SCSI command to get the real serial number.
		StGetChipSerialNumberInfo apiSerialNumberSize(SerialNoInfoSizeOfSerialNumberInBytes);
		if ( pDevice->SendCommand(apiSerialNumberSize) == ERROR_SUCCESS )
		{
			StGetChipSerialNumberInfo apiSerialNumber(SerialNoInfoSerialNumber, apiSerialNumberSize.GetSizeOfSerialNoInBytes());
			if ( pDevice->SendCommand(apiSerialNumber) == ERROR_SUCCESS )
			{
				serialNumber = apiSerialNumber.ResponseString();
			}
		}
	}
	else {
		// if it not our special stmfgmsc.sb firmware,
		// the SCSI command will not be supported, and it is unneccessary.
		// get the serial number from the device path.
		int pos = 0;
		devPath.Tokenize(_T("#"), pos); devPath.Tokenize(_T("#"), pos);
		serialNumber.Format(_T("%s"), devPath.Tokenize(_T("#"), pos).MakeUpper());
	}

	return serialNumber;
}


CString COperation::GetDeviceStateString(DeviceState_t _state) const
{
	CString str;
	
	switch (_state)
	{
		case DISCONNECTED:
			str = "DISCONNECTED";
			break;
		case CONNECTED_UNKNOWN:
			str = "CONNECTED_UNKNOWN";
			break;
		case HID_MODE:
			str = "HID_MODE";
			break;
		case RECOVERY_MODE:
			str = "RECOVERY_MODE";
			break;
		case UPDATER_MODE:
			str = "UPDATER_MODE";
			break;
		case MFG_MSC_MODE:
			str = "MFG_MSC_MODE";
			break;
		case MSC_MODE:
			str = "MSC_MODE";
			break;
		case MTP_MODE:
			str = "MTP_MODE";
			break;
		default:
			str = "UNKNOWN_DEVICE_MODE";
			break;
	}

	return str;
}

CString COperation::GetEventString(WPARAM _event) const
{
	CString str;

	switch (_event)
	{
		case OPEVENT_START:
			str = "Start";
			break;
		case OPEVENT_STOP:
			str = "Stop";
			break;
		case OPEVENT_DEVICE_ARRIVAL:
			str = "DeviceArrival";
			break;
		case OPEVENT_DEVICE_REMOVAL:
			str = "DeviceRemoval";
			break;
		case OPEVENT_VOLUME_ARRIVAL:
			str = "VolumeArrival";
			break;
		case OPEVENT_VOLUME_REMOVAL:
			str = "VolumeRemoval";
			break;
		case OPEVENT_WKR_THREAD_COMPLETE:
			str = "ThreadComplete";
			break;
		case OPEVENT_KILL:
			str = "Kill"; 
			break;
		default:
			str = "UnknownEvent";
			break;
	}
	return str;
}

