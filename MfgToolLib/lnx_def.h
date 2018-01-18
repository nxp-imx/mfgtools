/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

//#define CString std::stringa


#ifndef __LNX_DEFS__

#define __LNX_DEFS__

#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <cstring>
#include <ctype.h>
#include <iostream>
#include <cstdlib>
#include "CString.h"
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <queue>
#include <map>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libusb.h>

#ifndef FALSE
#define FALSE (0)
#endif
#define BOOL bool
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define _tcschr strchr
#define _totlower tolower
#define	_tcslen strlen
#define _istspace isspace
#define _ttol atol
#define _tcstoull strtoul
#define _tcspbrk strpbrk
#define _totupper toupper
#define _tfopen fopen
#define _tstat stat
#define _tctime ctime
#define _stat64i32 stat
#define _stat stat
#define _fputts fputs
#define _tcstol strtol
#define sscanf_s sscanf

//#define .Format =sprintf
#define TCHAR char
#define _T(x) x
#define StringFromIID(x,y)
#define CoTaskMemFree(x)
#define _stscanf_s sscanf
#define MAX_PATH 260
#define INVALID_HANDLE_VALUE (void *)0xFFFFFFFFUL

#define ERROR_SUCCESS 0

typedef int INT;
typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef unsigned long long UINT64;
//#define BYTE unsigned char
//#define UINT unsigned int
//#define DWORD unsigned int
//#define WORD unsigned short  /// define windows types for linux
typedef unsigned short USHORT;
typedef DWORD DEVINST;
typedef uint64_t HANDLE;
typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID,*LPGUID;
#define GUID_NULL {0}
typedef long LONG;
typedef const LPGUID LPCGUID;
typedef const char * LPCTSTR;
typedef char * LPTSTR;
typedef const char * LPCSTR;
typedef unsigned long * ULONG_PTR;
typedef char * LPOLESTR;
typedef void * HDEVINFO;
typedef unsigned long long ULONGLONG;
typedef long long __time64_t;
typedef long long __int64;
typedef unsigned long DWORD_PTR;
typedef unsigned int UINT;
typedef DWORD HINSTANCE;
typedef DWORD HMODULE;
typedef void * LPVOID;
typedef USHORT VERSION_DESCRIPTOR, *PVERSION_DESCRIPTOR;

typedef UCHAR * PUCHAR;
typedef unsigned long ULONG;
typedef void VOID;
typedef unsigned int UINT32;
#define _tcstoul strtoul
#define _tstoi64 atoll
typedef DWORD LRESULT;
typedef WORD WPARAM;
typedef uint64_t LPARAM;
typedef DWORD HWND;
typedef DWORD HDEVNOTIFY;
typedef BYTE * PBYTE;
typedef LONG * PLONG;
typedef ULONG * PULONG;
typedef WORD LANGID;

#define __AFXWIN_H__
#define IsEmpty empty
//std::string str_format(const std::string fmt_str, ...);
typedef struct _SP_DEVINFO_DATA {
    DWORD cbSize;
    GUID  ClassGuid;
    DWORD DevInst;    // DEVINST handle
    ULONG_PTR Reserved;
} SP_DEVINFO_DATA, *PSP_DEVINFO_DATA;

inline void Sleep(int ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
};

#define LANG_NEUTRAL 0x00
#define SUBLANG_NEUTRAL 0x03

#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))
#define PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((WORD  )(lgid) >> 10)

#pragma pack(push, sensedata, 1)
typedef struct _SENSE_DATA {
    UCHAR ErrorCode:7;
    UCHAR Valid:1;
    UCHAR SegmentNumber;
    UCHAR SenseKey:4;
    UCHAR Reserved:1;
    UCHAR IncorrectLength:1;
    UCHAR EndOfMedia:1;
    UCHAR FileMark:1;
    UCHAR Information[4];
    UCHAR AdditionalSenseLength;
    UCHAR CommandSpecificInformation[4];
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR FieldReplaceableUnitCode;
    UCHAR SenseKeySpecific[3];
} SENSE_DATA, *PSENSE_DATA;
#pragma pack(pop, sensedata)


