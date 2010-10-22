/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StScsi_9x.cpp: implementation of the CStScsi_9x class.
//
//////////////////////////////////////////////////////////////////////

#include "stheader.h"
#include "StGlobals.h"
#include "ddildl_defs.h"
#include "StByteArray.h"
#include "StVersionInfo.h"
#include "StDdiApi.h"
#include "StConfigInfo.h"
#include "StUpdater.h"
#include "StProgress.h"
#include <scsidefs.h>
#include <wnaspi32.h>
#include "StTrace.h"
#include "StRegistry.h"
#include "StScsi_9x.h"

// DeviceIoControl infrastructure
#if !defined (VWIN32_DIOC_DOS_IOCTL)
#define VWIN32_DIOC_DOS_IOCTL		1
#define VWIN32_DIOC_DOS_INT13		4
#define VWIN32_DIOC_DOS_INT25		2
#define VWIN32_DIOC_DOS_INT26		3
#define VWIN32_DIOC_DOS_DRIVEINFO	6

typedef struct _DIOC_REGISTERS {
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS, *PDIOC_REGISTERS;

#define VWIN32_DIOC_DOS_IOCTL      1

#endif //#if !defined (VWIN32_DIOC_DOS_IOCTL)

#define CARRY_FLAG             0x0001

#pragma pack(1)

HINSTANCE				g_handle_dll_wnaspi32 = 0;

typedef struct _DISKIO {
  DWORD  dwStartSector;   // starting logical sector number
  WORD   wSectors;        // number of sectors
  DWORD  dwBuffer;        // address of read/write buffer
} DISKIO, * PDISKIO;

typedef struct _DOSDPB {
    BYTE    specialFunc;    // 
    BYTE    devType;        // 
    WORD    devAttr;        // 
    WORD    cCyl;           // number of cylinders
    BYTE    mediaType;      // 
    WORD    cbSec;          // Bytes per sector
    BYTE    secPerClus;     // Sectors per cluster
    WORD    cSecRes;        // Reserved sectors
    BYTE    cFAT;           // FATs
    WORD    cDir;           // Root Directory Entries
    WORD    cSec;           // Total number of sectors in image
    BYTE    bMedia;         // Media descriptor
    WORD    secPerFAT;      // Sectors per FAT
    WORD    secPerTrack;    // Sectors per track
    WORD    cHead;          // Heads
    DWORD   cSecHidden;     // Hidden sectors
    DWORD   cTotalSectors;  // Total sectors, if cbSec is zero
    BYTE    reserved[6];    // 
} DOSDPB, *PDOSDPB;

#pragma pack()

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStScsi_9x::CStScsi_9x(CStUpdater *_p_updater, string _name):CStScsi(_p_updater, _name)
{
	DWORD			ASPI32Status;

	m_adapter_id = 0xFF;
    m_lun = 0;

    if (g_handle_dll_wnaspi32 == 0)
    {
    	g_handle_dll_wnaspi32 = LoadLibraryW ( L"WNASPI32.DLL" );

	    if ( g_handle_dll_wnaspi32 == 0 )
    	{
	    	m_last_error = STERR_FAILED_TO_LOAD_WNASPI32;
		    return;
        }
    }

   	m_handle_dll_wnaspi32 = g_handle_dll_wnaspi32; //LoadLibraryW ( L"WNASPI32.DLL" );
   // load the address of GetASPI32SupportInfo
    m_p_fn_get_aspi32_support_info = ( DWORD ( __cdecl *)( void ) )
    GetProcAddress ( m_handle_dll_wnaspi32, "GetASPI32SupportInfo" );
    if ( m_p_fn_get_aspi32_support_info == NULL )
    {
        m_last_error = STERR_FAILED_TO_GET_FUNCTION_PTR_IN_WNASPI32_DLL;
        return;
    }

    // load the address of SendASPI32Command
    m_p_fn_send_aspi32_command = ( DWORD ( __cdecl *)( LPSRB psrb ) )
    GetProcAddress ( m_handle_dll_wnaspi32, "SendASPI32Command" );
    if ( m_p_fn_send_aspi32_command == NULL )
    {
	    m_last_error = STERR_FAILED_TO_GET_FUNCTION_PTR_IN_WNASPI32_DLL;
        return;
    }

    m_last_error = STERR_NONE;

   	ASPI32Status = m_p_fn_get_aspi32_support_info();
	
    switch (HIBYTE(LOWORD(ASPI32Status)))
   	{
        case SS_COMP:
    //		Count = (LOBYTE(LOWORD(ASPI32Status)))+1;
	    	break;
    	default:
	    	m_last_error = STERR_STATE_OF_WNASPI32_NOT_INITIALIZED;	
    }

	m_state = SCSI_STATE_UNINITIALIZED;
}

CStScsi_9x::~CStScsi_9x()
{
	m_p_fn_get_aspi32_support_info = NULL;
	m_p_fn_send_aspi32_command = NULL;
	//FreeLibrary (m_handle_dll_wnaspi32);
}

ST_ERROR CStScsi_9x::Initialize(USHORT driveToFind, USHORT UpgradeOrNormal)
{
	if(!m_p_fn_get_aspi32_support_info)
	{
		return STERR_STATE_OF_WNASPI32_NOT_INITIALIZED;
	}
	ST_ERROR		err = STERR_NONE;
	INQUIRYDATA		InquiryData;
	string			strRegKey;
	BYTE			Count = 0;
	BYTE			FirstRevisionChar = 0;
	wstring			strSCSIProductString;
	wstring			strSCSIMfgString;
	CStScsiInquiry	api_scsi_inquiry;
	wstring			drive_letters;
	BOOL			found = FALSE;
	UCHAR			phys_drive_number=0xFF;
	size_t			num_spaces=0;

	err = GetNumAdapters(Count);
	if( err != STERR_NONE )
		return err;

	m_drive_letter = L'\0';

    if (UpgradeOrNormal == NORMAL_MSC)
    {
	    GetUpdater()->GetConfigInfo()->GetSCSIMfgString(strSCSIMfgString);
	    GetUpdater()->GetConfigInfo()->GetSCSIProductString(strSCSIProductString);
    }
    else
    {
        strSCSIMfgString = (LPCTSTR)GENERIC_UPGRADE_MFG_STRING;
        strSCSIProductString = (LPCTSTR)GENERIC_UPGRADE_PRODUCT_STRING;
    }
	
	for( UCHAR HaId=0; HaId<Count; HaId++ )
	{
		memset(&InquiryData, 0, sizeof(InquiryData));
		m_adapter_id = HaId;

        // DD
        m_lun = (UCHAR) driveToFind;

		if (SendDdiApiCommand(&api_scsi_inquiry) == STERR_NONE)
		{
			api_scsi_inquiry.GetInquiryData(&InquiryData);
            //CHAR umsg[100];
            //sprintf (umsg, "On HaId %d we find %s %s", HaId, strSCSIMfgString.c_str(), strSCSIProductString.c_str());
            //MessageBoxA(  NULL, umsg, "test", MB_OK);					

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
				if( !_strnicmp( (const char*)InquiryData.VendorId, strSCSIMfgString.c_str(), strSCSIMfgString.length() ) )		
				{
					ST_BOOLEAN media_system; 
					ST_ERROR err=STERR_NONE;
					
					m_adapter_id = HaId; //this assignment is important before calling IsSystemMedia().

                    // Check if the device is hostlink in limited MSC mode
                    // If so, we get out returning the limited support error.
                    err = GetProtocolVersionMajor( m_ProtocolVersion );

                    if (m_ProtocolVersion == ST_LIMITED_HOSTLINK)
                    {
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
						//m_lun = ???;
						found = TRUE;
						FirstRevisionChar = InquiryData.ProductRevisionLevel[0];
						break;
					}
					else
					{
						m_adapter_id = 0xFF;
					}
				}
			}
		}
	}	
	
	if (!found)
	{
#ifdef DEBUG
		// at least log this to debug output.						
		_RPT1(_CRT_WARN,"CStScsi_9x::Initialize() Didn't find SCSI device.(1)\n",0);
#endif
		m_state = SCSI_STATE_UNINITIALIZED;
		return STERR_FAILED_TO_LOCATE_SCSI_DEVICE;
	}

	if( GetPhysicalDriveNumber(m_adapter_id, phys_drive_number) == STERR_NONE )
	{
		err = PhysicalDriveNumberToLogicalDriveLetter(phys_drive_number, m_drive_letter);
		if( err == STERR_NONE )
		{
#ifdef DEBUG
		// at least log this to debug output.						
		_RPT1(_CRT_WARN,"CStScsi_9x::Initialize() Didn't find SCSI device.(2) err=%d\n", err);
#endif
			m_state = SCSI_STATE_INITIALIZED;
			return STERR_NONE;
		}
	}

	strRegKey = strSCSIMfgString;
	if ( (num_spaces = 8 - strRegKey.length()) > 0 )
		strRegKey.append(num_spaces, ' ');
	strRegKey.append(strSCSIProductString);
	if ( (num_spaces = 8 + 16 - strRegKey.length()) > 0 )
		strRegKey.append(num_spaces, ' ');
	strRegKey.append(1, FirstRevisionChar);
	
	CStGlobals::SpacesToUnderScores(strRegKey);

	CStRegistry::FindDriveLettersForScsiDevice(strRegKey, drive_letters);
	
	err = GuessTheRightDriveLetter( drive_letters, m_drive_letter );
	if( err != STERR_NONE )
	{
#ifdef DEBUG
		// at least log this to debug output.						
		_RPT1(_CRT_WARN,"CStScsi_9x::Initialize() Didn't find SCSI device.(3) err=%d\n", err);
#endif
		return err;
	}
	m_state = SCSI_STATE_INITIALIZED;
	return STERR_NONE;
}

