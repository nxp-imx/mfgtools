// StScsi_Nt.cpp: implementation of the CStScsi_Nt class.
//
//////////////////////////////////////////////////////////////////////
#include "StHeader.h"
#include "stglobals.h"
#include "StByteArray.h"
#include "ddildl_defs.h"
#include "StConfigInfo.h"
#include "ddildl_defs.h"
#include "StVersionInfo.h"
#include "StUpdater.h"
#include "StDdiApi.h"
#include "StProgress.h"
#include <ntddscsi.h>
//#include <ntdddisk.h>
#include <scsidefs.h>
#include <wnaspi32.h>

#include "StScsi_Nt.h"

#define LOCK_TIMEOUT        1000       // 1 Second
#define LOCK_RETRIES        10

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

HANDLE CStScsi_Nt::m_sync_event = NULL;

CStScsi_Nt::CStScsi_Nt(CStUpdater *_p_updater, string _name):CStScsi(_p_updater, _name)
{
	m_spt = NULL;
}

CStScsi_Nt::~CStScsi_Nt()
{

}

ST_ERROR CStScsi_Nt::Initialize(USHORT driveToFind, USHORT UpgradeOrNormal)
{
	wchar_t			DriveToOpen[16];
	wchar_t			ch=L'';
	INQUIRYDATA		InquiryData;
	wstring			w_strSCSIProductString;
	string			strSCSIProductString;
	wstring			w_strSCSIMfgString;
	string			strSCSIMfgString;
	long			Count = 0;
	CStScsiInquiry	api_scsi_inquiry;
	wstring			drives(L"");
	USHORT			driveFound = 0;
	ST_ERROR        err = STERR_NONE;
	USHORT			vid = 0, pid = 0, secondary_pid = 0;

    if (UpgradeOrNormal == NORMAL_MSC)
    {
	    GetUpdater()->GetConfigInfo()->GetSCSIMfgString(w_strSCSIMfgString);
	    GetUpdater()->GetConfigInfo()->GetSCSIProductString(w_strSCSIProductString);
		USES_CONVERSION;
		strSCSIMfgString = W2A(w_strSCSIMfgString.c_str());
		strSCSIProductString = W2A(w_strSCSIProductString.c_str());
		GetUpdater()->GetConfigInfo()->GetUSBVendorId(vid);
		GetUpdater()->GetConfigInfo()->GetUSBProductId(pid);
		GetUpdater()->GetConfigInfo()->GetSecondaryUSBProductId(secondary_pid);
    }
    else
    {
        strSCSIMfgString = GENERIC_UPGRADE_MFG_STRING;
        strSCSIProductString = GENERIC_UPGRADE_PRODUCT_STRING;
    }


	drives = QueryAllLogicalDrives();

	m_drive_letter = '\0';

	for( size_t index=0; index<drives.length(); index++ )
	{
		ch=drives[ index ];
		if( ch == L'A' )
			continue;
		
		swprintf( DriveToOpen, 16, L"%c:\\", ch );
		
		if( ::GetDriveType ( DriveToOpen ) != DRIVE_REMOVABLE )
		{
			continue;
		}

		if( m_handle && (m_handle != INVALID_HANDLE_VALUE) )
		{
			CloseHandle( m_handle );
			m_handle = INVALID_HANDLE_VALUE;
		}

		swprintf( DriveToOpen, 16, L"\\\\.\\%c:", ch );

        m_handle = CStGlobals::CreateFile(
            DriveToOpen,						// device interface name
            GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                               // lpSecurityAttributes
            OPEN_EXISTING,                      // dwCreationDistribution
            0,                                  // dwFlagsAndAttributes
            NULL                                // hTemplateFile
        );
                
		if( m_handle == INVALID_HANDLE_VALUE ) 
		{
			//CStTrace::trace( "CreateFile failed with error: %d\n", GetLastError() );
			continue;
		}

		if( SendDdiApiCommand(&api_scsi_inquiry) == STERR_NONE )
		{
/***
char szMsg[100];
char mfg[9];
char prod[17];
int i;
***/
			api_scsi_inquiry.GetInquiryData(&InquiryData);
/***
            for (i = 0; i < 8; ++i)
                if (InquiryData.VendorId[i] == ' ')
                    mfg[i] = '.';
                else
                    mfg[i] = InquiryData.VendorId[i];
            mfg[8] = 0;

            for (i = 0; i < 16; ++i)
                if (InquiryData.ProductId[i] == ' ')
                    prod[i] = '.';
                else
                    prod[i] = InquiryData.ProductId[i];
            prod[16] = 0;

            sprintf (szMsg, "Found Mfg: %s Product: %s", mfg, prod);
            MessageBoxA(NULL, szMsg, "test", MB_OK);
***/		

			if (InquiryData.DeviceType != DIRECT_ACCESS_DEVICE)
			{
                CloseHandle( m_handle );
				m_handle = INVALID_HANDLE_VALUE;
				continue;
			}

			ST_BOOLEAN product_id_match = FALSE;
            if ( GetUpdater()->GetConfigInfo()->UseScsiProductSubstringQualifier() ) {
                size_t index = strSCSIProductString.find_last_not_of(' ');
                strSCSIProductString.resize(index+1);
                product_id_match = strstr( (const char*)InquiryData.ProductId, strSCSIProductString.c_str() ) != NULL;
            }
            else {
                product_id_match = !_strnicmp( (const char*)InquiryData.ProductId, strSCSIProductString.c_str(), strSCSIProductString.length () );
            }

            if( product_id_match )		
			{
				if( !_strnicmp( (const char*)InquiryData.VendorId, strSCSIMfgString.c_str(), strSCSIMfgString.length () ) )		
				{
					ST_BOOLEAN media_system=FALSE; 

					// Is this the drive we want?  
					if (driveFound != driveToFind)
					{
						CloseHandle( m_handle );  // no, continue
						m_handle = INVALID_HANDLE_VALUE;
						++driveFound;
						continue;
					}

					// Is this really the drive we want?
				    if (UpgradeOrNormal == NORMAL_MSC)
					{
						if (!UsbIdsMatch(w_strSCSIMfgString.c_str(), w_strSCSIProductString.c_str(), vid, pid))
						{
							if (!secondary_pid )
							{
								CloseHandle( m_handle );  // no, continue
								m_handle = INVALID_HANDLE_VALUE;
								continue;
							}
							else if (!UsbIdsMatch(w_strSCSIMfgString.c_str(), w_strSCSIProductString.c_str(), vid, secondary_pid))
							{
								CloseHandle( m_handle );  // no, continue
								m_handle = INVALID_HANDLE_VALUE;
								continue;
							}
						}
					}

                    // Check if the device is hostlink in limited MSC mode
                    // If so, we get out returning the limited support error.
                    GetProtocolVersionMajor( m_ProtocolVersionMajor );
                    GetProtocolVersionMinor( m_ProtocolVersionMinor );
					m_ProtocolVersion = m_ProtocolVersionMajor;

                    if (m_ProtocolVersion == ST_LIMITED_HOSTLINK)
                    {
						m_drive_letter = ch;
			    		CloseHandle( m_handle );
				    	m_handle = INVALID_HANDLE_VALUE;
                        return STERR_LIMITED_VENDOR_SUPPORT;
                    }

					err = IsSystemMedia(media_system);

   					if( err != STERR_NONE )
    				{
						err = STERR_NONE;
#ifdef DEBUG
						// at least log this to debug output.						
						_RPT1(_CRT_WARN,"IsSystemMedia call failed with error : %d\n", err);
#endif
   					}

					// allow non-system media for low nand solutions
					// since only external media is enumerated as a drive
					if(media_system || GetUpdater()->GetConfigInfo()->IsLowNandSolution())
					{
						m_drive_letter = ch;
						Count ++;
						CloseHandle(m_handle);
						m_handle = INVALID_HANDLE_VALUE;
						break;
					}
				}
			}
		}
		CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
	}	
	if (Count <= 0)
	{
		return STERR_FAILED_TO_LOCATE_SCSI_DEVICE;
	}
	return err;
}

