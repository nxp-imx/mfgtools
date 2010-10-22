/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    UMSSGUID.h

Abstract:

 The below GUID is used to generate symbolic links to
  driver instances created from user mode

Environment:

    Kernel & user mode

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1997-1998 Microsoft Corporation.  All Rights Reserved.

Revision History:

    11/18/97 : created

--*/
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#ifndef UMSSGUID_INC
#define UMSSGUID_INC

#include <initguid.h>

// {1181f4a0-f284-11d2-9068-00609797ea5a} for bus GUID
DEFINE_GUID(GUID_BUS_UMSS, 
0x1181f4a0, 0xf284, 0x11d2, 0x90, 0x68, 0x0, 0x60, 0x97, 0x97, 0xea, 0x5a);

#endif // end, #ifndef UMSSGUID_INC
