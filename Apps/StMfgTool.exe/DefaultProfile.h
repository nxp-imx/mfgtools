//
// DefaultProfile.h
//
// Locked default profile definition.  Specify profile specifics here and define
// the LOCKED_DEFAULT_PROFILE constant to use this profile, and lock out any other
// profiles in the Profiles folder.
//
// Firmware files are attached resources.
#include "Libs/DevSupport/StMedia.h"

//#define RESTRICTED_PC_IDS TRUE
//#define LOCKED_DEFAULT_PROFILE TRUE

#ifdef LOCKED_DEFAULT_PROFILE
#define DEFAULT_PROFILE_NAME			_T("(LOCKED)SDK Firmware Updater")
#define DEFAULT_PROFILE_VID				_T("0x066F")
#define DEFAULT_PROFILE_PID				_T("0xA010")
#define DEFAULT_PROFILE_SCSI_MFG		_T("SigmaTel")
#define DEFAULT_PROFILE_SCSI_PROD		_T("SDK Device")
#define DEFAULT_PROFILE_LABEL			_T("");
#define DEFAULT_PROFILE_USE_LABEL		FALSE
#define DEFAULT_PROFILE_UPDATE_OP		_T("UPDATE=Update-SDK5.00(no .rsc),120,1") // from profile.ini
#define DEFAULT_PROFILE_PARAM			UL_PARAM_USE_STATIC_FW
#define DEFAULT_PROFILE_UPDATE_FW		_T("updater.sb")
#define  DEFAULT_PROFILE_COPY_OP		_T("")                      // uncomment for no copy operation
//#define DEFAULT_PROFILE_COPY_OP			_T("COPY=Copy-media,120,1") // from profile.ini

#define DEFAULT_PROFILE_DRV1_FILE				_T("")
#define DEFAULT_PROFILE_DRV1_DESC				_T("Data")
#define DEFAULT_PROFILE_DRV1_TYPE				media::DriveType_Data
#define DEFAULT_PROFILE_DRV1_TAG				media::DriveTag_Data
#define DEFAULT_PROFILE_DRV1_FLAGS				media::DriveFlag_Format
#define DEFAULT_PROFILE_DRV1_REQKB				0

#define DEFAULT_PROFILE_DRV2_FILE				_T("")
#define DEFAULT_PROFILE_DRV2_DESC				_T("Janus Drive")
#define DEFAULT_PROFILE_DRV2_TYPE				media::DriveType_HiddenData
#define DEFAULT_PROFILE_DRV2_TAG				media::DriveTag_DataJanus
#define DEFAULT_PROFILE_DRV2_FLAGS				media::DriveFlag_JanusInit
#define DEFAULT_PROFILE_DRV2_REQKB				0

#define DEFAULT_PROFILE_DRV3_FILE				_T("persist.bin")
#define DEFAULT_PROFILE_DRV3_DESC				_T("Hidden 2")
#define DEFAULT_PROFILE_DRV3_TYPE				media::DriveType_HiddenData
#define DEFAULT_PROFILE_DRV3_TAG				media::DriveTag_DataSettings
#define DEFAULT_PROFILE_DRV3_FLAGS				media::DriveFlag_ImageData
#define DEFAULT_PROFILE_DRV3_REQKB				0

#define DEFAULT_PROFILE_DRV4_FILE				_T("firmware.sb")
#define DEFAULT_PROFILE_DRV4_DESC				_T("Firmware Img 1")
#define DEFAULT_PROFILE_DRV4_TYPE				media::DriveType_System
#define DEFAULT_PROFILE_DRV4_TAG				media::DriveTag_FirmwareImg
#define DEFAULT_PROFILE_DRV4_FLAGS				media::DriveFlag_ImageDataRsc
#define DEFAULT_PROFILE_DRV4_REQKB				0

#define DEFAULT_PROFILE_DRV5_FILE				_T("firmware.sb")
#define DEFAULT_PROFILE_DRV5_DESC				_T("Firmware Img 2")
#define DEFAULT_PROFILE_DRV5_TYPE				1
#define DEFAULT_PROFILE_DRV5_TAG				media::DriveTag_FirmwareImg2
#define DEFAULT_PROFILE_DRV5_FLAGS				media::DriveFlag_ImageDataRsc
#define DEFAULT_PROFILE_DRV5_REQKB				0

#define DEFAULT_PROFILE_DRV6_FILE				_T("firmware.sb")
#define DEFAULT_PROFILE_DRV6_DESC				_T("Firmware Img 3")
#define DEFAULT_PROFILE_DRV6_TYPE				media::DriveType_System
#define DEFAULT_PROFILE_DRV6_TAG				media::DriveTag_FirmwareImg3
#define DEFAULT_PROFILE_DRV6_FLAGS				media::DriveFlag_ImageDataRsc
#define DEFAULT_PROFILE_DRV6_REQKB				0
/*
#define DEFAULT_PROFILE_DRV7_FILE				_T("firmware.rsc")
#define DEFAULT_PROFILE_DRV7_DESC				_T("Firmware Rsc 1")
#define DEFAULT_PROFILE_DRV7_TYPE				media::DriveType_System
#define DEFAULT_PROFILE_DRV7_TAG				media::DriveTag_PlayerRsc
#define DEFAULT_PROFILE_DRV7_FLAGS				media::DriveFlag_ImageDataRsc
#define DEFAULT_PROFILE_DRV7_REQKB				0

#define DEFAULT_PROFILE_DRV8_FILE				_T("firmware.rsc")
#define DEFAULT_PROFILE_DRV8_DESC				_T("Firmware Rsc 2")
#define DEFAULT_PROFILE_DRV8_TYPE				media::DriveType_System
#define DEFAULT_PROFILE_DRV8_TAG				media::DriveTag_PlayerRsc2
#define DEFAULT_PROFILE_DRV8_FLAGS				media::DriveFlag_ImageDataRsc
#define DEFAULT_PROFILE_DRV8_REQKB				0

#define DEFAULT_PROFILE_DRV9_FILE				_T("firmware.rsc")
#define DEFAULT_PROFILE_DRV9_DESC				_T("Firmware Rsc 3")
#define DEFAULT_PROFILE_DRV9_TYPE				media::DriveType_System
#define DEFAULT_PROFILE_DRV9_TAG				media::DriveTag_PlayerRsc3
#define DEFAULT_PROFILE_DRV9_FLAGS				media::DriveFlag_ImageDataRsc
#define DEFAULT_PROFILE_DRV9_REQKB				0
*/

#endif
