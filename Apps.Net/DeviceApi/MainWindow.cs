using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;

using DevSupport;
using DevSupport.Api;
using DevSupport.DeviceManager;
using DevSupport.DeviceManager.UTP;
using DevSupport.Media;

namespace DeviceApi
{
    public partial class MainWindow : Form
    {
        private Device CurrentDevice;
        private UpdateTransportProtocol Utp;
        private Api CurrentApi;
        private Utils.FormPersistence FormPersistence;
        private List<Api> ScsiApiList;
        private List<Api> HidApiList;
        private List<Api> WpdApiList;
        private List<Api> RecoveryApiList;
        private List<Api> WinUsbApiList;
        public List<Api> UtpProtocolList;
        private String LastProtocol;
        private Dictionary<String, Dictionary<String, Api> > LastApis;
        private KeyValuePair<String, int> LastPort = new KeyValuePair<String, int>("", -1);

        public MainWindow()
        {
            Thread.CurrentThread.Name = "MainWindow thread";
            Trace.TraceInformation("MainWindow() - {0}({1})", Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode());

            InitializeComponent();

            FormPersistence = new Utils.FormPersistence(this, @"Freescale\DeviceApi\Settings");
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
        
        private void FileMenuItem_Exit_Click(object sender, EventArgs e)
        {
            Application.Exit();

        }

        private void MainMenuItem_File_RejectAutoPlay_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem menuItem = sender as ToolStripMenuItem;

            // Save the selection to the Registry
            FormPersistence.Profile.Write("RejectAutoPlayCheckState", menuItem.Checked ? 1 : 0);

            if (menuItem.Checked)
            {
                DeviceManager.Instance.CancelAutoPlay += new CancelAutoPlayEventHandler(DeviceManager_CancelAutoPlay);
            }
            else
            {
                DeviceManager.Instance.CancelAutoPlay -= DeviceManager_CancelAutoPlay;
            }
        }

        private void MainMenuItem_File_TrackUsbPort_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem menuItem = sender as ToolStripMenuItem;

            // Save the selection to the Registry
            FormPersistence.Profile.Write("TrackUsbPortCheckState", menuItem.Checked ? 1 : 0);
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
            FormPersistence.Profile.Write("Hub Name", CurrentDevice.UsbHub.Path);
            FormPersistence.Profile.Write("Hub Index", CurrentDevice.UsbPort);
            LastPort = new KeyValuePair<string, int>(CurrentDevice.UsbHub.Path, CurrentDevice.UsbPort);

            CurrentApi = LoadDeviceApis(CurrentDevice);

            LogTextBox.AppendText(String.Format("User selected \"{0}\" device.\r\n",CurrentDevice.ToString()));

            UpdateStatus();
        }

        private void UpdateStatus()
        {
            SendButton.Enabled = ( (CurrentDevice != null) && (CurrentApi != null) );
            MainMenuItem_Device.Text = (CurrentDevice != null) ? CurrentDevice.ToString() : "Select &Device";
            MainMenuItem_Device.ImageIndex = (CurrentDevice != null) ? CurrentDevice.ClassIconIndex : -1;

            DeviceStatusStripTextCtrl.Text = (CurrentDevice != null) ? CurrentDevice.ToString() : "No device selected.";
            DeviceStatusStripTextCtrl.ImageIndex = (CurrentDevice != null) ? CurrentDevice.ClassIconIndex : -1;

            ApiStatusStripTextCtrl.Text = (CurrentApi != null) ? CurrentApi.ToString() + ": Ready." : "No api selected.";

            if (CurrentDevice is Volume)
            {
                chkUseUTP.Visible = true;
                if (chkUseUTP.Checked == true)
                    utpGrid.Visible = true;
                else
                    utpGrid.Visible = false;
            }
            else
            {
                chkUseUTP.Visible = false;
                utpGrid.Visible = false;
            }
        }

