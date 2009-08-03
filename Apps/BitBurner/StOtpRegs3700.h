#pragma once

#include "Common/StdString.h"
#include "Common/StdInt.h"
#include "Libs/DevSupport/StHidApi.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class StOtpRegs3700
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StOtpRegs
{
    // StOtpRegs defines
public:
	enum OtpRegisterType { OtpRegType_Control = 1, OtpRegType_Crypto, OtpRegType_Customer, OtpRegType_HwCaps, OtpRegType_SwCaps, OtpRegType_SgtlOperations, OtpRegType_Unassigned, OtpRegType_Rom };
    enum OtpRegisters
	{
		/*HW_OCOTP_CTRL=0, HW_OCOTP_DATA,*/ HW_OCOTP_CUST0=0, HW_OCOTP_CUST1, HW_OCOTP_CUST2, HW_OCOTP_CUST3, HW_OCOTP_CRYPTO0, HW_OCOTP_CRYPTO1, HW_OCOTP_CRYPTO2, HW_OCOTP_CRYPTO3,
		HW_OCOTP_HWCAP0, HW_OCOTP_HWCAP1, HW_OCOTP_HWCAP2, HW_OCOTP_HWCAP3, HW_OCOTP_HWCAP4, HW_OCOTP_HWCAP5, HW_OCOTP_SWCAP, HW_OCOTP_CUSTCAP, HW_OCOTP_LOCK,
		HW_OCOTP_OPS0, HW_OCOTP_OPS1, HW_OCOTP_OPS2, HW_OCOTP_OPS3, HW_OCOTP_UN0, HW_OCOTP_UN1, HW_OCOTP_UN2, HW_OCOTP_ROM0, HW_OCOTP_ROM1, HW_OCOTP_ROM2, HW_OCOTP_ROM3,
		HW_OCOTP_ROM4, HW_OCOTP_ROM5, HW_OCOTP_ROM6, HW_OCOTP_ROM7, HW_OCOTP_VERSION
	};
	enum PermissionType { PermissionType_Internal = 1, PermissionType_External };

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // struct Register
    //
    // - Holds the _headsPerCylinder and _sectorsPerTrack member variables and a Packed struct used for constructing a
    //   3-byte word representing a Cylinder,Head,Sector (C,H,S) address.
    // - Several functions ar available for converting LBA addresses to CHS address and member variable access.
    // 
    // - Initialized by: CHS(const uint32_t totalSectors)
    //
    //   - Param: const uint32_t totalSectors - The total number of sectors from Absolute Sector 0 (MBR) to the end
    //                                          of the media.
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	struct OtpField
    {
		static const uint32_t FieldMask[];
		OtpField( LPCTSTR name, const uint32_t offset, const uint8_t length, LPCTSTR desc, const std::map<uint32_t,CStdString> valueList)
			: Name(name)
			, Offset(offset)
			, Length(length)
			, Desc(desc)
			, ValueList(valueList)
		{};

		OtpField& operator=(const OtpField& field)
		{
			return *this;
		};
		const uint32_t Value(const uint32_t regValue) const;
		const CStdString Format(const uint32_t regValue) const;
		const CStdString Name;
		const CStdString Desc;
		const uint8_t Offset;
		const uint8_t Length;
		const std::map<uint32_t,CStdString> ValueList;
    };

    struct OtpRegister
    {
		OtpRegister(const uint32_t offset, LPCTSTR name, LPCTSTR desc, OtpRegisterType type, PermissionType permission = PermissionType_Internal) 
			: Offset(offset)
			, Name(name)
			, Desc(desc)
			, Type(type)
			, Permissions(permission)
			, Value(0)
		{};

		~OtpRegister()
		{ 
			Fields.clear();
		};

		OtpRegister& operator=(const OtpRegister& reg)
		{
			return *this;
		};

		void AddField(LPCTSTR name, const uint32_t offset, const uint8_t length, LPCTSTR desc, const std::map<uint32_t,CStdString> valueList);
		
		const uint32_t Offset;
		const CStdString Name;
		const CStdString Desc;
		const OtpRegisterType Type;
		const PermissionType Permissions;
		uint32_t Value;

		std::vector<OtpField> Fields;
		HidPitcInquiry::OtpRegInfoPage Info;

    private:
        void InitRegisterList();
    };

	std::vector<OtpRegister> _registers;

    virtual ~StOtpRegs() { _registers.clear(); };
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class StOtpRegs3700
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StOtpRegs3700 : public StOtpRegs
{
public:
    StOtpRegs3700();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class StOtpRegs3770
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StOtpRegs3770 : public StOtpRegs
{
public:
    StOtpRegs3770();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class StOtpRegs3780
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StOtpRegs3780 : public StOtpRegs
{
public:
    StOtpRegs3780();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class StOtpRegsMX23
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StOtpRegsMX23 : public StOtpRegs
{
public:
    StOtpRegsMX23();
};
