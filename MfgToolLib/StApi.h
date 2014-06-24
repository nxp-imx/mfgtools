/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// StApi.h: interface for the StApi class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STDDIAPI_H__343A6E6E_764F_415F_9746_7B39782BA186__INCLUDED_)
#define AFX_STDDIAPI_H__343A6E6E_764F_415F_9746_7B39782BA186__INCLUDED_

#ifndef WIN32
	#ifdef MACOSX
		#include "stscsi_mac.h"
	#else
		#include "stscsi_macos9.h"
	#endif	 // MACOSX
//	typedef void *PDISK_GEOMETRY;
//	typedef void *PDRIVE_LAYOUT_INFORMATION;
#else // !WIN32
	#include <Wtypes.h>
	#define _NTSCSI_USER_MODE_     // to keep srb.h from being included
	#include <scsi.h>
	#include <ntddscsi.h>
#endif	 // WIN32

#include <map>
#include <assert.h>
#include "StMedia.h"
#include "StVersionInfo.h"
#include "ParameterT.h"

namespace api
{

	#define ST_SCSIOP_READ						0xC0
	#define ST_SCSIOP_WRITE						0xC1
    /// The StUtp Command. Byte[0] of the CDB.
	#define ST_SCSIOP_UTP						0xF0

	enum _API_TYPE {
		API_TYPE_ST_SCSI =0, 
		API_TYPE_SCSI    =1, 
		API_TYPE_BLTC    =2, 
		API_TYPE_PITC    =3,
		API_TYPE_ST_MTP  =4,
		API_TYPE_UTP     =5
	};

	const UCHAR ST_READ_CMD=0, ST_WRITE_CMD=1, ST_WRITE_CMD_PLUS_DATA=2;

	enum _DDI_COMMAND_SET {
		DDI_GET_PROTOCOL_VERSION				= 0x00,					
		DDI_GET_STATUS							= 0x01,							
		DDI_GET_LOGICAL_MEDIA_INFO				= 0x02,							
		DDI_GET_ALLOCATION_TABLE				= 0x05,							
		DDI_SET_ALLOCATE_TABLE					= 0x06,									
		DDI_ERASE_LOGICAL_MEDIA					= 0x07,									
		DDI_ERASE_LOGICAL_MEDIA_ASYNC			= 0x08,									
		DDI_GET_LOGICAL_DRIVE_INFO				= 0x12,									
		DDI_READ_LOGICAL_DRIVE_SECTOR			= 0x13,									
		DDI_SET_LOGICAL_DRIVE_INFO				= 0x20,									
		DDI_WRITE_LOGICAL_DRIVE_SECTOR			= 0x23,
		DDI_ERASE_LOGICAL_DRIVE					= 0x2f,
		DDI_GET_CHIP_MAJOR_REV_ID				= 0x30,
		DDI_CHIP_RESET							= 0x31,
		DDI_GET_CHIP_SERIAL_NUMBER_INFO			= 0x32, // ProtocolUpdater2, ProtocolUpdaterJanus2
		DDI_FLUSH_LOGICAL_DRIVE					= 0x33, //TODO: add DDI_FLUSH_LOGICAL_DRIVE
		DDI_GET_PHYSICAL_MEDIA_INFO				= 0x34, //TODO: add DDI_GET_PHYSICAL_MEDIA_INFO // response size == 8(words)*3Bytes == 24 bytes
		DDI_READ_PHYSICAL_MEDIA_SECTOR   		= 0x35, //TODO: add DDI_READ_PHYSICAL_MEDIA_SECTOR
		DDI_GET_CHIP_PART_REV_ID                = 0x36,
		DDI_GET_ROM_REV_ID      				= 0x37,
		DDI_WRITE_LOGICAL_512_DRIVE_SECTOR		= 0x38,
		DDI_GET_JANUS_STATUS                    = 0x40,
		DDI_INITIALIZE_JANUS                    = 0x41,
		DDI_RESET_TO_RECOVERY                   = 0x42,
		DDI_INIT_DATA_STORE						= 0x43,
		DDI_RESET_TO_UPDATER                    = 0x44,
		DDI_GET_DEVICE_PROPERTIES               = 0x45, // ProtocolUpdater2, ProtocolUpdaterJanus2
		DDI_SET_UPDATE_FLAG						= 0x46,
		DDI_FILTER_PING							= 0x9F
	};

	enum _ROM_REVISION_TYPE {
		TA1,
		TA2,
		TA3,
		TA3A,
		TA4,
		TA5,
		TA6
	};

	enum _LOGICAL_MEDIA_INFO{
		MediaInfoNumberOfDrives = 0,
		MediaInfoSizeInBytes = 1,
		MediaInfoAllocationUnitSizeInBytes = 2,
		MediaInfoIsInitialized = 3,
		MediaInfoMediaState = 4,
		MediaInfoIsWriteProtected = 5,
		MediaInfoPhysicalMediaType = 6,
		MediaInfoSizeOfSerialNumberInBytes = 7,
		MediaInfoSerialNumber = 8,
		MediaInfoIsSystemMedia = 9,
		MediaInfoIsMediaPresent = 10,
		MediaInfoNandPageSizeInBytes = 11,	// ProtocolUpdater2, ProtocolUpdaterJanus2 - 2048 typical value   2Kbyte page size
		MediaInfoNandMfgId = 12,		// ProtocolUpdater2, ProtocolUpdaterJanus2 - Cmd only sent if media type is nand. 1 byte like 0xec for samsung.
		MediaInfoNandIdDetails = 13,	// ProtocolUpdater2, ProtocolUpdaterJanus2 - Cmd only sent if media type is nand.
										//     Id details for nand include remaining 4 of 5 byte nand HW read id hw cmd.
		MediaInfoNandChipEnables = 14   // ProtocolUpdater2, ProtocolUpdaterJanus2 - num CE discovered at driver init time (up to
									    //     build option max supported num CE)
	};

	enum _LOGICAL_DRIVE_INFO{
		DriveInfoSectorSizeInBytes         = 0,
		DriveInfoEraseSizeInBytes          = 1,
		DriveInfoSizeInBytes               = 2,
		DriveInfoSizeInMegaBytes           = 3,
		DriveInfoSizeInSectors             = 4,
		DriveInfoType                      = 5,
		DriveInfoTag                       = 6,
		DriveInfoComponentVersion          = 7,
		DriveInfoProjectVersion            = 8,
		DriveInfoIsWriteProtected          = 9,
		DriveInfoSizeOfSerialNumberInBytes = 10,
		DriveInfoSerialNumber              = 11,
		DriveInfoMediaPresent              = 12,
		DriveInfoMediaChange               = 13
	};

	enum _PROTOCOL_VERSIONS{
        ProtocolUpdater         = 1,  // full featured updater (pre SDK3.0)
        ProtocolLimetedHostlink = 2,  // limited hostlink capabilities(not all vendor cmds supported; cannot update f/w
	    ProtocolUpdater2        = 3,  // normal updater without advanced scsi functions
        ProtocolUpdaterJanus    = 4,  // full featured updater with Janus support
        ProtocolUpdaterJanus2   = 5,  // add advanced scsi functions
		ProtocolUpdaterJanus3	= 6	  // SDK5 Janus init changes
    };

#define ST_SCSI_DEFAULT_TIMEOUT		    30		// seven seconds
#define ST_SCSI_EXTENDED_TIMEOUT		10*60	// 10 minutes
#define ST_SCSI_ERASE_MEDIA_TIMEOUT		5*60	// 5 minutes


#pragma pack(1)
	//
	// Sigmatel SCSI Command Descriptor Block.
	//
	union _ST_SCSI_CDB {

		// API_TYPE_ST_SCSI
		// Standard DDI 16-Byte CDB
		//
		struct _CDB16 {
		UCHAR OperationCode;			// 0xC0, 0xC1 - ST_SCSIOP_READ, ST_SCSIOP_WRITE
		UCHAR Command;
		UCHAR SubCommand;				// OPTIONAL
		UCHAR Reserved[13];
		};

		// API_TYPE_ST_SCSI
		// Get/SetLogicalDriveInfo DDI 16-Byte CDB
		//
		struct _CDBDRIVEINFO16 {
		UCHAR OperationCode;			// 0xC0, 0xC1 - ST_SCSIOP_READ, ST_SCSIOP_WRITE
		UCHAR Command;				// DDI_GET_LOGICAL_DRIVE_INFO, DDI_SET_LOGICAL_DRIVE_INFO
		UCHAR DriveNumber;
		UCHAR TypeInfo;
			union
			{
				struct {
				UCHAR Tag;				// DDI_SET_LOGICAL_DRIVE_INFO: Tag
				UCHAR Reserved11[11];
				};

				struct {
				UCHAR Major[2];				// DDI_SET_LOGICAL_DRIVE_INFO: Project/Component Version Major
				UCHAR Minor[2];				// DDI_SET_LOGICAL_DRIVE_INFO: Project/Component Version Minor
				UCHAR Revision[2];			// DDI_SET_LOGICAL_DRIVE_INFO: Project/Component Version Revision
				UCHAR Reserved6[6];
				};
			};
		};

		// API_TYPE_ST_SCSI
		// Read/WriteLogicalDriveSector DDI 16-Byte CDB
		//
		struct _CDBRWDRIVESECTOR16 {
			UCHAR OperationCode;		// 0xC0, 0xC1 - ST_SCSIOP_READ, ST_SCSIOP_WRITE
			UCHAR Command;			// DDI_READ_LOGICAL_DRIVE_SECTOR, DDI_WRITE_LOGICAL_DRIVE_SECTOR, DDI_READ_PHYSICAL_MEDIA_SECTOR
			UCHAR DriveNumber;
			ULONGLONG Start;
			UINT Count;
			UCHAR Reserved;
		};

		// API_TYPE_UTP
		// 
		//
		struct _CDBUTP16 {
			UCHAR OperationCode;		// 0xF0 - ST_SCSIOP_UTP
			UCHAR Command;			// 
			UINT Tag;
			__int64 LParam;
			UCHAR Reserved[2];
		};

	};

