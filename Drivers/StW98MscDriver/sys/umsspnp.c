/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    Usblspnp.c 

Abstract:

    USB Mass Storage Device Sample Driver
    Plug and Play module

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

    1/13/99: MRB	Adapted from the BULKUSB DDK sample.

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
#include "umssguid.h"


NTSTATUS
UMSS_ProcessPnPIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
Routine Description:

    Dispatch table routine for IRP_MJ_PNP.
    Process the Plug and Play IRPs sent to this device.

Arguments:

    DeviceObject - pointer to our FDO (Functional Device Object)
    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/

{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS waitStatus;
    PDEVICE_OBJECT stackDeviceObject;
    KEVENT startDeviceEvent;


    ENTER(UMSS_ProcessPnPIrp);
    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get a pointer to the device extension
    //

    deviceExtension = DeviceObject->DeviceExtension;
    stackDeviceObject = deviceExtension->TopOfStackDeviceObject;

    UMSS_KdPrint( DBGLVL_MEDIUM, ( "IRP_MJ_PNP, minor %s\n",
    UMSS_StringForPnpMnFunc( irpStack->MinorFunction ) ));

    // inc the FDO device extension's pending IO count for this Irp
    UMSS_IncrementIoCount(DeviceObject);

    UMSS_ASSERT( IRP_MJ_PNP == irpStack->MajorFunction );

    switch (irpStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:

            // The PnP Manager sends this IRP after it has assigned resources, 
            // if any, to the device. The device may have been recently enumerated
            // and is being started for the first time, or the device may be 
            // restarting after being stopped for resource reconfiguration. 

            // Initialize an event we can wait on for the PDO to be done with this irp
            KeInitializeEvent(&startDeviceEvent, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);

            // Set a completion routine so it can signal our event when
            // the PDO is done with the Irp
            IoSetCompletionRoutine(
                Irp,
                UMSS_IrpCompletionRoutine,
                &startDeviceEvent,  // pass the event to the completion routine as the Context
                TRUE,    // invoke on success
                TRUE,    // invoke on error
                TRUE     // invoke on cancellation
                );

            // let the PDO process the IRP
            ntStatus = IoCallDriver(stackDeviceObject, Irp);

            // if PDO is not done yet, wait for the event to be set in our completion routine
            if (ntStatus == STATUS_PENDING)
            {
                // wait for irp to complete
                waitStatus = KeWaitForSingleObject(
                    &startDeviceEvent,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL
                    );
            }

            // Now we're ready to do our own startup processing.
            // USB client drivers such as us set up URBs (USB Request Packets) to send requests
            // to the host controller driver (HCD). The URB structure defines a format for all
            // possible commands that can be sent to a USB device.
            // Here, we request the device descriptor and store it,
            // and configure the device.
            ntStatus = UMSS_StartDevice(DeviceObject);

            // Give the device a second to init, spinup, etc.
            KeStallExecutionProcessor(1000);
		
            Irp->IoStatus.Status = ntStatus;

            IoCompleteRequest (Irp, IO_NO_INCREMENT);

            UMSS_DecrementIoCount(DeviceObject);

            RETURN(ntStatus, UMSS_ProcessPnPIrp);  // end, case IRP_MN_START_DEVICE


        case IRP_MN_QUERY_DEVICE_RELATIONS:
            // Enumerate our child PDO
            ntStatus = UMSS_FdoDeviceQuery(DeviceObject, Irp);

            UMSS_DecrementIoCount(DeviceObject);

            RETURN(ntStatus, UMSS_ProcessPnPIrp);


        case IRP_MN_QUERY_STOP_DEVICE:

            // The IRP_MN_QUERY_STOP_DEVICE/IRP_MN_STOP_DEVICE sequence only occurs
            // during "polite" shutdowns, such as the user explicitily requesting the
            // service be stopped in, or requesting unplug from the Pnp tray icon.
            // This sequence is NOT received during "impolite" shutdowns,
            // such as someone suddenly yanking the USB cord or otherwise 
            // unexpectedly disabling/resetting the device.

            // If a driver sets STATUS_SUCCESS for this IRP,
            // the driver must not start any operations on the device that
            // would prevent that driver from successfully completing an IRP_MN_STOP_DEVICE
            // for the device.
            // For mass storage devices such as disk drives, while the device is in the
            // stop-pending state,the driver holds IRPs that require access to the device,
            // but for most USB devices, there is no 'persistent storage', so we will just
            // refuse any more IO until restarted or the stop is cancelled

            // If a driver in the device stack determines that the device cannot be
            // stopped for resource reconfiguration, the driver is not required to pass
            // the IRP down the device stack. If a query-stop IRP fails,
            // the PnP Manager sends an IRP_MN_CANCEL_STOP_DEVICE to the device stack,
            // notifying the drivers for the device that the query has been cancelled
            // and that the device will not be stopped.
  

            // It is possible to receive this irp when the device has not been started
            //  ( as on a boot device )

            UMSS_KdPrint( DBGLVL_MINIMUM,("IRP_MN_QUERY_STOP_DEVICE\n"));


            if (!deviceExtension->DeviceStarted)
            {
                // if get when never started, just pass on
                UMSS_KdPrint( DBGLVL_MEDIUM,("UMSS_ProcessPnPIrp() IRP_MN_QUERY_STOP_DEVICE when device not started\n"));
                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (deviceExtension->TopOfStackDeviceObject, Irp);
                UMSS_DecrementIoCount(DeviceObject);

                RETURN(ntStatus, UMSS_ProcessPnPIrp);
            }


            // We'll not veto it; pass it on and flag that stop was requested.
            // Once StopDeviceRequested is set no new IOCTL or read/write irps will be passed
            // down the stack to lower drivers; all will be quickly failed
            deviceExtension->StopDeviceRequested = TRUE;

            break; // end, case IRP_MN_QUERY_STOP_DEVICE



        case IRP_MN_CANCEL_STOP_DEVICE:

            // The PnP Manager uses this IRP to inform the drivers for a device
            // that the device will not be stopped for resource reconfiguration.
            // This should only be received after a successful IRP_MN_QUERY_STOP_DEVICE.


            // It is possible to receive this irp when the device has not been started

            if (!deviceExtension->DeviceStarted)
            {
                // if get when never started, just pass on
                UMSS_KdPrint( DBGLVL_MEDIUM,("UMSS_ProcessPnPIrp() IRP_MN_CANCEL_STOP_DEVICE when device not started\n"));
                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (deviceExtension->TopOfStackDeviceObject, Irp);
                UMSS_DecrementIoCount(DeviceObject);
                return ntStatus;
            }

            // Reset this flag so new IOCTL and IO Irp processing will be re-enabled
            deviceExtension->StopDeviceRequested = FALSE;
            break; // end, case IRP_MN_CANCEL_STOP_DEVICE


        case IRP_MN_STOP_DEVICE:

            // The PnP Manager sends this IRP to stop a device so it can reconfigure
            // its hardware resources. The PnP Manager only sends this IRP if a prior
            // IRP_MN_QUERY_STOP_DEVICE completed successfully.

            UMSS_KdPrint( DBGLVL_MINIMUM,("IRP_MN_STOP_DEVICE\n"));

            //
            // Send the select configuration urb with a NULL pointer for the configuration
            // handle, this closes the configuration and puts the device in the 'unconfigured'
            // state.
            //
            ntStatus = UMSS_StopDevice(DeviceObject);

            break; // end, case IRP_MN_STOP_DEVICE



        case IRP_MN_QUERY_REMOVE_DEVICE:

            //  In response to this IRP, drivers indicate whether the device can be
            //  removed without disrupting the system.
            //  If a driver determines it is safe to remove the device,
            //  the driver completes any outstanding I/O requests, arranges to hold any subsequent
            //  read/write requests, and sets Irp->IoStatus.Status to STATUS_SUCCESS. Function
            //  and filter drivers then pass the IRP to the next-lower driver in the device stack.
            //  The underlying bus driver calls IoCompleteRequest.
        
            //  If a driver sets STATUS_SUCCESS for this IRP, the driver must not start any
            //  operations on the device that would prevent that driver from successfully completing
            //  an IRP_MN_REMOVE_DEVICE for the device. If a driver in the device stack determines
            //  that the device cannot be removed, the driver is not required to pass the
            //  query-remove IRP down the device stack. If a query-remove IRP fails, the PnP Manager
            //  sends an IRP_MN_CANCEL_REMOVE_DEVICE to the device stack, notifying the drivers for
            //  the device that the query has been cancelled and that the device will not be removed.
    
            // It is possible to receive this irp when the device has not been started
 
            UMSS_KdPrint( DBGLVL_MINIMUM,("IRP_MN_QUERY_REMOVE_DEVICE\n"));

            if (!deviceExtension->DeviceStarted)
            {
                // if get when never started, just pass on
                UMSS_KdPrint( DBGLVL_MEDIUM,("UMSS_ProcessPnPIrp() IRP_MN_QUERY_STOP_DEVICE when device not started\n"));
                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (deviceExtension->TopOfStackDeviceObject, Irp);
                UMSS_DecrementIoCount(DeviceObject);
         
                RETURN(ntStatus, UMSS_ProcessPnPIrp);
            }

            // Once RemoveDeviceRequested is set no new IOCTL or read/write irps will be passed
            // down the stack to lower drivers; all will be quickly failed
            deviceExtension->RemoveDeviceRequested = TRUE;

            // Wait for any io request pending in our driver to
            // complete before returning success.
            // This  event is set when deviceExtension->PendingIoCount goes to 1
            waitStatus = KeWaitForSingleObject(
                &deviceExtension->NoPendingIoEvent,
                Suspended,
                KernelMode,
                FALSE,
                NULL
                );

            break; // end, case IRP_MN_QUERY_REMOVE_DEVICE



        case IRP_MN_CANCEL_REMOVE_DEVICE:

            // The PnP Manager uses this IRP to inform the drivers
            // for a device that the device will not be removed.
            // It is sent only after a successful IRP_MN_QUERY_REMOVE_DEVICE.
    
            if (!deviceExtension->DeviceStarted)
            {
                // if get when never started, just pass on
                UMSS_KdPrint( DBGLVL_MEDIUM,("UMSS_ProcessPnPIrp() IRP_MN_CANCEL_REMOVE_DEVICE when device not started\n"));
                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (deviceExtension->TopOfStackDeviceObject, Irp);
                UMSS_DecrementIoCount(DeviceObject);

                RETURN(ntStatus, UMSS_ProcessPnPIrp);
            }

            // Reset this flag so new IOCTL and IO Irp processing will be re-enabled
            deviceExtension->RemoveDeviceRequested = FALSE;
   
            break; // end, case IRP_MN_CANCEL_REMOVE_DEVICE
     


        case IRP_MN_SURPRISE_REMOVAL:

            UMSS_KdPrint( DBGLVL_MEDIUM,("UMSS_ProcessPnPIrp() IRP_MN_SURPRISE_REMOVAL\n"));
          
            // For a surprise-style device removal ( i.e. sudden cord yank ), 
            // the physical device has already been removed so the PnP Manager sends 
            // the remove IRP without a prior query-remove. A device can be in any state
            // when it receives a remove IRP as a result of a surprise-style removal.

            // match the inc at the begining of the dispatch routine
            UMSS_DecrementIoCount(DeviceObject);

            //
            // Once DeviceRemoved is set no new IOCTL or read/write irps will be passed
            // down the stack to lower drivers; all will be quickly failed
            //
            deviceExtension->DeviceRemoved = TRUE;
     
            // If any pipes are still open, call USBD with URB_FUNCTION_ABORT_PIPE
            // This call will also close the pipes; if any user close calls get through,
            // they will be noops
            UMSS_AbortPipes( DeviceObject );


            // We don't explicitly wait for the below driver to complete, but just make
            // the call and go on, finishing cleanup
            IoCopyCurrentIrpStackLocationToNext(Irp);

            ntStatus = IoCallDriver(stackDeviceObject, Irp);

            RETURN(ntStatus, UMSS_ProcessPnPIrp);


        case IRP_MN_REMOVE_DEVICE:

            // The PnP Manager uses this IRP to direct drivers to remove a device. 
            // For a "polite" device removal, the PnP Manager sends an 
            // IRP_MN_QUERY_REMOVE_DEVICE prior to the remove IRP. In this case, 
            // the device is in the remove-pending state when the remove IRP arrives.
            // For a surprise-style device removal ( i.e. sudden cord yank ), 
            // the physical device has already been removed and the PnP Manager may not 
            //  have sent IRP_MN_SURPRISE_REMOVAL. A device can be in any state
            // when it receives a remove IRP as a result of a surprise-style removal.

            UMSS_KdPrint( DBGLVL_MINIMUM,("IRP_MN_REMOVE_DEVICE\n"));

            // match the inc at the begining of the dispatch routine
            UMSS_DecrementIoCount(DeviceObject);

            //
            // Once DeviceRemoved is set no new IOCTL or read/write irps will be passed
            // down the stack to lower drivers; all will be quickly failed
            //
            deviceExtension->DeviceRemoved = TRUE;

            // If any pipes are still open, call USBD with URB_FUNCTION_ABORT_PIPE
            // This call will also close the pipes; if any user close calls get through,
            // they will be noops
            UMSS_AbortPipes( DeviceObject );
    
            // We don't explicitly wait for the below driver to complete, but just make
            // the call and go on, finishing cleanup
            IoCopyCurrentIrpStackLocationToNext(Irp);

            ntStatus = IoCallDriver(stackDeviceObject, Irp);

            //
            // The final decrement to device extension PendingIoCount == 0
            // will set deviceExtension->RemoveEvent, enabling device removal.
            // If there is no pending IO at this point, the below decrement will be it.
            //
            UMSS_DecrementIoCount(DeviceObject);
      

            // wait for any io request pending in our driver to
            // complete for finishing the remove

            KeWaitForSingleObject(
                &deviceExtension->RemoveEvent,
                Suspended,
                KernelMode,
                FALSE,
                NULL
                );

            //
            // Delete the link and FDO we created
            //
            UMSS_RemoveDevice(DeviceObject);
    

            UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_ProcessPnPIrp() Detaching from %08X\n",
                              deviceExtension->TopOfStackDeviceObject));

            IoDetachDevice(deviceExtension->TopOfStackDeviceObject);

            UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_ProcessPnPIrp() Deleting %08X\n",
                              DeviceObject));

            IoDeleteDevice (DeviceObject);

            RETURN(ntStatus, UMSS_ProcessPnPIrp);


        default:
            UMSS_KdPrint( DBGLVL_MAXIMUM,("UMSS_ProcessPnPIrp() Minor PnP IOCTL not handled\n"));

    } /* case MinorFunction  */


    if (!NT_SUCCESS(ntStatus))
    {
        // if anything went wrong, return failure  without passing Irp down
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp,
            IO_NO_INCREMENT
            );

        UMSS_DecrementIoCount(DeviceObject);

        RETURN(ntStatus, UMSS_ProcessPnPIrp);
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // All PNP_POWER messages get passed to the TopOfStackDeviceObject
    // we were given in PnPAddDevice
    //

    UMSS_KdPrint( DBGLVL_MAXIMUM,("UMSS_ProcessPnPIrp() Passing PnP Irp down, status = %x\n", ntStatus));

    ntStatus = IoCallDriver(stackDeviceObject, Irp);

    UMSS_DecrementIoCount(DeviceObject);

    RETURN(ntStatus, UMSS_ProcessPnPIrp);
}


