////////////////////////////////////////////////////////////////////////////////
// Copyright(C) SigmaTel, Inc. 2003
//
// Filename: ddildl_defs.h
// Description: 
////////////////////////////////////////////////////////////////////////////////

#ifndef _DDILDL_DEFS_H
#define _DDILDL_DEFS_H

///////////////////////////////////////////////////////////////////////////////
// Definitions
///////////////////////////////////////////////////////////////////////////////

#define MAX_MEDIA_TABLE_ENTRIES                 20
#define MAX_DOWNLOAD_SIZE_IN_BYTES	128*512 //(128 sectors of 512 bytes each at a time)

#define NORMAL_MSC      0       // Look for MSC SCSI device in normal mode (hostlink.sb)
#define UPGRADE_MSC     1       // Look for the upgrade MSC device (updater.sb)

#define GENERIC_UPGRADE_MFG_STRING      "_Generic"
#define GENERIC_UPGRADE_PRODUCT_STRING  "MSC Recovery    "

#define JANUS_OK                    0x00
#define JANUS_CORRUPT               0x01

#ifndef ONE_MB
#define ONE_MB	(1024*1024)
#endif

typedef enum {
    NoActiveDeviceMode = 0,
    RecoveryDeviceMode = 1,
    MTPDeviceMode = 2,
    MSCDeviceMode = 3,
    LimitedMSCDeviceMode = 4,
    UpdaterDeviceMode = 5,
	HidDeviceMode = 6
} STMP_DEVICE_MODE, * P_STMP_DEVICE_MODE;

///////////////////////////////////////////////////////////////////////////////
// Typedefs
///////////////////////////////////////////////////////////////////////////////

typedef enum {
    MediaTypeNand = 0,
    MediaTypeMMC = 1,
    MediaTypeHDD = 2,
    MediaTypeRAM = 3,
	MediaTypeiNAND = 4
} PHYSICAL_MEDIA_TYPE, * P_PHYSICAL_MEDIA_TYPE;

typedef enum {
    SerialNoInfoSizeOfSerialNumberInBytes = 0,
    SerialNoInfoSerialNumber = 1
} SERIAL_NO_INFO, * P_SERIAL_NO_INFO;

#define NUM_NAND_MFG_ID_BYTES	5 // 5 bytes returned as NAND Manufacturer ID
typedef struct _STMP_NAND_ID
{
	UCHAR	nandB0;
	UCHAR	nandB1;
	UCHAR	nandB2;
	UCHAR	nandB3;
	UCHAR	nandB4;
	UCHAR	filler[3];
} STMP_NAND_ID, *PSTMP_NAND_ID;

typedef enum {
    MediaInfoNumberOfDrives = 0,
    MediaInfoSizeInBytes = 1,
    MediaInfoAllocationUnitSizeInBytes = 2,
    MediaInfoIsInitialized = 3,
    MediaInfoMediaState = 4,
    MediaInfoIsWriteProtected = 5,
    MediaInfoPhysicalMediaType = 6,
    MediaInfoSizeOfSerialNumberInBytes = 7,
    MediaInfoSerialNumber = 8,
    MediaInfoIsSystemMedia = 9,
    MediaInfoIsMediaPresent = 10,
	MediaInfoNandPageSizeInBytes = 11, // 2048 typical value   2Kbyte page size 
    MediaInfoNandMfgId = 12,     // Cmd only sent if media type is nand. 1 byte like 0xec for samsung.
    MediaInfoNandIdDetails = 13,  // Cmd only sent if media type is nand. Id details for nand
								//include remaining 4 of 5 byte nand HW read id hw cmd.
	MediaInfoNandChipEnables = 14   // num CE discovered at driver init time (up to
									// build option max supported num CE)
} LOGICAL_MEDIA_INFO, * P_LOGICAL_MEDIA_INFO;

typedef enum {
    DriveInfoSectorSizeInBytes = 0,
    DriveInfoEraseSizeInBytes = 1,
    DriveInfoSizeInBytes = 2,
    DriveInfoSizeInMegaBytes = 3,
    DriveInfoSizeInSectors = 4,
    DriveInfoType = 5,
    DriveInfoTag = 6,
    DriveInfoComponentVersion = 7,
    DriveInfoProjectVersion = 8,
    DriveInfoIsWriteProtected = 9,
    DriveInfoSizeOfSerialNumberInBytes = 10,
    DriveInfoSerialNumber = 11,
    DriveInfoMediaPresent = 12,
    DriveInfoMediaChange = 13
} LOGICAL_DRIVE_INFO, * P_LOGICAL_DRIVE_INFO;



#define DRIVE_TAG_STMPSYS_S         0x00
#define DRIVE_TAG_USBMSC_S          0x01
#define DRIVE_TAG_HOSTLINK_S        0x01
#define DRIVE_TAG_RESOURCE_BIN      0x02
#define DRIVE_TAG_EXTRA_S           0x03
#define DRIVE_TAG_RESOURCE1_BIN     0x04
#define DRIVE_TAG_OTG_S             0x05
#define DRIVE_TAG_HOSTRSC_BIN       0x06
#define DRIVE_TAG_MARK_S	        0x06
#define DRIVE_TAG_IRDASYS_S         0x07
#define DRIVE_TAG_SETTINGS_BIN      0x07
#define DRIVE_TAG_OTG_RSC			0x08
#define DRIVE_TAG_DATA              0x0A
#define DRIVE_TAG_DATA_HIDDEN       0x0B
#define DRIVE_TAG_DATA_SETTINGS     0x0C
#define DRIVE_TAG_BOOTMANAGER_S     0x50
#define DRIVE_TAG_UPDATER_NAND_S    0xFE
#define DRIVE_TAG_UPDATER_S         0xFF

// Do not use this enum... use the defs above.  We need to use defs so customers
//  may extend the system drives without DDI source code.
/*
typedef enum {
    ResourceBinDriveTag = 0x00,
    BootManagerDriveTag = 0x50,
    StmpSysDriveTag = 0x01,
    UsbMscDriveTag = 0x02,
    DataDriveTag = 0x0A
} LOGICAL_DRIVE_TAG, * P_LOGICAL_DRIVE_TAG;
*/

typedef enum {
    DriveTypeInvalid = -1,
    DriveTypeData = 0,
    DriveTypeSystem = 1,
    DriveTypeHiddenData = 2,
	DriveTypeNonVolatile = 3,
    DriveTypeUnknown = 4
} LOGICAL_DRIVE_TYPE, * P_LOGICAL_DRIVE_TYPE;

#pragma pack (push, 1)
 
typedef struct {
    UCHAR DriveNumber;
    UCHAR Type;
    UCHAR Tag;
    ULONGLONG SizeInBytes;
} MEDIA_ALLOCATION_TABLE_ENTRY, * P_MEDIA_ALLOCATION_TABLE_ENTRY;

typedef struct {
    USHORT wNumEntries;
    MEDIA_ALLOCATION_TABLE_ENTRY Entry[MAX_MEDIA_TABLE_ENTRIES];
} MEDIA_ALLOCATION_TABLE, * P_MEDIA_ALLOCATION_TABLE;

#pragma pack (pop)

#endif // #ifndef _DDILDL_DEFS_H
