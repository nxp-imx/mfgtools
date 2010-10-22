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
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;

using DevSupport.DeviceManager;
using DevSupport.Media;

namespace LiveUpdaterExample
{
    public partial class MainWindow : Form
    {
        private Device CurrentDevice = null;
        private String FirmwareFilename = "firmware.sb";

        public MainWindow()
        {
            InitializeComponent();
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            // Create an instance of the DeviceManager
            try
            {
                DeviceManager.Instance.DeviceChanged += new DeviceManager.DeviceChangedEventHandler(DeviceManager_DeviceChanged);
            }
            catch (Exception err)
            {
                Trace.WriteLine(err.Message);
            }

            // Attach the Device Menu to the main menu Device item
            this.MainMenuItem_Device.DropDown = this.DeviceMenuStrip;
            // Attach the Device Menu ImageList
            this.DeviceMenuStrip.ImageList = DeviceManager.ImageList;
            this.MainWindowMenuStrip.ImageList = DeviceManager.ImageList;

            // find any suitable devices
            FillDeviceMenu();

            UpdateStatus();
            
        }

        private void MainWindow_FormClosing(object sender, FormClosingEventArgs e)
        {
            DeviceManager.Instance.Dispose();
        }

        private void FileMenuItem_Open_Click(object sender, EventArgs e)
        {
            OpenFileDialog openDialog = new OpenFileDialog();
            openDialog.Filter = "Firmware Files (*.sb)|*.sb|All Files (*.*)|*.*";

            if (openDialog.ShowDialog() == DialogResult.OK)
            {
                FirmwareFilename = openDialog.FileName;
                UpdateStatus();
            }
        }

        private void FileMenuItem_Exit_Click(object sender, EventArgs e)
        {
            Application.Exit();

        }

        private void CloseButton_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void UpdateButton_Click(object sender, EventArgs e)
        {
            if (CurrentDevice == null)
                return;

            if (CurrentDevice is ILiveUpdater)
            {
                UpdateButton.Enabled = false;
                CloseButton.Enabled = false;

                StatusTextBox.Text = "Copying firmware to device...";

                // register for the progress events
                CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

                FirmwareInfo info = FirmwareInfo.FromFile(FirmwareFilename);
                ProgressBar.Maximum = (Int32)info.DataSize;

                Int32 ret = ((ILiveUpdater)CurrentDevice).CopyUpdateFileToMedia(FirmwareFilename);
                
                // unregister for the progress events
                CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

                if (ret == DevSupport.Api.Api.CmdSuccess)
                {
                    StatusTextBox.Text = "Copy finished.\r\n You must restart your device to complete the update.";
                }
                else
                {
                    StatusTextBox.Text = CurrentDevice.ErrorString;
                }

                CloseButton.Enabled = true;
            }
        }

        void CurrentDevice_SendCommandProgress(object sender, Device.SendCommandProgressArgs args)
        {
            if (args.InProgress)
            {
                ProgressBar.Maximum = (int)args.Maximum;
                ProgressBar.Value = Convert.ToInt32(args.Position);
            }
        }

        private void FillDeviceMenu()
        {
            DeviceMenuStrip.Items.Clear();

            // display volumes
            VolumeDeviceClass volumeDeviceClass = VolumeDeviceClass.Instance;
            foreach (Volume volume in volumeDeviceClass.Devices)
            {
                if (volume.IsUsb)
                {
                    ToolStripMenuItem menuItem = new ToolStripMenuItem(volume.ToString());
                    menuItem.Name = volume.DeviceInstanceId;
                    menuItem.ImageIndex = volume.ClassIconIndex;
                    menuItem.Tag = volume;

                    if (DeviceMenuStrip.Items.Contains(menuItem) == false)
                        DeviceMenuStrip.Items.Add(menuItem);
                }
            }

            // Add MTP devices
            WpdDeviceClass wpdDeviceClass = WpdDeviceClass.Instance;
            foreach (WpdDevice wpdDev in wpdDeviceClass.Devices)
            {
                ToolStripMenuItem menuItem = new ToolStripMenuItem(wpdDev.ToString());
                menuItem.Name = wpdDev.DeviceInstanceId;
                menuItem.ImageIndex = wpdDev.ClassIconIndex;
                menuItem.Tag = wpdDev;

                if (DeviceMenuStrip.Items.Contains(menuItem) == false)
                    DeviceMenuStrip.Items.Add(menuItem);
            }
        }
        
