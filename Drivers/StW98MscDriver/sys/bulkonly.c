/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    BulkOnly.c 

Abstract:

    Implements bulk-only protocol for USB Mass Storage device

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

    06/30/00: MRB  Added bulk-only support to USB mass storage sample

--*/
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "umss.h"
 
 
VOID
UMSS_BulkOnlyStartIo(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
Routine Description:

    Handler for all I/O requests using bulk-only protocol.

 Arguments:

    DeviceExtension - Device extension for our FDO.

Return Value:

    NONE

--*/

{
    PIOPACKET IoPacket;
    PCOMMAND_BLOCK_WRAPPER CBW;
    
    ENTER(UMSS_BulkOnlyStartIo);

    IoPacket = DeviceExtension->IoPacket;

    DeviceExtension->Retry = TRUE;

    // Setup the command block wrapper for this request
    CBW = &(DeviceExtension->CBW);
    CBW->dCBWSignature = CBW_SIGNATURE;
    CBW->dCBWTag = 0;
    CBW->dCBWDataTransferLength = DeviceExtension->BytesToTransfer;
    CBW->bmCBWFlags = (IoPacket->Flags & IO_FLAGS_DATA_IN) ? 0x80 : 0;
    CBW->bCBWLun = IoPacket->Lun;
    CBW->bCBWLength = IoPacket->CdbLength;
    RtlCopyMemory(CBW->CBWCB, IoPacket->Cdb, IoPacket->CdbLength);

    // Send the command block wrapper to the device.
    // Calls UMSS_BulkOnlySendCBWComplete when transfer completes.
    UMSS_BulkTransfer(
        DeviceExtension,
        DATA_OUT,
        CBW,
        sizeof(COMMAND_BLOCK_WRAPPER),
        UMSS_BulkOnlySendCBWComplete
        );

    EXIT(UMSS_BulkOnlyStartIo);
}



NTSTATUS
UMSS_BulkOnlySendCBWComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    )
/*++
Routine Description:

    Completion handler for command phase of bulk-only I/O request.

 Arguments:

    DeviceObject - Previous device object.
    Irp - Irp used for sending command.
    Reference - Our FDO device extension.

Return Value:

    Driver-originated IRPs always return STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    NTSTATUS NtStatus;
    PURB Urb;
    PDEVICE_EXTENSION DeviceExtension;
    PIOPACKET IoPacket;

    ENTER(UMSS_BulkOnlySendCBWComplete);

    DeviceExtension = (PDEVICE_EXTENSION)Reference;

    IoPacket = DeviceExtension->IoPacket;
    Urb = DeviceExtension->Urb;
	    
    UMSS_KdPrint(DBGLVL_HIGH, ("UMSS_BulkOnlyCBWComplete - Urb Status = %x, Irp Status= %x\n",
        Urb->UrbHeader.Status,
        Irp->IoStatus.Status));

    NtStatus = Irp->IoStatus.Status;
	
    if (!NT_SUCCESS(NtStatus))
    {
        UMSS_KdPrint( DBGLVL_MINIMUM,("CBW Failure!\n"));
        UMSS_KdPrint(DBGLVL_MINIMUM, ("UMSS_BulkOnlyCBWComplete - Urb Status = %x, Irp Status= %x\n",
            Urb->UrbHeader.Status,
            Irp->IoStatus.Status));

        if (USBD_HALTED(Urb->UrbHeader.Status))
        {
            //Schedule a work-item to do a reset recovery
            UMSS_ScheduleWorkItem((PVOID)DeviceExtension, UMSS_BulkOnlyResetRecovery);
        }
        else
        {
            // Device failed CBW without stalling, so complete with error
            UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
        }
    }
    else
    {
        // CBW was accepted by device, so start data phase of I/O operation
        UMSS_BulkOnlyTransferData(DeviceExtension);
    }

    RETURN(STATUS_MORE_PROCESSING_REQUIRED, UMSS_BulkOnlySendCBWComplete);
}

VOID
UMSS_BulkOnlyResetRecovery(
    IN PVOID Reference
    )
/*++
Routine Description:

    Worker function used to execute a reset recovery after a stall.

 Arguments:

    Reference - Our device extension.

Return Value:

    NONE

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
    PURB Urb;
    NTSTATUS NtStatus;
    ULONG PortStatus;

    ENTER(UMSS_BulkOnlyResetRecovery);

    DeviceExtension = (PDEVICE_EXTENSION)Reference;
    Urb = DeviceExtension->Urb;

    // Steps for reset recovery:
    // 1. Verify device is still attached.
    // 2. Send device a mass storage reset command on the default endpoint.
    // 3. Reset the bulk-in endpoint.
    // 4. Reset the bulk-out endpoint.
    // 5. Complete the original I/O request with error.

    NtStatus = UMSS_GetPortStatus(DeviceExtension->Fdo, &PortStatus);

    if ( NT_SUCCESS(NtStatus) && !(PortStatus & USBD_PORT_CONNECTED))
    {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Device is gone, marking as removed\n"));
 
        //Device is gone, set flag to indicate this
        DeviceExtension->DeviceRemoved = TRUE;
        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
        EXIT(UMSS_BulkOnlyResetRecovery);
        return;
    }

    // Build the mass storage reset command
    UsbBuildVendorRequest(
        Urb,
        URB_FUNCTION_CLASS_INTERFACE,
        (USHORT) sizeof (struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
        0,
        0,
        BULK_ONLY_MASS_STORAGE_RESET,
        0,
        0,
        NULL,
        NULL,
        0,
        NULL
        );

    // Send mass storage reset command to device
    NtStatus = UMSS_CallUSBD(DeviceExtension->Fdo, Urb);

    if (!NT_SUCCESS(NtStatus))
    {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Reset Recovery failed!\n"));
    }
    else
    {
        //Reset Bulk-in endpoint
        NtStatus = UMSS_ResetPipe(
            DeviceExtension->Fdo,
            DeviceExtension->UsbInterface->Pipes[DeviceExtension->DataInPipe].PipeHandle
            );

        if (!NT_SUCCESS(NtStatus))
        {
            UMSS_KdPrint( DBGLVL_MINIMUM,("Unable to clear Bulk-in endpoint\n"));
        }

        //Reset Bulk-out endpoint
        NtStatus = UMSS_ResetPipe(
            DeviceExtension->Fdo,
            DeviceExtension->UsbInterface->Pipes[DeviceExtension->DataOutPipe].PipeHandle
            );

        if (!NT_SUCCESS(NtStatus))
        {
            UMSS_KdPrint( DBGLVL_MINIMUM,("Unable to clear Bulk-out endpoint\n"));
        }
    }
    UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);

    EXIT(UMSS_BulkOnlyResetRecovery);
}


VOID 
UMSS_BulkOnlyTransferData(
    PDEVICE_EXTENSION DeviceExtension
    )
/*++
Routine Description:

    Schedules a bulk data transfer to/from the device.

 Arguments:

    DeviceExtension - Our FDO's device extension.

Return Value:

    NONE

--*/

