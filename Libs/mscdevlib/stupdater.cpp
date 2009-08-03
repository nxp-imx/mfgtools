// StUpdater.cpp: implementation of the CStUpdater class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "StHeader.h"
#include "stglobals.h"
#include "ddildl_defs.h"
#include "StConfigInfo.h"
#include "StVersionInfo.h"
#include "StProgress.h"
#include "StUpdater.h"
#include "StByteArray.h"
#include <ntddscsi.h>
#include "stddiapi.h"

#include <scsidefs.h>
#include <wnaspi32.h>
//#include <winioctl.h>

#include "StScsi.h"
#include "StScsi_Nt.h"
#include "StFwComponent.h"
#include "StDataDrive.h"
#include "StSystemDrive.h"
#include "StHiddenDataDrive.h"
#include "StRecoveryDev.h"
#include "StHidDevice.h"
#include "StUsbMscDev.h"
#include <wmsdk.h>
#include <mswmdm.h>
#include "StMtpdevice.h"
#include <assert.h>

#define ONE_MB  (1024*1024)

USHORT forcedROMId = 0;
extern CStGlobals g_globals;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStUpdater::CStUpdater(CStConfigInfo *_p_config_info, string _name):CStBase(_name)
{

	m_p_base_cmdlineprocessor = NULL;
	m_p_progress = NULL;
	m_p_base_resource = NULL;
	m_p_config_info = _p_config_info;
	m_p_usbmsc_dev = NULL;
    m_p_mtpdev = NULL;
	m_p_recovery_dev = NULL;
	m_p_hid_dev = NULL;
    m_DeviceMode = NoActiveDeviceMode;
	m_p_arr_ver_info_upgrade = NULL;
    m_langid = GetUserDefaultLangID(); //LANG_NEUTRAL;
	m_p_error = new CStError();
	if( !m_p_error )
	{
		m_last_error = STERR_NO_MEMORY;
	}
}

CStUpdater::~CStUpdater()
{
	if( m_p_error )
	{
		delete m_p_error;
	}

    // close any device classes allocated
    CloseDevice();

	if( m_p_arr_ver_info_upgrade )
	{
		delete m_p_arr_ver_info_upgrade;
		m_p_arr_ver_info_upgrade = NULL;
	}
}

void CStUpdater::SetResource( CStBaseToResource *_p_base_resource )
{
	m_p_base_resource = _p_base_resource;
}

void CStUpdater::SetCmdLineProcessor( CStBaseToCmdLineProcessor* _p_base_cmdlineprocessor )
{
	m_p_base_cmdlineprocessor = _p_base_cmdlineprocessor;
}

void CStUpdater::SetLogger( CStBaseToLogger* _p_base_logger )
{
	m_p_base_logger = _p_base_logger;
}

CStConfigInfo* CStUpdater::GetConfigInfo()
{
	return m_p_config_info;
}

void CStUpdater::SetConfigInfo(CStConfigInfo* ptr) 
{
	m_p_config_info = ptr;
}


ST_ERROR CStUpdater::FindDevice(FIND_DEVICE_TYPE deviceWeAreLookingFor)
 {
     ST_ERROR err = STERR_NONE;

     if ( ( deviceWeAreLookingFor == FIND_ANY_DEVICE  || deviceWeAreLookingFor == FIND_MTP_DEVICE )&& 
		 g_globals.m_bMTPSupported == TRUE )
     {
        // Check for an MTP enumerated device
        if ((err = FindMTPDevice()) == STERR_NONE)
        {
            if ( m_DeviceMode != MTPDeviceMode)
                CloseDevice();
            else
			{
				GetLogger()->Log( _T("Found MTP device.") );
                return err; // we have found our MTP device
			}
        }
     }

    // Look for a device in MSC mode
    if (deviceWeAreLookingFor == FIND_ANY_DEVICE ||
        deviceWeAreLookingFor == FIND_MSC_DEVICE ||
        deviceWeAreLookingFor == FIND_UPD_DEVICE)
    {
        err = FindMSCDevice(deviceWeAreLookingFor);

        if ( ( err == STERR_NONE || err == STERR_MEDIA_STATE_UNINITIALIZED || err == STERR_FAILED_TO_OPEN_FILE) )
		{
			return err;  // we have found our MSC device
		}
    }

    if (deviceWeAreLookingFor == FIND_REC_DEVICE || err != STERR_NONE)
    {   // Finally, look for a device in recovery mode
        err = FindDeviceToRecover();
    }

    return err;
 }

 void CStUpdater::CloseDevice()
 {
    if (m_p_mtpdev )
    {
        delete (m_p_mtpdev);
        m_p_mtpdev = NULL;
    }

    if (m_p_usbmsc_dev)
    {
        delete (m_p_usbmsc_dev);
        m_p_usbmsc_dev = NULL;
    }

    if( m_p_recovery_dev )
	{
		delete m_p_recovery_dev;
        m_p_recovery_dev = NULL;
	}

    if( m_p_hid_dev )
	{
		delete m_p_hid_dev;
        m_p_hid_dev = NULL;
	}
	
	m_DeviceMode = NoActiveDeviceMode;

 }

BOOL CStUpdater::IsResetToRecoveryRequired()
{
    return ( m_DeviceMode == MTPDeviceMode || 
             m_DeviceMode == LimitedMSCDeviceMode);
}


BOOL CStUpdater::IsDeviceInRecoveryMode()
{
    return ( m_DeviceMode == RecoveryDeviceMode );
}

BOOL CStUpdater::IsDeviceInHidMode()
{
    return ( m_DeviceMode == HidDeviceMode );
}

BOOL CStUpdater::IsDeviceInUpdaterMode()
{
    return ( m_DeviceMode == UpdaterDeviceMode );
}

BOOL CStUpdater::IsDeviceInMtpMode()
{
	return ( m_DeviceMode == MTPDeviceMode );
}

BOOL CStUpdater::IsDeviceInMscMode()
{
	return ( m_DeviceMode == MSCDeviceMode );
}

BOOL CStUpdater::IsDeviceInLimitedMscMode()
{
	return ( m_DeviceMode == LimitedMSCDeviceMode );
}

BOOL CStUpdater::IsDeviceReady()
{
	if ( m_DeviceMode != NoActiveDeviceMode && m_DeviceMode != RecoveryDeviceMode )
		return TRUE;

	return FALSE;
}


BOOL CStUpdater::IsDevicePresent()
{
	if ( m_DeviceMode == NoActiveDeviceMode )
		return FALSE;
	else
		return TRUE;
}


ST_ERROR CStUpdater::ResetForRecovery()
{
	GetLogger()->Log( _T("Trying to Reset To Recovery.") );
	ST_ERROR err = STERR_NONE;

    // reset the device to boot recovery mode
    if (m_p_mtpdev )
    {
        if (m_p_mtpdev->m_pWmdmDevice)
		{
            err = m_p_mtpdev->ResetMtpDeviceForRecovery();
			if (err == STERR_NONE)
				GetLogger()->Log( _T("Sending ResetToRecovery(0x97F3) cmd to MTP device. (PASS)") );
			else
				GetLogger()->Log( _T("Sending ResetToRecovery(0x97F3) cmd to MTP device. (FAIL)") );
		}
    }
    else if (m_p_usbmsc_dev)
    {
        err = m_p_usbmsc_dev->ResetMSCDeviceForRecovery();
		if (err == STERR_NONE)
			GetLogger()->Log( _T("Sending ResetToRecovery(0x42) cmd to MSC device. (PASS)") );
		else
			GetLogger()->Log( _T("Sending ResetToRecovery(0x42) cmd to MSC device. (FAIL)") );
    }

    if ( err == STERR_NONE )
    {
        CloseDevice();
    }
    return err;
}

