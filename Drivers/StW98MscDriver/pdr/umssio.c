/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    UmssIo.c 

Abstract:

    IOS port driver for USB Mass Storage Sample driver
    I/O module

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    03/19/99: MRB  Original

--*/

#define WANTVXDWRAPS
#define _NTDEF_

#include <debug.h>
#include <vxdwraps.h>

#include <srb.h>
#include <scsi.h>
#include <aep.h>
#include <drp.h>
#include <isp.h>
#include <srb.h>
#include <ior.h>
#include <iop.h>
#include <dcb.h>
#include <ilb.h>
#include "UMSSPDR.h"
#include "umssdbg.h"

#include "vendor.h"

// NOTE: This module cannot include VMM.H, due to conflicts in base
// definitions that occur when SCSI.H is included.  Any calls to
// VMM or VxD services should be made from another source file.


// Indicate locked code and data.  We can't use macros defined in VMM.H,
// so we use the same pragmas defined in VMM.H for locked code and data.
#pragma code_seg("_LTEXT", "LCODE")
#pragma data_seg("_LDATA", "LCODE")

// IOS ILB function type definitions
typedef PVOID (__cdecl *ILB_internal_request_func)( struct ISP * );
typedef PVOID (__cdecl *ILB_enqueue_iop_func)( pIOP, PDCB);
typedef pIOP (__cdecl *ILB_dequeue_iop_func)( PDCB);


VOID
UMSSPDR_AerDeviceInquiry(
    PAEP_inquiry_device Aep
    )
/*++

Routine Description:

    AEP_DEVICE_INQUIRY handler.  Builds INQUIRY SRB and sends it to
    our USB device to find out what device is attached.  Returned INQUIRY data
    is copied into the DCB.

Arguments:

    None

Return Value:

    NONE
--*/

