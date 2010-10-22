/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    Cbi.c 

Abstract:

    Implements control-bulk-interupt protocol for USB Mass Storage devices

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

    06/30/00: MRB  Split out CBI-specific code into this module.

--*/
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#ifdef SUPPORT_CBI

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "umss.h"



VOID
UMSS_CbiStartIo(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
Routine Description:

    Handler for all I/O requests using CB or CBI tranfer protocols.

 Arguments:

    DeviceExtension - Device extension for our FDO.

Return Value:

    NONE

--*/
    
{
    PIOPACKET IoPacket;

    ENTER(UMSS_CbiStartIo);

    IoPacket = DeviceExtension->IoPacket;

    // Send the ADSC request to the device
    // Calls UMSS_CbiSendADSCComplete when transfer completes
    UMSS_ClassSpecificRequest(
        DeviceExtension,
        ACCEPT_DEVICE_SPECIFIC_COMMAND,
        DATA_OUT,
        IoPacket->Cdb,
        IoPacket->CdbLength,
        UMSS_CbiSendADSCComplete
        );

    EXIT(UMSS_CbiStartIo);
}



NTSTATUS
UMSS_CbiSendADSCComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    )
/*++
Routine Description:

    Completion handler for command phase of I/O request.

 Arguments:

    DeviceObject - Previous device object.
    Irp - Irp used for sending command.
    Reference - Our FDO's device extension.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    NTSTATUS NtStatus;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PIOPACKET IoPacket;

    ENTER(UMSS_CbiSendADSCComplete);

    DeviceExtension = (PDEVICE_EXTENSION) Reference;

    IoPacket = DeviceExtension->IoPacket;

    DBG_STATUS(DeviceExtension);

    NtStatus = Irp->IoStatus.Status;
	
    if (!NT_SUCCESS(NtStatus))
    {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Command Block Failure!!!\n"));

        //BUGBUG - Should reset device here?

        // Device failed Command Block, complete with error
        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);

    }
    else if (DeviceExtension->BytesToTransfer)
    {

        // Normally we would simply start transferring the bulk data right now.
        // However, this function is called in the context of the completion
        // handler for the command transfer.  There is a bug in the Windows 
        // 98 golden version of UHCD.SYS that disables bandwidth reclamation 
        // during IRP completion after a control transfer.  This means that
        // any bulk transfers scheduled at this time would only transfer
        // at 1 packet per frame, approximately 16 times slower than normal.
        // To deal with this we schedule a DPC callback that will submit the bulk
        // transfers when bandwidth reclamation is enabled.
	
        UMSS_KdPrint( DBGLVL_HIGH,("Queuing Data Transfer DPC\n"));

        // Queue the DPC
        KeInsertQueueDpc(DeviceExtension->CbiTransferDataDpc, Reference, 0 );

    }
    else if (DeviceExtension->DeviceProtocol == PROTOCOL_CBI) 
    {
        // Device supports interrupt pipe, so get status
        UMSS_CbiGetStatus(DeviceExtension);
    }
    else
    {
        // Device does not report status, so complete request
        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_SUCCESS);
    }

    // All driver-orignated IRPs must return STATUS_MORE_PROCESSING_REQUIRED.
    RETURN(STATUS_MORE_PROCESSING_REQUIRED, UMSS_CbiSendADSCComplete);
}



VOID
UMSS_CbiResetPipe(
    IN PVOID Reference
    )
/*++
Routine Description:

    Worker thread function for resetting bulk pipe after stall.

 Arguments:

    Reference - Our FDO device extension.

Return Value:

    NONE

--*/