ST_ERROR CStScsi_Nt::SendCommand( 
	CStByteArray* _p_command_arr, 
	UCHAR _cdb_len, 
	BOOL _direction_out, 
	CStByteArray& _response_arr,
    ULONG ulTimeOut
)
{
    PSPT_WITH_BUFFERS   spt;
    size_t				data_length;
    DWORD               return_status = ERROR_SUCCESS;
	UCHAR				scsi_cmd; 				
	DWORD				dwThreadId=0;
	HANDLE				thread_handle=INVALID_HANDLE_VALUE;
	DWORD				wait_result;
	size_t				index;

	if( m_handle == INVALID_HANDLE_VALUE )
	{
		ST_ERROR err = Open();
		if( err != STERR_NONE )
		{
			return err;
		}
	}

	_p_command_arr->GetAt(0, scsi_cmd);

	data_length = (size_t)CStGlobals::Max(_response_arr.GetCount(), 
		( _p_command_arr->GetCount() - _cdb_len ));

	//
	// Allocate the buffer to send scsi_pass_through command.
	//
    spt = AllocateSPT(_cdb_len, _direction_out, scsi_cmd, data_length, ulTimeOut);

	if(spt == NULL)
	{
		return STERR_NO_MEMORY;
	}

	//
	// copy the command to cdb array
	//
	for( index = 0; index < _cdb_len ; index ++)
	{
		_p_command_arr->GetAt(index, spt->Spt.Cdb[index]);
	}

	//
	// if command contains data copy it to the output buffer.
	//
	for(index = _cdb_len; index < _p_command_arr->GetCount(); index ++)
	{
		_p_command_arr->GetAt( index, spt->DataBuffer[index - _cdb_len] );
	}

	m_spt = spt;

	CString logStr;
	if ( m_spt->Spt.Cdb[0] == 0xC0 || m_spt->Spt.Cdb[0] == 0xC1 )
	{
		logStr.Format(_T("    Sending Vendor-Specific SCSI %s cmd: 0x%02x(%d), type: 0x%02x(%d), timeout: %d seconds."), 
			m_spt->Spt.Cdb[0] == 0xC0 ? _T("Read") : _T("Write"),
			m_spt->Spt.Cdb[1], m_spt->Spt.Cdb[1],
			m_spt->Spt.Cdb[2], m_spt->Spt.Cdb[2],
			m_spt->Spt.TimeOutValue);
	}
	else
	{
		logStr.Format(_T("    Sending Formal SCSI cmd: 0x%02x(%d), timeout: %d seconds."), 
			m_spt->Spt.Cdb[0], m_spt->Spt.Cdb[0],
			m_spt->Spt.TimeOutValue);
	}
	GetUpdater()->GetLogger()->Log(logStr);

	thread_handle = CreateThread(
		NULL,                        // default security attributes 
		0,                           // use default stack size  
		CStScsi_Nt::BeginSendCommandThread,      // thread function 
		this,						 // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);  
	
	wait_result = CStGlobals::WaitForSingleObjectEx( thread_handle, 0, FALSE );
	while( wait_result != WAIT_OBJECT_0 )
	{	
        Sleep(0);
		if( GetUpdater()->GetProgress() )
		{
			GetUpdater()->GetProgress()->UpdateProgress(FALSE);
		}
		wait_result = CStGlobals::WaitForSingleObjectEx( thread_handle, 0, FALSE );
	}	

	m_spt = NULL;
    CloseHandle(thread_handle);

	if( m_system_last_error != ERROR_SUCCESS )
	{
		logStr.Format(_T("     FAILED! error: 0x%08X(%d)"), m_system_last_error, m_system_last_error);
		GetUpdater()->GetLogger()->Log(logStr);
	    FreeSPT(spt);
		m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;
		return STERR_FAILED_DEVICE_IO_CONTROL;
    }
	
	if (spt->Spt.ScsiStatus != SCSISTAT_GOOD)
	{
		logStr = _T("     SCSI SENSE ERROR! data:");
		for ( int i =0; i < spt->Spt.SenseInfoLength; ++i )
			logStr.AppendFormat(_T(" %02X"), spt->SenseInfoBuffer[i]);
		GetUpdater()->GetLogger()->Log(logStr);

		return_status = !ERROR_SUCCESS;	// DeviceTypeQualifier == 0x01 means the external disk is removed
		m_last_error = STERR_FAILED_TO_SEND_SCSI_COMMAND;
		SaveSenseData(spt->SenseInfoBuffer, spt->Spt.SenseInfoLength);
		GetUpdater()->GetErrorObject()->SaveStatus(this, GetSenseData());
		m_last_error = STERR_NONE;
	}
	else
	{
		for(index=0; index<_response_arr.GetCount(); index++)
		{
			_response_arr.SetAt(index, spt->DataBuffer[index]);
		}
	}

    FreeSPT(spt);

	if(return_status == ERROR_SUCCESS)
		return STERR_NONE;
	return STERR_FAILED_TO_SEND_SCSI_COMMAND;
}

DWORD WINAPI CStScsi_Nt::BeginSendCommandThread( LPVOID pParam )
{
	CStScsi_Nt* _p_scsi_nt = (CStScsi_Nt*)pParam;
	DWORD       returned   = 0;

	if(!CStGlobals::DeviceIoControl(_p_scsi_nt->GetHandle(),
                         IOCTL_SCSI_PASS_THROUGH,
                         _p_scsi_nt->m_spt,
                         (ULONG)_p_scsi_nt->m_spt->TotalSize,
						 _p_scsi_nt->m_spt,
                         (ULONG)_p_scsi_nt->m_spt->TotalSize,
                         &returned,
                         NULL))
	{
		_p_scsi_nt->SetSystemLastError(CStGlobals::GetLastError());
	}
	else
	{
		_p_scsi_nt->SetSystemLastError(ERROR_SUCCESS);
	}
	return 0;
}

PSPT_WITH_BUFFERS CStScsi_Nt::AllocateSPT (
    UCHAR		_cdb_len,
    BOOL		_data_out,
	UCHAR		/*_scsi_cmd*/,
    size_t		_data_size,
    ULONG       _timeout
)
{
    size_t				total_size;
    PSPT_WITH_BUFFERS   pspt;

    total_size = sizeof(SPT_WITH_BUFFERS) + _data_size;

    pspt = (PSPT_WITH_BUFFERS)LocalAlloc(LPTR, total_size); // LPTR includes LMEM_ZEROINIT

    if (pspt == NULL)
    {
        return pspt;
    }

    memset(pspt, 0, total_size);

    pspt->TotalSize = total_size;

    pspt->Spt.Length = sizeof(pspt->Spt);

    pspt->Spt.CdbLength = _cdb_len;

    pspt->Spt.SenseInfoLength = sizeof(pspt->SenseInfoBuffer);

    pspt->Spt.DataIn = (_data_out) ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_IN;

	pspt->Spt.DataTransferLength = (ULONG)_data_size;

    pspt->Spt.TimeOutValue = _timeout;

    pspt->Spt.SenseInfoOffset =
        (DWORD)((ULONG_PTR)&pspt->SenseInfoBuffer[0] - (ULONG_PTR)pspt);

    pspt->Spt.DataBufferOffset =
        (DWORD)((ULONG_PTR)&pspt->DataBuffer[0] - (ULONG_PTR)pspt);

    return pspt;
}

//******************************************************************************
//
// FreeSPT()
//
//******************************************************************************

VOID CStScsi_Nt::FreeSPT (
    PSPT_WITH_BUFFERS _p_spt
)
{
    LocalFree((void*)_p_spt);
}

ST_ERROR CStScsi_Nt::Open()
{
	ST_ERROR err = STERR_NONE;
	if( m_handle == INVALID_HANDLE_VALUE )
	{
		wchar_t	DriveToOpen[16];

		swprintf( DriveToOpen, 16, L"\\\\.\\%c:", GetDriveLetter() );
		m_handle = CStGlobals::CreateFile(
            DriveToOpen,						// device interface name
            GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                               // lpSecurityAttributes
            OPEN_EXISTING,                      // dwCreationDistribution
            0,                                  // dwFlagsAndAttributes
            NULL                                // hTemplateFile
        );
                
		if( m_handle == INVALID_HANDLE_VALUE ) 
		{
			//CStTrace::trace( "CreateFile failed with error: %d\n", GetLastError() );
			m_system_last_error = CStGlobals::GetLastError();
			err = STERR_INVALID_DEVICE_HANDLE;
		}
	}

	return err;
}



ST_ERROR CStScsi_Nt::Close()
{
	ST_ERROR err = STERR_NONE;
	if( m_handle != INVALID_HANDLE_VALUE )
	{
		if ( !CloseHandle(m_handle) )
			err = GetLastError();
		m_handle = INVALID_HANDLE_VALUE;
	}
	return err;
}

ST_ERROR CStScsi_Nt::Lock(BOOL /*_media_new*/)
{
	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
	int nTryCount;

	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;

	// Do this in a loop until a timeout period has expired
	for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
	{
		if (CStGlobals::DeviceIoControl(
				m_handle,
				FSCTL_LOCK_VOLUME,
				NULL, 0,
				NULL, 0,
				&dwBytesReturned,
				NULL 
			) )
		{
#ifdef DEBUG
			_RPT1(_CRT_WARN,"Lock took %d tries\n", nTryCount); 
#endif
			return STERR_NONE;
		}

		Sleep( dwSleepAmount );
	}

#ifdef DEBUG
	_RPT0(_CRT_WARN,"CStScsi_Nt::Lock - FAILED\n"); 
#endif
	return STERR_FAILED_TO_LOCK_THE_DRIVE;
}

ST_ERROR CStScsi_Nt::Unlock(BOOL /*_media_new*/)
{
	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
	int nTryCount;

	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;

	// Do this in a loop until a timeout period has expired
	for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
	{
		if (CStGlobals::DeviceIoControl(
				m_handle,
				FSCTL_UNLOCK_VOLUME,
				NULL, 0,
				NULL, 0,
				&dwBytesReturned,
				NULL 
			) )
		{		
			return STERR_NONE;
		}
		Sleep( dwSleepAmount );
	}

	return STERR_FAILED_TO_UNLOCK_THE_DRIVE;
}

ST_ERROR CStScsi_Nt::AcquireFormatLock(BOOL /*_media_new*/)
{
	//
	// There is nothing like obtaining a separate lock for formatting the media in NT based systems.
	//
	return STERR_NONE;
}

ST_ERROR CStScsi_Nt::ReleaseFormatLock(BOOL /*_media_new*/)
{
	//
	// This will be called after formatting the media.
	// A good place to dismount, this will make operating system to forget about the present file system on the media,
	// and refreshes.
	//

	return Dismount();
}

ST_ERROR CStScsi_Nt::Dismount()
{
	DWORD dwBytesReturned;
	DWORD dwSleepAmount;
	int nTryCount;

	dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;
	// Do this in a loop until a timeout period has expired
	for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
	{
		if (CStGlobals::DeviceIoControl(
				m_handle,
				FSCTL_DISMOUNT_VOLUME,
				NULL, 0,
				NULL, 0,
				&dwBytesReturned,
				NULL 
			) )
		{		
			return STERR_NONE;
		}
		Sleep( dwSleepAmount );
	}

	return STERR_FAILED_TO_DISMOUNT_THE_DRIVE;
}

#define IOCTL_DISK_DELETE_DRIVE_LAYOUT      CTL_CODE(IOCTL_DISK_BASE, 0x0040, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

wstring CStScsi_Nt::QueryAllLogicalDrives()
{
	DWORD drive_bits=0;
	DWORD mask = 0x0001;
	wstring drives(L"");

	drive_bits = ::GetLogicalDrives();
	for(wchar_t ch = L'A'; ch <= L'Z'; ch++ )
	{
		if( mask & drive_bits )
			drives = drives + ch;
		mask = mask * 2;
	}
	return drives;
}
ST_ERROR CStScsi_Nt::FormatPartition( PDISK_GEOMETRY _p_dg, ULONG _hidden_sectors )
{
	DWORD dwBytesReturned;
	PDRIVE_LAYOUT_INFORMATION pdli = AllocPartitionInfo(4);

	pdli->PartitionEntry[0].StartingOffset.QuadPart = _hidden_sectors * _p_dg->BytesPerSector; 
		
	pdli->PartitionEntry[0].PartitionLength.QuadPart =
		( _p_dg->Cylinders.QuadPart * _p_dg->SectorsPerTrack * _p_dg->TracksPerCylinder * _p_dg->BytesPerSector ) - 
			pdli->PartitionEntry[0].StartingOffset.QuadPart;

	pdli->PartitionEntry[0].HiddenSectors = _hidden_sectors;
	pdli->PartitionEntry[0].PartitionNumber = 1;

	pdli->PartitionEntry[0].PartitionType = PARTITION_HUGE;  // >= 32MB partitions

	pdli->PartitionEntry[0].BootIndicator = 0;
	pdli->PartitionEntry[0].RecognizedPartition = TRUE;
	pdli->PartitionEntry[0].RewritePartition = TRUE;
	
	for (DWORD i = 1; i < pdli->PartitionCount; ++i)
	{
		pdli->PartitionEntry[i].StartingOffset.QuadPart  = 0;
		pdli->PartitionEntry[i].PartitionLength.QuadPart = 0;
		pdli->PartitionEntry[i].HiddenSectors = 0;
		pdli->PartitionEntry[i].PartitionNumber = i+1;   // partition numbers are 1-based
		pdli->PartitionEntry[i].PartitionType = PARTITION_ENTRY_UNUSED;
		pdli->PartitionEntry[i].BootIndicator = 0;
		pdli->PartitionEntry[i].RecognizedPartition = FALSE;
		pdli->PartitionEntry[i].RewritePartition = TRUE;
	}

	BOOL result = CStGlobals::DeviceIoControl(
			m_handle,
			IOCTL_DISK_DELETE_DRIVE_LAYOUT,
			NULL, 0,
			NULL, 0,
			&dwBytesReturned,
			NULL
		);

/*	if( !result )
	{
		m_system_last_error = CStGlobals::GetLastError();
		m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;
		FreePartitionInfo( pdli );
		return STERR_FAILED_DEVICE_IO_CONTROL;
	}

*/	result = CStGlobals::DeviceIoControl(
			m_handle,
			IOCTL_DISK_SET_DRIVE_LAYOUT,
			pdli, CalcPartitionInfoSizeBytes(4),
			NULL, 0,
			&dwBytesReturned,
			NULL
		);

	FreePartitionInfo( pdli );

	if( result )
	{
		return STERR_NONE;
	}

	m_system_last_error = CStGlobals::GetLastError();
	m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
	GetUpdater()->GetErrorObject()->SaveStatus(this);
	m_system_last_error = 0;
	m_last_error = STERR_NONE;

	return STERR_FAILED_DEVICE_IO_CONTROL;
}

/*-----------------------------------------------------------------------------
AllocPartitionInfo (numPartitions)

Allocates free store to hold a DRIVE_LAYOUT_INFORMATION structure and array
of PARTITION_INFORMATION structures big enough for the number of partitions
specified.

Parameters:
   numPartitons
      Number of partitions that the structure must be able to contain.  The
      array of PARTITION_INFORMATION_STRUCTURES will contain one record for
      each partition.

Return Value:
   Returns a pointer to the memory block if successful.  Returns NULL if
   it could not allocate the block.

Notes:
   Use FreePartitionInfo to free the memory block when finished with it.
-----------------------------------------------------------------------------*/
PDRIVE_LAYOUT_INFORMATION CStScsi_Nt::AllocPartitionInfo (int numPartitions)
{
   DWORD dwBufSize;
   PDRIVE_LAYOUT_INFORMATION pdli;

   dwBufSize = CalcPartitionInfoSizeBytes (numPartitions);

   pdli = ((PDRIVE_LAYOUT_INFORMATION) LocalAlloc (LPTR, dwBufSize));

   if (pdli != NULL)
   {
      pdli->PartitionCount = numPartitions;
      pdli->Signature = 0;
   }

   return pdli;
}


/*-----------------------------------------------------------------------------
FreePartitionInfo (pdli)

Call this to free the memory allocated by AllocPartitionInfo and
GetPartitionInfo.

Return Value:
   Returns TRUE if partition information was freed; FALSE otherwise.

-----------------------------------------------------------------------------*/
BOOL CStScsi_Nt::FreePartitionInfo (PDRIVE_LAYOUT_INFORMATION pdli)
{
   return (LocalFree(pdli) == NULL);
}

/*-----------------------------------------------------------------------------
CalcPartitionInfoSizeBytes (hDrive)
-----------------------------------------------------------------------------*/
DWORD CStScsi_Nt::CalcPartitionInfoSizeBytes (int numPartitions)
{
   DWORD dwBufSize;

   dwBufSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
               sizeof(PARTITION_INFORMATION) * (numPartitions -1);

   return dwBufSize;
}

ST_ERROR CStScsi_Nt::ReadGeometry( PDISK_GEOMETRY _p_dg )
{
	DWORD BytesReturned;
	if( !DeviceIoControl( m_handle,
			IOCTL_DISK_GET_DRIVE_GEOMETRY,
			NULL,
			0,
			_p_dg,
			sizeof(DISK_GEOMETRY),
			&BytesReturned,
			NULL ) )
	{
		m_system_last_error = CStGlobals::GetLastError();
		m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;
		return STERR_FAILED_DEVICE_IO_CONTROL;
	}

	return STERR_NONE;
}

ST_ERROR CStScsi_Nt::DriveLayout( PDRIVE_LAYOUT_INFORMATION _p_dl )
{
	DWORD BytesReturned;
	if( !DeviceIoControl( m_handle,
			IOCTL_DISK_GET_DRIVE_LAYOUT,
			NULL,
			0,
			_p_dl,
			sizeof(DRIVE_LAYOUT_INFORMATION) + ( sizeof(PARTITION_INFORMATION) * 4 ) ,
			&BytesReturned,
			NULL ) )
	{
		m_system_last_error = CStGlobals::GetLastError();
		m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;
		return STERR_FAILED_DEVICE_IO_CONTROL;
	}

	return STERR_NONE;
}

ST_ERROR CStScsi_Nt::OpenPhysicalDrive(USHORT driveToFind, USHORT UpgradeOrNormal)
{
	wchar_t			DriveToOpen[32];

	INQUIRYDATA		InquiryData;
	wstring			w_strSCSIProductString;
	string			strSCSIProductString;
	wstring			w_strSCSIMfgString;
	string			strSCSIMfgString;
	CStScsiInquiry	api_scsi_inquiry;
	BOOL			drive_opened = FALSE;
	USHORT			driveFound = 0;


    if (UpgradeOrNormal == NORMAL_MSC)
    {
	    GetUpdater()->GetConfigInfo()->GetSCSIMfgString(w_strSCSIMfgString);
	    GetUpdater()->GetConfigInfo()->GetSCSIProductString(w_strSCSIProductString);

		USES_CONVERSION;
		strSCSIMfgString = W2A(w_strSCSIMfgString.c_str());
		strSCSIProductString = W2A(w_strSCSIProductString.c_str());
    }
    else
    {
        strSCSIMfgString = GENERIC_UPGRADE_MFG_STRING;
        strSCSIProductString = GENERIC_UPGRADE_PRODUCT_STRING;
    }


	if( m_handle && (m_handle != INVALID_HANDLE_VALUE) )
	{
		CloseHandle( m_handle );
		m_handle = INVALID_HANDLE_VALUE;
	}

	for( int drive = 0; drive < 100; drive ++ )
	{
		
		swprintf( DriveToOpen, 32, L"\\\\.\\PhysicalDrive%d", drive );
		
		m_handle = CStGlobals::CreateFile(
            DriveToOpen,						// device interface name
            GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                               // lpSecurityAttributes
            OPEN_EXISTING,                      // dwCreationDistribution
            0,                                  // dwFlagsAndAttributes
            NULL                                // hTemplateFile
        );
                
		if( m_handle == INVALID_HANDLE_VALUE ) 
		{
			//CStTrace::trace( "CreateFile failed with error: %d\n", GetLastError() );
			continue;
		}
		if( SendDdiApiCommand(&api_scsi_inquiry) == STERR_NONE )
		{
			api_scsi_inquiry.GetInquiryData(&InquiryData);
		
			if (InquiryData.DeviceType != DIRECT_ACCESS_DEVICE)
			{
                CloseHandle( m_handle );
				m_handle = INVALID_HANDLE_VALUE;
				continue;
			}

			ST_BOOLEAN product_id_match = FALSE;
            if ( GetUpdater()->GetConfigInfo()->UseScsiProductSubstringQualifier() ) {
                size_t index = strSCSIProductString.find_last_not_of(' ');
                strSCSIProductString.resize(index+1);
                product_id_match = strstr( (const char*)InquiryData.ProductId, strSCSIProductString.c_str() ) != NULL;
            }
            else {
                product_id_match = !_strnicmp( (const char*)InquiryData.ProductId, strSCSIProductString.c_str(), strSCSIProductString.length () );
            }

            if( product_id_match )		
			{
				if( !_strnicmp( (const char*)InquiryData.VendorId, strSCSIMfgString.c_str(), strSCSIMfgString.length () ) )		
				{
					// Is this the drive we want?  
					if (driveFound != driveToFind)
					{
						CloseHandle( m_handle );  // no, continue
						m_handle = INVALID_HANDLE_VALUE;
						++driveFound;
						continue;
					}


					drive_opened = TRUE;
					break; //stop
				}
			}
		}
		if( m_handle && (m_handle != INVALID_HANDLE_VALUE) )
		{
			CloseHandle( m_handle );
			m_handle = INVALID_HANDLE_VALUE;
		}
	}	

	if ( !drive_opened )
	{
		return STERR_FAILED_TO_LOCATE_SCSI_DEVICE;
	}
	return STERR_NONE;
}

//////////////////////////////////////////////////////////////////////////
//
// To address large media we must use the SetFilePointerEx()API.  However,
// this API is not available on Win98 or WinMe.  If we call and link to the
// API normally, the application fails to load on those platforms.  So, we
// make the call dynamically.
//
//////////////////////////////////////////////////////////////////////////

typedef BOOL (WINAPI* WinFunc)(HANDLE, LARGE_INTEGER,PLARGE_INTEGER,DWORD);

ST_ERROR CStScsi_Nt::WriteSector( CStByteArray* _p_sector, ULONG _num_sectors, ULONG _start_sector_number, 
								ULONG _sector_size )
{
	ST_ERROR result=STERR_NONE;
	DWORD bytes_written=0;
    WinFunc MySetFilePointerEx;

	
    LARGE_INTEGER l_start_sector, position;
    l_start_sector.QuadPart = _start_sector_number * _sector_size;

    MySetFilePointerEx = (WinFunc)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "SetFilePointerEx");
    if (!MySetFilePointerEx)
    {
		return STERR_FAILED_TO_WRITE_SECTOR;
    }
	// position it to the correct place for reading
    BOOL success = (MySetFilePointerEx)( m_handle, l_start_sector, &position, FILE_BEGIN );
	if( ( !success ) && ( CStGlobals::GetLastError() != NO_ERROR ) )
	{
		m_system_last_error = CStGlobals::GetLastError();
		m_last_error = STERR_FAILED_TO_WRITE_SECTOR;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;

		result = STERR_FAILED_TO_WRITE_SECTOR;
		return result;
	}
	
	PUCHAR buf = new UCHAR[ _num_sectors * _sector_size ];
	
	_p_sector->Read( (void*)buf, _num_sectors * _sector_size, 0 );

	if( !WriteFile( m_handle, buf, (DWORD)( _num_sectors * _sector_size ), &bytes_written, NULL ) )
	{
		m_system_last_error = CStGlobals::GetLastError();
		m_last_error = STERR_FAILED_TO_WRITE_SECTOR;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;

		result = STERR_FAILED_TO_WRITE_SECTOR;
	}
	
	delete[] buf;

	return result;
}