        private void ApiListCtrl_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.ApiListCtrl.SelectedItems.Count > 0)
            {
                this.CurrentApi = LastApis[CurrentDevice.GetType().Name][LastProtocol] = (Api)this.ApiListCtrl.SelectedItems[0].Tag;
                this.ApiPropertyGridCtrl.SelectedObject = this.CurrentApi;
            }
            else
            {
                this.CurrentApi = LastApis[CurrentDevice.GetType().Name][LastProtocol] = null;
            }

            this.ApiStatusStripProgressBar.Visible = false;
            UpdateStatus();
        }

        private void SendButton_Click(object sender, EventArgs e)
        {
/*            if (CurrentDevice is Volume)
            {
                using (UpdateTransportProtocol utp = new UpdateTransportProtocol(CurrentDevice as Volume))
                {
                    utp.UtpWrite("a", "firmware.sb");
                    utp.UtpWrite("w50", "firmware.sb");
                    utp.UtpRead("r50", "test.sb");
                }
            }
*/
            Int32 result = DevSupport.Win32.ERROR_SUCCESS;
            String logStr = String.Empty;

            // register for the progress events
            CurrentDevice.SendCommandProgress += new Device.SendCommandProgressHandler(CurrentDevice_SendCommandProgress);

            // See if the api is really a Device member function.
            object[] apiAttributes = CurrentApi.GetType().GetCustomAttributes(typeof(Api.MemberFunctionAttribute), false);
            if (apiAttributes.Length > 0)
            {
//                LogTextBox.AppendText(String.Format("Calling {0}.{1}() member function...\r\n", CurrentDevice.GetType().Name, CurrentApi.ToString()));
                //
                // call the Member Function
                //
                List<object> functionArgs = new List<object>();

                // Build the parameters for the Member Function call.
                PropertyInfo[] apiProperties = CurrentApi.GetType().GetProperties();
                foreach (PropertyInfo apiPropertyInfo in apiProperties)
                {
                    object[] apiPropertyAttributes =
                        apiPropertyInfo.GetCustomAttributes(typeof(Api.MemberFunctionArgAttribute), false);
                    if (apiPropertyAttributes.Length > 0)
                    {
                        functionArgs.Add(CurrentApi.GetType().InvokeMember(apiPropertyInfo.Name,
                            BindingFlags.GetProperty, null, CurrentApi, null));
                    }
                }

                // call the member function.
                object retValue = null;
                if (chkUseUTP.Checked)
                {
                    LogTextBox.AppendText(String.Format("Calling {0}.{1}() member function...\r\n", Utp.GetType().Name, CurrentApi.ToString()));

                    retValue = Utp.GetType().InvokeMember(CurrentApi.ToString(),
                        BindingFlags.InvokeMethod | BindingFlags.Public | BindingFlags.Instance, null,
                        Utp, functionArgs.ToArray());

                    Int32 iRet = Convert.ToInt32(retValue);
                    Utils.DecimalConverterEx decConvertor = new Utils.DecimalConverterEx();
                    logStr = String.Format(" {0} - {1}\r\n", iRet == 0 ? "PASS" : "EXIT", decConvertor.ConvertToString(iRet));
                }
                else
                {
                    LogTextBox.AppendText(String.Format("Calling {0}.{1}() member function...\r\n", CurrentDevice.GetType().Name, CurrentApi.ToString()));

                    retValue = CurrentDevice.GetType().InvokeMember(CurrentApi.ToString(),
                        BindingFlags.InvokeMethod | BindingFlags.Public | BindingFlags.Instance, null,
                        CurrentDevice, functionArgs.ToArray());

                    if (CurrentApi.Direction == Api.CommandDirection.ReadWithData)
                    {
                        if (CurrentApi is Api.IProcessResponse)
                        {
                            Byte[] retBytes = null;
                            if (retValue is Byte[])
                            {
                                retBytes = retValue as Byte[];
                            }
                            else if (retValue is bool)
                            {
                                retBytes = BitConverter.GetBytes((bool)retValue);
                            }
                             
                            Api.IProcessResponse responseApi = CurrentApi as Api.IProcessResponse;
                            result = responseApi.ProcessResponse(retBytes, 0, (UInt32)retBytes.Length);
                            logStr = CurrentApi.FormatResponse(" ", "\r\n", true);
                        }
                    }
                    else
                    {
                        result = Convert.ToInt32(retValue);
                    }
                }
            }
            else
            {
                LogTextBox.AppendText(String.Format("Sending {0} command...\r\n", CurrentApi.ToString()));
                //
                // send the command
                //
                result = CurrentDevice.SendCommand(CurrentApi);
                logStr = CurrentApi.FormatResponse(" ", "\r\n", true);
            }

            // unregister for the progress events
            CurrentDevice.SendCommandProgress -= CurrentDevice_SendCommandProgress;

            // refresh the property grid to display the Response fields
            ApiPropertyGridCtrl.Refresh();

//            if (result == DevSupport.Win32.ERROR_SUCCESS)
//            {
//                logStr = CurrentApi.FormatResponse(" ", "\r\n", true) ;
//            }
//            else
//            {
//                logStr = CurrentDevice.ErrorString;
//            }

            LogTextBox.AppendText(logStr);

        }

        private 

        void CurrentDevice_SendCommandProgress(object sender, Device.SendCommandProgressArgs args)
        {
            // first time, so turn it on and set it up
            if (!ApiStatusStripProgressBar.Visible)
            {
                ApiStatusStripProgressBar.Value = Convert.ToInt32(args.Position);
                ApiStatusStripProgressBar.Visible = true;
                ApiStatusStripProgressBar.Maximum = (int)args.Maximum;
            }
            else
            {
                if (args.InProgress)
                {
                    ApiStatusStripProgressBar.Maximum = (int)args.Maximum;
                    ApiStatusStripProgressBar.Value = Convert.ToInt32(args.Position);
                }
                else
                {
                    ApiStatusStripProgressBar.Visible = false;

                    if (args.Error == Api.CmdSuccess)
                    {
                        ApiStatusStripTextCtrl.Text = args.Name + ": Done.";
                    }
                    else
                    {
                        ApiStatusStripTextCtrl.Text = args.Name + ": Error!";
                    }
                }
            }
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            // Add "About..." menu item to system menu.
            IntPtr sysMenuHandle = Win32.GetSystemMenu(this.Handle, false);
            Win32.AppendMenu(sysMenuHandle, Win32.MF_SEPARATOR, 0, string.Empty);
            Win32.AppendMenu(sysMenuHandle, Win32.MF_STRING, Win32.IDM_ABOUT, "About " + this.Text + "...");

            // Create an instance of the DeviceManager
            try
            {
                DeviceManager.Instance.DeviceChanged += new DeviceManager.DeviceChangedEventHandler(DeviceManager_DeviceChanged);
//                DeviceManager.Instance.DeviceChangedFiltered += new DeviceManager.DeviceChangedEventFiltered(OnDeviceChangedNotify);
            }
            catch (Exception err)
            {
                Trace.WriteLine(err.Message);
            }

            // Init the RejectAutoPlay menu item with the saved value from the registry.
            // Default to Checked if not present. 
            MainMenuItem_File_RejectAutoPlay.Checked = (FormPersistence.Profile.ReadInt("RejectAutoPlayCheckState", 1) == 1);
            if (MainMenuItem_File_RejectAutoPlay.Checked)
            {
                // Turn on the CancelAutoPlay functionality and add EventHandler to update UI when 
                // AutoPlay events arre rejected.
                DeviceManager.Instance.CancelAutoPlay += new CancelAutoPlayEventHandler(DeviceManager_CancelAutoPlay);
                //
                // The list of Volumes can be changed via the CancelAutoPlayDriveList property.
                // DeviceManager.Instance.CancelAutoPlayDriveList = "DEF";
                //
                // Allow AutoPlay functionality by calling:
                // DeviceManager.Instance.CancelAutoPlay -= DeviceManager_CancelAutoPlay;
            }

            // Init the TrackUsbPort menu item with the saved value from the registry.
            // Default to Checked if not present. 
            MainMenuItem_File_TrackUsbPort.Checked = (FormPersistence.Profile.ReadInt("TrackUsbPortCheckState", 1) == 1);
            LastPort = new KeyValuePair<string, int>(FormPersistence.Profile.ReadString("Hub Name", ""), FormPersistence.Profile.ReadInt("Hub Index", -1));
            
            // Attach the Device Menu to the main menu Device item
            this.MainMenuItem_Device.DropDown = this.DeviceMenuStrip;
            // Attach the Device Menu ImageList
            this.DeviceMenuStrip.ImageList = DeviceManager.ImageList;
            this.StatusStripCtrl.ImageList = DeviceManager.ImageList;
            this.MainWindowMenuStrip.ImageList = DeviceManager.ImageList;

            // find any suitable devices
            FillDeviceMenu();
	        // Init the APi list box
            InitApiListCtrl();
	        
            // see if there is a device connected to the saved USB port.
            if (MainMenuItem_File_TrackUsbPort.Checked)
            {
                CurrentDevice = FindDeviceByPort(LastPort);
                if (CurrentDevice != null)
                {
                    LogTextBox.AppendText(String.Format("Track USB Port(Hub{0},Port{1}): AutoSelect \"{2}\".\r\n", CurrentDevice.UsbHub.Index, CurrentDevice.UsbPort, CurrentDevice.ToString()));
                }

                CurrentApi = LoadDeviceApis(CurrentDevice);
                UpdateStatus();
            }
        }

        private void MainWindow_FormClosing(object sender, FormClosingEventArgs e)
        {
            DeviceManager.Instance.Dispose();
        }

        void DeviceManager_DeviceChanged(DeviceChangedEventArgs e)
        {
            Trace.WriteLine(String.Format("*** MainWindow.DeviceManager_DeviceChanged(): {0}: {1}, {2}({3})", e.Event, e.DeviceId, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
            String logStr = e.Event + ": " + e.DeviceId + "\r\n"; ;
            LogTextBox.AppendText(logStr);

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
                    CurrentApi = LoadDeviceApis(CurrentDevice);
                }
            }
            else if ( MainMenuItem_File_TrackUsbPort.Checked )
            {
                CurrentDevice = FindDeviceByPort(LastPort);
                CurrentApi = LoadDeviceApis(CurrentDevice);

                if (CurrentDevice != null)
                {
                    logStr = String.Format("Track USB Port(Hub{0},Port{1}): AutoSelect \"{2}\".\r\n", CurrentDevice.UsbHub.Index, CurrentDevice.UsbPort, CurrentDevice.ToString());
                    LogTextBox.AppendText(logStr);
                }
            }

            UpdateStatus();
        }

        private void DeviceManager_CancelAutoPlay(Object args)
        {
            CancelAutoPlayEventArgs capeArgs = args as CancelAutoPlayEventArgs;

            String logStr = String.Format("Rejected \"{0}\" AutoPlay event for device: {1}( {2} )\r\n", capeArgs.Type, capeArgs.Label, capeArgs.Path);
            Trace.WriteLine(String.Format("*** MainWindow.OnCancelAutoPlayNotify(): {0}, {1}({2})", logStr, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

            LogTextBox.AppendText(logStr);
        }

        private void FillDeviceMenu()
        {
            DeviceMenuStrip.Items.Clear();

            // Add WinUsb devices
            WinUsbDeviceClass winUsbDeviceClass = WinUsbDeviceClass.Instance;
            foreach (Device winUsbDev in winUsbDeviceClass.Devices)
            {
                ToolStripMenuItem menuItem = new ToolStripMenuItem(winUsbDev.ToString());
                menuItem.Name = winUsbDev.DeviceInstanceId;
                menuItem.ImageIndex = winUsbDev.ClassIconIndex;
                menuItem.Tag = winUsbDev;

                if (DeviceMenuStrip.Items.Contains(menuItem) == false)
                    DeviceMenuStrip.Items.Add(menuItem);
            }

            // Add HID devices
            HidDeviceClass hidDeviceClass = HidDeviceClass.Instance;
            foreach (Device hidDev in hidDeviceClass.Devices)
            {
                ToolStripMenuItem menuItem = new ToolStripMenuItem(hidDev.ToString());
                menuItem.Name = hidDev.DeviceInstanceId;
                menuItem.ImageIndex = hidDev.ClassIconIndex;
                menuItem.Tag = hidDev;

                if (DeviceMenuStrip.Items.Contains(menuItem) == false)
                    DeviceMenuStrip.Items.Add(menuItem);
            }

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

        private void InitApiLists()
        {
            
            LastApis = new Dictionary<String,Dictionary<String, Api>>();
            LastApis[typeof(Volume).Name] = new Dictionary<String, Api>();
            LastApis[typeof(Volume).Name]["ScsiApiList"] = null;
            LastApis[typeof(Volume).Name]["UtpProtocolList"] = null;
            LastApis[typeof(HidDevice).Name] = new Dictionary<String, Api>();
            LastApis[typeof(HidDevice).Name]["HidApiList"] = null;
            LastApis[typeof(WpdDevice).Name] = new Dictionary<String, Api>();
            LastApis[typeof(WpdDevice).Name]["WpdApiList"] = null;
            LastApis[typeof(RecoveryDevice).Name] = new Dictionary<String, Api>();
            LastApis[typeof(RecoveryDevice).Name]["RecoveryApiList"] = null;
            LastApis[typeof(WinUsbDevice).Name] = new Dictionary<String, Api>();
            LastApis[typeof(WinUsbDevice).Name]["WinUsbApiList"] = null;

            HidApiList = new List<Api>();

            HidApiList.Add(new HidApi.Inquiry(HidApi.Inquiry.InfoPageType.Chip, 0x01234567));
            HidApiList.Add(new HidApi.DevicePowerDown());
            HidApiList.Add(new HidApi.DeviceReset());
            HidApiList.Add(new HidApi.RequestSense());
            HidApiList.Add(new HidApi.DownloadFw("updater.sb"));
            HidApiList.Add(new HidApi.PitcRequestSense());
            HidApiList.Add(new HidApi.PitcTestUnitReady());
            HidApiList.Add(new HidApi.PitcInquiry(HidApi.PitcInquiry.InfoPageType.OtpReg, 0));
            HidApiList.Add(new HidApi.PitcRead(0, 1, 0, 4));
            HidApiList.Add(new HidApi.PitcWrite(0, 1, 4, null));

            if (WpdDeviceClass.Instance.IsSupported)
            {
                WpdApiList = new List<Api>();

                WpdApiList.Add(new WpdApi.DeviceReset());
                WpdApiList.Add(new WpdApi.EraseBootmanager());
                WpdApiList.Add(new WpdApi.GetDriveInfo(LogicalDrive.Tag.FirmwareImg, ScsiVendorApi.LogicalDriveInfo.ComponentVersion));
                WpdApiList.Add(new WpdApi.ResetToRecovery());
                WpdApiList.Add(new WpdApi.ResetToUpdater());
                WpdApiList.Add(new WpdApi.SetUpdateFlag());
                WpdApiList.Add(new WpdApi.SwitchToMsc());
            }
            ScsiApiList = new List<Api>();

            ScsiApiList.Add(new ScsiVendorApi.WriteDrive(LogicalDrive.Tag.DataSettings, "settings.bin"));
//            ScsiApiList.Add(new ScsiVendorApi.WriteAllFwDrives("firmware.sb"));
            ScsiApiList.Add(new ScsiVendorApi.ReadDrive(LogicalDrive.Tag.DataSettings));
            ScsiApiList.Add(new ScsiVendorApi.DumpDrive(LogicalDrive.Tag.DataSettings));
            ScsiApiList.Add(new ScsiVendorApi.VerifyDrive(LogicalDrive.Tag.DataSettings, "settings.bin"));
//            ScsiApiList.Add(new ScsiVendorApi.WriteJanusHeader());
//            ScsiApiList.Add(new ScsiVendorApi.FormatDataDrive(LogicalDrive.Tag.Data, DevSupport.FormatBuilder.FormatInfo.FileSystemType.Default, "SigmaTel", false));

            ScsiApiList.Add(new ScsiFormalApi.Inquiry());
            ScsiApiList.Add(new ScsiFormalApi.ReadCapacity());
            ScsiApiList.Add(new ScsiFormalApi.Read(0x800, 0, 1));
            ScsiApiList.Add(new ScsiFormalApi.Write(0x800, 1, 1, new Byte[0x800]));
            ScsiApiList.Add(new ScsiFormalApi.ModeSense10());
            ScsiApiList.Add(new ScsiFormalApi.StartStop(true, 0, false, false));

            ScsiApiList.Add(new ScsiVendorApi.GetProtocolVersion());
            ScsiApiList.Add(new ScsiVendorApi.GetStatus());
            ScsiApiList.Add(new ScsiVendorApi.GetLogicalMediaInfo(ScsiVendorApi.LogicalMediaInfo.NumberOfDrives, 32));
            ScsiApiList.Add(new ScsiVendorApi.GetAllocationTable());
            ScsiApiList.Add(new ScsiVendorApi.GetAllocationTableEx());
            MediaAllocationCmdEntry[] driveArray = new MediaAllocationCmdEntry[6];
            driveArray[0] = new MediaAllocationCmdEntry(LogicalDrive.Type.System, LogicalDrive.Tag.FirmwareImg, 5242880);
            driveArray[1] = new MediaAllocationCmdEntry(LogicalDrive.Type.Data, LogicalDrive.Tag.Data, 0);
            driveArray[2] = new MediaAllocationCmdEntry(LogicalDrive.Type.HiddenData, LogicalDrive.Tag.DataJanus, 0);
            driveArray[3] = new MediaAllocationCmdEntry(LogicalDrive.Type.HiddenData, LogicalDrive.Tag.DataSettings, 0);
            driveArray[4] = new MediaAllocationCmdEntry(LogicalDrive.Type.System, LogicalDrive.Tag.FirmwareImg2, 5242880);
            driveArray[5] = new MediaAllocationCmdEntry(LogicalDrive.Type.System, LogicalDrive.Tag.FirmwareImg3, 5242880);
            ScsiApiList.Add(new ScsiVendorApi.SetAllocationTable(driveArray));
            ScsiApiList.Add(new ScsiVendorApi.EraseLogicalMedia(false));
            ScsiApiList.Add(new ScsiVendorApi.GetLogicalDriveInfo(3, ScsiVendorApi.LogicalDriveInfo.ComponentVersion, 32));
            ScsiApiList.Add(new ScsiVendorApi.GetLogicalDriveInfo(LogicalDrive.Tag.FirmwareImg, ScsiVendorApi.LogicalDriveInfo.ComponentVersion, 32));
            ScsiApiList.Add(new ScsiVendorApi.ReadLogicalDriveSector(3, 0x800, 0, 1));
            ScsiApiList.Add(new ScsiVendorApi.ReadLogicalDriveSector(LogicalDrive.Tag.FirmwareImg, 0x800, 0, 1));
            ScsiApiList.Add(new ScsiVendorApi.WriteLogicalDriveSector(3, 0x800, 0, 1, new Byte[0x800]));
            ScsiApiList.Add(new ScsiVendorApi.WriteLogicalDriveSector(LogicalDrive.Tag.FirmwareImg3, 0x800, 0, 1, new Byte[0x800]));
            ScsiApiList.Add(new ScsiVendorApi.EraseLogicalDrive(2));
            ScsiApiList.Add(new ScsiVendorApi.EraseLogicalDrive(LogicalDrive.Tag.FirmwareImg3));
            ScsiApiList.Add(new ScsiVendorApi.ChipReset());
            ScsiApiList.Add(new ScsiVendorApi.GetChipSerialNumberInfo());
            ScsiApiList.Add(new ScsiVendorApi.FlushLogicalDrive(2));
            ScsiApiList.Add(new ScsiVendorApi.FlushLogicalDrive(LogicalDrive.Tag.FirmwareImg3));
            ScsiApiList.Add(new ScsiVendorApi.GetPhysicalMediaInfo());
            ScsiApiList.Add(new ScsiVendorApi.ReadPhysicalMediaSector(0, 0x840, 0, 1));
            ScsiApiList.Add(new ScsiVendorApi.GetChipMajorRevId());
            ScsiApiList.Add(new ScsiVendorApi.GetChipPartRevId());
            ScsiApiList.Add(new ScsiVendorApi.GetRomRevId());
            ScsiApiList.Add(new ScsiVendorApi.GetJanusStatus());
            ScsiApiList.Add(new ScsiVendorApi.InitializeJanus());
            ScsiApiList.Add(new ScsiVendorApi.ResetToRecovery());
            ScsiApiList.Add(new ScsiVendorApi.InitializeDataStore(0));
            ScsiApiList.Add(new ScsiVendorApi.ResetToUpdater());
            ScsiApiList.Add(new ScsiVendorApi.GetDeviceProperties(ScsiVendorApi.GetDeviceProperties.DevicePropertiesType.PhysicalExternalRamSize));
            ScsiApiList.Add(new ScsiVendorApi.SetUpdateFlag());
            ScsiApiList.Add(new ScsiVendorApi.FilterPing());

            RecoveryApiList = new List<Api>();
            RecoveryApiList.Add(new RecoveryApi.DownloadFile("updater36.sb"));

            WinUsbApiList = new List<Api>();
            WinUsbApiList.Add(new RecoveryApi.DownloadFile("updater36.sb"));
            WinUsbApiList.Add(new ScsiVendorApi.GetProtocolVersion());

            UtpProtocolList = new List<Api>();
            UtpProtocolList.Add(new ScsiUtpApi.UtpCommand("w"));
//            UtpProtocolList.Add(new ScsiUtpApi.UtpReset(UpdateTransportProtocol.ResetType.ChipReset));
            UtpProtocolList.Add(new ScsiUtpApi.UtpRead("r50", "test.sb"));
            UtpProtocolList.Add(new ScsiUtpApi.UtpWrite("w50", "firmware.sb"));
            UtpProtocolList.Add(new ScsiUtpMsg.Poll(1, 0x55aa));
            UtpProtocolList.Add(new ScsiUtpMsg.Exec(2, 10, "This is a test command."));
            UtpProtocolList.Add(new ScsiUtpMsg.Get(3, 0x3344, 32));
            UtpProtocolList.Add(new ScsiUtpMsg.Put(4, 0x5566, new Byte[32]));
//            UtpProtocolList.Add(new ScsiUtpMsg.Reset(1, 1));

        }

        private Api LoadDeviceApis(Device device)
        {
            Api selectedApi = null;

            ApiListCtrl.BeginUpdate();

            ApiListCtrl.Items.Clear();

            List<Api> apiList = new List<Api>();
            
            if ( device is HidDevice )
            {
                apiList = HidApiList;
                LastProtocol = "HidApiList";
            }
            else if (device is WpdDevice)
            {
                apiList = WpdApiList;
                LastProtocol = "WpdApiList";
            }
            else if (device is Volume)
            {
                if (chkUseUTP.Checked == true)
                {
                    apiList = UtpProtocolList;
                    LastProtocol = "UtpProtocolList";
                }
                else
                {
                    apiList = ScsiApiList;
                    LastProtocol = "ScsiApiList";
                }
            }
            else if (device is RecoveryDevice)
            {
                apiList = RecoveryApiList;
                LastProtocol = "RecoveryApiList";
            }
            else if (device is WinUsbDevice)
            {
                apiList = WinUsbApiList;
                LastProtocol = "WinUsbApiList";
            }
            else
            {
                // we're outta here
                ApiListCtrl.EndUpdate();
                return selectedApi;
            }

            // Now add them to the control
            foreach (Api api in apiList)
            {
                ListViewItem item = new ListViewItem();
                item.Name = item.Text = api.ToString();
                item.ImageKey = api.ImageKey;
                item.Tag = api;

                // Tooltip text of the form "apiName(paramType paramName, ...)"
                String paramStr = String.Empty;
                ConstructorInfo[] constInfoArr = api.GetType().GetConstructors();
                for (int i = 0; i < constInfoArr.Length; ++i)
                {
                    ParameterInfo[] paramInfoArr = constInfoArr[i].GetParameters();
                    for (int j = 0; j < paramInfoArr.Length; ++j)
                    {
                        ParameterInfo param = paramInfoArr[j];
                        paramStr += String.Format("{0} {1}{2}", param.ParameterType.Name, param.Name, j + 1 == paramInfoArr.Length ? "" : ", ");
                    }
                    item.ToolTipText += String.Format("{0}({1}){2}", api.ToString(), paramStr, i + 1 == constInfoArr.Length ? "" : "\r\n");
                    paramStr = String.Empty;
                }
                ApiListCtrl.Items.Add(item);
            }

            // select the last api selected for this device type
            Api lastApi = LastApis[device.GetType().Name][LastProtocol];
            if (lastApi != null)
            {
                ApiListCtrl.Items[ApiListCtrl.Items.IndexOfKey(lastApi.ToString())].Selected = true;
                selectedApi = (Api)ApiListCtrl.Items[ApiListCtrl.Items.IndexOfKey(lastApi.ToString())].Tag;
            }
            
            ApiListCtrl.EndUpdate();

            return selectedApi;
        }

        private void InitApiListCtrl()
        {
            ApiListCtrl.BeginUpdate();

            // Add a colum to the control
            ApiListCtrl.Columns.Add("Name", 
                ApiListCtrl.ClientRectangle.Width - SystemInformation.VerticalScrollBarWidth,
                HorizontalAlignment.Left);

            // Various ListView properties. MultiSelect, ShowToolTipItems, ImageList.
            ApiListCtrl.MultiSelect = false;
            ApiListCtrl.ShowItemToolTips = true;
            ApiListCtrl.SmallImageList = Api.ImageList;

            ApiListCtrl.EndUpdate();

            InitApiLists();
        }

        private void ApiPropertyGridCtrl_SelectedGridItemChanged(object sender, SelectedGridItemChangedEventArgs e)
        {
            ApiPropertyGridCtrl.Refresh();
        }

        private Device FindDeviceByPort(KeyValuePair<string, int> lastPort)
        {
            Device returnDevice = null;

            foreach (ToolStripMenuItem mi in DeviceMenuStrip.Items)
            {
                Device device = (Device)mi.Tag;
                
                if (String.Compare(device.UsbHub.Path, lastPort.Key, true) == 0)
                {
                    if (device.UsbPort == lastPort.Value)
                    {
                        returnDevice = device;
                        break;
                    }
                }
            }

            return returnDevice;
        }

        private void chkUseUTP_CheckedChanged(object sender, EventArgs e)
        {
            CurrentApi = LoadDeviceApis(CurrentDevice);
            if (chkUseUTP.Checked == true)
            {
                Utp = new UpdateTransportProtocol(CurrentDevice as Volume);
                utpGrid.SelectedObject = Utp;
                utpGrid.Visible = true;
            }
            else
            {
                utpGrid.Visible = false;
                Utp.Dispose();
                Utp = null;
            }
        }


/*        
        private void UpdateCommandParams()
        {
            String str;

            // Command Data
            FormatCDB();
        	
            // CDB size
//            if ( IsCustomCommand() )
//	            str = "16 bytes (max)";
//            else
//	            str.Format("{0} bytes", CurrentApi.GetCdbSize());
//            _cdbSizeCtrl.SetWindowText(str);

            // Read/Write
            if ( IsCustomCommand() )
            {
	            // allow the user to select read or write for custom commands
	            ReadRadioBtn.Enabled = true;
	            WriteRadioBtn.Enabled = true;
            }
            else
            {
	            ReadRadioBtn.Enabled = false;
	            WriteRadioBtn.Enabled = false;
            }
            ReadRadioBtn.Checked = ( CurrentApi.Direction == Api.CmdDirection.Read );
            WriteRadioBtn.Checked = !ReadRadioBtn.Checked;

            // Response size
            if ( IsCustomCommand() )
            {
	            // allow the user to specify the response size for custom commands
	            CmdSizeCtrl.ReadOnly = false;
            }
            else
            {
	            CmdSizeCtrl.ReadOnly = true;
            }
            CmdSizeCtrl.Text = String.Format("0x{0:X}", CurrentApi.TransferSize);

            // Response
            ResponseCtrl.Text = "";

            // Status
            UpdateStatus();
        }

        private bool IsCustomCommand()
        {
            if ( String.Compare(CurrentApi.ToString(), "CUSTOM", true) == 0 )
	            return true;
            else
	            return false;
        }

        private void FormatCDB()
        {
            // get the cursor position

            String str = "";
            Byte[] bytes = CurrentApi.Cdb.GetBytes();
            
            int index;
            for (index = 0; index < CurrentApi.Cdb.GetBytes().Length; ++index)
            {
	            str += String.Format("{0:02X} ", bytes[index]);
	            if ( (index+1)%8 == 0 )
                {
		            str.Remove(str.Length-1);
		            str += "\r\n";
	            }
            }
            if ( (index)%8 == 0 )
	            str.Remove(str.Length-2);

            CmdTextCtrl.Text = str;

            // put the cursor back
        }
*/
/*
        private void ParseCDB()
        {
            String str = CmdTextCtrl.Text;
            str.Replace(' ','');
            str.Remove(_T('\r'));
            str.Remove(_T('\n'));
            str.Remove(_T('\t'));

            uint32_t buffer;
            uint8_t * cdbBuf = (uint8_t *)_currentApi->GetCdbPtr();
            for (int i=0; i<str.GetLength(); i+=2)
            {
	            _stscanf_s(str.Mid(i, 2).GetBuffer(), _T("%02x"), &buffer);
	            *cdbBuf = (uint8_t)buffer;
	            ++cdbBuf;
            }
        }
*/
    }
}