NTSTATUS
UMSS_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:

    This routine is called to create and initialize our Functional Device Object (FDO).
    For monolithic drivers, this is done in DriverEntry(), but Plug and Play devices
    wait for a PnP event

Arguments:

    DriverObject - pointer to the driver object for this instance of BulkUsb
    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/

{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PDEVICE_EXTENSION       deviceExtension;
    USBD_VERSION_INFORMATION versionInformation;
    ULONG i;


    ENTER(UMSS_PnPAddDevice);

    //
    // create our funtional device object (FDO)
    //

    ntStatus =
        UMSS_CreateDeviceObject(DriverObject, PhysicalDeviceObject, &deviceObject);

    if (NT_SUCCESS(ntStatus))
    {
        deviceExtension = deviceObject->DeviceExtension;

        deviceExtension->Fdo = deviceObject;

        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // we do not support direct io for read/write
        //
        // deviceObject->Flags |= DO_DIRECT_IO;

        deviceObject->Flags |= DO_POWER_PAGABLE;

        // initialize our device extension
        //
        // remember the Physical device Object
        //
        deviceExtension->PhysicalDeviceObject=PhysicalDeviceObject;


        deviceExtension->DeviceRemoved = FALSE;
        //
        // Attach to the PDO
        //
        deviceExtension->TopOfStackDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);


#ifdef SUPPORT_CBI
        // Initialize the DPC we use to schedule data transfers to/from the device
        deviceExtension->CbiTransferDataDpc = (PRKDPC)UMSS_ExAllocatePool(
                                                       NonPagedPool,
                                                       sizeof(KDPC)
                                                       );

        KeInitializeDpc(
            deviceExtension->CbiTransferDataDpc,
            UMSS_CbiTransferDataDPC,
            NULL
            );
#endif

        // Get a copy of the physical device's capabilities into a
        // DEVICE_CAPABILITIES struct in our device extension;
        // We are most interested in learning which system power states
        // are to be mapped to which device power states for handling
        // IRP_MJ_SET_POWER Irps.
        UMSS_QueryCapabilities(PhysicalDeviceObject,
            &deviceExtension->DeviceCapabilities
            );


        // We want to determine what level to auto-powerdown to; This is the lowest
        //  sleeping level that is LESS than D3; 
        // If all are set to D3, auto powerdown/powerup will be disabled.

        deviceExtension->PowerDownLevel = PowerDeviceUnspecified; // init to disabled

        for (i=PowerSystemSleeping1; i<= PowerSystemSleeping3; i++)
        {
            if ( deviceExtension->DeviceCapabilities.DeviceState[i] < PowerDeviceD3 )
                deviceExtension->PowerDownLevel = deviceExtension->DeviceCapabilities.DeviceState[i];
        }

#if DBG

        // May want override auto power-down level from registry;
        // ( CurrentControlSet\Services\BulkUsb\Parameters )
        // Setting to 0 or 1 in registry disables auto power-down
        UMSS_GetRegistryDword( UMSS_REGISTRY_PARAMETERS_PATH,
            L"PowerDownLevel",
            &(deviceExtension->PowerDownLevel)
            );



        //
        // display the device  caps
        //
        UMSS_KdPrint( DBGLVL_MEDIUM,(" >>>>>> DeviceCaps\n"));
        UMSS_KdPrint( DBGLVL_MEDIUM,(" SystemWake = %s\n",
        UMSS_StringForSysState( deviceExtension->DeviceCapabilities.SystemWake ) ));
        UMSS_KdPrint( DBGLVL_MEDIUM,(" DeviceWake = %s\n",
        UMSS_StringForDevState( deviceExtension->DeviceCapabilities.DeviceWake) ));

        for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++)
        {
            UMSS_KdPrint( DBGLVL_MEDIUM,(" Device State Map: sysstate %s = devstate %s\n",
                UMSS_StringForSysState( i ),
                UMSS_StringForDevState( deviceExtension->DeviceCapabilities.DeviceState[i] ) ));
        }
        UMSS_KdPrint( DBGLVL_MEDIUM,(" <<<<<<<<DeviceCaps\n"));
