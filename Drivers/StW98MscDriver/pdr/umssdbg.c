/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    Ummsdbg.c 

Abstract:

    IOS port driver for USB LS-120 drive
    Debug functions

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
#include "umssdbg.h"

#pragma VxD_LOCKED_CODE_SEG 
#pragma VxD_LOCKED_DATA_SEG 

#ifdef DEBUG
DWORD UMSSPDR_DebugLevel=DBGLVL_DEFAULT;
#endif
