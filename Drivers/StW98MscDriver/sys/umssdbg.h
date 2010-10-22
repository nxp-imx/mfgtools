/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    UMSSDbg.h

Abstract:

	Debug macros 

Environment:

    Kernel mode

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.

Revision History:

    01/13/99: MRB  Adapted from BULKUSB DDK sample.

--*/
/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#ifndef UMSSDBG_INCD
#define UMSSDBG_INCD


// registry path used for parameters global to all instances of the driver
// In original sampe, this was a DBG only definition within umssdbg.h, but we create it at installation.
#define UMSS_REGISTRY_PARAMETERS_PATH  \
        L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\SERVICES\\STUMS\\Parameters"

#if DBG

// Defines for debug verbosity level,
//   i.e., test UMSS_KdPrintLevel( n, format....) against these values
//  Example:
//
// UMSS_KdPrint( DBGLVL_MEDIUM ,("Entering DriverEntry(), RegistryPath=\n    %ws\n", RegistryPath->Buffer ));
// 
//  The above will print iff UMSS_DebugLevel >= DBGLVL_MEDIUM
//
#define DBGLVL_OFF                      0               // if UMSS_DebugLevel set to this, there is NO debug output 
#define DBGLVL_MINIMUM			1		// minimum verbosity	
#define DBGLVL_DEFAULT			2		// default verbosity level if no registry override
#define DBGLVL_MEDIUM			3		// medium verbosity
#define DBGLVL_HIGH                     4               // highest 'safe' level (without severely affecting timing )
#define DBGLVL_MAXIMUM			5		// maximum level, may be dangerous


