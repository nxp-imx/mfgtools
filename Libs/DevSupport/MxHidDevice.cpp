#include "stdafx.h"
#include <Assert.h>
#include <cfgmgr32.h>
#include <basetyps.h>
#include <setupapi.h>
#include <initguid.h>
extern "C" {
#include "Libs/WDK/hidsdi.h"
}
#include "Libs/WDK/hidclass.h"
#include "Device.h"
#include "MxHidDevice.h"

#define DEVICE_TIMEOUT			INFINITE // 5000 ms
#define DEVICE_READ_TIMEOUT   10

HANDLE MxHidDevice::m_sync_event_tx = NULL;
HANDLE MxHidDevice::m_sync_event_rx = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
MxHidDevice::MxHidDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
{
    m_hid_drive_handle = INVALID_HANDLE_VALUE;
    m_sync_event_tx = NULL;
    m_sync_event_rx = NULL;
    m_pReadReport = NULL;
    m_pWriteReport = NULL;
    _chipFamily = MX508;

    Init();
}

MxHidDevice::~MxHidDevice()
{
    Close();
    Trash();
}

int MxHidDevice::Trash()
{
    if( m_sync_event_tx != NULL )
    {
        CloseHandle(m_sync_event_tx);
        m_sync_event_tx = NULL;
    }
    if( m_sync_event_rx != NULL )
    {
        CloseHandle(m_sync_event_rx);
        m_sync_event_rx = NULL;
    }
    return ERROR_SUCCESS;
}

int MxHidDevice::Close()
{
    if( m_hid_drive_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hid_drive_handle);
        m_hid_drive_handle = INVALID_HANDLE_VALUE;
    }
    FreeIoBuffers();

    return ERROR_SUCCESS;
}

void MxHidDevice::FreeIoBuffers()
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

