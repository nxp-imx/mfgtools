// StRecoveryDev.cpp: implementation of the CStHidDevice class.
//
//////////////////////////////////////////////////////////////////////

#include "StHeader.h"
#include "StGlobals.h"
#include "ddildl_defs.h"
#include "StVersionInfo.h"
#include "StConfigInfo.h"
#include "StUpdater.h"
#include "StByteArray.h"
#include "StFwComponent.h"
#include "StProgress.h"


#include "StHidDevice.h"

//0xa441a6e1, 0xec62, 0x46fb, 0x99, 0x89, 0x2c, 0xd7, 0x8f, 0x1a, 0xaa, 0x34);
DEFINE_GUID( GUID_DEVINTERFACE_HID, 0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, \
             0x11, 0x11, 0x00, 0x00, 0x30);

#define DEVICE_TIMEOUT			5000 // ms

HANDLE CStHidDevice::m_sync_event = NULL;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CStHidDevice::CStHidDevice(CStUpdater* _p_updater, string _name):CStDevice(_p_updater, _name)
{
	m_hid_drive_handle = INVALID_HANDLE_VALUE;
	m_sync_event = NULL;
	m_p_fw_component = NULL;
	m_device_name = L"";
	m_pReadReport = NULL;
	m_pWriteReport = NULL;
}

CStHidDevice::~CStHidDevice()
{
	Trash();
}

ST_ERROR CStHidDevice::Trash()
{
	if( m_p_fw_component )
	{
		delete m_p_fw_component;
		m_p_fw_component = NULL;
	}
	if( m_sync_event )
	{
		CloseHandle(m_sync_event);
		m_sync_event = NULL;
	}
	Close();
	return STERR_NONE;
}

ST_ERROR CStHidDevice::Close()
{
	if( m_hid_drive_handle != INVALID_HANDLE_VALUE)
	{
		if( m_hid_drive_handle )
		{
			CloseHandle(m_hid_drive_handle);
		}
		m_hid_drive_handle = INVALID_HANDLE_VALUE;
	}

	FreeIoBuffers();

	return STERR_NONE;
}


void CStHidDevice::FreeIoBuffers()
{
    if( m_pReadReport )
    {
        free( m_pReadReport );
        m_pReadReport = NULL;
    }
    
    if( m_pWriteReport )
    {
        free( m_pWriteReport );
        m_pWriteReport = NULL;
    }

}

ST_ERROR CStHidDevice::Open(const wchar_t* _p_device_name)
{
	m_hid_drive_handle = CStGlobals::CreateFile(_p_device_name,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_OVERLAPPED,
				NULL);

	if( m_hid_drive_handle == INVALID_HANDLE_VALUE )
	{
		m_system_last_error = CStGlobals::GetLastError();
		return STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE;
	}
	return STERR_NONE;
}

