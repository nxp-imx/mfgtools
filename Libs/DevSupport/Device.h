#pragma once

#include "SetupApi.h"
#include "DeviceClass.h"
#include "Property.h"
#include "WindowsVersionInfo.h"
#include "StApi.h"

#include "Libs\Loki\Singleton.h"
#include "Libs\Loki\Functor.h"

class DeviceClass;
class CMutex;

class Device : public Property
{
	friend class DeviceClass;
	//! \brief Needed by SingletonHolder to create class.
    friend struct Loki::CreateUsingNew<Device>;
public:
	Device(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~Device(void);

	static const uint32_t MaxTransferSizeInBytes	= 128 * 512; //(128 sectors of 512 bytes each at a time) says who??
//	static const uint32_t MaxTransferSizeInBytes	= 2 * 1024 * 1024; //2 MB per Frank Li

	typedef enum DeviceCapabilities
    {
        Unknown = 0x00000000,
        // matches cfmgr32.h CM_DEVCAP_* definitions

        LockSupported = 0x00000001,
        EjectSupported = 0x00000002,
        Removable = 0x00000004,
        DockDevice = 0x00000008,
        UniqueId = 0x00000010,
        SilentInstall =0x00000020,
        RawDeviceOk = 0x00000040,
        SurpriseRemovalOk = 0x00000080,
        HardwareDisabled = 0x00000100,
        NonDynamic = 0x00000200,
    };
	typedef struct NotifyStruct
	{
        enum dataDir { dataDir_Off, dataDir_FromDevice, dataDir_ToDevice };
        NotifyStruct(LPCTSTR name, dataDir dir, uint32_t max)
            : inProgress(true)
            , direction(dir)
            , position(0)
			, maximum(max)
            , error(ERROR_SUCCESS)
            , status(name){};
        bool inProgress;
        dataDir direction;
        uint32_t position;
		uint32_t maximum;
        int32_t error;
		CStdString status;
	};

 	//! \brief Callback function typedef. i.e. void OnUpdateUiProgress(const NotifyStruct&)
    typedef Loki::Functor<void, LOKI_TYPELIST_1(const NotifyStruct&), Loki::ClassLevelLockable> UI_Callback;

	uint32_t InstanceHandle();
	DeviceCapabilities Capabilities();
	virtual bool IsUsb();
	virtual bool ValidateUsbIds();
	Device* Parent();
	virtual Device* UsbDevice();
	virtual std::list<Device*>& RemovableDevices();
	DWORD Eject(bool allowUI);
	virtual int32_t CompareTo(Device* device);
	virtual void RegisterProperties();
	virtual bool SelfTest(bool eject = false);
	virtual uint32_t SendCommand(StApi& api, uint8_t* additionalInfo = NULL) { return ERROR_CALL_NOT_IMPLEMENTED; };
//	virtual CStdString GetSendCommandErrorStr() { return CStdString(); };
	virtual uint32_t ResetChip() { return ERROR_CALL_NOT_IMPLEMENTED; };
	virtual uint32_t ResetToRecovery() { return ERROR_CALL_NOT_IMPLEMENTED; };
	virtual uint32_t OldResetToRecovery() { return ERROR_CALL_NOT_IMPLEMENTED; };
    HANDLE RegisterCallback(UI_Callback callbackFn);
    bool UnregisterCallback(HANDLE hObserver = 0);
	uint32_t GetDeviceType();

public:
	// PROPERTIES
	class path : public StringProperty { public: CStdString get(); }_path;
	class usbPath : public StringProperty { public: CStdString get(); }_usbPath;
    class classStr : public StringProperty { public: CStdString get(); }_classStr;
    class classGuid : public GuidProperty { public: LPGUID get(); }_classGuid;
    class description : public StringProperty { public: CStdString get(); }_description;
    class friendlyName : public StringProperty { public: virtual CStdString get(); }_friendlyName;
	class driver : public StringProperty { public: CStdString get(); }_driver;
	class deviceInstanceID : public StringProperty { public: CStdString get(); }_deviceInstanceID;
	class enumerator : public StringProperty { public: CStdString get(); }_enumerator;
	class classDesc : public StringProperty { public: CStdString get(); }_classDesc;
	class classIconIndex : public Int32Property { public: int32_t get(); }_classIconIndex;
	class hub : public StringProperty { public: CStdString get(); }_hub;
	class hubIndex : public Int32Property { public: int32_t get(); }_hubIndex;
	class maxPacketSize : public Int32Property { public: int32_t get();}_maxPacketSize;

protected:
    DeviceClass *_deviceClass;
    std::map<HANDLE, UI_Callback> _callbacks;
    void Notify(const NotifyStruct& nsInfo);
	static CMutex m_mutex;
private:
	Device      *_parent;
    DeviceCapabilities _capabilities;
	DEVINST _hDevInst;
	HDEVINFO _deviceInfoSet;
    SP_DEVINFO_DATA _deviceInfoData;
	std::list<Device*> _removableDevices;
	void Trash();
    CStdString _classGuidStr;
	DWORD InitDevInfo();
	CStdString GetProperty(DWORD property, CStdString defaultValue);
	DWORD GetProperty(DWORD property, DWORD defaultValue);
};