{
    PVOID DataBuffer;
    ULONG DataBufferLength;
  
    ENTER(UMSS_BulkOnlyTransferData);

    // Steps for data phase
    // 1. Get data buffer fragment (either SGD list, flat buffer, or none).
    // 2. Schedule data transfer if neccessary.
    // 3. Repeat 1-2 until all data transferred, or endpoint stalls.
    // 4. Move to status phase.

    // Get next data buffer element, if any
    DataBuffer = UMSS_GetBuffer(DeviceExtension, &DataBufferLength);

    if (NULL == DataBuffer)
    {
        //No data to transfer, so move to status phase
        UMSS_BulkOnlyGetStatus(DeviceExtension);
    }
    else
    {
        // Schedule the data transfer.
        // Calls UMSS_BulkOnlyTransferDataComplete when transfer completes.
        UMSS_BulkTransfer(
            DeviceExtension,
            (UCHAR)((DeviceExtension->IoPacket->Flags & IO_FLAGS_DATA_IN) ?
                DATA_IN : DATA_OUT),
            DataBuffer,
            DataBufferLength,
            UMSS_BulkOnlyTransferDataComplete
            );
    }
    EXIT(UMSS_BulkOnlyTransferData);
}


NTSTATUS
UMSS_BulkOnlyTransferDataComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    )
/*++
Routine Description:

    Completion handler for bulk data transfer requests.

 Arguments:

    DeviceObject - Previous device object.
    Irp - Irp used for sending command.
    Reference - Our device extension.

Return Value:

    Driver-originated IRPs always return STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    NTSTATUS NtStatus;
    PDEVICE_EXTENSION DeviceExtension;

    ENTER(UMSS_BulkOnlyTransferDataComplete);

    DeviceExtension = (PDEVICE_EXTENSION)Reference;

    NtStatus = Irp->IoStatus.Status;

    UMSS_KdPrint(DBGLVL_HIGH, ("UMSS_TransferDataComplete - Urb Status = %x, Irp Status= %x\n",
        DeviceExtension->Urb->UrbHeader.Status,
        NtStatus));

    if (!NT_SUCCESS(NtStatus))
    {
        // Check if we need to clear stalled pipe
        if (USBD_HALTED(DeviceExtension->Urb->UrbHeader.Status))
        {
            if (!UMSS_ScheduleWorkItem((PVOID)DeviceExtension, UMSS_BulkOnlyResetPipeAndGetStatus))
            {
                UMSS_KdPrint( DBGLVL_MINIMUM,("Failed to allocate work-item to reset pipe!\n"));
                TRAP();
                UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
            }
        }
    }
    else
    {
        // Start next part of data phase
        UMSS_BulkOnlyTransferData(DeviceExtension);
    }

    RETURN(STATUS_MORE_PROCESSING_REQUIRED, UMSS_BulkOnlyTransferDataComplete);
}


VOID
UMSS_BulkOnlyResetPipeAndGetStatus(
    IN PVOID Reference
    )
/*++
Routine Description:

    Worker function used to reset a stalled pipe during data phase.

 Arguments:

    Reference - Our device extension.

Return Value:

    NONE

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
    PURB Urb;
    USBD_PIPE_HANDLE PipeHandle;

    ENTER(UMSS_BulkOnlyResetPipeAndGetStatus);

    DeviceExtension = (PDEVICE_EXTENSION)Reference;

    Urb = DeviceExtension->Urb;
    PipeHandle = Urb->UrbBulkOrInterruptTransfer.PipeHandle;

    // Reset the endpoint
    UMSS_ResetPipe(
        DeviceExtension->Fdo,
        PipeHandle
        );

    // Data phase is finished since the endpoint stalled, so go to status phase
    UMSS_BulkOnlyGetStatus(DeviceExtension);

    EXIT(UMSS_BulkOnlyResetPipeAndGetStatus);
}


VOID 
UMSS_BulkOnlyGetStatus(
    PDEVICE_EXTENSION DeviceExtension
    )
/*++
Routine Description:

    Schedules bulk transfer to get CSW

 Arguments:

    DeviceExtension - Our device extension.

Return Value:

    NONE

--*/

