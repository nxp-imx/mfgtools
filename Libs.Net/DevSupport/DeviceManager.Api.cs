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
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Resources;
using System.Runtime.InteropServices;

using Utils;

namespace DevSupport.Api
{
    [TypeDescriptionProvider(typeof(ApiCustomTypeDescriptor))]
    public abstract class Api
    {
        public const UInt32 DefaultTimeout = 7;		// seven seconds
        public const UInt32 ExtendedTimeout = 10 * 60;	// 10 minutes
        public const UInt32 EraseMediaTimeout = 5 * 60;	// 5 minutes
        public const Int32 CmdSuccess = 0;

        public enum CommandDirection : byte 
        { 
            WriteWithData = Win32.SCSI_IOCTL_DATA_OUT,
            ReadWithData = Win32.SCSI_IOCTL_DATA_IN, 
            NoData = Win32.SCSI_IOCTL_DATA_UNSPECIFIED
        }

        protected enum CommandType { ScsiCmd, StScsiCmd, HidBltcCmd, HidPitcCmd, StMtpCmd, UtpScsiMsg, UtpScsiApi, MxRomApi, MxRamKrnlApi }

        public class MemberFunctionAttribute : Attribute
        {
            public MemberFunctionAttribute() { }
        }

        public class MemberFunctionArgAttribute : Attribute
        {
            public MemberFunctionArgAttribute() { }
        }

        public class MemberFunctionArgByRefAttribute : Attribute
        {
            public MemberFunctionArgByRefAttribute() { }
        }

        public interface IProcessResponse
        {
            Int32 ProcessResponse(Byte[] data, Int64 start, Int64 count);
        }

        public static System.Windows.Forms.ImageList ImageList
        {
            get
            {
                if (_ImageList == null)
                {
                    ResourceManager rm = new ResourceManager("DevSupport.Resources", Assembly.GetExecutingAssembly());
                    _ImageList = new System.Windows.Forms.ImageList();

                    foreach (string apiType in Enum.GetNames(typeof(CommandType)))
                    {
                        _ImageList.Images.Add(apiType, (Icon)rm.GetObject(apiType));
                    }
                }
                return _ImageList;
            }
        }
        private static System.Windows.Forms.ImageList _ImageList;

        protected Api(Api.CommandType cmdType, Api.CommandDirection readWrite, UInt32 timeout)
//            : base(TypeDescriptor.GetProvider(this).GetTypeDescriptor(this))
        {
            _CommandType = cmdType;
            _Direction = readWrite;
            _Timeout = timeout;
            _ResponseString = " OK\r\n";

//            TypeDescriptor.AddProvider(new ApiTypeDescriptionProvider(TypeDescriptor.GetProvider(this)), this);
//            TypeDescriptionProvider provider = TypeDescriptor.GetProvider(this);
//            ICustomTypeDescriptor ctd = provider.GetTypeDescriptor(this);
//            base(TypeDescriptor.GetProvider(this).GetTypeDescriptor(this));
        }

        [Description("The direction of the api. Read is from device. Write is to device."), Category("General")]
        public virtual Api.CommandDirection Direction
        {
            get { return _Direction; }
        }
        private Api.CommandDirection _Direction;

        [Description("The size in bytes the api will send/receive"), Category("General")]
        public virtual Int64 TransferSize
        {
            get { return _TransferSize; }
            /*protected*/ set { _TransferSize = value; }
        }
        protected Int64 _TransferSize;

        [Description("The number of seconds the api will wait on the device."), Category("General")]
        public virtual UInt32 Timeout
        {
            get { return _Timeout; }
            set { _Timeout = value; }
        }
        private UInt32 _Timeout;

        [Description("The string denoting the image key for the api in the Api.ImageList"), Category("General")]
        public virtual string ImageKey
        {
            get { return _CommandType.ToString(); }
        }
        private Api.CommandType _CommandType;

        [Description("The data associated with the api"), Category("Data")]
        [Browsable(false)]
        public virtual Byte[] Data
        {
            get { return _Data; }
            set { _Data = value; }
        }
        protected Byte[] _Data;

        [Browsable(false)]
        virtual public String ResponseString
        {
            get { return _ResponseString; }
            protected set { _ResponseString = value; }
        }
        protected String _ResponseString;

        public override string ToString()
        {
            return GetType().Name;
        }

