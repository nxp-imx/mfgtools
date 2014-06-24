/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StHidApi.h: interface for the StApi class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(STHIDAPI_H__INCLUDED)
#define STHIDAPI_H__INCLUDED

#include "StApi.h"
#include "StFwComponent.h"

namespace api
{
    //------------------------------------------------------------------------------
    // ROM Command Set
    //------------------------------------------------------------------------------
    const UCHAR
        BLTC_INQUIRY                = 0x01,
        BLTC_DOWNLOAD_FW            = 0x02,
        BLTC_REQUEST_SENSE          = 0x03,
        BLTC_DEVICE_RESET           = 0x04,
        BLTC_DEVICE_POWER_DOWN      = 0x05;

    //------------------------------------------------------------------------------
    // Base PITC Command Set
    // Required commands for any PITC
    //------------------------------------------------------------------------------
    const UCHAR
        PITC_TEST_UNIT_READY        = 0x10,
        PITC_REQUEST_SENSE          = 0x11,
        PITC_INQUIRY                = 0x12,
        PITC_READ                   = 0x13,
        PITC_WRITE                  = 0x14;

    //------------------------------------------------------------------------------
    // Extended PITC Command Set
    //------------------------------------------------------------------------------
    const UCHAR
        PITC_SOME_FUTURE_CMD        = 0x80;

    //------------------------------------------------------------------------------
    // PITC Identifiers
    //------------------------------------------------------------------------------
    const UINT
        PITC_TYPE_LOAD_TEST         = 0x8000,
        PITC_TYPE_RAM_TEST          = 0x8001,
        PITC_TYPE_OTP_ACCESS        = 0x8002;

    //------------------------------------------------------------------------------
    // HID Report Types (IDs)
    //------------------------------------------------------------------------------
    const UCHAR 
        HID_BLTC_REPORT_TYPE_COMMAND_OUT    = 0x01,
        HID_BLTC_REPORT_TYPE_DATA_OUT       = 0x02, 
        HID_BLTC_REPORT_TYPE_DATA_IN        = 0x03, 
        HID_BLTC_REPORT_TYPE_STATUS_IN      = 0x04,
        HID_PITC_REPORT_TYPE_COMMAND_OUT    = 0x05,
        HID_PITC_REPORT_TYPE_DATA_OUT       = 0x06, 
        HID_PITC_REPORT_TYPE_DATA_IN        = 0x07, 
        HID_PITC_REPORT_TYPE_STATUS_IN      = 0x08;


#pragma pack(1)
    //------------------------------------------------------------------------------
    // HID Command Descriptor Block.
    //------------------------------------------------------------------------------
    union _ST_HID_CDB {

        // API_TYPE_BLTC, API_TYPE_PITC
        // HID Command CDB
        //
        struct _CDBHIDCMD {
        UCHAR Command;
        UCHAR Reserved[15];
        }CDBHIDCMD;

        // API_TYPE_BLTC, API_TYPE_PITC
        // HID Inquiry CDB
        //
        struct _CDBHIDINFO {
        UCHAR  Command;
        UCHAR  InfoPage;
        UINT InfoParam;
        UCHAR Reserved[10];
        };

        // API_TYPE_BLTC
        // HID Download Firmware CDB
        //
        struct _CDBHIDDOWNLOAD {
        UCHAR Command;
        UINT Length;
        UCHAR Reserved[11];
        };

        // API_TYPE_PITC
        // HID Read/Write CDB
        //
        struct _CDBHIDREADWRITE {
        UCHAR Command;
        UINT Address;
        UINT Length;
        UINT Flags;         // optional
        UCHAR Reserved[3];
        };
    };
#pragma pack()

    ////////////////////////////////////////////////////////////////////////////////
    //
    // ST_HID_BLTC_COMMAND: HidInquiry
    //
    ////////////////////////////////////////////////////////////////////////////////
    class HidInquiry : public StApiT<_ST_HID_CDB::_CDBHIDINFO>
    {
    public:
        friend class StApiFactory;
        static const UCHAR InfoPage_Chip = 0x01, InfoPage_PitcStatus = 0x02, InfoPage_secConfig = 0x3;
		static const UINT PITC_STATUS_READY=0x00000000, PITC_STATUS_NOT_READY=0x00000001;
		static const UINT BLTC_SEC_CONFIG_DISABLE=0x00000000, BLTC_SEC_CONFIG_FAB=0x00000001,
							  BLTC_SEC_CONFIG_ENGINEERING=0x00000002,BLTC_SEC_CONFIG_PRODUCTION=0x00000003;
		HidInquiry(const UCHAR infoPage, const UINT infoParam = 0);
        virtual ~HidInquiry() {};

    private:
        static StApi* Create(CString paramStr);
        HidInquiry(CString& paramStr);
        virtual void ParseCdb();
        virtual void PrepareCommand();
        virtual void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count);
    public:
        virtual const CString& ResponseString();

        USHORT GetChipId() const;
        USHORT GetChipRevision() const;
        USHORT GetRomVersion() const;
        USHORT GetRomLoaderProtocolVersion() const;
        UINT GetPitcStatus() const;
		UINT GetSecConfig() const;