	struct _ALLOCATE_MEDIA_CMD_ENTRY
	{
		UCHAR  type;
		UCHAR  tag;
		ULONGLONG size;
	};

#pragma pack()

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND(BASE CLASS): StApi
	//
	//////////////////////////////////////////////////////////////////////
	class StApi
	{
	public:
		typedef std::vector<StApi*> StApiList;
		StApi(UCHAR const type, UCHAR const write, CString const name)
			: _tag(0)
			, _write(write)
			, _xferLength(0)
            , _timeout(ST_SCSI_DEFAULT_TIMEOUT) // default 7 seconds
			, _type(type)
			, _apiName(name)
			, _sendDataPtr(NULL)
			, _responseStr(_T(""))
			, _responseDataPtr(NULL)
		{};
        virtual ~StApi(); 
		const UCHAR IsWriteCmd() const { return _write; };
		const UINT GetTransferSize() const { return _xferLength; };
		const UINT GetTimeout() const { return _timeout; };
		const UINT GetTag() const { return _tag; };
		void SetTag( const UINT tag ) { _tag = tag; };
		const UCHAR * const GetCmdDataPtr() const { return _sendDataPtr; };
        // Returns the address of the response data.
        const UCHAR * const GetDataPtr() { return _responseDataPtr; };
		virtual	const UCHAR * const GetCdbPtr() const = 0;
		virtual	const UINT GetCdbSize() const = 0;
		virtual	void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
        {
			assert( count >= _xferLength );

			if ( _responseDataPtr != NULL )
			{
				free(_responseDataPtr);
				_responseDataPtr = NULL;
			}

			_responseDataPtr = (UCHAR*)malloc(_xferLength);
			if ( _responseDataPtr != NULL )
            {
				memcpy(_responseDataPtr, pData, _xferLength);
       			FormatReadResponse(_responseDataPtr, 16);
            }
		};

        virtual	void PrepareCommand() {};
		
		// Used mainly for UI support. Most applications
		// should provide all necessary Parameters and/or Data
		// at construction time.
		const LPCTSTR GetName() const { return (LPCTSTR)_apiName; };
		const UCHAR GetType() const { return _type; };
		const bool HasParameters() const { return _params.size() > 0; };
		Parameter::ParamMap& GetParameters() { return _params; };
		virtual void ParseCdb() {};
		virtual int SetCommandData(const UCHAR * const pData, const size_t size);
		virtual const CString& ResponseString() {return _responseStr;};

		// Helper functions for Big-Endian CDBs
		static USHORT const Swap2(const UCHAR *const p)
		{
			return (p[0]<<8) + p[1];
		}
		static UINT const Swap4(const UCHAR *const p)
		{
			return (p[0]<<24) + (p[1]<<16) + (p[2]<<8) + p[3];
		}
		static ULONGLONG const Swap8(const UCHAR *const p)
		{
            UCHAR bytes[8];
            bytes[0] = p[7];
            bytes[1] = p[6];
            bytes[2] = p[5];
            bytes[3] = p[4];
            bytes[4] = p[3];
            bytes[5] = p[2];
            bytes[6] = p[1];
            bytes[7] = p[0];
            return *(ULONGLONG*)bytes;
		}

		// TODO: this could be cleaned up and made more robust
/*		bool ParseParameterStr(CString& paramStr, Parameter& param)
		{
			int colonPos, endPos;
			colonPos = paramStr.Find(L':');
			if ( (endPos = paramStr.FindOneOf(L",\r\n\0")) == -1 )
				endPos = paramStr.GetLength();
			CString valueStr = paramStr.Mid(colonPos+1, endPos-(colonPos+1));
			// prepare string for next call
			paramStr = paramStr.Right(paramStr.GetLength()-(endPos+1));
			// put value into parameter
			param.Parse(valueStr);
			return 0;
		}
*/
		// Creates a string representing space separated data bytes (*pByte)
		// with a 4-digit hex address per line. Number of bytes displayed 
		// per line is specified by width. Data word sizes is specified by wordSize.
		void FormatReadResponse(const UCHAR* const pByte, const UCHAR width, const UCHAR wordSize = 1)
		{
			if (_xferLength == 0 )
				return;

			_responseStr = _T("0000: ");

			for ( size_t i=0; i<_xferLength; i+=wordSize )
			{
				switch ( wordSize )
				{
				case 4:
				{
					UINT FourByteWord = Swap4(&pByte[i]);
					_responseStr.AppendFormat(_T(" %08X"), FourByteWord);
					break;
				}
				case 1:
				default:
					_responseStr.AppendFormat(_T(" %02X"), pByte[i]);
					break;
				}
				if ( ((i+wordSize)%width == 0) &&
					 ((i+wordSize) < _xferLength) ) {
					_responseStr.AppendFormat(_T("\r\n%04X: "), i+wordSize);
				}
			}
		}
		
		class SenseKey
		{
		public:
			SenseKey(UCHAR key)
			{
				switch (key)
				{
					case SCSI_SENSE_NO_SENSE:
						_str = _T("NO_SENSE");
						break;
					case SCSI_SENSE_RECOVERED_ERROR:
						_str = _T("RECOVERED_ERROR");
						break;
					case SCSI_SENSE_NOT_READY:
						_str = _T("NOT_READY");
						break;
					case SCSI_SENSE_MEDIUM_ERROR:
						_str = _T("MEDIUM_ERROR");
						break;
					case SCSI_SENSE_HARDWARE_ERROR:
						_str = _T("HARDWARE_ERROR");
						break;
					case SCSI_SENSE_ILLEGAL_REQUEST:
						_str = _T("ILLEGAL_REQUEST");
						break;
					case SCSI_SENSE_UNIT_ATTENTION:
						_str = _T("UNIT_ATTENTION");
						break;
					case SCSI_SENSE_DATA_PROTECT:
						_str = _T("DATA_PROTECT");
						break;
					case SCSI_SENSE_BLANK_CHECK:
						_str = _T("BLANK_CHECK");
						break;
					case SCSI_SENSE_UNIQUE:
						_str = _T("VENDOR_SPECIFIC");
						break;
					case SCSI_SENSE_COPY_ABORTED:
						_str = _T("COPY_ABORTED");
						break;
					case SCSI_SENSE_ABORTED_COMMAND:
						_str = _T("ABORTED_COMMAND");
						break;
					case SCSI_SENSE_EQUAL:
						_str = _T("EQUAL");
						break;
					case SCSI_SENSE_VOL_OVERFLOW:
						_str = _T("OVERFLOW");
						break;
					case SCSI_SENSE_MISCOMPARE:
						_str = _T("MISCOMPARE");
						break;
					case SCSI_SENSE_RESERVED:
						_str = _T("RESERVED");
						break;
					default:
						_str = _T("UNKNOWN");
				}
				_str.AppendFormat(_T("(%02X)"), key);
			}
			CString& ToString() { return _str; };
		private:
			CString _str;
		};

