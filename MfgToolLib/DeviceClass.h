/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "SetupApi.h"
#include "Property.h"
#include "Device.h"
#include "MfgToolLib_Export.h"

/// <summary>
/// A generic base class for physical device classes.
/// </summary>
class DeviceClass : public Property
{
public:
	typedef enum __DeviceClassType
	{
        DeviceTypeNone = -1,
		DeviceTypeDisk = 0,
        DeviceTypeMsc,			//Updater stage(the second stage)
        DeviceTypeHid,
        DeviceTypeMxHid,
        DeviceTypeMxRom,
        DeviceTypeUsbController,
        DeviceTypeUsbHub,
    } DEV_CLASS_TYPE;

	typedef struct NotifyStruct
	{
		int cmdOpIndex;    // specify which CmdOperation
		Device* Device;
		DEV_CLASS_TYPE Type;
		CString Hub;
		DWORD HubIndex;		//specify which hub
		DWORD PortIndex;		//specify which port in the specified hub
		DWORD Event;
		TCHAR DriverLetter;
	};

	enum DeviceListType 
	{ 
		DeviceListType_Old, 
		DeviceListType_Current, 
		DeviceListType_New 
	};

	enum DeviceListAction 
	{ 
		DeviceListAction_None, 
		DeviceListAction_Add, 
		DeviceListAction_Remove 
	};

	DeviceClass(LPCGUID iFaceGuid, LPCGUID devGuid, LPCTSTR enumerator, DEV_CLASS_TYPE type, INSTANCE_HANDLE handle);
	virtual ~DeviceClass();
	
	HDEVINFO GetDevInfoSet();
	void DestroyDevInfoSet();
	DWORD EnumDeviceInterfaceDetails(DWORD index, CString& devPath, PSP_DEVINFO_DATA pDevData);

	int RefreshPort(const CString hubPath, const int hubIndex);
	int ClearPort(const CString hubPath, const int hubIndex);
	
	virtual DEVICES_ARRAY& Devices();
	virtual std::list<Device*>& Refresh();
	virtual Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path);

	virtual NotifyStruct AddUsbDevice(LPCTSTR path);
	virtual NotifyStruct RemoveUsbDevice(LPCTSTR path);
	virtual Device* FindDeviceByUsbPath(CString pathToFind, const DeviceListType devList, const DeviceListAction devListAction = DeviceListAction_None);
	
public:
	GuidProperty(_classIfaceGuid);	//GUID for a device interface class.
    GuidProperty(_classDevGuid);	//GUID for a device setup class.
    StringProperty(_enumerator);
	
	class classDesc : public StringProperty 
	{ 
	public: 
		CString get();
	}_classDesc;

	DEV_CLASS_TYPE _deviceClassType;
	HDEVINFO _deviceInfoSet;
	DEVICES_ARRAY _devices;
	DEVICES_ARRAY _oldDevices;
	HANDLE devicesMutex;

	USHORT m_msc_vid;
	USHORT m_msc_pid;
	void SetMSCVidPid(USHORT vid, USHORT pid);

	INSTANCE_HANDLE m_pLibHandle;
};

typedef std::map<DWORD, DeviceClass*> DEV_CLASS_ARRAY;