        // TODO: COMBINE/MOVE THIS with DisplayData(Byte[] data, Byte width, Byte wordSize) AND
        // String StringFromHexBytes(Byte[] bytes)
        /// <summary>
		/// Creates a string representing space separated data bytes (*pByte)
		/// with a 4-digit hex address per line.
		/// </summary>
		/// <param name="data">The data to format.</param>
		/// <param name="width">Number of bytes displayed per line.</param>
		/// <param name="wordSize">Number of bytes to group together as a word.</param>
		/// <returns></returns>
        protected String FormatReadResponse( Byte[] data, Byte width, Byte wordSize)
		{
			String responseStr = " 0000:";

			for ( int i = 0; i < data.Length; i += wordSize )
			{
				switch ( wordSize )
				{
				case 4:
				{
                    Byte[] bytes = new Byte[sizeof(UInt32)];
                    Array.Copy(data, i, bytes, 0, sizeof(UInt32));
                    Array.Reverse(bytes);

                    UInt32 u32Word = BitConverter.ToUInt32(bytes, 0);
                    responseStr += String.Format(" {0:X8}", u32Word);
					break;
				}
				case 1:
				default:
                    responseStr += String.Format(" {0:X2}", data[i]);
					break;
				}
				if ( ((i+wordSize)%width == 0) && ((i+wordSize) < data.Length) )
                {
					responseStr += String.Format("\r\n {0:X4}:", i+wordSize);
				}
			}
            return responseStr + "\r\n";
		}

        public String FormatResponse(String indent, String newLine, bool trailingNewLine)
        {
            if (String.IsNullOrEmpty(newLine))
                newLine = "\r\n";

            if (String.IsNullOrEmpty(indent))
                indent = " ";

            String newResponse = indent + ResponseString;
            newResponse = newResponse.Replace("\r\n", newLine + indent);
            newResponse = newResponse.TrimEnd();

            if (trailingNewLine)
                newResponse += newLine;

            return newResponse;
        }

    } // class Api

    public class CDB
    {
        public CDB()
        {
            Fields = new Dictionary<string, FieldBase>();
        }

        // data
        private Byte[] _Cdb = new Byte[16];
        private Dictionary<String, FieldBase> Fields;

        public void Add<T>(String name, T value)
        {
            
            Fields.Add(name, new Field<T>());
            ToField(name, value);
        }

        // Helper functions for Big-Endian CDBs
        public void ToField(String name, object value)
        {
            // calculate the offset in the bytes array.
            int offset = 0;
             foreach (KeyValuePair<string, FieldBase> entry in Fields)
            {
                if (entry.Key == name)
                    break;

                offset += entry.Value.Size;
            }

            FieldBase field = Fields[name];
             
            // create a byte array of the required size
            Byte[] bytes = new Byte[field.Size];

            // convert the value to an array of bytes
            if (field.Type == typeof(Byte))
            {
                bytes[0] = (Byte)value;
            }
            else if (field.Type == typeof(UInt16))
            {
                bytes = BitConverter.GetBytes((UInt16)value);
            }
            else if (field.Type == typeof(Int16))
            {
                bytes = BitConverter.GetBytes((Int16)value);
            }
            else if (field.Type == typeof(UInt32))
            {
                bytes = BitConverter.GetBytes((UInt32)value);
            }
            else if (field.Type == typeof(Int32))
            {
                bytes = BitConverter.GetBytes((Int32)value);
            }
            else if (field.Type == typeof(UInt64))
            {
                bytes = BitConverter.GetBytes((UInt64)value);
            }
            else if (field.Type == typeof(Int64))
            {
                bytes = BitConverter.GetBytes((Int64)value);
            }
            else
            {
                throw new NotImplementedException();
            }

            // reverse the bytes for the Big-endian CDB
            Array.Reverse(bytes);

            // write the bytes to the correct place in the CDB array
            bytes.CopyTo(_Cdb, offset);
        }

        public object FromField(String name)
        {
            // calculate the offset in the byte array
            int offset = 0;
            foreach (KeyValuePair<string, FieldBase> entry in Fields)
            {
                if (entry.Key == name)
                    break;

                offset += entry.Value.Size;
            }

            FieldBase field = Fields[name];

            // create a byte array of the required size
            Byte[] bytes = new Byte[field.Size];
            Array.Copy(_Cdb, offset, bytes, 0, field.Size);
            Array.Reverse(bytes);

