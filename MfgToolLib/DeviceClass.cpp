/*
 * Copyright 2009-2013, 2016 Freescale Semiconductor, Inc.
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

#include "stdafx.h"
#include "DeviceClass.h"
#include "HubClass.h"
#include "UsbHub.h"
#include "DeviceManager.h"
#include "MfgToolLib_Export.h"

DeviceClass::DeviceClass(LPCGUID iFaceGuid, LPCGUID devGuid, LPCTSTR enumerator, DEV_CLASS_TYPE type, INSTANCE_HANDLE handle)
: Property(true)
, _deviceInfoSet(INVALID_HANDLE_VALUE)
, _deviceClassType(type)
{
	if(iFaceGuid)
		_classIfaceGuid.put(*iFaceGuid);
	if(devGuid)
        _classDevGuid.put(*devGuid);
	if (enumerator)
        _enumerator.put(enumerator);

	m_pLibHandle = handle;

	_classDesc.describe(this, _T("Class Description"), _T(""));
	_classIfaceGuid.describe(this, _T("Device Interface GUID"), _T("Describes the device class interface."));
	_classDevGuid.describe(this, _T("Device Class GUID"), _T("Describes the device class."));
	_enumerator.describe(this, _T("Enumerator"), _T(""));
	devicesMutex = new pthread_mutex_t;

	pthread_mutex_init(devicesMutex,NULL);// = CreateMutex(NULL, FALSE, NULL);
	if(devicesMutex == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("create DeviceClass::devicesMutex failed"));
		throw 1;
	}

	m_msc_vid = 0;
	m_msc_pid = 0;
}

DeviceClass::~DeviceClass()
{
}

HDEVINFO DeviceClass::GetDevInfoSet()
{
    return _deviceInfoSet;
}

void DeviceClass::DestroyDevInfoSet()
{
}

DWORD DeviceClass::EnumDeviceInterfaceDetails(DWORD index, CString& devPath, PSP_DEVINFO_DATA pDevData)

{
	return ERROR_NO_MORE_ITEMS;
}

//Enum all devices according to GUID
DEVICES_ARRAY& DeviceClass::Devices()
{
	return _devices;
}

CString DeviceClass::classDesc::get()
{
    return _value;
}

Device* DeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
    Device* dev = new Device(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);

	return dev;
}

Device* DeviceClass::FindDeviceByUsbPath(CString pathToFind, const DeviceListType devListType, const DeviceListAction devListAction )
{

}

DeviceClass::NotifyStruct DeviceClass::AddUsbDevice(LPCTSTR path,libusb_device *dev)
{
#ifndef __linux__

	NotifyStruct nsInfo = {0};
	CString pathToFind = path + 4;
	Device * pDevice = NULL;


	int RetryCount = 0;
	while( RetryCount < RETRY_COUNT )
	{
		// see if it is already in our list of Devices()
		pDevice = FindDeviceByUsbPath(pathToFind, DeviceListType_Current, DeviceListAction_None);
		if ( pDevice == NULL )
		{
			// it's not in our Collection of constructed devices
			// so lets get a new list of our devices from Windows
			// and see if it is there.
			pDevice = FindDeviceByUsbPath(pathToFind, DeviceListType_New, DeviceListAction_Add);
			if(pDevice != NULL)
			{
				//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::AddUsbDevice() successful %s add to current list"), path);
				break;
			}
			else
			{
				RetryCount++;
			}
		}
		else
		{
			break;
		}
	}
	if(RetryCount < RETRY_COUNT)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::AddUsbDevice() successful %s add to current list, retrycount: %d"), pathToFind, RetryCount);
	}
	else
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceClass::AddUsbDevice() failed %s NOT belong to me"), pathToFind);
	}

	if ( pDevice != NULL )
	{
		nsInfo.Device = pDevice;
        nsInfo.Type = _deviceClassType;
		nsInfo.DriverLetter = _T('\0');
        nsInfo.PortIndex = pDevice->_hubIndex.get();
        nsInfo.Hub = pDevice->_hub.get();
		// Find our Hub in gDeviceManager's list of [Hub].Devices()
		usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
		usb::Hub *pHub = pHubClass->FindHubByPath(nsInfo.Hub);
		if(pHub != NULL)
		{
			nsInfo.HubIndex = pHub->_index.get();
		}

		// Delete device from old device list since it's in our current list
//		FindDeviceByUsbPath(pathToFind, DeviceListType_Old, DeviceListAction_Remove);

		if(RefreshPort(nsInfo.Hub, nsInfo.PortIndex) != ERROR_SUCCESS)
		{
			nsInfo.Device = NULL;
		}
	}

	return nsInfo;
#else

	NotifyStruct nsInfo = {0};
	SP_DEVINFO_DATA devData;
	devData.cbSize = sizeof(SP_DEVINFO_DATA);

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Add USB device\n"));
	Device *pDevice = CreateDevice(this,devData , _T(""));
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Create Device %p\n"), pDevice);
	int rc=libusb_open(dev,&pDevice->m_libusbdevHandle);
	 if (LIBUSB_SUCCESS != rc) {
	    if(rc==LIBUSB_ERROR_ACCESS){
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("failed to open no access\n"));
	    }
	    LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("could not open USB Device\n"));
	    delete pDevice;
	    return nsInfo;
	}
	if (libusb_kernel_driver_active(pDevice->m_libusbdevHandle, 0)){
               libusb_detach_kernel_driver(pDevice->m_libusbdevHandle, 0);
	}
	pDevice->NotifyOpen();

        rc = libusb_claim_interface(pDevice->m_libusbdevHandle, 0);
	if (rc) {
                fprintf(stderr, "Failed to claim interface\n");
		delete pDevice;
		return nsInfo;
         }

	if ( pDevice != NULL )
         {
         nsInfo.pDevice = pDevice;
         nsInfo.Type = _deviceClassType;
         nsInfo.DriverLetter = _T('\0');
         nsInfo.PortIndex = pDevice->_hubIndex.get();
         nsInfo.Hub = pDevice->_hub.get();
                 // Find our Hub in gDeviceManager's list of [Hub].Devices()
         usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);
         usb::Hub *pHub = pHubClass->FindHubByPath(nsInfo.Hub);
                 if(pHub != NULL)
                 {
                         nsInfo.HubIndex = pHub->_index.get();
                 }

                 // Delete device from old device list since it's in our current list
 //              FindDeviceByUsbPath(pathToFind, DeviceListType_Old, DeviceListAction_Remove);
         }

	 return nsInfo;

#endif
}

DeviceClass::NotifyStruct DeviceClass::RemoveUsbDevice(LPCTSTR path)
{

	NotifyStruct nsInfo = {0};
	return nsInfo;
}

void DeviceClass::SetMSCVidPid(USHORT vid, USHORT pid)
{
	m_msc_vid = vid;
	m_msc_pid = pid;
}

std::list<Device*>& DeviceClass::Refresh()
{
	while ( _devices.size() > 0 )
	{
		Device * dev = _devices.back();
		_devices.pop_back();
		delete dev;
	}

	return Devices();
}

int DeviceClass::RefreshPort(const CString hubPath, const int hubIndex)
{
	// Find our Hub in gDeviceManager's list of [Hub].Devices()
	usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);

	ASSERT(pHubClass);

	usb::Hub* pHub = pHubClass->FindHubByPath(hubPath);

	if ( pHub )
	{
		return pHub->RefreshPort(hubIndex);
	}
	else
	{
		return ERROR_FILE_NOT_FOUND;
	}
}

int DeviceClass::ClearPort(const CString hubPath, const int hubIndex)
{
	// Find our Hub in gDeviceManager's list of [Hub].Devices()
	usb::HubClass* pHubClass = dynamic_cast<usb::HubClass*>(g_devClasses[DeviceClass::DeviceTypeUsbHub]);

	ASSERT(pHubClass);

	usb::Hub* pHub = pHubClass->FindHubByPath(hubPath);

	if ( pHub )
	{
		return pHub->ClearPort(hubIndex);
	}
	else
	{
		return ERROR_FILE_NOT_FOUND;
	}
}