{
    ENTER(UMSS_BulkOnlyGetStatus);

    // Schedule bulk transfer to get command status wrapper from device
    UMSS_BulkTransfer(
        DeviceExtension,
        DATA_IN,
        &(DeviceExtension->CSW),
        sizeof(COMMAND_STATUS_WRAPPER),
        UMSS_BulkOnlyGetStatusComplete
        );

    EXIT(UMSS_BulkOnlyGetStatus);
}


NTSTATUS
UMSS_BulkOnlyGetStatusComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    )
/*++
Routine Description:

    Completion handler for bulk data transfer request.

 Arguments:

    DeviceObject - Previous device object.
    Irp - Irp used for sending command.
    Reference - Our FDO.

Return Value:

    Driver-originated IRPs always return STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    NTSTATUS NtStatus;
    PDEVICE_EXTENSION DeviceExtension;
    PCOMMAND_STATUS_WRAPPER CSW;

    ENTER(UMSS_BulkOnlyGetStatusComplete);

    DeviceExtension = (PDEVICE_EXTENSION) Reference;

    CSW = &(DeviceExtension->CSW);

    NtStatus = Irp->IoStatus.Status;

    if ( NT_SUCCESS(NtStatus) &&
         (CSW->dCSWSignature == CSW_SIGNATURE) )
    {
        if (CSW->bCSWStatus == CSW_STATUS_PASSED)
        {
            // Received valid CSW with good status

            UMSS_CompleteRequest(DeviceExtension, IO_STATUS_SUCCESS);
        }
        else if(CSW->bCSWStatus == CSW_STATUS_PHASE_ERROR)
        {
            // A Phase Error occurred, reset device
            UMSS_ScheduleWorkItem((PVOID)DeviceExtension,
                                UMSS_BulkOnlyResetRecovery);
        }
        else
	{
	      UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
	}
    }
    else if ( (!NT_SUCCESS(NtStatus)) &&
              (USBD_HALTED(DeviceExtension->Urb->UrbHeader.Status)) &&
              (DeviceExtension->Retry) )
    {
        // Device stalled CSW transfer, retry once before failing

        DeviceExtension->Retry = FALSE;
        if (!UMSS_ScheduleWorkItem((PVOID)DeviceExtension, UMSS_BulkOnlyResetPipeAndGetStatus))
        {
            UMSS_KdPrint( DBGLVL_MINIMUM,("Failed to allocate work-item to reset pipe!\n"));
            TRAP();
            UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
        }
    }
    else 
    {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Device failed status phase, ntStatus = %x, URB status = %x, CSW = %x\n",
                      NtStatus,
                      DeviceExtension->Urb->UrbHeader.Status,
                      CSW));

        // An error has occured.  Reset the device.

        if (!UMSS_ScheduleWorkItem((PVOID)DeviceExtension, UMSS_BulkOnlyResetRecovery))
        {
            UMSS_KdPrint( DBGLVL_MINIMUM,("Failed to allocate work-item to reset pipe!\n"));
            TRAP();
            UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
        }
    }

    RETURN(STATUS_MORE_PROCESSING_REQUIRED, UMSS_BulkOnlyGetStatusComplete);
}


CHAR
UMSS_BulkOnlyGetMaxLun(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
Routine Description:

    Queries Bulk-Only device for maximum LUN number

 Arguments:

    DeviceExtension - Our device extension.

Return Value:

    Maximum LUN number for device, or 0 if error occurred.

--*/

