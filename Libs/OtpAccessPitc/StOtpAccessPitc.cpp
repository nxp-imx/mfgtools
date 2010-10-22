/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "StOtpAccessPitc.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StOtpAccessPitc : public StPitc Implementation
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StOtpAccessPitc::StOtpAccessPitc(Device * const device, LPCTSTR fileName)
 : StPitc(device, fileName) 
{
	api::HidInquiry apiInquiry(api::HidInquiry::InfoPage_Chip);
	device->SendCommand(apiInquiry);
	
	_ChipId = apiInquiry.GetChipId();
}

StOtpAccessPitc::StOtpAccessPitc(Device * const device)
 : StPitc(device) 
{
	api::HidInquiry apiInquiry(api::HidInquiry::InfoPage_Chip);
	device->SendCommand(apiInquiry);
	
	_ChipId = apiInquiry.GetChipId();

	switch ( _ChipId )
	{
		case 0x3780:
			GetFwComponent().Load(_T("OtpAccessPitc.3780.sb"), StFwComponent::LoadFlag_FileFirst);
			break;
		case 0x37B0:
		case 0x3770:
//			GetFwComponent().Load(_T("OtpAccessPitc.3770.sb"), StFwComponent::LoadFlag_FileFirst);
//			break;
		case 0x3700:
			GetFwComponent().Load(_T("OtpAccessPitc.3700.sb"), StFwComponent::LoadFlag_FileFirst);
			break;
		default:
			// invalid device
			_ChipId = 0;
	}
}

StOtpAccessPitc::~StOtpAccessPitc(void)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpAccessPitc::GetOtpRegisterInfo(const uint8_t reg, HidPitcInquiry::OtpRegInfoPage * const pInfo)
// 
// - Gets all available information about the specified OTP register in the form of an OtpRegInfoPage.
//
// - Param: (IN)  const uint8_t reg - The register to get information about.
//          (OUT) HidPitcInquiry::OtpRegInfoPage * const pInfo - The address of a OtpInfoPage struct that receives the information.
//
// - Returns: uint32_t : ERROR_SUCCESS(0) - No errors
//                     
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 uint32_t StOtpAccessPitc::GetOtpRegisterInfo(const uint8_t reg, HidPitcInquiry::OtpRegInfoPage * const pInfo)
{
    HidPitcInquiry api(HidPitcInquiry::InfoPage_OtpReg, reg);

    uint32_t err = SendPitcCommand(api);

    if ( err == ERROR_SUCCESS )
	{
        *pInfo = api.GetOtpRegInfoPage();
	}

    return err;
 }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpAccessPitc::GetOtpRegisterInfo(const uint8_t reg, CStdString desc)
// 
// - Gets all available information about the specified OTP register in the form of an OtpRegInfoPage.
//
// - Param: (IN)  const uint8_t reg - The register to get information about.
//          (OUT) CStdString desc - The string version of the OtpInfoPage struct.
//
// - Returns: uint32_t : ERROR_SUCCESS(0) - No errors
//                     
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StOtpAccessPitc::GetOtpRegisterInfo(const uint8_t reg, CStdString& desc)
{
    HidPitcInquiry api(HidPitcInquiry::InfoPage_OtpReg, reg);

    uint32_t err = SendPitcCommand(api);

    if ( err == ERROR_SUCCESS )
	{
		desc = api.ResponseString();
	}
    
	return err;
 }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpAccessPitc::OtpRegisterRead(const uint8_t reg) const
// 
// - Reads the 32-bit contents of the specified OTP register.
//
// - Param: (IN)  const uint8_t reg - The register to read.
//          (OUT) uint32_t * const pValue - The value read from the register.
//
// - Returns: uint32_t : ERROR_SUCCESS(0) - No errors
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StOtpAccessPitc::OtpRegisterRead(const uint8_t reg, uint32_t * const pValue)
{ 
    HidPitcRead api(reg, 1, 0, sizeof(uint32_t)); 

    uint32_t err = SendPitcCommand(api);

    if ( err == ERROR_SUCCESS )
	{ 
		// turn the value around after we read it
		*pValue = api.Swap4(api.GetDataPtr());
	}

    return err;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpAccessPitc::OtpRegisterWrite(const uint8_t reg, const uint32_t value) const
// 
// - Writes the specified 32-bit value to the specified OTP register.
//
// - Param: (IN) const uint8_t reg - The register to write.
//          (IN) const uint32_t value - The value to write to the register.
//          (IN) const bool lock - Lock the register after the write.
//
// - Returns: uint32_t : ERROR_SUCCESS(0) - No errors
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StOtpAccessPitc::OtpRegisterWrite(const uint8_t reg, const uint32_t value, const bool lock)
{
	// turn the value around before we write it.
	uint32_t regValue = api::StApi::Swap4((uint8_t*)&value);

	HidPitcWrite api(reg, 1, lock, (uint8_t*)&regValue, sizeof(uint32_t));

    uint32_t err = SendPitcCommand(api);

    return err;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpAccessPitc::OtpRegisterLock(const uint8_t reg) const
// 
// - Sets the LOCK_BIT in the HW_OCOTP_LOCK register corresponding to the specified OTP register.
//
// - Param: (IN) const uint8_t reg - The register to lock.
//
// - Returns: uint32_t : ERROR_SUCCESS(0) - No errors
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t StOtpAccessPitc::OtpRegisterLock(const uint8_t reg)
{
	uint32_t error = ERROR_SUCCESS;

	// First get the info for the OTP Register so we know what the lock bit is.
	HidPitcInquiry::OtpRegInfoPage otpInfo;
	error = GetOtpRegisterInfo(reg, &otpInfo);
	if ( error != ERROR_SUCCESS )
	{
		return error;
	}
	
	// Now get the current value of the HW_OCOTP_LOCK register
	uint32_t regValue = 0;
	error = OtpRegisterRead(16/*HW_OCOTP_LOCK*/, &regValue);
	if ( error != ERROR_SUCCESS )
	{
		return error;
	}

	// OR in the new lock bit
	regValue |= 1 << otpInfo.LockBit;
		
	// Update the HW_OCOTP_LOCK register
	error = OtpRegisterWrite(16/*HW_OCOTP_LOCK*/, regValue, false);

    return error;
}
