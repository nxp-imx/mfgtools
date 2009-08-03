namespace Updater
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
            this.CloseButton = new System.Windows.Forms.Button();
            this.UpdateButton = new System.Windows.Forms.Button();
            this.panel1 = new System.Windows.Forms.Panel();
            this.ShowLogButton = new System.Windows.Forms.Button();
            this.ShowHideImageList = new System.Windows.Forms.ImageList(this.components);
            this.PreserveJanusCheckBox = new System.Windows.Forms.CheckBox();
            this.EraseMediaChekBox = new System.Windows.Forms.CheckBox();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.UpgradeVersionTextBox = new System.Windows.Forms.TextBox();
            this.CurrentVersionTextBox = new System.Windows.Forms.TextBox();
            this.PictureBox = new System.Windows.Forms.PictureBox();
            this.CurrentFirmwareVersionLabel = new System.Windows.Forms.Label();
            this.StatusTextBox = new System.Windows.Forms.TextBox();
            this.UpgradeFirmwareVersionLabel = new System.Windows.Forms.Label();
            this.ProgressBar = new System.Windows.Forms.ProgressBar();
            this.ActionDetailsTextBox = new System.Windows.Forms.TextBox();
            this.OutputWindow = new System.Windows.Forms.RichTextBox();
            this.panel1.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // CloseButton
            // 
            resources.ApplyResources(this.CloseButton, "CloseButton");
            this.CloseButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.CloseButton.Name = "CloseButton";
            this.CloseButton.UseVisualStyleBackColor = true;
            this.CloseButton.Click += new System.EventHandler(this.CloseButton_Click);
            // 
            // UpdateButton
            // 
            resources.ApplyResources(this.UpdateButton, "UpdateButton");
            this.UpdateButton.Name = "UpdateButton";
            this.UpdateButton.UseVisualStyleBackColor = true;
            this.UpdateButton.Click += new System.EventHandler(this.UpdateButton_Click);
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.ShowLogButton);
            this.panel1.Controls.Add(this.PreserveJanusCheckBox);
            this.panel1.Controls.Add(this.EraseMediaChekBox);
            this.panel1.Controls.Add(this.UpdateButton);
            this.panel1.Controls.Add(this.CloseButton);
            resources.ApplyResources(this.panel1, "panel1");
            this.panel1.Name = "panel1";
            // 
            // ShowLogButton
            // 
            resources.ApplyResources(this.ShowLogButton, "ShowLogButton");
            this.ShowLogButton.ImageList = this.ShowHideImageList;
            this.ShowLogButton.Name = "ShowLogButton";
            this.ShowLogButton.UseVisualStyleBackColor = true;
            this.ShowLogButton.Visible = global::Updater.Properties.Settings.Default.ShowLog;
            this.ShowLogButton.Click += new System.EventHandler(this.ShowLogButton_Click);
            // 
            // ShowHideImageList
            // 
            this.ShowHideImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("ShowHideImageList.ImageStream")));
            this.ShowHideImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.ShowHideImageList.Images.SetKeyName(0, "Hide.ico");
            this.ShowHideImageList.Images.SetKeyName(1, "Show.ico");
            // 
            // PreserveJanusCheckBox
            // 
            resources.ApplyResources(this.PreserveJanusCheckBox, "PreserveJanusCheckBox");
            this.PreserveJanusCheckBox.Checked = global::Updater.Properties.Settings.Default.PreserveJanus;
            this.PreserveJanusCheckBox.Name = "PreserveJanusCheckBox";
            this.PreserveJanusCheckBox.UseVisualStyleBackColor = true;
            // 
            // EraseMediaChekBox
            // 
            resources.ApplyResources(this.EraseMediaChekBox, "EraseMediaChekBox");
            this.EraseMediaChekBox.Checked = global::Updater.Properties.Settings.Default.EraseMedia;
            this.EraseMediaChekBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.EraseMediaChekBox.Name = "EraseMediaChekBox";
            this.EraseMediaChekBox.UseVisualStyleBackColor = true;
            this.EraseMediaChekBox.CheckedChanged += new System.EventHandler(this.EraseMediaChekBox_CheckedChanged);
            // 
            // splitContainer1
            // 
            resources.ApplyResources(this.splitContainer1, "splitContainer1");
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tableLayoutPanel1);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.OutputWindow);
            // 
            // tableLayoutPanel1
            // 
            resources.ApplyResources(this.tableLayoutPanel1, "tableLayoutPanel1");
            this.tableLayoutPanel1.Controls.Add(this.UpgradeVersionTextBox, 2, 2);
            this.tableLayoutPanel1.Controls.Add(this.CurrentVersionTextBox, 2, 1);
            this.tableLayoutPanel1.Controls.Add(this.PictureBox, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.CurrentFirmwareVersionLabel, 1, 1);
            this.tableLayoutPanel1.Controls.Add(this.StatusTextBox, 1, 3);
            this.tableLayoutPanel1.Controls.Add(this.UpgradeFirmwareVersionLabel, 1, 2);
            this.tableLayoutPanel1.Controls.Add(this.ProgressBar, 0, 4);
            this.tableLayoutPanel1.Controls.Add(this.ActionDetailsTextBox, 1, 4);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            // 
            // UpgradeVersionTextBox
            // 
            resources.ApplyResources(this.UpgradeVersionTextBox, "UpgradeVersionTextBox");
            this.UpgradeVersionTextBox.Name = "UpgradeVersionTextBox";
            this.UpgradeVersionTextBox.ReadOnly = true;
            // 
            // CurrentVersionTextBox
            // 
            resources.ApplyResources(this.CurrentVersionTextBox, "CurrentVersionTextBox");
            this.CurrentVersionTextBox.Name = "CurrentVersionTextBox";
            this.CurrentVersionTextBox.ReadOnly = true;
            // 
            // PictureBox
            // 
            resources.ApplyResources(this.PictureBox, "PictureBox");
            this.PictureBox.Name = "PictureBox";
            this.tableLayoutPanel1.SetRowSpan(this.PictureBox, 2);
            this.PictureBox.TabStop = false;
            // 
            // CurrentFirmwareVersionLabel
            // 
            resources.ApplyResources(this.CurrentFirmwareVersionLabel, "CurrentFirmwareVersionLabel");
            this.CurrentFirmwareVersionLabel.Name = "CurrentFirmwareVersionLabel";
            // 
            // StatusTextBox
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.StatusTextBox, 2);
            resources.ApplyResources(this.StatusTextBox, "StatusTextBox");
            this.StatusTextBox.Name = "StatusTextBox";
            this.StatusTextBox.ReadOnly = true;
            // 
            // UpgradeFirmwareVersionLabel
            // 
            resources.ApplyResources(this.UpgradeFirmwareVersionLabel, "UpgradeFirmwareVersionLabel");
            this.UpgradeFirmwareVersionLabel.Name = "UpgradeFirmwareVersionLabel";
            // 
            // ProgressBar
            // 
            resources.ApplyResources(this.ProgressBar, "ProgressBar");
            this.ProgressBar.Name = "ProgressBar";
            // 
            // ActionDetailsTextBox
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.ActionDetailsTextBox, 2);
            resources.ApplyResources(this.ActionDetailsTextBox, "ActionDetailsTextBox");
            this.ActionDetailsTextBox.Name = "ActionDetailsTextBox";
            this.ActionDetailsTextBox.ReadOnly = true;
            // 
            // OutputWindow
            // 
            this.OutputWindow.BackColor = System.Drawing.SystemColors.Window;
            resources.ApplyResources(this.OutputWindow, "OutputWindow");
            this.OutputWindow.Name = "OutputWindow";
            this.OutputWindow.ReadOnly = true;
            this.OutputWindow.TextChanged += new System.EventHandler(this.OutputWindow_TextChanged);
            // 
            // MainWindow
            // 
            this.AcceptButton = this.UpdateButton;
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.CloseButton;
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.panel1);
            this.MaximizeBox = false;
            this.Name = "MainWindow";
            this.Load += new System.EventHandler(this.MainWindow_Load);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainWindow_FormClosing);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button CloseButton;
        private System.Windows.Forms.Button UpdateButton;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.RichTextBox OutputWindow;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.TextBox UpgradeVersionTextBox;
        private System.Windows.Forms.TextBox CurrentVersionTextBox;
        private System.Windows.Forms.ProgressBar ProgressBar;
        private System.Windows.Forms.PictureBox PictureBox;
        private System.Windows.Forms.Label CurrentFirmwareVersionLabel;
        private System.Windows.Forms.Label UpgradeFirmwareVersionLabel;
        private System.Windows.Forms.CheckBox PreserveJanusCheckBox;
        private System.Windows.Forms.CheckBox EraseMediaChekBox;
        private System.Windows.Forms.TextBox StatusTextBox;
        private System.Windows.Forms.TextBox ActionDetailsTextBox;
        private System.Windows.Forms.Button ShowLogButton;
        private System.Windows.Forms.ImageList ShowHideImageList;
    }
}