ST_ERROR CStHidDevice::Initialize()
{
	wchar_t sLinkName[MAX_PATH];
	ST_ERROR err = STERR_NONE;
	PLATFORM pf	= CStGlobals::GetPlatform();
	Trash(); //make a clean start

	//
	// Find and open a handle to the hid driver object.
	//

	if( pf == OS_98 || pf == OS_ME )
    {
		err = STERR_UNSUPPORTED_OPERATING_SYSTEM;
	}
	else
	{
    	if( GetUsbDeviceFileName((LPGUID) &GUID_DEVINTERFACE_HID, sLinkName, MAX_PATH) )
		{
	    	err = Open( sLinkName );
		    if( err == STERR_NONE )
				m_device_name = wstring( sLinkName );
		}
		else
			// failed to find our device
			return STERR_FAILED_TO_FIND_HID_DEVICE;
    }

/**
if (err == STERR_NONE)
{
WCHAR szMsg[MAX_PATH]; // these things can be quite long
wsprintf (szMsg, L"opened name: %s", sLinkName);
MessageBox(NULL, szMsg, L"test", MB_OK);
}
**/

	if( err != STERR_NONE )
	{
		return err;
	}


	CHAR updater_drive_number = GetUpdater()->GetConfigInfo()->GetDriveIndex(DRIVE_TAG_UPDATER_S);
	// try to use usbmsc.sb if updater.sb doesn't exist
	if ( updater_drive_number == -1 )
		updater_drive_number = GetUpdater()->GetConfigInfo()->GetDriveIndex(DRIVE_TAG_USBMSC_S);

	m_p_fw_component = new CStFwComponent(GetUpdater(), updater_drive_number, GetUpdater()->GetLanguageId());

	if( !m_p_fw_component )
	{
		return STERR_NO_MEMORY;
	}

	if( m_p_fw_component->GetLastError() != STERR_NONE )
	{
		return m_p_fw_component->GetLastError();
	}

	if( m_p_fw_component->GetFileType() == FW_FILE_TYPE_UNKNOWN)
	{
		return STERR_FAILED_TO_OPEN_FILE;
	}

	m_sync_event = CStGlobals::CreateEvent( NULL, TRUE, FALSE, NULL );
	if( !m_sync_event )
	{
		m_system_last_error = CStGlobals::GetLastError();
		return STERR_FAILED_TO_CREATE_EVENT_OBJECT;
	}

    memset(&m_Capabilities, 0, sizeof(m_Capabilities));

    err = AllocateIoBuffers();
	if ( err != STERR_NONE )
		return err;

	
	return STERR_NONE;
}




ST_ERROR CStHidDevice::DownloadUsbMsc()
{
	ULONG			usb_iterations;
	ULONG			usb_data_fraction;
	size_t			current_pos_in_arr = 0;
	UCHAR			*buf;
	ST_ERROR 		err = STERR_NONE;
	ULONG			ulWriteSize = m_Capabilities.OutputReportByteLength - 1;
	_ST_HID_CBW		cbw = {0};
	UINT			length = (UINT)m_p_fw_component->GetSizeInBytes();
	UINT			tag = 0;
//	BOOL			device_rebooted = FALSE;

	buf = (UCHAR *)malloc ( m_Capabilities.OutputReportByteLength );
	if ( !buf )
		return STERR_NO_MEMORY;

	memset( &cbw, 0, sizeof( _ST_HID_CBW ) );
	cbw.Cdb.Command = BLTC_DOWNLOAD_FW;
	cbw.Cdb.Length = Swap4((UCHAR *)&length);

    memset( m_pWriteReport, 0xDB, m_Capabilities.OutputReportByteLength) ;
    m_pWriteReport->ReportId = HID_BLTC_REPORT_TYPE_COMMAND_OUT;
	cbw.Tag = ++tag;
	cbw.Signature = CBW_BLTC_SIGNATURE;
	cbw.XferLength = length;
	cbw.Flags = CBW_HOST_TO_DEVICE_DIR;

	memcpy( &m_pWriteReport->Payload[0], &cbw, sizeof( _ST_HID_CBW ) );

	if( (err = Write( (UCHAR *)m_pWriteReport, m_Capabilities.OutputReportByteLength )) != STERR_NONE )
	{
		CancelIo( m_hid_drive_handle );
		free (buf);
		return err;
	}

	//
	// Write to the device, splitting in the right number of blocks
	//
	usb_iterations = (ULONG)(m_p_fw_component->GetSizeInBytes() / ulWriteSize);
	if( m_p_fw_component->GetSizeInBytes() % ulWriteSize )
	{
		usb_iterations ++;
	}

	usb_data_fraction = (ULONG)(m_p_fw_component->GetSizeInBytes() % ulWriteSize);
	if( !usb_data_fraction ) 
	{
		usb_data_fraction = ulWriteSize;
	}

//	_p_progress->SetCurrentTask( TASK_TYPE_NONE, (ULONG)usb_iterations );

	for( ULONG iteration=0; iteration<(usb_iterations-1); iteration++ )
	{
		m_p_fw_component->GetData(current_pos_in_arr, ulWriteSize, buf);

		memset( m_pWriteReport, 0xDB, ulWriteSize);
	    m_pWriteReport->ReportId = HID_BLTC_REPORT_TYPE_DATA_OUT;
		memcpy(&m_pWriteReport->Payload[0], buf, ulWriteSize);

		if( (err = Write( (UCHAR *)m_pWriteReport, m_Capabilities.OutputReportByteLength )) != STERR_NONE )
		{
			CancelIo( m_hid_drive_handle );
			free (buf);
			return err;
		}
//		GetUpdater()->GetProgress()->UpdateProgress();
		current_pos_in_arr += ulWriteSize;
	}

	memset( m_pWriteReport, 0xDB, ulWriteSize);

	m_p_fw_component->GetData(current_pos_in_arr, ulWriteSize, buf);
    m_pWriteReport->ReportId = HID_BLTC_REPORT_TYPE_DATA_OUT;
	memcpy(&m_pWriteReport->Payload[0], buf, ulWriteSize);

	err = Write( (UCHAR *)m_pWriteReport, m_Capabilities.OutputReportByteLength );

	free (buf);

	return err;
}

