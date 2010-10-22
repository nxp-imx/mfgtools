/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// UsbEject version 1.0 March 2006
// written by Simon Mourier <email: simon [underscore] mourier [at] hotmail [dot] com>

using System;
using System.ComponentModel;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A generic base class for physical device classes.
    /// </summary>
    public abstract class DeviceClass : IEnumerable
    {
        #region IEnumerable Implementation

        /// <summary>
        /// Supports foreach(Device dev in DeviceClass)
        /// </summary>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return Devices.GetEnumerator();
        }
        
        #endregion

        /// <summary>
        /// Initializes a new instance of the DeviceClass class.
        /// </summary>
        /// <param name="iFaceGuid">The device Interface Guid. ie. GUID_DEVINTERFACE_XXXX</param>
        /// <param name="classGuid">The device Class Guid. ie. GUID_DEVCLASS_XXXX</param>
        /// <param name="classGuid">The device class Enumerator. ie. USB, USBSTOR, PCI</param>
        protected DeviceClass(Guid iFaceGuid, Guid classGuid, String enumerator)
        {
            _InterfaceGuid = iFaceGuid;
            _ClassGuid = classGuid;
            _Enumerator = enumerator;

//            BuildDeviceCollection();
        }

        internal virtual Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new Device(deviceInstance, path);
        }

        /// <summary>
        /// The Description property of the DeviceClass
        /// </summary>
        public override string ToString()
        {
            return Description;
        }

        /// <summary>
        /// Gets the device's interface guid.
        /// </summary>
        public Guid InterfaceGuid
        {
            get { return _InterfaceGuid; }
        }
        private Guid _InterfaceGuid;

        /// <summary>
        /// Gets the device's class guid.
        /// </summary>
        public Guid ClassGuid
        {
            get { return _ClassGuid; }
        }
        private Guid _ClassGuid;

        /// <summary>
        /// Gets the DeviceClass enumerator. ex. USB, PCI, USBSTOR
        /// </summary>
        public String Enumerator
        {
            get { return _Enumerator; }
        }
        private String _Enumerator;

        /// <summary>
        /// Gets the DeviceClass icon index.
        /// </summary>
        public int ClassIconIndex
        {
            get 
            { 
                int classIconIndex = -1;

                if (!Win32.SetupDiGetClassImageIndex(DeviceManager.ClassImageList, ref _ClassGuid, out classIconIndex))
                {
                    int error = Marshal.GetLastWin32Error();
                    Trace.WriteLine(String.Format("Warning: {0}.get_ClassIconIndex - SetupDiGetClassImageIndex({1}) failed. {2}({3})", this.GetType(), _ClassGuid, new Win32Exception(error).Message, error));
                }
                return classIconIndex;
            }
        }

        /// <summary>
        /// Gets the DeviceClass description.
        /// </summary>
        public String Description
        {
            get
            {
                if (_Description == null)
                {
                    int requiredSize;
                    StringBuilder desc = new StringBuilder(1024);
                    Guid classGuid = ClassGuid;
                    if (Win32.SetupDiGetClassDescription(ref classGuid, desc, desc.Capacity, out requiredSize))
                    {
                        _Description = desc.ToString();
                    }
                    else
                    {
                        int error = Marshal.GetLastWin32Error();
                        Trace.WriteLine(String.Format("Warning: {0}.get_Description - SetupDiGetClassDescription({1}) failed. {2}({3})", this.GetType(), classGuid, new Win32Exception(error).Message, error));
                    }
                }
                return _Description;
            }
        }
        private String _Description;

        /// <summary>
        /// Gets the list of devices of this device class.
        /// </summary>
        public Collection<Device> Devices
        {
            get 
            {
                lock (this)
                {
                    if (_Devices == null)
                    {
                        BuildDeviceCollection();
                    }
                }

                return _Devices;
            }
        }
        protected Collection<Device> _Devices;

        /// <summary>
        /// Gets the number of devices contained in the DeviceClass
        /// </summary>
        public int Count
        {
            get 
            {
                if (_Devices != null)
                    return _Devices.Count;
                else
                    return 0;
            }
        }

        /// <summary>
        /// Rebuilds the collection of devices for this device class.
        /// </summary>
        public Collection<Device> Refresh()
        {
            // Dispose of the current collection of Devices
            lock(this)
            {
                if (_Devices != null)
                {
                    foreach (Device dev in _Devices)
                    {
                        dev.Dispose();
                    }
                    _Devices.Clear();
                    _Devices = null;
                }

                // Rebuild the list
                BuildDeviceCollection();
            }

            return _Devices;
        }

        protected Collection<Device> BuildDeviceCollection()
        {
            _Devices = new Collection<Device>();

            Device tempDevice = null;
            Dictionary<IntPtr, String> devInstDataSet = GetDevInstDataSet();
            foreach (KeyValuePair<IntPtr, String> devInstData in devInstDataSet)
            {
                tempDevice = CreateDevice(devInstData.Key, devInstData.Value);
                if (tempDevice != null)
                {
                    // init the hub and hub index fields, because if we wait till the 
                    // device is gone, it will be too late.
                    int port = tempDevice.UsbPort;

                    _Devices.Add(tempDevice);
                }
            }

            return _Devices;
        }

        virtual protected Dictionary<IntPtr, String> GetDevInstDataSet()
        {
            int error = Win32.S_OK;
            Dictionary<IntPtr, String> devInstDataSet = new Dictionary<IntPtr, String>();

            IntPtr infoSet;
            
            // decide the criteria for the devices
            if (_InterfaceGuid != Guid.Empty) //&& gWinVersionInfo().IsWinNT()
            {
                infoSet = Win32.SetupDiGetClassDevs(ref _InterfaceGuid, 0, IntPtr.Zero,
                    Win32.DIGCF_DEVICEINTERFACE | Win32.DIGCF_PRESENT);
            }
            else
            {
                if (String.IsNullOrEmpty(_Enumerator))
                {
                    infoSet = Win32.SetupDiGetClassDevs(ref _ClassGuid, 0, IntPtr.Zero,
                        Win32.DIGCF_PRESENT);
                }
                else
                {
                    infoSet = Win32.SetupDiGetClassDevs(0, _Enumerator, IntPtr.Zero,
                        Win32.DIGCF_PRESENT | Win32.DIGCF_ALLCLASSES);
                }
            }

            Win32.SP_DEVINFO_DATA devInfoData = new Win32.SP_DEVINFO_DATA();
            String devPath = String.Empty;

            for (int index = 0; /*no check*/; ++index)
            {
                if (InterfaceGuid != Guid.Empty /*&& gWinVersionInfo().IsWinNT()*/ )
                {
                    error = EnumDeviceInterfaceDetails(index, infoSet, InterfaceGuid, out devPath, devInfoData);
                    if (error == Win32.ERROR_NO_MORE_ITEMS)
                    {
                        break;
                    }
                }
                else
                {
                    if (!Win32.SetupDiEnumDeviceInfo(infoSet, index, devInfoData))
                    {
                        error = Marshal.GetLastWin32Error();
                        if (error != Win32.ERROR_NO_MORE_ITEMS)
                        {
                            throw new Win32Exception(error);
                        }
                        break;
                    }
                }

                int status = 0;
                int problemNumber = 0;

                Win32.CONFIGRET ret = Win32.CM_Get_DevNode_Status(out status, out problemNumber, devInfoData.devInst, 0);
                if (ret == Win32.CONFIGRET.CR_SUCCESS)
                {
                    if ((status & 0x00000400) == 0x00000400) // 0x00000400 == DN_HAS_PROBLEM
                        throw new Exception("DN_HAS_PROBLEM");
                }
                else
                {
                    Trace.WriteLine(String.Format("{0}.Parent() - ERROR: devInst:{1} ({2})", this.GetType(), devInfoData.devInst, ret));
                }
                devInstDataSet[devInfoData.devInst] = devPath;
            }
            
            Win32.SetupDiDestroyDeviceInfoList(infoSet);

            return devInstDataSet;
        }

        private int EnumDeviceInterfaceDetails(int index, IntPtr devInfoSet, Guid iFaceGuid, out String devPath, Win32.SP_DEVINFO_DATA devInfoData)
        {
            Win32.SP_DEVICE_INTERFACE_DATA interfaceData = new Win32.SP_DEVICE_INTERFACE_DATA();

            if (!Win32.SetupDiEnumDeviceInterfaces(devInfoSet, null, ref iFaceGuid, index, interfaceData))
            {
                int error = Marshal.GetLastWin32Error();
                if (error == Win32.ERROR_NO_MORE_ITEMS)
                {
                    devPath = String.Empty;
                    devInfoData = null;
                    return error;
                }
                else
                    throw new Win32Exception(error);
            }

            int requiredSize = 0;
            if (!Win32.SetupDiGetDeviceInterfaceDetail(devInfoSet, interfaceData, IntPtr.Zero, 0, ref requiredSize, devInfoData))
            {
                int error = Marshal.GetLastWin32Error();
                if (error != Win32.ERROR_INSUFFICIENT_BUFFER)
                    throw new Win32Exception(error);
            }

            IntPtr buffer = Marshal.AllocHGlobal(requiredSize);
            Win32.SP_DEVICE_INTERFACE_DETAIL_DATA detailData = new Win32.SP_DEVICE_INTERFACE_DETAIL_DATA();
            Marshal.StructureToPtr(detailData, buffer, false);

            if (!Win32.SetupDiGetDeviceInterfaceDetail(devInfoSet, interfaceData, buffer, requiredSize, ref requiredSize, devInfoData))
            {
                Marshal.FreeHGlobal(buffer);
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            IntPtr pDevicePath = (IntPtr)((int)buffer + (int)Marshal.OffsetOf(typeof(Win32.SP_DEVICE_INTERFACE_DETAIL_DATA), "devicePath"));
            devPath = Marshal.PtrToStringAuto(pDevicePath);
            Marshal.FreeHGlobal(buffer);

            return Win32.ERROR_SUCCESS;
        }
        
        public Device FindDeviceByPath(String devPath, bool refresh)
        {

            if (String.IsNullOrEmpty(devPath))
            {
                return null;
            }

            // Refresh the Device list?
            if (refresh)
            {
                _Devices.Clear();
                _Devices = null;
            }

            // Find the Device in our list of devices.
            foreach (Device dev in Devices)
            {
                if (dev != null)
                {
                    if (String.Compare(dev.Path, 4, devPath, 4, devPath.Length-4, true) == 0)
                    {
                        return dev;
                    }
                }
            }

            return null;
        }

        public virtual Device FindDeviceByUsbPath(String devPath, bool refresh)
        {

            if (String.IsNullOrEmpty(devPath))
            {
                return null;
            }

            // Refresh the Device list?
            if (refresh)
            {
                _Devices.Clear();
                _Devices = null;
            }

            // Find the Device in our list of devices.
            foreach (Device dev in Devices)
            {
                if (dev != null && dev.UsbDevice != null)
                {
                    if (String.Compare(dev.UsbDevice.Path, 4, devPath, 4, devPath.Length - 4, true) == 0)
                    {
                        return dev;
                    }
                }
            }

            return null;
        }

        public Device FindDeviceByDriver(String driverName)
        {
            if (String.IsNullOrEmpty(driverName))
            {
                return null;
            }

            // Find the Device in our list of devices.
            foreach (Device dev in Devices)
            {
                if (dev != null)
                {
                    if (String.Compare(dev.Driver, driverName, true) == 0)
                    {
                        return dev;
                    }
                }
            }

            return null;
        }

        internal virtual DeviceChangedEventArgs[] AddUsbDevice(String path, DeviceChangeEvent devEvent)
        {
	        DeviceChangedEventArgs[] eventArgs = null;

	        // see if it is already in our list of Devices()
            Device device = FindDeviceByUsbPath(path, false);

	        if ( device == null )
	        {
		        // it's not in our Collection of constructed devices
		        // so lets get a new list of our devices from Windows
		        // and see if it is there.
                device = FindDeviceByUsbPath(path, true);
	        }
        	
	        if ( device != null )
	        {
                Trace.WriteLine(String.Format("{0}.AddUsbDevice() Created:{1}", this, path));

                eventArgs = new DeviceChangedEventArgs[1];
                eventArgs[0] = new DeviceChangedEventArgs(devEvent, device.DeviceInstanceId, device.GetType(), device.UsbHub.Index, device.UsbPort);
	        }

            return eventArgs;
        }

        internal virtual DeviceChangedEventArgs[] RemoveUsbDevice(String path, DeviceChangeEvent devEvent)
        {
            DeviceChangedEventArgs[] eventArgs = null;

	        // see if it is in our list of Devices
            Device device = FindDeviceByUsbPath(path, false);

	        if ( device != null )
	        {
                Trace.WriteLine(String.Format("{0}.RemoveUsbDevice() Removed:{1}", this, path));

                eventArgs = new DeviceChangedEventArgs[1];
                eventArgs[0] = new DeviceChangedEventArgs(devEvent, device.DeviceInstanceId, device.GetType(), device.UsbHub.Index, device.UsbPort);

                // If we found it, remove it from our list of devices
                _Devices.Remove(device);

                device.UsbHub[device.UsbPort].Refresh();
            }

            return eventArgs;
        }
       
    }

    /// <summary>
    /// The device class for the Computer device.
    /// </summary>
    public sealed class ComputerDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the ComputerDeviceClass class.
        /// </summary>
        private ComputerDeviceClass()
            : base(Guid.Empty, Win32.GUID_DEVCLASS_COMPUTER, String.Empty)
        { }

        /// <summary>
        /// Gets the single ComputerDeviceClass instance.
        /// </summary>
        public static ComputerDeviceClass Instance
        {
            get { return Utils.Singleton<ComputerDeviceClass>.Instance; }
        }
    }
}