ST_ERROR CStScsi_9x::SendCommand( 
	CStByteArray* _p_command_arr, 
	UCHAR _cdb_len, 
	BOOL _dir_out, 
	CStByteArray& _response_arr,
    ULONG ulTimeOut
)
{
    DWORD               aspi_status;
	SRB_ExecSCSICmd		srb;			
	PUCHAR				p_uchar;
	size_t				data_length;
	HANDLE				hEvent=NULL;
	size_t				index;

	data_length = (size_t)CStGlobals::Max( ( _p_command_arr->GetCount() - _cdb_len ), 
		_response_arr.GetCount());

	p_uchar = new UCHAR[data_length];
	if(!p_uchar)
	{
		return STERR_NO_MEMORY;
	}

	CStGlobals::MakeMemoryZero(p_uchar, data_length);
 
	//
	// if command contains data copy it to the output buffer.
	//
	for( index = _cdb_len; index < _p_command_arr->GetCount(); index ++ )
	{
		_p_command_arr->GetAt(index, p_uchar[index - _cdb_len]);
	}

	hEvent = CStGlobals::CreateEvent( NULL, TRUE, FALSE, NULL );
	if( hEvent == NULL )
    {
		m_system_last_error = CStGlobals::GetLastError();
		m_last_error = STERR_FAILED_TO_CREATE_EVENT_OBJECT;
		GetUpdater()->GetErrorObject()->SaveStatus(this);
		m_system_last_error = 0;
		m_last_error = STERR_NONE;
		delete[] p_uchar;
		return STERR_FAILED_TO_CREATE_EVENT_OBJECT;
    }

	memset(&srb, 0, sizeof(SRB_ExecSCSICmd));

	srb.SRB_Lun         = m_lun;
	srb.SRB_Cmd			= SC_EXEC_SCSI_CMD;
	srb.SRB_HaId		= m_adapter_id;
	srb.SRB_Flags		= _dir_out ? SRB_DIR_OUT : SRB_DIR_IN;
	srb.SRB_Flags		|= SRB_EVENT_NOTIFY;
	srb.SRB_BufPointer	= (unsigned char*)p_uchar;
	srb.SRB_BufLen		= (DWORD)data_length;
	srb.SRB_SenseLen	= SENSE_LEN;
	srb.SRB_CDBLen		= _cdb_len;
	srb.SRB_PostProc	= (LPVOID) hEvent;

	//
	// copy the command to cdb array
	//
	for( index = 0; index < _cdb_len; index ++ )
	{
		_p_command_arr->GetAt(index, srb.CDBByte[index]);
	}

	ResetEvent( hEvent );

    if( ( aspi_status = m_p_fn_send_aspi32_command( (void*) &srb ) ) == SS_PENDING )
	{
        aspi_status = CStGlobals::WaitForSingleObjectEx( hEvent, 0, FALSE );
		while( aspi_status == WAIT_TIMEOUT )
		{	
			if( GetUpdater()->GetProgress() )
			{
				GetUpdater()->GetProgress()->UpdateProgress(FALSE);
			}
			aspi_status = CStGlobals::WaitForSingleObjectEx( hEvent, 0, FALSE );
		}	
		if(aspi_status == WAIT_OBJECT_0)
		{
			aspi_status = ERROR_SUCCESS;
		}
	}
    
	if (srb.SRB_Status != SS_COMP)
	{
		aspi_status = !ERROR_SUCCESS; // DeviceTypeQualifier == 0x01 means the external disk is removed
		m_system_last_error = srb.SRB_Status;
		m_last_error = STERR_FAILED_TO_SEND_SCSI_COMMAND;
		SaveSenseData(srb.SenseArea, srb.SRB_SenseLen);
		GetUpdater()->GetErrorObject()->SaveStatus(this, GetSenseData());
		m_system_last_error = 0;
		m_last_error = STERR_NONE;
	}
	else
	{
		for(index = 0; index < _response_arr.GetCount(); index ++ )
		{
			_response_arr.SetAt(index, p_uchar[index]);
		}
	}

	CloseHandle( hEvent );
	delete[] p_uchar;

	if (aspi_status == ERROR_SUCCESS)
		return STERR_NONE;
	return STERR_FAILED_TO_SEND_SCSI_COMMAND;
    UNREFERENCED_PARAMETER(ulTimeOut);
}

