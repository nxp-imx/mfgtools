/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Resources;
using System.Text;
using System.Threading;
using System.Windows.Forms;

using System.Runtime.InteropServices;

using DevSupport;
using DevSupport.DeviceManager;
using DevSupport.Api;

//using DevSupport;
//using DevSupport.Api;
//using DevSupport.DeviceManager;
using DevSupport.DeviceManager.UTP;
using DevSupport.Media;

namespace DeviceEnum
{
    public partial class MainWindow : Form
    {
        //        private bool _Loading;
        private Utils.FormPersistence formPersistence;
        private Utils.ToolStripRadioButtonMenuItem viewDeviceTypeToolStripMenuItem =
            new Utils.ToolStripRadioButtonMenuItem("View by device type");
        private Utils.ToolStripRadioButtonMenuItem viewUSBConnectionToolStripMenuItem =
            new Utils.ToolStripRadioButtonMenuItem("View by USB connection");

        public MainWindow()
        {
            Thread.CurrentThread.Name = "MainWindow thread";
            Trace.TraceInformation("MainWindow() - {0}({1})", Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode());

            InitializeComponent();

            HidApi.Inquiry api = new HidApi.Inquiry(HidApi.Inquiry.InfoPageType.Chip, 0xabcd);

            this.viewToolStripMenuItem.DropDownItems.Insert(0, viewDeviceTypeToolStripMenuItem);
            this.viewToolStripMenuItem.DropDownItems.Insert(1, viewUSBConnectionToolStripMenuItem);
            viewDeviceTypeToolStripMenuItem.RadioButtonSelected += new Utils.ToolStripRadioButtonMenuItem.RadioButtonSelectedEvent(OnViewByDeviceTypeSelected);
            viewUSBConnectionToolStripMenuItem.RadioButtonSelected += new Utils.ToolStripRadioButtonMenuItem.RadioButtonSelectedEvent(OnViewByUSBConnectionSelected);

            formPersistence = new Utils.FormPersistence(this, @"Freescale\DeviceEnum\Settings");

            // Setup the TreeView ImageList
            DeviceManager.AttachImageList(treeViewDevices);
//            treeViewDevices.ImageList = DeviceManager.ImageList;

            // Init the RejectAutoPlay checkbox with the saved value from the registry.
            // Default to Checked if not present. 
            checkBoxCancelAutoPlay.CheckState = (CheckState)formPersistence.Profile.ReadInt("RejectAutoPlayCheckState", (int)CheckState.Checked);
            if (checkBoxCancelAutoPlay.CheckState == CheckState.Checked)
            {
                // Turn on the CancelAutoPlay functionality and add EventHandler to update UI when 
                // AutoPlay events are rejected.
                DeviceManager.Instance.CancelAutoPlay += new CancelAutoPlayEventHandler(OnCancelAutoPlayNotify);
                //
                // The list of Volumes can be changed via the CancelAutoPlayDriveList property.
                // DeviceManager.Instance.CancelAutoPlayDriveList = "DEF";
                //
                // Allow AutoPlay functionality by calling:
                // DeviceManager.Instance.CancelAutoPlay -= OnCancelAutoPlayNotify;
            }

            // Create an instance of the DeviceManager
            try
            {
                DeviceManager.Instance.DeviceChanged += new DeviceManager.DeviceChangedEventHandler(DeviceManager_DeviceChange);
                // DeviceManager.Instance.DeviceChangedFiltered += new DeviceManager.DeviceChangedEventFiltered(DeviceManager_DeviceChange);
            }
            catch (Exception e)
            {
                Trace.WriteLine(e.Message);
            }

            labelOS.Text = String.Format("OS: {0}", Environment.OSVersion);
            viewDeviceTypeToolStripMenuItem.CheckState = (CheckState)formPersistence.Profile.ReadInt("ViewByDeviceTypeMenuItemCheckedState", (int)CheckState.Checked);
            viewUSBConnectionToolStripMenuItem.CheckState = (CheckState)formPersistence.Profile.ReadInt("ViewUSBConnectionMenuItemCheckedState", (int)CheckState.Unchecked);
        }

