/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
//using System.Collections.Generic;
using System.ComponentModel;
//using System.Diagnostics;
//using System.Runtime.InteropServices;
using PortableDeviceApiLib;
using PortableDeviceTypesLib;
using DevSupport.WPD;

namespace DevSupport.Api
{
    abstract public class WpdApi : Api
    {
        /// <summary>
        /// MTP Vendor-specific Command Set.
        /// </summary>
        public enum CommandSet : ushort
        {
//	        Undefined        = 0x97F0,
            DeviceReset      = 0x97F1,
            EraseBootmanager = 0x97F2,
            ResetToRecovery  = 0x97F3,
            ResetToUpdater   = 0x97F4,
            GetDriveInfo     = 0x97F5,
            SetUpdateFlag    = 0x97F6,
            SwitchToMsc      = 0x97F7
        }

        protected WpdApi(WpdApi.CommandSet cmd, Api.CommandType cmdType, Api.CommandDirection dir, UInt32 timeout)
            : base(cmdType, dir, timeout)
        {
            Command = cmd;

            _CommandValues = (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceValuesClass();
            _ParamVariants =
                (PortableDeviceApiLib.IPortableDevicePropVariantCollection)new PortableDevicePropVariantCollectionClass();

            switch ( dir )
            {
                case CommandDirection.NoData:
                {
                    _CommandValues.SetGuidValue(ref PortableDevicePKeys.WPD_PROPERTY_COMMON_COMMAND_CATEGORY,
                        ref MtpExtensions.WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITHOUT_DATA_PHASE.fmtid);

                    _CommandValues.SetUnsignedIntegerValue(ref PortableDevicePKeys.WPD_PROPERTY_COMMON_COMMAND_ID,
                        MtpExtensions.WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITHOUT_DATA_PHASE.pid);

                    break;
                }
                case CommandDirection.ReadWithData:
                {
                    _CommandValues.SetGuidValue(ref PortableDevicePKeys.WPD_PROPERTY_COMMON_COMMAND_CATEGORY,
                        ref MtpExtensions.WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITH_DATA_TO_READ.fmtid);

                    _CommandValues.SetUnsignedIntegerValue(ref PortableDevicePKeys.WPD_PROPERTY_COMMON_COMMAND_ID,
                        MtpExtensions.WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITH_DATA_TO_READ.pid);

                    break;
                }
                case CommandDirection.WriteWithData:
                {
                    _CommandValues.SetGuidValue(ref PortableDevicePKeys.WPD_PROPERTY_COMMON_COMMAND_CATEGORY,
                        ref MtpExtensions.WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITH_DATA_TO_WRITE.fmtid);

                    _CommandValues.SetUnsignedIntegerValue(ref PortableDevicePKeys.WPD_PROPERTY_COMMON_COMMAND_ID,
                        MtpExtensions.WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITH_DATA_TO_WRITE.pid);

                    break;
                }
                default:
                    throw (new ArgumentOutOfRangeException("dir"));

            }

            // Specify the actual MTP op-code that we want to execute here
            _CommandValues.SetUnsignedIntegerValue(ref MtpExtensions.WPD_PROPERTY_MTP_EXT_OPERATION_CODE, (UInt16)Command);

            // Add MTP parameters collection to our main parameter list
            _CommandValues.SetIPortableDevicePropVariantCollectionValue(ref MtpExtensions.WPD_PROPERTY_MTP_EXT_OPERATION_PARAMS, _ParamVariants);
        }
        protected PortableDeviceApiLib.IPortableDevicePropVariantCollection _ParamVariants;

        virtual public Int32 ProcessResponse(PortableDeviceApiLib.IPortableDevicePropVariantCollection responseParams)
        {
            UInt32 paramCount = 0;
            responseParams.GetCount(ref paramCount);
            if ( paramCount > 0 )
            {
                // The first response parameter contains the number of objects result
                PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue = WPD.Utils.ToPropVarient((UInt32)0);
                responseParams.GetAt(0, ref propvarValue);
                paramCount = WPD.Utils.PropVariantToUInt32(propvarValue);
            }
            
            return Win32.ERROR_SUCCESS;
        }

        // Parameter properties
        [Description("The Command (UInt16) for the api."), Category("General")]
        [TypeConverter(typeof(Utils.EnumConverterEx))]
        public WpdApi.CommandSet Command
        {
            get { return _Command; }
            protected set { _Command = value; }
        }
        private WpdApi.CommandSet _Command;

        [Browsable(false)]
        public PortableDeviceApiLib.IPortableDeviceValues CommandValues
        {
            get { return _CommandValues; }
        }
        private PortableDeviceApiLib.IPortableDeviceValues _CommandValues;
        
        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// DeviceReset
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class DeviceReset : WpdApi
        {
            // Constructor
            public DeviceReset()
                : base(WpdApi.CommandSet.DeviceReset, Api.CommandType.StMtpCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
            }
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// EraseBootmanager
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class EraseBootmanager : WpdApi
        {
            // Constructor
            public EraseBootmanager()
                : base(WpdApi.CommandSet.EraseBootmanager, Api.CommandType.StMtpCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
            }
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// GetDriveVersion
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class GetDriveInfo : WpdApi
        {
            // Constructor
            public GetDriveInfo(DevSupport.Media.LogicalDrive.Tag driveTag, ScsiVendorApi.LogicalDriveInfo driveInfoType)
                : base(WpdApi.CommandSet.GetDriveInfo, Api.CommandType.StMtpCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
                DriveTag = driveTag;
                DriveInfoType = driveInfoType;

//                DevSupport.Media.LogicalDriveTag tag = DriveTag;
//                ScsiVendorApi.LogicalDriveInfo info = DriveInfoType;
                
                _ParamVariants.Add(ref _DriveTag);
                _ParamVariants.Add(ref _DriveInfoType);
            }

            // Parameter properties
            [Description("The tag denoting the drive to get info from."), Category("Parameters")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public Media.LogicalDrive.Tag DriveTag
            {
                get { return (Media.LogicalDrive.Tag)WPD.Utils.PropVariantToUInt32(_DriveTag); }
                set { _DriveTag = WPD.Utils.ToPropVarient((UInt32)value); }
            }
            private PortableDeviceApiLib.tag_inner_PROPVARIANT _DriveTag;

            [Description("Type of info to get from the drive."), Category("Parameters")]
            [TypeConverter(typeof(Utils.EnumConverterEx))]
            public ScsiVendorApi.LogicalDriveInfo DriveInfoType
            {
                get { return (ScsiVendorApi.LogicalDriveInfo)WPD.Utils.PropVariantToUInt32(_DriveInfoType); }
                set
                {
                    _DriveInfoType = WPD.Utils.ToPropVarient((UInt32)value);

                    // TODO: CLW - Set TransferSize according to the type of info being asked for.
                }
            }
            private PortableDeviceApiLib.tag_inner_PROPVARIANT _DriveInfoType;

            // Response properties
            [Category("Response")]
            public Version Version
            {
                get { return _Version; }
            }
            private Version _Version;

            public override Int32 ProcessResponse(PortableDeviceApiLib.IPortableDevicePropVariantCollection responseParams)
            {
                Int32 retValue = Win32.ERROR_SUCCESS;

                UInt32 count = 0;
                responseParams.GetCount(ref count);
                
                if (count == 4)
                {
                    // The first response parameter contains the number of objects result
                    PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue = WPD.Utils.ToPropVarient((UInt32)0);

                    responseParams.GetAt(1, ref propvarValue);
                    Int32 major = WPD.Utils.PropVariantToInt32(propvarValue);

                    responseParams.GetAt(2, ref propvarValue);
                    Int32 minor = WPD.Utils.PropVariantToInt32(propvarValue);

                    responseParams.GetAt(3, ref propvarValue);
                    Int32 revision = WPD.Utils.PropVariantToInt32(propvarValue);

                    _Version = new Version(major, minor, 0, revision);

                    _ResponseString = String.Format(" {0}:\r\n -{1}: {2}\r\n", DriveTag, DriveInfoType, Version);
                }
                else
                {
                    _Version = null;
                    _ResponseString = " ERROR: Invalid number of response paramters.\r\n";
                    retValue = Win32.ERROR_INVALID_PARAMETER;
                }

                return retValue;
            }
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ResetToRecovery
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class ResetToRecovery : WpdApi
        {
            // Constructor
            public ResetToRecovery()
                : base(WpdApi.CommandSet.ResetToRecovery, Api.CommandType.StMtpCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
            }
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// ResetToUpdater
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class ResetToUpdater : WpdApi
        {
            // Constructor
            public ResetToUpdater()
                : base(WpdApi.CommandSet.ResetToUpdater, Api.CommandType.StMtpCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
            }
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// SetUpdateFlag
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class SetUpdateFlag : WpdApi
        {
            // Constructor
            public SetUpdateFlag()
                : base(WpdApi.CommandSet.SetUpdateFlag, Api.CommandType.StMtpCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
            }
        }

        /// <summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// 
        /// SwitchToMsc
        /// 
        /// ////////////////////////////////////////////////////////////////////////////////////
        /// </summary>
        public class SwitchToMsc : WpdApi
        {
            // Constructor
            public SwitchToMsc()
                : base(WpdApi.CommandSet.SwitchToMsc, Api.CommandType.StMtpCmd, Api.CommandDirection.NoData, DefaultTimeout)
            {
            }
        }

    } // class WpdApi

} // namespcae DevSupport.Api
