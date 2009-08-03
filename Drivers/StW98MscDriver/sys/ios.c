/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    Usbios.c 

Abstract:

    Sample USB device driver for USB Mass Storage devices
    Interface for IOS I/O requests

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

    01/13/99: MRB  Adapted from BULKUSB DDK sample.

--*/
#define GLOBAL_VARS

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "umss.h"




PDEVICE_OBJECT _stdcall
UMSS_GetNextPDO(
    IN PDEVICE_OBJECT *Fdo
    )
/*++
Routine Description:

    Called by the IOS port driver. Walks list of FDOs 
    associated with this driver, returning the PDO the FDO
    is attached to.  Allows IOS port driver to distinguish
    between multiple instances of the USB device.

 Arguments:

    DeviceObject -- ptr to previous FDO, or NULL if returning 1st PDO

Return Value:

    Next PDO, or
    NULL if no more FDOs associated with this driver.

--*/

{
    PDEVICE_EXTENSION DeviceExtension;

    ENTER(UMSS_GetNextPDO);

    // First Fdo?
    if (NULL == *Fdo)
    {

        // Get first Device Object associated with this driver
        *Fdo = UMSSDriverObject->DeviceObject;

    }
    else
    {
        // Get next device object for this driver
        *Fdo = (*Fdo)->NextDevice;
    }

    // No more device obects?
    if (NULL == *Fdo)
        return NULL;

    //Filter out our own child PDOs
    DeviceExtension = (*Fdo)->DeviceExtension;

    while (DO_FDO != DeviceExtension->DeviceObjectType)
    {
        *Fdo = (*Fdo)->NextDevice;
        if (NULL == *Fdo)
            return NULL;
        DeviceExtension = (*Fdo)->DeviceExtension;
    }

    RETURN(DeviceExtension->PhysicalDeviceObject, UMSS_GetNextPDO);
}
	


VOID _stdcall
UMSS_RegisterCompletionHandler(
    PDEVICE_OBJECT DeviceObject,
    COMPLETION_HANDLER CompletionHandler
    )
/*++
Routine Description:

    Called by the IOS port driver. Registers a completion
    handler in the port driver that is called whenever
    we complete an I/O request.

 Arguments:

    DeviceObject -- FDO we are registering the completion handler for.
    CompletionHandler - Address of completion handler.

Return Value:

    NONE

--*/

{
    PDEVICE_EXTENSION DeviceExtension;

    ENTER(UMSS_RegisterCompletionHandler);

    DeviceExtension = DeviceObject->DeviceExtension;
    DeviceExtension->CompleteRequest = CompletionHandler;

    EXIT(UMSS_RegisterCompletionHandler);

}


CHAR _stdcall
UMSS_GetMaxLun(
    IN PDEVICE_OBJECT Fdo
    )
/*++
Routine Description:

    Called by the IOS port driver. returns maximum LUN number for device.

 Arguments:

    DeviceObject -- FDO of device

Return Value:

    Maximum LUN number of device.

--*/

{
    PDEVICE_EXTENSION DeviceExtension;

    DeviceExtension = Fdo->DeviceExtension;

    return (DeviceExtension->MaxLun);
}



VOID _stdcall
UMSS_StartRequest(
    IN PIOPACKET IoPacket
    )
/*++
Routine Description:

    Called by the IOS port driver to start an I/O request

    Please Note!! - All USB transfers that occur in response
    to this request must be done asynchronously.  IOS and IFSMGR
    implements their own thread blocking mechanisms, and it
    is possible to create a race condition if we also block
    waiting for a USB transfer to complete.

 Arguments:

    IoPacket -- Structure containing pertient information about the
                I/O request.

Return Value:

    NONE

--*/