ST_ERROR CStScsi_9x::Open()
{
	ST_ERROR err = STERR_NONE;

	if( m_handle == INVALID_HANDLE_VALUE )
	{
		m_handle = CStGlobals::CreateFile (L"\\\\.\\vwin32", 0, 0, NULL, 0,
                      FILE_FLAG_DELETE_ON_CLOSE, NULL);

		if( m_handle == INVALID_HANDLE_VALUE ) 
		{
			m_system_last_error = CStGlobals::GetLastError();
			err = STERR_INVALID_DEVICE_HANDLE;
		}
	}
	return err;
}

ST_ERROR CStScsi_9x::Close()
{
	CloseHandle(m_handle);
	m_handle = INVALID_HANDLE_VALUE;
	return STERR_NONE;
}

/******************************************************************************** 
Purpose:
   Takes a logical volume lock on a logical volume.

Local variables:
   hVWin32
      An open handle to VWIN32.

   bDriveNum
      The logical drive number to lock. 0 = default, 1 = A:, 2 = B:,
      3 = C:, etc.

   bLockLevel
      Can be 0, 1, 2, or 3. Level 0 is an exclusive lock that can only
      be taken when there are no open files on the specified drive.
      Levels 1 through 3 form a hierarchy where 1 must be taken before
      2, which must be taken before 3.

   _permissions
      Specifies how the lock will affect file operations when lock levels
      1 through 3 are taken. Also specifies whether a formatting lock
      should be taken after a level 0 lock.

      Zero is a valid permission.

Return Value:
   If successful, returns TRUE.  If unsuccessful, returns FALSE.
*********************************************************************************/ 
ST_ERROR CStScsi_9x::Lock(WORD _permissions, UCHAR _lock_type)
{
   BOOL				fResult;
   DIOC_REGISTERS	regs = {0};
   BYTE				bDeviceCat;  // can be either 0x48 or 0x08
   BYTE				bDriveNum = (UCHAR)(( GetDriveLetter() - L'A' ) + 1);
   BYTE				bLockLevel = 0;
   DWORD			cb;

   /*
      Try first with device category 0x48 for FAT32 volumes. If it
      doesn't work, try again with device category 0x08. If that
      doesn't work, then the lock failed.
   */ 

   bDeviceCat = 0x48;

ATTEMPT_AGAIN:
   // Set up the parameters for the call.
   regs.reg_EAX = 0x440D;
   regs.reg_EBX = MAKEWORD(bDriveNum, bLockLevel);
   regs.reg_ECX = MAKEWORD(_lock_type, bDeviceCat);
   regs.reg_EDX = _permissions;

   fResult = CStGlobals::DeviceIoControl (m_handle, VWIN32_DIOC_DOS_IOCTL,
                              &regs, sizeof(regs), &regs, sizeof(regs),
                              &cb, 0);

   // See if DeviceIoControl and the lock succeeded
   fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);

   // If DeviceIoControl or the lock failed, and device category 0x08
   // hasn't been tried, retry the operation with device category 0x08.
   if (!fResult && (bDeviceCat != 0x08))
   {
      bDeviceCat = 0x08;
      goto ATTEMPT_AGAIN;
   }

	if(!fResult)
	{
		return STERR_FAILED_TO_LOCK_THE_DRIVE;
	}
	return STERR_NONE;
}

