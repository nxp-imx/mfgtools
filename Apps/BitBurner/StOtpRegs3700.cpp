#include "StOtpRegs3700.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpRegs IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const uint32_t StOtpRegs::OtpField::FieldMask[] = { 0x00000000, 0x00000001, 0x00000003, 0x00000007, 
																		 0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
																		 0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
																		 0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
																		 0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
																		 0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
																		 0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
																		 0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
																		 0xFFFFFFFF };

const CStdString StOtpRegs::OtpField::Format(const uint32_t regValue) const
{
    CStdString str;
	uint32_t fieldValue = Value(regValue);
	
	if ( ValueList.empty() )
	{
		int dataSize = 0; 
		DWORD tempMax = FieldMask[Length];
		while ( tempMax & 0x0F )
		{
			++dataSize;
			tempMax >>= 4;
		}
		CStdString strFormat;
		strFormat.Format(_T("0x%%0%dX (%%d)"), dataSize);

		str.Format(strFormat, fieldValue, fieldValue);
	}
	else
	{
        std::map<uint32_t, CStdString>::const_iterator key;
        key = ValueList.find(fieldValue);
        if ( key == ValueList.end() )
            str = _T("Not found.");
        else
            str = key->second;
	}

    return str;
}

const uint32_t StOtpRegs::OtpField::Value(const uint32_t regValue) const
{
	return (regValue >> Offset) & FieldMask[Length];
}


