/****************************************************************************
*
*  (C) COPYRIGHT 2004, MICROSOFT CORP.
*
*  VERSION:     1.0
*
*  DESCRIPTION:
*    Structures and constants needed to issue vendor-specific Media Transfer
*    Protocol (MTP) commands through DeviceIOControl mechanism.
*
*****************************************************************************/
#pragma once

#include "Common/StdInt.h"
#include "StApi.h"

namespace api
{

	//
	// Pass this value in the dwIoControlCode argument of IWMDMDevice3::DeviceIoControl
	// to execute a direct MTP command
	//
	#define IOCTL_MTP_CUSTOM_COMMAND    0x3150544D

	#define MTP_OPCODE_GET_DEVICE_INFO	0x1001
	// MAY CONFLICT WITH MICROSOFT
	// #define MTP_OPCODE_SIGMATEL_RESET     0x9201
	// #define MTP_OPCODE_SIGMATEL_ERASEBOOT 0x9202
	// OFFICIALLY RELEASED
	#define MTP_OPCODE_SIGMATEL_UNDEFINED         0x97F0 
	#define MTP_OPCODE_SIGMATEL_RESET             0x97F1
	#define MTP_OPCODE_SIGMATEL_ERASEBOOT         0x97F2
	#define MTP_OPCODE_SIGMATEL_FORCERECV         0x97F3
	#define MTP_OPCODE_SIGMATEL_RESET_TO_UPDATER  0x97F4
	#define MTP_OPCODE_SIGMATEL_GET_DRIVE_VERSION 0x97F5
	#define MTP_OPCODE_SIGMATEL_SET_UPDATE_FLAG   0x97F6
	#define MTP_OPCODE_SIGMATEL_SWITCH_TO_MSC     0x97F7
	//
	// MTP command request
	//
	const uint32_t MTP_COMMAND_MAX_PARAMS  = 5;
	const uint32_t MTP_RESPONSE_MAX_PARAMS = 5;

	//
	// MTP response codes
	//
	const uint16_t MTP_RESPONSE_OK = 0x2001;

	#define CMD_BUF_SIZE 1024

	#pragma pack(push, Old, 1)

	typedef struct _MTP_COMMAND_DATA_IN
	{
		uint16_t   OpCode;                         // Opcode
		uint32_t   NumParams;                      // Number of parameters passed in
		uint32_t   Params[MTP_COMMAND_MAX_PARAMS]; // Parameters to the command
		uint32_t   NextPhase;                      // Indicates whether the command has a read data,
												   //   write data, or no data phase.
		uint32_t   CommandWriteDataSize;           // Number of bytes contained in CommandWriteData.
		uint8_t    CommandWriteData[CMD_BUF_SIZE]; // Optional first byte of data to
												   //   write to the device if NextPhase is MTP_NEXTPHASE_WRITE_DATA
	} MTP_COMMAND_DATA_IN, *PMTP_COMMAND_DATA_IN;

	//
	// MTP response block
	//
	typedef struct _MTP_COMMAND_DATA_OUT
	{
		uint16_t   ResponseCode;                       // Response code
		uint32_t   NumParams;                          // Number of parameters for this response
		uint32_t   Params[MTP_RESPONSE_MAX_PARAMS];    // Parameters of the response
		uint32_t   CommandReadDataSize;                // Number of bytes contained in CommandReadData.
		uint8_t    CommandReadData[CMD_BUF_SIZE];      // Optional first byte of data to
													   //   read from the device if NextPhase is MTP_NEXTPHASE_READ_DATA
	} MTP_COMMAND_DATA_OUT, *PMTP_COMMAND_DATA_OUT;

	#pragma pack(pop, Old)

	//
	// Handy structure size constants
	//
	#define SIZEOF_REQUIRED_COMMAND_DATA_IN (sizeof(MTP_COMMAND_DATA_IN) - CMD_BUF_SIZE)
	#define SIZEOF_REQUIRED_COMMAND_DATA_OUT (sizeof(MTP_COMMAND_DATA_OUT) - CMD_BUF_SIZE)

	//
	// NextPhase constants
	//
	#define MTP_NEXTPHASE_READ_DATA     1
	#define MTP_NEXTPHASE_WRITE_DATA    2
	#define MTP_NEXTPHASE_NO_DATA       3