    private:
        ParameterT<UCHAR> _infoPage;
        ParameterT<UINT> _infoParam;
        struct ChipInfoPage {
            USHORT ChipId;
            USHORT ChipRevision;
            USHORT RomVersion;
            USHORT RomLoaderProtocolVersion;
            ChipInfoPage() { clear(); };
            const UINT size() const { return sizeof(*this); };
            void clear() { ChipId = ChipRevision = RomVersion = RomLoaderProtocolVersion = 0; };
        }_chipInfo;
        UINT _pitcStatus;
		UINT _secConfigStatus;
    };
    //////////////////////////////////////////////////////////////////////
    //
    // ST_HID_BLTC_COMMAND: HidDownloadFw
    //
    //////////////////////////////////////////////////////////////////////
    class HidDownloadFw : public StApiT<_ST_HID_CDB::_CDBHIDDOWNLOAD>
    {
        friend class StApiFactory;

    public:
        HidDownloadFw(const UCHAR * const pData, const size_t size);
        virtual ~HidDownloadFw() {};
    private:
        static StApi* Create(CString paramStr);
        HidDownloadFw(LPCTSTR fileName);
        virtual void PrepareCommand();
    };

    //////////////////////////////////////////////////////////////////////
    //
    // ST_HID_BLTC_COMMAND: HidBltcRequestSense
    //
    //////////////////////////////////////////////////////////////////////
    enum _BLTC_SENSE_CODE {
        BLTC_SENSE_NO_ERRORS                        = 0x00000000,
        BLTC_SENSE_INVALID_CBW                      = 0x00000001,
        BLTC_SENSE_INVALID_CDB_COMMAND              = 0x00000002,
        BLTC_SENSE_INVALID_INQUIRY_PAGE             = 0x00010001,
        BLTC_SENSE_FW_DLOAD_INVALID_LENGTH          = 0x00020001,
        BLTC_SENSE_ROM_LOADER_INVALID_COMMAND       = 0x00020002, 
        BLTC_SENSE_ROM_LOADER_DECRYPTION_FAILURE    = 0x00020003
    };

    class HidBltcRequestSense : public StApiT<_ST_HID_CDB::_CDBHIDCMD>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CString paramStr);
        HidBltcRequestSense();
        virtual ~HidBltcRequestSense() {};
        virtual void PrepareCommand();
        virtual void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count);
    public:
        virtual const CString& ResponseString();

        UINT GetSenseCode() const;

    private:
        UINT _senseCode;
    };

    //////////////////////////////////////////////////////////////////////
    //
    // ST_HID_BLTC_COMMAND: HidDeviceReset
    //
    //////////////////////////////////////////////////////////////////////
    class HidDeviceReset : public StApiT<_ST_HID_CDB::_CDBHIDCMD>
    {
        friend class StApiFactory;

	public:
        HidDeviceReset();
        virtual ~HidDeviceReset() {};

	private:
        static StApi* Create(CString paramStr);
        virtual void PrepareCommand();
    };

    //////////////////////////////////////////////////////////////////////
    //
    // ST_HID_BLTC_COMMAND: HidDevicePowerDown
    //
    //////////////////////////////////////////////////////////////////////
    class HidDevicePowerDown : public StApiT<_ST_HID_CDB::_CDBHIDCMD>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CString paramStr);
        HidDevicePowerDown();
        virtual ~HidDevicePowerDown() {};
        virtual void PrepareCommand();
    };

    //////////////////////////////////////////////////////////////////////
    //
    // ST_HID_PITC_COMMAND: HidTestUnitReady
    //
    //////////////////////////////////////////////////////////////////////
    class HidTestUnitReady : public StApiT<_ST_HID_CDB::_CDBHIDCMD>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CString paramStr);
        virtual void PrepareCommand();
    public:
        HidTestUnitReady();
        virtual ~HidTestUnitReady() {};
    };

    //////////////////////////////////////////////////////////////////////
    //
    // ST_HID_PITC_COMMAND: HidPitcRequestSense
    //
    //////////////////////////////////////////////////////////////////////
    enum _PITC_SENSE_CODE {
        PITC_SENSE_NO_ERRORS                        = 0x00000000,
        PITC_SENSE_INVALID_CBW                      = 0x00000001,
        PITC_SENSE_INVALID_CDB_COMMAND              = 0x00000002,
        PITC_SENSE_INVALID_INQUIRY_PAGE             = 0x00120001,
        PITC_SENSE_NO_SENSE_INFO                    = 0x00120202,
        PITC_SENSE_INVALID_OTP_REGISTER             = 0x00120302,
        PITC_SENSE_OTP_INFO_DENIED                  = 0x00120303,
        PITC_SENSE_INVALID_READ_ADDRESS             = 0x00130010,
        PITC_SENSE_BUFFER_READ_OVERFLOW             = 0x00130011,
        PITC_SENSE_ACCESS_READ_DENIED               = 0x00130012,
        PITC_SENSE_INVALID_WRITE_ADDRESS            = 0x00140010,
        PITC_SENSE_BUFFER_WRITE_OVERFLOW            = 0x00140011,
        PITC_SENSE_ACCESS_WRITE_DENIED              = 0x00140012
    };

    class HidPitcRequestSense : public StApiT<_ST_HID_CDB::_CDBHIDCMD>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CString paramStr);
        virtual void PrepareCommand();
        virtual void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count);
    public:
        virtual const CString& ResponseString();

        HidPitcRequestSense();
        virtual ~HidPitcRequestSense() {};

        UINT GetSenseCode() const;

    private:
        UINT _senseCode;
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // ST_HID_PITC_COMMAND: HidPitcInquiry
    //
    ////////////////////////////////////////////////////////////////////////////////
    class HidPitcInquiry : public StApiT<_ST_HID_CDB::_CDBHIDINFO>
    {
    public:
        friend class StApiFactory;
        static const UCHAR InfoPage_Pitc = 0x01, InfoPage_PitcSense = 0x02, InfoPage_OtpReg = 0x03, InfoPage_PersistentReg = 0x04;

    private:
        static StApi* Create(CString paramStr);
        HidPitcInquiry(CString& paramStr);
        virtual void ParseCdb();
        virtual void PrepareCommand();
        virtual void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count);
    public:
        virtual const CString& ResponseString();

        HidPitcInquiry(const UCHAR infoPage, const UINT infoParam = 0);
        virtual ~HidPitcInquiry() {};

        const USHORT GetPitcId() const;
        const StVersionInfo& GetPitcVersion() const;

        const CString GetPitcSenseString() const;
        
        struct OtpRegInfoPage {
            UINT Address;
            UCHAR  LockBit;
            UCHAR  OtpBank;
            UCHAR  OtpWord;
            UCHAR  Locked;
            UCHAR  Shadowed;
            OtpRegInfoPage() { clear(); };
            const UINT size() const { return sizeof(*this); };
            void clear() { Address = LockBit = OtpBank = OtpWord = Locked = Shadowed = 0; };
        };

        const OtpRegInfoPage GetOtpRegInfoPage() const;
        const UINT GetOtpRegAddress() const;
        const UCHAR GetOtpRegLockBit() const;
        const UCHAR GetOtpRegBank() const;
        const UCHAR GetOtpRegWord() const;
        const UCHAR IsOtpRegLocked() const;
        const UCHAR IsOtpRegShadowed() const;

        const UINT GetPersistentRegAddress() const;
        const UINT GetPersistentRegValue() const;
 
    private:
        ParameterT<UCHAR>  _infoPage;
        ParameterT<UINT> _infoParam;
        
        struct PitcInfoPage {
            UINT Id;
		    StVersionInfo Version;
            PitcInfoPage() { clear(); };
            const UINT size() const { return (UINT) sizeof(Id) + Version.size(); };
            void clear() { Id = 0;  Version.clear(); };
        }_pitcInfo;

        CString _pitcSenseInfo;
        
        OtpRegInfoPage _otpRegInfo;

        struct PersistentInfoPage {
            UINT Address;
            UINT Value;
            PersistentInfoPage() { clear(); };
            const UINT size() const { return (UINT) sizeof(Address) + sizeof(Value); };
            void clear() { Address = Value = 0; };
        }_persistentInfo;
    };

    //////////////////////////////////////////////////////////////////////
    //
    // ST_HID_PITC_COMMAND: HidPitcRead
    //
    //////////////////////////////////////////////////////////////////////
    class HidPitcRead : public StApiT<_ST_HID_CDB::_CDBHIDREADWRITE>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CString paramStr);
        HidPitcRead(CString paramStr);
        virtual void ParseCdb();
        virtual void PrepareCommand();
        virtual void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count);
    public:
        virtual const CString& ResponseString();

        HidPitcRead(const UINT address, const UINT length, const UINT flags, const UINT dataSize);
        virtual ~HidPitcRead() {};

    private:
        ParameterT<UINT> _address;
        ParameterT<UINT> _length;
		ParameterT<UINT> _flags;
		ParameterT<UINT> _dataSize;
    };

    //////////////////////////////////////////////////////////////////////
    //
    // ST_HID_PITC_COMMAND: HidPitcWrite
    //
    //////////////////////////////////////////////////////////////////////
    class HidPitcWrite : public StApiT<_ST_HID_CDB::_CDBHIDREADWRITE>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CString paramStr);
        HidPitcWrite(LPCTSTR fileName);
        virtual void ParseCdb();
        virtual void PrepareCommand();

    public:
        HidPitcWrite(const UINT address, const UINT length, const UINT flags, 
            const UCHAR * const pData = NULL, const UINT dataSize = 0);
        virtual ~HidPitcWrite() {};

    private:
        ParameterT<UINT> _address;
        ParameterT<UINT> _length;
		ParameterT<UINT> _flags;
    };

} // namespace api

using namespace api;

#endif // !defined(STHIDAPI_H__INCLUDED)
