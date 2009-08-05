/*
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

using DevSupport.Media;
using DevSupport.DeviceManager.UTP;
*/
namespace api
{
	typedef CStringT<char,StrTraitMFC<char> > CAnsiString;

	class ScsiUtpMsg : public StApiT<_ST_SCSI_CDB::_CDBUTP16> // ScsiApi
    {
	public:

        /// <summary>
        /// Utp Command Set. 
        /// Byte[1] of the CDB for Vendor-specific SCSI commands.
        /// </summary>
        typedef enum _CommandSet
        {
            Poll  = 0x00,
            Exec  = 0x01,
            Get   = 0x02,
            Put   = 0x03,
            Reset = 0xAA
        } CommandSet;

		static CString MsgToString(CommandSet msg )
		{
			CString str;
			switch (msg)
			{
			case Poll:
				str = _T("Poll(0x00)");
				break;
			case Exec:
				str = _T("Exec(0x01)");
				break;
			case Get:
				str = _T("Get(0x02)");
				break;
			case Put:
				str = _T("Put(0x03)");
				break;
			case Reset:
				str = _T("Reset(0xAA)");
				break;
			default:
				str.Format(_T("UNKNOWN(%#02x)"), msg);
				break;
			}

			return str;
		}

		/// <summary>
        /// Utp Response Code Set. 
        /// Byte[12-13] of the SENSE_DATA structure for Vendor-specific SCSI commands.
        /// </summary>
        typedef enum 
        {
            PASS = 0x8000,
            EXIT = 0x8001,
            BUSY = 0x8002,
            SIZE = 0x8003
        }ResponseCodeType;

	protected:
		ScsiUtpMsg(ScsiUtpMsg::_CommandSet cmd, uint8_t dir, int64_t length, uint32_t tag, int64_t lparam)
            : StApiT<_ST_SCSI_CDB::_CDBUTP16>(API_TYPE_UTP, dir, _T("UtpCmd"))
        {
			_cdb.OperationCode = ST_SCSIOP_UTP;
			_cdb.Command = cmd;
			_cdb.Tag = Swap4((uint8_t*)&tag);
			_cdb.LParam = Swap8((uint8_t*)&lparam);

            _xferLength = length;
        }

	public:
		virtual CString ToString() { CString str; str.Format(_T("Unknown(tag:%d, lParam:%#08x, cmd:%#01x)"), _cdb.Tag, _cdb.LParam, _cdb.Command); return str;};

		ResponseCodeType GetResponseCode()
        {
            if (ScsiSenseStatus == SCSISTAT_GOOD)
				return PASS;
            else
            {
                if (ScsiSenseData.SenseKey == SCSI_SENSE_UNIQUE)
                    return (ResponseCodeType)((ScsiSenseData.AdditionalSenseCode << 8) + ScsiSenseData.AdditionalSenseCodeQualifier);
                else
					return EXIT;
            }
        };

		static CString CodeToString(ResponseCodeType code )
		{
			CString str;
			switch (code)
			{
			case PASS:
				str = _T("PASS(0x8000)");
				break;
			case EXIT:
				str = _T("EXIT(0x8001)");
				break;
			case BUSY:
				str = _T("BUSY(0x8002)");
				break;
			case SIZE:
				str = _T("SIZE(0x8003)");
				break;
			default:
				str.Format(_T("UNKNOWN(%#x)"), code);
				break;
			}

			return str;
		}

		int32_t GetResponseInfo()
        {
            uint8_t tempData[4] = { ScsiSenseData.Information[0], ScsiSenseData.Information[1], ScsiSenseData.Information[2], ScsiSenseData.Information[3] };
            return (int32_t)Swap4(tempData);
        }

        int64_t GetResponseSize()
        {
			if (GetResponseCode() == SIZE)
            {
                uint8_t tempData[8] =
                { 
                    ScsiSenseData.CommandSpecificInformation[0], ScsiSenseData.CommandSpecificInformation[1], ScsiSenseData.CommandSpecificInformation[2], ScsiSenseData.CommandSpecificInformation[3], 
                    ScsiSenseData.Information[0], ScsiSenseData.Information[1], ScsiSenseData.Information[2], ScsiSenseData.Information[3]
                };
                return (int64_t)Swap8(tempData);
            }
            else
            {
                return 0;
            }
        };