{
    PURB Urb;
    PIRP Irp;
    CHAR StackSize;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION NextStack;
    ULONG UrbSize;
    ULONG i;
    PDEVICE_OBJECT DeviceObject;

    ENTER(UMSS_StartRequest);

    UMSS_KdPrint( DBGLVL_HIGH,("DataBuffer=%x, DataLength=%x, Flags=%x, Iop=%x\n",
                     IoPacket->DataBuffer,
                     IoPacket->DataLength,
                     IoPacket->Flags,
                     IoPacket->Iop));

#ifdef DBG
    if (IoPacket->CdbLength > 16)
    {
        UMSS_KdPrint( DBGLVL_DEFAULT,("CDB Length = %d, should be <= 16\n", IoPacket->CdbLength));
        TRAP();
    }

    // Dump CDB contents
    for (i=0; i<IoPacket->CdbLength; i++)
                UMSS_KdPrint( DBGLVL_HIGH,("  CDB[%x]=%x\n", i,((char*)(IoPacket->Cdb))[i]));
#endif

    DeviceObject = (PDEVICE_OBJECT)(IoPacket->Fdo);
    DeviceExtension = DeviceObject->DeviceExtension;

    // Increment I/O count.  It will decrement when I/O request completes.
    UMSS_IncrementIoCount(DeviceObject);

    DeviceExtension->IoPacket = IoPacket;

    // Make sure device is still present
    if (DeviceExtension->DeviceRemoved)
    {
        UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_StartRequest - device removed, returning error\n"));
        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
        EXIT(UMSS_StartRequest);
        return;
    }

    // Allocate IRP for our I/O request
    StackSize = (CCHAR)(DeviceExtension->TopOfStackDeviceObject->StackSize + 1);
    Irp = IoAllocateIrp(StackSize, FALSE);

    if (!Irp) {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Failure due to memory error\n"));

        // Can't allocate IRP - complete request with error and return
        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_OUT_OF_MEMORY);
        EXIT(UMSS_StartRequest);
        return;
    }
	
    // Allocate URB.  It will be used for control, bulk, and interrupt
    // transfers, so insure it is large enough for both types of URBs
    UrbSize = max(sizeof (struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                  sizeof (struct _URB_BULK_OR_INTERRUPT_TRANSFER) );

    Urb = UMSS_ExAllocatePool(NonPagedPool, UrbSize);

    if (!Urb) {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Failure due to memory error\n"));

        IoFreeIrp(Irp);

        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_OUT_OF_MEMORY);
        EXIT(UMSS_StartRequest);
        return;
    }



    // Store all the transfer request info in our device extension
    DeviceExtension->Urb = Urb;
    DeviceExtension->Irp = Irp;
    DeviceExtension->CurrentSGD = 0;
    DeviceExtension->IoPacket = IoPacket;

    // Is there a data phase?
    if (IoPacket->Flags & (IO_FLAGS_DATA_IN | IO_FLAGS_DATA_OUT))
        DeviceExtension->BytesToTransfer = IoPacket->DataLength;
    else
        DeviceExtension->BytesToTransfer = 0;


    if (DeviceExtension->DeviceProtocol == PROTOCOL_BULKONLY)
    {
        UMSS_BulkOnlyStartIo(DeviceExtension);
    }
#ifdef SUPPORT_CBI
    else if ((DeviceExtension->DeviceProtocol == PROTOCOL_CBI) ||
             (DeviceExtension->DeviceProtocol == PROTOCOL_CB))
    {
        UMSS_CbiStartIo(DeviceExtension);
    }
#endif
    else
    {
        UMSS_CompleteRequest(DeviceExtension, IO_STATUS_DEVICE_ERROR);
    }
    EXIT(UMSS_StartRequest);
}


VOID
UMSS_CompleteRequest(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG Status
    )
/*++
Routine Description:

    Completes IOS request by calling back into the IOS port driver.
    
 Arguments:

    DeviceExtension - Our device extension
    Status - Status of I/O request being completed

Return Value:

    NONE

--*/

{
    PIOPACKET IoPacket = DeviceExtension->IoPacket;
    PDEVICE_OBJECT DeviceObject;

    ENTER(UMSS_CompleteRequest);

    DeviceObject = DeviceExtension->Fdo;
    DeviceExtension->IoPacket->Status = Status;
    if (DeviceExtension->Irp)
    {
        IoFreeIrp(DeviceExtension->Irp);
    }

    if (DeviceExtension->Urb)
    {
        UMSS_ExFreePool(DeviceExtension->Urb);
    }

    DeviceExtension->IoPacket = NULL;
    DeviceExtension->Urb = NULL;
    DeviceExtension->Irp = NULL;
    DeviceExtension->CurrentSGD = 0;

	if (DeviceExtension->InternalRequest)
		KeSetEvent(&DeviceExtension->InternalRequestCompleteEvent, 0, FALSE);
	else
	    DeviceExtension->CompleteRequest(IoPacket);

    // I/O request completed, so decrement I/O count on FDO.
    UMSS_DecrementIoCount(DeviceObject);

    EXIT(UMSS_CompleteRequest);
}