ST_ERROR CStScsi_Nt::ReadSector( CStByteArray* _p_sector, ULONG _num_sectors, ULONG _start_sector_number, 
								ULONG _sector_size )
{
	ST_ERROR result=STERR_NONE;
	DWORD bytes_read=0;
    WinFunc MySetFilePointerEx;

    LARGE_INTEGER l_start_sector, position;
    l_start_sector.QuadPart = _start_sector_number * _sector_size;

    MySetFilePointerEx = (WinFunc)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "SetFilePointerEx");
    if (!MySetFilePointerEx)
    {
		return STERR_FAILED_TO_WRITE_SECTOR;
    }

	// position it to the correct place for reading
    BOOL success = (MySetFilePointerEx)( m_handle, l_start_sector, &position, FILE_BEGIN );
	if( ( !success ) && ( CStGlobals::GetLastError() != NO_ERROR ) )
	{
		m_system_last_error = CStGlobals::GetLastError();
		m_last_error = STERR_FAILED_TO_READ_SECTOR;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;

		result = STERR_FAILED_TO_READ_SECTOR;
		return result;
	}
	
	PUCHAR buf = new UCHAR[ _num_sectors * _sector_size ];
	
	memset( buf, 0, _num_sectors * _sector_size );
	
	if( !ReadFile( m_handle, buf, (DWORD)(_num_sectors * _sector_size), &bytes_read, NULL ) )
	{
		m_system_last_error = CStGlobals::GetLastError();
		m_last_error = STERR_FAILED_TO_READ_SECTOR;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;

		result = STERR_FAILED_TO_READ_SECTOR;
	}
	
	if( result == STERR_NONE )
	{
		_p_sector->Write( (void*)buf, _num_sectors * _sector_size, 0 );
	}

	delete[] buf;

	return result;
}


