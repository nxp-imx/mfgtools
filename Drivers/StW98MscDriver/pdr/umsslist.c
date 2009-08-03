/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    umsslist.c 

Abstract:

    IOS port driver for USB Mass Storage Sample driver
    DCB linked list management functions

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    10/05/00: MRB  Added linked-list functions

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

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

PUSBDDB UmssDdbPtr = NULL;


BOOL
UMSSPDR_AddDcbToList(
    PUSBDDB Ddb,
    PDCB Dcb
    )
/*++

Routine Description:

    Adds a DCB to a DDB's DCB list

Arguments:

    Ddb - DDB that the DCB list is attached to.
    Dcb - DCB to be added to the list.

Return Value:

    TRUE if successful
    FALSE if unsuccessful

--*/

{
    BYTE        NewLun;

    ENTER(UMSSPDR_AddDcbToList)

    NewLun = Dcb->DCB_scsi_lun;

    // Check if we are past the maximum LUN value for this device
    if (NewLun > Ddb->MaxLun)
    {
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("Configured DCB LUN greater than MaxLun!!\n"));
        Trap();
        RETURN(FALSE, UMSSPDR_AddDcbToList)
    }

    // Make sure the slot for this LUN is not already filled.
    if (NULL != Ddb->Dcb[NewLun])
    {
        // This is likely an INQUIRY DCB, or a re-enumeration of an existing
        // DCB.  We should not overwrite an existing DCB in this case.
        RETURN(FALSE, UMSSPDR_AddDcbToList)
    }

    // Store DCB in the DDB
    Ddb->Dcb[NewLun] = Dcb;

    RETURN(TRUE, UMSSPDR_AddDcbToList)
}


BOOL
UMSSPDR_RemoveDcbFromList(
    PUSBDDB Ddb,
    PDCB Dcb
    )
/*++

Routine Description:

    Removes a DCB to a DDB's DCB list

Arguments:

    Ddb - DDB that the DCB list is attached to.
    Dcb - DCB to be removed from the list.

Return Value:

    TRUE if successful
    FALSE if unsuccessful

--*/

{
    BYTE OldLun;

    ENTER(UMSSPDR_RemoveDcbFromList)

    OldLun = Dcb->DCB_scsi_lun;

    // Verify that DCB is in the correct slot
    if (Dcb != Ddb->Dcb[OldLun])
    {
        // This can occur if we are trying to remove an INQUIRY DCB that
        // was already removed, or was never added in the first place.
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("DCB not found\n"));
        RETURN(FALSE, UMSSPDR_RemoveDcbFromList)
    }
    else if (OldLun > Ddb->MaxLun)
    {
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("DCB to be removed has invalid LUN\n"));
        RETURN(FALSE, UMSSPDR_RemoveDcbFromList)
    }

    // Remove LUN from DDB list
    Ddb->Dcb[OldLun] = NULL;

    RETURN(TRUE, UMSSPDR_RemoveDcbFromList)
}



PDCB
UMSSPDR_GetFirstDcb(
    PUSBDDB Ddb
    )
/*++

Routine Description:

    Returns the first DCB node in a DDB's DCB list

Arguments:

    Ddb - DDB that the DCB list is attached to.

Return Value:

    Pointer to the first PDCB in the DDB's list, or NULL
1
--*/

{
    BYTE Lun=0;

    ENTER(UMSSPDR_GetFirstDcb)

    //Get the first DCB node in the DDB's list
    while (Lun <= Ddb->MaxLun)
    {
        if (Ddb->Dcb[Lun])
            RETURN(Ddb->Dcb[Lun], UMSSPDR_GetFirstDcb)
        Lun++;
    }

    RETURN(NULL, UMSSPDR_GetFirstDcb)
}



PDCB
UMSSPDR_GetNextDcb(
    PUSBDDB Ddb,
    PDCB PreviousDcb
    )
/*++

Routine Description:

    Returns the next DCB node in the DDB's DCB list

Arguments:

    Ddb - DDB that the DCB list is attached to.
    PreviousDcb - Previous DCB.

Return Value:

    Next DCB node in list, or NULL.

--*/

{
    BYTE PrevLun;

    ENTER(UMSSPDR_GetNextDcb)

    PrevLun = PreviousDcb->DCB_scsi_lun;
    PrevLun++;

    while (PrevLun <= Ddb->MaxLun)
    {
        if (Ddb->Dcb[PrevLun])
            RETURN(Ddb->Dcb[PrevLun], UMSSPDR_GetNextDcb)
        PrevLun++;
    }

    RETURN(NULL, UMSSPDR_GetNextNode)
}



VOID
UMSSPDR_AddDdbToList(
    PUSBDDB NewDdb
    )
/*++

Routine Description:

    Adds a DDB to our linked-list of DDBs controlled by this port driver

Arguments:

    NewDdb - DDB to be added to the list.

Return Value:

    None
--*/
{
    PUSBDDB CurrDdbPtr;

    ENTER(UMSSPDR_AddDdbToList)

    if (NULL == UmssDdbPtr)
    {
        UmssDdbPtr = NewDdb;
    }
    else
    {
        CurrDdbPtr = UmssDdbPtr;

        while (CurrDdbPtr->NextDdb != NULL)
            CurrDdbPtr = CurrDdbPtr->NextDdb;

        CurrDdbPtr->NextDdb = NewDdb;
        NewDdb->NextDdb = NULL;
    }
    EXIT(UMSSPDR_AddDdbToList)
}


BOOL
UMSSPDR_RemoveDdbFromList(
    PUSBDDB OldDdb
    )
/*++

Routine Description:

    Removes a DDB to our linked-list of DDBs controlled by this port driver

Arguments:

    OldDdb - DDB to be from the list.

Return Value:

    TRUE if DDB found and removed from list.
    FALSE if DDB not found in list.

--*/
{
    PUSBDDB CurrDdb;

    ENTER(UMSSPDR_RemoveDdbFromList)

    if (NULL == UmssDdbPtr)
        RETURN(FALSE, UMSSPDR_RemoveDdbFromList)

    if (OldDdb == UmssDdbPtr)
    {
        UmssDdbPtr = OldDdb->NextDdb;
        RETURN(TRUE, UMSSPDR_RemoveDdbFromList)
    }

    CurrDdb = UmssDdbPtr;

    while (CurrDdb->NextDdb != OldDdb)
    {
        if (NULL == CurrDdb->NextDdb)
            RETURN(FALSE, UMSSPDR_RemoveDdbFromList)

        CurrDdb = CurrDdb->NextDdb;
    }

    CurrDdb->NextDdb = OldDdb->NextDdb;

    RETURN(TRUE, UMSSPDR_RemoveDdbFromList)
}

PUSBDDB
UMSSPDR_GetFirstDdb(
    )
/*++

Routine Description:

    Retrieves first DDB from our DDB list

Arguments:

    NONE

Return Value:

    Returns the head of the DDB list, or NULL

--*/
{
    ENTER(UMSSPDR_GetFirstDdb)

    RETURN(UmssDdbPtr, UMSSPDR_GetFirstDdb)
}

PUSBDDB
UMSSPDR_GetNextDdb(
    PUSBDDB Ddb
    )
/*++

Routine Description:

    Retrieves next DDB from our DDB list

Arguments:

    NONE

Return Value:

    Returns a pointer to the next DDB in the DDB list, or NULL

--*/
{
    ENTER(UMSSPDR_GetNextDdb)
    
    if (NULL == Ddb)
        RETURN(NULL, UMSSPDR_GetNextDdb)

    RETURN(Ddb->NextDdb, UMSSPDR_GetNextDdb)
}
