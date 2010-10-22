/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "Libs/DevSupport/StPitc.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class StOtpAccessPitc
// 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StOtpAccessPitc : public StPitc
{
public:
    StOtpAccessPitc(Device * const device, LPCTSTR fileName);
    StOtpAccessPitc(Device * const device);
    virtual ~StOtpAccessPitc(void);

	uint32_t GetOtpRegisterInfo( const uint8_t reg, HidPitcInquiry::OtpRegInfoPage * const pInfo );
	uint32_t GetOtpRegisterInfo(const uint8_t reg, CStdString& desc);
    uint32_t OtpRegisterRead( const uint8_t reg, uint32_t * const pValue );
    uint32_t OtpRegisterWrite( const uint8_t reg, const uint32_t value, const bool lock );
	uint32_t OtpRegisterLock(const uint8_t reg);
	uint16_t GetChipId() const { return _ChipId; };
private:
	uint16_t _ChipId;
};
