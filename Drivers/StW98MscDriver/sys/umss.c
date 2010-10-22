/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    UMSS.c 

Abstract:

    USB device driver for USB storage device
    Main module

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
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#define GLOBAL_VARS

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "umss.h"


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++
Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path
		   to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/

{
#if DBG
    // should be done before any debug output is done.
    // read our debug verbosity level from the registry
    UMSS_GetRegistryDword( UMSS_REGISTRY_PARAMETERS_PATH, //absolute registry path
				     L"DebugLevel",     // REG_DWORD ValueName
                                     &UMSS_DebugLevel );    // Value receiver
#endif

    ENTER(DriverEntry);
    UMSS_KdPrint(DBGLVL_DEFAULT, ("RegistryPath=\n    %ws\n", RegistryPath->Buffer ));

    // Remember our driver object, for when we create our child PDO
    UMSSDriverObject = DriverObject;

    // Create dispatch points for create, close, unload
    DriverObject->MajorFunction[IRP_MJ_CREATE] = UMSS_DispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = UMSS_DispatchIrp;
    DriverObject->DriverUnload = UMSS_Unload;

    // User mode DeviceIoControl() calls will be routed here
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = UMSS_DispatchIrp;

    // User mode ReadFile()/WriteFile() calls will be routed here
    DriverObject->MajorFunction[IRP_MJ_WRITE] = UMSS_DispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_READ] = UMSS_DispatchIrp;

    // routines for handling system PNP and power management requests
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = UMSS_DispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_PNP] = UMSS_DispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = UMSS_DispatchIrp;

    // The Functional Device Object (FDO) will not be created for PNP devices until 
    // this routine is called upon device plug-in.
    DriverObject->DriverExtension->AddDevice = UMSS_PnPAddDevice;

    RETURN(STATUS_SUCCESS, DriverEntry);
}


NTSTATUS
UMSS_DispatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
Routine Description:

    IRP dispatch routine.  
    
Arguments:

    DeviceObject - pointer to our FDO (Functional Device Object)
    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/

