/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// UsbPort.cpp : implementation file
//

#include "stdafx.h"
#include "UsbHubMgr.h"
#include "DeviceManager.h"
#include "Volume.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

usb::Port::Port(usb::Hub* pHub, int32_t index) 
: Property(true)
, _parentHub(pHub)
, _index(index)
, _device(NULL)
{
	_name.Format(_T("Port %d"), index);

	_index.describe(this, _T("Index"));
	_connected.describe(this, _T("Connection status"));
	_isHub.describe(this, _T("Device is hub"));

	// typedef enum _USB_CONNECTION_STATUS from usbioctl.h
	_connected.ValueList[NoDeviceConnected]        = L"NoDeviceConnected";
	_connected.ValueList[DeviceConnected]          = L"DeviceConnected";
	_connected.ValueList[DeviceFailedEnumeration]  = L"DeviceFailedEnumeration";
	_connected.ValueList[DeviceGeneralFailure]     = L"DeviceGeneralFailure";
	_connected.ValueList[DeviceCausedOvercurrent]  = L"DeviceCausedOvercurrent";
	_connected.ValueList[DeviceNotEnoughPower]     = L"DeviceNotEnoughPower";
	_connected.ValueList[DeviceNotEnoughBandwidth] = L"DeviceNotEnoughBandwidth";
	_connected.ValueList[DeviceHubNestedTooDeeply] = L"DeviceHubNestedTooDeeply";
	_connected.ValueList[DeviceInLegacyHub]        = L"DeviceInLegacyHub";
}

usb::Port::~Port(void)
{
}

void usb::Port::Clear()
{
	DWORD error = ERROR_SUCCESS;

	// Reset Port Properties
	memset(&_connectionInfo, 0, sizeof(_connectionInfo));
	_connectionInfo.ConnectionIndex = _index.get();
	
	_propertyList.erase(_propertyList.begin(), _propertyList.end());
	_index.describe(this, _T("Index"));
	_connected.describe(this, _T("Connection status"));
	_isHub.describe(this, _T("Device is hub"));

	_connected.put(NoDeviceConnected);
	_isHub.put(false);

	_device = NULL;
}

int32_t usb::Port::Refresh()
{
	DWORD error = ERROR_SUCCESS;

	// Reset Port Properties
	memset(&_connectionInfo, 0, sizeof(_connectionInfo));
	_connectionInfo.ConnectionIndex = _index.get();
	
	_propertyList.erase(_propertyList.begin(), _propertyList.end());
	_index.describe(this, _T("Index"));
	_connected.describe(this, _T("Connection status"));
	_isHub.describe(this, _T("Device is hub"));

	_connected.put(NoDeviceConnected);
	_isHub.put(false);

	_device = NULL;

	// Open hub
	HANDLE hHub = _parentHub->Open();
	if ( hHub == INVALID_HANDLE_VALUE )
	{
		error=GetLastError();
		ATLTRACE(_T("*** ERROR1 0x%X (%d): usb::Port::Refresh() hub: %d port:%d\n"), error, error, _parentHub->_index.get(), _index.get());
		return error;
	}

	// Get Connection Information
	DWORD BytesReturned;
	BOOL Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, &_connectionInfo, sizeof(_connectionInfo),
							&_connectionInfo, sizeof(_connectionInfo), &BytesReturned, NULL);
	if (!Success) 
	{
		error = GetLastError();
		CloseHandle(hHub);
		ATLTRACE(_T("*** ERROR2 0x%X (%d): usb::Port::Refresh() hub: %d port:%d\n"), error, error, _parentHub->_index.get(), _index.get());
		return error;
	}

	// Initialize Port Properties
	_connected.put(_connectionInfo.ConnectionStatus);
	_isHub.put(_connectionInfo.DeviceIsHub);

	//
	// There is a device connected to this Port
	//
	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		DWORD bytes = sizeof(DWORD)*2 + MAX_PATH*2;
		PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)malloc(bytes);
		driverName->ConnectionIndex = _index.get(); 
		DWORD i=0;
		do{
			// Get the name of the driver key of the device attached to the specified port.
			Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, driverName,
									bytes, driverName, bytes, &BytesReturned, NULL);


			i++;
//t			ATLTRACE(_T("*** ***: usb::Port::Refresh() polling iteration: #%d: %d port:%d\n"),i,_parentHub->_index.get(), _index.get());
			//Max 3mins are waited.
			if(i>=180 || Success)
				break;

			//Polling the status per second.
			Sleep(1000);

		}while(!Success);

		if (!Success) 
		{
			error = GetLastError();
			CloseHandle(hHub);
			free (driverName);
			ATLTRACE(_T("*** ERROR3 0x%X (%d): usb::Port::Refresh() hub: %d port:%d\n"), error, error, _parentHub->_index.get(), _index.get());
			return error;
		}

		if ( _connectionInfo.DeviceIsHub )
		{
			_device = FindHub(driverName->DriverKeyName);
			if ( _device != NULL )
			{
				_device->describe(this, _device->name());
				_name.Format(_T("Port %d - Hub %d"), _index.get(), ((usb::Hub*)_device)->_index.get());
			}
			else
			{
				_name = _T("Hub [Connected] DISABLED");
			}
		}
		else
		{
			_device = FindDevice(driverName->DriverKeyName);
			if ( _device != NULL )
			{
				_device->describe(this, _T("Device"));
				_name.Format(_T("Port %d [Connected] %s"), _index.get(), _device->_description.get());
			}
			else
			{
				_name.Format(_T("Port %d [Connected] UNKNOWN"), _index.get());
			}

			if ( _connectionInfo.Speed == UsbFullSpeed )
				_name.append(_T(" <Full speed (v1.1)>"));
			if ( _connectionInfo.Speed == UsbHighSpeed )
				_name.append(_T(" <High speed (v2.0)>"));
		}

		free (driverName);

	} // end if(connected)
	else
	{
		_name.Format(_T("Port %d [Not connected]"), _index.get());
	}

	// Close the hub
	CloseHandle(hHub);