		virtual CString GetSendCommandErrorStr()
		{
			CString msg;
			switch ( ScsiSenseStatus )
			{
			case SCSISTAT_GOOD:
				msg.Format(_T("SCSI Status: GOOD(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			case SCSISTAT_CHECK_CONDITION:
				msg.Format(_T("SCSI Status: CHECK_CONDITION(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			case SCSISTAT_CONDITION_MET:
				msg.Format(_T("SCSI Status: CONDITION_MET(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			case SCSISTAT_BUSY:
				msg.Format(_T("SCSI Status: BUSY(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			case SCSISTAT_INTERMEDIATE:
				msg.Format(_T("SCSI Status: INTERMEDIATE(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			case SCSISTAT_INTERMEDIATE_COND_MET:
				msg.Format(_T("SCSI Status: INTERMEDIATE_COND_MET(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			case SCSISTAT_RESERVATION_CONFLICT:
				msg.Format(_T("SCSI Status: RESERVATION_CONFLICT(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			case SCSISTAT_COMMAND_TERMINATED:
				msg.Format(_T("SCSI Status: COMMAND_TERMINATED(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			case SCSISTAT_QUEUE_FULL:
				msg.Format(_T("SCSI Status: QUEUE_FULL(0x%02X)\r\n"), ScsiSenseStatus);
				break;
			default:
				msg.Format(_T("SCSI Status: UNKNOWN(0x%02X)\r\n"), ScsiSenseStatus);
			}

			msg += _T("Sense data:\r\n");
			msg.AppendFormat(_T("   ErrorCode: 0x%02X\r\n"), ScsiSenseData.ErrorCode);
			msg.AppendFormat(_T("   Valid: %s\r\n"), ScsiSenseData.Valid ? _T("true") : _T("false"));
			msg.AppendFormat(_T("   SegmentNumber: 0x%02X\r\n"), ScsiSenseData.SegmentNumber);
			SenseKey key(ScsiSenseData.SenseKey);
			msg.AppendFormat(_T("   SenseKey: %s\r\n"), key.ToString());
			msg.AppendFormat(_T("   IncorrectLength: %s\r\n"), ScsiSenseData.IncorrectLength ? _T("true") : _T("false"));
			msg.AppendFormat(_T("   EndOfMedia: %s\r\n"), ScsiSenseData.EndOfMedia ? _T("true") : _T("false"));
			msg.AppendFormat(_T("   FileMark: %s\r\n"), ScsiSenseData.FileMark ? _T("true") : _T("false"));
			msg.AppendFormat(_T("   Information[0-3]: %02X %02X %02X %02X \r\n"), ScsiSenseData.Information[0], ScsiSenseData.Information[1], ScsiSenseData.Information[2], ScsiSenseData.Information[3]);
			msg.AppendFormat(_T("   AdditionalSenseLength: 0x%02X\r\n"), ScsiSenseData.AdditionalSenseLength);
			msg.AppendFormat(_T("   CommandSpecificInformation[0-3]: %02X %02X %02X %02X\r\n"), ScsiSenseData.CommandSpecificInformation[0], ScsiSenseData.CommandSpecificInformation[1], ScsiSenseData.CommandSpecificInformation[2], ScsiSenseData.CommandSpecificInformation[3]);
			msg.AppendFormat(_T("   AdditionalSenseCode: 0x%02X\r\n"), ScsiSenseData.AdditionalSenseCode);
			msg.AppendFormat(_T("   AdditionalSenseCodeQualifier: 0x%02X\r\n"), ScsiSenseData.AdditionalSenseCodeQualifier);
			msg.AppendFormat(_T("   FieldReplaceableUnitCode: 0x%02X\r\n"), ScsiSenseData.FieldReplaceableUnitCode);
			msg.AppendFormat(_T("   SenseKeySpecific[0-2]: %02X %02X %02X\r\n"), ScsiSenseData.SenseKeySpecific[0], ScsiSenseData.SenseKeySpecific[1], ScsiSenseData.SenseKeySpecific[2]);

			return msg;
		}


		// Use ONLY for creating runtime commands
		// This info should be calculated/set as required for each StApi class definition
		void SetWriteCmd(UCHAR write) { _write = write; }; 
		void SetTransferSize(UINT xferLength) { _xferLength = xferLength; };

	protected:
		// 
		UINT _tag;
		UCHAR _write;
		UINT _xferLength;
        int _timeout;
		// Used mainly for UI support. Most applications
		// should provide all necessary Parameters and/or Data
		// at construction time.
		UCHAR _type;
		CString	_apiName;
		UCHAR * _sendDataPtr;
		CString _responseStr;
		UCHAR* _responseDataPtr;
		Parameter::ParamMap _params;
	public:
		SENSE_DATA ScsiSenseData;
		UCHAR ScsiSenseStatus;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND(TEMPLATED BASE CLASS): StApiT
	//
	//////////////////////////////////////////////////////////////////////
	template <typename T>
	class StApiT : public StApi
	{
	public:
		StApiT(UCHAR const type, UCHAR const write, CString const name)
			: StApi(type, write, name)
		{
			memset(&_cdb, 0, sizeof(_cdb));
		};
		virtual ~StApiT()	{};

		const UCHAR * const GetCdbPtr() const { return (UCHAR*)&_cdb; };
		const UINT GetCdbSize() const { return sizeof(_cdb); };

	protected:
		T _cdb;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetProtocolVersion
	//
	//////////////////////////////////////////////////////////////////////
	class StGetProtocolVersion : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetProtocolVersion() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetProtocolVersion"))
			, _major(0)
			, _minor(0)
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_PROTOCOL_VERSION;
			_xferLength = sizeof(_major) + sizeof(_minor);
		};

		virtual ~StGetProtocolVersion() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			_major = pData[0];
			_minor = pData[1];
//			_responseStr.Format(_T("major: 0x%02X\r\nminor: 0x%02X"), GetMajorVersion(), GetMinorVersion());
			switch (_major)
			{
				case ProtocolUpdater:
					_responseStr.Format(_T("ProtocolUpdater (%d.%d):\r\n    Full-featured updater (pre SDK3.0).\r\n    Does not include Janus support."), GetMajorVersion(), GetMinorVersion());
					break;
				case ProtocolLimetedHostlink:
					_responseStr.Format(_T("ProtocolLimetedHostlink (%d.%d):\r\n    Limited hostlink capabilities.\r\n    Not all vendor cmds supported.\r\n    Can not update f/w."), GetMajorVersion(), GetMinorVersion());
					break;
				case ProtocolUpdater2:
					_responseStr.Format(_T("ProtocolUpdater2 (%d.%d):\r\n    Extends ProtocolUpdater to include DDI_GET_DEVICE_PROPERTIES.\r\n    Does not include Janus support."), GetMajorVersion(), GetMinorVersion());
					break;						   
				case ProtocolUpdaterJanus:
					_responseStr.Format(_T("ProtocolUpdaterJanus (%d.%d):\r\n    Extends ProtocolUpdater to include Janus support."), GetMajorVersion(), GetMinorVersion());
					break;
				case ProtocolUpdaterJanus2:
					_responseStr.Format(_T("ProtocolUpdaterJanus2 (%d.%d):\r\n    Extends ProtocolUpdaterJanus to include DDI_GET_DEVICE_PROPERTIES."), GetMajorVersion(), GetMinorVersion());
					break;
				default:
					_responseStr.Format(_T("Unknown Protocol (%d.%d):"), GetMajorVersion(), GetMinorVersion());
					break;
			}
			// list all the protocols
/*			_responseStr += _T("\r\n\r\nProtocolUpdaterJanus2 (5.x):\r\n    Extends ProtocolUpdaterJanus to include DDI_GET_DEVICE_PROPERTIES.");
			_responseStr += _T("\r\nProtocolUpdaterJanus (4.x):\r\n    Extends ProtocolUpdater to include Janus support.");
			_responseStr += _T("\r\nProtocolUpdater2 (3.x):\r\n    Extends ProtocolUpdater to include DDI_GET_DEVICE_PROPERTIES.\r\n    Does not include Janus support.");
			_responseStr += _T("\r\nProtocolLimetedHostlink (2.x):\r\n    Limited hostlink capabilities.\r\n    Not all vendor cmds supported.\r\n    Can not update f/w.");
			_responseStr += _T("\r\nProtocolUpdater (1.x):\r\n    Full-featured updater (pre SDK3.0).\r\n    Does not include Janus support.");
*/		};

		UCHAR GetMajorVersion() { return _major; };
		UCHAR GetMinorVersion() { return _minor; };

	private:
		UCHAR _major;
		UCHAR _minor;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetChipMajorRevId
	//
	//////////////////////////////////////////////////////////////////////
	class StGetChipMajorRevId : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetChipMajorRevId() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetChipMajorRevId"))
			, _chipId(0)
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_CHIP_MAJOR_REV_ID;
			_xferLength = sizeof(_chipId);
		};

		virtual ~StGetChipMajorRevId() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			_chipId = Swap2(pData);
			_responseStr.Format(_T("chipId: 0x%04X"), GetChipMajorRevId());	
		};

		USHORT GetChipMajorRevId() { return _chipId; };

	private:
		USHORT _chipId;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetChipPartRevId
	//
	//////////////////////////////////////////////////////////////////////
	class StGetChipPartRevId : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetChipPartRevId() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetChipPartRevId"))
			, _chipRev(0)
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_CHIP_PART_REV_ID;
			_xferLength = sizeof(_chipRev);
		};

		virtual ~StGetChipPartRevId() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			_chipRev = Swap2(pData);;
			_responseStr.Format(_T("chipRev: 0x%04X"), GetChipPartRevId());	
		};

		USHORT GetChipPartRevId() { return _chipRev; };

	private:
		USHORT _chipRev;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetROMRevId
	//
	//////////////////////////////////////////////////////////////////////
	class StGetROMRevId : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetROMRevId() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetROMRevId"))
			, _romRev(0)
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_ROM_REV_ID;
			_xferLength = sizeof(_romRev);
		};

		virtual ~StGetROMRevId() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			_romRev = Swap2(pData);
			_responseStr.Format(_T("romRev: 0x%04X"), GetROMRevId());	
		};

		USHORT GetROMRevId() { return _romRev; };

	private:
		USHORT _romRev;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetStatus
	//
	//////////////////////////////////////////////////////////////////////
	class StGetStatus : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetStatus() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetStatus"))
			, _status(0)
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_STATUS;
			_xferLength = sizeof(_status);
		};

		virtual ~StGetStatus() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			_status = Swap2(pData);
			_responseStr.Format(_T("status: 0x%04X"), GetStatus());	
		};

		USHORT GetStatus() { return _status; };

	private:
		USHORT _status;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetLogicalMediaInfo
	//
	//////////////////////////////////////////////////////////////////////
	class StGetLogicalMediaInfo : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetLogicalMediaInfo(UCHAR typeInfo, UINT responseSize=0)
			: StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetLogicalMediaInfo"))
		{
			_params[L"Info Type"] = &_typeInfo;
			_params[L"Response Size"] = &_responseSize;

			_typeInfo.Value = typeInfo;
			_responseSize.Value = responseSize;

			PrepareCommand();
			InitInfoTypes();
		};

		void InitInfoTypes()
		{
			_typeInfo.ValueList[MediaInfoNumberOfDrives]			= L"NumberOfDrives";
			_typeInfo.ValueList[MediaInfoSizeInBytes]				= L"SizeInBytes";
			_typeInfo.ValueList[MediaInfoAllocationUnitSizeInBytes] = L"AllocationUnitSizeInBytes";
			_typeInfo.ValueList[MediaInfoIsInitialized]				= L"IsInitialized";
			_typeInfo.ValueList[MediaInfoMediaState]				= L"MediaState";
			_typeInfo.ValueList[MediaInfoIsWriteProtected]			= L"IsWriteProtected";
			_typeInfo.ValueList[MediaInfoPhysicalMediaType]			= L"PhysicalMediaType";
			_typeInfo.ValueList[MediaInfoSizeOfSerialNumberInBytes] = L"SizeOfSerialNumberInBytes";
			_typeInfo.ValueList[MediaInfoSerialNumber]				= L"SerialNumber";
			_typeInfo.ValueList[MediaInfoIsSystemMedia]				= L"IsSystemMedia";
			_typeInfo.ValueList[MediaInfoIsMediaPresent]			= L"IsMediaPresent";
			_typeInfo.ValueList[MediaInfoNandPageSizeInBytes]		= L"NandPageSizeInBytes";
			_typeInfo.ValueList[MediaInfoNandMfgId]					= L"NandMfgId";
			_typeInfo.ValueList[MediaInfoNandIdDetails]				= L"NandIdDetails";
			_typeInfo.ValueList[MediaInfoNandChipEnables]			= L"NandChipEnables";
		}

		virtual ~StGetLogicalMediaInfo() {};

		void ParseCdb()
		{
			_typeInfo.Value = _cdb.SubCommand;
			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_LOGICAL_MEDIA_INFO;
			_cdb.SubCommand = _typeInfo.Value;

			 
			switch(_typeInfo.Value)
			{
				case MediaInfoNumberOfDrives:
				case MediaInfoMediaState:
				case MediaInfoSizeOfSerialNumberInBytes:
				case MediaInfoPhysicalMediaType:
				case MediaInfoAllocationUnitSizeInBytes:
				case MediaInfoNandPageSizeInBytes:
				case MediaInfoNandMfgId:
				case MediaInfoNandChipEnables:
					_xferLength = sizeof(UINT);
					break;
				case MediaInfoIsInitialized:
				case MediaInfoIsWriteProtected:
				case MediaInfoIsSystemMedia:
				case MediaInfoIsMediaPresent:
					_xferLength = sizeof(UCHAR);
					break;
				case MediaInfoSizeInBytes:
				case MediaInfoNandIdDetails:
					_xferLength = sizeof(ULONGLONG);
					break;
				case MediaInfoSerialNumber:
					_xferLength = _responseSize.Value;
					break;
				default:
					_xferLength = 0;
			}
		};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			memset(_responseBuffer, 0, sizeof(_responseBuffer));

			switch(_cdb.SubCommand)
			{
				case MediaInfoNumberOfDrives:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("numDrives: %d"), GetNumberOfDrives());
					break;
				}
				case MediaInfoSizeInBytes:
				{
					*(ULONGLONG*)_responseBuffer = Swap8(pData);
					_responseStr.Format(_T("Media size: 0x%X bytes"), GetSizeInBytes());
					break;
				}
				case MediaInfoAllocationUnitSizeInBytes:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("Allocation Unit size: 0x%X bytes"), GetAllocationUnitSizeInBytes());
					break;
				}
				case MediaInfoIsInitialized:
				{
					*_responseBuffer = *(UCHAR*)pData;
					_responseStr.Format(_T("Is Initialized: %d"), IsInitialized());
					break;
				}
				case MediaInfoMediaState:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("Media state: 0x%X"), GetMediaState());
					break;
				}
				case MediaInfoIsWriteProtected:
				{
					*_responseBuffer = *(UCHAR*)pData;
					_responseStr.Format(_T("Write protected: %d"), IsWriteProtected());
					break;
				}
				case MediaInfoPhysicalMediaType:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("Physical media type: %d"), GetPhysicalMediaType());
					break;
				}
				case MediaInfoSizeOfSerialNumberInBytes:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("SN size: 0x%X bytes"), GetSizeOfSerialNumber());
					break;
				}
				case MediaInfoSerialNumber:
				{
					_responseStr = _T("Media SN: ");

					UCHAR *pByte = (UCHAR*)pData;
					std::vector<UCHAR> bytes;
					for ( UINT i=0; i<_xferLength; ++i )
						bytes.push_back(pByte[i]);
					for ( UINT i=0; i<_xferLength; ++i )
					{
						_responseBuffer[i] = bytes.back();
						bytes.pop_back();
						_responseStr.AppendFormat(_T("%02X"), _responseBuffer[i]);
					}
					break;
				}
				case MediaInfoIsSystemMedia:
				{
					*_responseBuffer = *(UCHAR*)pData;
					_responseStr.Format(_T("Is System media: %d"), IsSystemMedia());
					break;
				}
				case MediaInfoIsMediaPresent:
				{
					*_responseBuffer = *(UCHAR*)pData;
					_responseStr.Format(_T("Is media present: %d"), IsMediaPresent());
					break;
				}
				case MediaInfoNandPageSizeInBytes: // todo
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("MediaInfoNandPageSizeInBytes: %d"), *(UINT*)_responseBuffer);
					break;
				}
				case MediaInfoNandMfgId: // todo
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("MediaInfoNandMfgId: 0x%02X"), *(UINT*)_responseBuffer);
					break;
				}
				case MediaInfoNandIdDetails: // todo
				{
					*(ULONGLONG*)_responseBuffer = Swap8(pData);
					_responseStr.Format(_T("MediaInfoNandIdDetails: 0x%016X"), *(ULONGLONG*)_responseBuffer);
					break;
				}
				case MediaInfoNandChipEnables: // todo
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("MediaInfoNandChipEnables: %d"), *(UINT*)_responseBuffer);
					break;
				}
				default:
					_responseStr = _T("Invalid GetLogicalMediaInfo sub-command.");
			}
		};

		USHORT GetNumberOfDrives()
		{
			if( _cdb.SubCommand != MediaInfoNumberOfDrives )
				return -1;

			return *(USHORT*)_responseBuffer;
		};

		ULONGLONG GetSizeInBytes()
		{
			if( _cdb.SubCommand != MediaInfoSizeInBytes )
				return 0;

			return *(ULONGLONG*)_responseBuffer;
		};

		UINT GetAllocationUnitSizeInBytes()
		{
			if( _cdb.SubCommand != MediaInfoAllocationUnitSizeInBytes )
				return 0;

			return *(UINT*)_responseBuffer;
		};

		bool IsInitialized()
		{
			if( _cdb.SubCommand != MediaInfoIsInitialized )
				return (bool)-1;

			return *(bool*)_responseBuffer;
		};

		UINT GetMediaState()
		{
			if( _cdb.SubCommand != MediaInfoMediaState )
				return 0;

			return *(UINT*)_responseBuffer;
		};

		bool IsWriteProtected()
		{
			if( _cdb.SubCommand != MediaInfoIsWriteProtected )
				return (bool)-1;

			return *(bool*)_responseBuffer;
		};

        media::PhysicalMediaType GetPhysicalMediaType()
		{
			if( _cdb.SubCommand != MediaInfoPhysicalMediaType )
                return media::MediaType_Invalid;
		
			return *(media::PhysicalMediaType*)_responseBuffer;
		};

		USHORT GetSizeOfSerialNumber()
		{
			if( _cdb.SubCommand != MediaInfoSizeOfSerialNumberInBytes )
				return 0;

			return *(USHORT*)_responseBuffer;
		};
		
		size_t GetSerialNumber(UCHAR* serialNumber)
		{
			if( _cdb.SubCommand != MediaInfoSerialNumber )
				return 0;

			memcpy(serialNumber, _responseBuffer, _xferLength);
			return _xferLength;
		};

		bool IsSystemMedia()
		{
			if( _cdb.SubCommand != MediaInfoIsSystemMedia )
				return (bool)-1;

			return *(bool*)_responseBuffer;
		};

		bool IsMediaPresent()
		{
			if( _cdb.SubCommand != MediaInfoIsMediaPresent )
				return (bool)-1;

			return *(bool*)_responseBuffer;
		};

	private:
		ParameterT<UCHAR> _typeInfo;
		ParameterT<UINT> _responseSize;
		UCHAR _responseBuffer[256];
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetLogicalDriveInfo
	//
	//////////////////////////////////////////////////////////////////////
	class StGetLogicalDriveInfo : public StApiT<_ST_SCSI_CDB::_CDBDRIVEINFO16>
	{
	public:
		StGetLogicalDriveInfo(UCHAR driveNumber, _LOGICAL_DRIVE_INFO typeInfo, UINT responseSize=0)
			: StApiT<_ST_SCSI_CDB::_CDBDRIVEINFO16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetLogicalDriveInfo"))
		{
			_params[L"Drive Number"] = &_driveNumber;
			_params[L"Info Type"] = &_typeInfo;
			_params[L"Response Size"] = &_responseSize;

			_driveNumber.Value = driveNumber;
			_typeInfo.Value = typeInfo;
			_responseSize.Value = responseSize;

			PrepareCommand();
			InitInfoTypes();
		};

		void InitInfoTypes()
		{
			_typeInfo.ValueList[DriveInfoSectorSizeInBytes]			= L"SectorSizeInBytes";
			_typeInfo.ValueList[DriveInfoEraseSizeInBytes]			= L"EraseSizeInBytes";
			_typeInfo.ValueList[DriveInfoSizeInBytes]				= L"SizeInBytes";
			_typeInfo.ValueList[DriveInfoSizeInMegaBytes]			= L"SizeInMegaBytes";
			_typeInfo.ValueList[DriveInfoSizeInSectors]				= L"SizeInSectors";
			_typeInfo.ValueList[DriveInfoType]						= L"Type";
			_typeInfo.ValueList[DriveInfoTag]						= L"Tag";
			_typeInfo.ValueList[DriveInfoComponentVersion]			= L"ComponentVersion";
			_typeInfo.ValueList[DriveInfoProjectVersion]			= L"ProjectVersion";
			_typeInfo.ValueList[DriveInfoIsWriteProtected]			= L"IsWriteProtected";
			_typeInfo.ValueList[DriveInfoSizeOfSerialNumberInBytes] = L"SizeOfSerialNumberInBytes";
			_typeInfo.ValueList[DriveInfoSerialNumber]				= L"SerialNumber";
			_typeInfo.ValueList[DriveInfoMediaPresent]				= L"MediaPresent";
			_typeInfo.ValueList[DriveInfoMediaChange]				= L"MediaChange";
		}

		virtual ~StGetLogicalDriveInfo() {};

		void ParseCdb()
		{
			_driveNumber.Value = _cdb.DriveNumber;
			_typeInfo.Value = _cdb.TypeInfo;

			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_LOGICAL_DRIVE_INFO;
			_cdb.DriveNumber = _driveNumber.Value;
			_cdb.TypeInfo = _typeInfo.Value;

			 
			switch(_typeInfo.Value)
			{
				case DriveInfoSectorSizeInBytes:
				case DriveInfoEraseSizeInBytes:
				case DriveInfoType:
				case DriveInfoTag:
				case DriveInfoSizeOfSerialNumberInBytes:
					_xferLength = sizeof(UINT);
					break;
				case DriveInfoSizeInMegaBytes:
				case DriveInfoSizeInBytes:
				case DriveInfoSizeInSectors:
					_xferLength = sizeof(ULONGLONG);
					break;
				case DriveInfoIsWriteProtected:
				case DriveInfoMediaPresent:
				case DriveInfoMediaChange:
					_xferLength = sizeof(UCHAR);
					break;
				case DriveInfoSerialNumber:
					_xferLength = _responseSize.Value;
					break;
				case DriveInfoComponentVersion:
				case DriveInfoProjectVersion:
					_xferLength = 3 * sizeof(USHORT);
					break;
				default:
					_xferLength = 0;
			}
		};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			memset(_responseBuffer, 0, sizeof(_responseBuffer));

			switch(_cdb.TypeInfo)
			{
				case DriveInfoSectorSizeInBytes:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("Sector size: 0x%X bytes"), GetSectorSize());
					break;
				}
				case DriveInfoEraseSizeInBytes:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("Erase size: 0x%X bytes"), GetEraseSizeInBytes());
					break;
				}
				case DriveInfoSizeInBytes:
				{
					*(ULONGLONG*)_responseBuffer = Swap8(pData);
					_responseStr.Format(_T("Drive size: 0x%X bytes"), GetSizeInBytes());
					break;
				}
				case DriveInfoSizeInMegaBytes:
				{
					*(ULONGLONG*)_responseBuffer = Swap8(pData);
					_responseStr.Format(_T("Drive size: %d MB"), GetSizeInMegaBytes());
					break;
				}
				case DriveInfoSizeInSectors:
				{
					*(ULONGLONG*)_responseBuffer = Swap8(pData);
					_responseStr.Format(_T("Drive size: 0x%X sectors"), GetSizeInSectors());
					break;
				}
				case DriveInfoType:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("Drive type: %d"), GetType());
					break;
				}
				case DriveInfoTag:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("Drive tag: 0x%02X"), GetTag());
					break;
				}
				case DriveInfoComponentVersion:
				{
					USHORT* pBuf = (USHORT*)_responseBuffer;
					pBuf[0] = Swap2(&pData[0]);
					pBuf[1] = Swap2(&pData[2]);
					pBuf[2] = Swap2(&pData[4]);
					_responseStr.Format(_T("Component version:\r\n major: %d\r\n minor: %d\r\n revision: %d"), pBuf[0], pBuf[1], pBuf[2]);
					break;
				}
				case DriveInfoProjectVersion:
				{
					USHORT* pBuf = (USHORT*)_responseBuffer;
					pBuf[0] = Swap2(&pData[0]);
					pBuf[1] = Swap2(&pData[2]);
					pBuf[2] = Swap2(&pData[4]);
					_responseStr.Format(_T("Project version:\r\n major: %d\r\n minor: %d\r\n revision: %d"), pBuf[0], pBuf[1], pBuf[2]);
					break;
				}
				case DriveInfoIsWriteProtected:
				{
					*_responseBuffer = *(UCHAR*)pData;
					_responseStr.Format(_T("Write protected: %d"), IsWriteProtected());
					break;
				}
				case DriveInfoSizeOfSerialNumberInBytes:
				{
					*(UINT*)_responseBuffer = Swap4(pData);
					_responseStr.Format(_T("SN size: 0x%X bytes"), GetSizeOfSerialNumberInBytes());
					break;
				}
				case DriveInfoSerialNumber:
				{
					_responseStr = _T("SN: ");

					UCHAR *pByte = (UCHAR*)pData;
					std::vector<UCHAR> bytes;
					for ( UINT i=0; i<_xferLength; ++i )
						bytes.push_back(pByte[i]);
					for ( UINT i=0; i<_xferLength; ++i )
					{
						_responseBuffer[i] = bytes.back();
						bytes.pop_back();
						_responseStr.AppendFormat(_T("%02X"), _responseBuffer[i]);
					}
					break;
				}
				case DriveInfoMediaPresent:
				{
					*_responseBuffer = *(UCHAR*)pData;
					_responseStr.Format(_T("Media present: %d"), IsMediaPresent());
					break;
				}
				case DriveInfoMediaChange:
				{
					*_responseBuffer = *(UCHAR*)pData;
					_responseStr.Format(_T("Media changed: %d"), IsMediaChanged());
					break;
				}
				default:
					_responseStr = _T("Invalid GetLogicalDriveInfo sub-command.");
			}
		};

		UINT GetSectorSize()
		{
			if(_cdb.TypeInfo != DriveInfoSectorSizeInBytes)
				return 0;

			return *(UINT*)_responseBuffer;
		};

		UINT GetEraseSizeInBytes()
		{
			if(_cdb.TypeInfo != DriveInfoEraseSizeInBytes)
				return 0;

			return *(UINT*)_responseBuffer;
		};

		ULONGLONG GetSizeInBytes()
		{
			if(_cdb.TypeInfo != DriveInfoSizeInBytes)
				return 0;

			return *(ULONGLONG*)_responseBuffer;
		};

		ULONGLONG GetSizeInMegaBytes()
		{
			if(_cdb.TypeInfo != DriveInfoSizeInMegaBytes)
				return 0;

			return *(ULONGLONG*)_responseBuffer;
		};

		ULONGLONG GetSizeInSectors()
		{
			if(_cdb.TypeInfo != DriveInfoSizeInSectors)
				return 0;

			return *(ULONGLONG*)_responseBuffer;
		};

        media::LogicalDriveType GetType()
		{
			if(_cdb.TypeInfo != DriveInfoType)
                return media::DriveType_Invalid;
		
            return *(media::LogicalDriveType*)_responseBuffer;
		};
		UINT GetTag()
		{
			if(_cdb.TypeInfo != DriveInfoTag)
				return -2;

			return *(UINT*)_responseBuffer;
		};
	    
		int GetComponentVersion(StVersionInfo& version)
		{
			if(_cdb.TypeInfo != DriveInfoComponentVersion)
				return -1;

			version.SetMajor((USHORT)_responseBuffer[0]);
			version.SetMinor((USHORT)_responseBuffer[1]);
			version.SetRevision((USHORT)_responseBuffer[2]);

			return ERROR_SUCCESS;
		};
		
		int GetProjectVersion(StVersionInfo& version)
		{
			if(_cdb.TypeInfo != DriveInfoProjectVersion)
				return -1;

			version.SetMajor((USHORT)_responseBuffer[0]);
			version.SetMinor((USHORT)_responseBuffer[1]);
			version.SetRevision((USHORT)_responseBuffer[2]);

			return ERROR_SUCCESS;
		};
		
		bool IsWriteProtected()
		{
			if(_cdb.TypeInfo != DriveInfoIsWriteProtected)
				return (bool)-1;

			return *(bool*)_responseBuffer;
		};
	    
		UINT GetSizeOfSerialNumberInBytes()
		{
			if(_cdb.TypeInfo != DriveInfoSizeOfSerialNumberInBytes)
				return 0;

			return *(UINT*)_responseBuffer;
		};

		size_t GetSerialNumber(UCHAR* serialNumber)
		{
			if(_cdb.TypeInfo != DriveInfoSerialNumber)
				return 0;

			memcpy(serialNumber, _responseBuffer, _xferLength);
			return _xferLength;
		};

		bool IsMediaPresent()
		{
			if(_cdb.TypeInfo != DriveInfoMediaPresent)
				return (bool)-1;

			return *(bool*)_responseBuffer;
		};

		bool IsMediaChanged()
		{
			if(_cdb.TypeInfo != DriveInfoMediaChange)
				return (bool)-1;

			return *(bool*)_responseBuffer;
		};

	private:
		ParameterT<UCHAR> _driveNumber;
		ParameterT<UCHAR> _typeInfo;
		ParameterT<UINT> _responseSize;
		UCHAR _responseBuffer[256];
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StSetLogicalDriveInfo
	//
	//////////////////////////////////////////////////////////////////////
	class StSetLogicalDriveInfo : public StApiT<_ST_SCSI_CDB::_CDBDRIVEINFO16>
	{
	public:
		StSetLogicalDriveInfo(UCHAR driveNumber, _LOGICAL_DRIVE_INFO typeInfo, UCHAR tag) 
			: StApiT<_ST_SCSI_CDB::_CDBDRIVEINFO16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("SetLogicalDriveInfo.tag"))
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_SET_LOGICAL_DRIVE_INFO;
			_cdb.DriveNumber = driveNumber;
			_cdb.TypeInfo = (UCHAR)typeInfo; // DriveInfoTag
			_cdb.Tag = tag;
		};
		
		StSetLogicalDriveInfo(UCHAR driveNumber, _LOGICAL_DRIVE_INFO typeInfo, StVersionInfo& version) 
			: StApiT<_ST_SCSI_CDB::_CDBDRIVEINFO16>(API_TYPE_ST_SCSI, ST_WRITE_CMD_PLUS_DATA, _T("SetLogicalDriveInfo.ver"))
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_SET_LOGICAL_DRIVE_INFO;
			_cdb.DriveNumber = driveNumber;
			_cdb.TypeInfo = (UCHAR)typeInfo;				// DriveInfoComponentVersion, DriveInfoProjectVersion
			_cdb.Major[0] = HIBYTE(version.GetMajor());
			_cdb.Major[1] = LOBYTE(version.GetMajor());
			_cdb.Minor[0] = HIBYTE(version.GetMinor());
			_cdb.Minor[1] = LOBYTE(version.GetMinor());
			_cdb.Revision[0] = HIBYTE(version.GetRevision());
			_cdb.Revision[1] = LOBYTE(version.GetRevision());
}

		virtual ~StSetLogicalDriveInfo() {};
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetLogicalTable
	//
	//////////////////////////////////////////////////////////////////////
	class StGetLogicalTable : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
        StGetLogicalTable(UCHAR numEntries=media::DefaultMediaTableEntries)
			: StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetLogicalTable"))
		{
			_params[L"Number of Entries"] = &_numEntries;
			_numEntries.Value = numEntries;

			PrepareCommand();
		};
		
		virtual ~StGetLogicalTable() {};

		void ParseCdb()
		{
			_numEntries.Value = _cdb.SubCommand;
			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_ALLOCATION_TABLE;
			_cdb.SubCommand = _numEntries.Value;

            _xferLength = sizeof(USHORT) + (_numEntries.Value * media::MediaAllocationEntry::Size());
		}
		
		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			_driveArray.Clear();

			USHORT  numEntries = Swap2(pData);

			// skip the USHORT numEntries at the beginning of the return data
			UCHAR* pInfo =  (UCHAR*)pData + sizeof(numEntries);
			// get the table entries
			for ( USHORT index=0; index < numEntries; ++index )
			{
                media::LogicalDrive myDrive(_T("NoFileName"));
				myDrive.DriveNumber = pInfo[0];
				myDrive.Type =  (media::LogicalDriveType)pInfo[1];
				myDrive.Tag =  (media::LogicalDriveTag)pInfo[2];
				myDrive.SizeInBytes = Swap8(&pInfo[3]);
				
                _driveArray.AddDrive(myDrive);
                
				pInfo += media::MediaAllocationEntry::Size();
			}
		};
		
        media::LogicalDriveArray GetEntryArray() { return _driveArray; };

		void UpdateDriveArray(media::LogicalDriveArray& updateArray)
		{
			size_t apiDriveIndex;
			for ( apiDriveIndex = 0; apiDriveIndex < _driveArray.Size(); ++apiDriveIndex )
			{
				media::LogicalDrive currentDrive = _driveArray[apiDriveIndex];
				updateArray.GetDrive(currentDrive.Tag).DriveNumber = currentDrive.DriveNumber;
				updateArray.GetDrive(currentDrive.Tag).SizeInBytes = currentDrive.SizeInBytes;
			}
		}
		
		CString& ResponseString()
		{
			_responseStr.Format(_T("numEntries: %d\r\n"), (UINT)_driveArray.Size());

			size_t index;
			for ( index = 0; index < _driveArray.Size(); ++index )
			{
				_responseStr.AppendFormat(_T(" entry: %d\r\n"), index);
				_responseStr.AppendFormat(_T("  Drive number: 0x%02X\r\n"), _driveArray[index].DriveNumber);
				_responseStr.AppendFormat(_T("  Drive tag: 0x%02X\r\n"), _driveArray[index].Tag);
				_responseStr.AppendFormat(_T("  Drive type: 0x%02X\r\n"), _driveArray[index].Type);
				_responseStr.AppendFormat(_T("  Drive size: 0x%X bytes\r\n"), _driveArray[index].SizeInBytes);
			}
			return _responseStr;
		};
		
	private:
		ParameterT<UCHAR> _numEntries;
		media::LogicalDriveArray _driveArray;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StAllocateLogicalMedia 
	//
	// TODO: NOT TESTED
	//
	//////////////////////////////////////////////////////////////////////
	class StAllocateLogicalMedia : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StAllocateLogicalMedia(media::MediaAllocationTable* pEntryArray)
			: StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD_PLUS_DATA, _T("AllocateLogicalMedia"))
		{
			_mediaArray = *pEntryArray;			
			
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_SET_ALLOCATE_TABLE;
			_cdb.SubCommand = (UCHAR)pEntryArray->size();

			_timeout = ST_SCSI_EXTENDED_TIMEOUT; //ST_SCSI_EXTENDED_TIMEOUT;
			PrepareCommand();
		};

		virtual ~StAllocateLogicalMedia() {};

		void PrepareCommand()
		{
			_xferLength = (UINT)_mediaArray.size() * sizeof(_ALLOCATE_MEDIA_CMD_ENTRY);
			
			if ( _sendDataPtr )
			{
				free( _sendDataPtr );
				_sendDataPtr = NULL;
			}
			_sendDataPtr = (UCHAR*)malloc(_xferLength);
			if ( _sendDataPtr == NULL )
				return;
			
			memset(_sendDataPtr, 0, _xferLength);

			_ALLOCATE_MEDIA_CMD_ENTRY* pCmdEntry = (_ALLOCATE_MEDIA_CMD_ENTRY*)_sendDataPtr;
			media::MediaAllocationTable::iterator pEntry;
			for ( pEntry = _mediaArray.begin(); pEntry != _mediaArray.end(); ++pEntry )
			{
				pCmdEntry->type = (*pEntry).Type.Value;
				pCmdEntry->tag  = (*pEntry).Tag.Value;
				pCmdEntry->size = Swap8((UCHAR*)&(*pEntry).SizeInBytes.Value);
				++pCmdEntry;
			}
			return;
		};

	private:
        media::MediaAllocationTable _mediaArray;
	};


	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StEraseLogicalMedia : 
	//
	// TODO: LONG COMMAND
	//
	//////////////////////////////////////////////////////////////////////
	class StEraseLogicalMedia : public api::StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StEraseLogicalMedia(bool preserveJanus = false) : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("EraseLogicalMedia"))
		{
			_params[L"Preserve Janus Drive:"] = &_preserveJanus;

			_preserveJanus.ValueList[false]	= L"false";
			_preserveJanus.ValueList[true]	= L"true";

			_preserveJanus.Value = preserveJanus;
			
			_timeout = ST_SCSI_ERASE_MEDIA_TIMEOUT;

			PrepareCommand();

		};
		
		virtual ~StEraseLogicalMedia() {};

		void ParseCdb()
		{
			_preserveJanus.Value = _cdb.SubCommand;

			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_ERASE_LOGICAL_MEDIA;
			_cdb.SubCommand = _preserveJanus.Value;

			_xferLength = 0;
		}

	private:
		ParameterT<UCHAR> _preserveJanus;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StEraseLogicalMediaAsync : 
	//
	// TODO: LONG COMMAND
	//
	//////////////////////////////////////////////////////////////////////
	class StEraseLogicalMediaAsync : public api::StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StEraseLogicalMediaAsync() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("EraseLogicalMediaAsync"))
		{
			PrepareCommand();
		};
		
		virtual ~StEraseLogicalMediaAsync() {};

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_ERASE_LOGICAL_MEDIA_ASYNC;

			_xferLength = 0;
		}
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StReadLogicalDriveSector
	//
	//////////////////////////////////////////////////////////////////////
	class StReadLogicalDriveSector : public StApiT<_ST_SCSI_CDB::_CDBRWDRIVESECTOR16>
	{
	public:
		StReadLogicalDriveSector(UCHAR driveNumber, UINT sectorSize, ULONGLONG sectorStart, UINT sectorCount)
			: StApiT<_ST_SCSI_CDB::_CDBRWDRIVESECTOR16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("ReadLogicalDriveSector"))
