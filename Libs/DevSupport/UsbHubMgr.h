#pragma once
#include "DeviceClass.h"
#include "UsbHub.h"

namespace usb
{

	/// <summary>
	/// The device class for disk devices.
	/// </summary>
	class HubMgr :	public DeviceClass
	{
	public:
		HubMgr();
		virtual ~HubMgr(void);
		void RefreshHubs();
		Hub* FindHubByDriver(LPCTSTR driverName);
		Hub* FindHubByPath(LPCTSTR pathName);

	private:
	   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

	public:
		CStdString ToString() { return _T("HubMgr"); }
	};
} // namespace usb