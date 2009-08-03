#pragma once

#include "Device.h"
#include "SetupApi.h"
#include "Property.h"

#include <list>

class Device;

/// <summary>
/// A generic base class for physical device classes.
/// </summary>
class DeviceClass : public Property
{
friend Device;

public:
	enum DeviceType {
		DeviceTypeNone=-1,
		DeviceTypeRecovery=0,
		DeviceTypeMsc,
		DeviceTypeMtp,
		DeviceTypeHid,
		DeviceTypeUsbController,
		DeviceTypeUsbHub//,
//		DeviceTypeUsbDevice
	};

	typedef struct NotifyStruct
	{
		Device* Device;
		DeviceType Type;
		CStdString Hub;
		int32_t HubIndex;
		int32_t Event;
	};
	DeviceClass(LPCGUID iFaceGuid, LPCGUID devGuid, LPCTSTR enumerator, DeviceType type);
	virtual ~DeviceClass(void);
	virtual DeviceType GetDeviceType() { return _deviceClassType; };
	virtual HDEVINFO GetClassDevs();
	int32_t EnumDeviceInterfaceDetails(int32_t index, CStdString& devPath, PSP_DEVINFO_DATA pDevData);

protected:
	StdStringArray _filters;
	DeviceType _deviceClassType;
	std::list<Device*> _devices;
	int32_t RefreshPort(const CStdString hubPath, const int32_t hubIndex);
	int32_t ClearPort(const CStdString hubPath, const int32_t hubIndex);
	HDEVINFO _deviceInfoSet;
	std::list<Device*> _oldDevices;

public:
	// PROPERTIES
	GuidProperty(_classIfaceGuid);
	GuidProperty(_classDevGuid);
	StringProperty(_enumerator);
	class classIconIndex : public Int32Property { public: int32_t get(); }_classIconIndex;
	class classDesc : public StringProperty { public: CStdString get(); }_classDesc;
private:
	SP_CLASSIMAGELIST_DATA _imageListData;

public:
	virtual Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);
	PSP_CLASSIMAGELIST_DATA ImageListPtr();

public:
	HIMAGELIST ImageList();
	HANDLE devicesMutex;
	virtual std::list<Device*>& Devices();
	virtual std::list<Device*>& Refresh();
	size_t SetFilters(StdStringArray filters);
	StdStringArray& GetFilters();
	size_t ClearFilters();
	virtual size_t AddFilter(uint16_t vid, uint16_t pid);
	virtual size_t AddFilter(LPCTSTR vid, LPCTSTR pid = NULL, LPCTSTR instance = NULL);
	virtual NotifyStruct AddUsbDevice(LPCTSTR path);
	virtual NotifyStruct RemoveUsbDevice(LPCTSTR path);
	virtual CStdString ToString() = 0;// { return _T("RecoveryDeviceClass"); }
};
