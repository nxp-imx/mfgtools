
using System;
using System.IO;
using System.ComponentModel;

namespace DevSupport.Api
{
    public class RecoveryApi : Api
    {
        protected RecoveryApi(Api.CommandType cmdType, Api.CommandDirection readWrite, UInt32 timeout)
            : base(cmdType, readWrite, timeout)
        { }

        /// ////////////////////////////////////////////////////////////////////////////////////
        /// <summary>
        /// 
        /// DownloadFile
        /// 
        /// </summary>
        /// ////////////////////////////////////////////////////////////////////////////////////
        [MemberFunction()]
        public class DownloadFile : RecoveryApi
        {
            // Constructor
            public DownloadFile(String filename)
                : base(CommandType.ScsiCmd, Api.CommandDirection.WriteWithData, DefaultTimeout)
            {
                Filename = filename;
            }

            // Override Data here to add the MemberFunctionArgAttribute to the property
            // so it will get passed to the member function invocation.
            [MemberFunctionArg()]
            public override byte[] Data
            {
                get { return base.Data; }
                set { base.Data = value; }
            }

            [Category("Parameters"), Description("The filename to download."), NotifyParentProperty(true)]
            [Editor(typeof(System.Windows.Forms.Design.FileNameEditor), typeof(System.Drawing.Design.UITypeEditor))]
            public String Filename
            {
                get { return _Filename; }
                set
                {
                    _Filename = value;

                    try
                    {
                        _FileInfo = Media.FirmwareInfo.FromFile(value);
                        using (FileStream fs = File.OpenRead(value))
                        {
                            Data = new Byte[_FileInfo.DataSize];

                            if (_FileInfo.StartingOffset != 0)
                                fs.Seek(_FileInfo.StartingOffset, SeekOrigin.Begin);

                            fs.Read(Data, 0, (Int32)_FileInfo.DataSize);

                            TransferSize = (UInt32)Data.Length;
                        }
                    }
                    catch (Exception e)
                    {
                        _FileInfo = new DevSupport.Media.FirmwareInfo();
                        _FileInfo.FileStatus = e.Message;
                        Data = null;
                        TransferSize = 0;
                    }
                }
            }
            private String _Filename;

            [Category("Status"), Description("The status of the file to download.")]
            [TypeConverter(typeof(ExpandableObjectConverter))]
            public Media.FirmwareInfo FileInfo
            {
                get
                {
                    return _FileInfo;
                }
            }
            private Media.FirmwareInfo _FileInfo;

        } // class DownloadFile
    }
}