{
    PDEVICE_EXTENSION DeviceExtension;

    ENTER(UMSS_CbiResetPipe);

    DeviceExtension = (PDEVICE_EXTENSION) Reference;

    // Reset the appropriate pipe, based on data direction
    UMSS_ResetPipe(
        DeviceExtension->Fdo,
        (DeviceExtension->IoPacket->Flags & IO_FLAGS_DATA_IN) ? 
           DeviceExtension->UsbInterface->Pipes[DeviceExtension->DataInPipe].PipeHandle : 
           DeviceExtension->UsbInterface->Pipes[DeviceExtension->DataOutPipe].PipeHandle
        );

    // Device stalled endpoint, so complete I/O operation with error.
    //BUGBUG is this correct?  Check spec...
    UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);

    EXIT(UMSS_CbiResetPipe);
}




VOID 
UMSS_CbiTransferDataDPC(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
/*++
Routine Description:

    DPC handler used to schedule bulk data transfer to/from the device
    after command phase.

 Arguments:

    Dpc - DPC object.
    DeferredContext - N/A
    SystemArgument1 - Points to our device extension.
    SystemArgument2 - N/A

Return Value:

    NONE

--*/

{
    ENTER(UMSS_CbiTransferDataDPC);

    // Schedule the data transfer
    UMSS_CbiTransferData((PDEVICE_EXTENSION) SystemArgument1);

    EXIT(UMSS_CbiTransferDataDPC);
}



VOID 
UMSS_CbiTransferData(
    PDEVICE_EXTENSION DeviceExtension
    )
/*++
Routine Description:

    Schedules bulk data transfer to/from the device.

 Arguments:

    DeviceExtension - Our device extension.

Return Value:

    None.
--*/
{
    PVOID Buffer;
    ULONG BufferLength;
  
    ENTER(UMSS_CbiTransferData);

    // Get next data buffer element, if any.
    Buffer = UMSS_GetBuffer(DeviceExtension, &BufferLength);

    if (NULL == Buffer)
    {
        //Done with data phase, so move to status phase if (supported)

        if (DeviceExtension->DeviceProtocol == PROTOCOL_CBI)
        {
            // Device supports interrupt pipe, so get status
            UMSS_CbiGetStatus(DeviceExtension);
        }
        else
        {
            // No interrupt pipe, so just complete the request
            UMSS_CompleteRequest(DeviceExtension, IO_STATUS_SUCCESS);
        }
    }
    else
    {
        // Transfer next element of the data phase
        UMSS_BulkTransfer(
            DeviceExtension,
            (UCHAR)((DeviceExtension->IoPacket->Flags & IO_FLAGS_DATA_IN) ?
               DATA_IN : DATA_OUT),
            Buffer,
            BufferLength,
            UMSS_CbiTransferDataComplete
            );
    }

    EXIT(UMSS_CbiTransferData);
}

              
NTSTATUS
UMSS_CbiTransferDataComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    )
/*++
Routine Description:

    Completion handler for bulk data transfer

 Arguments:

    DeviceObject - Previous device object in the stack.
    Irp - Irp being completed.
    Reference - Our FDO's device extension.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    NTSTATUS NtStatus;
    PDEVICE_EXTENSION DeviceExtension;
    BOOLEAN Scheduled;

    ENTER(UMSS_CbiTransferDataComplete);

    DeviceExtension = (PDEVICE_EXTENSION) Reference;

    DBG_STATUS(DeviceExtension);

    NtStatus = Irp->IoStatus.Status;

    if (!NT_SUCCESS(NtStatus))
    {
        //Device failed Data Transfer
        // Check if we need to clear stalled pipe
        if (USBD_HALTED(DeviceExtension->Urb->UrbHeader.Status))
        {

            // Reset pipe can only be done at passive level, so we need
            // to schedule a work item to do it.
            if (!UMSS_ScheduleWorkItem((PVOID)DeviceExtension, UMSS_CbiResetPipe))
            {
                UMSS_KdPrint( DBGLVL_MINIMUM,("Failed to allocate work-item to reset pipe!\n"));
                TRAP();
                UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
            }
        }
        else
        {
            UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
        }
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Transfer succeeded
    //

    UMSS_CbiTransferData(DeviceExtension);

    RETURN(STATUS_MORE_PROCESSING_REQUIRED, UMSS_CbiTransferDataComplete);
}


VOID 
UMSS_CbiGetStatus(
    PDEVICE_EXTENSION DeviceExtension
    )
/*++
Routine Description:

    Schedules an interrupt transfer from the device to get status.

 Arguments:

    DeviceExtension - Our FDO's device extension.

Return Value:

    NONE

--*/

