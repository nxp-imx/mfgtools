/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "SetupApi.h"
#include<assert.h>
#include "WindowsVersionInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// Why do we have to go through this? A: Macro expansion rules.
#define STRIZE2(A)		#A
#define STRIZE(A)		STRIZE2(A)
#define FINDPROC(A) if ((A = (P ## A)GetProcAddress(hModuleSetup, STRIZE(A))) == NULL) available = FALSE
#define FINDPROCH(H,A) if ((A = (P ## A)GetProcAddress(H, STRIZE(A))) == NULL) available = FALSE

SetupApi::SetupApi(void)
: SetupDiCreateDeviceInfoList(NULL),
  SetupDiGetClassDescriptionA(NULL),
  SetupDiGetClassImageIndex(NULL),
  SetupDiGetClassImageList(NULL),
  SetupDiDestroyClassImageList(NULL),
  SetupDiOpenDeviceInfoA(NULL),
  SetupDiGetDeviceRegistryPropertyA(NULL),
  SetupDiGetDeviceInterfaceDetailA(NULL),
  SetupDiGetDeviceInstanceIdA(NULL),
  CM_Get_Device_IDA(NULL),
  CM_Request_Device_EjectA(NULL),
  SetupDiGetClassDevsA(NULL),
  SetupDiEnumDeviceInterfaces(NULL),
  SetupDiEnumDeviceInfo(NULL),
  SetupDiDestroyDeviceInfoList(NULL),
  CM_Get_Child(NULL),
  CM_Get_Sibling(NULL),
  CM_Get_Parent(NULL),
  CM_Get_Device_ID_Size(NULL),
  CM_Get_DevNode_Status(NULL),
  CM_Open_DevNode_Key(NULL),
#if defined(UNICODE)
  SetupDiGetClassDescriptionW(NULL),
  SetupDiOpenDeviceInfoW(NULL),
  SetupDiGetDeviceInterfaceDetailW(NULL),
  SetupDiGetClassDevsW(NULL),
  SetupDiGetDeviceRegistryPropertyW(NULL),
  SetupDiGetDeviceInstanceIdW(NULL),
  CM_Get_Device_IDW(NULL),
  CM_Request_Device_EjectW(NULL),
#endif
  available(false),
  hModuleSetup(NULL),
  hModuleCfgMgr(NULL)
{
//    TRACEC(TRACE_USBENUM, "Enter SetupAPI constructor.\n");
    memset(this, 0, sizeof(*this));
    hModuleSetup = LoadLibrary(_T("SETUPAPI.dll"));
    if (hModuleSetup)
    {	    
		// We got the library, that should work no matter what
		// we're running on.
		FINDPROC(SetupDiCreateDeviceInfoList);
		FINDPROC(SetupDiGetClassImageIndex);
		FINDPROC(SetupDiGetClassImageList);
		FINDPROC(SetupDiDestroyClassImageList);
		FINDPROC(SetupDiEnumDeviceInterfaces);
		FINDPROC(SetupDiEnumDeviceInfo);
		FINDPROC(SetupDiDestroyDeviceInfoList);

		FINDPROC(SetupDiGetClassDescriptionA);
		FINDPROC(SetupDiOpenDeviceInfoA);
		FINDPROC(SetupDiGetDeviceInterfaceDetailA);
		FINDPROC(SetupDiGetClassDevsA);
		FINDPROC(SetupDiGetDeviceRegistryPropertyA);
		FINDPROC(SetupDiGetDeviceInstanceIdA);
		if (gWinVersionInfo().IsWinNT())
		{
			FINDPROC(CM_Get_DevNode_Registry_PropertyA);
			FINDPROC(CM_Get_Child);
			FINDPROC(CM_Get_Sibling);
			FINDPROC(CM_Get_Parent);
			FINDPROC(CM_Get_Device_IDA);
			FINDPROC(CM_Get_Device_ID_Size);
			FINDPROC(CM_Get_DevNode_Status);
			FINDPROC(CM_Request_Device_EjectA);
			FINDPROC(CM_Open_DevNode_Key);
		}
		else
		{
			// These functions live in a different dll on win98.
			hModuleCfgMgr = LoadLibrary(_T("CFGMGR32.dll"));
			if (hModuleCfgMgr)
			{	    
				FINDPROCH(hModuleCfgMgr, CM_Get_DevNode_Registry_PropertyA);
				FINDPROCH(hModuleCfgMgr, CM_Get_Child);
				FINDPROCH(hModuleCfgMgr, CM_Get_Sibling);
				FINDPROCH(hModuleCfgMgr, CM_Get_Parent);
				FINDPROCH(hModuleCfgMgr, CM_Get_Device_IDA);
				FINDPROCH(hModuleCfgMgr, CM_Get_Device_ID_Size);
				FINDPROCH(hModuleCfgMgr, CM_Get_DevNode_Status);
				FINDPROCH(hModuleCfgMgr, CM_Request_Device_EjectA); //w98 not available
				FINDPROCH(hModuleCfgMgr, CM_Open_DevNode_Key);
			}
		}

		// Get wide versions of functions if on unicode
#if defined(UNICODE)
		if (gWinVersionInfo().IsWinNT())
		{
			FINDPROC(CM_Get_DevNode_Registry_PropertyW);
			FINDPROC(SetupDiGetClassDescriptionW);
			FINDPROC(SetupDiOpenDeviceInfoW);
			FINDPROC(SetupDiGetDeviceInterfaceDetailW);
			FINDPROC(SetupDiGetClassDevsW);
			FINDPROC(SetupDiGetDeviceRegistryPropertyW);
			FINDPROC(SetupDiGetDeviceInstanceIdW);
			FINDPROC(CM_Get_Device_IDW);
			FINDPROC(CM_Request_Device_EjectW);
		}
#endif
			
		if (CM_Get_DevNode_Registry_PropertyA &&
			SetupDiCreateDeviceInfoList       &&
			SetupDiGetClassDescriptionA       &&
			SetupDiGetClassImageIndex         &&
			SetupDiGetClassImageList		  &&
			SetupDiDestroyClassImageList      &&
			SetupDiEnumDeviceInterfaces	      &&
			SetupDiEnumDeviceInfo             &&
			SetupDiDestroyDeviceInfoList	  &&
			SetupDiOpenDeviceInfoA            &&
			SetupDiGetDeviceInterfaceDetailA  &&
			SetupDiGetClassDevsA		      &&
			SetupDiGetDeviceRegistryPropertyA &&
			SetupDiGetDeviceInstanceIdA       &&
			CM_Get_Child                      &&
			CM_Get_Sibling                    &&
			CM_Get_Parent                     &&
			CM_Get_Device_IDA                 &&
			CM_Request_Device_EjectA          &&
			CM_Open_DevNode_Key)
		{
#if defined(UNICODE)	
			// if NT based
			if (gWinVersionInfo().IsWinNT())
			{
				// Check we have the wide versions as well
				if (CM_Get_DevNode_Registry_PropertyW &&
					SetupDiGetClassDescriptionW       &&
					SetupDiOpenDeviceInfoW            &&
					SetupDiGetDeviceInterfaceDetailW  &&
					SetupDiGetClassDevsW              &&	
					SetupDiGetDeviceRegistryPropertyW &&
					SetupDiGetDeviceInstanceIdW       &&
					CM_Get_Device_IDW                 &&
					CM_Request_Device_EjectW)
				{
					available = true;
				}
			}
			else
			{
				available = true;
			}
#else
			available = true;
#endif
		}
	}
//    TRACEC(TRACE_USBENUM, "Leave SetupAPI constructor, hModule=%p.\n", hModule);
}

