// UsbEject version 1.0 March 2006
// written by Simon Mourier <email: simon [underscore] mourier [at] hotmail [dot] com>

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;
//using Microsoft.Win32;

//using DevSupport.Api;

namespace DevSupport.DeviceManager
{
    public interface ILiveUpdater
    {
        Byte[] GetDeviceDataFromFile(String fileName);
        Int32 CopyUpdateFileToMedia(String fileName);
    }

    public interface IResetToRecovery
    {
        Int32 ResetToRecovery();
    }

    public interface IChipReset
    {
        Int32 ChipReset();
    }

    public interface IRecoverable
    {
        Int32 LoadRecoveryDevice(String filename);
    }

    public interface IUpdater
    {
        Boolean IsCompatible(Media.LogicalDrive[] newDriveArray);
        Int32 ResetToUpdater();
        Int32 FinalInit();
    }

    /// <summary>
    /// A generic base class for physical devices.
    /// </summary>
    [TypeConverter(typeof(ExpandableObjectConverter))]
    public class Device : IDisposable, IFormattable //IComparable
    {
        private String _Path;
        private String _Description;
        private IntPtr _DeviceInfoSet;
        private Win32.SP_DEVINFO_DATA _DeviceInfoData;
        private IntPtr _DeviceInstance;
        private String _DeviceInstanceId;
        private String _Class;
        private Guid _ClassGuid;
        private int _ClassIconIndex;
        private Device _Parent;
        private Win32.DeviceCapabilities _Capabilities = Win32.DeviceCapabilities.Unknown;
//        private Collection<Device> _removableDevices;
        private String _FriendlyName;
        private String _Enumerator;
        private String _Driver;
        private UsbHub _UsbHub;
        private Device _UsbDevice;
        private int _UsbPort;

        #region virtual IFormattable Implementation

        public virtual string ToString(string format, IFormatProvider formatProvider)
        {
            return this.ToString();
        }

        #endregion

        public override string ToString()
        {
            String descStr = Description;

            if (!String.IsNullOrEmpty(FriendlyName) &&
                String.Compare(Description, FriendlyName, true) != 0)
            {
                descStr += " - " + FriendlyName;
            }

            return descStr;
        }
        
        internal Device(IntPtr deviceInstance, String path)
        {
            if ( deviceInstance == IntPtr.Zero )
                throw new ArgumentNullException("deviceInstance");

            _Path = path; // may be null
            _DeviceInstance = deviceInstance;
        }

        /// <summary>
        /// Gets the device's path.
        /// </summary>
        public string Path
        {
            get
            {
                if ( String.IsNullOrEmpty(_Path) )
                {
		            IntPtr hKey = new IntPtr();
                    Win32.CONFIGRET cr = Win32.CM_Open_DevNode_Key(DeviceInstance, Win32.KEY_QUERY_VALUE,
                        0, Win32.RegDisposition_OpenExisting, out hKey, Win32.CM_REGISTRY_HARDWARE);
                    if (cr == Win32.CONFIGRET.CR_SUCCESS)
                    {
                        StringBuilder pData = new StringBuilder(1024);
                        int dataSize = pData.Capacity;
                        int error = Win32.RegQueryValueEx(hKey, "SymbolicName", IntPtr.Zero, IntPtr.Zero, pData, ref dataSize);
                        if (error == Win32.ERROR_SUCCESS)
                        {
                            // change path to something we can use for CreateFile()
                            // TODO: check to see if the Device Arrival path is the same form as this
                            // and if so , use the whole path when doing comparisons.
                            // _Path = pData.ToString().Replace(@"\??", @"\\.");
                            _Path = pData.ToString();
                        }
                        else
                        {
                            Win32Exception e = new Win32Exception(error);
                            Trace.WriteLine(String.Format("Warning: {0}.Path() - RegQueryValueEx(\"SymbolicName\") failed. {1}", Class, e.Message));
                        }

                        Win32.RegCloseKey(hKey);
                    }
                    else
                    {
                        Trace.WriteLine(String.Format("Warning: {0}.Path() - CM_Open_DevNode_Key() failed. {1}, {2}({3})", Class, cr, DeviceInstanceId, DeviceInstance));
                    }
		        }

                return _Path;
            } // get
        } // Path

        /// <summary>
        /// Gets the device's instance handle.
        /// </summary>
        public IntPtr DeviceInstance
        {
            get
            {
                return _DeviceInstance;
            }
        }

