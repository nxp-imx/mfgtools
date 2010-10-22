/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/****************************************************************************
*
*  (C) COPYRIGHT 2004, MICROSOFT CORP.
*
*  VERSION:     1.0
*
*  DESCRIPTION:
*    Structures and constants needed to issue vendor-specific Media Transfer
*    Protocol (MTP) commands through DeviceIOControl mechanism.
*
*****************************************************************************/
#pragma once

//
// Pass this value in the dwIoControlCode argument of IWMDMDevice3::DeviceIoControl
// to execute a direct MTP command
//
#define IOCTL_MTP_CUSTOM_COMMAND    0x3150544d

#define MTP_OPCODE_GET_DEVICE_INFO	0x1001
// MAY CONFLICT WITH MICROSOFT
// #define MTP_OPCODE_SIGMATEL_RESET     0x9201
// #define MTP_OPCODE_SIGMATEL_ERASEBOOT 0x9202
// OFFICIALLY RELEASED
#define MTP_OPCODE_SIGMATEL_UNDEFINED         0x97F0 
#define MTP_OPCODE_SIGMATEL_RESET             0x97F1
#define MTP_OPCODE_SIGMATEL_ERASEBOOT         0x97F2
#define MTP_OPCODE_SIGMATEL_FORCERECV         0x97F3
#define MTP_OPCODE_SIGMATEL_RESET_TO_UPDATER  0x97F4
#define MTP_OPCODE_SIGMATEL_GET_DRIVE_VERSION 0x97F5
#define MTP_OPCODE_SIGMATEL_SET_UPDATE_FLAG   0x97F6

//
// MTP command request
//
const DWORD MTP_COMMAND_MAX_PARAMS  = 5;
const DWORD MTP_RESPONSE_MAX_PARAMS = 5;

//
// MTP response codes
//
const WORD MTP_RESPONSE_OK = 0x2001;

#define CMD_BUF_SIZE 1024

#pragma pack(push, Old, 1)

typedef struct _MTP_COMMAND_DATA_IN
{
    WORD    OpCode;                         // Opcode
    DWORD   NumParams;                      // Number of parameters passed in
    DWORD   Params[MTP_COMMAND_MAX_PARAMS]; // Parameters to the command
    DWORD   NextPhase;                      // Indicates whether the command has a read data,
                                            // write data, or no data phase.
    DWORD   CommandWriteDataSize;           // Number of bytes contained in CommandWriteData.
    BYTE    CommandWriteData[CMD_BUF_SIZE]; // Optional first byte of data to
                                            // write to the device if NextPhase is MTP_NEXTPHASE_WRITE_DATA
} MTP_COMMAND_DATA_IN, *PMTP_COMMAND_DATA_IN;

//
// MTP response block
//
typedef struct _MTP_COMMAND_DATA_OUT
{
    WORD    ResponseCode;                       // Response code
    DWORD   NumParams;                          // Number of parameters for this response
    DWORD   Params[MTP_RESPONSE_MAX_PARAMS];    // Parameters of the response
    DWORD   CommandReadDataSize;                // Number of bytes contained in CommandReadData.
    BYTE    CommandReadData[CMD_BUF_SIZE];      // Optional first byte of data to
                                                // read from the device if 
                                                // MTP_COMMAND_DATA_IN::NextPhase was MTP_NEXTPHASE_READ_DATA
} MTP_COMMAND_DATA_OUT, *PMTP_COMMAND_DATA_OUT;

#pragma pack(pop, Old)

//
// Handy structure size constants
//
#define SIZEOF_REQUIRED_COMMAND_DATA_IN (sizeof(MTP_COMMAND_DATA_IN) - CMD_BUF_SIZE)
#define SIZEOF_REQUIRED_COMMAND_DATA_OUT (sizeof(MTP_COMMAND_DATA_OUT) - CMD_BUF_SIZE)

//
// NextPhase constants
//
#define MTP_NEXTPHASE_READ_DATA     1
#define MTP_NEXTPHASE_WRITE_DATA    2
#define MTP_NEXTPHASE_NO_DATA       3


