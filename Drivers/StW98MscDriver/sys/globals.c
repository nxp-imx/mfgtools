/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    globals.c

Abstract:

        global variable declarations

Environment:

    Kernel mode

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.

Revision History:

    01/13/99: MRB  Adapted from BULKUSB DDK sample.

--*/


#include "wdm.h"


//Global variable for storing our Driver Object
PDRIVER_OBJECT  UMSSDriverObject = NULL;
