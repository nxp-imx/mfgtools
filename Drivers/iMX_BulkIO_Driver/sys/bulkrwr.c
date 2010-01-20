/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.
--*/

/*---------------------------------------------------------------------------
* Copyright (C) 2009, Freescale Semiconductor, Inc. All Rights Reserved.
* THIS SOURCE CODE, AND ITS USE AND DISTRIBUTION, IS SUBJECT TO THE TERMS
* AND CONDITIONS OF THE APPLICABLE LICENSE AGREEMENT 
*--------------------------------------------------------------------------*/

//---------------------------------------------------------------------------
// Module Name:
//       bulkrwr.c
//
// Abstract:
//       This file has routines to perform reads and writes for bulk transfers.
//
// Environment:
//       Kernel mode
//
//---------------------------------------------------------------------------

#include "private.h"

EVT_WDF_WORKITEM ReadWriteWorkItem;

VOID
ReadWriteBulkEndPoints(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN WDF_REQUEST_TYPE RequestType
    )
/*++

Routine Description:

    This callback is invoked when the framework received  WdfRequestTypeRead or
    WdfRequestTypeWrite request. This read/write is performed in stages of
    MAX_TRANSFER_SIZE. Once a stage of transfer is complete, then the
    request is circulated again, until the requested length of transfer is
    performed.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.

    Request - Handle to a framework request object. This one represents
              the WdfRequestTypeRead/WdfRequestTypeWrite IRP received by the framework.

    Length - Length of the input/output buffer.

Return Value:

   VOID

--*/
{
    PMDL                    newMdl=NULL, requestMdl = NULL;
    PURB                    urb = NULL;
    WDFMEMORY               urbMemory;
    ULONG                   totalLength = Length;
    ULONG                   stageLength = 0;
    ULONG                   urbFlags = 0;
    NTSTATUS                status;
    ULONG_PTR               virtualAddress = 0;
    PREQUEST_CONTEXT        rwContext = NULL;
    PFILE_CONTEXT           fileContext = NULL;
    WDFUSBPIPE              pipe;
    WDF_USB_PIPE_INFORMATION   pipeInfo;
    WDF_OBJECT_ATTRIBUTES   objectAttribs;
    USBD_PIPE_HANDLE        usbdPipeHandle;
    PDEVICE_CONTEXT         deviceContext;

    //DbgPrint("ReadWriteBulkEndPoints - begins\n");

    //
    // First validate input parameters.
    //
    deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

    if (totalLength > deviceContext->MaximumTransferSize) {
        DbgPrint("Transfer length > circular buffer\n");
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((RequestType != WdfRequestTypeRead) &&
        (RequestType != WdfRequestTypeWrite)) {
        DbgPrint("RequestType has to be either Read or Write\n");
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;
    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe, &pipeInfo);

    if(WdfUsbPipeTypeBulk != pipeInfo.PipeType) {
        DbgPrint("Usbd pipe type is not bulk or interrupt\n");
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Exit;

    }

    rwContext = GetRequestContext(Request);

    if(RequestType == WdfRequestTypeRead) {

        status = WdfRequestRetrieveOutputWdmMdl(Request, &requestMdl);
        if(!NT_SUCCESS(status)){
            DbgPrint("WdfRequestRetrieveOutputWdmMdl failed %x\n", status);
            goto Exit;
        }

        urbFlags |= USBD_TRANSFER_DIRECTION_IN;
        rwContext->Read = TRUE;
        DbgPrint("Read operation\n");

    } else {
        status = WdfRequestRetrieveInputWdmMdl(Request, &requestMdl);
        if(!NT_SUCCESS(status)){
            DbgPrint("WdfRequestRetrieveInputWdmMdl failed %x\n", status);
            goto Exit;
        }

        urbFlags |= USBD_TRANSFER_DIRECTION_OUT;
        rwContext->Read = FALSE;
        DbgPrint("Write operation\n");
    }

    urbFlags |= USBD_SHORT_TRANSFER_OK;
    virtualAddress = (ULONG_PTR) MmGetMdlVirtualAddress(requestMdl);

    //
    // the transfer request is for totalLength.
    // we can perform a max of MAX_TRANSFER_SIZE
    // in each stage.
    //
    if (totalLength > MAX_TRANSFER_SIZE) {
        stageLength = MAX_TRANSFER_SIZE;
    }
    else {
        stageLength = totalLength;
    }

    newMdl = IoAllocateMdl((PVOID) virtualAddress,
                           totalLength,
                           FALSE,
                           FALSE,
                           NULL);

    if (newMdl == NULL) {
        DbgPrint("Failed to alloc mem for mdl\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    //
    // map the portion of user-buffer described by an mdl to another mdl
    //
    IoBuildPartialMdl(requestMdl,
                      newMdl,
                      (PVOID) virtualAddress,
                      stageLength);

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
    objectAttribs.ParentObject = Request;

    status = WdfMemoryCreate(&objectAttribs,
                             NonPagedPool,
                             POOL_TAG,
                             sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                             &urbMemory,
                             (PVOID*) &urb);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to alloc mem for urb\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(pipe);

    UsbBuildInterruptOrBulkTransferRequest(urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           usbdPipeHandle,
                                           NULL,
                                           newMdl,
                                           stageLength,
                                           urbFlags,
                                           NULL);

    status = WdfUsbTargetPipeFormatRequestForUrb(pipe, Request, urbMemory, NULL  );
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to format requset for urb\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(Request, ReadWriteCompletion, NULL);

    //
    // set REQUEST_CONTEXT  parameters.
    //
    rwContext->UrbMemory       = urbMemory;
    rwContext->Mdl             = newMdl;
    rwContext->Length          = totalLength - stageLength;
    rwContext->Numxfer         = 0;
    rwContext->VirtualAddress  = virtualAddress + stageLength;

    if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS)) {
        status = WdfRequestGetStatus(Request);
        ASSERT(!NT_SUCCESS(status));
    }

Exit:
    if (!NT_SUCCESS(status)) {
        WdfRequestCompleteWithInformation(Request, status, 0);

        if (newMdl != NULL) {
            IoFreeMdl(newMdl);
        }
    }

    DbgPrint("ReadWriteBulkEndPoints - ends\n");

    return;
}


VOID
ReadWriteCompletion(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
/*++

Routine Description:

    This is the completion routine for reads/writes
    If the irp completes with success, we check if we
    need to recirculate this irp for another stage of
    transfer.

Arguments:

    Context - Driver supplied context
    Device - Device handle
    Request - Request handle
    Params - request completion params

Return Value:
    None

--*/
{
    PMDL                    requestMdl;
    WDFUSBPIPE              pipe;
    ULONG                   stageLength;
    NTSTATUS               status;
    PREQUEST_CONTEXT        rwContext;
    PURB                    urb;
    PCHAR                   operation;
    ULONG                   bytesReadWritten;

    UNREFERENCED_PARAMETER(Context);
    rwContext = GetRequestContext(Request);

    if (rwContext->Read) {
        operation = "Read";
    } else {
        operation = "Write";
    }

    pipe = (WDFUSBPIPE) Target   ;
    status = CompletionParams->IoStatus.Status;

    if (!NT_SUCCESS(status)){
        //
        // Queue a workitem to reset the pipe because the completion could be
        // running at DISPATCH_LEVEL.
        //
        QueuePassiveLevelCallback(WdfIoTargetGetDevice(Target), pipe);
        goto End;
    }

    urb = (PURB) WdfMemoryGetBuffer(rwContext->UrbMemory, NULL);
    bytesReadWritten = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
    rwContext->Numxfer += bytesReadWritten;

    //
    // If there is anything left to transfer.
    //
    if (rwContext->Length == 0) {
        //
        // this is the last transfer
        //
        WdfRequestSetInformation(Request, rwContext->Numxfer);
        goto End;
    }

    //
    // Start another transfer
    //
    DbgPrint("Stage next %s transfer...\n", operation);

    if (rwContext->Length > MAX_TRANSFER_SIZE) {
        stageLength = MAX_TRANSFER_SIZE;
    }
    else {
        stageLength = rwContext->Length;
    }

    //
    // Following call is required to free any mapping made on the partial MDL
    // and reset internal MDL state.
    //
    MmPrepareMdlForReuse(rwContext->Mdl);

    if (rwContext->Read) {
        status = WdfRequestRetrieveOutputWdmMdl(Request, &requestMdl);
        if(!NT_SUCCESS(status)){
            DbgPrint("WdfRequestRetrieveOutputWdmMdl for Read failed %x\n", status);
            goto End;
        }
    } else {
        status = WdfRequestRetrieveInputWdmMdl(Request, &requestMdl);
        if(!NT_SUCCESS(status)){
            DbgPrint("WdfRequestRetrieveInputWdmMdl for Write failed %x\n", status);
            goto End;
        }
    }

    IoBuildPartialMdl(requestMdl,
                      rwContext->Mdl,
                      (PVOID) rwContext->VirtualAddress,
                      stageLength);

    //
    // reinitialize the urb
    //
    urb->UrbBulkOrInterruptTransfer.TransferBufferLength = stageLength;

    rwContext->VirtualAddress += stageLength;
    rwContext->Length -= stageLength;

    //
    // Format the request to send a URB to a USB pipe.
    //
    status = WdfUsbTargetPipeFormatRequestForUrb(pipe,
                                Request,
                                rwContext->UrbMemory,
                                NULL);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to format requset for urb\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    WdfRequestSetCompletionRoutine(Request, ReadWriteCompletion, NULL);

    //
    // Send the request asynchronously.
    //
    if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS)) {
        DbgPrint("WdfRequestSend for %s failed\n", operation);
        status = WdfRequestGetStatus(Request);
        goto End;
    }

    //
    // Else when the request completes, this completion routine will be
    // called again.
    //
    return;

End:
    //
    // We are here because the request failed or some other call failed.
    // Dump the request context, complete the request and return.
    //
    DbgPrintRWContext(rwContext);

    IoFreeMdl(rwContext->Mdl);

    DbgPrint("%s request completed with status 0x%x\n",operation, status);

    WdfRequestComplete(Request, status);

    return;
}