/*******************************************************************************
Purpose:
   Unlocks a logical volume that was locked with Lock().

Local variables:
   m_handle
      An open handle to VWIN32.

   bDriveNum
      The logical drive number to unlock. 0 = default, 1 = A:, 2 = B:,
      3 = C:, etc.

Return Value:
   If successful, returns TRUE. If unsuccessful, returns FALSE.

Comments:
   Must be called the same number of times as LockLogicalVolume() to
   completely unlock a volume.

   Only the lock owner can unlock a volume.
********************************************************************************/
ST_ERROR CStScsi_9x::Unlock(BOOL _media_new)
{
	BOOL				fResult;
	DIOC_REGISTERS	regs = {0};
	BYTE				bDeviceCat;  // can be either 0x48 or 0x08
	BYTE				bDriveNum = (UCHAR)(( GetDriveLetter() - L'A' ) + 1);
	DWORD			cb;

	/* Try first with device category 0x48 for FAT32 volumes. If it
		doesn't work, try again with device category 0x08. If that
		doesn't work, then the unlock failed.
	*/ 

	bDeviceCat = 0x48;

	ATTEMPT_AGAIN:
	// Set up the parameters for the call.
	regs.reg_EAX = 0x440D;
	regs.reg_EBX = bDriveNum;
	if( _media_new )
		regs.reg_ECX = MAKEWORD(0x6B, bDeviceCat);
	else
		regs.reg_ECX = MAKEWORD(0x6A, bDeviceCat);
	fResult = CStGlobals::DeviceIoControl (m_handle, VWIN32_DIOC_DOS_IOCTL,
		&regs, sizeof(regs), &regs, sizeof(regs),
		&cb, 0);

	// See if DeviceIoControl and the unlock succeeded
	fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);

	// If DeviceIoControl or the unlock failed, and device category 0x08
	// hasn't been tried, retry the operation with device category 0x08.
	if (!fResult && (bDeviceCat != 0x08))
	{
		bDeviceCat = 0x08;
		goto ATTEMPT_AGAIN;
	}

	if(!fResult)
	{
		return STERR_FAILED_TO_UNLOCK_THE_DRIVE;
	}
	
	return STERR_NONE;
}

