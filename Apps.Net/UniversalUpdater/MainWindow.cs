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
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.Xml.Serialization;

using DevSupport;
using DevSupport.DeviceManager;
using DevSupport.DeviceManager.UTP;
using DevSupport.Media;

namespace UniversalUpdater
{
    public partial class MainWindow : Form
    {
        private enum UpdateAction { NotStarted, Working, Ready };

        private delegate Int32 DoCommandAsync(Command cmd);
        private delegate Int32 DoUtpCommandAsync(String cmd);
        private AutoResetEvent DeviceChangeSignal = new AutoResetEvent(false);

        private UpdateTransportProtocol UTProtocol = null;
        private UpdaterOperations UpdaterOps = null;
        private CommandList CurrentCommandList = null;
        private Dictionary<DeviceMode, DeviceDesc> DeviceDescs = new Dictionary<DeviceMode,DeviceDesc>();

        private Device CurrentDevice = null;
        private DeviceMode CurrentDeviceMode = DeviceMode.Unknown;
        private UpdateAction CurrentUpdateAction = UpdateAction.NotStarted;
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

        public MainWindow()
        {
            InitializeComponent();
            this.Icon = Properties.Resources.Application;
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

        // This delegate enables asynchronous calls for setting
        // the text property on a TextBox control.
        delegate void SetTextCallback(Control control, String text, String action);

        // This method demonstrates a pattern for making thread-safe
        // calls on a Windows Forms control. 
        //
        // If the calling thread is different from the thread that
        // created the control, this method creates a
        // SetTextCallback and calls itself asynchronously using the
        // Invoke method.
        //
        // If the calling thread is the same as the thread that created
        // the control, the Text property is set directly. 
        private void SetText(Control control, String text, String action)
        {
            if (control.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText);
                this.BeginInvoke(d, new object[] { control, text, action });
            }
            else
            {
                switch(action)
                {
                    case "add":
                        control.Text += text;
                        break;
                    default:
                        control.Text = text;
                        break;
                }
            }
        }

        /// <summary>
        /// This is the handler for the CancelAutoPlay notifications. We don't need to do anything here.
        /// </summary>
        /// <param name="args"></param>
        private void DeviceManager_CancelAutoPlayNotify(Object args) { }

