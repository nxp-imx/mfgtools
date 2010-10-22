/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel.Design;
using System.Configuration;
using System.IO;
using System.Text;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Xml.Serialization;
using Utils;


namespace DevSupport.Media
{
    public class FirmwareInfo
    {
        private const String FileSignature_sgtl = "sgtl";
        private const String FileSignature_STMP = "\u001ASTMP";
        private const String FileSignature_RSRC = "\u001ARSRC";
        private const String VersionLabel_Product = "Product Version:";
        private const String VersionLabel_Component = "Component Version:";

        public enum FileType
        {
	        Invalid    = 0,
            RawBinary  = 1,
            Stmp35xx   = 2,
            Stmp36xx   = 3,
	        Rsrc36xx   = 4,
            Stmp37xx   = 5,
		    Rsrc37xx   = 6 
        }

        [StructLayout(LayoutKind.Sequential, Size = 6, Pack = 1)]
        public struct Version
        {
            private UInt16 _Major;
            private UInt16 _Minor;
            private UInt16 _Revision;

            public UInt16 Major
            {
                get { return _Major; }
                private set { _Major = value; }
            }

            public UInt16 Minor
            {
                get { return _Minor; }
                private set { _Minor = value; }
            }

            public UInt16 Revision
            {
                get { return _Revision; }
                private set { _Revision = value; }
            }

            public Version(Byte[] data)
            {
                Debug.Assert(data.Length >= Marshal.SizeOf(typeof(Version)));

                _Major = BitConverter.ToUInt16(data, 0);
                _Minor = BitConverter.ToUInt16(data, 2);
                _Revision = BitConverter.ToUInt16(data, 4);
            }

            public Version(UInt16 major, UInt16 minor, UInt16 revision)
            {
                _Major = major;
                _Minor = minor;
                _Revision = revision;
            }

            public override string ToString()
            {
                return ToString("000.000.#000");
            }

            public string ToString(string format)
            {
                String[] versionSections = format.Split('.');
                String retString = Major.ToString(versionSections[0]);
                if (versionSections.Length > 1)
                {
                    retString += "." + Minor.ToString(versionSections[1]);
                    if (versionSections.Length > 2)
                    {
                        retString += "." + Revision.ToString(versionSections[2]);
                    }
                }

                return retString;
            }
        }

        [StructLayout(LayoutKind.Sequential, Size = 12)]
        public struct VersionField
        {
            public UInt16 Major;
            public UInt16 c_pad0_1;
            public UInt16 Minor;
            public UInt16 c_pad0_2;
            public UInt16 Revision;
            public UInt16 c_pad0_3;

