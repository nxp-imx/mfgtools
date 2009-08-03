/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    UMSSPDR.H

Abstract:

    IOS port driver for USB Mass Storage Sample driver
    main header file for driver

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    03/19/99: MRB  Original

--*/

//
// Include header file shared with UMSS.SYS
//
#include "..\iopacket.h"



//
// Type definitions
//

//Type definitions for exported functions in UMSS.SYS driver
typedef DWORD (_stdcall *UMSSPDR_GETNEXTPDO)(PVOID);
typedef PVOID (_stdcall *UMSSPDR_STARTREQUEST)(PIOPACKET);
typedef PVOID (_stdcall *UMSSPDR_REGISTERHANDLER)(DWORD, PVOID);
typedef CHAR  (_stdcall *UMSSPDR_GETMAXLUN)(DWORD);


//Type definition for IOS callback handler
typedef VOID  (*PIOPCB) (pIOP);

//
// External declarations for global variables
//

// IOS Linkage Block
extern ILB UMSSPDR_Ilb;

// Function pointers for exported entrypoints in UMSS.SYS
extern UMSSPDR_GETNEXTPDO pfnGetNextPdo;
extern UMSSPDR_STARTREQUEST pfnStartRequest;
extern UMSSPDR_REGISTERHANDLER pfnRegisterCompletionHandler;
extern UMSSPDR_GETMAXLUN pfnGetMaxLun;


//
// Symbolic constants
//

// USB mass storage definitions
#define    UMSSPDRName        "USB Mass Storage"
#define    UMSSPDRRev         1
#define    UMSSPDRFeature     DRP_FC_IO_FOR_INQ_AEP 
#define    UMSSPDR_Major_Ver  0x03
#define    UMSSPDR_Minor_Ver  0x0A

// USBDDB flags
#define USBDDB_FLAG_BUSY 0x01
#define USBDDB_FLAG_ERROR 0x02


//
// Structure definitions
//

// DDB structure definition
#include <ddb.h>
typedef struct _USBDDB
{
        DDB      Ddb;
        DWORD    Signature;
        DWORD    Fdo;
        DWORD    Flags;
        BYTE     Cdb[12];
        IOPACKET IoPacket;
        BYTE     MaxLun;
        struct _USBDDB  *NextDdb;
        PDCB     Dcb[1];          //Variable length array of PDCBs
} USBDDB, *PUSBDDB;


//
// Function prototyes
//

// UMSSIO.C
VOID   UMSSPDR_Request(pIOP piop);
VOID   UMSSPDR_AerDeviceInquiry(PAEP_inquiry_device Aep);
VOID   UMSSPDR_ErrorHandler(pIOP Iop);
VOID   UMSSPDR_CompleteIOP(pIOP Iop);
VOID   UMSSPDR_StartIo(pIOP Iop);
VOID   UMSSPDR_ProcessIoPacket(PIOPACKET IoPacket, pIOP Iop);
pIOP   UMSSPDR_GetNextIop(PUSBDDB Ddb);
VOID   _stdcall UMSSPDR_CompleteRequest(PIOPACKET IoPacket);

// UMSSAER.C
VOID   UMSSPDR_AsyncEventHandler (PAEP paep);
VOID   UMSSPDR_AerInit(PAEP_bi_init Aep);
VOID   UMSSPDR_AerUninit(PAEP_bi_uninit Aep);
VOID   UMSSPDR_AerConfigDcb(PAEP_dcb_config Aep);
VOID   UMSSPDR_AerUnconfigDcb(PAEP_dcb_unconfig Aep);
VOID   UMSSPDR_AerBootComplete(PAEP_boot_done Aep);

// UMSSLIST.C
BOOL UMSSPDR_AddDcbToList(PUSBDDB Ddb, PDCB Dcb);
BOOL UMSSPDR_RemoveDcbFromList(PUSBDDB Ddb, PDCB Dcb);
PDCB UMSSPDR_GetFirstDcb(PUSBDDB Ddb);
PDCB UMSSPDR_GetNextDcb(PUSBDDB Ddb, PDCB PreviousDcb);
VOID UMSSPDR_AddDdbToList(PUSBDDB NewDdb);
BOOL UMSSPDR_RemoveDdbFromList(PUSBDDB OldDdb);
PUSBDDB UMSSPDR_GetFirstDdb(VOID);
PUSBDDB UMSSPDR_GetNextDdb(PUSBDDB Ddb);

// UMSSCTL.ASM
VOID _cdecl IosRegister (PDRP DrvPkt);


//
// Macro definitions
//

// Macros to hide ILB function call ugliness
#define ILBService(_x_) UMSSPDR_Ilb.ILB_service_rtn((PISP)_x_)
#define ILBCriteria(_x_) ((ILB_internal_request_func)(UMSSPDR_Ilb.ILB_int_io_criteria_rtn))((PISP)_x_)
#define ILBEnqueueIop(Iop, Dcb) ((ILB_enqueue_iop_func)(UMSSPDR_Ilb.ILB_enqueue_iop))(piop, Dcb)
#define ILBDequeueIop(Dcb) ((ILB_dequeue_iop_func)(UMSSPDR_Ilb.ILB_dequeue_iop))(Dcb)
#define ILBRequest(Iop, Dcb)  \
    _asm mov edi, Iop         \
    _asm mov ebx, Dcb         \
    _asm mov edx, 0           \
    _asm call UMSSPDR_Ilb.ILB_internal_request 
#define SaveEbx() _asm push ebx
#define RestoreEbx() _asm pop ebx
#define IOPCallBack(IopCB, Iop) ((PIOPCB)(IopCB->IOP_CB_address))(Iop)  