{
    PURB Urb;
    PIRP Irp;
    USBD_PIPE_HANDLE PipeHandle;
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION NextStack;

    ENTER("UMSS_CbiGetStatus");

    Urb = DeviceExtension->Urb;
    Irp = DeviceExtension->Irp;
    PipeHandle = DeviceExtension->UsbInterface->Pipes[DeviceExtension->StatusPipe].PipeHandle;

    // Build a URB for our interrupt transfer
    UsbBuildInterruptOrBulkTransferRequest(
        Urb,
        sizeof (struct _URB_BULK_OR_INTERRUPT_TRANSFER),
        PipeHandle,
        (PVOID)&(DeviceExtension->Idb),
        NULL,
        sizeof(INTERRUPT_DATA_BLOCK),
        USBD_TRANSFER_DIRECTION_IN,
        NULL
        );

    NextStack = IoGetNextIrpStackLocation(Irp);
    
    UMSS_ASSERT(NextStack != NULL);
    UMSS_ASSERT(DeviceExtension->Fdo->StackSize>1);

    NextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextStack->Parameters.Others.Argument1 = Urb;
    NextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
	
    IoSetCompletionRoutine(
        Irp,
        UMSS_CbiGetStatusComplete,
        (PVOID)DeviceExtension, 
        TRUE,    // invoke on success
        TRUE,    // invoke on error
        TRUE     // invoke on cancellation of the Irp
        );

    // Pass Irp to the USB driver stack without checking status.
    // We don't look at the return status since we will 
    // always complete the IOS request in our
    // completion handler, regardless of status.

    // Call USB driver stack
    IoCallDriver(DeviceExtension->TopOfStackDeviceObject, Irp);

    EXIT(UMSS_CbiGetStatus);
}


NTSTATUS
UMSS_CbiGetStatusComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    )
/*++
Routine Description:

    Completion handler for interrupt status transfer

 Arguments:

    DeviceObject - Previous device object in the stack.
    Irp - Irp being completed.
    Reference - Our FDO's device extension.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    NTSTATUS NtStatus;
    PDEVICE_EXTENSION DeviceExtension;
    PINTERRUPT_DATA_BLOCK Idb;

    ENTER(UMSS_CbiGetStatusComplete);

    DeviceExtension = (PDEVICE_EXTENSION) Reference;

    NtStatus = Irp->IoStatus.Status;

    if (!NT_SUCCESS(NtStatus))
    {
        //Device failed Data Transfer
        // Check if we need to clear stalled pipe
        if (USBD_HALTED(DeviceExtension->Urb->UrbHeader.Status))
        {
            if (!UMSS_ScheduleWorkItem((PVOID)DeviceExtension, UMSS_CbiResetPipe))
            {
                UMSS_KdPrint( DBGLVL_MINIMUM,("Failed to allocate work-item to reset pipe!\n"));
                TRAP();

                UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);

                RETURN(STATUS_MORE_PROCESSING_REQUIRED, UMSS_CbiGetStatusComplete);
            }
        }
    }

    //
    // Interrupt transfer succeeded
    //

    Idb = &(DeviceExtension->Idb);

    // Check for an error in the status block
    if ((0 != Idb->bType) || (0 != (Idb->bValue & 0x3)))
    {
        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
    }
    else
    {
        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_SUCCESS);
    }

    RETURN(STATUS_MORE_PROCESSING_REQUIRED, UMSS_CbiGetStatusComplete);
}

#endif
