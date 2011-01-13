/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "Disk.h"
#include "DeviceManager.h"

Disk::Disk(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
{
	_driveNumber.describe(this, _T("Physical Disk Number"), _T(""));
	_driveNumber.put(-1);
	_driveNumber.get();
}

Disk::~Disk(void)
{
}

/// <summary>
/// Property: Gets the drive number for the Disk.
/// </summary>
int32_t Disk::driveNumber::get()
{
	if ( Value == -1 )
	{
		Disk* disk = dynamic_cast<Disk*>(_owner);
		ASSERT(disk);

//		if (!disk->_logicalDrive.get().IsEmpty())
//        {
			if ( gWinVersionInfo().IsWinNT() )
			{
//				CStdString str;
//				str.Format(_T("\\\\.\\%s"), vol->_logicalDrive.get());
//				HANDLE hFile = CreateFile(str, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				HANDLE hFile = CreateFile(disk->_path.get().c_str(), 0, 0, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					DWORD error = GetLastError();
					ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
					ATLTRACE(_T("*** Error: %d, Drive: %s\n"), error, disk->_path.get().c_str());
//clw					throw;
				}
				
				DWORD bytesReturned = 0;
				STORAGE_DEVICE_NUMBER driveNumber;
				if (!DeviceIoControl(hFile, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &driveNumber, sizeof(driveNumber), &bytesReturned, NULL))
				{
					// do nothing here on purpose
					DWORD error = GetLastError();
					ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
					ATLTRACE(_T("*** Error: %d, Drive: %s\n"), error, disk->_path.get().c_str());
				}
				CloseHandle(hFile);

				if (bytesReturned > 0)
				{
					Value = driveNumber.DeviceNumber;
				}
			}
			else
			{
				ATLTRACE(_T("*** ASSERTION FAILED: Line %d of file %s\n"), __LINE__, __TFILE__);
				throw;
				//TODO: Can't use CreateFile() to open a volume on Win98
			}
//		}
    }
	return Value;
}
