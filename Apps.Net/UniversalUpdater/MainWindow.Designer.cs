/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
namespace UniversalUpdater
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
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.OverallProgressBar = new System.Windows.Forms.ProgressBar();
            this.TaskProgressBar = new System.Windows.Forms.ProgressBar();
            this.TaskTextBox = new System.Windows.Forms.TextBox();
            this.CurrentVersionTextBox = new System.Windows.Forms.TextBox();
            this.UpgradeVersionTextBox = new System.Windows.Forms.TextBox();
            this.CurrentVersionLabel = new System.Windows.Forms.Label();
            this.StatusTextBox = new System.Windows.Forms.TextBox();
            this.UpgradeVersionLabel = new System.Windows.Forms.Label();
            this.PictureBox = new System.Windows.Forms.PictureBox();
            this.OutputWindow = new System.Windows.Forms.RichTextBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.Operations_LayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this.StatusStrip = new System.Windows.Forms.StatusStrip();
            this.DeviceDescStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox)).BeginInit();
            this.panel1.SuspendLayout();
            this.StatusStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // CloseButton
            // 
            this.CloseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.CloseButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.CloseButton.Location = new System.Drawing.Point(381, 89);
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
            this.UpdateButton.Location = new System.Drawing.Point(300, 89);
            this.UpdateButton.Name = "UpdateButton";
            this.UpdateButton.Size = new System.Drawing.Size(75, 23);
            this.UpdateButton.TabIndex = 1;
            this.UpdateButton.Text = "Update";
            this.UpdateButton.UseVisualStyleBackColor = true;
            this.UpdateButton.Click += new System.EventHandler(this.UpdateButton_Click);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.IsSplitterFixed = true;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tableLayoutPanel1);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.OutputWindow);
            this.splitContainer1.Size = new System.Drawing.Size(468, 294);
            this.splitContainer1.SplitterDistance = 150;
            this.splitContainer1.TabIndex = 2;
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanel1.ColumnCount = 7;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 8F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 8F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 13F));
            this.tableLayoutPanel1.Controls.Add(this.OverallProgressBar, 1, 5);
            this.tableLayoutPanel1.Controls.Add(this.TaskProgressBar, 1, 4);
            this.tableLayoutPanel1.Controls.Add(this.TaskTextBox, 4, 4);
            this.tableLayoutPanel1.Controls.Add(this.CurrentVersionTextBox, 5, 1);
            this.tableLayoutPanel1.Controls.Add(this.UpgradeVersionTextBox, 5, 2);
            this.tableLayoutPanel1.Controls.Add(this.CurrentVersionLabel, 2, 1);
            this.tableLayoutPanel1.Controls.Add(this.StatusTextBox, 2, 3);
            this.tableLayoutPanel1.Controls.Add(this.UpgradeVersionLabel, 2, 2);
            this.tableLayoutPanel1.Controls.Add(this.PictureBox, 1, 1);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 7;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 8F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 8F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(468, 150);
            this.tableLayoutPanel1.TabIndex = 9;
            // 
            // OverallProgressBar
            // 
            this.OverallProgressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.tableLayoutPanel1.SetColumnSpan(this.OverallProgressBar, 5);
            this.OverallProgressBar.Location = new System.Drawing.Point(11, 125);
            this.OverallProgressBar.MaximumSize = new System.Drawing.Size(0, 10);
            this.OverallProgressBar.Name = "OverallProgressBar";
            this.OverallProgressBar.Size = new System.Drawing.Size(438, 10);
            this.OverallProgressBar.TabIndex = 7;
            // 
            // TaskProgressBar
            // 
            this.TaskProgressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.tableLayoutPanel1.SetColumnSpan(this.TaskProgressBar, 2);
            this.TaskProgressBar.Location = new System.Drawing.Point(11, 99);
            this.TaskProgressBar.MaximumSize = new System.Drawing.Size(0, 10);
            this.TaskProgressBar.Name = "TaskProgressBar";
            this.TaskProgressBar.Size = new System.Drawing.Size(212, 10);
            this.TaskProgressBar.TabIndex = 2;
            // 
            // TaskTextBox
            // 
            this.TaskTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.tableLayoutPanel1.SetColumnSpan(this.TaskTextBox, 2);
            this.TaskTextBox.Location = new System.Drawing.Point(237, 89);
            this.TaskTextBox.Name = "TaskTextBox";
            this.TaskTextBox.ReadOnly = true;
            this.TaskTextBox.Size = new System.Drawing.Size(212, 20);
            this.TaskTextBox.TabIndex = 8;
            // 
            // CurrentVersionTextBox
            // 
            this.CurrentVersionTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.CurrentVersionTextBox.Location = new System.Drawing.Point(346, 11);
            this.CurrentVersionTextBox.Name = "CurrentVersionTextBox";
            this.CurrentVersionTextBox.ReadOnly = true;
            this.CurrentVersionTextBox.Size = new System.Drawing.Size(103, 20);
            this.CurrentVersionTextBox.TabIndex = 3;
            // 
            // UpgradeVersionTextBox
            // 
            this.UpgradeVersionTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.UpgradeVersionTextBox.Location = new System.Drawing.Point(346, 37);
            this.UpgradeVersionTextBox.Name = "UpgradeVersionTextBox";
            this.UpgradeVersionTextBox.ReadOnly = true;
            this.UpgradeVersionTextBox.Size = new System.Drawing.Size(103, 20);
            this.UpgradeVersionTextBox.TabIndex = 4;
            // 
            // CurrentVersionLabel
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.CurrentVersionLabel, 3);
            this.CurrentVersionLabel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.CurrentVersionLabel.Location = new System.Drawing.Point(120, 8);
            this.CurrentVersionLabel.Name = "CurrentVersionLabel";
            this.CurrentVersionLabel.Size = new System.Drawing.Size(220, 26);
            this.CurrentVersionLabel.TabIndex = 5;
            this.CurrentVersionLabel.Text = "Current Firmware Version:";
            this.CurrentVersionLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // StatusTextBox
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.StatusTextBox, 3);
            this.StatusTextBox.Location = new System.Drawing.Point(120, 63);
            this.StatusTextBox.Name = "StatusTextBox";
            this.StatusTextBox.ReadOnly = true;
            this.StatusTextBox.Size = new System.Drawing.Size(150, 20);
            this.StatusTextBox.TabIndex = 1;
            this.StatusTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // UpgradeVersionLabel
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.UpgradeVersionLabel, 3);
            this.UpgradeVersionLabel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.UpgradeVersionLabel.Location = new System.Drawing.Point(120, 34);
            this.UpgradeVersionLabel.Name = "UpgradeVersionLabel";
            this.UpgradeVersionLabel.Size = new System.Drawing.Size(220, 26);
            this.UpgradeVersionLabel.TabIndex = 6;
            this.UpgradeVersionLabel.Text = "Upgrade Firmware Version:";
            this.UpgradeVersionLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // PictureBox
            // 
            this.PictureBox.Location = new System.Drawing.Point(11, 11);
            this.PictureBox.Name = "PictureBox";
            this.tableLayoutPanel1.SetRowSpan(this.PictureBox, 3);
            this.PictureBox.Size = new System.Drawing.Size(59, 46);
            this.PictureBox.TabIndex = 0;
            this.PictureBox.TabStop = false;
            // 
            // OutputWindow
            // 
            this.OutputWindow.BackColor = System.Drawing.SystemColors.Window;
            this.OutputWindow.Dock = System.Windows.Forms.DockStyle.Fill;
            this.OutputWindow.Location = new System.Drawing.Point(0, 0);
            this.OutputWindow.Name = "OutputWindow";
            this.OutputWindow.ReadOnly = true;
            this.OutputWindow.Size = new System.Drawing.Size(468, 140);
            this.OutputWindow.TabIndex = 0;
            this.OutputWindow.Text = "";
            this.OutputWindow.TextChanged += new System.EventHandler(this.OutputWindow_TextChanged);
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.Operations_LayoutPanel);
            this.panel1.Controls.Add(this.UpdateButton);
            this.panel1.Controls.Add(this.CloseButton);
            this.panel1.Controls.Add(this.StatusStrip);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 294);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(468, 147);
            this.panel1.TabIndex = 3;
            // 
            // Operations_LayoutPanel
            // 
            this.Operations_LayoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Operations_LayoutPanel.ColumnCount = 1;
            this.Operations_LayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.Operations_LayoutPanel.Dock = System.Windows.Forms.DockStyle.Left;
            this.Operations_LayoutPanel.Location = new System.Drawing.Point(0, 0);
            this.Operations_LayoutPanel.Name = "Operations_LayoutPanel";
            this.Operations_LayoutPanel.Padding = new System.Windows.Forms.Padding(10, 0, 0, 0);
            this.Operations_LayoutPanel.RowCount = 1;
            this.Operations_LayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.Operations_LayoutPanel.Size = new System.Drawing.Size(113, 125);
            this.Operations_LayoutPanel.TabIndex = 4;
            this.toolTip1.SetToolTip(this.Operations_LayoutPanel, "UCL Operations");
            // 
            // StatusStrip
            // 
            this.StatusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.DeviceDescStatusLabel});
            this.StatusStrip.Location = new System.Drawing.Point(0, 125);
            this.StatusStrip.Name = "StatusStrip";
            this.StatusStrip.Size = new System.Drawing.Size(468, 22);
            this.StatusStrip.TabIndex = 3;
            this.StatusStrip.Text = "statusStrip1";
            // 
            // DeviceDescStatusLabel
            // 
            this.DeviceDescStatusLabel.Name = "DeviceDescStatusLabel";
            this.DeviceDescStatusLabel.Size = new System.Drawing.Size(0, 17);
            // 
            // MainWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(468, 441);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.panel1);
            this.Name = "MainWindow";
            this.Text = "Universal Updater";
            this.Load += new System.EventHandler(this.MainWindow_Load);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainWindow_FormClosing);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox)).EndInit();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.StatusStrip.ResumeLayout(false);
            this.StatusStrip.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button CloseButton;
        private System.Windows.Forms.Button UpdateButton;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.RichTextBox OutputWindow;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.PictureBox PictureBox;
        private System.Windows.Forms.TextBox StatusTextBox;
        private System.Windows.Forms.ProgressBar TaskProgressBar;
        private System.Windows.Forms.TextBox CurrentVersionTextBox;
        private System.Windows.Forms.TextBox UpgradeVersionTextBox;
        private System.Windows.Forms.Label UpgradeVersionLabel;
        private System.Windows.Forms.Label CurrentVersionLabel;
        private System.Windows.Forms.TextBox TaskTextBox;
        private System.Windows.Forms.ProgressBar OverallProgressBar;
        private System.Windows.Forms.StatusStrip StatusStrip;
        private System.Windows.Forms.ToolStripStatusLabel DeviceDescStatusLabel;
        private System.Windows.Forms.TableLayoutPanel Operations_LayoutPanel;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
    }
}