/*
VOID _stdcall
UMSS_IosTimeout(
    PDEVICE_OBJECT DeviceObject,
    )
/*++
Routine Description:

    Called by the IOS port driver when an IOS timeout occurs.
    Cancel any pending I/O and reset the device.

 Arguments:

    DeviceObject -- FDO for our device.

Return Value:

    None.        
--/
{
//mrbmrb - to be implemented
    // See if we have a pending I/O request
    if (NULL != DeviceExtension-IoPacket)
    {
        
*/


PVOID
UMSS_GetBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG* BufferSize
    )
/*++
Routine Description:

    Parses IOS scatter-gather descriptors and returns next data buffer
    fragment to be used in transferring data from/to device.

 Arguments:

    DeviceExtension - Our device extension.
    BufferSize - ULONG to recieve transfer length.

Return Value:
    Extracted data buffer, or NULL if no more data to transfer.

--*/

{
    BlockDev_Scatter_Gather * Sgd;
    PVOID Buffer;

    ENTER(UMSS_GetBuffer);

    if (0 == DeviceExtension->BytesToTransfer)
    {
        RETURN(NULL, UMSS_GetBuffer);
    }

    // If we are doing scatter gather, we will need to find the current
    // SGD element and use its data buffer
    if (DeviceExtension->IoPacket->Flags & IO_FLAGS_SCATTER_GATHER)
    {
        // Retrieve the SGD list stored in the IoPacket
        Sgd = (BlockDev_Scatter_Gather *)(DeviceExtension->IoPacket->DataBuffer);

        // Extract SGD's data buffer pointer
        Buffer = (PVOID)Sgd[DeviceExtension->CurrentSGD].BD_SG_Buffer_Ptr;

        // SGD length is block count, need to convert to byte count
        // and increment the current SGD counter
        *BufferSize = Sgd[DeviceExtension->CurrentSGD++].BD_SG_Count *
                         DeviceExtension->IoPacket->BlockSize;

        UMSS_KdPrint( DBGLVL_HIGH,("SGDBuffer=%x, BuffSize=%x\n", Buffer, *BufferSize));
    } else
    {
        // No SGD, just a flat linear data buffer
        Buffer = (PVOID)DeviceExtension->IoPacket->DataBuffer;
        *BufferSize = DeviceExtension->BytesToTransfer;
    }

    DeviceExtension->BytesToTransfer -= *BufferSize;

    RETURN(Buffer, UMSS_GetBuffer);
}


VOID
UMSS_ClassSpecificRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR Request,
    IN UCHAR TransferDirection,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
    )
/*++
Routine Description:

    Sends a class-specific request to the device

    NOTE: Assumes device extension has valid IRP and URB.

 Arguments:

    DeviceExtension - Our device extension.
    Request - Class specific request value.
    TransferDirection - Direction of data transfer.
    Buffer - Address of data buffer to use for data transfer.
    BufferLength - Number of bytes to transfer.
    CompletionRoutine - Function to call when request completes.

Return Value:

    NONE.

--*/

