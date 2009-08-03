#include "stdafx.h"
#include "Disk.h"
#include "DeviceManager.h"

Disk::Disk(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
{
}

Disk::~Disk(void)
{
}

/// <summary>
/// Property: Gets the drive number for the Disk.
/// </summary>
int32_t Disk::driveNumber::get()
{
	Device* dev = dynamic_cast<Device*>(_owner);
	ASSERT(dev);
/*
	DWORD error;
	if ( _value == -1 )
		if ( !gSetupApi().SetupDiGetClassImageIndex(dev->_deviceClass->ImageListPtr(), dev->_classGuid.get(), (PINT)&_value))
			error = GetLastError();
//	BOOL ret = SetupDiGetClassBitmapIndex(dev->_classGuid.get(), (PINT)&_value);
//	if (!ret)
//		error = GetLastError();
*/
	return Value;
}
