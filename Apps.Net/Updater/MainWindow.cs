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
using System.ComponentModel;
using System.Diagnostics;
using System.Data;
using System.Drawing;
using System.IO;
//using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.Xml.Serialization;

using Microsoft.Win32.SafeHandles;

using DevSupport;
using DevSupport.Api;
using DevSupport.DeviceManager;
using DevSupport.Media;

namespace Updater
{
    public partial class MainWindow : Form
    {
        private enum DeviceState { Unknown, RecoveryMode, UpdaterMode, UserMode_Mtp, UserMode_Msc, UserMode, NotConnected };
        private enum UpdateAction { NotStarted, ResetToRecovery, LoadUpdaterFile, WriteMedia, FinalInit, ResetToUser, Complete };
        
        private Device CurrentDevice = null;
        private DeviceState CurrentDeviceState = DeviceState.Unknown;
        private UpdateAction NextUpdateAction = UpdateAction.NotStarted;
        private struct UsbPortId
        {
            public int Hub;
            public int Port;
            public UsbPortId(int hub, int port)
            {
                Hub = hub;
                Port = port;
            }
        }
        private UsbPortId CurrentPort;
        private Int32 SavedLogWindowHeight; 

        // Need to get this from the Settings object. 
        // Just putting it here till I figure out how to put it in the application settings.
        private LogicalDrive[] DriveArray; /* = 
            {
                new LogicalDrive("firmware.sb", "Firmware Image 1", LogicalDrive.Type.System, LogicalDrive.Tag.FirmwareImg, 0, 0, LogicalDrive.Flags.ImageData),
                new LogicalDrive(null, "Data Drive", LogicalDrive.Type.Data, LogicalDrive.Tag.Data, 0, 0, LogicalDrive.Flags.Format),
                new LogicalDrive(null, "Janus Drive", LogicalDrive.Type.HiddenData, LogicalDrive.Tag.DataJanus, 0, 0, LogicalDrive.Flags.JanusInit),
                new LogicalDrive(null, "Settings Drive", LogicalDrive.Type.HiddenData, LogicalDrive.Tag.DataSettings, 0, 0, LogicalDrive.Flags.None),
                new LogicalDrive("firmware.sb", "Firmware Image 2", LogicalDrive.Type.System, LogicalDrive.Tag.FirmwareImg2, 0, 0, LogicalDrive.Flags.ImageData),
                new LogicalDrive("firmware.sb", "Firmware Image 3", LogicalDrive.Type.System, LogicalDrive.Tag.FirmwareImg3, 0, 0, LogicalDrive.Flags.ImageData),
            };
*/
        /// <summary>
        /// This is the handler for the CancelAutoPlay notifications. We don't need to do anything here.
        /// </summary>
        /// <param name="args"></param>
        private void DeviceManager_CancelAutoPlayNotify(Object args) {}

        public MainWindow()
        {
            InitializeComponent();
            this.Icon = Properties.Resources.Application;
            if (!Properties.Settings.Default.ShowLog)
            {
                ShowLogButton_Click(null, null);
            }
        }