#define ENTER(FunctionName) UMSS_KdPrint(DBGLVL_HIGH, ("-> %s\n", #FunctionName));
#define EXIT(FunctionName) UMSS_KdPrint(DBGLVL_HIGH, ("<- %s\n", #FunctionName));
#define RETURN(RetVal, FunctionName) \
    UMSS_KdPrint(DBGLVL_HIGH, ("<- %s, ret = %x\n", #FunctionName, RetVal)); \
    return (RetVal);

#define DBG_STATUS(DeviceExtension)     UMSS_KdPrint(DBGLVL_HIGH, \
        ("Urb Status=%x, Irp Status=%x\n", \
        DeviceExtension->Urb->UrbHeader.Status, \
        DeviceExtension->Irp->IoStatus.Status));


#ifndef DBGSTR_PREFIX
#define DBGSTR_PREFIX "STUMS: " 
#endif





#define DPRINT DbgPrint

#define TRAP() DbgBreakPoint();


#define UMSS_DBGOUTSIZE              512


typedef struct _UMSS_DBGDATA {

	// mirrors device extension pending io count
	ULONG PendingIoCount;

	// count of pipe errors detected during the life of this device instance
	ULONG PipeErrorCount;

	// count of pipe resets performed during the life of this device instance
	ULONG ResetPipeCount;

	// count of pipe resets performed during the life of this device instance
	ULONG AbortPipeCount;

} UMSS_DBGDATA, *PUMSS_DBGDATA;

//these declared in debug 'c' file
extern int UMSS_DebugLevel; 
extern int gExAllocCount;
extern PUMSS_DBGDATA gpDbg; 


static const PCHAR szIrpMajFuncDesc[] =
{  // note this depends on corresponding values to the indexes in wdm.h
   "IRP_MJ_CREATE",
   "IRP_MJ_CREATE_NAMED_PIPE",
   "IRP_MJ_CLOSE",
   "IRP_MJ_READ",
   "IRP_MJ_WRITE",
   "IRP_MJ_QUERY_INFORMATION",
   "IRP_MJ_SET_INFORMATION",
   "IRP_MJ_QUERY_EA",
   "IRP_MJ_SET_EA",
   "IRP_MJ_FLUSH_BUFFERS",
   "IRP_MJ_QUERY_VOLUME_INFORMATION",
   "IRP_MJ_SET_VOLUME_INFORMATION",
   "IRP_MJ_DIRECTORY_CONTROL",
   "IRP_MJ_FILE_SYSTEM_CONTROL",
   "IRP_MJ_DEVICE_CONTROL",
   "IRP_MJ_INTERNAL_DEVICE_CONTROL",
   "IRP_MJ_SHUTDOWN",
   "IRP_MJ_LOCK_CONTROL",
   "IRP_MJ_CLEANUP",
   "IRP_MJ_CREATE_MAILSLOT",
   "IRP_MJ_QUERY_SECURITY",
   "IRP_MJ_SET_SECURITY",
   "IRP_MJ_POWER",          
   "IRP_MJ_SYSTEM_CONTROL", 
   "IRP_MJ_DEVICE_CHANGE",  
   "IRP_MJ_QUERY_QUOTA",    
   "IRP_MJ_SET_QUOTA",      
   "IRP_MJ_PNP"            
};
//IRP_MJ_MAXIMUM_FUNCTION defined in wdm.h


static const PCHAR szPnpMnFuncDesc[] =
{	// note this depends on corresponding values to the indexes in wdm.h 

    "IRP_MN_START_DEVICE",
    "IRP_MN_QUERY_REMOVE_DEVICE",
    "IRP_MN_REMOVE_DEVICE",
    "IRP_MN_CANCEL_REMOVE_DEVICE",
    "IRP_MN_STOP_DEVICE",
    "IRP_MN_QUERY_STOP_DEVICE",
    "IRP_MN_CANCEL_STOP_DEVICE",
    "IRP_MN_QUERY_DEVICE_RELATIONS",
    "IRP_MN_QUERY_INTERFACE",
    "IRP_MN_QUERY_CAPABILITIES",
    "IRP_MN_QUERY_RESOURCES",
    "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
    "IRP_MN_QUERY_DEVICE_TEXT",
    "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
    "IRP_MN_READ_CONFIG",
    "IRP_MN_WRITE_CONFIG",
    "IRP_MN_EJECT",
    "IRP_MN_SET_LOCK",
    "IRP_MN_QUERY_ID",
    "IRP_MN_QUERY_PNP_DEVICE_STATE",
    "IRP_MN_QUERY_BUS_INFORMATION",
    "IRP_MN_DEVICE_USAGE_NOTIFICATION",
	"IRP_MN_SURPRISE_REMOVAL"
};

#define IRP_PNP_MN_FUNCMAX	IRP_MN_SURPRISE_REMOVAL



static const PCHAR szSystemPowerState[] = 
{
    "PowerSystemUnspecified",
    "PowerSystemWorking",
    "PowerSystemSleeping1",
    "PowerSystemSleeping2",
    "PowerSystemSleeping3",
    "PowerSystemHibernate",
    "PowerSystemShutdown",
    "PowerSystemMaximum"
};

static const PCHAR szDevicePowerState[] = 
{
    "PowerDeviceUnspecified",
    "PowerDeviceD0",
    "PowerDeviceD1",
    "PowerDeviceD2",
    "PowerDeviceD3",
    "PowerDeviceMaximum"
};




BOOLEAN
UMSS_GetRegistryDword(
    IN      PWCHAR    RegPath,
    IN      PWCHAR    ValueName,
    IN OUT  PULONG    Value
    );


#define UMSS_KdPrintCond( ilev, cond, _x_) \
        if( UMSS_DebugLevel && ( ilev <= UMSS_DebugLevel ) && ( cond )) { \
			DPRINT( DBGSTR_PREFIX ); \
			DPRINT _x_ ; \
	}



#define UMSS_KdPrint( ilev, _x_)  UMSS_KdPrintCond( ilev, TRUE, _x_ )


#define UMSS_TrapCond( ilev, cond ) if ( UMSS_DebugLevel && ( ilev <= UMSS_DebugLevel ) && (cond) ) TRAP()
#define UMSS_Trap( ilev )      UMSS_TrapCond( ilev, TRUE )


#define UMSS_ASSERT( cond ) ASSERT( cond )

#define UMSS_StringForDevState( devState )  szDevicePowerState[ devState ] 

#define UMSS_StringForSysState( sysState )  szSystemPowerState[ sysState ] 

#define UMSS_StringForPnpMnFunc( mnfunc ) szPnpMnFuncDesc[ mnfunc ]

#define UMSS_StringForIrpMjFunc(  mjfunc ) szIrpMajFuncDesc[ mjfunc ]

PVOID 
    UMSS_ExAllocatePool(
        IN POOL_TYPE PoolType,
        IN ULONG NumberOfBytes
        );


VOID 
    UMSS_ExFreePool(
        IN PVOID p
        );



#else // if not DBG

// dummy definitions that go away in the retail build

#define UMSS_KdPrintCond( ilev, cond, _x_) 
#define UMSS_KdPrint( ilev, _x_)  
#define UMSS_TrapCond( ilev, cond ) 
#define UMSS_Trap( ilev )
#define UMSS_ASSERT( cond )
#define UMSS_StringForDevState( devState )
#define UMSS_StringForSysState( sysState ) 
#define UMSS_StringForPnpMnFunc( mnfunc )
#define UMSS_StringForIrpMjFunc(  mjfunc ) 

#define UMSS_ExFreePool( p )   ExFreePool( p )
#define UMSS_ExAllocatePool( typ, siz )  ExAllocatePool( typ, siz )
#define TRAP() 

#define ENTER(FunctionName) 
#define EXIT(FunctionName)
#define RETURN(RetVal, FunctionName)  return(RetVal)
#define DBG_STATUS(DeviceExtension)


#endif //DBG

#endif // included