{
    PIO_STACK_LOCATION IrpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;

    ENTER(UMSS_DispatchIrp);

    DeviceExtension = DeviceObject->DeviceExtension;
    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    if (DO_FDO == DeviceExtension->DeviceObjectType)
    {
        switch (IrpStack->MajorFunction)
        {
            case IRP_MJ_SYSTEM_CONTROL:
                ntStatus = UMSS_ProcessSysControlIrp(DeviceObject, Irp);
                break;

            case IRP_MJ_PNP:
                ntStatus = UMSS_ProcessPnPIrp(DeviceObject, Irp);
                break;

            case IRP_MJ_POWER:
                ntStatus = UMSS_ProcessPowerIrp(DeviceObject, Irp);
                break;
        }
    }
    else if (DO_PDO == DeviceExtension->DeviceObjectType)
    {
        switch (IrpStack->MajorFunction)
        {
            case IRP_MJ_PNP:
                ntStatus = UMSS_PdoProcessPnPIrp(DeviceObject, Irp);
                break;

            case IRP_MJ_POWER:
                ntStatus = UMSS_PdoProcessPowerIrp(DeviceObject, Irp);
                break;
        }
			
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    RETURN(ntStatus, UMSS_DispatchIrp);
}


NTSTATUS
UMSS_ProcessSysControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
Routine Description:

    Main dispatch table routine for IRP_MJ_SYSTEM_CONTROL
	We basically just pass these down to the PDO

Arguments:

    DeviceObject - pointer to FDO device object
    Irp          - pointer to an I/O Request Packet

Return Value:

	Status returned from lower driver

--*/

{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS waitStatus;
    PDEVICE_OBJECT stackDeviceObject;

    ENTER(UMSS_ProcessSysControlIrp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get a pointer to the device extension
    //
    deviceExtension = DeviceObject->DeviceExtension;
    stackDeviceObject = deviceExtension->TopOfStackDeviceObject;

    UMSS_IncrementIoCount(DeviceObject);

    UMSS_ASSERT( IRP_MJ_SYSTEM_CONTROL == irpStack->MajorFunction );

    IoCopyCurrentIrpStackLocationToNext(Irp);

    ntStatus = IoCallDriver(stackDeviceObject,
			    Irp);

    UMSS_DecrementIoCount(DeviceObject);

    RETURN(ntStatus, UMSS_ProcessSysControlIrp);
}


VOID
UMSS_Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++
Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:

    NONE

--*/

{
    ENTER(UMSS_Unload);

    //
    // Free any global resources allocated
    // in DriverEntry.

    // We have few or none because for a PNP device, almost all
    // allocation is done in PnpAddDevice() and all freeing 
    // while handling IRP_MN_REMOVE_DEVICE:
    //
    UMSS_ASSERT( gExAllocCount == 0 );

    EXIT(UMSS_Unload);
}



NTSTATUS
UMSS_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT *DeviceObject
    )
/*++
Routine Description:

    Creates a Functional DeviceObject

Arguments:

    DriverObject - pointer to the driver object for device
    DeviceObject - pointer to DeviceObject pointer to return
		    created device object.
    Instance - instance of the device create.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/

{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension;
    USHORT i;

    ENTER(UMSS_CreateDeviceObject);

    ntStatus = IoCreateDevice(
        DriverObject,
        sizeof (DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_UNKNOWN,
        FILE_AUTOGENERATED_DEVICE_NAME,
        FALSE,
        DeviceObject
        );

    if (NT_SUCCESS(ntStatus))
    {
        deviceExtension = (PDEVICE_EXTENSION) ((*DeviceObject)->DeviceExtension);
    }

    UMSS_KdPrintCond(
        DBGLVL_DEFAULT,
        (!(NT_SUCCESS(ntStatus))),
        ("UMSS_CreateDeviceObject() IoCreateDevice() FAILED\n"));

 
    if (!NT_SUCCESS(ntStatus))
    {
        RETURN(ntStatus, UMSS_CreateDeviceObject);
    }

    deviceExtension->DeviceObjectType = DO_FDO;

    //default maximum transfer size per io request
    deviceExtension->MaximumTransferSize =  UMSS_MAX_TRANSFER_SIZE ;

    // this event is triggered when there is no pending io of any kind and device is removed
    KeInitializeEvent(&deviceExtension->RemoveEvent, NotificationEvent, FALSE);

    // this event is triggered when self-requested power irps complete
    KeInitializeEvent(&deviceExtension->SelfRequestedPowerIrpEvent, NotificationEvent, FALSE);

    // this event is triggered when there is no pending io  (pending io count == 1 )
    KeInitializeEvent(&deviceExtension->NoPendingIoEvent, NotificationEvent, FALSE);

    // spinlock used to protect inc/dec iocount logic
    KeInitializeSpinLock (&deviceExtension->IoCountSpinLock);

    RETURN(ntStatus, UMSS_CreateDeviceObject);
}


NTSTATUS
UMSS_CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    )
/*++
Routine Description:

    Passes a URB to the USBD class driver
    The client device driver passes USB request block (URB) structures 
    to the class driver as a parameter in an IRP with Irp->MajorFunction
    set to IRP_MJ_INTERNAL_DEVICE_CONTROL and the next IRP stack location 
    Parameters.DeviceIoControl.IoControlCode field set to 
    IOCTL_INTERNAL_USB_SUBMIT_URB. 

Arguments:

    DeviceObject - pointer to the physical device object (PDO)
    Urb - pointer to an already-formatted Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/

{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    ENTER(UMSS_CallUSBD);

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
              IOCTL_INTERNAL_USB_SUBMIT_URB,
              deviceExtension->TopOfStackDeviceObject, //Points to the next-lower driver's device object
              NULL, // optional input bufer; none needed here
              0,    // input buffer len if used
              NULL, // optional output bufer; none needed here
              0,    // output buffer len if used
              TRUE, // If InternalDeviceControl is TRUE the target driver's Dispatch
                    //  outine for IRP_MJ_INTERNAL_DEVICE_CONTROL or IRP_MJ_SCSI 
                    // is called; otherwise, the Dispatch routine for 
                    // IRP_MJ_DEVICE_CONTROL is called.
              &event,     // event to be signalled on completion
              &ioStatus);  // Specifies an I/O status block to be set when the request is completed the lower driver. 

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    UMSS_ASSERT(nextStack != NULL);

    //
    // pass the URB to the USB driver stack
    //
    nextStack->Parameters.Others.Argument1 = Urb;

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);

    UMSS_KdPrint( DBGLVL_MAXIMUM,("UMSS_CallUSBD() return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING)
    {
        status = KeWaitForSingleObject(
                     &event,
                     Suspended,
                     KernelMode,
                     FALSE,
                     NULL);
    } else
    {
	ioStatus.Status = ntStatus;
    }

    UMSS_KdPrint( DBGLVL_MAXIMUM,("UMSS_CallUSBD() URB status = %x status = %x irp status %x\n",
	Urb->UrbHeader.Status, status, ioStatus.Status));

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    RETURN(ntStatus, UMSS_CallUSBD);
}


NTSTATUS
UMSS_ConfigureDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:

    Initializes a given instance of the device on the USB and
    selects and saves the configuration.

Arguments:

    DeviceObject - pointer to the physical device object for this instance of
                   the device.

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb;
    ULONG siz;

    ENTER(UMSS_ConfigureDevice);

    deviceExtension = DeviceObject->DeviceExtension;

    UMSS_ASSERT( deviceExtension->UsbConfigurationDescriptor == NULL );

    urb = UMSS_ExAllocatePool(NonPagedPool,
			 sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
    if ( !urb )
        return STATUS_INSUFFICIENT_RESOURCES;

    // When USB_CONFIGURATION_DESCRIPTOR_TYPE is specified for DescriptorType
    // in a call to UsbBuildGetDescriptorRequest(),
    // all interface, endpoint, class-specific, and vendor-specific descriptors 
    // for the configuration also are retrieved. 
    // The caller must allocate a buffer large enough to hold all of this 
    // information or the data is truncated without error.
    // Therefore the 'siz' set below is just a 'good guess', and we may have to retry

    siz = sizeof(USB_CONFIGURATION_DESCRIPTOR) + 512;  

    // We will break out of this 'retry loop' when UsbBuildGetDescriptorRequest()
    // has a big enough deviceExtension->UsbConfigurationDescriptor buffer not to truncate
    while( 1 )
    {
        deviceExtension->UsbConfigurationDescriptor = UMSS_ExAllocatePool(NonPagedPool, siz);

        if ( !deviceExtension->UsbConfigurationDescriptor )
        {
            UMSS_ExFreePool(urb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        UsbBuildGetDescriptorRequest(
            urb,
            (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
            USB_CONFIGURATION_DESCRIPTOR_TYPE,
            0,
            0,
            deviceExtension->UsbConfigurationDescriptor,
            NULL,
            siz,
            NULL
            );

        ntStatus = UMSS_CallUSBD(DeviceObject, urb);

        UMSS_KdPrint( DBGLVL_HIGH,("UMSS_CallUSBD() Configuration Descriptor = %x, len %x\n",
                                       deviceExtension->UsbConfigurationDescriptor,
                                       urb->UrbControlDescriptorRequest.TransferBufferLength));
        //
        // if we got some data see if it was enough.
        // NOTE: we may get an error in URB because of buffer overrun
        if (urb->UrbControlDescriptorRequest.TransferBufferLength>0 &&
            deviceExtension->UsbConfigurationDescriptor->wTotalLength > siz)
        {
            siz = deviceExtension->UsbConfigurationDescriptor->wTotalLength;
            UMSS_ExFreePool(deviceExtension->UsbConfigurationDescriptor);
            deviceExtension->UsbConfigurationDescriptor = NULL;
        }
        else
        {
            break;  // we got it on the first try
        }

    } // end, while (retry loop )

    UMSS_ExFreePool(urb);
    UMSS_ASSERT( deviceExtension->UsbConfigurationDescriptor );

    //
    // We have the configuration descriptor for the configuration we want.
    // Now we issue the select configuration command to get
    // the  pipes associated with this configuration.
    //

    ntStatus = UMSS_SelectInterface(
                   DeviceObject,
                   deviceExtension->UsbConfigurationDescriptor
                   );


    RETURN(ntStatus, UMSS_ConfigureDevice);
} 


NTSTATUS
UMSS_SelectInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    )
/*++
Routine Description:

    Initializes an device with (possibly) multiple interfaces;
    This minidriver only supports one interface (with multiple endpoints).

Arguments:

    DeviceObject - pointer to the device object for this instance of the 
                   device.
    ConfigurationDescriptor - pointer to the USB configuration
                   descriptor containing the interface and endpoint
                   descriptors.

Return Value:

    NT status code

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb = NULL;
    ULONG i;
    USHORT siz;
    PUSBD_INTERFACE_LIST_ENTRY interfaceList = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor = NULL;
    PUSBD_INTERFACE_INFORMATION Interface = NULL;

    ENTER(UMSS_SelectInterface);

    deviceExtension = DeviceObject->DeviceExtension;


    UMSS_KdPrint( DBGLVL_HIGH,("UMSS_SelectInterface() called with NULL Interface\n"));

    interfaceList = UMSS_ExAllocatePool(
                        PagedPool,
                        sizeof(USBD_INTERFACE_LIST_ENTRY) * 2);

    if (!interfaceList)
    {
        RETURN(STATUS_INSUFFICIENT_RESOURCES, UMSS_SelectInterface);
    }

    interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
                                  ConfigurationDescriptor,
                                  ConfigurationDescriptor, //search from start of config  descriptro
                                  -1,   // interface number not a criteria; we only support one interface
                                  -1,   // not interested in alternate setting here either
                                  -1,   // interface class not a criteria
                                  -1,   // interface subclass not a criteria
                                  -1    // interface protocol not a criteria
                                  );

    if ( !interfaceDescriptor )
    {
        UMSS_KdPrint( DBGLVL_HIGH,("UMSS_SelectInterface() ParseConfigurationDescriptorEx() failed\n  returning STATUS_INSUFFICIENT_RESOURCES\n"));
        UMSS_ExFreePool(interfaceList);
        RETURN(STATUS_INSUFFICIENT_RESOURCES, UMSS_SelectInterface);
    }

    interfaceList[0].InterfaceDescriptor = interfaceDescriptor;
    interfaceList[1].InterfaceDescriptor = NULL;

    urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor, interfaceList);

    UMSS_ExFreePool(interfaceList);

    if (urb)
    {
	Interface = &urb->UrbSelectConfiguration.Interface;

        for (i=0; i< Interface->NumberOfPipes; i++)
        {
	    //
	    // perform any pipe initialization here
	    //
	    Interface->Pipes[i].MaximumTransferSize = deviceExtension->MaximumTransferSize;
	    Interface->Pipes[i].PipeFlags = 0;
	}

        siz = GET_SELECT_CONFIGURATION_REQUEST_SIZE(1, Interface->NumberOfPipes);

	UsbBuildSelectConfigurationRequest(
            urb,
            siz,
            ConfigurationDescriptor);

        ntStatus = UMSS_CallUSBD(DeviceObject, urb);
    }
    else
    {
        UMSS_KdPrint( DBGLVL_HIGH,("UMSS_SelectInterface() USBD_CreateConfigurationRequest() failed\n  returning STATUS_INSUFFICIENT_RESOURCES\n"));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
	// Save the configuration handle for this device
	//
	deviceExtension->UsbConfigurationHandle =
	    urb->UrbSelectConfiguration.ConfigurationHandle;
        
        deviceExtension->UsbInterface = UMSS_ExAllocatePool(
                                            NonPagedPool,
                                            Interface->Length
                                            );

        if (deviceExtension->UsbInterface)
        {
            // save a copy of the interface information returned
            RtlCopyMemory(deviceExtension->UsbInterface, Interface, Interface->Length);

            Interface = deviceExtension->UsbInterface;
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        // Per Q200977, we must re-select the interface to get the correct
        // max transfer size if this is a composite device

        siz = GET_SELECT_INTERFACE_REQUEST_SIZE (Interface->NumberOfPipes);

        RtlZeroMemory(urb, siz);

        urb->UrbSelectInterface.Hdr.Length = siz;
        urb->UrbSelectInterface.Hdr.Function = URB_FUNCTION_SELECT_INTERFACE;
        urb->UrbSelectInterface.ConfigurationHandle = deviceExtension->UsbConfigurationHandle;

        for (i=0; i< Interface->NumberOfPipes; i++)
        {
	    Interface->Pipes[i].MaximumTransferSize = deviceExtension->MaximumTransferSize;
	}

        RtlCopyMemory(
            &urb->UrbSelectInterface.Interface,
            Interface,
            Interface->Length);

        ntStatus = UMSS_CallUSBD(DeviceObject, urb);
    }

    if (NT_SUCCESS(ntStatus))
    {
        ULONG j;

        // save a copy of the interface information returned (again)
        RtlCopyMemory(
            deviceExtension->UsbInterface,
            &urb->UrbSelectInterface.Interface,
            urb->UrbSelectInterface.Interface.Length);

        //
        // Dump the interface to the debugger
        //
        UMSS_KdPrint( 1,("---------\n"));
        UMSS_KdPrint( 1,("NumberOfPipes 0x%x\n", deviceExtension->UsbInterface->NumberOfPipes));
        UMSS_KdPrint( 1,("Length 0x%x\n", deviceExtension->UsbInterface->Length));
        UMSS_KdPrint( 1,("Alt Setting 0x%x\n", deviceExtension->UsbInterface->AlternateSetting));
        UMSS_KdPrint( 1,("Interface Number 0x%x\n", deviceExtension->UsbInterface->InterfaceNumber));
        UMSS_KdPrint( 1,("Class, subclass, protocol 0x%x 0x%x 0x%x\n",
            deviceExtension->UsbInterface->Class,
            deviceExtension->UsbInterface->SubClass,
            deviceExtension->UsbInterface->Protocol));

        // Dump the pipe info
        for (j=0; j<Interface->NumberOfPipes; j++)
        {
            PUSBD_PIPE_INFORMATION pipeInformation;
            pipeInformation = &deviceExtension->UsbInterface->Pipes[j];

            UMSS_KdPrint( 1,("---------\n"));
            UMSS_KdPrint( 1,("PipeType 0x%x\n", pipeInformation->PipeType));
            UMSS_KdPrint( 1,("EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
            UMSS_KdPrint( 1,("MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
            UMSS_KdPrint( 1,("Interval 0x%x\n", pipeInformation->Interval));
            UMSS_KdPrint( 1,("Handle 0x%x\n", pipeInformation->PipeHandle));
            UMSS_KdPrint( 1,("MaximumTransferSize 0x%x\n", pipeInformation->MaximumTransferSize));

            switch (pipeInformation->PipeType)
            {
                 case UsbdPipeTypeBulk:
                     if (USBD_PIPE_DIRECTION_IN(pipeInformation))
                     {
                         UMSS_KdPrint( 1,("DataInPipe 0x%x\n", j));
                         deviceExtension->DataInPipe = j;
                     }
                     else
                     {
                         UMSS_KdPrint( 1,("DataOutPipe 0x%x\n", j));
                         deviceExtension->DataOutPipe = j;
                     }
                     break;
			

                 case UsbdPipeTypeInterrupt:
                     UMSS_KdPrint( 1,("StatusPipe 0x%x\n", j));
                     deviceExtension->StatusPipe = j;
                     break;


                 default:
                     UMSS_KdPrint( 1,("Unknown pipe 0x%x\n", j));
                     break;
            }
        }

        UMSS_KdPrint( DBGLVL_MEDIUM,("---------\n"));


        // Figure out the protocol this storage device uses
        deviceExtension->DeviceProtocol = PROTOCOL_UNDEFINED;

        if (!UMSS_GetDeviceProtocolFromRegistry(DeviceObject))
        {
            if (!UMSS_GetDeviceProtocolFromDescriptor(DeviceObject))
            {
                // Can't determine device protocol
                ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
            }
        }

		if (NT_SUCCESS(ntStatus) && !UMSS_CheckForDeviceType(DeviceObject))
		{
			// Not our device
			ntStatus = STATUS_PLUGPLAY_NO_DEVICE;  // Is this the right status to return?
		}
    }


    if (NT_SUCCESS(ntStatus) && PROTOCOL_BULKONLY == deviceExtension->DeviceProtocol)
    {
        // Query device for max LUN number
        deviceExtension->MaxLun = UMSS_BulkOnlyGetMaxLun(deviceExtension);
    }
    else
    {
        // CBI protocol doesn't implement LUNs, so set to 0
        deviceExtension->MaxLun = 0;
    }

    if (urb)
    {
        // don't call the UMSS_ExFreePool since the buffer was 
        //  alloced by USBD_CreateConfigurationRequest, not UMSS_ExAllocatePool()
        ExFreePool(urb);
    }

    RETURN(ntStatus, UMSS_SelectInterface);
}



NTSTATUS
UMSS_ResetPipe(
    IN PDEVICE_OBJECT DeviceObject,
    USBD_PIPE_HANDLE PipeHandle
    )
/*++
Routine Description:

    Reset a given USB pipe.

    NOTES:

    This will reset the host to Data0 and should also reset the device to Data0 

Arguments:

    Ptrs to our FDO and a USBD_PIPE_INFORMATION struct

Return Value:

    NT status code

--*/

{
    NTSTATUS ntStatus;
    PURB urb;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;

    ENTER(UMSS_ResetPipe);

    UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_ResetPipe() Reset Pipe %x\n", PipeHandle));

    urb = UMSS_ExAllocatePool(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));

    if (urb)
    {
	urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
	urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
        urb->UrbPipeRequest.PipeHandle = PipeHandle;

        ntStatus = UMSS_CallUSBD(DeviceObject, urb);

        UMSS_ExFreePool(urb);

    }
    else
    {
	ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!(NT_SUCCESS(ntStatus)))
    {

#if DBG
        if ( gpDbg )
            gpDbg->PipeErrorCount++;
#endif
        UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_ResetPipe() FAILED, ntStatus =0x%x\n", ntStatus ));

    }
    else
    {

#if DBG
        if ( gpDbg )
            gpDbg->ResetPipeCount++;
#endif
    UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_ResetPipe() SUCCESS, ntStatus =0x%x\n", ntStatus ));

    }

    RETURN(ntStatus, UMSS_ResetPipe);
}

