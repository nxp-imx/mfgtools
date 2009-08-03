#pragma once
#include "DeviceClass.h"

namespace usb
{
	class ControllerMgr : public DeviceClass
	{
		friend class DeviceManager;
	private:
		ControllerMgr(void);
		virtual ~ControllerMgr(void);

	private:
	   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

	public:
		CStdString ToString() { return _T("ControllerMgr"); }
	};
} // namespace usb