SetupApi::~SetupApi(void)
{
//    TRACEC(TRACE_USBENUM, "Enter SetupAPI destructor, hModule=%p.\n", hModule);
	if (hModuleSetup)
		FreeLibrary(hModuleSetup);

	if (hModuleCfgMgr)
		FreeLibrary(hModuleCfgMgr);

//    TRACEC(TRACE_USBENUM, "Leave SetupAPI destructor.\n");
}

HDEVINFO SetupApi::apiSetupDiGetClassDevs(LPCGUID  ClassGuid, 
								LPCTSTR  Enumerator, 
								HWND  hwndParent, 
								DWORD  Flags)
{
	USES_CONVERSION;
#if defined(UNICODE)
    // if on NT based ODS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		return SetupDiGetClassDevsW(ClassGuid, Enumerator, hwndParent, Flags);	
    }
    // else use ANSI versions
    else
    {
		// If have an enumerator convert string to ansi
		if (Enumerator != NULL)
		{
			std::string ansiEnum = W2A(Enumerator);
			return SetupDiGetClassDevsA(ClassGuid, ansiEnum.c_str(), hwndParent, Flags);	
		}
		else
		{
			return SetupDiGetClassDevsA(ClassGuid, NULL, hwndParent, Flags);	
		}
    }
#else
    return SetupDiGetClassDevs(ClassGuid, Enumerator, hwndParent, Flags);
#endif
}

BOOL SetupApi::apiSetupDiOpenDeviceInfo(
		HDEVINFO DeviceInfoSet,
		stdstring::PCTSTR DeviceInstanceId,
		HWND hwndParent,
		DWORD OpenFlags,
		PSP_DEVINFO_DATA DeviceInfoData)
{
#if defined(UNICODE)
	USES_CONVERSION;
    // if on NT based ODS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		return SetupDiOpenDeviceInfoW(DeviceInfoSet, DeviceInstanceId, hwndParent, OpenFlags, DeviceInfoData);	
    }
    // else use ANSI versions
    else
    {
		// convert DeviceInstanceId string to ansi
		std::string ansiDeviceInstanceId = W2A(DeviceInstanceId);
		return SetupDiOpenDeviceInfoA(DeviceInfoSet, ansiDeviceInstanceId.c_str(), hwndParent, OpenFlags, DeviceInfoData);	
    }
#else
    return SetupDiOpenDeviceInfo(DeviceInfoSet, DeviceInstanceId, hwndParent, OpenFlags, DeviceInfoData);
#endif
}

