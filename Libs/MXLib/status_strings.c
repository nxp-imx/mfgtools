////////////////////////////////////////////////////////////////
// File - status_strings.c
//
// Library for handling the status returned from WinDriver API.
//
// Copyright (c) 2003 Jungo Ltd.  http://www.jungo.com 
////////////////////////////////////////////////////////////////

#ifdef __KERNEL__
    #include "kpstdlib.h"
#endif

#include <stdio.h>
#include "status_strings.h"

typedef struct
{
    DWORD dwStatus;
    const char *sErrDesc;
} STATUS_STRINGS, *PSTATUS_STRINGS;

#define ERROR_CODE_NOT_FOUND "Unrecognized error code"

STATUS_STRINGS statusStrings[] = 
{
    {WD_STATUS_SUCCESS, "Success"},
    {WD_STATUS_INVALID_WD_HANDLE, "Invalid WinDriver handle"},
    
// The following statuses are returned by WinDriver:
    {WD_WINDRIVER_STATUS_ERROR, "Error"},

    {WD_INVALID_HANDLE, "Invalid handle"},
    {WD_INVALID_PIPE_NUMBER, "Invalid pipe number"},
    {WD_READ_WRITE_CONFLICT, "Conflict between read and write operations"},
                                             // request to write to an IN (read) pipe
    {WD_ZERO_PACKET_SIZE, "Packet size is zero"},
    {WD_INSUFFICIENT_RESOURCES, "Insufficient resources"},
    {WD_UNKNOWN_PIPE_TYPE, "Unknown pipe type"},
    {WD_SYSTEM_INTERNAL_ERROR, "Intenal system error"},
    {WD_DATA_MISMATCH, "Data mismatch"},
    {WD_NO_LICENSE, "No valid license"},
    {WD_INVALID_PARAMETER, "Invalid parameter"},
    {WD_NOT_IMPLEMENTED, "Function not implemented"},
    {WD_KERPLUG_FAILURE, "KernelPlugin failure"},
    {WD_FAILED_ENABLING_INTERRUPT, "Failed enabling interrupt"},
    {WD_INTERRUPT_NOT_ENABLED, "Interrupt not enabled"},
    {WD_RESOURCE_OVERLAP, "Resource overlap"},
    {WD_DEVICE_NOT_FOUND, "Device not found"},
    {WD_WRONG_UNIQUE_ID, "Wrong unique ID"},
    {WD_OPERATION_ALREADY_DONE, "Operation already done"},
    {WD_USB_DESCRIPTOR_ERROR, "Usb descriptor error"},
    {WD_SET_CONFIGURATION_FAILED, "Set configuration operation failed"},
    {WD_CANT_OBTAIN_PDO, "Cannot obtain PDO"},
    {WD_TIME_OUT_EXPIRED, "TimeOut expired"},
    {WD_IRP_CANCELED, "IRP operation cancelled"},
    {WD_FAILED_USER_MAPPING, "Failed to map in user space"},
    {WD_FAILED_KERNEL_MAPPING, "Failed to map in kernel space"},
    {WD_NO_RESOURCES_ON_DEVICE, "No resources on the device"},
    {WD_NO_EVENTS, "No events"},
    {WD_INCORRECT_VERSION, "Incorrect WinDriver version installed"},
    {WD_TRY_AGAIN, "Try again"},
    {WD_WINDRIVER_NOT_FOUND, "Failed open WinDriver"}, 
// The following statuses are returned by USBD:
    // USBD status types:
    {WD_USBD_STATUS_SUCCESS, "USBD: Success"},
    {WD_USBD_STATUS_PENDING, "USBD: Operation pending"},
    {WD_USBD_STATUS_ERROR, "USBD: Error"},
    {WD_USBD_STATUS_HALTED, "USBD: Halted"},

    // USBD status codes:
    // NOTE: The following status codes are comprised of one of the status types above and an
    // error code [i.e. 0xXYYYYYYYL - where: X = status type; YYYYYYY = error code].
    // The same error codes may also appear with one of the other status types as well.

    // HC (Host Controller) status codes.
    // [NOTE: These status codes use the WD_USBD_STATUS_HALTED status type]:
    {WD_USBD_STATUS_CRC, "HC status: CRC"},
    {WD_USBD_STATUS_BTSTUFF, "HC status: Bit stuffing "},
    {WD_USBD_STATUS_DATA_TOGGLE_MISMATCH, "HC status: Data toggle mismatch"},
    {WD_USBD_STATUS_STALL_PID, "HC status: PID stall"},
    {WD_USBD_STATUS_DEV_NOT_RESPONDING, "HC status: Device not responding"},
    {WD_USBD_STATUS_PID_CHECK_FAILURE, "HC status: PID check failed"},
    {WD_USBD_STATUS_UNEXPECTED_PID, "HC status: Unexpected PID"},
    {WD_USBD_STATUS_DATA_OVERRUN, "HC status: Data overrun"},
    {WD_USBD_STATUS_DATA_UNDERRUN, "HC status: Data underrun"},
    {WD_USBD_STATUS_RESERVED1, "HC status: Reserved1"},
    {WD_USBD_STATUS_RESERVED2, "HC status: Reserved2"},
    {WD_USBD_STATUS_BUFFER_OVERRUN, "HC status: Buffer overrun"},
    {WD_USBD_STATUS_BUFFER_UNDERRUN, "HC status: Buffer Underrun"},
    {WD_USBD_STATUS_NOT_ACCESSED, "HC status: Not accessed"},
    {WD_USBD_STATUS_FIFO, "HC status: Fifo"},

    // Returned by HCD (Host Controller Driver) if a transfer is submitted to an endpoint that is
    // stalled:
    {WD_USBD_STATUS_ENDPOINT_HALTED, "HCD: Trasnfer submitted to stalled endpoint"},

    // Software status codes
    // [NOTE: The following status codes have only the error bit set]:
    {WD_USBD_STATUS_NO_MEMORY, "USBD: Out of memory"},
    {WD_USBD_STATUS_INVALID_URB_FUNCTION, "USBD: Invalid URB function"},
    {WD_USBD_STATUS_INVALID_PARAMETER, "USBD: Invalid parameter"},

    // Returned if client driver attempts to close an endpoint/interface
    // or configuration with outstanding transfers:
    {WD_USBD_STATUS_ERROR_BUSY, "USBD: Attempted to close enpoint/interface/configuration with outstanding transfer"},

    // Returned by USBD if it cannot complete a URB request. Typically this
    // will be returned in the URB status field when the Irp is completed
    // with a more specific NT error code. [The Irp statuses are indicated in
    // WinDriver's Monitor Debug Messages (wddebug_gui) tool]:
    {WD_USBD_STATUS_REQUEST_FAILED, "USBD: URB request failed"},

    {WD_USBD_STATUS_INVALID_PIPE_HANDLE, "USBD: Invalid pipe handle"},

    // Returned when there is not enough bandwidth available
    // to open a requested endpoint:
    {WD_USBD_STATUS_NO_BANDWIDTH, "USBD: Not enough bandwidth for endpoint"},

    // Generic HC (Host Controller) error:
    {WD_USBD_STATUS_INTERNAL_HC_ERROR, "USBD: Host controller error"},

    // Returned when a short packet terminates the transfer
    // i.e. USBD_SHORT_TRANSFER_OK bit not set:
    {WD_USBD_STATUS_ERROR_SHORT_TRANSFER, "USBD: Trasnfer terminated with short packet"},

    // Returned if the requested start frame is not within
    // USBD_ISO_START_FRAME_RANGE of the current USB frame,
    // NOTE: that the stall bit is set:
    {WD_USBD_STATUS_BAD_START_FRAME, "USBD: Start frame outside range"},

    // Returned by HCD (Host Controller Driver) if all packets in an iso transfer complete with
    // an error:
    {WD_USBD_STATUS_ISOCH_REQUEST_FAILED, "HCD: Isochronous transfer completed with error"},

    // Returned by USBD if the frame length control for a given
    // HC (Host Controller) is already taken by another driver:
    {WD_USBD_STATUS_FRAME_CONTROL_OWNED, "USBD: Frame length control already taken"},

    // Returned by USBD if the caller does not own frame length control and
    // attempts to release or modify the HC frame length:
    {WD_USBD_STATUS_FRAME_CONTROL_NOT_OWNED, "USBD: Attemped operation on frame length control not owned by caller"},
};

const char * DLLCALLCONV Stat2Str(DWORD dwStatus) 
{
    DWORD i;
    for(i=0; i<sizeof(statusStrings)/sizeof(STATUS_STRINGS); i++)
        if (statusStrings[i].dwStatus==dwStatus)
            return statusStrings[i].sErrDesc;
    return ERROR_CODE_NOT_FOUND;
}
