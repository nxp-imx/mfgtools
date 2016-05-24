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
#if 0
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
#endif
	    }
	return Value;
}
