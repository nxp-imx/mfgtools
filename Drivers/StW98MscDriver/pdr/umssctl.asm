;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright 1999  Microsoft Corporation.  All Rights Reserved.              *
;                                                                           *
;****************************************************************************

PAGE 58,132
;******************************************************************************
TITLE CONTROL - ControlDispatch for USB Mass Storage Port Driver
;******************************************************************************
;

    .386p

;******************************************************************************
;                I N C L U D E S
;******************************************************************************

    .xlist
    include vmm.inc
    include ios.inc
    include debug.inc
    .list

; the following equate makes the VXD dynamically loadable.
UMSSPDR_DYNAMIC EQU 1



;============================================================================
;        V I R T U A L   D E V I C E   D E C L A R A T I O N
;============================================================================

extrn _Drv_Reg_Pkt:DWORD

DECLARE_VIRTUAL_DEVICE    STUMSPDR, 1, 0, UMSSPDR_Control,, \
                        UNDEFINED_INIT_ORDER,,,_Drv_Reg_Pkt 

VxD_LOCKED_CODE_SEG


;===========================================================================
;
;   PROCEDURE: UMSSPDR_Control
;
;   DESCRIPTION:
;    Device control procedure for the USB Mass Storage IOS Port Driver
;
;   ENTRY:
;    EAX = Control call ID
;
;   EXIT:
;    If carry clear then
;        Successful
;    else
;        Control call failed
;
;   USES:
;    EAX, EBX, ECX, EDX, ESI, EDI, Flags
;
;============================================================================

BeginProc UMSSPDR_Control
    Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, UMSSPDR_Dynamic_Init, sCall
    Control_Dispatch SYS_DYNAMIC_DEVICE_EXIT, UMSSPDR_Dynamic_Exit, sCall
    clc
    ret
EndProc UMSSPDR_Control


VxD_LOCKED_CODE_ENDS


VxD_ICODE_SEG

public _IosRegister

BeginProc _IosRegister, esp
        ArgVar DrvPkt, DWORD

        EnterProc
        VxDCall IOS_Register <DrvPkt>
        LeaveProc                

        Return

EndProc _IosRegister

VxD_ICODE_ENDS



END