        protected override void WndProc(ref Message m)
        {
            if (m.Msg == Win32.WM_SYSCOMMAND)
            {
                switch (m.WParam.ToInt32())
                {
                    case Win32.IDM_ABOUT:
                        AboutBox aboutBox = new AboutBox();
                        aboutBox.ShowDialog(this);
                        return;
                    default:
                        break;
                }
            }

            base.WndProc(ref m);
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            Thread.CurrentThread.Name = "MainWindow UI Thread";
            Trace.WriteLine(String.Format("*** MainWindow.Load(): {0}({1})", Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

            // Add "About..." menu item to system menu.
            IntPtr sysMenuHandle = Win32.GetSystemMenu(this.Handle, false);
            Win32.AppendMenu(sysMenuHandle, Win32.MF_SEPARATOR, 0, string.Empty);
            Win32.AppendMenu(sysMenuHandle, Win32.MF_STRING, Win32.IDM_ABOUT, "About " + this.Text + "...");

            // Get the DriveArray from the app.config file.
            DriveArray = Properties.Settings.Default.DriveArray;

            XmlSerializer xmlr = new XmlSerializer(typeof(LogicalDrive[]));
            FileStream fs = new FileStream("test.xml", FileMode.OpenOrCreate);
            xmlr.Serialize(fs, DriveArray);
            fs.Close();

            // Create an instance of the DeviceManager and register for device change notifications.
            DeviceManager.Instance.DeviceChanged += new DeviceManager.DeviceChangedEventHandler(DeviceManager_DeviceChanged);
            // Turn off AutoPlay to keep those pesky windows from popping up while we are updating.
            DeviceManager.Instance.CancelAutoPlay += new CancelAutoPlayEventHandler(DeviceManager_CancelAutoPlayNotify);

            // Put the ugrade firmware version in the UI.
            try
            {
                // Init the actual firmware file sizes for each file in the Drive Array.
                LogicalDrive.InitFileSizes(ref DriveArray);
                
                FirmwareInfo firmwareInfo = FirmwareInfo.FromDriveArray(DriveArray, LogicalDrive.Tag.FirmwareImg);
                UpgradeVersionTextBox.Text = firmwareInfo.ProjectVersion.ToString("0.00");
            }
            catch (Exception err)
            {
                DialogResult result = MessageBox.Show(this, err.Message);
                Application.Exit();
            }

            // See if our device is connected.
            CurrentDevice = FindDevice(DeviceState.Unknown);
            UpdateStatus();
        }


        private Device FindDevice(DeviceState deviceState)
        {
            Device tempDevice = null;

            switch (deviceState)
            {
                case DeviceState.RecoveryMode:
                    
                    tempDevice = DeviceManager.Instance.FindDevice(typeof(HidDeviceClass), 
                        Properties.Settings.Default.RecoveryUsbVid, 
                        null /*Properties.Settings.Default.RecoveryUsbPid*/);

                    if (tempDevice == null)
                    {
                        tempDevice = DeviceManager.Instance.FindDevice(typeof(RecoveryDeviceClass),
                            Properties.Settings.Default.RecoveryUsbVid,
                            null /*Properties.Settings.Default.RecoveryUsbPid*/);
                    }
                    
                    if (tempDevice != null)
                    {
                        CurrentDeviceState = DeviceState.RecoveryMode;
                        CurrentPort = new UsbPortId(tempDevice.UsbHub.Index, tempDevice.UsbPort);
                    }
                    break;

                case DeviceState.UpdaterMode:
                    
                    tempDevice = DeviceManager.Instance.FindDevice(typeof(VolumeDeviceClass), 
                        Properties.Settings.Default.UpdaterUsbVid, 
                        Properties.Settings.Default.UpdaterUsbPid);

                    if (tempDevice != null)
                    {
                        CurrentDeviceState = DeviceState.UpdaterMode;
                        CurrentPort = new UsbPortId(tempDevice.UsbHub.Index, tempDevice.UsbPort);
                    }
                    break;

                case DeviceState.UserMode:
                    
                    tempDevice = FindDevice(DeviceState.UserMode_Mtp);

                    if (tempDevice == null)
                    {
                        tempDevice = FindDevice(DeviceState.UserMode_Msc);
                    }

                    break;

                case DeviceState.UserMode_Mtp:

                    tempDevice = DeviceManager.Instance.FindDevice(typeof(WpdDeviceClass), 
                        Properties.Settings.Default.MtpUsbVid, 
                        Properties.Settings.Default.MtpUsbPid);
                    
                    if (tempDevice != null)
                    {
                        CurrentDeviceState = DeviceState.UserMode_Mtp;
                        CurrentPort = new UsbPortId(tempDevice.UsbHub.Index, tempDevice.UsbPort);
                    }
                    break;

                case DeviceState.UserMode_Msc:

                    tempDevice = DeviceManager.Instance.FindDevice(typeof(VolumeDeviceClass),
                        Properties.Settings.Default.MscUsbVid,
                        Properties.Settings.Default.MscUsbPid);
                    
                    if (tempDevice != null)
                    {
                        CurrentDeviceState = DeviceState.UserMode_Msc;
                        CurrentPort = new UsbPortId(tempDevice.UsbHub.Index, tempDevice.UsbPort);
                    }
                    break;

                case DeviceState.Unknown:

                    tempDevice = FindDevice(DeviceState.UserMode);

                    if (tempDevice == null)
                    {
                        tempDevice = FindDevice(DeviceState.UpdaterMode);
                    }
                    if (tempDevice == null)
                    {
                        tempDevice = FindDevice(DeviceState.RecoveryMode);
                    }
                    break;
            }

            if (tempDevice == null)
            {
                CurrentDeviceState = DeviceState.NotConnected;
                CurrentPort = new UsbPortId();
//                if (deviceState != DeviceState.Unknown && deviceState != DeviceState.UserMode)
//                    OutputWindow.Text += String.Format("Specified device is not connected in {0}.\n", deviceState);
            }
//            else
//            {
//                if (deviceState != DeviceState.Unknown && deviceState != DeviceState.UserMode)
//                    OutputWindow.Text += String.Format("Device is connected in {0}.\n", CurrentDeviceState);
//            }
            
            return tempDevice;
        }

        private void DoUpdateProcess(UpdateAction action)
        {
            Boolean success = true;

            switch (action)
            {
                case UpdateAction.ResetToRecovery:
                    StatusTextBox.Text = "Putting device into Recovery-mode...";
                    OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    success = ResetToRecovery();
                    if (success)
                    {
                        StatusTextBox.Text = "Waiting for Recovery-mode device...";
                        OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    }
                    break;
                case UpdateAction.LoadUpdaterFile:
                    StatusTextBox.Text = "Loading updater.sb...";
                    OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    success = LoadUpdaterFile();
                    if (success)
                    {
                        StatusTextBox.Text = "Waiting for MSC Updater-mode device...";
                        OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    }
                    break;
                case UpdateAction.WriteMedia:
                    StatusTextBox.Text = "Writing firmware to the media...";
                    OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    success = WriteMedia2();
                    if (success)
                    {
                        StatusTextBox.Text = "Waiting for MSC Updater-mode device...";
                        OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    }
                    break;
                case UpdateAction.FinalInit:
                    StatusTextBox.Text = "Doing final initializations...";
                    OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    success = FinalInit();
                    if (success)
                    {
                        StatusTextBox.Text = "Waiting for MTP/MSC User-mode device...";
                        OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    }
                    break;
                case UpdateAction.Complete:
                    StatusTextBox.Text = "Update Complete.";
                    OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
                    UpdateStatus();
                    break;
            }

            if (!success)
            {
                CloseButton.Enabled = true;
                StatusTextBox.Text = "Update Failed.";
                OutputWindow.Text += "*" + StatusTextBox.Text + "\r\n";
            }
        }

        private Boolean ResetToRecovery()
        {
            OutputWindow.Text += "Sending ResetToRecovery command.\r\n";

            if (CurrentDevice is IResetToRecovery)
            {
                if (((IResetToRecovery)CurrentDevice).ResetToRecovery() != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += CurrentDevice.ErrorString + "\r\n";
                    return false;
                }
            }
            else
            {
                OutputWindow.Text += String.Format(" ERROR: Device \"{0}\" does not support ResetToRecovery command.\r\n", CurrentDevice.ToString());
                return false;
            }

            return true;
        }

        private Boolean LoadUpdaterFile()
        {
            Boolean retValue = true;

            String updaterFilename = Properties.Settings.Default.UpdaterFilename;
            OutputWindow.Text += String.Format("Loading {0}.\r\n", updaterFilename);

            if (CurrentDevice is IRecoverable)
            {
                // register for the progress events
                CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);
                Device.SendCommandProgressHandler callback = new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

                if (((IRecoverable)CurrentDevice).LoadRecoveryDevice(updaterFilename/*, callback*/) != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += CurrentDevice.ErrorString + "\r\n";
                    retValue = false;
                }
            }
            else
            {
                OutputWindow.Text += String.Format(" ERROR: Device \"{0}\" does not support the LoadRecoveryDevice() funtion.\r\n", CurrentDevice.ToString());
                retValue = false;
            }

            // unregister for the progress events
            CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

            return retValue;
        }

        private Boolean WriteMedia()
        {
            Boolean retValue = false;
            Int32 result = 0;
            ScsiVendorApi.GetAllocationTable apiGetAllocationTable = null;

            //
            // Start process (level 0)
            //
            OutputWindow.Text += "Beginning the WriteMedia process.\r\n";

            Volume volume = CurrentDevice as Volume;
            if (volume == null)
            {
                OutputWindow.Text += String.Format("  ERROR: Device \"{0}\" does not support IUpdater interface.\r\n", CurrentDevice.ToString());
                return false;
            }

            //
            // Lock volume (level 1)
            //
            OutputWindow.Text += String.Format(" Locking the volume. LockType: {0}.", Volume.LockType.Logical);
            SafeFileHandle hVolume = volume.Lock(Volume.LockType.Logical);
            if (hVolume.IsInvalid)
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                return false;
            }
            OutputWindow.Text += " - PASS\r\n";

            // register for the progress events
            CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

            // If we are not planning on erasing the media, make sure the new DriveArray will fit
            // into the current media allocation.
            if (EraseMediaChekBox.Checked == false)
            {
                //
                // Check allocation (level 1)
                //
                ActionDetailsTextBox.Text = "Checking the media allocation...";
                OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
                OutputWindow.Text += " Checking existing media allocation is compatible with the new drive array.\r\n";
                String errString = String.Empty;
                Boolean isCompatible = false;

                // Get the existing media allocation. (level 2)
                OutputWindow.Text += "  Sending GetAllocationTable command.";
                apiGetAllocationTable = new ScsiVendorApi.GetAllocationTable();
                result = volume.SendCommand(hVolume, apiGetAllocationTable);
                if (result != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                }
                else
                {
                    OutputWindow.Text += " - PASS\r\n";
                    // Got the table, see if it is compatible. (level 2)
                    OutputWindow.Text += "  Checking compatibliity.";
                    isCompatible = Volume.IsCompatible(apiGetAllocationTable.DriveEntryArray, DriveArray, out errString);
                    if (isCompatible)
                    {
                        OutputWindow.Text += " - PASS\r\n";
                    }
                    else
                    {
                        OutputWindow.Text += " - FAILED\r\n";
                        OutputWindow.Text += "   " + errString + "\r\n";
                    }
                }
                // If we didn't get the table or it is not compatible,
                // warn the user that all data will be erased.
                if (!isCompatible || result != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += "  WARNING: Existing media allocation is not compatible with the new firmware size and/or layout.\r\n";
                    DialogResult dialogResult = MessageBox.Show("The media must be erased in order to upgrade your firmware.\r\n\r\nPressing 'Cancel' will stop the update application and allow you to back up your data.\r\n\r\nPressing 'OK' will allow the updater application to continue, but will erase all the data on your player.", "Erase Media", MessageBoxButtons.OKCancel);
                    if (dialogResult == DialogResult.OK)
                    {
                        OutputWindow.Text += "  <User clicked the \"OK\" button to allow erasing the media.>\r\n";
                        EraseMediaChekBox.Checked = true;
                        PreserveJanusCheckBox.Checked = false;
                    }
                    else
                    {
                        OutputWindow.Text += " <User clicked the \"Cancel\" button to stop the update process.>\r\n";
                        StatusTextBox.Text = "Please back-up your data and re-run the application.";
                        goto WriteMediaExit;
                    }
                }
            }

            if (EraseMediaChekBox.Checked == true)
            {
                //
                // TODO: Save stuff
                //
                
                //
                // Erase the media. (level 1)
                //
                ActionDetailsTextBox.Text = "Erasing the media...";
                OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
                OutputWindow.Text += String.Format(" Sending EraseLogicalMedia command. PreserveJanus = {0}.", PreserveJanusCheckBox.Checked);
                ScsiVendorApi.EraseLogicalMedia apiEraseMedia = new ScsiVendorApi.EraseLogicalMedia(PreserveJanusCheckBox.Checked);
                result = volume.SendCommand(hVolume, apiEraseMedia);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "  " + volume.ErrorString + "\r\n";
                    goto WriteMediaExit;
                }

                //
                // Allocate the media. (level 1)
                //
                ActionDetailsTextBox.Text = "Allocating the media...";
                OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
                OutputWindow.Text += "  SetAllocationTable command parameters:\r\n";
                ScsiVendorApi.SetAllocationTable apiAllocateMedia =
                    new ScsiVendorApi.SetAllocationTable(LogicalDrive.ToAllocationCmdEntryArray(DriveArray));
                for (int i = 0; i < apiAllocateMedia.DriveEntryArray.Length; ++i )
                {
                    OutputWindow.Text += String.Format("   entry[{0}]: {1}\r\n", i, apiAllocateMedia.DriveEntryArray[i].ToString());
                }
                OutputWindow.Text += "  Sending SetAllocationTable command.";
                result = volume.SendCommand(hVolume, apiAllocateMedia);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    goto WriteMediaExit;
                }
                OutputWindow.Text += "  Sending GetAllocationTable command.";
                apiGetAllocationTable = new ScsiVendorApi.GetAllocationTable();
                result = volume.SendCommand(hVolume, apiGetAllocationTable);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    goto WriteMediaExit;
                }