            public Version ToVersion()
            {
                return new Version(Utils.Utils.BcdToDecimal(Major), Utils.Utils.BcdToDecimal(Minor), Utils.Utils.BcdToDecimal(Revision));
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Size = 52, Pack = 1)]
       	public struct FirstBlockHeader
	    {
		    public UInt32  RomVersion;
		    public UInt32  ImageSize;
		    public UInt32  CipherTextOffset;
		    public UInt32  UserDataOffset;
		    public UInt32  KeyTransformCode;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public Char[] Tag;
            public VersionField ProductVersion;
            public VersionField ComponentVersion;
		    public UInt16  DriveTag;
            public UInt16  reserved;
    	}

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Size = 96, Pack = 1)]
        public struct BootImageHeader
        {
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 20)]
            public Byte[] Digest;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public Char[] Signature;
            public Byte MajorVersion;
            public Byte MinorVersion;
            public UInt16 Flags;
            public UInt32 ImageBlocks;
            public UInt32 FirstBootTagBlock;
            public UInt32 FirstBootableSectionID;
            public UInt16 KeyCount;
            public UInt16 KeyDictionaryBlock;
            public UInt16 HeaderBlocks;
            public UInt16 SectionCount;
            public UInt16 SectionHeaderSize;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
            public Byte[] Padding0;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public Char[] Signature2;
            public UInt64 Timestamp;
            public VersionField ProductVersion;
            public VersionField ComponentVersion;
            public UInt16 DriveTag;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6)]
            public Byte[] Padding1;
        }

        #region Properties

        public FileType Type
        {
            get { return _Type; }
            private set { _Type = value; }
        }
        private FileType _Type;

        public String Name
        {
            get { return _Name; }
            private set { _Name = value; }
        }
        private String _Name;

        [ReadOnly(true)]
        public String FileStatus
        {
            get { return _FileStatus; }
            set { _FileStatus = value; }
        }
        private String _FileStatus = "OK";

        public DateTime Date
        {
            get { return _Date; }
            private set { _Date = value; }
        }
        private DateTime _Date;

        [TypeConverter(typeof(Utils.ByteFormatConverter))]
        public UInt32 DataSize
        {
            get { return _DataSize; }
            private set { _DataSize = value; }
        }
        private UInt32 _DataSize;

        public Version ComponentVersion
        {
            get { return _ComponentVersion; }
            private set { _ComponentVersion = value; }
        }
        private Version _ComponentVersion;

        public Version ProjectVersion
        {
            get { return _ProjectVersion; }
            private set { _ProjectVersion = value; }
        }
        private Version _ProjectVersion;

        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public UInt32 StartingOffset
        {
            get { return _StartingOffset; }
            private set { _StartingOffset = value; }
        }
        private UInt32 _StartingOffset;

        public UInt16 DriveTag
        {
            get { return _DriveTag; }
            private set { _DriveTag = value; }
        }
        private UInt16 _DriveTag;

        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public UInt16 Flags
        {
            get { return _Flags; }
            private set { _Flags = value; }
        }
        private UInt16 _Flags;

        public object Header
        {
            get { return _Header; }
            private set { _Header = value; }
        }
        private object _Header;

        #endregion

        public override string ToString()
        {
            return FileStatus;
        }

        public static UInt32 BlockAlignedSize(UInt32 fileSize, UInt32 sectorSize)
        {
            UInt32 blockAlignedSize = fileSize / sectorSize;

            if (fileSize % sectorSize > 0)
                ++blockAlignedSize;

            return blockAlignedSize *  sectorSize;
        }

        private static FirmwareInfo ParseHeader(Byte[] header)
        {
            FirmwareInfo info = new FirmwareInfo();

          	// 3700-style firmware files have a BootImageHeader at the beginning of the file. 
	        // The signature is unfortunately placed at the same offset for 36xx and 37xx.  So for
	        // 37xx we have two signatures.
            // "STMP" should be located in the m_signature field and "sgtl" in the second signature. 
            //
            // The FileTypeTag_Stmp needs to be incremented by 1 to skip the 0x1a leading byte for all but 3500-stlye
            // firmware files.
            if (header.Length >= Marshal.SizeOf(typeof(BootImageHeader)))
	        {
                BootImageHeader biHeader =
                    (BootImageHeader)Utils.Utils.ByteArrayToStructure(header, typeof(BootImageHeader));

                // check 3700-stye STMP || 3700-stye RSRC
                if ( ( FileSignature_sgtl == new String(biHeader.Signature2) ) &&
                     (FileSignature_STMP.Contains(new String(biHeader.Signature)) || FileSignature_RSRC.Contains(new String(biHeader.Signature))))
		        {
                    if (FileSignature_STMP.Contains(new String(biHeader.Signature)))
                    {
                        info.Type = FileType.Stmp37xx;
                    }
                    else
                    {
                        info.Type = FileType.Rsrc37xx;
                    }
                    
                    info.StartingOffset = 0;
                    info.ComponentVersion = biHeader.ComponentVersion.ToVersion();
                    info.ProjectVersion = biHeader.ProductVersion.ToVersion();
                    info.Flags = biHeader.Flags;
                    info.DriveTag = biHeader.DriveTag;
                    info.Header = biHeader;

                    return info;
		        }
            }

            // 3600-style firmware files have a FirstBlockHeader at the beginning of the file.
            // "STMP" or "RSRC" should be located in the tag field.
            //
            // The FileTypeTag_xxxx needs to be incremented by 1 to skip the 0x1a leading byte for all but 3500-stlye
            // firmware files.
            if (header.Length >= Marshal.SizeOf(typeof(FirstBlockHeader)))
            {
                FirstBlockHeader fbHeader =
                    (FirstBlockHeader)Utils.Utils.ByteArrayToStructure(header, typeof(FirstBlockHeader));

                // check 3600-stye STMP || 3600-stye RSRC
                if (FileSignature_STMP.Contains(new String(fbHeader.Tag)) || FileSignature_RSRC.Contains(new String(fbHeader.Tag)))    
                {
                    if (FileSignature_STMP.Contains(new String(fbHeader.Tag)))
                    {
                        info.Type = FileType.Stmp36xx;
                    }
                    else
                    {
                        info.Type = FileType.Rsrc36xx;
                    }

		            info.StartingOffset = 0;
                    info.ComponentVersion = fbHeader.ComponentVersion.ToVersion();
                    info.ProjectVersion = fbHeader.ProductVersion.ToVersion();
                    info.DriveTag = fbHeader.DriveTag;
                    info.Header = fbHeader;

                    return info;
                }
            }

            // 3500-style files have 2 lines of plain text defining the ProductVersion and ComponentVersion.
            // All information up to and including the (0x1A)STMP tag must be stripped off of the file. Thus, for 
            // 3500-style files, _startingOffset will be non-zero. For all other types, _startingOffset will be 0.
            //
            // The full FileTypeTag_Stmp should be present for 3500-style files.
            String strHeader = Utils.Utils.StringFromAsciiBytes(header);
            
            // check 3500-stye STMP
            int index = strHeader.IndexOf(FileSignature_STMP);
            if ( index != -1 )
            {
                strHeader = strHeader.Remove(index);

                info.StartingOffset = (UInt32)(index + FileSignature_STMP.Length);
                info.Type = FileType.Stmp35xx;
                info.Header = strHeader;

                // check for VersionLabel_Product and VersionLabel_Component
                Char[] delimeters = {'\r', '\n'};
                String[] versionStrings = strHeader.Split(delimeters, StringSplitOptions.RemoveEmptyEntries);
                foreach ( String str in versionStrings )
                {
                    if ( str.StartsWith(VersionLabel_Product) )
                    {
                        String versionSegment = str.Replace(VersionLabel_Product, "");
                        String[] versionParts = versionSegment.Split('.');
                        if ( versionParts.Length == 3 )
                        {
                            info.ProjectVersion = new Version(UInt16.Parse(versionParts[0]), 
                                UInt16.Parse(versionParts[1]), UInt16.Parse(versionParts[2]) );
                        }
                    }
                    else if ( str.StartsWith(VersionLabel_Component) )
                    {
                        String versionSegment = str.Replace(VersionLabel_Component, "");
                        String[] versionParts = versionSegment.Split('.');
                        if ( versionParts.Length == 3 )
                        {
                            info.ComponentVersion = new Version(UInt16.Parse(versionParts[0]), 
                                UInt16.Parse(versionParts[1]), UInt16.Parse(versionParts[2]) );
                        }
                    }
                }
                
                return info;
            }

            // Couldn't find any of our signatures so treat file as Raw Binary data
            info.StartingOffset = 0;
            info.Type = FileType.RawBinary;
            return info;
        }

        public static FirmwareInfo FromFile(String filename)
        {
            FirmwareInfo retInfo = null;

            using (FileStream fs = File.OpenRead(filename) )
            {
                Byte[] fwHeader = new Byte[1024];
                fs.Read(fwHeader, 0, 1024);

                retInfo = ParseHeader(fwHeader);
                retInfo.Name = fs.Name;
                retInfo.Date = File.GetLastWriteTime(filename);
                retInfo.DataSize = (UInt32)fs.Length - retInfo.StartingOffset;
            }

            return retInfo;
        }

        public static FirmwareInfo FromDriveArray(LogicalDrive[] driveArray, LogicalDrive.Tag driveTag)
        {
            FirmwareInfo retInfo = null;

            foreach (LogicalDrive drive in driveArray)
            {
                if (drive.DriveTag == driveTag)
                {
                    retInfo = FromFile(drive.Filename);
                    break;
                }
            }

            return retInfo;
        }

    } // class Firmware

    /// <summary>
    /// The Drive Array Entry structure returned from ScsiVendorApi.GetAllocationTable.
    /// </summary>
    [Obsolete("Sector Size is now returned instead of Drive Numbers. Please use the MediaAllocationEntryEx class.")]
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    [TypeConverter(typeof(ExpandableObjectConverter))]
    public class MediaAllocationEntry
    {
        public MediaAllocationEntry()
            : this(LogicalDrive.InvalidDriveNumber,  LogicalDrive.Type.Invalid, LogicalDrive.Tag.Invalid, 0)
        {
        }
        
        public MediaAllocationEntry(Byte number, LogicalDrive.Type type, LogicalDrive.Tag tag, Int64 sizeInBytes)
        {
            _DriveNumber = number;
            _Type = type;
            _Tag = tag;
            _SizeInBytes = sizeInBytes;
        }
        
        public MediaAllocationEntry(Byte[] bytes, ref int offset)
        {
            _DriveNumber = bytes[offset++];
            _Type = (LogicalDrive.Type)bytes[offset++];
            _Tag = (LogicalDrive.Tag)bytes[offset++];
            _SizeInBytes = BitConverter.ToInt64(bytes, offset);
            offset += sizeof(Int64);
        }

        private Byte _DriveNumber;
        private LogicalDrive.Type _Type;
        private LogicalDrive.Tag _Tag;
        private Int64 _SizeInBytes;

        public Byte DriveNumber { get { return _DriveNumber; } set { _DriveNumber = value; } }
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public LogicalDrive.Type Type { get { return _Type; } set { _Type = value; } }
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public LogicalDrive.Tag Tag { get { return _Tag; } set { _Tag = value; } }
        [TypeConverter(typeof(Utils.ByteFormatConverter))]
        public Int64 SizeInBytes { get { return _SizeInBytes; } set { _SizeInBytes = value; } }

        public override string ToString()
        {
            ByteFormatConverter converter = new ByteFormatConverter();

            String str = String.Format("num:{0}, type:{1}, tag:{2}, size:{3}", DriveNumber, Type, Tag, converter.ConvertToString(SizeInBytes));
            return str;
        }

        public Byte[] GetBytes()
        {
            List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
            bytes.Add(DriveNumber);
            bytes.Add((Byte)Type);
            bytes.Add((Byte)Tag);
            bytes.AddRange(BitConverter.GetBytes(SizeInBytes));
            return bytes.ToArray();
        }

        public static MediaAllocationEntry GetEntry(LogicalDrive.Tag tag, MediaAllocationEntry[] mediaArray)
        {
            foreach (MediaAllocationEntry entry in mediaArray)
            {
                if (entry.Tag == tag)
                    return entry;
            }

            return null;
        }
    }

    /// <summary>
    /// The Drive Array Entry structure returned from ScsiVendorApi.GetAllocationTable.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    [TypeConverter(typeof(ExpandableObjectConverter))]
    public class MediaAllocationEntryEx
    {
        public MediaAllocationEntryEx()
            : this(LogicalDrive.Type.Invalid, LogicalDrive.Tag.Invalid, 0, 0)
        {
        }

        public MediaAllocationEntryEx(LogicalDrive.Type type, LogicalDrive.Tag tag, UInt64 size, UInt32 sectorSize)
        {
            _Type = type;
            _Tag = tag;
            _Size = size;
            _SectorSize = sectorSize;
        }

        public MediaAllocationEntryEx(Byte[] bytes, ref int offset)
        {
            _Type = (LogicalDrive.Type)bytes[offset++];
            _Tag = (LogicalDrive.Tag)bytes[offset++];
            _Size = BitConverter.ToUInt64(bytes, offset);
            offset += sizeof(UInt64);
            _SectorSize = BitConverter.ToUInt32(bytes, offset);
            offset += sizeof(UInt32);
        }

        private LogicalDrive.Type _Type;
        private LogicalDrive.Tag _Tag;
        private UInt64 _Size;
        private UInt32 _SectorSize;

        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public LogicalDrive.Type Type { get { return _Type; } set { _Type = value; } }
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public LogicalDrive.Tag Tag { get { return _Tag; } set { _Tag = value; } }
        [TypeConverter(typeof(Utils.ByteFormatConverter))]
        public UInt64 Size { get { return _Size; } set { _Size = value; } }
        [TypeConverter(typeof(Utils.ByteFormatConverter))]
        public UInt32 SectorSize { get { return _SectorSize; } set { _SectorSize = value; } }

        public override string ToString()
        {
            ByteFormatConverter converter = new ByteFormatConverter();

            String str = String.Format("type:{0}, tag:{1}, size:{2}, sector size:{3}", Type, Tag, converter.ConvertToString(Size), converter.ConvertToString(SectorSize));
            return str;
        }

        public Byte[] GetBytes()
        {
            List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
            bytes.Add((Byte)Type);
            bytes.Add((Byte)Tag);
            bytes.AddRange(BitConverter.GetBytes(Size));
            bytes.AddRange(BitConverter.GetBytes(SectorSize));
            return bytes.ToArray();
        }

        public static MediaAllocationEntryEx GetEntry(LogicalDrive.Tag tag, MediaAllocationEntryEx[] mediaArray)
        {
            foreach (MediaAllocationEntryEx entry in mediaArray)
            {
                if (entry.Tag == tag)
                    return entry;
            }

            return null;
        }
    }

    /// <summary>
    /// The Drive Array Entry structure sent to the device with ScsiVendorApi.SetAllocationTable.
    /// </summary>
    [SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes")]
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    [TypeConverter(typeof(ExpandableObjectConverter))]
    [Serializable]
    public class MediaAllocationCmdEntry
	{
        private LogicalDrive.Type _Type;
        private LogicalDrive.Tag _Tag;
		private Int64 _Size;

        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public LogicalDrive.Type Type { get { return _Type; } set { _Type = value; } }
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public LogicalDrive.Tag Tag { get { return _Tag; } set { _Tag = value; } }
        [TypeConverter(typeof(Utils.ByteFormatConverter))]
        public Int64 Size { get { return _Size; } set { _Size = value; } }

        public MediaAllocationCmdEntry()
            : this(LogicalDrive.Type.Invalid, LogicalDrive.Tag.Invalid, 0)
        { }

        public MediaAllocationCmdEntry(LogicalDrive.Type type, LogicalDrive.Tag tag, Int64 size)
        {
            _Type = type;
            _Tag = tag;
            _Size = size;
        }

        public override string ToString()
        {
            ByteFormatConverter converter = new ByteFormatConverter();

            String str = String.Format("type:{0}, tag:{1}, size:{2}", Type, Tag, converter.ConvertToString(Size));
            return str;
        }

        public Byte[] GetBytes()
        {
            List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
            bytes.Add((Byte)Type);
            bytes.Add((Byte)Tag);
            bytes.AddRange(BitConverter.GetBytes(Size));
            return bytes.ToArray();
        }
    };


//    [TypeConverter(typeof(ExpandableObjectConverter))]
//    [SettingsSerializeAs(SettingsSerializeAs.Xml)]
//    [TypeConverter(typeof(ExpandableObjectConverter))]
    public class DefaultLogicalDrive
    {
        public DefaultLogicalDrive() { }
        public DefaultLogicalDrive(Byte data)
        {
            DriveNumber = data;
        }
        public Byte DriveNumber
        {
            get { return 0xFF; }
            set { _DriveNumber = value; }
        }
       private Byte _DriveNumber;
    }

    public class LogicalDriveConverter : TypeConverter
    {
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if ( sourceType == typeof(string) )
                return true;
            else
                return base.CanConvertFrom(context, sourceType);
        }

        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            return base.CanConvertTo(context, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            string s = value as string;
            if (s != null)
            {
                // "filename, description, type, tag, requestedSize, flags"
                String[] sArr = s.Split(new char[1] {','}, StringSplitOptions.RemoveEmptyEntries);

                LogicalDrive drive = new LogicalDrive(
                    sArr[0].Trim(),
                    sArr[1].Trim(),
                    Utils.EnumHelper<LogicalDrive.Type>.Parse(sArr[2].Trim()),
                    Utils.EnumHelper<LogicalDrive.Tag>.Parse(sArr[3].Trim()), 
                    0,
                    Int64.Parse(sArr[4].Trim()),
                    Utils.EnumHelper<LogicalDrive.Flags>.Parse(sArr[5].Trim()));

                return drive;
            }
            return base.ConvertFrom(context, culture, value);
        }
        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            LogicalDrive drive = value as LogicalDrive;
            return destinationType == typeof(string)
                ? drive.ToString()
                : base.ConvertTo(context, culture, value, destinationType);
        }
    }