//			, _responseDataPtr(NULL)
		{
			_params[L"Drive Number"] = &_driveNumber;
			_params[L"Starting Sector"] = &_sectorStart;
			_params[L"Sector Count"] = &_sectorCount;
			_params[L"Sector Size"] = &_sectorSize;

			_driveNumber.Value = driveNumber;
			_sectorStart.Value = sectorStart;
			_sectorCount.Value = sectorCount;
			_sectorSize.Value = sectorSize;

			PrepareCommand();
		};
		
		virtual ~StReadLogicalDriveSector() {};

		void ParseCdb()
		{
			_driveNumber.Value = _cdb.DriveNumber;
			_sectorStart.Value = Swap8((UCHAR*)&_cdb.Start);
			_sectorCount.Value = Swap4((UCHAR*)&_cdb.Count);

			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_READ_LOGICAL_DRIVE_SECTOR;
			_cdb.DriveNumber = _driveNumber.Value;
			_cdb.Start = Swap8((UCHAR*)&_sectorStart.Value);
			_cdb.Count = Swap4((UCHAR*)&_sectorCount.Value);

			_xferLength = _sectorCount.Value * _sectorSize.Value;
		}
		
		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			if ( _responseDataPtr != NULL )
			{
				free(_responseDataPtr);
				_responseDataPtr = NULL;
			}

			_responseDataPtr = (UCHAR*)malloc(_xferLength);
			if ( _responseDataPtr != NULL )
				memcpy(_responseDataPtr, pData, _xferLength);
		};

