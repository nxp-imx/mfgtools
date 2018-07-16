/*
 * Copyright 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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

// MfgToolLib.h : main header file for the MfgToolLib DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#include <vector>
#include <map>
#include "XMLite.h"
#include "MfgToolLib_Export.h"
#include "CallbackDef.h"
#include "DeviceClass.h"
#include "UsbPort.h"
#include "UpdateTransportProtocol.h"
#include "CmdOperation.h"
#include "MxHidDevice.h"

// CMfgToolLibApp
// See MfgToolLib.cpp for the implementation of this class
//

class CMfgToolLibApp : public CWinApp
{
public:
	CMfgToolLibApp();

	CString m_strDllFullPath;

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

/************************************************************
* constant definition
************************************************************/
#define LOG_MODULE_MAIN_APP					1
#define LOG_MODULE_MFGTOOL_LIB				2

#define LOG_LEVEL_FATAL_ERROR				1
#define LOG_LEVEL_WARNING					5
#define LOG_LEVEL_NORMAL_MSG				10

#define RETRY_COUNT							20

#define DEFAULT_TIMEOUT_CHANGE_STATE		180

#define DEFAULT_UCL_XML_FILE_NAME			_T("ucl2.xml")

/************************************************************
* Struct&Class definition
************************************************************/
typedef struct _tagCfgParam
{
	CString	chip;
	CString	list;
} CFG_PARAMETER;

#define ROM_INFO_HID					   0x1
#define ROM_INFO_HID_MX23				   0x2
#define ROM_INFO_HID_MX50				   0x4
#define ROM_INFO_HID_MX6				   0x8
#define ROM_INFO_HID_SKIP_DCD			  0x10
#define ROM_INFO_HID_MX8_MULTI_IMAGE	  0x20
#define ROM_INFO_HID_MX8_STREAM			  0x40
#define ROM_INFO_HID_UID_STRING			  0x80
#define ROM_INFO_HID_SYSTEM_ADDR_MAP	 0x100     /*MX8QM use system view address map*/
#define ROM_INFO_HID_ECC_ALIGN			 0x200
#define ROM_INFO_HID_NO_CMD				 0x400


struct ROM_INFO
{
	TCHAR * Name;
	UINT    FreeAddr;
	DeviceClass *pDeviceClass;
	UINT	flags;
};

// <LIST ...> ... </LIST>
class CCommandList : public XNode
{
public:
	// [XmlAttribute("name")]
	CString GetName();
	// [XmlAttribute("desc")]
	CString GetDescription();
};
//a class for ucl.xml file, this is the toppest class
class CUclXml : public XNode
{
public:
	// [XmlElement("LIST")]
	CCommandList* GetCmdListNode(LPCTSTR name);
};

class COpState
{
public:
	COpState()
	{
		memset(&romInfo, 0, sizeof(ROM_INFO));
		uiVid = 0;
		uiPid = 0;
		bcdDevice = -1;
		dwTimeout = -1;
	};
	MX_DEVICE_STATE opState;
	CString strStateName;
	CString strDevice;
	ROM_INFO romInfo;
	UINT uiVid;
	UINT uiPid;
	INT bcdDevice;
	DWORD dwTimeout;   // waiting for change to next state(for example: the timeout of find command)
};
typedef std::vector<COpState*> OP_STATE_ARRAY;

enum HAB_t
{
	HabUnknown = -1,
	HabEnabled = 0x12343412,
	HabDisabled = 0x56787856
};

//command class family
class COpCommand
{
public:
	COpCommand();
	virtual ~COpCommand();
	virtual UINT ExecuteCommand(int index);
	virtual void SetBodyString(int index, const CString &str);
	virtual void SetTemplateBodyString(const CString &str);
	virtual CString GetBodyString(int index) const;
	virtual CString GetTemplateBodyString() const;
	virtual void SetDescString(CString &str);
	virtual CString GetDescString();
	virtual void SetIfDevString(CString &str);
	virtual void SetIfHabString(CString &str);
	virtual bool IsRun(CString &dev);
	virtual bool IsRun(HAB_t habState);
	INSTANCE_HANDLE m_pLibVars;

protected:
	std::map<int, CString> m_bodyStringMap;
	CString m_templateBodyString;
	CString m_descString;
	CString m_ifdev;
	CString m_ifhab;
};

