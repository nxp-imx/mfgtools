/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.
--*/

/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

//---------------------------------------------------------------------------
//
// Module Name:
//       public.h
//
// Abstract:
//       This file contains definitions which are available to applications.
//
// Environment:
//       User & Kernel mode
//
//---------------------------------------------------------------------------

#ifndef _USER_H
#define _USER_H

#include <initguid.h>

// {00873FDF-61A8-11D1-AA5E-00C04FB1728B}
DEFINE_GUID(GUID_CLASS_IMX_USB, 0x00873FDF, 0x61A8, 0x11D1, 0xAA, 0x5E, 0x00, 0xC0, 0x4F, 0xB1, 0x72, 0x8B);

#define IOCTL_INDEX             0x0000


#define IOCTL_IMXDEVICE_GET_DEVICE_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX,     \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_IMXDEVICE_GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX + 1,     \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)
                                                   
#define IOCTL_IMXDEVICE_RESET_DEVICE          CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX + 2, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_IMXDEVICE_RESET_PIPE            CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX + 3, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#endif

