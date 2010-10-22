/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
/*
 * Please leave this Copyright notice in your code if you use it
 * Written by Decebal Mihailescu [http://www.codeproject.com/script/articles/list_articles.asp?userid=634640]
 */
using System;
using System.Collections.Generic;
using System.ComponentModel.Design;
using System.Drawing.Design;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Reflection;
using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace Utils
{
    #region enum classes
    /// <summary>
    /// Additional Enum helpers
    /// </summary>
    public static class EnumHelper<EnumType> where EnumType : struct, IComparable, IConvertible, IFormattable
    {
        /// <summary>
        /// parses a string into the generic enum type
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static EnumType Parse(string value)
        {
            if (typeof(EnumType).IsEnum)
                return (EnumType)Enum.Parse(typeof(EnumType), value);
            throw new System.ArgumentException(string.Format("{0} is not an Enum.", typeof(EnumType).Name));
        }
        /// <summary>
        /// enum values as a strong typed Array
        /// </summary>
        /// <returns></returns>
        public static EnumType[] GetValuesInArray()
        {
            return GetValuesInEnumeration().ToArray<EnumType>();
        }
        /// <summary>
        /// enum values as a strong typed List
        /// </summary>
        /// <returns></returns>
        public static IList<EnumType> GetValuesInList()
        {
            return GetValuesInEnumeration().ToList<EnumType>();
        }
        /// <summary>
        /// enum values as a strong typed Enumeration
        /// </summary>
        /// <returns></returns>
        public static IEnumerable<EnumType> GetValuesInEnumeration()
        {
            if (typeof(EnumType).IsEnum)
                return System.Enum.GetValues(typeof(EnumType)).Cast<EnumType>();
            throw new System.ArgumentException(string.Format("{0} is not an Enum.", typeof(EnumType).Name));
        }
        /// <summary>
        /// gets only the descriptions of the enum items
        /// </summary>
        /// <returns></returns>
        public static List<string> GetItemDescriptionsInEnum()
        {
            if (!typeof(EnumType).IsEnum)
                throw new System.ArgumentException(string.Format("{0} is not an Enum.", typeof(EnumType).Name));
            Array ar = System.Enum.GetValues(typeof(EnumType));
            List<string> lst =  new List<string>(ar.Length);
            foreach (object item in ar)
            {
                Enum en = (Enum)item;
                string des = en.GetDescription();
                if (!string.IsNullOrEmpty(des))
                    lst.Add(des);
            }
            return lst;
            //return System.Enum.GetValues(typeof(EnumType)).Cast<EnumType>().ToList<EnumType>().ConvertAll<string>(delegate(EnumType val)
            //{
            //    Enum en = (Enum)(object)val;
            //    return en.GetDescription();
            //});           
        }

        /// <summary>
        /// converts ordinal types or strings to Enum
        /// </summary>
        /// <param name="value"></param>
        /// <param name="retv"></param>
        /// <returns></returns>
        public static bool SafeConvertToEnum(object value, out EnumType retv)
        {
            Type enumType = typeof(EnumType);
            if (!enumType.IsEnum)
                throw new System.ArgumentException(string.Format("{0} is not an Enum.", enumType.Name));
            if (value == null)
            {
                retv = default(EnumType);
                return false;
            }
            Type valType = value.GetType();
            bool isString = valType == typeof(string);
            bool isOrdinal = valType.IsPrimitive || typeof(decimal) == valType || valType.IsEnum;
            if (!isOrdinal && !isString)
                throw new System.ArgumentException(string.Format("{0} can not be converted to an enum", valType.Name));

            try
            {
                checked
                {
                    if (valType == Enum.GetUnderlyingType(enumType))
                        retv = (EnumType)value;
                    else
                    {
                        if(isString)
                            retv = (EnumType) Enum.Parse(typeof(EnumType), value as string);
                        else
                            if (valType.IsEnum)
                            {
                                Enum en = (Enum)value;
                                object zero = Activator.CreateInstance(valType);
                                value = (en.CompareTo(zero) >= 0)?Convert.ToUInt64(value):Convert.ToUInt64(value);
                            }
                                retv = (EnumType)Enum.Parse(typeof(EnumType), value.ToString());
                    }
                }
                if (!((System.Enum)(object)retv).IsValidEnumValue())
                {

                    retv = default(EnumType);
                    return false;
                }
            }
            catch(ArgumentException)
            {
                retv = default(EnumType);
                return false;
            }
            catch (OverflowException)
            {
                retv = default(EnumType);
                return false;
            }
            catch (InvalidCastException)
            {
                retv = default(EnumType);
                return false;
            }
            catch (Exception ex)
            {
                throw new System.ArgumentException(string.Format("Can't convert value {0}\nfrom the type of {1} into the underlying enum type of {2}\nbecause {3}",
                    value, valType.Name, Enum.GetUnderlyingType(enumType).Name, ex.Message), ex);
            }
            return true;
        }
    }

    /// <summary>
    /// provides extender methods for enum values
    /// </summary>
    public static class EnumExtenders
    {

        /// <summary>
        /// Get the description attribute from an enumerated item
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static string GetDescription(this Enum value)
        {
            FieldInfo field = value.GetType().GetField(value.ToString());
            if (field == null)
                return string.Empty;
            DescriptionAttribute[] attributes = (DescriptionAttribute[])field.GetCustomAttributes(typeof(DescriptionAttribute), false);
            return (attributes.Length > 0) ? attributes[0].Description : string.Empty;
        }
        /// <summary>
        /// check among the enumeration values
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static bool IsDefined(this System.Enum value)
        {
            return System.Enum.IsDefined(value.GetType(), value);
        }

        /// <summary>
        /// validates the value against the real enumeration
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static bool IsValidEnumValue(this System.Enum value)
        {
            if (value.HasFlags())
                return IsFlagsEnumDefined(value);
            else
                return value.IsDefined();
        }

        /// <summary>
        /// checks an enum with flags attribute for validity
        /// </summary>
        /// <param name="value"></param>
        /// <returns>true if the value can be constructed or exists</returns>
        private static bool IsFlagsEnumDefined(System.Enum value)
        {// modeled after Enum's InternalFlagsFormat
            Type underlyingenumtype = Enum.GetUnderlyingType(value.GetType());
            switch (Type.GetTypeCode(underlyingenumtype))
            {
                case TypeCode.Int16:
                case TypeCode.Int32:
                case TypeCode.Int64:
                case TypeCode.SByte:
                case TypeCode.Single:
                    {
                        object obj = Activator.CreateInstance(underlyingenumtype);
                        long svalue = System.Convert.ToInt64(value);
                        if (svalue < 0)
                            throw new ArgumentException(
                                string.Format("Can't process negative {0} as {1} enum with flags", svalue, value.GetType().Name));
                    }
                    break;
                default:
                    break;
            }

            ulong flagsset = System.Convert.ToUInt64(value);
            Array values = Enum.GetValues(value.GetType());//.Cast<ulong>().ToArray<ulong>();
            int flagno = values.Length - 1;
            ulong initialflags = flagsset;
            ulong flag = 0;
            //start with the highest values
            while (flagno >= 0)
            {
                flag = System.Convert.ToUInt64(values.GetValue(flagno));
                if ((flagno == 0) && (flag == 0))
                {
                    break;
                }
                //if the flags set contain this flag
                if ((flagsset & flag) == flag)
                {
                    //unset this flag
                    flagsset -= flag;
                    if (flagsset == 0)
                        return true;
                }
                flagno--;
            }
            if (flagsset != 0)
            {
                return false;
            }
            if (initialflags != 0 || flag == 0)
            {
                return true;
            }
            return false;
        }

        /// <summary>
        /// tells if it has the flags attribute or not
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static bool HasFlags(this System.Enum value)
        {
            return value.GetType().GetCustomAttributes(typeof(System.FlagsAttribute), false).Length > 0;
        }
    }
    #endregion

    #region Type Converters

    public class DecimalConverterEx : DecimalConverter
    {
//        public DecimalConverterEx(Type type) { ConvertToType = type; }
//        private Type ConvertToType;

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            string s = value as string;
            if (s != null)
            {
                System.Globalization.NumberStyles numStyle;
                String[] sArr = s.Split(' ');
                if (sArr.Length > 1)
                    s = sArr[0];

                if (s.StartsWith("0x"))
                {
                    s = s.Substring(2);
                    numStyle = System.Globalization.NumberStyles.HexNumber;
                }
                else
                {
                    s = s.Replace("(", ""); s = s.Replace(")", "");
                    numStyle = System.Globalization.NumberStyles.Integer | System.Globalization.NumberStyles.AllowThousands;
                }

                if (context.PropertyDescriptor.PropertyType == typeof(Byte))
                    return Byte.Parse(s, numStyle);
                else if (context.PropertyDescriptor.PropertyType == typeof(SByte))
                    return SByte.Parse(s, numStyle);
                else if (context.PropertyDescriptor.PropertyType == typeof(UInt16))
                    return UInt16.Parse(s, numStyle);
                else if (context.PropertyDescriptor.PropertyType == typeof(Int16))
                    return Int16.Parse(s, numStyle);
                else if (context.PropertyDescriptor.PropertyType == typeof(UInt32))
                    return UInt32.Parse(s, numStyle);
                else if (context.PropertyDescriptor.PropertyType == typeof(Int32))
                    return Int32.Parse(s, numStyle);
                else if (context.PropertyDescriptor.PropertyType == typeof(UInt64))
                    return UInt64.Parse(s, numStyle);
                else if (context.PropertyDescriptor.PropertyType == typeof(Int64))
                    return Int64.Parse(s, numStyle);
                else
                    throw new NotSupportedException();
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            String formatString = "0x{0:X" + Convert.ToString(Marshal.SizeOf(value) * 2) + "} ({1})";

            UInt64 uint64;
            if (UInt64.TryParse(value.ToString(), out uint64))
            {
                return destinationType == typeof(string)
                    ? String.Format(formatString, uint64, uint64.ToString("#,#0"))
                    : base.ConvertTo(context, culture, value, destinationType);
            }
            else
            {
                return destinationType == typeof(string)
                    ? String.Format(formatString, value, Convert.ToInt64(value).ToString("#,#0"))
                    : base.ConvertTo(context, culture, value, destinationType);
            }
        }
    }

    public class ByteFormatConverter : DecimalConverter
    {
        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            string s = value as string;
            if (s != null)
            {
                String[] sArr = s.Split(' ');
                Double size = 0;

                if (sArr.Length > 1)
                {
                    if (sArr[1] == "GB")
                        size = Convert.ToDouble(sArr[0]) * 1024 * 1024 * 1024;
                    else if (sArr[1] == "MB")
                        size = Convert.ToDouble(sArr[0]) * 1024 * 1024;
                    else if (sArr[1] == "KB")
                        size = Convert.ToDouble(sArr[0]) * 1024;
                    else
                        size = Convert.ToDouble(sArr[0]);

                    return Convert.ToInt64(size);
                }
                else if (sArr.Length == 1)
                {
                    return Convert.ToInt64(sArr[0]);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }
        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            return destinationType == typeof(string)
                ? Utils.ScaleBytes(value) + String.Format(" ({0} bytes)", Convert.ToUInt64(value).ToString("#,#0"))
                : base.ConvertTo(context, culture, value, destinationType);
        }
    }

    public class EnumConverterEx : EnumConverter
    {
        public EnumConverterEx(Type enumType) : base(enumType) { }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            string s = value as string;
            if (s != null)
            {
                String[] sArr = s.Split(' ');
                return Enum.Parse(EnumType, sArr[0]);
            }
            return base.ConvertFrom(context, culture, value);
        }
        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            return destinationType == typeof(string)
                ? String.Format("{0} (0x{0:X})", value)
                : base.ConvertTo(context, culture, value, destinationType);
        }
    }

    public class BinaryEditorEx : UITypeEditor
    {
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            //return base.GetEditStyle(context);
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            // Attempts to obtain an IWindowsFormsEditorService.
            IWindowsFormsEditorService edSvc =
                (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
            if (edSvc == null)
            {
                return null;
            }

            // Displays a BinaryEditorExDialog Form to get a user-adjustable 
            // byte[] value.
            using (BinaryEditorExDialog form = new BinaryEditorExDialog((Byte[])value))
            {
                if (edSvc.ShowDialog(form) == DialogResult.OK)
                {
                    return form.Data;
                }
            }

            // If OK was not pressed, return the original value
            return value;
        }
    }

    #endregion

    static public class Utils
    {
        public static String StringFromAsciiBytes(Byte[] bytes)
        {
            System.Text.ASCIIEncoding encoder = new System.Text.ASCIIEncoding();
            return encoder.GetString(bytes);
        }

        public static String StringFromAsciiBytes(Byte[] bytes, int byteIndex, int byteCount)
        {
            System.Text.ASCIIEncoding encoder = new System.Text.ASCIIEncoding();
            return encoder.GetString(bytes, byteIndex, byteCount);
        }

        public static Byte[] StringToAsciiBytes(String inputString)
        {
            System.Text.ASCIIEncoding  encoder = new System.Text.ASCIIEncoding();
            return encoder.GetBytes(inputString);
        }

        public static String StringFromUnicodeBytes(Byte[] bytes)
        {
            System.Text.UnicodeEncoding encoder = new System.Text.UnicodeEncoding();
            return encoder.GetString(bytes);
        }

        public static String StringFromHexBytes(Byte[] bytes, String spaces)
        {
            String responseStr = String.Empty;

            foreach (Byte dataByte in bytes )
            {
                responseStr += String.Format("{0:X2}{1}", dataByte, spaces);
            }
            return responseStr;
        }

        // GB, MB, KB, bytes
        public static String ScaleBytes(object sizeInBytes)
        {
            double originalSize = Convert.ToDouble(sizeInBytes);
            double scaledSize = 0;

            scaledSize = originalSize / (1024 * 1024 * 1024);
            if (scaledSize > 1.0) // GB
            {
                return scaledSize.ToString("#,#.###' GB'");
            }
            
            scaledSize = originalSize / (1024 * 1024);
            if (scaledSize > 1.0) // MB
            {
                return scaledSize.ToString("#.###' MB'");
            }

            scaledSize = originalSize / 1024;
            if (scaledSize > 1.0) // KB
            {
                return scaledSize.ToString("#.###' KB'");
            }
            else // bytes
            {
                return originalSize.ToString("#,0' bytes'");
            }
        } // ScaleBytes()
/*
        public static object RawDeserialize( byte[] rawData, int position, Type anyType )
        {
            int rawsize = Marshal.SizeOf( anyType );
            if( rawsize > rawData.Length )
                return null;
            
            IntPtr buffer = Marshal.AllocHGlobal( rawsize );
            
            Marshal.Copy( rawData, position, buffer, rawsize );
            object retobj = Marshal.PtrToStructure( buffer, anyType );
            
            Marshal.FreeHGlobal( buffer );
            
            return retobj;
        }

        public static byte[] RawSerialize( object anything )
        {
            int rawSize = Marshal.SizeOf( anything );
            IntPtr buffer = Marshal.AllocHGlobal( rawSize );
            
            Marshal.StructureToPtr( anything, buffer, false );
            byte[] rawDatas = new byte[ rawSize ];
            Marshal.Copy( buffer, rawDatas, 0, rawSize );

            Marshal.FreeHGlobal( buffer );
            
            return rawDatas;
        }
 
        public static byte[] StructureToByteArray(object obj)
        {

            int len = Marshal.SizeOf(obj);

            byte[] arr = new byte[len];

            IntPtr ptr = Marshal.AllocHGlobal(len);

            Marshal.StructureToPtr(obj, ptr, true);

            Marshal.Copy(ptr, arr, 0, len);

            Marshal.FreeHGlobal(ptr);

            return arr;

        }
*/
        public static object ByteArrayToStructure(byte[] bytes, Type structureType)
        {

            GCHandle hDataIn = GCHandle.Alloc(bytes, GCHandleType.Pinned);

            object retObject = Marshal.PtrToStructure(hDataIn.AddrOfPinnedObject(), structureType);
            
            hDataIn.Free();

            return retObject;
        }

        ////////////////////////////////////////////////////////////////////////////////
	    //! Converts a four digit BCD number to the decimal equivalent.
	    //! Remember that the BCD value is big endian but read as a 16-bit little 
	    //! endian number. So we have to byte swap during this conversion.
	    //!
	    //! \param bcdNumber BCD value in reverse byte order.
	    //! \return A decimal version of \a bcdNumber.
	    ////////////////////////////////////////////////////////////////////////////////
	    public static UInt16 BcdToDecimal(UInt16 bcdNumber)
        {
            UInt16 resultVersion = 0;
            resultVersion = (UInt16)((bcdNumber & 0x0000000f) * 100);
            resultVersion += (UInt16)(((bcdNumber & 0x000000f0) >> 4) * 1000);
            resultVersion += (UInt16)(((bcdNumber & 0x00000f00) >> 8) * 1);
            resultVersion += (UInt16)(((bcdNumber & 0x0000f000) >> 12) * 10);
		    return resultVersion;
	    } 


    } // class Utils
}