//    [Serializable]
    [TypeConverter(typeof(LogicalDriveConverter))]
    [SettingsSerializeAs(SettingsSerializeAs.Xml)]
    public class LogicalDrive// : IXmlSerializable
    {
//        public const byte DefaultMediaTableEntries = 20; // 20 - Default number of Media Table Entries
        public const byte InvalidDriveNumber = 0xFF;     // 0xFF - Unassigned drive number
//        public LogicalDrive[] DriveArray;

        [FlagsAttribute]
        public enum Flags
        {
            None = 0x00,
            ImageData = 0x01,
            FileData = 0x02,
            Format = 0x04,
            JanusInit = 0x08
        }
        
        [SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32")]
        public enum Type : byte
        {
            Invalid = 0xFF,
            Data = 0,
            System = 1,
            HiddenData = 2,
            Unknown = 3
        }

        [SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32")]
        public enum Tag : byte
        {
            tData = 0x00,
            Player = 0x00,
            Hostlink = 0x01,
            UsbMsc = 0x01,
            tJanus = 0x02,
            FirmwareRsc = 0x02,
            FirmwareRsc2 = 0x12,
            FirmwareRsc3 = 0x22,
            PlayerRsc = 0x02,
            PlayerRsc2 = 0x12,
            PlayerRsc3 = 0x22,
            tSettings = 0x03,
            Extra = 0x03,
            ExtraRsc = 0x04,
            Otg = 0x05,
            HostlinkRsc = 0x06,
            HostlinkRsc2 = 0x16,
            HostlinkRsc3 = 0x26,
            Mark = 0x06,
            [SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase")]
            IrDA = 0x07,
            SettingsBin = 0x07,
            OtgRsc = 0x08,
            Data = 0x0A,
            Data2 = 0x1A,
            DataJanus = 0x0B,
            DataSettings = 0x0C,
            FirmwareImg = 0x50,
            Bootmanger = 0x50,
            FirmwareImg2 = 0x60,
            FirmwareImg3 = 0x70,
            Invalid = 0xF0,
            UpdaterNand = 0xFE,
            Updater = 0xFF
        }

        public LogicalDrive() 
        { 
        }

        public override string ToString()
        {
            return String.Format("{0}, {1}, {2}, {3}, {4}, {5}", Filename, Description, DriveType, DriveTag, RequestedSize, OperationFlags);
        }
        
        public LogicalDrive(String filename, String description, LogicalDrive.Type driveType, LogicalDrive.Tag driveTag, Int64 firmwareSize, Int64 requestedSize, LogicalDrive.Flags operationFlags)
        {
            Filename = filename;
            Description = description;
            DriveType = driveType;
            DriveTag = driveTag;
            FirmwareSize = firmwareSize;
            RequestedSize = requestedSize;
            OperationFlags = operationFlags;
        }

        [XmlAttribute("filename")]
        public String Filename
        {
            get { return _Filename; }
            set { _Filename = value; }
        }
        private String _Filename;

        [XmlAttribute("desc")]
        public String Description
        {
            get { return _Description; }
            set { _Description = value; }
        }
        private String _Description;

        [XmlAttribute("type")]
        public LogicalDrive.Type DriveType
        {
            get { return _DriveType; }
            set { _DriveType = value; }
        }
        private LogicalDrive.Type _DriveType;

        [XmlAttribute("tag")]
        public LogicalDrive.Tag DriveTag
        {
            get { return _DriveTag; }
            set { _DriveTag = value; }
        }
        private LogicalDrive.Tag _DriveTag;

        [XmlIgnore()]
        public Int64 FirmwareSize
        {
            get { return _FirmwareSize; }
            set { _FirmwareSize = value; }
        }
        private Int64 _FirmwareSize;

        [XmlAttribute("requestedSize")]
        public Int64 RequestedSize
        {
            get { return _RequestedSize; }
            set { _RequestedSize = value; }
        }
        private Int64 _RequestedSize;

        [XmlAttribute("flags")]
        public LogicalDrive.Flags OperationFlags
        {
            get { return _OperationFlags; }
            set { _OperationFlags = value; }
        }
        private LogicalDrive.Flags _OperationFlags;

        public static MediaAllocationCmdEntry[] ToAllocationCmdEntryArray(LogicalDrive[] driveArray)
        {
            MediaAllocationCmdEntry[] cmdArr = new MediaAllocationCmdEntry[driveArray.Length];
            for ( int index = 0; index < driveArray.Length; ++index)
            {
                cmdArr[index] = new MediaAllocationCmdEntry(driveArray[index].DriveType,
                    driveArray[index].DriveTag,
                    Math.Max(driveArray[index].FirmwareSize, driveArray[index].RequestedSize));
            }

            return cmdArr;
        }

        public static void InitFileSizes(ref LogicalDrive[] driveArray)
        {
            foreach ( LogicalDrive drive in driveArray )
            {
                if ( !String.IsNullOrEmpty(drive.Filename) )
                    drive.FirmwareSize = new FileInfo(drive.Filename).Length;
            }

        }

/*
        #region IXmlSerializable Members

        public System.Xml.Schema.XmlSchema GetSchema()
        {
            return null;
        }

        public void ReadXml(System.Xml.XmlReader reader)
        {
            XmlSerializer x = new
            XmlSerializer(typeof(LogicalDrive));

            reader.Read();
            LogicalDrive drive = x.Deserialize(reader) as LogicalDrive;

            if (drive == null)
                return;
        }

        public void WriteXml(System.Xml.XmlWriter writer)
        {
            throw new NotImplementedException();
        }

        #endregion
*/
    }

//  [SuppressMessage("Microsoft.Performance", "CA1815:OverrideEqualsAndOperatorEqualsOnValueTypes")]
    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct JanusDriveHeader 
    {
        public Byte[] First16Bytes;
        public UInt32 JanusSignature0;
        public UInt32 JanusSignature1;
        public UInt16 JanusFormatId;
        public UInt16 JanusBootSectorOffset;
        public UInt32 JanusDriveTotalSectors;

        public JanusDriveHeader(UInt32 numSectors)
        {
            First16Bytes = new Byte[16] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            JanusSignature0 = 0x80071119;
            JanusSignature1 = 0x19082879;
            JanusFormatId = 0x100;
            JanusBootSectorOffset = 20;
            JanusDriveTotalSectors = numSectors;
        }

        public Byte[] ToArray()
        {
            List<Byte> arr = new List<Byte>();
            arr.AddRange(First16Bytes);
            arr.AddRange(BitConverter.GetBytes(JanusSignature0));
            arr.AddRange(BitConverter.GetBytes(JanusSignature1));
            arr.AddRange(BitConverter.GetBytes(JanusFormatId));
            arr.AddRange(BitConverter.GetBytes(JanusBootSectorOffset));
            arr.AddRange(BitConverter.GetBytes(JanusDriveTotalSectors));

            return arr.ToArray();
        }
    }
}