class COpCmd_Find : public COpCommand
{
public:
	virtual UINT ExecuteCommand(int index);
	void SetTimeout(CString &strTO);
	UINT GetTimeout() const
	{
		return m_timeout;
	}

private:
	UINT m_timeout;
};

class COpCmd_Boot : public COpCommand
{
public:
	COpCmd_Boot();
	virtual ~COpCmd_Boot();
	virtual UINT ExecuteCommand(int index);
	UINT SetFileMapping(CString &strFile);
	void CloseFileMapping();
	CString GetFilename() const
	{
		return m_FileName;
	}

private:
	CString m_FileName;
	UCHAR *m_pDataBuf;
	__int64 m_qwFileSize;
};

class COpCmd_Init : public COpCommand
{
public:
	COpCmd_Init();
	virtual ~COpCmd_Init();
	virtual UINT ExecuteCommand(int index);
	UINT SetFileMapping(CString &strFile);
	void CloseFileMapping();
	CString GetFilename() const
	{
		return m_FileName;
	}

private:
	CString m_FileName;
	UCHAR *m_pDataBuf;
	__int64 m_qwFileSize;
};

class COpCmd_Load : public COpCommand
{
public:
	COpCmd_Load();
	virtual ~COpCmd_Load();
	virtual UINT ExecuteCommand(int index);
	UINT SetFileMapping(CString &strFile);
	void CloseFileMapping();
	void SetAddress(CString &strAddr);
	void SetloadSection(CString &str);
	void SetsetSection(CString &str);
	void SetIsHasFlashHeader(CString &str);
	DWORD GetFileDataSize();
private:
	CString m_FileName;
	UINT m_address;
	CString m_strloadSection;
	CString m_strsetSection;
	BOOL m_bHasFlashHeader;
	__int64 m_qwFileSize;
	UCHAR *m_pDataBuf;
	int m_iPercentComplete;
};

class COpCmd_Jump : public COpCommand
{
public:
	virtual UINT ExecuteCommand(int index);
	BOOL m_bIngoreError;
};

class COpCmd_Push : public COpCommand
{
public:
	COpCmd_Push();
	virtual ~COpCmd_Push();
	virtual UINT ExecuteCommand(int index);
	UINT SetFileMapping(CString &strFile);
	void CloseFileMapping();
	BOOL m_bIngoreError;
	CString m_SavedFileName;
private:
	CString m_FileName;
	__int64 m_qwFileSize;
	UCHAR *m_pDataBuf;
};

class COpCmd_Blhost : public COpCommand
{
public:
	COpCmd_Blhost();
	virtual ~COpCmd_Blhost();

	enum KibbleStatusCode
	{
		KBL_Status_Success = 0x0,
		KBL_Status_AbortDataPhase = 0x2712,
		KBL_Status_UnknownProperty = 0x283c,
	};

	enum ResultBufferSize
	{
		Result_Buffer_Size = 1024,
	};
	virtual UINT ExecuteCommand(int index);
	virtual UINT GetSecureState(int index, HAB_t *secureState);
	UINT SetFileMapping(CString &strFile);
	bool SetTimeout(const CString strValue);
	bool SetTimeout(const uint32_t value);
	void CloseFileMapping();
	BOOL m_bIngoreError;
	CString m_SavedFileName;
private:
	CString m_FileName;
	uint32_t m_timeout;
	__int64 m_qwFileSize;
	UCHAR *m_pDataBuf;
	struct BLHOST_RESULT {
		CString command;
		CString Response;
		CString Result_description;
		int error_code;
	};
	DWORD ExecuteBlhostCommand(CString csArguments, CString& out_text);
	void Parse_blhost_output_for_error_code(CString& jsonStream, BLHOST_RESULT* pblhostResult);
	void Parse_blhost_output_for_response(CString& jsonStream, BLHOST_RESULT* pblhostResult);
};

/*
class COpCmd_Burn : public COpCommand
{
public:
	COpCmd_Burn();
	virtual ~COpCmd_Burn();
	virtual UINT ExecuteCommand(int index);
	void SetFileName(CString strFile);
private:
	CString m_FileName;
};
*/
typedef std::vector<COpCommand*> OP_COMMAND_ARRAY;
typedef std::map<MX_DEVICE_STATE, OP_COMMAND_ARRAY> StateCommansMap_t;

typedef struct _PortDevInfo
{
	BOOL m_bUsed;
	BOOL m_bConnected;
	CString hubPath;
	int hubIndex;
	int portIndex;
	CString DeviceDesc;
	CString SerialId;
} PORT_DEV_INFO;

