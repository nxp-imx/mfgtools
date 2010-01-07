// UsbPort.h : header file
//
#pragma once

#pragma warning( disable : 4200 )
#include "Libs/WDK/usbioctl.h"
#pragma warning( default : 4200 )

#include "Property.h"
#include "Device.h"

namespace usb
{
	class Hub;

	// usb::Port
	class Port : public Property
	{
	public:
		Port(Hub* pHub, int32_t index);
		virtual ~Port(void);
		Hub *const _parentHub;
		int32_t Refresh();
		void Clear();
		CStdString GetUsbDevicePath(void);
		CStdString GetDriverKeyName(void);
		CStdString GetDeviceDescription(void);
		CStdString GetDriveLetters(void);
		uint32_t GetDeviceType(void);
		int32_t GetIndex(void) {return _index.get();};
		CStdString GetName(void);


		// PROPERTIES
		Int32Property _index;
		Int32Property _connected;
		Int32Property _isHub;
//		StringProperty _driverName; 
		Device* _device;

	private:
		Hub* FindHub(LPCTSTR driverName);
		Device* FindDevice(LPCTSTR driverName);
	
		USB_NODE_CONNECTION_INFORMATION_EX _connectionInfo;
	/*
		// PROPERTIES
		class volumeName : public StringProperty { public: CStdString get(); }_volumeName;
		class logicalDrive : public StringProperty { public: CStdString get(); }_logicalDrive;

	//	int GetDriveCount(void);
		CString GetDevicePath(void);
		CString GetDriveLetters(void);
		CString GetDriverKeyName(void);
		CString GetDeviceDescription(void);

		DWORD Refresh(bool DriveLetterRefreshOnly = false);
		void GetPortData(HANDLE HubHandle);
	//	NODE_CONNECTION_INFORMATION GetConnectionInformation(void);

	protected:
		int m_iDriveCount;
		HANDLE m_hHubHandle;

		CString m_sDevicePath;
		CString m_sDriveLetter;
		CString m_sDriverKeyName;
		CString m_sDeviceDescription;

		HDEVINFO m_HardwareDeviceInfo;
		SP_DEVINFO_DATA m_DeviceInfoData;
	//	NODE_CONNECTION_INFORMATION m_ConnectionInformation;

		DWORD GetDeviceDescAndPath(void);
		USHORT GetDeviceDescriptor(USHORT LanguageID, PUCHAR BufferPtr);
		USHORT GetConfigurationDescriptor(USHORT LanguageID);
		USHORT GetStringDescriptor(USHORT LanguageID, UCHAR Index);	
		void GetDriveLetters(PSP_DEVINFO_DATA pDeviceInfoData);
		CString GetDeviceRegistryProperty(DWORD Property);
	*/
	};
}