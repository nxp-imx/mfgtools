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
#include "Device.h"
#pragma warning( disable : 4200 )
#ifndef __linux__
#include <usbioctl.h>
#endif
#pragma warning( default : 4200 )

#include "MfgToolLib_Export.h"
#include "DeviceManager.h"
#include "DeviceClass.h"
#include "HubClass.h"
//CMutex Device::m_mutex(FALSE, _T("Device.SendCommand"));

Device::Device(DeviceClass *deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
: Property(true)
, _parent(NULL)
, _deviceInfoSet(INVALID_HANDLE_VALUE)
{
	m_pLibHandle = handle;

	_hubIndex.put(0);
	_classGuidStr.Empty();
	_capabilities = Unknown;
	_usbPath.put(_T(""));
	memset(&_deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));

	_deviceClass = deviceClass;
	_hDevInst = devInst;

	//w98 - In Windows 98 the hub path starts with \DosDevices\
    //w98 - Replace it with \\.\ so we can use it with CreateFile.
	path.Replace(_T("\\DosDevices"), _T("\\\\."));
    //wxp - In Windows XP the hub path may start with \??\
    //wxp - Replace it with \\.\ so we can use it for CreateFile
	path.Replace(_T("\\??"), _T("\\\\."));
	path.Replace(_T("\\\\?"), _T("\\\\."));
	_path.put(path);

	InitDevInfo();

    m_dwIndex = -1;
	InitEvent(&m_hDevCanDeleteEvent);

	_description.describe(this, _T("Device Description"), _T(""));
    _friendlyName.describe(this, _T("Friendly Name"), _T(""));
    _path.describe(this, _T("Path"), _T(""));
    _usbPath.describe(this, _T("UsbPath"), _T(""));
    _driver.describe(this, _T("Driver"), _T(""));
    _deviceInstanceID.describe(this, _T("Device Instance Id"), _T(""));
    _enumerator.describe(this, _T("Enumerator"), _T(""));
    _classStr.describe(this, _T("Class"), _T(""));
    _compatibleIds.describe(this, _T("Compatible Ids"), _T(""));
    _classDesc.describe(this, _T("Class Description"), _T(""));
    _classGuid.describe(this, _T("Device Class GUID"), _T("Describes the device class."));
    _hub.describe(this, _T("USB Hub Path"), _T("Path of Hub USB Device is connected to."));
    _hubIndex.describe(this, _T("USB Hub Port"), _T("Hub Port the USB Device is connected to."));
    if (Parent() != NULL)
    {
		_parent->describe(this, _T("Parent"), _T("Parent device in the system DevNode tree."));
    }
}

void Device::Reset(DEVINST devInst, CString path)
{
}

void Device::NotifyOpen()
{
}

Device::~Device()
{
#ifndef __linux__
	_classGuid.put(GUID_NULL);
#endif
    _hubIndex.put(0);
    _capabilities = Unknown;
    _classGuidStr.Empty();
    _usbPath.put(_T(""));

	if ( _parent )
    {
        delete _parent;
        _parent = NULL;
    }

	if ( _deviceInfoSet != INVALID_HANDLE_VALUE )
    {
        _deviceInfoSet = INVALID_HANDLE_VALUE;
    }

	if(m_hDevCanDeleteEvent != NULL)
	{
		DestroyEvent(m_hDevCanDeleteEvent);
		m_hDevCanDeleteEvent = NULL;
	}
}

/// <summary>
/// Gets the device's instance handle.
/// </summary>
DEVINST Device::InstanceHandle()
{
    return _hDevInst;
}

DWORD Device::InitDevInfo()
{

	DWORD error = ERROR_SUCCESS;
    return error;
}

/// <summary>
/// Gets the device's parent device or null if this device has not parent.
/// </summary>
Device* Device::Parent()
{
    return _parent;
}

/// <summary>
/// Property: Gets the device's path.
/// </summary>
CString Device::path::get()
{
    return  _value;

}

/// <summary>
/// Gets the device connected to the USB bus.
/// </summary>
Device* Device::UsbDevice()
{
    if (Parent() == NULL)
        return NULL;

    if (_enumerator.get().CompareNoCase(_T("USB")) == 0)
    {
        // if current node is hub device, then return this node.
        if ((_compatibleIds.get().MakeUpper().Find(_T("CLASS_09")) >= 0) || (_description.get().MakeUpper().Find(_T("HUB")) >= 0))
        {
            return this;
        }
        // Check if it's parent is an usb hub.
        // If yes, this node is the correct usb device.
        if ((Parent()->_compatibleIds.get().MakeUpper().Find(_T("CLASS_09")) >= 0) || (Parent()->_description.get().MakeUpper().Find(_T("HUB")) >= 0))
        {
            return this;
        }
        else // If not, this node is not the correct usb device, but a child of the usb device.
        {
            return Parent()->UsbDevice();
        }
    }
    return Parent()->UsbDevice();
}