ST_ERROR CStScsi_9x::Dismount()
{
	return STERR_NONE;
}

ST_ERROR CStScsi_9x::Lock(BOOL _media_new)
{
	if( _media_new )
	{
		return AcquirePhysicalLock( 0 );
	}
	return AcquireLogicalLock( 0 );
}

ST_ERROR CStScsi_9x::AcquirePhysicalLock(WORD _permissions)
{
	return Lock(_permissions, 0x4B);
}

ST_ERROR CStScsi_9x::AcquireLogicalLock(WORD _permissions)
{
	return Lock(_permissions, 0x4A);
}

ST_ERROR CStScsi_9x::AcquireFormatLock(BOOL _media_new)
{
	if( _media_new )
		return AcquirePhysicalLock(4);
	return AcquireLogicalLock(4);
}

ST_ERROR CStScsi_9x::ReleaseFormatLock(BOOL _media_new)
{
	return Unlock(_media_new);
}

ST_ERROR CStScsi_9x::GetNumAdapters(UCHAR& _count)
{
	SRB_HAInquiry	srb;
    DWORD           aspi_status;

	memset(&srb, 0, sizeof(SRB_HAInquiry));

	srb.SRB_Cmd			= SC_HA_INQUIRY;

    if( ( aspi_status = m_p_fn_send_aspi32_command( (void*) &srb ) ) != SS_COMP )
	{
		_count = 0;
		return STERR_FAILED_TO_SEND_SCSI_COMMAND;
	}

	_count = srb.HA_Count;

	return STERR_NONE;
}

