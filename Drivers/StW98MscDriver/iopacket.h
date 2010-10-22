/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    iopacket.h

Abstract:

    header file shared by USBSTOR.PDR and USBLS120.SYS

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
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

// Structure for passing I/O requests between port driver and
// WDM USB driver

#pragma pack(1)

typedef struct _IOPACKET
{
        PVOID   Fdo;
        PVOID   Iop;
        UCHAR*  Cdb;
        UCHAR   CdbLength;
        PVOID   DataBuffer;
        ULONG   DataLength;
        ULONG   BlockSize;
        ULONG   Flags;
        ULONG   Status;
        CHAR    Lun;
} IOPACKET, *PIOPACKET;

#pragma pack()

// Status values
#define IO_STATUS_SUCCESS 0
#define IO_STATUS_PENDING 1
#define IO_STATUS_DEVICE_ERROR 2
#define IO_STATUS_OUT_OF_MEMORY 3
#define IO_STATUS_DATA_OVERRUN 4

//Flags values
#define IO_FLAGS_DATA_IN 1
#define IO_FLAGS_DATA_OUT 2
#define IO_FLAGS_SCATTER_GATHER 4




