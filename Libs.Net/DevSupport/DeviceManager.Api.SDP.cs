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
    public class MxRomCmd : Api
    {
        /// <summary>
        /// MX ROM MSg Command Set. 
        /// Byte[0-1] of the CDB for MX ROM-specific commands.
        /// </summary>
        public enum CommandSet : ushort
        {
            ReadRegister = 0x0101,
            WriteMemory = 0x0202,
            WriteFile = 0x0404,
            GetErrorStatus = 0x0505,
            RamKernelCmd = 0x0606,
            DCDWrite = 0x0A0A,     // new for MX50
            JumpAddress = 0x0B0B      // new for MX50
        }

        /// <summary>
        /// MX ROM Format Set. 
        /// Byte[6] of the CDB for MX ROM-specific commands.
        /// Used only for ReadRegister and WriteRegister commands
        /// </summary>
        public enum DataFormat : byte
        {
            Ignored = 0,
            _8_Bit = 0x08,
            _16_Bit = 0x10,
            _32_Bit = 0x20
        }

        /// <summary>
        /// Rom Response Code Set. 
        /// </summary>
        public enum ResponseCodeType : uint
        {
            Success = 0x00000000,
            HabDisabled = 0x56787856,
            HabEnabled = 0x12343412,
            WriteAck = 0x128A8A12
        }

        public MxRomCmd(MxRomCmd.CommandSet cmd, DataFormat format, UInt32 address, UInt32 length, UInt32 data,
            Api.CommandDirection dir, UInt32 timeout)
            : base(Api.CommandType.MxRomApi, dir, timeout)
        {
            _CDB = new CDB();

            _CDB.Add("Command", cmd);
            _CDB.Add("Address", address);
            _CDB.Add("Format", format);
            _CDB.Add("Data Count", length);
            _CDB.Add("Data", data);

            TransferSize = length;
        }

        [Description("The Command word for the ROM command."), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public MxRomCmd.CommandSet Command
        {
            get { return (MxRomCmd.CommandSet)_CDB.FromField("Command"); }
            private set { _CDB.ToField("Command", value); }
        }

        [Description("Valid for ReadRegister, WriteRegister, WriteFile, and JumpAddress."), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        virtual public UInt32 Address
        {
            get { return (UInt32)_CDB.FromField("Address"); }
            set { _CDB.ToField("Address", value); }
        }

        [Description("The format of data for the ROM command.\nOnly vaild for ReadRegister and WriteRegister"), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        virtual public MxRomCmd.DataFormat Format
        {
            get { return (MxRomCmd.DataFormat)_CDB.FromField("Format"); }
            set { _CDB.ToField("Format", value); }
        }

        [Description("Size of data to read or write.\n Valid for ???"), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        virtual public UInt32 DataCount
        {
            get { return (UInt32)_CDB.FromField("Data Count"); }
            set { _CDB.ToField("Data Count", value); }
        }

        [Description("Value to write.\n Only valid for WriteRegister"), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public virtual new UInt32 Data
        {
            get { return (UInt32)_CDB.FromField("Data"); }
            set { _CDB.ToField("Data", value); }
        }

        [Description("The Command Data Block for the api"), Category("Data")]
        public Byte[] Cdb
        {
            get
            {
                return _CDB.ToByteArray();
            }
            set {/* */}
        }
        protected CDB _CDB;

        virtual public Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
        {
            return Win32.ERROR_SUCCESS;
        }

        #region Response Properties

        [Category("Response"), ReadOnly(true)]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public ResponseCodeType ResponseCode
        {
            get { return _ResponseCode; }
            set { _ResponseCode = value; }
        }
        protected ResponseCodeType _ResponseCode;

        [Category("Response")]
        public override string ResponseString
        {
            get
            {
                Utils.EnumConverterEx enumCnvrt = new Utils.EnumConverterEx(typeof(ResponseCodeType));
                return enumCnvrt.ConvertToString(ResponseCode);
            }
        }

        #endregion

        // Override Properties here to so they won't be Browsable
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

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// GetStatus
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [MemberFunction()]
        public class GetStatus : MxRomCmd, IProcessResponse
        {
            // Constructor
            public GetStatus()
                : base(MxRomCmd.CommandSet.GetErrorStatus, DataFormat.Ignored, 0, 0, 0, Api.CommandDirection.NoData, 0)
            {
            }

            public override string ToString()
            {
                return "GetStatus";
            }

            [Browsable(false)]
            public override UInt32 Address
            {
                get { return base.Address; }
                set { base.Address = value; }
            }

            [Browsable(false)]
            public override MxRomCmd.DataFormat Format
            {
                get { return base.Format; }
                set { base.Format = value; }
            }

            [Browsable(false)]
            public override UInt32 DataCount
            {
                get { return base.DataCount; }
                set { base.DataCount = value; }
            }

            [Browsable(false)]
            public override UInt32 Data
            {
                get { return base.Data; }
                set { base.Data = value; }
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                _ResponseCode = (ResponseCodeType)BitConverter.ToUInt32(data, 0);

                return Win32.ERROR_SUCCESS;
            }

        } // class GetStatus

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// WriteMemory
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [MemberFunction()]
        public class WriteMemory : MxRomCmd
        {
            // Constructor
            public WriteMemory(UInt32 address, UInt32 data, DataFormat format)
                : base(MxRomCmd.CommandSet.WriteMemory, format, address, 0, data, Api.CommandDirection.NoData, 0)
            {
            }

            public override string ToString()
            {
                return "WriteMemory";
            }

            [MemberFunctionArg()]
            public override UInt32 Address
            {
                get { return base.Address; }
                set { base.Address = value; }
            }

            [MemberFunctionArg()]
            public override UInt32 Data
            {
                get { return base.Data; }
                set { base.Data = value; }
            }

            [MemberFunctionArg()]
            public override MxRomCmd.DataFormat Format
            {
                get { return base.Format; }
                set { base.Format = value; }
            }

            [Browsable(false)]
            public override UInt32 DataCount
            {
                get { return base.DataCount; }
                set { base.DataCount = value; }
            }

        } // class WriteMemory
    
    } // class MxRomCmd

    public class MxRamKrnlCmd : Api
    {
        public const UInt32 MaxModelStringLength = 128;

        /// <summary>
        /// MX Ram Kernel Command Ids. 
        /// Used to construct the MX RAM Kernel-specific commands.
        /// </summary>
        public enum RklCmdId : byte
        {
            Flash = 0x00,
            Fuse = 0x01,
            Common = 0x02,
            Extend = 0x03
        }

        /// <summary>
        /// MX Ram Kernel Command Set. 
        /// Byte[2-3] of the CDB for MX RAM Kernel-specific commands.
        /// </summary>
        public enum RklCmdSet : ushort
        {
            FlashInit = ((RklCmdId.Flash << 8) | 0x01),
            FlashErase = ((RklCmdId.Flash << 8) | 0x02),
            FlashDump = ((RklCmdId.Flash << 8) | 0x03),
            FlashProgram = ((RklCmdId.Flash << 8) | 0x04),
            FlashProgramUB = ((RklCmdId.Flash << 8) | 0x05),
            FlashGetCapacity = ((RklCmdId.Flash << 8) | 0x06),
            FuseRead = ((RklCmdId.Fuse << 8) | 0x01),
            FuseSense = ((RklCmdId.Fuse << 8) | 0x02),
            FuseOverride = ((RklCmdId.Fuse << 8) | 0x03),
            FuseProgram = ((RklCmdId.Fuse << 8) | 0x04),
            Reset = ((RklCmdId.Common << 8) | 0x01),
            Download = ((RklCmdId.Common << 8) | 0x02),
            Execute = ((RklCmdId.Common << 8) | 0x03),
            GetVersion = ((RklCmdId.Common << 8) | 0x04),
            Com2Usb = ((RklCmdId.Extend << 8) | 0x01),
            SwapBi = ((RklCmdId.Extend << 8) | 0x02),
            FL_BBT = ((RklCmdId.Extend << 8) | 0x03),
            FL_INTLV = ((RklCmdId.Extend << 8) | 0x04),
            FL_LBA = ((RklCmdId.Extend << 8) | 0x05),
        }

        public enum ResponseCode : short
        {
            Success         =    0,
            InvalidChannel  = -256,
            InvalidChecksum = -257,
            InvalidParam    = -258
        }

        public struct ResponseType
        {
            public ResponseCode ack;	// ack
            public UInt16 csum;         // data checksum
            public UInt32 len;	        // data len

            public Byte[] ToByteArray()
            {
                List<Byte> bytes = new List<Byte>(8);

                bytes.AddRange(BitConverter.GetBytes((short)ack));
                bytes.AddRange(BitConverter.GetBytes(csum));
                bytes.AddRange(BitConverter.GetBytes(len));

                return bytes.ToArray();
            }

            public override string ToString()
            {
                Utils.DecimalConverterEx decCnvrtr = new Utils.DecimalConverterEx();
                Utils.EnumConverterEx enumCnvrtr = new Utils.EnumConverterEx(typeof(ResponseCode));

                return String.Format("Ack={0}, ChkSum={1}, Len={2}",
                    enumCnvrtr.ConvertToString(ack), decCnvrtr.ConvertToString(csum), decCnvrtr.ConvertToString(len));
            }
        }

        public MxRamKrnlCmd(RklCmdSet cmd, UInt32 address, UInt32 param1, UInt32 param2)
            : base(Api.CommandType.MxRamKrnlApi, CommandDirection.NoData, 0)
        {
            _CDB = new CDB();

            _CDB.Add("RomCommand", MxRomCmd.CommandSet.RamKernelCmd);
            _CDB.Add("Command", cmd);
            _CDB.Add("Address", address);
            _CDB.Add("Param 1", param1);
            _CDB.Add("Param 2", param2);
        }
        
        [Description("The Command word for the ROM command."), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public MxRomCmd.CommandSet RomCommand
        {
            get { return (MxRomCmd.CommandSet)_CDB.FromField("RomCommand"); }
            private set { _CDB.ToField("RomCommand", value); }
        }

        [Description("The Command word for the RAM Kernel command."), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public MxRamKrnlCmd.RklCmdSet Command
        {
            get { return (MxRamKrnlCmd.RklCmdSet)_CDB.FromField("Command"); }
            private set { _CDB.ToField("Command", value); }
        }

        [Description("Command address - Optional."), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        virtual public UInt32 Address
        {
            get { return (UInt32)_CDB.FromField("Address"); }
            set { _CDB.ToField("Address", value); }
        }

        [Description("Param 1 - Optional."), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        virtual public UInt32 Param1
        {
            get { return (UInt32)_CDB.FromField("Param 1"); }
            set { _CDB.ToField("Param 1", value); }
        }

        [Description("Param 2 - Optional."), Category("General")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        virtual public UInt32 Param2
        {
            get { return (UInt32)_CDB.FromField("Param 2"); }
            set { _CDB.ToField("Param 2", value); }
        }

        [Description("The Command Data Block for the api"), Category("Data")]
        public Byte[] Cdb
        {
            get
            {
                return _CDB.ToByteArray();
            }
            set {/* */}
        }
        protected CDB _CDB;

        virtual public Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
        {
            Array.Reverse(data, 0, sizeof(Int16));
            Ack = (ResponseCode)BitConverter.ToInt16(data, 0);
            Array.Reverse(data, 2, sizeof(UInt16));
            DataCheckSum = BitConverter.ToUInt16(data, 2);
            Array.Reverse(data, 4, sizeof(UInt32));
            DataLength = BitConverter.ToUInt32(data, 4);

            return Win32.ERROR_SUCCESS;
        }

        #region Response Properties

        [Category("Response")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public MxRomCmd.ResponseCodeType RomResponse
        {
            get { return _RomResponse; }
            protected set { _RomResponse = value; }
        }
        private MxRomCmd.ResponseCodeType _RomResponse;

        [Category("Response"), ReadOnly(true)]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public virtual ResponseCode Ack
        {
            get { return _Response.ack; }
            set { _Response.ack = value; }
        }

        [Description("Data checksum"), Category("Response")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public virtual UInt16 DataCheckSum
        {
            get { return _Response.csum; }
            protected set { _Response.csum = value; }
        }

        [Description("Data length"), Category("Response")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public virtual UInt32 DataLength
        {
            get { return _Response.len; }
            protected set { _Response.len = value; }
        }

        public ResponseType _Response;

        [Category("Response")]
        public override string ResponseString
        {
            get
            {
                Utils.DecimalConverterEx decCnvrtr = new Utils.DecimalConverterEx();
                Utils.EnumConverterEx enumCnvrtr = new Utils.EnumConverterEx(typeof(ResponseCode));

                return String.Format("Ack={0}, ChkSum={1}, Len={2}", 
                    enumCnvrtr.ConvertToString(Ack), decCnvrtr.ConvertToString(DataCheckSum), decCnvrtr.ConvertToString(DataLength));
            }
        }

        #endregion

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// GetVersion
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [MemberFunction()]
        public class GetVersion : MxRamKrnlCmd, IProcessResponse
        {
            // Constructor
            public GetVersion()
                : base(MxRamKrnlCmd.RklCmdSet.GetVersion, 0, 0, 0)
            {
            }

            public override string ToString()
            {
                return "GetRamKrnlVersion";
            }

            [Browsable(false)]
            public override UInt32 Address
            {
                get { return base.Address; }
                set { base.Address = value; }
            }

            [Browsable(false)]
            public override UInt32 Param1
            {
                get { return base.Param1; }
                set { base.Param1 = value; }
            }

            [Browsable(false)]
            public override UInt32 Param2
            {
                get { return base.Param2; }
                set { base.Param2 = value; }
            }

            #region Response Properties

            [Description("Flash Model"), Category("Response")]
            [MemberFunctionArg(), MemberFunctionArgByRef()]
            public String FlashModel
            {
                get { return _FlashModel; }
                private set { _FlashModel = value; }
            }
            private String _FlashModel = String.Empty;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            [MemberFunctionArg(), MemberFunctionArgByRef()]
            public UInt16 ChipId
            {
                get { return DataCheckSum; }
            }

            [Category("Response")]
            public override string ResponseString
            {
                get
                {
                    if (RomResponse != MxRomCmd.ResponseCodeType.Success)
                        return String.Format("Bootstrap is running. {0}", RomResponse);
                    else
                    {
                        Utils.DecimalConverterEx decCnvrtr = new Utils.DecimalConverterEx();
                        Utils.EnumConverterEx enumCnvrtr = new Utils.EnumConverterEx(typeof(ResponseCode));
                        return String.Format("Ack={0}, ChipId={1}, Model={2}", enumCnvrtr.ConvertToString(Ack), decCnvrtr.ConvertToString(ChipId), FlashModel);
                    }
                }
            }

            [Browsable(false)]
            public override UInt16 DataCheckSum
            {
                get { return base.DataCheckSum; }
                protected set { base.DataCheckSum = value; }
            }

            [Browsable(false)]
            public override UInt32 DataLength
            {
                get { return base.DataLength; }
                protected set { base.DataLength = value; }
            }


            #endregion
            
            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                if (data == null)
                    return Win32.ERROR_INVALID_PARAMETER;

                if (data.Length == 4)
                {
                    _RomResponse = (MxRomCmd.ResponseCodeType)BitConverter.ToUInt32(data, 0);

                }
                else
                {
                    _RomResponse = MxRomCmd.ResponseCodeType.Success;

                    base.ProcessResponse(data, 0, Marshal.SizeOf(_Response));

                    int modelLen = data.Length - Marshal.SizeOf(_Response);
                    if (modelLen > 0)
                    {
                        FlashModel = Utils.Utils.StringFromAsciiBytes(data, Marshal.SizeOf(_Response), modelLen);
                    }
                }

                return Win32.ERROR_SUCCESS;
            }

        } // class GetVersion

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// FlashInit
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [MemberFunction()]
        public class FlashInit : MxRamKrnlCmd
        {
            // Constructor
            public FlashInit()
                : base(MxRamKrnlCmd.RklCmdSet.FlashInit, 0, 0, 0)
            {
            }

            public override string ToString()
            {
                return "FlashInit";
            }

            [Browsable(false)]
            public override UInt32 Address
            {
                get { return base.Address; }
                set { base.Address = value; }
            }

            [Browsable(false)]
            public override UInt32 Param1
            {
                get { return base.Param1; }
                set { base.Param1 = value; }
            }

            [Browsable(false)]
            public override UInt32 Param2
            {
                get { return base.Param2; }
                set { base.Param2 = value; }
            }

            #region Response Properties

            [Browsable(false)]
            public override UInt16 DataCheckSum
            {
                get { return base.DataCheckSum; }
                protected set { base.DataCheckSum = value; }
            }

            [Browsable(false)]
            public override UInt32 DataLength
            {
                get { return base.DataLength; }
                protected set { base.DataLength = value; }
            }

            #endregion

        } // class FlashInit

    } // class MxRamKrnlCmd

/*
        public class MxRomApi : ScsiApi
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
        }
*/
} // namespace DevSupport.Api