        /// <summary>
        /// Gets the device instance id.
        /// </summary>
        public String DeviceInstanceId
        {
            get
            {
                if ( _DeviceInstanceId == null )
                {
                    StringBuilder sb = new StringBuilder(1024);
                    Win32.CONFIGRET cr = Win32.CM_Get_Device_ID(DeviceInstance, sb, sb.Capacity, 0);
                    if (cr != Win32.CONFIGRET.CR_SUCCESS)
                    {
                        Trace.WriteLine(String.Format("Error: {0}.DeviceInstanceId() - CM_Get_Device_ID() failed. {1} DevInst:{2}", Class, cr, DeviceInstance));
                        throw new Win32Exception(cr.ToString());
                    }

                    _DeviceInstanceId = sb.ToString();
                }

                return _DeviceInstanceId;
            }
        }

        /// <summary>
        /// Make a new devInfoSet that is not tied to a class so we can create Parents and such.
        /// Also, we will be able to get our own Registry Properties and not have to ask the DeviceClass.
        /// </summary>
        public IntPtr DeviceInfoSet
        {
            get
            {
	            if ( _DeviceInfoSet == IntPtr.Zero )
                {
                    // Make a new devInfoSet that is not tied to a class
	                // so we can create Parents and such. Also, we will be able 
	                // to get our own Registry Properties and not have to ask the DeviceClass.
	                _DeviceInfoSet = Win32.SetupDiCreateDeviceInfoList(IntPtr.Zero, IntPtr.Zero);
	                if ( _DeviceInfoSet.ToInt32() == Win32.INVALID_HANDLE_VALUE )
	                {
		                throw new Win32Exception(Marshal.GetLastWin32Error());
	                }
                }
                return _DeviceInfoSet;
            }
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            if (_DeviceInfoSet != IntPtr.Zero)
            {
                Win32.SetupDiDestroyDeviceInfoList(_DeviceInfoSet);
                _DeviceInfoSet = IntPtr.Zero;
            }
        }

        public Win32.SP_DEVINFO_DATA DeviceInfoData
        {
            get
            {
                if ( _DeviceInfoData == null )
                {
                    // Initialize our SP_DEVINFO_DATA struct for future calls to SetupDiGetDeviceRegistryProperty()
                    _DeviceInfoData = new Win32.SP_DEVINFO_DATA();
                    if (!Win32.SetupDiOpenDeviceInfo(DeviceInfoSet, DeviceInstanceId, IntPtr.Zero, 0, _DeviceInfoData))
                    {
	                    throw new Win32Exception(Marshal.GetLastWin32Error());
                    }
                }
                return _DeviceInfoData;
            }
        }

        /// <summary>
        /// Gets the device's class name.
        /// </summary>
        public string Class
        {
            get
            {
                if (_Class == null)
                {
                    _Class = GetProperty(DeviceInfoSet, DeviceInfoData, Win32.SetupDiRegistryProperty.SPDRP_CLASS, null);
                }
                return _Class;
            }
        }

        /// <summary>
        /// Gets the device's class Guid.
        /// </summary>
        public Guid ClassGuid
        {
            get
            {
                if (_ClassGuid == Guid.Empty)
                {
                    _ClassGuid = GetProperty(DeviceInfoSet, DeviceInfoData, Win32.SetupDiRegistryProperty.SPDRP_CLASSGUID, Guid.Empty);
                }
                
                return _ClassGuid;
            }
        }

        /// <summary>
        /// Gets the device's class icon index.
        /// </summary>
        public int ClassIconIndex
        {
            get
            {
                _ClassIconIndex = -1;

                if (ClassGuid != null)
                {
                    Guid classGuid = ClassGuid;

                    Win32.SP_CLASSIMAGELIST_DATA imageList = DeviceManager.ClassImageList;
                    if (!Win32.SetupDiGetClassImageIndex(imageList, ref classGuid, out _ClassIconIndex))
                    {
                        int error = Marshal.GetLastWin32Error();
                    }
                }                
                return _ClassIconIndex;
            }
        }

        /// <summary>
        /// Gets the device's description.
        /// </summary>
        public string Description
        {
            get
            {
                if (_Description == null)
                {
                    _Description = GetProperty(DeviceInfoSet, DeviceInfoData, Win32.SetupDiRegistryProperty.SPDRP_DEVICEDESC, null);
                }
                return _Description;
            }
        }

