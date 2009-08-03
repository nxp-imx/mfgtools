/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    Umssaer.c 

Abstract:

    IOS port driver for USB Mass Storage Sample Driver
    Async Event Handler module

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

#include <basedef.h>
#include <vmm.h>
#include <debug.h>
#include <vxdwraps.h>
#include <winerror.h>
#include <aep.h>
#include <drp.h>
#include <isp.h>
#include <ior.h>
#include <iop.h>
#include <dcb.h>
#include <ilb.h>
#include <configmg.h>
#include <ntkern.h>
#include "UMSSPDR.h"
#include "umssdbg.h"
#include "umssaer.h"

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG



VOID
UMSSPDR_AsyncEventHandler (
    PAEP Aep
    )
/*++

Routine Description:

    IOS Asychronous event handler.

Arguments:

    Aep - Asychronous Event Packet.  Structure varies depending
          on AER type.

Return Value:

    Status returned in Aep->AEP_result

--*/
{
    ENTER(UMSSPDR_AsyncEventHandler)

    DBG_AEP_NAME(Aep);

    // presume success
    Aep->AEP_result = AEP_SUCCESS;

    switch (Aep->AEP_func)
    {
        case AEP_INITIALIZE:
            UMSSPDR_AerInit((PAEP_bi_init)Aep);
            break;
		
        case AEP_UNINITIALIZE:
            UMSSPDR_AerUninit((PAEP_bi_uninit)Aep);
            break;

        case AEP_DEVICE_INQUIRY:
            UMSSPDR_AerDeviceInquiry((PAEP_inquiry_device)Aep);
            break;

        case AEP_CONFIG_DCB:
            UMSSPDR_AerConfigDcb((PAEP_dcb_config)Aep);
            break;

        case AEP_UNCONFIG_DCB:
            UMSSPDR_AerUnconfigDcb((PAEP_dcb_unconfig)Aep);
            break;

        case AEP_BOOT_COMPLETE:
            UMSSPDR_AerBootComplete((PAEP_boot_done)Aep);
            break;

        case AEP_IOP_TIMEOUT:
            break;

        default:
            Aep->AEP_result = AEP_FAILURE;

    } // end switch on AEP function

    UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("AEP result = %x\n", Aep->AEP_result));

    EXIT(UMSSPDR_AsyncEventHandler)
}


VOID
UMSSPDR_AerInit(
    PAEP_bi_init Aep
    )
/*++

Routine Description:

    Initialization routine.

Arguments:

    Aep - AEP_bi_init structure.  

Return Value:

    NONE

--*/

{

    DWORD          Pdo, Fdo=0;
    DEVNODE        ParentDevNode;
    ISP_ddb_create Isp;
    PUSBDDB        UsbDdb;

    ENTER(UMSSPDR_AerInit)

    // We need to find the FDO created by UMSS.SYS for the current USB storage
    // device.  This FDO will be passed as a parameter to UMSS.SYS when we
    // do I/O calls so it knows which USB device to talk to.

    // To identify the correct FDO, we call an exported function in 
    // UMSS.SYS that enumerates each PDO/FDO pair attached to UMSS.SYS.
    // We can then use an exported service from NTKERN.VXD to translate
    // the PDO to a devnode handle, and then compare it to our parent's devnode
    // handle (we are loaded via a psuedo-child device created by UMSS.SYS,
    // so our parent is the physical USB device).

    // First, get our parent's DevNode handle
    if (CM_Get_Parent(&ParentDevNode, (DEVNODE)Aep->AEP_bi_i_hdevnode, 0) != CR_SUCCESS)
    {
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("Can't find our parent DevNode!\n\r"));
        Aep->AEP_bi_i_hdr.AEP_result = AEP_FAILURE;
        EXIT(UMSSPDR_AerInit)
    }
		

    //Loop thru all the PDO/FDO pairs looking for a match to our parent DevNode
    while ((Pdo = pfnGetNextPdo(&Fdo)) && (_NtKernPhysicalDeviceObjectToDevNode(Pdo) != ParentDevNode));
    if (!Pdo)
    {
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("Can't find PDO for our parent DevNode!\n\r"));
        Aep->AEP_bi_i_hdr.AEP_result = AEP_FAILURE;
        EXIT(UMSSPDR_AerInit)
    }
    
    // Bulk-only spec specifies max lun as 15
    Aep->AEP_bi_i_max_lun = 15;

    // Set maximum target ID for device
    Aep->AEP_bi_i_max_target = 0;

    // Allocate our DDB
    Isp.ISP_ddb_hdr.ISP_func = ISP_CREATE_DDB;

    // Include space for an array of DCB pointers in our DDB
    Isp.ISP_ddb_size = sizeof(USBDDB) + sizeof(PDCB) * (Aep->AEP_bi_i_max_lun);

    Isp.ISP_ddb_flags = 0;

    ILBService(&Isp);

    if (Isp.ISP_ddb_hdr.ISP_result)
    {
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("Can't allocate DDB!\n\r"));
        Trap();
        Aep->AEP_bi_i_hdr.AEP_result = AEP_FAILURE;
        EXIT(UMSSPDR_AerInit)
    }

    // Retrieve our new DDB
    UsbDdb = (PUSBDDB)(Isp.ISP_ddb_ptr);

    // Store pertinent information in our DDB.
    UsbDdb->Ddb.DDB_number_buses = 1;
    UsbDdb->Ddb.DDB_devnode_ptr = Aep->AEP_bi_i_hdevnode;
    UsbDdb->Fdo = Fdo;
    UsbDdb->Flags = 0;

    // Call UMSS.SYS to find out how many LUNs are on this device
    UsbDdb->MaxLun = pfnGetMaxLun(Fdo);

    // Add Ddb to our list of DDBs
    UMSSPDR_AddDdbToList(UsbDdb);

    // Register our I/O completion handler with UMSS.SYS
    pfnRegisterCompletionHandler(Fdo, UMSSPDR_CompleteRequest);

    EXIT(UMSSPDR_AerInit)
}



