/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
class MxHidDeviceClass : public DeviceClass
{
friend class DeviceManager;
private:
	MxHidDeviceClass(void);
	virtual ~MxHidDeviceClass(void);

	size_t AddFilter(uint16_t vid, uint16_t pid);
	size_t AddFilter(LPCTSTR Prefix, uint16_t vid, uint16_t pid);
	size_t AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance = _T(""));

private:
   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

public:
	CStdString ToString() { return _T("MxHidDeviceClass"); }
};
