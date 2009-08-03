/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    UMSSPDR.C 

Abstract:

    IOS port driver for USB Mass Storage Sample Driver
    Main module

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    03/19/99: MRB  Original

--*/

#define WANTVXDWRAPS

#include <basedef.h>
#include <vmm.h>
#include <debug.h>
#include <vxdwraps.h>
#include <winerror.h>
#include <dcb.h>
#include <drp.h>
#include <ilb.h>
#include <aep.h>
#include <ios.h>
#include <ior.h>
#include <iop.h>
#include <vxdldr.h>
#include "umssdbg.h"
#include "UMSSPDR.h"


#pragma VxD_LOCKED_DATA_SEG

// Structure containing entrypoints for IOS services
ILB UMSSPDR_Ilb;


// Pointers to exported functions in UMSS.SYS
UMSSPDR_GETNEXTPDO pfnGetNextPdo;
UMSSPDR_STARTREQUEST pfnStartRequest;
UMSSPDR_REGISTERHANDLER pfnRegisterCompletionHandler;
UMSSPDR_GETMAXLUN pfnGetMaxLun;


#pragma VxD_IDATA_SEG

// IOS driver registration packet for this port driver
DRP Drv_Reg_Pkt = {
		    "XXXXXXXX",
		    DRP_MISC_PD,
                    UMSSPDR_AsyncEventHandler,
                    &UMSSPDR_Ilb,
                    UMSSPDRName,
                    UMSSPDRRev,
                    UMSSPDRFeature,
		    DRP_IF_STD,
		    DRP_BT_SCSI,
		    0,
		    0,
		    "00",
		    0
		    };


#pragma VxD_ICODE_SEG
#pragma VxD_IDATA_SEG

DWORD _stdcall
UMSSPDR_Dynamic_Init(
    void
    )
/*++

Routine Description:

    VxD dynamic initialization routine, called when the port driver
    is dynamically loaded by IOS.  This function lies in an init-only
    code segment, and will be discarded after initialization
    completes.

Arguments:

    None

Return Value:

    VXD_SUCCESS if successful,
    VXD_FAILURE otherwise

--*/
{
    HPEMODULE hWdmMod;
	
    ENTER(UMSSPDR_Dynamic_Init)

    // Get module handle for UMSS.SYS WDM/USB driver
    hWdmMod = _PELDR_GetModuleHandle("STUMS.SYS");

    // Fail if UMSS.SYS is not loaded
    if (!hWdmMod)
    {
        UMSSPDR_DebugPrintf(DBGLVL_DEFAULT, ("Unable to find STUMS.SYS!!\n\r"));
        RETURN(VXD_FAILURE, UMSSPDR_Dynamic_Init)
    }

    // Get the addresses of the functions exported by UMSSPDR.SYS
    pfnGetNextPdo = (UMSSPDR_GETNEXTPDO) _PELDR_GetProcAddress(hWdmMod, "UMSS_GetNextPDO",0);
    pfnRegisterCompletionHandler = (UMSSPDR_REGISTERHANDLER) _PELDR_GetProcAddress(hWdmMod, "UMSS_RegisterCompletionHandler",0);
    pfnStartRequest = (UMSSPDR_STARTREQUEST) _PELDR_GetProcAddress(hWdmMod, "UMSS_StartRequest",0);
    pfnGetMaxLun = (UMSSPDR_GETMAXLUN) _PELDR_GetProcAddress(hWdmMod, "UMSS_GetMaxLun",0);

    // Fail if we can't find the exported functions
    if (!pfnGetNextPdo || !pfnRegisterCompletionHandler || !pfnStartRequest || !pfnGetMaxLun)
    {
        UMSSPDR_DebugPrintf(DBGLVL_DEFAULT, ("Unable to find entrypoints in STUMS.SYS\n\r"));
        RETURN(VXD_FAILURE, UMSSPDR_Dynamic_Init)
    }

    // Register our driver with IOS
    IosRegister(&Drv_Reg_Pkt);


    RETURN(VXD_SUCCESS, UMSSPDR_Dynamic_Init)
}


#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

DWORD _stdcall
UMSSPDR_Dynamic_Exit(
    void
    )
/*++

Routine Description:

    VxD dynamic exit routine

Arguments:

    None

Return Value:

    VXD_SUCCESS if successful,
    VXD_FAILURE otherwise

--*/

{
    ENTER(UMSSPDR_Dynamic_Exit)

    RETURN(VXD_SUCCESS, UMSSPDR_Dynamic_Exit)
}


