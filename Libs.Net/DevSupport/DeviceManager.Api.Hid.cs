using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;
using System.Text;

namespace DevSupport.Api
{
    abstract public class HidApi : Api
    {
        /// <summary>
        /// Hid Command Set.
        /// </summary>
        public enum CommandSet : byte
        {
            /// <summary>
            /// ROM Command Set.
            /// </summary>
            Inquiry = 0x01,
            DownloadFw = 0x02,
            RequestSense = 0x03,
            DeviceReset = 0x04,
            DevicePowerDown = 0x05,
            /// <summary>
            /// Base PITC Command Set. These are required commands for any PITC.
            /// </summary>
            PitcTestUnitReady = 0x10,
            PitcRequestSense = 0x11,
            PitcInquiry = 0x12,
            PitcRead = 0x13,
            PitcWrite = 0x14
            /// <summary>
            /// Extended PITC commands should start at 0x80 and would not be required for all PITCs.
            /// </summary>
            /// ,SomeFutureCommand = 0x80
        }

        //------------------------------------------------------------------------------
        // PITC Identifiers
        //------------------------------------------------------------------------------
        const UInt32
            PITC_TYPE_LOAD_TEST = 0x8000,
            PITC_TYPE_RAM_TEST = 0x8001,
            PITC_TYPE_OTP_ACCESS = 0x8002;

        //------------------------------------------------------------------------------
        // HID Report Types (IDs)
        //------------------------------------------------------------------------------
        public enum HidReportType : byte
        {
            BltcCommandOut = 0x01,
            BltcDataOut = 0x02,
            BltcDataIn = 0x03,
            BltcStatusIn = 0x04,
            PitcCommandOut = 0x05,
            PitcDataOut = 0x06,
            PitcDataIn = 0x07,
            PitcStatusIn = 0x08
        }

        protected HidApi(HidApi.CommandSet cmd, Api.CommandType cmdType, Api.CommandDirection dir, UInt32 timeout)
            : base(cmdType, dir, timeout)
        {
            _CDB = new CDB();
            _CDB.Add("Command", cmd);
        }

        virtual public Int32 ProcessResponse(Byte[] data, UInt32 start, UInt32 count)
        {
            return Win32.ERROR_SUCCESS;
        }

        [TypeConverter(typeof(Utils.EnumConverterEx))]
        [Description("The Command byte for the api."), Category("General")]
        public HidApi.CommandSet Command
        {
            get { return (HidApi.CommandSet)_CDB.FromField("Command"); }
            private set { _CDB.ToField("Command", value); }
        }

        [Description("The tag denoting the instance of the api."), Category("General")]
        [DefaultValueAttribute(0)]
        public UInt32 Tag
        {
            get { return _Tag; }
            set { _Tag = value; }
        }
        protected UInt32 _Tag;

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