{
    PDCB                Dcb;
    pIOP                Iop;
    PIOR                Ior;
    ISP_IOP_alloc       IOPCreateISP;
    ISP_mem_alloc       AllocISP;
    ULONG               IOPLength;
    PSCSI_REQUEST_BLOCK Srb;                       
    PCDB                Cdb;
    PINQUIRYDATA        pInquiryBuf;
    PUSBDDB             UsbDdb;
	    
    ENTER(UMSSPDR_AerDeviceInquiry)

    Dcb = (PDCB)Aep->AEP_i_d_dcb;

    UsbDdb = (PUSBDDB) Aep->AEP_i_d_hdr.AEP_ddb;

    // Remove the INQUIRY DCB from our DCB list.  The INQUIRY DCB only exists
    // during the device inquiry phase.  There is no AEP_UnconfigDcb sent for
    // it when it is deallocated, so we need to remove it now.  
    UMSSPDR_RemoveDcbFromList(UsbDdb, Dcb);

    // Verify that the INQUIRY is for a valid LUN
    if (Dcb->DCB_scsi_lun > UsbDdb->MaxLun)
    {
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("INQUIRY sent for invalid LUN\n"));
        Aep->AEP_i_d_hdr.AEP_result = AEP_NO_INQ_DATA;
        EXIT(UMSSPDR_AerDeviceInquiry)
    }
 
    // Allocate an IOP for our INQUIRY command
    IOPLength = sizeof( IOP ) +
                sizeof( SCSI_REQUEST_BLOCK ) +
                sizeof( INQUIRYDATA );

    ((PISP)&IOPCreateISP)->ISP_func = ISP_CREATE_IOP;
    IOPCreateISP.ISP_IOP_size       = IOPLength;
    IOPCreateISP.ISP_delta_to_ior   = (ULONG)&(((pIOP)0L)->IOP_ior);
    IOPCreateISP.ISP_i_c_flags      = ISP_M_FL_MUST_SUCCEED;

    // Call IOS
    ILBService(&IOPCreateISP);

    // Our allocated IOP memory will be formatted like this:
    // ---------------------------
    // |  IOP/IOR                |
    // ---------------------------
    // |  SRB                    |
    // ---------------------------
    // |  INQUIRY DATA BUFFER    |
    // ---------------------------

    //
    // Build our I/O request
    //
    Iop = (pIOP)(PVOID)IOPCreateISP.ISP_IOP_ptr;
    Ior = (PIOR)&(Iop->IOP_ior);
    Srb = (SCSI_REQUEST_BLOCK *)(Iop + 1);

    pInquiryBuf = (PINQUIRYDATA) (((PCHAR)Iop)+ IOPLength - sizeof(INQUIRYDATA));

    Iop->IOP_original_dcb = Iop->IOP_physical_dcb = (ULONG)Dcb;

    Iop->IOP_srb = (ULONG)Srb;

    Ior->IOR_func = IOR_SCSI_PASS_THROUGH;
    Ior->IOR_req_vol_handle = 0L;
    Ior->IOR_flags = IORF_SYNC_COMMAND | 
                        IORF_CHAR_COMMAND |
                        IORF_VERSION_002 | 
                        IORF_SRB_VALID | 
                        IORF_BYPASS_VOLTRK |
                        IORF_INHIBIT_GEOM_RECOMPUTE;

    Ior->IOR_buffer_ptr = (ULONG) pInquiryBuf;
    Ior->IOR_xfer_count = sizeof( INQUIRYDATA );
    Ior->IOR_sgd_lin_phys = (ULONG)0;
    Ior->IOR_next = 0L;     

    Srb->Length = sizeof( SCSI_REQUEST_BLOCK );
    Srb->TimeOutValue = 30; 
    Srb->CdbLength = 6;

    // We won't do a REQUEST SENSE if the INQUIRY command fails,
    // since we will always fail on any error that occurs for
    // INQUIRY.
    Srb->SenseInfoBuffer = NULL;

    Srb->SrbFlags = SRB_FLAGS_DATA_IN |
                    SRB_FLAGS_DISABLE_AUTOSENSE;

    Srb->DataBuffer = (PVOID) Ior->IOR_buffer_ptr;
    Srb->DataTransferLength = sizeof( INQUIRYDATA );
    Srb->PathId = Dcb->DCB_bus_number;
    Srb->TargetId = Dcb->DCB_scsi_target_id;
    Srb->Lun = Dcb->DCB_scsi_lun;
    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->SrbExtension = (PCHAR)Srb;

    // Setup INQUIRY CDB
    Cdb = (PCDB)Srb->Cdb;
    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = Srb->Lun;
    Cdb->CDB6INQUIRY.Reserved1 = 0;
    Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;
    Cdb->CDB6INQUIRY.PageCode = 0;
    Cdb->CDB6INQUIRY.IReserved = 0;
    Cdb->CDB6INQUIRY.Control = 0;


    // Call IOS to fill in SG descriptors, etc.
    ILBCriteria(Iop);

    // Submit request to IOS 
    ILBRequest(Iop, Dcb);

    if( SRB_STATUS( Srb->SrbStatus ) == SRB_STATUS_SUCCESS ||
        SRB_STATUS( Srb->SrbStatus ) == SRB_STATUS_DATA_OVERRUN )
    {

        UMSSPDR_DebugPrintf(DBGLVL_DEFAULT, ("INQUIRY succeeded\n"));

        // Copy INQUIRY data into the DCB.  It is very important
	    // that the INQUIRY Peripheral Device Type field be
		// correct, as this is what IOS uses to determine what
        // kind of device has been enumerated (this is true for
	    // all bus types, including SCSI, IDE, ATAPI).
		_lmemcpy( (PCHAR)(Dcb->DCB_inquiry_flags),
				      (PCHAR)pInquiryBuf,
					  sizeof( Dcb->DCB_inquiry_flags ) +
	                  sizeof( Dcb->DCB_vendor_id ) +
		              sizeof( Dcb->DCB_product_id ) +
			          sizeof( Dcb->DCB_rev_level ));
    } 
    else
    {
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("No INQUIRY data found!\n"));
        Aep->AEP_i_d_hdr.AEP_result = AEP_NO_INQ_DATA;
    }

    // Free our IOP
    ((PISP)&AllocISP)->ISP_func  = ISP_DEALLOC_MEM;
    ((PISP_mem_dealloc)&AllocISP)->ISP_mem_ptr_da = (ULONG)Iop;
    ((ILB_internal_request_func)(UMSSPDR_Ilb.ILB_service_rtn))((PISP)&AllocISP);

    EXIT(UMSSPDR_AerDeviceInquiry)
}