ST_ERROR CStHidDevice::Write(UCHAR* _buf, ULONG _size)
{
	DWORD status;

	// Preparation
	m_overlapped.Offset				= 0;
    m_overlapped.OffsetHigh			= 0;
	m_overlapped.hEvent				= (PVOID)(ULONGLONG)_size;

	CStGlobals::ResetEvent( CStHidDevice::m_sync_event );

	// Write to the device
	if( !CStGlobals::WriteFileEx( m_hid_drive_handle, _buf, _size, &m_overlapped, 
		CStHidDevice::WriteCompletionRoutine ) )
	{
		m_system_last_error = CStGlobals::GetLastError();
		return STERR_FAILED_TO_WRITE_FILE_DATA;
	}

	// wait for completion
	if( (status = CStGlobals::WaitForSingleObjectEx( m_sync_event, DEVICE_TIMEOUT, TRUE )) == WAIT_TIMEOUT )
	{
		CStGlobals::CancelIo( m_hid_drive_handle );
		return STERR_DEVICE_TIMEOUT;
	}

	if( m_overlapped.Offset == 0 )
		return STERR_FAILED_TO_WRITE_FILE_DATA;
	else
		return STERR_NONE;
}

VOID CStHidDevice::WriteCompletionRoutine( 
	DWORD _err_code, 
	DWORD _bytes_transferred, 
	LPOVERLAPPED _lp_overlapped
)
{
	if( ((ULONG)(ULONGLONG)_lp_overlapped->hEvent != _bytes_transferred) || _err_code )
	{
		*(BOOL *)&_lp_overlapped->Offset = 0;
	}
	else
	{
		*(BOOL *)&_lp_overlapped->Offset = _bytes_transferred;
	}

	CStGlobals::SetEvent( m_sync_event );
}





HANDLE CStHidDevice::OpenOneDevice (
    IN       HDEVINFO                    HardwareDeviceInfo,
    IN       PSP_INTERFACE_DEVICE_DATA   DeviceInfoData,
	IN		 wchar_t *devName,
	rsize_t	 bufsize
    )
