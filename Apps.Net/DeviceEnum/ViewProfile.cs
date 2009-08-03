// DevPlanner - Improve Your estimatng skils!

using Microsoft.Win32;
using System;
using System.Diagnostics;
using System.Drawing;

namespace Utils
{
	public class Names
	{
		public static string Bounds = "Bounds";
		public static string Column = "Column";
		public static string WindowState = "WindowState";
		public static string X = "X";
		public static string Y = "Y";
		public static string Width = "Width";
		public static string Height = "Height";
        public static string SplitterDistance = "SplitterDistance";
        public static string Software = "Software\\";
	}

	/// <summary>
	/// Summary description for ViewProfile.
	/// </summary>
	public class ViewProfile
	{
		private string subkey;
		public ViewProfile(string subkey)
		{
			this.subkey = subkey;
		}


        public Rectangle Read(string fieldName)
		{
                Rectangle rect = new Rectangle();
                rect.X = ReadInt(fieldName + Names.X);
                rect.Y = ReadInt(fieldName + Names.Y);
                rect.Width = ReadInt(fieldName + Names.Width);
                rect.Height = ReadInt(fieldName + Names.Height);
                return rect;
		}

        public void Write(string fieldName, Rectangle rect)
		{
			Write(fieldName + Names.X, rect.X);
			Write(fieldName + Names.Y, rect.Y);
			Write(fieldName + Names.Width, rect.Width);
			Write(fieldName + Names.Height, rect.Height);
		}

		public int ReadInt(string fieldName)
		{
            RegistryKey key = Registry.CurrentUser.OpenSubKey(SubKey);
            int val = (int)key.GetValue(fieldName);

            return val;
		}

        public int ReadInt(string fieldName, int defaultValue)
        {
            int val = defaultValue;
            
            using(RegistryKey key = Registry.CurrentUser.OpenSubKey(SubKey))
            {
                try { val = (int)key.GetValue(fieldName); }
                catch { Trace.WriteLine(String.Format("{0} does not exist.", fieldName)); }
            }

            return val;
        }
        
        public void Write(string fieldName, int value)
		{
			RegistryKey key = 
				Registry.CurrentUser.CreateSubKey(SubKey);
			key.SetValue(fieldName, value);
		}

		public string SubKey
		{
			get
			{
				return Names.Software + subkey;
			}
		}
	}
}
