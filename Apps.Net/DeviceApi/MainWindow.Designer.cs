/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
namespace DeviceApi
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainWindow));
            this.StatusStripCtrl = new System.Windows.Forms.StatusStrip();
            this.DeviceStatusStripTextCtrl = new System.Windows.Forms.ToolStripStatusLabel();
            this.ApiStatusStripTextCtrl = new System.Windows.Forms.ToolStripStatusLabel();
            this.ApiStatusStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this.MainMenuItem_File_Exit = new System.Windows.Forms.ToolStripMenuItem();
            this.DeviceMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.MainWindowMenuStrip = new System.Windows.Forms.MenuStrip();
            this.MainMenuItem_File = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuItem_File_RejectAutoPlay = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuItem_File_TrackUsbPort = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.MainMenuItem_Device = new System.Windows.Forms.ToolStripDropDownButton();
            this.ApiPropertyGridCtrl = new System.Windows.Forms.PropertyGrid();
            this.SendButton = new System.Windows.Forms.Button();
            this.ApiListCtrl = new System.Windows.Forms.ListView();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.splitContainer2 = new System.Windows.Forms.SplitContainer();
            this.detailsGrid = new System.Windows.Forms.PropertyGrid();
            this.chkUseUTP = new System.Windows.Forms.CheckBox();
            this.splitContainer3 = new System.Windows.Forms.SplitContainer();
            this.LogTextBox = new System.Windows.Forms.TextBox();
            this.StatusStripCtrl.SuspendLayout();
            this.MainWindowMenuStrip.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.splitContainer2.Panel1.SuspendLayout();
            this.splitContainer2.Panel2.SuspendLayout();
            this.splitContainer2.SuspendLayout();
            this.splitContainer3.Panel1.SuspendLayout();
            this.splitContainer3.Panel2.SuspendLayout();
            this.splitContainer3.SuspendLayout();
            this.SuspendLayout();
            // 
            // StatusStripCtrl
            // 
            this.StatusStripCtrl.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.DeviceStatusStripTextCtrl,
            this.ApiStatusStripTextCtrl,
            this.ApiStatusStripProgressBar});
            resources.ApplyResources(this.StatusStripCtrl, "StatusStripCtrl");
            this.StatusStripCtrl.Name = "StatusStripCtrl";
            // 
            // DeviceStatusStripTextCtrl
            // 
            this.DeviceStatusStripTextCtrl.Margin = new System.Windows.Forms.Padding(0, 3, 50, 2);
            this.DeviceStatusStripTextCtrl.Name = "DeviceStatusStripTextCtrl";
            resources.ApplyResources(this.DeviceStatusStripTextCtrl, "DeviceStatusStripTextCtrl");
            // 
            // ApiStatusStripTextCtrl
            // 
            this.ApiStatusStripTextCtrl.Margin = new System.Windows.Forms.Padding(0, 3, 50, 2);
            this.ApiStatusStripTextCtrl.Name = "ApiStatusStripTextCtrl";
            resources.ApplyResources(this.ApiStatusStripTextCtrl, "ApiStatusStripTextCtrl");
            // 
            // ApiStatusStripProgressBar
            // 
            this.ApiStatusStripProgressBar.Name = "ApiStatusStripProgressBar";
            resources.ApplyResources(this.ApiStatusStripProgressBar, "ApiStatusStripProgressBar");
            // 
            // MainMenuItem_File_Exit
            // 
            this.MainMenuItem_File_Exit.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.MainMenuItem_File_Exit.Name = "MainMenuItem_File_Exit";
            resources.ApplyResources(this.MainMenuItem_File_Exit, "MainMenuItem_File_Exit");
            this.MainMenuItem_File_Exit.Click += new System.EventHandler(this.FileMenuItem_Exit_Click);
            // 
            // DeviceMenuStrip
            // 
            this.DeviceMenuStrip.Name = "DeviceMenuStrip";
            this.DeviceMenuStrip.ShowCheckMargin = true;
            resources.ApplyResources(this.DeviceMenuStrip, "DeviceMenuStrip");
            this.DeviceMenuStrip.ItemClicked += new System.Windows.Forms.ToolStripItemClickedEventHandler(this.DeviceMenuStrip_ItemClicked);
            // 
            // MainWindowMenuStrip
            // 
            this.MainWindowMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenuItem_File,
            this.MainMenuItem_Device});
            resources.ApplyResources(this.MainWindowMenuStrip, "MainWindowMenuStrip");
            this.MainWindowMenuStrip.Name = "MainWindowMenuStrip";
            // 
            // MainMenuItem_File
            // 
            this.MainMenuItem_File.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenuItem_File_RejectAutoPlay,
            this.MainMenuItem_File_TrackUsbPort,
            this.toolStripSeparator1,
            this.MainMenuItem_File_Exit});
            this.MainMenuItem_File.Name = "MainMenuItem_File";
            resources.ApplyResources(this.MainMenuItem_File, "MainMenuItem_File");
            // 
            // MainMenuItem_File_RejectAutoPlay
            // 
            this.MainMenuItem_File_RejectAutoPlay.CheckOnClick = true;
            this.MainMenuItem_File_RejectAutoPlay.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.MainMenuItem_File_RejectAutoPlay.Name = "MainMenuItem_File_RejectAutoPlay";
            resources.ApplyResources(this.MainMenuItem_File_RejectAutoPlay, "MainMenuItem_File_RejectAutoPlay");
            this.MainMenuItem_File_RejectAutoPlay.Click += new System.EventHandler(this.MainMenuItem_File_RejectAutoPlay_Click);
            // 
            // MainMenuItem_File_TrackUsbPort
            // 
            this.MainMenuItem_File_TrackUsbPort.CheckOnClick = true;
            this.MainMenuItem_File_TrackUsbPort.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.MainMenuItem_File_TrackUsbPort.Name = "MainMenuItem_File_TrackUsbPort";
            resources.ApplyResources(this.MainMenuItem_File_TrackUsbPort, "MainMenuItem_File_TrackUsbPort");
            this.MainMenuItem_File_TrackUsbPort.Click += new System.EventHandler(this.MainMenuItem_File_TrackUsbPort_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            resources.ApplyResources(this.toolStripSeparator1, "toolStripSeparator1");
            // 
            // MainMenuItem_Device
            // 
            this.MainMenuItem_Device.Name = "MainMenuItem_Device";
            resources.ApplyResources(this.MainMenuItem_Device, "MainMenuItem_Device");
            // 
            // ApiPropertyGridCtrl
            // 
            resources.ApplyResources(this.ApiPropertyGridCtrl, "ApiPropertyGridCtrl");
            this.ApiPropertyGridCtrl.Name = "ApiPropertyGridCtrl";
            this.ApiPropertyGridCtrl.ToolbarVisible = false;
            this.ApiPropertyGridCtrl.SelectedGridItemChanged += new System.Windows.Forms.SelectedGridItemChangedEventHandler(this.ApiPropertyGridCtrl_SelectedGridItemChanged);
            // 
            // SendButton
            // 
            resources.ApplyResources(this.SendButton, "SendButton");
            this.SendButton.Name = "SendButton";
            this.SendButton.UseVisualStyleBackColor = true;
            this.SendButton.Click += new System.EventHandler(this.SendButton_Click);
            // 
            // ApiListCtrl
            // 
            resources.ApplyResources(this.ApiListCtrl, "ApiListCtrl");
            this.ApiListCtrl.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.ApiListCtrl.HideSelection = false;
            this.ApiListCtrl.Name = "ApiListCtrl";
            this.ApiListCtrl.UseCompatibleStateImageBehavior = false;
            this.ApiListCtrl.View = System.Windows.Forms.View.Details;
            this.ApiListCtrl.SelectedIndexChanged += new System.EventHandler(this.ApiListCtrl_SelectedIndexChanged);
            // 
            // splitContainer1
            // 
            resources.ApplyResources(this.splitContainer1, "splitContainer1");
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.ApiListCtrl);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.splitContainer2);
            // 
            // splitContainer2
            // 
            resources.ApplyResources(this.splitContainer2, "splitContainer2");
            this.splitContainer2.Name = "splitContainer2";
            // 
            // splitContainer2.Panel1
            // 
            this.splitContainer2.Panel1.Controls.Add(this.ApiPropertyGridCtrl);
            // 
            // splitContainer2.Panel2
            // 
            this.splitContainer2.Panel2.Controls.Add(this.detailsGrid);
            this.splitContainer2.Panel2.Controls.Add(this.chkUseUTP);
            this.splitContainer2.Panel2.Controls.Add(this.SendButton);
            // 
            // detailsGrid
            // 
            resources.ApplyResources(this.detailsGrid, "detailsGrid");
            this.detailsGrid.Name = "detailsGrid";
            this.detailsGrid.ToolbarVisible = false;
            // 
            // chkUseUTP
            // 
            resources.ApplyResources(this.chkUseUTP, "chkUseUTP");
            this.chkUseUTP.Name = "chkUseUTP";
            this.chkUseUTP.UseVisualStyleBackColor = true;
            this.chkUseUTP.CheckedChanged += new System.EventHandler(this.chkUseUTP_CheckedChanged);
            // 
            // splitContainer3
            // 
            resources.ApplyResources(this.splitContainer3, "splitContainer3");
            this.splitContainer3.Name = "splitContainer3";
            // 
            // splitContainer3.Panel1
            // 
            this.splitContainer3.Panel1.Controls.Add(this.splitContainer1);
            // 
            // splitContainer3.Panel2
            // 
            this.splitContainer3.Panel2.Controls.Add(this.LogTextBox);
            // 
            // LogTextBox
            // 
            this.LogTextBox.BackColor = System.Drawing.SystemColors.Window;
            resources.ApplyResources(this.LogTextBox, "LogTextBox");
            this.LogTextBox.Name = "LogTextBox";
            this.LogTextBox.ReadOnly = true;
            // 
            // MainWindow
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.splitContainer3);
            this.Controls.Add(this.MainWindowMenuStrip);
            this.Controls.Add(this.StatusStripCtrl);
            this.MainMenuStrip = this.MainWindowMenuStrip;
            this.Name = "MainWindow";
            this.Load += new System.EventHandler(this.MainWindow_Load);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainWindow_FormClosing);
            this.StatusStripCtrl.ResumeLayout(false);
            this.StatusStripCtrl.PerformLayout();
            this.MainWindowMenuStrip.ResumeLayout(false);
            this.MainWindowMenuStrip.PerformLayout();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.splitContainer2.Panel1.ResumeLayout(false);
            this.splitContainer2.Panel2.ResumeLayout(false);
            this.splitContainer2.Panel2.PerformLayout();
            this.splitContainer2.ResumeLayout(false);
            this.splitContainer3.Panel1.ResumeLayout(false);
            this.splitContainer3.Panel2.ResumeLayout(false);
            this.splitContainer3.Panel2.PerformLayout();
            this.splitContainer3.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.StatusStrip StatusStripCtrl;
        private System.Windows.Forms.ToolStripStatusLabel DeviceStatusStripTextCtrl;
        private System.Windows.Forms.ToolStripProgressBar ApiStatusStripProgressBar;
        private System.Windows.Forms.ToolStripStatusLabel ApiStatusStripTextCtrl;
        private System.Windows.Forms.ContextMenuStrip DeviceMenuStrip;
        private System.Windows.Forms.MenuStrip MainWindowMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem MainMenuItem_File;
        private System.Windows.Forms.ToolStripMenuItem MainMenuItem_File_Exit;
        private System.Windows.Forms.ToolStripDropDownButton MainMenuItem_Device;
        private System.Windows.Forms.ListView ApiListCtrl;
        private System.Windows.Forms.Button SendButton;
        private System.Windows.Forms.PropertyGrid ApiPropertyGridCtrl;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.SplitContainer splitContainer2;
        private System.Windows.Forms.SplitContainer splitContainer3;
        private System.Windows.Forms.TextBox LogTextBox;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem MainMenuItem_File_RejectAutoPlay;
        private System.Windows.Forms.ToolStripMenuItem MainMenuItem_File_TrackUsbPort;
        private System.Windows.Forms.CheckBox chkUseUTP;
        private System.Windows.Forms.PropertyGrid detailsGrid;
    }
}

