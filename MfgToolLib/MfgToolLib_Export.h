/*
 * Copyright (C) 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#define USE_UNICODE

/************************************************************
* Error Code
************************************************************/
#define MFGLIB_ERROR_SUCCESS							0
#define MFGLIB_ERROR_UNKNOWN							1
#define MFGLIB_ERROR_ALREADY_EXIST						2
#define MFGLIB_ERROR_NO_MEMORY							3
#define MFGLIB_ERROR_FILE_NOT_EXIST						4
#define MFGLIB_ERROR_INVALID_PARAM						5
#define MFGLIB_ERROR_INVALID_VALUE						6
#define MFGLIB_ERROR_CMD_EXECUTE_FAILED					7
#define MFGLIB_ERROR_SUCCESS_UPDATE_COMPLETE			8
#define MFGLIB_ERROR_SIZE_IS_SMALL						9
#define MFGLIB_ERROR_XML_NO_CFG_ITEM					10
#define MFGLIB_ERROR_XML_NO_STATE_ITEM					11
#define MFGLIB_ERROR_XML_NO_MATCHED_LIST				12
#define MFGLIB_ERROR_XML_NO_CMDS_IN_LIST				13
#define MFGLIB_ERROR_INVALID_OPERATION_ID				14
#define MFGLIB_ERROR_LOG_FILE_OPEN_FAILED				15
#define MFGLIB_ERROR_XML_CMD_NO_STATE					16
#define MFGLIB_ERROR_XML_CMD_NO_TYPE					17
#define MFGLIB_ERROR_DEV_MANAGER_IS_RUNNING				18
#define MFGLIB_ERROR_THREAD_CREATE_FAILED				19
#define MFGLIB_ERROR_DEV_MANAGER_RUN_FAILED				20
#define MFGLIB_ERROR_FILE_OPEN_FAILED					21
#define MFGLIB_ERROR_NO_FILENAME						22
#define MFGLIB_ERROR_INVALID_STATE_NAME					23
#define MFGLIB_ERROR_INVALID_DEV_NAME					24
#define MFGLIB_ERROR_CONDITION_NOT_READY				25
#define MFGLIB_ERROR_PHASE_NOT_PARSED					26
#define MFGLIB_ERROR_CALLBACK_INVALID_TYPE				27
#define MFGLIB_ERROR_NOT_FIND							28
#define MFGLIB_ERROR_ALREADY_INITIALIZED				29
#define MFGLIB_ERROR_PROFILE_NOT_SET					30
#define MFGLIB_ERROR_LIST_NOT_SET						31
#define MFGLIB_ERROR_NOT_INITIALIZED					32
#define MFGLIB_ERROR_SKIP								33

/************************************************************
* Type definition
************************************************************/
#ifdef  USE_UNICODE
typedef wchar_t CHAR_t;
typedef wchar_t BYTE_t;
#else
typedef char CHAR_t;
typedef unsigned char BYTE_t;
#endif

typedef void *INSTANCE_HANDLE;

/************************************************************
* MfgTool can support max boards simultaneously
************************************************************/
#define MAX_BOARD_NUMBERS        4
/************************************************************
* MfgToolLib can support max characters in a character buffer
************************************************************/
#define MAX_CHAR_NUMBERS         260

/************************************************************
* Struct&Class definition
************************************************************/
typedef enum _Device_Change_Event
{
	MX_DEVICE_ARRIVAL_EVT,
	MX_VOLUME_ARRIVAL_EVT,
	MX_DEVICE_REMOVAL_EVT,
	MX_VOLUME_REMOVAL_EVT,
	MX_HUB_ARRIVAL_EVT,
	MX_HUB_REMOVAL_EVT
} MX_DEVICE_CHANGE_EVENT;

typedef enum _Device_State
{
	MX_BOOTSTRAP = 0,
	MX_UPDATER,
	MX_DISCONNECTED = 10,
} MX_DEVICE_STATE;

typedef enum _Device_Type
{
	DEV_HID_MX6Q = 0,
	DEV_HID_MX6D,
	DEV_HID_MX6SL,
	DEV_HID_MX6SX,
	DEV_HID_MX28,
	DEV_MSC_UPDATER,
	DEV_UNKNOWN
} MX_DEVICE_TYPE;

typedef enum _Callback_Type
{
	DeviceChange = 0,
	OperateResult
} CALLBACK_TYPE;