typedef struct _t_lib_vars
{
	CUclXml* g_pXmlHandler;
	CString g_strPath;
	CString g_strUclFilename;
	CFG_PARAMETER g_CfgParam;
	int g_iMaxBoardNum;
	CCmdOpreation *g_CmdOperationArray[MAX_BOARD_NUMBERS];
	DWORD g_CmdOpThreadID[MAX_BOARD_NUMBERS];
	HANDLE g_hDevCanDeleteEvts[MAX_BOARD_NUMBERS];
	OP_STATE_ARRAY g_OpStates;
	StateCommansMap_t g_StateCommands;
	PORT_DEV_INFO g_PortDevInfoArray[MAX_BOARD_NUMBERS];
	void* g_cbDevChangeHandle[MAX_BOARD_NUMBERS];
	void* g_cbOpResultHandle[MAX_BOARD_NUMBERS];
} MFGLIB_VARS;

typedef void (*PINTERNALCALLBACK)(INSTANCE_HANDLE handle, DeviceClass::NotifyStruct *pnsinfo);
/*
typedef struct _port_list
{
	CString hubPath;
	int hubIndex;
	int portIndex;
	PORT_ID portID;
	Device *pHubDevice;
} USB_PORT_NODE;
*/
/************************************************************
* Global variables declaration
************************************************************/
extern CMfgToolLibApp theApp;
/*
extern CUclXml* g_pXmlHandler;
extern CString g_strPath;
extern CString g_strUclFilename;
extern CFG_PARAMETER g_CfgParam;
extern int g_iMaxBoardNum;
extern std::vector<CCmdOpreation*> g_CmdOperationArray;
extern HANDLE g_hDevCanDeleteEvts[MAX_BOARD_NUMBERS];
extern PORT_DEV_INFO g_PortDevInfoArray[MAX_BOARD_NUMBERS];
extern OP_STATE_ARRAY g_OpStates;
extern StateCommansMap_t g_StateCommands;
*/
extern std::vector<MFGLIB_VARS *> g_LibVarsArray;
extern HANDLE g_hOneInstance;
//extern std::vector<USB_PORT_NODE *> g_PortTable;
/************************************************************
* Function declaration
************************************************************/
DWORD InitLogManager();
void DeinitLogManager();
void LogMsg(DWORD moduleID, DWORD levelID, const wchar_t * format, ... );
DWORD ParseUclXml(MFGLIB_VARS *pLibVars);
void ReleaseUclCommands(MFGLIB_VARS *pLibVars);
DWORD InitCmdOperation(MFGLIB_VARS *pLibVars, int WndIndex);
void DeinitCmdOperation(MFGLIB_VARS *pLibVars, int WndIndex);
DWORD InitDeviceManager(MFGLIB_VARS *pLibVars);
void DeinitDeviceManager();
void AutoScanDevice(MFGLIB_VARS *pLibVars, int iPortUsedNums);
OP_STATE_ARRAY *GetOpStates(MFGLIB_VARS *pLibVars);
UINT GetStateCommandSize(MFGLIB_VARS *pLibVars, MX_DEVICE_STATE currentState);
DWORD GetOpStatesTimeout(OP_STATE_ARRAY *pOpStateArray, MX_DEVICE_STATE _state);
void RegisterUIInfoUpdateCallback(MFGLIB_VARS *pLibVars, OperateResultUpdateStruct *pCB);
void UnregisterUIInfoUpdateCallback(MFGLIB_VARS *pLibVars, int DeviceIndex);
void RegisterUIDevChangeCallback(MFGLIB_VARS *pLibVars, DeviceChangeCallbackStruct *pCB);
void UnregisterUIDevChangeCallback(MFGLIB_VARS *pLibVars, int DeviceIndex);
void gOnDeviceChangeNotify(INSTANCE_HANDLE handle, DeviceClass::NotifyStruct *pNsInfo);
DWORD GetCurrentDeviceDesc(MFGLIB_VARS *pLibVars, int DeviceIndex, TCHAR* desc, int maxSize);
BOOL FindLibraryHandle(MFGLIB_VARS *pLibVars);
int FindOperationIndex(MFGLIB_VARS *pLibVars, DWORD operationID);
CString ReplaceKeywords(CString str);
CString ReplaceUsbPortKeywords(CString str, UINT hub, UINT port);
void ReplaceAllUsbPortKeyWords(MFGLIB_VARS *pLibVars, int WndIndex);