        void DeviceManager_DeviceChange(DeviceChangedEventArgs e)
        {
            Trace.WriteLine(String.Format("*** MainWindow.DeviceManager_DeviceChange(): {0}: {1}, {2}({3})", e.Event, e.DeviceId, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));
            String msg = e.Event + ": " + e.DeviceId + "\r\n"; ;
//            String msg = e.Event + ": " + e.DeviceId + " (" + e.Device + ")\r\n"; ;
            textBoxMessages.AppendText(msg);

/*            if (e.Device != null)
            {
                switch (e.Event)
                {
                    case DeviceChangeEvent.DeviceArrival:
                    case DeviceChangeEvent.VolumeArrival:
                    case DeviceChangeEvent.HubArrival:
                        AddDeviceToTree(e.Device);
                        break;
                    case DeviceChangeEvent.DeviceRemoval:
                    case DeviceChangeEvent.VolumeRemoval:
                    case DeviceChangeEvent.HubRemoval:
                        RemoveDeviceFromTree(e.Device.Driver);
                        break;
                    default:
                        break;
                }
            }
*/        }

        public void RemoveDeviceFromTree(String deviceDriver)
        {
            TreeNode[] nodes = treeViewDevices.Nodes.Find(deviceDriver, true);
            foreach (TreeNode node in nodes)
            {
                if (node.Tag is UsbPort)
                {
                    RefreshUsbPortNode(node);
                }
                else
                {
                    treeViewDevices.Nodes.Remove(node);
                }
            }
        }

        void RefreshUsbPortNode(TreeNode node)
        {
            // save the insertion point (hubNode)
            TreeNode hubNode = node.Parent;
            
            // Remove the curent port
            treeViewDevices.Nodes.Remove(node);

            // Insert the new node
            InsertHubPort((UsbPort)node.Tag, hubNode);
        }

        public void AddDeviceToTree(Device device)
        {
            if (viewDeviceTypeToolStripMenuItem.CheckState == CheckState.Checked)
            {
                Type deviceType = device.GetType();

                TreeNode deviceNode = new TreeNode(device.ToString());
                deviceNode.ImageIndex = device.ClassIconIndex;
                deviceNode.SelectedImageIndex = deviceNode.ImageIndex;
                deviceNode.Tag = device;
                deviceNode.Name = device.Driver;

                // The DeviceClass nodes are contained by the TopLevel Computer node.
                TreeNodeCollection deviceClassNodes = treeViewDevices.Nodes[0].Nodes;

                if (deviceType == typeof(RecoveryDevice))
                {
                    int recClassIndex = deviceClassNodes.IndexOfKey(RecoveryDeviceClass.Instance.Description);
                    if (recClassIndex != -1)
                    {
                        deviceClassNodes[recClassIndex].Nodes.Add(deviceNode);
                        deviceNode.EnsureVisible();
                    }
                }
                else if (deviceType == typeof(MxRomDevice))
                {
                    int mxClassIndex = deviceClassNodes.IndexOfKey(MxRomDeviceClass.Instance.Description);
                    if (mxClassIndex != -1)
                    {
                        deviceClassNodes[mxClassIndex].Nodes.Add(deviceNode);
                        deviceNode.EnsureVisible();
                    }
                }
                else if (deviceType == typeof(HidDevice))
                {
                    int hidClassIndex = deviceClassNodes.IndexOfKey(HidDeviceClass.Instance.Description);
                    if (hidClassIndex != -1)
                    {
                        deviceClassNodes[hidClassIndex].Nodes.Add(deviceNode);
                        deviceNode.EnsureVisible();
                    }
                }
                else if (deviceType == typeof(WpdDevice))
                {
                    int wpdClassIndex = deviceClassNodes.IndexOfKey(WpdDeviceClass.Instance.Description);
                    if (wpdClassIndex != -1)
                    {
                        deviceClassNodes[wpdClassIndex].Nodes.Add(deviceNode);
                        deviceNode.EnsureVisible();
                    }
                }
/*                else if (deviceType == typeof(ScsiDevice))
                {
                    int scsiClassIndex = deviceClassNodes.IndexOfKey(ScsiDeviceClass.Instance.Description);
                    if (scsiClassIndex != -1)
                    {
                        deviceClassNodes[scsiClassIndex].Nodes.Add(deviceNode);
                        deviceNode.EnsureVisible();
                    }
                }
*/                else if (deviceType == typeof(Volume))
                {
                    int volClassIndex = deviceClassNodes.IndexOfKey(VolumeDeviceClass.Instance.Description);
                    if (volClassIndex != -1)
                    {
                        deviceClassNodes[volClassIndex].Nodes.Add(deviceNode);
                        deviceNode.EnsureVisible();

                        foreach (Device disk in ((Volume)device).Disks)
                        {
                            TreeNode diskNode = deviceNode.Nodes.Add(disk.ToString());
                            diskNode.ImageIndex = disk.ClassIconIndex;
                            diskNode.SelectedImageIndex = diskNode.ImageIndex;
                            diskNode.Tag = disk;

                            diskNode.EnsureVisible();
                        }
                    }
                }
            }
            else
            {
                // View by USB connection.
                TreeNode[] nodes = treeViewDevices.Nodes.Find(device.UsbHub[device.UsbPort].ToString("ID",null), true);
                foreach (TreeNode node in nodes)
                {
                    RefreshUsbPortNode(node);
                }
            }
        }

