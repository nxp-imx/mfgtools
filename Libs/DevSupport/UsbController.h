#pragma once

#include "UsbHub.h"

namespace usb
{
	class Controller : public Device
	{
	public:
		Controller(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
		virtual ~Controller(void);
		Hub * GetRootHub();

	private:
		HANDLE Open();
		int32_t Initialize();

		StringProperty _rootHubFilename;
	};
} // namespace usb