/*++
Routine Description:

    Given the HardwareDeviceInfo, representing a handle to the plug and
    play information, and deviceInfoData, representing a specific usb device,
    open that device and fill in all the relevant information in the given
    USB_DEVICE_DESCRIPTOR structure.

Arguments:

    HardwareDeviceInfo:  handle to info obtained from Pnp mgr via SetupDiGetClassDevs()
    DeviceInfoData:      ptr to info obtained via SetupDiEnumInterfaceDevice()

Return Value:

    return HANDLE if the open and initialization was successfull,
	else INVLAID_HANDLE_VALUE.

--*/
{
    PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
    ULONG                                predictedLength = 0;
    ULONG                                requiredLength = 0;
	HANDLE								 hOut = INVALID_HANDLE_VALUE;

    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
    SetupDiGetInterfaceDeviceDetail (
            HardwareDeviceInfo,
            DeviceInfoData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node


    predictedLength = requiredLength;
    // sizeof (SP_FNCLASS_DEVICE_DATA) + 512;

    functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc (predictedLength);
    functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

    //
    // Retrieve the information from Plug and Play.
    //
    if (! SetupDiGetInterfaceDeviceDetail (
               HardwareDeviceInfo,
               DeviceInfoData,
               functionClassDeviceData,
               predictedLength,
               &requiredLength,
               NULL)) {
		free( functionClassDeviceData );
        return INVALID_HANDLE_VALUE;
    }

	wcscpy_s( devName, bufsize, functionClassDeviceData->DevicePath) ;

	// Is this our vid+pid?  If not, ignore it.
	if ( wcsstr( devName, L"hid#vid_066f") || wcsstr( devName, L"HID#VID_066F") )
		if ( wcsstr( devName, L"pid_3700") || wcsstr( devName, L"PID_3700") ||
			 wcsstr( devName, L"pid_3770") || wcsstr( devName, L"PID_3770") ||
			 wcsstr( devName, L"pid_3780") || wcsstr( devName, L"PID_3780") )
		{
		    hOut = CreateFile (
                  functionClassDeviceData->DevicePath,
                  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL, // no SECURITY_ATTRIBUTES structure
                  OPEN_EXISTING, // No special create flags
                  0, // No special attributes
                  NULL); // No template file
		}

	free( functionClassDeviceData );
	return hOut;
}


HANDLE CStHidDevice::OpenUsbDevice( LPGUID  pGuid, wchar_t *outNameBuf, rsize_t bufsize)
/*++
Routine Description:

   Do the required PnP things in order to find
   the next available proper device in the system at this time.

Arguments:

    pGuid:      ptr to GUID registered by the driver itself
    outNameBuf: the generated name for this device

Return Value:

    return HANDLE if the open and initialization was successful,
	else INVLAID_HANDLE_VALUE.
--*/
{
   ULONG NumberDevices;
   HANDLE hOut = INVALID_HANDLE_VALUE;
   HDEVINFO                 hardwareDeviceInfo;
   SP_INTERFACE_DEVICE_DATA deviceInfoData;
   ULONG                    i;
   BOOLEAN                  done;

   NumberDevices = 0;

   //
   // Open a handle to the plug and play dev node.
   // SetupDiGetClassDevs() returns a device information set that contains info on all
   // installed devices of a specified class.
   //
   hardwareDeviceInfo = SetupDiGetClassDevs (
                           pGuid,
                           NULL, // Define no enumerator (global)
                           NULL, // Define no
                           (DIGCF_PRESENT | // Only Devices present
                            DIGCF_INTERFACEDEVICE)); // Function class devices.

   //
   // Take a wild guess at the number of devices we have;
   // Be prepared to realloc and retry if there are more than we guessed
   //
   NumberDevices = 4;
   done = FALSE;
   deviceInfoData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

   i=0;
   while (!done) {
      NumberDevices *= 2;


      for (; i < NumberDevices; i++) {

         // SetupDiEnumDeviceInterfaces() returns information about device interfaces
         // exposed by one or more devices. Each call returns information about one interface;
         // the routine can be called repeatedly to get information about several interfaces
         // exposed by one or more devices.

         if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                         0, // We don't care about specific PDOs
										 pGuid,
                                         i,
                                         &deviceInfoData)) {

            hOut = OpenOneDevice (hardwareDeviceInfo, &deviceInfoData, outNameBuf, bufsize);
			if ( hOut != INVALID_HANDLE_VALUE )
			{
               done = TRUE;
               break;
			}
         }
		 else
		 {
            if (ERROR_NO_MORE_ITEMS == CStGlobals::GetLastError())
			{
               done = TRUE;
               break;
            }
         }
      }
   }

   NumberDevices = i;

   // SetupDiDestroyDeviceInfoList() destroys a device information set
   // and frees all associated memory.

   SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
   return hOut;
}




