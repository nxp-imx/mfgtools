//////////////////////////////////////////////////////////////////////
// File - USB_DIAG_LIB.H
//
// Library for USB diagnostics and samples, using WinDriver functions.
// 
// Copyright (c) 2003 Jungo Ltd.  http://www.jungo.com
// 
//////////////////////////////////////////////////////////////////////

#ifndef _USB_DIAG_LIB_H_
#define _USB_DIAG_LIB_H_
//#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "windrvr.h"

typedef struct  
{
    DWORD dwPipeNum;
    HANDLE hDevice;
    DWORD dwPacketSize;
    PVOID pContext;
    BOOL fStopped;
    HANDLE hThread;
    DWORD dwError;
} USB_LISTEN_PIPE;

typedef struct
{
    DWORD dwInterfaceNum;
    DWORD dwAlternateSetting;
    WDU_DEVICE_HANDLE hDev;
    HANDLE hEvent;
    DWORD dwAttachError;
} DEVICE_CONTEXT;

typedef struct 
{
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize0;
    
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char iManufacturer;
    unsigned char iProduct;
    unsigned char iSerialNumber;
    unsigned char bNumConfigurations;
} USB_DEVICE_DESCRIPTOR;

enum {MAX_BUFFER_SIZE = 4096};

#ifdef __cplusplus
}
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

// Function: PrintHexBuffer()
//   Print a buffer in HEX format
void PrintHexBuffer(PVOID pBuffer, DWORD dwBytes);

// Function: GetHexBuffer()
//   Get hex buffer from user
DWORD GetHexBuffer(PVOID pBuffer, DWORD dwBytes);

// Function: pipeType2Str()
//   Returns a string identifying the pipe type
char *pipeType2Str(ULONG pipeType);

// Function: PrintPipesInfo()
//   Prints the pipes information for the specified alternate setting
void PrintPipesInfo(WDU_ALTERNATE_SETTING *pAltSet);

// Function: PrintDeviceConfigurations()
//   Prints the device's configurations information
void PrintDeviceConfigurations(HANDLE hDevice);

// Function: ReadWritePipesMenu()
//   Displays menu to read/write from the device's pipes
void ReadWritePipesMenu(HANDLE hDevice);

extern BOOL USB_GetDeviceDescriptor(USB_DEVICE_DESCRIPTOR *pDescriptor);
extern BOOL USB_OpenLocateDevice(int id);
void USB_CloseLocateDevice(void);
BOOL WriteToUsb(DWORD dwSize,PUINT pCommand,int id);
BOOL ReadFromUsb(DWORD dwSize, int id, PVOID pBuffer);

#ifdef __cplusplus
}
#endif


//#endif
