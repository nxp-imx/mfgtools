namespace DeviceEnum
{
    partial class MainWindow
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.Label labelStatus;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainWindow));
            System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator5;
            this.labelOS = new System.Windows.Forms.Label();
            this.treeViewDevices = new System.Windows.Forms.TreeView();
            this.propertyGridDevice = new System.Windows.Forms.PropertyGrid();
            this.splitContainerDevices = new System.Windows.Forms.SplitContainer();
            this.textBoxMessages = new System.Windows.Forms.TextBox();
            this.checkBoxCancelAutoPlay = new System.Windows.Forms.CheckBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.mainMenuStrip = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.volumesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mscFiltersToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.usbOnlyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mscFiltersAddToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.wpdDevicesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.wpdFiltersToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.wpdFiltersAddToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.hidToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.hidFiltersToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.hidFiltersAddToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.recoveryDeviceToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.recoveryFiltersToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.recoveryFiltersAddToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            labelStatus = new System.Windows.Forms.Label();
            toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            toolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            this.splitContainerDevices.Panel1.SuspendLayout();
            this.splitContainerDevices.Panel2.SuspendLayout();
            this.splitContainerDevices.SuspendLayout();
            this.panel1.SuspendLayout();
            this.mainMenuStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // labelStatus
            // 
            resources.ApplyResources(labelStatus, "labelStatus");
            labelStatus.Name = "labelStatus";
            // 
            // toolStripSeparator1
            // 
            toolStripSeparator1.Name = "toolStripSeparator1";
            resources.ApplyResources(toolStripSeparator1, "toolStripSeparator1");
            // 
            // toolStripSeparator2
            // 
            toolStripSeparator2.Name = "toolStripSeparator2";
            resources.ApplyResources(toolStripSeparator2, "toolStripSeparator2");
            // 
            // toolStripSeparator3
            // 
            toolStripSeparator3.Name = "toolStripSeparator3";
            resources.ApplyResources(toolStripSeparator3, "toolStripSeparator3");
            // 
            // toolStripSeparator4
            // 
            toolStripSeparator4.Name = "toolStripSeparator4";
            resources.ApplyResources(toolStripSeparator4, "toolStripSeparator4");
            // 
            // toolStripSeparator5
            // 
            toolStripSeparator5.Name = "toolStripSeparator5";
            resources.ApplyResources(toolStripSeparator5, "toolStripSeparator5");
            // 
            // labelOS
            // 
            resources.ApplyResources(this.labelOS, "labelOS");
            this.labelOS.Name = "labelOS";
            // 
            // treeViewDevices
            // 
            resources.ApplyResources(this.treeViewDevices, "treeViewDevices");
            this.treeViewDevices.HideSelection = false;
            this.treeViewDevices.Name = "treeViewDevices";
            this.treeViewDevices.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeViewDevices_AfterSelect);
            // 
            // propertyGridDevice
            // 
            this.propertyGridDevice.BackColor = System.Drawing.SystemColors.Control;
            this.propertyGridDevice.CommandsBackColor = System.Drawing.SystemColors.Control;
            resources.ApplyResources(this.propertyGridDevice, "propertyGridDevice");
            this.propertyGridDevice.Name = "propertyGridDevice";
            this.propertyGridDevice.PropertySort = System.Windows.Forms.PropertySort.Alphabetical;
            this.propertyGridDevice.ToolbarVisible = false;
            // 
            // splitContainerDevices
            // 
            resources.ApplyResources(this.splitContainerDevices, "splitContainerDevices");
            this.splitContainerDevices.Name = "splitContainerDevices";
            // 
            // splitContainerDevices.Panel1
            // 
            this.splitContainerDevices.Panel1.Controls.Add(this.treeViewDevices);
            // 
            // splitContainerDevices.Panel2
            // 
            this.splitContainerDevices.Panel2.Controls.Add(this.propertyGridDevice);
            // 
            // textBoxMessages
            // 
            resources.ApplyResources(this.textBoxMessages, "textBoxMessages");
            this.textBoxMessages.BackColor = System.Drawing.SystemColors.Window;
            this.textBoxMessages.Name = "textBoxMessages";
            this.textBoxMessages.ReadOnly = true;
            // 
            // checkBoxCancelAutoPlay
            // 
            resources.ApplyResources(this.checkBoxCancelAutoPlay, "checkBoxCancelAutoPlay");
            this.checkBoxCancelAutoPlay.Checked = true;
            this.checkBoxCancelAutoPlay.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxCancelAutoPlay.Name = "checkBoxCancelAutoPlay";
            this.checkBoxCancelAutoPlay.UseVisualStyleBackColor = true;
            this.checkBoxCancelAutoPlay.CheckedChanged += new System.EventHandler(this.checkBoxCancelAutoPlay_CheckedChanged);
            // 
            // panel1
            // 
            resources.ApplyResources(this.panel1, "panel1");
            this.panel1.Controls.Add(this.checkBoxCancelAutoPlay);
            this.panel1.Controls.Add(labelStatus);
            this.panel1.Controls.Add(this.labelOS);
            this.panel1.Controls.Add(this.textBoxMessages);
            this.panel1.Name = "panel1";
            // 
            // mainMenuStrip
            // 
            this.mainMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.viewToolStripMenuItem});
            resources.ApplyResources(this.mainMenuStrip, "mainMenuStrip");
            this.mainMenuStrip.Name = "mainMenuStrip";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            resources.ApplyResources(this.fileToolStripMenuItem, "fileToolStripMenuItem");
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            resources.ApplyResources(this.exitToolStripMenuItem, "exitToolStripMenuItem");
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // viewToolStripMenuItem
            // 
            this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            toolStripSeparator1,
            this.volumesToolStripMenuItem,
            this.wpdDevicesToolStripMenuItem,
            this.hidToolStripMenuItem,
            this.recoveryDeviceToolStripMenuItem});
            this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            resources.ApplyResources(this.viewToolStripMenuItem, "viewToolStripMenuItem");
            // 
            // volumesToolStripMenuItem
            // 
            this.volumesToolStripMenuItem.Checked = true;
            this.volumesToolStripMenuItem.CheckOnClick = true;
            this.volumesToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.volumesToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mscFiltersToolStripMenuItem,
            toolStripSeparator2,
            this.usbOnlyToolStripMenuItem,
            this.mscFiltersAddToolStripMenuItem});
            resources.ApplyResources(this.volumesToolStripMenuItem, "volumesToolStripMenuItem");
            this.volumesToolStripMenuItem.Name = "volumesToolStripMenuItem";
            // 
            // mscFiltersToolStripMenuItem
            // 
            this.mscFiltersToolStripMenuItem.CheckOnClick = true;
            this.mscFiltersToolStripMenuItem.Name = "mscFiltersToolStripMenuItem";
            resources.ApplyResources(this.mscFiltersToolStripMenuItem, "mscFiltersToolStripMenuItem");
            // 
            // usbOnlyToolStripMenuItem
            // 
            this.usbOnlyToolStripMenuItem.CheckOnClick = true;
            this.usbOnlyToolStripMenuItem.Name = "usbOnlyToolStripMenuItem";
            resources.ApplyResources(this.usbOnlyToolStripMenuItem, "usbOnlyToolStripMenuItem");
            // 
            // mscFiltersAddToolStripMenuItem
            // 
            this.mscFiltersAddToolStripMenuItem.Name = "mscFiltersAddToolStripMenuItem";
            resources.ApplyResources(this.mscFiltersAddToolStripMenuItem, "mscFiltersAddToolStripMenuItem");
            // 
            // wpdDevicesToolStripMenuItem
            // 
            this.wpdDevicesToolStripMenuItem.Checked = true;
            this.wpdDevicesToolStripMenuItem.CheckOnClick = true;
            this.wpdDevicesToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.wpdDevicesToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.wpdFiltersToolStripMenuItem,
            toolStripSeparator3,
            this.wpdFiltersAddToolStripMenuItem});
            resources.ApplyResources(this.wpdDevicesToolStripMenuItem, "wpdDevicesToolStripMenuItem");
            this.wpdDevicesToolStripMenuItem.Name = "wpdDevicesToolStripMenuItem";
            // 
            // wpdFiltersToolStripMenuItem
            // 
            this.wpdFiltersToolStripMenuItem.CheckOnClick = true;
            this.wpdFiltersToolStripMenuItem.Name = "wpdFiltersToolStripMenuItem";
            resources.ApplyResources(this.wpdFiltersToolStripMenuItem, "wpdFiltersToolStripMenuItem");
            // 
            // wpdFiltersAddToolStripMenuItem
            // 
            this.wpdFiltersAddToolStripMenuItem.Name = "wpdFiltersAddToolStripMenuItem";
            resources.ApplyResources(this.wpdFiltersAddToolStripMenuItem, "wpdFiltersAddToolStripMenuItem");
            // 
            // hidToolStripMenuItem
            // 
            this.hidToolStripMenuItem.Checked = true;
            this.hidToolStripMenuItem.CheckOnClick = true;
            this.hidToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.hidToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.hidFiltersToolStripMenuItem,
            toolStripSeparator4,
            this.hidFiltersAddToolStripMenuItem});
            resources.ApplyResources(this.hidToolStripMenuItem, "hidToolStripMenuItem");
            this.hidToolStripMenuItem.Name = "hidToolStripMenuItem";
            // 
            // hidFiltersToolStripMenuItem
            // 
            this.hidFiltersToolStripMenuItem.CheckOnClick = true;
            this.hidFiltersToolStripMenuItem.Name = "hidFiltersToolStripMenuItem";
            resources.ApplyResources(this.hidFiltersToolStripMenuItem, "hidFiltersToolStripMenuItem");
            // 
            // hidFiltersAddToolStripMenuItem
            // 
            this.hidFiltersAddToolStripMenuItem.Name = "hidFiltersAddToolStripMenuItem";
            resources.ApplyResources(this.hidFiltersAddToolStripMenuItem, "hidFiltersAddToolStripMenuItem");
            // 
            // recoveryDeviceToolStripMenuItem
            // 
            this.recoveryDeviceToolStripMenuItem.Checked = true;
            this.recoveryDeviceToolStripMenuItem.CheckOnClick = true;
            this.recoveryDeviceToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.recoveryDeviceToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.recoveryFiltersToolStripMenuItem,
            toolStripSeparator5,
            this.recoveryFiltersAddToolStripMenuItem});
            resources.ApplyResources(this.recoveryDeviceToolStripMenuItem, "recoveryDeviceToolStripMenuItem");
            this.recoveryDeviceToolStripMenuItem.Name = "recoveryDeviceToolStripMenuItem";
            // 
            // recoveryFiltersToolStripMenuItem
            // 
            this.recoveryFiltersToolStripMenuItem.CheckOnClick = true;
            this.recoveryFiltersToolStripMenuItem.Name = "recoveryFiltersToolStripMenuItem";
            resources.ApplyResources(this.recoveryFiltersToolStripMenuItem, "recoveryFiltersToolStripMenuItem");
            // 
            // recoveryFiltersAddToolStripMenuItem
            // 
            this.recoveryFiltersAddToolStripMenuItem.Name = "recoveryFiltersAddToolStripMenuItem";
            resources.ApplyResources(this.recoveryFiltersAddToolStripMenuItem, "recoveryFiltersAddToolStripMenuItem");
            // 
            // MainWindow
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.splitContainerDevices);
            this.Controls.Add(this.mainMenuStrip);
            this.MainMenuStrip = this.mainMenuStrip;
            this.Name = "MainWindow";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainWindow_FormClosing);
            this.splitContainerDevices.Panel1.ResumeLayout(false);
            this.splitContainerDevices.Panel2.ResumeLayout(false);
            this.splitContainerDevices.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.mainMenuStrip.ResumeLayout(false);
            this.mainMenuStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label labelOS;
        private System.Windows.Forms.TreeView treeViewDevices;
        private System.Windows.Forms.PropertyGrid propertyGridDevice;
        private System.Windows.Forms.SplitContainer splitContainerDevices;
        private System.Windows.Forms.TextBox textBoxMessages;
        private System.Windows.Forms.CheckBox checkBoxCancelAutoPlay;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.MenuStrip mainMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem volumesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem wpdDevicesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem hidToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem recoveryDeviceToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem mscFiltersToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem mscFiltersAddToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem wpdFiltersToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem wpdFiltersAddToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem hidFiltersToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem hidFiltersAddToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem recoveryFiltersToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem recoveryFiltersAddToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem usbOnlyToolStripMenuItem;
    }
}