//		UCHAR* GetDataPtr() { return _responseDataPtr; };
		
		CString& ResponseString()
		{
			FormatReadResponse(_responseDataPtr, 16);
			return _responseStr;
		};
	private:
		ParameterT<UCHAR> _driveNumber;
		ParameterT<ULONGLONG> _sectorStart;
		ParameterT<UINT> _sectorCount;
		ParameterT<UINT> _sectorSize;
//		UCHAR* _responseDataPtr;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StWriteLogicalDriveSector
	//
	//////////////////////////////////////////////////////////////////////
	class StWriteLogicalDriveSector : public StApiT<_ST_SCSI_CDB::_CDBRWDRIVESECTOR16>
	{
	public:
		StWriteLogicalDriveSector(UCHAR driveNumber, UINT sectorSize, ULONGLONG sectorStart, 
			UINT sectorCount, UCHAR* pData, bool bWrite512=false)
			: StApiT<_ST_SCSI_CDB::_CDBRWDRIVESECTOR16>(API_TYPE_ST_SCSI, ST_WRITE_CMD_PLUS_DATA, _T("WriteLogicalDriveSector"))
		{
			_params[L"Drive Number"] = &_driveNumber;
			_params[L"Starting Sector"] = &_sectorStart;
			_params[L"Sector Count"] = &_sectorCount;
			_params[L"Sector Size"] = &_sectorSize;
			_params[L"Write as 512"] = &_bWrite512;

			_driveNumber.Value = driveNumber;
			_sectorStart.Value = sectorStart;
			_sectorCount.Value = sectorCount;
			_sectorSize.Value = sectorSize;
			_bWrite512.Value = bWrite512;

			SetCommandData(pData, sectorCount * sectorSize);

            InitWriteTypes();

		};

		void InitWriteTypes()
		{
			_bWrite512.ValueList[true]	= L"true";
			_bWrite512.ValueList[false]	= L"false";
		}

		virtual ~StWriteLogicalDriveSector() {};

		void ParseCdb()
		{
			_driveNumber.Value = _cdb.DriveNumber;
			_sectorStart.Value = Swap8((UCHAR*)&_cdb.Start);
			_sectorCount.Value = Swap4((UCHAR*)&_cdb.Count);

			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			if (_bWrite512.Value == true)
				_cdb.Command = DDI_WRITE_LOGICAL_512_DRIVE_SECTOR;
			else
				_cdb.Command = DDI_WRITE_LOGICAL_DRIVE_SECTOR;
			_cdb.DriveNumber = _driveNumber.Value;
			_cdb.Start = Swap8((UCHAR*)&_sectorStart.Value);
			_cdb.Count = Swap4((UCHAR*)&_sectorCount.Value);

			if ( _xferLength < _sectorCount.Value * _sectorSize.Value )
			{
				UCHAR* tempBuf = (UCHAR*)malloc(_sectorCount.Value * _sectorSize.Value);
				memset(tempBuf, 0xFF, _sectorCount.Value * _sectorSize.Value);
				memcpy(tempBuf, _sendDataPtr, _xferLength);

				SetCommandData(tempBuf, _sectorCount.Value * _sectorSize.Value);
				free (tempBuf);
			}
			
		}

    private:
		ParameterT<UCHAR> _driveNumber;
		ParameterT<ULONGLONG> _sectorStart;
		ParameterT<UINT> _sectorCount;
		ParameterT<UINT> _sectorSize;
		ParameterT<bool> _bWrite512;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StEraseLogicalDrive
	//
	//////////////////////////////////////////////////////////////////////
	class StEraseLogicalDrive : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StEraseLogicalDrive(UCHAR driveNumber)
			: StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("EraseLogicalDrive"))
		{
			_params[L"Drive Number"] = &_driveNumber;
			_driveNumber.Value = driveNumber;

			PrepareCommand();
		};
		
		virtual ~StEraseLogicalDrive() {};

		void ParseCdb()
		{
			_driveNumber.Value = _cdb.SubCommand;
			PrepareCommand();
		};

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_ERASE_LOGICAL_DRIVE;
			_cdb.SubCommand = _driveNumber.Value;
		};
		
	private:
		ParameterT<UCHAR> _driveNumber;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StFilterPing
	//
	//////////////////////////////////////////////////////////////////////
	class StFilterPing : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StFilterPing() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("FilterPing"))
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_FILTER_PING;
			_xferLength = sizeof(_status);
		};

		virtual ~StFilterPing() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			memcpy(&_status, pData, sizeof(_status));
		};

		USHORT GetStatus() { return _status; };

	private:
		USHORT _status;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StChipReset
	//
	//////////////////////////////////////////////////////////////////////
	class StChipReset : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StChipReset() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("ChipReset"))
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_CHIP_RESET;
		};
		
		virtual ~StChipReset() {};
	};

    //////////////////////////////////////////////////////////////////////
    //
    // DDI_COMMAND: StGetChipSerialNumberInfo
    //
    //////////////////////////////////////////////////////////////////////
	enum _SERIAL_NO_INFO{
		SerialNoInfoSizeOfSerialNumberInBytes = 0,
		SerialNoInfoSerialNumber = 1
	};

	class StGetChipSerialNumberInfo : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetChipSerialNumberInfo(_SERIAL_NO_INFO typeInfo, USHORT serialNoSize = 16)
			: StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetChipSerialNumberInfo"))
		{
			_params[L"Info Type"] = &_typeInfo;
			_params[L"Serial Number Size"] = &_sizeOfSerialNoInBytes;

			_typeInfo.Value = typeInfo;
			_sizeOfSerialNoInBytes.Value = serialNoSize;

			InitInfoTypes();

			PrepareCommand();
		};

		void InitInfoTypes()
		{
			_typeInfo.ValueList[SerialNoInfoSizeOfSerialNumberInBytes]	= L"SizeOfSerialNumberInBytes";
			_typeInfo.ValueList[SerialNoInfoSerialNumber]				= L"SerialNumber";
		};

		virtual ~StGetChipSerialNumberInfo() {};

		void ParseCdb()
		{
			_typeInfo.Value = _cdb.SubCommand;

			PrepareCommand();
		};

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_CHIP_SERIAL_NUMBER_INFO;
			_cdb.SubCommand = _typeInfo.Value;

			switch(_typeInfo.Value)
			{
				case SerialNoInfoSizeOfSerialNumberInBytes:
					_xferLength = sizeof(USHORT);
					break;
				case SerialNoInfoSerialNumber:
					_xferLength = _sizeOfSerialNoInBytes.Value;
					break;
				default:
					_xferLength = 0;
			}
		};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			switch(_cdb.SubCommand)
			{
				case SerialNoInfoSizeOfSerialNumberInBytes:
				{
					_sizeOfSerialNoInBytes = *(USHORT*)pData;
					_responseStr.Format(_T("SN size: 0x%04X (%d) bytes"), GetSizeOfSerialNoInBytes(), GetSizeOfSerialNoInBytes());
					break;
				}
				case SerialNoInfoSerialNumber:
				{
					_responseStr = _T("SN: 0x");

					UINT index;
					for ( index=0; index < count; ++index )
					{
						_serialNumber.push_back(pData[index]);
						_responseStr.AppendFormat(_T("%02X"), _serialNumber.back());
					}
					break;
				}
				default:
					_responseStr = _T("Invalid StGetChipSerialNumberInfo sub-command.");
			}
		};

		USHORT GetSizeOfSerialNoInBytes()
		{
			return _sizeOfSerialNoInBytes.Value;
		};

		std::vector<UCHAR> GetSerialNumber()
		{
			if(_cdb.SubCommand != SerialNoInfoSerialNumber)
			{
				_serialNumber.clear();
			}

			return _serialNumber;
		};

	private:
		ParameterT<UCHAR> _typeInfo;
		ParameterT<USHORT> _sizeOfSerialNoInBytes;
		std::vector<UCHAR> _serialNumber;
	};

    //////////////////////////////////////////////////////////////////////
    //
    // DDI_COMMAND: StGetPhysicalMediaInfo
    //
    //////////////////////////////////////////////////////////////////////
