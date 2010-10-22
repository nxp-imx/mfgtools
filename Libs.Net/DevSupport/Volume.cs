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
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32.SafeHandles;

using DevSupport.Api;
using DevSupport.Media;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A volume device.
    /// </summary>
    public class Volume : Device, ILiveUpdater, IResetToRecovery, IChipReset
    {
        private string _VolumeName;
        private string _LogicalDrive;
        private int[] _DiskNumbers;
        private Collection<Device> _Disks;
        public const UInt32 MaxTransferSize = 128 * 512; //(128 sectors of 512 bytes each at a time) says who??
        public const UInt32 MaxPhysicalTransferSize = 16 * 1024; // 16 KB limit on physical data because of redundant area.
        
        public enum LockType { Logical, Physical }
        private const Int32 LockTimeout = 1000;       // 1 Second
        private const Int32 LockRetries = 10;

        internal Volume(IntPtr deviceInstance, String path)
            : base(deviceInstance, path)
        {
            String driveLetter = LogicalDrive;
        }

        /// <summary>
        /// Gets the volume's name.
        /// </summary>
        public string VolumeName
        {
            get
            {
                if (_VolumeName == null)
                {
                    StringBuilder sb = new StringBuilder(1024);
                    if (!Win32.GetVolumeNameForVolumeMountPoint(Path + "\\", sb, sb.Capacity))
                    {
                        // throw new Win32Exception(Marshal.GetLastWin32Error());

                    }

                    if (sb.Length > 0)
                    {
                        _VolumeName = sb.ToString();
                    }
                }
                return _VolumeName;
            }
        }

        /// <summary>
        /// Gets the volume's logical drive in the form [letter]:\
        /// </summary>
        public string LogicalDrive
        {
            get
            {
                if ((_LogicalDrive == null) && (VolumeName != null))
                {
                    foreach (string drive in Environment.GetLogicalDrives())
                    {
                        StringBuilder volName = new StringBuilder(1024);
                        if (Win32.GetVolumeNameForVolumeMountPoint(drive, volName, volName.Capacity))
                        {
                            if (volName.ToString() == VolumeName)
                            {
                                _LogicalDrive = drive.Replace("\\", "");
                                break;
                            }
                        }
                    }
                }
                return _LogicalDrive;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this volume is a based on USB devices.
        /// </summary>
        public override bool IsUsb
        {
            get
            {
                if (Disks != null)
                {
                    foreach (Device disk in Disks)
                    {
                        if (disk.IsUsb)
                            return true;
                    }
                }
                return base.IsUsb;
            }
        }

        /// <summary>
        /// Handle to use when volume is locked.
        /// </summary>
        public SafeFileHandle LockHandle
        {
            get { return _LockHandle; }
            set { _LockHandle = value; }
        }
        private SafeFileHandle _LockHandle = new SafeFileHandle(IntPtr.Zero, true);

        /// <summary>
        /// Gets a list of underlying disks for this volume.
        /// </summary>
        public Collection<Device> Disks
        {
            get
            {
                if (_Disks == null)
                {
                    _Disks = new Collection<Device>();

                    DiskDeviceClass diskMgr = DiskDeviceClass.Instance;

                    if (IsUsb)
                    {
                        foreach (Device disk in diskMgr)
                        {
                            if (disk.Child != null &&
                                disk.Child.DeviceInstance == this.DeviceInstance)
                            {
                                _Disks.Add(disk);
                            }
                        }
                    }
                    else
                    {
                        if (DiskNumbers != null && DiskNumbers.Length > 0)
                        {
                            foreach (int index in DiskNumbers)
                            {
                                if (index < diskMgr.Devices.Count)
                                {
                                    _Disks.Add(diskMgr.Devices[index]);
                                }
                            }
                        }
                    }
                }

                return _Disks;
            }
        }

        public int[] DiskNumbers
        {
            get
            {
                if (_DiskNumbers == null)
                {
                    List<int> numbers = new List<int>();
                    if (LogicalDrive != null)
                    {

                        Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
                        secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
                        SafeFileHandle hFile = Win32.CreateFile(@"\\.\" + LogicalDrive, Win32.GENERIC_READ,
                            Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE,
                            ref secAttribs, Win32.OPEN_EXISTING, 0, IntPtr.Zero);
                        if (hFile.IsInvalid)
                        {
                            // got here when I disconnected a device while the app was coming up.
                            // The drive went away, so what should we do now?
                            return null;
                            throw new Win32Exception(Marshal.GetLastWin32Error());
                        }

                        int size = 0x400; // some big size
                        IntPtr buffer = Marshal.AllocHGlobal(size);
                        int bytesReturned = 0;
                        try
                        {
                            if (!Win32.DeviceIoControl(hFile, Win32.IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                IntPtr.Zero, 0, buffer, size, out bytesReturned, IntPtr.Zero))
                            {
                                // do nothing here on purpose
                                string errMsg = new Win32Exception(Marshal.GetLastWin32Error()).Message;
                            }
                        }
                        finally
                        {
                            hFile.Close();
                        }

                        if (bytesReturned > 0)
                        {
                            int numberOfDiskExtents = (int)Marshal.PtrToStructure(buffer, typeof(int));
                            for (int i = 0; i < numberOfDiskExtents; i++)
                            {
                                IntPtr extentPtr = new IntPtr(buffer.ToInt32() + Marshal.SizeOf(typeof(long)) + i * Marshal.SizeOf(typeof(Win32.DISK_EXTENT)));
                                Win32.DISK_EXTENT extent = (Win32.DISK_EXTENT)Marshal.PtrToStructure(extentPtr, typeof(Win32.DISK_EXTENT));
                                numbers.Add(extent.DiskNumber);
                            }
                        }
                        Marshal.FreeHGlobal(buffer);
                    }

                    _DiskNumbers = new int[numbers.Count];
                    numbers.CopyTo(_DiskNumbers);
                }
                return _DiskNumbers;
            }
        }

        public override string ToString()
        {
            string str = LogicalDrive;

            if (!String.IsNullOrEmpty(LogicalDrive))
            {
                str = LogicalDrive + "    ";
            }

            if (IsUsb)
                str += Parent.FriendlyName;
            else
                str += base.ToString();

            return str;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public class NT_SCSI_REQUEST
        {
            [MarshalAs(UnmanagedType.Struct, SizeConst = 18)]
            internal Win32.SCSI_PASS_THROUGH PassThrough = new Win32.SCSI_PASS_THROUGH();
            public UInt64 Tag;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 18)]
            public Byte[] SenseData = new Byte[18];
            [MarshalAs(UnmanagedType.ByValArray)]
            public Byte[] DataBuffer;          // Allocate buffer space

            public NT_SCSI_REQUEST(UInt32 dataBufferSize)
            {
                if (dataBufferSize == 0)
                    dataBufferSize = 1;

                DataBuffer = new Byte[dataBufferSize];
            }
        }

        public override int SendCommand(DevSupport.Api.Api api)
        {
            // reset the error string
            ErrorString = String.Empty;

            Api.ScsiApi scsiApi = api as Api.ScsiApi;
            if (scsiApi == null)
            {
                ErrorString = String.Format(" Error: Can not send \"{0}\" api type to \"{1}\" device.", api.ImageKey, this.ToString());
                return Win32.ERROR_INVALID_PARAMETER;
            }

            // tell the UI we are beginning a command.
            SendCommandProgressArgs cmdProgress = new SendCommandProgressArgs(api.ToString(), api.Direction, api.TransferSize);
            cmdProgress.Status = String.Format("SendCommand({0})", api.ToString());
            DoSendProgress(cmdProgress);

            // Open the device
//            Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
//            secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
//            SafeFileHandle hDrive = Win32.CreateFile(Path, Win32.GENERIC_READ | Win32.GENERIC_WRITE,
//                Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE, ref secAttribs, Win32.OPEN_EXISTING, Win32.FILE_FLAG_OVERLAPPED, IntPtr.Zero);
            SafeFileHandle hDrive = Lock(LockType.Logical);
            
            if (hDrive.IsInvalid)
            {
                cmdProgress.Error = Marshal.GetLastWin32Error();
                string errorMsg = new Win32Exception(cmdProgress.Error).Message;
                cmdProgress.Status += String.Format(" Error: {0}. ({1})", errorMsg, cmdProgress.Error);
                DoSendProgress(cmdProgress);

                Trace.WriteLine(String.Format("Volume.SendCommand({0}) {1}", api.ToString(), cmdProgress.Status));
                return cmdProgress.Error;
            }

            cmdProgress.Error = SendCommand(hDrive, api);

            Unlock(hDrive, true);

            // tell the UI we are done
            cmdProgress.InProgress = false;
            DoSendProgress(cmdProgress);

            return cmdProgress.Error;
        }

        private NativeOverlapped _OverLapped;
        private ManualResetEvent _DeviceIoCtrlEvent = new ManualResetEvent(false);
        private delegate bool DeviceIoControlAsync(SafeFileHandle hDevice, int dwIoControlCode, IntPtr lpInBuffer, int nInBufferSize, IntPtr lpOutBuffer, int nOutBufferSize, out int lpBytesReturned, IntPtr lpOverlapped);

        public Int32 SendCommand(SafeFileHandle hDrive, Api.Api api)
        {
            // reset the error string
            ErrorString = String.Empty;
//            _ScsiSenseStatus = Win32.SCSISTAT_GOOD;
//            _ScsiSenseData = new SENSE_DATA();

            Api.ScsiApi scsiApi = api as Api.ScsiApi;
            if (scsiApi == null)
            {
                ErrorString = String.Format(" Error: Can not send \"{0}\" api type to \"{1}\" device.", api.ImageKey, this.ToString());
                return Win32.ERROR_INVALID_PARAMETER;
            }

            // Allocate the SCSI request
            NT_SCSI_REQUEST pRequest = new NT_SCSI_REQUEST(Convert.ToUInt32(scsiApi.TransferSize));
            int totalSize = Marshal.SizeOf(pRequest) + (int)scsiApi.TransferSize;

            //
            // Set up structure for DeviceIoControl
            //
            // Length
            pRequest.PassThrough.Length = (UInt16)Marshal.SizeOf(pRequest.PassThrough);
            // SCSI Address
            //	sScsiRequest.sPass.PathId = psDeviceInfo->sScsiAddress.PathId;
            //	sScsiRequest.sPass.TargetId = psDeviceInfo->sScsiAddress.TargetId;
            //	sScsiRequest.sPass.Lun = psDeviceInfo->sScsiAddress.Lun;
            // CDB Information
            scsiApi.Cdb.CopyTo(pRequest.PassThrough.Cdb, 0);
            pRequest.PassThrough.CdbLength = (Byte)scsiApi.Cdb.Length;
            // Data
            pRequest.PassThrough.DataBufferOffset = (UInt32)Marshal.OffsetOf(typeof(NT_SCSI_REQUEST), "DataBuffer");
            pRequest.PassThrough.DataTransferLength = Convert.ToUInt32(scsiApi.TransferSize);
            pRequest.PassThrough.DataIn = (Byte)scsiApi.Direction;
            // Sense Information
            pRequest.PassThrough.SenseInfoOffset = (UInt32)Marshal.OffsetOf(typeof(NT_SCSI_REQUEST), "SenseData");
            pRequest.PassThrough.SenseInfoLength = (Byte)pRequest.SenseData.Length;
            // Timeout
            pRequest.PassThrough.TimeOutValue = scsiApi.Timeout; // seconds

            IntPtr requestPtr = Marshal.AllocHGlobal(totalSize);
            Marshal.StructureToPtr(pRequest, requestPtr, true);
            if ( api.Direction == DevSupport.Api.Api.CommandDirection.WriteWithData &&
                 scsiApi.TransferSize != 0 )
            {
                Marshal.Copy(scsiApi.Data, 0, (IntPtr)(requestPtr.ToInt32() + pRequest.PassThrough.DataBufferOffset), (Int32)scsiApi.TransferSize);
            }

            // Sending command asynchronously because MS documentation says, Regarding IOCTL_SCSI_PASS_THROUGH,
            // "Applications must not attempt to send a pass-through request asynchronously. 
            //  All pass-through requests must be synchronous."
            // So we are sending via an asynchronous delegate. 
            Int32 err = Win32.ERROR_SUCCESS;
            Int32 dwBytesReturned;
            DeviceIoControlAsync deviceIoControlDelegate = new DeviceIoControlAsync(Win32.DeviceIoControl);
            IAsyncResult ar = deviceIoControlDelegate.BeginInvoke(
                hDrive,
                Win32.IOCTL_SCSI_PASS_THROUGH,
                requestPtr,
                totalSize,
                requestPtr,
                totalSize,
                out dwBytesReturned,
                IntPtr.Zero,
                null, null);

            // Wait for the asynchronous operation to finish.
            while (!ar.IsCompleted)
            {
                // Keep UI messages moving, so the form remains 
                // responsive during the asynchronous operation.
                Application.DoEvents();
            }

            Marshal.PtrToStructure(requestPtr, pRequest);

            // Get the result of the operation.
            bool bResult = deviceIoControlDelegate.EndInvoke(out dwBytesReturned, ar);
            if (!bResult)
            {
                // DeviceIoControl failed so get the error code
                err = Marshal.GetLastWin32Error();
                ErrorString = " Error: " + new Win32Exception(err).Message + ".";
            }
            else 
            {
                // DeviceIoControl completed successfully
                if (scsiApi.Direction == DevSupport.Api.Api.CommandDirection.ReadWithData)
                {
                    // let the api parse the return data if there is any.
                    Byte[] localData = new Byte[scsiApi.TransferSize];
                    Marshal.Copy((IntPtr)(requestPtr.ToInt32() + pRequest.PassThrough.DataBufferOffset), localData, 0, (Int32)scsiApi.TransferSize);
//                    Marshal.PtrToStructure(requestPtr, pRequest);
//                    Marshal.FreeHGlobal(requestPtr);

                    err = scsiApi.ProcessResponse(localData/*pRequest.DataBuffer*/, 0, scsiApi.TransferSize);
                }
            }

            scsiApi.SenseStatus = pRequest.PassThrough.ScsiStatus;
            if (scsiApi.SenseStatus != Win32.ScsiSenseStatus.GOOD)
            {
                scsiApi.SenseData.CopyFrom(pRequest.SenseData);
                if (err == Win32.ERROR_SUCCESS /*&& scsiApi.SenseData.SenseKey != Win32.ScsiSenseKey.SCSI_SENSE_UNIQUE*/)
                    err = (Int32)scsiApi.SenseStatus;
            }

            Marshal.FreeHGlobal(requestPtr);
            return err;
        }

        public Int32 WriteDrive(Media.LogicalDrive.Tag tag, Byte[] data)
        {
            Int32 ret = Win32.ERROR_SUCCESS;

            // Calculate the total size of this api to set up the UI progressbar.
            UInt32 totalTransferSize = (UInt32)data.Length;
            totalTransferSize +=  4096;                     /*Accounts for partial sector*/
            totalTransferSize += (UInt32)data.Length / 10;  /*OpenDrive*/
            totalTransferSize += (UInt32)data.Length / 10;  /*GetAllocationTable*/
            totalTransferSize += (UInt32)data.Length / 10;  /*GetSectorSize*/
            totalTransferSize += (UInt32)data.Length / 10;  /*CheckSize*/
            totalTransferSize += (UInt32)data.Length / 5;   /*EraseDrive*/
            totalTransferSize += (UInt32)data.Length / 10;  /*CloseDrive*/

            // tell the UI we are beginning a command.
            SendCommandProgressArgs cmdProgress =
                new SendCommandProgressArgs("WriteDrive", Api.Api.CommandDirection.WriteWithData, totalTransferSize);
            cmdProgress.Status = String.Format("WriteDrive({0}).Open()", tag);
            DoSendProgress(cmdProgress);

//            Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
//            secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
//            SafeFileHandle hVolume = Win32.CreateFile(Path, Win32.GENERIC_READ | Win32.GENERIC_WRITE,
//                Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE, ref secAttribs, Win32.OPEN_EXISTING, 0, IntPtr.Zero);
            SafeFileHandle hVolume = Lock(LockType.Logical);
            if (hVolume.IsInvalid)
            {
                ret = Marshal.GetLastWin32Error();
                ErrorString = " Error: " + new Win32Exception(ret).Message + ".";
                Debug.WriteLine(String.Format("!ERROR: WriteDrive({0}).CreateFile() - {1}", tag, ErrorString));
                goto WriteDriveExit;
            }
            // Update the UI
            cmdProgress.Position += data.Length / 10;   /*OpenDrive*/ ;
            cmdProgress.Status = String.Format("WriteDrive({0}).SendCommand(GetAllocationTable)", tag);
            DoSendProgress(cmdProgress);

            //
            // Get the drive number for the provided drive tag.
            //
            ScsiVendorApi.GetAllocationTable apiGetTable = new ScsiVendorApi.GetAllocationTable();
            ret = SendCommand(hVolume, apiGetTable);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.WriteDrive({0}).SendCommand({1}) - {2}", tag, apiGetTable.ToString(), ErrorString));
                goto WriteDriveExit;
            }
            Byte driveNumber = apiGetTable.GetEntry(tag).DriveNumber;
            // Update the UI
            cmdProgress.Position += data.Length / 10;   /*GetAllocationTable*/;
            cmdProgress.Status = String.Format("WriteDrive({0}).SendCommand(GetLogicalDriveInfo, SectorSizeInBytes) driveNumber = {1}.", tag, driveNumber);
            DoSendProgress(cmdProgress);

            // Get the sector size
            ScsiVendorApi.GetLogicalDriveInfo apiDriveInfo =
                new ScsiVendorApi.GetLogicalDriveInfo(driveNumber, ScsiVendorApi.LogicalDriveInfo.SectorSizeInBytes, 0);
            ret = SendCommand(hVolume, apiDriveInfo);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.WriteDrive({0}).SendCommand({1}) - {2}", tag, apiDriveInfo.ToString(), ErrorString));
                goto WriteDriveExit;
            }
            UInt32 sectorSize = apiDriveInfo.SectorSize;
            // Update the UI
            cmdProgress.Position += data.Length / 10;   /*GetSectorSize*/ ;
            cmdProgress.Status = String.Format("WriteDrive({0}).WillDataFit() sectorSize = {1}", tag, sectorSize);
            DoSendProgress(cmdProgress);

            // Check the data will fit on the drive
            if (apiGetTable.GetEntry(tag).SizeInBytes < data.Length)
            {
                ret = Win32.ERROR_DISK_FULL;
                ErrorString = " Error: " + new Win32Exception(ret).Message + ".";
                Debug.WriteLine(String.Format("!ERROR: Volume.WriteDrive({0}) - {1}", tag, ErrorString));
                goto WriteDriveExit;
            }
            // Update the UI
            cmdProgress.Position += data.Length / 10;   /*CheckSize*/ ;
            cmdProgress.Status = String.Format("WriteDrive({0}).SendCommand(EraseLogicalDrive) dataSize = {1}, driveSize = {2}", tag, data.Length, apiGetTable.GetEntry(tag).SizeInBytes);
            DoSendProgress(cmdProgress);

            //
            // Erase the drive.
            //
            ScsiVendorApi.EraseLogicalDrive apiErase = new ScsiVendorApi.EraseLogicalDrive(driveNumber);
            ret = SendCommand(hVolume, apiErase);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.WriteDrive({0}).SendCommand({1}) - {2}", tag, apiGetTable.ToString(), ErrorString));
                goto WriteDriveExit;
            }
            // Update the UI
            cmdProgress.Position += data.Length / 5;    /*EraseDrive*/ ;
            cmdProgress.Status = String.Format("WriteDrive({0}).SendCommand(WriteLogicalDriveSector)", tag);
            DoSendProgress(cmdProgress);

            // Variables for iteration
            UInt32 sectorsPerWrite = MaxTransferSize / sectorSize;
            Byte[] buffer = new Byte[MaxTransferSize];

            //
            // Write drive in chunks
            //
            UInt32 byteIndex, numBytesToWrite = 0;
            for (byteIndex = 0; byteIndex < data.Length; byteIndex += numBytesToWrite)
            {
                // Init the buffer to 0xFF
                for (int i = 0; i < buffer.Length; ++i)
                    buffer[i] = 0xFF;

                // Get some data
                numBytesToWrite = (UInt32)Math.Min(sectorsPerWrite * sectorSize, data.Length - byteIndex);
                Array.Copy(data, byteIndex, buffer, 0, numBytesToWrite);

                // Get bytes to write in terms of sectors
                UInt32 sectorsToWrite = numBytesToWrite / sectorSize;
                if ((numBytesToWrite % sectorSize) > 0)
                    ++sectorsToWrite;

                // Write the data to the device
                ScsiVendorApi.WriteLogicalDriveSector apiWrite =
                    new ScsiVendorApi.WriteLogicalDriveSector(driveNumber, sectorSize,
                        byteIndex / sectorSize, sectorsToWrite, buffer);

                ret = SendCommand(hVolume, apiWrite);
/*
                SendCommandAsync sendCommandDelegate = new SendCommandAsync(SendCommand);
                IAsyncResult ar = sendCommandDelegate.BeginInvoke(apiWrite, null, null);
                // Wait for the asynchronous operation to finish.
                while (!ar.IsCompleted)
                {
                    // Keep UI messages moving, so the form remains 
                    // responsive during the asynchronous operation.
                    Application.DoEvents();
                }

                // Get the result of the operation.
                ret = sendCommandDelegate.EndInvoke(ar);
*/
                if (ret != Win32.ERROR_SUCCESS)
                {
                    Debug.WriteLine(String.Format("!ERROR: Volume.WriteDrive({0}).SendCommand({1}) - {2}", tag, apiWrite, ErrorString));
                    goto WriteDriveExit;
                }
                // Update the UI
                cmdProgress.Position += (Int32)apiWrite.TransferSize;
                cmdProgress.Status = String.Format("WriteDrive({0}).SendCommand(WriteLogicalDriveSector, {1}, {2})", tag, byteIndex, numBytesToWrite);
                DoSendProgress(cmdProgress);
            }

        WriteDriveExit:

            Unlock(hVolume, true);

            // tell the UI we are done
            cmdProgress.Position = (Int32)totalTransferSize;
            cmdProgress.Error = ret;
            cmdProgress.InProgress = false;
            DoSendProgress(cmdProgress);

            return ret;
        }

        public Int32 WriteAllFwDrives(Byte[] data)
        {
            Int32 ret = Win32.ERROR_SUCCESS;

            // The list of possible drives to write.
            LogicalDrive.Tag[] fwDriveTags = new LogicalDrive.Tag[3] { Media.LogicalDrive.Tag.FirmwareImg, Media.LogicalDrive.Tag.FirmwareImg2, Media.LogicalDrive.Tag.FirmwareImg3 };

            //
            // Open the device.
            //
//            Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
//            secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
//            SafeFileHandle hVolume = Win32.CreateFile(Path, Win32.GENERIC_READ | Win32.GENERIC_WRITE,
//                Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE, ref secAttribs, Win32.OPEN_EXISTING, 0, IntPtr.Zero);
            SafeFileHandle hVolume = Lock(LockType.Logical);
            if (hVolume.IsInvalid)
            {
                ret = Marshal.GetLastWin32Error();
                ErrorString = " Error: " + new Win32Exception(ret).Message + ".";
                Debug.WriteLine(String.Format("!ERROR: Volume.WriteAllFwDrives().CreateFile() - {0}", ErrorString));
                return ret;
            }

            //
            // Get the drive array.
            //
            ScsiVendorApi.GetAllocationTable apiGetTable = new ScsiVendorApi.GetAllocationTable();
            ret = SendCommand(hVolume, apiGetTable);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.WriteAllFwDrives().SendCommand({0}) - {1}", apiGetTable.ToString(), ErrorString));
                Unlock(hVolume, true);
                return ret;
            }

            //
            // Close the device
            //
            Unlock(hVolume, true);

            //
            // Write each drive in the drive arry that is in our list.
            //
            foreach (MediaAllocationEntry drive in apiGetTable.DriveEntryArray )
            {
                if (Array.IndexOf(fwDriveTags, drive.Tag) != -1)
                {
                    ret = WriteDrive(drive.Tag, data);
                    if (ret != Win32.ERROR_SUCCESS)
                        break;
                }
            }

            return ret;
        }

        public Byte[] ReadDrive(Media.LogicalDrive.Tag tag)
        {
            Int32 ret = Win32.ERROR_SUCCESS;

            // tell the UI we are beginning a command.
            SendCommandProgressArgs tempProgress =
                new SendCommandProgressArgs("ReadDrive", Api.Api.CommandDirection.ReadWithData, 0);
            tempProgress.Status = String.Format("ReadDrive({0}).Open()", tag);
            DoSendProgress(tempProgress);

//            Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
//            secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
//            SafeFileHandle hVolume = Win32.CreateFile(Path, Win32.GENERIC_READ | Win32.GENERIC_WRITE,
//                Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE, ref secAttribs, Win32.OPEN_EXISTING, 0, IntPtr.Zero);
            SafeFileHandle hVolume = Lock(LockType.Logical);
            if (hVolume.IsInvalid)
            {
                ret = Marshal.GetLastWin32Error();
                ErrorString = " Error: " + new Win32Exception(ret).Message;
                Debug.WriteLine(String.Format("!ERROR: Volume.ReadDrive({0}).CreateFile() - {1}", tag, ErrorString));
                return null;
            }
            // Update the UI
            tempProgress.Status = String.Format("ReadDrive({0}).SendCommand(GetAllocationTable)", tag);
            DoSendProgress(tempProgress);

            //
            // Get the drive number for the provided drive tag.
            //
            ScsiVendorApi.GetAllocationTable apiGetTable = new ScsiVendorApi.GetAllocationTable();
            ret = SendCommand(hVolume, apiGetTable);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.ReadDrive({0}).SendCommand({1}) - {2}", tag, apiGetTable.ToString(), ErrorString));
                Unlock(hVolume, true);
                return null;
            }
            Byte driveNumber = apiGetTable.GetEntry(tag).DriveNumber;
            List<Byte> data = null;

            Int64 length = apiGetTable.GetEntry(tag).SizeInBytes;
            if ( length - 4096 > Int32.MaxValue)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.ReadDrive({0}).AllocateBuffer({1}) - Drive too big to read.", tag, apiGetTable.GetEntry(tag).SizeInBytes));
                Unlock(hVolume, true);
                return null;
            }
            else
            {
                data = new List<Byte>(Convert.ToInt32(apiGetTable.GetEntry(tag).SizeInBytes));
            }

            // Update the UI
            tempProgress.Status = String.Format("ReadDrive({0}).SendCommand(GetLogicalDriveInfo) driveNumber = {1}, driveSize = {2}.", tag, driveNumber, length);
            DoSendProgress(tempProgress);

            // Get the sector size
            ScsiVendorApi.GetLogicalDriveInfo apiDriveInfo =
                new ScsiVendorApi.GetLogicalDriveInfo(driveNumber, ScsiVendorApi.LogicalDriveInfo.SectorSizeInBytes, 0);
            ret = SendCommand(hVolume, apiDriveInfo);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.ReadDrive({0}).SendCommand({1}) - {2}", tag, apiDriveInfo.ToString(), ErrorString));
                Unlock(hVolume, true);
                return null;
            }
            UInt32 sectorSize = apiDriveInfo.SectorSize;

            // Update the UI
            SendCommandProgressArgs cmdProgress =
                new SendCommandProgressArgs("ReadDrive", Api.Api.CommandDirection.ReadWithData, (UInt32)length + 4096);
            cmdProgress.Status = String.Format("ReadDrive({0}).SendCommand(ReadLogicalDriveSector) sectorSize = {1}", tag, sectorSize);
            DoSendProgress(cmdProgress);

            // Variables for iteration
            UInt32 sectorsPerRead = MaxTransferSize / sectorSize;
            Byte[] buffer = new Byte[MaxTransferSize];

            //
            // Read drive in chunks
            //
            UInt32 byteIndex, numBytesToRead = 0;
            for (byteIndex = 0; byteIndex < length; byteIndex += numBytesToRead)
            {
                // Get bytes to write in terms of sectors
                numBytesToRead = (UInt32)Math.Min(sectorsPerRead * sectorSize, length - byteIndex);
                UInt32 sectorsToRead = numBytesToRead / sectorSize;
                if ((numBytesToRead % sectorSize) > 0)
                    ++sectorsToRead;

                // Read the data from the device
                ScsiVendorApi.ReadLogicalDriveSector apiReadSector =
                    new ScsiVendorApi.ReadLogicalDriveSector(driveNumber, sectorSize,
                        byteIndex / sectorSize, sectorsToRead);

                ret = SendCommand(hVolume, apiReadSector);
                if (ret != Win32.ERROR_SUCCESS)
                {
                    Debug.WriteLine(String.Format("!ERROR: Volume.ReadDrive({0}).SendCommand({1}) - {2}", tag, apiReadSector.ToString(), ErrorString));
                    goto ReadDriveExit;
                }

                // Get the data
                if ( apiReadSector.Data != null )
                    data.AddRange(apiReadSector.Data);
//                Array.Copy(apiReadSector.Data, 0, data, byteIndex, numBytesToRead);

                // Update the UI
                cmdProgress.Maximum = (UInt32)length;
                cmdProgress.Position += (Int32)apiReadSector.TransferSize;
                cmdProgress.Status = String.Format("ReadDrive({0}).SendCommand(ReadLogicalDriveSector, {1}, {2})", tag, byteIndex, numBytesToRead);
                DoSendProgress(cmdProgress);

            }

        ReadDriveExit:

            Unlock(hVolume, true);

            // tell the UI we are done
            cmdProgress.Error = ret;
            cmdProgress.InProgress = false;
            DoSendProgress(cmdProgress);

            return data.ToArray();
        }

        public bool VerifyDrive(Media.LogicalDrive.Tag tag, Byte[] data)
        {
            Byte[] driveData = ReadDrive(tag);
            Array.Resize(ref driveData, data.Length);

            for (int i = 0; i < data.Length; ++i)
            {
                if (data[i] != driveData[i])
                {
                    ErrorString = String.Format("Index = {0}, media = {1}, firmware = {2}", i, driveData[i], data[i]);
                    return false;
                }
            }

            return true;
        }

        public Int32 DumpDrive(Media.LogicalDrive.Tag tag)
        {
            Int32 ret = Win32.ERROR_SUCCESS;

            // tell the UI we are beginning a command.
            SendCommandProgressArgs tempProgress =
                new SendCommandProgressArgs("DumpDrive", Api.Api.CommandDirection.ReadWithData, 0);
            tempProgress.Status = String.Format("DumpDrive({0}).Open()", tag);
            DoSendProgress(tempProgress);

            SafeFileHandle hVolume = Lock(LockType.Logical);
            if (hVolume.IsInvalid)
            {
                ret = Marshal.GetLastWin32Error();
                ErrorString = " Error: " + new Win32Exception(ret).Message;
                Debug.WriteLine(String.Format("!ERROR: Volume.DumpDrive({0}).CreateFile() - {1}", tag, ErrorString));
                return ret;
            }
            // Update the UI
            tempProgress.Status = String.Format("DumpDrive({0}).SendCommand(GetAllocationTable)", tag);
            DoSendProgress(tempProgress);

            //
            // Get the drive number for the provided drive tag.
            //
            ScsiVendorApi.GetAllocationTable apiGetTable = new ScsiVendorApi.GetAllocationTable();
            ret = SendCommand(hVolume, apiGetTable);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.DumpDrive({0}).SendCommand({1}) - {2}", tag, apiGetTable.ToString(), ErrorString));
                Unlock(hVolume, true);
                return ret;
            }
            Byte driveNumber = apiGetTable.GetEntry(tag).DriveNumber;
            Int64 length = apiGetTable.GetEntry(tag).SizeInBytes;

            // Update the UI
            tempProgress.Status = String.Format("DumpDrive({0}).SendCommand(GetLogicalDriveInfo) driveNumber = {1}, driveSize = {2}.", tag, driveNumber, length);
            DoSendProgress(tempProgress);

            // Get the sector size
            ScsiVendorApi.GetLogicalDriveInfo apiDriveInfo =
                new ScsiVendorApi.GetLogicalDriveInfo(driveNumber, ScsiVendorApi.LogicalDriveInfo.SectorSizeInBytes, 0);
            ret = SendCommand(hVolume, apiDriveInfo);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.DumpDrive({0}).SendCommand({1}) - {2}", tag, apiDriveInfo.ToString(), ErrorString));
                Unlock(hVolume, true);
                return ret;
            }
            UInt32 sectorSize = apiDriveInfo.SectorSize;

            // Update the UI
            SendCommandProgressArgs cmdProgress =
                new SendCommandProgressArgs("DumpDrive", Api.Api.CommandDirection.ReadWithData, 100);
            cmdProgress.Status = String.Format("DumpDrive({0}).SendCommand(ReadLogicalDriveSector) sectorSize = {1}", tag, sectorSize);
            DoSendProgress(cmdProgress);

            // Open output file
            Utils.EnumConverterEx converter = new Utils.EnumConverterEx(typeof(Media.LogicalDrive.Tag));
            FileStream fs = new FileStream(String.Format("./{0}.bin", converter.ConvertToString(tag)), FileMode.OpenOrCreate, FileAccess.Write);
            
            // Variables for iteration
            UInt32 sectorsPerRead = MaxTransferSize / sectorSize;

            //
            // Read drive in chunks
            //
            UInt32 byteIndex, numBytesToRead = 0;
            Int64 totalBytesRead = 0;
            for (byteIndex = 0; byteIndex < length; byteIndex += numBytesToRead)
            {
                // Get bytes to write in terms of sectors
                numBytesToRead = (UInt32)Math.Min(sectorsPerRead * sectorSize, length - byteIndex);
                UInt32 sectorsToRead = numBytesToRead / sectorSize;
                if ((numBytesToRead % sectorSize) > 0)
                    ++sectorsToRead;

                // Read the data from the device
                ScsiVendorApi.ReadLogicalDriveSector apiReadSector =
                    new ScsiVendorApi.ReadLogicalDriveSector(driveNumber, sectorSize,
                        byteIndex / sectorSize, sectorsToRead);

                ret = SendCommand(hVolume, apiReadSector);
                if (ret != Win32.ERROR_SUCCESS)
                {
                    Debug.WriteLine(String.Format("!ERROR: Volume.DumpDrive({0}).SendCommand({1}) - {2}", tag, apiReadSector.ToString(), ErrorString));
                    goto DumpDriveExit;
                }

                // Write the data to the file
                fs.Write(apiReadSector.Data, 0, apiReadSector.Data.Length);

                // Update the UI
                totalBytesRead += apiReadSector.TransferSize;
                cmdProgress.Position = Convert.ToInt32(totalBytesRead * 100 / length);
                cmdProgress.Status = String.Format("DumpDrive({0}).SendCommand(ReadLogicalDriveSector, {1}, {2})", tag, byteIndex, numBytesToRead);
                DoSendProgress(cmdProgress);
            }

        DumpDriveExit:

            Unlock(hVolume, true);
            fs.Close();

            // tell the UI we are done
            cmdProgress.Error = ret;
            cmdProgress.InProgress = false;
            DoSendProgress(cmdProgress);

            return ret;
        }

        public Int32 WriteJanusHeader(SafeFileHandle hVolume)
        {
            //
            // Get the drive number for the provided drive tag.
            //
            ScsiVendorApi.GetAllocationTable apiGetTable = new ScsiVendorApi.GetAllocationTable();
            Int32 ret = SendCommand(hVolume, apiGetTable);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.WriteJanusHeader().SendCommand({0}) - {1}", apiGetTable.ToString(), ErrorString));
                return ret;
            }
            Byte driveNumber = apiGetTable.GetEntry(Media.LogicalDrive.Tag.DataJanus).DriveNumber;

            // Get the sector size
            ScsiVendorApi.GetLogicalDriveInfo apiDriveInfo =
                new ScsiVendorApi.GetLogicalDriveInfo(driveNumber, ScsiVendorApi.LogicalDriveInfo.SizeInSectors, 0);
            ret = SendCommand(apiDriveInfo);
            if (ret != Win32.ERROR_SUCCESS)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.WriteJanusHeader().SendCommand({0}) - {1}", apiDriveInfo.ToString(), ErrorString));
                return ret;
            }
            UInt32 janusSectors = Convert.ToUInt32(apiDriveInfo.SizeInSectors);

            return WriteDrive(Media.LogicalDrive.Tag.DataJanus, new JanusDriveHeader(janusSectors).ToArray());
        }

        public Int32 FormatDataDrive(LogicalDrive.Tag dataDriveTag, FormatBuilder.FormatInfo.FileSystemType fileSystem, String volumeLabel, bool includeMBR)
        {
            // Get the size of the data drive
			ScsiFormalApi.ReadCapacity apiReadCapacity = new ScsiFormalApi.ReadCapacity();
            Int32 retValue = SendCommand(apiReadCapacity);
			if ( retValue != Win32.ERROR_SUCCESS )
			{
                Debug.WriteLine(String.Format("!ERROR: Volume.FormatDataDrive({0}, {1}, {2}, {3}).SendCommand({4}) - {5}", dataDriveTag, fileSystem, volumeLabel, includeMBR, apiReadCapacity.ToString(), ErrorString));
				return retValue;
			}
			UInt32 sectors = apiReadCapacity.LogicalBlockAddress + 1;
            UInt32 sectorSize = apiReadCapacity.BytesPerBlock;

            // Get the FormatInfo
			FormatBuilder.FormatInfo formatInfo =
                new DevSupport.FormatBuilder.FormatInfo(sectors, (UInt16)sectorSize, fileSystem, volumeLabel, includeMBR);

			// Create the Format Image
            FormatBuilder.FormatImage formatImage = new DevSupport.FormatBuilder.FormatImage(formatInfo);

            return retValue = WriteDrive(dataDriveTag, formatImage.TheImage);
/*
            SafeFileHandle hVolume = Lock(LockType.Physical);
            if (hVolume.IsInvalid)
            {
                Debug.WriteLine(String.Format("!ERROR: Volume.FormatDataDrive({0}, {1}, {2}, {3}).Lock({4}) - Failed.", dataDriveTag, fileSystem, volumeLabel, includeMBR, LockType.Physical));
                ErrorString = String.Format(" ERROR: {0}. ({1})", new Win32Exception(Win32.ERROR_LOCK_FAILED).Message, Win32.ERROR_LOCK_FAILED);
                return Win32.ERROR_LOCK_FAILED;
            }
            Unlock(hVolume);

            return retValue;
*/
        }

        // Locks a volume. A locked volume can be accessed only through handles to the file object that locks the volume.
        public SafeFileHandle Lock(LockType lockType)
        {
            Int32 bytesReturned;
            Int32 error = Win32.ERROR_SUCCESS;

            String disk;
            if (lockType == LockType.Physical)
            {
                if (DiskNumbers != null && DiskNumbers.Length > 0)
                {
                    disk = String.Format("\\\\.\\PhysicalDrive{0}", DiskNumbers[0]);
                }
                else
                {
                    ErrorString = String.Format("Error: Failed to get Physical Disk number for Logical drive {0}.", LogicalDrive);
                    Debug.WriteLine(String.Format("Volume.Lock() - {0}", ErrorString));
                    return LockHandle;
                }
            }
            else
            {
                disk = Path;
            }

            Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
            secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
            LockHandle = Win32.CreateFile(
                disk,
                Win32.GENERIC_READ | Win32.GENERIC_WRITE,
                Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE,
                ref secAttribs,
                Win32.OPEN_EXISTING,
                Win32.FILE_FLAG_OVERLAPPED,
                IntPtr.Zero);

            if (LockHandle.IsInvalid)
            {
                error = Marshal.GetLastWin32Error();
                ErrorString = "Error: " + new Win32Exception(error).Message + ".";
                Debug.WriteLine(String.Format("Volume.Lock() - {0}", ErrorString));
                return LockHandle;
            }

            _DeviceIoCtrlEvent.Reset();
            _OverLapped = new NativeOverlapped();
            _OverLapped.EventHandle = _DeviceIoCtrlEvent.SafeWaitHandle.DangerousGetHandle();

            Boolean bResult = Win32.DeviceIoControl(
                        LockHandle,
                        Win32.FSCTL_LOCK_VOLUME,
                        IntPtr.Zero, 0,
                        IntPtr.Zero, 0,
                        out bytesReturned,
                        ref _OverLapped );
            
            if (bResult)
            {
                // DeviceIoControl completed successfully

                // Pump messages while we wait.
                while (!_DeviceIoCtrlEvent.WaitOne(0, false))
                {
                    Application.DoEvents();
                    Debug.WriteLine(String.Format("Volume.Lock() - waiting for lock.", ErrorString));
                }

                return LockHandle;
            }

            // DeviceIoControl failed so get the error code
            error = Marshal.GetLastWin32Error();
            ErrorString = "Error: " + new Win32Exception(error).Message + ".";
            Debug.WriteLine(String.Format("Volume.Lock() - {0}", ErrorString));

            return LockHandle;
        }

        // Unlocks a volume.
        public Int32 Unlock(SafeFileHandle hDrive, bool close)
        {
            Int32 bytesReturned;
            Int32 error = Win32.ERROR_SUCCESS;

            _DeviceIoCtrlEvent.Reset();
            _OverLapped = new NativeOverlapped();
            _OverLapped.EventHandle = _DeviceIoCtrlEvent.SafeWaitHandle.DangerousGetHandle();

            Boolean bResult = Win32.DeviceIoControl(
                        hDrive,
                        Win32.FSCTL_UNLOCK_VOLUME,
                        IntPtr.Zero, 0,
                        IntPtr.Zero, 0,
                        out bytesReturned,
                        ref _OverLapped);

            if (bResult)
            {
                // Pump messages while we wait.
                while (!_DeviceIoCtrlEvent.WaitOne(0, false))
                {
                    Application.DoEvents();
                }

                // DeviceIoControl completed successfully

                if ( close )
                    hDrive.Close();
                
                return Win32.ERROR_SUCCESS;
            }

            // DeviceIoControl failed so get the error code
            if ( close )
                hDrive.Close();
            
            error = Marshal.GetLastWin32Error();
            ErrorString = "Error: " + new Win32Exception(error).Message + ".";
            Debug.WriteLine(String.Format("Volume.UnLock() - {0}", ErrorString));

            return error;
        }

        // Dismounts a volume.
        Int32 Dismount(SafeFileHandle hDrive)
        {
            Int32 bytesReturned;
            Int32 sleepAmount;
            Int32 tryCount;

            sleepAmount = LockTimeout / LockRetries;

            // Do this in a loop until a timeout period has expired
            for (tryCount = 0; tryCount < LockRetries; ++tryCount)
            {
                if (Win32.DeviceIoControl(
                        hDrive,
                        Win32.FSCTL_DISMOUNT_VOLUME,
                        IntPtr.Zero, 0,
                        IntPtr.Zero, 0,
                        out bytesReturned,
                        IntPtr.Zero
                    ))
                {
                    return Win32.ERROR_SUCCESS;
                }
                Thread.Sleep(sleepAmount);
            }

            Int32 error = Marshal.GetLastWin32Error();
            ErrorString = new Win32Exception(error).Message + ".";

            return error;
        }

        // Invalidates the cached partition table and re-enumerates the device.
        Int32 Update(SafeFileHandle hDrive)
        {
            Int32 bytesReturned;
            Int32 sleepAmount;
            Int32 tryCount;

            sleepAmount = LockTimeout / LockRetries;

            // Do this in a loop until a timeout period has expired
            for (tryCount = 0; tryCount < LockRetries; ++tryCount)
            {
                if (Win32.DeviceIoControl(
                        hDrive,
                        Win32.IOCTL_DISK_UPDATE_PROPERTIES,
                        IntPtr.Zero, 0,
                        IntPtr.Zero, 0,
                        out bytesReturned,
                        IntPtr.Zero
                    ))
                {
                    return Win32.ERROR_SUCCESS;
                }
                Thread.Sleep(sleepAmount);
            }

            Int32 error = Marshal.GetLastWin32Error();
            ErrorString = new Win32Exception(error).Message + ".";

            return error;
        }

        #region ILiveUpdater Members

        public byte[] GetDeviceDataFromFile(string fileName)
        {
            Byte[] returnBytes = null;

            String fullFilename = LogicalDrive + @"\" + fileName;
            try
            {
                returnBytes = File.ReadAllBytes(fullFilename);
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
            }

            return returnBytes;
        }

        public int CopyUpdateFileToMedia(string fileName)
        {
            // reset the error string
            Int32 retValue = Win32.ERROR_SUCCESS;
            ErrorString = String.Empty;

            ///
            /// Get the firmware file data
            ///
            FileInfo fileInfo = new FileInfo(fileName);
            if (!fileInfo.Exists)
            {
                ErrorString = String.Format(" Error: \"{0}\" does not exist.", fileName);
                return Win32.ERROR_FILE_NOT_FOUND;
            }

            Byte[] fileData = new Byte[fileInfo.Length];
            try
            {
                fileData = File.ReadAllBytes(fileInfo.FullName);
            }
            catch (Exception e)
            {
                ErrorString = String.Format(" Error: Problem reading {0}. {1}.", fileName, e.Message);
                return Win32.ERROR_OPEN_FAILED;
            }

            ///
            /// Tell the UI we are beginning a command.
            ///
            SendCommandProgressArgs cmdProgress = new SendCommandProgressArgs("CopyUpdateFileToMedia", Api.Api.CommandDirection.WriteWithData, (UInt32)fileInfo.Length);
            DoSendProgress(cmdProgress);

            ///
            /// Open the device. File will be overwritten if it exists on the device.
            ///
            String destFilename = LogicalDrive + @"\" + System.IO.Path.GetFileName(fileName);
            using (FileStream writeStream = new FileStream(destFilename, System.IO.FileMode.Create))
            {
                ///
                /// Transfer the content to the device
                ///

                uint optimalBufferSize = Volume.MaxTransferSize;

                // write the bytes
                Int32 totalBytesWritten = 0;
                Byte[] streamBuffer = new byte[optimalBufferSize];

                while (totalBytesWritten < (Int32)fileInfo.Length)
                {
                    Int32 numBytesToWrite = Math.Min(streamBuffer.Length, fileData.Length - totalBytesWritten);
                    Array.Copy(fileData, totalBytesWritten, streamBuffer, 0, numBytesToWrite);
                    try
                    {
                        writeStream.Write(streamBuffer, 0, numBytesToWrite);
                        totalBytesWritten += numBytesToWrite;

                        // Update the UI
                        cmdProgress.Position = Convert.ToInt32(totalBytesWritten);
                        DoSendProgress(cmdProgress);
                    }
                    catch (Exception e)
                    {
                        ErrorString = e.Message;
                        retValue = Win32.ERROR_GEN_FAILURE;
                        goto CopyUpdateFileToMedia_Exit;
                    }
                }

                if (totalBytesWritten != fileInfo.Length)
                {
                    ErrorString = String.Format(" Error: Only wrote {0} of {1} bytes to device.", totalBytesWritten, fileInfo.Length);
                    retValue = Win32.ERROR_BAD_LENGTH;
                    goto CopyUpdateFileToMedia_Exit;
                }
            }

        CopyUpdateFileToMedia_Exit:

            // Tell the UI we are done.
            cmdProgress.Error = retValue;
            cmdProgress.InProgress = false;
            DoSendProgress(cmdProgress);

            return Win32.ERROR_SUCCESS;
        }

        #endregion

        #region IResetToRecovery Members

        public Int32 ResetToRecovery()
        {
            Int32 retValue = 0;

            ScsiVendorApi.ResetToRecovery api = new ScsiVendorApi.ResetToRecovery();

            if (LockHandle.IsInvalid)
            {
                retValue = SendCommand(api);
            }
            else
            {
                Unlock(LockHandle, false);
                retValue = SendCommand(LockHandle, api);
            }

            return retValue;
        }

        #endregion

        #region IChipReset Members

        public int ChipReset()
        {
            Int32 retValue = 0;

            ScsiVendorApi.ChipReset api = new ScsiVendorApi.ChipReset();
            if (LockHandle.IsInvalid)
            {
                retValue = SendCommand(api);
            }
            else
            {
                Unlock(LockHandle, false);
                retValue = SendCommand(LockHandle, api);
            }

            return retValue;
        }

        #endregion
