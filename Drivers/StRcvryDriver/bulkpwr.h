/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bulkpwr.h

Abstract:

Environment:

    Kernel mode

Notes:

  	Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef _BULKUSB_POWER_H
#define _BULKUSB_POWER_H

typedef struct _POWER_COMPLETION_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PIRP           SIrp;
} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;

typedef struct _WORKER_THREAD_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PIRP           Irp;
    PIO_WORKITEM   WorkItem;
} WORKER_THREAD_CONTEXT, *PWORKER_THREAD_CONTEXT;

IO_WORKITEM_ROUTINE HoldIoRequestsWorkerRoutine;
DRIVER_CANCEL CancelQueued;
IO_COMPLETION_ROUTINE SysPoCompletionRoutine;
IO_COMPLETION_ROUTINE WaitWakeCompletionRoutine;
IO_COMPLETION_ROUTINE FinishDevPoUpIrp;
IO_COMPLETION_ROUTINE FinishDevPoDnIrp;
DRIVER_DISPATCH BulkUsb_DispatchPower;

NTSTATUS
HandleSystemQueryPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
HandleSystemSetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
HandleDeviceQueryPower(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    );

VOID
SendDeviceIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
DevPoCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject, 
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
HandleDeviceSetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );


NTSTATUS
SetDeviceFunctional(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DeviceExtension
    );


NTSTATUS
HoldIoRequests(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
QueueRequest(
    IN OUT PDEVICE_EXTENSION DeviceExtension,
    IN PIRP                  Irp
    );

NTSTATUS
IssueWaitWake(
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
CancelWaitWake(
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
WaitWakeCallback( 
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

PCHAR
PowerMinorFunctionString (
    IN UCHAR MinorFunction
    );

#endif