//t	ATLTRACE2(_T("usb::Port::Refresh() - Hub %d, %s\r\n"), _parentHub->_index.get(), _name.c_str());

	return error;
}

usb::Hub* usb::Port::FindHub(LPCTSTR driverName)
{
	CStdString driverNameStr = driverName;
	usb::Hub* pHub = NULL;

	// Find our Hub in gDeviceManager's list of [Hub].Devices()
	usb::HubMgr* pHubManager = dynamic_cast<usb::HubMgr*>(gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbHub]);
	
	assert(pHubManager);
	
	return pHubManager->FindHubByDriver(driverName);
}

Device* usb::Port::FindDevice(LPCTSTR driverName)
{
	CStdString driverNameStr = driverName;
	Device* pUsbDevice = NULL;

	if ( driverNameStr.IsEmpty() )
		return NULL;
	
	// Find our Device in gDeviceManager's Master List of [*DeviceClass].Devices()
	std::list<Device*> DeviceList = gDeviceManager::Instance().Devices();
	std::list<Device*>::iterator device;
	for ( device = DeviceList.begin(); device != DeviceList.end(); ++device )
	{
		if ( (*device)->UsbDevice() != NULL )
		{
			if ( driverNameStr.CompareNoCase( (*device)->UsbDevice()->_driver.get() ) == 0 )
			{
				//ATLTRACE2(_T("DeviceClass::AddUsbDevice()  Found existing(%d): %s\r\n"), device.size(), pDevice->_usbPath.get().c_str());
				pUsbDevice = (*device);
				break;
			}
		}
	}

//	assert (pUsbDevice);

	return pUsbDevice;
}

CStdString usb::Port::GetUsbDevicePath(void)
{
	CStdString pathStr = _T("");

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL ) //assert(_device);
		{
			if ( _device->UsbDevice() != NULL )
			{
				pathStr = _device->UsbDevice()->_path.get();
			}
		}
	}

	return pathStr;
}

CStdString usb::Port::GetDriverKeyName(void)
{
	CStdString driverStr = _T("");

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL ) //assert(_device);
		{
			driverStr = _device->_driver.get();
		}
	}

	return driverStr;
}

CStdString usb::Port::GetDeviceDescription(void)
{
	CStdString descStr = _T("");

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL )
		{
			switch( _device->GetDeviceType() )
			{
				case DeviceClass::DeviceTypeMsc:
				{
					Volume* pVolume = dynamic_cast<Volume*>(_device);
					descStr = pVolume->_friendlyName.get();
					break;
				}
				case DeviceClass::DeviceTypeHid:
				case DeviceClass::DeviceTypeMxHid:
				//case DeviceClass::DeviceTypeMtp:
				case DeviceClass::DeviceTypeRecovery:
				case DeviceClass::DeviceTypeUsbController:
				case DeviceClass::DeviceTypeUsbHub:
				default:
					descStr = _device->_description.get().c_str();
			}
		}
	}

	return descStr;
}

CStdString usb::Port::GetDriveLetters(void)
{
	CStdString driveStr = _T("");

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		DWORD ret = WaitForSingleObject(gDeviceManager::Instance()[DeviceClass::DeviceTypeMsc]->devicesMutex, INFINITE);

		if ( _device != NULL && 
			 _device->GetDeviceType() == DeviceClass::DeviceTypeMsc )
		{
			// Go though the list of Volumes and see if any of them have the same USB hub and hubIndex
			// that identify this usb::Port.
			std::list<Device*> VolumeList = gDeviceManager::Instance()[DeviceClass::DeviceTypeMsc]->Devices();
			std::list<Device*>::iterator vol;
			for ( vol = VolumeList.begin(); vol != VolumeList.end(); ++vol )
			{
				if ( (_index.get() == (*vol)->_hubIndex.get()) &&
					 (_parentHub->_path.get().CompareNoCase((*vol)->_hub.get().c_str()) == 0) )
				{
					if ( !driveStr.IsEmpty() )
					{
						driveStr += _T(',');
					}
					Volume* pVol = dynamic_cast<Volume*>(*vol);
					driveStr += pVol->_logicalDrive.get().c_str();
				}
			}
		}
		ReleaseMutex(gDeviceManager::Instance()[DeviceClass::DeviceTypeMsc]->devicesMutex);
	}

	return driveStr;
}

uint32_t usb::Port::GetDeviceType(void)
{
	uint32_t devType = DeviceClass::DeviceTypeNone;

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL )
		{
			devType = _device->GetDeviceType();
		}
	}

	return devType;
}

CStdString  usb::Port::GetName(void)
{
	return _parentHub->_path.get();
}