#define SCSI_SENSE_NO_SENSE         0x00
#define SCSI_SENSE_RECOVERED_ERROR  0x01
#define SCSI_SENSE_NOT_READY        0x02
#define SCSI_SENSE_NOT_READY        0x02
#define SCSI_SENSE_MEDIUM_ERROR     0x03
#define SCSI_SENSE_HARDWARE_ERROR   0x04
#define SCSI_SENSE_ILLEGAL_REQUEST  0x05
#define SCSI_SENSE_UNIT_ATTENTION   0x06
#define SCSI_SENSE_DATA_PROTECT     0x07
#define SCSI_SENSE_BLANK_CHECK      0x08
#define SCSI_SENSE_UNIQUE           0x09
#define SCSI_SENSE_COPY_ABORTED     0x0A
#define SCSI_SENSE_ABORTED_COMMAND  0x0B
#define SCSI_SENSE_EQUAL            0x0C
#define SCSI_SENSE_VOL_OVERFLOW     0x0D
#define SCSI_SENSE_MISCOMPARE       0x0E
#define SCSI_SENSE_RESERVED         0x0F

//
#define SCSISTAT_GOOD                  0x00
#define SCSISTAT_CHECK_CONDITION       0x02
#define SCSISTAT_CONDITION_MET         0x04
#define SCSISTAT_BUSY                  0x08
#define SCSISTAT_INTERMEDIATE          0x10
#define SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define SCSISTAT_RESERVATION_CONFLICT  0x18
#define SCSISTAT_COMMAND_TERMINATED    0x22
#define SCSISTAT_QUEUE_FULL            0x28


#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

#pragma pack(1)

typedef struct _CDB6INQUIRY {
        UCHAR OperationCode;    // 0x12 - SCSIOP_INQUIRY
        UCHAR Reserved1 : 5;
        UCHAR LogicalUnitNumber : 3;
        UCHAR PageCode;
        UCHAR IReserved;
        UCHAR AllocationLength;
        UCHAR Control;
} CDB6INQUIRY;


typedef struct _CDB12 {
        UCHAR OperationCode;
        UCHAR RelativeAddress : 1;
        UCHAR Reserved1 : 2;
        UCHAR ForceUnitAccess : 1;
        UCHAR DisablePageOut : 1;
        UCHAR LogicalUnitNumber : 3;
        UCHAR LogicalBlock[4];
        UCHAR TransferLength[4];
        UCHAR Reserved2;
        UCHAR Control;
} CDB12;
#pragma pack()
#pragma pack(push, inquiry, 1)
typedef struct _INQUIRYDATA {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;
    UCHAR DeviceTypeModifier : 7;
    UCHAR RemovableMedia : 1;
    union {
        UCHAR Versions;
        struct {
            UCHAR ANSIVersion : 3;
            UCHAR ECMAVersion : 3;
            UCHAR ISOVersion : 2;
        };
    };
    UCHAR ResponseDataFormat : 4;
    UCHAR HiSupport : 1;
    UCHAR NormACA : 1;
    UCHAR TerminateTask : 1;
    UCHAR AERC : 1;
    UCHAR AdditionalLength;
    union {
        UCHAR Reserved;
        struct {
            UCHAR PROTECT : 1;
            UCHAR Reserved_1 : 2;
            UCHAR ThirdPartyCoppy : 1;
            UCHAR TPGS : 2;
            UCHAR ACC : 1;
            UCHAR SCCS : 1;
       };
    };
    UCHAR Addr16 : 1;               // defined only for SIP devices.
    UCHAR Addr32 : 1;               // defined only for SIP devices.
    UCHAR AckReqQ: 1;               // defined only for SIP devices.
    UCHAR MediumChanger : 1;
    UCHAR MultiPort : 1;
    UCHAR ReservedBit2 : 1;
    UCHAR EnclosureServices : 1;
    UCHAR ReservedBit3 : 1;
    UCHAR SoftReset : 1;
    UCHAR CommandQueue : 1;
    UCHAR TransferDisable : 1;      // defined only for SIP devices.
    UCHAR LinkedCommands : 1;
    UCHAR Synchronous : 1;          // defined only for SIP devices.
    UCHAR Wide16Bit : 1;            // defined only for SIP devices.
    UCHAR Wide32Bit : 1;            // defined only for SIP devices.
    UCHAR RelativeAddressing : 1;
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
    UCHAR VendorSpecific[20];
    UCHAR Reserved3[2];
    VERSION_DESCRIPTOR VersionDescriptors[8];
    UCHAR Reserved4[30];
} INQUIRYDATA, *PINQUIRYDATA;
#pragma pack(pop, inquiry)

