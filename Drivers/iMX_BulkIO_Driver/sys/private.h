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
//
// Module Name:
//       private.h
//
// Abstract:
//       Contains structure definitions and function prototypes private to
//       the driver.
//
// Environment:
//       Kernel mode
//
//---------------------------------------------------------------------------



#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <initguid.h>
#include <ntddk.h>
#include <ntintsafe.h>
#include "usbdi.h"
#include "usbdlib.h"

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)

#include <wdf.h>
#include <wdfusb.h>
#include "public.h"


#ifndef _H
#define _H

#define POOL_TAG (ULONG) 'SBSU'

#undef ExAllocatePool
#define ExAllocatePool(type, size) \
    ExAllocatePoolWithTag(type, size, POOL_TAG);

#if DBG

#define iMX_DbgPrint(level, _x_) \
            if((level) <= DebugLevel) { \
                DbgPrint("iMX USB: "); DbgPrint _x_; \
            }

#else

#define iMX_DbgPrint(level, _x_)

#endif


#define MAX_TRANSFER_SIZE   512
#define REMOTE_WAKEUP_MASK 0x20

#define DEFAULT_REGISTRY_TRANSFER_SIZE 65536

#define GetListHeadEntry(ListHead)  ((ListHead)->Flink)

#define IDLE_CAPS_TYPE IdleUsbSelectiveSuspend


//
// A structure representing the instance information associated with
// this particular device.
//

typedef struct _DEVICE_CONTEXT {

    USB_DEVICE_DESCRIPTOR           UsbDeviceDescriptor;

    PUSB_CONFIGURATION_DESCRIPTOR   UsbConfigurationDescriptor;

    WDFUSBDEVICE                    WdfUsbTargetDevice;

    ULONG                           WaitWakeEnable;

    BOOLEAN                         IsDeviceHighSpeed;

    WDFUSBINTERFACE                 UsbInterface;

    UCHAR                           NumberConfiguredPipes;

    ULONG                           MaximumTransferSize;
    

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT,
                                                          GetDeviceContext)
//
// This context is associated with every open handle.
//
typedef struct _FILE_CONTEXT {

    WDFUSBPIPE Pipe;

} FILE_CONTEXT, *PFILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILE_CONTEXT,
                                                        GetFileContext)

//
// This context is associated with every request recevied by the driver
// from the app.
//
typedef struct _REQUEST_CONTEXT {

    WDFMEMORY         UrbMemory;
    PMDL              Mdl;
    ULONG             Length;         // remaining to xfer
    ULONG             Numxfer;        // cumulate xfer
    ULONG_PTR         VirtualAddress; // va for next segment of xfer.
    BOOLEAN           Read;           // TRUE if Read
} REQUEST_CONTEXT, * PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT            ,
                                                GetRequestContext)

typedef struct _WORKITEM_CONTEXT {
    WDFDEVICE       Device;
    WDFUSBPIPE      Pipe;
} WORKITEM_CONTEXT, *PWORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORKITEM_CONTEXT,
                                                GetWorkItemContext)

extern ULONG DebugLevel;

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD IMX_DeviceAdd;

EVT_WDF_DEVICE_PREPARE_HARDWARE IMX_DevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE IMX_DeviceReleaseHardware;
EVT_WDF_DEVICE_FILE_CREATE IMX_DeviceFileCreate;

EVT_WDF_IO_QUEUE_IO_READ IMX_DeviceIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE IMX_DeviceIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL IMX_DeviceIoControl;
EVT_WDF_REQUEST_COMPLETION_ROUTINE ReadWriteCompletion;

WDFUSBPIPE
GetPipeFromName(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUNICODE_STRING FileName
    );

VOID
ReadWriteBulkEndPoints(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN ULONG Length,
    IN WDF_REQUEST_TYPE RequestType
    );

NTSTATUS
ResetPipe(
    IN WDFUSBPIPE             Pipe
    );

NTSTATUS
ResetDevice(
    IN WDFDEVICE Device
    );


NTSTATUS
ReadAndSelectDescriptors(
    IN WDFDEVICE Device
    );

NTSTATUS
ConfigureDevice(
    IN WDFDEVICE Device
    );

NTSTATUS
SelectInterfaces(
    IN WDFDEVICE Device
    );

NTSTATUS
IMXSetPowerPolicy(
        __in WDFDEVICE Device
    );

NTSTATUS
AbortPipes(
    IN WDFDEVICE Device
    );


NTSTATUS
QueuePassiveLevelCallback(
    IN WDFDEVICE    Device,
    IN WDFUSBPIPE   Pipe
    );

VOID
DbgPrintRWContext(
    PREQUEST_CONTEXT                 rwContext
    );

NTSTATUS
IMX_ReadFdoRegistryKeyValue(
    __in  WDFDRIVER   Driver,
    __in LPWSTR      Name,
    __out PULONG      Value
    );

#endif

