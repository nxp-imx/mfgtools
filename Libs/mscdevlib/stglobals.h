// StGlobals.h: interface for the CStGlobals class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STGLOBALS_H__90D08B71_ECD8_4C17_BA53_6EFB360CD467__INCLUDED_)
#define AFX_STGLOBALS_H__90D08B71_ECD8_4C17_BA53_6EFB360CD467__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef enum _PLATFORM {
	OS_98 = 0,
	OS_ME,
	OS_2K,
	OS_XP,
    OS_XP64,
	OS_MAC9,
	OS_MACX,
    OS_VISTA32,
    OS_VISTA64,
	OS_WINDOWS7,
	OS_UNSUPPORTED
} PLATFORM;

typedef enum _MESSAGE_TYPE {
	MSG_TYPE_ERROR = 0,
	MSG_TYPE_ERROR_UNDEFINED = 1,
	MSG_TYPE_INFO,
	MSG_TYPE_WARNING,
	MSG_TYPE_QUESTION
} MESSAGE_TYPE, *PMESSAGE_TYPE;

typedef enum _SCSI_STATE {
	SCSI_STATE_UNINITIALIZED,
	SCSI_STATE_INITIALIZED
} SCSI_STATE;

typedef OVERLAPPED ST_OVERLAPPED;
typedef HANDLE ST_HANDLE;
typedef UCHAR ST_BOOLEAN;

#pragma warning( push )
#pragma warning( disable : 4201 )

#define _NTSRB_     // to keep srb.h from being included
#include <scsi.h>

#pragma warning( pop )

/*
typedef union _EIGHT_BYTE {

    struct {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
        UCHAR Byte4;
        UCHAR Byte5;
        UCHAR Byte6;
        UCHAR Byte7;
    };

    ULONGLONG AsULongLong;
} EIGHT_BYTE, *PEIGHT_BYTE;
*/
class CStGlobals : public CStBase
{

public:
    BOOLEAN     m_bMTPSupported;

	CStGlobals(string name="CStGlobals");
	virtual ~CStGlobals();
	static ST_ERROR SpacesToUnderScores(string& strParm);
    static long GetLastError();
    static PLATFORM GetPlatform();
	static size_t Max(size_t, size_t);
	static ST_ERROR MakeMemoryZero(PUCHAR buf, size_t size);

	static HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, 
		LPCWSTR lpName);
	
	static HANDLE CreateFile(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
		HANDLE hTemplateFile);
	
	static BOOL DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, 
		DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, 
		LPOVERLAPPED lpOverlapped);

	static BOOL WriteFileEx(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
		LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

	static DWORD WaitForSingleObjectEx(HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable);

	static BOOL SetEvent(HANDLE hEvent); 

	static BOOL ResetEvent(HANDLE hEvent); 

	static BOOL CancelIo(HANDLE hFile);

};

#endif // !defined(AFX_STGLOBALS_H__90D08B71_ECD8_4C17_BA53_6EFB360CD467__INCLUDED_)