LONG
UMSS_DecrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:

    We keep a pending IO count ( extension->PendingIoCount )  in the device extension.
    The first increment of this count is done on adding the device.
    Subsequently, the count is incremented for each new IRP received and
    decremented when each IRP is completed or passed on.

    Transition to 'one' therefore indicates no IO is pending and signals
    deviceExtension->NoPendingIoEvent. This is needed for processing
    IRP_MN_QUERY_REMOVE_DEVICE

    Transition to 'zero' signals an event ( deviceExtension->RemoveEvent )
    to enable device removal. This is used in processing for IRP_MN_REMOVE_DEVICE
 
Arguments:

    DeviceObject -- ptr to our FDO

Return Value:

    deviceExtension->PendingIoCount

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    LONG ioCount;
    KIRQL oldIrql;

    deviceExtension = DeviceObject->DeviceExtension;
    KeAcquireSpinLock (&deviceExtension->IoCountSpinLock, &oldIrql);

    ioCount = InterlockedDecrement(&deviceExtension->PendingIoCount);

#if DBG
    InterlockedDecrement(&gpDbg->PendingIoCount);
#endif

    UMSS_TrapCond( DBGLVL_HIGH,( 0 > ioCount ) );

    if (ioCount==1)
    {
	// trigger no pending io
	KeSetEvent(
            &deviceExtension->NoPendingIoEvent,
            1,
            FALSE
            );
    }

    if (ioCount==0)
    {
	// trigger remove-device event
	KeSetEvent(
            &deviceExtension->RemoveEvent,
            1,
            FALSE
            );
    }

    KeReleaseSpinLock (&deviceExtension->IoCountSpinLock, oldIrql);

    UMSS_KdPrint( DBGLVL_HIGH,("Exit UMSS_DecrementIoCount() Pending io count = %x\n", ioCount));
    return ioCount;
}