/*
        public Boolean IsCompatible(Media.LogicalDrive[] newDriveArray)
        {
            // Get the existing allocation table on the media.
            ScsiVendorApi.GetAllocationTable apiGetAllocation = new ScsiVendorApi.GetAllocationTable();
            Int32 retValue = SendCommand(apiGetAllocation);
            if (retValue != Win32.ERROR_SUCCESS)
            {
                return false;
            }

            // Compare the number of logical drives
            if (apiGetAllocation.NumReturnedEntries != newDriveArray.Length)
            {
                ErrorString = String.Format("Number of media allocations ({0}) != New drive array size ({1})", apiGetAllocation.NumReturnedEntries, newDriveArray.Length);
                return false;
            }

            // Compare the type and size of each array by tag
            foreach ( LogicalDrive newDrive in newDriveArray )
            {
                MediaAllocationEntry existingDrive = apiGetAllocation.GetEntry(newDrive.DriveTag);
                // The tags should match, but may not if there was not an Allocation Entry with the new drive tag.
                if (existingDrive.Tag != newDrive.DriveTag)
                {
                    ErrorString = String.Format("Media[{0}].Tag({1}) != Drive[{0}].Tag({2})", newDrive.DriveTag, existingDrive.Tag, newDrive.DriveTag);
                    return false;
                }
                if (existingDrive.Type != newDrive.DriveType)
                {
                    ErrorString = String.Format("Media[{0}].Type({1}) != Drive[{0}].Type({2})", newDrive.DriveTag, existingDrive.Type, newDrive.DriveType);
                    return false;
                }
                // For system drives, check that the actual firmware size will fit into the allocation.
                if ( existingDrive.Type == Media.LogicalDrive.Type.System )
                {
                    if (existingDrive.SizeInBytes < newDrive.FirmwareSize)
                    {
                        ErrorString = String.Format("Media[{0}].Size({1}) < Drive[{0}].FirmwareSize({2})", newDrive.DriveTag, existingDrive.SizeInBytes, newDrive.FirmwareSize);
                        return false;
                    }
                }
            }

            return true;
        }
*/
        public static Boolean IsCompatible(MediaAllocationEntry[] mediaArray, Media.LogicalDrive[] driveArray, out String errDesc)
        {
            // Compare the number of logical drives
            if (mediaArray.Length != driveArray.Length)
            {
                errDesc = String.Format("Number of media allocations ({0}) != New drive array size ({1}).", mediaArray.Length, driveArray.Length);
                return false;
            }

            Utils.EnumConverterEx tagCvtr = new Utils.EnumConverterEx(typeof(Media.LogicalDrive.Tag));
            Utils.ByteFormatConverter byteCvtr = new Utils.ByteFormatConverter();

            // Compare the type and size of each array by tag
            foreach (LogicalDrive newDrive in driveArray)
            {
                
                MediaAllocationEntry existingDrive = MediaAllocationEntry.GetEntry(newDrive.DriveTag, mediaArray);
                // The tags should match, but may not if there was not an Allocation Entry with the new drive tag.
                if (existingDrive == null)
                {
                    errDesc = String.Format("Drive {0} is not allocated on the media.", 
                        tagCvtr.ConvertToString(newDrive.DriveTag));
                    return false;
                }
                if (existingDrive.Type != newDrive.DriveType)
                {
                    errDesc = String.Format("{0} (media) != {1} (drive) for Tag = {2}.",
                        existingDrive.Type, newDrive.DriveType, tagCvtr.ConvertToString(newDrive.DriveTag));
                    return false;
                }
                // For system drives, check that the actual firmware size will fit into the allocation.
                if (existingDrive.Type == Media.LogicalDrive.Type.System)
                {
                    if (existingDrive.SizeInBytes < newDrive.FirmwareSize)
                    {
                        errDesc = String.Format("Media size({0}) < Firmware size({1}) for Tag = {2}.",
                            byteCvtr.ConvertToString(existingDrive.SizeInBytes),
                            byteCvtr.ConvertToString(newDrive.FirmwareSize),
                            tagCvtr.ConvertToString(newDrive.DriveTag));
                        return false;
                    }
                }
            }
            errDesc = String.Empty;
            return true;
        }

        public static Boolean IsCompatible(MediaAllocationEntryEx[] mediaArray, Media.LogicalDrive[] driveArray, out String errDesc)
        {
            // Compare the number of logical drives
            if (mediaArray.Length != driveArray.Length)
            {
                errDesc = String.Format("Number of media allocations ({0}) != New drive array size ({1}).", mediaArray.Length, driveArray.Length);
                return false;
            }

            Utils.EnumConverterEx tagCvtr = new Utils.EnumConverterEx(typeof(Media.LogicalDrive.Tag));
            Utils.ByteFormatConverter byteCvtr = new Utils.ByteFormatConverter();

            // Compare the type and size of each array by tag
            foreach (LogicalDrive newDrive in driveArray)
            {

                MediaAllocationEntryEx existingDrive = MediaAllocationEntryEx.GetEntry(newDrive.DriveTag, mediaArray);
                // The tags should match, but may not if there was not an Allocation Entry with the new drive tag.
                if (existingDrive == null)
                {
                    errDesc = String.Format("Drive {0} is not allocated on the media.",
                        tagCvtr.ConvertToString(newDrive.DriveTag));
                    return false;
                }
                if (existingDrive.Type != newDrive.DriveType)
                {
                    errDesc = String.Format("{0} (media) != {1} (drive) for Tag = {2}.",
                        existingDrive.Type, newDrive.DriveType, tagCvtr.ConvertToString(newDrive.DriveTag));
                    return false;
                }
                // For system drives, check that the actual firmware size will fit into the allocation.
                if (existingDrive.Type == Media.LogicalDrive.Type.System)
                {
                    if (existingDrive.Size < (UInt64)newDrive.FirmwareSize)
                    {
                        errDesc = String.Format("Media size({0}) < Firmware size({1}) for Tag = {2}.",
                            byteCvtr.ConvertToString(existingDrive.Size),
                            byteCvtr.ConvertToString(newDrive.FirmwareSize),
                            tagCvtr.ConvertToString(newDrive.DriveTag));
                        return false;
                    }
                }
            }
            errDesc = String.Empty;
            return true;
        }
    }
}