BOOL SetupApi::apiSetupDiGetDeviceInterfaceDetail(HDEVINFO  DeviceInfoSet,
	PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData,
	PSP_DEVICE_INTERFACE_DETAIL_DATA  DeviceInterfaceDetailData, 
	DWORD  DeviceInterfaceDetailDataSize,
	PDWORD  RequiredSize, 
	PSP_DEVINFO_DATA  DeviceInfoData)
{
	USES_CONVERSION;
#if defined(UNICODE)
    // If on NT based OS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		return SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet, DeviceInterfaceData,
			DeviceInterfaceDetailData, DeviceInterfaceDetailDataSize,
			RequiredSize, DeviceInfoData);	
    }
    // Else use ANSI version
    else
    {
		// Getting the data
		if (DeviceInterfaceDetailData != NULL && DeviceInterfaceDetailDataSize > 0)
		{
			// set up retrieval using ANSI vars
			BOOL bRet = FALSE;
			UINT32 charCount = (DeviceInterfaceDetailDataSize - sizeof(DWORD)) / 2;
			UINT32 ansiSize = charCount + sizeof(DWORD);
			PSP_DEVICE_INTERFACE_DETAIL_DATA_A ansiDetails = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)malloc(DeviceInterfaceDetailDataSize);
			ansiDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

			if (SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, DeviceInterfaceData,
			ansiDetails, ansiSize,
			0, DeviceInfoData))
			{
				// convert string to unicode
				std::wstring unicodePath = A2W(ansiDetails->DevicePath);
				_tcscpy_s(DeviceInterfaceDetailData->DevicePath, unicodePath.c_str());
				bRet = TRUE;	
			}	    

			free(ansiDetails);
			return bRet;
		}
		// Just getting the size
		else
		{
			BOOL bRet = SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, DeviceInterfaceData,
			NULL, NULL,
			RequiredSize, DeviceInfoData);
		    
			if (RequiredSize != NULL)
			{
				if (*RequiredSize > sizeof(DWORD))
				{
					*RequiredSize+= *RequiredSize - sizeof(DWORD);		
				}	    
			}

			return bRet;
		}
    }
#else
    return SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, DeviceInterfaceData,
	    DeviceInterfaceDetailData, DeviceInterfaceDetailDataSize,
	    RequiredSize, DeviceInfoData);
#endif
}

BOOL SetupApi::apiSetupDiGetDeviceRegistryProperty(
	HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData,
    DWORD Property,
    PDWORD PropertyRegDataType,
    PBYTE PropertyBuffer,
    DWORD PropertyBufferSize,
    PDWORD RequiredSize)
{
#if defined(UNICODE)
	USES_CONVERSION;
    // If on NT based OS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		return SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, Property,
			PropertyRegDataType, PropertyBuffer, PropertyBufferSize, RequiredSize);	
    }
    // Else use ANSI version
    else
    {
		// Getting the data
		if (PropertyBuffer != NULL && PropertyBufferSize > 0)
		{
			// set up retrieval using ANSI vars
			BOOL bRet = FALSE;

			if (bRet = SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, DeviceInfoData, Property,
				PropertyRegDataType, PropertyBuffer, PropertyBufferSize, RequiredSize))
			{
				assert (PropertyRegDataType != NULL);
				if ( *PropertyRegDataType == REG_SZ ||
					 *PropertyRegDataType == REG_EXPAND_SZ ||
					 *PropertyRegDataType == REG_MULTI_SZ )
				{
					// convert string to unicode
					// TODO: don't know how this works with REG_EXPAND_SZ or REG_MULTI_SZ
					std::string ansiPropertyBuffer = (PSTR)PropertyBuffer;
					std::wstring unicodePropertyBuffer = A2W(ansiPropertyBuffer.c_str());
//					unicodePropertyBuffer.append(1, _T('\0'));
					if ( PropertyBufferSize >= (unicodePropertyBuffer.size()+1)*sizeof(TCHAR) )
					{
						memcpy(PropertyBuffer, unicodePropertyBuffer.data(), (unicodePropertyBuffer.length()+1)*sizeof(TCHAR));
						bRet = true;
					}
					else
					{
						if ( PropertyBufferSize >= sizeof(DWORD) )
						{
							UCHAR zero = '\0';
							memcpy(PropertyBuffer, &zero, 4);
						}
						bRet = false;
						SetLastError(ERROR_INSUFFICIENT_BUFFER);
					}
				}
			}	    
			return bRet;
		}
		// Just getting the size
		else
		{
			BOOL bRet = SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, DeviceInfoData, Property,
				PropertyRegDataType, NULL, NULL, RequiredSize);
		    
			if (RequiredSize != NULL)
			{
				*RequiredSize = *RequiredSize * sizeof(TCHAR);		
		    }

			return bRet;
		}
    }
#else
	return SetupDiGetDeviceRegistryProperty(DeviceInfoSet, DeviceInfoData, Property,
		PropertyRegDataType, PropertyBuffer, PropertyBufferSize, RequiredSize);	
#endif
}