                //
                // Write the Janus Header if necessary. (level 1)
                //
                if (PreserveJanusCheckBox.Checked == false)
                {
                    ActionDetailsTextBox.Text = " Writing the Janus header info...";
                    OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";

                    // Get the size in sectors of the Janus drive
                    Byte driveNumber = apiGetAllocationTable.GetEntry(LogicalDrive.Tag.DataJanus).DriveNumber;
                    OutputWindow.Text += String.Format("  Sending GetLogicalDriveInfo({0}, {1}) command.", driveNumber, ScsiVendorApi.LogicalDriveInfo.SizeInSectors);
                    ScsiVendorApi.GetLogicalDriveInfo apiDriveInfo = new ScsiVendorApi.GetLogicalDriveInfo(
                            driveNumber, ScsiVendorApi.LogicalDriveInfo.SizeInSectors, 0);
                    result = volume.SendCommand(hVolume, apiDriveInfo);
                    if (result == Win32.ERROR_SUCCESS)
                    {
                        OutputWindow.Text += " - PASS\r\n";
                    }
                    else
                    {
                        OutputWindow.Text += " - FAILED\r\n";
                        OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                        goto WriteMediaExit;
                    }
                    UInt32 janusSectors = Convert.ToUInt32(apiDriveInfo.SizeInSectors);
                    Byte[] janusHeaderData = new JanusDriveHeader(janusSectors).ToArray();

                    result = WriteDrive(volume, hVolume, apiGetAllocationTable.GetEntry(LogicalDrive.Tag.DataJanus), janusHeaderData, "  ");
                    if (result != Win32.ERROR_SUCCESS)
                    {
                        goto WriteMediaExit;
                    }
                }

                //
                // Format the Data drive (level 1)
                //
                ActionDetailsTextBox.Text = " Formatting the Data drive...";
                OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
                
                // Get the size of the data drive
                OutputWindow.Text += "  Sending ReadCapacity() command.";
                ScsiFormalApi.ReadCapacity apiReadCapacity = new ScsiFormalApi.ReadCapacity();
                result = volume.SendCommand(hVolume, apiReadCapacity);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    goto WriteMediaExit;
                }
                UInt32 sectors = apiReadCapacity.LogicalBlockAddress + 1;
                UInt32 sectorSize = apiReadCapacity.BytesPerBlock;

                // Get the FormatInfo
                DevSupport.FormatBuilder.FormatInfo formatInfo =
                    new DevSupport.FormatBuilder.FormatInfo(sectors, (UInt16)sectorSize,
                        DevSupport.FormatBuilder.FormatInfo.FileSystemType.Default,
                        Properties.Settings.Default.VolumeLabel, false);

                // Create the Format Image
                DevSupport.FormatBuilder.FormatImage formatImage = 
                    new DevSupport.FormatBuilder.FormatImage(formatInfo);

                result = WriteDrive(volume, hVolume, apiGetAllocationTable.GetEntry(LogicalDrive.Tag.Data),
                    formatImage.TheImage, "  ");