VOID
UMSSPDR_Request(
    pIOP piop
    )
{
/*++

Routine Description:

    I/O request handler for the USB device.

Arguments:

    piop - I/O Packet containing IOS request information.

Return Value:

    None

--*/

    DWORD DeviceObject;
    PDCB Dcb;
    pIOP Iop;
    PIOR Ior;
    PUSBDDB Ddb;

    ENTER(UMSSPDR_Request)

    DeviceObject = (DWORD)((pDCB_cd_entry)piop->IOP_calldown_ptr)->DCB_cd_ddb;
    Dcb = (PDCB)piop->IOP_physical_dcb;
    Ddb = (PUSBDDB)((pDCB_cd_entry)piop->IOP_calldown_ptr)->DCB_cd_ddb;

    UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("Request - IOP = %x, DCB=%x\n", piop, Dcb));

    // Put the IOP on the queue for this DCB
    ILBEnqueueIop(piop, Dcb);

    // Check if we are currently busy
    if (Ddb->Flags & USBDDB_FLAG_BUSY)
    {
        UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("Device is busy\n"));

        // Device is already busy, so just return.  IOP will be
        // dequeued and processed after the current request completes
        EXIT(UMSSPDR_Request)
    }

    // Mark this device as now busy
    Ddb->Flags |= USBDDB_FLAG_BUSY;

    // Device is idle, so get next IOP for processing
    Iop = ILBDequeueIop(Dcb);

    // No IOP pending for current LUN, so check if other LUNs have I/O pending
    if (NULL == Iop)
        Iop = UMSSPDR_GetNextIop(Ddb);

    UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("Request - next IOP = %x\n", Iop));

    // Did we dequeue an IOP?
    if (Iop)
        // Yes, start processing it.
        UMSSPDR_StartIo(Iop);
    else
    {
        // No.  Something's wrong, this should never occur.
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("Unable to dequeue IOP\n"));
        Trap(); 
        Ddb->Flags &= ~USBDDB_FLAG_BUSY;
    }

    EXIT(UMSSPDR_Request)
}


