// StRecoveryDev.cpp: implementation of the CStRecoveryDev class.
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

#include "StRecoveryDev.h"
/*
// {A441A6E1-EC62-46FB-9989-2CD78F1AAA34}
DEFINE_GUID(GUID_DEVINTERFACE_STMP3XXX_USB_BULK_DEVICE,
0xa441a6e1, 0xec62, 0x46fb, 0x99, 0x89, 0x2c, 0xd7, 0x8f, 0x1a, 0xaa, 0x34);

// {9FFF066D-3ED3-4567-9123-8B82CFE1CDD4}
DEFINE_GUID(GUID_CLASS_STMP3XXX_USB_BULK_DEVICE,
0x9FFF066D, 0x3ED3, 0x4567, 0x91, 0x23, 0x8B, 0x82, 0xCF, 0xE1, 0xCD, 0xD4);
*/

#define PIPE_NAME				L"\\PIPE00"
#define PIPE_SIZE				4096 //??? this will be different for usb 1.1 and 2.0
#define DEVICE_TIMEOUT			1000

HANDLE CStRecoveryDev::m_sync_event = NULL;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CStRecoveryDev::CStRecoveryDev(CStUpdater* _p_updater, string _name):CStDevice(_p_updater, _name)
{
	m_recovery_drive_handle = INVALID_HANDLE_VALUE;
	m_p_fw_component = NULL;
	m_device_name = L"";
}

CStRecoveryDev::~CStRecoveryDev()
{
	Trash();
}

ST_ERROR CStRecoveryDev::Trash()
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

ST_ERROR CStRecoveryDev::Close()
{
	if( m_recovery_drive_handle != INVALID_HANDLE_VALUE)
	{
		if( m_recovery_drive_handle )
		{
			CloseHandle(m_recovery_drive_handle);
		}
		m_recovery_drive_handle = INVALID_HANDLE_VALUE;
	}

	return STERR_NONE;
}

ST_ERROR CStRecoveryDev::Open(const wchar_t* _p_device_name)
{
	m_recovery_drive_handle = CStGlobals::CreateFile(_p_device_name,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_OVERLAPPED,
				NULL);

	if( m_recovery_drive_handle == INVALID_HANDLE_VALUE )
	{
		m_system_last_error = CStGlobals::GetLastError();
		return STERR_FAILED_TO_FIND_DEVICE_IN_RECOVERY_MODE;
	}
	return STERR_NONE;
}

ST_ERROR CStRecoveryDev::Initialize()
{
	wchar_t sLinkName[MAX_PATH];
	ST_ERROR err = STERR_NONE;
	PLATFORM pf	= CStGlobals::GetPlatform();
	Trash(); //make a clean start

	//
	// Find and open a handle to the stmp3rec driver object.
	//

	if( pf == OS_XP64 || pf == OS_VISTA32 || pf == OS_VISTA64 || pf == OS_WINDOWS7)
    {
    	GetUsbDeviceFileName((LPGUID) &GUID_DEVINTERFACE_STMP3XXX_USB_BULK_DEVICE, sLinkName, MAX_PATH);

    	err = Open( sLinkName );
	    if( err == STERR_NONE )
	    	m_device_name = wstring( sLinkName );
        else
            return err;
    }
    else
    {
    	for( long i=0; i<100; i++ )
	    {
		    swprintf( sLinkName, MAX_PATH, L"\\\\.\\StMp3RecDevice%d", i );

    		err = Open( sLinkName );
	    	if( err == STERR_NONE )
		    {
			    m_device_name = wstring( sLinkName );
    			break;
	    	}
    	}
    }
/*
WCHAR szMsg[MAX_PATH]; // these things can be quite long
wsprintf (szMsg, L"opened name: %s", sLinkName);
MessageBox(NULL, szMsg, L"test", MB_OK);
*/
	if( err != STERR_NONE )
	{
		return err;
	}

	CHAR updater_drive_number = -1;
	GetUpdater()->GetConfigInfo()->GetUpdaterDriveIndex(updater_drive_number);
	// try to use usbmsc.sb if updter doesn't exist
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
	
	return STERR_NONE;
}