#endif

        // We keep a pending IO count ( extension->PendingIoCount )  in the device extension.
        // The first increment of this count is done on adding the device.
        // Subsequently, the count is incremented for each new IRP received and
        // decremented when each IRP is completed or passed on.

        // Transition to 'one' therefore indicates no IO is pending and signals
        // deviceExtension->NoPendingIoEvent. This is needed for processing
        // IRP_MN_QUERY_REMOVE_DEVICE

        // Transition to 'zero' signals an event ( deviceExtension->RemoveEvent )
        // to enable device removal. This is used in processing for IRP_MN_REMOVE_DEVICE
        //
        UMSS_IncrementIoCount(deviceObject);

    }

    USBD_GetUSBDIVersion(&versionInformation);

    if( NT_SUCCESS( ntStatus ) )  
    {
        NTSTATUS actStat;
        // try to power down device until IO actually requested
        actStat = UMSS_SelfSuspendOrActivate( deviceObject, TRUE );
    }

    RETURN(ntStatus, UMSS_PnPAddDevice);
}



NTSTATUS
UMSS_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:

    Called from UMSS_ProcessPnPIrp(), the dispatch routine for IRP_MJ_PNP.
    Initializes a given instance of the device on the USB.
    USB client drivers such as us set up URBs (USB Request Packets) to send requests
    to the host controller driver (HCD). The URB structure defines a format for all
    possible commands that can be sent to a USB device.
    Here, we request the device descriptor and store it, and configure the device.


