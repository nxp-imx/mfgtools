/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "Disk.h"
#include "DeviceManager.h"

Disk::Disk(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
: Device(deviceClass, devInst, path, handle)
{
	_driveNumber.describe(this, _T("Physical Disk Number"), _T(""));
	_driveNumber.put(-1);
//	_driveNumber.get();

	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("new Disk[%p]"), this);
}

Disk::~Disk(void)
{
	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("delete Disk[%p]"), this);
}

/// <summary>
/// Property: Gets the drive number for the Disk.
/// </summary>
int Disk::driveNumber::get()
{
	if ( Value == -1 )
	{
		Disk* disk = dynamic_cast<Disk*>(_owner);
		ASSERT(disk);

		//only used in Windows NT and above
		HANDLE hFile = CreateFile(disk->_path.get().GetBuffer(), 0, 0, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
		disk->_path.get().ReleaseBuffer();
		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("***CreateFile Error: %d, Drive: %s\n"), error, disk->_path.get());
            return Value;
		}
		
		DWORD bytesReturned = 0;
		STORAGE_DEVICE_NUMBER driveNumber;
		if (!DeviceIoControl(hFile, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &driveNumber, sizeof(driveNumber), &bytesReturned, NULL))
		{
			// do nothing here on purpose
			DWORD error = GetLastError();
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("***DeviceIoControl Error: %d, Drive: %s\n"), error, disk->_path.get());
            return Value;
		}
		CloseHandle(hFile);

		if (bytesReturned > 0)
		{
			Value = driveNumber.DeviceNumber;
		}
    }
	
	return Value;
}