BOOL CStUpdater::OldResetForRecovery()
{
	GetLogger()->Log( _T("Trying to Erase BootManager and Reset Chip to force Recovery mode on 35xx.") );
    // reset the device to boot recovery mode
    if (m_p_mtpdev )
    {
        if (m_p_mtpdev->m_pWmdmDevice)
		{
			GetLogger()->Log( _T("Sending EraseBootManager(0x97F2) and ChipReset(0x97F1) cmds to MTP device.") );
            m_p_mtpdev->OldResetMtpDeviceForRecovery();
		}
    }
    else
    if (m_p_usbmsc_dev)
    {
		GetLogger()->Log( _T("Sending EraseLogicalDrive(BootManager) and ChipReset cmds to MSC device.") );
        m_p_usbmsc_dev->OldResetMSCDeviceForRecovery();
    }

    CloseDevice();

    return TRUE;
}

ST_ERROR CStUpdater::ResetToMscUpdaterMode()
{
	ST_ERROR err = STERR_FUNCTION_NOT_SUPPORTED;

	if ( GetConfigInfo()->BuiltFor35xx() )
	{
		GetLogger()->Log( _T("Trying to ResetToUpdater mode on 35xx.") );
		// get the upgrade updater.sb component version
		CStVersionInfo updaterNandVersion, updaterUpgradeVersion;
		CHAR updater_index = -1;
		GetConfigInfo()->GetUpdaterDriveIndex(updater_index);
		CStFwComponent* p_fw_component = new CStFwComponent(this, updater_index, GetLanguageId());
		if( p_fw_component->GetLastError() != STERR_NONE)
		{
			return p_fw_component->GetLastError();
		}
		p_fw_component->GetComponentVersion(updaterUpgradeVersion);
		
		// reset the device to boot updater mode
		if (m_p_mtpdev )
		{
			// get the nand updater.sb component version from the mtp device
			CString msg = _T("Sending GetUpdaterNandVersion(0x97F5, drvTag:0xFE) cmd to MTP device.");
			if ( m_p_mtpdev->MtpGetUpdaterNandVersion(updaterNandVersion) == STERR_NONE )
			{
				msg.AppendFormat(_T(" (PASS) ver: %d%.d%.d"), updaterNandVersion.GetHigh(), updaterNandVersion.GetMid(), updaterNandVersion.GetLow());
				GetLogger()->Log( msg );
				if ( updaterUpgradeVersion > updaterNandVersion )
				{
					// if the upgrade version is newer, fail to ResetToUpdater. Hopefully the app will
					// ResetToRecovery instead.
					err = STERR_FUNCTION_NOT_SUPPORTED;
				}
				else
				{
					// updater.sb on the nand is current, so ResetToUpdater
					err = m_p_mtpdev->ResetMtpDeviceToMscUpdaterMode();
					if (err == STERR_NONE)
						GetLogger()->Log( _T("Sending ResetToUpdater(0x97F4) cmd to MTP device. (PASS)") );
					else
						GetLogger()->Log( _T("Sending ResetToUpdater(0x97F4) cmd to MTP device. (FAIL)") );
				}
			}
			else
			{
				msg.Append(_T(" (FAIL)"));
				GetLogger()->Log( msg );
			}
		}
		else if (m_p_usbmsc_dev)
		{
			// get the nand updater.sb component version from the msc device
			CString msg = _T("Sending GetAllocationTable(0x05), GetLogicalDriveInfo(0x12):ComponentVersion cmds to MSC device.");
			if ( m_p_usbmsc_dev->GetUpdaterDriveVersion(updaterNandVersion) == STERR_NONE )
			{
				msg.AppendFormat(_T(" (PASS) ver: %d%.d%.d"), updaterNandVersion.GetHigh(), updaterNandVersion.GetMid(), updaterNandVersion.GetLow());
				GetLogger()->Log( msg );
				if ( updaterUpgradeVersion > updaterNandVersion )
				{
					// if the upgrade version is newer, fail to ResetToUpdater. Hopefully the app will
					// ResetToRecovery instead.
					err = STERR_FUNCTION_NOT_SUPPORTED;
				}
				else
				{
					// updater.sb on the nand is current, so ResetToUpdater
					err = m_p_usbmsc_dev->ResetMscDeviceToMscUpdaterMode();
					if (err == STERR_NONE)
						GetLogger()->Log( _T("Sending ResetToUpdater(0x44) cmd to MSC device. (PASS)") );
					else
						GetLogger()->Log( _T("Sending ResetToUpdater(0x44) cmd to MSC device. (FAIL)") );
				}
			}
			else
			{
				msg.Append(_T("(FAIL)"));
				GetLogger()->Log( msg );
			}
		}
	}
	else
	{
		GetLogger()->Log( _T("Trying to ResetToUpdater mode on 36xx or 37xx.") );
		// reset the device to boot updater mode
		if (m_p_mtpdev )
		{
			// updater.sb on the nand is current, so ResetToUpdater
			err = m_p_mtpdev->ResetMtpDeviceToMscUpdaterMode();
			if (err == STERR_NONE)
				GetLogger()->Log( _T("Sending ResetToUpdater(0x97F4) cmd to MTP device. (PASS)") );
			else
				GetLogger()->Log( _T("Sending ResetToUpdater(0x97F4) cmd to MTP device. (FAIL)") );
		}
		else if (m_p_usbmsc_dev)
		{
			// updater.sb on the nand is current, so ResetToUpdater
			err = m_p_usbmsc_dev->ResetMscDeviceToMscUpdaterMode();
			if (err == STERR_NONE)
				GetLogger()->Log( _T("Sending ResetToUpdater(0x44) cmd to MSC device. (PASS)") );
			else
				GetLogger()->Log( _T("Sending ResetToUpdater(0x44) cmd to MSC device. (FAIL)") );
		}
	}
    
	if ( err == STERR_NONE )
    {
        CloseDevice();
    }
    
	return err;
}