        private void LoadDevices()
        {
            treeViewDevices.BeginUpdate();

            propertyGridDevice.SelectedObject = null;
            treeViewDevices.Nodes.Clear();

            // make the Computer root node
            ComputerDeviceClass computerDeviceClass = ComputerDeviceClass.Instance;
            TreeNode computerNode = new TreeNode(computerDeviceClass.ToString());
            computerNode.ImageIndex = computerDeviceClass.ClassIconIndex;
            computerNode.SelectedImageIndex = computerNode.ImageIndex;
            computerNode.Tag = computerDeviceClass;
            computerNode.Name = computerNode.Text;
            treeViewDevices.Nodes.Add(computerNode);

            // display Recovery-mode devices
            RecoveryDeviceClass recoveryDeviceClass = RecoveryDeviceClass.Instance;
            TreeNode recoveryDevNode = new TreeNode(recoveryDeviceClass.ToString());
            recoveryDevNode.ImageIndex = recoveryDeviceClass.ClassIconIndex;
            recoveryDevNode.SelectedImageIndex = recoveryDevNode.ImageIndex;
            recoveryDevNode.Tag = recoveryDeviceClass;
            recoveryDevNode.Name = recoveryDevNode.Text;
            computerNode.Nodes.Add(recoveryDevNode);

            foreach (Device recoveryDev in recoveryDeviceClass.Devices)
            {
                AddDeviceToTree(recoveryDev);
            }

            // display Mx ROM devices
            MxRomDeviceClass mxRomDeviceClass = MxRomDeviceClass.Instance;
            TreeNode mxRomDevNode = new TreeNode(mxRomDeviceClass.ToString());
            mxRomDevNode.ImageIndex = mxRomDeviceClass.ClassIconIndex;
            mxRomDevNode.SelectedImageIndex = mxRomDevNode.ImageIndex;
            mxRomDevNode.Tag = mxRomDeviceClass;
            mxRomDevNode.Name = mxRomDevNode.Text;
            computerNode.Nodes.Add(mxRomDevNode);

            foreach (MxRomDevice mxRomDev in mxRomDeviceClass.Devices)
            {
                AddDeviceToTree(mxRomDev);
            }

            // display HID devices
            HidDeviceClass hidDeviceClass = HidDeviceClass.Instance;
            TreeNode hidDevNode = new TreeNode(hidDeviceClass.ToString());
            hidDevNode.ImageIndex = hidDeviceClass.ClassIconIndex;
            hidDevNode.SelectedImageIndex = hidDevNode.ImageIndex;
            hidDevNode.Tag = hidDeviceClass;
            hidDevNode.Name = hidDevNode.Text;
            computerNode.Nodes.Add(hidDevNode);

            foreach (Device hidDev in hidDeviceClass.Devices)
            {
                AddDeviceToTree(hidDev);
            }

            // display SCSI devices
/*            ScsiDeviceClass scsiDeviceClass = ScsiDeviceClass.Instance;
            TreeNode scsiDevNode = new TreeNode(scsiDeviceClass.ToString());
            scsiDevNode.ImageIndex = scsiDeviceClass.ClassIconIndex;
            scsiDevNode.SelectedImageIndex = scsiDevNode.ImageIndex;
            scsiDevNode.Tag = scsiDeviceClass;
            scsiDevNode.Name = scsiDevNode.Text;
            computerNode.Nodes.Add(scsiDevNode);

            foreach (Device scsiDev in scsiDeviceClass.Devices)
            {
                AddDeviceToTree(scsiDev);
            }
*/
            // display volumes
            VolumeDeviceClass volumeDeviceClass = VolumeDeviceClass.Instance;
            TreeNode volumesNode = new TreeNode(volumeDeviceClass.ToString());
            volumesNode.ImageIndex = (int)volumeDeviceClass.ClassIconIndex;
            volumesNode.SelectedImageIndex = volumesNode.ImageIndex;
            volumesNode.Tag = volumeDeviceClass;
            volumesNode.Name = volumesNode.Text;
            computerNode.Nodes.Add(volumesNode);

            foreach (Volume volume in volumeDeviceClass.Devices)
            {
                AddDeviceToTree(volume);
            }

            // display WPD devices
            if (WpdDeviceClass.Instance.IsSupported)
            {
                WpdDeviceClass wpdDeviceClass = WpdDeviceClass.Instance;
                TreeNode wpdDevNode = new TreeNode(wpdDeviceClass.ToString());
                wpdDevNode.ImageIndex = wpdDeviceClass.ClassIconIndex;
                wpdDevNode.SelectedImageIndex = wpdDevNode.ImageIndex;
                wpdDevNode.Tag = wpdDeviceClass;
                wpdDevNode.Name = wpdDevNode.Text;
                computerNode.Nodes.Add(wpdDevNode);

                foreach (WpdDevice wpdDev in wpdDeviceClass.Devices)
                {
                    AddDeviceToTree(wpdDev);
                }
            }
            treeViewDevices.ExpandAll();

            treeViewDevices.EndUpdate();
        }