DWORD SetupApi::apiCM_Get_DevNode_Registry_Property(
	DEVINST dnDevInst,
    ULONG ulProperty,
    PULONG pulRegDataType,
    PVOID Buffer,
    PULONG pulLength,
    ULONG ulFlags)
{
#if defined(UNICODE)
	USES_CONVERSION;
    // If on NT based OS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		return CM_Get_DevNode_Registry_PropertyW(dnDevInst, ulProperty, pulRegDataType,
			Buffer, pulLength, ulFlags);	
    }
    // Else use ANSI version
    else
    {
		// Getting the data
		if (Buffer != NULL && *pulLength > 0)
		{
			ULONG submitted_buffer_size = *pulLength;

			// set up retrieval using ANSI vars
			DWORD dwRet = CM_Get_DevNode_Registry_PropertyA(dnDevInst, ulProperty, pulRegDataType,
				Buffer, pulLength, ulFlags);

			if (dwRet == ERROR_SUCCESS)
			{
				assert (pulRegDataType != NULL);
				if ( *pulRegDataType == REG_SZ ||
					 *pulRegDataType == REG_EXPAND_SZ ||
					 *pulRegDataType == REG_MULTI_SZ )
				{
					// convert string to unicode
					// TODO: don't know how this works with REG_EXPAND_SZ or REG_MULTI_SZ
					std::string ansiBuffer = (PSTR)Buffer;
					std::wstring unicodeBuffer = A2W(ansiBuffer.c_str());
					if ( submitted_buffer_size >= (unicodeBuffer.size()+1)*sizeof(TCHAR) )
					{
						memcpy(Buffer, unicodeBuffer.data(), (unicodeBuffer.length()+1)*sizeof(TCHAR));
						dwRet = CR_SUCCESS;
					}
					else
					{
						if (submitted_buffer_size >= sizeof(DWORD))
						{
							UCHAR zero = '\0';
							memcpy(Buffer, &zero, 4);
						}
						dwRet = ERROR_INSUFFICIENT_BUFFER;
					}
				}
				else
				{
					ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
					throw;
				}
			}

			return dwRet;
		}
		// Just getting the size
		else
		{
			BOOL dwRet = CM_Get_DevNode_Registry_PropertyA(dnDevInst, ulProperty, pulRegDataType,
				Buffer, pulLength, ulFlags);
		    
			if (pulLength != NULL)
			{
				*pulLength = *pulLength * sizeof(TCHAR);		
		    }

			return dwRet;
		}
    }
#else
	return CM_Get_DevNode_Registry_Property(dnDevInst, ulProperty, pulRegDataType,
				Buffer, pulLength, ulFlags);	
#endif
}

BOOL SetupApi::apiSetupDiGetDeviceInstanceId(
	HDEVINFO DeviceInfoSet,
	PSP_DEVINFO_DATA DeviceInfoData,
	PTSTR DeviceInstanceId,
	DWORD DeviceInstanceIdSize,
	PDWORD RequiredSize)
{
#if defined(UNICODE)
	USES_CONVERSION;
    // If on NT based OS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		return SetupDiGetDeviceInstanceIdW(DeviceInfoSet, DeviceInfoData,
			DeviceInstanceId, DeviceInstanceIdSize, RequiredSize);	
    }
    // Else use ANSI version
    else
    {
		// Getting the data
		if (DeviceInstanceId != NULL && DeviceInstanceIdSize > 0)
		{
			// set up retrieval using ANSI vars
			BOOL bRet = FALSE;
			UINT32 charCount = (DeviceInstanceIdSize - sizeof(DWORD)) / 2;
			UINT32 ansiSize = charCount + sizeof(DWORD);
			PSTR ansiDeviceInstanceId = (PSTR)malloc(ansiSize);

			if (SetupDiGetDeviceInstanceIdA(DeviceInfoSet, DeviceInfoData,
			ansiDeviceInstanceId, ansiSize, RequiredSize))
			{
				// convert string to unicode
				std::wstring unicodeDeviceInstanceId = A2W(ansiDeviceInstanceId);
				_tcscpy_s(DeviceInstanceId, ansiSize, unicodeDeviceInstanceId.c_str());
				bRet = TRUE;	
			}	    

			free(ansiDeviceInstanceId);
			return bRet;
		}
		// Just getting the size
		else
		{
			BOOL bRet = SetupDiGetDeviceInstanceIdA(DeviceInfoSet, DeviceInfoData,
			NULL, NULL,
			RequiredSize);
		    
			if (RequiredSize != NULL)
			{
				if (*RequiredSize > sizeof(DWORD))
				{
					*RequiredSize+= *RequiredSize - sizeof(DWORD);		
				}	    
		    }

			return bRet;
		}
    }
#else
	return SetupDiGetDeviceInstanceId(DeviceInfoSet, DeviceInfoData,
			DeviceInstanceId, DeviceInstanceIdSize, RequiredSize);
#endif
}

BOOL SetupApi::apiSetupDiGetClassDescription(
		LPGUID  ClassGuid,
		PTSTR  ClassDescription,
		DWORD  ClassDescriptionSize,
		PDWORD  RequiredSize)
{
#if defined(UNICODE)
	USES_CONVERSION;
    // If on NT based OS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		return SetupDiGetClassDescriptionW(ClassGuid, ClassDescription, ClassDescriptionSize, RequiredSize);	
    }
    // Else use ANSI version
    else
    {
		// Getting the data
		if (ClassDescription != NULL && ClassDescriptionSize > 0)
		{
			// set up retrieval using ANSI vars
			BOOL bRet = FALSE;
			UINT32 charCount = (ClassDescriptionSize - sizeof(DWORD)) / 2;
			UINT32 ansiSize = charCount + sizeof(DWORD);
			PSTR ansiClassDescription = (PSTR)malloc(ansiSize);

			if (SetupDiGetClassDescriptionA(ClassGuid, ansiClassDescription, ansiSize, RequiredSize))
			{
				// convert string to unicode
				std::wstring unicodeClassDescription = A2W(ansiClassDescription);
				_tcscpy_s(ClassDescription, ansiSize, unicodeClassDescription.c_str());
				bRet = TRUE;	
			}	    

			free(ansiClassDescription);
			return bRet;
		}
		// Just getting the size
		else
		{
			BOOL bRet = SetupDiGetClassDescriptionA(ClassGuid, NULL, NULL, RequiredSize);
		    
			if (RequiredSize != NULL)
			{
				if (*RequiredSize > sizeof(DWORD))
				{
					*RequiredSize+= *RequiredSize - sizeof(DWORD);		
				}	    
		    }

			return bRet;
		}
    }