{
    PURB Urb=NULL;
    ULONG UrbSize;
    CHAR MaxLun;
    NTSTATUS NtStatus;


    ENTER(UMSS_BulkOnlyGetMaxLun);

    UrbSize = (USHORT) sizeof (struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    Urb = UMSS_ExAllocatePool(NonPagedPool, UrbSize);

    if (!Urb)
    {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Failed to allocate URB, setting max LUN to 0\n"));
        MaxLun = 0;
    }
    else
    {
        // Build the get max lun command
        UsbBuildVendorRequest(
            Urb,
            URB_FUNCTION_CLASS_INTERFACE,
            (USHORT) sizeof (struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
            USBD_TRANSFER_DIRECTION_IN,
            0,
            BULK_ONLY_GET_MAX_LUN,
            0,
            0,
            &MaxLun,
            NULL,
            sizeof(MaxLun),
            NULL
            );

        // Send get max lun command to device
        NtStatus = UMSS_CallUSBD(DeviceExtension->Fdo, Urb);

        if (!NT_SUCCESS(NtStatus))
        {
            UMSS_KdPrint( DBGLVL_MINIMUM,("Get Max LUN command failed, setting max LUN to 0!\n"));
            MaxLun=0;
        }
    }

    if (Urb)
        UMSS_ExFreePool(Urb);

    UMSS_KdPrint( DBGLVL_MINIMUM,("Max LUN = %x\n", MaxLun));

    RETURN(MaxLun, UMSS_BulkOnlyGetMaxLun);
}
