using System;

namespace DevSupport.WPD
{
    public class Utils
    {
        [Flags]
        public enum StgmConstants
        {
            STGM_READ = 0x0,
            STGM_WRITE = 0x1,
            STGM_READWRITE = 0x2,
            STGM_SHARE_DENY_NONE = 0x40,
            STGM_SHARE_DENY_READ = 0x30,
            STGM_SHARE_DENY_WRITE = 0x20,
            STGM_SHARE_EXCLUSIVE = 0x10,
            STGM_PRIORITY = 0x40000,
            STGM_CREATE = 0x1000,
            STGM_CONVERT = 0x20000,
            STGM_FAILIFTHERE = 0x0,
            STGM_DIRECT = 0x0,
            STGM_TRANSACTED = 0x10000,
            STGM_NOSCRATCH = 0x100000,
            STGM_NOSNAPSHOT = 0x200000,
            STGM_SIMPLE = 0x8000000,
            STGM_DIRECT_SWMR = 0x400000,
            STGM_DELETEONRELEASE = 0x4000000
        }

        /// <summary>
        /// Helper functions to package/unpackage data into/from a PROPVARIANT structure.
        /// </summary>
        ///
        public static PortableDeviceApiLib.tag_inner_PROPVARIANT ToPropVarient(String inputString)
        {
            PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue;

            // We'll use an IPortableDeviceValues object to transform the
            // string into a PROPVARIANT
            PortableDeviceApiLib.IPortableDeviceValues pValues =
                (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceTypesLib.PortableDeviceValuesClass();

            // We insert the string value into the IPortableDeviceValues object
            // using the SetStringValue method
            pValues.SetStringValue(ref PortableDevicePKeys.WPD_OBJECT_ID, inputString);

            // We then extract the string into a PROPVARIANT by using the 
            // GetValue method
            pValues.GetValue(ref PortableDevicePKeys.WPD_OBJECT_ID, out propvarValue);

            return propvarValue;
        }
        ///
        public static PortableDeviceApiLib.tag_inner_PROPVARIANT ToPropVarient(UInt32 inputValue)
        {
            PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue;

            // We'll use an IPortableDeviceValues object to transform the
            // UInt32 into a PROPVARIANT
            PortableDeviceApiLib.IPortableDeviceValues pValues =
                (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceTypesLib.PortableDeviceValuesClass();

            // We insert the UInt32 value into the IPortableDeviceValues object
            // using the SetUnsignedIntegerValue method
            pValues.SetUnsignedIntegerValue(ref PortableDevicePKeys.WPD_OBJECT_ID, inputValue);

            // We then extract the UInt32 into a PROPVARIANT by using the 
            // GetValue method
            pValues.GetValue(ref PortableDevicePKeys.WPD_OBJECT_ID, out propvarValue);

            return propvarValue;
        }
        ///
        public static Int32 PropVariantToInt32(PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue)
        {
            Int32 value;

            // We'll use an IPortableDeviceValues object to transform the
            // PROPVARIANT into a Int32.
            PortableDeviceApiLib.IPortableDeviceValues pValues =
                (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceTypesLib.PortableDeviceValuesClass();

            // We insert the PROPVARIANT value into the IPortableDeviceValues object
            // using the SetValue method.
            pValues.SetValue(ref PortableDevicePKeys.WPD_OBJECT_ID, ref propvarValue);

            // We then extract the PROPVARIANT into a Int32 by using the 
            // GetUnsignedIntegerValue method.
            pValues.GetSignedIntegerValue(ref PortableDevicePKeys.WPD_OBJECT_ID, out value);

            return value;
        }
        
        ///
        public static UInt32 PropVariantToUInt32(PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue)
        {
            UInt32 value;

            // We'll use an IPortableDeviceValues object to transform the
            // PROPVARIANT into a UInt32.
            PortableDeviceApiLib.IPortableDeviceValues pValues =
                (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceTypesLib.PortableDeviceValuesClass();

            // We insert the PROPVARIANT value into the IPortableDeviceValues object
            // using the SetValue method.
            pValues.SetValue(ref PortableDevicePKeys.WPD_OBJECT_ID, ref propvarValue);

            // We then extract the PROPVARIANT into a UInt32 by using the 
            // GetUnsignedIntegerValue method.
            pValues.GetUnsignedIntegerValue(ref PortableDevicePKeys.WPD_OBJECT_ID, out value);

            return value;
        }

        public static String PropVariantToString(PortableDeviceApiLib.tag_inner_PROPVARIANT propvarValue)
        {
            String value;

            // We'll use an IPortableDeviceValues object to transform the
            // PROPVARIANT into a String.
            PortableDeviceApiLib.IPortableDeviceValues pValues =
                (PortableDeviceApiLib.IPortableDeviceValues)new PortableDeviceTypesLib.PortableDeviceValuesClass();

            // We insert the PROPVARIANT value into the IPortableDeviceValues object
            // using the SetValue method.
            pValues.SetValue(ref PortableDevicePKeys.WPD_OBJECT_ID, ref propvarValue);

            // We then extract the PROPVARIANT into a String by using the 
            // GetStringValue method.
            pValues.GetStringValue(ref PortableDevicePKeys.WPD_OBJECT_ID, out value);

            return value;
        }
    }
} // namespace DevSupport.WPD