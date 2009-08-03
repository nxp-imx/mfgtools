#pragma once

#include "Device.h"
//#include "DiskDeviceClass.h"
#include "StApi.h"

/// <summary>
/// A volume device.
/// </summary>
class Volume : public Device
{
public:
    enum LockType{ LockType_Physical, LockType_Logical };

	Volume(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~Volume(void);

	SENSE_DATA _scsiSenseData; //TODO: move this?
	uint8_t _scsiSenseStatus; //TODO: move this?
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
	virtual CStdString GetSendCommandErrorStr();
	virtual uint32_t ResetChip();
	virtual uint32_t ResetToRecovery();
	virtual uint32_t OldResetToRecovery();
	int32_t WriteDrive(const HANDLE hVolume, const uint8_t driveNumber, const std::vector<uint8_t>& data, const size_t dataSize, Device::UI_Callback callback);
	int32_t ReadDrive(const HANDLE hVolume, const uint8_t driveNumber, std::vector<uint8_t>& data, const size_t dataSize, Device::UI_Callback callback);
	HANDLE Lock(LockType lockType);
    int32_t Unlock(HANDLE hDrive);
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

	class SenseKey
	{
	public:
		SenseKey(uint8_t key)
		{
			switch (key)
			{
				case SCSI_SENSE_NO_SENSE:
					_str = _T("NO_SENSE");
					break;
				case SCSI_SENSE_RECOVERED_ERROR:
					_str = _T("RECOVERED_ERROR");
					break;
				case SCSI_SENSE_NOT_READY:
					_str = _T("NOT_READY");
					break;
				case SCSI_SENSE_MEDIUM_ERROR:
					_str = _T("MEDIUM_ERROR");
					break;
				case SCSI_SENSE_HARDWARE_ERROR:
					_str = _T("HARDWARE_ERROR");
					break;
				case SCSI_SENSE_ILLEGAL_REQUEST:
					_str = _T("ILLEGAL_REQUEST");
					break;
				case SCSI_SENSE_UNIT_ATTENTION:
					_str = _T("UNIT_ATTENTION");
					break;
				case SCSI_SENSE_DATA_PROTECT:
					_str = _T("DATA_PROTECT");
					break;
				case SCSI_SENSE_BLANK_CHECK:
					_str = _T("BLANK_CHECK");
					break;
				case SCSI_SENSE_UNIQUE:
					_str = _T("VENDOR_SPECIFIC");
					break;
				case SCSI_SENSE_COPY_ABORTED:
					_str = _T("COPY_ABORTED");
					break;
				case SCSI_SENSE_ABORTED_COMMAND:
					_str = _T("ABORTED_COMMAND");
					break;
				case SCSI_SENSE_EQUAL:
					_str = _T("EQUAL");
					break;
				case SCSI_SENSE_VOL_OVERFLOW:
					_str = _T("OVERFLOW");
					break;
				case SCSI_SENSE_MISCOMPARE:
					_str = _T("MISCOMPARE");
					break;
				case SCSI_SENSE_RESERVED:
					_str = _T("RESERVED");
					break;
				default:
					_str = _T("UNKNOWN");
			}
			_str.AppendFormat(_T("(%02X)"), key);
		}
		CStdString& ToString() { return _str; };
	private:
		CStdString _str;
	};
};

