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
#include <list>
#include "MfgToolLib_Export.h"

class DeviceClass;
class CMutex;

class Device : public Property
{
public:
	Device(DeviceClass* deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
	virtual ~Device();

	static const DWORD MaxTransferSizeInBytes    = 128 * 512; //(128 sectors of 512 bytes each at a time) says who??

	typedef enum DeviceCapabilities
    {
        Unknown = 0x00000000,
        // matches cfmgr32.h CM_DEVCAP_* definitions
        LockSupported = 0x00000001,
        EjectSupported = 0x00000002,
        Removable = 0x00000004,
        DockDevice = 0x00000008,
        UniqueId = 0x00000010,
        SilentInstall =0x00000020,
        RawDeviceOk = 0x00000040,
        SurpriseRemovalOk = 0x00000080,
        HardwareDisabled = 0x00000100,
        NonDynamic = 0x00000200,
    };

	typedef struct NotifyStruct
	{
        enum dataDir { dataDir_Off, dataDir_FromDevice, dataDir_ToDevice };
        NotifyStruct(LPCTSTR name, dataDir dir, UINT max)
            : inProgress(true)
            , direction(dir)
            , position(0)
			, maximum(max)
            , error(ERROR_SUCCESS)
            , status(name){};
        bool inProgress;
        dataDir direction;
        UINT position;
		UINT maximum;
        int error;
		CString status;
	};
	
	DWORD InitDevInfo();
	Device* Parent();	//get the parent device
	virtual Device* UsbDevice();
	CString GetProperty(DWORD property, CString defaultValue);
	DWORD GetProperty(DWORD property, DWORD defaultValue);
	DEVINST InstanceHandle();
	DeviceCapabilities Capabilities();
	virtual bool IsUsb();
	virtual bool ValidateUsbIds();
	DWORD GetDeviceType();
	void Reset(DEVINST devInst, CString path);
	DWORD GetDeviceWndIndex(void);
	void SetDeviceWndIndex(DWORD dwIndex);
	
public:
	// PROPERTIES
	class path : public StringProperty 
	{ 
	public: 
		CString get();
	}_path;		//the hole device path
	
	class usbPath : public StringProperty 
	{ 
	public: 
		CString get(); 
	}_usbPath;
	
	class classGuid : public GuidProperty 
	{ 
	public: 
		LPGUID get(); 
	}_classGuid;
	
	class hubIndex : public Int32Property 
	{ 
	public: 
		int get(); 
		int getmsc(USHORT vid, USHORT pid);
		int get2();
	}_hubIndex;	//this device is connected to which port in the hub(from 1)
	
	class description : public StringProperty 
	{ 
	public: 
		CString get(); 
	}_description;
	
	class friendlyName : public StringProperty 
	{ 
	public: 
		virtual CString get(); 
	}_friendlyName;
	
	class driver : public StringProperty 
	{ 
	public: 
		CString get(); 
	}_driver;
	
	class deviceInstanceID : public StringProperty 
	{ 
	public: 
		CString get(); 
	}_deviceInstanceID;
	
	class enumerator : public StringProperty 
	{ 
	public: 
		CString get(); 
	}_enumerator;
	
	class classStr : public StringProperty 
	{ 
	public: 
		CString get(); 
	}_classStr;
	
	class classDesc : public StringProperty 
	{ 
	public: 
		CString get(); 
	}_classDesc;
	
	class hub : public StringProperty 
	{ 
	public: 
		CString get();
	}_hub;	//hub¡¯s device path
	
	DeviceClass* _deviceClass;
	DeviceCapabilities _capabilities;
	Device* _parent;
	DEVINST _hDevInst;
	HDEVINFO _deviceInfoSet;
	SP_DEVINFO_DATA _deviceInfoData;
	CString _classGuidStr;

	HANDLE m_hDevCanDeleteEvent;
	DWORD  m_dwIndex;

	INSTANCE_HANDLE m_pLibHandle;
};

typedef std::list<Device *> DEVICES_ARRAY;