VOID
UMSS_IncrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:

    We keep a pending IO count ( extension->PendingIoCount )  in the device extension.
    The first increment of this count is done on adding the device.
    Subsequently, the count is incremented for each new IRP received and
    decremented when each IRP is completed or passed on.
 
Arguments:

    DeviceObject -- ptr to our FDO

Return Value:

    NONE

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    KIRQL oldIrql;

    deviceExtension = DeviceObject->DeviceExtension;

    UMSS_KdPrint( DBGLVL_HIGH,("Enter UMSS_IncrementIoCount() Pending io count = %x\n", deviceExtension->PendingIoCount));

    KeAcquireSpinLock (&deviceExtension->IoCountSpinLock, &oldIrql);

    InterlockedIncrement(&deviceExtension->PendingIoCount);
#if DBG
    InterlockedIncrement(&gpDbg->PendingIoCount);
#endif
    KeReleaseSpinLock (&deviceExtension->IoCountSpinLock, oldIrql);

    UMSS_KdPrint( DBGLVL_HIGH,("Exit UMSS_IncrementIoCount() Pending io count = %x\n", deviceExtension->PendingIoCount));
}



BOOLEAN UMSS_GetDeviceProtocolFromRegistry(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:

    Retrieves the device protocol to be used for the USB mass storage
    device from the device's registry section.

 Arguments:

    DeviceObject - Device object for USB mass storage device.

Return Value:

    TRUE if device protocol found in registry.
    FALSE if device protocol not found in registry.

--*/

{
    HANDLE RegistryHandle;
    UCHAR DeviceProtocol;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    PDEVICE_EXTENSION DeviceExtension;

    ENTER(UMSS_GetDeviceProtocolFromRegistry);

    DeviceExtension = DeviceObject->DeviceExtension;

    if (NT_SUCCESS(IoOpenDeviceRegistryKey(
           DeviceExtension->PhysicalDeviceObject,
           PLUGPLAY_REGKEY_DEVICE,
           STANDARD_RIGHTS_ALL,
           &RegistryHandle) ) )
    {
        RtlZeroMemory (&paramTable[0], sizeof(paramTable));

        paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name          = L"DeviceProtocol";
        paramTable[0].EntryContext  = &DeviceProtocol;
        paramTable[0].DefaultType   = REG_BINARY;
        paramTable[0].DefaultData   = &DeviceProtocol;
        paramTable[0].DefaultLength = sizeof(DeviceProtocol);

        RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                               (PCWSTR)RegistryHandle,
                               &paramTable[0],
                               NULL,           // Context
                               NULL);          // Environment

        ZwClose(RegistryHandle);

        DeviceExtension->DeviceProtocol = DeviceProtocol;

        switch (DeviceProtocol)
        {
            case PROTOCOL_BULKONLY:
                UMSS_KdPrint( DBGLVL_DEFAULT,("Protocol from registry - BULK ONLY\n"));
                return TRUE;

            case PROTOCOL_CBI:
                UMSS_KdPrint( DBGLVL_DEFAULT,("Protocol from registry - CONTROL/BULK/INTERRUPT\n"));
                return TRUE;

            case PROTOCOL_CB:
                UMSS_KdPrint( DBGLVL_DEFAULT,("Protocol from registry - CONTROL/BULK\n"));
                return TRUE;
        }
    }

    DeviceExtension->DeviceProtocol = PROTOCOL_UNDEFINED;

    RETURN(FALSE, UMSS_GetDeviceProtocolFromRegistry);
}