ST_ERROR CStUpdater::InitializeDeviceInterface( BOOL bReadyToUpdate )
{
    ST_ERROR err = STERR_NONE;
    BOOL bDeviceReady = FALSE;
//    BOOL bWaitingForRecovery = FALSE;
    int iWaitingForRecovery = 0;

    BOOL bWaitingForUpdater = FALSE;
    BOOL bRecoverDevice = bReadyToUpdate;
    FIND_DEVICE_TYPE deviceWeAreLookingFor = FIND_ANY_DEVICE;
	

	if ( IsDeviceReady() )
		CloseDevice();

	GetLogger()->Log( _T("Looking for any STMP MSC, MTP, HID, or Recovery device.") );
    FindDevice(deviceWeAreLookingFor);

	GetProgress()->UpdateDeviceMode();

    if ( !bReadyToUpdate && IsDevicePresent() &&
							(   IsDeviceInMtpMode() ||
							    IsDeviceInLimitedMscMode() ||
							  (!IsDeviceInRecoveryMode() && !IsDeviceInHidMode() && GetConfigInfo()->GetForceRecvMode()))  )
    {	
        // If we are in MTP or limited MSC, we deterine if the device can be reset
		// to updater mode (>36xx devices) or not (35xx devices).

		GetProgress()->SetCurrentTask (TASK_TYPE_INIT_DEVICE_INTERFACE, 10);

		if ( GetConfigInfo()->GetForceRecvMode() == FALSE )
			err = ResetToMscUpdaterMode();

		// If the above ResetToMscUpdaterMode failed try resetting to recovery mode.
		// Do nothing, and perform the reset during the update process.
		if ( err != STERR_NONE || GetConfigInfo()->GetForceRecvMode() == TRUE )
        {
            err = ResetForRecovery();
		    // If the above ResetForRecovery failed, we are probably talking to a 35xx device.
		    // Do nothing, and perform the reset during the update process.
		    if ( err != STERR_NONE )
            {
                return STERR_NONE;
            }
            else // expecting a recovery mode device
            {
                // Otherwise, go find the device in recovery mode and get it in MSC updater mode.
				GetLogger()->Log( _T("Waiting for a Player Recovery Class device or a HID device...") );
                deviceWeAreLookingFor = FIND_REC_DEVICE;
//                bWaitingForRecovery = TRUE;
                iWaitingForRecovery = 1;
		        bRecoverDevice = TRUE;
                Sleep (1500);
            }
        }
        else // expecting a UPDATER device
        {
            // Otherwise, go find the device in recovery mode and get it in MSC updater mode.
			// NOTE:(5.4.2007, clw) There is a bug (Stmp00014409) in the hostlink fw where the firmware returns the SCSI IDs 
			// used by hostlink ( SigmaTel/SDK Device ) instead of the update.sb SCSI IDs (_Generic/MSC Recovery)
			GetLogger()->Log( _T("Waiting for a UPDATER MSC device...") );
            deviceWeAreLookingFor = FIND_UPD_DEVICE; // FIND_MSC_DEVICE;
            bWaitingForUpdater = TRUE;
//		    bRecoverDevice = FALSE;
            Sleep (1500);
        }
    }

    if ( IsDeviceInUpdaterMode() || IsDeviceInMscMode() )
	{
		GetLogger()->Log( _T("Found an UPDATER MSC device or a Hostlink MSC device that supports updating cmds.") );
        return STERR_NONE; // good to go
	}

	// So we should now be in (or waiting for) updater mode, recovery mode, MTP, or limited MTP.  The later two only for
	// 35xx devices during the update process.  We need to get into updater mode.

    
    //
    // This loop repeats until a device is found and updated or the user cancels
    //
	while ( !bDeviceReady )
	{
		if ( WaitForSingleObject( m_hStopTrigger, 0 ) != WAIT_TIMEOUT)
			break;  // we've been closed

        err = FindDevice(deviceWeAreLookingFor);

        if( err == STERR_FAILED_TO_OPEN_FILE )
		{
			break;		
		}

		GetProgress()->UpdateDeviceMode();

/* SDK2.6 stuff
//		if ( bWaitingForRecovery && 
		if ( iWaitingForRecovery == 1 && 
			err == STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE )
		{
			// Stupid SDK2.6xx f/w failure to erase bootmgr.
			// Check if we find a MSC device after resetting to RECV,
			// if so, fail it.  The user must manually reset to recovery
			// mode.
			err = FindDevice(FIND_MSC_DEVICE);
			if ( err == STERR_NONE || err == STERR_MEDIA_STATE_UNINITIALIZED )
			{
				// Fail it with special error code
				CloseDevice();
				err = STERR_MANUAL_RECOVERY_REQUIRED;
				GetLogger()->Log( _T("ResetToRecovery Failed. Found Hostlink MSC device when we were expecting a Player Recovery Class device.") );
				return err;
			}

            if ( bReadyToUpdate )
                GetProgress()->UpdateProgress();
			Sleep (1000);
			continue;
		}
*/

		// Added to work around another 3700 ROM problem.  Requires ResetToRecovery again after first
		// attempt to initialize HID device.
		if ( GetConfigInfo()->BuiltFor37xx() && iWaitingForRecovery > 0 && err == STERR_FAILED_TO_FIND_HID_DEVICE )
		{
			Sleep(2000);
			GetLogger()->Log( _T("FAILED ResetToRecovery. Trying again...") );

			err = FindDevice(FIND_MTP_DEVICE);
			if ( ! IsDeviceInMtpMode() )
			{
				err = FindDevice(FIND_MSC_DEVICE);
			}
			
			if ( IsDeviceInMtpMode() || IsDeviceInMscMode() )
			{
				err = ResetForRecovery();
				if ( err == STERR_NONE ) 
				{
					GetLogger()->Log( _T("Waiting for a Player Recovery Class device or a HID device...") );
					deviceWeAreLookingFor = FIND_REC_DEVICE;
					++iWaitingForRecovery;
					bRecoverDevice = TRUE;
	                Sleep (5000);
					continue;  // go back to the top of the loop to find the device
				}
				else
				{
					return err;
				}
			}
//			else
//			{
//				Sleep(1000);
//				continue;
//			}
		}

// End Added

        if ( err != STERR_NONE && err != STERR_MEDIA_STATE_UNINITIALIZED )
        {
			err = STERR_NONE;

//			if (bWaitingForUpdater || bWaitingForRecovery)
			if (bWaitingForUpdater || iWaitingForRecovery > 0)
			{
				continue;
			}
			else
			{
				break; // probably no device connected
			}
        }

//        if ( bWaitingForRecovery && !IsDeviceInRecoveryMode() && !IsDeviceInHidMode() )
        if ( iWaitingForRecovery > 0 && !IsDeviceInRecoveryMode() && !IsDeviceInHidMode() )
        {
            if ( bReadyToUpdate )
                GetProgress()->UpdateProgress();
            CloseDevice();
            continue;
        }

		// NOTE:(5.4.2007, clw) There is a bug (Stmp00014409) in the hostlink fw where the firmware returns the SCSI IDs 
		// used by hostlink ( SigmaTel/SDK Device ) instead of the update.sb SCSI IDs (_Generic/MSC Recovery)
        if ( bWaitingForUpdater && !IsDeviceInUpdaterMode()/*clw !IsDeviceInMscMode*/ )
        {
            if ( bReadyToUpdate )
                GetProgress()->UpdateProgress();
            CloseDevice();
            continue;
        }

        //
        // Check for the device in MTP or hostlink limited MSC mode.
        // Hostlink MSC mode is limited and cannot be used to update the device.
        // If found, cause it to reset into recovery mode if requested by the input parameter.
		//
		// If CFG_AUTO_RECOVERY is set to false, don't bother trying to force 
		// recovery mode. Just let the app keep searching for something it can update.
		//

		GetProgress()->SetCurrentTask (TASK_TYPE_INIT_DEVICE_INTERFACE, 10);

        if ( bRecoverDevice && !IsDeviceInRecoveryMode() && !IsDeviceInHidMode() &&
			(( IsResetToRecoveryRequired() && GetConfigInfo()->AllowAutoRecovery() == TRUE ) ||
			 ( !IsDeviceInUpdaterMode() && GetConfigInfo()->GetForceRecvMode() == TRUE  ) ))
        {
            // No need to warn user since we no longer erase boot mgr
			err = ResetForRecovery();

			// Unless it fails due to a back level version of f/w that doesn't support the single operation
			if ( err == STERR_FUNCTION_NOT_SUPPORTED )
			{
				// now do the old erase bootmgr and reset
				OldResetForRecovery();
			}
			
			GetLogger()->Log( _T("Waiting for a Player Recovery Class device or a HID device...") );
            deviceWeAreLookingFor = FIND_REC_DEVICE;
//            bWaitingForRecovery = TRUE;
            iWaitingForRecovery = 1;
            if ( bReadyToUpdate )
                GetProgress()->UpdateProgress();
            Sleep(1000); // let things settle

			if ( WaitForSingleObject( m_hStopTrigger, 0 ) != WAIT_TIMEOUT)
				continue;  // we've been closed

			if ( bReadyToUpdate )
                GetProgress()->UpdateProgress();
            Sleep(1000); // let things settle

			for(int i=0; i<180; i++) 
			{
				// Wait for upto 3 minutes to find the device in recovery mode
				// Enumeration process for recovery device may take time on a clean machine
				if ( WaitForSingleObject( m_hStopTrigger, 0 ) != WAIT_TIMEOUT)
					break;  // we've been closed

				err = FindDevice(deviceWeAreLookingFor);
				if ( err == STERR_NONE )
					break;
				else
					Sleep(1000);
			}

            continue; // go find it in recovery mode
        }
    		
    	if( IsDeviceInRecoveryMode() || IsDeviceInHidMode() )
	    {
            // We have a device in recovery mode; recover the device (download updater.sb and reset)
		    DWORD active_drives_bitmap=0;
    			
//            bWaitingForRecovery = FALSE;
            iWaitingForRecovery = 0;
		    active_drives_bitmap = ::GetLogicalDrives();

		    err = RecoverDevice();
	        if( err != STERR_NONE )
		    {
		        break;		
    	    }
			GetLogger()->Log( _T("Waitng for up to 2 minutes for a new Drive to mount...") );
    	    WaitForDriveToAppear(active_drives_bitmap, bReadyToUpdate);

            FreeScsiDevice();

            // if there isn't an updater.sb then we are looking for a MSC device.
			CHAR updater_drive_number = -1;
			GetConfigInfo()->GetUpdaterDriveIndex(updater_drive_number);
			if ( updater_drive_number == -1 )
			{
				GetLogger()->Log( _T("Waiting for a Hostlink MSC device because there is no updater.sb...") );
				deviceWeAreLookingFor = FIND_MSC_DEVICE;
			}
			else
			{
				GetLogger()->Log( _T("Waiting for an UPDATER MSC device...") );
				deviceWeAreLookingFor = FIND_UPD_DEVICE;
			}

            // Now go back to the top of the loop and find the device in UPDATE mode (updater.sb).
        }
        else
        {
            bDeviceReady = TRUE;
            continue;
        }

    } // end while !bDeviceReady

    if ( bReadyToUpdate )
	    GetProgress()->SetCurrentTask( TASK_TYPE_NONE, (ULONG)0 );

    if ( bDeviceReady )
	{
		GetLogger()->Log( _T("The device should now be in a mode that supports updating.") );
	}
	else
	{
		if ( err == STERR_NONE )
		{
			GetLogger()->Log(_T("Failed to find STMP device connected."));
		}
		else
		{
			CString logStr;
			logStr.Format(_T("ERROR! Failed to InitializeDeviceInterface(). error: 0x%08X(%d)"), err, err);
			GetLogger()->Log(logStr);
		}
	}

	GetProgress()->UpdateDeviceMode();

	return err;
}