Arguments:

    DeviceObject - pointer to the FDO (Functional Device Object)

Return Value:

    NT status code

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    PURB urb;
    ULONG siz;

    ENTER(UMSS_StartDevice);


    deviceExtension = DeviceObject->DeviceExtension;

    urb = UMSS_ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    UMSS_KdPrintCond( DBGLVL_HIGH,!urb, ("UMSS_StartDevice() FAILED UMSS_ExAllocatePool() for URB\n"));

    if (urb)
    {
        siz = sizeof(USB_DEVICE_DESCRIPTOR);

        deviceDescriptor = UMSS_ExAllocatePool(NonPagedPool, siz);

        UMSS_KdPrintCond( DBGLVL_HIGH, !deviceDescriptor, ("UMSS_StartDevice() FAILED UMSS_ExAllocatePool() for deviceDescriptor\n"));

        if (deviceDescriptor)
        {
            UsbBuildGetDescriptorRequest(urb,
                (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                USB_DEVICE_DESCRIPTOR_TYPE,
                0,
                0,
                deviceDescriptor,
                NULL,
                siz,
                NULL
                );


            ntStatus = UMSS_CallUSBD(DeviceObject, urb);

            UMSS_KdPrintCond( DBGLVL_DEFAULT, !NT_SUCCESS(ntStatus), ("UMSS_StartDevice() FAILED UMSS_CallUSBD(DeviceObject, urb)\n"));

            if (NT_SUCCESS(ntStatus))
            {
                UMSS_KdPrint( DBGLVL_MEDIUM,("Device Descriptor = %x, len %x\n",
                    deviceDescriptor,
                    urb->UrbControlDescriptorRequest.TransferBufferLength)
                    );

                UMSS_KdPrint( DBGLVL_MEDIUM,("USB Mass Storage Device Descriptor:\n"));
                UMSS_KdPrint( DBGLVL_MEDIUM,("-----------------------------------\n"));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bLength %d\n", deviceDescriptor->bLength));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bDescriptorType 0x%x\n", deviceDescriptor->bDescriptorType));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bcdUSB 0x%x\n", deviceDescriptor->bcdUSB));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bDeviceClass 0x%x\n", deviceDescriptor->bDeviceClass));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bDeviceSubClass 0x%x\n", deviceDescriptor->bDeviceSubClass));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bDeviceProtocol 0x%x\n", deviceDescriptor->bDeviceProtocol));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bMaxPacketSize0 0x%x\n", deviceDescriptor->bMaxPacketSize0));
                UMSS_KdPrint( DBGLVL_MEDIUM,("idVendor 0x%x\n", deviceDescriptor->idVendor));
                UMSS_KdPrint( DBGLVL_MEDIUM,("idProduct 0x%x\n", deviceDescriptor->idProduct));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bcdDevice 0x%x\n", deviceDescriptor->bcdDevice));
                UMSS_KdPrint( DBGLVL_MEDIUM,("iManufacturer 0x%x\n", deviceDescriptor->iManufacturer));
                UMSS_KdPrint( DBGLVL_MEDIUM,("iProduct 0x%x\n", deviceDescriptor->iProduct));
                UMSS_KdPrint( DBGLVL_MEDIUM,("iSerialNumber 0x%x\n", deviceDescriptor->iSerialNumber));
                UMSS_KdPrint( DBGLVL_MEDIUM,("bNumConfigurations 0x%x\n", deviceDescriptor->bNumConfigurations));
            }
        }
        else
        {
            // if we got here we failed to allocate deviceDescriptor
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(ntStatus))
        {
            deviceExtension->UsbDeviceDescriptor = deviceDescriptor;
        }
        else if (deviceDescriptor)
        {
            UMSS_ExFreePool(deviceDescriptor);
        }

        UMSS_ExFreePool(urb);

    }
    else
    {
        // if we got here we failed to allocate the urb
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = UMSS_ConfigureDevice(DeviceObject);

        UMSS_KdPrintCond( DBGLVL_MEDIUM,!NT_SUCCESS(ntStatus),("UMSS_StartDevice UMSS_ConfigureDevice() FAILURE (%x)\n", ntStatus));
    }

    if (NT_SUCCESS(ntStatus))
    {
        deviceExtension->DeviceStarted = TRUE;
    }

    RETURN(ntStatus, UMSS_StartDevice);
}


