/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
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
    public abstract class ScsiApi : Api
    {
        protected ScsiApi(Api.CommandType cmdType, Api.CommandDirection dir, UInt32 timeout)
            : base(cmdType, dir, timeout)
        {
            _CDB = new CDB();
            _SenseData = new Win32.SENSE_DATA();
        }

        [Description("The Command Data Block for the api"), Category("Data")]
        public virtual Byte[] Cdb
        {
            get
            {
                return _CDB.ToByteArray();
            }
            set {/* */}
        }
        protected CDB _CDB;

        [Browsable(false)]
        [Description("The tag denoting the instance of the api."), Category("General")]
        [DefaultValueAttribute(0)]
        public UInt32 CbwTag
        {
            get { return _CbwTag; }
            set { _CbwTag = value; }
        }
        protected UInt32 _CbwTag;

        virtual public Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
        {
            return Win32.ERROR_SUCCESS;
        }

        [Browsable(false)]
        [Description("The SCSI Sense Status."), Category("Response")]
        public Win32.ScsiSenseStatus SenseStatus
        {
            get { return _SenseStatus; }
            set { _SenseStatus = value; }
        }
        private Win32.ScsiSenseStatus _SenseStatus;

        [Browsable(false)]
        [Description("The SCSI Sense Data object."), Category("Response")]
        public Win32.SENSE_DATA SenseData
        {
            get { return _SenseData; }
            set { _SenseData = value; }
        }
        private Win32.SENSE_DATA _SenseData;

        public override string ResponseString
        {
            get
            {
                if (String.IsNullOrEmpty(base.ResponseString) || SenseStatus != Win32.ScsiSenseStatus.GOOD)
                {
                    Utils.EnumConverterEx stsCvtr = new Utils.EnumConverterEx(typeof(Win32.ScsiSenseStatus));

                    base.ResponseString = "SCSI Status: " + stsCvtr.ConvertToString(SenseStatus);

                    if (SenseData.Valid)
                        base.ResponseString += "\r\nSCSI Sense Data:\r\n" + SenseData.ToString();
                }
                return base.ResponseString;
            }
        }

    } // class ScsiApi

    public class ScsiVendorApi : ScsiApi
    {
        /// <summary>
        /// The StScsi Command Operations. Byte[0] of the CDB.
        /// </summary>
        public enum CommandOp : byte
        {
            ReadOp = 0xC0,
            WriteOp = 0xC1
        }

        /// <summary>
        /// Scsi Vendor-specific Command Set. 
        /// Byte[1] of the CDB for Vendor-specific SCSI commands.
        /// </summary>
        public enum CommandSet : byte
        {
            GetProtocolVersion = 0x00,
            GetStatus = 0x01,
            GetLogicalMediaInfo = 0x02,
            GetAllocationTable = 0x05,
            SetAllocationTable = 0x06,
            EraseLogicalMedia = 0x07,
            GetLogicalDriveInfo = 0x12,
            ReadLogicalDriveSector = 0x13,
            SetLogicalDriveInfo = 0x20,
            WriteLogicalDriveSector = 0x23,
            EraseLogicalDrive = 0x2f,
            GetChipMajorRevId = 0x30,
            ChipReset = 0x31,
            GetChipSerialNumberInfo = 0x32, // ProtocolUpdater2, ProtocolUpdaterJanus2
            FlushLogicalDrive = 0x33, //TODO: add DDI_FLUSH_LOGICAL_DRIVE
            GetPhysicalMediaInfo = 0x34, //TODO: add DDI_GET_PHYSICAL_MEDIA_INFO // response size == 8(words)*3Bytes == 24 bytes
            ReadPhysicalMediaSector = 0x35, //TODO: add DDI_READ_PHYSICAL_MEDIA_SECTOR
            GetChipPartRevId = 0x36,
            GetRomRevId = 0x37,
            WriteLogicalDriveSector512 = 0x38,
            GetJanusStatus = 0x40,
            InitializeJanus = 0x41,
            ResetToRecovery = 0x42,
            InitializeDataStore = 0x43,
            ResetToUpdater = 0x44,
            GetDeviceProperties = 0x45, // ProtocolUpdater2, ProtocolUpdaterJanus2
            SetUpdateFlag = 0x46,
            FilterPing = 0x9F,
        }

        /// <summary>
        /// Types of info obtained from the GetLogicalMediaInfo api.
        /// </summary>
        public enum LogicalMediaInfo : byte
        {
            NumberOfDrives = 0,
            SizeInBytes = 1,
            AllocationUnitSizeInBytes = 2,
            IsInitialized = 3,
            MediaState = 4,
            IsWriteProtected = 5,
            PhysicalMediaType = 6,
            SizeOfSerialNumberInBytes = 7,
            SerialNumber = 8,
            IsSystemMedia = 9,
            IsMediaPresent = 10,
            // ProtocolUpdater2, ProtocolUpdaterJanus2 - 2048 typical value   2Kbyte page size
            NandPageSizeInBytes = 11,
            // ProtocolUpdater2, ProtocolUpdaterJanus2 - Cmd only sent if media type is nand. 1 byte like 0xec for samsung.
            NandManufacturer = 12,
            // ProtocolUpdater2, ProtocolUpdaterJanus2 - Cmd only sent if media type is nand.
            //     Id details for nand include remaining 4 of 5 byte nand HW read id hw cmd.
            NandDetails = 13,
            // ProtocolUpdater2, ProtocolUpdaterJanus2 - num CE discovered at driver init time (up to		    
            //     build option max supported num CE)
            NandChipEnables = 14
        }

        public enum NandManufacturerId : byte
        {
            Unknown  = 0x00,
            Renesas  = 0x07,
            StMicro  = 0x20,
            Micron   = 0x2C,
//            Intel    = 0x2C,
            Sandisk  = 0x45,
            Intel    = 0x89,
            Toshiba  = 0x98,
            MSystems = 0x98,
            Hynix    = 0xAD,
            Samsung  = 0xEC
        }

        public enum MediaTypes
        {
            Nand = 0,
            MMC = 1,
            HDD = 2,
            RAM = 3,
            INand = 4 // future
        }

        public enum MediaStates
        {
            MediaStateUnknown,
            MediaStateErased,
            MediaStateAllocated
        }

        /// <summary>
        /// Determines whether DriveNumber or DriveTag is used for a given api.
        /// </summary>
        public enum DriveMechanism { Tag, Number }
        public DriveMechanism _DriveMechanism;

        /// <summary>
        /// Used with Get/SetLogicalDriveInfo
        /// </summary>
        public enum LogicalDriveInfo : byte
        {
            SectorSizeInBytes = 0,
            EraseSizeInBytes = 1,
            SizeInBytes = 2,
            SizeInMegaBytes = 3,
            SizeInSectors = 4,
            Type = 5,
            Tag = 6,
            ComponentVersion = 7,
            ProjectVersion = 8,
            IsWriteProtected = 9,
            SizeOfSerialNumberInBytes = 10,
            SerialNumber = 11,
            IsMediaPresent = 12,
            MediaChange = 13
        }

        /// <summary>
        /// Used with GetProtocolVersion.
        /// </summary>
        public enum ProtocolMajorVersion
        {
            Invalid = 0,
            ProtocolUpdater = 1,  // full featured updater (pre SDK3.0)
            ProtocolLimitedHostlink = 2,  // limited hostlink capabilities(not all vendor cmds supported; cannot update f/w
            ProtocolUpdater2 = 3,  // normal updater without advanced scsi functions
            ProtocolUpdaterJanus = 4,  // full featured updater with Janus support
            ProtocolUpdaterJanus2 = 5,  // add advanced scsi functions
            ProtocolUpdaterJanus3 = 6	  // SDK5 Janus init changes
        }

        protected ScsiVendorApi(ScsiVendorApi.CommandSet cmd, Api.CommandType cmdType, Api.CommandDirection dir, UInt32 timeout)
            : base(cmdType, dir, timeout)
        {
            _CDB.Add("Operation", (dir == Api.CommandDirection.WriteWithData) ? ScsiVendorApi.CommandOp.WriteOp : ScsiVendorApi.CommandOp.ReadOp);
            _CDB.Add("Command", cmd);
        }

        [Description("The Operation byte for the api."), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        virtual public ScsiVendorApi.CommandOp Operation
        {
            get { return (ScsiVendorApi.CommandOp)_CDB.FromField("Operation"); }
            private set { _CDB.ToField("Operation", value); }
        }

        [Description("The Command byte for the api."), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public ScsiVendorApi.CommandSet Command
        {
            get { return (ScsiVendorApi.CommandSet)_CDB.FromField("Command"); }
            private set { _CDB.ToField("Command", value); }
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetProtocolVersion
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetProtocolVersion : ScsiVendorApi
        {
            // Parameters

            // Constructor
            public GetProtocolVersion()
                : base(ScsiVendorApi.CommandSet.GetProtocolVersion, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                TransferSize = sizeof(Byte)/*Major*/ + sizeof(Byte)/*Minor*/;
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= _TransferSize);

                _Major = data[0];
                _Minor = data[1];

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            public Byte Major
            {
                get { return _Major; }
            }
            private Byte _Major;


            [Category("Response")]
            public Byte Minor
            {
                get { return _Minor; }
            }
            private Byte _Minor;

//            [Category("Response")]
            [Browsable(false)]
            override public String ResponseString
            {
                get
                {
                    if (SenseStatus == Win32.ScsiSenseStatus.GOOD)
                    {
                        switch ((ProtocolMajorVersion)Major)
                        {
                            case ProtocolMajorVersion.ProtocolUpdater:
                                _ResponseString = String.Format("({1}.{2}) {0}\r\n  - Full-featured updater (pre SDK3.0). Does not include Janus support.\r\n", (ProtocolMajorVersion)Major, Major, Minor);
                                break;
                            case ProtocolMajorVersion.ProtocolLimitedHostlink:
                                _ResponseString = String.Format("({1}.{2}) {0}\r\n  - Limited hostlink capabilities. Not all vendor cmds supported. Can not update f/w.\r\n", (ProtocolMajorVersion)Major, Major, Minor);
                                break;
                            case ProtocolMajorVersion.ProtocolUpdater2:
                                _ResponseString = String.Format("({1}.{2}) {0}\r\n  - Extends ProtocolUpdater(1) to include DDI_GET_DEVICE_PROPERTIES. Does not include Janus support.\r\n", (ProtocolMajorVersion)Major, Major, Minor);
                                break;
                            case ProtocolMajorVersion.ProtocolUpdaterJanus:
                                _ResponseString = String.Format("({1}.{2}) {0}\r\n  - Extends ProtocolUpdater(1) to include Janus support.\r\n", (ProtocolMajorVersion)Major, Major, Minor);
                                break;
                            case ProtocolMajorVersion.ProtocolUpdaterJanus2:
                                _ResponseString = String.Format("({1}.{2}) {0}\r\n  - Extends ProtocolUpdaterJanus(4) to include DDI_GET_DEVICE_PROPERTIES.\r\n", (ProtocolMajorVersion)Major, Major, Minor);
                                break;
                            case ProtocolMajorVersion.ProtocolUpdaterJanus3:
                                if (Minor == 0)
                                    _ResponseString = String.Format("({1}.{2}) {0} (MSC)\r\n  - Extends ProtocolUpdaterJanus2(5) to include SDK5 Janus init changes, but running in MSC-mode.\r\n", (ProtocolMajorVersion)Major, Major, Minor);
                                else
                                    _ResponseString = String.Format("({1}.{2}) {0}\r\n  - Extends ProtocolUpdaterJanus2(5) to include SDK5 Janus init changes.\r\n", (ProtocolMajorVersion)Major, Major, Minor);
                                break;
                            default:
                                _ResponseString = String.Format("({0}.{1}) Unknown Protocol\r\n", Major, Minor);
                                break;
                        }
                        /* list all the protocols			
                        _responseStr += _T("\r\n\r\nProtocolUpdaterJanus2 (5.x):\r\n    Extends ProtocolUpdaterJanus to include DDI_GET_DEVICE_PROPERTIES.\r\n");
                        _responseStr += _T("\r\nProtocolUpdaterJanus (4.x):\r\n    Extends ProtocolUpdater to include Janus support.\r\n");
                        _responseStr += _T("\r\nProtocolUpdater2 (3.x):\r\n    Extends ProtocolUpdater to include DDI_GET_DEVICE_PROPERTIES.\r\n    Does not include Janus support.\r\n");
                        _responseStr += _T("\r\nProtocolLimetedHostlink (2.x):\r\n    Limited hostlink capabilities.\r\n    Not all vendor cmds supported.\r\n    Can not update f/w.\r\n");
                        _responseStr += _T("\r\nProtocolUpdater (1.x):\r\n    Full-featured updater (pre SDK3.0).\r\n    Does not include Janus support.\r\n");
                        */
                        return _ResponseString;
                    }
                    else
                        return base.ResponseString;
                }
            }

            #endregion // Response

        } // class GetProtocolVersion

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetStatus
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetStatus : ScsiVendorApi
        {
            // Parameters

            // Constructor
            public GetStatus()
                : base(ScsiVendorApi.CommandSet.GetStatus, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                TransferSize = sizeof(UInt16/*Status*/);
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                Array.Reverse(data);

                _Status = BitConverter.ToUInt16(data, 0);
                _ResponseString = String.Format(" Chip status: 0x{0:X4}\r\n", Status);

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 Status
            {
                get { return _Status; }
            }
            private UInt16 _Status;

            #endregion // Response

        } // class GetStatus

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetLogicalMediaInfo
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class GetLogicalMediaInfo : ScsiVendorApi, IFilterProperties
        {
            // Constructor
            public GetLogicalMediaInfo(ScsiVendorApi.LogicalMediaInfo infoType, UInt32 serialNumberSize)
                : base(ScsiVendorApi.CommandSet.GetLogicalMediaInfo, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("InfoType", infoType);

                SerialNumberSize = serialNumberSize;
                TransferSize = GetResponseSize(infoType, serialNumberSize);
            }

            // Parameters
            [Description("The type of info to get."), Category("Parameters")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public ScsiVendorApi.LogicalMediaInfo InfoType
            {
                get { return (ScsiVendorApi.LogicalMediaInfo)_CDB.FromField("InfoType"); }
                set
                {
                    _CDB.ToField("InfoType", value);
                    TransferSize = GetResponseSize(value, SerialNumberSize);
                }
            }

            [Description("Only used for InfoType = SerialNumber. Specifies the size of the return buffer. Call GetLogicalMediaInfo(SizeOfSerialNumberInBytes,0) first to get this parameter."), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SerialNumberSize
            {
                get { return _SerialNumberSize; }
                set { _SerialNumberSize = value; }
            }
            private UInt32 _SerialNumberSize;

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                switch ((ScsiVendorApi.LogicalMediaInfo)_CDB.FromField("InfoType"))
                {
                    case LogicalMediaInfo.NumberOfDrives:
                        {
                            Array.Reverse(data);
                            NumberOfDrives = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Number of drives: {0}\r\n", NumberOfDrives);
                            break;
                        }
                    case LogicalMediaInfo.SizeInBytes:
                        {
                            Array.Reverse(data);
                            SizeInBytes = BitConverter.ToUInt64(data, 0);
                            ResponseString = String.Format(" Media size: {0}, {1} bytes\r\n", Utils.Utils.ScaleBytes(SizeInBytes), SizeInBytes);
                            break;
                        }
                    case LogicalMediaInfo.AllocationUnitSizeInBytes:
                        {
                            Array.Reverse(data);
                            AllocationUnitSizeInBytes = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Allocation Unit size: 0x{0:X} ({0}) bytes\r\n", AllocationUnitSizeInBytes);
                            break;
                        }
                    case LogicalMediaInfo.IsInitialized:
                        {
                            IsInitialized = Convert.ToBoolean(data[0]);
                            ResponseString = String.Format(" Is Initialized: {0}\r\n", IsInitialized);
                            break;
                        }
                    case LogicalMediaInfo.MediaState:
                        {
                            Array.Reverse(data);
                            MediaState = (MediaStates)BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Media state: 0x{0:X} ({0})\r\n", MediaState);
                            break;
                        }
                    case LogicalMediaInfo.IsWriteProtected:
                        {
                            IsInitialized = Convert.ToBoolean(data[0]);
                            ResponseString = String.Format(" Write protected: {0}\r\n", IsWriteProtected);
                            break;
                        }
                    case LogicalMediaInfo.PhysicalMediaType:
                        {
                            Array.Reverse(data);
                            MediaType = (MediaTypes)BitConverter.ToUInt32(data, 0);

                            Utils.EnumConverterEx converter = new Utils.EnumConverterEx(typeof(MediaTypes));
                            ResponseString = " Physical media type: " + converter.ConvertToString(MediaType) + "\r\n";
                            break;
                        }
                    case LogicalMediaInfo.SizeOfSerialNumberInBytes:
                        {
                            Array.Reverse(data);
                            SizeOfSerialNumberInBytes = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Serial number size: 0x{0:X} ({0})\r\n", SizeOfSerialNumberInBytes);
                            break;
                        }
                    case LogicalMediaInfo.SerialNumber:
                        {
                            _SerialNumber = Utils.Utils.StringFromAsciiBytes(data);

                            ResponseString = " Media serial number: " + SerialNumber + "\r\n";

                            break;
                        }
                    case LogicalMediaInfo.IsSystemMedia:
                        {
                            IsSystemMedia = Convert.ToBoolean(data[0]);
                            ResponseString = String.Format(" Is System media: {0}\r\n", IsSystemMedia);
                            break;
                        }
                    case LogicalMediaInfo.IsMediaPresent:
                        {
                            IsMediaPresent = Convert.ToBoolean(data[0]);
                            ResponseString = String.Format(" Is media present: {0}\r\n", IsMediaPresent);
                            break;
                        }
                    case LogicalMediaInfo.NandPageSizeInBytes:
                        {
                            Array.Reverse(data);
                            NandPageSizeInBytes = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" NAND page size: 0x{0:X} ({0})\r\n", NandPageSizeInBytes);
                            break;
                        }
                    case LogicalMediaInfo.NandManufacturer:
                        {
                            Array.Reverse(data);
                            NandManufacturer = (NandManufacturerId)BitConverter.ToInt32(data, 0);
                            ResponseString = String.Format(" Nand Manufacturer: {0} 0x{1:X6}({2:X2})\r\n",
                                NandManufacturer.ToString(),
                                (((Int32)NandManufacturer) & 0xFFFFFF00)>>8,
                                (Byte)NandManufacturer);
                            break;
                        }
                    case LogicalMediaInfo.NandDetails:
                        {
                            UInt64 details = BitConverter.ToUInt64(data, 0);
                            // Display only top 6 bytes as the lower two bytes are always 0.
                            NandDetails = String.Format("{0:X2}.{1:X2}.{2:X8}",
                                data[7], data[6], BitConverter.ToInt32(data, 2));

                            // Get cell type from bits 2,3 in third byte
		                    int cellType = data[5] >> 2 & 0x03;
                            NandDetails += (cellType == 0) ? " (SLC)" : 
                                String.Format(" (MLC<{0}>)", cellType << 1);

                            ResponseString = " Nand details: " + NandDetails + "\r\n";
                    		
                            break;
                        }
                    case LogicalMediaInfo.NandChipEnables:
                        {
                            Array.Reverse(data);
                            NandChipEnables = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Number NAND chip enables: 0x{0:X} ({0})\r\n", NandChipEnables);
                            break;
                        }
                    default:
                        ResponseString = "Invalid GetLogicalMediaInfo InfoType.";
                        break;
                }
            
                return Win32.ERROR_SUCCESS;
            
            } // ProcessResponse()

            // Helper functions
            private UInt32 GetResponseSize(ScsiVendorApi.LogicalMediaInfo infoType, UInt32 serialNumberSize)
            {
                UInt32 responseSize = 0;

                switch (infoType)
                {
                    case LogicalMediaInfo.NumberOfDrives:
                    case LogicalMediaInfo.MediaState:
                    case LogicalMediaInfo.SizeOfSerialNumberInBytes:
                    case LogicalMediaInfo.PhysicalMediaType:
                    case LogicalMediaInfo.AllocationUnitSizeInBytes:
                    case LogicalMediaInfo.NandPageSizeInBytes:
                    case LogicalMediaInfo.NandManufacturer:
                    case LogicalMediaInfo.NandChipEnables:
                        responseSize = sizeof(UInt32);
                        break;
                    case LogicalMediaInfo.IsInitialized:
                    case LogicalMediaInfo.IsWriteProtected:
                    case LogicalMediaInfo.IsSystemMedia:
                    case LogicalMediaInfo.IsMediaPresent:
                        responseSize = sizeof(Byte);
                        break;
                    case LogicalMediaInfo.SizeInBytes:
                    case LogicalMediaInfo.NandDetails:
                        responseSize = sizeof(UInt64);
                        break;
                    case LogicalMediaInfo.SerialNumber:
                    default:
                        responseSize = serialNumberSize;
                        break;
                }

                return responseSize;
            }

            #region Response Properties

            #region IFilterProperties Members

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                return ((LogicalMediaInfo)responseAttribute.Criteria == InfoType);
            }

            #endregion

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.NumberOfDrives)]
            public UInt32 NumberOfDrives
            {
                get { return _NumberOfDrives; }
                private set { _NumberOfDrives = value; }
            }
            private UInt32 _NumberOfDrives;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.SizeInBytes)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt64 SizeInBytes
            {
                get { return _SizeInBytes; }
                private set { _SizeInBytes = value; }
            }
            private UInt64 _SizeInBytes;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.AllocationUnitSizeInBytes)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt32 AllocationUnitSizeInBytes
            {
                get { return _AllocationUnitSizeInBytes; }
                private set { _AllocationUnitSizeInBytes = value; }
            }
            private UInt32 _AllocationUnitSizeInBytes;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.IsInitialized)]
            public bool IsInitialized
            {
                get { return _IsInitialized; }
                private set { _IsInitialized = value; }
            }
            private bool _IsInitialized;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.MediaState)]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public ScsiVendorApi.MediaStates MediaState
            {
                get { return _MediaState; }
                private set { _MediaState = value; }
            }
            private ScsiVendorApi.MediaStates _MediaState;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.IsWriteProtected)]
            public bool IsWriteProtected
            {
                get { return _IsWriteProtected; }
                private set { _IsWriteProtected = value; }
            }
            private bool _IsWriteProtected;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.PhysicalMediaType)]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public MediaTypes MediaType
            {
                get { return _MediaType; }
                private set { _MediaType = value; }
            }
            private MediaTypes _MediaType;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.SizeOfSerialNumberInBytes)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt32 SizeOfSerialNumberInBytes
            {
                get { return _SizeOfSerialNumberInBytes; }
                private set { _SizeOfSerialNumberInBytes = value; }
            }
            private UInt32 _SizeOfSerialNumberInBytes;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.SerialNumber)]
            public String SerialNumber
            {
                get { return _SerialNumber; }
            }
            private String _SerialNumber;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.IsSystemMedia)]
            public bool IsSystemMedia
            {
                get { return _IsSystemMedia; }
                private set { _IsSystemMedia = value; }
            }
            private bool _IsSystemMedia;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.IsMediaPresent)]
            public bool IsMediaPresent
            {
                get { return _IsMediaPresent; }
                private set { _IsMediaPresent = value; }
            }
            private bool _IsMediaPresent;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.NandPageSizeInBytes)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 NandPageSizeInBytes
            {
                get { return _NandPageSizeInBytes; }
                private set { _NandPageSizeInBytes = value; }
            }
            private UInt32 _NandPageSizeInBytes;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.NandManufacturer)]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public NandManufacturerId NandManufacturer
            {
                get { return _NandManufacturer; }
                private set { _NandManufacturer = value; }
            }
            private NandManufacturerId _NandManufacturer;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.NandDetails)]
            public String NandDetails
            {
                get { return _NandDetails; }
                private set { _NandDetails = value; }
            }
            private String _NandDetails;

            [Category("Response"), BrowsableResponse(LogicalMediaInfo.NandChipEnables)]
            public UInt32 NandChipEnables
            {
                get { return _NandChipEnables; }
                private set { _NandChipEnables = value; }
            }
            private UInt32 _NandChipEnables;

            #endregion // Response

        } // class GetLogicalMediaInfo

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetAllocationTable
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [Obsolete("Sector Size is now returned instead of Drive Numbers. Please use the GetAllocationTableEx api.")]
        public class GetAllocationTable : ScsiVendorApi
        {
            public const byte DefaultMediaTableEntries = 20; // 20 - Default number of Media Table Entries
            
            // Constructors
            public GetAllocationTable(Byte numEntries)
                : base(ScsiVendorApi.CommandSet.GetAllocationTable, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("NumberOfEntries", numEntries);

                TransferSize = sizeof(Int16) + (numEntries * (uint)Marshal.SizeOf(typeof(MediaAllocationEntry)));
            }

            public GetAllocationTable()
                : this(DefaultMediaTableEntries)
            {}

            // Parameters
            [Description("The number of drives in the table."), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte NumberOfEntries
            {
                get { return (Byte)_CDB.FromField("NumberOfEntries"); }
                set
                {
                    _CDB.ToField("NumberOfEntries", value);
                    TransferSize = sizeof(Int16) + (value * (uint)Marshal.SizeOf(typeof(MediaAllocationEntry)));
                }
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                Array.Reverse(data, 0, sizeof(Int16));
                NumReturnedEntries = BitConverter.ToUInt16(data, 0);
                ResponseString = " Number of returned entries: " + NumReturnedEntries + "\r\n";

                DriveEntryArray = new MediaAllocationEntry[NumReturnedEntries];
                int offset = sizeof(Int16);
                for (uint i = 0; i < NumReturnedEntries; ++i)
                {
                    Array.Reverse(data, offset + 3, sizeof(Int64));
                    _DriveEntryArray[i] = new MediaAllocationEntry(data, ref offset);
                    ResponseString += " " + _DriveEntryArray[i] + "\r\n";
                }

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Description("The number of drives allocated on the media."), Category("Response")]
            public UInt16 NumReturnedEntries
            {
                get { return _NumReturnedEntries; }
                private set { _NumReturnedEntries = value; }
            }
            private UInt16 _NumReturnedEntries;

            [Description("The details of all drives allocated on the media."), Category("Response")]
            public MediaAllocationEntry[] DriveEntryArray
            {
                get
                {
                    if ( _DriveEntryArray == null )
                        return null;
                    // return a copy to protect our readonly data
                    MediaAllocationEntry[] entriesCopy = new MediaAllocationEntry[_DriveEntryArray.Length];
                    int index = 0;
                    foreach (MediaAllocationEntry entry in _DriveEntryArray)
                    {
                        entriesCopy[index++] = new MediaAllocationEntry(entry.DriveNumber, entry.Type, entry.Tag, entry.SizeInBytes);
                    }
                    return entriesCopy;
                }
                private set { _DriveEntryArray = value; }
            }
            private MediaAllocationEntry[] _DriveEntryArray;

            #endregion // Response

            public MediaAllocationEntry GetEntry(LogicalDrive.Tag tag)
            {
                foreach (MediaAllocationEntry entry in DriveEntryArray)
                {
                    if (entry.Tag == tag)
                        return entry;
                }

                return new MediaAllocationEntry();
            }

        } // class GetAllocationTable

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetAllocationTableEx
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetAllocationTableEx : ScsiVendorApi
        {
            public const byte DefaultMediaTableEntries = 20; // 20 - Default number of Media Table Entries

            // Constructors
            public GetAllocationTableEx(Byte numEntries)
                : base(ScsiVendorApi.CommandSet.GetAllocationTable, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("NumberOfEntries", numEntries);

                TransferSize = sizeof(Int16) + (numEntries * (uint)Marshal.SizeOf(typeof(MediaAllocationEntryEx)));
            }

            public GetAllocationTableEx()
                : this(DefaultMediaTableEntries)
            {}

            // Parameters
            [Description("The number of drives in the table."), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte NumberOfEntries
            {
                get { return (Byte)_CDB.FromField("NumberOfEntries"); }
                set
                {
                    _CDB.ToField("NumberOfEntries", value);
                    TransferSize = sizeof(Int16) + (value * (uint)Marshal.SizeOf(typeof(MediaAllocationEntryEx)));
                }
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                Array.Reverse(data, 0, sizeof(Int16));
                NumReturnedEntries = BitConverter.ToUInt16(data, 0);
                ResponseString = " Number of returned entries: " + NumReturnedEntries + "\r\n";

                DriveEntryArray = new MediaAllocationEntryEx[NumReturnedEntries];
                int offset = sizeof(Int16);
                for (uint i = 0; i < NumReturnedEntries; ++i)
                {
                    Array.Reverse(data, offset + 2, sizeof(UInt64));
                    Array.Reverse(data, offset + 2 + 8, sizeof(UInt32));
                    _DriveEntryArray[i] = new MediaAllocationEntryEx(data, ref offset);
                    ResponseString += " " + _DriveEntryArray[i] + "\r\n";
                }

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Description("The number of drives allocated on the media."), Category("Response")]
            public UInt16 NumReturnedEntries
            {
                get { return _NumReturnedEntries; }
                private set { _NumReturnedEntries = value; }
            }
            private UInt16 _NumReturnedEntries;

            [Description("The details of all drives allocated on the media."), Category("Response")]
            public MediaAllocationEntryEx[] DriveEntryArray
            {
                get
                {
                    if (_DriveEntryArray == null)
                        return null;
                    // return a copy to protect our readonly data
                    MediaAllocationEntryEx[] entriesCopy = new MediaAllocationEntryEx[_DriveEntryArray.Length];
                    int index = 0;
                    foreach (MediaAllocationEntryEx entry in _DriveEntryArray)
                    {
                        entriesCopy[index++] = new MediaAllocationEntryEx(entry.Type, entry.Tag, entry.Size, entry.SectorSize);
                    }
                    return entriesCopy;
                }
                private set { _DriveEntryArray = value; }
            }
            private MediaAllocationEntryEx[] _DriveEntryArray;

            #endregion // Response

            public MediaAllocationEntryEx GetEntry(LogicalDrive.Tag tag)
            {
                foreach (MediaAllocationEntryEx entry in DriveEntryArray)
                {
                    if (entry.Tag == tag)
                        return entry;
                }

                return new MediaAllocationEntryEx();
            }

        } // class GetAllocationTableEx

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// SetAllocationTable
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class SetAllocationTable : ScsiVendorApi
        {
            public const byte DefaultMediaTableEntries = 20; // 20 - Default number of Media Table Entries

            // Constructors
            public SetAllocationTable(MediaAllocationCmdEntry[] entries)
                : base(ScsiVendorApi.CommandSet.SetAllocationTable, Api.CommandType.StScsiCmd, Api.CommandDirection.WriteWithData, ExtendedTimeout)
            {
                // Parameters
                _CDB.Add("NumberOfEntries", (Byte)0);
                DriveEntryArray = entries;

                TransferSize = (uint)DriveEntryArray.Length * (uint)Marshal.SizeOf(typeof(MediaAllocationCmdEntry));
            }

            // Parameters
            [Description("The number of drives in the table."), Category("Parameters")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
//            [RefreshProperties(RefreshProperties.All)]
            public Byte NumberOfEntries
            {
                get { return Convert.ToByte(_CDB.FromField("NumberOfEntries")); }
                private set
                {
                    _CDB.ToField("NumberOfEntries", value);
                    TransferSize = value * (uint)Marshal.SizeOf(typeof(MediaAllocationCmdEntry));
                }
            }

            [Description("The drive entries in the table."), Category("Parameters")]
            public MediaAllocationCmdEntry[] DriveEntryArray
            {
                get 
                {
                    if (_DriveEntryArray == null)
                        _DriveEntryArray = new MediaAllocationCmdEntry[0];
                
                    return _DriveEntryArray;
                }
                
                set
                {
                    try
                    {
                        _DriveEntryArray = new MediaAllocationCmdEntry[value.Length];

                        int nextAvailable = 2;
                        foreach (MediaAllocationCmdEntry entry in value)
                        {
                            if (entry.Tag == LogicalDrive.Tag.FirmwareImg)
                            {
                                _DriveEntryArray[0] = entry;
                                continue;
                            }
                            if (entry.Tag == LogicalDrive.Tag.Data ||
                                entry.Tag == LogicalDrive.Tag.tData)
                            {
                                _DriveEntryArray[1] = entry;
                                continue;
                            }
                            if (_DriveEntryArray.Length >= nextAvailable)
                                _DriveEntryArray[nextAvailable++] = entry;
                        }
                    }
                    catch
                    {
//                        throw new Inva
                    }
                    // update the NumberOfEntries parameter in the CDB
                    NumberOfEntries = (Byte)DriveEntryArray.Length;
                    // put the DriveArray in the Data property
                    int offset = 0;
                    Data = new Byte[DriveEntryArray.Length * Marshal.SizeOf(typeof(MediaAllocationCmdEntry))];
                    foreach (MediaAllocationCmdEntry entry in DriveEntryArray)
                    {
                        Array.Copy(entry.GetBytes(), 0, Data, offset, Marshal.SizeOf(entry));
                        Array.Reverse(Data, offset + 2, sizeof(Int64));
                        offset += Marshal.SizeOf(entry);
                    }
                }
            }
            private MediaAllocationCmdEntry[] _DriveEntryArray;

        } // class SetAllocationTable

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// EraseLogicalMedia
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class EraseLogicalMedia : ScsiVendorApi
        {
            // Constructor
            public EraseLogicalMedia(bool preserveJanus)
                : base(ScsiVendorApi.CommandSet.EraseLogicalMedia, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, EraseMediaTimeout)
            {
                // Parameters
                _CDB.Add("PreserveJanus", Convert.ToByte(preserveJanus));
            }

            // Parameters
            [Description("Set this flag to \"True\" to preserve the Janus partition when erasing the media. Default is \"False\"."), Category("Parameters")]
            public bool PreserveJanus
            {
                get { return Convert.ToBoolean((Byte)_CDB.FromField("PreserveJanus")); }
                set { _CDB.ToField("PreserveJanus", Convert.ToByte(value)); }
            }
        } // class EraseLogicalMedia

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetLogicalDriveInfo
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class GetLogicalDriveInfo : ScsiVendorApi, IFilterProperties
        {
            // Constructor
            [Obsolete("Drive Numbers have been replaced with Drive Tags. Please use the DriveTag constructor.")]
            public GetLogicalDriveInfo(Byte driveNumber, ScsiVendorApi.LogicalDriveInfo infoType, UInt32 serialNumberSize)
                : base(ScsiVendorApi.CommandSet.GetLogicalDriveInfo, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Number;

                // Parameters
                _CDB.Add("DriveNumber", driveNumber);
                _CDB.Add("InfoType", infoType);

                SerialNumberSize = serialNumberSize;
                TransferSize = GetResponseSize(infoType, serialNumberSize);
            }

            // Parameters
            [Category("Parameters"), BrowsableResponse(DriveMechanism.Number), Description("The drive to get info about.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte DriveNumber
            {
                get { return Convert.ToByte(_CDB.FromField("DriveNumber")); }
                set
                {
                    _CDB.ToField("DriveNumber", value);
                }
            }

            // Constructor
            public GetLogicalDriveInfo(LogicalDrive.Tag driveTag, ScsiVendorApi.LogicalDriveInfo infoType, UInt32 serialNumberSize)
                : base(ScsiVendorApi.CommandSet.GetLogicalDriveInfo, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Tag;

                // Parameters
                _CDB.Add("DriveTag", driveTag);
                _CDB.Add("InfoType", infoType);

                SerialNumberSize = serialNumberSize;
                TransferSize = GetResponseSize(infoType, serialNumberSize);
            }

            // Parameters
            [Category("Parameters"), BrowsableResponse(DriveMechanism.Tag), Description("The drive to get info about.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public LogicalDrive.Tag DriveTag
            {
                get { return (LogicalDrive.Tag)_CDB.FromField("DriveTag"); }
                set
                {
                    _CDB.ToField("DriveTag", value);
                }
            }

            [Category("Parameters"), Description("The type of info to get.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public ScsiVendorApi.LogicalDriveInfo InfoType
            {
                get { return (ScsiVendorApi.LogicalDriveInfo)_CDB.FromField("InfoType"); }
                set
                {
                    _CDB.ToField("InfoType", value);
                    TransferSize = GetResponseSize(value, SerialNumberSize);
                }
            }

            [Category("Parameters"), Description("Only used for InfoType = SerialNumber. Specifies the size of the return buffer. Call GetLogicalMediaInfo(SizeOfSerialNumberInBytes,0) first to get this parameter.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SerialNumberSize
            {
                get { return _SerialNumberSize; }
                set { _SerialNumberSize = value; }
            }
            private UInt32 _SerialNumberSize;

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                switch ((ScsiVendorApi.LogicalDriveInfo)_CDB.FromField("InfoType"))
                {
                    case LogicalDriveInfo.SectorSizeInBytes:
                        {
                            Array.Reverse(data);
                            SectorSize = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Sector size:: {0}, 0x{1:X} ({1}) bytes\r\n", Utils.Utils.ScaleBytes(SectorSize), SectorSize);
                            break;
                        }
                    case LogicalDriveInfo.EraseSizeInBytes:
                        {
                            Array.Reverse(data);
                            EraseSize = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Erase size: {0}, 0x{1:X} ({1}) bytes\r\n", Utils.Utils.ScaleBytes(EraseSize), EraseSize);
                            break;
                        }
                    case LogicalDriveInfo.SizeInBytes:
                        {
                            Array.Reverse(data);
                            Size = BitConverter.ToUInt64(data, 0);
                            ResponseString = String.Format(" Drive size: {0}, 0x{1:X} ({1}) bytes\r\n", Utils.Utils.ScaleBytes(Size), Size);
                            break;
                        }
                    case LogicalDriveInfo.SizeInMegaBytes:
                        {
                            Array.Reverse(data);
                            SizeInMB = BitConverter.ToUInt64(data, 0);
                            ResponseString = String.Format(" Drive size: {0} MB\r\n", SizeInMB);
                            break;
                        }
                    case LogicalDriveInfo.SizeInSectors:
                        {
                            Array.Reverse(data);
                            SizeInSectors = BitConverter.ToUInt64(data, 0);
                            ResponseString = String.Format(" Drive size: 0x{0:X} ({0}) sectors\r\n", SizeInSectors);
                            break;
                        }
                    case LogicalDriveInfo.Type:
                        {
                            Array.Reverse(data);
                            Type = (LogicalDrive.Type)BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Drive type: {0}\r\n", new Utils.EnumConverterEx(typeof(LogicalDrive.Type)).ConvertToString(Type));
                            break;
                        }
                    case LogicalDriveInfo.Tag:
                        {
                            Array.Reverse(data);
                            DriveTag = (LogicalDrive.Tag)BitConverter.ToUInt32(data, 0);

                            Utils.EnumConverterEx converter = new Utils.EnumConverterEx(typeof(LogicalDrive.Tag));
                            ResponseString = String.Format(" Drive tag: {0}\r\n", converter.ConvertToString(DriveTag));
                            break;
                        }
                    case LogicalDriveInfo.ComponentVersion:
                        {
                            ComponentVersion = new FirmwareInfo.Version(data);
                            ResponseString = String.Format(" Component version: {0}\r\n", ComponentVersion);
                            break;
                        }
                    case LogicalDriveInfo.ProjectVersion:
                        {
                            ProjectVersion = new FirmwareInfo.Version(data);
                            ResponseString = String.Format(" Project version: {0}\r\n", ProjectVersion);
                            break;
                        }
                    case LogicalDriveInfo.IsWriteProtected:
                        {
                            IsWriteProtected = Convert.ToBoolean(data[0]);

                            ResponseString = " IsWriteProtected: " + IsWriteProtected + "\r\n";

                            break;
                        }
                    case LogicalDriveInfo.SizeOfSerialNumberInBytes:
                        {
                            Array.Reverse(data);
                            SizeOfSerialNumberInBytes = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Serial number size: 0x{0:X} ({0})\r\n", SizeOfSerialNumberInBytes);
                            break;
                        }
                    case LogicalDriveInfo.SerialNumber:
                        {
                            _SerialNumber = Utils.Utils.StringFromAsciiBytes(data);

                            ResponseString = " Media serial number: " + SerialNumber + "\r\n";

                            break;
                        }
                    case LogicalDriveInfo.IsMediaPresent:
                        {
                            IsMediaPresent = Convert.ToBoolean(data[0]);
                            ResponseString = String.Format(" Is media present: {0}\r\n", IsMediaPresent);
                            break;
                        }
                    case LogicalDriveInfo.MediaChange:
                        {
                            MediaChange = Convert.ToBoolean(data[0]);
                            ResponseString = String.Format(" Media change: {0}\r\n", MediaChange);
                            break;
                        }
                    default:
                        ResponseString = "Invalid GetLogicalDriveInfo InfoType.";
                        break;
                }

                return Win32.ERROR_SUCCESS;

            } // ProcessResponse()

            // Helper functions
            private UInt32 GetResponseSize(ScsiVendorApi.LogicalDriveInfo infoType, UInt32 serialNumberSize)
            {
                UInt32 responseSize = 0;

                switch (infoType)
                {
                    case LogicalDriveInfo.SectorSizeInBytes:
                    case LogicalDriveInfo.EraseSizeInBytes:
                    case LogicalDriveInfo.Type:
                    case LogicalDriveInfo.Tag:
                    case LogicalDriveInfo.SizeOfSerialNumberInBytes:
                        responseSize = sizeof(UInt32);
                        break;
                    case LogicalDriveInfo.IsWriteProtected:
                    case LogicalDriveInfo.MediaChange:
                    case LogicalDriveInfo.IsMediaPresent:
                        responseSize = sizeof(Byte);
                        break;
                    case LogicalDriveInfo.SizeInBytes:
                    case LogicalDriveInfo.SizeInMegaBytes:
                    case LogicalDriveInfo.SizeInSectors:
                        responseSize = sizeof(UInt64);
                        break;
                    case LogicalDriveInfo.ComponentVersion:
                    case LogicalDriveInfo.ProjectVersion:
                        responseSize = 3 * sizeof(UInt16);
                        break;
                    case LogicalDriveInfo.SerialNumber:
                    default:
                        responseSize = serialNumberSize;
                        break;
                }

                return responseSize;
            }

            #region Response Properties

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.SectorSizeInBytes)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt32 SectorSize
            {
                get { return _SectorSize; }
                private set { _SectorSize = value; }
            }
            private UInt32 _SectorSize;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.EraseSizeInBytes)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt32 EraseSize
            {
                get { return _EraseSize; }
                private set { _EraseSize = value; }
            }
            private UInt32 _EraseSize;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.SizeInBytes)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt64 Size
            {
                get { return _Size; }
                private set { _Size = value; }
            }
            private UInt64 _Size;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.SizeInMegaBytes)]
            public UInt64 SizeInMB
            {
                get { return _SizeInMB; }
                private set { _SizeInMB = value; }
            }
            private UInt64 _SizeInMB;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.SizeInSectors)]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt64 SizeInSectors
            {
                get { return _SizeInSectors; }
                private set { _SizeInSectors = value; }
            }
            private UInt64 _SizeInSectors;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.Type)]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public LogicalDrive.Type Type
            {
                get { return _Type; }
                private set { _Type = value; }
            }
            private LogicalDrive.Type _Type;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.Tag)]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public LogicalDrive.Tag Tag
            {
                get { return _Tag; }
                private set { _Tag = value; }
            }
            private LogicalDrive.Tag _Tag;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.ComponentVersion)]
            public FirmwareInfo.Version ComponentVersion
            {
                get { return _ComponentVersion; }
                private set { _ComponentVersion = value; }
            }
            private FirmwareInfo.Version _ComponentVersion;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.ProjectVersion)]
            public FirmwareInfo.Version ProjectVersion
            {
                get { return _ProjectVersion; }
                private set { _ProjectVersion = value; }
            }
            private FirmwareInfo.Version _ProjectVersion;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.IsWriteProtected)]
            public bool IsWriteProtected
            {
                get { return _IsWriteProtected; }
                private set { _IsWriteProtected = value; }
            }
            private bool _IsWriteProtected;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.SizeOfSerialNumberInBytes)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt32 SizeOfSerialNumberInBytes
            {
                get { return _SizeOfSerialNumberInBytes; }
                private set { _SizeOfSerialNumberInBytes = value; }
            }
            private UInt32 _SizeOfSerialNumberInBytes;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.SerialNumber)]
            public String SerialNumber
            {
                get { return _SerialNumber; }
            }
            private String _SerialNumber;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.IsMediaPresent)]
            public bool IsMediaPresent
            {
                get { return _IsMediaPresent; }
                private set { _IsMediaPresent = value; }
            }
            private bool _IsMediaPresent;

            [Category("Response"), BrowsableResponse(LogicalDriveInfo.MediaChange)]
            public bool MediaChange
            {
                get { return _MediaChange; }
                private set { _MediaChange = value; }
            }
            private bool _MediaChange;

            #endregion // Response

            #region IFilterProperties Members

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                if (responseAttribute.Criteria is DriveMechanism)
                {
                    return ((DriveMechanism)responseAttribute.Criteria == _DriveMechanism);
                }
                else
                {
                    return ((LogicalDriveInfo)responseAttribute.Criteria == InfoType);
                }
            }

            #endregion
        } // class GetLogicalDriveInfo

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ReadLogicalDriveSector
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class ReadLogicalDriveSector : ScsiVendorApi, IFilterProperties
        {
            // Constructor
            [Obsolete("Drive Numbers have been replaced with Drive Tags. Please use the DriveTag constructor.")]
            public ReadLogicalDriveSector(Byte driveNumber, UInt32 sectorSize, UInt64 sectorStart, UInt32 sectorCount)
                : base(ScsiVendorApi.CommandSet.ReadLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Number;

                // Parameters
                _CDB.Add("DriveNumber", driveNumber);
                _CDB.Add("SectorStart", sectorStart);
                _CDB.Add("SectorCount", sectorCount);

                SectorSize = sectorSize;
            }

            // Constructor
            public ReadLogicalDriveSector(LogicalDrive.Tag driveTag, UInt32 sectorSize, UInt64 sectorStart, UInt32 sectorCount)
                : base(ScsiVendorApi.CommandSet.ReadLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Tag;

                // Parameters
                _CDB.Add("DriveTag", driveTag);
                _CDB.Add("SectorStart", sectorStart);
                _CDB.Add("SectorCount", sectorCount);

                SectorSize = sectorSize;
            }

            #region Parameter Properties

            // Parameters
            [Category("Parameters"), BrowsableResponse(DriveMechanism.Number), Description("The drive to read from.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte DriveNumber
            {
                get { return (Byte)_CDB.FromField("DriveNumber"); }
                set { _CDB.ToField("DriveNumber", value); }
            }

            [Category("Parameters"), BrowsableResponse(DriveMechanism.Tag), Description("The drive to read from.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public LogicalDrive.Tag DriveTag
            {
                get { return (LogicalDrive.Tag)_CDB.FromField("DriveTag"); }
                set
                {
                    _CDB.ToField("DriveTag", value);
                }
            }

            [Category("Parameters"), Description("The size in bytes of a sector on the media.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SectorSize
            {
                get { return _SectorSize; }
                set
                {
                    _SectorSize = value;
                    TransferSize = _SectorSize * SectorCount;
                }
            }
            private UInt32 _SectorSize;

            [Category("Parameters"), Description("The starting sector.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt64 SectorStart
            {
                get { return (UInt64)_CDB.FromField("SectorStart"); }
                set { _CDB.ToField("SectorStart", value); }
            }

            [Category("Parameters"), Description("The number of sectors to read.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SectorCount
            {
                get { return (UInt32)_CDB.FromField("SectorCount"); }
                set
                {
                    _CDB.ToField("SectorCount", value);
                    TransferSize = value * SectorSize;
                }
            }

            #endregion

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

            [Browsable(false)]
            override public String ResponseString
            {
                get
                {
                    return FormatReadResponse(Data, 16, 1);
                }
            }

            #endregion // Response

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                return ((DriveMechanism)responseAttribute.Criteria == _DriveMechanism);
            }

        } // class ReadLogicalDriveSector

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// WriteLogicalDriveSector
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class WriteLogicalDriveSector : ScsiVendorApi, IFilterProperties
        {
            // Constructors
            [Obsolete("Drive Numbers have been replaced with Drive Tags. Please use the DriveTag constructor.")]
            public WriteLogicalDriveSector(Byte driveNumber, UInt32 sectorSize, UInt64 sectorStart, UInt32 sectorCount, Byte[] data)
                : this(driveNumber, sectorSize, sectorStart, sectorCount, data, false)
            { }

            [Obsolete("Drive Numbers have been replaced with Drive Tags. Please use the DriveTag constructor.")]
            public WriteLogicalDriveSector(Byte driveNumber, UInt32 sectorSize, UInt64 sectorStart, UInt32 sectorCount, Byte[] data, bool write512)
                : base(write512 ? ScsiVendorApi.CommandSet.WriteLogicalDriveSector512 : ScsiVendorApi.CommandSet.WriteLogicalDriveSector, Api.CommandType.StScsiCmd,
                Api.CommandDirection.WriteWithData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Number;

                // Parameters
                _CDB.Add("DriveNumber", driveNumber);
                _CDB.Add("SectorStart", sectorStart);
                _CDB.Add("SectorCount", sectorCount);

                SectorSize = sectorSize;
                Data = data;
                WriteAs512 = write512;
            }

            public WriteLogicalDriveSector(LogicalDrive.Tag driveTag, UInt32 sectorSize, UInt64 sectorStart, UInt32 sectorCount, Byte[] data)
                : this(driveTag, sectorSize, sectorStart, sectorCount, data, false)
            { }

            public WriteLogicalDriveSector(LogicalDrive.Tag driveTag, UInt32 sectorSize, UInt64 sectorStart, UInt32 sectorCount, Byte[] data, bool write512)
                : base(write512 ? ScsiVendorApi.CommandSet.WriteLogicalDriveSector512 : ScsiVendorApi.CommandSet.WriteLogicalDriveSector, Api.CommandType.StScsiCmd,
                Api.CommandDirection.WriteWithData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Tag;

                // Parameters
                _CDB.Add("DriveTag", driveTag);
                _CDB.Add("SectorStart", sectorStart);
                _CDB.Add("SectorCount", sectorCount);

                SectorSize = sectorSize;
                Data = data;
                WriteAs512 = write512;
            }

            #region Parameter Properties

            // Parameters
            [Category("Parameters"), BrowsableResponse(DriveMechanism.Number), Description("The logical drive to write.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte DriveNumber
            {
                get { return (Byte)_CDB.FromField("DriveNumber"); }
                set { _CDB.ToField("DriveNumber", value); }
            }

            [Category("Parameters"), BrowsableResponse(DriveMechanism.Tag), Description("The logical drive to write.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public LogicalDrive.Tag DriveTag
            {
                get { return (LogicalDrive.Tag)_CDB.FromField("DriveTag"); }
                set
                {
                    _CDB.ToField("DriveTag", value);
                }
            }

            [Category("Parameters"), Description("The size in bytes of a sector on the media.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SectorSize
            {
                get { return _SectorSize; }
                set
                {
                    _SectorSize = value;
                    TransferSize = _SectorSize * SectorCount;
                    Byte[] tempData = Data;
                    Data = new Byte[TransferSize];
                    Array.Copy(tempData, 0, Data, 0, Math.Min(tempData.Length, Data.Length));
                }
            }
            private UInt32 _SectorSize;

            [Category("Parameters"), Description("The starting sector.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt64 SectorStart
            {
                get { return (UInt64)_CDB.FromField("SectorStart"); }
                set { _CDB.ToField("SectorStart", value); }
            }

            [Category("Parameters"), Description("The number of sectors to write.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SectorCount
            {
                get { return (UInt32)_CDB.FromField("SectorCount"); }
                set
                {
                    _CDB.ToField("SectorCount", value);
                    TransferSize = value * SectorSize;
                    Byte[] tempData = Data;
                    Data = new Byte[TransferSize];
                    Array.Copy(tempData, 0, Data, 0, Math.Min(tempData.Length, Data.Length));
                }
            }

            [Category("Parameters"), Description("Write data as 512-byte sectors. (deprecated SDK2.6-only)")]
            public bool WriteAs512
            {
                get { return ((ScsiVendorApi.CommandSet)_CDB.FromField("Command")) == CommandSet.WriteLogicalDriveSector512; }
                set { _CDB.ToField("Command", value ? ScsiVendorApi.CommandSet.WriteLogicalDriveSector512 : ScsiVendorApi.CommandSet.WriteLogicalDriveSector); }
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
            
            #endregion

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                return ((DriveMechanism)responseAttribute.Criteria == _DriveMechanism);
            }

        } // class WriteLogicalDriveSector

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// EraseLogicalDrive
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class EraseLogicalDrive : ScsiVendorApi, IFilterProperties
        {
            // Constructor
            [Obsolete("Drive Numbers have been replaced with Drive Tags. Please use the DriveTag constructor.")]
            public EraseLogicalDrive(Byte driveNumber)
                : base(ScsiVendorApi.CommandSet.EraseLogicalDrive, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Number;

                // Parameters
                _CDB.Add("DriveNumber", driveNumber);
            }

            // Parameters
            [Category("Parameters"), BrowsableResponse(DriveMechanism.Number), Description("The drive to erase.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte DriveNumber
            {
                get { return Convert.ToByte(_CDB.FromField("DriveNumber")); }
                set
                {
                    _CDB.ToField("DriveNumber", value);
                }
            }

            public EraseLogicalDrive(LogicalDrive.Tag driveTag)
                : base(ScsiVendorApi.CommandSet.EraseLogicalDrive, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Tag;

                // Parameters
                _CDB.Add("DriveTag", driveTag);
            }

            [Category("Parameters"), BrowsableResponse(DriveMechanism.Tag), Description("The drive to erase.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public LogicalDrive.Tag DriveTag
            {
                get { return (LogicalDrive.Tag)_CDB.FromField("DriveTag"); }
                set
                {
                    _CDB.ToField("DriveTag", value);
                }
            }

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                return ((DriveMechanism)responseAttribute.Criteria == _DriveMechanism);
            }

        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetChipMajorRevId
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetChipMajorRevId : ScsiVendorApi
        {
            // Parameters

            // Constructor
            public GetChipMajorRevId()
                : base(ScsiVendorApi.CommandSet.GetChipMajorRevId, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                TransferSize = sizeof(UInt16/*ChipId*/);
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= _TransferSize);

                Array.Reverse(data);

                _ChipMajorRevision = BitConverter.ToUInt16(data, 0);
                _ResponseString = String.Format(" Chip major revision: 0x{0:X4}\r\n", ChipMajorRevision);	

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 ChipMajorRevision
            {
                get { return _ChipMajorRevision; }
            }
            private UInt16 _ChipMajorRevision;

            #endregion // Response

        } // class GetChipMajorRevId

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ChipReset
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class ChipReset : ScsiVendorApi
        {
            // Constructor
            public ChipReset()
                : base(ScsiVendorApi.CommandSet.ChipReset, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {}

        } // class ChipReset

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetChipSerialNumberInfo
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetChipSerialNumberInfo : ScsiVendorApi
        {
            public enum SerialNumberInfo : byte { SizeOfSerialNumberInBytes = 0, SerialNumber = 1 }

            // Constructors
            /// <summary>
            /// Api for obtaining the STMP chip serial number. The infoType is set to SerialNumberInfo.SerialNumber 
            /// and the SerialNumberSize is set to 16.
            /// </summary>
            public GetChipSerialNumberInfo()
                : this(SerialNumberInfo.SerialNumber, 16)
            { }

            public GetChipSerialNumberInfo(SerialNumberInfo infoType, UInt16 serialNumberSize)
                : base(ScsiVendorApi.CommandSet.GetChipSerialNumberInfo, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("InfoType", infoType);

                SerialNumberSize = serialNumberSize;
                TransferSize = (infoType == SerialNumberInfo.SizeOfSerialNumberInBytes) ? (UInt16)sizeof(UInt16) : SerialNumberSize;
            }

            // Parameters
            [Category("Parameters"), Description("The type of info to get.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public SerialNumberInfo InfoType
            {
                get { return (SerialNumberInfo)_CDB.FromField("InfoType"); }
                set
                {
                    _CDB.ToField("InfoType", value);
                    TransferSize = (value == SerialNumberInfo.SizeOfSerialNumberInBytes) ? (UInt16)sizeof(UInt16) : SerialNumberSize;
                }
            }

            [Category("Parameters"), Description("Only used for InfoType = SerialNumber. Specifies the size of the return buffer. Call GetChipSerialNumberInfo(SizeOfSerialNumberInBytes,0) first to get this parameter.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 SerialNumberSize
            {
                get { return _SerialNumberSize; }
                set
                {
                    _SerialNumberSize = value;
                    if (InfoType == SerialNumberInfo.SizeOfSerialNumberInBytes)
                        TransferSize = _SerialNumberSize;
                }
            }
            private UInt16 _SerialNumberSize;

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                switch ((SerialNumberInfo)_CDB.FromField("InfoType"))
                {
                    case SerialNumberInfo.SizeOfSerialNumberInBytes:
                        {
                            _SerialNumberSize = BitConverter.ToUInt16(data, 0);
                            ResponseString = String.Format(" Serial number size: 0x{0:X} ({0})\r\n", SerialNumberSize);
                            break;
                        }
                    case SerialNumberInfo.SerialNumber:
                        {
                            _SerialNumber = Utils.Utils.StringFromHexBytes(data, String.Empty);
                            ResponseString = " Chip serial number: " + SerialNumber + "\r\n";
                            break;
                        }
                    default:
                        ResponseString = "Invalid SerialNumberInfo InfoType.";
                        return Win32.ERROR_INVALID_DATA;
                }

                return Win32.ERROR_SUCCESS;

            } // ProcessResponse()

            #region Response Properties

            [Category("Response")]
            public String SerialNumber
            {
                get { return _SerialNumber; }
            }
            private String _SerialNumber;

            #endregion
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// FlushLogicalDrive
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class FlushLogicalDrive : ScsiVendorApi, IFilterProperties
        {
            // Constructor
            [Obsolete("Drive Numbers have been replaced with Drive Tags. Please use the DriveTag constructor.")]
            public FlushLogicalDrive(Byte driveNumber)
                : base(ScsiVendorApi.CommandSet.FlushLogicalDrive, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Number;

                // Parameters
                _CDB.Add("DriveNumber", driveNumber);
            }

            // Parameters
            [Category("Parameters"), BrowsableResponse(DriveMechanism.Number), Description("The drive to flush.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte DriveNumber
            {
                get { return Convert.ToByte(_CDB.FromField("DriveNumber")); }
                set
                {
                    _CDB.ToField("DriveNumber", value);
                }
            }
            
            public FlushLogicalDrive(LogicalDrive.Tag driveTag)
                : base(ScsiVendorApi.CommandSet.FlushLogicalDrive, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
                _DriveMechanism = DriveMechanism.Tag;

                // Parameters
                _CDB.Add("DriveTag", driveTag);
            }

            [Category("Parameters"), BrowsableResponse(DriveMechanism.Tag), Description("The drive to flush.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public LogicalDrive.Tag DriveTag
            {
                get { return (LogicalDrive.Tag)_CDB.FromField("DriveTag"); }
                set
                {
                    _CDB.ToField("DriveTag", value);
                }
            }

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                return ((DriveMechanism)responseAttribute.Criteria == _DriveMechanism);
            }

        } // class FlushLogicalDrive

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetPhysicalMediaInfo
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetPhysicalMediaInfo : ScsiVendorApi
        {
            // Constructor
            public GetPhysicalMediaInfo()
                : base(ScsiVendorApi.CommandSet.GetPhysicalMediaInfo, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                TransferSize = 8 * sizeof(UInt32);
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                _NumNANDChips       = BitConverter.ToUInt32(data, 0);
                _ChipNumber         = BitConverter.ToUInt32(data, 4);
                _TotalSectors       = BitConverter.ToUInt32(data, 8);
                _TotalPages         = BitConverter.ToUInt32(data, 12);
                _TotalBlocks        = BitConverter.ToUInt32(data, 16);
                _TotalInternalDie   = BitConverter.ToUInt32(data, 20);
                _BlocksPerDie       = BitConverter.ToUInt32(data, 24);
                _TotalZones         = BitConverter.ToUInt32(data, 28);

                return Win32.ERROR_SUCCESS;
            } // ProcessResponse()

            #region Response Properties

            [Category("Response")]
            public UInt32 NumNANDChips
            {
                get { return _NumNANDChips; }
            }
            private UInt32 _NumNANDChips;

            [Category("Response")]
            public UInt32 ChipNumber
            {
                get { return _ChipNumber; }
            }
            private UInt32 _ChipNumber;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 TotalSectors
            {
                get { return _TotalSectors; }
            }
            private UInt32 _TotalSectors;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 TotalPages
            {
                get { return _TotalPages; }
            }
            private UInt32 _TotalPages;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 TotalBlocks
            {
                get { return _TotalBlocks; }
            }
            private UInt32 _TotalBlocks;

            // (1/2/4/...) - number of chips pretending to be a single chip
            [Category("Response")]
            public UInt32 TotalInternalDie
            {
                get { return _TotalInternalDie; }
            }
            private UInt32 _TotalInternalDie;

            // (wTotalBlocks / wTotalInternalDice )
            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 BlocksPerDie
            {
                get { return _BlocksPerDie; }
            }
            private UInt32 _BlocksPerDie;

            [Category("Response")]
            public UInt32 TotalZones
            {
                get { return _TotalZones; }
            }
            private UInt32 _TotalZones;

            public override string ResponseString
            {
                get
                {
                    Utils.DecimalConverterEx cnvrt = new Utils.DecimalConverterEx();

                    _ResponseString =
                        String.Format(" NAND Count:\t\t{0}\r\n", NumNANDChips) +
                        String.Format(" Chip Number:\t\t{0}\r\n", ChipNumber) +
                        String.Format(" Total Sectors:\t\t{0}\r\n", cnvrt.ConvertToString(TotalSectors)) +
                        String.Format(" Total Pages:\t\t{0}\r\n", cnvrt.ConvertToString(TotalPages)) +
                        String.Format(" Total Blocks:\t\t{0}\r\n", cnvrt.ConvertToString(TotalBlocks)) +
                        String.Format(" Total Internal Die:\t{0}\r\n", TotalInternalDie) +
                        String.Format(" Blocks per Die:\t{0}\r\n", cnvrt.ConvertToString(BlocksPerDie)) +
                        String.Format(" Total Zones:\t\t{0}\r\n", TotalZones);

                    return _ResponseString;
                }
            }
            
            #endregion // Response

        } // class GetPhysicalMediaInfo

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ReadPhysicalMediaSector
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class ReadPhysicalMediaSector : ScsiVendorApi
        {
            // Constructor
            public ReadPhysicalMediaSector(Byte chipNumber, UInt32 sectorSize, UInt64 sectorStart, UInt32 sectorCount)
                : base(ScsiVendorApi.CommandSet.ReadPhysicalMediaSector, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("ChipNumber", chipNumber);
                _CDB.Add("SectorStart", sectorStart);
                _CDB.Add("SectorCount", sectorCount);

                SectorSize = sectorSize;
            }

            #region Parameter Properties

            // Parameters
            [Category("Parameters"), Description("The NAND chip to read.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte DriveNumber
            {
                get { return (Byte)_CDB.FromField("ChipNumber"); }
                set { _CDB.ToField("ChipNumber", value); }
            }

            [Category("Parameters"), Description("The size in bytes of a sector on the media.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SectorSize
            {
                get { return _SectorSize; }
                set
                {
                    _SectorSize = value;
                    TransferSize = _SectorSize * SectorCount;
                }
            }
            private UInt32 _SectorSize;

            [Category("Parameters"), Description("The starting sector.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt64 SectorStart
            {
                get { return (UInt64)_CDB.FromField("SectorStart"); }
                set { _CDB.ToField("SectorStart", value); }
            }

            [Category("Parameters"), Description("The number of sectors to read.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SectorCount
            {
                get { return (UInt32)_CDB.FromField("SectorCount"); }
                set
                {
                    _CDB.ToField("SectorCount", value);
                    TransferSize = value * SectorSize;
                }
            }

            #endregion

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

            [Browsable(false)]
            override public String ResponseString
            {
                get
                {
                    return FormatReadResponse(Data, 16, 1);
                }
            }

            #endregion // Response

        } // class ReadPhysicalMediaSector

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetChipPartRevId
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetChipPartRevId : ScsiVendorApi
        {
            // Parameters

            // Constructor
            public GetChipPartRevId()
                : base(ScsiVendorApi.CommandSet.GetChipPartRevId, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                TransferSize = sizeof(UInt16/*ChipRev*/);
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= _TransferSize);

                Array.Reverse(data);

                _ChipPartRevision = BitConverter.ToUInt16(data, 0);
                _ResponseString = String.Format(" Chip part revision: 0x{0:X4}\r\n", ChipPartRevision);

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 ChipPartRevision
            {
                get { return _ChipPartRevision; }
            }
            private UInt16 _ChipPartRevision;

            #endregion // Response

        } // class GetChipPartRevId

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetRomRevId
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetRomRevId : ScsiVendorApi
        {
            // Parameters

            // Constructor
            public GetRomRevId()
                : base(ScsiVendorApi.CommandSet.GetRomRevId, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                TransferSize = sizeof(UInt16/*RomRevision*/);
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= _TransferSize);

                Array.Reverse(data);

                _RomRevision = BitConverter.ToUInt16(data, 0);
                _ResponseString = String.Format("ROM revision: 0x{0:X4}", RomRevision);

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 RomRevision
            {
                get { return _RomRevision; }
            }
            private UInt16 _RomRevision;

            #endregion // Response

        } // class GetRomRevId

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetJanusStatus
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetJanusStatus : ScsiVendorApi
        {
            // Parameters

            // Constructor
            public GetJanusStatus()
                : base(ScsiVendorApi.CommandSet.GetJanusStatus, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                TransferSize = sizeof(Byte/*JanusStatus*/);
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                _JanusStatus = data[0];
                _ResponseString = String.Format(" Janus status: 0x{0:X2}\r\n", JanusStatus);

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte JanusStatus
            {
                get { return _JanusStatus; }
            }
            private Byte _JanusStatus;

            #endregion // Response

        } // class GetJanusStatus

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// InitializeJanus
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class InitializeJanus : ScsiVendorApi
        {
            // Constructor
            public InitializeJanus()
                : base(ScsiVendorApi.CommandSet.InitializeJanus, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, ExtendedTimeout)
            {}

        } // class InitializeJanus

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ResetToRecovery
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class ResetToRecovery : ScsiVendorApi
        {
            // Constructor
            public ResetToRecovery()
                : base(ScsiVendorApi.CommandSet.ResetToRecovery, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            { }

        } // class ResetToRecovery

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// InitializeDataStore
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class InitializeDataStore : ScsiVendorApi
        {
            // Constructor
            public InitializeDataStore(Byte storeNumber)
                : base(ScsiVendorApi.CommandSet.InitializeDataStore, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, ExtendedTimeout)
            {
                // Parameters
                _CDB.Add("StoreNumber", storeNumber);
            }

            // Parameters
            [Category("Parameters"), Description("The store to initialize.")]
            public Byte StoreNumber
            {
                get { return Convert.ToByte(_CDB.FromField("StoreNumber")); }
                set
                {
                    _CDB.ToField("StoreNumber", value);
                }
            }
        } // class InitializeDataStore

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ResetToUpdater
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class ResetToUpdater : ScsiVendorApi
        {
            // Constructor
            public ResetToUpdater()
                : base(ScsiVendorApi.CommandSet.ResetToUpdater, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            { }

        } // class ResetToUpdater

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetDeviceProperties
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        [TypeDescriptionProvider(typeof(ApiTypeDescriptionProvider))]
        public class GetDeviceProperties : ScsiVendorApi, IFilterProperties
        {
            public enum DevicePropertiesType : byte { PhysicalExternalRamSize = 0, VirtualExternalRamSize = 1 }

            // Constructor
            public GetDeviceProperties(DevicePropertiesType infoType)
                : base(ScsiVendorApi.CommandSet.GetDeviceProperties, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("InfoType", infoType);

                TransferSize = sizeof(UInt32);
            }

            // Parameters
            [Category("Parameters"), Description("The type of info to get.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public DevicePropertiesType InfoType
            {
                get { return (DevicePropertiesType)_CDB.FromField("InfoType"); }
                set
                {
                    _CDB.ToField("InfoType", value);
                }
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                // FOR SOME REASON THE F/W RETURNS THIS VALUE BACKWARDS
                // Array.Reverse(data);
                
                switch ((DevicePropertiesType)_CDB.FromField("InfoType"))
                {
                    case DevicePropertiesType.PhysicalExternalRamSize:
                        {
                            PhysicalExternalRamSize = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Physical SDRAM: {0}, 0x{1:X} ({1}) bytes\r\n", Utils.Utils.ScaleBytes(PhysicalExternalRamSize), PhysicalExternalRamSize);
                            break;
                        }
                    case DevicePropertiesType.VirtualExternalRamSize:
                        {
                            VirtualExternalRamSize = BitConverter.ToUInt32(data, 0);
                            ResponseString = String.Format(" Virtual SDRAM: {0}, 0x{1:X} ({1}) bytes\r\n", Utils.Utils.ScaleBytes(VirtualExternalRamSize), VirtualExternalRamSize);
                            break;
                        }
                    default:
                        ResponseString = String.Format(" Unknown property: 0x{0:X8} ({0})\r\n", BitConverter.ToUInt32(data, 0));
                        return Win32.ERROR_INVALID_PARAMETER;
                }

                return Win32.ERROR_SUCCESS;

            } // ProcessResponse()

            #region Response Properties

            [Category("Response"), BrowsableResponse(DevicePropertiesType.PhysicalExternalRamSize)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt32 PhysicalExternalRamSize
            {
                get { return _PhysicalExternalRamSize; }
                private set { _PhysicalExternalRamSize = value; }
            }
            private UInt32 _PhysicalExternalRamSize;

            [Category("Response"), BrowsableResponse(DevicePropertiesType.VirtualExternalRamSize)]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt32 VirtualExternalRamSize
            {
                get { return _VirtualExternalRamSize; }
                private set { _VirtualExternalRamSize = value; }
            }
            private UInt32 _VirtualExternalRamSize;

            #endregion // Response

            #region IFilterProperties Members

            public bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute)
            {
                return ((DevicePropertiesType)responseAttribute.Criteria == InfoType);
            }

            #endregion
        } // class GetDeviceProperties

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// SetUpdateFlag
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class SetUpdateFlag : ScsiVendorApi
        {
            // Constructor
            public SetUpdateFlag()
                : base(ScsiVendorApi.CommandSet.SetUpdateFlag, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            { }

        } // class SetUpdateFlag

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// FilterPing
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class FilterPing : ScsiVendorApi
        {
            // Constructor
            public FilterPing()
                : base(ScsiVendorApi.CommandSet.FilterPing, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            { }

        } // class FilterPing

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// WriteDrive
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class WriteDrive : ScsiVendorApi
        {
            // Constructor
            public WriteDrive(LogicalDrive.Tag tag, String filename)
                : base(ScsiVendorApi.CommandSet.WriteLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.WriteWithData, DefaultTimeout)
            {
                Filename = filename;
                Tag = tag;
            }

            [Category("Parameters"), Description("Specifies the drive to write.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            [MemberFunctionArg()]
            public LogicalDrive.Tag Tag
            {
                get { return _Tag; }
                set { _Tag = value; }
            }
            private LogicalDrive.Tag _Tag;

            // Override Data here to add the MemberFunctionArgAttribute to the property
            // so it will get passed to the member function invocation.
            [MemberFunctionArg()]
            public override byte[] Data
            {
                get { return base.Data; }
                set { base.Data = value; }
            }

            [Category("Parameters"), Description("The filename to download."), NotifyParentProperty(true)]
            [Editor(typeof(System.Windows.Forms.Design.FileNameEditor), typeof(System.Drawing.Design.UITypeEditor))]
            public String Filename
            {
                get { return _Filename; }
                set
                {
                    _Filename = value;

                    try
                    {
                        _FileInfo = Media.FirmwareInfo.FromFile(value);
                        using (FileStream fs = File.OpenRead(value) )
                        {
                            Data = new Byte[_FileInfo.DataSize];
                            
                            if (_FileInfo.StartingOffset != 0)
                                fs.Seek(_FileInfo.StartingOffset, SeekOrigin.Begin);

                            fs.Read(Data, 0, (Int32)_FileInfo.DataSize);
                            
                            TransferSize = (UInt32)Data.Length;
                        }
                    }
                    catch (Exception e)
                    {
                        _FileInfo = new DevSupport.Media.FirmwareInfo();
                        _FileInfo.FileStatus = e.Message;
                        Data = null;
                        TransferSize = 0;
                    }
                }
            }
            private String _Filename;

            [Category("Status"), Description("The status of the file to download.")]
            [TypeConverter(typeof(ExpandableObjectConverter))]
            public Media.FirmwareInfo FileInfo
            {
                get
                {
                    return _FileInfo;
                }
            }
            private Media.FirmwareInfo _FileInfo;

        } // class WriteDrive

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// ReadDrive
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class ReadDrive : ScsiVendorApi, IProcessResponse
        {
            // Constructor
            public ReadDrive(LogicalDrive.Tag tag)
                : base(ScsiVendorApi.CommandSet.ReadLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                Tag = tag;
            }

            [Category("Parameters"), Description("Specifies the drive to read.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            [MemberFunctionArg()]
            public LogicalDrive.Tag Tag
            {
                get { return _Tag; }
                set { _Tag = value; }
            }
            private LogicalDrive.Tag _Tag;

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Data = new Byte[count];

                data.CopyTo(Data, 0);

                return Win32.ERROR_SUCCESS;
            }

            // Override Data here to add the MemberFunctionArgAttribute to the property
            // so it will get passed to the member function invocation.
            [Category("Response"), Browsable(true)]
            public override byte[] Data
            {
                get { return base.Data; }
                set { base.Data = value; }
            }

            /* Takes WAY TOO LONG for big drives
            [Browsable(false)]
            override public String ResponseString
            {
                get { return FormatReadResponse(Data, 16, 1); }
            } */

        } // class ReadDrive

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// DumpDrive
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class DumpDrive : ScsiVendorApi, IProcessResponse
        {
            // Constructor
            public DumpDrive(LogicalDrive.Tag tag)
                : base(ScsiVendorApi.CommandSet.ReadLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
                Tag = tag;
            }

            [Category("Parameters"), Description("Specifies the drive to dump.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            [MemberFunctionArg()]
            public LogicalDrive.Tag Tag
            {
                get { return _Tag; }
                set { _Tag = value; }
            }
            private LogicalDrive.Tag _Tag;

        } // class DumpDrive

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// VerifyDrive
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class VerifyDrive : ScsiVendorApi, IProcessResponse
        {
            // Constructor
            public VerifyDrive(LogicalDrive.Tag tag, String filename)
                : base(ScsiVendorApi.CommandSet.ReadLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                Filename = filename;
                Tag = tag;
            }

            [Category("Parameters"), Description("Specifies the drive to write.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            [MemberFunctionArg()]
            public LogicalDrive.Tag Tag
            {
                get { return _Tag; }
                set { _Tag = value; }
            }
            private LogicalDrive.Tag _Tag;

            // Override Data here to add the MemberFunctionArgAttribute to the property
            // so it will get passed to the member function invocation.
            [MemberFunctionArg()]
            public override byte[] Data
            {
                get { return base.Data; }
                set { base.Data = value; }
            }

            [Category("Parameters"), Description("The filename to compare to the drive."), NotifyParentProperty(true)]
            [Editor(typeof(System.Windows.Forms.Design.FileNameEditor), typeof(System.Drawing.Design.UITypeEditor))]
            public String Filename
            {
                get { return _Filename; }
                set
                {
                    _Filename = value;

                    try
                    {
                        _FileInfo = Media.FirmwareInfo.FromFile(value);
                        using (FileStream fs = File.OpenRead(value))
                        {
                            Data = new Byte[_FileInfo.DataSize];

                            if (_FileInfo.StartingOffset != 0)
                                fs.Seek(_FileInfo.StartingOffset, SeekOrigin.Begin);

                            fs.Read(Data, 0, (Int32)_FileInfo.DataSize);

                            TransferSize = (UInt32)Data.Length;
                        }
                    }
                    catch (Exception e)
                    {
                        _FileInfo = new DevSupport.Media.FirmwareInfo();
                        _FileInfo.FileStatus = e.Message;
                        Data = null;
                        TransferSize = 0;
                    }
                }
            }
            private String _Filename;

            [Category("Status"), Description("The status of the file to compare with.")]
            [TypeConverter(typeof(ExpandableObjectConverter))]
            public Media.FirmwareInfo FileInfo
            {
                get
                {
                    return _FileInfo;
                }
            }
            private Media.FirmwareInfo _FileInfo;

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                _IsFileEqualToDevice = BitConverter.ToBoolean(data, 0);

                ResponseString = String.Format(" File equals drive: {0}\r\n", IsFileEqualToDevice);

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            public bool IsFileEqualToDevice
            {
                get { return _IsFileEqualToDevice; }
                private set { _IsFileEqualToDevice = value; }
            }
            private bool _IsFileEqualToDevice;

            #endregion // Response

        } // class VerifyDrive

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// WriteAllFwDrives
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class WriteAllFwDrives : ScsiVendorApi
        {
            // Constructor
            public WriteAllFwDrives(String filename)
                : base(ScsiVendorApi.CommandSet.WriteLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.WriteWithData, DefaultTimeout)
            {
                Filename = filename;
            }

            // Override Data here to add the MemberFunctionArgAttribute to the property
            // so it will get passed to the member function invocation.
            [MemberFunctionArg()]
            public override byte[] Data
            {
                get { return base.Data; }
                set { base.Data = value; }
            }

            [Category("Parameters"), Description("The filename to compare to the drive."), NotifyParentProperty(true)]
            [Editor(typeof(System.Windows.Forms.Design.FileNameEditor), typeof(System.Drawing.Design.UITypeEditor))]
            public String Filename
            {
                get { return _Filename; }
                set
                {
                    _Filename = value;

                    try
                    {
                        _FileInfo = Media.FirmwareInfo.FromFile(value);
                        using (FileStream fs = File.OpenRead(value))
                        {
                            Data = new Byte[_FileInfo.DataSize];

                            if (_FileInfo.StartingOffset != 0)
                                fs.Seek(_FileInfo.StartingOffset, SeekOrigin.Begin);

                            fs.Read(Data, 0, (Int32)_FileInfo.DataSize);

                            TransferSize = (UInt32)Data.Length;
                        }
                    }
                    catch (Exception e)
                    {
                        _FileInfo = new DevSupport.Media.FirmwareInfo();
                        _FileInfo.FileStatus = e.Message;
                        Data = null;
                        TransferSize = 0;
                    }
                }
            }
            private String _Filename;

            [Category("Status"), Description("The status of the file to compare with.")]
            [TypeConverter(typeof(ExpandableObjectConverter))]
            public Media.FirmwareInfo FileInfo
            {
                get
                {
                    return _FileInfo;
                }
            }
            private Media.FirmwareInfo _FileInfo;

        } // class WriteAllFwDrives

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// WriteJanusHeader
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class WriteJanusHeader : ScsiVendorApi
        {
            // Constructor
            public WriteJanusHeader()
                : base(ScsiVendorApi.CommandSet.WriteLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.WriteWithData, DefaultTimeout)
            {
                TransferSize = (UInt32)Data.Length;
            }

            // Just show the JanusDriveHeader in the property grid
            [Category("Parameters"), Description("The Janus header info to write to the drive.")]
            [Browsable(true)]
            public override Byte[] Data
            {
                get { return new JanusDriveHeader(0x400).ToArray(); }
                set { /* */ }
            }

        } // class WriteJanusHeader

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// FormatDataDrive
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class FormatDataDrive : ScsiVendorApi
        {
            // Constructor
            public FormatDataDrive(LogicalDrive.Tag driveTag, FormatBuilder.FormatInfo.FileSystemType fileSystem, String volumeLabel, bool includeMBR)
                : base(ScsiVendorApi.CommandSet.WriteLogicalDriveSector, Api.CommandType.StScsiCmd, Api.CommandDirection.WriteWithData, DefaultTimeout)
            {
                DriveTag = driveTag;
                FileSystem = fileSystem;
                VolumeLabel = volumeLabel;
            }

            [MemberFunctionArg()]
            [Category("Parameters"), Description("Specifies the drive to format.")]
            public LogicalDrive.Tag DriveTag
            {
                get { return _DriveTag; }
                set { _DriveTag = value; }
            }
            private LogicalDrive.Tag _DriveTag;

            [MemberFunctionArg()]
            [Category("Parameters"), Description("The requested file system.")]
            public FormatBuilder.FormatInfo.FileSystemType FileSystem
            {
                get { return _FileSystem; }
                set { _FileSystem = value; }
            }
            private FormatBuilder.FormatInfo.FileSystemType _FileSystem;

            [MemberFunctionArg()]
            [Category("Parameters"), Description("The volume label. Labels longer than 11 character may be truncated.")]
            public String VolumeLabel
            {
                get { return _VolumeLabel; }
                set { _VolumeLabel = value; }
            }
            private String _VolumeLabel;

            [MemberFunctionArg()]
            [Category("Parameters"), Description("Include MBR upto PBS in image. True will include the MBR and Hidden Sectors between the MBR and PBS. False will create an image that starts with the PBS.")]
            public Boolean IncludeMBR
            {
                get { return _IncludeMBR; }
                set { _IncludeMBR = value; }
            }
            private Boolean _IncludeMBR;

        } // class FormatDataDrive

    } // class ScsiVendorApi
    
    public class ScsiFormalApi : ScsiApi
    {
        /// <summary>
        /// Scsi Command Set. 
        /// Byte[0] of the CDB for SCSI commands.
        /// </summary>
        public enum CommandSet : byte
        {
            Inquiry         = 0x12, // SCSIOP_INQUIRY
            StartStop       = 0x1B, // SCSIOP_START_STOP_UNIT
            ReadCapacity    = 0x25, // SCSIOP_READ_CAPACITY
            Read            = 0x28, // SCSIOP_READ
            Write           = 0x2A, // SCSIOP_WRITE
            ModeSense10     = 0x5A  // SCSIOP_MODE_SENSE10
        }

        protected ScsiFormalApi(ScsiFormalApi.CommandSet cmd, Api.CommandType cmdType, Api.CommandDirection dir, UInt32 timeout)
            : base(cmdType, dir, timeout)
        {
            _CDB.Add("Command", cmd);
        }

        [Description("The Command byte for the api."), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public ScsiFormalApi.CommandSet Command
        {
            get { return (ScsiFormalApi.CommandSet)_CDB.FromField("Command"); }
            private set { _CDB.ToField("Command", value); }
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// Inquiry
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class Inquiry : ScsiFormalApi
        {
            public enum DeviceTypeEnum : byte
            {
                DirectAccess = 0x00,            /// disks
                SequentialAccess = 0x01,        /// tapes
                Printer = 0x02,                 /// printers
                Processor = 0x03,               /// scanners, printers, etc
                WriteOnceReadMultiple = 0x04,   /// worms
                ReadOnlyDirectAccess = 0x05,    /// cdroms
                Scanner = 0x06,                 /// scanners
                Optical = 0x07,                 /// optical disks
                MediumChanger = 0x08,           /// jukebox
                Communication = 0x09,           /// network
                LogicalUnitNotPresent = 0x7F
            }

            public enum DeviceTypeQualifierEnum : byte
            {
                Active = 0x00, /// The operating system supports the device, and the device is present.
                NotActive = 0x01, /// The operating system supports the device, but the device is not present.
                NotSupported = 0x03  /// The operating system does not support this device.
            }

            // Constructor
            public Inquiry()
                : base(ScsiFormalApi.CommandSet.Inquiry, Api.CommandType.ScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                _CDB.Add("LUN", (Byte)0);
                _CDB.Add("PageCode", (Byte)0);
                _CDB.Add("IReserved", (Byte)0);
                _CDB.Add("AllocationLength", (Byte)0x60); // sizeof(INQUIRYDATA)
                _CDB.Add("Control", (Byte)0);

                TransferSize = 0x60; // sizeof(INQUIRYDATA)
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                DeviceType = (DeviceTypeEnum)(data[0] & 0x1F);
                DeviceTypeQualifier = (DeviceTypeQualifierEnum)((data[0] & 0xE0) >> 5);

                DeviceTypeModifier = (Byte)(data[1] & 0x7F);
                RemovableMedia = (data[1] & 0x80) == 0x80;

                Versions = data[2];

                ResponseDataFormat = (Byte)(data[3] & 0x0F);
                HiSupport = (data[3] & 0x10) == 0x10;
                NormACA = (data[3] & 0x20) == 0x20;
                TerminateTask = (data[3] & 0x40) == 0x40;
                AERC = (data[3] & 0x80) == 0x80;

                AdditionalLength = data[4];
                Reserved = data[5];

                Addr16 = (data[6] & 0x01) == 0x01;
                Addr32 = (data[6] & 0x02) == 0x02;
                AckReqQ = (data[6] & 0x04) == 0x04;
                MediumChanger = (data[6] & 0x08) == 0x08;
                MultiPort = (data[6] & 0x10) == 0x10;
                ReservedBit2 = (data[6] & 0x20) == 0x20;
                EnclosureServices = (data[6] & 0x40) == 0x40;
                ReservedBit3 = (data[6] & 0x80) == 0x80;

                SoftReset = (data[7] & 0x01) == 0x01;
                CommandQueue = (data[7] & 0x02) == 0x02;
                TransferDisable = (data[7] & 0x04) == 0x04;
                LinkedCommands = (data[7] & 0x08) == 0x08;
                Synchronous = (data[7] & 0x10) == 0x10;
                Wide16Bit = (data[7] & 0x20) == 0x20;
                Wide32Bit = (data[7] & 0x40) == 0x40;
                RelativeAddressing = (data[7] & 0x80) == 0x80;

                VendorId = Utils.Utils.StringFromAsciiBytes(data, 8, 8).Replace('\0', ' ');
                ProductId = Utils.Utils.StringFromAsciiBytes(data, 16, 16).Replace('\0', ' '); ;
                ProductRevisionLevel = Utils.Utils.StringFromAsciiBytes(data, 32, 4).Replace('\0', ' '); ;

                Array.Copy(data, 36, VendorSpecific, 0, 20);
                Array.Copy(data, 56, Reserved3, 0, 40);

                ResponseString = String.Format(" {0} {1} {2}\r\n RemovableMedia: {3}\r\n", VendorId, ProductId, ProductRevisionLevel, RemovableMedia);
                
                // Just something I noticed on the card reader on my system
                if (VendorSpecific[0] != 0)
                {
                    ResponseString += " VendorSpecific: " + Utils.Utils.StringFromUnicodeBytes(VendorSpecific) + "\r\n";
                }
                return Win32.ERROR_SUCCESS;

            } // ProcessResponse()

            #region Response Properties

            [Category("Response")]
            [Description("Specifies the type of device. For a complete list of symbolic constants that indicate the various device types.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public DeviceTypeEnum DeviceType
            {
                get { return _DeviceType; }
                private set { _DeviceType = value; }
            }
            private DeviceTypeEnum _DeviceType;

            [Category("Response")]
            [Description("Indicates whether the device is present or not.")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public DeviceTypeQualifierEnum DeviceTypeQualifier
            {
                get { return _DeviceTypeQualifier; }
                private set { _DeviceTypeQualifier = value; }
            }
            private DeviceTypeQualifierEnum _DeviceTypeQualifier;

            [Category("Response")]
            [Description("Specifies the device type modifier, if any, as defined by SCSI. If no device type modifier exists, this member is zero.")]
            public Byte DeviceTypeModifier
            {
                get { return _DeviceTypeModifier; }
                private set { _DeviceTypeModifier = value; }
            }
            private Byte _DeviceTypeModifier;

            [Category("Response")]
            [Description("Indicates, when TRUE, that the media is removable, and when FALSE that the media is not removable.")]
            public bool RemovableMedia
            {
                get { return _RemovableMedia; }
                private set { _RemovableMedia = value; }
            }
            private bool _RemovableMedia;

            [Category("Response")]
            [Description("Indicates the version of the inquiry data standard that this data conforms to. For more information about the version values allowed in this field, see the SCSI Primary Commands - 2 (SPC-2) specification.")]
            public Byte Versions
            {
                get { return _Versions; }
                private set { _Versions = value; }
            }
            private Byte _Versions;

            [Category("Response")]
            [Description("Indicates the SCSI standard that governs the response data format. The value of this member must be 2.")]
            public Byte ResponseDataFormat
            {
                get { return _ResponseDataFormat; }
                private set { _ResponseDataFormat = value; }
            }
            private Byte _ResponseDataFormat;

            [Category("Response")]
            [Description("Indicates, when zero, that the target does not use the hierarchical addressing model to assign LUNs to logical units. A value of 1 indicates the target uses the hierarchical addressing model to assign LUNs to logical units.")]
            public bool HiSupport
            {
                get { return _HiSupport; }
                private set { _HiSupport = value; }
            }
            private bool _HiSupport;

            [Category("Response")]
            [Description("Indicates, when set to one, that the operating system supports setting the NACA bit to one in the control byte of the command descriptor block (CDB). A value of zero indicates that the system does not support setting the NACA bit to one. For more information about the function of the NACA bit and the control byte in a CDB, see the SCSI Primary Commands - 2 (SPC-2) specification.")]
            public bool NormACA
            {
                get { return _NormACA; }
                private set { _NormACA = value; }
            }
            private bool _NormACA;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target device supports the SCSI TERMINATE TASK task management function. A value of zero indicates that the target device does not support the TERMINATE TASK task management function.")]
            public bool TerminateTask
            {
                get { return _TerminateTask; }
                private set { _TerminateTask = value; }
            }
            private bool _TerminateTask;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target device supports the asynchronous event reporting capability. A value of zero indicates that the target device does not support asynchronous event reports. Details of the asynchronous event reporting support are protocol-specific. For more information about asynchronous even reporting, see the SCSI Primary Commands - 2 (SPC-2) specification.")]
            public bool AERC
            {
                get { return _AERC; }
                private set { _AERC = value; }
            }
            private bool _AERC;

            [Category("Response")]
            [Description("Specifies the length in bytes of the parameters of the command descriptor block (CDB).")]
            public Byte AdditionalLength
            {
                get { return _AdditionalLength; }
                private set { _AdditionalLength = value; }
            }
            private Byte _AdditionalLength;

            [Category("Response")]
            [Description("Reserved.")]
            public Byte Reserved
            {
                get { return _Reserved; }
                private set { _Reserved = value; }
            }
            private Byte _Reserved;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target supports 16-bit wide SCSI addresses. A value of zero indicates that the device does not support 32-bit wide SCSI addresses.")]
            public bool Addr16
            {
                get { return _Addr16; }
                private set { _Addr16 = value; }
            }
            private bool _Addr16;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target supports 32-bit wide SCSI addresses. A value of zero indicates that the device does not support 32-bit wide SCSI addresses.")]
            public bool Addr32
            {
                get { return _Addr32; }
                private set { _Addr32 = value; }
            }
            private bool _Addr32;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target supports a request and acknowledge data transfer handshake on the secondary bus. A value of zero indicates that the target does not support this function.")]
            public bool AckReqQ
            {
                get { return _AckReqQ; }
                private set { _AckReqQ = value; }
            }
            private bool _AckReqQ;

            [Category("Response")]
            [Description("Indicates, when set to one, that the device is embedded within or attached to a medium transport element. A value of zero indicates that the device is not embedded within or attached to a medium transport element.")]
            public bool MediumChanger
            {
                get { return _MediumChanger; }
                private set { _MediumChanger = value; }
            }
            private bool _MediumChanger;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target device is a multiport (2 or more ports) device that conforms to the SCSI-3 multiport device requirements. A value of zero indicates that this device has a single port and does not implement the multiport requirements.")]
            public bool MultiPort
            {
                get { return _MultiPort; }
                private set { _MultiPort = value; }
            }
            private bool _MultiPort;

            [Category("Response")]
            [Description("Reserved.")]
            public bool ReservedBit2
            {
                get { return _ReservedBit2; }
                private set { _ReservedBit2 = value; }
            }
            private bool _ReservedBit2;

            [Category("Response")]
            [Description("Indicates, when set to one, that the device contains an embedded enclosure services component. A value of zero indicates that the device does not contain an embedded enclosure services component.")]
            public bool EnclosureServices
            {
                get { return _EnclosureServices; }
                private set { _EnclosureServices = value; }
            }
            private bool _EnclosureServices;

            [Category("Response")]
            [Description("Reserved.")]
            public bool ReservedBit3
            {
                get { return _ReservedBit3; }
                private set { _ReservedBit3 = value; }
            }
            private bool _ReservedBit3;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target device supports soft resets. A value of zero indicates that the target does not support soft resets.")]
            public bool SoftReset
            {
                get { return _SoftReset; }
                private set { _SoftReset = value; }
            }
            private bool _SoftReset;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target device supports command queuing for this logical unit. However, a value of zero does not necessarily indicate that the target device does not support command queuing. The meaning of these values depends on the values present in the SCSI inquiry data. For information about the meaning of the command queuing bit, see the SCSI Primary Commands - 2 (SPC-2) specification.")]
            public bool CommandQueue
            {
                get { return _CommandQueue; }
                private set { _CommandQueue = value; }
            }
            private bool _CommandQueue;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target supports the SCSI CONTINUE TASK and TARGET TRANSFER DISABLE messages. A value of zero indicates that the device does not support one or both of these messages. For more information about the CONTINUE TASK and TARGET TRANSFER DISABLE messages, see the SCSI Primary Commands - 2 (SPC-2) specification.")]
            public bool TransferDisable
            {
                get { return _TransferDisable; }
                private set { _TransferDisable = value; }
            }
            private bool _TransferDisable;

            [Category("Response")]
            [Description("Indicates, when set to one, that the operating system supports linked commands. A value of zero indicates the operating system does not support linked commands.")]
            public bool LinkedCommands
            {
                get { return _LinkedCommands; }
                private set { _LinkedCommands = value; }
            }
            private bool _LinkedCommands;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target supports synchronous data transfer. A value of zero indicates that the target does not support synchronous data transfer.")]
            public bool Synchronous
            {
                get { return _Synchronous; }
                private set { _Synchronous = value; }
            }
            private bool _Synchronous;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target supports 16-bit wide data transfers. A value of zero indicates that the device does not support 16-bit wide data transfers.")]
            public bool Wide16Bit
            {
                get { return _Wide16Bit; }
                private set { _Wide16Bit = value; }
            }
            private bool _Wide16Bit;

            [Category("Response")]
            [Description("Indicates, when set to one, that the target supports 32-bit wide data transfers. A value of zero indicates that the device does not support 32-bit wide data transfers.")]
            public bool Wide32Bit
            {
                get { return _Wide32Bit; }
                private set { _Wide32Bit = value; }
            }
            private bool _Wide32Bit;

            [Category("Response")]
            [Description("Indicates, when set to one, that the operating system supports the relative addressing mode. A value of zero indicates the operating system does not support relative addressing.")]
            public bool RelativeAddressing
            {
                get { return _RelativeAddressing; }
                private set { _RelativeAddressing = value; }
            }
            private bool _RelativeAddressing;

            [Category("Response")]
            [Description("Contains eight bytes of ASCII data that identifies the vendor of the product.")]
            public String VendorId
            {
                get { return _VendorId; }
                private set { _VendorId = value; }
            }
            private String _VendorId;

            [Category("Response")]
            [Description("Contains sixteen bytes of ASCII data that indicates the product ID, as defined by the vendor. The data shall be left-aligned within this field and the unused bytes filled with ASCII blanks.")]
            public String ProductId
            {
                get { return _ProductId; }
                private set { _ProductId = value; }
            }
            private String _ProductId;

            [Category("Response")]
            [Description("Contains four bytes of ASCII data that indicates the product revision level, as defined by the vendor.")]
            public String ProductRevisionLevel
            {
                get { return _ProductRevisionLevel; }
                private set { _ProductRevisionLevel = value; }
            }
            private String _ProductRevisionLevel;

            [Category("Response")]
            [Description("Contains 20 bytes of vendor-specific data.")]
            public Byte[] VendorSpecific
            {
                get { return _VendorSpecific; }
                private set { _VendorSpecific = value; }
            }
            private Byte[] _VendorSpecific = new Byte[20];

            [Category("Response")]
            [Description("Reserved.")]
            public Byte[] Reserved3
            {
                get { return _Reserved3; }
                private set { _Reserved3 = value; }
            }
            private Byte[] _Reserved3 = new Byte[40];

            #endregion // Response

        } // class Inquiry

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ReadCapacity
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class ReadCapacity : ScsiFormalApi
        {
            // Constructor
            public ReadCapacity()
                : base(ScsiFormalApi.CommandSet.ReadCapacity, Api.CommandType.ScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                TransferSize = 0x08; // sizeof(READ_CAPACITY_DATA)
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                Array.Reverse(data, 0, sizeof(UInt32));
                LogicalBlockAddress = BitConverter.ToUInt32(data, 0);
                Array.Reverse(data, 4, sizeof(UInt32));
                BytesPerBlock = BitConverter.ToUInt32(data, 4);

                TotalSize = (LogicalBlockAddress + 1) * BytesPerBlock;

                Utils.DecimalConverterEx cvtr = new Utils.DecimalConverterEx();

                ResponseString =
                    String.Format(" LogicalBlockAddress: {0}\r\n", cvtr.ConvertToString(LogicalBlockAddress)) +
                    String.Format(" BytesPerBlock: {0}\r\n", cvtr.ConvertToString(BytesPerBlock)) +
                    String.Format(" TotalSize: {0}, 0x{1:X} ({2}) bytes\r\n", Utils.Utils.ScaleBytes(TotalSize), TotalSize, TotalSize.ToString("#,#0"));

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 LogicalBlockAddress
            {
                get { return _LogicalBlockAddress; }
                private set { _LogicalBlockAddress = value; }
            }
            private UInt32 _LogicalBlockAddress;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 BytesPerBlock
            {
                get { return _BytesPerBlock; }
                private set { _BytesPerBlock = value; }
            }
            private UInt32 _BytesPerBlock;

            [Category("Response")]
            [TypeConverter(typeof(Utils.ByteFormatConverter))]
            public UInt64 TotalSize
            {
                get { return _TotalSize; }
                private set { _TotalSize = value; }
            }
            private UInt64 _TotalSize;

            #endregion

        } // class ReadCapacity

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// Read
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class Read : ScsiFormalApi
        {
            // Constructor
            public Read(UInt16 sectorSize, UInt32 sectorStart, UInt16 sectorCount)
                : base(ScsiFormalApi.CommandSet.Read, Api.CommandType.ScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("RelativeAddress", (Byte)0);
                _CDB.Add("LogicalBlockByte0", sectorStart);
                _CDB.Add("Reserved", (Byte)0);
                _CDB.Add("TransferBlocksMsb", sectorCount);

                SectorSize = sectorSize;
            }

            #region Parameter Properties

            // Parameters
            [Category("Parameters"), Description("The size in bytes of a sector on the media.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 SectorSize
            {
                get { return _SectorSize; }
                set
                {
                    _SectorSize = value;
                    TransferSize = (UInt32)_SectorSize * SectorCount;
                }
            }
            private UInt16 _SectorSize;

            [Category("Parameters"), Description("The starting sector.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SectorStart
            {
                get { return (UInt32)_CDB.FromField("LogicalBlockByte0"); }
                set { _CDB.ToField("LogicalBlockByte0", value); }
            }

            [Category("Parameters"), Description("The number of sectors to read.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 SectorCount
            {
                get { return (UInt16)_CDB.FromField("TransferBlocksMsb"); }
                set
                {
                    _CDB.ToField("TransferBlocksMsb", value);
                    TransferSize = (UInt32)value * SectorSize;
                }
            }

            #endregion

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

            [Browsable(false)]
            override public String ResponseString
            {
                get
                {
                    return FormatReadResponse(Data, 16, 1);
                }
            }

            #endregion // Response

        } // class Read

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// Write
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class Write : ScsiFormalApi
        {
            // Constructor
            public Write(UInt16 sectorSize, UInt32 sectorStart, UInt16 sectorCount, Byte[] data)
                : base(ScsiFormalApi.CommandSet.Write, Api.CommandType.ScsiCmd, Api.CommandDirection.WriteWithData, DefaultTimeout)
            {
                // Parameters
                _CDB.Add("RelativeAddress", (Byte)0);
                _CDB.Add("LogicalBlockByte0", sectorStart);
                _CDB.Add("Reserved", (Byte)0);
                _CDB.Add("TransferBlocksMsb", sectorCount);

                SectorSize = sectorSize;
                Data = data;
            }

            #region Parameter Properties

            // Parameters
            [Category("Parameters"), Description("The size in bytes of a sector on the media.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 SectorSize
            {
                get { return _SectorSize; }
                set
                {
                    _SectorSize = value;
                    TransferSize = (UInt32)_SectorSize * SectorCount;
                    Byte[] tempData = Data;
                    Data = new Byte[TransferSize];
                    Array.Copy(tempData, 0, Data, 0, Math.Min(tempData.Length, Data.Length));
                }
            }
            private UInt16 _SectorSize;

            [Category("Parameters"), Description("The starting sector.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt32 SectorStart
            {
                get { return (UInt32)_CDB.FromField("LogicalBlockByte0"); }
                set { _CDB.ToField("LogicalBlockByte0", value); }
            }

            [Category("Parameters"), Description("The number of sectors to write.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 SectorCount
            {
                get { return (UInt16)_CDB.FromField("TransferBlocksMsb"); }
                set
                {
                    _CDB.ToField("TransferBlocksMsb", value);
                    TransferSize = (UInt32)value * SectorSize;
                    Byte[] tempData = Data;
                    Data = new Byte[TransferSize];
                    Array.Copy(tempData, 0, Data, 0, Math.Min(tempData.Length, Data.Length));
                }
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

            #endregion

        } // class Write

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ModeSense10
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class ModeSense10 : ScsiFormalApi
        {
            public const Byte ModePageFlexibile = 0x05; // disk

            // Constructor
            public ModeSense10()
                : base(ScsiFormalApi.CommandSet.ModeSense10, Api.CommandType.ScsiCmd, Api.CommandDirection.ReadWithData, DefaultTimeout)
            {
                _CDB.Add("LUN", (Byte)0);                   // cdb[1]
                _CDB.Add("PageCode", ModePageFlexibile);    // cdb[2]
                _CDB.Add("Reserved3", (UInt32)0);           // cdb[3], cdb[4], cdb[5], cdb[6]
                _CDB.Add("AllocationLength", (UInt16)0x28); // cdb[7], cdb[8] = sizeof(MODE_FLEXIBLE_DISK_PAGE)
                _CDB.Add("Control", (Byte)0);               // cdb[9]

                TransferSize = 0x28; // sizeof(MODE_FLEXIBLE_DISK_PAGE)
            }

            // Base class override implementations
            public override Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count)
            {
                Debug.Assert(count >= TransferSize);

                PageCode = (Byte) (data[0] & 0x3F);
                PageSavable = (data[0] & 0x80) == 0x80;
                PageLength = data[1];
                Array.Reverse(data, 2, sizeof(UInt16));
                TransferRate = BitConverter.ToUInt16(data, 2);
                NumberOfHeads = data[4];
                SectorsPerTrack = data[5];
                Array.Reverse(data, 6, sizeof(UInt16));
                BytesPerSector = BitConverter.ToUInt16(data, 6);
                Array.Reverse(data, 8, sizeof(UInt16));
                NumberOfCylinders = BitConverter.ToUInt16(data, 8);
                Array.Reverse(data, 10, sizeof(UInt16));
                StartWritePrecom = BitConverter.ToUInt16(data, 10);
                Array.Reverse(data, 12, sizeof(UInt16));
                StartReducedCurrent = BitConverter.ToUInt16(data, 12);
                Array.Reverse(data, 14, sizeof(UInt16));
                StepRate = BitConverter.ToUInt16(data, 14);
                StepPluseWidth = data[16];
                Array.Reverse(data, 17, sizeof(UInt16));
                HeadSettleDelay = BitConverter.ToUInt16(data, 17);
                /*
                TODO: Add below:
                UCHAR MotorOnDelay;
                UCHAR MotorOffDelay;
                UCHAR Reserved2 : 5;
                UCHAR MotorOnAsserted : 1;
                UCHAR StartSectorNumber : 1;
                UCHAR TrueReadySignal : 1;
                UCHAR StepPlusePerCyclynder : 4;
                UCHAR Reserved3 : 4;
                UCHAR WriteCompenstation;
                UCHAR HeadLoadDelay;
                UCHAR HeadUnloadDelay;
                UCHAR Pin2Usage : 4;
                UCHAR Pin34Usage : 4;
                UCHAR Pin1Usage : 4;
                UCHAR Pin4Usage : 4;
                UCHAR MediumRotationRate[2];
                UCHAR Reserved4[2];
                */

                Utils.DecimalConverterEx cvtr = new Utils.DecimalConverterEx();

                ResponseString =
                    String.Format(" PageCode: {0}\r\n", cvtr.ConvertToString(PageCode)) +
                    String.Format(" PageSavable: {0}\r\n", PageSavable) +
                    String.Format(" PageLength: {0}\r\n", cvtr.ConvertToString(PageLength)) +
                    String.Format(" TransferRate: {0}\r\n", cvtr.ConvertToString(TransferRate)) +
                    String.Format(" NumberOfHeads: {0}\r\n", cvtr.ConvertToString(NumberOfHeads)) +
                    String.Format(" SectorsPerTrack: {0}\r\n", cvtr.ConvertToString(SectorsPerTrack)) +
                    String.Format(" BytesPerSector: {0}\r\n", cvtr.ConvertToString(BytesPerSector)) +
                    String.Format(" NumberOfCylinders: {0}\r\n", cvtr.ConvertToString(NumberOfCylinders)) +
                    String.Format(" StartWritePrecom: {0}\r\n", cvtr.ConvertToString(StartWritePrecom)) +
                    String.Format(" StartReducedCurrent: {0}\r\n", cvtr.ConvertToString(StartReducedCurrent)) +
                    String.Format(" StepRate: {0}\r\n", cvtr.ConvertToString(StepRate)) +
                    String.Format(" StepPluseWidth: {0}\r\n", cvtr.ConvertToString(StepPluseWidth)) +
                    String.Format(" HeadSettleDelay: {0}\r\n", cvtr.ConvertToString(HeadSettleDelay));
                    /*
                    TODO: Add below:
                    UCHAR MotorOnDelay;
                    UCHAR MotorOffDelay;
                    UCHAR Reserved2 : 5;
                    UCHAR MotorOnAsserted : 1;
                    UCHAR StartSectorNumber : 1;
                    UCHAR TrueReadySignal : 1;
                    UCHAR StepPlusePerCyclynder : 4;
                    UCHAR Reserved3 : 4;
                    UCHAR WriteCompenstation;
                    UCHAR HeadLoadDelay;
                    UCHAR HeadUnloadDelay;
                    UCHAR Pin2Usage : 4;
                    UCHAR Pin34Usage : 4;
                    UCHAR Pin1Usage : 4;
                    UCHAR Pin4Usage : 4;
                    UCHAR MediumRotationRate[2];
                    UCHAR Reserved4[2];
                    */

                return Win32.ERROR_SUCCESS;
            }

            #region Response Properties
            
            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte PageCode
            {
                get { return _PageCode; }
                private set { _PageCode = value; }
            }
            private Byte _PageCode;

            [Category("Response")]
            public bool PageSavable
            {
                get { return _PageSavable; }
                private set { _PageSavable = value; }
            }
            private bool _PageSavable;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte PageLength
            {
                get { return _PageLength; }
                private set { _PageLength = value; }
            }
            private Byte _PageLength;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 TransferRate
            {
                get { return _TransferRate; }
                private set { _TransferRate = value; }
            }
            private UInt16 _TransferRate;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte NumberOfHeads
            {
                get { return _NumberOfHeads; }
                private set { _NumberOfHeads = value; }
            }
            private Byte _NumberOfHeads;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte SectorsPerTrack
            {
                get { return _SectorsPerTrack; }
                private set { _SectorsPerTrack = value; }
            }
            private Byte _SectorsPerTrack;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 BytesPerSector
            {
                get { return _BytesPerSector; }
                private set { _BytesPerSector = value; }
            }
            private UInt16 _BytesPerSector;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 NumberOfCylinders
            {
                get { return _NumberOfCylinders; }
                private set { _NumberOfCylinders = value; }
            }
            private UInt16 _NumberOfCylinders;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 StartWritePrecom
            {
                get { return _StartWritePrecom; }
                private set { _StartWritePrecom = value; }
            }
            private UInt16 _StartWritePrecom;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 StartReducedCurrent
            {
                get { return _StartReducedCurrent; }
                private set { _StartReducedCurrent = value; }
            }
            private UInt16 _StartReducedCurrent;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 StepRate
            {
                get { return _StepRate; }
                private set { _StepRate = value; }
            }
            private UInt16 _StepRate;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte StepPluseWidth
            {
                get { return _StepPluseWidth; }
                private set { _StepPluseWidth = value; }
            }
            private Byte _StepPluseWidth;

            [Category("Response")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public UInt16 HeadSettleDelay
            {
                get { return _HeadSettleDelay; }
                private set { _HeadSettleDelay = value; }
            }
            private UInt16 _HeadSettleDelay;

            /*
            TODO: Add below:
            UCHAR MotorOnDelay;
            UCHAR MotorOffDelay;
            UCHAR Reserved2 : 5;
            UCHAR MotorOnAsserted : 1;
            UCHAR StartSectorNumber : 1;
            UCHAR TrueReadySignal : 1;
            UCHAR StepPlusePerCyclynder : 4;
            UCHAR Reserved3 : 4;
            UCHAR WriteCompenstation;
            UCHAR HeadLoadDelay;
            UCHAR HeadUnloadDelay;
            UCHAR Pin2Usage : 4;
            UCHAR Pin34Usage : 4;
            UCHAR Pin1Usage : 4;
            UCHAR Pin4Usage : 4;
            UCHAR MediumRotationRate[2];
            UCHAR Reserved4[2];
            */
            #endregion

        } // class ModeSense10

        public class StartStop : ScsiFormalApi
        {
            // Constructor
            public StartStop(Boolean immediate, Byte logicalUnitNumber, Boolean start, Boolean loadEject)
                : base(ScsiFormalApi.CommandSet.StartStop, Api.CommandType.ScsiCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
                // cdb[1]
                _CDB.Add("ImmediateLun", (Byte)(Convert.ToByte(immediate) | (logicalUnitNumber << 5)));
                // cdb[2], cdb[3]
                _CDB.Add("Reserved2", (UInt16)0);
                // cdb[4]
                _CDB.Add("StartLoadEject", (Byte)(Convert.ToByte(start) | (Convert.ToByte(loadEject)<<1)));
                // cdb[5]
                _CDB.Add("Control", (Byte)0);
            }
        
/*            
           struct _START_STOP {
                UCHAR OperationCode;    // 0x1B - SCSIOP_START_STOP_UNIT
                UCHAR Immediate: 1;
                UCHAR Reserved1 : 4;
                UCHAR LogicalUnitNumber : 3;
                UCHAR Reserved2[2];
                UCHAR Start : 1;
                UCHAR LoadEject : 1;
                UCHAR Reserved3 : 6;
                UCHAR Control;
           } START_STOP, *PSTART_STOP;
*/
            #region Parameter Properties

            // Parameters
            [Category("Parameters"), Description("Take action immediately.")]
            public Boolean Immediate
            {
                get { return Convert.ToBoolean((Byte)_CDB.FromField("ImmediateLun") & 0x01); }
                set { _CDB.ToField("ImmediateLun", (Byte)(Convert.ToByte(value) | (LogicalUnitNumber << 5))); }
            }

            [Category("Parameters"), Description("Logical Unit Number.")]
            [TypeConverter(typeof(Utils.DecimalConverterEx))]
            public Byte LogicalUnitNumber
            {
                get { return (Byte)(((Byte)_CDB.FromField("ImmediateLun") & 0xE0)>>5); }
                set { _CDB.ToField("ImmediateLun", (Byte)((value << 5) | Convert.ToByte(Immediate))); }
            }

            [Category("Parameters"), Description("Start = true, Stop = false.")]
            public Boolean Start
            {
                get { return Convert.ToBoolean((Byte)_CDB.FromField("StartLoadEject") & 0x01); }
                set { _CDB.ToField("StartLoadEject", (Byte)(Convert.ToByte(value) | (LoadEject ? 0x02 : 0x00))); }
            }

            [Category("Parameters"), Description("Move the tray.")]
            public Boolean LoadEject
            {
                get { return Convert.ToBoolean((Byte)_CDB.FromField("StartLoadEject") & 0x02); }
                set { _CDB.ToField("StartLoadEject", (Byte)((Convert.ToByte(value) << 1) | (Start ? 0x01 : 0x00))); }
            }

            #endregion
        } // class StartStop

    } // class ScsiFormalApi

} // namespace DevSupport.Api