ST_ERROR CStUpdater::FindMTPDevice()
{
    // Find the device in MTP mode
    wchar_t szMfg[128];
    USHORT VendorId, ProductId;

    m_p_config_info->GetMtpMfgString(szMfg,128);
    m_p_config_info->GetUSBProductId(ProductId);
	m_p_config_info->GetUSBVendorId(VendorId);

    if (m_p_mtpdev == NULL)
    {
        m_p_mtpdev = new CStMtpDevice (szMfg, VendorId, ProductId);
        if ( m_p_mtpdev == NULL )
            return STERR_NO_MEMORY;
    }

    if (m_p_mtpdev->m_pWmdmDevice)
        m_DeviceMode = MTPDeviceMode;
	else
	{	// Close device and check for secondary PID, if any.
		CloseDevice();

		m_p_config_info->GetSecondaryUSBProductId(ProductId);
		if (ProductId)
		{
		    m_p_mtpdev = new CStMtpDevice (szMfg, VendorId, ProductId);
			if ( m_p_mtpdev == NULL )
	            return STERR_NO_MEMORY;

		    if (m_p_mtpdev->m_pWmdmDevice)
				m_DeviceMode = MTPDeviceMode;
			else
			{
				CloseDevice();
			}
		}
	}


//    if (m_p_mtpdev->m_pWmdmDevice)
  //  {
    //    MessageBox(NULL, L"Found our device", L"test", MB_OK);
    //}

    return STERR_NONE;
}

ST_ERROR CStUpdater::FindMSCDevice(FIND_DEVICE_TYPE deviceWeAreLookingFor)
{
    ST_ERROR err = STERR_NONE;

	if( m_p_usbmsc_dev != NULL )
    {
        CloseDevice();
    }

    // Find the device in normal MSC mode
	if( m_p_usbmsc_dev == NULL )
	{
		m_p_usbmsc_dev = new CStUsbMscDev( this );
		if( m_p_usbmsc_dev == NULL )
		{
			return STERR_NO_MEMORY;
		}
	}

    if (deviceWeAreLookingFor == FIND_ANY_DEVICE || deviceWeAreLookingFor == FIND_MSC_DEVICE)
    {
    	err = m_p_usbmsc_dev->Initialize(NORMAL_MSC);
        if ( err == STERR_NONE || err == STERR_MEDIA_STATE_UNINITIALIZED || err == STERR_LIMITED_VENDOR_SUPPORT)
        {
            m_p_error->ClearStatus();

            if ( m_p_usbmsc_dev->AreVendorCmdsLimited() )
            {
				GetLogger()->Log( _T("Found Limited MSC device (SDK2.6xx).") );
                m_DeviceMode = LimitedMSCDeviceMode; // enumerated limited hostlink.sb
            }
			else
			{
				GetLogger()->Log( _T("Found Hostlink MSC device.") );
				m_DeviceMode = MSCDeviceMode;
			}
			
			return err;
        }
    }

    if (deviceWeAreLookingFor == FIND_UPD_DEVICE || err != STERR_NONE)
    {
    	err = m_p_usbmsc_dev->Initialize(UPGRADE_MSC);

        if (err == STERR_NONE || err == STERR_MEDIA_STATE_UNINITIALIZED)
        {
            m_p_error->ClearStatus();
 
			GetLogger()->Log( _T("Found Updater MSC device.") );
            m_DeviceMode = UpdaterDeviceMode; // enumerated updater.sb
        }
    }

    if (err != STERR_NONE && err != STERR_MEDIA_STATE_UNINITIALIZED)
    {
		CloseDevice();
    }
    return err;
}