        void DeviceManager_DeviceChanged(DeviceChangedEventArgs e)
        {
            Debug.WriteLine(String.Format("*** MainWindow.DeviceManager_DeviceChanged(): {0}: {1}, {2}({3})", e.Event, e.DeviceId, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

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

            DeviceMode oldMode = CurrentDeviceMode;

            if (CurrentDeviceMode == DeviceMode.Disconnected)
            {
                switch (e.Event)
                {
                    case DeviceChangeEvent.DeviceArrival:

                        CurrentDevice = FindDevice(DeviceMode.Recovery);

                        if (CurrentDevice == null)
                            CurrentDevice = FindDevice(DeviceMode.UserMtp);

                        break;

                    case DeviceChangeEvent.VolumeArrival:

                        CurrentDevice = FindDevice(DeviceMode.UserMsc);

                        if (CurrentDevice == null)
                            CurrentDevice = FindDevice(DeviceMode.Updater);

                        break;
                }

//                if (CurrentDevice != null)
//                    CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);
            }
            else
            {
                if (e.Event == DeviceChangeEvent.DeviceRemoval || e.Event == DeviceChangeEvent.VolumeRemoval)
                {
                    if (CurrentDevice != null)
                    {
                        CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;
                        CurrentDevice.Dispose();
                        CurrentDevice = null;
                    }
                    if (/*CurrentUpdateAction == UpdateAction.Working &&*/ UTProtocol != null) // ugly
                    {
                        UTProtocol.Dispose();
                        UTProtocol = null;
                    }

                    CurrentDeviceMode = DeviceMode.Disconnected;
                    DeviceDescStatusLabel.Text = DeviceMode.Disconnected.ToString();

                    if (CurrentUpdateAction == UpdateAction.Ready)
                        CurrentUpdateAction = UpdateAction.NotStarted;
/*
                    if (CurrentUpdateAction == UpdateAction.Ready)
                    {
                        CurrentUpdateAction = UpdateAction.NotStarted;
                        UpdateStatus();
                    }
*/
                }
            }

            if (oldMode != CurrentDeviceMode)
                DeviceChangeSignal.Set();

            UpdateStatus();
            
         }

        private Int32 DoList(CommandList list)
        {
            if (String.IsNullOrEmpty(list.Description))
                StatusTextBox.Text = String.Empty;
            else
            {
                StatusTextBox.Text = list.Description;
                OutputWindow.Text += "(s) " + StatusTextBox.Text + "\r\n";
            }
            OutputWindow.Text += "LIST: " + list.Name + "\r\n";

            // Set Overall Progress Bar - increment before and after command.
            OverallProgressBar.Maximum = list.Commands.Length * 2;
            OverallProgressBar.Value = 0;

            Int32 retValue = 0;

            foreach (Command cmd in list.Commands)
            {
                ++OverallProgressBar.Value;

                retValue = DoCommand(cmd);
                if (retValue != 0)
                {
                    if (cmd.OnError == "ignore")
                    {
                        retValue = 0;
                        ++OverallProgressBar.Value;
                        continue;
                    }
                    else
                        break;
                }

                ++OverallProgressBar.Value;
            }

            return retValue;
        }

        private Int32 DoCommand(Command cmd)
        {
            Int32 retValue = 0;

            if (!String.IsNullOrEmpty(cmd.Description))
            {
                TaskTextBox.Text = cmd.Description;
                OutputWindow.Text += " (t) " + TaskTextBox.Text + "\r\n";
            }
            OutputWindow.Text += " CMD: " + cmd.ToString() + "\r\n";

            // register for the progress events
//            if ( CurrentDevice != null )
//                CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

            switch (cmd.CmdType)
            {
                case "boot":
                    // Reset device to ROM and load file.
                    DoCommandAsync bootDelegate = new DoCommandAsync(DoBoot);
                    IAsyncResult arBoot = bootDelegate.BeginInvoke(cmd, null, null);

                    // Wait for the asynchronous operation to finish.
                    while (!arBoot.IsCompleted)
                    {
                        // Keep UI messages moving, so the form remains 
                        // responsive during the asynchronous operation.
                        Application.DoEvents();
                    }

                    // Get the result of the operation.
                    retValue = bootDelegate.EndInvoke(arBoot);

                    break;
                
                case "show":
                    retValue = DoShow(cmd);
                    break;
                
                case "format":
                    break;
                
                case "push":

                    // register for the progress events
                    CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);
                    
                    if (String.IsNullOrEmpty(cmd.Filename))
                    {
                        // send command and wait for device to finish.
                        DoUtpCommandAsync utpCmdDelegate = new DoUtpCommandAsync(UTProtocol.UtpCommand);
                        IAsyncResult arUtpCmd = utpCmdDelegate.BeginInvoke(cmd.CommandString, null, null);

                        // Wait for the asynchronous operation to finish.
                        while (!arUtpCmd.IsCompleted)
                        {
                            // Keep UI messages moving, so the form remains 
                            // responsive during the asynchronous operation.
                            Application.DoEvents();
                        }

                        // Get the result of the operation.
                        retValue = utpCmdDelegate.EndInvoke(arUtpCmd);
//                        retValue = UTProtocol.UtpCommand(cmd.CommandString);
                    }
                    else
                    {
                        retValue = UTProtocol.UtpWrite(cmd.CommandString, cmd.Filename);
                    }

                    // unregister for the progress events
                    CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

                    break;
                
                case "pull":
                    
                    // register for the progress events
                    CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

                    retValue = UTProtocol.UtpRead(cmd.CommandString, cmd.Filename);

                    // unregister for the progress events
                    CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

                    break;
                
                case "drop":
                    // send command and wait for device to disconnect from the bus.
                    DoCommandAsync dropDelegate = new DoCommandAsync(DoDrop);
                    IAsyncResult arDrop = dropDelegate.BeginInvoke(cmd, null, null);

                    // Wait for the asynchronous operation to finish.
                    while (!arDrop.IsCompleted)
                    {
                        // Keep UI messages moving, so the form remains 
                        // responsive during the asynchronous operation.
                        Application.DoEvents();
                    }

                    // Get the result of the operation.
                    retValue = dropDelegate.EndInvoke(arDrop);
                    break;
                
                case "find":
                    // See if specified device is connected and wait for it
                    // if it is not.
                    DoCommandAsync findDelegate = new DoCommandAsync(DoFind);
                    IAsyncResult arFind = findDelegate.BeginInvoke(cmd, null, null);

                    // Wait for the asynchronous operation to finish.
                    while (!arFind.IsCompleted)
                    {
                        // Keep UI messages moving, so the form remains 
                        // responsive during the asynchronous operation.
                        Application.DoEvents();
                    }

                    // Get the result of the operation.
                    retValue = findDelegate.EndInvoke(arFind);
                    break;
            }

            // unregister for the progress events
//            if ( CurrentDevice != null )
//                CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

            return retValue;
        }

        void CurrentDevice_SendCommandProgress(object sender, Device.SendCommandProgressArgs args)
        {
            if (args.InProgress)
            {
                TaskProgressBar.Maximum = (int)args.Maximum;
                TaskProgressBar.Value = Convert.ToInt32(args.Position);
                //                ActionDetailsTextBox.Text = args.Name;
                Utils.DecimalConverterEx decConverter = new Utils.DecimalConverterEx();
                //                OutputWindow.Text += String.Format("*Processing {0}. Position = {1}.\r\n", args.Name, decConverter.ConvertToString(args.Position));
//                OutputWindow.Text += "   " + args.Status + "\r\n";
                if (!String.IsNullOrEmpty(args.Status))
                    OutputWindow.Text += "   " + args.Status + "\r\n";
//                else
//                    OutputWindow.Text += "nothing to add.\r\n";
            }
            else
            {
                if (!String.IsNullOrEmpty(args.Status))
                    OutputWindow.Text += "   " + args.Status + "\r\n";
//                else
//                    OutputWindow.Text += "nothing to add.\r\n";
                TaskProgressBar.Value = 0;
            }
        }

        private Int32 DoDrop(Command cmd)
        {
            // Send the command.
            Int32 retValue = UTProtocol.UtpDrop(cmd.CommandString);

            if (retValue == 0)
            {
                // Wait for device to disconnect from the bus.
                while (CurrentDeviceMode != DeviceMode.Disconnected)
                {
                    // wait 15 seconds for device to change state.
                    if (!DeviceChangeSignal.WaitOne(15 * 1000, false))
                    {
                        SetText(OutputWindow, String.Format(" ERROR: Timeout. {0} never disconnected.\r\n", CurrentDevice.ToString()), "add");
                        retValue = -65536;
                        break;
                    }
                }
            }

            return retValue;
        
        } // DoDrop()

        private Int32 DoFind(Command cmd)
        {
            Int32 retValue = 0;

            DeviceMode newDevMode = (DeviceMode)Enum.Parse(typeof(DeviceMode), cmd.CommandString, true);
            DeviceDesc newDevDesc = null;
            if (DeviceDescs.ContainsKey(newDevMode))
                newDevDesc = DeviceDescs[newDevMode];

            SetText(OutputWindow, String.Format("Looking for {0}-mode device.\r\n", newDevMode), "add");

            if (CurrentDevice != null)
            {
                if (CurrentDeviceMode == newDevMode)
                    return 0;
            }

            // Wait for device to connect to the bus.
            while (CurrentDeviceMode != newDevMode)
            {
                // wait 15 seconds for device to change state.
                if (!DeviceChangeSignal.WaitOne(15 * 1000, false))
                {
                    SetText(OutputWindow, String.Format(" ERROR: Timeout. Never found {0}.\r\n", newDevDesc.ToString()), "add");
                    retValue = -65536;
                    break;
                }
            }

            if (CurrentDeviceMode == newDevMode)
                retValue = 0;
            else
                retValue = -65536;

            return retValue;

        } // DoFind()

        private Int32 DoBoot(Command cmd)
        {
            Int32 retValue = 0;

            if (String.IsNullOrEmpty(Thread.CurrentThread.Name))
                Thread.CurrentThread.Name = "DoBoot";

            // Reset Device to Recovery-mode
            retValue = DoResetToRecovery();
            if (retValue != 0)
            {
                // error
            }

            // Look for device in Recovery-mode
            retValue = DoFind(cmd);
            if (retValue != 0)
            {
                // error
            }

            retValue = DoLoad(cmd.Filename);

            return retValue;

        } // DoBoot()

        private Int32 DoResetToRecovery()
        {
            Int32 retValue = 0;

            //
            // Send Reset Command
            //
            switch (CurrentDeviceMode)
            {
                case DeviceMode.Recovery:
                    // Already in Recovery mode, so we're done.
                    SetText(OutputWindow, "Already in Recovery-mode.\r\n", "add");
                    break;
                case DeviceMode.UserMsc:
                case DeviceMode.UserMtp:
                case DeviceMode.User:
                case DeviceMode.Updater:
                if (CurrentDevice is IResetToRecovery)
                    {
                        SetText(OutputWindow, "Sending ResetToRecovery command.\r\n", "add");
                        if (((IResetToRecovery)CurrentDevice).ResetToRecovery() != Win32.ERROR_SUCCESS)
                        {
                            SetText(OutputWindow, CurrentDevice.ErrorString + "\r\n", "add");
                            retValue = -65536;
                        }
                    }
                    break;
                case DeviceMode.Disconnected:
                case DeviceMode.Unknown:
                default:
                    SetText(OutputWindow, String.Format(" ERROR: Device \"{0}\" does not support ResetToRecovery command.\r\n", CurrentDevice == null ? "No Device" : CurrentDevice.ToString()), "add");
                    retValue = -65536;
                    break;
            }

            return retValue;
        }

        Int32 DoLoad(String filename)
        {
            Int32 retValue = 0;

            if (CurrentDevice is IRecoverable)
            {
                SetText(OutputWindow, String.Format("Loading {0}.\r\n", filename), "add");

                // register for the progress events
                CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

                if (((IRecoverable)CurrentDevice).LoadRecoveryDevice(filename) != Win32.ERROR_SUCCESS)
                {
                    SetText(OutputWindow, CurrentDevice.ErrorString + "\r\n", "add");
                    retValue = -65536;
                }

                // unregister for the progress events
                CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;
            }
            else
            {
                SetText(OutputWindow, String.Format("ERROR: Device {0} is not in the correct mode for loading {1}.\r\n", CurrentDevice, filename), "add");
                retValue = -65536;
            }

//            SetText(OutputWindow, String.Format("DONE Loading {0}.\r\n", filename), "add");
            return retValue;
        }

        Int32 DoShow(Command cmd)
        {
            Int32 retValue = 0;
            DeviceInfo devInfo = null;

            // Get the device information.
            try
            {
                XmlSerializer xmlr = new XmlSerializer(typeof(DeviceInfo));
                FileStream fs = new FileStream(cmd.Filename, FileMode.Open);
                devInfo = (DeviceInfo)xmlr.Deserialize(fs);
                fs.Close();
            }
            catch (Exception err)
            {
                DialogResult result = MessageBox.Show(this, err.Message);
            }
            
            if ( devInfo != null )
                CurrentVersionTextBox.Text = devInfo.FW_Version;

            return retValue;
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            Thread.CurrentThread.Name = "MainWindow UI Thread";
            Debug.WriteLine(String.Format("*** MainWindow.Loading(): {0}({1})", Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

            // Add "About..." menu item to system menu.
            IntPtr sysMenuHandle = Win32.GetSystemMenu(this.Handle, false);
            Win32.AppendMenu(sysMenuHandle, Win32.MF_SEPARATOR, 0, string.Empty);
            Win32.AppendMenu(sysMenuHandle, Win32.MF_STRING, Win32.IDM_ABOUT, "About " + this.Text + "...");

            // Get the update instruction list.
            try
            {
                XmlSerializer xmlr = new XmlSerializer(typeof(UpdaterOperations));
                FileStream fs = new FileStream("ucl.xml", FileMode.Open, FileAccess.Read);
                UpdaterOps = (UpdaterOperations)xmlr.Deserialize(fs);
                fs.Close();
                DisplayUpdaterOps(UpdaterOps, Properties.Settings.Default.OperationList);
            }
            catch (Exception err)
            {
                DialogResult result = MessageBox.Show(this, err.Message);
                Application.Exit();
                return;
            }

            // Put the ugrade firmware version in the UI.
            try
            {
                FirmwareInfo firmwareInfo = FirmwareInfo.FromFile(UpdaterOps.ConfigData.FirmwareVersion.Filename);
                UpgradeVersionTextBox.Text = firmwareInfo.ProjectVersion.ToString("0.00");
            }
            catch (Exception err)
            {
                DialogResult result = MessageBox.Show(this, err.Message);
                Application.Exit();
                return;
            }

            // See if our device is connected
            foreach (DeviceDesc desc in UpdaterOps.ConfigData.DeviceDescs)
            {
                DeviceDescs.Add(desc.Mode, desc);
            }
            CurrentDevice = FindDevice(DeviceMode.Unknown);

            // Create an instance of the DeviceManager and register for device change notifications.
            DeviceManager.Instance.DeviceChanged += new DeviceManager.DeviceChangedEventHandler(DeviceManager_DeviceChanged);
            // Turn off AutoPlay to keep those pesky windows from popping up while we are updating.
            DeviceManager.Instance.CancelAutoPlay += new CancelAutoPlayEventHandler(DeviceManager_CancelAutoPlayNotify);

            UpdateStatus();
        }

        private void MainWindow_FormClosing(object sender, FormClosingEventArgs e)
        {
            DeviceManager.Instance.Dispose();
        }

        private void CloseButton_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void OutputWindow_TextChanged(object sender, EventArgs e)
        {
            HandleRef hRef = new HandleRef(OutputWindow, OutputWindow.Handle);
            Win32.SendMessage(hRef, Win32.WM_VSCROLL, (IntPtr)Win32.SB_BOTTOM, (IntPtr)IntPtr.Zero);
        }

        private void UpdateButton_Click(object sender, EventArgs e)
        {
            Int32 retValue = 0;
            OutputWindow.Text += "<User clicked \"Update\" button.>\r\n";

            CurrentUpdateAction = UpdateAction.Working;
            UpdateStatus();

            retValue = DoList(CurrentCommandList);

            CurrentUpdateAction = UpdateAction.Ready;
            UpdateStatus();

            if ( retValue > 0 )
                MessageBox.Show(this, "This update operation is not possible.\r\n\r\nPlease select a different operation and retry.\r\n\r\nCode: " + retValue.ToString());
            else if ( retValue < 0 )
                MessageBox.Show(this, "This update operation failed.\r\n\r\nCode: " + retValue.ToString());
        }

        private void DisplayUpdaterOps(UpdaterOperations list, String defaultList)
        {
            foreach (CommandList op in list.CommandLists)
            {
                if (op.Name != "Exit")
                {
                    RadioButton button = new RadioButton();
                    button.Text = op.Name;
                    button.Tag = op;
                    if (op.Name == defaultList)
                    {
                        button.Checked = true;
                        CurrentCommandList = op;
                    }
                    button.CheckedChanged += new EventHandler(UpdaterSelection_Changed);
                    toolTip1.SetToolTip(button, op.Description);
                    Operations_LayoutPanel.Controls.Add(button);
                }
            }
            if (CurrentCommandList == null && Operations_LayoutPanel.Controls[0] != null)
            {
                ((RadioButton)Operations_LayoutPanel.Controls[0]).Checked = true;
            }
            else
            {
                UpdateButton.Enabled = false;
            }
        }

        private void UpdaterSelection_Changed(Object sender, EventArgs e)
        {
            RadioButton button = (RadioButton)sender;
            if (button.Checked)
            {
                CurrentCommandList = (CommandList)button.Tag;
                OutputWindow.Text += String.Format("<User selected \"{0}\" operation.>\r\n", CurrentCommandList.Name);
                OverallProgressBar.Value = 0;
            }
            else
            {
                CurrentCommandList = null;
            }
        }

        private void UpdateStatus()
        {
            if (CurrentUpdateAction == UpdateAction.NotStarted)
            {
                if (CurrentDevice != null)
                {
                    UpdateCurrentDeviceStatus();

                    OutputWindow.Text += String.Format("Device{1} is connected in {0} mode on USB Hub {2} - Port {3}.\r\n",
                        CurrentDeviceMode,
                        (CurrentDevice is Volume) ? "(" + ((Volume)CurrentDevice).LogicalDrive + ")" : String.Empty,
                        CurrentDevice.UsbHub.Index,
                        CurrentDevice.UsbPort);

                    StatusTextBox.Text = "Ready.";
                    OutputWindow.Text += "(s) " + StatusTextBox.Text + "\r\n";
                    foreach (Control ctrl in Operations_LayoutPanel.Controls)
                    {
                        ctrl.Enabled = true;
                    }
                    UpdateButton.Enabled = true;
                    CloseButton.Enabled = true;
                }
                else
                {
                    CurrentVersionTextBox.Text = String.Empty;
                    TaskTextBox.Text = String.Empty;
                    UpdateButton.Enabled = false;
                    CloseButton.Enabled = true;
                    PictureBox.Image = null;
                    StatusTextBox.Text = "Please connect device.";
                    OutputWindow.Text += "(s) " + StatusTextBox.Text + "\r\n";
                    foreach (Control ctrl in Operations_LayoutPanel.Controls)
                    {
                        ctrl.Enabled = false;
                    }
                }
            }
            else if (CurrentUpdateAction == UpdateAction.Working)
            {
                UpdateButton.Enabled = false;
                CloseButton.Enabled = false;
                foreach (Control ctrl in Operations_LayoutPanel.Controls)
                {
                    ctrl.Enabled = false;
                }
            }
            else if (CurrentUpdateAction == UpdateAction.Ready)
            {
                StatusTextBox.Text = "Ready.";
                OutputWindow.Text += "(s) " + StatusTextBox.Text + "\r\n";
                foreach (Control ctrl in Operations_LayoutPanel.Controls)
                {
                    ctrl.Enabled = true;
                }
                UpdateButton.Enabled = true;
                CloseButton.Enabled = true;
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

            return new Icon(stream, new Size(48, 48));
        }

        private Device FindDevice(DeviceMode mode)
        {
            Device tempDevice = null;

            switch (mode)
            {
                case DeviceMode.Recovery:

                    if ( DeviceDescs.ContainsKey(DeviceMode.Recovery) )
                    {
                        UInt16? vid = DeviceDescs[DeviceMode.Recovery].Vid;
                        UInt16? pid = DeviceDescs[DeviceMode.Recovery].Pid;

                        tempDevice = DeviceManager.Instance.FindDevice(typeof(HidDeviceClass), vid, pid);

                        if (tempDevice == null)
                        tempDevice = DeviceManager.Instance.FindDevice(typeof(RecoveryDeviceClass), vid, pid);
                    }
                    if (tempDevice != null)
                    {
                        DeviceDescStatusLabel.Text = DeviceDescs[DeviceMode.Recovery].ToString();
                        CurrentDeviceMode = DeviceMode.Recovery;
                        CurrentPort = new UsbPortId(tempDevice.UsbHub.Index, tempDevice.UsbPort);
                    }
                    break;

                case DeviceMode.Updater:

                    if (DeviceDescs.ContainsKey(DeviceMode.Updater))
                    {
                        UInt16? vid = DeviceDescs[DeviceMode.Updater].Vid;
                        UInt16? pid = DeviceDescs[DeviceMode.Updater].Pid;

                        tempDevice = DeviceManager.Instance.FindDevice(typeof(VolumeDeviceClass), vid, pid);
                    }
                    if (tempDevice != null)
                    {
                        DeviceDescStatusLabel.Text = DeviceDescs[DeviceMode.Updater].ToString();
                        CurrentDeviceMode = DeviceMode.Updater;
                        CurrentPort = new UsbPortId(tempDevice.UsbHub.Index, tempDevice.UsbPort);
                        
                        if (UTProtocol != null)
                            UTProtocol.Dispose();

                        UTProtocol = new UpdateTransportProtocol((Volume)tempDevice);
                    }
                    break;

                case DeviceMode.User:

                    tempDevice = FindDevice(DeviceMode.UserMtp);

                    if (tempDevice == null)
                    {
                        tempDevice = FindDevice(DeviceMode.UserMsc);
                    }

                    break;

                case DeviceMode.UserMtp:

                    if (DeviceDescs.ContainsKey(DeviceMode.UserMtp))
                    {
                        UInt16? vid = DeviceDescs[DeviceMode.UserMtp].Vid;
                        UInt16? pid = DeviceDescs[DeviceMode.UserMtp].Pid;

                        tempDevice = DeviceManager.Instance.FindDevice(typeof(WpdDeviceClass), vid, pid);
                    }
                    if (tempDevice != null)
                    {
                        DeviceDescStatusLabel.Text = DeviceDescs[DeviceMode.UserMtp].ToString();
                        CurrentDeviceMode = DeviceMode.UserMtp;
                        CurrentPort = new UsbPortId(tempDevice.UsbHub.Index, tempDevice.UsbPort);
                    }
                    break;

                case DeviceMode.UserMsc:

                    if (DeviceDescs.ContainsKey(DeviceMode.UserMsc))
                    {
                        UInt16? vid = DeviceDescs[DeviceMode.UserMsc].Vid;
                        UInt16? pid = DeviceDescs[DeviceMode.UserMsc].Pid;

                        tempDevice = DeviceManager.Instance.FindDevice(typeof(VolumeDeviceClass), vid, pid);
                    }
                    if (tempDevice != null)
                    {
                        DeviceDescStatusLabel.Text = DeviceDescs[DeviceMode.UserMsc].ToString();
                        CurrentDeviceMode = DeviceMode.UserMsc;
                        CurrentPort = new UsbPortId(tempDevice.UsbHub.Index, tempDevice.UsbPort);
                    }
                    break;

                case DeviceMode.Unknown:

                    tempDevice = FindDevice(DeviceMode.User);

                    if (tempDevice == null)
                    {
                        tempDevice = FindDevice(DeviceMode.Updater);
                    }
                    if (tempDevice == null)
                    {
                        tempDevice = FindDevice(DeviceMode.Recovery);
                    }
                    break;
            }

            if (tempDevice == null)
            {
                DeviceDescStatusLabel.Text = DeviceMode.Disconnected.ToString();
                CurrentDeviceMode = DeviceMode.Disconnected;
                CurrentPort = new UsbPortId();
//                if (deviceState != DeviceState.Unknown && deviceState != DeviceState.UserMode)
//                    OutputWindow.Text += String.Format("Specified device is not connected in {0}.\n", deviceState);
            }
//            else
//            {
//                if (deviceState != DeviceState.Unknown && deviceState != DeviceState.UserMode)
//                    OutputWindow.Text += String.Format("Device is connected in {0}.\n", CurrentDeviceMode);
//            }

            return tempDevice;
        }

    }
}