// 6-byte commands:
#define SCSIOP_TEST_UNIT_READY          0x00
#define SCSIOP_REZERO_UNIT              0x01
#define SCSIOP_REWIND                   0x01
#define SCSIOP_REQUEST_BLOCK_ADDR       0x02
#define SCSIOP_REQUEST_SENSE            0x03
#define SCSIOP_FORMAT_UNIT              0x04
#define SCSIOP_READ_BLOCK_LIMITS        0x05
#define SCSIOP_REASSIGN_BLOCKS          0x07
#define SCSIOP_INIT_ELEMENT_STATUS      0x07
#define SCSIOP_READ6                    0x08
#define SCSIOP_RECEIVE                  0x08
#define SCSIOP_WRITE6                   0x0A
#define SCSIOP_PRINT                    0x0A
#define SCSIOP_SEND                     0x0A
#define SCSIOP_SEEK6                    0x0B
#define SCSIOP_TRACK_SELECT             0x0B
#define SCSIOP_SLEW_PRINT               0x0B
#define SCSIOP_SET_CAPACITY             0x0B // tape
#define SCSIOP_SEEK_BLOCK               0x0C
#define SCSIOP_PARTITION                0x0D
#define SCSIOP_READ_REVERSE             0x0F
#define SCSIOP_WRITE_FILEMARKS          0x10
#define SCSIOP_FLUSH_BUFFER             0x10
#define SCSIOP_SPACE                    0x11
#define SCSIOP_INQUIRY                  0x12
#define SCSIOP_VERIFY6                  0x13
#define SCSIOP_RECOVER_BUF_DATA         0x14
#define SCSIOP_MODE_SELECT              0x15
#define SCSIOP_RESERVE_UNIT             0x16
#define SCSIOP_RELEASE_UNIT             0x17
#define SCSIOP_COPY                     0x18
#define SCSIOP_ERASE                    0x19
#define SCSIOP_MODE_SENSE               0x1A
#define SCSIOP_START_STOP_UNIT          0x1B
#define SCSIOP_STOP_PRINT               0x1B
#define SCSIOP_LOAD_UNLOAD              0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC       0x1C
#define SCSIOP_SEND_DIAGNOSTIC          0x1D
#define SCSIOP_MEDIUM_REMOVAL           0x1E

// 10-byte commands
#define SCSIOP_READ_FORMATTED_CAPACITY  0x23
#define SCSIOP_READ_CAPACITY            0x25
#define SCSIOP_READ                     0x28
#define SCSIOP_WRITE                    0x2A
#define SCSIOP_SEEK                     0x2B
#define SCSIOP_LOCATE                   0x2B
#define SCSIOP_POSITION_TO_ELEMENT      0x2B
#define SCSIOP_WRITE_VERIFY             0x2E
#define SCSIOP_VERIFY                   0x2F
#define SCSIOP_SEARCH_DATA_HIGH         0x30
#define SCSIOP_SEARCH_DATA_EQUAL        0x31
#define SCSIOP_SEARCH_DATA_LOW          0x32
#define SCSIOP_SET_LIMITS               0x33
#define SCSIOP_READ_POSITION            0x34
#define SCSIOP_SYNCHRONIZE_CACHE        0x35
#define SCSIOP_COMPARE                  0x39
#define SCSIOP_COPY_COMPARE             0x3A
#define SCSIOP_WRITE_DATA_BUFF          0x3B
#define SCSIOP_READ_DATA_BUFF           0x3C
#define SCSIOP_WRITE_LONG               0x3F
#define SCSIOP_CHANGE_DEFINITION        0x40
#define SCSIOP_WRITE_SAME               0x41
#define SCSIOP_READ_SUB_CHANNEL         0x42
#define SCSIOP_UNMAP                    0x42 // block device
#define SCSIOP_READ_TOC                 0x43
#define SCSIOP_READ_HEADER              0x44
#define SCSIOP_REPORT_DENSITY_SUPPORT   0x44 // tape
#define SCSIOP_PLAY_AUDIO               0x45
#define SCSIOP_GET_CONFIGURATION        0x46
#define SCSIOP_PLAY_AUDIO_MSF           0x47
#define SCSIOP_PLAY_TRACK_INDEX         0x48
#define SCSIOP_SANITIZE                 0x48 // block device
#define SCSIOP_PLAY_TRACK_RELATIVE      0x49
#define SCSIOP_GET_EVENT_STATUS         0x4A
#define SCSIOP_PAUSE_RESUME             0x4B
#define SCSIOP_LOG_SENSE                0x4D
#define SCSIOP_STOP_PLAY_SCAN           0x4E
#define SCSIOP_XDWRITE                  0x50
#define SCSIOP_XPWRITE                  0x51
#define SCSIOP_READ_DISK_INFORMATION    0x51
#define SCSIOP_READ_DISC_INFORMATION    0x51 // proper use of disc over disk
#define SCSIOP_READ_TRACK_INFORMATION   0x52
#define SCSIOP_XDWRITE_READ             0x53
#define SCSIOP_RESERVE_TRACK_RZONE      0x53
#define SCSIOP_SEND_OPC_INFORMATION     0x54 // optimum power calibration
#define SCSIOP_MODE_SELECT10            0x55
#define SCSIOP_RESERVE_UNIT10           0x56
#define SCSIOP_RESERVE_ELEMENT          0x56
#define SCSIOP_RELEASE_UNIT10           0x57
#define SCSIOP_RELEASE_ELEMENT          0x57
#define SCSIOP_REPAIR_TRACK             0x58
#define SCSIOP_MODE_SENSE10             0x5A
#define SCSIOP_CLOSE_TRACK_SESSION      0x5B
#define SCSIOP_READ_BUFFER_CAPACITY     0x5C
#define SCSIOP_SEND_CUE_SHEET           0x5D
#define SCSIOP_PERSISTENT_RESERVE_IN    0x5E
#define SCSIOP_PERSISTENT_RESERVE_OUT   0x5F