ST_ERROR CStUpdater::FindDeviceToRecover()
{
    ST_ERROR err = STERR_NONE;

	if ( m_p_recovery_dev == NULL && m_p_hid_dev == NULL )
	{
	    // Find the device in recovery mode
		if( m_p_recovery_dev == NULL)
		{
			m_p_recovery_dev = new CStRecoveryDev( this );
			if( m_p_recovery_dev == NULL )
			{
				return STERR_NO_MEMORY;
			}
		}
		
		err = m_p_recovery_dev->Initialize();

		if (err == STERR_NONE)
		{
            GetLogger()->Log( _T("Found Player Recovery Class device.") );
			m_DeviceMode = RecoveryDeviceMode;
		}
		else
			CloseDevice();
		if (err == STERR_FAILED_TO_OPEN_FILE)
			return err;

/*** DISABLE HID interface */
		if ( err != STERR_NONE )
		{
			// Look for HID device
			m_p_hid_dev = new CStHidDevice( this );
			if( m_p_hid_dev == NULL )
			{
				return STERR_NO_MEMORY;
			}

			err = m_p_hid_dev->Initialize();

			if (err == STERR_NONE)
			{
				GetLogger()->Log( _T("Found HID device.") );
				m_DeviceMode = HidDeviceMode;
			}
			else
				CloseDevice();
		}
/***/
	}

    return err;
}


ST_ERROR CStUpdater::IsEraseMediaRequired(BOOL& _erase_media_required)
{
	return m_p_usbmsc_dev->IsEraseMediaRequired(_erase_media_required);
}

ST_ERROR CStUpdater::CheckForFwResources()
{ // Checks for f/w resources and opens them if not open
	return m_p_usbmsc_dev->CheckForFwResources();
}

void CStUpdater::SetTotalTasks(USHORT _operation)
{
    GetProgress()->SetTotalTasks( m_p_usbmsc_dev->GetTotalTasks(_operation) );
}

ST_ERROR CStUpdater::PrepareForUpdate()
{
	return ( m_p_usbmsc_dev->GetReadyToDownload() );
}

ST_ERROR CStUpdater::CompletedUpdate()
{
	return ( m_p_usbmsc_dev->CleanupAfterDownload() );
}

ST_ERROR CStUpdater::SaveDrive(int driveTag, CString fileName)
{
	ST_ERROR err = STERR_NONE;
	CString logText;
	CStByteArray* driveBytes = NULL;
	ULONGLONG driveSize = m_p_usbmsc_dev->GetDriveSize(driveTag);
	if ( driveSize == 0 )
		goto Exit_SaveDrive;

	logText.Format( _T("Saving drive (tag:%02x) to a file."), driveTag);
	GetLogger()->Log(logText);
    
	// Save the settings drive to a file
	driveBytes = new CStByteArray((size_t)driveSize);
	err = m_p_usbmsc_dev->ReadDrive(driveTag, driveBytes);
	if ( err != STERR_NONE )
		goto Exit_SaveDrive;

    HANDLE hFile = CStGlobals::CreateFile(
            fileName,						    // file name
            GENERIC_WRITE,						// dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                               // lpSecurityAttributes
            CREATE_ALWAYS,                      // dwCreationDistribution
            FILE_ATTRIBUTE_NORMAL,              // dwFlagsAndAttributes
            NULL                                // hTemplateFile
    );

	if( hFile != INVALID_HANDLE_VALUE )
    {
        DWORD dwBytesWritten;

		WriteFile(hFile, driveBytes->m_p_t, (DWORD)driveBytes->GetCount(), &dwBytesWritten, NULL);

        CloseHandle (hFile);

        if (dwBytesWritten == driveBytes->GetCount())
            err = STERR_NONE;
    }

Exit_SaveDrive:
	if( driveBytes )
		delete driveBytes;

    return err;
}

ST_ERROR CStUpdater::RestoreDrive(int driveTag, CString fileName)
{
    ST_ERROR err = STERR_FAILED_TO_WRITE_FILE_DATA;
	CString errMsg;
	DWORD dwBytesRead = 0;
	DWORD fileSize = 0;

	// Can't restore if no drive to restore to.
	ULONGLONG driveSize = m_p_usbmsc_dev->GetDriveSize(driveTag);
	if ( driveSize == 0 )
		return STERR_NONE;

	// Can't restore if file isn't present.
	if ( _taccess_s(fileName, 0) != 0 )
		return STERR_NONE;

	if ( (driveTag == DRIVE_TAG_DATA_SETTINGS) && (!m_p_usbmsc_dev->HasSettingsFile()) )
		return STERR_NONE;
	
	errMsg.Format( _T("Restoring drive (tag:%02x) from a file."), driveTag);
	GetLogger()->Log(errMsg);

    HANDLE hFile = CStGlobals::CreateFile(
            fileName,						    // file name
            GENERIC_READ,						// dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                               // lpSecurityAttributes
            OPEN_EXISTING,                      // dwCreationDistribution
            FILE_ATTRIBUTE_NORMAL,              // dwFlagsAndAttributes
            NULL                                // hTemplateFile
    );

	if( hFile != INVALID_HANDLE_VALUE )
    {
		fileSize = ::GetFileSize(hFile, NULL);
		
		if ( fileSize <= driveSize )
		{
			errMsg.Format(_T(" - File(%d) fits on drive(%d). Writing file..."), fileSize, driveSize);
			GetLogger()->Log(errMsg);
			
			// Restore the drive from a file
			CStByteArray* driveBytes = new CStByteArray(fileSize);

			ReadFile(hFile, driveBytes->m_p_t, fileSize, &dwBytesRead, NULL);
	        CloseHandle (hFile);

			if (dwBytesRead == fileSize)
			{
    			err = m_p_usbmsc_dev->WriteDrive(driveTag, driveBytes);
			}

			if( driveBytes )
				delete driveBytes;
		}
		else
		{
			errMsg.Format(_T(" - File(%d) does NOT fit on drive(%d). Skipping restore drive(tag: %02x)."), fileSize, driveSize, driveTag);
			GetLogger()->Log(errMsg);
			err = STERR_NONE;
		}
    }
	else
	{
		errMsg.Format(_T(" - Could NOT open %s. Skipping restore drive(tag: %02x)."), fileName, driveTag); 
		GetLogger()->Log(errMsg);
	}

	// delete the file
	DeleteFile(fileName);

	return err;
}

