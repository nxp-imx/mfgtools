/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#ifdef CUSTOM_BUILD
    #pragma message("including Customization/StDeviceDefs.h")
    #include "../Customization/StDeviceDefs.h"
#else
//    #pragma message("including Apps/*/Product.h")
    #include "StDeviceDefs.h"
#endif

#include "../Common/StdString.h"
using namespace stdstring;

class CStDeviceInfo
{
public:
    CStDeviceInfo
    (
		CStdString _usb_vendor_id			= UsbVendorId,
		CStdString _usb_prdct_id			= UsbPrdctId,
		CStdString _scsi_mfg_str			= ScsiMfgStr,
		CStdString _scsi_prdct_str			= ScsiPrdctStr,
		CStdString _mtp_vendor_str			= MtpVendorStr,
		CStdString _mtp_prdct_str			= MtpPrdctStr,
		CStdString _scsi_updater_mfg_str	= ScsiUpdaterMfgStr,
		CStdString _scsi_updater_prdct_str	= ScsiUpdaterPrdctStr,
		CStdString _usb_updater_vendor_id	= UsbUpdaterVendorId,
		CStdString _usb_updater_prdct_id	= UsbUpdaterPrdctId,
		CStdString _scsi_volume				= ScsiVolumes,
        CStdString _drive_array_str			= DriveArrayStr
    )
     : usb_vendor_id(_usb_vendor_id)
	 , usb_prdct_id(_usb_prdct_id)
     , scsi_mfg_str(_scsi_mfg_str)
	 , scsi_prdct_str(_scsi_prdct_str)
	 , mtp_vendor_str(_mtp_vendor_str)
	 , mtp_prdct_str(_mtp_prdct_str)
	 , scsi_updater_mfg_str(_scsi_updater_mfg_str)
	 , scsi_updater_prdct_str(_scsi_updater_prdct_str)
	 , usb_updater_vendor_id(_usb_updater_vendor_id)
	 , usb_updater_prdct_id(_usb_updater_prdct_id)
	 , scsi_volume(_scsi_volume)
	 , drive_array_str(_drive_array_str)
	{
	};

	CStdString scsi_volume;
    CStdString scsi_mfg_str;
	CStdString scsi_prdct_str;
	CStdString mtp_vendor_str;
	CStdString mtp_prdct_str;
	CStdString usb_vendor_id;
	CStdString usb_prdct_id;
	CStdString scsi_updater_mfg_str;
	CStdString scsi_updater_prdct_str;
	CStdString usb_updater_vendor_id;
	CStdString usb_updater_prdct_id;
    CStdString drive_array_str;
};