NTSTATUS
UMSS_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Called from UMSS_ProcessPnPIrp() to
    clean up our device instance's allocated buffers; free symbolic links

Arguments:

    DeviceObject - pointer to the FDO

Return Value:

    NT status code from free symbolic link operation

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UNICODE_STRING deviceLinkUnicodeString;

    ENTER(UMSS_RemoveDevice);

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Free device descriptor structure
    //

    if (deviceExtension->UsbDeviceDescriptor)
    {
        UMSS_ExFreePool(deviceExtension->UsbDeviceDescriptor);
    }

    //
    // Free up the UsbInterface structure
    //
    if (deviceExtension->UsbInterface)
    {
        UMSS_ExFreePool(deviceExtension->UsbInterface);
    }

    // free up the USB config discriptor
    if (deviceExtension->UsbConfigurationDescriptor)
    {
        UMSS_ExFreePool(deviceExtension->UsbConfigurationDescriptor);
    }

#ifdef SUPPORT_CBI
    // free the data transfer DPC 
    if (deviceExtension->CbiTransferDataDpc)
    {
        UMSS_ExFreePool(deviceExtension->CbiTransferDataDpc);
    }
#endif

    RETURN(ntStatus, UMSS_RemoveDevice);
}




NTSTATUS
UMSS_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Stops a given instance of a USB mass storage device on the USB.
    We basically just tell USB this device is now 'unconfigured'

