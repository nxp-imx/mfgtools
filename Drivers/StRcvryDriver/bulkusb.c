/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bulkusb.c

Abstract:

    Bulk USB device driver for SigmaTel STMP3410/3500/3600 devices.
	Main module

Author:

Environment:

    kernel mode only

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "bulkusb.h"
#include "bulkpnp.h"
#include "bulkpwr.h"
#include "bulkdev.h"
#include "bulkwmi.h"
#include "bulkusr.h"
#include "bulkrwr.h"

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

//
// Globals
//

GLOBALS Globals;
ULONG   DebugLevel = 4;

DRIVER_UNLOAD BulkUsb_DriverUnload;
DRIVER_ADD_DEVICE BulkUsb_AddDevice;
DRIVER_INITIALIZE DriverEntry;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING UniRegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, BulkUsb_DriverUnload)
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING UniRegistryPath
    )
/*++ 

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.    

Arguments:
    
    DriverObject - pointer to driver object 

    RegistryPath - pointer to a unicode string representing the path to driver 
                   specific key in the registry.

Return Values:

    NT status value
    
--*/
{

    NTSTATUS        ntStatus;
    PUNICODE_STRING registryPath;
	ULONG index;
    
    PAGED_CODE();

    //
    // initialization of variables
    //

    registryPath = &Globals.BulkUsb_RegistryPath;

    //
    // Allocate pool to hold a null-terminated copy of the path.
    // Safe in paged pool since all registry routines execute at
    // PASSIVE_LEVEL.
    //

    registryPath->MaximumLength = UniRegistryPath->Length + sizeof(UNICODE_NULL);
    registryPath->Length        = UniRegistryPath->Length;
    registryPath->Buffer        = ExAllocatePool(PagedPool,
                                                 registryPath->MaximumLength);

    if (!registryPath->Buffer) {

        BulkUsb_DbgPrint(1, ("Failed to allocate memory for registryPath\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntry_Exit;
    } 


    RtlZeroMemory (registryPath->Buffer, 
                   registryPath->MaximumLength);
    RtlMoveMemory (registryPath->Buffer, 
                   UniRegistryPath->Buffer, 
                   UniRegistryPath->Length);

    ntStatus = STATUS_SUCCESS;

   	for( index=0; index<MAX_INSTANCES; ++index )
		Globals.Stmp3RecInstances[index] = INSTANCE_FREE;

    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BulkUsb_DispatchDevCtrl;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = BulkUsb_DispatchPower;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = BulkUsb_DispatchPnP;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = BulkUsb_DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = BulkUsb_DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = BulkUsb_DispatchClean;
    DriverObject->MajorFunction[IRP_MJ_READ]           =
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = BulkUsb_DispatchReadWrite;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = BulkUsb_DispatchSysCtrl;
    DriverObject->DriverUnload                         = BulkUsb_DriverUnload;
    DriverObject->DriverExtension->AddDevice           = (PDRIVER_ADD_DEVICE)
                                                         BulkUsb_AddDevice;
DriverEntry_Exit:

	BulkUsb_DbgPrint(1, ("DriverEntry() path:%ws\n", Globals.BulkUsb_RegistryPath.Buffer));
    return ntStatus;
}

VOID
BulkUsb_DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Description:

    This function will free the memory allocations in DriverEntry.

Arguments:

    DriverObject - pointer to driver object 

Return:
	
    None

--*/
{
    PUNICODE_STRING registryPath;
    
    UNREFERENCED_PARAMETER( DriverObject );
	
    PAGED_CODE();

    BulkUsb_DbgPrint(3, ("BulkUsb_DriverUnload - begins\n"));

    registryPath = &Globals.BulkUsb_RegistryPath;

    if(registryPath->Buffer) {

        ExFreePool(registryPath->Buffer);
        registryPath->Buffer = NULL;
    }

    BulkUsb_DbgPrint(3, ("BulkUsb_DriverUnload - ends\n"));

    return;
}

NTSTATUS
BulkUsb_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Description:

Arguments:

    DriverObject - Store the pointer to the object representing us.

    PhysicalDeviceObject - Pointer to the device object created by the
                           undelying bus driver.

Return:
	
    STATUS_SUCCESS - if successful 
    STATUS_UNSUCCESSFUL - otherwise

--*/
{
    NTSTATUS          ntStatus;
    PDEVICE_OBJECT    deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    POWER_STATE       state;
	UNICODE_STRING    tempDeviceName;
	WCHAR			  nameBuffer[128];
	ULONG             index, tempInstance;
	UNICODE_STRING    tempInstanceName;
	WCHAR			  instanceBuffer[128];
	UNICODE_STRING    tempInstanceIndex;
	WCHAR			  instanceIndexBuffer[128];
//    KIRQL             oldIrql;

//	UNREFERENCED_PARAMETER( oldIrql );
	
    BulkUsb_DbgPrint(3, ("BulkUsb_AddDevice - begins\n"));

//// SGTL - start

    // StMp3RecDevice 
    tempDeviceName.MaximumLength = sizeof(nameBuffer)/sizeof(nameBuffer[0]);
    tempDeviceName.Buffer        = nameBuffer;
	tempDeviceName.Length        = 0;
    RtlUnicodeStringCopyString (&tempDeviceName, L"\\Device\\");

	tempInstanceName.MaximumLength = sizeof(instanceBuffer)/sizeof(instanceBuffer[0]);
    tempInstanceName.Buffer        = instanceBuffer;
    tempInstanceName.Length        = 0;
	RtlUnicodeStringCopyString (&tempInstanceName, L"StMp3RecDevice");
	
	tempInstanceIndex.MaximumLength = sizeof(instanceIndexBuffer)/sizeof(instanceIndexBuffer[0]);
    tempInstanceIndex.Buffer        = instanceIndexBuffer;
	tempInstanceIndex.Length        = 0;

    tempInstance = NO_INSTANCE_AVAILABLE;
    for( index=0; index<MAX_INSTANCES; ++index )
    {
        if( Globals.Stmp3RecInstances[index] == INSTANCE_FREE )
        {
            tempInstance = index;
            Globals.Stmp3RecInstances[index] = INSTANCE_USED;
            break;
        }   
    }
    if( tempInstance == NO_INSTANCE_AVAILABLE )
   	{
        BulkUsb_DbgPrint( 1, ("exit BulkUsb_AddDevice() STATUS_INSUFFICIENT_RESOURCES\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
   	}


    RtlIntegerToUnicodeString(tempInstance, 10, &tempInstanceIndex);
    RtlUnicodeStringCat(&tempInstanceName, &tempInstanceIndex);
	
    RtlUnicodeStringCat(&tempDeviceName, &tempInstanceName);

//// SGTL - end

    deviceObject = NULL;
    ntStatus = IoCreateDevice(
                    DriverObject,                   // our driver object
                    sizeof(DEVICE_EXTENSION),       // extension size for us
                    &tempDeviceName/*NULL*/,            // name for this device
                    FILE_DEVICE_UNKNOWN,
                    0/*FILE_AUTOGENERATED_DEVICE_NAME*/, // device characteristics
                    FALSE,                          // Not exclusive
                    &deviceObject);                 // Our device object

    if(!NT_SUCCESS(ntStatus)) {
        //
        // returning failure here prevents the entire stack from functioning,
        // but most likely the rest of the stack will not be able to create
        // device objects either, so it is still OK.
        //                
        BulkUsb_DbgPrint(1, ("Failed to create device object (0x%X)\n", ntStatus));
        return ntStatus;
    }

    //
    // Initialize the device extension
    //

    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;
    deviceExtension->FunctionalDeviceObject = deviceObject;
    deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    deviceObject->Flags |= DO_DIRECT_IO;

    //
    // initialize the device state lock and set the device state
    //

    KeInitializeSpinLock(&deviceExtension->DevStateLock);
    INITIALIZE_PNP_STATE(deviceExtension);

    //
    //initialize OpenHandleCount
    //
    deviceExtension->OpenHandleCount = 0;

    //
    // Initialize the selective suspend variables
    //
    KeInitializeSpinLock(&deviceExtension->IdleReqStateLock);
    deviceExtension->IdleReqPend = 0;
    deviceExtension->PendingIdleIrp = NULL;

    //
    // Hold requests until the device is started
    //

    deviceExtension->QueueState = HoldRequests;

    //
    // Initialize the queue and the queue spin lock
    //

    InitializeListHead(&deviceExtension->NewRequestsQueue);
    KeInitializeSpinLock(&deviceExtension->QueueLock);

    //
    // Initialize the remove event to not-signaled.
    //

    KeInitializeEvent(&deviceExtension->RemoveEvent, 
                      SynchronizationEvent, 
                      FALSE);

    //
    // Initialize the stop event to signaled.
    // This event is signaled when the OutstandingIO becomes 1
    //

    KeInitializeEvent(&deviceExtension->StopEvent, 
                      SynchronizationEvent, 
                      TRUE);

    //
    // OutstandingIo count biased to 1.
    // Transition to 0 during remove device means IO is finished.
    // Transition to 1 means the device can be stopped
    //

    deviceExtension->OutStandingIO = 1;
    KeInitializeSpinLock(&deviceExtension->IOCountLock);

    //
    // Delegating to WMILIB
    //
    ntStatus = BulkUsb_WmiRegistration(deviceExtension);

    if(!NT_SUCCESS(ntStatus)) {

        BulkUsb_DbgPrint(1, ("BulkUsb_WmiRegistration failed with %X\n", ntStatus));
        IoDeleteDevice(deviceObject);
        return ntStatus;
    }

    //
    // set the flags as underlying PDO
    //

    if(PhysicalDeviceObject->Flags & DO_POWER_PAGABLE) {

        deviceObject->Flags |= DO_POWER_PAGABLE;
    }

    //
    // Typically, the function driver for a device is its 
    // power policy owner, although for some devices another 
    // driver or system component may assume this role. 
    // Set the initial power state of the device, if known, by calling 
    // PoSetPowerState.
    // 

    deviceExtension->DevPower = PowerDeviceD0;
    deviceExtension->SysPower = PowerSystemWorking;

    state.DeviceState = PowerDeviceD0;
    PoSetPowerState(deviceObject, DevicePowerState, state);

    //
    // attach our driver to device stack
    // The return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //

    deviceExtension->TopOfStackDeviceObject = 
                IoAttachDeviceToDeviceStack(deviceObject,
                                            PhysicalDeviceObject);

    if(NULL == deviceExtension->TopOfStackDeviceObject) {

        BulkUsb_WmiDeRegistration(deviceExtension);
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }
        
    //
    // Register device interfaces
    //

    ntStatus = IoRegisterDeviceInterface(deviceExtension->PhysicalDeviceObject, 
                                         &GUID_DEVINTERFACE_STMP3XXX_USB_BULK_DEVICE, 
                                         NULL, 
                                         &deviceExtension->InterfaceName);

    if(!NT_SUCCESS(ntStatus)) {

        BulkUsb_WmiDeRegistration(deviceExtension);
        IoDetachDevice(deviceExtension->TopOfStackDeviceObject);
        IoDeleteDevice(deviceObject);
        return ntStatus;
    }
	
	// save DeviceName
	deviceExtension->Instance = tempInstance;

	deviceExtension->DeviceName.MaximumLength = tempDeviceName.MaximumLength;
    deviceExtension->DeviceName.Length = 0;
#pragma prefast(suppress: 6014, "Deallocation handled in HandleSurpriseRemoval() and HandleRemoveDevice()")
    deviceExtension->DeviceName.Buffer = ExAllocatePool(NonPagedPool, tempDeviceName.MaximumLength);

    if (!deviceExtension->DeviceName.Buffer) {

        BulkUsb_DbgPrint(1, ("Failed to allocate memory for deviceName\n"));

		BulkUsb_WmiDeRegistration(deviceExtension);
        IoDetachDevice(deviceExtension->TopOfStackDeviceObject);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    } 

	RtlUnicodeStringCopy(&deviceExtension->DeviceName, &tempDeviceName);
	
	// Create SymbolicLink
    // StMp3RecDevice 
    RtlUnicodeStringCopyString (&tempDeviceName, L"\\DosDevices\\");
	RtlUnicodeStringCat(&tempDeviceName, &tempInstanceName);

    ntStatus = IoCreateSymbolicLink(&tempDeviceName, &deviceExtension->DeviceName);

	if (!NT_SUCCESS(ntStatus)) {
	    BulkUsb_DbgPrint( 1, ("BulkUsb_CreateDeviceObject() FAILED to Create additional symbolic link. (0x%x)\n", ntStatus));
	} else {
	    BulkUsb_DbgPrint( 1, ("BulkUsb_CreateDeviceObject() Created additional symbolic link %ws l:0x%x ml:0x%x\n", tempDeviceName.Buffer, tempDeviceName.Length, tempDeviceName.MaximumLength));
	}
	// Save the SymbolicLinkName
	deviceExtension->SymbolicLinkName.MaximumLength = tempDeviceName.MaximumLength;
    deviceExtension->SymbolicLinkName.Length        = 0;
#pragma prefast(suppress: 6014, "Deallocation handled in HandleSurpriseRemoval() and HandleRemoveDevice()")
    deviceExtension->SymbolicLinkName.Buffer        = ExAllocatePool(NonPagedPool,
                                                 tempDeviceName.MaximumLength);

    if (!deviceExtension->SymbolicLinkName.Buffer) {

        BulkUsb_DbgPrint(1, ("Failed to allocate memory for SymbolicLinkName\n"));

		BulkUsb_WmiDeRegistration(deviceExtension);
        IoDetachDevice(deviceExtension->TopOfStackDeviceObject);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    } 
	RtlUnicodeStringCopy(&deviceExtension->SymbolicLinkName, &tempDeviceName);

	// remember version
	if(RtlIsNtDdiVersionAvailable(NTDDI_LONGHORN)) {

        deviceExtension->WdmVersion = WinVistaOrBetter;
    }
    else if(RtlIsNtDdiVersionAvailable(NTDDI_WINXP)) {

        deviceExtension->WdmVersion = WinXpOrBetter;
    }
    else if(RtlIsNtDdiVersionAvailable(NTDDI_WIN2K)) {

        deviceExtension->WdmVersion = Win2kOrBetter;
    }

    deviceExtension->SSRegistryEnable = 0;
    deviceExtension->SSEnable = 0;

    //
    // WinXP only
    // check the registry flag -
    // whether the device should selectively
    // suspend when idle
    //

    if(WinXpOrBetter == deviceExtension->WdmVersion) {

        BulkUsb_GetRegistryDword(BULKUSB_REGISTRY_PARAMETERS_PATH,
                                 L"BulkUsbEnable",
                                 (PULONG)&deviceExtension->SSRegistryEnable);

        if(deviceExtension->SSRegistryEnable) {

            //
            // initialize DPC
            //
            KeInitializeDpc(&deviceExtension->DeferredProcCall, 
                            DpcRoutine, 
                            deviceObject);

            //
            // initialize the timer.
            // the DPC and the timer in conjunction, 
            // monitor the state of the device to 
            // selectively suspend the device.
            //
            KeInitializeTimerEx(&deviceExtension->Timer,
                                NotificationTimer);

            //
            // Initialize the NoDpcWorkItemPendingEvent to signaled state.
            // This event is cleared when a Dpc is fired and signaled
            // on completion of the work-item.
            //
            KeInitializeEvent(&deviceExtension->NoDpcWorkItemPendingEvent, 
                              NotificationEvent, 
                              TRUE);

            //
            // Initialize the NoIdleReqPendEvent to ensure that the idle request
            // is indeed complete before we unload the drivers.
            //
            KeInitializeEvent(&deviceExtension->NoIdleReqPendEvent,
                              NotificationEvent,
                              TRUE);
        }
    }

    //
    // Clear the DO_DEVICE_INITIALIZING flag.
    // Note: Do not clear this flag until the driver has set the
    // device power state and the power DO flags. 
    //

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    BulkUsb_DbgPrint(3, ("BulkUsb_AddDevice - ends\n"));

    return ntStatus;
}

