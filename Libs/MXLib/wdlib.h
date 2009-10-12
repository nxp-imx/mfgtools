#ifndef _WDLIB_H
#define _WDLIB_H

#if defined(__cplusplus)
    extern "C" {
#endif

#define WDL_VER 600
#define WDL_VER_STR  "WDLib v6.00 Jungo (c) 2002 - 2003 Build Date: " __DATE__

typedef unsigned int WDL_HANDLE;
typedef unsigned int WDL_TIMEOUT;

#if defined(WIN32)
    #define DllExport __declspec(dllexport)
    #define WDL_TIMEOUT_FOREVER INFINITE
#else
    #define DllExport
    #define WDL_TIMEOUT_FOREVER 0xffffffff
#endif
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

enum WDL_STATUS { 
    WDL_SUCCESS = 0, 
    WDL_FAIL = 0x21000001, 
    WDL_TIMEOUT_EXPIRED = 0x21000002,
    WDL_FAILED_TO_REGISTER_NOTIFICATIONS = 0x21000003,
    WDL_INVALID_HANDLE = 0x21000004, 
    WDL_BUFFER_TOO_SMALL = 0x21000005, 
    WDL_DEVICE_NOT_CONNECTED = 0x21000006,
    WDL_NO_LICENSE = 0x21000007,
}; 

typedef void (__cdecl *WDL_CALLBACK)(WDL_HANDLE handle, void *context);

typedef struct WDL_CALLBACK_OPS
{ 
    WDL_CALLBACK cbConnect;     // connect
    WDL_CALLBACK cbDisconnect;  // disconnect
    WDL_CALLBACK cbPower;       // power change (N/A)
    WDL_CALLBACK cbDataAvailable;       // Read data available (N/A)
    WDL_CALLBACK cbDataSent;    // Write data sent (N/A)
} WDL_CALLBACK_OPS;

DllExport const char *WDL_Stat2Str(WDL_STATUS status);

DllExport WDL_STATUS WDL_Version(u32 *version_number, const char **version_string);

DllExport WDL_STATUS WDL_Init(WDL_HANDLE *handle, u16 vid, u16 pid,
    void *context, char *license);

DllExport WDL_STATUS WDL_Close(WDL_HANDLE handle);

DllExport WDL_STATUS WDL_Read(WDL_HANDLE handle, int reportid, u8 *buffer,
    size_t max_buffer_size, size_t *buffer_read_size, WDL_TIMEOUT timeout);

DllExport WDL_STATUS WDL_Write(WDL_HANDLE handle, int reportid, u8 *buffer,
    size_t buffer_size, WDL_TIMEOUT timeout);

DllExport WDL_STATUS WDL_GetFeature(WDL_HANDLE handle, int reportid,
    u8 *buffer, size_t max_buffer_size);

DllExport WDL_STATUS WDL_SetFeature(WDL_HANDLE handle, int reportid,
    u8 *buffer, size_t max_buffer_size);

DllExport WDL_STATUS WDL_IsAttached(WDL_HANDLE handle);

DllExport WDL_STATUS WDL_SetNotificationCallback(WDL_HANDLE handle,
    WDL_CALLBACK_OPS *ops);

DllExport WDL_STATUS WDL_CheckHandle(WDL_HANDLE handle);

#ifdef __cplusplus
}
#endif
#endif