        CString GetResponseString()
        {
			if (GetResponseCode() == PASS)
                _responseStr = _T("PASS(0x8000)");
            else if (ScsiSenseData.SenseKey == SCSI_SENSE_UNIQUE)
            {
				if (GetResponseCode() == SIZE)
					_responseStr.Format(_T("SIZE(0x8003), Response Size: %#x"), GetResponseSize());
				else if ( GetResponseCode() == BUSY)
                    _responseStr.Format(_T("BUSY(0x8002), TicksRemaining: %d"), GetResponseInfo());
                else
					_responseStr.Format(_T("EXIT(0x8001), Response Info: %#x"), GetResponseInfo());
            }
				
			return CString(StApi::ResponseString());
        };
    }; // ScsiUtpMsg

    /// <summary>
    /// ////////////////////////////////////////////////////////////////////////////////////
    /// Poll
    /// ////////////////////////////////////////////////////////////////////////////////////
    /// </summary>
    class Poll : public ScsiUtpMsg
    {
	public:
		typedef enum _LParams { None, GetUtpVersion }LParams;

        // Constructor
        Poll(uint32_t tag, int64_t lParam)
			: ScsiUtpMsg(ScsiUtpMsg::Poll, ST_WRITE_CMD, 0, tag, lParam)
        {
        }

        CString ToString()
        {
			CString retStr, lparam;
            
            switch(_cdb.LParam)
			{
				case None:
					lparam = _T("None(0)");
					break;
				case GetUtpVersion:
					lparam = _T("GetUtpVersion(1)");
					break;
				default:
					lparam.Format(_T("Unknown(%d)"), _cdb.LParam);
					break;
			}

			retStr.Format(_T("Poll(tag:%d, lParam:%s)"), _cdb.Tag, lparam);

			return retStr;
        }

   }; // class Poll

    /// <summary>
    /// ////////////////////////////////////////////////////////////////////////////////////
    /// Exec
    /// ////////////////////////////////////////////////////////////////////////////////////
    /// </summary>
    class Exec : public ScsiUtpMsg
    {
	private:
		CAnsiString m_UtpCommand;

	public:
        // Constructor
        Exec(uint32_t tag, int64_t lParam, CString utpCommand)
			: ScsiUtpMsg(ScsiUtpMsg::Exec, ST_WRITE_CMD_PLUS_DATA, 0, tag, lParam)
        {
            USES_CONVERSION;

			m_UtpCommand = T2A(utpCommand);

			if (m_UtpCommand.IsEmpty())
            {
                _xferLength = 0;
                if ( _sendDataPtr )
				{
					free(_sendDataPtr);
					_sendDataPtr = NULL;
				}
            }
            else
            {
                _xferLength = (uint32_t)m_UtpCommand.GetLength();
                _sendDataPtr = (uint8_t*)malloc(_xferLength);

				memcpy_s(_sendDataPtr, _xferLength, m_UtpCommand.GetBuffer(), _xferLength);
            }
        }

//		CString GetUtpCommand() { USES_CONVERSION; return CString(A2T(m_UtpCommand)); };

		CString ToString()
        {
            USES_CONVERSION;

			CString str;
			str.Format(_T("Exec(tag:%d, lParam:%#08x, cmd:"), _cdb.Tag, _cdb.LParam);
			str += CString(A2T(m_UtpCommand)) + _T(")");

			return str;
        }
    
    }; // class Exec

    /// <summary>
    /// ////////////////////////////////////////////////////////////////////////////////////
    /// Get
    /// ////////////////////////////////////////////////////////////////////////////////////
    /// </summary>
    class Get : public ScsiUtpMsg
    {
	public:
        // Constructor
        Get(uint32_t tag, int64_t lParam, int64_t length)
			: ScsiUtpMsg(ScsiUtpMsg::Get, ST_READ_CMD, length, tag, lParam)
        {
        }