        private void LoadUsbTree()
        {
            propertyGridDevice.SelectedObject = null;
            treeViewDevices.Nodes.Clear();

            // make the Computer root node
            ComputerDeviceClass computerDeviceClass = ComputerDeviceClass.Instance;
            TreeNode computerNode = new TreeNode(computerDeviceClass.ToString());
            computerNode.ImageIndex = computerDeviceClass.ClassIconIndex;
            computerNode.SelectedImageIndex = computerNode.ImageIndex;
            computerNode.Tag = computerDeviceClass;
            treeViewDevices.Nodes.Add(computerNode);

            // Add USB Controller devices
            UsbControllerClass usbControllerClass = UsbControllerClass.Instance;
            foreach (UsbController usbController in usbControllerClass.Devices)
            {
                TreeNode usbControllerNode = new TreeNode(usbController.ToString());
                usbControllerNode.ImageIndex = usbController.ClassIconIndex;
                usbControllerNode.SelectedImageIndex = usbControllerNode.ImageIndex;
                usbControllerNode.Tag = usbController;
                computerNode.Nodes.Add(usbControllerNode);

                // Add USB Root UsbHub devices
                if (InsertHub(usbController.RootHub, usbControllerNode, null))
                {
                    computerNode.Expand();
                }

            } // foreach( USB Controller )

        } // private void LoadUsbTree()

        private bool InsertHub(UsbHub hub, TreeNode tnParent, String port)
        {
            bool hubExpanded = false;

            if (hub == null)
                throw new ArgumentNullException("hub"); ;

            // Add USB UsbHub
            TreeNode hubNode = new TreeNode(hub.ToString(port, null));
            hubNode.ImageIndex = hub.ClassIconIndex;
            hubNode.SelectedImageIndex = hubNode.ImageIndex;
            hubNode.Tag = hub;
            tnParent.Nodes.Add(hubNode);

            // Add USB Ports
            for (int index = 1; index <= hub.NumberOfPorts; ++index)
            {
                hubExpanded = InsertHubPort(hub[index], hubNode);
                if ( hubExpanded )
                    tnParent.Expand();
            
            } // for ( ports )

            return hubExpanded;
        }