ST_ERROR CStUpdater::UpdateDevice(USHORT _operation)
{
	ST_ERROR err    = STERR_NONE;
	//PLATFORM pf     = CStGlobals::GetPlatform();

	// called from stupdaterdlg
//    GetProgress()->SetTotalTasks( m_p_usbmsc_dev->GetTotalTasks(_operation) );

	// called from stupdaterdlg
//	if ( (err = m_p_usbmsc_dev->GetReadyToDownload()) != STERR_NONE )
//		return err;

    if ( _operation & UPDATE_ERASE_MEDIA || _operation & UPDATE_REMOVE_DRM )
    {
 		// Save the Settings drive to a file only if there is no settings.bin file shipped with the updater.exe
		if (!m_p_usbmsc_dev->HasSettingsFile())
		{
			if ( (err = SaveDrive(DRIVE_TAG_DATA_SETTINGS, CString(_T("settings.bin")))) != STERR_NONE )
			{
 				GetLogger()->Log( _T("  FAIL") );
    			//return err;
				return STERR_NONE;
			}
		}
		
		// wipe the media clean
		GetProgress()->SetCurrentTask(TASK_TYPE_ERASE_MEDIA, 10);
 		GetLogger()->Log( _T("Erasing entire media INCLUDING the Janus drive.") );
		if ( (err = m_p_usbmsc_dev->EraseLogicalMedia(FALSE)) != STERR_NONE )
        {
 			GetLogger()->Log( _T("  FAIL") );
    	    return err;
        }

 		GetLogger()->Log( _T("Setting Allocation Table.") );
        if ( (err = m_p_usbmsc_dev->SetAllocationTable()) != STERR_NONE)
		{
 			GetLogger()->Log( _T("  FAIL") );
            return err;
		}
    }
	else
	{
        // not a total wipe-out
		if ( _operation & UPDATE_FORMAT_DATA )
		{
 			// Save the Settings drive to a file only if there is no settings.bin file shipped with the updater.exe
			if (!m_p_usbmsc_dev->HasSettingsFile())
			{
				if ( (err = SaveDrive(DRIVE_TAG_DATA_SETTINGS, CString(_T("settings.bin")))) != STERR_NONE )
				{
 					GetLogger()->Log( _T("  FAIL") );
    				//return err;
					return STERR_NONE;
				}
			}
			
			if (m_p_usbmsc_dev->GetProtocolVersion() >= ST_UPDATER_JANUS_7)
			{
				if ( (err = SaveDrive(DRIVE_TAG_DATA_HIDDEN, CString(_T("janus.bin")))) != STERR_NONE )
				{
					GetLogger()->Log( _T("  FAIL") );
					//return err;
					return STERR_NONE;
				}
			}

			if ( _operation & UPDATE_SAVE_HDS )
			{
			    // save HDS 
 				GetLogger()->Log( _T("Saving the HDS.") );
   				if ( (err = m_p_usbmsc_dev->SaveRestoreHDS(SAVE_HDS)) != STERR_NONE )
				{
					GetLogger()->Log( _T("  FAIL") );
					return err;
				}
			}

			// wipe out everything 
			GetProgress()->SetCurrentTask(TASK_TYPE_ERASE_MEDIA, 10);
 			GetLogger()->Log( _T("Erasing entire media INCLUDING the Janus drive.") );
            if ( (err = m_p_usbmsc_dev->EraseLogicalMedia( FALSE )) != STERR_NONE )
			{
				GetLogger()->Log( _T("  FAIL") );
	    	    return err;
			}

 			GetLogger()->Log( _T("Setting Allocation Table.") );
	        if ( (err = m_p_usbmsc_dev->SetAllocationTable()) != STERR_NONE)
			{
				GetLogger()->Log( _T("  FAIL") );
		        return err;
			}
		}
	}

    // DownLoadFirmware erases each logical drive before downloading the
    // firmware image associated with it.
	if ( _operation & UPDATE_FIRMWARE )
		if ( (err = m_p_usbmsc_dev->DownloadFirmware()) != STERR_NONE )
			return err;

	// for the WinCE updater, we're done.
	if ( GetConfigInfo()->IsWinCESolution() )
	{
		return err;
	}

	// if we have not preserved the hidden drive, format it.
    if ( (_operation & UPDATE_ERASE_MEDIA || _operation & UPDATE_REMOVE_DRM) ||
		 (_operation & UPDATE_FORMAT_DATA && m_p_usbmsc_dev->GetProtocolVersion() < ST_UPDATER_JANUS) )
	{
        if ( (err = m_p_usbmsc_dev->FormatHiddenDrive()) != STERR_NONE )
		    return err;
	}

	// Restore the Settings drive from a file
    if ( (_operation & UPDATE_ERASE_MEDIA) || (_operation & UPDATE_REMOVE_DRM) || (_operation & UPDATE_FORMAT_DATA) )
	{
		if ( (err = RestoreDrive(DRIVE_TAG_DATA_SETTINGS, CString(_T("settings.bin")))) != STERR_NONE )
		{
			GetLogger()->Log( _T("  FAIL") );
			return err;
		}

		if ( (err = RestoreDrive(DRIVE_TAG_DATA_HIDDEN, CString(_T("janus.bin")))) != STERR_NONE )
		{
			GetLogger()->Log( _T("  FAIL") );
			return err;
		}
	}

	//if low nand solution, exit now
	if (GetConfigInfo()-> IsLowNandSolution())
		return err;

    if ( _operation & UPDATE_FORMAT_DATA )
	{
        // Now format the data area.
        if ( (err = m_p_usbmsc_dev->FormatDataDrive()) != STERR_NONE )
		    return err;

        // Restoring HDS before InitializeStore saves time for f/w on boot
	    if ( _operation & UPDATE_SAVE_HDS ) // restore HDS if we are formatting
		{
			GetLogger()->Log( _T("Restoring the HDS.") );
			if ( (err = m_p_usbmsc_dev->SaveRestoreHDS(RESTORE_HDS)) != STERR_NONE )
			{
				GetLogger()->Log( _T(" FAIL") );
	   		    return err;
			}
		}
	}


    if ( _operation & UPDATE_INIT_JANUS ) // Initialize Janus
	{
        // Only initialize Janus if we wiped out the hidden drive.
		GetLogger()->Log( _T("Sending Janus Init command.") );
       	err = m_p_usbmsc_dev->JanusInitialization();
        if ( err != STERR_NONE )
        {
//            wchar_t szMsg[50];
//            wsprintf (szMsg, L"InitJanus Err was %d", err);
//            MessageBox(NULL, szMsg, L"Test", MB_OK);
			GetLogger()->Log( _T(" FAIL") );
            return err;
        }
        else
		{
			GetLogger()->Log( _T(" PASS") );
//            MessageBox(NULL, L"InitJanus OK", L"TEST", MB_OK);
		}
	}

    if ( _operation & UPDATE_INIT_STORE )
	{
        // Only initialize the data store if we formatted the data area.
		GetLogger()->Log( _T("Sending DataStore Init command.") );
       	err = m_p_usbmsc_dev->DataStoreInitialization();
        if ( err != STERR_NONE )
        {
//            wchar_t szMsg[50];
//            wsprintf (szMsg, L"StoreInit Err was %d", err);
//            MessageBox(NULL, szMsg, L"Test", MB_OK);
			GetLogger()->Log( _T(" FAIL") );
            return err;
        }
        else
		{
			GetLogger()->Log( _T(" PASS") );
//            MessageBox(NULL, L"StoreInit OK", L"TEST", MB_OK);
		}
	}

	if ( _operation & UPDATE_FORMAT_DATA )
	{
        // Copy any 2nd data drive contents
	    if ( _operation & UPDATE_2DD_CONTENT)
		{
			GetLogger()->Log( _T("Transferring 2nd Data drive content.") );
		    if ( (err = m_p_usbmsc_dev->Transfer2DDContent()) != STERR_NONE )
			{
				GetLogger()->Log( _T(" FAIL") );
    		    return err;
			}
		}
    }

	// called in stupdaterdlg
	//m_p_usbmsc_dev->CleanupAfterDownload();
	//m_p_usbmsc_dev->CloseScsi();

    return err;
}