#include <setupapi.h>
#include <usbiodef.h>
#include <devguid.h>
#include <cfgmgr32.h>
BOOL CStScsi_Nt::UsbIdsMatch(wchar_t drive, USHORT vid, USHORT pid)
{
	// Get Volume path for drive letter.
	CString drive_letter, volume_path, device_path;
	drive_letter.Format(_T("%c:\\"), drive);
	if (!::GetVolumeNameForVolumeMountPoint(drive_letter, volume_path.GetBufferSetLength(128), 128))
		return FALSE;
	volume_path.ReleaseBuffer();

	// Get the list of volumes.
	HDEVINFO volumeDevInfoSet = ::SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_VOLUME, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if ( volumeDevInfoSet == INVALID_HANDLE_VALUE )
		return FALSE;
	
	// Enumerate the volumes.
	BOOL success = TRUE;
	SP_DEVICE_INTERFACE_DATA interfaceData;
	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	
	for(DWORD volumeIndex = 0; success == TRUE; ++volumeIndex)
    {
		success = ::SetupDiEnumDeviceInterfaces(volumeDevInfoSet, NULL, (LPGUID)&GUID_DEVINTERFACE_VOLUME, volumeIndex, &interfaceData);
		if (!success)
			continue;

		// Get the path for the volume.
		DWORD requiredSize, detailSize = 0x400;
		PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(detailSize);
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		SP_DEVINFO_DATA volumeDevInfoData;
		volumeDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (! ::SetupDiGetDeviceInterfaceDetail(volumeDevInfoSet, &interfaceData, detailData, detailSize, &requiredSize, &volumeDevInfoData))
		{
			free(detailData);
			continue;
		}

		CString devPath = (PTSTR)detailData->DevicePath;
		devPath.AppendChar(_T('\\'));

		// Get the Volume for the volume path.
		if (! ::GetVolumeNameForVolumeMountPoint(devPath, device_path.GetBufferSetLength(128), 128))
		{
			free(detailData);
			continue;
		}
		device_path.ReleaseBuffer();

		// See if it matches the Volume that we got using the drive letter.
		if ( device_path.CompareNoCase(volume_path) != 0 )
		{
			free(detailData);
			continue;
		}

		// Get USBSTOR devnode from Volume devnode.
		DWORD diskDevInst;
		DWORD error = CM_Get_Parent(&diskDevInst, volumeDevInfoData.DevInst, 0);
		if (error != CR_SUCCESS)
		{
			free(detailData);
			success = FALSE;
			break;
		}

		// Get USB devnode from USBSTOR devnode.
		DWORD usbDevInst;
		error = CM_Get_Parent(&usbDevInst, diskDevInst, 0);
		if (error != CR_SUCCESS)
		{
			free(detailData);
			success = FALSE;
			break;
		}

		// Get the DeviceID string from the USB device devnode.
		TCHAR buf[4096];
		CONFIGRET cr = CM_Get_Device_ID(usbDevInst, buf, sizeof(buf), 0);
		CString matchString, devIdString = buf;

		// See if the USB DeviceID string contains our players VID/PID.
		matchString.Format(_T("vid_%04x&pid_%04x"), vid, pid);
		if (devIdString.MakeLower().Find(matchString) != -1 )
		{
			free(detailData);
			success = TRUE;
			break;
		}

	} // end for (each volume)

	::SetupDiDestroyDeviceInfoList(volumeDevInfoSet);

	return success;
}