//    typedef UCHAR P_STMP_WORD[3];
//    #define STMP_WORD(x) ((UINT)( x[2]<<16 | x[1]<<8 | x[0] ))
//    typedef enum info_index { iNumNANDChips = 0, wChipNumber, wTotalSectors, wTotalPages, wTotalBlocks, wTotalInternalDice, wBlocksPerDie, wTotalZones };
    typedef struct {
        UINT iNumNANDChips;
        UINT wChipNumber;
        UINT wTotalSectors;
        UINT wTotalPages;
        UINT wTotalBlocks;
        UINT wTotalInternalDice;       // (1/2/4/...) - number of chips pretending to be a single chip
        UINT wBlocksPerDie;            // (wTotalBlocks / wTotalInternalDice )   
        UINT wTotalZones;
    } PHYSICAL_MEDIA_INFO, * P_PHYSICAL_MEDIA_INFO;

    class StGetPhysicalMediaInfo : public StApiT<_ST_SCSI_CDB::_CDB16>
    {
    public:
	    StGetPhysicalMediaInfo() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetPhysicalMediaInfo"))
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_PHYSICAL_MEDIA_INFO;

   			_xferLength = sizeof(_info);
		};

        virtual ~StGetPhysicalMediaInfo() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			memcpy(&_info, pData, sizeof(_info));
		};

 		CString& ResponseString()
		{
            _responseStr.AppendFormat(_T("NAND Count:\t%d\r\n"), GetNandCount());
            _responseStr.AppendFormat(_T("Chip Number:\t%d\r\n"), GetChipNumber());
            _responseStr.AppendFormat(_T("Total Sectors:\t%d\r\n"), GetTotalSectors());
            _responseStr.AppendFormat(_T("Total Pages:\t%d\r\n"), GetTotalPages());
            _responseStr.AppendFormat(_T("Total Blocks:\t%d\r\n"), GetTotalBlocks());
            _responseStr.AppendFormat(_T("Total Internal Die:\t%d\r\n"), GetTotalInternalDie());
            _responseStr.AppendFormat(_T("Blocks per Die:\t%d\r\n"), GetBlocksPerDie());
            _responseStr.AppendFormat(_T("Total Zones:\t%d\r\n"), GetTotalZones());
			return _responseStr;
		};

        UINT GetNandCount()			{ return _info.iNumNANDChips; };
        UINT GetChipNumber()		{ return _info.wChipNumber; };
        UINT GetTotalSectors()		{ return _info.wTotalSectors; };
        UINT GetTotalPages()		{ return _info.wTotalPages; };
        UINT GetTotalBlocks()		{ return _info.wTotalBlocks; };
        UINT GetTotalInternalDie()	{ return _info.wTotalInternalDice; };
        UINT GetBlocksPerDie()		{ return _info.wBlocksPerDie; };
        UINT GetTotalZones()		{ return _info.wTotalZones; };
    private:
        PHYSICAL_MEDIA_INFO _info;
    };

    //////////////////////////////////////////////////////////////////////
    //
    // DDI_COMMAND: StReadPhysicalMediaSector
    //
    //////////////////////////////////////////////////////////////////////
    class StReadPhysicalMediaSector : public StApiT<_ST_SCSI_CDB::_CDBRWDRIVESECTOR16>
    {
    public:

	    StReadPhysicalMediaSector(UCHAR chipNumber, UINT sectorSize, ULONGLONG sectorStart, UINT sectorCount)
			: StApiT<_ST_SCSI_CDB::_CDBRWDRIVESECTOR16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("ReadPhysicalMediaSector"))
