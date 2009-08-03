/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    UMSS.h

Abstract:

	Kernel mode definitions and function prototypes

Environment:

    Kernel mode

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.

Revision History:

    01/13/99: MRB  Adapted from BULKUSB DDK sample.

--*/

#ifndef UMSS_INCD
#define UMSS_INCD

#include "UMSSdbg.h"
#include "..\iopacket.h"

//
// Structures and definitions from USB Mass Storage Specifications(s)
//

#define CLASS_MASS_STORAGE 0x08

// Transfer protocol definitions
#define PROTOCOL_CBI       0x00
#define PROTOCOL_CB        0x01
#define PROTOCOL_BULKONLY  0x50

#define PROTOCOL_UNDEFINED 0xFF  // Not in spec

#define ACCEPT_DEVICE_SPECIFIC_COMMAND 0

#define BULK_ONLY_MASS_STORAGE_RESET 0xFF
#define BULK_ONLY_GET_MAX_LUN 0xFE

#define CBW_SIGNATURE 0x43425355l
#define CSW_SIGNATURE 0x53425355l

#define CSW_STATUS_PASSED         0x00
#define CSW_STATUS_FAILED         0x01
#define CSW_STATUS_PHASE_ERROR    0x02


#pragma pack (push, 1)

typedef struct _COMMAND_BLOCK_WRAPPER
{
        ULONG dCBWSignature;
        ULONG dCBWTag;
        ULONG dCBWDataTransferLength;
        UCHAR bmCBWFlags;
        UCHAR bCBWLun;
        UCHAR bCBWLength;
        UCHAR CBWCB[16];
} COMMAND_BLOCK_WRAPPER, *PCOMMAND_BLOCK_WRAPPER;

typedef struct _COMMAND_STATUS_WRAPPER
{
        ULONG dCSWSignature;
        ULONG dCSWTag;
        ULONG dCSWDataResidue;
        UCHAR bCSWStatus;
} COMMAND_STATUS_WRAPPER, *PCOMMAND_STATUS_WRAPPER;


typedef struct _INTERRUPT_DATA_BLOCK
{
        UCHAR bType;
        UCHAR bValue;
} INTERRUPT_DATA_BLOCK, *PINTERRUPT_DATA_BLOCK;

#pragma pack (pop)

// 
// Driver specific definitions and structures
//

// We use a max transfer size of 64K
#define UMSS_MAX_TRANSFER_SIZE       0x10000

// Flags to indicate if a device object is a FDO or a child PDO
#define DO_FDO  1
#define DO_PDO  2

// Direction of data transfer
#define DATA_OUT 0
#define DATA_IN  1

#define max(a, b)  (((a) > (b)) ? (a) : (b))

// String definitions for our child PDO.  These should be changed
// to be unique for your device to avoid name conflicts with
// other drivers based on this code.
#define CHILD_PDO_NAME  L"\\Device\\STUMS0"
#define CHILD_DEVICE_ID L"STUMS\\DISK\0"

// IOS SGD structure from BLOCKDEV.H
typedef struct _BlockDev_Scatter_Gather
{
   ULONG BD_SG_Count;
   ULONG BD_SG_Buffer_Ptr;
}  BlockDev_Scatter_Gather;

extern PDRIVER_OBJECT UMSSDriverObject;

// I/O completion handler registered by IOS port driver
typedef void (_stdcall *COMPLETION_HANDLER)(PIOPACKET);



typedef void (*UMSS_WORKER_ROUTINE)(PVOID);

typedef struct _UMSS_WORKER_PACKET
{
    COMPLETION_HANDLER Routine;
    PVOID Context;
    PWORK_QUEUE_ITEM WorkItem;
} UMSS_WORKER_PACKET, *PUMSS_WORKER_PACKET;
    
    




//
// A structure representing the instance information associated with
// this particular device.
//
typedef struct _DEVICE_EXTENSION 
{
    PDEVICE_OBJECT TopOfStackDeviceObject;      // Device object we call when submitting Urbs
    
    PDEVICE_OBJECT PhysicalDeviceObject;            // The bus driver object
    
    DEVICE_POWER_STATE CurrentDevicePowerState; 
    
    USBD_CONFIGURATION_HANDLE UsbConfigurationHandle; 
	
    PUSB_CONFIGURATION_DESCRIPTOR UsbConfigurationDescriptor; // Ptr to our device's config desc.
    
    PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor;             // Ptr to our device's device descriptor
    
    PUSBD_INTERFACE_INFORMATION UsbInterface;       // A copy of our selected interface
    
    DEVICE_CAPABILITIES DeviceCapabilities;         // Device capabilities returned by the bus driver
    
    PIRP PowerIrp;                  // The currently-being-handled system-requested power irp request
    
    KEVENT RemoveEvent;             // Triggered when pending I/O count goes to 0.
    
    KEVENT NoPendingIoEvent;
    
    KEVENT SelfRequestedPowerIrpEvent;      // signals driver-generated power request is finished
	
    KSPIN_LOCK      IoCountSpinLock;        // spinlock used to protect inc/dec iocount logic
	
    ULONG PendingIoCount;                           // Counter for pending I/O operations

    ULONG OpenPipeCount;                            // count of open pipes

    BOOLEAN DeviceRemoved;                          //flag set when processing IRP_MN_REMOVE_DEVICE
    
    BOOLEAN RemoveDeviceRequested;          // set when driver returns success to IRP_MN_QUERY_REMOVE_DEVICE
    
    BOOLEAN StopDeviceRequested;        // set when driver returns success to IRP_MN_QUERY_STOP_DEVICE
    
    BOOLEAN DeviceStarted;                          // flag set when device has been successfully started

    // flag set when IRP_MN_WAIT_WAKE is received and we're in a power state
    // where we can signal a wait
    BOOLEAN EnabledForWakeup;

     // used to flag that we're currently handling a self-generated power request
    BOOLEAN SelfPowerIrp;

    // default power state to power down to on self-suspend 
    ULONG PowerDownLevel; 

    // default maximum transfer per irp size         
    ULONG MaximumTransferSize;

    // Device object type (either FDO or child PDO)
    ULONG DeviceObjectType;

    // In data bulk pipe
    ULONG DataInPipe;

    // Out data bulk pipe
    ULONG DataOutPipe;
	
    // Status interrupt pipe
    ULONG StatusPipe;
	    
    // Child PDO created to load the IOS port driver
    PDEVICE_OBJECT ChildPdo;

    // Completion handler registered by IOS port driver
    COMPLETION_HANDLER CompleteRequest;

    PDEVICE_OBJECT Fdo;
    PIOPACKET IoPacket;
    PIRP Irp;
    PURB Urb;
    ULONG BytesToTransfer;
    ULONG CurrentSGD;

#ifdef SUPPORT_CBI
    PRKDPC CbiTransferDataDpc;
#endif

    INTERRUPT_DATA_BLOCK Idb;

    UCHAR DeviceProtocol;

    CHAR MaxLun;

    COMMAND_BLOCK_WRAPPER CBW;
    COMMAND_STATUS_WRAPPER CSW;

    BOOLEAN Retry;

	//
	// STUMS additions
	//
	UCHAR DeviceType;

	BOOLEAN InternalRequest;

	KEVENT InternalRequestCompleteEvent;	// Triggered when an internal request completes

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


// function prototypes

//----------------- UMSS.C ----------------------
NTSTATUS
UMSS_ProcessSysControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
UMSS_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
UMSS_CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    );

