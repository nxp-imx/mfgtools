/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "Device.h"

class Disk : public Device
{
public:
	Disk(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~Disk(void);

	//PROPERTIES
	class driveNumber : public Int32Property { public: int32_t get(); }_driveNumber;
};