VOID
UMSSPDR_StartIo(
    pIOP Iop
    )
{
/*++

Routine Description:

    Function that calls WDM driver to initiate I/O transaction.

Arguments:

    Iop - Dequeued IOP for next request.

Return Value:

    None

--*/

    DWORD DeviceObject;
    PDCB Dcb;
    PIOR Ior;
    PUSBDDB Ddb;
    SCSI_REQUEST_BLOCK *Srb;
    PIOPACKET IoPacket;


    ENTER(UMSSPDR_StartIo)

    DeviceObject = (DWORD)((pDCB_cd_entry)Iop->IOP_calldown_ptr)->DCB_cd_ddb;
    Dcb = (PDCB)Iop->IOP_physical_dcb;
    Ddb = (PUSBDDB)((pDCB_cd_entry)Iop->IOP_calldown_ptr)->DCB_cd_ddb;
    Ior = &(Iop->IOP_ior);
    Srb = (PSCSI_REQUEST_BLOCK)Iop->IOP_srb;

    UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("StartIo - IOP = %x, DCB=%x\n", Iop, Dcb));

    // Get address of IOPACKET that we send to WDM driver
    IoPacket = &(Ddb->IoPacket);

    if (Ior->IOR_func > IOR_ASYNCHRONOUS_DRIVE_SPINUP)
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("Bogus IOR function - %x\n", Ior->IOR_func));

    UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("UMSSPDR_StartIo - IOP=%x, IOPACKET=%x\n", Iop, IoPacket));

    if ((Ior->IOR_flags & IORF_SRB_VALID) &&
        (Ior->IOR_func <= IOR_ASYNCHRONOUS_DRIVE_SPINUP) &&
        (Srb->CdbLength <= 16)) //SP 06/18/2003 modified form original limit of 12
    {
        UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("Calling WDM driver\n"));

        Srb->DataBuffer = (PVOID)Ior->IOR_buffer_ptr;

        // Populate the IOPACKET structure
        IoPacket->Fdo = Ddb->Fdo;
        IoPacket->Cdb = Srb->Cdb;
        IoPacket->CdbLength = Srb->CdbLength;
        IoPacket->DataBuffer = Srb->DataBuffer;
        IoPacket->DataLength = Srb->DataTransferLength;
        IoPacket->Lun = Srb->Lun;
        IoPacket->Iop = (PVOID)Iop;
        IoPacket->Flags = 0;
        IoPacket->Status = IO_STATUS_PENDING;
        IoPacket->BlockSize = Dcb->DCB_bdd.DCB_apparent_blk_size;

        if (Ior->IOR_flags & IORF_SCATTER_GATHER)
            IoPacket->Flags |= IO_FLAGS_SCATTER_GATHER;

        if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
            IoPacket->Flags |= IO_FLAGS_DATA_IN;
        else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT)
            IoPacket->Flags |= IO_FLAGS_DATA_OUT;

        UMSSPDR_ProcessIoPacket(IoPacket, Iop);

        // Call WDM driver
        pfnStartRequest(IoPacket);
    }
    else
    {
        UMSSPDR_DebugPrintf(DBGLVL_DEFAULT, ("IOP with no SRB, IOR_func=%x\n",
                                        Ior->IOR_func));

        // We only handle request with valid SRBs
        Ior->IOR_status = IORS_INVALID_COMMAND;
        UMSSPDR_CompleteIOP(Iop);
    }
    EXIT(UMSSPDR_StartIo)
}


VOID
UMSSPDR_ProcessIoPacket(
    PIOPACKET IoPacket,
    pIOP Iop
    )
{
    SCSI_REQUEST_BLOCK * Srb;
    PIOR Ior;

    ENTER(UMSSPDR_ProcessIoPacket)

    Ior = &(Iop->IOP_ior);
    Srb = (PSCSI_REQUEST_BLOCK)Iop->IOP_srb;

    switch (Srb->Cdb[0])
    {
        case SCSIOP_READCD:
            // CDVSD bug - doesn't set SRB data length for READ_CD commands
            IoPacket->DataLength = Ior->IOR_xfer_count;
            if (!(Ior->IOR_flags & IORF_CHAR_COMMAND))
            {
                IoPacket->DataLength *= IoPacket->BlockSize;
            }
            break;
    }

    EXIT(UMSSPDR_ProcessIoPacket)
}


