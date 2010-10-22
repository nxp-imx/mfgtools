/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ummsdbg.h

Abstract:

    IOS port driver for USB Mass Storage Sample driver
    debug header file

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


#ifdef DEBUG

#define DBGLVL_OFF                      0               // if UMSS_DebugLevel set to this, there is NO debug output 
#define DBGLVL_MINIMUM			1		// minimum verbosity	
#define DBGLVL_DEFAULT			2		// default verbosity level if no registry override
#define DBGLVL_MEDIUM			3		// medium verbosity
#define DBGLVL_HIGH                     4               // highest 'safe' level (without severely affecting timing )
#define DBGLVL_MAXIMUM			5		// maximum level, may be dangerous


extern DWORD UMSSPDR_DebugLevel;

#define UMSSPDR_DebugPrintf(DebugMask, _x_) \
     if( UMSSPDR_DebugLevel >= DebugMask) \
     { \
         Debug_Printf("STUMSPDR: "); \
         Debug_Printf##_x_; \
     }

#define UMSS_ASSERT(exp) \
    if (exp) \
    { \
        Debug_Printf("STUMSPDR: ASSERTION FAILURE, "); \
        Debug_Printf(#exp);  \
        Debug_Printf(", File %s, ",__FILE__); \
        Debug_Printf("Line %d\n", __LINE__); \
        Trap(); \
    }

#define DBG_MIN(_x_)           \
    if( UMSSPDR_DebugLevel >= DBGLVL_MINIMUM) \
    { \
         Debug_Printf("STUMSPDR: "); \
         Debug_Printf##(_x_); \
    }
 
#define DBG_DEF(_x_)           \
    if( UMSSPDR_DebugLevel >= DBGLVL_DEFAULT) \
    { \
         Debug_Printf("STUMSPDR: "); \
         Debug_Printf##(_x_);  \
    }

#define DBG_MED(_x_)           \
    if( UMSSPDR_DebugLevel >= DBGLVL_MEDIUM) \
    { \
         Debug_Printf("STUMSPDR: "); \
         Debug_Printf##(_x_);   \
    }

#define DBG_HIGH(_x_)           \
    if( UMSSPDR_DebugLevel >= DBGLVL_HIGH) \
    { \
         Debug_Printf("STUMSPDR: "); \
         Debug_Printf##(_x_);   \
    }

#define DBG_MAX(_x_)           \
    if( UMSSPDR_DebugLevel >= DBGLVL_MAXIMIM) \
    { \
         Debug_Printf("STUMSPDR: "); \
         Debug_Printf##(_x_);   \
    }

#define ENTER(FunctionName) UMSSPDR_DebugPrintf(DBGLVL_HIGH, ("-> %s\n", #FunctionName));

#define EXIT(FunctionName) \
    { \
        UMSSPDR_DebugPrintf(DBGLVL_HIGH, ("<- %s\n", #FunctionName)); \
        return;  \
    }

#define RETURN(RetVal, FunctionName) \
    { \
        UMSSPDR_DebugPrintf(DBGLVL_HIGH, ("<- %s, ret = %x\n", #FunctionName, RetVal)); \
        return (RetVal); \
    }

#else

#define UMSSPDR_DebugPrintf(DebugMask, _x_)
#define UMSS_ASSERT( exp )
#define ENTER(FunctionName) 
#define EXIT(FunctionName)  return;
#define RETURN(RetVal, FunctionName)  return(RetVal);

#endif