//                result = volume.FormatDataDrive(LogicalDrive.Tag.Data,
//                    DevSupport.FormatBuilder.FormatInfo.FileSystemType.Default,
//                    Properties.Settings.Default.VolumeLabel, false);
                if (result != Win32.ERROR_SUCCESS)
                {
                    goto WriteMediaExit;
                }

            } // if (EraseMedia)

            //
            // Write the firmware drives
            //
            foreach (LogicalDrive drive in DriveArray)
            {
                if (drive.DriveType == LogicalDrive.Type.System)
                {
                    ActionDetailsTextBox.Text = String.Format("Writing {0}...", drive.Description);
                    OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";

                    //
                    // Get the firmware file data
                    //
                    Byte[] data = null;
                    FirmwareInfo firmwareInfo = null;
                    
                    try
                    {
                        firmwareInfo = FirmwareInfo.FromFile(drive.Filename);
                        if (firmwareInfo.Type == DevSupport.Media.FirmwareInfo.FileType.Stmp37xx)
                        {
                            data = File.ReadAllBytes(drive.Filename);
                        }
                        else
                        {
                            OutputWindow.Text += " - FAILED\r\n";
                            OutputWindow.Text += "  Invalid firmware file type.\r\n";
                            goto WriteMediaExit;
                        }
                    }
                    catch (Exception e)
                    {
                        OutputWindow.Text += " - FAILED\r\n";
                        OutputWindow.Text += "  " + e.Message + "\r\n";
                        goto WriteMediaExit;
                    }

                    ///
                    /// Write the data to the drive
                    ///
                    MediaAllocationEntry entry = apiGetAllocationTable.GetEntry(drive.DriveTag);
                    result = WriteDrive(volume, hVolume, entry, data, "  ");
                    if (result != Win32.ERROR_SUCCESS)
                    {
                        goto WriteMediaExit;
                    }

                    ///
                    /// Verify the data on the drive
                    ///

                    ActionDetailsTextBox.Text = String.Format("Verifying {0}...", drive.Description);
                    OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";

                    Byte[] driveData = ReadDrive(volume, hVolume, entry, data.Length, "  ");
                    Array.Resize(ref driveData, data.Length);

                    for (int i = 0; i < data.Length; ++i)
                    {
                        if (data[i] != driveData[i])
                        {
                            Utils.DecimalConverterEx decCvtr = new Utils.DecimalConverterEx();

                            Byte[] excerpt = new Byte[11];
                            int start = 5 - Math.Abs(i - 5);
                            int end = start + excerpt.Length - 1;

                            OutputWindow.Text += String.Format("   Error: Failed Verify({0}) at location {1}: media: {2} firmware {3}.\r\n", entry.Tag, i, decCvtr.ConvertToString(driveData[i]), decCvtr.ConvertToString(data[i]));
                            Array.Copy(driveData, start, excerpt, 0, excerpt.Length);
                            OutputWindow.Text += String.Format("   {0} : media[{1}-{2}]\r\n", Utils.Utils.StringFromHexBytes(excerpt, " "), start, end);
                            Array.Copy(data, start, excerpt, 0, excerpt.Length);
                            OutputWindow.Text += String.Format("   {0} : firmware[{1}-{2}]\r\n", Utils.Utils.StringFromHexBytes(excerpt, " "), start, end);
                            goto WriteMediaExit;
                        }
                    }

                } // if ( system drive )
            
            } // foreach ( drive in array )

            //
            // ResetToUpdater for Final Initializations
            //
            ActionDetailsTextBox.Text = "Resetting for final initialization...";
            OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
            OutputWindow.Text += " Sending ResetToUpdater command.";
            ScsiVendorApi.ResetToUpdater apiResetToUpdater = new ScsiVendorApi.ResetToUpdater();
            result = volume.SendCommand(hVolume, apiResetToUpdater);
            if (result == Win32.ERROR_SUCCESS)
            {
                OutputWindow.Text += " - PASS\r\n";
            }
            else
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += "  " + volume.ErrorString + "\r\n";
                goto WriteMediaExit;
            }

            retValue = true;
            ActionDetailsTextBox.Text = String.Empty;

        WriteMediaExit:

            // unregister for the progress events
            CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

            // only unlock the volume if there was an error. Otherwise, the volume should go
            // away and comeback which will automatically unlock it.
            if (!retValue)
            {
                // unlock the Volume
                OutputWindow.Text += " Unlocking the volume.";
                result = volume.Unlock(hVolume, true);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    retValue = false;
                }
            }
            else
            {
                OutputWindow.Text += " Not unlocking volume because it should be gone or going away.\r\n";
            }

            return retValue;
        }

        private Boolean WriteMedia2()
        {
            Boolean retValue = false;
            Int32 result = 0;
            ScsiVendorApi.GetAllocationTableEx apiGetAllocationTableEx = null;

            //
            // Start process (level 0)
            //
            OutputWindow.Text += "Beginning the WriteMedia process.\r\n";

            Volume volume = CurrentDevice as Volume;
            if (volume == null)
            {
                OutputWindow.Text += String.Format("  ERROR: Device \"{0}\" does not support IUpdater interface.\r\n", CurrentDevice.ToString());
                return false;
            }

            //
            // Lock volume (level 1)
            //
            OutputWindow.Text += String.Format(" Locking the volume. LockType: {0}.", Volume.LockType.Logical);
            SafeFileHandle hVolume = volume.Lock(Volume.LockType.Logical);
            if (hVolume.IsInvalid)
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                return false;
            }
            OutputWindow.Text += " - PASS\r\n";

            // register for the progress events
            CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

            // If we are not planning on erasing the media, make sure the new DriveArray will fit
            // into the current media allocation.
            if (EraseMediaChekBox.Checked == false)
            {
                //
                // Check allocation (level 1)
                //
                ActionDetailsTextBox.Text = "Checking the media allocation...";
                OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
                OutputWindow.Text += " Checking existing media allocation is compatible with the new drive array.\r\n";
                String errString = String.Empty;
                Boolean isCompatible = false;

                // Get the existing media allocation. (level 2)
                OutputWindow.Text += "  Sending GetAllocationTable command.";
                apiGetAllocationTableEx = new ScsiVendorApi.GetAllocationTableEx();
                result = volume.SendCommand(hVolume, apiGetAllocationTableEx);
                if (result != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                }
                else
                {
                    OutputWindow.Text += " - PASS\r\n";
                    // Got the table, see if it is compatible. (level 2)
                    OutputWindow.Text += "  Checking compatibliity.";
                    isCompatible = Volume.IsCompatible(apiGetAllocationTableEx.DriveEntryArray, DriveArray, out errString);
                    if (isCompatible)
                    {
                        OutputWindow.Text += " - PASS\r\n";
                    }
                    else
                    {
                        OutputWindow.Text += " - FAILED\r\n";
                        OutputWindow.Text += "   " + errString + "\r\n";
                    }
                }
                // If we didn't get the table or it is not compatible,
                // warn the user that all data will be erased.
                if (!isCompatible || result != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += "  WARNING: Existing media allocation is not compatible with the new firmware size and/or layout.\r\n";
                    DialogResult dialogResult = MessageBox.Show("The media must be erased in order to upgrade your firmware.\r\n\r\nPressing 'Cancel' will stop the update application and allow you to back up your data.\r\n\r\nPressing 'OK' will allow the updater application to continue, but will erase all the data on your player.", "Erase Media", MessageBoxButtons.OKCancel);
                    if (dialogResult == DialogResult.OK)
                    {
                        OutputWindow.Text += "  <User clicked the \"OK\" button to allow erasing the media.>\r\n";
                        EraseMediaChekBox.Checked = true;
                        PreserveJanusCheckBox.Checked = false;
                    }
                    else
                    {
                        OutputWindow.Text += " <User clicked the \"Cancel\" button to stop the update process.>\r\n";
                        StatusTextBox.Text = "Please back-up your data and re-run the application.";
                        goto WriteMediaExit;
                    }
                }
            }

            if (EraseMediaChekBox.Checked == true)
            {
                //
                // TODO: Save stuff
                //

                //
                // Erase the media. (level 1)
                //
                ActionDetailsTextBox.Text = "Erasing the media...";
                OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
                OutputWindow.Text += String.Format(" Sending EraseLogicalMedia command. PreserveJanus = {0}.", PreserveJanusCheckBox.Checked);
                ScsiVendorApi.EraseLogicalMedia apiEraseMedia = new ScsiVendorApi.EraseLogicalMedia(PreserveJanusCheckBox.Checked);
                result = volume.SendCommand(hVolume, apiEraseMedia);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "  " + volume.ErrorString + "\r\n";
                    goto WriteMediaExit;
                }

                //
                // Allocate the media. (level 1)
                //
                ActionDetailsTextBox.Text = "Allocating the media...";
                OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
                OutputWindow.Text += "  SetAllocationTable command parameters:\r\n";
                ScsiVendorApi.SetAllocationTable apiAllocateMedia =
                    new ScsiVendorApi.SetAllocationTable(LogicalDrive.ToAllocationCmdEntryArray(DriveArray));
                for (int i = 0; i < apiAllocateMedia.DriveEntryArray.Length; ++i)
                {
                    OutputWindow.Text += String.Format("   entry[{0}]: {1}\r\n", i, apiAllocateMedia.DriveEntryArray[i].ToString());
                }
                OutputWindow.Text += "  Sending SetAllocationTable command.";
                result = volume.SendCommand(hVolume, apiAllocateMedia);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    goto WriteMediaExit;
                }
                OutputWindow.Text += "  Sending GetAllocationTable command.";
                apiGetAllocationTableEx = new ScsiVendorApi.GetAllocationTableEx();
                result = volume.SendCommand(hVolume, apiGetAllocationTableEx);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    goto WriteMediaExit;
                }

                //
                // Write the Janus Header if necessary. (level 1)
                //
                if (PreserveJanusCheckBox.Checked == false)
                {
                    ActionDetailsTextBox.Text = " Writing the Janus header info...";
                    OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";

                    MediaAllocationEntryEx entry = apiGetAllocationTableEx.GetEntry(LogicalDrive.Tag.tJanus);
                    UInt32 janusSectors = Convert.ToUInt32(entry.Size / entry.SectorSize);
                    Byte[] janusHeaderData = new JanusDriveHeader(janusSectors).ToArray();

                    result = WriteDrive2(volume, hVolume, apiGetAllocationTableEx.GetEntry(LogicalDrive.Tag.tJanus), janusHeaderData, "  ");
                    if (result != Win32.ERROR_SUCCESS)
                    {
                        goto WriteMediaExit;
                    }
                }

                //
                // Format the Data drive (level 1)
                //
                ActionDetailsTextBox.Text = " Formatting the Data drive...";
                OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";

                // Get the size of the data drive
                OutputWindow.Text += "  Sending ReadCapacity() command.";
                ScsiFormalApi.ReadCapacity apiReadCapacity = new ScsiFormalApi.ReadCapacity();
                result = volume.SendCommand(hVolume, apiReadCapacity);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    goto WriteMediaExit;
                }
                UInt32 sectors = apiReadCapacity.LogicalBlockAddress + 1;
                UInt32 sectorSize = apiReadCapacity.BytesPerBlock;

                // Get the FormatInfo
                DevSupport.FormatBuilder.FormatInfo formatInfo =
                    new DevSupport.FormatBuilder.FormatInfo(sectors, (UInt16)sectorSize,
                        DevSupport.FormatBuilder.FormatInfo.FileSystemType.Default,
                        Properties.Settings.Default.VolumeLabel, true);

                // Create the Format Image
                DevSupport.FormatBuilder.FormatImage formatImage =
                    new DevSupport.FormatBuilder.FormatImage(formatInfo);

                result = WriteDrive2(volume, hVolume, apiGetAllocationTableEx.GetEntry(LogicalDrive.Tag.tData),
                    formatImage.TheImage, "  ");

                //                result = volume.FormatDataDrive(LogicalDrive.Tag.Data,
                //                    DevSupport.FormatBuilder.FormatInfo.FileSystemType.Default,
                //                    Properties.Settings.Default.VolumeLabel, false);
                if (result != Win32.ERROR_SUCCESS)
                {
                    goto WriteMediaExit;
                }

            } // if (EraseMedia)

            //
            // Write the firmware drives
            //
            foreach (LogicalDrive drive in DriveArray)
            {
                if (drive.DriveType == LogicalDrive.Type.System)
                {
                    ActionDetailsTextBox.Text = String.Format("Writing {0}...", drive.Description);
                    OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";

                    //
                    // Get the firmware file data
                    //
                    Byte[] data = null;
                    FirmwareInfo firmwareInfo = null;

                    try
                    {
                        firmwareInfo = FirmwareInfo.FromFile(drive.Filename);
                        if (firmwareInfo.Type == DevSupport.Media.FirmwareInfo.FileType.Stmp37xx)
                        {
                            data = File.ReadAllBytes(drive.Filename);
                        }
                        else
                        {
                            OutputWindow.Text += " - FAILED\r\n";
                            OutputWindow.Text += "  Invalid firmware file type.\r\n";
                            goto WriteMediaExit;
                        }
                    }
                    catch (Exception e)
                    {
                        OutputWindow.Text += " - FAILED\r\n";
                        OutputWindow.Text += "  " + e.Message + "\r\n";
                        goto WriteMediaExit;
                    }

                    ///
                    /// Write the data to the drive
                    ///
                    MediaAllocationEntryEx entry = apiGetAllocationTableEx.GetEntry(drive.DriveTag);
                    result = WriteDrive2(volume, hVolume, entry, data, "  ");
                    if (result != Win32.ERROR_SUCCESS)
                    {
                        goto WriteMediaExit;
                    }

                    ///
                    /// Verify the data on the drive
                    ///

                    ActionDetailsTextBox.Text = String.Format("Verifying {0}...", drive.Description);
                    OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";

                    Byte[] driveData = ReadDrive2(volume, hVolume, entry, data.Length, "  ");
                    Array.Resize(ref driveData, data.Length);

                    for (int i = 0; i < data.Length; ++i)
                    {
                        if (data[i] != driveData[i])
                        {
                            Utils.DecimalConverterEx decCvtr = new Utils.DecimalConverterEx();

                            Byte[] excerpt = new Byte[11];
                            int start = 5 - Math.Abs(i - 5);
                            int end = start + excerpt.Length - 1;

                            OutputWindow.Text += String.Format("   Error: Failed Verify({0}) at location {1}: media: {2} firmware {3}.\r\n", entry.Tag, i, decCvtr.ConvertToString(driveData[i]), decCvtr.ConvertToString(data[i]));
                            Array.Copy(driveData, start, excerpt, 0, excerpt.Length);
                            OutputWindow.Text += String.Format("   {0} : media[{1}-{2}]\r\n", Utils.Utils.StringFromHexBytes(excerpt, " "), start, end);
                            Array.Copy(data, start, excerpt, 0, excerpt.Length);
                            OutputWindow.Text += String.Format("   {0} : firmware[{1}-{2}]\r\n", Utils.Utils.StringFromHexBytes(excerpt, " "), start, end);
                            goto WriteMediaExit;
                        }
                    }

                } // if ( system drive )

            } // foreach ( drive in array )

            //
            // ResetToUpdater for Final Initializations
            //
            ActionDetailsTextBox.Text = "Resetting for final initialization...";
            OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
            OutputWindow.Text += " Sending ResetToUpdater command.";
            ScsiVendorApi.ResetToUpdater apiResetToUpdater = new ScsiVendorApi.ResetToUpdater();
            result = volume.SendCommand(hVolume, apiResetToUpdater);
            if (result == Win32.ERROR_SUCCESS)
            {
                OutputWindow.Text += " - PASS\r\n";
            }
            else
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += "  " + volume.ErrorString + "\r\n";
                goto WriteMediaExit;
            }

            retValue = true;
            ActionDetailsTextBox.Text = String.Empty;

        WriteMediaExit:

            // unregister for the progress events
            CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

            // only unlock the volume if there was an error. Otherwise, the volume should go
            // away and comeback which will automatically unlock it.
            if (!retValue)
            {
                // unlock the Volume
                OutputWindow.Text += " Unlocking the volume.";
                result = volume.Unlock(hVolume, true);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    retValue = false;
                }
            }
            else
            {
                OutputWindow.Text += " Not unlocking volume because it should be gone or going away.\r\n";
            }

            return retValue;
        }

        private Boolean FinalInit()
        {
            Boolean success = false;

            OutputWindow.Text += "Beginning the FinalInit process.\r\n";

            Volume volume = CurrentDevice as Volume;
            if (volume == null)
            {
                OutputWindow.Text += String.Format(" ERROR: Device \"{0}\" does not support IUpdater interface.\r\n", CurrentDevice.ToString());
                return false;
            }

            OutputWindow.Text += " Locking the volume. LockType = " + Volume.LockType.Logical.ToString();
            SafeFileHandle hVolume = volume.Lock(Volume.LockType.Logical);
            if (hVolume.IsInvalid)
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                return false;
            }
            OutputWindow.Text += " - PASS\r\n";

            //
            // Janus Init
            //
            ActionDetailsTextBox.Text = "Initializing DRM...";
            OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
            OutputWindow.Text += " Sending InitializeJanus command.";

            ScsiVendorApi.InitializeJanus apiJanusInit = new ScsiVendorApi.InitializeJanus();

            Int32 result = volume.SendCommand(hVolume, apiJanusInit);

            if (result == Win32.ERROR_SUCCESS)
            {
                OutputWindow.Text += " - PASS\r\n";
            }
            else
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                goto FinalInitExit;
            }

            //
            // Store Init
            //
            ActionDetailsTextBox.Text = "Initializing Data Store...";
            OutputWindow.Text += " *" + ActionDetailsTextBox.Text + "\r\n";
            OutputWindow.Text += " Sending InitializeDataStore command.";

            ScsiVendorApi.InitializeDataStore apiStoreInit = new ScsiVendorApi.InitializeDataStore(0);

            result = volume.SendCommand(hVolume, apiStoreInit);

            if (result == Win32.ERROR_SUCCESS)
            {
                OutputWindow.Text += " - PASS\r\n";
            }
            else
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                goto FinalInitExit;
            }

            //
            // TODO: Restore stuff?
            //

            //
            // Reset back to User-mode
            //
            ActionDetailsTextBox.Text = String.Empty;
            OutputWindow.Text += " Sending ChipReset command.";

            ScsiVendorApi.ChipReset apiChipReset = new ScsiVendorApi.ChipReset();

            result = volume.SendCommand(hVolume, apiChipReset);

            if (result == Win32.ERROR_SUCCESS)
            {
                OutputWindow.Text += " - PASS\r\n";
            }
            else
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                goto FinalInitExit;
            }

            success = true;

        FinalInitExit:

            // only unlock the volume if there was an error. Otherwise, the volume should go
            // away and comeback which will automatically unlock it.
            if (!success)
            {
                OutputWindow.Text += " Unlocking the volume.";
                result = volume.Unlock(hVolume, true);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += "   " + volume.ErrorString + "\r\n";
                    success = false;
                }
            }
            else
            {
                OutputWindow.Text += " Not unlocking volume because it should be gone or going away.\r\n";
            }

            return success;
        }

        public Int32 WriteDrive(Volume volume, SafeFileHandle hVolume, MediaAllocationEntry driveInfo, Byte[] data, String indent)
        {
            Int32 result = Win32.ERROR_SUCCESS;

            OutputWindow.Text += String.Format("{0}Writing drive {1}...\r\n", indent, driveInfo.Tag);

            // Get the sector size
            OutputWindow.Text += String.Format("{0} Sending GetLogicalDriveInfo({1}, {2}) command.", indent, driveInfo.DriveNumber, ScsiVendorApi.LogicalDriveInfo.SectorSizeInBytes);
            ScsiVendorApi.GetLogicalDriveInfo apiDriveInfo =
                new ScsiVendorApi.GetLogicalDriveInfo(driveInfo.DriveNumber, ScsiVendorApi.LogicalDriveInfo.SectorSizeInBytes, 0);
            result = volume.SendCommand(hVolume, apiDriveInfo);
            if (result == Win32.ERROR_SUCCESS)
            {
                OutputWindow.Text += " - PASS\r\n";
            }
            else
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += indent + "  " + volume.ErrorString + "\r\n";
                return result;
            }
            UInt32 sectorSize = apiDriveInfo.SectorSize;
            OutputWindow.Text += String.Format("{0}  Sector size = {1}.\r\n", indent, sectorSize);

            // Check the data will fit on the drive
            Utils.ByteFormatConverter byteCvtr = new Utils.ByteFormatConverter();
            if (driveInfo.SizeInBytes < data.Length)
            {
                result = DevSupport.Win32.ERROR_DISK_FULL;
                OutputWindow.Text += String.Format("{0} ERROR: Data will NOT fit on drive {1}.\r\n", indent, driveInfo.Tag);
                OutputWindow.Text += String.Format("{0}  Media size: {1} < Data size: {2}.\r\n", indent,
                            byteCvtr.ConvertToString(driveInfo.SizeInBytes),
                            byteCvtr.ConvertToString(data.Length));
                return result;
            }
            else
            {
                OutputWindow.Text += String.Format("{0} Size Check - Data will fit on drive {1}.\r\n", indent, driveInfo.Tag);
                OutputWindow.Text += String.Format("{0}  Media size: {1} >= Data size: {2}.\r\n", indent,
                            byteCvtr.ConvertToString(driveInfo.SizeInBytes),
                            byteCvtr.ConvertToString(data.Length));
            }

            //
            // Erase the drive.
            //
            if (EraseMediaChekBox.Checked == false)
            {
                OutputWindow.Text += String.Format("{0} Sending EraseLogicalDrive({1}) command.", indent, driveInfo.DriveNumber);
                ScsiVendorApi.EraseLogicalDrive apiErase = new ScsiVendorApi.EraseLogicalDrive(driveInfo.DriveNumber);
                result = volume.SendCommand(hVolume, apiErase);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += indent + "  " + volume.ErrorString + "\r\n";
                    return result;
                }
            }
            else
            {
                OutputWindow.Text += String.Format("{0} NOT Erasing {1} because Media should already be erased.\r\n", indent, driveInfo.Tag);
            }
            // Variables for iteration
            UInt32 sectorsPerWrite = Volume.MaxTransferSize / sectorSize;
            Byte[] buffer = new Byte[Volume.MaxTransferSize];

            //
            // Write drive in chunks
            //
            OutputWindow.Text += String.Format("{0} Sending WriteLogicalDriveSector({1}) commands.\r\n", indent, driveInfo.DriveNumber);
            
            // tell the UI we are beginning a command.
            Device.SendCommandProgressArgs cmdProgress =
                new Device.SendCommandProgressArgs("WriteDrive", Api.CommandDirection.WriteWithData, (UInt32)data.Length + 4096);
            cmdProgress.Status = String.Format("{0}  WriteDrive.Begin", driveInfo.Tag);
            volume.DoSendProgress(cmdProgress);

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
                    new ScsiVendorApi.WriteLogicalDriveSector(driveInfo.DriveNumber, sectorSize,
                        byteIndex / sectorSize, sectorsToWrite, buffer);

                result = volume.SendCommand(hVolume, apiWrite);
                if (result != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += indent + "  Error:" + volume.ErrorString + "\r\n";
                    break;
                }
                // Update the UI
                cmdProgress.Position += (Int32)apiWrite.TransferSize;
                cmdProgress.Status = String.Format("{0}  WriteDrive() Position: {1}.", indent, cmdProgress.Position);
                volume.DoSendProgress(cmdProgress);
            }

            // tell the UI we are done
            cmdProgress.Position = data.Length;
            cmdProgress.Error = result;
            cmdProgress.InProgress = false;
            volume.DoSendProgress(cmdProgress);

            OutputWindow.Text += String.Format("{0} Writing drive {1}. - Done.\r\n", indent, driveInfo.Tag);

            return result;
        }

        public Int32 WriteDrive2(Volume volume, SafeFileHandle hVolume, MediaAllocationEntryEx driveInfo, Byte[] data, String indent)
        {
            Int32 result = Win32.ERROR_SUCCESS;

            OutputWindow.Text += String.Format("{0}Writing drive {1}...\r\n", indent, driveInfo.Tag);
            OutputWindow.Text += String.Format("{0}  Sector size = {1}.\r\n", indent, driveInfo.SectorSize);

            // Check the data will fit on the drive
            Utils.ByteFormatConverter byteCvtr = new Utils.ByteFormatConverter();
            if (driveInfo.Size < Convert.ToUInt64(data.Length))
            {
                result = DevSupport.Win32.ERROR_DISK_FULL;
                OutputWindow.Text += String.Format("{0} ERROR: Data will NOT fit on drive {1}.\r\n", indent, driveInfo.Tag);
                OutputWindow.Text += String.Format("{0}  Media size: {1} < Data size: {2}.\r\n", indent,
                            byteCvtr.ConvertToString(driveInfo.Size),
                            byteCvtr.ConvertToString(data.Length));
                return result;
            }
            else
            {
                OutputWindow.Text += String.Format("{0} Size Check - Data will fit on drive {1}.\r\n", indent, driveInfo.Tag);
                OutputWindow.Text += String.Format("{0}  Media size: {1} >= Data size: {2}.\r\n", indent,
                            byteCvtr.ConvertToString(driveInfo.Size),
                            byteCvtr.ConvertToString(data.Length));
            }

            //
            // Erase the drive.
            //
            if (EraseMediaChekBox.Checked == false)
            {
                OutputWindow.Text += String.Format("{0} Sending EraseLogicalDrive({1}) command.", indent, driveInfo.Tag);
                ScsiVendorApi.EraseLogicalDrive apiErase = new ScsiVendorApi.EraseLogicalDrive(driveInfo.Tag);
                result = volume.SendCommand(hVolume, apiErase);
                if (result == Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += " - PASS\r\n";
                }
                else
                {
                    OutputWindow.Text += " - FAILED\r\n";
                    OutputWindow.Text += indent + "  " + volume.ErrorString + "\r\n";
                    return result;
                }
            }
            else
            {
                OutputWindow.Text += String.Format("{0} NOT Erasing {1} because Media should already be erased.\r\n", indent, driveInfo.Tag);
            }
            // Variables for iteration
            UInt32 sectorsPerWrite = Volume.MaxTransferSize / driveInfo.SectorSize;
            Byte[] buffer = new Byte[Volume.MaxTransferSize];

            //
            // Write drive in chunks
            //
            OutputWindow.Text += String.Format("{0} Sending WriteLogicalDriveSector({1}) commands.\r\n", indent, driveInfo.Tag);

            // tell the UI we are beginning a command.
            Device.SendCommandProgressArgs cmdProgress =
                new Device.SendCommandProgressArgs("WriteDrive", Api.CommandDirection.WriteWithData, (UInt32)data.Length + 4096);
            cmdProgress.Status = String.Format("{0}  WriteDrive.Begin", driveInfo.Tag);
            volume.DoSendProgress(cmdProgress);

            UInt32 byteIndex, numBytesToWrite = 0;
            for (byteIndex = 0; byteIndex < data.Length; byteIndex += numBytesToWrite)
            {
                // Init the buffer to 0xFF
                for (int i = 0; i < buffer.Length; ++i)
                    buffer[i] = 0xFF;

                // Get some data
                numBytesToWrite = (UInt32)Math.Min(sectorsPerWrite * driveInfo.SectorSize, data.Length - byteIndex);
                Array.Copy(data, byteIndex, buffer, 0, numBytesToWrite);

                // Get bytes to write in terms of sectors
                UInt32 sectorsToWrite = numBytesToWrite / driveInfo.SectorSize;
                if ((numBytesToWrite % driveInfo.SectorSize) > 0)
                    ++sectorsToWrite;

                // Write the data to the device
                ScsiVendorApi.WriteLogicalDriveSector apiWrite =
                    new ScsiVendorApi.WriteLogicalDriveSector(driveInfo.Tag, driveInfo.SectorSize,
                        byteIndex / driveInfo.SectorSize, sectorsToWrite, buffer);

                result = volume.SendCommand(hVolume, apiWrite);
                if (result != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += indent + "  Error:" + volume.ErrorString + "\r\n";
                    break;
                }
                // Update the UI
                cmdProgress.Position += (Int32)apiWrite.TransferSize;
                cmdProgress.Status = String.Format("{0}  WriteDrive() Position: {1}.", indent, cmdProgress.Position);
                volume.DoSendProgress(cmdProgress);
            }

            // tell the UI we are done
            cmdProgress.Position = data.Length;
            cmdProgress.Error = result;
            cmdProgress.InProgress = false;
            volume.DoSendProgress(cmdProgress);

            OutputWindow.Text += String.Format("{0} Writing drive {1}. - Done.\r\n", indent, driveInfo.Tag);

            return result;
        }

        public Byte[] ReadDrive(Volume volume, SafeFileHandle hVolume, MediaAllocationEntry driveInfo, Int32 length, String indent)
        {
            OutputWindow.Text += String.Format("{0}Reading drive {1}...\r\n", indent, driveInfo.Tag);

            Byte[] data = null;
            if (length == 0)
                data = new Byte[driveInfo.SizeInBytes];
            else
                data = new Byte[length];

            // Get the sector size
            OutputWindow.Text += String.Format("{0} Sending GetLogicalDriveInfo({1}, {2}) command.", indent, driveInfo.DriveNumber, ScsiVendorApi.LogicalDriveInfo.SectorSizeInBytes);
            ScsiVendorApi.GetLogicalDriveInfo apiDriveInfo =
                new ScsiVendorApi.GetLogicalDriveInfo(driveInfo.DriveNumber, ScsiVendorApi.LogicalDriveInfo.SectorSizeInBytes, 0);
            Int32 result = volume.SendCommand(hVolume, apiDriveInfo);
            if (result == Win32.ERROR_SUCCESS)
            {
                OutputWindow.Text += " - PASS\r\n";
            }
            else
            {
                OutputWindow.Text += " - FAILED\r\n";
                OutputWindow.Text += indent + "  " + volume.ErrorString + "\r\n";
                return data;
            }
            UInt32 sectorSize = apiDriveInfo.SectorSize;
            OutputWindow.Text += String.Format("{0}  Sector size = {1}.\r\n", indent, sectorSize);

            // Variables for iteration
            UInt32 sectorsPerRead = Volume.MaxTransferSize / sectorSize;
            Byte[] buffer = new Byte[Volume.MaxTransferSize];

            //
            // Read drive in chunks
            //
            OutputWindow.Text += String.Format("{0} Sending ReadLogicalDriveSector({1}) commands.\r\n", indent, driveInfo.DriveNumber);

            // tell the UI we are beginning a command.
            Device.SendCommandProgressArgs cmdProgress =
                new Device.SendCommandProgressArgs("ReadDrive", Api.CommandDirection.ReadWithData, (UInt32)data.Length + 4096);
            cmdProgress.Status = String.Format("{0}  ReadDrive.Begin", driveInfo.Tag);
            volume.DoSendProgress(cmdProgress);

            UInt32 byteIndex, numBytesToRead = 0;
            for (byteIndex = 0; byteIndex < data.Length; byteIndex += numBytesToRead)
            {
                // Get bytes to write in terms of sectors
                numBytesToRead = (UInt32)Math.Min(sectorsPerRead * sectorSize, data.Length - byteIndex);
                UInt32 sectorsToRead = numBytesToRead / sectorSize;
                if ((numBytesToRead % sectorSize) > 0)
                    ++sectorsToRead;

                // Read the data from the device
                ScsiVendorApi.ReadLogicalDriveSector apiReadSector =
                    new ScsiVendorApi.ReadLogicalDriveSector(driveInfo.DriveNumber, sectorSize,
                        byteIndex / sectorSize, sectorsToRead);

                result = volume.SendCommand(hVolume, apiReadSector);
                if (result != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += indent + "  Error:" + volume.ErrorString + "\r\n";
                    break;
                }

                // Get the data
                Array.Copy(apiReadSector.Data, 0, data, byteIndex, numBytesToRead);

                // Update the UI
                cmdProgress.Position += (Int32)apiReadSector.TransferSize;
                cmdProgress.Status = String.Format("{0}  ReadDrive() Position: {1}.", indent, cmdProgress.Position);
                volume.DoSendProgress(cmdProgress);
            }

            // tell the UI we are done
            cmdProgress.Position = data.Length;
            cmdProgress.Error = result;
            cmdProgress.InProgress = false;
            volume.DoSendProgress(cmdProgress);

            OutputWindow.Text += String.Format("{0} Reading drive {1}. - Done.\r\n", indent, driveInfo.Tag);

            return data;
        }

        public Byte[] ReadDrive2(Volume volume, SafeFileHandle hVolume, MediaAllocationEntryEx driveInfo, Int32 length, String indent)
        {
            Int32 result = Win32.ERROR_SUCCESS;

            OutputWindow.Text += String.Format("{0}Reading drive {1}...\r\n", indent, driveInfo.Tag);
            OutputWindow.Text += String.Format("{0}  Sector size = {1}.\r\n", indent, driveInfo.SectorSize);

            Byte[] data = null;
            if (length == 0)
                data = new Byte[driveInfo.Size];
            else
                data = new Byte[length];

            // Variables for iteration
            UInt32 sectorsPerRead = Volume.MaxTransferSize / driveInfo.SectorSize;
            Byte[] buffer = new Byte[Volume.MaxTransferSize];

            //
            // Read drive in chunks
            //
            OutputWindow.Text += String.Format("{0} Sending ReadLogicalDriveSector({1}) commands.\r\n", indent, driveInfo.Tag);

            // tell the UI we are beginning a command.
            Device.SendCommandProgressArgs cmdProgress =
                new Device.SendCommandProgressArgs("ReadDrive", Api.CommandDirection.ReadWithData, (UInt32)data.Length + 4096);
            cmdProgress.Status = String.Format("{0}  ReadDrive.Begin", driveInfo.Tag);
            volume.DoSendProgress(cmdProgress);

            UInt32 byteIndex, numBytesToRead = 0;
            for (byteIndex = 0; byteIndex < data.Length; byteIndex += numBytesToRead)
            {
                // Get bytes to write in terms of sectors
                numBytesToRead = (UInt32)Math.Min(sectorsPerRead * driveInfo.SectorSize, data.Length - byteIndex);
                UInt32 sectorsToRead = numBytesToRead / driveInfo.SectorSize;
                if ((numBytesToRead % driveInfo.SectorSize) > 0)
                    ++sectorsToRead;

                // Read the data from the device
                ScsiVendorApi.ReadLogicalDriveSector apiReadSector =
                    new ScsiVendorApi.ReadLogicalDriveSector(driveInfo.Tag, driveInfo.SectorSize,
                        byteIndex / driveInfo.SectorSize, sectorsToRead);

                result = volume.SendCommand(hVolume, apiReadSector);
                if (result != Win32.ERROR_SUCCESS)
                {
                    OutputWindow.Text += indent + "  Error:" + volume.ErrorString + "\r\n";
                    break;
                }

                // Get the data
                Array.Copy(apiReadSector.Data, 0, data, byteIndex, numBytesToRead);

                // Update the UI
                cmdProgress.Position += (Int32)apiReadSector.TransferSize;
                cmdProgress.Status = String.Format("{0}  ReadDrive() Position: {1}.", indent, cmdProgress.Position);
                volume.DoSendProgress(cmdProgress);
            }

            // tell the UI we are done
            cmdProgress.Position = data.Length;
            cmdProgress.Error = result;
            cmdProgress.InProgress = false;
            volume.DoSendProgress(cmdProgress);

            OutputWindow.Text += String.Format("{0} Reading drive {1}. - Done.\r\n", indent, driveInfo.Tag);

            return data;
        }

        private void MainWindow_FormClosing(object sender, FormClosingEventArgs e)
        {
            DeviceManager.Instance.Dispose();
        }

        private void CloseButton_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void UpdateButton_Click(object sender, EventArgs e)
        {
            OutputWindow.Text += "<User clicked \"Update\" button.>\r\n";

            if (CurrentDevice is HidDevice || CurrentDevice is RecoveryDevice)
                NextUpdateAction = UpdateAction.LoadUpdaterFile;
            else if (CurrentDeviceState == DeviceState.UpdaterMode)
                NextUpdateAction = UpdateAction.WriteMedia;
            else
                NextUpdateAction = UpdateAction.ResetToRecovery;

            UpdateStatus();

            DoUpdateProcess(NextUpdateAction);
        }

        void CurrentDevice_SendCommandProgress(object sender, Device.SendCommandProgressArgs args)

        {
            if (args.InProgress)
            {
                ProgressBar.Maximum = (int)args.Maximum;
                ProgressBar.Value = Convert.ToInt32(args.Position);
//                ActionDetailsTextBox.Text = args.Name;
                Utils.DecimalConverterEx decConverter = new Utils.DecimalConverterEx();
//                OutputWindow.Text += String.Format("*Processing {0}. Position = {1}.\r\n", args.Name, decConverter.ConvertToString(args.Position));
                OutputWindow.Text += "   " + args.Status + "\r\n";
            }
            else
            {
                OutputWindow.Text += "   nothing to add.\r\n";
                ProgressBar.Value = 0;
            }
        }

        void DeviceManager_DeviceChanged(DeviceChangedEventArgs e)
        {
            Trace.WriteLine(String.Format("*** MainWindow.DeviceManager_DeviceChanged(): {0}: {1}, {2}({3})", e.Event, e.DeviceId, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

            // If we know what port we are working with, 
            // dismiss all device changes that are not on our port.
            if (CurrentPort.Hub != 0 && CurrentPort.Port != 0)
            {
                if ((e.Hub != CurrentPort.Hub) ||
                     (e.Port != CurrentPort.Port))
                {
                    return;
                }
            }
            OutputWindow.Text += "<" + e.Event + ": " + e.DeviceId + ">\r\n";

            switch (NextUpdateAction)
            {
                case UpdateAction.NotStarted:
                {
                    if (CurrentDeviceState == DeviceState.NotConnected)
                    {
                        switch (e.Event)
                        {
                            case DeviceChangeEvent.DeviceArrival:

                                CurrentDevice = FindDevice(DeviceState.RecoveryMode);

                                if (CurrentDevice == null)
                                    CurrentDevice = FindDevice(DeviceState.UserMode_Mtp);

                                break;

                            case DeviceChangeEvent.VolumeArrival:

                                CurrentDevice = FindDevice(DeviceState.UserMode_Msc);

                                if (CurrentDevice == null)
                                    CurrentDevice = FindDevice(DeviceState.UpdaterMode);

                                break;
                        }
                    }
                    else
                    {
                        if (e.Event == DeviceChangeEvent.DeviceRemoval || e.Event == DeviceChangeEvent.VolumeRemoval)
                        {
                            CurrentDevice = null;
                            CurrentDeviceState = DeviceState.NotConnected;
                        }
                    }

                    UpdateStatus();

                    break;
                }
                case UpdateAction.ResetToRecovery:
                {
                    if (e.Event == DeviceChangeEvent.DeviceArrival)
                    {
                        CurrentDevice = FindDevice(DeviceState.RecoveryMode);
                        if (CurrentDevice != null)
                        {
                            OutputWindow.Text += String.Format("Device is connected in {0} on USB Hub {1} - Port {2}.\r\n",
                                CurrentDeviceState, CurrentDevice.UsbHub.Index, CurrentDevice.UsbPort);
                            NextUpdateAction = UpdateAction.LoadUpdaterFile;
                            DoUpdateProcess(NextUpdateAction);
                        }
                        else
                        {
                            // error
                        }
                    }

                    break;
                }
                case UpdateAction.LoadUpdaterFile:
                {
                    if (e.Event == DeviceChangeEvent.VolumeArrival)
                    {
                        CurrentDevice = FindDevice(DeviceState.UpdaterMode);
                        if (CurrentDevice != null)
                        {
                            OutputWindow.Text += String.Format("Device is connected in {0}{1} on USB Hub {2} - Port {3}.\r\n",
                                CurrentDeviceState,
                                (CurrentDevice is Volume) ? "(" + ((Volume)CurrentDevice).LogicalDrive + ")" : String.Empty,
                                CurrentDevice.UsbHub.Index,
                                CurrentDevice.UsbPort);
                            NextUpdateAction = UpdateAction.WriteMedia;
                            DoUpdateProcess(NextUpdateAction);
                        }
                        else
                        {
                            // error
                        }
                    }

                    break;
                }
                case UpdateAction.WriteMedia:
                {
                    if (e.Event == DeviceChangeEvent.VolumeArrival)
                    {
                        CurrentDevice = FindDevice(DeviceState.UpdaterMode);
                        if (CurrentDevice != null)
                        {
                            OutputWindow.Text += String.Format("Device is connected in {0}{1} on USB Hub {2} - Port {3}.\r\n",
                                CurrentDeviceState,
                                (CurrentDevice is Volume) ? "(" + ((Volume)CurrentDevice).LogicalDrive + ")" : String.Empty,
                                CurrentDevice.UsbHub.Index,
                                CurrentDevice.UsbPort);
                            NextUpdateAction = UpdateAction.FinalInit;
                            DoUpdateProcess(NextUpdateAction);
                        }
                        else
                        {
                            // error
                        }
                    }

                    break;
                }
                case UpdateAction.FinalInit:
                {
                    if ( e.Event == DeviceChangeEvent.VolumeArrival || 
                         e.Event == DeviceChangeEvent.DeviceArrival )
                    {
                        CurrentDevice = FindDevice(DeviceState.UserMode);
                        if (CurrentDevice != null)
                        {
                            OutputWindow.Text += String.Format("Device is connected in {0}{1} on USB Hub {2} - Port {3}.\r\n",
                                CurrentDeviceState,
                                (CurrentDevice is Volume) ? "(" + ((Volume)CurrentDevice).LogicalDrive + ")" : String.Empty,
                                CurrentDevice.UsbHub.Index,
                                CurrentDevice.UsbPort);
                            NextUpdateAction = UpdateAction.Complete;
                           DoUpdateProcess(NextUpdateAction);
                        }
                        else
                        {
                            // error
                        }
                    }

                    break;
                }
            }
        }

        private void UpdateStatus()
        {
            if (NextUpdateAction == UpdateAction.NotStarted)
            {
                PreserveJanusCheckBox.Enabled = EraseMediaChekBox.Checked;

                if (CurrentDevice != null)
                {
                    UpdateCurrentDeviceStatus();

                    StatusTextBox.Text = "Ready.";
                    UpdateButton.Enabled = true;
                    OutputWindow.Text += String.Format("Device is connected in {0}{1} on USB Hub {2} - Port {3}.\r\n",
                        CurrentDeviceState,
                        (CurrentDevice is Volume) ? "(" + ((Volume)CurrentDevice).LogicalDrive + ")" : String.Empty,
                        CurrentDevice.UsbHub.Index,
                        CurrentDevice.UsbPort);
                }
                else
                {
                    StatusTextBox.Text = "Please connect device.";
                    CurrentVersionTextBox.Text = String.Empty;
                    UpdateButton.Enabled = false;
                    PictureBox.Image = null;
                    OutputWindow.Text += "Device is not connected.\r\n";
                }
            }
            else if (NextUpdateAction == UpdateAction.Complete)
            {
                UpdateCurrentDeviceStatus();
                CloseButton.Enabled = true;
            }
            else
            {
                UpdateButton.Enabled = false;
                CloseButton.Enabled = false;
                EraseMediaChekBox.Enabled = false;
                PreserveJanusCheckBox.Enabled = false;
            }
        }

        private void UpdateCurrentDeviceStatus()
        {
            if (CurrentDevice is ILiveUpdater)
            {
                Byte[] devInfoData = ((ILiveUpdater)CurrentDevice).GetDeviceDataFromFile("DeviceInfo.txt");
                if (devInfoData != null)
                {
                    CurrentVersionTextBox.Text = ParseDeviceInfoData(devInfoData).ToString();
                }
                else
                {
                    CurrentVersionTextBox.Text = String.Empty;
                }

                Byte[] devIconData = ((ILiveUpdater)CurrentDevice).GetDeviceDataFromFile("DevIcon.fil");
                if (devIconData != null)
                {
                    PictureBox.Image = MakeImage(devIconData).ToBitmap();
                }
                else
                {
                    PictureBox.Image = null;
                }
            }
        }

        private FirmwareInfo.Version ParseDeviceInfoData(Byte[] data)
        {
            FirmwareInfo.Version returnVersion = new FirmwareInfo.Version();

            // convert the bytes into a string
            System.Text.ASCIIEncoding encoder = new System.Text.ASCIIEncoding();
            String dataString = encoder.GetString(data);

            // split the string into lines
            Char[] newlineChars = new Char[2] { '\r', '\n' };
            String[] versionStrings = dataString.Split(newlineChars, StringSplitOptions.RemoveEmptyEntries);

            // COMP.5.005.0057
            // PROD.5.005.0057
            foreach (String str in versionStrings)
            {
                String[] versionParts = str.Split('.');
                if (versionParts.Length == 4 && versionParts[0] == "PROD")
                {
                    returnVersion = new FirmwareInfo.Version(UInt16.Parse(versionParts[1]),
                        UInt16.Parse(versionParts[2]), UInt16.Parse(versionParts[3]));
                    break;
                }
            }

            return returnVersion;
        }

        private Icon MakeImage(Byte[] data)
        {
            MemoryStream stream = new MemoryStream(data);

            return new Icon(stream,new Size(48, 48));
        }

        private void EraseMediaChekBox_CheckedChanged(object sender, EventArgs e)
        {
            PreserveJanusCheckBox.Enabled = EraseMediaChekBox.Checked;
        }

        private void OutputWindow_TextChanged(object sender, EventArgs e)
        {
            HandleRef hRef = new HandleRef(OutputWindow, OutputWindow.Handle);
            Win32.SendMessage(hRef, Win32.WM_VSCROLL, (IntPtr)Win32.SB_BOTTOM, (IntPtr)IntPtr.Zero);
        }

        private void ShowLogButton_Click(object sender, EventArgs e)
        {
            if (splitContainer1.Panel2Collapsed)
            {
                //
                // Show Log Window
                //
                splitContainer1.Panel2Collapsed = false;
                this.Top -= SavedLogWindowHeight;
                this.Height += SavedLogWindowHeight;
                ShowLogButton.ImageKey = "Hide.ico";
            }
            else
            {
                //
                // Hide Log Window
                //
                SavedLogWindowHeight = splitContainer1.Panel2.Height;
                this.Height -= SavedLogWindowHeight;
                this.Top += SavedLogWindowHeight;
                splitContainer1.Panel2Collapsed = true;
                ShowLogButton.ImageKey = "Show.ico";
            }
        }

        private void OnShowLog(bool show)
        {
        }

    } // class MainWindow

} // namespace