BOOLEAN UMSS_CheckForDeviceType(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:

    Retrieves the device chip ID.  We hardcode the GetChipId request.
	Format and data taken from updater sending the SCSI pass through
	command.

 Arguments:

    DeviceObject - Device object for USB mass storage device.

Return Value:

    Non-zero if the returned chip ID matches a valid ID
    FALSE if device does not return a chip ID, or is an unsupported ID

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
	IOPACKET IoPacket;
    PURB Urb;
    PIRP Irp;
    CHAR StackSize;
	CHAR cdb[16] = {0};
	USHORT chipId = 0;
	NTSTATUS ntStatus;

    ENTER(UMSS_CheckForDeviceType);

    DeviceExtension = DeviceObject->DeviceExtension;

    // Allocate IRP for our I/O request
    StackSize = (CCHAR)(DeviceExtension->TopOfStackDeviceObject->StackSize + 1);
    Irp = IoAllocateIrp(StackSize, FALSE);

    if (!Irp) {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Failure due to memory allocation error - Irp\n"));
	    RETURN(FALSE, UMSS_CheckForDeviceType);
	 }

	// Allocate the URB for the request
    Urb = UMSS_ExAllocatePool(NonPagedPool, sizeof (struct _URB_BULK_OR_INTERRUPT_TRANSFER));

    if (!Urb) {
        UMSS_KdPrint( DBGLVL_MINIMUM,("Failure due to memory allocation error - Urb\n"));
        IoFreeIrp(Irp);
	    RETURN(FALSE, UMSS_CheckForDeviceType);
    }

    // Increment I/O count.  It will decrement when I/O request completes.
    UMSS_IncrementIoCount(DeviceObject);

	// initialize to zero; will be the return value.  non-zero indicates our device
    DeviceExtension->DeviceType = 0x00;

	// Initialize the event we will wait on while the request completes
	DeviceExtension->InternalRequest = TRUE;
	KeInitializeEvent (&DeviceExtension->InternalRequestCompleteEvent, NotificationEvent, FALSE);

	// hardcode the request
	IoPacket.Fdo = DeviceExtension->Fdo;
	IoPacket.Iop = NULL;
	IoPacket.Cdb = cdb;
	IoPacket.CdbLength = 16;
    IoPacket.DataBuffer = &chipId;
    IoPacket.DataLength = 2;
    IoPacket.BlockSize = 0x200;
    IoPacket.Flags = 1;
    IoPacket.Status = 1;
    IoPacket.Lun = 0;

	// The SCSI cdb
	cdb[0] = (CHAR)0xC0;  // from updater stddiapi.h: ST_SCSIOP_READ_COMMAND
	cdb[1] = (CHAR)0x30;  // from updater stddiapi.h: DDI_GET_CHIP_MAJOR_REV_ID

    // Store all the transfer request info in our device extension
    DeviceExtension->Urb = Urb;
    DeviceExtension->Irp = Irp;
    DeviceExtension->CurrentSGD = 0;
    DeviceExtension->IoPacket = &IoPacket;

    // There a data phase
    DeviceExtension->BytesToTransfer = IoPacket.DataLength;

	// Start the request and wait for the completion routine to set the event
    UMSS_BulkOnlyStartIo(DeviceExtension);

	ntStatus = KeWaitForSingleObject (&DeviceExtension->InternalRequestCompleteEvent, 
					Suspended, KernelMode, FALSE, NULL);

	// Reset the event
	DeviceExtension->InternalRequest = FALSE;
	KeResetEvent (&DeviceExtension->InternalRequestCompleteEvent);

	if (ntStatus != STATUS_SUCCESS)
	{
        UMSS_KdPrint( DBGLVL_MINIMUM,("Internal request failed\n"));
	    RETURN(FALSE, UMSS_CheckForDeviceType);
	}
	else
	{	// Chip Id is written in Big Endian
/*		if ( (chipId == 0x1034) ||  // That's a 3410
			 (chipId == 0x0035) ||  // That's a 3500
			 (chipId == 0x0036) ||  // That's a 3600
			 (chipId == 0x6036) ||  // That's a 3600
			 (chipId == 0x5036) ||  // That's a 3600
			 (chipId == 0xFF36) ||  // That's a 3600
             (chipId == 0x3036) ||  // That's a 3630
			 (chipId == 0x0020) )	// That's a 2000 thumb drive

*/      if ( ((chipId & 0x00FF) == 0x0036) ||     // That's a 3600
		     (chipId == 0x1034) ||              // That's a 3410
			 (chipId == 0x0035)                 // That's a 3500
            )
		    DeviceExtension->DeviceType = 0x01;
	}

	// Free the irp and URB if not done so by the completion routine.
	// This would only happen if an error occurred early on.
    if (DeviceExtension->Irp)
	{
		IoFreeIrp(DeviceExtension->Irp);
		DeviceExtension->Irp = NULL;
    }

	if (DeviceExtension->Urb)
    {
	    UMSS_ExFreePool(DeviceExtension->Urb);
		DeviceExtension->Urb = NULL;
	}

	// Return the DeviceType as non-zero (successful ID), or
	// zero (device excluded).
    RETURN(DeviceExtension->DeviceType, UMSS_CheckForDeviceType);
}