BOOL CStHidDevice::GetUsbDeviceFileName( LPGUID  pGuid, wchar_t *outNameBuf, rsize_t bufsize)
/*++
Routine Description:

    Given a ptr to a driver-registered GUID, give us a string with the device name
    that can be used in a CreateFile() call.
    Actually briefly opens and closes the device and sets outBuf if successfull;
    returns FALSE if not

Arguments:

    pGuid:      ptr to GUID registered by the driver itself
    outNameBuf: the generated zero-terminated name for this device

Return Value:

    TRUE on success else FALSE

--*/
{
	HANDLE hDev = OpenUsbDevice( pGuid, outNameBuf, bufsize );
	if ( hDev != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hDev );
		return TRUE;
	}
	return FALSE;

}

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HANDLE, PVOID);
typedef UINT (CALLBACK* LPFNDLLFUNC2)(PVOID);

ST_ERROR CStHidDevice::AllocateIoBuffers()
{
	ST_ERROR error = STERR_NONE;
	HINSTANCE h_HidDll;
	LPFNDLLFUNC1 lpfnDllFunc1; 
	LPFNDLLFUNC2 lpfnDllFunc2; 

	h_HidDll = LoadLibrary(L"hid.dll");
	if (h_HidDll == NULL)
	{
		error = STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE;
		m_system_last_error = CStGlobals::GetLastError();
        return error;
	}
	// Get the Capabilities including the max size of the report buffers
    PHIDP_PREPARSED_DATA  PreparsedData = NULL;

	lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(h_HidDll,
                                           "HidD_GetPreparsedData");
	if (!lpfnDllFunc1)
	{
		// handle the error
		m_system_last_error = CStGlobals::GetLastError();
		FreeLibrary(h_HidDll);       
		return STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE;
	}

    if ( !lpfnDllFunc1(m_hid_drive_handle, &PreparsedData) )
    {
		m_system_last_error = CStGlobals::GetLastError();
		error = STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE;
		FreeLibrary(h_HidDll);       
        return error;
    }

	lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(h_HidDll,
                                           "HidP_GetCaps");
	if (!lpfnDllFunc1)
	{
		// handle the error
		m_system_last_error = CStGlobals::GetLastError();
		error = STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE;
	}
	else
	    if( lpfnDllFunc1(PreparsedData, &m_Capabilities) != HIDP_STATUS_SUCCESS )
		{
			m_system_last_error = CStGlobals::GetLastError();
			error = STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE;
		}

	lpfnDllFunc2 = (LPFNDLLFUNC2)GetProcAddress(h_HidDll,
                                           "HidD_FreePreparsedData");
	if (!lpfnDllFunc2)
	{
		// handle the error
		m_system_last_error = CStGlobals::GetLastError();
		FreeLibrary(h_HidDll);       
		return STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE;
	}
 
    lpfnDllFunc2(PreparsedData);

	FreeLibrary(h_HidDll);       

	// Allocate a Read and Write Report buffers
    FreeIoBuffers();

    if ( m_Capabilities.InputReportByteLength )
    {
        m_pReadReport = (_ST_HID_DATA_REPORT*)malloc( m_Capabilities.InputReportByteLength );
        if ( m_pReadReport == NULL )
            return STERR_NO_MEMORY;
    }

    if ( m_Capabilities.OutputReportByteLength )
    {
        m_pWriteReport = (_ST_HID_DATA_REPORT*)malloc( m_Capabilities.OutputReportByteLength );
        if ( m_pWriteReport == NULL )
            return STERR_NO_MEMORY;
    }

    return STERR_NONE;
}