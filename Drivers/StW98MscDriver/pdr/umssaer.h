/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    UMSSAER.h

Abstract:

    IOS port driver for USB Mass Storage Sample driver

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    10/06/00: MRB  Added string literals for AEP events

--*/

#ifdef DEBUG

static const PCHAR szAepFuncDesc[] =
{
    "AEP_INITIALIZE",
    "AEP_SYSTEM_CRIT_SHUTDOWN",
    "AEP_BOOT_COMPLETE",
    "AEP_CONFIG_DCB",
    "AEP_UNCONFIG_DCB",
    "AEP_IOP_TIMEOUT",
    "AEP_DEVICE_INQUIRY",
    "AEP_HALF_SEC",
    "AEP_1_SEC",
    "AEP_2_SECS",
    "AEP_4_SECS",
    "AEP_DBG_DOT_CMD",
    "AEP_ASSOCIATE_DCB",
    "AEP_REAL_MODE_HANDOFF",
    "AEP_SYSTEM_SHUTDOWN",
    "AEP_UNINITIALIZE",
    "AEP_DCB_LOCK",
    "AEP_MOUNT_NOTIFY",     
    "AEP_CREATE_VRP",       
    "AEP_DESTROY_VRP",      
    "AEP_REFRESH_DRIVE",    
    "AEP_PEND_UNCONFIG_DCB",
    "AEP_1E_VEC_UPDATE",
    "AEP_CHANGE_RPM",    
    "AEP_QUERY_RPM", 
    "AEP_CONFIG_3MODE",
    "AEP_SYSTEM_REBOOT",
    "AEP_QUERY_CD_SPINDOWN",
    "AEP_FUNC_POWER_MESSAGE",
    "AEP_FUNC_ACPI_REQUEST"
};


#define DBG_AEP_NAME(Aep) \
    if (Aep->AEP_func <= AEP_MAX_FUNC) \
        UMSSPDR_DebugPrintf(DBGLVL_MINIMUM, ("%s\n\r", szAepFuncDesc[Aep->AEP_func]));

#else

#define DBG_AEP_NAME(Aep)

#endif