/*
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;
using System.IO;

namespace WindowsApplication1 {
  public partial class Form1 : Form {
    // Class to report progress
    private class UIProgress {
      public UIProgress(string name_, long bytes_, long maxbytes_) {
        name = name_; bytes = bytes_; maxbytes = maxbytes_;
      }
      public string name;
      public long bytes;
      public long maxbytes;
    }
    // Class to report exception {
    private class UIError {
      public UIError(Exception ex, string path_) {
        msg = ex.Message; path = path_; result = DialogResult.Cancel;
      }
      public string msg;
      public string path;
      public DialogResult result;
    }
    private BackgroundWorker mCopier;
    private delegate void ProgressChanged(UIProgress info);
    private delegate void CopyError(UIError err);
    private ProgressChanged OnChange;
    private CopyError OnError;

    public Form1() {
      InitializeComponent();
      mCopier = new BackgroundWorker();
      mCopier.DoWork += Copier_DoWork;
      mCopier.RunWorkerCompleted += Copier_RunWorkerCompleted;
      mCopier.WorkerSupportsCancellation = true;
      OnChange += Copier_ProgressChanged;
      OnError += Copier_Error;
      button1.Click += button1_Click;
      ChangeUI(false);
    }

    private void Copier_DoWork(object sender, DoWorkEventArgs e) {
      // Create list of files to copy
      string[] theExtensions = { "*.jpg", "*.jpeg", "*.bmp", "*.png", "*.gif" };
      List<FileInfo> files = new List<FileInfo>();
      string path = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
      DirectoryInfo dir = new DirectoryInfo(path);
      long maxbytes = 0;
      foreach (string ext in theExtensions) {
        FileInfo[] folder = dir.GetFiles(ext, SearchOption.AllDirectories);
        foreach (FileInfo file in folder) {
          if ((file.Attributes & FileAttributes.Directory) != 0) continue;
          files.Add(file);
          maxbytes += file.Length;
        }
      }
      // Copy files
      long bytes = 0;
      foreach (FileInfo file in files) {
        try {
          this.BeginInvoke(OnChange, new object[] { new UIProgress(file.Name, bytes, maxbytes) });
          File.Copy(file.FullName, @"c:\temp\" + file.Name, true);
        }
        catch (Exception ex) {
          UIError err = new UIError(ex, file.FullName); 
          this.Invoke(OnError, new object[] { err });
          if (err.result == DialogResult.Cancel) break;
        }
        bytes += file.Length;
      }
    }
    private void Copier_ProgressChanged(UIProgress info) {
      // Update progress
      progressBar1.Value = (int)(100.0 * info.bytes / info.maxbytes);
      label1.Text = "Copying " + info.name;
    }
    private void Copier_Error(UIError err) {
      // Error handler
      string msg = string.Format("Error copying file {0}\n{1}\nClick OK to continue copying files", err.path, err.msg);
      err.result = MessageBox.Show(msg, "Copy error", MessageBoxButtons.OKCancel, MessageBoxIcon.Exclamation);
    }
    private void Copier_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e) {
      // Operation completed, update UI
      ChangeUI(false);
    }
    private void ChangeUI(bool docopy) {
      label1.Visible = docopy;
      progressBar1.Visible = docopy;
      button1.Text = docopy ? "Cancel" : "Copy";
      label1.Text = "Starting copy...";
      progressBar1.Value = 0;
    }
    private void button1_Click(object sender, EventArgs e) {
      bool docopy = button1.Text == "Copy";
      ChangeUI(docopy);
      if (docopy) mCopier.RunWorkerAsync();
      else mCopier.CancelAsync();
    }
  }
}
*/