            // convert the byte array to the appropriate type
            if (field.Size == sizeof(Byte))
            {
                return _Cdb[offset];
            }
            else if (field.Size == sizeof(UInt16))
            {
                return BitConverter.ToUInt16(bytes, 0);
            }
            else if (field.Size == sizeof(UInt32))
            {
                return BitConverter.ToUInt32(bytes, 0);
            }
            else if (field.Size == sizeof(UInt64))
            {
                if (field.Type == typeof(Int64))
                    return BitConverter.ToInt64(bytes, 0);
                else
                    return BitConverter.ToUInt64(bytes, 0);
            }
            else
            {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// The CDB fields as a Big-endian byte array.
        /// </summary>
        /// <returns>A copy of the underlying data instead of a reference.</returns>
        public Byte[] ToByteArray()
        {
            Byte[] cdbCopy = new Byte[16];
            Array.Copy(_Cdb, cdbCopy, _Cdb.Length);
            return cdbCopy;
        }

        abstract public class FieldBase
        {
            protected FieldBase()
            {
            }

            abstract public Type Type
            {
                get;
            }

            abstract public int Size
            {
                get;
            }
        }

        public class Field<T> : FieldBase
        {
            public Field()
                : base()
            {
            }

            override public Type Type
            {
                get {
                    if (typeof(T).IsEnum == true)
                        return Enum.GetUnderlyingType(typeof(T));
                    else
                        return typeof(T);
                }
            }

            override public int Size
            {
                get
                {
                    if (typeof(T).IsEnum == true)
                        return Marshal.SizeOf(Enum.GetUnderlyingType(typeof(T)));
                    else
                        return Marshal.SizeOf(typeof(T));
                }
            }
        }
    }
    
    /// <summary>
    /// This is our custom provider. It simply provides a custom type descriptor
    /// and delegates all its other tasks to its parent 
    /// </summary>
    public sealed class ApiTypeDescriptionProvider : TypeDescriptionProvider
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public ApiTypeDescriptionProvider(TypeDescriptionProvider parent)
            : base(parent)
        {
        }
        public ApiTypeDescriptionProvider()
            : this(TypeDescriptor.GetProvider(typeof(IFilterProperties)))
        {
        }

        /// <summary>
        /// Create and return our custom type descriptor and chain it with the original 
        /// custom type descriptor
        /// </summary>
        public override ICustomTypeDescriptor GetTypeDescriptor(Type objectType, object instance)
        {
            return new ApiCustomTypeDescriptor(base.GetTypeDescriptor(objectType, instance), instance);
        }
    }

    /// <summary>
    /// This is our custom type descriptor. It creates a new property and returns it along
    /// with the original list
    /// </summary>
    internal sealed class ApiCustomTypeDescriptor : CustomTypeDescriptor
    {
        private IFilterProperties _FilterApi; 
        /// <summary>
        /// Constructor
        /// </summary>
        internal ApiCustomTypeDescriptor(ICustomTypeDescriptor parent, object instance)
            : base(parent)
        {
            _FilterApi = instance as IFilterProperties;
        }

        /// <summary>
        /// This method add a new property to the original collection
        /// </summary>
        public override PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            if (_FilterApi != null)
            {
                PropertyDescriptorCollection baseProps = base.GetProperties(attributes);

                List<PropertyDescriptor> propertyList = new List<PropertyDescriptor>();
                foreach (PropertyDescriptor prop in baseProps)
                {
                    if (prop.Category == "Response" || prop.Category == "Parameters")
                    {
                        BrowsableResponseAttribute browsableAttribute = prop.Attributes[typeof(BrowsableResponseAttribute)] as BrowsableResponseAttribute;
                        if (browsableAttribute != null)
                        {
                            if (_FilterApi.IsResponseProperyBrowsable(browsableAttribute))
                                propertyList.Add(prop);
                        }
                        else
                            propertyList.Add(prop);
                    }
                    else
                    {
                        propertyList.Add(prop);
                    }
                }

                // Finally return the list
                return new PropertyDescriptorCollection(propertyList.ToArray(), true);
            }
            else
                return base.GetProperties(attributes);
        }
    }

    public class BrowsableResponseAttribute : Attribute
    {
        private object _Criteria;

        public BrowsableResponseAttribute(object criteria)
        {
            _Criteria = criteria;
        }

        public object Criteria
        {
            get { return _Criteria; }
            set { _Criteria = value; }
        }
    }

    public interface IFilterProperties
    {
        bool IsResponseProperyBrowsable(BrowsableResponseAttribute responseAttribute);
    }

}
/*
public class RoleCustomTypeDescriptor : CustomTypeDescriptor 
{ 
    public RoleCustomTypeDescriptor(ICustomTypeDescriptor parent, object instance) 
        : base(parent) 
    { 
        _instance = instance as RoleProvider; 
    } 
    
    private RoleProvider _instance; 
    public override PropertyDescriptorCollection GetProperties() 
    { 
        return GetProperties(null);
    }
    
    public override PropertyDescriptorCollection GetProperties(Attribute[] attributes)
    { 
        PropertyDescriptorCollection props = base.GetProperties(attributes); 
        List<PropertyDescriptor> allProperties = new List<PropertyDescriptor>(); 
        foreach (PropertyDescriptor prop in props) 
        { 
            allProperties.Add(prop); 
        } 
        foreach (string key in _instance.Roles.Keys) 
        { 
            PropertyDescriptor kpd = new RolePropertyDescriptor(key); 
            allProperties.Add(kpd); 
        } 
        
        return new PropertyDescriptorCollection(allProperties.ToArray());
    } 
} 
*/