        private bool InsertHubPort(UsbPort port, TreeNode hubNode)
        {
            bool hubExpanded = false;

            TreeNode portNode = new TreeNode(port.ToString());
            portNode.ImageIndex = hubNode.ImageIndex;
            portNode.SelectedImageIndex = portNode.ImageIndex;
            portNode.Name = port.ToString("ID", null);
            portNode.Tag = port;

            if (port.NodeConnectionInfo.ConnectionStatus != Win32.USB_CONNECTION_STATUS.NoDeviceConnected)
            {
                if (port.AttachedDevice != null)
                {
                    portNode.ImageIndex = port.AttachedDevice.ClassIconIndex;
                    portNode.SelectedImageIndex = portNode.ImageIndex;
                    portNode.Name = port.AttachedDevice.Driver;

                    if (port.AttachedDevice is UsbHub)
                    {
                        hubExpanded = InsertHub(port.AttachedDevice as UsbHub, hubNode/*portNode*/, port.ToString("P", null));
                    }
                    else
                    {
                        hubNode.Nodes.Add(portNode);
                    }
                }

                if (!hubExpanded)
                {
                    hubNode.Expand();
                    hubExpanded = true;
                }
            }
            else
            {
                hubNode.Nodes.Add(portNode);
            }

            return hubExpanded;
        }
 
        private void treeViewDevices_AfterSelect(object sender, TreeViewEventArgs e)
        {
            // update property grid
            try
            {
                propertyGridDevice.SelectedObject = e.Node.Tag; // device;
            }
            catch (Exception err)
            {
                Trace.WriteLine(err.Message);
            }

        }

        private void MainWindow_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Dispose of the DeviceManager
            //            DeviceManager.Instance.Close();
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void OnViewByDeviceTypeSelected()
        {
            // Save the selection to the Registry
            formPersistence.Profile.Write("ViewUSBConnectionMenuItemCheckedState", (int)viewUSBConnectionToolStripMenuItem.CheckState);
            formPersistence.Profile.Write("ViewByDeviceTypeMenuItemCheckedState", (int)viewDeviceTypeToolStripMenuItem.CheckState);
            
            LoadDevices();
        }

        private void OnViewByUSBConnectionSelected()
        {
            // Save the selection to the Registry
            formPersistence.Profile.Write("ViewUSBConnectionMenuItemCheckedState", (int)viewUSBConnectionToolStripMenuItem.CheckState);
            formPersistence.Profile.Write("ViewByDeviceTypeMenuItemCheckedState", (int)viewDeviceTypeToolStripMenuItem.CheckState);

            LoadUsbTree();
        }

        private void OnCancelAutoPlayNotify(Object args)
        {
            CancelAutoPlayEventArgs capeArgs = args as CancelAutoPlayEventArgs;

            String text = String.Format("Rejected \"{0}\" AutoPlay event for device: {1}( {2} )", capeArgs.Type, capeArgs.Label, capeArgs.Path);
            Trace.WriteLine(String.Format("*** MainWindow.OnCancelAutoPlayNotify(): {0}, {1}({2})", text, Thread.CurrentThread.Name, Thread.CurrentThread.GetHashCode()));

            textBoxMessages.AppendText(text + "\r\n");
        }

        private void checkBoxCancelAutoPlay_CheckedChanged(object sender, EventArgs e)
        {
            // Save the selection to the Registry
            formPersistence.Profile.Write("RejectAutoPlayCheckState", (int)checkBoxCancelAutoPlay.CheckState);

            if (checkBoxCancelAutoPlay.CheckState == CheckState.Checked)
            {
                DeviceManager.Instance.CancelAutoPlay += new CancelAutoPlayEventHandler(OnCancelAutoPlayNotify);
            }
            else
            {
                DeviceManager.Instance.CancelAutoPlay -= OnCancelAutoPlayNotify;
            }
        }
    }
}