VOID
ReadWriteWorkItem(
    IN WDFWORKITEM  WorkItem
    )
{
    PWORKITEM_CONTEXT pItemContext;
    NTSTATUS status;

    UsbSamp_DbgPrint(3, ("ReadWriteWorkItem called\n"));

    pItemContext = GetWorkItemContext(WorkItem);

    status = ResetPipe(pItemContext->Pipe);
    if (!NT_SUCCESS(status)) {

        UsbSamp_DbgPrint(1, ("ResetPipe failed 0x%x\n", status));

        status = ResetDevice(pItemContext->Device);
        if(!NT_SUCCESS(status)){

            UsbSamp_DbgPrint(1, ("ResetDevice failed 0x%x\n", status));
        }
    }

    WdfObjectDelete(WorkItem);

    return;
}

NTSTATUS
QueuePassiveLevelCallback(
    IN WDFDEVICE    Device,
    IN WDFUSBPIPE   Pipe
    )
/*++

Routine Description:

    This routine is used to queue workitems so that the callback
    functions can be executed at PASSIVE_LEVEL in the conext of
    a system thread.

Arguments:


Return Value:

--*/
{
    NTSTATUS                       status = STATUS_SUCCESS;
    PWORKITEM_CONTEXT               context;
    WDF_OBJECT_ATTRIBUTES           attributes;
    WDF_WORKITEM_CONFIG             workitemConfig;
    WDFWORKITEM                     hWorkItem;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, WORKITEM_CONTEXT);
    attributes.ParentObject = Device;

    WDF_WORKITEM_CONFIG_INIT(&workitemConfig, ReadWriteWorkItem);

    status = WdfWorkItemCreate( &workitemConfig,
                                &attributes,
                                &hWorkItem);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    context = GetWorkItemContext(hWorkItem);

    context->Device = Device;
    context->Pipe = Pipe;

    //
    // Execute this work item.
    //
    WdfWorkItemEnqueue(hWorkItem);

    return STATUS_SUCCESS;
}

VOID
DbgPrintRWContext(
    PREQUEST_CONTEXT rwContext
    )
{
    UNREFERENCED_PARAMETER(rwContext);

    UsbSamp_DbgPrint(3, ("rwContext->UrbMemory       = %p\n",
                         rwContext->UrbMemory));
    UsbSamp_DbgPrint(3, ("rwContext->Mdl             = %p\n",
                         rwContext->Mdl));
    UsbSamp_DbgPrint(3, ("rwContext->Length          = %d\n",
                         rwContext->Length));
    UsbSamp_DbgPrint(3, ("rwContext->Numxfer         = %d\n",
                         rwContext->Numxfer));
    UsbSamp_DbgPrint(3, ("rwContext->VirtualAddress  = %p\n",
                         rwContext->VirtualAddress));
    return;
}


