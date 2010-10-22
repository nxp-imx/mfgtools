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
using System.Drawing;
using System.IO;
//using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Utils
{
    public partial class BinaryEditorExDialog : Form
    {
        public BinaryEditorExDialog(Byte[] data)
        {
            Data = data;

            InitializeComponent();
            StartPosition = FormStartPosition.CenterParent;

            DataViewTextBox.Text = DisplayData(Data, 16, 1);
        }

        public Byte[] Data
        {
            get { return _Data; }
            set { _Data = value; }
        }
        private Byte[] _Data;

        private void OkButton_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
            Close();
        }

        private void CancelButton_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }

        private void ImportButton_Click(object sender, EventArgs e)
        {
            OpenFileDialog openDialog = new OpenFileDialog();
            openDialog.Filter = "Firmware Files (*.sb,*.rsc)|*.sb;*.rsc|Binary Files (*.bin, *.dat)|*.bin;*.dat|All Files (*.*)|*.*";

            if (openDialog.ShowDialog() == DialogResult.OK)
            {
                FileStream stream = new FileStream(openDialog.FileName, FileMode.Open, FileAccess.Read, FileShare.Read);

                if (stream != null)
                {
                    Data = new Byte[Data.Length];
                    int ret = stream.Read(Data, 0, (int)Data.Length);

                    DataViewTextBox.Text = DisplayData(Data, 16, 1);
                }
            }
        }

        private void SaveButton_Click(object sender, EventArgs e)
        {
            FileStream stream = null;

            SaveFileDialog saveDialog = new SaveFileDialog();
            saveDialog.FileName = "Untitled.bin";
            saveDialog.Filter = "Firmware Files (*.sb,*.rsc)|*.sb;*.rsc|Binary Files (*.bin, *.dat)|*.bin;*.dat|All Files (*.*)|*.*";
            saveDialog.FilterIndex = 2;
            saveDialog.RestoreDirectory = true;

            if (saveDialog.ShowDialog() == DialogResult.OK)
            {
                if ((stream = (FileStream)saveDialog.OpenFile()) != null)
                {
                    stream.Write(Data, 0, Data.Length);
                    stream.Close();
                }
            }
        }

        // TODO: COMBINE/MOVE THIS with FormatReadResponse( Byte[] data, Byte width, Byte wordSize) AND
        // String StringFromHexBytes(Byte[] bytes)
        /// <summary>
        /// Creates a string representing space separated data bytes (*pByte)
        /// with a 4-digit hex address per line.
        /// </summary>
        /// <param name="data">The data to format.</param>
        /// <param name="width">Number of bytes displayed per line.</param>
        /// <param name="wordSize">Number of bytes to group together as a word.</param>
        /// <returns></returns>
        protected String DisplayData(Byte[] data, Byte width, Byte wordSize)
        {
            if (data == null)
                return "No data to display.";

            String responseStr = " 0000:";

            for (int i = 0; i < data.Length; i += wordSize)
            {
                switch (wordSize)
                {
                    case 4:
                        {
                            Byte[] bytes = new Byte[sizeof(UInt32)];
                            Array.Copy(data, i, bytes, 0, sizeof(UInt32));
                            Array.Reverse(bytes);

                            UInt32 u32Word = BitConverter.ToUInt32(bytes, 0);
                            responseStr += String.Format(" {0:X8}", u32Word);
                            break;
                        }
                    case 1:
                    default:
                        responseStr += String.Format(" {0:X2}", data[i]);
                        break;
                }
                if (((i + wordSize) % width == 0) && ((i + wordSize) < data.Length))
                {
                    responseStr += String.Format("\r\n {0:X4}:", i + wordSize);
                }
            }
            return responseStr + "\r\n";
        }

    }
    

}