Arguments:

    DeviceObject - pointer to the device object for this instance of a USB storage device

Return Value:

    NT status code

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    ULONG siz;

    ENTER(UMSS_StopDevice);

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Send the select configuration urb with a NULL pointer for the configuration
    // handle. This closes the configuration and puts the device in the 'unconfigured'
    // state.
    //
    siz = sizeof(struct _URB_SELECT_CONFIGURATION);

    urb = UMSS_ExAllocatePool(NonPagedPool, siz);

    if (urb)
    {
        UsbBuildSelectConfigurationRequest(
            urb,
            (USHORT) siz,
            NULL
            );

        ntStatus = UMSS_CallUSBD(DeviceObject, urb);

        UMSS_KdPrintCond( DBGLVL_DEFAULT,!NT_SUCCESS(ntStatus),("UMSS_StopDevice() FAILURE Configuration Closed status = %x usb status = %x.\n", ntStatus, urb->UrbHeader.Status));

        UMSS_ExFreePool(urb);
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }


    if (NT_SUCCESS(ntStatus))
    {
        deviceExtension->DeviceStarted = FALSE;
    }

    deviceExtension->StopDeviceRequested = FALSE;

    RETURN(ntStatus, UMSS_StopDevice);
}



NTSTATUS
UMSS_IrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Used as a  general purpose completion routine so it can signal an event,
    passed as the Context, when the next lower driver is done with the input Irp.
    This routine is used by both PnP and Power Management logic.

    Even though this routine does nothing but set an event, it must be defined and
    prototyped as a completetion routine for use as such


Arguments:

    DeviceObject - Pointer to the device object for the class device.
    Irp - Irp completed.
    Context - Driver defined context, in this case a pointer to an event.

Return Value:

    The function value is the final status from the operation.

--*/

{
    PKEVENT event = Context;

    ENTER(UMSS_IrpCompletionRoutine);

    // Set the input event
    KeSetEvent(
        event,
        1,       // Priority increment  for waiting thread.
        FALSE);  // Flag this call is not immediately followed by wait.

    // This routine must return STATUS_MORE_PROCESSING_REQUIRED because we have not yet called
    // IoFreeIrp() on this IRP.
    RETURN(STATUS_MORE_PROCESSING_REQUIRED, UMSS_IrpCompletionRoutine);
}