ST_ERROR CStUpdater::RecoverDevice()
{
	ST_ERROR err = STERR_NONE;

    // Don't close the handle after recovery.  The device is already rebooted and
    // our handle is invalid.  Trying to close it can cause the "dead usb port" issue
    // where the device will no longer enumerate on that port.
    // we only run once and we exit so it should not be a big deal to let Windows clean it up
    // on process termination.

	if ( m_p_recovery_dev )
	{
		GetLogger()->Log( _T("Downloading firmware to a Player Recovery Class device...") );
		err = m_p_recovery_dev->DownloadUsbMsc();
		delete m_p_recovery_dev;
		m_p_recovery_dev = NULL;
	}
	else if ( m_p_hid_dev )
	{
		GetLogger()->Log( _T("Downloading firmware to a HID device...") );
		err = m_p_hid_dev->DownloadUsbMsc();
		delete m_p_hid_dev;
		m_p_hid_dev = NULL;
	}

	if ( err == STERR_NONE )
		GetLogger()->Log( _T("  Done. (PASS)") );
	else
		GetLogger()->Log( _T("  Done. (FAIL)") );

	return err;
}



ST_ERROR CStUpdater::GetChipId(USHORT& _id)
{
	if ( m_p_usbmsc_dev )
		_id = m_p_usbmsc_dev->GetChipId();

	return STERR_NONE;
}

ST_ERROR CStUpdater::GetPartId(USHORT& _id)
{
	if ( m_p_usbmsc_dev )
		_id = m_p_usbmsc_dev->GetPartId();

	return STERR_NONE;
}

ST_ERROR CStUpdater::GetROMId(USHORT& _id)
{
    if (forcedROMId)
        _id = forcedROMId;
    else
		if ( m_p_usbmsc_dev )
		    _id = m_p_usbmsc_dev->GetROMId();

	return STERR_NONE;
}


ST_ERROR CStUpdater::GetComponentVersions(
	CStVersionInfoPtrArray** _p_arr_current_component_vers, 
	CStVersionInfoPtrArray** _p_arr_upgrade_component_vers
)
{
	if( m_p_usbmsc_dev == NULL )
	{
		return STERR_DEVICE_STATE_UNINITALIZED;
	}
	m_p_usbmsc_dev->GetCurrentComponentVersions(_p_arr_current_component_vers);
	m_p_usbmsc_dev->GetUpgradeComponentVersions(_p_arr_upgrade_component_vers);
	return STERR_NONE;
}

ST_ERROR CStUpdater::GetProjectVersions(
	CStVersionInfo* _p_current_project_ver, 
	CStVersionInfo* _p_upgrade_project_ver
)
{
	if( m_p_usbmsc_dev == NULL )
	{
		return STERR_DEVICE_STATE_UNINITALIZED;
	}
	m_p_usbmsc_dev->GetCurrentProjectVersion(_p_current_project_ver);
	m_p_usbmsc_dev->GetUpgradeProjectVersion(_p_upgrade_project_ver);

	return STERR_NONE;
}

wchar_t CStUpdater::GetDriveLetter()
{
	if ( m_p_usbmsc_dev)
		return m_p_usbmsc_dev->GetDriveLetter();
	else
		return (wchar_t)0;
}

ST_ERROR CStUpdater::DoJustFormat()
{
	ST_ERROR err = STERR_NONE;

	GetProgress()->SetTotalTasks(m_p_usbmsc_dev->GetTotalTasks(UPDATE_FORMAT_DATA));

	err = m_p_usbmsc_dev->GetReadyToDownload();
	if( err != STERR_NONE )
		return err;

	err = m_p_usbmsc_dev->DoJustFormat();
	m_p_usbmsc_dev->CleanupAfterDownload();

	return err;
}

void CStUpdater::FreeScsiDevice()
{
	if( m_p_usbmsc_dev )
		m_p_usbmsc_dev->DestroyScsi();
}

ST_ERROR CStUpdater::ResetChip()
{
    if (m_p_usbmsc_dev)
        return m_p_usbmsc_dev->ResetChip();
    else
        return STERR_DEVICE_STATE_UNINITALIZED;
}

ST_ERROR CStUpdater::GetJanusStatus(UCHAR& _status)
{
    if (m_p_usbmsc_dev)
        return m_p_usbmsc_dev->GetJanusStatus(_status);
    else
        return STERR_DEVICE_STATE_UNINITALIZED;
}

#include <atltime.h>
void CStUpdater::WaitForDriveToAppear(DWORD _last_drives_bitmap, BOOL bUpdating)
{
	DWORD current_bitmap= ::GetLogicalDrives();
	CTime start_time = CTime::GetCurrentTime();

	bool bUserClosed = false;

	while( _last_drives_bitmap == current_bitmap ) 
	{
		if ( WaitForSingleObject( m_hStopTrigger, 0 ) != WAIT_TIMEOUT)
		{
			bUserClosed = true;
			GetLogger()->Log( _T("Quit waiting for a new Drive to mount.") );
			break;  // we've been closed
		}

        if ( bUpdating )
		    GetProgress()->UpdateProgress();
		::Sleep(500);
		current_bitmap = ::GetLogicalDrives();
		if( _last_drives_bitmap != current_bitmap )
		{
			//looks like drive has appeared
			break;
		}
		
		CTime current_time =  CTime::GetCurrentTime();
		CTimeSpan cts = current_time - start_time;
		if( cts.GetTotalSeconds() > 180L )
		{
			//passed three minutes so return
			GetLogger()->Log( _T("No drive showed up after 3 minutes, so quit waiting.") );
			return;
		}

	}

	if ( !bUserClosed )
	{
		GetLogger()->Log( _T("A new Drive mounted. Wait 10 seconds for it to stablize...") );
	
		//wait for ten seconds for the drive to stabilize.
		for( int i=0; i<5; i++)
		{
			if ( WaitForSingleObject( m_hStopTrigger, 0 ) != WAIT_TIMEOUT)
			{
				GetLogger()->Log( _T("Quit waiting for Drive to stablize.") );
				break;  // we've been closed
			}

			::Sleep( 1000 );
			
			if ( bUpdating )
				GetProgress()->UpdateProgress();
		}
	}
}


//
// Function to get the firmware versions from the local binary files without having
// an enumerated stusbmscdev.