void _stdcall
UMSSPDR_CompleteRequest(
    PIOPACKET IoPacket
    )
{
/*++

Routine Description:

    I/O completion handler for the USB device.

Arguments:

    IoPacket - IOPACKET containing request info

Return Value:

    None

--*/

    PSCSI_REQUEST_BLOCK Srb;
    PUSBDDB Ddb;
    int i;
    pIOP Iop;

    ENTER(UMSSPDR_CompleteRequest)

    Iop = (pIOP)IoPacket->Iop;
    Ddb = (PUSBDDB)((pDCB_cd_entry)Iop->IOP_calldown_ptr)->DCB_cd_ddb;
    Srb = (PSCSI_REQUEST_BLOCK)Iop->IOP_srb;

    UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("UMSSPDR_CompleteRequest - IOP=%x, IOPACKET=%x\n", Iop, IoPacket));

    // Are we completing a REQUEST SENSE command?
    if (Ddb->Flags & USBDDB_FLAG_ERROR)
    {
        UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("REQUEST SENSE complete\n"));

        if (IoPacket->Status == IO_STATUS_DEVICE_ERROR)
        {
            UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("REQUEST SENSE Failed!\n"));

            // Our request sense failed, so just return error without
            // sense data.
            Srb->SrbStatus = SRB_STATUS_ERROR;
            Trap();
        }
        else
        if (IoPacket->Status == IO_STATUS_SUCCESS)
        {
            //BUGBUG - put in debug code to dump sense info

            Srb->SrbStatus = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
        }

        Ddb->Flags &= ~USBDDB_FLAG_ERROR;
    }

    // Normal I/O request
    else switch (IoPacket->Status)
    {
        case IO_STATUS_DEVICE_ERROR:
            UMSSPDR_DebugPrintf(DBGLVL_DEFAULT, ("I/O request failed\n"));

            // Call error handler for REQUEST SENSE handling
            UMSSPDR_ErrorHandler(Iop);
            EXIT(UMSSPDR_CompleteRequest)

        case IO_STATUS_OUT_OF_MEMORY:
            UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("I/O request failed with memory error\n"));

            Srb->SrbStatus = SRB_STATUS_ERROR;
            Trap();
            break;

        case IO_STATUS_SUCCESS:
            UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("I/O request succeeded\n"));

            // I/O request succeeded
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            break;

        case IO_STATUS_DATA_OVERRUN:
            UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("I/O request succeeded - Data overrun/underrun\n"));

            // I/O request succeeded
            Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
            break;

        default:
            UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("Unknown error occurred!\n"));

            // This should never happen
            Srb->SrbStatus = SRB_STATUS_ERROR;
            Trap();
    }

    // Complete the request
    UMSSPDR_CompleteIOP(Iop);

    EXIT(UMSSPDR_CompleteRequest)
}


VOID
UMSSPDR_ErrorHandler(
    pIOP Iop
    )
{
/*++

Routine Description:

    Error handler for failed I/O requests.  Will send a REQUEST SENSE
    command to the device to find out why it failed, if SRB indicates
    autosense.

Arguments:

    Iop - IOP for failed request

Return Value:

    None

--*/

    SCSI_REQUEST_BLOCK *Srb;
    PUSBDDB Ddb;
    PIOPACKET IoPacket;

    ENTER(UMSSPDR_ErrorHandler)

    Srb = (PSCSI_REQUEST_BLOCK)Iop->IOP_srb;
    Ddb = (PUSBDDB)((pDCB_cd_entry)Iop->IOP_calldown_ptr)->DCB_cd_ddb;

    if (Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE)
    {
        // Client does not want sense data, so just set error status
        // and complete the request.
        Srb->SrbStatus = SRB_STATUS_ERROR;

        // Complete the request
        UMSSPDR_CompleteIOP(Iop);
        EXIT(UMSSPDR_ErrorHandler)
    }

    if ((NULL == Srb->SenseInfoBuffer) || (0 == Srb->SenseInfoBufferLength))
    {
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("Invalid Sense Info buffer\n"));

        // Invalid Sense Info buffer without the
        // SRB_FLAGS_DISABLE_AUTOSENSE flag.  This shouldn't happen.

        Srb->SrbStatus = SRB_STATUS_ERROR;
        Trap();

        // Complete the request
        UMSSPDR_CompleteIOP(Iop);
        EXIT(UMSSPDR_ErrorHandler)
    }

    // Indicate that we are processing an error
    Ddb->Flags |= USBDDB_FLAG_ERROR;

    // Build a REQUEST SENSE CDB
    Ddb->Cdb[0] =  SCSIOP_REQUEST_SENSE;
    Ddb->Cdb[1] =  0x00;
    Ddb->Cdb[2] =  0x00;
    Ddb->Cdb[3] =  0x00;
    Ddb->Cdb[4] =  Srb->SenseInfoBufferLength;
    Ddb->Cdb[5] =  0x00;
    Ddb->Cdb[6] =  0x00;
    Ddb->Cdb[7] =  0x00;
    Ddb->Cdb[8] =  0x00;
    Ddb->Cdb[9] =  0x00;
    Ddb->Cdb[10] = 0x00;
    Ddb->Cdb[11] = 0x00;

    IoPacket = &(Ddb->IoPacket);

    // Populate the IOPACKET structure sent to our WDM driver
    IoPacket->Fdo = Ddb->Fdo;
    IoPacket->Cdb = Ddb->Cdb;
    IoPacket->CdbLength = 12;
    IoPacket->DataBuffer = Srb->SenseInfoBuffer;
    IoPacket->DataLength = Srb->SenseInfoBufferLength;
    IoPacket->Iop = (PVOID)Iop;
    IoPacket->Flags = IO_FLAGS_DATA_IN;
    IoPacket->Status = IO_STATUS_PENDING;
    IoPacket->BlockSize = 512;

    // Call the WDM driver
    pfnStartRequest(IoPacket);

    EXIT(UMSSPDR_ErrorHandler)
}



