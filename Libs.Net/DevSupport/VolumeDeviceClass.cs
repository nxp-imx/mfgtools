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
using System.Diagnostics;

using DevSupport;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// The device class for volume devices.
    /// </summary>
    public class VolumeDeviceClass : DeviceClass
    {
        /// <summary>
        /// Initializes a new instance of the VolumeDeviceClass class.
        /// </summary>
        private VolumeDeviceClass()
            : base(Win32.GUID_DEVINTERFACE_VOLUME, Win32.GUID_DEVCLASS_VOLUME, "STORAGE")
        {}

        /// <summary>
        /// Gets the single VolumeDeviceClass instance.
        /// </summary>
        public static VolumeDeviceClass Instance
        {
            get { return Utils.Singleton<VolumeDeviceClass>.Instance; }
        }

        internal override Device CreateDevice(IntPtr deviceInstance, String path)
        {
            return new Volume(deviceInstance, path);
        }

        internal override DeviceChangedEventArgs[] AddUsbDevice(String driveLetters, DeviceChangeEvent devEvent)
        {
            if (devEvent != DeviceChangeEvent.VolumeArrival)
                return null;

            DeviceChangedEventArgs[] eventArgs = new DeviceChangedEventArgs[driveLetters.Length];
            int index = 0;

            lock (this)
            {
                // Rebuild the Disk and Volume DeviceClasses to pick up the newly arrived device.
                DiskDeviceClass.Instance.Refresh();
                this.Refresh();

                foreach (Char driveLetter in driveLetters.ToCharArray())
                {
                    // It should be in our list of Devices()
                    foreach (Volume volume in Devices)
                    {
                        if (volume.LogicalDrive.Contains(driveLetter.ToString()))
                        {
                            Trace.WriteLine(String.Format("{0}.AddUsbDevice() Created:{1}", this, driveLetter));
                            volume.RefreshUsbPort();

                            eventArgs[index++] = new DeviceChangedEventArgs(devEvent, driveLetter.ToString(), volume.GetType(), volume.UsbHub.Index, volume.UsbPort);
                            break;
                        }
                    }
                }
            }

            return eventArgs;
        }

        internal override DeviceChangedEventArgs[] RemoveUsbDevice(String driveLetters, DeviceChangeEvent devEvent)
        {
            if (devEvent != DeviceChangeEvent.VolumeRemoval)
                return null;

            DeviceChangedEventArgs[] eventArgs = new DeviceChangedEventArgs[driveLetters.Length];
            int index = 0;

            lock (this)
            {
                foreach (Char driveLetter in driveLetters)
                {
                    // It should be in our list of Devices()
                    foreach (Volume volume in Devices)
                    {
                        if (volume.LogicalDrive.Contains(driveLetter.ToString()))
                        {
                            Trace.WriteLine(String.Format("{0}.RemoveUsbDevice() Removed:{1}", this, driveLetter));

                            eventArgs[index++] = new DeviceChangedEventArgs(devEvent, driveLetter.ToString(), volume.GetType(), volume.UsbHub.Index, volume.UsbPort);

                            // If we found it, remove it from our list of devices
                            volume.RefreshUsbPort();
                            _Devices.Remove(volume);
                            volume.Dispose();

                            break;
                        }
                    }
                }
                // Remove the Disks from the static list
                DiskDeviceClass.Instance.Refresh();
            }
            return eventArgs;
        }
    }
}
