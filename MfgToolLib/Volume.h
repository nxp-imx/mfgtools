/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "Device.h"
#include "Disk.h"
#include "StApi.h"


#define MAX_SCSI_DATA_TRANSFER_SIZE 0x10000
/// <summary>
/// A volume device.
/// </summary>
class Volume : public Device
{
public:
	Volume(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
	virtual ~Volume(void);

	enum LockType{ LockType_Physical, LockType_Logical };

	bool IsUsb();
	virtual Device* UsbDevice();
	Disk* StorageDisk();
	HANDLE Lock(LockType lockType);
	int Unlock(HANDLE hDrive, bool close = true);
	virtual UINT SendCommand(StApi& api, UCHAR* additionalInfo = NULL);
	UINT SendCommand(HANDLE hDrive, StApi& api, UCHAR* additionalInfo, NotifyStruct& nsInfo);

	UCHAR *m_pBuffer;

	// PROPERTIES
	class volumeName : public StringProperty { public: CString get(); }_volumeName;
	class logicalDrive : public StringProperty { public: CString get(); }_logicalDrive;
	class friendlyName : public Device::friendlyName { public: CString get(); }_friendlyName;
	class diskNumber : public Int32Property { public: diskNumber(int val):Int32Property(val){}; int get(); }_diskNumber;

	void NotifyUpdateUI(int cmdOpIndex, int position, int maximum);

private:
#pragma pack (push, 1)
	struct _NT_SCSI_REQUEST
	{
		SCSI_PASS_THROUGH PassThrough;	//should include "ntddscsi.h"
		__int64          Tag;
		SENSE_DATA        SenseData;		//should include "scsi.h"
		UCHAR           DataBuffer[1];          // Allocate buffer space
	};
#pragma pack (pop)

	Disk* _Disk;
	HANDLE _hEvent;
};