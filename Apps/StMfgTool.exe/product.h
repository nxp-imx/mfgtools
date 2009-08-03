#ifndef STMP3_INSTALL_SHIELD 
#pragma once
#endif

#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE
	#define TRUE 1
#endif

// must be equal to the firmware
#define CFG_SCSI_MFG					"_Generic"
#define CFG_SCSI_PRODUCT				"MSCN            "


//#define CFG_VENDOR_ICON					"../../customer/SigmaTel MSCN Audio Player/vendor.ico"
//#define CFG_UPDATE_ICON					"..\\..\\resources\\general.ico"
//#define CFG_SEARCH_AVI					"..\\..\\resources\\stsearch.avi"

#define CFG_DEF_FORMAT_DATA_AREA		FALSE	
#define CFG_DEF_VOLUME_LABEL			L""

// used in regdll to clean registry
// used in updater to refresh device
#define CFG_USB_VENDOR_ID				0x066F
#define CFG_USB_PRODUCT_ID				0x8000
// used in setup files to clean registry
#define CFG_USBMSC_CLASS_ID				"VID_066F&PID_8000"

#define CFG_INSTALL_UPDATER				TRUE
#define CFG_INSTALL_UNINSTALL			TRUE

#define CFG_UPDATER_NAME_L				L"StUpdaterApp.exe"
#define CFG_EXTERNAL_MEDIA              "NONE"
#define CFG_ALWAYS_REBOOT               FALSE
#define CFG_ALWAYS_FORCE_RECOVERY       FALSE

#ifndef STMP3_INSTALL_SHIELD //typedef and enum declarations do not work with installshield.
// these defines are only used by the updater application.
/*
typedef struct _DRIVE_DESC {
   CStdString	chName[MAX_PATH];
   CStdString	wchDescription[MAX_PATH];
   UCHAR	uchType;					// Minimum: 1 for data drive, 3 for system drives, 0 for non-volatile
   UCHAR	uchTag;
   BOOLEAN	bEncrypted;
   ULONG	AdditionalMemory;
} DRIVE_DESC, *PDRIVE_DESC;

#define CFG_NUM_DRIVES					5	//  (Min 4, Max 10)
DRIVE_DESC g_arr_drives[CFG_NUM_DRIVES] = {
//  { firmware             firmware                type (data,          Tag                  Encrypted Additional
//	  filename			  description          system, nonvolatile)                                      Memory
//                                                                                                      in bytes      
	{"",                L"",                    DriveTypeData,      DRIVE_TAG_DATA,             FALSE,     0},
	{"bootmanager.sb",  L"Boot Manager",        DriveTypeSystem,    DRIVE_TAG_BOOTMANAGER_S,    TRUE,      0},
	{"usbmsc.sb",       L"USB Mass Storage",    DriveTypeSystem,    DRIVE_TAG_USBMSC_S,         TRUE,      0},
	{"stmpsys.sb",      L"Player",              DriveTypeSystem,    DRIVE_TAG_STMPSYS_S,        TRUE,      0},
	{"resource.bin",    L"Resource Manager",    DriveTypeSystem,    DRIVE_TAG_RESOURCE_BIN,     TRUE,      0}
};

#define CFG_BOOTMGR_INDEX				1		// default is 1
#define CFG_USBMSC_INDEX				2		// default is 2
#define CFG_PLAYER_INDEX				3		// default is 3
*/
#endif //STMP3_INSTALL_SHIELD

//#define EXISTING_SCSI_PLAYER   FALSE
//#define EXISTING_USBMSC23_PLAYER TRUE
//#if ( EXISTING_USBMSC23_PLAYER == TRUE ) || ( EXISTING_SCSI_PLAYER == TRUE )
// For removing existing player
// used by componentevents.rul.RemoveOldPlayer()
// used by removescsi.rul.RemoveScsiPlayer()
//#define TARGETNAMEFORPROD				"mscn"

// used by regdll.dll.RemoveDeviceForDeviceType()
//#ifdef DEFINE_GUID
//DEFINE_GUID(CLSID_DEVICECLASS_USB, 0x57586F82, 0x00B6, 0x40b5, 0xB5, 0xE8, 0x93, 0x31, 0xF3, 0x2A, 0x86, 0x4B );
//DEFINE_GUID(CLSID_DEVICECLASS_USBMSC, 0x5C0EB8B2, 0xC111, 0x4021, 0x87, 0x54, 0x96, 0x86, 0xBF, 0x2F, 0xAB, 0xB6 );
//#endif

// used in componentevents.rul.RemoveOldPlayer()
// used by removescsi.rul.RemoveScsiPlayer()
// used in regdll.cpp.CheckServiceName()
//#define GUID_MP3 "{57586F82-00B6-40b5-B5E8-9331F32A864B}"
// used in componentevents.rul.RemoveOldPlayer()
// used in regdll.cpp.CheckServiceName()
//#define GUID_USBMSCR "{5C0EB8B2-C111-4021-8754-9686BF2FABB6}"
// used in regdll.cpp.RemovePlayerRemnants()
//#define GUID_HOOK "{262e6512-1611-4d54-b6f5-58a6719b31ec}"
// used in componentevents.rul.RemoveOldPlayer()
//#define CFG_OLD_DCC_CLASS_ID "VID_066F&PID_8001"
// used in componentevents.rul.RemoveOldPlayer() to remove Enum\Model key in win98
//#define CFG_OLD_CFG_MODEL "MSCN"

//#endif // ( EXISTING_USBMSC23_PLAYER == TRUE ) || ( EXISTING_SCSI_PLAYER == TRUE )