NTSTATUS
UMSS_FdoDeviceQuery(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
/*++
Routine Description:

    Handler for IRP_MN_QUERY_DEVICE_RELATIONS.
    Enumerates our virtual child PDO.

Arguments:

    DeviceObject - pointer to our child PDO
    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/

{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_EXTENSION DeviceExtension;
    PDEVICE_EXTENSION PdoExtension;
    UNICODE_STRING PdoUniName;
    static UCHAR PdoCount=0;

    //NOTE: This name needs to be unique to each hardware vendor
    //      to avoid name collision between different
    //      drivers based on this code
    WCHAR PdoName[] = CHILD_PDO_NAME;

    ENTER(UMSS_FdoDeviceQuery);
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;

    switch (irpStack->Parameters.QueryDeviceRelations.Type)
    {
        case BusRelations:
 
            // Allocate space for 1 child PDO
            // Don't use UMSS_ExAllocatePool, since DeviceRelations structure
            // will be freed by the OS rather than this driver.
            DeviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

            // If we can't allocate DeviceRelations structure, bail
            if (!DeviceRelations)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                DeviceRelations->Count = 0;
			
                if ((!DeviceExtension->ChildPdo) && (PdoCount<9))
                {
                    RtlInitUnicodeString (&PdoUniName, PdoName);
                    PdoName [((sizeof(PdoName)/sizeof(WCHAR)) - 2)] = L'0' + PdoCount++;

                    ntStatus = IoCreateDevice(
                        UMSSDriverObject,
                        sizeof(DEVICE_EXTENSION),
                        &PdoUniName,
                        FILE_DEVICE_MASS_STORAGE,
                        0,
                        FALSE,
                        &DeviceExtension->ChildPdo
                        );

                    if(NT_SUCCESS(ntStatus))
                    {
                        PdoExtension = DeviceExtension->ChildPdo->DeviceExtension;

                        PdoExtension->Fdo = DeviceObject;

                        // Mark the device object as our child PDO
                        PdoExtension->DeviceObjectType = DO_PDO;

                        DeviceExtension->ChildPdo->Flags &= ~DO_DEVICE_INITIALIZING;
                        DeviceExtension->ChildPdo->Flags |= DO_POWER_PAGABLE;
		
                        DeviceRelations->Objects[DeviceRelations->Count++] = DeviceExtension->ChildPdo;
                        ObReferenceObject(DeviceExtension->ChildPdo);
                    }
                }
                else
                {
                    // Child PDO already exists, just return it
                    DeviceRelations->Objects[DeviceRelations->Count++] = DeviceExtension->ChildPdo;
                    ObReferenceObject(DeviceExtension->ChildPdo);
                }
		
                Irp->IoStatus.Information = (ULONG)DeviceRelations;
                ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
            }
		
            break;

	default:
            // We pass on any non-BusRelations IRP
            IoSkipCurrentIrpStackLocation (Irp);
            ntStatus = IoCallDriver (DeviceExtension->TopOfStackDeviceObject, Irp);

    }

    RETURN(ntStatus, UMSS_FdoDeviceQuery);
}
		


NTSTATUS
UMSS_PdoProcessPnPIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
Routine Description:

    Dispatch table routine for IRP_MJ_PNP.
    Process the Plug and Play IRPs sent to our PDO.

Arguments:

    DeviceObject - pointer to our child PDO
    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/

{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT stackDeviceObject, Fdo;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //
    
    ENTER(UMSS_PdoProcessPnPIrp);

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get a pointer to the device extension
    //

    deviceExtension = DeviceObject->DeviceExtension;
    stackDeviceObject = deviceExtension->TopOfStackDeviceObject;

    UMSS_KdPrint( DBGLVL_MEDIUM, ( "enter UMSS_ProcessPnPIrp() IRP_MJ_PNP, minor %s\n",
    UMSS_StringForPnpMnFunc( irpStack->MinorFunction ) ));

    // inc the FDO device extension's pending IO count for this Irp
    //UMSS_IncrementIoCount(DeviceObject);

    UMSS_ASSERT( IRP_MJ_PNP == irpStack->MajorFunction );

    switch (irpStack->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            ntStatus = UMSS_PdoDeviceQuery(DeviceObject, Irp);
            break;


	case IRP_MN_QUERY_ID:
            ntStatus = UMSS_PdoQueryID(DeviceObject, Irp);
            break;


	case IRP_MN_REMOVE_DEVICE:
//MRBMRB - make sure ChildPdo field is reset to NULL
            Fdo = deviceExtension->Fdo;
            deviceExtension = Fdo->DeviceExtension;
            deviceExtension->ChildPdo = NULL;

            IoDeleteDevice(DeviceObject);
            break;


	case IRP_MN_QUERY_BUS_INFORMATION:
            ntStatus = UMSS_QueryBusInfo(DeviceObject, Irp);
            break;
    }

    RETURN(ntStatus, UMSS_PdoProcessPnPIrp);
}


NTSTATUS
UMSS_QueryBusInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:

    Handler for IRP_MN_QUERY_BUS_INFO.  Returns info about our virtual bus.

Arguments:

    DeviceObject - pointer to our child PDO
    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/

{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpStack;
    PPNP_BUS_INFORMATION BusInfo;
        	
    ENTER(UMSS_QueryBusInfo);

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    BusInfo = ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));

    // If we can't allocate BusInfo structure, bail
    if (!BusInfo)
    {
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        BusInfo->BusTypeGuid = GUID_BUS_UMSS;
        BusInfo->LegacyBusType = PNPBus;
        BusInfo->BusNumber = 0;
			
        Irp->IoStatus.Information = (ULONG)BusInfo;
        ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    RETURN(ntStatus, UMSS_QueryBusInfo);
}										



NTSTATUS
UMSS_PdoDeviceQuery(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:

    Handler for IRP_MN_QUERY_DEVICE_RELATIONS for our virtual child device.
    
Arguments:

    DeviceObject - pointer to our child PDO
    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/

{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_EXTENSION DeviceExtension;
        	
    ENTER(UMSS_PdoDeviceQuery);

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;

    switch (irpStack->Parameters.QueryDeviceRelations.Type)
    {
	case TargetDeviceRelation:

            // Allocate space for 1 child device
            DeviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

            // If we can't allocate DeviceRelations structure, bail
            if (!DeviceRelations)
            {
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                ObReferenceObject(DeviceObject);
			
                DeviceRelations->Count = 1;
                DeviceRelations->Objects[0] = DeviceObject;
			
                Irp->IoStatus.Information = (ULONG)DeviceRelations;
                ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
            }
		
            break;

	default:
            ntStatus = Irp->IoStatus.Status;
    }

    RETURN(ntStatus, UMSS_PdoDeviceQuery);
}										



NTSTATUS
UMSS_PdoQueryID(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
/*++
Routine Description:

    Handler for IRP_MN_QUERY_ID.  Returns hardware, device, and instance
    IDs for our virtual child PDO.

Arguments:

    DeviceObject - pointer to our child PDO
    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/

{
    PIO_STACK_LOCATION ioStack;
    PWCHAR buffer;
    WCHAR IDString[] = CHILD_DEVICE_ID;
    WCHAR InstanceIDString[] = L"0000";
    ULONG length;

    ENTER(UMSS_PdoQueryID);

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    switch (ioStack->Parameters.QueryId.IdType)
    {
        case BusQueryHardwareIDs:
            // return a multi WCHAR (null terminated) string (null terminated)
            // array for use in matching hardare ids in inf files;


        case BusQueryDeviceID:
            // return a WCHAR (null terminated) string describing the device
            // For symplicity we make it exactly the same as the Hardware ID.
       
            length = sizeof (IDString) * sizeof(WCHAR);
            buffer = ExAllocatePool (PagedPool, length);

            if (buffer)
            {
                RtlCopyMemory (buffer, IDString, length);
            }

            Irp->IoStatus.Information = (ULONG) buffer;
            break;


        case BusQueryInstanceID:
            length = sizeof (InstanceIDString) * sizeof(WCHAR);
            buffer = ExAllocatePool (PagedPool, length);

            if (buffer)
            {
                RtlCopyMemory (buffer, InstanceIDString, length);
            }

            Irp->IoStatus.Information = (ULONG) buffer;
            break;
    }

    RETURN(STATUS_SUCCESS, UMSS_PdoQueryID);
}


NTSTATUS
UMSS_AbortPipes(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Called as part of sudden device removal handling.
    Cancels any pending transfers for all open pipes. 
    If any pipes are still open, call USBD with URB_FUNCTION_ABORT_PIPE
    Also marks the pipe 'closed' in our saved  configuration info.

Arguments:

    Ptrs to our FDO

Return Value:

    NT status code

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    PDEVICE_EXTENSION deviceExtension;
    ULONG i;

    PUSBD_INTERFACE_INFORMATION interface;
    PUSBD_PIPE_INFORMATION PipeInfo;

    ENTER(UMSS_AbortPipes);

    deviceExtension = DeviceObject->DeviceExtension;
    interface = deviceExtension->UsbInterface;

    for (i=0; i<interface->NumberOfPipes; i++)
    {
        PipeInfo =  &interface->Pipes[i]; // PUSBD_PIPE_INFORMATION  PipeInfo;

        if ( PipeInfo->PipeFlags )
        {
            UMSS_KdPrint( DBGLVL_HIGH,("UMSS_AbortPipes() Aborting open  Pipe %d\n", i));

            urb = UMSS_ExAllocatePool(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));

            if (urb)
            {
                urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
                urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
                urb->UrbPipeRequest.PipeHandle = PipeInfo->PipeHandle;

                ntStatus = UMSS_CallUSBD(DeviceObject, urb);

                UMSS_ExFreePool(urb);

            }
            else
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                UMSS_KdPrint( DBGLVL_HIGH,("UMSS_AbortPipes() FAILED urb alloc\n" ));
                break;
            }


            if (!(NT_SUCCESS(ntStatus)))
            {
                // if we failed, dump out
#if DBG
                if ( gpDbg )
                    gpDbg->PipeErrorCount++;
#endif
                break;
            }
            else
            {
                PipeInfo->PipeFlags = FALSE; // mark the pipe 'closed'
                deviceExtension->OpenPipeCount--;
#if DBG
                if ( gpDbg )
                    gpDbg->AbortPipeCount++;
#endif
            }

        } // end, if pipe open
    } // end, for all pipes

    RETURN(ntStatus, UMSS_AbortPipes);
}


