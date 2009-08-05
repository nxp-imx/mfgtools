#pragma once

#include "Device.h"
//#include "DiskDeviceClass.h"
#include "StApi.h"

class UpdateTransportProtocol;

/// <summary>
/// A volume device.
/// </summary>
class Volume : public Device
{
	friend class UpdateTransportProtocol;
public:
    enum LockType{ LockType_Physical, LockType_Logical };

	Volume(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~Volume(void);

//	SENSE_DATA _scsiSenseData; //TODO: move this?
//	uint8_t _scsiSenseStatus; //TODO: move this?
	uint32_t m_BytesWritten;
	uint32_t m_BytesRead;
//	bool IsUsb();
//	Device* UsbDevice();
//	bool ValidateUsbIds();
//	std::list<Device*>& Disks();
//	std::list<Device*>& RemovableDevices();
	int32_t CompareTo(Device* device);
	bool SelfTest(bool eject = true);
	virtual uint32_t SendCommand(StApi& api, uint8_t* additionalInfo = NULL);
//	virtual CStdString GetSendCommandErrorStr();
	virtual uint32_t ResetChip();
	virtual uint32_t ResetToRecovery();
	virtual uint32_t OldResetToRecovery();
	int32_t WriteDrive(const HANDLE hVolume, const uint8_t driveNumber, const std::vector<uint8_t>& data, const size_t dataSize, Device::UI_Callback callback);
	int32_t ReadDrive(const HANDLE hVolume, const uint8_t driveNumber, std::vector<uint8_t>& data, const size_t dataSize, Device::UI_Callback callback);
	HANDLE Lock(LockType lockType);
    int32_t Unlock(HANDLE hDrive, bool close = true);
    int32_t Dismount(HANDLE hDrive);
    int32_t Update(HANDLE hDrive);
	// PROPERTIES
	class volumeName : public StringProperty { public: CStdString get(); }_volumeName;
	class logicalDrive : public StringProperty { public: CStdString get(); }_logicalDrive;
	class friendlyName : public StringProperty { public: CStdString get(); }_friendlyName;
	class diskNumber : public Int32Property { public: diskNumber(int32_t val):Int32Property(val){}; int32_t get(); }_diskNumber;

//private:
    uint32_t SendCommand(HANDLE hDrive, StApi& api, uint8_t* additionalInfo, NotifyStruct& nsInfo);
private:

#pragma pack (push, 1)

	struct _NT_SCSI_REQUEST
	{
		SCSI_PASS_THROUGH PassThrough;
		uint64_t          Tag;
		SENSE_DATA        SenseData;
		uint8_t           DataBuffer[1];          // Allocate buffer space
	};
#pragma pack (pop)
	int32_t WaitForCmdToFinish();
//	SENSE_DATA _scsiSenseData; //TODO: move this?
//	uint8_t _scsiSenseStatus; //TODO: move this?
	OVERLAPPED _overLapped;
	HANDLE _hEvent;
//	std::vector<uint32_t> _diskNumbers;
// 	DiskDeviceClass _disksClass;
//	std::list<Device*> _disks;
//	std::list<Device*> _removableDevices;
	void Trash();
//	std::vector<uint32_t>& DiskNumbers();
};

