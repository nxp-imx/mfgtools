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


	bool FinishState = false;
	struct libusb_device_handle *m_libusbdevHandle;

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
	virtual void NotifyOpen();

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
	}_hub;	//hub��s device path

	DeviceClass* _deviceClass;
	DeviceCapabilities _capabilities;
	Device* _parent;
	DEVINST _hDevInst;
	HDEVINFO _deviceInfoSet;
	SP_DEVINFO_DATA _deviceInfoData;
	CString _classGuidStr;

	myevent * m_hDevCanDeleteEvent;
	DWORD  m_dwIndex;

	INSTANCE_HANDLE m_pLibHandle;
};

typedef std::list<Device *> DEVICES_ARRAY;