    //////////////////////////////////////////////////////////////////////
    //
    // ST_MTP_COMMAND: MtpResetToRecovery
    //
    //////////////////////////////////////////////////////////////////////
    class MtpResetToRecovery : public StApiT<_MTP_COMMAND_DATA_IN>
    {
        friend class StApiFactory;

	public: //clw origiinally wanted only StApiFactory to create apis
        MtpResetToRecovery();
        virtual ~MtpResetToRecovery() {};
    
	private:
        static StApi* Create(CStdString paramStr);
        virtual void PrepareCommand();
    };

    //////////////////////////////////////////////////////////////////////
    //
    // ST_MTP_COMMAND: MtpEraseBootmanager
    //
    //////////////////////////////////////////////////////////////////////
    class MtpEraseBootmanager : public StApiT<_MTP_COMMAND_DATA_IN>
    {
        friend class StApiFactory;

	public:
		MtpEraseBootmanager();
        virtual ~MtpEraseBootmanager() {};

	private:
        static StApi* Create(CStdString paramStr);
        virtual void PrepareCommand();
    };

	    //////////////////////////////////////////////////////////////////////
    //
    // ST_MTP_COMMAND: MtpDeviceReset
    //
    //////////////////////////////////////////////////////////////////////
    class MtpDeviceReset : public StApiT<_MTP_COMMAND_DATA_IN>
    {
        friend class StApiFactory;

	public:
        MtpDeviceReset();
        virtual ~MtpDeviceReset() {};

	private:
        static StApi* Create(CStdString paramStr);
        virtual void PrepareCommand();
    };

	//////////////////////////////////////////////////////////////////////
    //
    // ST_MTP_COMMAND: MtpResetToUpdater
    //
    //////////////////////////////////////////////////////////////////////
    class MtpResetToUpdater : public StApiT<_MTP_COMMAND_DATA_IN>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CStdString paramStr);
        MtpResetToUpdater();
        virtual ~MtpResetToUpdater() {};
        virtual void PrepareCommand();
    };

	//////////////////////////////////////////////////////////////////////
    //
    // ST_MTP_COMMAND: MtpGetDriveVersion
    //
    //////////////////////////////////////////////////////////////////////
    class MtpGetDriveVersion : public StApiT<_MTP_COMMAND_DATA_IN>
    {
        friend class StApiFactory;
	
    private:
        static StApi* Create(CStdString paramStr);
//        MtpGetDriveVersion(CStdString paramStr);
		MtpGetDriveVersion(const media::LogicalDriveTag driveTag = media::DriveTag_UpdaterNand,
			const _LOGICAL_DRIVE_INFO versionType = DriveInfoComponentVersion);
        virtual ~MtpGetDriveVersion() {};
        virtual void ParseCdb();
        virtual void PrepareCommand();
		void InitParamTypes();
        virtual void ProcessResponse(const uint8_t *const pData, const uint32_t start, const uint32_t count);

    public:
		StVersionInfo GetVersion() const { return _version; };

	private:
		ParameterT<media::LogicalDriveTag> _driveTag;
		ParameterT<_LOGICAL_DRIVE_INFO> _versionType;
		
		StVersionInfo _version;
   };

	//////////////////////////////////////////////////////////////////////
    //
    // ST_MTP_COMMAND: MtpSetUpdateFlag
    //
    //////////////////////////////////////////////////////////////////////
    class MtpSetUpdateFlag : public StApiT<_MTP_COMMAND_DATA_IN>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CStdString paramStr);
        MtpSetUpdateFlag();
        virtual ~MtpSetUpdateFlag() {};
        virtual void PrepareCommand();
    };

	//////////////////////////////////////////////////////////////////////
    //
    // ST_MTP_COMMAND: MtpSwitchToMsc
    //
    //////////////////////////////////////////////////////////////////////
    class MtpSwitchToMsc : public StApiT<_MTP_COMMAND_DATA_IN>
    {
        friend class StApiFactory;

    private:
        static StApi* Create(CStdString paramStr);
        MtpSwitchToMsc();
        virtual ~MtpSwitchToMsc() {};
        virtual void PrepareCommand();
    };

}

