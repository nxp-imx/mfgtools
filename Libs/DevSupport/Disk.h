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