typedef struct _Device_Change_Notify
{
	DWORD OperationID;
    BYTE_t Hub[MAX_CHAR_NUMBERS];
    int HubIndex;
	int PortIndex;
    MX_DEVICE_CHANGE_EVENT Event;
	BYTE_t DeviceDesc[MAX_CHAR_NUMBERS];
	BYTE_t DriverLetter;
} DEVICE_CHANGE_NOTIFY;

#define NO_ONE_COMMAND_PROGRESS_RANGE				-1
#define COMMAND_STATUS_EXECUTE_COMPLETE				1
#define COMMAND_STATUS_EXECUTE_ERROR				2
#define COMMAND_STATUS_EXECUTE_RUNNING				3
typedef struct _Operate_Result
{
	DWORD OperationID;
	BYTE_t cmdInfo[MAX_CHAR_NUMBERS];
	int cmdIndex;
	int cmdStatus;
	BOOL bProgressWithinCommand;
	int DoneWithinCommand;
	int TotalWithinCommand;
	int CurrentPhaseIndex;
} OPERATE_RESULT;

typedef struct _Operation_Information
{
	DWORD OperationID;
	BOOL bConnected;
	BYTE_t HubName[MAX_CHAR_NUMBERS];
	int HubIndex;
	int PortIndex;
	BYTE_t DeviceDesc[MAX_CHAR_NUMBERS];
	MX_DEVICE_STATE ConnectedDeviceState;
} OPERATION_INFOMATION;

typedef struct _Operations_Information
{
	int OperationInfoNumbers;
	OPERATION_INFOMATION *pOperationInfo;
} OPERATIONS_INFORMATION;

typedef struct _Phase_Information
{
	BYTE_t szName[MAX_PATH];
	int index;
	UINT uiVid;
	UINT uiPid;
	UINT uiPhaseCommandNumbers;
} PHASE_INFORMATION;

typedef struct _Phases_Information
{
	int PhaseInfoNumbers;
	PHASE_INFORMATION *pPhaseInfo;
} PHASES_INFORMATION;

typedef void (*PCALLBACK_OPERATE_RESULT)(OPERATE_RESULT *pNsInfo);
typedef void (*PCALLBACK_DEVICE_CHANGE)(DEVICE_CHANGE_NOTIFY *pNsInfo);

/************************************************************
* Export Functions declaration
************************************************************/
DWORD MfgLib_Initialize();
DWORD MfgLib_Uninitialize();
DWORD MfgLib_CreateInstanceHandle(INSTANCE_HANDLE *pHandle);
DWORD MfgLib_DestoryInstanceHandle(INSTANCE_HANDLE handle);
DWORD MfgLib_SetProfileName(INSTANCE_HANDLE handle, BYTE_t *strName);
DWORD MfgLib_SetListName(INSTANCE_HANDLE handle, BYTE_t *strName);
DWORD MfgLib_SetUCLFile(INSTANCE_HANDLE handle, BYTE_t *strName);
DWORD MfgLib_SetMaxBoardNumber(INSTANCE_HANDLE handle, int boardNum);
DWORD MfgLib_InitializeOperation(INSTANCE_HANDLE handle);
DWORD MfgLib_UninitializeOperation(INSTANCE_HANDLE handle);
DWORD MfgLib_StartOperation(INSTANCE_HANDLE handle, DWORD OperationID);
DWORD MfgLib_StopOperation(INSTANCE_HANDLE handle, DWORD OperationID);
DWORD MfgLib_GetOperationInformation(INSTANCE_HANDLE handle, OPERATIONS_INFORMATION *pOperationsInfo);
DWORD MfgLib_GetPhaseInformation(INSTANCE_HANDLE handle, PHASES_INFORMATION *pPhasesInfo);
DWORD MfgLib_GetTotalCommandNumbers(INSTANCE_HANDLE handle, UINT *Number);
DWORD MfgLib_RegisterCallbackFunction(INSTANCE_HANDLE handle, CALLBACK_TYPE cbType, void *pFunc);
DWORD MfgLib_UnregisterCallbackFunction(INSTANCE_HANDLE handle, CALLBACK_TYPE cbType, void *pFunc);
DWORD MfgLib_GetLibraryVersion(BYTE_t* version, int maxSize);
DWORD MfgLib_SetUCLKeyWord(CHAR_t *key, CHAR_t *value);



