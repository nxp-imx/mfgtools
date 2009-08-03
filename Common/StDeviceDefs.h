#pragma once

#define UsbVendorId         ""
#define UsbPrdctId          ""
#define ScsiMfgStr          ""
#define ScsiPrdctStr        ""
#define MtpVendorStr        ""
#define MtpPrdctStr         ""
#define ScsiUpdaterMfgStr   "_Generic"
#define ScsiUpdaterPrdctStr "MSC Recovery    "
#define UsbUpdaterVendorId  "066F"
#define UsbUpdaterPrdctId   "A000"
#define ScsiVolumes         ""
#define DriveArrayStr                                   \
 "              ,                 , 0, 0x0A, 0, 0,"     \
 "bootmanager.sb, Boot Manager    , 1, 0x50, 1, 0,"     \
 "usbmsc.sb     , USB Mass Storage, 1, 0x01, 1, 0,"     \
 "stmpsys.sb    , Player          , 1, 0x00, 1, 0,"     \
 "resource.bin  , Resource Manager, 1, 0x02, 1, 0"
