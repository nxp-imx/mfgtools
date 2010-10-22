/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
namespace LiveUpdaterExample
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
            this.CloseButton = new System.Windows.Forms.Button();
            this.UpdateButton = new System.Windows.Forms.Button();
            this.CurrentVersionTextBox = new System.Windows.Forms.TextBox();
            this.UpgradeVersionTextBox = new System.Windows.Forms.TextBox();
            this.CurrentFirmwareVersionLabel = new System.Windows.Forms.Label();
            this.UpgradeFirmwareVersionLabel = new System.Windows.Forms.Label();
            this.MainWindowMenuStrip = new System.Windows.Forms.MenuStrip();
            this.MainMenuItem_File = new System.Windows.Forms.ToolStripMenuItem();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.MainMenuItem_Device = new System.Windows.Forms.ToolStripDropDownButton();
            this.DeviceMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.FirmwareFilenameLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.ProgressBar = new System.Windows.Forms.ProgressBar();
            this.StatusTextBox = new System.Windows.Forms.TextBox();
            this.PictureBox = new System.Windows.Forms.PictureBox();
            this.MainWindowMenuStrip.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // CloseButton
            // 
            this.CloseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.CloseButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.CloseButton.Location = new System.Drawing.Point(408, 64);
            this.CloseButton.Name = "CloseButton";
            this.CloseButton.Size = new System.Drawing.Size(75, 23);
            this.CloseButton.TabIndex = 0;
            this.CloseButton.Text = "Close";
            this.CloseButton.UseVisualStyleBackColor = true;
            this.CloseButton.Click += new System.EventHandler(this.CloseButton_Click);
            // 
            // UpdateButton
            // 
            this.UpdateButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.UpdateButton.Location = new System.Drawing.Point(408, 34);
            this.UpdateButton.Name = "UpdateButton";
            this.UpdateButton.Size = new System.Drawing.Size(75, 23);
            this.UpdateButton.TabIndex = 1;
            this.UpdateButton.Text = "Update";
            this.UpdateButton.UseVisualStyleBackColor = true;
            this.UpdateButton.Click += new System.EventHandler(this.UpdateButton_Click);
            // 
            // CurrentVersionTextBox
            // 
            this.CurrentVersionTextBox.Location = new System.Drawing.Point(258, 36);
            this.CurrentVersionTextBox.Name = "CurrentVersionTextBox";
            this.CurrentVersionTextBox.ReadOnly = true;
            this.CurrentVersionTextBox.Size = new System.Drawing.Size(124, 20);
            this.CurrentVersionTextBox.TabIndex = 2;
            this.CurrentVersionTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // UpgradeVersionTextBox
            // 
            this.UpgradeVersionTextBox.Location = new System.Drawing.Point(258, 66);
            this.UpgradeVersionTextBox.Name = "UpgradeVersionTextBox";
            this.UpgradeVersionTextBox.ReadOnly = true;
            this.UpgradeVersionTextBox.Size = new System.Drawing.Size(124, 20);
            this.UpgradeVersionTextBox.TabIndex = 3;
            this.UpgradeVersionTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // CurrentFirmwareVersionLabel
            // 
            this.CurrentFirmwareVersionLabel.AutoSize = true;
            this.CurrentFirmwareVersionLabel.Location = new System.Drawing.Point(102, 39);
            this.CurrentFirmwareVersionLabel.Name = "CurrentFirmwareVersionLabel";
            this.CurrentFirmwareVersionLabel.Size = new System.Drawing.Size(127, 13);
            this.CurrentFirmwareVersionLabel.TabIndex = 4;
            this.CurrentFirmwareVersionLabel.Text = "Current Firmware Version:";
            // 
            // UpgradeFirmwareVersionLabel
            // 
            this.UpgradeFirmwareVersionLabel.AutoSize = true;
            this.UpgradeFirmwareVersionLabel.Location = new System.Drawing.Point(102, 69);
            this.UpgradeFirmwareVersionLabel.Name = "UpgradeFirmwareVersionLabel";
            this.UpgradeFirmwareVersionLabel.Size = new System.Drawing.Size(134, 13);
            this.UpgradeFirmwareVersionLabel.TabIndex = 5;
            this.UpgradeFirmwareVersionLabel.Text = "Upgrade Firmware Version:";
            // 
            // MainWindowMenuStrip
            // 
            this.MainWindowMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MainMenuItem_File,
            this.MainMenuItem_Device});
            this.MainWindowMenuStrip.Location = new System.Drawing.Point(0, 0);
            this.MainWindowMenuStrip.Name = "MainWindowMenuStrip";
            this.MainWindowMenuStrip.Size = new System.Drawing.Size(495, 24);
            this.MainWindowMenuStrip.TabIndex = 6;
            this.MainWindowMenuStrip.Text = "MainWindowMenuStrip";
            // 
            // MainMenuItem_File
            // 
            this.MainMenuItem_File.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripMenuItem,
            this.toolStripSeparator1,
            this.exitToolStripMenuItem});
            this.MainMenuItem_File.Name = "MainMenuItem_File";
            this.MainMenuItem_File.Size = new System.Drawing.Size(35, 20);
            this.MainMenuItem_File.Text = "&File";
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.Size = new System.Drawing.Size(112, 22);
            this.openToolStripMenuItem.Text = "&Open...";
            this.openToolStripMenuItem.Click += new System.EventHandler(this.FileMenuItem_Open_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(109, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(112, 22);
            this.exitToolStripMenuItem.Text = "E&xit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.FileMenuItem_Exit_Click);
            // 
            // MainMenuItem_Device
            // 
            this.MainMenuItem_Device.Name = "MainMenuItem_Device";
            this.MainMenuItem_Device.Size = new System.Drawing.Size(84, 17);
            this.MainMenuItem_Device.Text = "Select &Device";
            // 
            // DeviceMenuStrip
            // 
            this.DeviceMenuStrip.Name = "DeviceMenuStrip";
            this.DeviceMenuStrip.ShowCheckMargin = true;
            this.DeviceMenuStrip.Size = new System.Drawing.Size(83, 4);
            this.DeviceMenuStrip.ItemClicked += new System.Windows.Forms.ToolStripItemClickedEventHandler(this.DeviceMenuStrip_ItemClicked);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FirmwareFilenameLabel});
            this.statusStrip1.Location = new System.Drawing.Point(0, 152);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(495, 22);
            this.statusStrip1.TabIndex = 7;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // FirmwareFilenameLabel
            // 
            this.FirmwareFilenameLabel.Name = "FirmwareFilenameLabel";
            this.FirmwareFilenameLabel.Size = new System.Drawing.Size(47, 17);
            this.FirmwareFilenameLabel.Text = "filename";
            // 
            // ProgressBar
            // 
            this.ProgressBar.Location = new System.Drawing.Point(13, 118);
            this.ProgressBar.Name = "ProgressBar";
            this.ProgressBar.Size = new System.Drawing.Size(162, 13);
            this.ProgressBar.TabIndex = 8;
            // 
            // StatusTextBox
            // 
            this.StatusTextBox.Location = new System.Drawing.Point(192, 107);
            this.StatusTextBox.Multiline = true;
            this.StatusTextBox.Name = "StatusTextBox";
            this.StatusTextBox.ReadOnly = true;
            this.StatusTextBox.Size = new System.Drawing.Size(291, 35);
            this.StatusTextBox.TabIndex = 9;
            this.StatusTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // PictureBox
            // 
            this.PictureBox.Location = new System.Drawing.Point(13, 39);
            this.PictureBox.Name = "PictureBox";
            this.PictureBox.Size = new System.Drawing.Size(48, 48);
            this.PictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
            this.PictureBox.TabIndex = 10;
            this.PictureBox.TabStop = false;
            // 
            // MainWindow
            // 
            this.AcceptButton = this.UpdateButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.CloseButton;
            this.ClientSize = new System.Drawing.Size(495, 174);
            this.Controls.Add(this.PictureBox);
            this.Controls.Add(this.StatusTextBox);
            this.Controls.Add(this.ProgressBar);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.UpdateButton);
            this.Controls.Add(this.CloseButton);
            this.Controls.Add(this.CurrentFirmwareVersionLabel);
            this.Controls.Add(this.CurrentVersionTextBox);
            this.Controls.Add(this.UpgradeFirmwareVersionLabel);
            this.Controls.Add(this.MainWindowMenuStrip);
            this.Controls.Add(this.UpgradeVersionTextBox);
            this.MaximizeBox = false;
            this.Name = "MainWindow";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "LiveUpdater Example";
            this.Load += new System.EventHandler(this.MainWindow_Load);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainWindow_FormClosing);
            this.MainWindowMenuStrip.ResumeLayout(false);
            this.MainWindowMenuStrip.PerformLayout();
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button CloseButton;
        private System.Windows.Forms.Button UpdateButton;
        private System.Windows.Forms.TextBox CurrentVersionTextBox;
        private System.Windows.Forms.TextBox UpgradeVersionTextBox;
        private System.Windows.Forms.Label CurrentFirmwareVersionLabel;
        private System.Windows.Forms.Label UpgradeFirmwareVersionLabel;
        private System.Windows.Forms.MenuStrip MainWindowMenuStrip;
        private System.Windows.Forms.ToolStripDropDownButton MainMenuItem_Device;
        private System.Windows.Forms.ContextMenuStrip DeviceMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem MainMenuItem_File;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel FirmwareFilenameLabel;
        private System.Windows.Forms.ProgressBar ProgressBar;
        private System.Windows.Forms.TextBox StatusTextBox;
        private System.Windows.Forms.PictureBox PictureBox;
    }
}