        CString ToString()
        {
            CString str;
			str.Format(_T("Get(tag:%d, pkt:%d, len:%#04x)"), _cdb.Tag, _cdb.LParam, _xferLength);

			return str;
        }
    
    }; // class Get

    /// <summary>
    /// ////////////////////////////////////////////////////////////////////////////////////
    /// Put
    /// ////////////////////////////////////////////////////////////////////////////////////
    /// </summary>
    class Put : public ScsiUtpMsg
    {
	public:
        // Constructor
		Put(uint32_t tag, int64_t lParam, std::vector<uint8_t> data)
			: ScsiUtpMsg(ScsiUtpMsg::Put, ST_WRITE_CMD_PLUS_DATA, data.size(), tag, lParam)
        {
			if ( data.size() )
			{
				_sendDataPtr = (uint8_t*)malloc(data.size());
				for ( unsigned int i = 0; i < data.size(); ++i )
					*(_sendDataPtr + i) = data[i];
			}
        }

        CString ToString()
        {
            CString str;
			str.Format(_T("Put(tag:%d, pkt:%d, len:%#04x)"), _cdb.Tag, _cdb.LParam, _xferLength);

			return str;
        }
    
    }; // class Put
    
/*
    public class ScsiUtpApi : ScsiApi
    {
        protected ScsiUtpApi(Api.CommandDirection dir)
            : base(Api.CommandType.UtpScsiApi, dir, 5) // timeout
        {
        }

        // Override Properties here to so they won't be Browsable
        [Browsable(false)]
        public override Byte[] Cdb
        {
            get { return base.Cdb; }
            set { base.Cdb = value; }
        }

        [Browsable(false)]
        public override CommandDirection Direction
        {
            get { return base.Direction; }
        }

        [Browsable(false)]
        public override string ImageKey
        {
            get { return base.ImageKey; }
        }

        [Browsable(false)]
        public override uint Timeout
        {
            get { return base.Timeout; }
            set { base.Timeout = value; }
        }

        [Browsable(false)]
        public override Int64 TransferSize
        {
            get { return base.TransferSize; }
            set { base.TransferSize = value; }
        }

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// UtpCommand
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class UtpCommand : ScsiUtpApi
        {
            // Constructor
            public UtpCommand(String cmd)
                : base(Api.CommandDirection.WriteWithData)
            {
                Command = cmd;
            }

            [Category("Parameters"), Description("The UTP command to execute.")]
            [MemberFunctionArg()]
            public String Command
            {
                get { return _Command; }
                set { _Command = value; }
            }
            private String _Command;

        } // class UtpCommand

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// UtpRead
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class UtpRead : ScsiUtpApi
        {
            // Constructor
            public UtpRead(String cmd, String filename)
                : base(Api.CommandDirection.ReadWithData)
            {
                Command = cmd;
                Filename = filename;
            }

            [Category("Parameters"), Description("The UTP command to execute.")]
            [MemberFunctionArg()]
            public String Command
            {
                get { return _Command; }
                set { _Command = value; }
            }
            private String _Command;

            [Category("Parameters"), Description("The file to create with data from the device.")]
            [MemberFunctionArg()]
            public String Filename
            {
                get { return _Filename; }
                set { _Filename = value; }
            }
            private String _Filename;

        } // class UtpRead

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// UtpWrite
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class UtpWrite : ScsiUtpApi
        {
            // Constructor
            public UtpWrite(String cmd, String filename)
                : base(Api.CommandDirection.WriteWithData)
            {
                Command = cmd;
                Filename = filename;
            }

            [Category("Parameters"), Description("The UTP command to execute.")]
            [MemberFunctionArg()]
            public String Command
            {
                get { return _Command; }
                set { _Command = value; }
            }
            private String _Command;

            [Category("Parameters"), Description("The file to write to the device.")]
            [MemberFunctionArg()]
            public String Filename
            {
                get { return _Filename; }
                set { _Filename = value; }
            }
            private String _Filename;

        } // class UtpWrite
    };
*/
} // namespace DevSupport.Api