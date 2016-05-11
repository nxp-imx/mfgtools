/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
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

#pragma once

//#include "SetupApi.h"
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
		Device* pDevice;
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

	virtual NotifyStruct AddUsbDevice(LPCTSTR path=NULL,struct libusb_device * dev=NULL);
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
	pthread_mutex_t *devicesMutex=NULL;

	USHORT m_msc_vid;
	USHORT m_msc_pid;
	void SetMSCVidPid(USHORT vid, USHORT pid);

	INSTANCE_HANDLE m_pLibHandle;
};

typedef std::map<DWORD, DeviceClass*> DEV_CLASS_ARRAY;