#else
	return SetupDiGetClassDescription(ClassGuid, ClassDescription, ClassDescriptionSize, RequiredSize);
#endif
}

DWORD SetupApi::apiCM_Get_Device_ID(
	DEVINST  dnDevInst,
	CStdString& deviceInstanceId)
{
//  DEVINST  dnDevInst,
//  PWCHAR   Buffer,
//  ULONG    BufferLen,
//  ULONG    ulFlags
	DWORD error;

	ULONG BufferLen; // number of characters
	error = CM_Get_Device_ID_Size(&BufferLen, dnDevInst, 0);
	if ( error != CR_SUCCESS )
	{
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
		throw;
	}

	// add 1 character for terminating NULL
	++BufferLen;
   
#if defined(UNICODE)
	USES_CONVERSION;
    // If on NT based OS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		// allocate BufferLen * sizeof(wchar)
		PWCHAR Buffer = (PWCHAR)malloc(sizeof(WCHAR) * BufferLen);

		error = CM_Get_Device_IDW(dnDevInst, Buffer, BufferLen, 0);
		if ( error != CR_SUCCESS )
		{
			ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
			throw;
		}
		
		deviceInstanceId = Buffer;
		free(Buffer);
		return error;
    }
    // Else use ANSI version
    else
    {
		// allocate BufferLen * sizeof(char)
		PCHAR Buffer = (PCHAR)malloc(sizeof(CHAR) * BufferLen);

		error = CM_Get_Device_IDA(dnDevInst, Buffer, BufferLen, 0);
		if ( error != CR_SUCCESS )
		{
			ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
			throw;
		}
		
		deviceInstanceId = A2W(Buffer);
		free(Buffer);
		return error;
    }
#else
	// allocate BufferLen * sizeof(char)
	PCHAR Buffer = (PWCHAR)malloc(sizeof(CHAR) * BufferLen);

	error = CM_Get_Device_ID(dnDevInst, Buffer, BufferLen, 0);
	if ( error != CR_SUCCESS )
	{
		ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
		throw;
	}
	
	deviceInstanceId = Buffer;
	free(Buffer);
	return error;
#endif
}

DWORD SetupApi::apiCM_Request_Device_Eject(
	DEVINST dnDevInst,
	PPNP_VETO_TYPE pVetoType,
    PTSTR pszVetoName,
    ULONG ulNameLength)
{
#if defined(UNICODE)
	USES_CONVERSION;
    // If on NT based OS use Wide versions
    if (gWinVersionInfo().IsWinNT())
    {
		return CM_Request_Device_EjectW(dnDevInst, pVetoType, pszVetoName, ulNameLength, 0);	
    }
    // Else use ANSI version
    else
    {
		return CM_Request_Device_EjectA(dnDevInst, pVetoType, (PSTR)pszVetoName, ulNameLength*2, 0);
    }
#else
	return CM_Request_Device_Eject(dnDevInst, pVetoType, pszVetoName, ulNameLength, 0);
#endif
}

bool SetupApi::IsDevNodeOk( DEVINST dnDevInst )
{
	DWORD error;
	ULONG  status;
	ULONG  problemNumber;
	
	error = gSetupApi().CM_Get_DevNode_Status(&status, &problemNumber, dnDevInst, 0);
	if ( error == ERROR_SUCCESS)
	{
		if ( (status & DN_HAS_PROBLEM) == DN_HAS_PROBLEM )
		{
			ATLTRACE2(_T("ERROR! Device::GetProperty() - Devnode has a problem: %s\r\n"), gSetupApi().CM_GetProblemString(problemNumber).c_str());
			return false;
		}
	}
	else
	{
		ATLTRACE2(_T("ERROR! Device::GetProperty() - CM_Get_DevNode_Status() failed: %s\r\n"), gSetupApi().CR_GetErrorString(error).c_str());
		return false;
	}

	return true;
}