//			, _responseDataPtr(NULL)
		{
			_params[L"Chip Number"] = &_chipNumber;
			_params[L"Starting Sector"] = &_sectorStart;
			_params[L"Sector Count"] = &_sectorCount;
			_params[L"Sector Size"] = &_sectorSize;

			_chipNumber.Value = chipNumber;
			_sectorStart.Value = sectorStart;
			_sectorCount.Value = sectorCount;
			_sectorSize.Value = sectorSize;

			PrepareCommand();
		};
		
        virtual ~StReadPhysicalMediaSector() {};

		void ParseCdb()
		{
			_chipNumber.Value = _cdb.DriveNumber;
			_sectorStart.Value = Swap8((UCHAR*)&_cdb.Start);
			_sectorCount.Value = Swap4((UCHAR*)&_cdb.Count);

			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_READ_PHYSICAL_MEDIA_SECTOR;
			_cdb.DriveNumber = _chipNumber.Value;
			_cdb.Start = Swap8((UCHAR*)&_sectorStart.Value);
			_cdb.Count = Swap4((UCHAR*)&_sectorCount.Value);

			_xferLength = _sectorCount.Value * _sectorSize.Value;
		}
		
		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			if ( _responseDataPtr != NULL )
			{
				free(_responseDataPtr);
				_responseDataPtr = NULL;
			}

			_responseDataPtr = (UCHAR*)malloc(_xferLength);
			if ( _responseDataPtr != NULL )
				memcpy(_responseDataPtr, pData, _xferLength);
		};

//		UCHAR* GetDataPtr() { return _responseDataPtr; };
		
		CString& ResponseString()
		{
			FormatReadResponse(_responseDataPtr, 16);
			return _responseStr;
		};
	private:
		ParameterT<UCHAR> _chipNumber;
		ParameterT<ULONGLONG> _sectorStart;
		ParameterT<UINT> _sectorCount;
		ParameterT<UINT> _sectorSize;
