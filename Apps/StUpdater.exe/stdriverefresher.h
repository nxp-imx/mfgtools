#pragma once

class CStDriveRefresher : public CStBase
{
public:
	CStDriveRefresher( CStError* _p_error );
	~CStDriveRefresher(void);

	ST_ERROR RefreshDevice( USHORT _usb_vendor_id, USHORT _usb_product_id, CStProgress* _p_progress);

private:

	CString ConstructDeviceString( USHORT _usb_vendor_id, USHORT _usb_product_id, char* _device_string, rsize_t _bufsize );
	ST_ERROR DisableEnableDevice(const char* _device_string, CStProgress* _p_progress);
	ST_ERROR CONFIGRET_to_ST_ERROR( DWORD _cfg_err );

	CStError* m_p_error;
	PLATFORM m_platform;

};