CStdString SetupApi::CR_GetErrorString( DWORD errorCode )
{
	CStdString errorStr = _T("");

	switch (errorCode)
	{
	case CR_SUCCESS:
		errorStr = _T("CR_SUCCESS(0)");
		break;
	case CR_DEFAULT:
		errorStr = _T("CR_DEFAULT(1)");
		break;
	case CR_OUT_OF_MEMORY:
		errorStr = _T("CR_OUT_OF_MEMORY(2)");
		break;
	case CR_INVALID_POINTER:
		errorStr = _T("CR_INVALID_POINTER(3)");
		break;
	case CR_INVALID_FLAG:
		errorStr = _T("CR_INVALID_FLAG(4)");
		break;
	case CR_INVALID_DEVNODE:
		errorStr = _T("CR_INVALID_DEVNODE(5)");
		break;
	case CR_INVALID_RES_DES:
		errorStr = _T("CR_INVALID_RES_DES(6)");
		break;
	case CR_INVALID_LOG_CONF:
		errorStr = _T("CR_INVALID_LOG_CONF(7)");
		break;
	case CR_INVALID_ARBITRATOR:
		errorStr = _T("CR_INVALID_ARBITRATOR(8)");
		break;
	case CR_INVALID_NODELIST:
		errorStr = _T("CR_INVALID_NODELIST(9)");
		break;
	case CR_DEVNODE_HAS_REQS:
		errorStr = _T("CR_DEVNODE_HAS_REQS(10)");
		break;
	case CR_INVALID_RESOURCEID:
		errorStr = _T("CR_INVALID_RESOURCEID(11)");
		break;
	case CR_DLVXD_NOT_FOUND:
		errorStr = _T("CR_DLVXD_NOT_FOUND(12)");
		break;
	case CR_NO_SUCH_DEVNODE:
		errorStr = _T("CR_NO_SUCH_DEVNODE(13)");
		break;
	case CR_NO_MORE_LOG_CONF:
		errorStr = _T("CR_NO_MORE_LOG_CONF(14)");
		break;
	case CR_NO_MORE_RES_DES:
		errorStr = _T("CR_NO_MORE_RES_DES(15)");
		break;
	case CR_ALREADY_SUCH_DEVNODE:
		errorStr = _T("CR_ALREADY_SUCH_DEVNODE(16)");
		break;
	case CR_INVALID_RANGE_LIST:
		errorStr = _T("CR_INVALID_RANGE_LIST(17)");
		break;
	case CR_INVALID_RANGE:
		errorStr = _T("CR_INVALID_RANGE(18)");
		break;
	case CR_FAILURE:
		errorStr = _T("CR_FAILURE(19)");
		break;
	case CR_NO_SUCH_LOGICAL_DEV:
		errorStr = _T("CR_NO_SUCH_LOGICAL_DEV(20)");
		break;
	case CR_CREATE_BLOCKED:
		errorStr = _T("CR_CREATE_BLOCKED(21)");
		break;
	case CR_NOT_SYSTEM_VM:
		errorStr = _T("CR_NOT_SYSTEM_VM(22)");
		break;
	case CR_REMOVE_VETOED:
		errorStr = _T("CR_REMOVE_VETOED(23)");
		break;
	case CR_APM_VETOED:
		errorStr = _T("CR_APM_VETOED(24)");
		break;
	case CR_INVALID_LOAD_TYPE:
		errorStr = _T("CR_INVALID_LOAD_TYPE(25)");
		break;
	case CR_BUFFER_SMALL:
		errorStr = _T("CR_BUFFER_SMALL(26)");
		break;
	case CR_NO_ARBITRATOR:
		errorStr = _T("CR_NO_ARBITRATOR(27)");
		break;
	case CR_NO_REGISTRY_HANDLE:
		errorStr = _T("CR_NO_REGISTRY_HANDLE(28)");
		break;
	case CR_REGISTRY_ERROR:
		errorStr = _T("CR_REGISTRY_ERROR(29)");
		break;
	case CR_INVALID_DEVICE_ID:
		errorStr = _T("CR_INVALID_DEVICE_ID(30)");
		break;
	case CR_INVALID_DATA:
		errorStr = _T("CR_INVALID_DATA(31)");
		break;
	case CR_INVALID_API:
		errorStr = _T("CR_INVALID_API(32)");
		break;
	case CR_DEVLOADER_NOT_READY:
		errorStr = _T("CR_DEVLOADER_NOT_READY(33)");
		break;
	case CR_NEED_RESTART:
		errorStr = _T("CR_NEED_RESTART(34)");
		break;
	case CR_NO_MORE_HW_PROFILES:
		errorStr = _T("CR_NO_MORE_HW_PROFILES(35)");
		break;
	case CR_DEVICE_NOT_THERE:
		errorStr = _T("CR_DEVICE_NOT_THERE(36)");
		break;
	case CR_NO_SUCH_VALUE:
		errorStr = _T("CR_NO_SUCH_VALUE(37)");
		break;
	case CR_WRONG_TYPE:
		errorStr = _T("CR_WRONG_TYPE(38)");
		break;
	case CR_INVALID_PRIORITY:
		errorStr = _T("CR_INVALID_PRIORITY(39)");
		break;
	case CR_NOT_DISABLEABLE:
		errorStr = _T("CR_NOT_DISABLEABLE(40)");
		break;
	case CR_FREE_RESOURCES:
		errorStr = _T("CR_FREE_RESOURCES(41)");
		break;
	case CR_QUERY_VETOED:
		errorStr = _T("CR_QUERY_VETOED(42)");
		break;
	case CR_CANT_SHARE_IRQ:
		errorStr = _T("CR_CANT_SHARE_IRQ(43)");
		break;
	case CR_NO_DEPENDENT:
		errorStr = _T("CR_NO_DEPENDENT(44)");
		break;
	case CR_SAME_RESOURCES:
		errorStr = _T("CR_SAME_RESOURCES(45)");
		break;
	case CR_NO_SUCH_REGISTRY_KEY:
		errorStr = _T("CR_NO_SUCH_REGISTRY_KEY(46)");
		break;
	case CR_INVALID_MACHINENAME:
		errorStr = _T("CR_INVALID_MACHINENAME(47)");
		break;
	case CR_REMOTE_COMM_FAILURE:
		errorStr = _T("CR_REMOTE_COMM_FAILURE(48)");
		break;
	case CR_MACHINE_UNAVAILABLE:
		errorStr = _T("CR_MACHINE_UNAVAILABLE(49)");
		break;
	case CR_NO_CM_SERVICES:
		errorStr = _T("CR_NO_CM_SERVICES(50)");
		break;
	case CR_ACCESS_DENIED:
		errorStr = _T("CR_ACCESS_DENIED(51)");
		break;
	case CR_CALL_NOT_IMPLEMENTED:
		errorStr = _T("CR_CALL_NOT_IMPLEMENTED(52)");
		break;
	case CR_INVALID_PROPERTY:
		errorStr = _T("CR_INVALID_PROPERTY(53)");
		break;
	case CR_DEVICE_INTERFACE_ACTIVE:
		errorStr = _T("CR_DEVICE_INTERFACE_ACTIVE(54)");
		break;
	case CR_NO_SUCH_DEVICE_INTERFACE:
		errorStr = _T("CR_NO_SUCH_DEVICE_INTERFACE(55)");
		break;
	case CR_INVALID_REFERENCE_STRING:
		errorStr = _T("CR_INVALID_REFERENCE_STRING(56)");
		break;
	case CR_INVALID_CONFLICT_LIST:
		errorStr = _T("CR_INVALID_CONFLICT_LIST(57)");
		break;
	case CR_INVALID_INDEX:
		errorStr = _T("CR_INVALID_INDEX(58)");
		break;
	case CR_INVALID_STRUCTURE_SIZE:
		errorStr = _T("CR_INVALID_STRUCTURE_SIZE(59)");
		break;
	default:
	case NUM_CR_RESULTS:
		errorStr = _T("NUM_CR_RESULTS(60)");
		break;
	}

	return errorStr;
}