ST_ERROR CStScsi_9x::GetPhysicalDriveNumber(UCHAR _ha_id, UCHAR& _drive_number)
{
	SRB_GetDiskInfo srb_di;
    DWORD           aspi_status;

	memset(&srb_di, 0, sizeof(SRB_GetDiskInfo));

    // DD
	srb_di.SRB_Lun      = m_lun;
	srb_di.SRB_Cmd		= SC_GET_DISK_INFO;
	srb_di.SRB_HaId		= _ha_id;
	if( ( aspi_status = m_p_fn_send_aspi32_command( (void*) &srb_di ) ) != SS_COMP )
	{
		_drive_number = 0xFF;
		return STERR_FAILED_TO_SEND_SCSI_COMMAND;
	}

	if( srb_di.SRB_Int13HDriveInfo >= 0x80 )
	{
		_drive_number = srb_di.SRB_Int13HDriveInfo;
		return STERR_NONE;
	}

	_drive_number = 0xFF;

	return STERR_INVALID_DISK_INFO;
}

ST_ERROR CStScsi_9x::PhysicalDriveNumberToLogicalDriveLetter(UCHAR _phy_drive_num, wchar_t& _drive_letter)
{
	ST_ERROR err= STERR_NONE;

	err = Open();
	if( err != STERR_NONE )
		return err;

	for( wchar_t ch=L'A'; ch<=L'Z'; ch++ )
	{
		DRIVE_MAP_INFO map = {0};
		err = GetDriveMap(ch, map);
		if( err == STERR_NONE )
		{
			if( map.dmiInt13Unit == _phy_drive_num )
			{
				_drive_letter = ch;
				Close();
				return STERR_NONE;
			}
		}
	}

	Close();
	return err;
}

ST_ERROR CStScsi_9x::GetDriveMap(wchar_t _drive_letter, DRIVE_MAP_INFO& _map)
{
	BOOL				fResult;
	DIOC_REGISTERS	regs = {0};
	BYTE				bDeviceCat;  // can be either 0x48 or 0x08
	BYTE				bDriveNum = (UCHAR)(( _drive_letter - L'A' ) + 1);
	DWORD			cb;

	_map.dmiAllocationLength = sizeof( DRIVE_MAP_INFO );
	/* Try first with device category 0x48 for FAT32 volumes. If it
		doesn't work, try again with device category 0x08. If that
		doesn't work, then the call failed.
	*/ 

	bDeviceCat = 0x48;

	ATTEMPT_AGAIN:
	// Set up the parameters for the call.
	regs.reg_EAX = 0x440D;
	regs.reg_EBX = bDriveNum;
	regs.reg_ECX = MAKEWORD(0x6f, bDeviceCat);
	regs.reg_EDX = (DWORD)(ULONG_PTR)&_map;

	fResult = CStGlobals::DeviceIoControl (m_handle, VWIN32_DIOC_DOS_IOCTL,
		&regs, sizeof(regs), &regs, sizeof(regs),
		&cb, 0);

	// See if DeviceIoControl and the unlock succeeded
	fResult = fResult && !(regs.reg_Flags & CARRY_FLAG);

	// If DeviceIoControl failed, and device category 0x08
	// hasn't been tried, retry the operation with device category 0x08.
	if (!fResult && (bDeviceCat != 0x08))
	{
		bDeviceCat = 0x08;
		goto ATTEMPT_AGAIN;
	}

	if(!fResult)
	{
		return STERR_FAILED_TO_GET_DRIVE_MAP;
	}

	return STERR_NONE;
}