// usbstor#disk&ven_sigmatel&prod_sdk_device&rev_

BOOL CStScsi_Nt::UsbIdsMatch(CString scsiManufacturer, CString scsiProduct, USHORT vid, USHORT pid)
{
	// Build the SCSI ID search string for the USBSTOR disk
	CString usbstorPath, device_path;
	usbstorPath.Format(_T("usbstor#disk&ven_%s&prod_%s&rev_"), scsiManufacturer.Trim(), scsiProduct.Trim());
	usbstorPath.Replace(_T(' '), _T('_'));
	usbstorPath.MakeLower();

	// Get the list of disks.
	HDEVINFO diskDevInfoSet = ::SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if ( diskDevInfoSet == INVALID_HANDLE_VALUE )
		return FALSE;
	
	// Enumerate the disks.
	BOOL success = TRUE;
	SP_DEVICE_INTERFACE_DATA interfaceData;
	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	
	for(DWORD volumeIndex = 0;  success==TRUE; ++volumeIndex)
    {
		success = ::SetupDiEnumDeviceInterfaces(diskDevInfoSet, NULL, (LPGUID)&GUID_DEVINTERFACE_DISK, volumeIndex, &interfaceData);
		if (!success)
			continue;

		// Get the path for the disk.
		DWORD requiredSize, detailSize = 0x400;
		PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(detailSize);
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		SP_DEVINFO_DATA diskDevInfoData;
		diskDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (! ::SetupDiGetDeviceInterfaceDetail(diskDevInfoSet, &interfaceData, detailData, detailSize, &requiredSize, &diskDevInfoData))
		{
			free(detailData);
			continue;
		}
		CString devPath = (PTSTR)detailData->DevicePath;

		// See if our USBSTOR search string is contained in the disk path.
		if ( devPath.MakeLower().Find(usbstorPath) == -1 )
		{
			free(detailData);
			continue;
		}

		// Get USB devnode from USBSTOR devnode.
		DWORD usbDevInst;
		CONFIGRET cr = CM_Get_Parent(&usbDevInst, diskDevInfoData.DevInst, 0);
		if (cr != CR_SUCCESS)
		{
			free(detailData);
			success = FALSE;
			break;
		}

		// Get the DeviceID string from the USB device devnode.
		TCHAR buf[4096];
		cr = CM_Get_Device_ID(usbDevInst, buf, sizeof(buf), 0);
		CString matchString, devIdString = buf;

		// See if the USB DeviceID string contains our players VID/PID.
		matchString.Format(_T("vid_%04x&pid_%04x"), vid, pid);
		if (devIdString.MakeLower().Find(matchString) != -1 )
		{
			free(detailData);
			success = TRUE;
			break;
		}

	} // end for (each volume)

	::SetupDiDestroyDeviceInfoList(diskDevInfoSet);

	return success;
}