//	CStVersionInfoPtrArray	*m_p_arr_ver_info_upgrade;
//	CStVersionInfo			m_ver_info_upgrade_project;
ST_ERROR CStUpdater::GetUpgradeVersions(
		CStVersionInfo* p_upgrade_project_ver,
		CStVersionInfoPtrArray** p_arr_upgrade_component_vers)
{
	UCHAR drive_index;
	UCHAR num_drives, num_sys_drives, sys_drive_index;
	UCHAR player_index;
	
	GetConfigInfo()->GetNumDrives(num_drives);
	GetConfigInfo()->GetNumSystemDrives(num_sys_drives);
	if (GetConfigInfo()->GetPlayerDriveIndex(player_index) == STERR_INVALID_DRIVE_TYPE)
		GetConfigInfo()->GetBootyDriveIndex(player_index); // SDK5

	if( m_p_arr_ver_info_upgrade )
	{
		delete m_p_arr_ver_info_upgrade;
		m_p_arr_ver_info_upgrade = NULL;
	}

	m_p_arr_ver_info_upgrade = new CStVersionInfoPtrArray(num_sys_drives+1);
	if( !m_p_arr_ver_info_upgrade )
	{
		return STERR_NO_MEMORY;
	}

	sys_drive_index = 0;
	for (drive_index = 0; drive_index < num_drives; ++ drive_index)
	{
		LOGICAL_DRIVE_TYPE drive_type;
	
		m_p_config_info->GetDriveType(drive_index, drive_type);

		if ( drive_type == DriveTypeSystem )
		{
			CStVersionInfo* ver = *m_p_arr_ver_info_upgrade->GetAt(sys_drive_index++); 

			CStFwComponent* p_fw_component = new CStFwComponent(this, drive_index, m_langid);

			if( p_fw_component && p_fw_component->GetLastError() != STERR_NONE)
			{
				ST_ERROR err = p_fw_component->GetLastError();
				delete p_fw_component;
				return err;
			}

			if( !p_fw_component )
				return STERR_NO_MEMORY;

			if ( drive_index == player_index )
				p_fw_component->GetProjectVersion(m_ver_info_upgrade_project);

			p_fw_component->GetComponentVersion(*ver);

			ver->m_loaded_from_resource = p_fw_component->IsLoadedFromResource();
			delete p_fw_component;

		}
	}

	if ( p_upgrade_project_ver )
		*p_upgrade_project_ver = m_ver_info_upgrade_project;

	if ( p_arr_upgrade_component_vers )
	{
		*p_arr_upgrade_component_vers = new CStVersionInfoPtrArray(num_sys_drives);
		if( !*p_arr_upgrade_component_vers )
		{
			return STERR_NO_MEMORY;
		}

		for( UCHAR index=0; index<num_sys_drives; index++)
		{
			CStVersionInfo* ver = *(*p_arr_upgrade_component_vers)->GetAt(index);

			*ver = *(*m_p_arr_ver_info_upgrade->GetAt(index));
		}
	}

	return STERR_NONE;
}


ST_ERROR CStUpdater::GetProjectUpgradeVersion(
		CStVersionInfo* p_upgrade_project_ver
		)
{
	ST_ERROR err = STERR_NONE;
	CStVersionInfo p_local_upgrade_project_ver;
	CStVersionInfoPtrArray* p_local_arr_upgrade_component_vers = NULL;

	if ( !m_p_arr_ver_info_upgrade )
	{	// We have not yet retrieved the versions (above)
		err = GetUpgradeVersions( &p_local_upgrade_project_ver,
							&p_local_arr_upgrade_component_vers);

		if ( err == STERR_NONE )
			*p_upgrade_project_ver = p_local_upgrade_project_ver;
	}
	else
		*p_upgrade_project_ver = m_ver_info_upgrade_project;

	if ( p_local_arr_upgrade_component_vers )
		delete p_local_arr_upgrade_component_vers;

	return err;
}

void CStUpdater::SetLanguageId(WORD _langid)
{
    m_langid = _langid;
}

WORD CStUpdater::GetLanguageId()
{
    return m_langid;
}


ST_ERROR CStUpdater::GetDataDriveInfo(int _drivenum, LPTSTR _drive, LPTSTR _fsname, USHORT _fsnamelen, ULONG& _sectors, USHORT& _secsize)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
		err = m_p_usbmsc_dev->GetDataDriveInfo( _drivenum, _sectors, _secsize);

	if ( err == STERR_NONE )
	{
		TCHAR drive[8];
		TCHAR ch;

		ch = GetDriveLetter();
		if (ch)
		{
			_stprintf_s( drive, 8, _T("%c:"), ch); 
			_tcscpy_s (_drive, 4, drive);
			_tcscat_s (drive, 8, L"\\");

			::GetVolumeInformation(
					drive,        
					NULL,     
					0,         
					NULL,  
					NULL,
					NULL,     
					_fsname, 
					_fsnamelen
					);
		}
		else
			*_drive = '\0';
	}

	return err;
}

ST_ERROR CStUpdater::GetMediaType(PHYSICAL_MEDIA_TYPE& _mediatype)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
		err = m_p_usbmsc_dev->GetPhysicalMediaType( _mediatype);

	return err;
}



ST_ERROR CStUpdater::GetSerialNumberSize(USHORT& _serialNoSize)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
	{
		err = m_p_usbmsc_dev->GetSizeOfSerialNumber( _serialNoSize );
	}

	return err;
}

ST_ERROR CStUpdater::GetSerialNumber(CStByteArray * _serialNo)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
	{
		err = m_p_usbmsc_dev->GetSerialNumber( _serialNo );
	}

	return err;
}



ST_ERROR CStUpdater::GetExternalRAMSizeInMB(ULONG& _externalRAMsize)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
		err = m_p_usbmsc_dev->GetExternalRAMSizeInMB( _externalRAMsize);

	return err;
}

ST_ERROR CStUpdater::GetVirtualRAMSizeInMB(ULONG& _virtualRAMsize)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
		err = m_p_usbmsc_dev->GetVirtualRAMSizeInMB( _virtualRAMsize);

	return err;
}

ST_ERROR CStUpdater::GetMediaSizeInBytes(ULONGLONG& _mediasize)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
		err = m_p_usbmsc_dev->GetMediaSizeInBytes( _mediasize);

	return err;
}

ST_ERROR CStUpdater::GetMediaNandPageSizeInBytes(ULONG& _nandpagesize)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
		err = m_p_usbmsc_dev->GetMediaNandPageSizeInBytes( _nandpagesize);

	return err;}

ST_ERROR CStUpdater::GetMediaNandMfgId(ULONG& _nandMfgId)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
	{
		err = m_p_usbmsc_dev->GetMediaNandMfgId( _nandMfgId );
	}

	return err;
}

ST_ERROR CStUpdater::GetMediaNandIdDetails(ULONGLONG& _nandIdDetails)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
	{
		err = m_p_usbmsc_dev->GetMediaNandIdDetails( _nandIdDetails );
	}

	return err;
}

ST_ERROR CStUpdater::GetMediaNandChipEnables(ULONG& _nandChipEnables)
{
	ST_ERROR err = STERR_NONE;

	if ( m_p_usbmsc_dev )
	{
		err = m_p_usbmsc_dev->GetMediaNandChipEnables( _nandChipEnables );
	}

	return err;
}


ST_ERROR CStUpdater::GetProtocolVersionMajor(UCHAR& _versionMajor)
{
	ST_ERROR err = STERR_NONE;

	_versionMajor = 0;

	if ( m_p_usbmsc_dev )
	{
		_versionMajor = m_p_usbmsc_dev->GetProtocolVersionMajor();
	}

	return err;
}

ST_ERROR CStUpdater::GetProtocolVersionMinor(UCHAR& _versionMinor)
{
	ST_ERROR err = STERR_NONE;

	_versionMinor = 0;

	if ( m_p_usbmsc_dev )
	{
		_versionMinor = m_p_usbmsc_dev->GetProtocolVersionMinor();
	}

	return err;
}