VOID
UMSSPDR_CompleteIOP(
    pIOP Iop
    )
{
/*++

Routine Description:

    Completes IOP request

Arguments:

    Iop - IOP to complete

Return Value:

    None

--*/

    IOP_callback_entry * IopCB;
    pIOP NextIop;
    PUSBDDB Ddb;
    PDCB Dcb;

    ENTER(UMSSPDR_CompleteIOP)

    Ddb = (PUSBDDB)((pDCB_cd_entry)Iop->IOP_calldown_ptr)->DCB_cd_ddb;
    Dcb = (PDCB)Iop->IOP_physical_dcb;

    UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("Complete - IOP = %x, DCB=%x\n", Iop, Dcb));

    // Find address of first callback handler
    Iop->IOP_callback_ptr -= sizeof (IOP_callback_entry);
    IopCB = (IOP_callback_entry *)(Iop->IOP_callback_ptr);

    // Complete the IOP by calling the first entry in the callback chain
    SaveEbx();
    IOPCallBack(IopCB, Iop);
    RestoreEbx();

    // We are no longer busy
    Ddb->Flags &= ~USBDDB_FLAG_BUSY;
        
    // See if there are any queued requests
    NextIop = ILBDequeueIop(Dcb);

    // No IOP pending for current LUN, so check if other LUNs have I/O pending
    if (NULL == NextIop)
        NextIop = UMSSPDR_GetNextIop(Ddb);

    UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("Request - NextIOP %x\n", NextIop));

    if (NextIop)
    {
        Ddb->Flags |= USBDDB_FLAG_BUSY;

        // There is a queued IOP, so go process it
        UMSSPDR_StartIo(NextIop);
    }

    EXIT(UMSSPDR_CompleteIOP)
}

        

pIOP
UMSSPDR_GetNextIop(
    PUSBDDB Ddb
    )
/*++

Routine Description:

    Dequeues next IOP for the device

Arguments:

    Ddb - DDB for device 

Return Value:

    Next pending IOP, or NULL

--*/

{
    pIOP Iop=NULL;
    PDCB Dcb;

    ENTER(UMSSPDR_GetNextIop)

    // Get the first DCB attached to this DDB
    Dcb = UMSSPDR_GetFirstDcb(Ddb);

    if (NULL == Dcb)
        RETURN(NULL, UMSSPDR_GetNextIop)

    UMSSPDR_DebugPrintf(DBGLVL_MAXIMUM, ("First DCB = %x\n", Dcb));

    // Loop thru all DCBs attached to this DDB, until we either find an IOP
    // or run out of DCBs
    while (!(Iop = ILBDequeueIop(Dcb)) && (Dcb = UMSSPDR_GetNextDcb(Ddb, Dcb)));

    RETURN(Iop, UMSSPDR_GetNextIop)
}
