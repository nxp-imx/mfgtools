/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    UMSSdbg.c 

Abstract:

     Debug output logic .
     This entire module is a noop in the free build

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    1/13/99: MRB	Adapted from the BULKUSB DDK sample.

--*/


#if DBG


#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"
#include "usbdlib.h"
#include "umss.h"


// begin, data/code  used only in DBG build

//  may be overridden  in registry in DBG buils only
// higher == more verbose, default is 1, 0 turns off all
int UMSS_DebugLevel = DBGLVL_MEDIUM ; 

// count outstanding allocations via ExAllocatePool
int gExAllocCount = 0;

UMSS_DBGDATA gDbgBuf = { 0, 0, 0, 0 }; 

// ptr to global debug data struct; txt buffer is only allocated in DBG builds
PUMSS_DBGDATA gpDbg = &gDbgBuf; 


BOOLEAN
UMSS_GetRegistryDword(
    IN      PWCHAR    RegPath,
    IN      PWCHAR    ValueName,
    IN OUT  PULONG    Value
    )

/*++

Routine Description:

    Obtain a Dword value from the registry


Arguments:

    RegPath  -- supplies absolute registry path
    ValueName    - Supplies the Value Name.
    Value      - receives the REG_DWORD value.

Return Value:

    TRUE if successfull, FALSE on fail.

--*/

{
    UNICODE_STRING path;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];  //zero'd second table terminates parms
    ULONG lDef = *Value;                     // default
    NTSTATUS status;
    BOOLEAN fres;
    WCHAR wbuf[ MAXIMUM_FILENAME_LENGTH ];

    UMSS_KdPrint( DBGLVL_HIGH,("Enter UMSS_GetRegistryDword() RegPath = %ws\n   ValueName =%ws\n", RegPath, ValueName));
    path.Length = 0;
    path.MaximumLength = MAXIMUM_FILENAME_LENGTH * sizeof( WCHAR );  // MAXIMUM_FILENAME_LENGTH defined in wdm.h
    path.Buffer = wbuf;


    RtlZeroMemory(path.Buffer, path.MaximumLength);
    RtlMoveMemory(path.Buffer, RegPath, wcslen( RegPath) * sizeof( WCHAR ));

    UMSS_KdPrint( DBGLVL_HIGH,("UMSS_GetRegistryDword() path= %ws\n", path.Buffer ));

    RtlZeroMemory(paramTable, sizeof(paramTable));

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;

    paramTable[0].Name = ValueName;

    paramTable[0].EntryContext = Value;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &lDef;
    paramTable[0].DefaultLength = sizeof(ULONG);


    status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                    path.Buffer, paramTable, NULL, NULL);

    if (NT_SUCCESS(status))
    {
        UMSS_KdPrint( DBGLVL_MEDIUM,("Exit UMSS_GetRegistryDWord() SUCCESS, value = decimal %d 0x%x\n", *Value, *Value));
        fres = TRUE;
    }
    else
    {
        UMSS_KdPrintCond( DBGLVL_MEDIUM, (status == STATUS_INVALID_PARAMETER) ,("UMSS_GetRegistryDWord() STATUS_INVALID_PARAMETER\n"));
        UMSS_KdPrintCond( DBGLVL_MEDIUM, (status == STATUS_OBJECT_NAME_NOT_FOUND) ,("UMSS_GetRegistryDWord() STATUS_OBJECT_NAME_NOT_FOUND\n"));

        fres = FALSE;
    }

    return fres;
}

PVOID 
UMSS_ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    )
{
    gExAllocCount++;

    UMSS_KdPrint( DBGLVL_HIGH,("UMSS_ExAllocatePool() gExAllocCount = dec %d\n", gExAllocCount ));

    return ExAllocatePool(  PoolType, NumberOfBytes );
}


VOID 
UMSS_ExFreePool(
    IN PVOID p
    )
{
    gExAllocCount--;

    UMSS_KdPrint( DBGLVL_HIGH,("UMSS_ExFreePool() gExAllocCount = dec %d\n", gExAllocCount ));

    ExFreePool(  p );
}


#endif // end , if DBG

