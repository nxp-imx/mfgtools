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
    const uint8_t
        BLTC_INQUIRY                = 0x01,
        BLTC_DOWNLOAD_FW            = 0x02,
        BLTC_REQUEST_SENSE          = 0x03,
        BLTC_DEVICE_RESET           = 0x04,
        BLTC_DEVICE_POWER_DOWN      = 0x05;

    //------------------------------------------------------------------------------
    // Base PITC Command Set
    // Required commands for any PITC
    //------------------------------------------------------------------------------
    const uint8_t
        PITC_TEST_UNIT_READY        = 0x10,
        PITC_REQUEST_SENSE          = 0x11,
        PITC_INQUIRY                = 0x12,
        PITC_READ                   = 0x13,
        PITC_WRITE                  = 0x14;

    //------------------------------------------------------------------------------
    // Extended PITC Command Set
    //------------------------------------------------------------------------------
    const uint8_t
        PITC_SOME_FUTURE_CMD        = 0x80;

    //------------------------------------------------------------------------------
    // PITC Identifiers
    //------------------------------------------------------------------------------
    const uint32_t
        PITC_TYPE_LOAD_TEST         = 0x8000,
        PITC_TYPE_RAM_TEST          = 0x8001,
        PITC_TYPE_OTP_ACCESS        = 0x8002;

    //------------------------------------------------------------------------------
    // HID Report Types (IDs)
    //------------------------------------------------------------------------------
    const uint8_t 
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
        uint8_t Command;
        uint8_t Reserved[15];
        }CDBHIDCMD;

        // API_TYPE_BLTC, API_TYPE_PITC
        // HID Inquiry CDB
        //
        struct _CDBHIDINFO {
        uint8_t  Command;
        uint8_t  InfoPage;
        uint32_t InfoParam;
        uint8_t Reserved[10];
        };

        // API_TYPE_BLTC
        // HID Download Firmware CDB
        //
        struct _CDBHIDDOWNLOAD {
        uint8_t Command;
        uint32_t Length;
        uint8_t Reserved[11];
        };

        // API_TYPE_PITC
        // HID Read/Write CDB
        //
        struct _CDBHIDREADWRITE {
        uint8_t Command;
        uint32_t Address;
        uint32_t Length;
        uint32_t Flags;         // optional
        uint8_t Reserved[3];
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
        static const uint8_t InfoPage_Chip = 0x01, InfoPage_PitcStatus = 0x02, InfoPage_secConfig = 0x3;
		static const uint32_t PITC_STATUS_READY=0x00000000, PITC_STATUS_NOT_READY=0x00000001;
		static const uint32_t BLTC_SEC_CONFIG_DISABLE=0x00000000, BLTC_SEC_CONFIG_FAB=0x00000001,
							  BLTC_SEC_CONFIG_ENGINEERING=0x00000002,BLTC_SEC_CONFIG_PRODUCTION=0x00000003;
		HidInquiry(const uint8_t infoPage, const uint32_t infoParam = 0);
        virtual ~HidInquiry() {};

    private:
        static StApi* Create(CStdString paramStr);
        HidInquiry(CStdString& paramStr);
        virtual void ParseCdb();
        virtual void PrepareCommand();
        virtual void ProcessResponse(const uint8_t *const pData, const uint32_t start, const uint32_t count);
    public:
        virtual const CStdString& ResponseString();

        uint16_t GetChipId() const;
        uint16_t GetChipRevision() const;
        uint16_t GetRomVersion() const;
        uint16_t GetRomLoaderProtocolVersion() const;
        uint32_t GetPitcStatus() const;
		uint32_t GetSecConfig() const;

    private:
        ParameterT<uint8_t> _infoPage;
        ParameterT<uint32_t> _infoParam;
        struct ChipInfoPage {
            uint16_t ChipId;
            uint16_t ChipRevision;
            uint16_t RomVersion;
            uint16_t RomLoaderProtocolVersion;
            ChipInfoPage() { clear(); };
            const uint32_t size() const { return sizeof(*this); };
            void clear() { ChipId = ChipRevision = RomVersion = RomLoaderProtocolVersion = 0; };
        }_chipInfo;
        uint32_t _pitcStatus;
		uint32_t _secConfigStatus;
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
        HidDownloadFw(const uint8_t * const pData, const size_t size);
        virtual ~HidDownloadFw() {};
    private:
        static StApi* Create(CStdString paramStr);
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
        static StApi* Create(CStdString paramStr);
        HidBltcRequestSense();
        virtual ~HidBltcRequestSense() {};
        virtual void PrepareCommand();
        virtual void ProcessResponse(const uint8_t *const pData, const uint32_t start, const uint32_t count);
    public:
        virtual const CStdString& ResponseString();

        uint32_t GetSenseCode() const;

    private:
        uint32_t _senseCode;
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
        static StApi* Create(CStdString paramStr);
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
        static StApi* Create(CStdString paramStr);
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
        static StApi* Create(CStdString paramStr);
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
        static StApi* Create(CStdString paramStr);
        virtual void PrepareCommand();
        virtual void ProcessResponse(const uint8_t *const pData, const uint32_t start, const uint32_t count);
    public:
        virtual const CStdString& ResponseString();

        HidPitcRequestSense();
        virtual ~HidPitcRequestSense() {};

        uint32_t GetSenseCode() const;

    private:
        uint32_t _senseCode;
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
        static const uint8_t InfoPage_Pitc = 0x01, InfoPage_PitcSense = 0x02, InfoPage_OtpReg = 0x03, InfoPage_PersistentReg = 0x04;

    private:
        static StApi* Create(CStdString paramStr);
        HidPitcInquiry(CStdString& paramStr);
        virtual void ParseCdb();
        virtual void PrepareCommand();
        virtual void ProcessResponse(const uint8_t *const pData, const uint32_t start, const uint32_t count);
    public:
        virtual const CStdString& ResponseString();

        HidPitcInquiry(const uint8_t infoPage, const uint32_t infoParam = 0);
        virtual ~HidPitcInquiry() {};

        const uint16_t GetPitcId() const;
        const StVersionInfo& GetPitcVersion() const;

        const CStdString GetPitcSenseString() const;
        
        struct OtpRegInfoPage {
            uint32_t Address;
            uint8_t  LockBit;
            uint8_t  OtpBank;
            uint8_t  OtpWord;
            uint8_t  Locked;
            uint8_t  Shadowed;
            OtpRegInfoPage() { clear(); };
            const uint32_t size() const { return sizeof(*this); };
            void clear() { Address = LockBit = OtpBank = OtpWord = Locked = Shadowed = 0; };
        };

        const OtpRegInfoPage GetOtpRegInfoPage() const;
        const uint32_t GetOtpRegAddress() const;
        const uint8_t GetOtpRegLockBit() const;
        const uint8_t GetOtpRegBank() const;
        const uint8_t GetOtpRegWord() const;
        const uint8_t IsOtpRegLocked() const;
        const uint8_t IsOtpRegShadowed() const;

        const uint32_t GetPersistentRegAddress() const;
        const uint32_t GetPersistentRegValue() const;
 
    private:
        ParameterT<uint8_t>  _infoPage;
        ParameterT<uint32_t> _infoParam;
        
        struct PitcInfoPage {
            uint32_t Id;
		    StVersionInfo Version;
            PitcInfoPage() { clear(); };
            const uint32_t size() const { return (uint32_t) sizeof(Id) + Version.size(); };
            void clear() { Id = 0;  Version.clear(); };
        }_pitcInfo;

        CStdString _pitcSenseInfo;
        
        OtpRegInfoPage _otpRegInfo;

        struct PersistentInfoPage {
            uint32_t Address;
            uint32_t Value;
            PersistentInfoPage() { clear(); };
            const uint32_t size() const { return (uint32_t) sizeof(Address) + sizeof(Value); };
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
        static StApi* Create(CStdString paramStr);
        HidPitcRead(CStdString paramStr);
        virtual void ParseCdb();
        virtual void PrepareCommand();
        virtual void ProcessResponse(const uint8_t *const pData, const uint32_t start, const uint32_t count);
    public:
        virtual const CStdString& ResponseString();

        HidPitcRead(const uint32_t address, const uint32_t length, const uint32_t flags, const uint32_t dataSize);
        virtual ~HidPitcRead() {};

    private:
        ParameterT<uint32_t> _address;
        ParameterT<uint32_t> _length;
		ParameterT<uint32_t> _flags;
		ParameterT<uint32_t> _dataSize;
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
        static StApi* Create(CStdString paramStr);
        HidPitcWrite(LPCTSTR fileName);
        virtual void ParseCdb();
        virtual void PrepareCommand();

    public:
        HidPitcWrite(const uint32_t address, const uint32_t length, const uint32_t flags, 
            const uint8_t * const pData = NULL, const uint32_t dataSize = 0);
        virtual ~HidPitcWrite() {};

    private:
        ParameterT<uint32_t> _address;
        ParameterT<uint32_t> _length;
		ParameterT<uint32_t> _flags;
    };

} // namespace api

using namespace api;

#endif // !defined(STHIDAPI_H__INCLUDED)
