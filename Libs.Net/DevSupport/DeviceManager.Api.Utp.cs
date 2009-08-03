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

namespace DevSupport.Api
{
    public class ScsiUtpMsg : ScsiApi
    {
        /// <summary>
        /// The StUtp Command. Byte[0] of the CDB.
        /// </summary>
        public const Byte CommandOp = 0xF0;

        /// <summary>
        /// Utp Command Set. 
        /// Byte[1] of the CDB for Vendor-specific SCSI commands.
        /// </summary>
        public enum CommandSet : byte
        {
            Poll  = 0x00,
            Exec  = 0x01,
            Get   = 0x02,
            Put   = 0x03,
            Reset = 0xAA
        }

        /// <summary>
        /// Utp Response Code Set. 
        /// Byte[12-13] of the SENSE_DATA structure for Vendor-specific SCSI commands.
        /// </summary>
        public enum ResponseCodeType : short
        {
            PASS = unchecked((short)0x8000),
            EXIT = unchecked((short)0x8001),
            BUSY = unchecked((short)0x8002),
            SIZE = unchecked((short)0x8003)
        }

        protected ScsiUtpMsg(ScsiUtpMsg.CommandSet cmd, Api.CommandDirection dir, Int64 length, UInt32 tag, Int64 lparam)
            : base(Api.CommandType.UtpScsiMsg, dir, 5/*timeout*/)
        {
            _CDB.Add("Operation", CommandOp);
            _CDB.Add("Command", cmd);
            _CDB.Add("Tag", tag);
            _CDB.Add("LParam", lparam);

            TransferSize = length;
        }