void StOtpRegs::OtpRegister::AddField(LPCTSTR name, const uint32_t offset, const uint8_t length, LPCTSTR desc, const std::map<uint32_t,CStdString> valueList)
{
    OtpField field(name, offset, length, desc, valueList);
    Fields.push_back(field);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpRegs3700 IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StOtpRegs3700::StOtpRegs3700()
{
    std::map<uint32_t,CStdString> valueList;

/*	{
		OtpRegister reg(HW_OCOTP_CTRL, _T("HW_OCOTP_CTRL"), _T("OTP Controller Control"), OtpRegType_Control);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_DATA, _T("HW_OCOTP_DATA"), _T("OTP Controller Write Data"), OtpRegType_Control);
        _registers.push_back(reg);
    }
*/  {
		OtpRegister reg(HW_OCOTP_CUST0, _T("HW_OCOTP_CUST0"), _T("Customer0"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST1, _T("HW_OCOTP_CUST1"), _T("Customer1"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST2, _T("HW_OCOTP_CUST2"), _T("Customer2"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST3, _T("HW_OCOTP_CUST3"), _T("Customer3"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
        OtpRegister reg(HW_OCOTP_CRYPTO0, _T("HW_OCOTP_CRYPTO0"), _T("Crypto Key0"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

       _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO1, _T("HW_OCOTP_CRYPTO1"), _T("Crypto Key1"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO2, _T("HW_OCOTP_CRYPTO2"), _T("Crypto Key2"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO3, _T("HW_OCOTP_CRYPTO3"), _T("Crypto Key3"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP0, _T("HW_OCOTP_HWCAP0"), _T("HW Capability0"), OtpRegType_HwCaps);
		
		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ADC_DISABLE (0)"),            0, 1, _T("Disables A/D converter."), valueList);
		reg.AddField(_T("SPDIF_DISABLE (1)"),          1, 1, _T("Disables SPDIF."), valueList);
        reg.AddField(_T("DRI_DISABLE (2)"),            2, 1, _T("Disables Digital Radio Interface."), valueList);
        reg.AddField(_T("I2C_MASTER_DISABLE (3)"),     3, 1, _T("Disables I2C master."), valueList);
        reg.AddField(_T("I2C_SLAVE_DISABLE (4)"),      4, 1, _T("Disables I2C slave."), valueList);
        reg.AddField(_T("TIMER0_DISABLE (5)"),         5, 1, _T("Disables timer 0."), valueList);
        reg.AddField(_T("TIMER1_DISABLE (6)"),         6, 1, _T("Disables timer 1."), valueList);
        reg.AddField(_T("TIMER2_DISABLE (7)"),         7, 1, _T("Disables timer 2."), valueList);
        reg.AddField(_T("TIMER3_DISABLE (8)"),         8, 1, _T("Disables timer 3."), valueList);
        reg.AddField(_T("ROT_DISABLE (9)"),            9, 1, _T("Disables rotary decoder."), valueList); // encoder?
        reg.AddField(_T("PWM0_DISABLE (10)"),         10, 1, _T("Disables Pulse Width Modulator 0."), valueList);
        reg.AddField(_T("PWM1_DISABLE (11)"),         11, 1, _T("Disables Pulse Width Modulator 1."), valueList);
        reg.AddField(_T("PWM2_DISABLE (12)"),         12, 1, _T("Disables Pulse Width Modulator 2."), valueList);
        reg.AddField(_T("PWM3_DISABLE (13)"),         13, 1, _T("Disables Pulse Width Modulator 3."), valueList);
        reg.AddField(_T("PWM4_DISABLE (14)"),         14, 1, _T("Disables Pulse Width Modulator 4."), valueList);
        reg.AddField(_T("LCDIF_DISABLE (15)"),        15, 1, _T("Disables LCD interface."), valueList);
        reg.AddField(_T("LRADC_TEMP1_DISABLE (16)"),  16, 1, _T("Disables LRADC temp1 current source."), valueList);
        reg.AddField(_T("LRADC_TEMP0_DISABLE (17)"),  17, 1, _T("Disables LRADC temp0 current source."), valueList);
        reg.AddField(_T("LRADC_TOUCH_DISABLE (18)"),  18, 1, _T("Disables LRADC touch screen controler."), valueList);
        reg.AddField(_T("LRADC5_DISABLE (19)"),       19, 1, _T("Disables LRADC channel 5 conversions."), valueList);
        reg.AddField(_T("LRADC4_DISABLE (20)"),       20, 1, _T("Disables LRADC channel 4 conversions."), valueList);
        reg.AddField(_T("LRADC3_DISABLE (21)"),       21, 1, _T("Disables LRADC channel 3 conversions."), valueList);
        reg.AddField(_T("LRADC2_DISABLE (22)"),       22, 1, _T("Disables LRADC channel 2 conversions."), valueList);
        reg.AddField(_T("LRADC1_DISABLE (23)"),       23, 1, _T("Disables LRADC channel 1 conversions."), valueList);
        reg.AddField(_T("LRADC0_DISABLE (24)"),       24, 1, _T("Disables LRADC channel 0 conversions."), valueList);
        reg.AddField(_T("Reserved (25)"),             25, 1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("RTC_DISABLE (26)"),          26, 1, _T("Disables the preservation of real time over power downs."), valueList);
        reg.AddField(_T("RTC_ALARM_DISABLE (27)"),    27, 1, _T("Disables the alarm function."), valueList);
        reg.AddField(_T("RTC_WATCHDOG_DISABLE (28)"), 28, 1, _T("Disables the watchdog timer."), valueList);
		
		valueList.clear();
		reg.AddField(_T("Reserved (31:29)"),          29, 3, _T("Reserved - do not blow these bits."), valueList);
        
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP1, _T("HW_OCOTP_HWCAP1"), _T("HW Capability1"), OtpRegType_HwCaps);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("PINCTRL_BANK0_DISABLE (0)"),      0,  1, _T("Disables GPIO BANK0."), valueList);
		reg.AddField(_T("PINCTRL_BANK1_DISABLE (1)"),      1,  1, _T("Disables GPIO BANK1."), valueList);
		reg.AddField(_T("PINCTRL_BANK2_DISABLE (2)"),      2,  1, _T("Disables GPIO BANK2."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:3)"),                3, 13, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SAIF1_DISABLE (16)"),            16,  1, _T("Disables SAIF1."), valueList);
		reg.AddField(_T("SAIF2_DISABLE (17)"),            17,  1, _T("Disables SAIF2."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (20:18)"),              18,  3, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
		reg.AddField(_T("BATTERY_CHARGE_DISABLE (21)"),   21,  1, _T("Disables battery charger."), valueList);

		valueList.clear();
		valueList[0] = _T("00");
		valueList[1] = _T("01");
		valueList[3] = _T("10");
		valueList[4] = _T("11");

		reg.AddField(_T("LOW_PWRCTRL_DISABLE (23:22)"),   22,  2, _T("Used in conjunction with LOWPWR_DISABLE[0] OTP bit to selectively connect two shunts between the battery pin and ground to disable two modes of low power operation (see LOWPWR_DISABLE)."), valueList);
		reg.AddField(_T("LOWPWR_DISABLE (25:24)"),        24,  2, _T("Used to disable three modes of low power operation. Connects shunts between battery pin and ground. ..."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("IR_DISABLE (26)"),               26,  1, _T("Disables infrared port."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - All speeds enabled.");
		valueList[1] = _T("01 - VFIR disabled, SIR to FIR enabled.");
		valueList[3] = _T("10 - SIR to MIR only enabled.");
		valueList[4] = _T("11 - SIR only enabled.");

		reg.AddField(_T("IR_SPEED (28:27)"),              27,  2, _T("Infrared Port Speed Select. 00 - All speeds enabled; 01 - VFIR disabled, SIR to FIR enabled; 10 - SIR to MIR only enabled; 11 - SIR only enabled."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("APPUART_DISABLE (29)"),          29,  1, _T("Disables all application UART capability."), valueList);
		reg.AddField(_T("APPUART_HI_SPEED_DISABLE (30)"), 30,  1, _T("Disables UART baud rates above 115Kb/s."), valueList);

		reg.AddField(_T("DAC_DISABLE (31)"),              31,  1, _T("Disables D/A converter."), valueList);

		_registers.push_back(reg);
	}
    {
	    OtpRegister reg(HW_OCOTP_HWCAP2, _T("HW_OCOTP_HWCAP2"), _T("HW Capability2"), OtpRegType_HwCaps);

		// Field: PACKAGE_TYPE
		valueList.clear();
		valueList[0] = _T("000 - 169 pin BGA");
		valueList[1] = _T("001 - 100 pin BGA");
		valueList[2] = _T("010 - 100 pin TQFP");
		valueList[3] = _T("011 - 128 pin TQFP");
		valueList[4] = _T("100 - Reserved");
		valueList[5] = _T("101 - Reserved");
		valueList[6] = _T("110 - Reserved");
		valueList[7] = _T("111 - Reserved");
		reg.AddField(_T("PACKAGE_TYPE (2:0)"),         0,  3, _T("Package Type: 000 - 169 pin BGA, 001 - 100 pin BGA, 010 - 100 pin TQFP, 011 - 128 pin TQFP, 100 through 111 - reserved."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("Reserved (3)"),               3, 1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("GPMI_DISABLE (4)"),           4, 1, _T("Disables GPMI capability."), valueList);
		reg.AddField(_T("SSP1_DISABLE (5)"),           5, 1, _T("Disables all SSP1 modes."), valueList);
		reg.AddField(_T("SSP1_MEMSTK_DISABLE (6)"),    6, 1, _T("Disables just the memstick capability."), valueList);
		reg.AddField(_T("SSP1_SD_DISABLE (7)"),        7, 1, _T("Disables just the SD and SDIO capabilities."), valueList);
		reg.AddField(_T("SSP2_DISABLE (8)"),           8, 1, _T("Disables all SSP2 modes."), valueList);
		reg.AddField(_T("SSP2_MEMSTK_DISABLE (9)"),    9, 1, _T("Disables just the memstick capability."), valueList);
		reg.AddField(_T("SSP2_SD_DISABLE (10)"),      10, 1, _T("Disables just the SD and SDIO capabilities."), valueList);
		reg.AddField(_T("DCP_CRYPTO_DISABLE (11)"),   11, 1, _T("Disables encryption/decryption/hashing capability within DCP."), valueList);
		reg.AddField(_T("DCP_CSC_DISABLE (12)"),      12, 1, _T("Disables color-space conversion capbility within the DCP."), valueList);
		reg.AddField(_T("JTAG_LOCKOUT (13)"),         13, 1, _T("Disables the JTAG debugger."), valueList);
		reg.AddField(_T("USE_SERIAL_JTAG (14)"),      14, 1, _T("This bit is used by the ROM to copy to the HW_DIGCTL_CTRL_USE_SERIAL_JTAG bit for JTAG boot mode. After it is copied, this bit forces the SJTAG block to switch to the 1-wire serial JTAG mode if it is set. If it is cleared, the alternate 6-wire parallel JTAG backup mode is enabled."), valueList);
		reg.AddField(_T("Reserved (15)"),             15, 1, _T("Reserved - do not blow this bit."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:16)"),          16,16, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP3, _T("HW_OCOTP_HWCAP3"), _T("HW Capability3"), OtpRegType_HwCaps);

		valueList.clear();
		reg.AddField(_T("Reserved (7:0)"),            0, 8, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_STATIC_DISABLE (8)"),     8, 1, _T("Turns off the NOR flash capability."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_DRAM_DISABLE (9)"),       9, 1, _T("Turns off the DDR/SDRAM capability. This does not affect the NOR flash capability."), valueList);
		reg.AddField(_T("USB_NO_OTG (10)"),           10, 1, _T("Disables all OTG functions."), valueList);
		reg.AddField(_T("USB_NO_HOST (11)"),          11, 1, _T("Disables all USB Host capability."), valueList);
		reg.AddField(_T("USB_NO_DEVICE (12)"),        12, 1, _T("Disables all USB device capability."), valueList);
		reg.AddField(_T("USB_NO_HS (13)"),            13, 1, _T("Disables all USB high speed capability."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:14)"),         14, 2, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_LARGE_DRAM_DISABLE (16)"), 16, 1, _T("Disables DDR/SDRAM modules above 2Mbytes. This does not affect the NOR flash capability."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:17)"),         17,15, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP4, _T("HW_OCOTP_HWCAP4"), _T("HW Capability4"), OtpRegType_HwCaps);

		valueList.clear();
		reg.AddField(_T("Reserved (31:0)"),            0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP5, _T("HW_OCOTP_HWCAP5"), _T("HW Capability5"), OtpRegType_HwCaps);
		valueList.clear();
		reg.AddField(_T("Reserved (31:0)"),            0, 32, _T("Reserved - do not blow these bits."), valueList);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_SWCAP, _T("HW_OCOTP_SWCAP"), _T("SW Capability"), OtpRegType_SwCaps);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_AACD_D (0)"),          0,  1, _T("Disables the AAC Decoder."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (2:1)"),             1,  2, _T("Reserved - do not blow these bits."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_MP3E_E (3)"),          3,  1, _T("Enables MP3 Encode."), valueList);
		reg.AddField(_T("SW_CAP_MP3PROD_D (4)"),       4,  1, _T("Disables MP3 Pro Decoder."), valueList);
		reg.AddField(_T("SW_CAP_WMAD_D (5)"),          5,  1, _T("Disables WMA Decode."), valueList);
		reg.AddField(_T("SW_CAP_WMAE_D (6)"),          6,  1, _T("Disables WMA Encode."), valueList);
		reg.AddField(_T("SW_CAP_WMA9DRM_D (7)"),       7,  1, _T("Disables WMA9DRM."), valueList);
		reg.AddField(_T("SW_CAP_JANUSDRM_D (8)"),      8,  1, _T("Disables Janus Digital Rights Management."), valueList);
		reg.AddField(_T("SW_CAP_DIVX_E (9)"),          9,  1, _T("Enables DIVX."), valueList);
		reg.AddField(_T("SW_CAP_MPEG4_E (10)"),       10,  1, _T("Enables MPEG4."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:11)"),          11,  5, _T("Reserved - do not blow these bits."), valueList);
		reg.AddField(_T("TRIM_VBG (18:16)"),          16,  3, _T("Trim value for variable band gap current."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("Reserved (19)"),             19,  1, _T("Reserved - do not blow this bit."), valueList);

		valueList.clear();
		reg.AddField(_T("TRIM_HS_USB (23:20)"),       20,  4, _T("Trim value for high speed USB current."), valueList);
		reg.AddField(_T("Reserved (31:24)"),          24,  8, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUSTCAP, _T("HW_OCOTP_CUSTCAP"), _T("Customer Capability"), OtpRegType_Customer, PermissionType_External);
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

#ifdef INTERNAL_BUILD
		reg.AddField(_T("CUST_JTAG_LOCKOUT (0)"),      0, 1, _T("Disables JTAG debugging capability. This bit will be listed as Reserved in the Tspec."), valueList);
#else
		reg.AddField(_T("Reserved (0)"),               0, 1, _T("Reserved - do not blow this bit."), valueList);
#endif
		reg.AddField(_T("RTC_XTAL_32000_PRESENT (1)"), 1, 1, _T("Set to indicate the presence of an optional 32.000KHz off-chip oscillator."), valueList);
        reg.AddField(_T("RTC_XTAL_32768_PRESENT (2)"), 2, 1, _T("Set to indicate the presence of an optional 32.768KHz off-chip oscillator."), valueList);
		
		valueList.clear();
		reg.AddField(_T("Reserved (31:3)"),            3, 29, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_LOCK, _T("HW_OCOTP_LOCK"), _T("LOCK"), OtpRegType_Control, PermissionType_External);
 
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("HW_OCOTP_CUST0 - LOCK_BIT (0)"),                   0, 1, _T("Indicates HW_OCOTP_CUST0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST1 - LOCK_BIT (1)"),                   1, 1, _T("Indicates HW_OCOTP_CUST1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST2 - LOCK_BIT (2)"),                   2, 1, _T("Indicates HW_OCOTP_CUST2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST3 - LOCK_BIT (3)"),                   3, 1, _T("Indicates HW_OCOTP_CUST3 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CRYPTO0-3 - LOCK_BIT (4)"),               4, 1, _T("Indicates HW_OCOTP_CRYPTO0-3 are locked."), valueList);
#ifdef INTERNAL_BUILD
		reg.AddField(_T("HW_OCOTP_HWCAP0-5,HW_OCOTP_SWCAP - LOCK_BIT (8)"), 8, 1, _T("Indicates HW_OCOTP_HWCAP0-5 and HW_OCOTP_SWCAP are locked."), valueList);
#endif
		reg.AddField(_T("HW_OCOTP_CUSTCAP - LOCK_BIT (9)"),                 9, 1, _T("Indicates HW_OCOTP_HWCAP0-5 and HW_OCOTP_SWCAP are locked."), valueList);
#ifdef INTERNAL_BUILD
		reg.AddField(_T("HW_OCOTP_UN0 - LOCK_BIT (16)"),                   16, 1, _T("Indicates HW_OCOTP_UN0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_UN1 - LOCK_BIT (17)"),                   17, 1, _T("Indicates HW_OCOTP_UN1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_UN2 - LOCK_BIT (18)"),                   18, 1, _T("Indicates HW_OCOTP_UN2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_OPS0-3 - LOCK_BIT (19)"),                19, 1, _T("Indicates HW_OCOTP_OPS0-3 are locked."), valueList);
#endif
		reg.AddField(_T("HW_OCOTP_ROM0 - LOCK_BIT (24)"),                  24, 1, _T("Indicates HW_OCOTP_ROM0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM1 - LOCK_BIT (25)"),                  25, 1, _T("Indicates HW_OCOTP_ROM1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM2 - LOCK_BIT (26)"),                  26, 1, _T("Indicates HW_OCOTP_ROM2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM3 - LOCK_BIT (27)"),                  27, 1, _T("Indicates HW_OCOTP_ROM3 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM4 - LOCK_BIT (28)"),                  28, 1, _T("Indicates HW_OCOTP_ROM4 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM5 - LOCK_BIT (29)"),                  29, 1, _T("Indicates HW_OCOTP_ROM5 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM6 - LOCK_BIT (30)"),                  30, 1, _T("Indicates HW_OCOTP_ROM6 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM7 - LOCK_BIT (31)"),                  31, 1, _T("Indicates HW_OCOTP_ROM7 is locked."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS0, _T("HW_OCOTP_OPS0"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS1, _T("HW_OCOTP_OPS1"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS2, _T("HW_OCOTP_OPS2"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS3, _T("HW_OCOTP_OPS3"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN0, _T("HW_OCOTP_UN0"), _T("Unassigned0"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN1, _T("HW_OCOTP_UN1"), _T("Unassigned1"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN2, _T("HW_OCOTP_UN2"), _T("Unassigned2"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM0, _T("HW_OCOTP_ROM0"), _T("ROM0"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		// Field: Reserved
		reg.AddField(_T("Reserved (0)"),                    0,  1, _T("Reserved - do not blow this bit."), valueList);

		// Field: USE_ALT_DEBUG_UART_PINS
		reg.AddField(_T("USE_ALT_DEBUG_UART_PINS (1)"),     1,  1, _T("Use alternate ROTARYA/B bedug UART RX/TX pins."), valueList);

		// Field: DISABLE_RECOVERY_MODE
#ifdef INTERNAL_BUILD
        reg.AddField(_T("DISABLE_RECOVERY_MODE (2)"),       2,  1, _T("Disables PSWITCH recovery mode via USB. This bit will be listed as Reserved in the Tspec., as this function will only be made available to customers who request it."), valueList);
#else
		reg.AddField(_T("Reserved (2)"),                    2,  1, _T("Reserved - do not blow this bit."), valueList);
#endif
		// Field: ENABLE_UNENCRYPTED_BOOT
		reg.AddField(_T("SD_MBR_BOOT (3)"),                 3,  1, _T("Enables master boot record (MBR) boot mode for SD boot."), valueList);

		// Field: ENABLE_UNENCRYPTED_BOOT
		reg.AddField(_T("ENABLE_UNENCRYPTED_BOOT (4)"),     4,  1, _T("Enables unencrypted boot modes."), valueList);

		// Field: ENABLE_USB_BOOT_SERIAL_NUM
		reg.AddField(_T("ENABLE_USB_BOOT_SERIAL_NUM (5)"),  5,  1, _T("Enables USB boot serial number."), valueList);

		// Field: DISABLE_SPI_NOR_FAST_READ
		reg.AddField(_T("DISABLE_SPI_NOR_FAST_READ (6)"),   6,  1, _T("Disables SPI NOR fast reads which are used by default."), valueList);

		// Field: Reserved
		reg.AddField(_T("Reserved (7)"),                    7,  1, _T("Reserved - do not blow this bit."), valueList);

		// Field: SSP_SCK_INDEX
		valueList.clear();
		reg.AddField(_T("SSP_SCK_INDEX (11:8)"),            8,  4, _T("Index to the SSP clock speed."), valueList);

		// Field: SD_BUS_WIDTH
		valueList.clear();
		valueList[0] = _T("00 - 4-bit");
		valueList[1] = _T("01 - 1-bit");
		valueList[2] = _T("10 - 8-bit");
		valueList[3] = _T("11 - Reserved");
		reg.AddField(_T("SD_BUS_WIDTH (13:12)"),           12,  2, _T("SD card bus width: 00 - 4-bit, 01 - 1-bit, 10 - 8-bit, 11 - Reserved."), valueList);

		// Field: SD_POWER_UP_DELAY
		valueList.clear();
		int i; CStdString str;
		valueList[0] = _T("0x00 - 20ms");
		for ( i=1; i<64; ++i )
		{
			str.Format(_T("0x%02X - %dms"), i, i*10);
			valueList[i] = str;
		}
		reg.AddField(_T("SD_POWER_UP_DELAY (19:14)"),      14,  6, _T("SD card power up delay required after enabling GPIO power gate: 000000 - 0ms, 000001 - 10ms, 00002 - 20ms, ... 111111 - 630ms."), valueList);

		// Field: SD_POWER_GATE_GPIO
		valueList.clear();
		valueList[0] = _T("00 - PWM3");
		valueList[1] = _T("01 - PWM4");
		valueList[2] = _T("10 - ROTARYA");
		valueList[3] = _T("11 - NO_GATE");
		reg.AddField(_T("SD_POWER_GATE_GPIO (21:20)"),     20,  2, _T("SD card power gate GPIO pin select: 00 - PWM3, 01 - PWM4, 10 - ROTARYA, 11 - NO_GATE."), valueList);

		// Field: USE_PARALLEL_JTAG
		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
		reg.AddField(_T("USE_PARALLEL_JTAG (22)"),         22, 1, _T("During JTAG boot mode, the ROM reads this bit, then inverts it, and writes the value to the HW_DIGCTL_USE_SERIAL_JTAG bit. If this bit is one, indicating parallel JATAG mode is selected, a zero is written to the DIGCTL_USE_SERIAL_JTAG bit which places the device into 6-wire JTAG mode. If this bit is zero, a one is written causing the SJTAG block to switch to the 1-wire serial JTAG mode."), valueList);

		// Field: ENABLE_PJTAG_12MA_DRIVE
		reg.AddField(_T("ENABLE_PJTAG_12MA_DRIVE (23)"),   23,  1, _T("Blow to force the 6-wire PJTAG pins to drive 12mA. Default is 4mA. Note that SJTAG is fixed at 8mA. Blowing this bit causes the ROM to program all six parallel JTAG pins to drive 12mA via the pin control registers."), valueList);
		
		// Field: BOOT_MODE
		valueList.clear();
		reg.AddField(_T("BOOT_MODE (31:24)"),              24,  8, _T("Encoded boot mode."), valueList);
		
		// Register: HW_OCOTP_ROM0
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM1, _T("HW_OCOTP_ROM1"), _T("ROM1"), OtpRegType_Rom, PermissionType_External);
        
		valueList.clear();

		reg.AddField(_T("NUMBER_OF_NANDS (2:0)"),       0,  3, _T("Encoded value indicates number of external NAND devices (0 to7). Zero indicates ROM will probe for the number of NAND devices connected to the system."), valueList);
		reg.AddField(_T("Reserved (7:3)"),              3,  5, _T("Reserved - do not blow these bits."), valueList);
		reg.AddField(_T("BOOT_SEARCH_COUNT (11:8)"),    8,  4, _T("Number of 64 page blocks that should be read by the boot loader."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("USE_ALTERNATE_CE (12)"),          12,  1, _T("Directs the boot loader to use the alternate chip enables."), valueList);
		reg.AddField(_T("SD_INIT_SEQ_1_DISABLE (13)"),     13,  1, _T("Disables the first initialization sequence for SD."), valueList);
        reg.AddField(_T("SD_CMD0_DISABLE (14)"),           14,  1, _T("Cmd0 (reset cmd) is called by default to reset the SD card during startup. Blow this bit to not reset hte card during SD boot."), valueList);
        reg.AddField(_T("SD_INIT_SEQ_2_ENSABLE (15)"),     15,  1, _T("Enables the second initialization sequence for SD boot."), valueList);
        reg.AddField(_T("SD_INCREASE_INIT_SEQ_TIME (16)"), 16,  1, _T("Increases the SD card initialization sequence time from 1ms (default) to 4ms."), valueList);
        reg.AddField(_T("SSP1_EXT_PULLUP (17)"),           17,  1, _T("Indicates external pull-ups implemented for SSP1."), valueList);
        reg.AddField(_T("SSP2_EXT_PULLUP (18)"),           18,  1, _T("Indicates external pull-ups implemented for SSP2."), valueList);
        
		valueList.clear();
		reg.AddField(_T("Reserved (31:19)"),               19, 13, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM2, _T("HW_OCOTP_ROM2"), _T("ROM2"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

		reg.AddField(_T("USB_PID (15:0)"),              0, 16, _T("USB Product ID."), valueList);
		reg.AddField(_T("USB_VID (31:16)"),            16, 16, _T("USB Vendor ID."), valueList);
        
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM3, _T("HW_OCOTP_ROM3"), _T("ROM3"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

#ifdef INTERNAL_BUILD
		reg.AddField(_T("OSC_TRIM (9:0)"),              0, 10, _T("Oscillator trim value for off-chip Bluetooth device(Elwood). This bit-field should be described instead as SDK reserved bits in the customer spec."), valueList);
#else
		reg.AddField(_T("Reserved (9:0)"),              0, 10, _T("Reserved - do not blow these bits."), valueList);
#endif
		reg.AddField(_T("Reserved (31:10)"),           10, 22, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM4, _T("HW_OCOTP_ROM4"), _T("ROM4"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

		reg.AddField(_T("Reserved (31:0)"),                0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM5, _T("HW_OCOTP_ROM5"), _T("ROM5"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

        reg.AddField(_T("Reserved (31:0)"),                0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM6, _T("HW_OCOTP_ROM6"), _T("ROM6"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
#ifdef INTERNAL_BUILD
		reg.AddField(_T("DISABLE_TEST_MODES (0)"),      0,  1, _T("Disables manufacturing test modes. This bit will be listed as Reserved in the Data Sheet."), valueList);
#else
		reg.AddField(_T("Reserved (0)"),                0,  1, _T("Reserved - do not blow this bit."), valueList);
#endif
		valueList.clear();
		reg.AddField(_T("Reserved (31:1)"),             1, 31, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM7, _T("HW_OCOTP_ROM7"), _T("ROM7"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ENABLE_PIN_BOOT_CHECK (0)"),   0,  1, _T("Enables the boot loader to first test the LCD_RS pin to determine if pin boot mode is enabled. If this bit is blown, and LCD_RS is pulled high, then boot mode is determined by the state of LCD_D[6:0] pins. If this bit is not blown, skip testing the LCD_RS pin and go directly to determining boot mode by reading the state of LCD_D[6:0]."), valueList);
		reg.AddField(_T("Reserved (1)"),                1,  1, _T("Reserved - do not blow this bit."), valueList);
        reg.AddField(_T("ENABLE_ARM_ICACHE (2)"),       2,  1, _T("Enables the ARM 926 ICache during boot."), valueList);
        reg.AddField(_T("I2C_USE_400KHZ (3)"),          3,  1, _T("Forces the I2C to be programmed by the boot loader to run at 400KHz. 100KHz is the default."), valueList);

		valueList.clear();
#ifdef INTERNAL_BUILD
		reg.AddField(_T("OCRAM_SS_TRIM (7:4)"),         4,  4, _T("On-chip RAM sense-amp speed trim value."), valueList);
#else
		reg.AddField(_T("Reserved (7:4)"),              4,  4, _T("Reserved - do not blow these bits."), valueList);
#endif
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
        reg.AddField(_T("ENABLE_SSP_12MA_DRIVE (8)"),   8,  1, _T("Forces the SSP pins to drive 12mA, default is 4mA."), valueList);
		reg.AddField(_T("RESET_USB_PHY_AT_STARTUP (9)"),9,  1, _T("Resets the USBPHY at startup."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:10)"),           10, 22, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_VERSION, _T("HW_OCOTP_VERSION"), _T("OTP Controller Version"), OtpRegType_Control, PermissionType_External);
        _registers.push_back(reg);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpRegs3770 IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StOtpRegs3770::StOtpRegs3770()
{
    std::map<uint32_t,CStdString> valueList;

/*	{
		OtpRegister reg(HW_OCOTP_CTRL, _T("HW_OCOTP_CTRL"), _T("OTP Controller Control"), OtpRegType_Control);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_DATA, _T("HW_OCOTP_DATA"), _T("OTP Controller Write Data"), OtpRegType_Control);
        _registers.push_back(reg);
    }
*/  {
		OtpRegister reg(HW_OCOTP_CUST0, _T("HW_OCOTP_CUST0"), _T("Customer0"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST1, _T("HW_OCOTP_CUST1"), _T("Customer1"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST2, _T("HW_OCOTP_CUST2"), _T("Customer2"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST3, _T("HW_OCOTP_CUST3"), _T("Customer3"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
        OtpRegister reg(HW_OCOTP_CRYPTO0, _T("HW_OCOTP_CRYPTO0"), _T("Crypto Key0"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

       _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO1, _T("HW_OCOTP_CRYPTO1"), _T("Crypto Key1"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO2, _T("HW_OCOTP_CRYPTO2"), _T("Crypto Key2"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO3, _T("HW_OCOTP_CRYPTO3"), _T("Crypto Key3"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP0, _T("HW_OCOTP_HWCAP0"), _T("HW Capability0"), OtpRegType_HwCaps);
		
		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ADC_DISABLE (0)"),            0, 1, _T("Disables A/D converter."), valueList);
		reg.AddField(_T("SPDIF_DISABLE (1)"),          1, 1, _T("Disables SPDIF."), valueList);
        reg.AddField(_T("DRI_DISABLE (2)"),            2, 1, _T("Disables Digital Radio Interface."), valueList);
        reg.AddField(_T("I2C_MASTER_DISABLE (3)"),     3, 1, _T("Disables I2C master."), valueList);
        reg.AddField(_T("I2C_SLAVE_DISABLE (4)"),      4, 1, _T("Disables I2C slave."), valueList);
        reg.AddField(_T("TIMER0_DISABLE (5)"),         5, 1, _T("Disables timer 0."), valueList);
        reg.AddField(_T("TIMER1_DISABLE (6)"),         6, 1, _T("Disables timer 1."), valueList);
        reg.AddField(_T("TIMER2_DISABLE (7)"),         7, 1, _T("Disables timer 2."), valueList);
        reg.AddField(_T("TIMER3_DISABLE (8)"),         8, 1, _T("Disables timer 3."), valueList);
        reg.AddField(_T("ROT_DISABLE (9)"),            9, 1, _T("Disables rotary decoder."), valueList); // encoder?
        reg.AddField(_T("PWM0_DISABLE (10)"),         10, 1, _T("Disables Pulse Width Modulator 0."), valueList);
        reg.AddField(_T("PWM1_DISABLE (11)"),         11, 1, _T("Disables Pulse Width Modulator 1."), valueList);
        reg.AddField(_T("PWM2_DISABLE (12)"),         12, 1, _T("Disables Pulse Width Modulator 2."), valueList);
        reg.AddField(_T("PWM3_DISABLE (13)"),         13, 1, _T("Disables Pulse Width Modulator 3."), valueList);
        reg.AddField(_T("PWM4_DISABLE (14)"),         14, 1, _T("Disables Pulse Width Modulator 4."), valueList);
        reg.AddField(_T("LCDIF_DISABLE (15)"),        15, 1, _T("Disables LCD interface."), valueList);
        reg.AddField(_T("LRADC_TEMP1_DISABLE (16)"),  16, 1, _T("Disables LRADC temp1 current source."), valueList);
        reg.AddField(_T("LRADC_TEMP0_DISABLE (17)"),  17, 1, _T("Disables LRADC temp0 current source."), valueList);
        reg.AddField(_T("LRADC_TOUCH_DISABLE (18)"),  18, 1, _T("Disables LRADC touch screen controler."), valueList);
        reg.AddField(_T("LRADC5_DISABLE (19)"),       19, 1, _T("Disables LRADC channel 5 conversions."), valueList);
        reg.AddField(_T("LRADC4_DISABLE (20)"),       20, 1, _T("Disables LRADC channel 4 conversions."), valueList);
        reg.AddField(_T("LRADC3_DISABLE (21)"),       21, 1, _T("Disables LRADC channel 3 conversions."), valueList);
        reg.AddField(_T("LRADC2_DISABLE (22)"),       22, 1, _T("Disables LRADC channel 2 conversions."), valueList);
        reg.AddField(_T("LRADC1_DISABLE (23)"),       23, 1, _T("Disables LRADC channel 1 conversions."), valueList);
        reg.AddField(_T("LRADC0_DISABLE (24)"),       24, 1, _T("Disables LRADC channel 0 conversions."), valueList);
        reg.AddField(_T("Reserved (25)"),             25, 1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("RTC_DISABLE (26)"),          26, 1, _T("Disables the preservation of real time over power downs."), valueList);
        reg.AddField(_T("RTC_ALARM_DISABLE (27)"),    27, 1, _T("Disables the alarm function."), valueList);
        reg.AddField(_T("RTC_WATCHDOG_DISABLE (28)"), 28, 1, _T("Disables the watchdog timer."), valueList);
		
		valueList.clear();
		reg.AddField(_T("Reserved (31:29)"),          29, 3, _T("Reserved - do not blow these bits."), valueList);
        
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP1, _T("HW_OCOTP_HWCAP1"), _T("HW Capability1"), OtpRegType_HwCaps);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("PINCTRL_BANK0_DISABLE (0)"),      0,  1, _T("Disables GPIO BANK0."), valueList);
		reg.AddField(_T("PINCTRL_BANK1_DISABLE (1)"),      1,  1, _T("Disables GPIO BANK1."), valueList);
		reg.AddField(_T("PINCTRL_BANK2_DISABLE (2)"),      2,  1, _T("Disables GPIO BANK2."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:3)"),                3, 13, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SAIF1_DISABLE (16)"),            16,  1, _T("Disables SAIF1."), valueList);
		reg.AddField(_T("SAIF2_DISABLE (17)"),            17,  1, _T("Disables SAIF2."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (20:18)"),              18,  3, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
		reg.AddField(_T("BATTERY_CHARGE_DISABLE (21)"),   21,  1, _T("Disables battery charger."), valueList);

		valueList.clear();
		valueList[0] = _T("00");
		valueList[1] = _T("01");
		valueList[3] = _T("10");
		valueList[4] = _T("11");

		reg.AddField(_T("LOW_PWRCTRL_DISABLE (23:22)"),   22,  2, _T("Used in conjunction with LOWPWR_DISABLE[0] OTP bit to selectively connect two shunts between the battery pin and ground to disable two modes of low power operation (see LOWPWR_DISABLE)."), valueList);
		reg.AddField(_T("LOWPWR_DISABLE (25:24)"),        24,  2, _T("Used to disable three modes of low power operation. Connects shunts between battery pin and ground. ..."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("IR_DISABLE (26)"),               26,  1, _T("Disables infrared port."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - All speeds enabled.");
		valueList[1] = _T("01 - VFIR disabled, SIR to FIR enabled.");
		valueList[3] = _T("10 - SIR to MIR only enabled.");
		valueList[4] = _T("11 - SIR only enabled.");

		reg.AddField(_T("IR_SPEED (28:27)"),              27,  2, _T("Infrared Port Speed Select. 00 - All speeds enabled; 01 - VFIR disabled, SIR to FIR enabled; 10 - SIR to MIR only enabled; 11 - SIR only enabled."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("APPUART_DISABLE (29)"),          29,  1, _T("Disables all application UART capability."), valueList);
		reg.AddField(_T("APPUART_HI_SPEED_DISABLE (30)"), 30,  1, _T("Disables UART baud rates above 115Kb/s."), valueList);

		reg.AddField(_T("DAC_DISABLE (31)"),              31,  1, _T("Disables D/A converter."), valueList);

		_registers.push_back(reg);
	}
    {
	    OtpRegister reg(HW_OCOTP_HWCAP2, _T("HW_OCOTP_HWCAP2"), _T("HW Capability2"), OtpRegType_HwCaps);

		// Field: PACKAGE_TYPE
		valueList.clear();
		valueList[0] = _T("000 - 169 pin BGA");
		valueList[1] = _T("001 - 100 pin BGA");
		valueList[2] = _T("010 - 100 pin TQFP");
		valueList[3] = _T("011 - 128 pin TQFP");
		valueList[4] = _T("100 - Reserved");
		valueList[5] = _T("101 - Reserved");
		valueList[6] = _T("110 - Reserved");
		valueList[7] = _T("111 - Reserved");
		reg.AddField(_T("PACKAGE_TYPE (2:0)"),         0,  3, _T("Package Type: 000 - 169 pin BGA, 001 - 100 pin BGA, 010 - 100 pin TQFP, 011 - 128 pin TQFP, 100 through 111 - reserved."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("Reserved (3)"),               3, 1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("GPMI_DISABLE (4)"),           4, 1, _T("Disables GPMI capability."), valueList);
		reg.AddField(_T("SSP1_DISABLE (5)"),           5, 1, _T("Disables all SSP1 modes."), valueList);
		reg.AddField(_T("SSP1_MEMSTK_DISABLE (6)"),    6, 1, _T("Disables just the memstick capability."), valueList);
		reg.AddField(_T("SSP1_SD_DISABLE (7)"),        7, 1, _T("Disables just the SD and SDIO capabilities."), valueList);
		reg.AddField(_T("SSP2_DISABLE (8)"),           8, 1, _T("Disables all SSP2 modes."), valueList);
		reg.AddField(_T("SSP2_MEMSTK_DISABLE (9)"),    9, 1, _T("Disables just the memstick capability."), valueList);
		reg.AddField(_T("SSP2_SD_DISABLE (10)"),      10, 1, _T("Disables just the SD and SDIO capabilities."), valueList);
		reg.AddField(_T("DCP_CRYPTO_DISABLE (11)"),   11, 1, _T("Disables encryption/decryption/hashing capability within DCP."), valueList);
		reg.AddField(_T("DCP_CSC_DISABLE (12)"),      12, 1, _T("Disables color-space conversion capbility within the DCP."), valueList);
		reg.AddField(_T("JTAG_LOCKOUT (13)"),         13, 1, _T("Disables the JTAG debugger."), valueList);
		reg.AddField(_T("USE_SERIAL_JTAG (14)"),      14, 1, _T("This bit is used by the ROM to copy to the HW_DIGCTL_CTRL_USE_SERIAL_JTAG bit for JTAG boot mode. After it is copied, this bit forces the SJTAG block to switch to the 1-wire serial JTAG mode if it is set. If it is cleared, the alternate 6-wire parallel JTAG backup mode is enabled."), valueList);
		reg.AddField(_T("Reserved (15)"),             15, 1, _T("Reserved - do not blow this bit."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:16)"),          16,16, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP3, _T("HW_OCOTP_HWCAP3"), _T("HW Capability3"), OtpRegType_HwCaps);

		valueList.clear();
		reg.AddField(_T("Reserved (7:0)"),            0, 8, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_STATIC_DISABLE (8)"),     8, 1, _T("Turns off the NOR flash capability."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_DRAM_DISABLE (9)"),       9, 1, _T("Turns off the DDR/SDRAM capability. This does not affect the NOR flash capability."), valueList);
		reg.AddField(_T("USB_NO_OTG (10)"),           10, 1, _T("Disables all OTG functions."), valueList);
		reg.AddField(_T("USB_NO_HOST (11)"),          11, 1, _T("Disables all USB Host capability."), valueList);
		reg.AddField(_T("USB_NO_DEVICE (12)"),        12, 1, _T("Disables all USB device capability."), valueList);
		reg.AddField(_T("USB_NO_HS (13)"),            13, 1, _T("Disables all USB high speed capability."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:14)"),         14, 2, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_LARGE_DRAM_DISABLE (16)"), 16, 1, _T("Disables DDR/SDRAM modules above 2Mbytes. This does not affect the NOR flash capability."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:17)"),         17,15, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP4, _T("HW_OCOTP_HWCAP4"), _T("HW Capability4"), OtpRegType_HwCaps);
		valueList.clear();
		reg.AddField(_T("Reserved (31:0)"),            0, 32, _T("Reserved - do not blow these bits."), valueList);
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP5, _T("HW_OCOTP_HWCAP5"), _T("HW Capability5"), OtpRegType_HwCaps);
		valueList.clear();
		reg.AddField(_T("Reserved (31:0)"),            0, 32, _T("Reserved - do not blow these bits."), valueList);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_SWCAP, _T("HW_OCOTP_SWCAP"), _T("SW Capability"), OtpRegType_SwCaps);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_AACD_D (0)"),          0,  1, _T("Disables the AAC Decoder."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (2:1)"),             1,  2, _T("Reserved - do not blow these bits."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_MP3E_E (3)"),          3,  1, _T("Enables MP3 Encode."), valueList);
		reg.AddField(_T("SW_CAP_MP3PROD_D (4)"),       4,  1, _T("Disables MP3 Pro Decoder."), valueList);
		reg.AddField(_T("SW_CAP_WMAD_D (5)"),          5,  1, _T("Disables WMA Decode."), valueList);
		reg.AddField(_T("SW_CAP_WMAE_D (6)"),          6,  1, _T("Disables WMA Encode."), valueList);
		reg.AddField(_T("SW_CAP_WMA9DRM_D (7)"),       7,  1, _T("Disables WMA9DRM."), valueList);
		reg.AddField(_T("SW_CAP_JANUSDRM_D (8)"),      8,  1, _T("Disables Janus Digital Rights Management."), valueList);
		reg.AddField(_T("SW_CAP_DIVX_E (9)"),          9,  1, _T("Enables DIVX."), valueList);
		reg.AddField(_T("SW_CAP_MPEG4_E (10)"),       10,  1, _T("Enables MPEG4."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:11)"),          11,  5, _T("Reserved - do not blow these bits."), valueList);
		reg.AddField(_T("TRIM_VBG (18:16)"),          16,  3, _T("Trim value for variable band gap current."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("Reserved (19)"),             19,  1, _T("Reserved - do not blow this bit."), valueList);

		valueList.clear();
		reg.AddField(_T("TRIM_HS_USB (23:20)"),       20,  4, _T("Trim value for high speed USB current."), valueList);
		reg.AddField(_T("Reserved (31:24)"),          24,  8, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUSTCAP, _T("HW_OCOTP_CUSTCAP"), _T("Customer Capability"), OtpRegType_Customer, PermissionType_External);
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

#ifdef INTERNAL_BUILD
		reg.AddField(_T("CUST_JTAG_LOCKOUT (0)"),      0, 1, _T("Disables JTAG debugging capability. This bit will be listed as Reserved in the Tspec."), valueList);
#else
		reg.AddField(_T("Reserved (0)"),               0, 1, _T("Reserved - do not blow this bit."), valueList);
#endif
		reg.AddField(_T("RTC_XTAL_32000_PRESENT (1)"), 1, 1, _T("Set to indicate the presence of an optional 32.000KHz off-chip oscillator."), valueList);
        reg.AddField(_T("RTC_XTAL_32768_PRESENT (2)"), 2, 1, _T("Set to indicate the presence of an optional 32.768KHz off-chip oscillator."), valueList);
		
		valueList.clear();
		reg.AddField(_T("Reserved (31:3)"),            3, 29, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_LOCK, _T("HW_OCOTP_LOCK"), _T("LOCK"), OtpRegType_Control, PermissionType_External);
 
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("HW_OCOTP_CUST0 - LOCK_BIT (0)"),                   0, 1, _T("Indicates HW_OCOTP_CUST0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST1 - LOCK_BIT (1)"),                   1, 1, _T("Indicates HW_OCOTP_CUST1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST2 - LOCK_BIT (2)"),                   2, 1, _T("Indicates HW_OCOTP_CUST2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST3 - LOCK_BIT (3)"),                   3, 1, _T("Indicates HW_OCOTP_CUST3 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CRYPTO0-3 - LOCK_BIT (4)"),               4, 1, _T("Indicates HW_OCOTP_CRYPTO0-3 are locked."), valueList);
#ifdef INTERNAL_BUILD
		reg.AddField(_T("HW_OCOTP_HWCAP0-5,HW_OCOTP_SWCAP - LOCK_BIT (8)"), 8, 1, _T("Indicates HW_OCOTP_HWCAP0-5 and HW_OCOTP_SWCAP are locked."), valueList);
#endif
		reg.AddField(_T("HW_OCOTP_CUSTCAP - LOCK_BIT (9)"),                 9, 1, _T("Indicates HW_OCOTP_HWCAP0-5 and HW_OCOTP_SWCAP are locked."), valueList);
#ifdef INTERNAL_BUILD
		reg.AddField(_T("HW_OCOTP_UN0 - LOCK_BIT (16)"),                   16, 1, _T("Indicates HW_OCOTP_UN0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_UN1 - LOCK_BIT (17)"),                   17, 1, _T("Indicates HW_OCOTP_UN1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_UN2 - LOCK_BIT (18)"),                   18, 1, _T("Indicates HW_OCOTP_UN2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_OPS0-3 - LOCK_BIT (19)"),                19, 1, _T("Indicates HW_OCOTP_OPS0-3 are locked."), valueList);
#endif
		reg.AddField(_T("HW_OCOTP_ROM0 - LOCK_BIT (24)"),                  24, 1, _T("Indicates HW_OCOTP_ROM0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM1 - LOCK_BIT (25)"),                  25, 1, _T("Indicates HW_OCOTP_ROM1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM2 - LOCK_BIT (26)"),                  26, 1, _T("Indicates HW_OCOTP_ROM2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM3 - LOCK_BIT (27)"),                  27, 1, _T("Indicates HW_OCOTP_ROM3 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM4 - LOCK_BIT (28)"),                  28, 1, _T("Indicates HW_OCOTP_ROM4 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM5 - LOCK_BIT (29)"),                  29, 1, _T("Indicates HW_OCOTP_ROM5 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM6 - LOCK_BIT (30)"),                  30, 1, _T("Indicates HW_OCOTP_ROM6 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM7 - LOCK_BIT (31)"),                  31, 1, _T("Indicates HW_OCOTP_ROM7 is locked."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS0, _T("HW_OCOTP_OPS0"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS1, _T("HW_OCOTP_OPS1"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS2, _T("HW_OCOTP_OPS2"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS3, _T("HW_OCOTP_OPS3"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN0, _T("HW_OCOTP_UN0"), _T("Unassigned0"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN1, _T("HW_OCOTP_UN1"), _T("Unassigned1"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN2, _T("HW_OCOTP_UN2"), _T("Unassigned2"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM0, _T("HW_OCOTP_ROM0"), _T("ROM0"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		// Field: Reserved
		reg.AddField(_T("Reserved (0)"),                    0,  1, _T("Reserved - do not blow this bit."), valueList);

		// Field: USE_ALT_DEBUG_UART_PINS
		reg.AddField(_T("USE_ALT_DEBUG_UART_PINS (1)"),     1,  1, _T("Use alternate ROTARYA/B bedug UART RX/TX pins."), valueList);

		// Field: DISABLE_RECOVERY_MODE
#ifdef INTERNAL_BUILD
        reg.AddField(_T("DISABLE_RECOVERY_MODE (2)"),       2,  1, _T("Disables PSWITCH recovery mode via USB. This bit will be listed as Reserved in the Tspec., as this function will only be made available to customers who request it."), valueList);
#else
		reg.AddField(_T("Reserved (2)"),                    2,  1, _T("Reserved - do not blow this bit."), valueList);
#endif
		// Field: ENABLE_UNENCRYPTED_BOOT
		reg.AddField(_T("SD_MBR_BOOT (3)"),                 3,  1, _T("Enables master boot record (MBR) boot mode for SD boot."), valueList);

		// Field: ENABLE_UNENCRYPTED_BOOT
		reg.AddField(_T("ENABLE_UNENCRYPTED_BOOT (4)"),     4,  1, _T("Enables unencrypted boot modes."), valueList);

		// Field: ENABLE_USB_BOOT_SERIAL_NUM
		reg.AddField(_T("ENABLE_USB_BOOT_SERIAL_NUM (5)"),  5,  1, _T("Enables USB boot serial number."), valueList);

		// Field: DISABLE_SPI_NOR_FAST_READ
		reg.AddField(_T("DISABLE_SPI_NOR_FAST_READ (6)"),   6,  1, _T("Disables SPI NOR fast reads which are used by default."), valueList);

		// Field: Reserved
		reg.AddField(_T("Reserved (7)"),                    7,  1, _T("Reserved - do not blow this bit."), valueList);

		// Field: SSP_SCK_INDEX
		valueList.clear();
		reg.AddField(_T("SSP_SCK_INDEX (11:8)"),            8,  4, _T("Index to the SSP clock speed."), valueList);

		// Field: SD_BUS_WIDTH
		valueList.clear();
		valueList[0] = _T("00 - 4-bit");
		valueList[1] = _T("01 - 1-bit");
		valueList[2] = _T("10 - 8-bit");
		valueList[3] = _T("11 - Reserved");
		reg.AddField(_T("SD_BUS_WIDTH (13:12)"),           12,  2, _T("SD card bus width: 00 - 4-bit, 01 - 1-bit, 10 - 8-bit, 11 - Reserved."), valueList);

		// Field: SD_POWER_UP_DELAY
		valueList.clear();
		int i; CStdString str;
		valueList[0] = _T("0x00 - 20ms");
		for ( i=1; i<64; ++i )
		{
			str.Format(_T("0x%02X - %dms"), i, i*10);
			valueList[i] = str;
		}
		reg.AddField(_T("SD_POWER_UP_DELAY (19:14)"),      14,  6, _T("SD card power up delay required after enabling GPIO power gate: 000000 - 0ms, 000001 - 10ms, 00002 - 20ms, ... 111111 - 630ms."), valueList);

		// Field: SD_POWER_GATE_GPIO
		valueList.clear();
		valueList[0] = _T("00 - PWM3");
		valueList[1] = _T("01 - PWM4");
		valueList[2] = _T("10 - ROTARYA");
		valueList[3] = _T("11 - NO_GATE");
		reg.AddField(_T("SD_POWER_GATE_GPIO (21:20)"),     20,  2, _T("SD card power gate GPIO pin select: 00 - PWM3, 01 - PWM4, 10 - ROTARYA, 11 - NO_GATE."), valueList);

		// Field: USE_PARALLEL_JTAG
		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
		reg.AddField(_T("USE_PARALLEL_JTAG (22)"),         22, 1, _T("During JTAG boot mode, the ROM reads this bit, then inverts it, and writes the value to the HW_DIGCTL_USE_SERIAL_JTAG bit. If this bit is one, indicating parallel JATAG mode is selected, a zero is written to the DIGCTL_USE_SERIAL_JTAG bit which places the device into 6-wire JTAG mode. If this bit is zero, a one is written causing the SJTAG block to switch to the 1-wire serial JTAG mode."), valueList);

		// Field: ENABLE_PJTAG_12MA_DRIVE
		reg.AddField(_T("ENABLE_PJTAG_12MA_DRIVE (23)"),   23,  1, _T("Blow to force the 6-wire PJTAG pins to drive 12mA. Default is 4mA. Note that SJTAG is fixed at 8mA. Blowing this bit causes the ROM to program all six parallel JTAG pins to drive 12mA via the pin control registers."), valueList);
		
		// Field: BOOT_MODE
		valueList.clear();
		reg.AddField(_T("BOOT_MODE (31:24)"),              24,  8, _T("Encoded boot mode."), valueList);
		
		// Register: HW_OCOTP_ROM0
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM1, _T("HW_OCOTP_ROM1"), _T("ROM1"), OtpRegType_Rom, PermissionType_External);
        
		valueList.clear();

		reg.AddField(_T("NUMBER_OF_NANDS (2:0)"),       0,  3, _T("Encoded value indicates number of external NAND devices (0 to7). Zero indicates ROM will probe for the number of NAND devices connected to the system."), valueList);
		reg.AddField(_T("Reserved (7:3)"),              3,  5, _T("Reserved - do not blow these bits."), valueList);
		reg.AddField(_T("BOOT_SEARCH_COUNT (11:8)"),    8,  4, _T("Number of 64 page blocks that should be read by the boot loader."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("USE_ALTERNATE_CE (12)"),          12,  1, _T("Directs the boot loader to use the alternate chip enables."), valueList);
		reg.AddField(_T("SD_INIT_SEQ_1_DISABLE (13)"),     13,  1, _T("Disables the first initialization sequence for SD."), valueList);
        reg.AddField(_T("SD_CMD0_DISABLE (14)"),           14,  1, _T("Cmd0 (reset cmd) is called by default to reset the SD card during startup. Blow this bit to not reset hte card during SD boot."), valueList);
        reg.AddField(_T("SD_INIT_SEQ_2_ENSABLE (15)"),     15,  1, _T("Enables the second initialization sequence for SD boot."), valueList);
        reg.AddField(_T("SD_INCREASE_INIT_SEQ_TIME (16)"), 16,  1, _T("Increases the SD card initialization sequence time from 1ms (default) to 4ms."), valueList);
        reg.AddField(_T("SSP1_EXT_PULLUP (17)"),           17,  1, _T("Indicates external pull-ups implemented for SSP1."), valueList);
        reg.AddField(_T("SSP2_EXT_PULLUP (18)"),           18,  1, _T("Indicates external pull-ups implemented for SSP2."), valueList);
		reg.AddField(_T("Reserved (19)"),                  19,  1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("ENABLE_NAND0_CE_RDY_PULLUP (20)"),20,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE0 and GPMI_RDY0."), valueList);
        reg.AddField(_T("ENABLE_NAND1_CE_RDY_PULLUP (21)"),21,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE1 and GPMI_RDY1."), valueList);
        reg.AddField(_T("ENABLE_NAND2_CE_RDY_PULLUP (22)"),22,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE2 and GPMI_RDY2."), valueList);
        reg.AddField(_T("ENABLE_NAND3_CE_RDY_PULLUP (23)"),23,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE3 and GPMI_RDY3."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:24)"),               24,  8, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM2, _T("HW_OCOTP_ROM2"), _T("ROM2"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

		reg.AddField(_T("USB_PID (15:0)"),              0, 16, _T("USB Product ID."), valueList);
		reg.AddField(_T("USB_VID (31:16)"),            16, 16, _T("USB Vendor ID."), valueList);
        
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM3, _T("HW_OCOTP_ROM3"), _T("ROM3"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

#ifdef INTERNAL_BUILD
		reg.AddField(_T("OSC_TRIM (9:0)"),              0, 10, _T("Oscillator trim value for off-chip Bluetooth device(Elwood). This bit-field should be described instead as SDK reserved bits in the customer spec."), valueList);
#else
		reg.AddField(_T("Reserved (9:0)"),              0, 10, _T("Reserved - do not blow these bits."), valueList);
#endif
		reg.AddField(_T("Reserved (31:10)"),           10, 22, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM4, _T("HW_OCOTP_ROM4"), _T("ROM4"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

		reg.AddField(_T("Reserved (31:0)"),                0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM5, _T("HW_OCOTP_ROM5"), _T("ROM5"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

        reg.AddField(_T("Reserved (31:0)"),                0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM6, _T("HW_OCOTP_ROM6"), _T("ROM6"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
#ifdef INTERNAL_BUILD
		reg.AddField(_T("DISABLE_TEST_MODES (0)"),      0,  1, _T("Disables manufacturing test modes. This bit will be listed as Reserved in the Data Sheet."), valueList);
#else
		reg.AddField(_T("Reserved (0)"),                0,  1, _T("Reserved - do not blow this bit."), valueList);
#endif
		valueList.clear();
		reg.AddField(_T("Reserved (31:1)"),             1, 31, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM7, _T("HW_OCOTP_ROM7"), _T("ROM7"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ENABLE_PIN_BOOT_CHECK (0)"),   0,  1, _T("Enables the boot loader to first test the LCD_RS pin to determine if pin boot mode is enabled. If this bit is blown, and LCD_RS is pulled high, then boot mode is determined by the state of LCD_D[6:0] pins. If this bit is not blown, skip testing the LCD_RS pin and go directly to determining boot mode by reading the state of LCD_D[6:0]."), valueList);
		reg.AddField(_T("Reserved (1)"),                1,  1, _T("Reserved - do not blow this bit."), valueList);
        reg.AddField(_T("ENABLE_ARM_ICACHE (2)"),       2,  1, _T("Enables the ARM 926 ICache during boot."), valueList);
        reg.AddField(_T("I2C_USE_400KHZ (3)"),          3,  1, _T("Forces the I2C to be programmed by the boot loader to run at 400KHz. 100KHz is the default."), valueList);

		valueList.clear();
#ifdef INTERNAL_BUILD
		reg.AddField(_T("OCRAM_SS_TRIM (7:4)"),         4,  4, _T("On-chip RAM sense-amp speed trim value."), valueList);
#else
		reg.AddField(_T("Reserved (7:4)"),              4,  4, _T("Reserved - do not blow these bits."), valueList);
#endif
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
        reg.AddField(_T("ENABLE_SSP_12MA_DRIVE (8)"),   8,  1, _T("Forces the SSP pins to drive 12mA, default is 4mA."), valueList);
		reg.AddField(_T("RESET_USB_PHY_AT_STARTUP (9)"),9,  1, _T("Resets the USBPHY at startup."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:10)"),           10, 22, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_VERSION, _T("HW_OCOTP_VERSION"), _T("OTP Controller Version"), OtpRegType_Control, PermissionType_External);
        _registers.push_back(reg);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpRegs3780 IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StOtpRegs3780::StOtpRegs3780()
{
    std::map<uint32_t,CStdString> valueList;

/*	{
		OtpRegister reg(HW_OCOTP_CTRL, _T("HW_OCOTP_CTRL"), _T("OTP Controller Control"), OtpRegType_Control);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_DATA, _T("HW_OCOTP_DATA"), _T("OTP Controller Write Data"), OtpRegType_Control);
        _registers.push_back(reg);
    }
*/  {
		OtpRegister reg(HW_OCOTP_CUST0, _T("HW_OCOTP_CUST0"), _T("Customer0"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST1, _T("HW_OCOTP_CUST1"), _T("Customer1"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST2, _T("HW_OCOTP_CUST2"), _T("Customer2"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST3, _T("HW_OCOTP_CUST3"), _T("Customer3"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
        OtpRegister reg(HW_OCOTP_CRYPTO0, _T("HW_OCOTP_CRYPTO0"), _T("Crypto Key0"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

       _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO1, _T("HW_OCOTP_CRYPTO1"), _T("Crypto Key1"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO2, _T("HW_OCOTP_CRYPTO2"), _T("Crypto Key2"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO3, _T("HW_OCOTP_CRYPTO3"), _T("Crypto Key3"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP0, _T("HW_OCOTP_HWCAP0"), _T("HW Capability0"), OtpRegType_HwCaps);
		
		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ADC_DISABLE (0)"),            0, 1, _T("Disables A/D converter."), valueList);
		reg.AddField(_T("SPDIF_DISABLE (1)"),          1, 1, _T("Disables SPDIF."), valueList);
        reg.AddField(_T("DRI_DISABLE (2)"),            2, 1, _T("Disables Digital Radio Interface."), valueList);
        reg.AddField(_T("I2C_MASTER_DISABLE (3)"),     3, 1, _T("Disables I2C master."), valueList);
        reg.AddField(_T("I2C_SLAVE_DISABLE (4)"),      4, 1, _T("Disables I2C slave."), valueList);
        reg.AddField(_T("TIMER0_DISABLE (5)"),         5, 1, _T("Disables timer 0."), valueList);
        reg.AddField(_T("TIMER1_DISABLE (6)"),         6, 1, _T("Disables timer 1."), valueList);
        reg.AddField(_T("TIMER2_DISABLE (7)"),         7, 1, _T("Disables timer 2."), valueList);
        reg.AddField(_T("TIMER3_DISABLE (8)"),         8, 1, _T("Disables timer 3."), valueList);
        reg.AddField(_T("ROT_DISABLE (9)"),            9, 1, _T("Disables rotary decoder."), valueList); // encoder?
        reg.AddField(_T("PWM0_DISABLE (10)"),         10, 1, _T("Disables Pulse Width Modulator 0."), valueList);
        reg.AddField(_T("PWM1_DISABLE (11)"),         11, 1, _T("Disables Pulse Width Modulator 1."), valueList);
        reg.AddField(_T("PWM2_DISABLE (12)"),         12, 1, _T("Disables Pulse Width Modulator 2."), valueList);
        reg.AddField(_T("PWM3_DISABLE (13)"),         13, 1, _T("Disables Pulse Width Modulator 3."), valueList);
        reg.AddField(_T("PWM4_DISABLE (14)"),         14, 1, _T("Disables Pulse Width Modulator 4."), valueList);
        reg.AddField(_T("LCDIF_DISABLE (15)"),        15, 1, _T("Disables LCD interface."), valueList);
        reg.AddField(_T("LRADC_TEMP1_DISABLE (16)"),  16, 1, _T("Disables LRADC temp1 current source."), valueList);
        reg.AddField(_T("LRADC_TEMP0_DISABLE (17)"),  17, 1, _T("Disables LRADC temp0 current source."), valueList);
        reg.AddField(_T("LRADC_TOUCH_DISABLE (18)"),  18, 1, _T("Disables LRADC touch screen controler."), valueList);
        reg.AddField(_T("LRADC5_DISABLE (19)"),       19, 1, _T("Disables LRADC channel 5 conversions."), valueList);
        reg.AddField(_T("LRADC4_DISABLE (20)"),       20, 1, _T("Disables LRADC channel 4 conversions."), valueList);
        reg.AddField(_T("LRADC3_DISABLE (21)"),       21, 1, _T("Disables LRADC channel 3 conversions."), valueList);
        reg.AddField(_T("LRADC2_DISABLE (22)"),       22, 1, _T("Disables LRADC channel 2 conversions."), valueList);
        reg.AddField(_T("LRADC1_DISABLE (23)"),       23, 1, _T("Disables LRADC channel 1 conversions."), valueList);
        reg.AddField(_T("LRADC0_DISABLE (24)"),       24, 1, _T("Disables LRADC channel 0 conversions."), valueList);
        reg.AddField(_T("Reserved (25)"),             25, 1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("RTC_DISABLE (26)"),          26, 1, _T("Disables the preservation of real time over power downs."), valueList);
        reg.AddField(_T("RTC_ALARM_DISABLE (27)"),    27, 1, _T("Disables the alarm function."), valueList);
        reg.AddField(_T("RTC_WATCHDOG_DISABLE (28)"), 28, 1, _T("Disables the watchdog timer."), valueList);
		
		valueList.clear();
		reg.AddField(_T("Reserved (31:29)"),          29, 3, _T("Reserved - do not blow these bits."), valueList);
        
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP1, _T("HW_OCOTP_HWCAP1"), _T("HW Capability1"), OtpRegType_HwCaps);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("PINCTRL_BANK0_DISABLE (0)"),      0,  1, _T("Disables GPIO BANK0."), valueList);
		reg.AddField(_T("PINCTRL_BANK1_DISABLE (1)"),      1,  1, _T("Disables GPIO BANK1."), valueList);
		reg.AddField(_T("PINCTRL_BANK2_DISABLE (2)"),      2,  1, _T("Disables GPIO BANK2."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:3)"),                3, 13, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SAIF1_DISABLE (16)"),            16,  1, _T("Disables SAIF1."), valueList);
		reg.AddField(_T("SAIF2_DISABLE (17)"),            17,  1, _T("Disables SAIF2."), valueList);
		reg.AddField(_T("MAGIC8_DISABLE (18)"),           18,  1, _T("Disables Magic 8 module."), valueList);
		reg.AddField(_T("APPUART2_DISABLE (19)"),         19,  1, _T("Disables all application UART capability."), valueList);
		reg.AddField(_T("APPUART2_HI_SPEED_DISABLE (20)"),20,  1, _T("Disables application UART baud rates above 115Kb/s."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (25:21)"),              21,  5, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("IR_DISABLE (26)"),               26,  1, _T("Disables infrared port."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - All speeds enabled.");
		valueList[1] = _T("01 - VFIR disabled, SIR to FIR enabled.");
		valueList[3] = _T("10 - SIR to MIR only enabled.");
		valueList[4] = _T("11 - SIR only enabled.");

		reg.AddField(_T("IR_SPEED (28:27)"),              27,  2, _T("Infrared Port Speed Select. 00 - All speeds enabled; 01 - VFIR disabled, SIR to FIR enabled; 10 - SIR to MIR only enabled; 11 - SIR only enabled."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("APPUART1_DISABLE (29)"),          29,  1, _T("Disables all application UART capability."), valueList);
		reg.AddField(_T("APPUART1_HI_SPEED_DISABLE (30)"), 30,  1, _T("Disables UART baud rates above 115Kb/s."), valueList);
		reg.AddField(_T("DAC_DISABLE (31)"),              31,  1, _T("Disables D/A converter."), valueList);

		_registers.push_back(reg);
	}
    {
	    OtpRegister reg(HW_OCOTP_HWCAP2, _T("HW_OCOTP_HWCAP2"), _T("HW Capability2"), OtpRegType_HwCaps);

		// Field: PACKAGE_TYPE
		valueList.clear();
		valueList[0] = _T("000 - 169 pin BGA");
		valueList[1] = _T("001 - 100 pin BGA");
		valueList[2] = _T("010 - 100 pin TQFP");
		valueList[3] = _T("011 - 128 pin TQFP");
		valueList[4] = _T("100 - Reserved");
		valueList[5] = _T("101 - Reserved");
		valueList[6] = _T("110 - Reserved");
		valueList[7] = _T("111 - Reserved");
		reg.AddField(_T("PACKAGE_TYPE (2:0)"),         0,  3, _T("Package Type: 000 - 169 pin BGA, 001 - 100 pin BGA, 010 - 100 pin TQFP, 011 - 128 pin TQFP, 100 through 111 - reserved."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("Reserved (3)"),               3, 1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("GPMI_DISABLE (4)"),           4, 1, _T("Disables GPMI capability."), valueList);
		reg.AddField(_T("SSP1_DISABLE (5)"),           5, 1, _T("Disables all SSP1 modes."), valueList);
		reg.AddField(_T("SSP1_MEMSTK_DISABLE (6)"),    6, 1, _T("Disables just the memstick capability."), valueList);
		reg.AddField(_T("SSP1_SD_DISABLE (7)"),        7, 1, _T("Disables just the SD and SDIO capabilities."), valueList);
		reg.AddField(_T("SSP2_DISABLE (8)"),           8, 1, _T("Disables all SSP2 modes."), valueList);
		reg.AddField(_T("SSP2_MEMSTK_DISABLE (9)"),    9, 1, _T("Disables just the memstick capability."), valueList);
		reg.AddField(_T("SSP2_SD_DISABLE (10)"),      10, 1, _T("Disables just the SD and SDIO capabilities."), valueList);
		reg.AddField(_T("DCP_CRYPTO_DISABLE (11)"),   11, 1, _T("Disables encryption/decryption/hashing capability within DCP."), valueList);
		reg.AddField(_T("DCP_CSC_DISABLE (12)"),      12, 1, _T("Disables color-space conversion capbility within the DCP."), valueList);
		reg.AddField(_T("JTAG_LOCKOUT (13)"),         13, 1, _T("Disables the JTAG debugger."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:14)"),          14,18, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP3, _T("HW_OCOTP_HWCAP3"), _T("HW Capability3"), OtpRegType_HwCaps);

		valueList.clear();
		reg.AddField(_T("Reserved (8:0)"),            0, 9, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_DRAM_DISABLE (9)"),       9, 1, _T("Turns off the DDR/SDRAM capability. This does not affect the NOR flash capability."), valueList);
		reg.AddField(_T("USB_NO_OTG (10)"),           10, 1, _T("Disables all OTG functions."), valueList);
		reg.AddField(_T("USB_NO_HOST (11)"),          11, 1, _T("Disables all USB Host capability."), valueList);
		reg.AddField(_T("USB_NO_DEVICE (12)"),        12, 1, _T("Disables all USB device capability."), valueList);
		reg.AddField(_T("USB_NO_HS (13)"),            13, 1, _T("Disables all USB high speed capability."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - No clock limit.");
		valueList[1] = _T("01 - CPU clock limited to less than or equal to 411.43 MHz.");
		valueList[3] = _T("10 - CPU clock limited to less than or equal to 360 MHz.");
		valueList[4] = _T("11 - CPU clock limited to less than or equal to 320 MHz.");

		reg.AddField(_T("CPU_CLK_LIMIT (15:14)"),    14, 2, _T("CPU Clock Limit Select: 00 - No clock limit, 01 - CPU limit <= 411.43 MHz, 10 - CPU limit <= 360 MHz, 11 - CPU limit <= 320 MHz."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_LARGE_DRAM_DISABLE (16)"), 16, 1, _T("Disables DDR/SDRAM modules above 2Mbytes. This does not affect the NOR flash capability."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:17)"),         17,15, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP4, _T("HW_OCOTP_HWCAP4"), _T("HW Capability4"), OtpRegType_HwCaps);

		valueList.clear();
		reg.AddField(_T("Reserved (11:0)"),            0, 12, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("PXP_DISABLE (12)"),          12,  1, _T("Disables the 2D graphics PiXel Pipeline unit."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (14:13)"),          13,  2, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("TVENC_COMPOSITE_DISABLE (15)"),   15, 1, _T("Disables composite output from the TVout encoder."), valueList);
		reg.AddField(_T("TVENC_MACROVISION_DISABLE (16)"), 16, 1, _T("Disables Macrovision output signaling from the TVout encoder."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:17)"),          17, 15, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP5, _T("HW_OCOTP_HWCAP5"), _T("HW Capability5"), OtpRegType_HwCaps);
		valueList.clear();
		reg.AddField(_T("Reserved (31:0)"),            0, 32, _T("Reserved - do not blow these bits."), valueList);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_SWCAP, _T("HW_OCOTP_SWCAP"), _T("SW Capability"), OtpRegType_SwCaps);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_AACD_D (0)"),          0,  1, _T("Disables the AAC Decoder."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (2:1)"),             1,  2, _T("Reserved - do not blow these bits."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_MP3E_E (3)"),          3,  1, _T("Enables MP3 Encode."), valueList);
		reg.AddField(_T("SW_CAP_MP3PROD_D (4)"),       4,  1, _T("Disables MP3 Pro Decoder."), valueList);
		reg.AddField(_T("SW_CAP_WMAD_D (5)"),          5,  1, _T("Disables WMA Decode."), valueList);
		reg.AddField(_T("SW_CAP_WMAE_D (6)"),          6,  1, _T("Disables WMA Encode."), valueList);
		reg.AddField(_T("SW_CAP_WMA9DRM_D (7)"),       7,  1, _T("Disables WMA9DRM."), valueList);
		reg.AddField(_T("SW_CAP_JANUSDRM_D (8)"),      8,  1, _T("Disables Janus Digital Rights Management."), valueList);
		reg.AddField(_T("SW_CAP_DIVX_E (9)"),          9,  1, _T("Enables DIVX."), valueList);
		reg.AddField(_T("SW_CAP_MPEG4_E (10)"),       10,  1, _T("Enables MPEG4."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:11)"),          11,  5, _T("Reserved - do not blow these bits."), valueList);
		reg.AddField(_T("TRIM_VBG (18:16)"),          16,  3, _T("Trim value for variable band gap current."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("Reserved (19)"),             19,  1, _T("Reserved - do not blow this bit."), valueList);

		valueList.clear();
		reg.AddField(_T("TRIM_HS_USB (23:20)"),       20,  4, _T("Trim value for high speed USB current."), valueList);
		reg.AddField(_T("Reserved (31:24)"),          24,  8, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUSTCAP, _T("HW_OCOTP_CUSTCAP"), _T("Customer Capability"), OtpRegType_Customer, PermissionType_External);
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

#ifdef INTERNAL_BUILD
		reg.AddField(_T("CUST_JTAG_LOCKOUT (0)"),      0, 1, _T("Disables JTAG debugging capability. This bit will be listed as Reserved in the Tspec."), valueList);
#else
		reg.AddField(_T("Reserved (0)"),               0, 1, _T("Reserved - do not blow this bit."), valueList);
#endif
		reg.AddField(_T("RTC_XTAL_32000_PRESENT (1)"), 1, 1, _T("Set to indicate the presence of an optional 32.000KHz off-chip oscillator."), valueList);
        reg.AddField(_T("RTC_XTAL_32768_PRESENT (2)"), 2, 1, _T("Set to indicate the presence of an optional 32.768KHz off-chip oscillator."), valueList);
		reg.AddField(_T("USE_PARALLEL_JTAG (3)"),      3, 1, _T("During JTAG boot mode, the ROM reads this bit, then inverts it, and writes the value to the HW_DIGCTL_CTRL_USE_SERIAL_JTAG bit. If this bit is one, indicating parallel JTAG mode is selected, a zero is written to the DIGCTL USE_SERIAL_JTAG bit which places the device into 6-wire JTAG mode, and if this bit is zero, a one instead is written causing the SJTAG block to switch to the 1-wire serial JTAG mode."), valueList);
        reg.AddField(_T("ENABLE_SJTAG_12MA_DRIVE (4)"),4, 1, _T("Blow to force the 1-wire DEBUG (serial JTAG) pin to drive 12mA, the default is 8mA (see ENABLE_PJTAG_12MA_DRIVE in the ROM0 register for 6-wire parallel JTAG). This is a hardware override which will cause the drive select bits in the PINCTRL block to reset to the 12mA drive settings rather than the normal default of 8mA. The user is still free to reprogram these bits to other drive levels."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:5)"),            5, 27, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_LOCK, _T("HW_OCOTP_LOCK"), _T("LOCK"), OtpRegType_Control, PermissionType_External);
 
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("HW_OCOTP_CUST0 - LOCK_BIT (0)"),                   0, 1, _T("Indicates HW_OCOTP_CUST0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST1 - LOCK_BIT (1)"),                   1, 1, _T("Indicates HW_OCOTP_CUST1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST2 - LOCK_BIT (2)"),                   2, 1, _T("Indicates HW_OCOTP_CUST2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST3 - LOCK_BIT (3)"),                   3, 1, _T("Indicates HW_OCOTP_CUST3 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CRYPTO0-3 - LOCK_BIT (4)"),               4, 1, _T("Indicates HW_OCOTP_CRYPTO0-3 are locked."), valueList);
#ifdef INTERNAL_BUILD
		reg.AddField(_T("HW_OCOTP_HWCAP0-5,HW_OCOTP_SWCAP - LOCK_BIT (8)"), 8, 1, _T("Indicates HW_OCOTP_HWCAP0-5 and HW_OCOTP_SWCAP are locked."), valueList);
#endif
		reg.AddField(_T("HW_OCOTP_CUSTCAP - LOCK_BIT (9)"),                 9, 1, _T("Indicates HW_OCOTP_HWCAP0-5 and HW_OCOTP_SWCAP are locked."), valueList);
#ifdef INTERNAL_BUILD
		reg.AddField(_T("HW_OCOTP_UN0 - LOCK_BIT (16)"),                   16, 1, _T("Indicates HW_OCOTP_UN0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_UN1 - LOCK_BIT (17)"),                   17, 1, _T("Indicates HW_OCOTP_UN1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_UN2 - LOCK_BIT (18)"),                   18, 1, _T("Indicates HW_OCOTP_UN2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_OPS0-3 - LOCK_BIT (19)"),                19, 1, _T("Indicates HW_OCOTP_OPS0-3 are locked."), valueList);
#endif
		reg.AddField(_T("HW_OCOTP_ROM0 - LOCK_BIT (24)"),                  24, 1, _T("Indicates HW_OCOTP_ROM0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM1 - LOCK_BIT (25)"),                  25, 1, _T("Indicates HW_OCOTP_ROM1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM2 - LOCK_BIT (26)"),                  26, 1, _T("Indicates HW_OCOTP_ROM2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM3 - LOCK_BIT (27)"),                  27, 1, _T("Indicates HW_OCOTP_ROM3 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM4 - LOCK_BIT (28)"),                  28, 1, _T("Indicates HW_OCOTP_ROM4 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM5 - LOCK_BIT (29)"),                  29, 1, _T("Indicates HW_OCOTP_ROM5 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM6 - LOCK_BIT (30)"),                  30, 1, _T("Indicates HW_OCOTP_ROM6 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM7 - LOCK_BIT (31)"),                  31, 1, _T("Indicates HW_OCOTP_ROM7 is locked."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS0, _T("HW_OCOTP_OPS0"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS1, _T("HW_OCOTP_OPS1"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS2, _T("HW_OCOTP_OPS2"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS3, _T("HW_OCOTP_OPS3"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN0, _T("HW_OCOTP_UN0"), _T("Unassigned0"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN1, _T("HW_OCOTP_UN1"), _T("Unassigned1"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN2, _T("HW_OCOTP_UN2"), _T("Unassigned2"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM0, _T("HW_OCOTP_ROM0"), _T("ROM0"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		// Field: Reserved
		reg.AddField(_T("Reserved (0)"),                    0,  1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("Reserved (1)"),                    1,  1, _T("Reserved - do not blow this bit.(was USE_ALT_DEBUG_UART_PINS on 37xx)"), valueList);

		// Field: DISABLE_RECOVERY_MODE
#ifdef INTERNAL_BUILD
        reg.AddField(_T("DISABLE_RECOVERY_MODE (2)"),       2,  1, _T("Disables PSWITCH recovery mode via USB. This bit will be listed as Reserved in the Tspec., as this function will only be made available to customers who request it."), valueList);
#else
		reg.AddField(_T("Reserved (2)"),                    2,  1, _T("Reserved - do not blow this bit."), valueList);
#endif
		// Field: ENABLE_UNENCRYPTED_BOOT
		reg.AddField(_T("SD_MBR_BOOT (3)"),                 3,  1, _T("Enables master boot record (MBR) boot mode for SD boot."), valueList);

		// Field: ENABLE_UNENCRYPTED_BOOT
		reg.AddField(_T("ENABLE_UNENCRYPTED_BOOT (4)"),     4,  1, _T("Enables unencrypted boot modes."), valueList);

		// Field: ENABLE_USB_BOOT_SERIAL_NUM
		reg.AddField(_T("ENABLE_USB_BOOT_SERIAL_NUM (5)"),  5,  1, _T("Enables USB boot serial number."), valueList);

		// Field: DISABLE_SPI_NOR_FAST_READ
		reg.AddField(_T("DISABLE_SPI_NOR_FAST_READ (6)"),   6,  1, _T("Disables SPI NOR fast reads which are used by default."), valueList);

		// Field: Reserved
		reg.AddField(_T("Reserved (7)"),                    7,  1, _T("Reserved - do not blow this bit."), valueList);

		// Field: SSP_SCK_INDEX
		valueList.clear();
		reg.AddField(_T("SSP_SCK_INDEX (11:8)"),            8,  4, _T("Index to the SSP clock speed."), valueList);

		// Field: SD_BUS_WIDTH
		valueList.clear();
		valueList[0] = _T("00 - 4-bit");
		valueList[1] = _T("01 - 1-bit");
		valueList[2] = _T("10 - 8-bit");
		valueList[3] = _T("11 - Reserved");
		reg.AddField(_T("SD_BUS_WIDTH (13:12)"),           12,  2, _T("SD card bus width: 00 - 4-bit, 01 - 1-bit, 10 - 8-bit, 11 - Reserved."), valueList);

		// Field: SD_POWER_UP_DELAY
		valueList.clear();
		int i; CStdString str;
		valueList[0] = _T("0x00 - 20ms");
		for ( i=1; i<64; ++i )
		{
			str.Format(_T("0x%02X - %dms"), i, i*10);
			valueList[i] = str;
		}
		reg.AddField(_T("SD_POWER_UP_DELAY (19:14)"),      14,  6, _T("SD card power up delay required after enabling GPIO power gate: 000000 - 0ms, 000001 - 10ms, 00002 - 20ms, ... 111111 - 630ms."), valueList);

		// Field: SD_POWER_GATE_GPIO
		valueList.clear();
		valueList[0] = _T("00 - PWM0");
		valueList[1] = _T("01 - LCD_DOTCLK");
		valueList[2] = _T("10 - PWM3");
		valueList[3] = _T("11 - NO_GATE");
		reg.AddField(_T("SD_POWER_GATE_GPIO (21:20)"),     20,  2, _T("SD card power gate GPIO pin select: 00 - PWM0, 01 - LCD_DOTCLK, 10 - PWM3, 11 - NO_GATE."), valueList);

		// Field: USE_PARALLEL_JTAG
		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
		reg.AddField(_T("USE_PARALLEL_JTAG (22)"),         22, 1, _T("During JTAG boot mode, the ROM reads this bit, then inverts it, and writes the value to the HW_DIGCTL_USE_SERIAL_JTAG bit. If this bit is one, indicating parallel JATAG mode is selected, a zero is written to the DIGCTL_USE_SERIAL_JTAG bit which places the device into 6-wire JTAG mode. If this bit is zero, a one is written causing the SJTAG block to switch to the 1-wire serial JTAG mode."), valueList);

		// Field: ENABLE_PJTAG_12MA_DRIVE
		reg.AddField(_T("ENABLE_PJTAG_12MA_DRIVE (23)"),   23,  1, _T("Blow to force the 6-wire PJTAG pins to drive 12mA. Default is 4mA. Note that SJTAG is fixed at 8mA. Blowing this bit causes the ROM to program all six parallel JTAG pins to drive 12mA via the pin control registers."), valueList);
		
		// Field: BOOT_MODE
		valueList.clear();
		reg.AddField(_T("BOOT_MODE (31:24)"),              24,  8, _T("Encoded boot mode."), valueList);
		
		// Register: HW_OCOTP_ROM0
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM1, _T("HW_OCOTP_ROM1"), _T("ROM1"), OtpRegType_Rom, PermissionType_External);
        
		valueList.clear();

		reg.AddField(_T("NUMBER_OF_NANDS (2:0)"),       0,  3, _T("Encoded value indicates number of external NAND devices (0 to7). Zero indicates ROM will probe for the number of NAND devices connected to the system."), valueList);
		reg.AddField(_T("Reserved (7:3)"),              3,  5, _T("Reserved - do not blow these bits."), valueList);
		reg.AddField(_T("BOOT_SEARCH_COUNT (11:8)"),    8,  4, _T("Number of 64 page blocks that should be read by the boot loader."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("USE_ALT_SSP1_DATA4-7 (12)"),      12,  1, _T("This bit is blown to enable alternate pin use for SSP1 data lines 4-7."), valueList);
		reg.AddField(_T("SD_INIT_SEQ_1_DISABLE (13)"),     13,  1, _T("Disables the first initialization sequence for SD."), valueList);
        reg.AddField(_T("SD_CMD0_DISABLE (14)"),           14,  1, _T("Cmd0 (reset cmd) is called by default to reset the SD card during startup. Blow this bit to not reset hte card during SD boot."), valueList);
        reg.AddField(_T("SD_INIT_SEQ_2_ENSABLE (15)"),     15,  1, _T("Enables the second initialization sequence for SD boot."), valueList);
        reg.AddField(_T("SD_INCREASE_INIT_SEQ_TIME (16)"), 16,  1, _T("Increases the SD card initialization sequence time from 1ms (default) to 4ms."), valueList);
        reg.AddField(_T("SSP1_EXT_PULLUP (17)"),           17,  1, _T("Indicates external pull-ups implemented for SSP1."), valueList);
        reg.AddField(_T("SSP2_EXT_PULLUP (18)"),           18,  1, _T("Indicates external pull-ups implemented for SSP2."), valueList);
		reg.AddField(_T("UNTOUCH_INTERNAL_SSP_PULLUP (19)"),19,  1, _T("If this bit is blown then internal pull-ups for SSP are neither enabled nor disabled. This bit is used only if external pull-ups are implemented and ROM1:18 and/or ROM1:17 are blown."), valueList);
		reg.AddField(_T("ENABLE_NAND0_CE_RDY_PULLUP (20)"),20,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE0 and GPMI_RDY0."), valueList);
        reg.AddField(_T("ENABLE_NAND1_CE_RDY_PULLUP (21)"),21,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE1 and GPMI_RDY1."), valueList);
        reg.AddField(_T("ENABLE_NAND2_CE_RDY_PULLUP (22)"),22,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE2 and GPMI_RDY2."), valueList);
        reg.AddField(_T("ENABLE_NAND3_CE_RDY_PULLUP (23)"),23,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE3 and GPMI_RDY3."), valueList);
		reg.AddField(_T("USE_ALT_GPMI_CE2 (24)"),          24,  1, _T("If the bit is blown then ROM NAND driver will enable alternate pins for GPMI_CE2."), valueList);
		reg.AddField(_T("USE_ALT_GPMI_RDY2 (25)"),         25,  1, _T("If the bit is blown then ROM NAND driver will enable alternate pins for GPMI_RDY2."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - GPMI_D15");
		valueList[1] = _T("01 - LCD_RESET");
		valueList[2] = _T("10 - SSP_DETECT");
		valueList[3] = _T("11 - ROTARYB");
		reg.AddField(_T("USE_ALT_GPMI_CE3 (27:26)"),       26,  2, _T("These bits are used by ROM NAND driver to enable one of 4 alternate pins for GPMI_CE3. 00–GPMI_D15, 01-LCD_RESET, 10-SSP_DETECT and 11-ROTARYB."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - GPMI_RDY3");
		valueList[1] = _T("01 - PWM2");
		valueList[2] = _T("10 - LCD_DOTCK");
		valueList[3] = _T("11 - Reserved");
		reg.AddField(_T("USE_ALT_GPMI_RDY3 (29:28)"),       28,  2, _T("These bits are used by ROM NAND driver to enable one of 4 alternate pins for GPMI_CE3. 00–GPMI_D15, 01-LCD_RESET, 10-SSP_DETECT and 11-ROTARYB."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("DISABLE_SECONDARY_BOOT (30)"),     30,  1, _T("If the bit is blown then ROM NAND driver will not use the secondary boot image."), valueList);
		reg.AddField(_T("Reserved (31)"),                   31,  1, _T("Reserved - do not blow this bit."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM2, _T("HW_OCOTP_ROM2"), _T("ROM2"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

		reg.AddField(_T("USB_PID (15:0)"),              0, 16, _T("USB Product ID."), valueList);
		reg.AddField(_T("USB_VID (31:16)"),            16, 16, _T("USB Vendor ID."), valueList);
        
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM3, _T("HW_OCOTP_ROM3"), _T("ROM3"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

#ifdef INTERNAL_BUILD
		reg.AddField(_T("OSC_TRIM (9:0)"),              0, 10, _T("Oscillator trim value for off-chip Bluetooth device(Elwood). This bit-field should be described instead as SDK reserved bits in the customer spec."), valueList);
#else
		reg.AddField(_T("Reserved (9:0)"),              0, 10, _T("Reserved - do not blow these bits."), valueList);
#endif
		reg.AddField(_T("Reserved (31:10)"),           10, 22, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM4, _T("HW_OCOTP_ROM4"), _T("ROM4"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

		reg.AddField(_T("Reserved (31:0)"),                0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM5, _T("HW_OCOTP_ROM5"), _T("ROM5"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

        reg.AddField(_T("Reserved (31:0)"),                0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM6, _T("HW_OCOTP_ROM6"), _T("ROM6"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
#ifdef INTERNAL_BUILD
		reg.AddField(_T("DISABLE_TEST_MODES (0)"),      0,  1, _T("Disables manufacturing test modes. This bit will be listed as Reserved in the Data Sheet."), valueList);
#else
		reg.AddField(_T("Reserved (0)"),                0,  1, _T("Reserved - do not blow this bit."), valueList);
#endif
		valueList.clear();
		reg.AddField(_T("Reserved (31:1)"),             1, 31, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM7, _T("HW_OCOTP_ROM7"), _T("ROM7"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ENABLE_PIN_BOOT_CHECK (0)"),   0,  1, _T("Enables the boot loader to first test the LCD_RS pin to determine if pin boot mode is enabled. If this bit is blown, and LCD_RS is pulled high, then boot mode is determined by the state of LCD_D[6:0] pins. If this bit is not blown, skip testing the LCD_RS pin and go directly to determining boot mode by reading the state of LCD_D[6:0]."), valueList);
		reg.AddField(_T("Reserved (1)"),                1,  1, _T("Reserved - do not blow this bit."), valueList);
        reg.AddField(_T("ENABLE_ARM_ICACHE (2)"),       2,  1, _T("Enables the ARM 926 ICache during boot."), valueList);
        reg.AddField(_T("I2C_USE_400KHZ (3)"),          3,  1, _T("Forces the I2C to be programmed by the boot loader to run at 400KHz. 100KHz is the default."), valueList);

		valueList.clear();
#ifdef INTERNAL_BUILD
		reg.AddField(_T("OCRAM_SS_TRIM (7:4)"),         4,  4, _T("On-chip RAM sense-amp speed trim value."), valueList);
#else
		reg.AddField(_T("Reserved (7:4)"),              4,  4, _T("Reserved - do not blow these bits."), valueList);
#endif
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
        reg.AddField(_T("ENABLE_SSP_12MA_DRIVE (8)"),   8,  1, _T("Forces the SSP pins to drive 12mA, default is 4mA."), valueList);

		reg.AddField(_T("Reserved (9)"),                9,  1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("USB_HID_SAFE_DATAOUT (10)"),  10,  1, _T("Forces USB report size to 64 bytes."), valueList);
		reg.AddField(_T("Reserved (11)"),              11,  1, _T("Reserved - do not blow this bit."), valueList);

		valueList.clear();

#ifdef INTERNAL_BUILD
		reg.AddField(_T("RECOVERY_BOOT_MODE (19:12)"), 12,  8, _T("If USB is not desirable for recovery boot mode then these bits can be blown to have the ROM boot from a non-USB device in recovery mode. This bit will be listed as Reserved in the Data Sheet, as this function will only be made available to customers who request it."), valueList);
#else
		reg.AddField(_T("Reserved (19:12)"),           12,  8, _T("Reserved - do not blow these bits."), valueList);
#endif
		reg.AddField(_T("Reserved (31:20)"),           20, 12, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_VERSION, _T("HW_OCOTP_VERSION"), _T("OTP Controller Version"), OtpRegType_Control, PermissionType_External);
        _registers.push_back(reg);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StOtpRegsMX23 IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StOtpRegsMX23::StOtpRegsMX23()
{
    std::map<uint32_t,CStdString> valueList;

/*	{
		OtpRegister reg(HW_OCOTP_CTRL, _T("HW_OCOTP_CTRL"), _T("OTP Controller Control"), OtpRegType_Control);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_DATA, _T("HW_OCOTP_DATA"), _T("OTP Controller Write Data"), OtpRegType_Control);
        _registers.push_back(reg);
    }
*/  {
		OtpRegister reg(HW_OCOTP_CUST0, _T("HW_OCOTP_CUST0"), _T("Customer0"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST1, _T("HW_OCOTP_CUST1"), _T("Customer1"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST2, _T("HW_OCOTP_CUST2"), _T("Customer2"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUST3, _T("HW_OCOTP_CUST3"), _T("Customer3"), OtpRegType_Customer, PermissionType_External);

		valueList.clear();
		reg.AddField(_T("CUSTOMER (31:0)"), 0, 32, _T("As defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
        OtpRegister reg(HW_OCOTP_CRYPTO0, _T("HW_OCOTP_CRYPTO0"), _T("Crypto Key0"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

       _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO1, _T("HW_OCOTP_CRYPTO1"), _T("Crypto Key1"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO2, _T("HW_OCOTP_CRYPTO2"), _T("Crypto Key2"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CRYPTO3, _T("HW_OCOTP_CRYPTO3"), _T("Crypto Key3"), OtpRegType_Crypto, PermissionType_External);
 
		valueList.clear();
		reg.AddField(_T("CRYPTO KEY (31:0)"), 0, 32, _T("Crypto Key defined by customers."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP0, _T("HW_OCOTP_HWCAP0"), _T("HW Capability0"), OtpRegType_HwCaps);
		
		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ADC_DISABLE (0)"),            0, 1, _T("Disables A/D converter."), valueList);
		reg.AddField(_T("SPDIF_DISABLE (1)"),          1, 1, _T("Disables SPDIF."), valueList);
        reg.AddField(_T("DRI_DISABLE (2)"),            2, 1, _T("Disables Digital Radio Interface."), valueList);
        reg.AddField(_T("I2C_MASTER_DISABLE (3)"),     3, 1, _T("Disables I2C master."), valueList);
        reg.AddField(_T("I2C_SLAVE_DISABLE (4)"),      4, 1, _T("Disables I2C slave."), valueList);
        reg.AddField(_T("TIMER0_DISABLE (5)"),         5, 1, _T("Disables timer 0."), valueList);
        reg.AddField(_T("TIMER1_DISABLE (6)"),         6, 1, _T("Disables timer 1."), valueList);
        reg.AddField(_T("TIMER2_DISABLE (7)"),         7, 1, _T("Disables timer 2."), valueList);
        reg.AddField(_T("TIMER3_DISABLE (8)"),         8, 1, _T("Disables timer 3."), valueList);
        reg.AddField(_T("ROT_DISABLE (9)"),            9, 1, _T("Disables rotary decoder."), valueList); // encoder?
        reg.AddField(_T("PWM0_DISABLE (10)"),         10, 1, _T("Disables Pulse Width Modulator 0."), valueList);
        reg.AddField(_T("PWM1_DISABLE (11)"),         11, 1, _T("Disables Pulse Width Modulator 1."), valueList);
        reg.AddField(_T("PWM2_DISABLE (12)"),         12, 1, _T("Disables Pulse Width Modulator 2."), valueList);
        reg.AddField(_T("PWM3_DISABLE (13)"),         13, 1, _T("Disables Pulse Width Modulator 3."), valueList);
        reg.AddField(_T("PWM4_DISABLE (14)"),         14, 1, _T("Disables Pulse Width Modulator 4."), valueList);
        reg.AddField(_T("LCDIF_DISABLE (15)"),        15, 1, _T("Disables LCD interface."), valueList);
        reg.AddField(_T("LRADC_TEMP1_DISABLE (16)"),  16, 1, _T("Disables LRADC temp1 current source."), valueList);
        reg.AddField(_T("LRADC_TEMP0_DISABLE (17)"),  17, 1, _T("Disables LRADC temp0 current source."), valueList);
        reg.AddField(_T("LRADC_TOUCH_DISABLE (18)"),  18, 1, _T("Disables LRADC touch screen controler."), valueList);
        reg.AddField(_T("LRADC5_DISABLE (19)"),       19, 1, _T("Disables LRADC channel 5 conversions."), valueList);
        reg.AddField(_T("LRADC4_DISABLE (20)"),       20, 1, _T("Disables LRADC channel 4 conversions."), valueList);
        reg.AddField(_T("LRADC3_DISABLE (21)"),       21, 1, _T("Disables LRADC channel 3 conversions."), valueList);
        reg.AddField(_T("LRADC2_DISABLE (22)"),       22, 1, _T("Disables LRADC channel 2 conversions."), valueList);
        reg.AddField(_T("LRADC1_DISABLE (23)"),       23, 1, _T("Disables LRADC channel 1 conversions."), valueList);
        reg.AddField(_T("LRADC0_DISABLE (24)"),       24, 1, _T("Disables LRADC channel 0 conversions."), valueList);
        reg.AddField(_T("Reserved (25)"),             25, 1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("RTC_DISABLE (26)"),          26, 1, _T("Disables the preservation of real time over power downs."), valueList);
        reg.AddField(_T("RTC_ALARM_DISABLE (27)"),    27, 1, _T("Disables the alarm function."), valueList);
        reg.AddField(_T("RTC_WATCHDOG_DISABLE (28)"), 28, 1, _T("Disables the watchdog timer."), valueList);
		
		valueList.clear();
		reg.AddField(_T("Reserved (31:29)"),          29, 3, _T("Reserved - do not blow these bits."), valueList);
        
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP1, _T("HW_OCOTP_HWCAP1"), _T("HW Capability1"), OtpRegType_HwCaps);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("PINCTRL_BANK0_DISABLE (0)"),      0,  1, _T("Disables GPIO BANK0."), valueList);
		reg.AddField(_T("PINCTRL_BANK1_DISABLE (1)"),      1,  1, _T("Disables GPIO BANK1."), valueList);
		reg.AddField(_T("PINCTRL_BANK2_DISABLE (2)"),      2,  1, _T("Disables GPIO BANK2."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:3)"),                3, 13, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SAIF1_DISABLE (16)"),            16,  1, _T("Disables SAIF1."), valueList);
		reg.AddField(_T("SAIF2_DISABLE (17)"),            17,  1, _T("Disables SAIF2."), valueList);
		reg.AddField(_T("MAGIC8_DISABLE (18)"),           18,  1, _T("Disables Magic 8 module."), valueList);
		reg.AddField(_T("APPUART2_DISABLE (19)"),         19,  1, _T("Disables all application UART capability."), valueList);
		reg.AddField(_T("APPUART2_HI_SPEED_DISABLE (20)"),20,  1, _T("Disables application UART baud rates above 115Kb/s."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (25:21)"),              21,  5, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("IR_DISABLE (26)"),               26,  1, _T("Disables infrared port."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - All speeds enabled.");
		valueList[1] = _T("01 - VFIR disabled, SIR to FIR enabled.");
		valueList[3] = _T("10 - SIR to MIR only enabled.");
		valueList[4] = _T("11 - SIR only enabled.");

		reg.AddField(_T("IR_SPEED (28:27)"),              27,  2, _T("Infrared Port Speed Select. 00 - All speeds enabled; 01 - VFIR disabled, SIR to FIR enabled; 10 - SIR to MIR only enabled; 11 - SIR only enabled."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("APPUART1_DISABLE (29)"),          29,  1, _T("Disables all application UART capability."), valueList);
		reg.AddField(_T("APPUART1_HI_SPEED_DISABLE (30)"), 30,  1, _T("Disables UART baud rates above 115Kb/s."), valueList);
		reg.AddField(_T("DAC_DISABLE (31)"),              31,  1, _T("Disables D/A converter."), valueList);

		_registers.push_back(reg);
	}
    {
	    OtpRegister reg(HW_OCOTP_HWCAP2, _T("HW_OCOTP_HWCAP2"), _T("HW Capability2"), OtpRegType_HwCaps);

		// Field: PACKAGE_TYPE
		valueList.clear();
		valueList[0] = _T("000 - 169 pin BGA");
		valueList[1] = _T("001 - 100 pin BGA");
		valueList[2] = _T("010 - 100 pin TQFP");
		valueList[3] = _T("011 - 128 pin TQFP");
		valueList[4] = _T("100 - Reserved");
		valueList[5] = _T("101 - Reserved");
		valueList[6] = _T("110 - Reserved");
		valueList[7] = _T("111 - Reserved");
		reg.AddField(_T("PACKAGE_TYPE (2:0)"),         0,  3, _T("Package Type: 000 - 169 pin BGA, 001 - 100 pin BGA, 010 - 100 pin TQFP, 011 - 128 pin TQFP, 100 through 111 - reserved."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("Reserved (3)"),               3, 1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("GPMI_DISABLE (4)"),           4, 1, _T("Disables GPMI capability."), valueList);
		reg.AddField(_T("SSP1_DISABLE (5)"),           5, 1, _T("Disables all SSP1 modes."), valueList);
		reg.AddField(_T("SSP1_MEMSTK_DISABLE (6)"),    6, 1, _T("Disables just the memstick capability."), valueList);
		reg.AddField(_T("SSP1_SD_DISABLE (7)"),        7, 1, _T("Disables just the SD and SDIO capabilities."), valueList);
		reg.AddField(_T("SSP2_DISABLE (8)"),           8, 1, _T("Disables all SSP2 modes."), valueList);
		reg.AddField(_T("SSP2_MEMSTK_DISABLE (9)"),    9, 1, _T("Disables just the memstick capability."), valueList);
		reg.AddField(_T("SSP2_SD_DISABLE (10)"),      10, 1, _T("Disables just the SD and SDIO capabilities."), valueList);
		reg.AddField(_T("DCP_CRYPTO_DISABLE (11)"),   11, 1, _T("Disables encryption/decryption/hashing capability within DCP."), valueList);
		reg.AddField(_T("DCP_CSC_DISABLE (12)"),      12, 1, _T("Disables color-space conversion capbility within the DCP."), valueList);
		reg.AddField(_T("JTAG_LOCKOUT (13)"),         13, 1, _T("Disables the JTAG debugger."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:14)"),          14,18, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP3, _T("HW_OCOTP_HWCAP3"), _T("HW Capability3"), OtpRegType_HwCaps);

		valueList.clear();
		reg.AddField(_T("Reserved (8:0)"),            0, 9, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_DRAM_DISABLE (9)"),       9, 1, _T("Turns off the DDR/SDRAM capability. This does not affect the NOR flash capability."), valueList);
		reg.AddField(_T("USB_NO_OTG (10)"),           10, 1, _T("Disables all OTG functions."), valueList);
		reg.AddField(_T("USB_NO_HOST (11)"),          11, 1, _T("Disables all USB Host capability."), valueList);
		reg.AddField(_T("USB_NO_DEVICE (12)"),        12, 1, _T("Disables all USB device capability."), valueList);
		reg.AddField(_T("USB_NO_HS (13)"),            13, 1, _T("Disables all USB high speed capability."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - No clock limit.");
		valueList[1] = _T("01 - CPU clock limited to less than or equal to 411.43 MHz.");
		valueList[3] = _T("10 - CPU clock limited to less than or equal to 360 MHz.");
		valueList[4] = _T("11 - CPU clock limited to less than or equal to 320 MHz.");

		reg.AddField(_T("CPU_CLK_LIMIT (15:14)"),    14, 2, _T("CPU Clock Limit Select: 00 - No clock limit, 01 - CPU limit <= 411.43 MHz, 10 - CPU limit <= 360 MHz, 11 - CPU limit <= 320 MHz."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("EMI_LARGE_DRAM_DISABLE (16)"), 16, 1, _T("Disables DDR/SDRAM modules above 2Mbytes. This does not affect the NOR flash capability."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:17)"),         17,15, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP4, _T("HW_OCOTP_HWCAP4"), _T("HW Capability4"), OtpRegType_HwCaps);

		valueList.clear();
		reg.AddField(_T("Reserved (11:0)"),            0, 12, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("PXP_DISABLE (12)"),          12,  1, _T("Disables the 2D graphics PiXel Pipeline unit."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (14:13)"),          13,  2, _T("Reserved - do not blow these bits."), valueList);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("TVENC_COMPOSITE_DISABLE (15)"),   15, 1, _T("Disables composite output from the TVout encoder."), valueList);
		reg.AddField(_T("TVENC_MACROVISION_DISABLE (16)"), 16, 1, _T("Disables Macrovision output signaling from the TVout encoder."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (31:17)"),          17, 15, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_HWCAP5, _T("HW_OCOTP_HWCAP5"), _T("HW Capability5"), OtpRegType_HwCaps);
		valueList.clear();
		reg.AddField(_T("Reserved (31:0)"),            0, 32, _T("Reserved - do not blow these bits."), valueList);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_SWCAP, _T("HW_OCOTP_SWCAP"), _T("SW Capability"), OtpRegType_SwCaps);

		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_AACD_D (0)"),          0,  1, _T("Disables the AAC Decoder."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (2:1)"),             1,  2, _T("Reserved - do not blow these bits."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_MP3E_E (3)"),          3,  1, _T("Enables MP3 Encode."), valueList);
		reg.AddField(_T("SW_CAP_MP3PROD_D (4)"),       4,  1, _T("Disables MP3 Pro Decoder."), valueList);
		reg.AddField(_T("SW_CAP_WMAD_D (5)"),          5,  1, _T("Disables WMA Decode."), valueList);
		reg.AddField(_T("SW_CAP_WMAE_D (6)"),          6,  1, _T("Disables WMA Encode."), valueList);
		reg.AddField(_T("SW_CAP_WMA9DRM_D (7)"),       7,  1, _T("Disables WMA9DRM."), valueList);
		reg.AddField(_T("SW_CAP_JANUSDRM_D (8)"),      8,  1, _T("Disables Janus Digital Rights Management."), valueList);
		reg.AddField(_T("SW_CAP_DIVX_E (9)"),          9,  1, _T("Enables DIVX."), valueList);
		reg.AddField(_T("SW_CAP_MPEG4_E (10)"),       10,  1, _T("Enables MPEG4."), valueList);
		reg.AddField(_T("SW_CAP_RMVB_E (11)"),        11,  1, _T("Enables RMVB."), valueList);
		reg.AddField(_T("SW_CAP_SMV_D (12)"),         12,  1, _T("Disables SMV."), valueList);
		reg.AddField(_T("SW_CAP_IND_TEMP (13)"),      13,  1, _T("Indicates industrial temperature device."), valueList);

		valueList.clear();
		reg.AddField(_T("Reserved (15:14)"),          14,  2, _T("Reserved - do not blow these bits."), valueList);
		reg.AddField(_T("TRIM_VBG (18:16)"),          16,  3, _T("Trim value for variable band gap current."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("SW_CAP_IMX23 (19)"),         19,  1, _T("Indicates i.MX23 device."), valueList);

		valueList.clear();
		reg.AddField(_T("TRIM_HS_USB (23:20)"),       20,  4, _T("Trim value for high speed USB current."), valueList);
		reg.AddField(_T("Reserved (31:24)"),          24,  8, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_CUSTCAP, _T("HW_OCOTP_CUSTCAP"), _T("Customer Capability"), OtpRegType_Customer, PermissionType_External);
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

#ifdef INTERNAL_BUILD
		reg.AddField(_T("CUST_JTAG_LOCKOUT (0)"),      0, 1, _T("Disables JTAG debugging capability. This bit will be listed as Reserved in the Tspec."), valueList);
#else
		reg.AddField(_T("Reserved (0)"),               0, 1, _T("Reserved - do not blow this bit."), valueList);
#endif
		reg.AddField(_T("RTC_XTAL_32000_PRESENT (1)"), 1, 1, _T("Set to indicate the presence of an optional 32.000KHz off-chip oscillator."), valueList);
        reg.AddField(_T("RTC_XTAL_32768_PRESENT (2)"), 2, 1, _T("Set to indicate the presence of an optional 32.768KHz off-chip oscillator."), valueList);
		reg.AddField(_T("USE_PARALLEL_JTAG (3)"),      3, 1, _T("During JTAG boot mode, the ROM reads this bit, then inverts it, and writes the value to the HW_DIGCTL_CTRL_USE_SERIAL_JTAG bit. If this bit is one, indicating parallel JTAG mode is selected, a zero is written to the DIGCTL USE_SERIAL_JTAG bit which places the device into 6-wire JTAG mode, and if this bit is zero, a one instead is written causing the SJTAG block to switch to the 1-wire serial JTAG mode."), valueList);
        reg.AddField(_T("ENABLE_SJTAG_12MA_DRIVE (4)"),4, 1, _T("Blow to force the 1-wire DEBUG (serial JTAG) pin to drive 12mA, the default is 8mA (see ENABLE_PJTAG_12MA_DRIVE in the ROM0 register for 6-wire parallel JTAG). This is a hardware override which will cause the drive select bits in the PINCTRL block to reset to the 12mA drive settings rather than the normal default of 8mA. The user is still free to reprogram these bits to other drive levels."), valueList);

		valueList.clear();
#ifdef INTERNAL_BUILD
		reg.AddField(_T("Reserved (27:5)"),            5, 23, _T("Reserved - do not blow these bits."), valueList);

		valueList[0] = _T("Development Board");
		valueList[1] = _T("EVK");
		valueList[2] = _T("Reserved");
		valueList[3] = _T("Reserved");
		
		reg.AddField(_T("INTERNAL_BOARD_ID (29:28)"), 28, 2, _T("Freescale Internal Board ID. 00 - Development Board, 01 - EVK, 10 - Reserved, 11 - Reserved. These bits will be listed as Reserved in the Data Sheet."), valueList);
		valueList.clear();
#else
		reg.AddField(_T("Reserved (29:5)"),            5, 25, _T("Reserved - do not blow these bits."), valueList);
#endif
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("CUST_DISABLE_JANUSDRM10 (30)"), 30, 1, _T("Disables WMA Janus DRM10."), valueList);
		reg.AddField(_T("CUST_DISABLE_WMADRM9 (31)"),    31, 1, _T("Disables WMA DRM9."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_LOCK, _T("HW_OCOTP_LOCK"), _T("LOCK"), OtpRegType_Control, PermissionType_External);
 
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("HW_OCOTP_CUST0 - LOCK_BIT (0)"),                   0, 1, _T("Indicates HW_OCOTP_CUST0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST1 - LOCK_BIT (1)"),                   1, 1, _T("Indicates HW_OCOTP_CUST1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST2 - LOCK_BIT (2)"),                   2, 1, _T("Indicates HW_OCOTP_CUST2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CUST3 - LOCK_BIT (3)"),                   3, 1, _T("Indicates HW_OCOTP_CUST3 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_CRYPTO0-3 - LOCK_BIT (4)"),               4, 1, _T("Indicates HW_OCOTP_CRYPTO0-3 are locked."), valueList);
#ifdef INTERNAL_BUILD
		reg.AddField(_T("HW_OCOTP_HWCAP0-5,HW_OCOTP_SWCAP - LOCK_BIT (8)"), 8, 1, _T("Indicates HW_OCOTP_HWCAP0-5 and HW_OCOTP_SWCAP are locked."), valueList);
#endif
		reg.AddField(_T("HW_OCOTP_CUSTCAP - LOCK_BIT (9)"),                 9, 1, _T("Indicates HW_OCOTP_HWCAP0-5 and HW_OCOTP_SWCAP are locked."), valueList);
#ifdef INTERNAL_BUILD
		reg.AddField(_T("HW_OCOTP_UN0 - LOCK_BIT (16)"),                   16, 1, _T("Indicates HW_OCOTP_UN0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_UN1 - LOCK_BIT (17)"),                   17, 1, _T("Indicates HW_OCOTP_UN1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_UN2 - LOCK_BIT (18)"),                   18, 1, _T("Indicates HW_OCOTP_UN2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_OPS0-3 - LOCK_BIT (19)"),                19, 1, _T("Indicates HW_OCOTP_OPS0-3 are locked."), valueList);
#endif
		reg.AddField(_T("HW_OCOTP_ROM0 - LOCK_BIT (24)"),                  24, 1, _T("Indicates HW_OCOTP_ROM0 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM1 - LOCK_BIT (25)"),                  25, 1, _T("Indicates HW_OCOTP_ROM1 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM2 - LOCK_BIT (26)"),                  26, 1, _T("Indicates HW_OCOTP_ROM2 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM3 - LOCK_BIT (27)"),                  27, 1, _T("Indicates HW_OCOTP_ROM3 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM4 - LOCK_BIT (28)"),                  28, 1, _T("Indicates HW_OCOTP_ROM4 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM5 - LOCK_BIT (29)"),                  29, 1, _T("Indicates HW_OCOTP_ROM5 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM6 - LOCK_BIT (30)"),                  30, 1, _T("Indicates HW_OCOTP_ROM6 is locked."), valueList);
		reg.AddField(_T("HW_OCOTP_ROM7 - LOCK_BIT (31)"),                  31, 1, _T("Indicates HW_OCOTP_ROM7 is locked."), valueList);

        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS0, _T("HW_OCOTP_OPS0"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS1, _T("HW_OCOTP_OPS1"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS2, _T("HW_OCOTP_OPS2"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_OPS3, _T("HW_OCOTP_OPS3"), _T("SigmaTel Operations0"), OtpRegType_SgtlOperations);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN0, _T("HW_OCOTP_UN0"), _T("Unassigned0"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN1, _T("HW_OCOTP_UN1"), _T("Unassigned1"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_UN2, _T("HW_OCOTP_UN2"), _T("Unassigned2"), OtpRegType_Unassigned);
        _registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM0, _T("HW_OCOTP_ROM0"), _T("ROM0"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		// Field: Reserved
		reg.AddField(_T("Reserved (0)"),                    0,  1, _T("Reserved - do not blow this bit."), valueList);
		reg.AddField(_T("Reserved (1)"),                    1,  1, _T("Reserved - do not blow this bit.(was USE_ALT_DEBUG_UART_PINS on 37xx)"), valueList);

		// Field: DISABLE_RECOVERY_MODE
#ifdef INTERNAL_BUILD
        reg.AddField(_T("DISABLE_RECOVERY_MODE (2)"),       2,  1, _T("Disables PSWITCH recovery mode via USB. This bit will be listed as Reserved in the Tspec., as this function will only be made available to customers who request it."), valueList);
#else
		reg.AddField(_T("Reserved (2)"),                    2,  1, _T("Reserved - do not blow this bit."), valueList);
#endif
		// Field: ENABLE_UNENCRYPTED_BOOT
		reg.AddField(_T("SD_MBR_BOOT (3)"),                 3,  1, _T("Enables master boot record (MBR) boot mode for SD boot."), valueList);

		// Field: ENABLE_UNENCRYPTED_BOOT
		reg.AddField(_T("ENABLE_UNENCRYPTED_BOOT (4)"),     4,  1, _T("Enables unencrypted boot modes."), valueList);

		// Field: ENABLE_USB_BOOT_SERIAL_NUM
		reg.AddField(_T("ENABLE_USB_BOOT_SERIAL_NUM (5)"),  5,  1, _T("Enables USB boot serial number."), valueList);

		// Field: DISABLE_SPI_NOR_FAST_READ
		reg.AddField(_T("DISABLE_SPI_NOR_FAST_READ (6)"),   6,  1, _T("Disables SPI NOR fast reads which are used by default."), valueList);

		// Field: Reserved
		reg.AddField(_T("Reserved (7)"),                    7,  1, _T("Reserved - do not blow this bit."), valueList);

		// Field: SSP_SCK_INDEX
		valueList.clear();
		reg.AddField(_T("SSP_SCK_INDEX (11:8)"),            8,  4, _T("Index to the SSP clock speed."), valueList);

		// Field: SD_BUS_WIDTH
		valueList.clear();
		valueList[0] = _T("00 - 4-bit");
		valueList[1] = _T("01 - 1-bit");
		valueList[2] = _T("10 - 8-bit");
		valueList[3] = _T("11 - Reserved");
		reg.AddField(_T("SD_BUS_WIDTH (13:12)"),           12,  2, _T("SD card bus width: 00 - 4-bit, 01 - 1-bit, 10 - 8-bit, 11 - Reserved."), valueList);

		// Field: SD_POWER_UP_DELAY
		valueList.clear();
		int i; CStdString str;
		valueList[0] = _T("0x00 - 20ms");
		for ( i=1; i<64; ++i )
		{
			str.Format(_T("0x%02X - %dms"), i, i*10);
			valueList[i] = str;
		}
		reg.AddField(_T("SD_POWER_UP_DELAY (19:14)"),      14,  6, _T("SD card power up delay required after enabling GPIO power gate: 000000 - 0ms, 000001 - 10ms, 00002 - 20ms, ... 111111 - 630ms."), valueList);

		// Field: SD_POWER_GATE_GPIO
		valueList.clear();
		valueList[0] = _T("00 - PWM0");
		valueList[1] = _T("01 - LCD_DOTCLK");
		valueList[2] = _T("10 - PWM3");
		valueList[3] = _T("11 - NO_GATE");
		reg.AddField(_T("SD_POWER_GATE_GPIO (21:20)"),     20,  2, _T("SD card power gate GPIO pin select: 00 - PWM0, 01 - LCD_DOTCLK, 10 - PWM3, 11 - NO_GATE."), valueList);

		// Field: USE_PARALLEL_JTAG
		valueList.clear();
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
		reg.AddField(_T("USE_PARALLEL_JTAG (22)"),         22, 1, _T("During JTAG boot mode, the ROM reads this bit, then inverts it, and writes the value to the HW_DIGCTL_USE_SERIAL_JTAG bit. If this bit is one, indicating parallel JATAG mode is selected, a zero is written to the DIGCTL_USE_SERIAL_JTAG bit which places the device into 6-wire JTAG mode. If this bit is zero, a one is written causing the SJTAG block to switch to the 1-wire serial JTAG mode."), valueList);

		// Field: ENABLE_PJTAG_12MA_DRIVE
		reg.AddField(_T("ENABLE_PJTAG_12MA_DRIVE (23)"),   23,  1, _T("Blow to force the 6-wire PJTAG pins to drive 12mA. Default is 4mA. Note that SJTAG is fixed at 8mA. Blowing this bit causes the ROM to program all six parallel JTAG pins to drive 12mA via the pin control registers."), valueList);
		
		// Field: BOOT_MODE
		valueList.clear();
		reg.AddField(_T("BOOT_MODE (31:24)"),              24,  8, _T("Encoded boot mode."), valueList);
		
		// Register: HW_OCOTP_ROM0
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM1, _T("HW_OCOTP_ROM1"), _T("ROM1"), OtpRegType_Rom, PermissionType_External);
        
		valueList.clear();

		reg.AddField(_T("NUMBER_OF_NANDS (2:0)"),       0,  3, _T("Encoded value indicates number of external NAND devices (0 to7). Zero indicates ROM will probe for the number of NAND devices connected to the system."), valueList);
		reg.AddField(_T("Reserved (7:3)"),              3,  5, _T("Reserved - do not blow these bits."), valueList);
		reg.AddField(_T("BOOT_SEARCH_COUNT (11:8)"),    8,  4, _T("Number of 64 page blocks that should be read by the boot loader."), valueList);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("USE_ALT_SSP1_DATA4-7 (12)"),      12,  1, _T("This bit is blown to enable alternate pin use for SSP1 data lines 4-7."), valueList);
		reg.AddField(_T("SD_INIT_SEQ_1_DISABLE (13)"),     13,  1, _T("Disables the first initialization sequence for SD."), valueList);
        reg.AddField(_T("SD_CMD0_DISABLE (14)"),           14,  1, _T("Cmd0 (reset cmd) is called by default to reset the SD card during startup. Blow this bit to not reset hte card during SD boot."), valueList);
        reg.AddField(_T("SD_INIT_SEQ_2_ENSABLE (15)"),     15,  1, _T("Enables the second initialization sequence for SD boot."), valueList);
        reg.AddField(_T("SD_INCREASE_INIT_SEQ_TIME (16)"), 16,  1, _T("Increases the SD card initialization sequence time from 1ms (default) to 4ms."), valueList);
        reg.AddField(_T("SSP1_EXT_PULLUP (17)"),           17,  1, _T("Indicates external pull-ups implemented for SSP1."), valueList);
        reg.AddField(_T("SSP2_EXT_PULLUP (18)"),           18,  1, _T("Indicates external pull-ups implemented for SSP2."), valueList);
		reg.AddField(_T("UNTOUCH_INTERNAL_SSP_PULLUP (19)"),19,  1, _T("If this bit is blown then internal pull-ups for SSP are neither enabled nor disabled. This bit is used only if external pull-ups are implemented and ROM1:18 and/or ROM1:17 are blown."), valueList);
		reg.AddField(_T("ENABLE_NAND0_CE_RDY_PULLUP (20)"),20,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE0 and GPMI_RDY0."), valueList);
        reg.AddField(_T("ENABLE_NAND1_CE_RDY_PULLUP (21)"),21,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE1 and GPMI_RDY1."), valueList);
        reg.AddField(_T("ENABLE_NAND2_CE_RDY_PULLUP (22)"),22,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE2 and GPMI_RDY2."), valueList);
        reg.AddField(_T("ENABLE_NAND3_CE_RDY_PULLUP (23)"),23,  1, _T("If this bit is set, the ROM NAND driver will enable internal pull up for pins GPMI_CE3 and GPMI_RDY3."), valueList);
		reg.AddField(_T("USE_ALT_GPMI_CE2 (24)"),          24,  1, _T("If the bit is blown then ROM NAND driver will enable alternate pins for GPMI_CE2."), valueList);
		reg.AddField(_T("USE_ALT_GPMI_RDY2 (25)"),         25,  1, _T("If the bit is blown then ROM NAND driver will enable alternate pins for GPMI_RDY2."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - GPMI_D15");
		valueList[1] = _T("01 - LCD_RESET");
		valueList[2] = _T("10 - SSP_DETECT");
		valueList[3] = _T("11 - ROTARYB");
		reg.AddField(_T("USE_ALT_GPMI_CE3 (27:26)"),       26,  2, _T("These bits are used by ROM NAND driver to enable one of 4 alternate pins for GPMI_CE3. 00–GPMI_D15, 01-LCD_RESET, 10-SSP_DETECT and 11-ROTARYB."), valueList);

		valueList.clear();
		valueList[0] = _T("00 - GPMI_RDY3");
		valueList[1] = _T("01 - PWM2");
		valueList[2] = _T("10 - LCD_DOTCK");
		valueList[3] = _T("11 - Reserved");
		reg.AddField(_T("USE_ALT_GPMI_RDY3 (29:28)"),       28,  2, _T("These bits are used by ROM NAND driver to enable one of 4 alternate pins for GPMI_CE3. 00–GPMI_D15, 01-LCD_RESET, 10-SSP_DETECT and 11-ROTARYB."), valueList);

		valueList.clear();
#ifdef INTERNAL_BUILD
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("DISABLE_SECONDARY_BOOT (30)"),     30,  1, _T("If the bit is blown then ROM NAND driver will not use the secondary boot image."), valueList);
		reg.AddField(_T("Reserved (31)"),                   31,  1, _T("Reserved - do not blow this bit."), valueList);
#else
		reg.AddField(_T("Reserved (31:30)"),                30,  2, _T("Reserved - do not blow these bits."), valueList);
#endif
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM2, _T("HW_OCOTP_ROM2"), _T("ROM2"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

		reg.AddField(_T("USB_PID (15:0)"),              0, 16, _T("USB Product ID."), valueList);
		reg.AddField(_T("USB_VID (31:16)"),            16, 16, _T("USB Vendor ID."), valueList);
        
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM3, _T("HW_OCOTP_ROM3"), _T("ROM3"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

#ifdef INTERNAL_BUILD
		reg.AddField(_T("OSC_TRIM (9:0)"),              0, 10, _T("Oscillator trim value for off-chip Bluetooth device(Elwood). This bit-field should be described instead as SDK reserved bits in the customer spec."), valueList);
		reg.AddField(_T("Reserved (31:10)"),           10, 22, _T("Reserved - do not blow these bits."), valueList);
#else
		reg.AddField(_T("Reserved (31:0)"),             0, 32, _T("Reserved - do not blow these bits."), valueList);
#endif

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM4, _T("HW_OCOTP_ROM4"), _T("ROM4"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

		reg.AddField(_T("Reserved (31:0)"),                0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM5, _T("HW_OCOTP_ROM5"), _T("ROM5"), OtpRegType_Rom, PermissionType_External);

		valueList.clear();

        reg.AddField(_T("Reserved (31:0)"),                0, 32, _T("Reserved - do not blow these bits."), valueList);

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM6, _T("HW_OCOTP_ROM6"), _T("ROM6"), OtpRegType_Rom, PermissionType_External);

#ifdef INTERNAL_BUILD
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");
		reg.AddField(_T("DISABLE_TEST_MODES (0)"),      0,  1, _T("Disables manufacturing test modes. This bit will be listed as Reserved in the Data Sheet."), valueList);
		valueList.clear();
		reg.AddField(_T("Reserved (31:1)"),             1, 31, _T("Reserved - do not blow these bits."), valueList);
#else
		valueList.clear();
		reg.AddField(_T("Reserved (31:0)"),             0, 32, _T("Reserved - do not blow these bits."), valueList);
#endif
		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_ROM7, _T("HW_OCOTP_ROM7"), _T("ROM7"), OtpRegType_Rom, PermissionType_External);

		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ENABLE_PIN_BOOT_CHECK (0)"),   0,  1, _T("Enables the boot loader to first test the LCD_RS pin to determine if pin boot mode is enabled. If this bit is blown, and LCD_RS is pulled high, then boot mode is determined by the state of LCD_D[6:0] pins. If this bit is not blown, skip testing the LCD_RS pin and go directly to determining boot mode by reading the state of LCD_D[6:0]."), valueList);
		reg.AddField(_T("Reserved (1)"),                1,  1, _T("Reserved - do not blow this bit."), valueList);
        reg.AddField(_T("ENABLE_ARM_ICACHE (2)"),       2,  1, _T("Enables the ARM 926 ICache during boot."), valueList);
        reg.AddField(_T("I2C_USE_400KHZ (3)"),          3,  1, _T("Forces the I2C to be programmed by the boot loader to run at 400KHz. 100KHz is the default."), valueList);

		valueList.clear();
#ifdef INTERNAL_BUILD
		reg.AddField(_T("OCRAM_SS_TRIM (7:4)"),         4,  4, _T("On-chip RAM sense-amp speed trim value."), valueList);
#else
		reg.AddField(_T("Reserved (7:4)"),              4,  4, _T("Reserved - do not blow these bits."), valueList);
#endif
		valueList[0] = _T("Not blown");
		valueList[1] = _T("Blown");

		reg.AddField(_T("ENABLE_SSP_12MA_DRIVE (8)"),   8,  1, _T("Forces the SSP pins to drive 12mA, default is 4mA."), valueList);

#ifdef INTERNAL_BUILD
        reg.AddField(_T("RESET_USB_PHY_AT_STARTUP (9)"),9,  1, _T("Forces USBPHY to reset at startup."), valueList);
		reg.AddField(_T("USB_HID_SAFE_DATAOUT (10)"),  10,  1, _T("Forces USB report size to 64 bytes."), valueList);
		reg.AddField(_T("Reserved (11)"),              11,  1, _T("Reserved - do not blow this bit."), valueList);
		valueList.clear();
		reg.AddField(_T("RECOVERY_BOOT_MODE (19:12)"), 12,  8, _T("If USB is not desirable for recovery boot mode then these bits can be blown to have the ROM boot from a non-USB device in recovery mode. This bit will be listed as Reserved in the Data Sheet, as this function will only be made available to customers who request it."), valueList);
		reg.AddField(_T("Reserved (31:20)"),           20, 12, _T("Reserved - do not blow these bits."), valueList);
#else
		valueList.clear();
		reg.AddField(_T("Reserved (31:9)"),             9, 23, _T("Reserved - do not blow these bits."), valueList);
#endif

		_registers.push_back(reg);
    }
    {
	    OtpRegister reg(HW_OCOTP_VERSION, _T("HW_OCOTP_VERSION"), _T("OTP Controller Version"), OtpRegType_Control, PermissionType_External);
        _registers.push_back(reg);
    }

}
