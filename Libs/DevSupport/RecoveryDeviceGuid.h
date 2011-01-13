/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#ifndef _RECOVERY_GUID_H
#define _RECOVERY_GUID_H

#include <initguid.h>
// Device Interface GUID for the USB Bulk Recovery Driver
// Used by StMp3Rec.sys and StUpdaterApp.exe
// {A441A6E1-EC62-46FB-9989-2CD78F1AAA34}
DEFINE_GUID(GUID_DEVINTERFACE_STMP3XXX_USB_BULK_DEVICE,
0xa441a6e1, 0xec62, 0x46fb, 0x99, 0x89, 0x2c, 0xd7, 0x8f, 0x1a, 0xaa, 0x34);

// Device Class GUID for the USB Bulk Recovery Driver
// Used in StMp3Rec.inf
// {9FFF066D-3ED3-4567-9123-8B82CFE1CDD4}
DEFINE_GUID(GUID_CLASS_STMP3XXX_USB_BULK_DEVICE,
0x9FFF066D, 0x3ED3, 0x4567, 0x91, 0x23, 0x8B, 0x82, 0xCF, 0xE1, 0xCD, 0xD4);

#endif