        [Description("The Operation byte for the api."), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        virtual public Byte Operation
        {
            get { return (Byte)_CDB.FromField("Operation"); }
            private set { _CDB.ToField("Operation", value); }
        }

        [Description("The Command byte for the api."), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public ScsiUtpMsg.CommandSet Command
        {
            get { return (ScsiUtpMsg.CommandSet)_CDB.FromField("Command"); }
            private set { _CDB.ToField("Command", value); }
        }

        [Description("The tag for the entire UTP transaction."), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public UInt32 Tag
        {
            get { return (UInt32)_CDB.FromField("Tag"); }
            /*private*/ set { _CDB.ToField("Tag", value); }
        }

        [Description("Meaning varies per UTP message."), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public Int64 LParam
        {
            get { return (Int64)_CDB.FromField("LParam"); }
            /*private*/ set { _CDB.ToField("LParam", value); }
        }

        #region Response Properties

        [Category("Response")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public ResponseCodeType ResponseCode
        {
            get 
            {
                if (SenseStatus == Win32.ScsiSenseStatus.GOOD)
                    return ResponseCodeType.PASS;
                else
                {
                    if (SenseData.SenseKey == Win32.ScsiSenseKey.SCSI_SENSE_UNIQUE)
                        return (ResponseCodeType)((SenseData.AdditionalSenseCode << 8) + SenseData.AdditionalSenseCodeQualifier);
                    else
                        return ResponseCodeType.EXIT;
                }
            }
        }

        [Category("Response")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public Int32 ResponseInfo
        {
            get 
            {
                Byte[] tempData = new Byte[4] { SenseData.Information[0], SenseData.Information[1], SenseData.Information[2], SenseData.Information[3] };
                Array.Reverse(tempData);
                return BitConverter.ToInt32(tempData, 0);
            }
        }

        [Category("Response")]
        [TypeConverter(typeof(Utils.ByteFormatConverter))]
        public Int64 ResponseSize
        {
            get
            {
                if (ResponseCode == ResponseCodeType.SIZE)
                {
                    Byte[] tempData = new Byte[8] 
                    { 
                        SenseData.CommandSpecificInformation[0], SenseData.CommandSpecificInformation[1], SenseData.CommandSpecificInformation[2], SenseData.CommandSpecificInformation[3], 
                        SenseData.Information[0], SenseData.Information[1], SenseData.Information[2], SenseData.Information[3]
                    };
                    Array.Reverse(tempData);
                    return BitConverter.ToInt64(tempData, 0);
                }
                else
                {
                    return 0;
                }
            }
        }

        [Category("Response")]
        public override string ResponseString
        {
            get
            {
                Utils.DecimalConverterEx decCnvrt = new Utils.DecimalConverterEx();
                Utils.EnumConverterEx enumCnvrt = new Utils.EnumConverterEx(typeof(ResponseCodeType));
                Utils.ByteFormatConverter byteCnvrt = new Utils.ByteFormatConverter();

                if (ResponseCode == ResponseCodeType.PASS)
                    return enumCnvrt.ConvertToString(ResponseCode);
                else if (SenseData.SenseKey == Win32.ScsiSenseKey.SCSI_SENSE_UNIQUE)
                {
                    if (ResponseCode == ResponseCodeType.SIZE)
                        return enumCnvrt.ConvertToString(ResponseCode) + ", Response Size: " + byteCnvrt.ConvertToString(ResponseSize);
                    else if ( ResponseCode == ResponseCodeType.BUSY)
                        return enumCnvrt.ConvertToString(ResponseCode) + ", TicksRemaining: " + ResponseInfo.ToString("#,#0");
                    else
                        return enumCnvrt.ConvertToString(ResponseCode) + ", Response Info: " + decCnvrt.ConvertToString(ResponseInfo);
                }
                else
                    return base.ResponseString;
            }
        }

        #endregion

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// Poll
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class Poll : ScsiUtpMsg
        {
            public enum LParams : ulong { None, GetUtpVersion }
            // Constructor
            public Poll(UInt32 tag, Int64 lParam)
                : base(ScsiUtpMsg.CommandSet.Poll, Api.CommandDirection.NoData, 0, tag, lParam)
            {
            }

            public override string ToString()
            {
                String lparam = String.Empty;
                
                try { lparam = Enum.GetName(typeof(LParams), LParam); }
                catch {}
                
                if ( String.IsNullOrEmpty(lparam) )
                    lparam = LParam.ToString("#,#0");
                else
                    lparam += "(" + LParam.ToString("#,#0") + ")";

                return String.Format("Poll(tag:{0}, lParam:{1})", Tag, lparam);
            }

       } // class Poll

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// Exec
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class Exec : ScsiUtpMsg
        {
            // Constructor
            public Exec(UInt32 tag, Int64 lParam, String utpCommand)
                : base(ScsiUtpMsg.CommandSet.Exec, Api.CommandDirection.WriteWithData, 0, tag, lParam)
            {
                UtpCommand = utpCommand;
            }

            #region Parameter Properties

            // Parameters
            [Category("Parameters"), Description("The UTP command.")]
            public String UtpCommand
            {
                get { return _UtpCommand; }
                set
                {
                    _UtpCommand = value;
                    if (_UtpCommand == null)
                    {
                        TransferSize = 0;
                        Data = null;
                    }
                    else
                    {
                        TransferSize = (UInt32)_UtpCommand.Length;
                        Data = new Byte[TransferSize];

                        ASCIIEncoding ascii = new ASCIIEncoding();
                        ascii.GetBytes(_UtpCommand, 0, (Int32)TransferSize, Data, 0);
                    }
                }
            }
            private String _UtpCommand;

            #endregion

            public override string ToString()
            {
                return String.Format("Exec(tag:{0}, lParam:0x{1:X}, cmd:{2})", Tag, LParam, UtpCommand);
            }
        
        } // class Exec

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// Get
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class Get : ScsiUtpMsg
        {
            // Constructor
            public Get(UInt32 tag, Int64 lParam, Int64 length)
                : base(ScsiUtpMsg.CommandSet.Get, Api.CommandDirection.ReadWithData, length, tag, lParam)
            {
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                Data = new Byte[TransferSize];

                data.CopyTo(Data, 0);

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            [Browsable(true)]
            public override byte[] Data
            {
                get { return base.Data; }
                set { base.Data = value; }
            }
/*
            [Browsable(false)]
            override public String ResponseString
            {
                get
                {
                    return FormatReadResponse(Data, 16, 1);
                }
            }
*/
            #endregion // Response

            public override string ToString()
            {
                return String.Format("Get(tag:{0}, pkt{1}, len:0x{2:X})", Tag, LParam, TransferSize);
            }
        
        } // class Get

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// Put
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class Put : ScsiUtpMsg
        {
            // Constructor
            public Put(UInt32 tag, Int64 lParam, Byte[] data)
                : base(ScsiUtpMsg.CommandSet.Put, Api.CommandDirection.WriteWithData, data != null ? data.Length : 0, tag, lParam)
            {
                Data = data;
            }

            [Category("Parameters"), Description("Data to write.")]
            [Browsable(true)]
            [Editor(typeof(Utils.BinaryEditorEx), typeof(System.Drawing.Design.UITypeEditor))]
            public override byte[] Data
            {
                get
                {
                    if (base.Data == null)
                        base.Data = new Byte[TransferSize];

                    return base.Data;
                }
                set { base.Data = value; }
            }

            public override string ToString()
            {
                return String.Format("Put(tag:{0}, pkt:{1}, len:0x{2:X})", Tag, LParam, TransferSize);
            }
        
        } // class Put
    
    } // ScsiUtpMsg

    public class ScsiUtpApi : ScsiApi
    {
        protected ScsiUtpApi(Api.CommandDirection dir)
            : base(Api.CommandType.UtpScsiApi, dir, 5/*timeout*/)
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
    }
} // namespace DevSupport.Api