        ////////////////////////////////////////////////////////////////////////////////
        //
        // ST_HID_BLTC_COMMAND: HidInquiry
        //
        ////////////////////////////////////////////////////////////////////////////////
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class Inquiry : HidApi, IFilterProperties
        {
            public enum PitcStatusType : int { Invalid = -1, Ready = 0x00000000, NotReady = 0x00000001 };
            public enum InfoPageType : byte { Chip = 0x01, PitcStatus = 0x02 };

            // Constructor
            public Inquiry(InfoPageType infoPage, UInt32 infoParam)
                : base(CommandSet.Inquiry, CommandType.HidBltcCmd, CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("InfoPage", infoPage);
                _CDB.Add("InfoParam", infoParam);
                
                // General Init
                TransferSize = (UInt32)(InfoPage == InfoPageType.Chip ? Marshal.SizeOf(_ChipInfo) : sizeof(PitcStatusType));
            }

            [Description("The type of info to get."), Category("Parameters")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
//            [RefreshProperties(RefreshProperties.All)]
            public InfoPageType InfoPage
            {
                get { return (InfoPageType)_CDB.FromField("InfoPage"); }
                set 
                {
                    _CDB.ToField("InfoPage", value);
                    TransferSize = (UInt32)(InfoPage == InfoPageType.Chip ? Marshal.SizeOf(_ChipInfo) : sizeof(PitcStatusType));
                }
            }

            [Description("Varies by InfoPage. (UInt32)"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 InfoParam
            {
                get { return Convert.ToUInt32(_CDB.FromField("InfoParam")); }
                set { _CDB.ToField("InfoParam", value); }
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, UInt32 start, UInt32 count)
            {
                Debug.Assert(count >= _TransferSize);

                if (InfoPage == InfoPageType.Chip)
                {
                    _ChipInfo.ChipId = BitConverter.ToUInt16(data, 0);
                    _ChipInfo.ChipRevision = BitConverter.ToUInt16(data, 2);
                    _ChipInfo.RomVersion = BitConverter.ToUInt16(data, 4);
                    _ChipInfo.RomLoaderProtocolVersion = BitConverter.ToUInt16(data, 6);
                }
                else if (InfoPage == InfoPageType.PitcStatus)
                {
                    _PitcStatus = (PitcStatusType)BitConverter.ToInt16(data, 0);
                }
                else
                {
                    return Win32.ERROR_INVALID_PARAMETER;
                }

                return Win32.ERROR_SUCCESS;
            }

            #region IFilterProperties Members

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                return ((InfoPageType)responseAttribute.Criteria == InfoPage);
            }

            #endregion

            #region ChipInfo Response Properties

            [StructLayout(LayoutKind.Sequential, Pack = 2)]
            private struct ChipInfoPage
            {
                public UInt16 ChipId;
                public UInt16 ChipRevision;
                public UInt16 RomVersion;
                public UInt16 RomLoaderProtocolVersion;
            }
            private ChipInfoPage _ChipInfo;

            [Category("Response"), BrowsableResponse(InfoPageType.Chip)] 
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 ChipId
            {
                get
                {
                    if (InfoPage != InfoPageType.Chip)
                        return 0;

                    return _ChipInfo.ChipId;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.Chip)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 ChipRevision
            {
                get
                {
                    if (InfoPage != InfoPageType.Chip)
                        return 0;

                    return _ChipInfo.ChipRevision;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.Chip)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 RomVersion
            {
                get
                {
                    if (InfoPage != InfoPageType.Chip)
                        return 0;

                    return _ChipInfo.RomVersion;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.Chip)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 RomLoaderProtocolVersion
            {
                get
                {
                    if (InfoPage != InfoPageType.Chip)
                        return 0;

                    return _ChipInfo.RomLoaderProtocolVersion;
                }
            }

            #endregion

            #region PitcStatus Response Properties

            [Category("Response"), BrowsableResponse(InfoPageType.PitcStatus)]
            public PitcStatusType PitcStatus
            {
                get 
                {
                    if (InfoPage != InfoPageType.PitcStatus)
                        return PitcStatusType.Invalid;
                    
                    return _PitcStatus;
                }
            }
            private PitcStatusType _PitcStatus = PitcStatusType.Invalid;

            #endregion

            [Browsable(false)]
            override public String ResponseString
            {
                get
                {
                    switch (InfoPage)
                    {
                        case InfoPageType.Chip:
                            {
                                _ResponseString =  String.Format(" Chip ID: 0x{0:X4}\r\n", ChipId);
                                _ResponseString += String.Format(" Chip Revision: 0x{0:X4}\r\n", ChipRevision);
                                _ResponseString += String.Format(" ROM Version: 0x{0:X4}\r\n", RomVersion);
                                _ResponseString += String.Format(" ROM Loader Protocol Version: 0x{0:X4}\r\n", RomLoaderProtocolVersion);
                                break;
                            }
                        case InfoPageType.PitcStatus:
                            {
                                switch (PitcStatus)
                                {
                                    case PitcStatusType.Ready:
                                        _ResponseString = " PITC Status: READY(0x00000000)\r\n";
                                        break;
                                    case PitcStatusType.NotReady:
                                        _ResponseString = " PITC Status: NOT_READY(0x00000001)\r\n";
                                        break;
                                    default:
                                        _ResponseString = String.Format(" PITC Status: UNKNOWN(0x{0})\r\n", PitcStatus.ToString("X"));
                                        break;
                                }
                                break;
                            }
                        default:
                            _ResponseString = String.Format(" Invalid Inquiry InfoPage.(0x{0})\r\n", InfoPage.ToString("X"));
                            break;

                    }
                    return _ResponseString;
                }
            }
        }

        public class DevicePowerDown : HidApi
        {
            // Constructor
            public DevicePowerDown()
                : base(CommandSet.DevicePowerDown, CommandType.HidBltcCmd, CommandDirection.NoData, DefaultTimeout)
            {
            }
        }
        public class DeviceReset : HidApi
        {
            // Constructor
            public DeviceReset()
                : base(CommandSet.DeviceReset, CommandType.HidBltcCmd, CommandDirection.NoData, DefaultTimeout)
            {
            }
        }
        //////////////////////////////////////////////////////////////////////
        //
        // ST_HID_BLTC_COMMAND: HidBltcRequestSense
        //
        //////////////////////////////////////////////////////////////////////
        public class RequestSense : HidApi
        {
            public enum BltcSenseCode : uint
            {
                NoErrors = 0x00000000,
                InvalidCbw = 0x00000001,
                InvalidCdbCommand = 0x00000002,
                InvalidInquiryPake = 0x00010001,
                InvalidPageLength = 0x00020001,
                RomLoaderInvalidCommand = 0x00020002,
                RomLoaderDecryptionFailure = 0x00020003,
                Invalid = 0xC01DF00D
            }
            
            private BltcSenseCode _SenseCode;

            // Constructor
            public RequestSense()
                : base(CommandSet.RequestSense, CommandType.HidBltcCmd, CommandDirection.ReadWithData, DefaultTimeout)
            {
                // General Init
                TransferSize = sizeof(UInt32);
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, UInt32 start, UInt32 count)
            {
                Debug.Assert(count >= _TransferSize);

                if (true)
                {
                    _SenseCode = (BltcSenseCode) BitConverter.ToUInt32(data, 0);
                }
                else if (true)
                {
                    _SenseCode = BltcSenseCode.Invalid;
                }
                else
                {
                    return Win32.ERROR_INVALID_PARAMETER;
                }

                return Win32.ERROR_SUCCESS;
            }

            #region Sense Code Response Properties

            [Category("Response")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public BltcSenseCode SenseCode
            {
                get { return _SenseCode; }
            }

            #endregion
        }

        public class DownloadFw : HidApi
        {
            // Constructor
            public DownloadFw(String filename)
                : base(CommandSet.DownloadFw, CommandType.HidBltcCmd, CommandDirection.WriteWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("Length", (uint)0);

                // General Init
                TransferSize = 0;

                FwFilename = filename;
            }

            [Category("Parameters"), Description("The filename of the firmware to download."), NotifyParentProperty(true)]
            [EditorAttribute(typeof(System.Windows.Forms.Design.FileNameEditor), typeof(System.Drawing.Design.UITypeEditor))]
            public String FwFilename
            {
                get { return _FwFilename; }
                set
                {
                    _FwFilename = value;

                    try
                    {
                        _FileInfo = Media.FirmwareInfo.FromFile(value);
//                        if (_FileInfo.Type == DevSupport.Media.FirmwareInfo.FileType.Stmp37xx)
                        {
                            Data = File.ReadAllBytes(value);
                            TransferSize = Data.Length;
                        }
//                        else
//                        {
//                            _FileInfo.FileStatus = "Invalid firmware file type.";
//                            TransferSize = 0;
//                        }
                    }
                    catch (Exception e)
                    {
                        _FileInfo = new DevSupport.Media.FirmwareInfo();
                        _FileInfo.FileStatus = e.Message;
                        TransferSize = 0;
                    }

                    _CDB.ToField("Length", Convert.ToUInt32(TransferSize));
                }
            }
            private String _FwFilename;

            [Category("Status"), Description("The status of the firmware file to download.")]
            [TypeConverter(typeof(ExpandableObjectConverter))]
            public Media.FirmwareInfo FileInfo
            {
                get
                {
                    return _FileInfo;
                }
            }
            private Media.FirmwareInfo _FileInfo;
        }

        //////////////////////////////////////////////////////////////////////
        //
        // ST_HID_PITC_COMMAND: HidPitcRequestSense
        //
        //////////////////////////////////////////////////////////////////////
        public class PitcRequestSense : HidApi
        {
            public enum PitcSenseCode : uint
            {
                NoErrors = 0x00000000,
                InvalidCbw = 0x00000001,
                InvalidCdbCommand = 0x00000002,
                InvalidInquiryPage = 0x00120001,
                NoSenseInfo = 0x00120202,
                InvalidOtpRegister = 0x00120302,
                OtpInfoDenied = 0x00120303,
                InvalidReadAddress = 0x00130010,
                BufferReadOverflow = 0x00130011,
                AccessReadDenied = 0x00130012,
                InvalidWriteAddress = 0x00140010,
                BufferWriteOverflow = 0x00140011,
                AccessWriteDenied = 0x00140012,
                Invalid = 0xC01DF00D
            };

            private PitcSenseCode _SenseCode;

            // Constructor
            public PitcRequestSense()
                : base(CommandSet.PitcRequestSense, CommandType.HidPitcCmd, CommandDirection.ReadWithData, DefaultTimeout)
            {
                // General Init
                TransferSize = sizeof(UInt32);
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, UInt32 start, UInt32 count)
            {
                Debug.Assert(count >= _TransferSize);

                _SenseCode = (PitcSenseCode) BitConverter.ToInt32(data, 0);

                if (Enum.IsDefined(typeof(PitcSenseCode), SenseCode))
                {
                    ResponseString = String.Format(" Sense code: {0} (0x{0:X})\r\n", SenseCode);
                }
                else
                {
                    ResponseString = String.Format(" Sense code: UnknownCode 0x{0:X8} ({0})\r\n", (Int32)SenseCode);
                }
                
                return Win32.ERROR_SUCCESS;
            }

            #region Sense Code Response Properties

            [Category("Response")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public PitcSenseCode SenseCode
            {
                get { return _SenseCode; }
            }

            #endregion
        }

        //////////////////////////////////////////////////////////////////////
        //
        // ST_HID_PITC_COMMAND: HidPitcRequestSense
        //
        //////////////////////////////////////////////////////////////////////
        public class PitcTestUnitReady : HidApi
        {
            // Constructor
            public PitcTestUnitReady()
                : base(CommandSet.PitcTestUnitReady, CommandType.HidPitcCmd, CommandDirection.NoData, DefaultTimeout)
            {
                // General Init
                TransferSize = 0;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // ST_HID_PITC_COMMAND: HidPitcInquiry
        //
        ////////////////////////////////////////////////////////////////////////////////
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class PitcInquiry : HidApi, IFilterProperties
        {
            public enum InfoPageType : byte { Pitc = 0x01, PitcSense = 0x02, OtpReg = 0x03, PersistentReg = 0x04 };

            private uint PageTransferSize()
            {
                uint _PageTransferSize = 0;

                switch (InfoPage)
                {
                    case InfoPageType.Pitc:
                        _PageTransferSize = (UInt32)Marshal.SizeOf(_PitcInfo);
                        break;
                    case InfoPageType.PitcSense:
                        _PageTransferSize = 64; // Spec says this is a 64-byte buffer containing a Unicode string.
                        break;
                    case InfoPageType.OtpReg:
                        _PageTransferSize = (UInt32)Marshal.SizeOf(_OtpRegInfo);
                        break;
                    case InfoPageType.PersistentReg:
                        _PageTransferSize = (UInt32)Marshal.SizeOf(_PersistentRegInfo);
                        break;
                    default:
                        _PageTransferSize = 0;
                        break;
                }

                return _PageTransferSize;
            }
            // Constructor
            public PitcInquiry(InfoPageType infoPage, UInt32 infoParam)
                : base(CommandSet.PitcInquiry, CommandType.HidPitcCmd, CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("InfoPage", infoPage);
                _CDB.Add("InfoParam", infoParam);

                // General Init
                TransferSize = PageTransferSize();
            }

            [Description("The type of info to get."), Category("Parameters")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public InfoPageType InfoPage
            {
                get { return (InfoPageType)_CDB.FromField("InfoPage"); }
                set
                {
                    _CDB.ToField("InfoPage", value);
                    TransferSize = PageTransferSize();
                }
            }
            [Description("Varies by InfoPage. (UInt32)"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 InfoParam
            {
                get { return (UInt32)_CDB.FromField("InfoParam"); }
                set { _CDB.ToField("InfoParam", value); }
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, UInt32 start, UInt32 count)
            {
                Debug.Assert(count >= _TransferSize);

                switch (InfoPage)
                {
                    case InfoPageType.Pitc:
                        Array.Reverse(data, 0, sizeof(UInt32));
                        _PitcInfo.Id = BitConverter.ToUInt32(data, 0);
                        Array.Reverse(data, 4, sizeof(UInt16));
                        _PitcInfo.VersionMajor = BitConverter.ToUInt16(data, 4);
                        Array.Reverse(data, 6, sizeof(UInt16));
                        _PitcInfo.VersionMinor = BitConverter.ToUInt16(data, 6);
                        Array.Reverse(data, 8, sizeof(UInt16));
                        _PitcInfo.VersionRevision = BitConverter.ToUInt16(data, 8);
                        break;
                    case InfoPageType.PitcSense:
                        _PitcStatus = Utils.Utils.StringFromUnicodeBytes(data);
                        break;
                    case InfoPageType.OtpReg:
                        Array.Reverse(data, 0, sizeof(UInt32));
                        _OtpRegInfo.Address = BitConverter.ToUInt32(data, 0);
                        _OtpRegInfo.LockBit = data[4];
                        _OtpRegInfo.OtpBank = data[5];
                        _OtpRegInfo.OtpWord = data[6];
                        _OtpRegInfo.Locked = data[7];
                        _OtpRegInfo.Shadowed = data[8];
                        break;
                    case InfoPageType.PersistentReg:
                        _PersistentRegInfo.Address  = BitConverter.ToUInt32(data, 0);
                        _PersistentRegInfo.Value  = BitConverter.ToUInt32(data, 4);
                        break;
                    default:
                        Debug.Assert(false, InfoPage + " is not a valid enumeration value to pass.");
                        return Win32.ERROR_INVALID_PARAMETER;
                        break;

                }

                return Win32.ERROR_SUCCESS;
            }

            String _PitcStatus = String.Empty;

            #region Pitc Response Properties

            [StructLayout(LayoutKind.Sequential, Pack = 2)]
            private struct PitcInfo
            {
                public UInt32 Id;
                public UInt16 VersionMajor;
                public UInt16 VersionMinor;
                public UInt16 VersionRevision;
            }
            private PitcInfo _PitcInfo;

            [Category("Response"), BrowsableResponse(InfoPageType.Pitc)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 Id
            {
                get
                {
                    if (InfoPage != InfoPageType.Pitc)
                        return 0;

                    return _PitcInfo.Id;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.Pitc)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 VersionMajor
            {
                get
                {
                    if (InfoPage != InfoPageType.Pitc)
                        return 0;

                    return _PitcInfo.VersionMajor;
                }
            }
            [Category("Response"), BrowsableResponse(InfoPageType.Pitc)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 VersionMinor
            {
                get
                {
                    if (InfoPage != InfoPageType.Pitc)
                        return 0;

                    return _PitcInfo.VersionMinor;
                }
            }
            [Category("Response"), BrowsableResponse(InfoPageType.Pitc)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 VersionRevision
            {
                get
                {
                    if (InfoPage != InfoPageType.Pitc)
                        return 0;

                    return _PitcInfo.VersionRevision;
                }
            }
            #endregion
            
            #region OtpRegInfo Response Properties

            [StructLayout(LayoutKind.Sequential, Pack = 2)]
            private struct OtpRegInfoPage
            {
                public UInt32 Address;
                public Byte LockBit;
                public Byte OtpBank;
                public Byte OtpWord;
                public Byte Locked;
                public Byte Shadowed;
            }
            private OtpRegInfoPage _OtpRegInfo;

            [Category("Response"), BrowsableResponse(InfoPageType.OtpReg)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 OtpAddress
            {
                get
                {
                    if (InfoPage != InfoPageType.OtpReg)
                        return 0;

                    return _OtpRegInfo.Address;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.OtpReg)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte LockBit
            {
                get
                {
                    if (InfoPage != InfoPageType.OtpReg)
                        return 0;

                    return _OtpRegInfo.LockBit;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.OtpReg)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte OtpBank
            {
                get
                {
                    if (InfoPage != InfoPageType.OtpReg)
                        return 0;

                    return _OtpRegInfo.OtpBank;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.OtpReg)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte OtpWord
            {
                get
                {
                    if (InfoPage != InfoPageType.OtpReg)
                        return 0;

                    return _OtpRegInfo.OtpWord;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.OtpReg)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte Locked
            {
                get
                {
                    if (InfoPage != InfoPageType.OtpReg)
                        return 0;

                    return _OtpRegInfo.Locked;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.OtpReg)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte Shadowed
            {
                get
                {
                    if (InfoPage != InfoPageType.OtpReg)
                        return 0;

                    return _OtpRegInfo.Shadowed;
                }
            }
            #endregion

            #region PersistentRegInfo Response Properties

            [StructLayout(LayoutKind.Sequential, Pack = 2)]
            private struct PersistentRegInfoPage
            {
                public UInt32 Address;
                public UInt32 Value;
            }
            private PersistentRegInfoPage _PersistentRegInfo;

            [Category("Response"), BrowsableResponse(InfoPageType.PersistentReg)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 PersAddress
            {
                get
                {
                    if (InfoPage != InfoPageType.PersistentReg)
                        return 0;

                    return _PersistentRegInfo.Address;
                }
            }

            [Category("Response"), BrowsableResponse(InfoPageType.PersistentReg)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 Value
            {
                get
                {
                    if (InfoPage != InfoPageType.PersistentReg)
                        return 0;

                    return _PersistentRegInfo.Value;
                }
            }
            #endregion

            #region PitcStatus Response Properties

            [Category("Response"), BrowsableResponse(InfoPageType.PitcSense)]
            //[TypeConverter(typeof(Utils.xxx))]
            public String PitcStatus
            {
                get
                {
                    if (InfoPage != InfoPageType.PitcSense)
                        return String.Empty;

                    return _PitcStatus ;
                }
            }
            #endregion

            #region IFilterProperties Members

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                return ((InfoPageType)responseAttribute.Criteria == InfoPage);
            }

            #endregion
        }

        //////////////////////////////////////////////////////////////////////
        //
        // ST_HID_PITC_COMMAND: HidPitcRead
        //
        //////////////////////////////////////////////////////////////////////
        public class PitcRead : HidApi
        {
            // Constructor
            public PitcRead(UInt32 address, UInt32 length, UInt32 flags, UInt32 datasize)
                : base(CommandSet.PitcRead, CommandType.HidPitcCmd, CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("Address", address);
                _CDB.Add("Length", length);
                _CDB.Add("Flags", flags);

                _datasize = datasize;

                // General Init
                TransferSize = length * datasize;
            }
            private UInt32 _datasize;

            [Description("Address to Read from"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 Address
            {
                get { return (UInt32)_CDB.FromField("Address"); }
                set { _CDB.ToField("Address", value); }
            }
            [Description("Number of data-units to Read"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 Length
            {
                get { return (UInt32)_CDB.FromField("Length"); }
                set
                {
                    _CDB.ToField("Length", value);
                    TransferSize = value * _datasize;
                }
            }
            [Description("Size in bytes of the data-units to Read"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 DataSize
            {
                get { return _datasize; }
                set
                {
                    _datasize = value;
                    TransferSize = Length * value;
                }
            }
            [Description("Flags which control the Read behaviour"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 Flags
            {
                get { return (UInt32)_CDB.FromField("Flags"); }
                set { _CDB.ToField("Flags", value); }
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, UInt32 start, UInt32 count)
            {
                Debug.Assert(count >= _TransferSize);

                Data = new byte[_TransferSize];

                Array.ConstrainedCopy(data, 0, Data, 0, (int)System.Math.Min(count, TransferSize));

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            [Browsable(true)]
            public override byte[] Data
            {
                get
                {
                    return base.Data;
                }
                set
                {
                    base.Data = value;
                }
            }
            [Browsable(false)]
            override public String ResponseString
            {
                get
                {
                    return FormatReadResponse(Data, 16, 1);
                }
            }
            #endregion
        }
        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// PitcWrite
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class PitcWrite : HidApi
        {
            // Constructor
            public PitcWrite(UInt32 address, UInt32 length, UInt32 flags, Byte[] data)
                : base(CommandSet.PitcWrite, CommandType.HidPitcCmd, CommandDirection.WriteWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("Address", address);
                _CDB.Add("Length", length);
                _CDB.Add("Flags", flags);

                Data = data;

                // General Init
                TransferSize = (UInt32) ((null == data) ? 0 : data.Length);
            }

            [Description("Address to Write to"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 Address
            {
                get { return (UInt32)_CDB.FromField("Address"); }
                set { _CDB.ToField("Address", value); }
            }
            [Description("Length of the data to Write"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 Length
            {
                get { return (UInt32)_CDB.FromField("Length"); }
                set
                {
                    _CDB.ToField("Length", value);
                    TransferSize = value;
                }
            }
            [Description("Flags which control the Write behavior"), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 Flags
            {
                get { return (UInt32)_CDB.FromField("Flags"); }
                set { _CDB.ToField("Flags", value); }
            }

            [Description("Data to Write"), Category("Parameters")]
            [Browsable(true)]
            public override byte[] Data
            {
                get
                {
                    return base.Data;
                }
                set
                {
                    base.Data = value;
                }
            }
        }
    };
}