// Modiifes m_Capabilities member variable
// Modiifes m_pReadReport member variable
// Modiifes m_pWriteReport member variable
int32_t MxHidDevice::AllocateIoBuffers()
{
    // Open the device
    /*HANDLE hHidDevice = CreateFile(_path.get(), 0, 0, NULL, OPEN_EXISTING, 0, NULL);

    if( hHidDevice == INVALID_HANDLE_VALUE )
    {
		int32_t error = GetLastError();
        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers().CreateFile ERROR:(%d)\r\n"), error);
        return error;
    }*/

    // Get the Capabilities including the max size of the report buffers
    PHIDP_PREPARSED_DATA  PreparsedData = NULL;
    if ( !HidD_GetPreparsedData(m_hid_drive_handle, &PreparsedData) )
    {
        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().GetPreparsedData ERROR:(%d)\r\n"), ERROR_GEN_FAILURE);
        return ERROR_GEN_FAILURE;
    }

    NTSTATUS sts = HidP_GetCaps(PreparsedData, &m_Capabilities);
	if( sts != HIDP_STATUS_SUCCESS )
    {
        HidD_FreePreparsedData(PreparsedData);
        ATLTRACE2(_T(" HidDevice::AllocateIoBuffers().GetCaps ERROR:(%d)\r\n"), HIDP_STATUS_INVALID_PREPARSED_DATA);
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    HidD_FreePreparsedData(PreparsedData);

    // Allocate a Read and Write Report buffers
    FreeIoBuffers();

    if ( m_Capabilities.InputReportByteLength )
    {
        m_pReadReport = (_MX_HID_DATA_REPORT*)malloc(m_Capabilities.InputReportByteLength);
        if ( m_pReadReport == NULL )
		{
	        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers(). Failed to allocate memory (1)\r\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
		}
    }

    if ( m_Capabilities.OutputReportByteLength )
    {
        m_pWriteReport = (_MX_HID_DATA_REPORT*)malloc(m_Capabilities.OutputReportByteLength);
        if ( m_pWriteReport == NULL )
		{
	        ATLTRACE2(_T(" MxHidDevice::AllocateIoBuffers(). Failed to allocate memory (2)\r\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
		}
    }

    return ERROR_SUCCESS;
}

/*
HANDLE MxHidDevice::OpenSpecifiedDevice (
        IN       HDEVINFO                    HardwareDeviceInfo,
        IN       PSP_INTERFACE_DEVICE_DATA   DeviceInterfaceData,
        OUT		 CString&					 devName,
        OUT      DWORD&						 devInst,
        rsize_t	 bufsize
        )
{
    PSP_INTERFACE_DEVICE_DETAIL_DATA     functionClassDeviceData = NULL;
    ULONG                                predictedLength = 0;
    ULONG                                requiredLength = 0;
    HANDLE								 hOut = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA						 devInfoData;

    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
    SetupDiGetInterfaceDeviceDetail (
        HardwareDeviceInfo,
        DeviceInterfaceData,
        NULL, // probing so no output buffer yet
        0, // probing so output buffer length of zero
        &requiredLength,
        NULL); // not interested in the specific dev-node


    predictedLength = requiredLength;
    // sizeof (SP_FNCLASS_DEVICE_DATA) + 512;

    functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc (predictedLength);
    functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    // Retrieve the information from Plug and Play.
    //
    if (! SetupDiGetInterfaceDeviceDetail (
        HardwareDeviceInfo,
        DeviceInterfaceData,
        functionClassDeviceData,
        predictedLength,
        &requiredLength,
        &devInfoData)) 
    {
            free( functionClassDeviceData );
            return INVALID_HANDLE_VALUE;
    }

    //	wcscpy_s( devName, bufsize, functionClassDeviceData->DevicePath) ;
    CString devPath = functionClassDeviceData->DevicePath;

	CStdString filter;
	filter.Format(_T("%s#vid_%04x&pid_%04x"), _T("HID"),m_vid, m_pid);

    // Is this our vid+pid?  If not, ignore it.
	if ( (devPath.MakeUpper().Find(filter.ToUpper()) != -1))
    {
        devName = devPath;
        devInst = devInfoData.DevInst;

        hOut = CreateFile (
            functionClassDeviceData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, // no SECURITY_ATTRIBUTES structure
            OPEN_EXISTING, // No special create flags
            0, // No special attributes
            NULL); // No template file
    } else {
        TRACE(__FUNCTION__ " VID-PID does not match\n");
    }

    free( functionClassDeviceData );
    return hOut;
}

HANDLE MxHidDevice::OpenUsbDevice( LPGUID  pGuid, CString& devpath, DWORD& outDevInst, rsize_t bufsize)
{
    ULONG                    NumberDevices;
    HANDLE                   hOut = INVALID_HANDLE_VALUE;
    HDEVINFO                 hDevInfo;
    SP_INTERFACE_DEVICE_DATA deviceInterfaceData;
    ULONG                    i;
    BOOLEAN                  done;

    NumberDevices = 0;

    //
    // Open a handle to the plug and play dev node.
    // SetupDiGetClassDevs() returns a device information set that contains info on all
    // installed devices of a specified class.
    //
    hDevInfo = SetupDiGetClassDevs (
        pGuid,
        NULL, // Define no enumerator (global)
        NULL, // Define no
        (DIGCF_PRESENT | // Only Devices present
        DIGCF_INTERFACEDEVICE)); // Function class devices.
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        // Insert error handling here.
        return INVALID_HANDLE_VALUE;
    }
    //
    // Take a wild guess at the number of devices we have;
    // Be prepared to realloc and retry if there are more than we guessed
    //
    NumberDevices = 4;
    done = FALSE;
    deviceInterfaceData.cbSize = sizeof (SP_INTERFACE_DEVICE_DATA);

    i=0;
    while (!done) {
        NumberDevices *= 2;  // keep increasing the number of devices until we reach the limit
        for (; i < NumberDevices; i++) {

            // SetupDiEnumDeviceInterfaces() returns information about device interfaces
            // exposed by one or more devices. Each call returns information about one interface;
            // the routine can be called repeatedly to get information about several interfaces
            // exposed by one or more devices.
            if ( SetupDiEnumDeviceInterfaces (
                hDevInfo,   // pointer to a device information set
                NULL,       // pointer to an SP_DEVINFO_DATA, We don't care about specific PDOs
                pGuid,      // pointer to a GUID
                i,          //zero-based index into the list of interfaces in the device information set
                &deviceInterfaceData)) // pointer to a caller-allocated buffer that contains a completed SP_DEVICE_INTERFACE_DATA structure
            {
                // open the device
                hOut = OpenSpecifiedDevice (hDevInfo, &deviceInterfaceData, devpath, outDevInst, bufsize);
                if ( hOut != INVALID_HANDLE_VALUE )
                {
                    done = TRUE;
                    TRACE(__FUNCTION__ " SetupDiEnumDeviceInterfaces PASS. index=%d\n",i);
                    break;
                } else {
                    TRACE(__FUNCTION__ " SetupDiEnumDeviceInterfaces FAILED to OPEN. index=%d\n",i);
                }
            }
            else
            {
                // EnumDeviceInterfaces error
                if (ERROR_NO_MORE_ITEMS == ::GetLastError())
                {
                    done = TRUE;
                    TRACE(__FUNCTION__ " SetupDiEnumDeviceInterfaces FAILED to find interface. index=%d\n",i);
                    break;
                } else {
                    TRACE(__FUNCTION__ " SetupDiEnumDeviceInterfaces FAILED for reason other then MAX_ITEMS. index=%d\n",i);
                }
            }
        }  // end-for
    }  // end-while

    // SetupDiDestroyDeviceInfoList() destroys a device information set and frees all associated memory.
    SetupDiDestroyDeviceInfoList (hDevInfo);
    return hOut;
}


int MxHidDevice::SetUsbDeviceId(int dwDevInst)
{ 
    // Get USB devnode from HID devnode.
    DWORD usbDevInst;
    int err = CM_Get_Parent(&usbDevInst, dwDevInst, 0);
    if( err != ERROR_SUCCESS )
    {
        assert(false);
        return err;
    }

    // Get the DeviceID string from the USB device devnode.
    TCHAR buf[MAX_PATH];
    err = CM_Get_Device_ID(usbDevInst, buf, sizeof(buf), 0);
    if( err != ERROR_SUCCESS )
    {
        assert(false);
        return err;
    }

    // Fixup name
    m_usb_device_id = buf;
    m_usb_device_id.Replace(_T('\\'), _T('#'));
    return err;
}

BOOL MxHidDevice::GetUsbDeviceFileName( LPGUID  pGuid, CString& outNameBuf, DWORD& outDevInst, rsize_t bufsize)
{
    HANDLE hDev = OpenUsbDevice( pGuid, outNameBuf, outDevInst, bufsize );
    if ( hDev == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }
    CloseHandle( hDev );
    return TRUE;
}
*/

int MxHidDevice::Init()
{
    int err = ERROR_SUCCESS;

    m_hid_drive_handle = ::CreateFile(_path.get(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if( m_hid_drive_handle == INVALID_HANDLE_VALUE )
    {
        return ::GetLastError();
    }

    // create TX and RX events
    m_sync_event_tx = ::CreateEvent( NULL, TRUE, FALSE, NULL );
    if( !m_sync_event_tx )
    {
        assert(false);
        return ::GetLastError();
    }
    m_sync_event_rx = ::CreateEvent( NULL, TRUE, FALSE, NULL );
    if( !m_sync_event_rx )
    {
        assert(false);
        return ::GetLastError();
    }

    memset(&m_Capabilities, 0, sizeof(m_Capabilities));

    err = AllocateIoBuffers();
	if ( err != ERROR_SUCCESS )
	{
		TRACE(__FUNCTION__ " ERROR: AllocateIoBuffers failed.\n");
		return err;
	}

    return ERROR_SUCCESS;
}


// Write to HID device
int MxHidDevice::Write(UCHAR* _buf, ULONG _size)
{
    DWORD status;

    // Preparation
    m_overlapped.Offset				= 0;
    m_overlapped.OffsetHigh			= 0;
    m_overlapped.hEvent				= (PVOID)(ULONGLONG)_size;

    ::ResetEvent( MxHidDevice::m_sync_event_tx );

    // Write to the device
    if( !::WriteFileEx( m_hid_drive_handle, _buf, _size, &m_overlapped, 
        WriteCompletionRoutine ) )
    {
        return ::GetLastError();
    }

    // wait for completion
    if( (status = ::WaitForSingleObjectEx( m_sync_event_tx, INFINITE, TRUE )) == WAIT_TIMEOUT )
    {
        ::CancelIo( m_hid_drive_handle );
        return WAIT_TIMEOUT;
    }

    if( m_overlapped.Offset == 0 )
        return -13 /*STERR_FAILED_TO_WRITE_FILE_DATA*/;
    else
        return ERROR_SUCCESS;
}

VOID MxHidDevice::WriteCompletionRoutine( 
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

    ::SetEvent( MxHidDevice::m_sync_event_tx );
}


// Read from HID device
int MxHidDevice::Read(void* _buf, UINT _size)
{
    DWORD status;

    // Preparation
    m_overlapped.Offset				= 0;
    m_overlapped.OffsetHigh			= 0;
    m_overlapped.hEvent				= (PVOID)(ULONGLONG)_size;

    ::ResetEvent( m_sync_event_rx );

    if( m_hid_drive_handle == INVALID_HANDLE_VALUE ) {
        return WAIT_TIMEOUT;
    }

    //  The read command does not sleep very well right now.
    Sleep(50); 

    // Read from device
    if( !::ReadFileEx( m_hid_drive_handle, _buf, _size, &m_overlapped, 
        ReadCompletionRoutine ) )
    {
        return ::GetLastError();
    }

    // wait for completion
    if( (status = (::WaitForSingleObjectEx( m_sync_event_rx, DEVICE_READ_TIMEOUT, TRUE ))) == WAIT_TIMEOUT )
    {
        ::CancelIo( m_hid_drive_handle );
        return WAIT_TIMEOUT;
    }

    if( m_overlapped.Offset == 0 )
        return -13 /*STERR_FAILED_TO_WRITE_FILE_DATA*/;
    else
        return ERROR_SUCCESS;
}

VOID MxHidDevice::ReadCompletionRoutine( 
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

    if( m_sync_event_rx != NULL) {
        SetEvent( m_sync_event_rx );
    }
}

/// <summary>
//-------------------------------------------------------------------------------------		
// Function to 16 byte SDP command format, these 16 bytes will be sent by host to 
// device in SDP command field of report 1 data structure
//
// @return
//		a report packet to be sent.
//-------------------------------------------------------------------------------------
//
VOID MxHidDevice::PackSDPCmd(PSDPCmd pSDPCmd)
{
    memset((UCHAR *)m_pWriteReport, 0, m_Capabilities.OutputReportByteLength);
    m_pWriteReport->ReportId = (unsigned char)REPORT_ID_SDP_CMD;
    PLONG pTmpSDPCmd = (PLONG)(m_pWriteReport->Payload);

	pTmpSDPCmd[0] = (  ((pSDPCmd->address  & 0x00FF0000) << 8) 
		          | ((pSDPCmd->address  & 0xFF000000) >> 8) 
		          |  (pSDPCmd->command   & 0x0000FFFF) );

	pTmpSDPCmd[1] = (   (pSDPCmd->dataCount & 0xFF000000)
		          | ((pSDPCmd->format   & 0x000000FF) << 16)
		          | ((pSDPCmd->address  & 0x000000FF) <<  8)
		          | ((pSDPCmd->address  & 0x0000FF00) >>  8 ));

	pTmpSDPCmd[2] = (   (pSDPCmd->data     & 0xFF000000)
		          | ((pSDPCmd->dataCount & 0x000000FF) << 16)
		          |  (pSDPCmd->dataCount & 0x0000FF00)
		          | ((pSDPCmd->dataCount & 0x00FF0000) >> 16));

	pTmpSDPCmd[3] = (  ((0x00  & 0x000000FF) << 24)
		          | ((pSDPCmd->data     & 0x00FF0000) >> 16) 
		          |  (pSDPCmd->data     & 0x0000FF00)
		          | ((pSDPCmd->data     & 0x000000FF) << 16));   

}

BOOL MxHidDevice::GetCmdAck(UINT RequiredCmdAck)
{
    memset((UCHAR *)m_pReadReport, 0, m_Capabilities.InputReportByteLength);

    //Get Report3, Device to Host:
    //4 bytes HAB mode indicating Production/Development part
	if ( Read( (UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength )  != ERROR_SUCCESS)
	{
		return FALSE;
	}
	if ( (*(unsigned int *)(m_pReadReport->Payload) != HabEnabled)  && 
		 (*(unsigned int *)(m_pReadReport->Payload) != HabDisabled) ) 
	{
		return FALSE;	
	}

    memset((UCHAR *)m_pReadReport, 0, m_Capabilities.InputReportByteLength);

    //Get Report4, Device to Host:
	if ( Read( (UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength ) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	if (*(unsigned int *)(m_pReadReport->Payload) != RequiredCmdAck)
	{
		TRACE("WriteReg(): Invalid write ack: 0x%x\n", ((PULONG)m_pReadReport)[0]);
		return FALSE; 
	}

    return TRUE;
}

BOOL MxHidDevice::WriteReg(PSDPCmd pSDPCmd)
{
    //First, pack the command to a report.
    PackSDPCmd(pSDPCmd);

	//Send the report to USB HID device
	if ( Write((unsigned char *)m_pWriteReport, m_Capabilities.OutputReportByteLength) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	if ( !GetCmdAck(ROM_WRITE_ACK) )
	{
		return FALSE;
	}
    
    return TRUE;
}

BOOL MxHidDevice::TransData(UINT address, UINT byteCount, const unsigned char * pBuf)
{
    SDPCmd SDPCmd;

    SDPCmd.command = ROM_KERNEL_CMD_WR_FILE;
    SDPCmd.dataCount = byteCount;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = address;

    //First, pack the command to a report.
    PackSDPCmd(&SDPCmd);

	//Send the report to USB HID device
	if ( Write((unsigned char *)m_pWriteReport, m_Capabilities.OutputReportByteLength)  != ERROR_SUCCESS)
	{
		return FALSE;
	}

    m_pWriteReport->ReportId = REPORT_ID_DATA;
    UINT MaxHidTransSize = m_Capabilities.OutputReportByteLength -1;
    UINT TransSize;

    while(byteCount > 0)
    {
        TransSize = (byteCount > MaxHidTransSize) ? MaxHidTransSize : byteCount;

        memcpy(m_pWriteReport->Payload, pBuf, TransSize);

        if (Write((unsigned char *)m_pWriteReport, m_Capabilities.OutputReportByteLength) != ERROR_SUCCESS)
            return FALSE;
        byteCount -= TransSize;
        pBuf += TransSize;
        //TRACE("Transfer Size: %d\n", MaxHidTransSize);
    }

    //It should be the fault of elvis_mcurom.elf which only returns report3
    memset((UCHAR *)m_pReadReport, 0, m_Capabilities.InputReportByteLength);
    //Get Report3, Device to Host:
    //4 bytes HAB mode indicating Production/Development part
	if ( Read( (UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength )  != ERROR_SUCCESS)
	{
		return FALSE;
	}
	if ( (*(unsigned int *)(m_pReadReport->Payload) != HabEnabled)  && 
		 (*(unsigned int *)(m_pReadReport->Payload) != HabDisabled) ) 
	{
		return FALSE;	
	}
    
    //below function should be invoked for mx50
	/*if ( !GetCmdAck(ROM_STATUS_ACK) )
	{
		return FALSE;
	}*/

	return TRUE;
}

BOOL MxHidDevice::Jump(UINT RAMAddress)
{
    SDPCmd SDPCmd;

    SDPCmd.command = ROM_KERNEL_CMD_JUMP_ADDR;
    SDPCmd.dataCount = 0;
    SDPCmd.format = 0;
    SDPCmd.data = 0;
    SDPCmd.address = RAMAddress;

	//Send write Command to USB
    //First, pack the command to a report.
    PackSDPCmd(&SDPCmd);

	//Send the report to USB HID device
	if ( Write((unsigned char *)m_pWriteReport, m_Capabilities.OutputReportByteLength) != ERROR_SUCCESS )
	{
		return FALSE;
	}

    memset((UCHAR *)m_pReadReport, 0, m_Capabilities.InputReportByteLength);
    //Get Report3, Device to Host:
    //4 bytes HAB mode indicating Production/Development part
	if ( Read( (UCHAR *)m_pReadReport, m_Capabilities.InputReportByteLength )  != ERROR_SUCCESS)
	{
		return FALSE;
	}
	if ( (*(unsigned int *)(m_pReadReport->Payload) != HabEnabled)  && 
		 (*(unsigned int *)(m_pReadReport->Payload) != HabDisabled) ) 
	{
		return FALSE;	
	}

	TRACE("*********Jump to Ramkernel successfully!**********\r\n");
	return TRUE;
}


// Write the data to i.MX
#define FLASH_HEADER_SIZE	0x20
#define ROM_TRANSFER_SIZE	0x400

BOOL MxHidDevice::Download(StFwComponent *fwComponent, UINT PhyRAMAddr4KRL, MemorySection loadSection, MemorySection setSection, BOOL HasFlashHeader)
{
    //Those parameter is hard-coded for test purpose
    //BOOL HasFlashHeader = FALSE;
    //MemorySection loadSection = MemSectionOTH;
    //MemorySection setSection = MemSectionAPP;
	//int PhyRAMAddr4KRL = 0x90040000;
    
    UCHAR* pBuffer = (UCHAR*)fwComponent->GetDataPtr();
    ULONGLONG dataCount = fwComponent->size();
	DWORD byteIndex, numBytesToWrite = 0;
	for ( byteIndex = 0; byteIndex < dataCount; byteIndex += numBytesToWrite )
	{
		// Get some data
		numBytesToWrite = min(MAX_SIZE_PER_DOWNLOAD_COMMAND, dataCount - byteIndex);

		if (!TransData(PhyRAMAddr4KRL + byteIndex, numBytesToWrite, pBuffer + byteIndex))
		{
			TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X, 0x%X, 0x%X) failed.\n"), \
                PhyRAMAddr4KRL + byteIndex, numBytesToWrite, loadSection, pBuffer + byteIndex);
			return FALSE;
		}
	}

	// If we are downloading to DCD or CSF, we don't need to send 
	if ( loadSection == MemSectionDCD || loadSection == MemSectionCSF )
	{
		return TRUE;
	}

	int FlashHdrAddr;
	const unsigned char * pHeaderData = NULL;

	//transfer length of ROM_TRANSFER_SIZE is a must to ROM code.
	unsigned char FlashHdr[ROM_TRANSFER_SIZE] = { 0 };
	
	// Just use the front of the data buffer if the data includes the FlashHeader
	if( HasFlashHeader )
	{
		FlashHdrAddr = PhyRAMAddr4KRL;
		pHeaderData = pBuffer;
	}
	else
	{
		// Otherwise, create a header and append the data
		
		//Copy image data with an offset of FLASH_HEADER_SIZE to the temp buffer.
		memcpy(FlashHdr + FLASH_HEADER_SIZE, pBuffer, ROM_TRANSFER_SIZE - FLASH_HEADER_SIZE);
		
		//We should write actual image address to the first dword of flash header.
		((int *)FlashHdr)[0] = PhyRAMAddr4KRL;

		FlashHdrAddr = PhyRAMAddr4KRL - FLASH_HEADER_SIZE;
		pHeaderData = (const unsigned char *)FlashHdr;
	}
    
	//Set execute address.
	if ( !TransData(FlashHdrAddr, ROM_TRANSFER_SIZE, pHeaderData) )
	{
		TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X, 0x%X, 0x%X) failed.\n"), \
            FlashHdrAddr, ROM_TRANSFER_SIZE, setSection, pHeaderData);
		return FALSE;
	}
    //return FALSE;
    if(setSection != MemSectionAPP)
        return TRUE;

    if( !Jump(FlashHdrAddr))
	{
        TRACE(_T("DownloadImage(): Failed to jump to RAM address: 0x%x.\n"), FlashHdrAddr);
		return FALSE;
	}

	return TRUE;
}

/*
//Actually, this is the setting for i.mx51 since we simulate mx508 HID ROM on mx51.
typedef struct 
{
    UINT addr;
    UINT data;
    UINT format;
} stMemoryInit;

static stMemoryInit mddrMx508[] = 
{
    {0x73fa88a0, 0x00000020, 32},
    {0x73fa850c, 0x000020c5, 32},
    {0x73fa8510, 0x000020c5, 32},
    {0x73fa883c, 0x00000002, 32},
    {0x73fa8848, 0x00000002, 32},
    {0x73fa84b8, 0x000000e7, 32},
    {0x73fa84bc, 0x00000045, 32},
    {0x73fa84c0, 0x00000045, 32},
    {0x73fa84c4, 0x00000045, 32},
    {0x73fa84c8, 0x00000045, 32},
    {0x73fa8820, 0x00000000, 32},
    {0x73fa84a4, 0x00000003, 32},
    {0x73fa84a8, 0x00000003, 32},
    {0x73fa84ac, 0x000000e3, 32},
    {0x73fa84b0, 0x000000e3, 32},
    {0x73fa84b4, 0x000000e3, 32},
    {0x73fa84cc, 0x000000e3, 32},
    {0x73fa84d0, 0x000000e2, 32},
    {0x83fd9000, 0x82a20000, 32},
    {0x83fd9008, 0x82a20000, 32},
    {0x83fd9010, 0x000ad0d0, 32},
    {0x83fd9004, 0x333574aa, 32},
    {0x83fd900c, 0x333574aa, 32},
    {0x83fd9014, 0x04008008, 32},
    {0x83fd9014, 0x0000801a, 32},
    {0x83fd9014, 0x0000801b, 32},
    {0x83fd9014, 0x00448019, 32},
    {0x83fd9014, 0x07328018, 32},
    {0x83fd9014, 0x04008008, 32},
    {0x83fd9014, 0x00008010, 32},
    {0x83fd9014, 0x00008010, 32},
    {0x83fd9014, 0x06328018, 32},
    {0x83fd9014, 0x03808019, 32},
    {0x83fd9014, 0x00408019, 32},
    {0x83fd9014, 0x00008000, 32},
    {0x83fd9014, 0x0400800c, 32},
    {0x83fd9014, 0x0000801e, 32},
    {0x83fd9014, 0x0000801f, 32},
    {0x83fd9014, 0x0000801d, 32},
    {0x83fd9014, 0x0732801c, 32},
    {0x83fd9014, 0x0400800c, 32},
    {0x83fd9014, 0x00008014, 32},
    {0x83fd9014, 0x00008014, 32},
    {0x83fd9014, 0x0632801c, 32},
    {0x83fd9014, 0x0380801d, 32},
    {0x83fd9014, 0x0040801d, 32},
    {0x83fd9014, 0x00008004, 32},
    {0x83fd9000, 0xb2a20000, 32},
    {0x83fd9008, 0xb2a20000, 32},
    {0x83fd9010, 0x000ad6d0, 32},
    {0x83fd9034, 0x90000000, 32},
    {0x83fd9014, 0x00000000, 32},
};

BOOL MxHidDevice::InitMemoryDevice()
{
    SDPCmd SDPCmd;

    SDPCmd.command = ROM_KERNEL_CMD_WR_MEM;
    SDPCmd.dataCount = 4;

    for(int i=0; i<sizeof(mddrMx508)/sizeof(stMemoryInit); i++)
    {
        SDPCmd.format = mddrMx508[i].format;
        SDPCmd.data = mddrMx508[i].data;
        SDPCmd.address = mddrMx508[i].addr;
        if ( !WriteReg(&SDPCmd) )
        {
            TRACE("In InitMemoryDevice(): write memory failed\n");
            return FALSE;
        }
    }

	return TRUE;
}
*/

BOOL MxHidDevice::InitMemoryDevice(CString filename)
{
	USES_CONVERSION;
	SDPCmd SDPCmd;

    SDPCmd.command = ROM_KERNEL_CMD_WR_MEM;
    SDPCmd.dataCount = 4;

	CFile scriptFile;
	CFileException fileException;
	if( !scriptFile.Open(filename, CFile::modeRead | CFile::shareDenyNone, &fileException) )
	{
		TRACE( _T("Can't open file %s, error = %u\n"), filename, fileException.m_cause );
	}

	CStringT<char,StrTraitMFC<char> > cmdString;
	scriptFile.Read(cmdString.GetBufferSetLength(scriptFile.GetLength()), scriptFile.GetLength());
	cmdString.ReleaseBuffer();

	XNode script;
	if ( script.Load(A2T(cmdString)) != NULL )
	{
		XNodes cmds = script.GetChilds(_T("CMD"));
		XNodes::iterator cmd = cmds.begin();
		for ( ; cmd != cmds.end(); ++cmd )
		{
			MemoryInitCommand* pCmd = (MemoryInitCommand*)(*cmd);
            SDPCmd.format = pCmd->GetFormat();
            SDPCmd.data = pCmd->GetData();
            SDPCmd.address = pCmd->GetAddress();
			if ( !WriteReg(&SDPCmd) )
            {
                TRACE("In InitMemoryDevice(): write memory failed\n");
                return FALSE;
            }
		}
	}

	return TRUE;
}