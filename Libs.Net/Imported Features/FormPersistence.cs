/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// DevPlanner - Improve Your estimatng skils!

using System;
using System.Windows.Forms;
using System.Drawing;
using Microsoft.Win32;

namespace Utils
{
	/// <summary>
	/// Automatic form persistence.
	/// </summary>
	public class FormPersistence
	{
		private System.Windows.Forms.Form mainFrame;
		private ViewProfile profile;
		private bool restoreComplete = false;

		public FormPersistence(System.Windows.Forms.Form mainFrame, 
			string subkey)
		{
			this.mainFrame = mainFrame;
			this.profile = new ViewProfile(subkey);

			this.mainFrame.Closing += 
				new System.ComponentModel.CancelEventHandler(this.MainFrame_Closing);
			this.mainFrame.Load += 
				new System.EventHandler(this.MainFrame_Load);
			this.mainFrame.Resize += 
				new System.EventHandler(this.MainFrame_Resize);
			this.mainFrame.Move += 
				new System.EventHandler(this.MainFrame_Move);
		}

		/// <summary>
		/// Restores windows state and position from registry.
		/// </summary>
		public void Restore()
		{
			try
			{
				this.restoreComplete = false;
				mainFrame.DesktopBounds = Profile.Read(Names.Bounds + mainFrame.Name);
				mainFrame.WindowState = 
					(FormWindowState)Profile.ReadInt(Names.WindowState + mainFrame.Name);
				foreach(Control control in mainFrame.Controls)
				{
					RestoreControl(control);
				}
			}
			catch(Exception)
			{
			}
			finally
			{
				this.restoreComplete = true;
			}
		}

		/// <summary>
		/// Saves windows state and position under registry.
		/// </summary>
		public void Save()
		{
			if(this.restoreComplete)
			{
				Profile.Write(Names.Bounds + mainFrame.Name, mainFrame.DesktopBounds);
				SaveWindowState();
				foreach(Control control in mainFrame.Controls)
				{
					SaveControl(control);
				}
			}
		}

		private void SaveWindowState()
		{
			Profile.Write(Names.WindowState + mainFrame.Name, (int)mainFrame.WindowState);
		}
        
		private void SaveColumnsOnly()
		{
			foreach(Control control in mainFrame.Controls)
			{
				SaveColumns(control);
			}
		}

		public ViewProfile Profile
		{
			get
			{
				return this.profile;
			}
		}

        private void RestoreControl(Control control)
        {
            Type controlType = control.GetType();
            bool isToolBar = (controlType == typeof(ToolBar));
            bool isStatusBar = (controlType == typeof(StatusBar));
            if (!isToolBar && !isStatusBar)
            {
                control.Bounds = Profile.Read(Names.Bounds + control.Name);
            }
            if (controlType == typeof(TabControl))
            {
                RestoreTab((TabControl)control);
            }
            if (controlType == typeof(SplitContainer))
            {
                RestoreSplitContainer((SplitContainer)control);
            }

            if (controlType == typeof(ListView))
            {
                RestoreColumns((ListView)control);
            }

//            foreach (Control childControl in control.Controls)
//            {
//                RestoreControl(childControl);
//            }
        }

        private void SaveControl(Control control)
        {
            Type controlType = control.GetType();
            bool isToolBar = (controlType == typeof(ToolBar));
            bool isStatusBar = (controlType == typeof(StatusBar));
            if (!isToolBar && !isStatusBar)
            {
                Profile.Write(Names.Bounds + control.Name, control.Bounds);
            }
            if (controlType == typeof(TabControl))
            {
                SaveTab((TabControl)control);
            }
            if (controlType == typeof(SplitContainer))
            {
                SaveSplitContainer((SplitContainer)control);
            }
            SaveColumns(control);

//            foreach (Control childControl in control.Controls)
//            {
//                SaveControl(childControl);
//            }
        }

        private void SaveTab(TabControl tab)
        {
            foreach (TabPage tabPg in tab.TabPages)
            {
                foreach (Control control in tabPg.Controls)
                {
                    SaveControl(control);
                }
            }
        }

        private void RestoreTab(TabControl tab)
        {
            foreach (TabPage tabPg in tab.TabPages)
            {
                foreach (Control control in tabPg.Controls)
                {
                    RestoreControl(control);
                }
            }
        }

        private void SaveSplitContainer(SplitContainer control)
        {
            Profile.Write(Names.Bounds + control.Name + Names.SplitterDistance, control.SplitterDistance);
        }

        private void RestoreSplitContainer(SplitContainer control)
        {
            control.SplitterDistance = Profile.ReadInt(Names.Bounds + control.Name + Names.SplitterDistance);
        }

        private void RestoreColumns(ListView listView)
		{
			foreach(ColumnHeader column in listView.Columns)
			{
                column.Width = Profile.ReadInt(Names.Column + listView.Name + column.Index);
			}
		}

		private void SaveColumns(Control control)
		{
			Type controlType = control.GetType();
			bool isListView = (controlType == typeof(ListView));
			if(isListView)
			{
				SaveColumns((ListView)control);
			}
		}

		private void SaveColumns(ListView listView)
		{
			foreach(ColumnHeader column in listView.Columns)
			{
                Profile.Write(Names.Column + listView.Name + column.Index, column.Width);
			}
		}

		private void MainFrame_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			if(mainFrame.WindowState == FormWindowState.Normal)
			{
				Save();
			}
			else
			{
				SaveColumnsOnly();
			}
		}

		private void MainFrame_Load(object sender, System.EventArgs e)
		{
			Restore();
		}

		private void MainFrame_Resize(object sender, System.EventArgs e)
		{
			if(mainFrame.WindowState == FormWindowState.Normal)
			{
				Save();
			}
			else
			{
				SaveWindowState();
			}
		}

		private void MainFrame_Move(object sender, System.EventArgs e)
		{
			if(mainFrame.WindowState == FormWindowState.Normal)
			{
				Save();
			}
			else
			{
				SaveWindowState();
			}
		}
	}
}