        /// <summary>
        /// Gets the device's friendly name.
        /// </summary>
        public string FriendlyName
        {
            get
            {
                if (_FriendlyName == null)
                {
                    _FriendlyName = GetProperty(DeviceInfoSet, DeviceInfoData, Win32.SetupDiRegistryProperty.SPDRP_FRIENDLYNAME, null);
                }
                return _FriendlyName;
            }
        }

        /// <summary>
        /// Gets the device's capabilities.
        /// </summary>
        public Win32.DeviceCapabilities Capabilities
        {
            get
            {
                if (_Capabilities == Win32.DeviceCapabilities.Unknown)
                {
                    _Capabilities = (Win32.DeviceCapabilities)GetProperty(DeviceInfoSet, DeviceInfoData, Win32.SetupDiRegistryProperty.SPDRP_CAPABILITIES, 0);
                }
                return _Capabilities;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this device is a USB device.
        /// </summary>
        public virtual bool IsUsb
        {
            get
            {
//                if (Class == "USB")
//                    return true;
                if ( Enumerator.ToUpper().CompareTo("USB") == 0 )
                    return true;

                if (Parent == null)
                    return false;

                return Parent.IsUsb;
            }
        }

        /// <summary>
        /// Gets the device's parent device or null if this device doesn't have a parent.
        /// </summary>
        public Device Parent
        {
            get
            {
                if (_Parent == null)
                {
                    int status = 0;
                    int problemNumber = 0;
                    
                    Win32.CONFIGRET ret = Win32.CM_Get_DevNode_Status(out status, out problemNumber, DeviceInstance, 0);
                    if (ret == Win32.CONFIGRET.CR_SUCCESS)
                    {
                        if ((status & 0x00000400) == 0x00000400) // 0x00000400 == DN_HAS_PROBLEM
                            throw new Exception("DN_HAS_PROBLEM");
                    }
                    else
                    {
                        Trace.WriteLine(String.Format("{0}.Parent() - ERROR: devInst:{1} ({2})", this.GetType(), DeviceInstance, ret));
                    }

                    IntPtr parentDevInst = IntPtr.Zero;
                    if (Win32.CM_Get_Parent(out parentDevInst, DeviceInstance, 0) == Win32.CONFIGRET.CR_SUCCESS)
                    {
                        _Parent = new Device(parentDevInst, null);
                    }
                }
                return _Parent;
            }
        }

        /// <summary>
        /// Gets the device's child device or null if this device doesn't have a child.
        /// </summary>
        public virtual Device Child
        {
            get
            {
                IntPtr childDevInst = IntPtr.Zero;
                if (Win32.CM_Get_Child(out childDevInst, DeviceInstance, 0) == Win32.CONFIGRET.CR_SUCCESS)
                {
                    return new Device(childDevInst, null);
                }
                else
                    return null;
            }
        }

        /// <summary>
        /// Gets the device's child devices.
        /// </summary>
        public virtual Collection<Device> Children
        {
            get
            {
                Collection<Device> children = new Collection<Device>();
                
                IntPtr devInst = IntPtr.Zero;
                IntPtr siblingDevInst = IntPtr.Zero;

                if (Win32.CM_Get_Child(out devInst, DeviceInstance, 0) == Win32.CONFIGRET.CR_SUCCESS)
                {
                    // Add the first child
                    children.Add(new Device(devInst, null));

                    // Get the child's siblings if any
                    while (Win32.CM_Get_Sibling(out siblingDevInst, devInst, 0) == Win32.CONFIGRET.CR_SUCCESS)
                    {
                        // Add the sibling
                        children.Add(new Device(siblingDevInst, null));
                        
                        devInst = siblingDevInst;
                        siblingDevInst = IntPtr.Zero;
                    }

                }

                return children;
            }
        }

        /// <summary>
        /// Gets this device or a device in the device's parent chain enumerated on the USB bus or null if this device is not a USB device.
        /// </summary>
        public Device UsbDevice
        {
            get
            {
                if (_UsbDevice == null)
                {
                    // UsbHub property sets _UsbDevice as a child of the UsbHub device
                    if (UsbHub == null)
                        return null;
                }

                return _UsbDevice;
            }
        }

        /// <summary>
        /// Gets the device's enumerator. ex. "USB"
        /// </summary>
        public String Enumerator
        {
            get
            {   //w98 this doesn't work in W98. There is no "Enumerator" value
                //w98 suggest maybe parsing deviceinstanceid?
                //w98 CM_Get_DevNode_Registry_Property(CM_DRP_ENUMERATOR_NAME)
                //w98 have to implement apiCM_Get_DevNode_Registry_Property in setupapi.cpp first
                if (_Enumerator == null)
                {
                    if (Environment.OSVersion.Platform == PlatformID.Win32NT)
                    {
                        _Enumerator = GetProperty(DeviceInfoSet, DeviceInfoData, Win32.SetupDiRegistryProperty.SPDRP_ENUMERATOR_NAME, null);
                    }
                    else
                    {	//w98 ERROR_INVALID_REG_PROPERTY (...|0x209)
                        //w98 so we have to use the CM_ function with the CM_DRP_ENUMERATOR_NAME arg
                        //w98 for this property.
                        //w98 ULONG type, length = MAX_PATH; BYTE Buffer[MAX_PATH]; 
                        //w98 DWORD error = gSetupApi().apiCM_Get_DevNode_Registry_Property(dev->_hDevInst, CM_DRP_ENUMERATOR_NAME, &type,
                        //w98 Buffer, &length, 0);

                        //w98 _Enumerator = (PTSTR)Buffer;
                    }
                }
                return _Enumerator;
            }
        }

        /// <summary>
        /// Gets the device's driver instance.
        /// </summary>
        public String Driver
        {
            get
            {
                if ( _Driver == null )
	            {
                    _Driver = GetProperty(DeviceInfoSet, DeviceInfoData, Win32.SetupDiRegistryProperty.SPDRP_DRIVER, null);
	            }

                return _Driver;
            }
        }

        /// <summary>
        /// Gets the USB UsbHub device that the device is connected to.
        /// The USB UsbHub device is the parent of the USB device
        /// </summary>
        public UsbHub UsbHub
        {
            get
            {
                if (_UsbHub == null)
                {
                    if (!IsUsb)
                        return null;

                    Device dev = this;
                    while (dev.Parent != null)
                    {
                        _UsbHub = UsbHubClass.Instance.FindDeviceByPath(dev.Parent.Path, false) as UsbHub;

                        if (_UsbHub != null)
                        {
                            _UsbDevice = dev;
                            break;
                        }

                        dev = dev.Parent;
                    }
                }

                return _UsbHub;
            }
        }

        /// <summary>
        /// Gets the USB Port on a given USB Hub that the device is connected to.
        /// UsbPorts are 1-based. Zero(0) represents an INVALID_PORT.
        /// </summary>
        public int UsbPort
        {
            get
            {
                if (_UsbPort == 0)
                {
                    if (this.UsbHub != null)
                    {
                        _UsbPort = this.UsbHub.FindPortIndex(this.UsbDevice.Driver);
                    }
                }

                return _UsbPort;
            }
        }

        internal protected void RefreshUsbPort()
        {
            try
            {
                UsbHub[UsbPort].Refresh();
            }
            catch (Exception e) { Trace.WriteLine(e.Message); }
        }

        internal String GetProperty(IntPtr hDevInfoSet, Win32.SP_DEVINFO_DATA devData, Win32.SetupDiRegistryProperty property, String defaultValue)
        {
            if (devData == null)
                throw new ArgumentNullException("devData");

            int propertyRegDataType = 0;
            int requiredSize;
            int propertyBufferSize = 1024;

            IntPtr propertyBuffer = Marshal.AllocHGlobal(propertyBufferSize);
            if (!Win32.SetupDiGetDeviceRegistryProperty(hDevInfoSet,
                devData,
                property,
                out propertyRegDataType,
                propertyBuffer,
                propertyBufferSize,
                out requiredSize))
            {
                Marshal.FreeHGlobal(propertyBuffer);
                int error = Marshal.GetLastWin32Error();
                if (error != Win32.ERROR_INVALID_DATA)
                    Trace.WriteLine(String.Format("Warning: {0}.GetProperty() - SetupDiGetDeviceRegistryProperty({1}) failed. {2}({3})", this.GetType(), property, new Win32Exception(error).Message, error));
//                throw new Win32Exception(error);
                return defaultValue;
            }

            string value = Marshal.PtrToStringAuto(propertyBuffer);
            Marshal.FreeHGlobal(propertyBuffer);
            return value;
        }

        internal int GetProperty(IntPtr hDevInfoSet, Win32.SP_DEVINFO_DATA devData, Win32.SetupDiRegistryProperty property, int defaultValue)
        {
            if (devData == null)
                throw new ArgumentNullException("devData");

            int propertyRegDataType = 0;
            int requiredSize;
            int propertyBufferSize = Marshal.SizeOf(typeof(int));

            IntPtr propertyBuffer = Marshal.AllocHGlobal(propertyBufferSize);
            if (!Win32.SetupDiGetDeviceRegistryProperty(hDevInfoSet,
                devData,
                property,
                out propertyRegDataType,
                propertyBuffer,
                propertyBufferSize,
                out requiredSize))
            {
                Marshal.FreeHGlobal(propertyBuffer);
                int error = Marshal.GetLastWin32Error();
                if (error != Win32.ERROR_INVALID_DATA)
                    Trace.WriteLine(String.Format("Warning: {0}.GetProperty() - SetupDiGetDeviceRegistryProperty({1}) failed. {2}({3})", this.GetType(), property, new Win32Exception(error).Message, error));
//                throw new Win32Exception(error);
                return defaultValue;
            }

            int value = (int)Marshal.PtrToStructure(propertyBuffer, typeof(int));
            Marshal.FreeHGlobal(propertyBuffer);
            return value;
        }

        internal Guid GetProperty(IntPtr hDevInfoSet, Win32.SP_DEVINFO_DATA devData, Win32.SetupDiRegistryProperty property, Guid defaultValue)
        {
            if (devData == null)
                throw new ArgumentNullException("devData");

            int propertyRegDataType = 0;
            int requiredSize;
            // ( 32(hex-digits) + 4(dashes) + 2(braces) + 1(null) ) * 2-bytes/char = 78 bytes
            // ex. "{745A17A0-74D3-11D0-B6FE-00A0C90F57DA}"
            int propertyBufferSize = 78;

            IntPtr propertyBuffer = Marshal.AllocHGlobal(propertyBufferSize);
            if (!Win32.SetupDiGetDeviceRegistryProperty(hDevInfoSet,
                devData,
                property,
                out propertyRegDataType,
                propertyBuffer,
                propertyBufferSize,
                out requiredSize))
            {
                int error = Marshal.GetLastWin32Error();
                Marshal.FreeHGlobal(propertyBuffer);

                if (error != Win32.ERROR_INVALID_DATA)
                    Trace.WriteLine(String.Format("Warning: {0}.GetProperty() - SetupDiGetDeviceRegistryProperty({1}) failed. {2}({3})", this.GetType(), property, new Win32Exception(error).Message, error));
//                throw new Win32Exception(error);
                
                return defaultValue;
            }

            String guidStr = Marshal.PtrToStringAuto(propertyBuffer);
            Marshal.FreeHGlobal(propertyBuffer);

            return new Guid(guidStr);
        }

	    virtual public Int32 SendCommand(Api.Api api)
        {
            throw new NotImplementedException();
        }
        public delegate Int32 SendCommandAsync(Api.Api api);
//        public delegate Boolean DeviceIoControlAsync( SafeFileHandle
//                hDrive,
//                Win32.IOCTL_SCSI_PASS_THROUGH,
//                requestPtr,
//                totalSize,
//                requestPtr,
//                totalSize,
//                out dwBytesReturned,
//                IntPtr.Zero);


        public virtual String ErrorString
        {
            get { return _ErrorString; }
            set { _ErrorString = value; }
        }
        private String _ErrorString;
        
        public class SendCommandProgressArgs : EventArgs
	    {
            public SendCommandProgressArgs(string name, Api.Api.CommandDirection direction, Int64 maximum)
            {
                Name = name;
                Direction = direction;
                InProgress = true;
                Maximum = maximum;
                Status = String.Empty;
            }

            public string Name;
            public Api.Api.CommandDirection Direction;
            public Int64 Maximum;
            public bool InProgress;
            public Int64 Position;
            public Int32 Error;
            public string Status;
	    }

        public delegate void SendCommandProgressHandler(object sender, SendCommandProgressArgs args);
        public event SendCommandProgressHandler SendCommandProgress;

        public void DoSendProgress(SendCommandProgressArgs args)
        {
//            Trace.WriteLine(String.Format("*** {0}.DoSendProgress(): {1}({2})", GetType().Name, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
            SendCommandProgressHandler localSendProgress = SendCommandProgress;
            if (localSendProgress != null)
            {
                Debug.Assert(localSendProgress.GetInvocationList().Length == 1);

                if ((localSendProgress.Target is ISynchronizeInvoke) &&
                     (((ISynchronizeInvoke)localSendProgress.Target).InvokeRequired))
                {
                    object[] argsArray = new object[2] { this, args };

                    IAsyncResult ar = ((ISynchronizeInvoke)localSendProgress.Target).BeginInvoke(localSendProgress, argsArray);
//                    ((ISynchronizeInvoke)localSendProgress.Target).EndInvoke(ar);
                }
                else
                    localSendProgress(this, args);
            }
        }

        public virtual bool Match(UInt16? vid, UInt16? pid)
        {
            String searchString;
 
            if ( vid != null )
                searchString = String.Format("VID_{0:X4}&PID_{1:X4}", vid, pid);
            else
                searchString = String.Format("&PID_{1:X4}", pid);

            if (IsUsb)
                return UsbDevice.DeviceInstanceId.ToUpper().Contains(searchString);
            else
                return DeviceInstanceId.ToUpper().Contains(searchString);
        }

        [Description("USB Vendor ID.")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public UInt16 Vid
        {
            get
            {
                UInt16 vid = 0;

                if (IsUsb)
                {
                    String vidStr = UsbDevice.DeviceInstanceId.Substring(UsbDevice.DeviceInstanceId.IndexOf("VID_", StringComparison.InvariantCultureIgnoreCase) + 4, 4);
                    UInt16.TryParse(vidStr, System.Globalization.NumberStyles.HexNumber, CultureInfo.InvariantCulture, out vid);
                }

                return vid;
            }
        }

        [Description("USB Product ID.")]
        [TypeConverter(typeof(Utils.DecimalConverterEx))]
        public UInt16 Pid
        {
            get
            {
                UInt16 pid = 0;

                if (IsUsb)
                {
                    String pidStr = UsbDevice.DeviceInstanceId.Substring(UsbDevice.DeviceInstanceId.IndexOf("&PID_", StringComparison.InvariantCultureIgnoreCase) + 5, 4);
                    UInt16.TryParse(pidStr, System.Globalization.NumberStyles.HexNumber, CultureInfo.InvariantCulture, out pid);
                }

                return pid;
            }
        }

        /// <summary>
        /// Gets this device's list of removable devices.
        /// Removable devices are parent devices that can be removed.
        /// </summary>
/*        public virtual Collection<Device> RemovableDevices
        {
            get
            {
                if (_removableDevices == null)
                {
                    _removableDevices = new Collection<Device>();

                    if ((Capabilities & DeviceCapabilities.Removable) != 0)
                    {
                        _removableDevices.Add(this);
                    }
                    else
                    {
                        if (Parent != null)
                        {
                            foreach (Device device in Parent.RemovableDevices)
                            {
                                _removableDevices.Add(device);
                            }
                        }
                    }
                }
                return _removableDevices;
            }
        }
*/
        /// <summary>
        /// Ejects the device.
        /// </summary>
        /// <param name="allowUI">Pass true to allow the Windows shell to display any related UI element, false otherwise.</param>
        /// <returns>null if no error occured, otherwise a contextual text.</returns>
/*        public string Eject(bool allowUI)
        {
            foreach (Device device in RemovableDevices)
            {
                if (allowUI)
                {
                    Win32.CM_Request_Device_Eject_NoUi(device.DeviceInstance, IntPtr.Zero, null, 0, 0);
                    // don't handle errors, there should be a UI for this
                }
                else
                {
                    StringBuilder sb = new StringBuilder(1024);

                    Win32.PNP_VETO_TYPE veto;
                    int hr = Win32.CM_Request_Device_Eject(device.DeviceInstance, out veto, sb, sb.Capacity, 0);
                    if (hr != 0)
                        throw new Win32Exception(hr);

                    if (veto != Win32.PNP_VETO_TYPE.Ok)
                        return veto.ToString();
                }

            }
            return null;
        }
*/
        /// <summary>
        /// Compares the current instance with another object of the same type.
        /// </summary>
        /// <param name="obj">An object to compare with this instance.</param>
        /// <returns>A 32-bit signed integer that indicates the relative order of the comparands.</returns>
/*        public virtual int CompareTo(object obj)
        {
            Device device = obj as Device;
            if (device == null)
                throw new ArgumentNullException("obj", "Argument obj is not a Device object.");

            return Index.CompareTo(device.Index);
        }
*/
    }
}