VOID
UMSSPDR_AerConfigDcb(
    PAEP_dcb_config Aep
    )
/*++

Routine Description:

    Configuration handler for new DCBs.

Arguments:

    Aep - AEP_dcb_config structure.  

Return Value:

    NONE
--*/

{
    PDCB Dcb;
    ISP_calldown_insert Isp;
    
    ENTER(UMSSPDR_AerConfigDcb)

    Dcb = (PDCB)Aep->AEP_d_c_dcb;

    //Set maximum scatter gather elements to 17
    Dcb->DCB_max_sg_elements = 17;

    //Set max transfer size to 64k
    Dcb->DCB_max_xfer_len = 0x10000;

    
    //Set port name to "UMSSPDR "
    *(PULONG)&Dcb->DCB_port_name[0] = 'MUTS';
    *(PULONG)&Dcb->DCB_port_name[4] = 'RDPS';

    // DCB_DEV2_IDE_FLOPTICAL is set specifically if we are a
    // LS-120 device.  This flag tells DISKVSD.VXD to special
    // handle this drive.  It causes the device type to change
    // from a hard-disk to a floppy.

    // This flag should only be set for LS-120 devices, as it
    // relies on capacity information being returned in the
    // vendor specific INQUIRY data, and assumes SFF-8070i
    // compatibility.   Devices that wish to appear as normal
    // hard-drives should *not* set the DCB_DEV2_IDE_FLOPTICAL
    // flag.

    Dcb->DCB_cmn.DCB_device_flags2 |= DCB_DEV2_ATAPI_DEVICE;
//                                    DCB_DEV2_IDE_FLOPTICAL;

    // Insert ourselves into the calldown stack for this device
    Isp.ISP_i_cd_hdr.ISP_func = ISP_INSERT_CALLDOWN;
    Isp.ISP_i_cd_dcb = (ULONG)Dcb;         
    Isp.ISP_i_cd_req = UMSSPDR_Request;     
    Isp.ISP_i_cd_expan_len = 0;             
    Isp.ISP_i_cd_lgn = Aep->AEP_d_c_hdr.AEP_lgn;      
    Isp.ISP_i_cd_ddb = (ULONG)Aep->AEP_d_c_hdr.AEP_ddb;        
    Isp.ISP_i_cd_flags = DCB_dmd_srb_cdb |
                         DCB_dmd_pageability;

    // Call IOS
    ILBService(&Isp);


    // Add the DCB to our DCB list for this DDB.  If it is a psuedo INQUIRY
    // DCB, it will be removed when we handle AEP_DEVICE_INQUIRY. 
    UMSSPDR_AddDcbToList((PUSBDDB)Aep->AEP_d_c_hdr.AEP_ddb, Dcb);

    EXIT(UMSSPDR_AerConfigDcb)
}


VOID
UMSSPDR_AerUnconfigDcb(
    PAEP_dcb_unconfig Aep
    )
/*++

Routine Description:

    Handler for removal of DCBs.

Arguments:

    Aep - AEP_dcb_unconfig structure.  

Return Value:

    NONE
--*/
{
    ENTER(UMSSPDR_AerUnconfigDcb)

    // Remove the DCB from our DCB list if it belongs to us
    UMSSPDR_RemoveDcbFromList((PUSBDDB)Aep->AEP_d_u_hdr.AEP_ddb, (PDCB)Aep->AEP_d_u_dcb);

    EXIT(UMSSPDR_AerUnconfigDcb)
}



VOID
UMSSPDR_AerBootComplete(
    PAEP_boot_done Aep
    )
/*++

Routine Description:

    Boot complete handler.  Determines if port driver should remain in memory.

Arguments:

    Aep - AEP_boot_done structure.  

Return Value:

    NONE
--*/

{
    PUSBDDB UsbDdb;

    UsbDdb = UMSSPDR_GetFirstDdb();

    // Walk the list DDBs, and see if we have any active DCBs attached
    while (NULL != UsbDdb)
    {
        // Check if there are any DCBs attached to this DDB
        if (NULL != UMSSPDR_GetFirstDcb(UsbDdb))
        {
            // Found a DCB, so we stay loaded
            EXIT(UMSSPDR_AerBootComplete);
        }

        UsbDdb = UMSSPDR_GetNextDdb(UsbDdb);
    }

    // Found no DCBs, so we should unload
    Aep->AEP_b_d_hdr.AEP_result = AEP_FAILURE;

    EXIT(UMSSPDR_AerBootComplete);
}        


VOID
UMSSPDR_AerUninit(
    PAEP_bi_uninit Aep
    )
/*++

Routine Description:

    Uninitialize routine.  Cleanup before we get unloaded.

Arguments:

    Aep - AEP_bi_uninit structure.  

Return Value:

    NONE
--*/

{
    ENTER(UMSSPDR_AerUninit);

    // Remove the DDB from our DDB list
    UMSSPDR_RemoveDdbFromList((PUSBDDB)Aep->AEP_bi_u_hdr.AEP_ddb);

    EXIT(UMSSPDR_AerUninit);
}