ST_ERROR CStScsi_9x::GuessTheRightDriveLetter( wstring drive_letters, wchar_t& _drive_letter )
{
	ST_ERROR err = STERR_NONE;

	if( drive_letters.length() <= 0 )
		return STERR_FAILED_TO_FIND_DRIVE_LETTER_IN_REGISTRY;

	if( drive_letters.length() == 1 )
	{
		//this is the only drive letter available so no further search
		_drive_letter = drive_letters[0];
		return STERR_NONE;
	}
	
	BOOL found = FALSE;
	
	err = Open();
	if( err != STERR_NONE )
		return err;

	for(size_t index=0; index<drive_letters.length(); index++)
	{
		DRIVE_MAP_INFO map;

		if( GetDriveMap( drive_letters[index], map ) != STERR_NONE )
		{
			//whatever state our device may be it should pass GetDriveMap. 
			//otherwise this is not the one we are looking for.
			continue;
		}

		if( map.dmiFlags == 0x0F && map.dmiInt13Unit == 0xFF )
		{
			//failed all the earlier detections, 
			//this drive cannot be a CDROM because FindDriveLettersForScsiDevice will filter it.
			//this drive cannot be floppy as dmiInt13Unit will be zero and dmiFlags will be 0x0F.
			//this drive cannot have a Int13Unit number as GetPhysicalDriveNumber has already failed.
			//this could be another device of same player connected in un-initialized state 
			//but presently we don't support it.
			//so we guess this drive is not initialized and this is the one we are searching for.
			_drive_letter = drive_letters[index];
			found = TRUE;
			break;
		}
	}

	Close();
	
	if ( found )
	{
		return STERR_NONE;
	}
	return STERR_FAILED_TO_FIND_DRIVE_LETTER_IN_REGISTRY;
}


ST_ERROR CStScsi_9x::ReadGeometry( PDISK_GEOMETRY _p_dg )
{
    DWORD			cb;
    DIOC_REGISTERS	reg;
    DOSDPB			dpb;
	BYTE			bDriveNum = (UCHAR)(( GetDriveLetter() - L'A' ) + 1);

    dpb.specialFunc = 0;  // return default type; do not hit disk

    reg.reg_EBX   = bDriveNum;       // BL = drive number (1-based)
    reg.reg_EDX   = (DWORD)(ULONG_PTR)&dpb;  // DS:EDX -> DPB
    reg.reg_ECX   = 0x0860;       // CX = Get DPB
    reg.reg_EAX   = 0x440D;       // AX = Ioctl
    reg.reg_Flags = CARRY_FLAG;   // assume failure

    // Make sure both DeviceIoControl and Int 21h succeeded.
    if (DeviceIoControl (m_handle, VWIN32_DIOC_DOS_IOCTL, &reg,
                        sizeof(reg), &reg, sizeof(reg),
                        &cb, 0)
        && !(reg.reg_Flags & CARRY_FLAG))
    {
		_p_dg->BytesPerSector		= dpb.cbSec;
		_p_dg->Cylinders.QuadPart	= dpb.cCyl;
		_p_dg->MediaType			= (MEDIA_TYPE)dpb.mediaType;
		_p_dg->SectorsPerTrack		= dpb.secPerTrack;
		_p_dg->TracksPerCylinder	= dpb.cHead;

		return STERR_NONE;
	}

	m_system_last_error = CStGlobals::GetLastError();
	m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
	GetUpdater()->GetErrorObject()->SaveStatus(this);
	m_system_last_error = 0;
	m_last_error = STERR_NONE;
	return STERR_FAILED_DEVICE_IO_CONTROL;
}