BOOLEAN
UMSS_GetDeviceProtocolFromDescriptor(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:

    Retrieves the device protocol to be used for the USB mass storage
    device from the device's interface descriptor.

 Arguments:

    DeviceObject - Device object for USB mass storage device.

Return Value:

    TRUE if valid device protocol found in descriptor.
    FALSE if valid device protocol not found in descriptor.

--*/

{
    ULONG DeviceProtocol;
    PDEVICE_EXTENSION DeviceExtension;

    ENTER(UMSS_GetDeviceProtocolFromDescriptor);

    DeviceExtension = DeviceObject->DeviceExtension;

    if (DeviceExtension->UsbInterface->Class == CLASS_MASS_STORAGE)
    {
        DeviceExtension->DeviceProtocol = DeviceExtension->UsbInterface->Protocol;

        switch (DeviceExtension->DeviceProtocol)
        {
            case PROTOCOL_BULKONLY:
                UMSS_KdPrint( DBGLVL_DEFAULT,("Protocol from descriptor - BULK ONLY\n"));
                return TRUE;

            case PROTOCOL_CBI:
                UMSS_KdPrint( DBGLVL_DEFAULT,("Protocol from descriptor - CONTROL/BULK/INTERRUPT\n"));
                return TRUE;

            case PROTOCOL_CB:
                UMSS_KdPrint( DBGLVL_DEFAULT,("Protocol from descriptor - CONTROL/BULK\n"));
                return TRUE;
        }
    }

    UMSS_KdPrint( DBGLVL_DEFAULT,("No protocol found in descriptor!\n"));

    DeviceExtension->DeviceProtocol = PROTOCOL_UNDEFINED;

    RETURN(FALSE, UMSS_GetDeviceProtocolFromDescriptor);
}


NTSTATUS
UMSS_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG PortStatus
    )