        void DeviceManager_DeviceChanged(DeviceChangedEventArgs e)
        {
            Trace.WriteLine(String.Format("*** MainWindow.DeviceManager_DeviceChanged(): {0}: {1}, {2}({3})", e.Event, e.DeviceId, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
            String logStr = e.Event + ": " + e.DeviceId + "\r\n"; ;

            FillDeviceMenu();

            if (CurrentDevice != null)
            {
                int index = DeviceMenuStrip.Items.IndexOfKey(CurrentDevice.DeviceInstanceId);
                if (index != -1)
                {
                    ToolStripMenuItem tsmi = DeviceMenuStrip.Items[index] as ToolStripMenuItem;
                    tsmi.Checked = true;
                    if (!Object.Equals(tsmi.Tag, CurrentDevice))
                    {
                        CurrentDevice = (Device)tsmi.Tag;
                    }
                }
                else
                {
                    CurrentDevice = null;
                }
            }
            else
            {
//                CurrentDevice = FindDeviceByPort(LastPort);

                if (CurrentDevice != null)
                {
                    logStr = String.Format("Track USB Port(Hub{0},Port{1}): AutoSelect \"{2}\".\r\n", CurrentDevice.UsbHub.Index, CurrentDevice.UsbPort, CurrentDevice.ToString());
                }
            }

            UpdateStatus();
        }

        private void DeviceMenuStrip_ItemClicked(object sender, ToolStripItemClickedEventArgs e)
        {
            ToolStripMenuItem menuItem = e.ClickedItem as ToolStripMenuItem;

            // if the current device is already selected, just return. Can't "un-select" a device.
            if (menuItem.Checked == true)
                return;
            // Un-select what ever device was previously selected
            foreach (ToolStripMenuItem mi in DeviceMenuStrip.Items)
                mi.Checked = false;

            // Check the selcted device and assign the CurrentDevice
            menuItem.Checked = true;
            CurrentDevice = (Device)menuItem.Tag;
//            FormPersistence.Profile.Write("Hub Name", CurrentDevice.UsbHub.Path);
//            FormPersistence.Profile.Write("Hub Index", CurrentDevice.UsbPort);
//            LastPort = new KeyValuePair<string, int>(CurrentDevice.UsbHub.Path, CurrentDevice.UsbPort);

            UpdateStatus();
        }

        private void UpdateStatus()
        {
            bool haveValidDevice = false, haveValidFirmware = false;

            MainMenuItem_Device.Text = (CurrentDevice != null) ? CurrentDevice.ToString() : "Select &Device";
            MainMenuItem_Device.ImageIndex = (CurrentDevice != null) ? CurrentDevice.ClassIconIndex : -1;

            if (!String.IsNullOrEmpty(FirmwareFilename))
            {
                try
                {
                    FirmwareInfo info = FirmwareInfo.FromFile(FirmwareFilename);
                    haveValidFirmware = (info.Type == FirmwareInfo.FileType.Stmp37xx);
                    if (haveValidFirmware)
                    {
                        UpgradeVersionTextBox.Text = info.ProjectVersion.ToString();
                        FirmwareFilenameLabel.Text = info.Name;
                    }
                    else
                    {
                        UpgradeVersionTextBox.Text = String.Empty;
                        FirmwareFilenameLabel.Text = "Invalid firmware file. Please select a different file.";
                    }
                }
                catch (Exception e)
                {
                    UpgradeVersionTextBox.Text = new FirmwareInfo.Version().ToString();
                    FirmwareFilenameLabel.Text = "ERROR: Re-Select a firmware file.";
                    haveValidFirmware = false;
                }
            }
            else
            {
                FirmwareFilenameLabel.Text = "Select a firmware file from the File menu.";
                haveValidFirmware = false;
            }

            if (CurrentDevice != null)
            {
                if (CurrentDevice is ILiveUpdater)
                {
                    Byte[] devInfoData = ((ILiveUpdater)CurrentDevice).GetDeviceDataFromFile("DeviceInfo.txt");
                    if (devInfoData != null)
                    {
                        CurrentVersionTextBox.Text = ParseDeviceInfoData(devInfoData).ToString();
                        haveValidDevice = true;
                    }
                    else
                    {
                        CurrentVersionTextBox.Text = String.Empty;
                        haveValidDevice = false;
                    }

                    Byte[] devIconData = ((ILiveUpdater)CurrentDevice).GetDeviceDataFromFile("DevIcon.fil");
                    if (devIconData != null)
                    {
                        PictureBox.Image = MakeImage(devIconData).ToBitmap();
                    }
                    else
                    {
                        PictureBox.Image = null; ;
                    }
                }
            }
            else
            {
                CurrentVersionTextBox.Text = String.Empty;
                haveValidDevice = false;
            }

            if (haveValidDevice && haveValidFirmware)
            {
                UpdateButton.Enabled = true;
                StatusTextBox.Text = "Ready";
            }
            else
            {
                UpdateButton.Enabled = false;

                if (haveValidDevice)
                {
                    StatusTextBox.Text = "Select a firmware file from the File menu.";
                }
                else if (haveValidFirmware)
                {
                    StatusTextBox.Text = "Select a device from the menu.";
                }
                else
                {
                    StatusTextBox.Text = "Select a device and a firmware file from the menu.";
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

    } // class MainWindow

} // namespace