CStdString SetupApi::CM_GetProblemString( DWORD problemCode )
{
	CStdString problemStr = _T("");

	switch (problemCode)
	{
	case CM_PROB_NOT_CONFIGURED:
		problemStr = _T("CM_PROB_NOT_CONFIGURED(1)");
		break;
	case CM_PROB_DEVLOADER_FAILED:
		problemStr = _T("CM_PROB_DEVLOADER_FAILED(2)");
		break;
	case CM_PROB_OUT_OF_MEMORY://              (0x00000003)   // out of memory
		problemStr = _T("CM_PROB_OUT_OF_MEMORY(3)");
		break;
	case CM_PROB_ENTRY_IS_WRONG_TYPE://        (0x00000004)   //
		problemStr = _T("CM_PROB_ENTRY_IS_WRONG_TYPE(4)");
		break;
	case CM_PROB_LACKED_ARBITRATOR://          (0x00000005)   //
		problemStr = _T("CM_PROB_LACKED_ARBITRATOR(5)");
		break;
	case CM_PROB_BOOT_CONFIG_CONFLICT://       (0x00000006)   // boot config conflict
		problemStr = _T("CM_PROB_BOOT_CONFIG_CONFLICT(6)");
		break;
	case CM_PROB_FAILED_FILTER://              (0x00000007)   //
		problemStr = _T("CM_PROB_FAILED_FILTER(7)");
		break;
	case CM_PROB_DEVLOADER_NOT_FOUND://        (0x00000008)   // Devloader not found
		problemStr = _T("CM_PROB_DEVLOADER_NOT_FOUND(8)");
		break;
	case CM_PROB_INVALID_DATA://               (0x00000009)   //
		problemStr = _T("CM_PROB_INVALID_DATA(9)");
		break;
	case CM_PROB_FAILED_START://               (0x0000000A)   //
		problemStr = _T("CM_PROB_FAILED_START(10)");
		break;
	case CM_PROB_LIAR://                       (0x0000000B)   //
		problemStr = _T("CM_PROB_LIAR(11)");
		break;
	case CM_PROB_NORMAL_CONFLICT://            (0x0000000C)   // config conflict
		problemStr = _T("CM_PROB_NORMAL_CONFLICT(12)");
		break;
	case CM_PROB_NOT_VERIFIED://               (0x0000000D)   //
		problemStr = _T("CM_PROB_NOT_VERIFIED(13)");
		break;
	case CM_PROB_NEED_RESTART://               (0x0000000E)   // requires restart
		problemStr = _T("CM_PROB_NEED_RESTART(14)");
		break;
	case CM_PROB_REENUMERATION://              (0x0000000F)   //
		problemStr = _T("CM_PROB_REENUMERATION(15)");
		break;
	case CM_PROB_PARTIAL_LOG_CONF://           (0x00000010)   //
		problemStr = _T("CM_PROB_PARTIAL_LOG_CONF(16)");
		break;
	case CM_PROB_UNKNOWN_RESOURCE://          (0x00000011)   // unknown res type
		problemStr = _T("CM_PROB_UNKNOWN_RESOURCE(17)");
		break;
	case CM_PROB_REINSTALL://                  (0x00000012)   //
		problemStr = _T("CM_PROB_REINSTALL(18)");
		break;
	case CM_PROB_REGISTRY://                   (0x00000013)   //
		problemStr = _T("CM_PROB_REGISTRY(19)");
		break;
	case CM_PROB_VXDLDR://                     (0x00000014)   // WINDOWS 95 ONLY
		problemStr = _T("CM_PROB_VXDLDR(20)");
		break;
	case CM_PROB_WILL_BE_REMOVED://            (0x00000015)   // devinst will remove
		problemStr = _T("CM_PROB_WILL_BE_REMOVED(21)");
		break;
	case CM_PROB_DISABLED://                   (0x00000016)   // devinst is disabled
		problemStr = _T("CM_PROB_DISABLED(22)");
		break;
	case CM_PROB_DEVLOADER_NOT_READY://        (0x00000017)   // Devloader not ready
		problemStr = _T("CM_PROB_DEVLOADER_NOT_READY(23)");
		break;
	case CM_PROB_DEVICE_NOT_THERE://           (0x00000018)   // device doesn't exist
		problemStr = _T("CM_PROB_DEVICE_NOT_THERE(24)");
		break;
	case CM_PROB_MOVED://                      (0x00000019)   //
		problemStr = _T("CM_PROB_MOVED(25)");
		break;
	case CM_PROB_TOO_EARLY://                  (0x0000001A)   //
		problemStr = _T("CM_PROB_TOO_EARLY(26)");
		break;
	case CM_PROB_NO_VALID_LOG_CONF://          (0x0000001B)   // no valid log config
		problemStr = _T("CM_PROB_NO_VALID_LOG_CONF(27)");
		break;
	case CM_PROB_FAILED_INSTALL://             (0x0000001C)   // install failed
		problemStr = _T("CM_PROB_FAILED_INSTALL(28)");
		break;
	case CM_PROB_HARDWARE_DISABLED://          (0x0000001D)   // device disabled
		problemStr = _T("CM_PROB_HARDWARE_DISABLED(29)");
		break;
	case CM_PROB_CANT_SHARE_IRQ://             (0x0000001E)   // can't share IRQ
		problemStr = _T("CM_PROB_CANT_SHARE_IRQ(30)");
		break;
	case CM_PROB_FAILED_ADD://                 (0x0000001F)   // driver failed add
		problemStr = _T("CM_PROB_FAILED_ADD(31)");
		break;
	case CM_PROB_DISABLED_SERVICE://           (0x00000020)   // service's Start = 4
		problemStr = _T("CM_PROB_DISABLED_SERVICE(32)");
		break;
	case CM_PROB_TRANSLATION_FAILED://         (0x00000021)   // resource translation failed
		problemStr = _T("CM_PROB_TRANSLATION_FAILED(33)");
		break;
	case CM_PROB_NO_SOFTCONFIG://              (0x00000022)   // no soft config
		problemStr = _T("CM_PROB_NO_SOFTCONFIG(34)");
		break;
	case CM_PROB_BIOS_TABLE://                 (0x00000023)   // device missing in BIOS table
		problemStr = _T("CM_PROB_BIOS_TABLE(35)");
		break;
	case CM_PROB_IRQ_TRANSLATION_FAILED://     (0x00000024)   // IRQ translator failed
		problemStr = _T("CM_PROB_IRQ_TRANSLATION_FAILED(36)");
		break;
	case CM_PROB_FAILED_DRIVER_ENTRY://        (0x00000025)   // DriverEntry() failed.
		problemStr = _T("CM_PROB_FAILED_DRIVER_ENTRY(37)");
		break;
	case CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD:// (0x00000026)   // Driver should have unloaded.
		problemStr = _T("CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD(38)");
		break;
	case CM_PROB_DRIVER_FAILED_LOAD://         (0x00000027)   // Driver load unsuccessful.
		problemStr = _T("CM_PROB_DRIVER_FAILED_LOAD(39)");
		break;
	case CM_PROB_DRIVER_SERVICE_KEY_INVALID:// (0x00000028)   // Error accessing driver's service key
		problemStr = _T("CM_PROB_DRIVER_SERVICE_KEY_INVALID(40)");
		break;
	case CM_PROB_LEGACY_SERVICE_NO_DEVICES://  (0x00000029)   // Loaded legacy service created no devices
		problemStr = _T("CM_PROB_LEGACY_SERVICE_NO_DEVICES(41)");
		break;
	case CM_PROB_DUPLICATE_DEVICE://           (0x0000002A)   // Two devices were discovered with the same name
		problemStr = _T("CM_PROB_DUPLICATE_DEVICE(42)");
		break;
	case CM_PROB_FAILED_POST_START://         (0x0000002B)   // The drivers set the device state to failed
		problemStr = _T("CM_PROB_FAILED_POST_START(43)");
		break;
	case CM_PROB_HALTED://                     (0x0000002C)   // This device was failed post start via usermode
		problemStr = _T("CM_PROB_HALTED(44)");
		break;
	case CM_PROB_PHANTOM://                    (0x0000002D)   // The devinst currently exists only in the registry
		problemStr = _T("CM_PROB_PHANTOM(45)");
		break;
	case CM_PROB_SYSTEM_SHUTDOWN://            (0x0000002E)   // The system is shutting down
		problemStr = _T("CM_PROB_SYSTEM_SHUTDOWN(46)");
		break;
	case CM_PROB_HELD_FOR_EJECT://             (0x0000002F)   // The device is offline awaiting removal
		problemStr = _T("CM_PROB_HELD_FOR_EJECT(47)");
		break;
	case CM_PROB_DRIVER_BLOCKED://             (0x00000030)   // One or more drivers is blocked from loading
		problemStr = _T("CM_PROB_DRIVER_BLOCKED(48)");
		break;
	case CM_PROB_REGISTRY_TOO_LARGE://         (0x00000031)   // System hive has grown too large
		problemStr = _T("CM_PROB_REGISTRY_TOO_LARGE(49)");
		break;
	default:
	case NUM_CM_PROB:
		problemStr = _T("NUM_CM_PROB(50)");
		break;
	}

	return problemStr;
}


// The one and only SetupApi object
SetupApi& gSetupApi()
{
	static SetupApi sApi;
	return sApi;
};