NTSTATUS
UMSS_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
UMSS_ConfigureDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UMSS_SelectInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    );

NTSTATUS
UMSS_ResetPipe(
    IN PDEVICE_OBJECT DeviceObject,
    USBD_PIPE_HANDLE PipeHandle
    );

VOID
UMSS_IncrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    );

LONG
UMSS_DecrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    );   

NTSTATUS
UMSS_DispatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
UMSS_GetDeviceProtocolFromRegistry(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
UMSS_CheckForDeviceType(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
UMSS_GetDeviceProtocolFromDescriptor(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UMSS_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG PortStatus
    );

NTSTATUS
UMSS_ResetParentPort(
    IN IN PDEVICE_OBJECT DeviceObject
    );


//--------------- UMSSPNP.C --------------------
NTSTATUS
UMSS_ProcessPnPIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
UMSS_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UMSS_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UMSS_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UMSS_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
UMSS_IrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UMSS_AbortPipes(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UMSS_FdoDeviceQuery(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UMSS_PdoDeviceQuery(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UMSS_PdoQueryID(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UMSS_PdoProcessPnPIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
UMSS_QueryBusInfo(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

//--------------- UMSSPWR.C --------------------
NTSTATUS
UMSS_PoRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );

NTSTATUS
UMSS_PoSelfRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );

NTSTATUS
UMSS_SelfRequestPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN POWER_STATE PowerState
    );

BOOLEAN
UMSS_SetDevicePowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_POWER_STATE DeviceState
    );

NTSTATUS
UMSS_PowerIrp_Complete(
    IN PDEVICE_OBJECT NullDeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UMSS_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    );

NTSTATUS
UMSS_ProcessPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );    

NTSTATUS
UMSS_SelfSuspendOrActivate(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN fSuspend
    );

BOOLEAN
UMSS_CanAcceptIoRequests(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UMSS_PdoProcessPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
UMSS_PdoSetPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//--------------- IOS.C --------------------
VOID
UMSS_CompleteRequest(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG Status
    );

PVOID
UMSS_GetBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG* BufferSize
    );

VOID
UMSS_ClassSpecificRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR Request,
    IN UCHAR TransferDirection,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
    );

VOID
UMSS_BulkTransfer(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR TransferDirection,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
    );

BOOLEAN
UMSS_ScheduleWorkItem(
    PVOID Context,
    UMSS_WORKER_ROUTINE Routine
    );

VOID 
UMSS_Worker(
    IN PVOID Reference
    );


#ifdef SUPPORT_CBI
//--------------- CBI.C --------------------
VOID
UMSS_CbiStartIo(
    IN PDEVICE_EXTENSION DeviceExtension
    );
    
NTSTATUS
UMSS_CbiSendADSCComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    );

VOID
UMSS_CbiResetPipe(
    IN PVOID Reference
    );

VOID 
UMSS_CbiTransferDataDPC(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID 
UMSS_CbiTransferData(
    PDEVICE_EXTENSION DeviceExtension
    );
              
NTSTATUS
UMSS_CbiTransferDataComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    );

VOID 
UMSS_CbiGetStatus(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
UMSS_CbiGetStatusComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    );
#endif

//--------------- BULKONLY.C --------------------
VOID
UMSS_BulkOnlyStartIo(
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
UMSS_BulkOnlySendCBWComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    );

VOID
UMSS_BulkOnlyResetRecovery(
    IN PVOID Reference
    );

VOID 
UMSS_BulkOnlyTransferData(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
UMSS_BulkOnlyTransferDataComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    );

VOID
UMSS_BulkOnlyResetPipeAndGetStatus(
    IN PVOID Reference
    );

VOID 
UMSS_BulkOnlyGetStatus(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
UMSS_BulkOnlyGetStatusComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Reference
    );

CHAR
UMSS_BulkOnlyGetMaxLun(
    IN PDEVICE_EXTENSION DeviceExtension
    );


#endif // already included