#define TRACE 
#define USAGE UINT
typedef struct _HIDP_CAPS
{
    USAGE    Usage;
    USAGE    UsagePage;
    USHORT   InputReportByteLength;
    USHORT   OutputReportByteLength;
    USHORT   FeatureReportByteLength;
    USHORT   Reserved[17];

    USHORT   NumberLinkCollectionNodes;

    USHORT   NumberInputButtonCaps;
    USHORT   NumberInputValueCaps;
    USHORT   NumberInputDataIndices;

    USHORT   NumberOutputButtonCaps;
    USHORT   NumberOutputValueCaps;
    USHORT   NumberOutputDataIndices;

    USHORT   NumberFeatureButtonCaps;
    USHORT   NumberFeatureValueCaps;
    USHORT   NumberFeatureDataIndices;
} HIDP_CAPS, *PHIDP_CAPS;

#define CALLBACK

#define ASSERT(x)
#define _tcscpy strcpy
#define _vsntprintf  vsnprintf
#define _vsctprintf _vscprintf
#define _tcserror strerror
inline int GetLastError() { return errno;}

#define ERROR_FILE_NOT_FOUND             2L


struct thread_msg{
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    thread_msg() : message(), wParam(), lParam() {}
    thread_msg(UINT msg, WPARAM wPar, LPARAM lPar):message(msg),wParam(wPar),lParam(lPar){}


};

typedef void * PVOID;
typedef signed int INT32;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        } DUMMYSTRUCTNAME;
        PVOID Pointer;
    } DUMMYUNIONNAME;

    HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;


inline int _vscprintf (const char * format, va_list pargs) {
	int retval;
	va_list argcopy; 
	va_copy(argcopy, pargs); 
	retval = vsnprintf(NULL, 0, format, argcopy);
	va_end(argcopy); 
	return retval;
}

#define CAnsiString CString



#define ERROR_INVALID_DATA               13L

//
// MessageId: ERROR_OUTOFMEMORY
//
// MessageText:
//
// Not enough storage is available to complete this operation.
//
#define ERROR_OUTOFMEMORY                14L

#define _MAX_DRIVE  3   /* max. length of drive component */
#define _MAX_DIR    256 /* max. length of path component */
#define _MAX_FNAME  256 /* max. length of file name component */
#define _MAX_EXT    256 /* max. length of extension component */

#define ERROR_OPEN_FAILED                110L

typedef HANDLE HRSRC;

#define _tcsrchr        strrchr

#define _tcscmp         strcmp

typedef enum _USB_CONNECTION_STATUS {
    NoDeviceConnected,
    DeviceConnected,

    /* failure codes, these map to fail reasons */
    DeviceFailedEnumeration,
    DeviceGeneralFailure,
    DeviceCausedOvercurrent,
    DeviceNotEnoughPower,
    DeviceNotEnoughBandwidth,
    DeviceHubNestedTooDeeply,
    DeviceInLegacyHub,
    DeviceEnumerating,
    DeviceReset
} USB_CONNECTION_STATUS, *PUSB_CONNECTION_STATUS;
#define ERROR_NOT_ENOUGH_MEMORY          8L    // dderror

#define ERROR_NO_MORE_ITEMS              259L





#define HID_SET_REPORT                  0x09
#define HID_REPORT_TYPE_OUTPUT          0x02
#define BLTC_CMD_INQUIRY                0x01
#define BLTC_CMD_DOWNLOAD_FIRMWARE      0x02

#define BLTC_PAGE_INQUIRY_CHIP_INFO     0x01












#else

#endif