//		UCHAR* _responseDataPtr;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StResetToRecovery
	//
	//////////////////////////////////////////////////////////////////////
	class StResetToRecovery : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StResetToRecovery() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("ResetToRecovery"))
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_RESET_TO_RECOVERY;
		};
		
		virtual ~StResetToRecovery() {};
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StInitializeJanus 
	//
	//////////////////////////////////////////////////////////////////////
	class StInitializeJanus : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StInitializeJanus() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("InitializeJanus"))
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_INITIALIZE_JANUS;
			_timeout = ST_SCSI_EXTENDED_TIMEOUT; //ST_SCSI_DEFAULT_TIMEOUT;
		};
		
		virtual ~StInitializeJanus() {};
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetJanusStatus
	//
	//////////////////////////////////////////////////////////////////////
	class StGetJanusStatus : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetJanusStatus() 
			: StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetJanusStatus"))
			, _status(0)
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_JANUS_STATUS;

			_xferLength = sizeof(_status);
		};
		
		virtual ~StGetJanusStatus() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			_status = *(UCHAR*)pData;
			_responseStr.Format(_T("Janus Status: 0x%02X"), GetJanusStatus());	
		};

		UCHAR GetJanusStatus() { return _status; };

	private:
		UCHAR _status;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StInitializeDataStore 
	//
	//////////////////////////////////////////////////////////////////////
	class StInitializeDataStore : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StInitializeDataStore(UCHAR storeNumber) 
			: StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("InitializeDataStore"))
		{
			_params[L"Store Number"] = &_storeNumber;
			_storeNumber.Value = storeNumber;
			_timeout = ST_SCSI_EXTENDED_TIMEOUT;

			PrepareCommand();
		};
		
		virtual ~StInitializeDataStore() {};

		void ParseCdb()
		{
			_storeNumber.Value = _cdb.SubCommand;
			PrepareCommand();
		};

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_INIT_DATA_STORE;
			_cdb.SubCommand = _storeNumber.Value;
		};
		
	private:
		ParameterT<UCHAR> _storeNumber;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StResetToUpdater
	//
	//////////////////////////////////////////////////////////////////////
	class StResetToUpdater : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StResetToUpdater() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("ResetToUpdater"))
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_RESET_TO_UPDATER;
		};
		
		virtual ~StResetToUpdater() {};
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StGetDeviceProperties
	//
	//////////////////////////////////////////////////////////////////////
	enum _DEVICE_PROPERTIES_TYPE {
		eDevicePhysicalExternalRamSz = 0,
		eDeviceVirtualExternalRamSz  = 1
	};
	
	class StGetDeviceProperties : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StGetDeviceProperties(_DEVICE_PROPERTIES_TYPE propertyType = eDevicePhysicalExternalRamSz)
			: StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_READ_CMD, _T("GetDeviceProperties"))
			, _exRamSize(0)
		{
			_params[L"Property Type:"] = &_propertyType;

			_propertyType.Value = propertyType;

			InitPropertyTypes();

			PrepareCommand();
		};

		void InitPropertyTypes()
		{
			_propertyType.ValueList[eDevicePhysicalExternalRamSz]	= L"PhysicalExternalRamSize";
			_propertyType.ValueList[eDeviceVirtualExternalRamSz]	= L"VirtualExternalRamSize";
		}

		virtual ~StGetDeviceProperties() {};

		void ParseCdb()
		{
			_propertyType.Value = _cdb.SubCommand;

			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = ST_SCSIOP_READ;
			_cdb.Command = DDI_GET_DEVICE_PROPERTIES;
			_cdb.SubCommand = _propertyType.Value;
			 
			_xferLength = sizeof(UINT);
		};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			// FOR SOME REASON THE F/W RETURNS THIS VALUE BACKWARDS
			// _exRamSize = Swap4(pData);
			_exRamSize = *(UINT*)pData;

			switch ( _propertyType.Value )
			{
				case eDevicePhysicalExternalRamSz:
					_responseStr.Format(_T("Physical SDRAM: %d MB"), GetExRamSize(), GetExRamSize());
					break;
				case eDeviceVirtualExternalRamSz:
					_responseStr.Format(_T("Virtual SDRAM: %d MB"), GetExRamSize(), GetExRamSize());
					break;
				default:
					_responseStr.Format(_T("Unknown property: 0x%08X (%d)"), GetExRamSize(), GetExRamSize());
					break;
			}
		}

		UINT GetExRamSize()
		{
			return _exRamSize;
		};

	private:
		ParameterT<UINT> _propertyType;
		UINT _exRamSize;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// DDI_COMMAND: StSetUpdateFlag
	//
	//////////////////////////////////////////////////////////////////////
	class StSetUpdateFlag : public StApiT<_ST_SCSI_CDB::_CDB16>
	{
	public:
		StSetUpdateFlag() : StApiT<_ST_SCSI_CDB::_CDB16>(API_TYPE_ST_SCSI, ST_WRITE_CMD, _T("SetUpdateFlag"))
		{
			_cdb.OperationCode = ST_SCSIOP_WRITE;
			_cdb.Command = DDI_SET_UPDATE_FLAG;
		};
		
		virtual ~StSetUpdateFlag() {};
	};

	//////////////////////////////////////////////////////////////////////
	//
	// SCSI_COMMAND: StScsiInquiry
	//
	//////////////////////////////////////////////////////////////////////
	class StScsiInquiry : public StApiT<_CDB::_CDB6INQUIRY>
	{
	public:
		StScsiInquiry() : StApiT<_CDB::_CDB6INQUIRY>(API_TYPE_SCSI, ST_READ_CMD, _T("ScsiInquiry"))
		{
			_cdb.OperationCode = SCSIOP_INQUIRY;
			_cdb.AllocationLength = sizeof(INQUIRYDATA);
			_xferLength = sizeof(INQUIRYDATA);
		};

		virtual ~StScsiInquiry() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			memcpy(&_data, pData, sizeof(_data));
		};

		INQUIRYDATA& GetInquiryData() { return _data; };
		
		CString& ResponseString()
		{
			FormatReadResponse((UCHAR*)&_data, 16);
			return _responseStr;
		};

	private:
		INQUIRYDATA _data;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// SCSI_COMMAND: StScsiStartStopUnit
	//
	//////////////////////////////////////////////////////////////////////
	class StScsiStartStopUnit : public StApiT<_CDB::_CDB12>
	{
	public:
		StScsiStartStopUnit() : StApiT<_CDB::_CDB12>(API_TYPE_SCSI, ST_WRITE_CMD, _T("ScsiStartStopUnit"))
		{
			_cdb.OperationCode = SCSIOP_START_STOP_UNIT;
			// cdb has been initialized to zeros so the cdb.Start bit should be clear.
		};

		virtual ~StScsiStartStopUnit() {};
	};

	//////////////////////////////////////////////////////////////////////
	//
	// SCSI_COMMAND: StReadCapacity
	//
	//////////////////////////////////////////////////////////////////////
	class StReadCapacity : public StApiT<_CDB::_CDB10>
	{
	public:
		StReadCapacity() : StApiT<_CDB::_CDB10>(API_TYPE_SCSI, ST_READ_CMD, _T("ReadCapacity"))
		{
			_cdb.OperationCode = SCSIOP_READ_CAPACITY;
			_xferLength = sizeof(READ_CAPACITY_DATA);
		};

		virtual ~StReadCapacity() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			_data.LogicalBlockAddress = Swap4(pData);
			_data.BytesPerBlock = Swap4(pData + sizeof(_data.LogicalBlockAddress));

			_responseStr.Format(_T("LogicalBlockAddress: 0x%X\r\nBytesPerBlock: 0x%X"), _data.LogicalBlockAddress, _data.BytesPerBlock);	
		};

		READ_CAPACITY_DATA& GetCapacity() { return _data; };

	private:
		READ_CAPACITY_DATA _data;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// SCSI_COMMAND: StRead
	//
	//////////////////////////////////////////////////////////////////////
	class StRead : public StApiT<_CDB::_CDB10>
	{
	public:
		StRead(UINT sectorStart, USHORT sectorCount, USHORT sectorSize)
			: StApiT<_CDB::_CDB10>(API_TYPE_SCSI, ST_READ_CMD, _T("Read"))
//			, _responseDataPtr(NULL)
		{
			
			_params[L"Starting Sector"] = &_sectorStart;
			_params[L"Sector Count"] = &_sectorCount;
			_params[L"Sector Size"] = &_sectorSize;

			_sectorStart.Value = sectorStart;
			_sectorCount.Value = sectorCount;
			_sectorSize.Value = sectorSize;

			PrepareCommand();
		};

		virtual ~StRead() {};

		void ParseCdb()
		{
			UINT swappedVar4;
			memcpy(&swappedVar4, &_cdb.LogicalBlockByte0, sizeof(swappedVar4));
			_sectorStart.Value = Swap4((UCHAR*)&swappedVar4);
			USHORT swappedVar2;
			memcpy(&swappedVar2, &_cdb.TransferBlocksMsb, sizeof(swappedVar2));
			_sectorCount.Value = Swap2((UCHAR*)&swappedVar2);

			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = SCSIOP_READ;
			UINT swappedVar4 = Swap4((UCHAR*)&_sectorStart.Value);
			memcpy(&_cdb.LogicalBlockByte0, &swappedVar4, sizeof(swappedVar4));
			USHORT swappedVar2 = Swap2((UCHAR*)&_sectorCount.Value);
			memcpy(&_cdb.TransferBlocksMsb, &swappedVar2, sizeof(swappedVar2));

			_xferLength = _sectorCount.Value * _sectorSize.Value;
		}
		
		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			if ( _responseDataPtr != NULL )
			{
				free(_responseDataPtr);
				_responseDataPtr = NULL;
			}

			_responseDataPtr = (UCHAR*)malloc(_xferLength);
			if ( _responseDataPtr != NULL )
			{
				memcpy(_responseDataPtr, pData, _xferLength);
			}
		};

//		UCHAR* GetDataPtr() { return _responseDataPtr; };
		
		CString& ResponseString()
		{
			FormatReadResponse(_responseDataPtr, 16);

			return _responseStr;
		};
	private:
		ParameterT<UINT> _sectorStart;
		ParameterT<USHORT> _sectorCount;
		ParameterT<USHORT> _sectorSize;
//		UCHAR* _responseDataPtr;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// SCSI_COMMAND: StWrite
	//
	//////////////////////////////////////////////////////////////////////
	class StWrite : public StApiT<_CDB::_CDB10>
	{
	public:
		StWrite(UINT sectorStart, USHORT sectorCount, USHORT sectorSize, UCHAR* pData)
			: StApiT<_CDB::_CDB10>(API_TYPE_SCSI, ST_WRITE_CMD_PLUS_DATA, _T("Write"))
		{
			_params[L"Starting Sector"] = &_sectorStart;
			_params[L"Sector Count"] = &_sectorCount;
			_params[L"Sector Size"] = &_sectorSize;

			_sectorStart.Value = sectorStart;
			_sectorCount.Value = sectorCount;
			_sectorSize.Value = sectorSize;

			SetCommandData(pData, sectorCount * sectorSize);
		};
		
		virtual ~StWrite() {};

		void ParseCdb()
		{
			UINT swappedVar4;
			memcpy(&swappedVar4, &_cdb.LogicalBlockByte0, sizeof(swappedVar4));
			_sectorStart.Value = Swap4((UCHAR*)&swappedVar4);
			USHORT swappedVar2;
			memcpy(&swappedVar2, &_cdb.TransferBlocksMsb, sizeof(swappedVar2));
			_sectorCount.Value = Swap2((UCHAR*)&swappedVar2);

			PrepareCommand();
		}

		void PrepareCommand()
		{
			_cdb.OperationCode = SCSIOP_WRITE;
			UINT swappedVar4 = Swap4((UCHAR*)&_sectorStart.Value);
			memcpy(&_cdb.LogicalBlockByte0, &swappedVar4, sizeof(swappedVar4));
			USHORT swappedVar2 = Swap2((UCHAR*)&_sectorCount.Value);
			memcpy(&_cdb.TransferBlocksMsb, &swappedVar2, sizeof(swappedVar2));

//			_xferLength = _sectorCount.Value * _sectorSize.Value;
		};

	private:
		ParameterT<UINT> _sectorStart;
		ParameterT<USHORT> _sectorCount;
		ParameterT<USHORT> _sectorSize;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// SCSI_COMMAND: StScsiModeSense10 class.
	//
	//////////////////////////////////////////////////////////////////////
	struct _MODESENSE10DATA
	{
		MODE_PARAMETER_HEADER10 Header;
		MODE_FLEXIBLE_DISK_PAGE Page;
	};

	class StModeSense10 : public StApiT<_CDB::_MODE_SENSE10>
	{
	public:
		StModeSense10()
			: StApiT<_CDB::_MODE_SENSE10>(API_TYPE_SCSI, ST_READ_CMD, _T("ModeSense10"))
		{
			_cdb.OperationCode = SCSIOP_MODE_SENSE10;
			_cdb.PageCode = 0x05;
			_cdb.AllocationLength[1] = 0x28; // LSB of parameter list length

			_xferLength = sizeof(_data);
		}

		virtual ~StModeSense10() {};

		void ProcessResponse(const UCHAR *const pData, const UINT start, const UINT count)
		{
			assert( count >= _xferLength );

			memset(&_data, 0xdb, sizeof(_MODESENSE10DATA));
			memcpy(&_data, pData, sizeof(_MODESENSE10DATA)); 
		};

		_MODESENSE10DATA& GetFlexibleDiskModePage() { return _data; };
		
		CString& ResponseString()
		{
			FormatReadResponse((UCHAR*)&_data, 16);
			return _responseStr;
		};
	
	private:
		_MODESENSE10DATA _data;
	};

	//////////////////////////////////////////////////////////////////////
	//
	// StApiFactory
	//
	//////////////////////////////////////////////////////////////////////
	class StApiFactory
	{
	public:
		typedef StApi* (*CreateApiCallback)(CString paramStr);
	private:
		typedef std::map<CString, CreateApiCallback> CallbackMap;
	
	public:
        StApiFactory();
		StApi* CreateApi(CString name, CString paramStr=_T(""));
	
	private:
		bool RegisterApi(CString name, CreateApiCallback createFn);
		bool UnregisterApi(CString name);
		CallbackMap _callbacks;
	};

} // namespace api

// The one and only StApiFactory object
extern api::StApiFactory& gStApiFactory();

using namespace api;

#endif // !defined(AFX_STDDIAPI_H__343A6E6E_764F_415F_9746_7B39782BA186__INCLUDED_)