/*++
Routine Description:

    Returns the status of the USB port that the device is plugged into.
    Used to determine if the device is still attached.

Arguments:

    DeviceObject - pointer to the physical device object (PDO)
    PortStatus - pointer to ULONG that receives the port status

Return Value:

    Result of the request for the port status.    

--*/

{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    ENTER(UMSS_GetPortStatus);

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
              IOCTL_INTERNAL_USB_GET_PORT_STATUS,
              deviceExtension->TopOfStackDeviceObject, //Points to the next-lower driver's device object
              NULL, // optional input bufer; none needed here
              0,    // input buffer len if used
              NULL, // optional output bufer; none needed here
              0,    // output buffer len if used
              TRUE, // If InternalDeviceControl is TRUE the target driver's Dispatch
                    //  outine for IRP_MJ_INTERNAL_DEVICE_CONTROL or IRP_MJ_SCSI 
                    // is called; otherwise, the Dispatch routine for 
                    // IRP_MJ_DEVICE_CONTROL is called.
              &event,     // event to be signalled on completion
              &ioStatus);  // Specifies an I/O status block to be set when the request is completed the lower driver. 

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    UMSS_ASSERT(nextStack != NULL);

    //
    // pass the URB to the USB driver stack
    //
    nextStack->Parameters.Others.Argument1 = PortStatus;

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);

    UMSS_KdPrint( DBGLVL_MAXIMUM,("UMSS_GetPortStatus() return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING)
    {
        status = KeWaitForSingleObject(
                     &event,
                     Suspended,
                     KernelMode,
                     FALSE,
                     NULL);
    } else
    {
	ioStatus.Status = ntStatus;
    }

    UMSS_KdPrint( DBGLVL_MAXIMUM,("UMSS_GetPortStatus() status = %x irp status %x\n",
        status, ioStatus.Status));

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    RETURN(ntStatus, UMSS_GetPortStatus);
}

NTSTATUS
UMSS_ResetParentPort(
    IN IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Reset our parent port

Arguments:

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION deviceExtension;

    ENTER(UMSS_ResetParentPort);

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_RESET_PORT,
                deviceExtension->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, // internal ( use IRP_MJ_INTERNAL_DEVICE_CONTROL )
                &event,
                &ioStatus);

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    UMSS_ASSERT(nextStack != NULL);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                            irp);
                            
    UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_ResetParentPort() return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {

        UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_ResetParentPort() Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

        UMSS_KdPrint( DBGLVL_DEFAULT,("UMSS_ResetParentPort() Wait for single object, returned %x\n", status));
        
    } else {
        ioStatus.Status = ntStatus;
    }

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    RETURN(ntStatus, UMSS_ResetParentPort);
}