/// <summary>
/// Property: Gets the device's USB device path w/o the first 4 characters.
/// </summary>
CString Device::usbPath::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);

    if ( _value.IsEmpty() )
    {
        if ( /*dev->IsUsb()*/dev->UsbDevice() )
        {
            // Trim the path because the first 4 characters of the path vary
            // depending on mechanisms used to obtian the path. For example,
            // some paths start with \\?\ while others start with \??\ with
            // all other characters being identical.
            _value = dev->UsbDevice()->_path.get().GetBuffer() + 4;
        }
    }
    return _value;
}

CString Device::GetProperty( DWORD property, CString defaultValue )
{
    return defaultValue;
}

DWORD Device::GetProperty(DWORD property, DWORD defaultValue)
{

    DWORD propertyRegDataType = 0;
    DWORD requiredSize = 0;
    DWORD propertyBufferSize = sizeof(DWORD);
    DWORD propertyBuffer;
    DWORD error;
    return propertyBuffer;
}

/// <summary>
/// Property: Gets the device's class Guid.
/// </summary>
LPGUID Device::classGuid::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return &_value;
}

/// <summary>
/// Property: Gets the device's description.
/// </summary>
CString Device::description::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return _value;
}

/// <summary>
/// Property: Gets the device's friendly name.
/// </summary>
CString Device::friendlyName::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return _value;
}

/// <summary>
/// Property: Gets the device's driver instance.
/// </summary>
CString Device::driver::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return _value;
}

/// <summary>
/// Property: Gets the device instance id.
/// </summary>
CString Device::deviceInstanceID::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return _value;
}

/// <summary>
/// Property: Gets the device's enumerator. ex. "USB"
/// </summary>
CString Device::enumerator::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return _value;
}

/// <summary>
/// Property: Gets the device's class name.
/// </summary>
CString Device::classStr::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return _value;
}

/// <summary>
/// Property: Gets the device's compatible IDs.
/// </summary>
CString Device::compatibleIds::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);
	return _value;
}


/// <summary>
/// Property: Gets the description for the device class.
/// </summary>
CString Device::classDesc::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return _value;
}

/// <summary>
/// Property: Gets the USB Hub device path that the device is connected to.
/// The USB Hub device is the parent of the USB device
/// </summary>
CString Device::hub::get()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
		int port_numbers_len = 7;
		uint8_t port_numbers[7];
		memset(port_numbers, 0, port_numbers_len);
		libusb_get_port_numbers(libusb_get_device(dev->m_libusbdevHandle), port_numbers, port_numbers_len);
		if (!_value.CompareNoCase(""))
		{
			for (int i = 0; i < port_numbers_len; i++)
			{
				_value += std::to_string(port_numbers[i]);
				_value += ".";
			}
		}
    return _value;
}

int Device::hubIndex::getmsc(USHORT vid, USHORT pid)
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);
	return Value;
}


/// <summary>
/// Property: Gets the USB Hub index that the device is connected to.
/// Hub Ports are 1-based. Zero(0) represents an INVALID_HUB_INDEX.
/// </summary>
int Device::hubIndex::get2()
{
    Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
    return Value;
}

int Device::hubIndex::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
    ASSERT(dev);
		Value = libusb_get_bus_number(libusb_get_device(dev->m_libusbdevHandle));
	return Value;
}

/// <summary>
/// Gets the device's capabilities.
/// </summary>
Device::DeviceCapabilities Device::Capabilities()
{
    return _capabilities;
}

/// <summary>
/// Gets a value indicating whether this device is a USB device.
/// </summary>
bool Device::IsUsb()
{

    if (Parent() == NULL)
        return false;

    if (_enumerator.get().CompareNoCase(_T("USB")) == 0)
        return true;

    return Parent()->IsUsb();
}

/// <summary>
/// Returns true if the USB IDs match the filter string.
/// </summary>
bool Device::ValidateUsbIds()
{
	return true;
}

/// <summary>
/// Gets the Device Type from the Device's DeviceClass.
/// </summary>
/// <returns>DWORD that indicates DeviceClass::DeviceType.</returns>
DWORD Device::GetDeviceType()
{
	return _deviceClass->_deviceClassType;
};

DWORD Device::GetDeviceWndIndex(void)
{
    return m_dwIndex;
}

void Device::SetDeviceWndIndex(DWORD dwIndex)
{
    m_dwIndex = dwIndex;
}