{
    PIRP Irp;
    PURB Urb;
    PIO_STACK_LOCATION NextStack;

    ENTER(UMSS_ClassSpecificRequest);

    Irp = DeviceExtension->Irp;
    Urb = DeviceExtension->Urb;

    // Build URB for the ADSC command
    UsbBuildVendorRequest(
        Urb,
        URB_FUNCTION_CLASS_INTERFACE,
        (USHORT) sizeof (struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
        (DATA_IN == TransferDirection) ? USBD_TRANSFER_DIRECTION_IN : 0,
        0,
        Request,
        0,
        0,
        Buffer,
        NULL,
        BufferLength,
        NULL
        );

    NextStack = IoGetNextIrpStackLocation(Irp);
    UMSS_ASSERT(NextStack != NULL);
    UMSS_ASSERT(DeviceExtension->Fdo->StackSize>1);

    // Initialize our Irp
    NextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextStack->Parameters.Others.Argument1 = Urb;
    NextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    // Register our Irp completion handler
    IoSetCompletionRoutine(
        Irp,
        CompletionRoutine,
        DeviceExtension, 
        TRUE,    // invoke on success
        TRUE,    // invoke on error
        TRUE     // invoke on cancellation of the Irp
        );

    // Pass Irp to the USB driver stack
    IoCallDriver(DeviceExtension->TopOfStackDeviceObject, Irp);

    EXIT(UMSS_ClassSpecificRequest);
}


VOID
UMSS_BulkTransfer(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR TransferDirection,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
    )
/*++
Routine Description:

    Initiates a bulk transfer with the USB device.

 Arguments:

    DeviceExtension - Our device extension.
    TransferDirection - Direction of data transfer.
    Buffer - Address of data buffer to use for data transfer.
    BufferLength - Number of bytes to transfer.
    CompletionRoutine - Function to call when request completes.

Return Value:

    NONE

--*/

{
    PIO_STACK_LOCATION NextStack;
    USBD_PIPE_HANDLE PipeHandle;
    PIRP Irp;
    PURB Urb;

    ENTER(UMSS_DoBulkTransfer);

    if (DATA_IN == TransferDirection)
        PipeHandle = DeviceExtension->UsbInterface->Pipes[DeviceExtension->DataInPipe].PipeHandle;
    else
        PipeHandle = DeviceExtension->UsbInterface->Pipes[DeviceExtension->DataOutPipe].PipeHandle;

    Urb = DeviceExtension->Urb;
    Irp = DeviceExtension->Irp;

    // Build a URB for our bulk transfer
    UsbBuildInterruptOrBulkTransferRequest(
        Urb,
        sizeof (struct _URB_BULK_OR_INTERRUPT_TRANSFER),
        PipeHandle,
        Buffer,
        NULL,
        BufferLength,
        (DATA_OUT == TransferDirection) ? USBD_TRANSFER_DIRECTION_OUT :
                    USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
        NULL
        );

    NextStack = IoGetNextIrpStackLocation(Irp);

    UMSS_ASSERT(NextStack != NULL);
    UMSS_ASSERT(DeviceExtension->Fdo->StackSize>1);

    // Initialize our Irp
    NextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextStack->Parameters.Others.Argument1 = Urb;
    NextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    // Register our Irp completion handler
    IoSetCompletionRoutine(
        Irp,
        CompletionRoutine,
        DeviceExtension, 
        TRUE,    // invoke on success
        TRUE,    // invoke on error
        TRUE     // invoke on cancellation of the Irp
        );

    // Pass Irp to the USB driver stack
    IoCallDriver(DeviceExtension->TopOfStackDeviceObject, Irp);

    EXIT(UMSS_DoBulkTransfer);
}



BOOLEAN
UMSS_ScheduleWorkItem(
    PVOID Context,
    UMSS_WORKER_ROUTINE Routine
    )
/*++
Routine Description:

    Wrapper for handling worker thread callbacks

 Arguments:

    Routine - Routine to be called when this work-item is processed
    Context - Value to be passed to worker routine

Return Value:
    TRUE if work item queued
    FALSE if work item not queued

--*/

{
    BOOLEAN RetVal = TRUE;
    PWORK_QUEUE_ITEM WorkItem;
    PUMSS_WORKER_PACKET WorkerPacket;

    ENTER(UMSS_ScheduleWorkItem);

    WorkItem = UMSS_ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
    WorkerPacket = UMSS_ExAllocatePool(NonPagedPool, sizeof(UMSS_WORKER_PACKET));     

    if ((WorkItem) && (WorkerPacket))
    {
        WorkerPacket->Routine = Routine;
        WorkerPacket->Context = Context;
        WorkerPacket->WorkItem = WorkItem;

        // Initialize the work-item
        ExInitializeWorkItem(
            WorkItem,
            UMSS_Worker,
            WorkerPacket
            );

        // Schedule the work-item
        ExQueueWorkItem(
            WorkItem,
            DelayedWorkQueue
            );

        UMSS_KdPrint( DBGLVL_MINIMUM,("Work-item queued\n"));
    }
    else
    {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Failed to allocate work-item\n"));

        if (WorkItem)
            UMSS_ExFreePool(WorkItem);

        if (WorkerPacket)
            UMSS_ExFreePool(WorkerPacket);

        RetVal = FALSE;
    }

    RETURN(RetVal, UMSS_ScheduleWorkItem);
}


VOID 
UMSS_Worker(
    IN PVOID Reference
    )
/*++
Routine Description:

    Wrapper for worker thread function.  Handles cleaning up work-item, etc.,
    before calling real worker function.

 Arguments:

    Reference - Context value to be passed to worker function.

Return Value:

    NONE

--*/

{
    PUMSS_WORKER_PACKET WorkerPacket;

    ENTER(UMSS_Worker);

    WorkerPacket = (PUMSS_WORKER_PACKET)Reference;

    WorkerPacket->Routine(WorkerPacket->Context);

    UMSS_ExFreePool(WorkerPacket->WorkItem);
    UMSS_ExFreePool(WorkerPacket);

    EXIT(UMSS_Worker);
}