ST_ERROR CStRecoveryDev::DownloadUsbMsc()
{
	ULONG			usb_iterations;
	ULONG			usb_data_fraction;
	size_t			current_pos_in_arr = 0;
	UCHAR			buf[PIPE_SIZE];
	ST_ERROR 		err = STERR_NONE;
//	BOOL			device_rebooted = FALSE;
	//
	// Write to the device, splitting in the right number of blocks
	//
	usb_iterations = (ULONG)(m_p_fw_component->GetSizeInBytes() / PIPE_SIZE);
	if( m_p_fw_component->GetSizeInBytes() % PIPE_SIZE )
	{
		usb_iterations ++;
	}

	usb_data_fraction = (ULONG)(m_p_fw_component->GetSizeInBytes() % PIPE_SIZE);
	if( !usb_data_fraction ) 
	{
		usb_data_fraction = PIPE_SIZE;
	}

//	_p_progress->SetCurrentTask( TASK_TYPE_NONE, (ULONG)usb_iterations );

	for( ULONG iteration=0; iteration<(usb_iterations-1); iteration++ )
	{
		m_p_fw_component->GetData(current_pos_in_arr, PIPE_SIZE, buf);

		if( (err = Write( buf, PIPE_SIZE )) != STERR_NONE )
		{
			return err;
		}
//		GetUpdater()->GetProgress()->UpdateProgress();
		current_pos_in_arr += PIPE_SIZE;
	}

	CStGlobals::MakeMemoryZero(buf, PIPE_SIZE);

	m_p_fw_component->GetData(current_pos_in_arr, PIPE_SIZE, buf);
	err = Write( buf, usb_data_fraction );

//	GetUpdater()->GetProgress()->UpdateProgress();
	PLATFORM pf	= CStGlobals::GetPlatform();
    if ( pf == OS_98 )
        Close();
/*	Sleep( 500 );
	//
	// wait for the device to close.
	//
	while( !device_rebooted )
	{
		_p_progress->StartSearch();
		ST_ERROR err = STERR_NONE;

		err = Open( m_device_name.c_str() );
		Close();
		if( err != STERR_NONE )
		{
			device_rebooted = TRUE;
		}
	}
*/
	return err;
}

ST_ERROR CStRecoveryDev::Write(UCHAR* _buf, ULONG _size)
{
	DWORD status;

	// Preparation
	m_overlapped.Offset				= 0;
    m_overlapped.OffsetHigh			= 0;
	m_overlapped.hEvent				= (PVOID)(ULONGLONG)_size;

	CStGlobals::ResetEvent( CStRecoveryDev::m_sync_event );

	// Write to the device
	if( !CStGlobals::WriteFileEx( m_recovery_drive_handle, _buf, _size, &m_overlapped, 
		CStRecoveryDev::WriteCompletionRoutine ) )
	{
		m_system_last_error = CStGlobals::GetLastError();
		return STERR_FAILED_TO_WRITE_FILE_DATA;
	}

	// wait for completion
	if( (status = CStGlobals::WaitForSingleObjectEx( m_sync_event, DEVICE_TIMEOUT, TRUE )) == WAIT_TIMEOUT )
	{
		CStGlobals::CancelIo( m_recovery_drive_handle );
		return STERR_DEVICE_TIMEOUT;
	}

	if( m_overlapped.Offset == 0 )
		return STERR_FAILED_TO_WRITE_FILE_DATA;
	else
		return STERR_NONE;
}

VOID CStRecoveryDev::WriteCompletionRoutine( 
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





HANDLE CStRecoveryDev::OpenOneDevice (
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

    hOut = CreateFile (
                  functionClassDeviceData->DevicePath,
                  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL, // no SECURITY_ATTRIBUTES structure
                  OPEN_EXISTING, // No special create flags
                  0, // No special attributes
                  NULL); // No template file

	free( functionClassDeviceData );
	return hOut;
}


HANDLE CStRecoveryDev::OpenUsbDevice( LPGUID  pGuid, wchar_t *outNameBuf, rsize_t bufsize)
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
   PUSB_DEVICE_DESCRIPTOR   usbDeviceInst;
   PUSB_DEVICE_DESCRIPTOR	*UsbDevices = &usbDeviceInst;

   *UsbDevices = NULL;
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

      if (*UsbDevices) {
         *UsbDevices = (PUSB_DEVICE_DESCRIPTOR)
               realloc (*UsbDevices, (NumberDevices * sizeof (USB_DEVICE_DESCRIPTOR)));
      } else {
         *UsbDevices = (PUSB_DEVICE_DESCRIPTOR) calloc (NumberDevices, sizeof (USB_DEVICE_DESCRIPTOR));
      }

      if (NULL == *UsbDevices) {

         // SetupDiDestroyDeviceInfoList destroys a device information set
         // and frees all associated memory.

         SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
         return INVALID_HANDLE_VALUE;
      }

      usbDeviceInst = *UsbDevices + i;

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
			if ( hOut != INVALID_HANDLE_VALUE ) {
               done = TRUE;
               break;
			}
         } else {
            if (ERROR_NO_MORE_ITEMS == CStGlobals::GetLastError()) {
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
   free ( *UsbDevices );
   return hOut;
}




BOOL CStRecoveryDev::GetUsbDeviceFileName( LPGUID  pGuid, wchar_t *outNameBuf, rsize_t bufsize)
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