/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Queue.c

Abstract:

    This file contains dispatch routines for create,
    close, device-control, read & write.

Environment:

    Kernel mode

--*/
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IMX_DeviceFileCreate)
#pragma alloc_text(PAGE, IMX_DeviceIoControl)
#pragma alloc_text(PAGE, IMX_DeviceIoRead)
#pragma alloc_text(PAGE, IMX_DeviceIoWrite)
#pragma alloc_text(PAGE, GetPipeFromName)
#pragma alloc_text(PAGE, ResetPipe)
#pragma alloc_text(PAGE, ResetDevice)
#pragma alloc_text(PAGE, StopAllPipes)
#pragma alloc_text(PAGE, StartAllPipes)
#endif


VOID
IMX_DeviceFileCreate(
    IN WDFDEVICE            Device,
    IN WDFREQUEST           Request,
    IN WDFFILEOBJECT        FileObject
    )
/*++

Routine Description:

    The framework calls the driver's IMX_DeviceFileCreate callback
    when the framework receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing a file.
    This callback is called synchronously, in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - copy of the create IO_STACK_LOCATION

Return Value:

   NT status code

--*/
{
    NTSTATUS                    status = STATUS_UNSUCCESSFUL;
    PUNICODE_STRING             fileName;
    PFILE_CONTEXT               pFileContext;
    PDEVICE_CONTEXT             pDevContext;
    WDFUSBPIPE                  pipe;

    DbgPrint("IMX_DeviceFileCreate - begins\n");

    PAGED_CODE();

    //
    // initialize variables
    //
    pDevContext = GetDeviceContext(Device);
    pFileContext = GetFileContext(FileObject);


    fileName = WdfFileObjectGetFileName(FileObject);
    
    DbgPrint("IMX_DeviceFileCreate: The length of fileName = 0x%x\n", fileName->Length);


    if (0 == fileName->Length) {
        //
        // opening a device as opposed to pipe.
        //
        status = STATUS_SUCCESS;
    }
    else {
        pipe = GetPipeFromName(pDevContext, fileName);

        if (pipe != NULL) {
            //
            // found a match
            //
            pFileContext->Pipe = pipe;

            WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    WdfRequestComplete(Request, status);

    DbgPrint("IMX_DeviceFileCreate - ends\n");

    return;
}

VOID
IMX_DeviceIoControl(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     OutputBufferLength,
    IN size_t     InputBufferLength,
    IN ULONG      IoControlCode
    )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.
Return Value:

    VOID

--*/
{
    WDFDEVICE          device;
    PVOID              ioBuffer;
    size_t             bufLength;
    NTSTATUS           status;
    PDEVICE_CONTEXT    pDevContext;
    PFILE_CONTEXT      pFileContext;
    ULONG              length = 0;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    DbgPrint("Entered IMX_DeviceIoControl\n");

    PAGED_CODE();

    //
    // initialize variables
    //
    device = WdfIoQueueGetDevice(Queue);
    pDevContext = GetDeviceContext(device);

    switch(IoControlCode) {
    case IOCTL_IMXDEVICE_GET_DEVICE_DESCRIPTOR:
         
         DbgPrint("   IOCTL_IMXDEVICE_GET_DEVICE_DESCRIPTOR\n");

         length = pDevContext->UsbDeviceDescriptor.bLength;
         
         status = WdfRequestRetrieveOutputBuffer(Request, length, &ioBuffer, &bufLength);
         if(!NT_SUCCESS(status)){
                DbgPrint("WdfRequestRetrieveInputBuffer failed\n");
                break;
         }
         
         RtlCopyMemory(ioBuffer,
                       &pDevContext->UsbDeviceDescriptor,
                       length);
                       
         break;

    case IOCTL_IMXDEVICE_RESET_PIPE:

        DbgPrint("   IOCTL_IMXDEVICE_RESET_PIPE\n");

        pFileContext = GetFileContext(WdfRequestGetFileObject(Request));

        if (pFileContext->Pipe == NULL) {
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            status = ResetPipe(pFileContext->Pipe);
        }

        break;

    case IOCTL_IMXDEVICE_GET_CONFIG_DESCRIPTOR:

        DbgPrint("   IOCTL_IMXDEVICE_GET_CONFIG_DESCRIPTOR\n");

        if (pDevContext->UsbConfigurationDescriptor) {

            length = pDevContext->UsbConfigurationDescriptor->wTotalLength;

            status = WdfRequestRetrieveOutputBuffer(Request, length, &ioBuffer, &bufLength);
            if(!NT_SUCCESS(status)){
                DbgPrint("WdfRequestRetrieveInputBuffer failed\n");
                break;
            }

            RtlCopyMemory(ioBuffer,
                          pDevContext->UsbConfigurationDescriptor,
                          length);

            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_INVALID_DEVICE_STATE;
        }

        break;

    case IOCTL_IMXDEVICE_RESET_DEVICE:

        DbgPrint("   IOCTL_IMXDEVICE_RESET_DEVICE\n");
        status = ResetDevice(device);
        break;

    default :
        DbgPrint("   STATUS_INVALID_DEVICE_REQUEST\n");
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, length);

    DbgPrint("Exit IMX_DeviceIoControl\n");

    return;
}

VOID
IMX_DeviceIoRead(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
/*++

Routine Description:

    Called by the framework when it receives Read requests.

Arguments:

    Queue - Default queue handle
    Request - Handle to the read/write request
    Lenght - Length of the data buffer associated with the request.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:


--*/
{
    PFILE_CONTEXT           fileContext = NULL;
    WDFUSBPIPE              pipe;
    WDF_USB_PIPE_INFORMATION   pipeInfo;

    PAGED_CODE();

    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;
    if (pipe == NULL) {
        DbgPrint("IMX_DeviceIoRead: pipe handle is NULL\n");
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, 0);
        return;
    }
    
    DbgPrint("IMX_DeviceIoRead: pipe handle is NOT NULL\n");
    
    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe, &pipeInfo);

    if((WdfUsbPipeTypeBulk == pipeInfo.PipeType) ||
       (WdfUsbPipeTypeInterrupt == pipeInfo.PipeType)) {

        ReadWriteBulkEndPoints(Queue, Request, (ULONG) Length, WdfRequestTypeRead);
        return;

    } else if(WdfUsbPipeTypeIsochronous == pipeInfo.PipeType){

        DbgPrint("ISO transfer is not supported for buffered I/O transfer\n");
        return;
    }

    WdfRequestCompleteWithInformation(Request, STATUS_INVALID_DEVICE_REQUEST, 0);

    return;
}

VOID
IMX_DeviceIoWrite(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
/*++

Routine Description:

    Called by the framework when it receives Write requests.

Arguments:

    Queue - Default queue handle
    Request - Handle to the read/write request
    Lenght - Length of the data buffer associated with the request.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:


--*/
{
    PFILE_CONTEXT           fileContext = NULL;
    WDFUSBPIPE              pipe;
    WDF_USB_PIPE_INFORMATION   pipeInfo;

    PAGED_CODE();

    //
    // Get the pipe associate with this request.
    //
    fileContext = GetFileContext(WdfRequestGetFileObject(Request));
    pipe = fileContext->Pipe;
    if (pipe == NULL) {
        DbgPrint("pipe handle is NULL\n");
        WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, 0);
        return;
    }
    WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
    WdfUsbTargetPipeGetInformation(pipe, &pipeInfo);

    if((WdfUsbPipeTypeBulk == pipeInfo.PipeType) ||
       (WdfUsbPipeTypeInterrupt == pipeInfo.PipeType)) {

        ReadWriteBulkEndPoints(Queue, Request, (ULONG) Length, WdfRequestTypeWrite);
        return;

    } else if(WdfUsbPipeTypeIsochronous == pipeInfo.PipeType){

        DbgPrint("ISO transfer is not supported for buffered I/O transfer\n");
        return;
    }

    WdfRequestCompleteWithInformation(Request, STATUS_INVALID_DEVICE_REQUEST, 0);

    return;
}

WDFUSBPIPE
GetPipeFromName(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUNICODE_STRING FileName
    )
/*++

Routine Description:

    This routine will pass the string pipe name and
    fetch the pipe handle.

Arguments:

    DeviceContext - pointer to Device Context

    FileName - string pipe name

Return Value:

    The device extension maintains a pipe context for
    the pipes on i.MX board.

--*/
{
    LONG                  ix;
    ULONG                 uval;
    ULONG                 nameLength;
    ULONG                 umultiplier;
    WDFUSBPIPE            pipe = NULL;

    PAGED_CODE();

    //
    // typedef WCHAR *PWSTR;
    //
    nameLength = (FileName->Length / sizeof(WCHAR));

    DbgPrint("GetPipeFromName - begins\n");

    if(nameLength != 0) {

        DbgPrint("Filename = %wZ nameLength = %d\n", FileName, nameLength);

        //
        // Parse the pipe#
        //
        ix = nameLength - 1;

        // if last char isn't digit, decrement it.
        while((ix > -1) &&
              ((FileName->Buffer[ix] < (WCHAR) '0')  ||
               (FileName->Buffer[ix] > (WCHAR) '9')))             {

            ix--;
        }

        if (ix > -1) {

            uval = 0;
            umultiplier = 1;

            // traversing least to most significant digits.

            while((ix > -1) &&
                  (FileName->Buffer[ix] >= (WCHAR) '0') &&
                  (FileName->Buffer[ix] <= (WCHAR) '9'))          {

                uval += (umultiplier *
                         (ULONG) (FileName->Buffer[ix] - (WCHAR) '0'));

                ix--;
                umultiplier *= 10;
            }
            pipe = WdfUsbInterfaceGetConfiguredPipe(
                DeviceContext->UsbInterface,
                (UCHAR)uval, //PipeIndex,
                NULL
                );

        }
    }

    DbgPrint("GetPipeFromName - ends\n");

    return pipe;
}

NTSTATUS
ResetPipe(
    IN WDFUSBPIPE Pipe
    )
/*++

Routine Description:

    This routine resets the pipe.

Arguments:

    Pipe - framework pipe handle

Return Value:

    NT status value

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    //
    //  This routine synchronously submits a URB_FUNCTION_RESET_PIPE
    // request down the stack.
    //
    status = WdfUsbTargetPipeResetSynchronously(Pipe,
                                WDF_NO_HANDLE, // WDFREQUEST
                                NULL  // PWDF_REQUEST_SEND_OPTIONS
                                );

    if (NT_SUCCESS(status)) {
        DbgPrint("ResetPipe - success\n");
        status = STATUS_SUCCESS;
    }
    else {
        DbgPrint("ResetPipe - failed\n");
    }

    return status;
}

VOID
StopAllPipes(
    IN PDEVICE_CONTEXT DeviceContext
    )
{
    UCHAR count,i;

    PAGED_CODE();

	count = DeviceContext->NumberConfiguredPipes;
    for (i = 0; i < count; i++) {
        WDFUSBPIPE pipe;
        pipe = WdfUsbInterfaceGetConfiguredPipe(DeviceContext->UsbInterface,
                                                i, //PipeIndex,
                                                NULL
                                                );
        WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(pipe),
                        WdfIoTargetCancelSentIo);
    }
}


VOID
StartAllPipes(
    IN PDEVICE_CONTEXT DeviceContext
    )
{
    NTSTATUS status;
    UCHAR count,i;

    PAGED_CODE();

    count = DeviceContext->NumberConfiguredPipes;
    for (i = 0; i < count; i++) {
        WDFUSBPIPE pipe;
        pipe = WdfUsbInterfaceGetConfiguredPipe(DeviceContext->UsbInterface,
                                                i, //PipeIndex,
                                                NULL
                                                );
        status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(pipe));
        if (!NT_SUCCESS(status)) {
            DbgPrint("StartAllPipes - failed pipe #%d\n", i);
        }
    }
}

NTSTATUS
ResetDevice(
    IN WDFDEVICE Device
    )
/*++

Routine Description:

    This routine calls WdfUsbTargetDeviceResetPortSynchronously to reset the device if it's still
    connected.

Arguments:

    Device - Handle to a framework device

Return Value:

    NT status value

--*/
{
    PDEVICE_CONTEXT pDeviceContext;
    NTSTATUS status;

    DbgPrint("ResetDevice - begins\n");

    PAGED_CODE();

    pDeviceContext = GetDeviceContext(Device);
    
    //
    // A reset-device
    // request will be stuck in the USB until the pending transactions
    // have been canceled. Similarly, if there are pending tranasfers on the BULK
    // IN/OUT pipe cancel them.
    // To work around this issue, the driver should stop the continuous reader
    // (by calling WdfIoTargetStop) before resetting the device, and restart the
    // continuous reader (by calling WdfIoTargetStart) after the request completes.
    //
    StopAllPipes(pDeviceContext);
    
    //
    // It may not be necessary to check whether device is connected before
    // resetting the port.
    //
    status = WdfUsbTargetDeviceIsConnectedSynchronous(pDeviceContext->WdfUsbTargetDevice);

    if(NT_SUCCESS(status)) {
        //status = WdfUsbTargetDeviceResetPortSynchronously(pDeviceContext->WdfUsbTargetDevice);
		status = WdfUsbTargetDeviceCyclePortSynchronously(pDeviceContext->WdfUsbTargetDevice);
    }

    StartAllPipes(pDeviceContext);
    
    DbgPrint("ResetDevice - ends\n");

    return status;
}

