using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
//using System.Math;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;

using Microsoft.Win32.SafeHandles;

using DevSupport.Api;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A Human Interface Device (HID).
    /// </summary>
    public class HidDevice : Device, IRecoverable//, IComparable
    {
        internal HidDevice(IntPtr deviceInstance, string path)
            : base(deviceInstance, path)
        {
            AllocateIoBuffers();
        }
        [DllImport("KERNEL32", SetLastError = true)]
        private static extern void FlushFileBuffers(IntPtr handle);
        //------------------------------------------------------------------------------
        // HID Command Block Wrapper (CBW)
        //------------------------------------------------------------------------------
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public class HidCBW
        {
            public UInt32 Signature;        // Signature: 0x43544C42, o "BLTC" (little endian) for the BLTC CBW
            public UInt32 Tag;              // Tag: to be returned in the csw
            public UInt32 XferLength;       // XferLength: number of bytes to transfer
            public CbwFlagsType Flags;      // Flags:
                                            //   Bit 7: direction - device shall ignore this bit if the
                                            //     XferLength field is zero, otherwise:
                                            //     0 = data-out from the host to the device,
                                            //     1 = data-in from the device to the host.
                                            //   Bits 6..0: reserved - shall be zero.
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
            public Byte[] Reserved;         // Reserved - shall be zero.
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
            public Byte[] Cdb;              // cdb: the command descriptor block

            public HidCBW()
            {
                Reserved = new Byte[2];
                Cdb = new Byte[16];
            }

            public Byte[] ToByteArray()
            {
                List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
                bytes.AddRange(BitConverter.GetBytes(Signature));
                bytes.AddRange(BitConverter.GetBytes(Tag));
                bytes.AddRange(BitConverter.GetBytes(XferLength));
                bytes.Add((Byte)Flags);
                bytes.AddRange(Reserved);
                bytes.AddRange(Cdb);
                return bytes.ToArray();
            }
        };
        // Signature value for _ST_HID_CBW
        const UInt32 CBW_BLTC_SIGNATURE = 0x43544C42; // "BLTC" (little endian)
        const UInt32 CBW_PITC_SIGNATURE = 0x43544950; // "PITC" (little endian)
        // Flags values for _ST_HID_CBW
        [Flags]
        public enum CbwFlagsType : byte
        {
            HostToDevice = 0x00, // "Data In"
            DeviceToHost = 0x80  // "Data Out"
        }
        //------------------------------------------------------------------------------
        // HID Command Status Wrapper (CSW)
        //------------------------------------------------------------------------------
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HidCSW
        {
            public UInt32 Signature;        // Signature: 0x53544C42, or "BLTS" (little endian) for the BLTS CSW
            public UInt32 Tag;              // Tag: matches the value from the CBW
            public UInt32 Residue;          // Residue: number of bytes not transferred
            public CommandStatusType Status;           // Status:
                                    //  00h command passed ("good status")
                                    //  01h command failed
                                    //  02h phase error
                                    //  03h to FFh reserved
            
            public Byte[] ToByteArray()
            {
                List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
                bytes.AddRange(BitConverter.GetBytes(Signature));
                bytes.AddRange(BitConverter.GetBytes(Tag));
                bytes.AddRange(BitConverter.GetBytes(Residue));
                bytes.Add((Byte)Status);
                return bytes.ToArray();
            }
        }
        // Signature value for _ST_HID_CSW
        const UInt32 CSW_BLTS_SIGNATURE = 0x53544C42; // "BLTS" (little endian)
        const UInt32 CSW_PITS_SIGNATURE = 0x53544950; // "PITS" (little endian)
        // Status values for _ST_HID_CSW
        public enum CommandStatusType : byte { Passed = 0x00, Failed = 0x01, PhaseError = 0x02 }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public class HidCommandReport
	    {
            public HidApi.HidReportType ReportId;
		    public HidCBW Cbw;

            public HidCommandReport()
            {
                Cbw = new HidCBW();
            }

            public Byte[] ToByteArray()
            {
                List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
                bytes.Add((Byte)ReportId);
                bytes.AddRange(Cbw.ToByteArray());
                return bytes.ToArray();
            }
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public class HidDataReport
	    {
            public HidApi.HidReportType ReportId;
		    public Byte[] Payload;

            public HidDataReport(UInt32 reportSize)
            {
                ReportId = 0;
                Payload = new Byte[reportSize - 1];
            }

            public HidDataReport(Byte[] reportBytes)
            {
                ReportId = (HidApi.HidReportType)reportBytes[0];
                Payload = new Byte[reportBytes.Length - 1];
                Array.Copy(reportBytes, 1, Payload, 0, reportBytes.Length - 1);
            }

            public Byte[] ToByteArray()
            {
                List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
                bytes.Add((Byte)ReportId);
                bytes.AddRange(Payload);
                return bytes.ToArray();
            }
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HidStatusReport
	    {
            public HidApi.HidReportType ReportId;
		    public HidCSW Csw;

            public HidStatusReport(Byte[] reportBytes)
            {
                ReportId = (HidApi.HidReportType)reportBytes[0];
                Csw.Signature = BitConverter.ToUInt32(reportBytes, 1);
                Csw.Tag = BitConverter.ToUInt32(reportBytes, 5);
                Csw.Residue = BitConverter.ToUInt32(reportBytes, 9);
                Csw.Status = (CommandStatusType)BitConverter.ToUInt32(reportBytes, 13);
            }

            public Byte[] ToByteArray()
            {
                List<Byte> bytes = new List<Byte>(Marshal.SizeOf(this));
                bytes.Add((Byte)ReportId);
                bytes.AddRange(Csw.ToByteArray());
                return bytes.ToArray();
            }
        }
        //------------------------------------------------------------------------------
        // HID Device Variables
        //------------------------------------------------------------------------------
        [TypeConverter(typeof(ExpandableObjectConverter))]
        public Win32.HIDP_CAPS HidCapabilities
        {
            get 
            {
                return _HidCapabilities;
            }
        }
        private Win32.HIDP_CAPS _HidCapabilities;

//        private HidDataReport _ReadReport;
//        private HidDataReport _WriteReport;

        private CommandStatusType _Status;
        private static UInt32 _CdbTag;

        //------------------------------------------------------------------------------
        // HID Device Functions
        //------------------------------------------------------------------------------
        // Modiifes _Capabilities member variable
        private int AllocateIoBuffers()
        {
            int error = Win32.ERROR_SUCCESS;

            // Open the device
            Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
            secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
            SafeFileHandle hHidDevice = Win32.CreateFile(Path, 0, 0, ref secAttribs, Win32.OPEN_EXISTING, 0, IntPtr.Zero);

            if( hHidDevice.IsInvalid )
            {
                error = Marshal.GetLastWin32Error();
//                Trace.WriteLine(String.Format(" HidDevice.AllocateIoBuffers() ERROR:{0}"), error);
                return error;
            }

            // Get the Capabilities including the max size of the report buffers
            IntPtr PreparsedData;
            if ( !Win32.HidD_GetPreparsedData(hHidDevice, out PreparsedData) )
            {
                hHidDevice.Close();
                error = Win32.ERROR_GEN_FAILURE;
//                Trace.WriteLine(String.Format(" HidDevice.AllocateIoBuffers() ERROR:{0}"), error);
                return error;
            }

            if (Win32.HidP_GetCaps(PreparsedData, ref _HidCapabilities) != Win32.HIDP_STATUS_SUCCESS)
            {
                hHidDevice.Close();
                Win32.HidD_FreePreparsedData(PreparsedData);
                error = Win32.HIDP_STATUS_INVALID_PREPARSED_DATA;
//                Trace.WriteLine(String.Format(" HidDevice.AllocateIoBuffers() ERROR:{0}"), error);
                return error;
            }

            hHidDevice.Close();
            Win32.HidD_FreePreparsedData(PreparsedData);
            return Win32.ERROR_SUCCESS;
        }

        public override bool Match(ushort? vid, ushort? pid)
        {
            if (pid == null && vid == 0x066f)
            {
                return (base.Match(0x066f, 0x3700) || base.Match(0x066f, 0x3770) || base.Match(0x066f, 0x3780));
            }
            else
            {
                return base.Match(vid, pid);
            }
        }

        override public Int32 SendCommand(Api.Api api)
        {
            // reset the error string
            ErrorString = String.Empty;
            this._Status = CommandStatusType.Passed;

            Api.HidApi hidApi = api as Api.HidApi;
            if (hidApi == null)
            {
                ErrorString = String.Format(" Error: Can not send \"{0}\" api type to \"{1}\" device.", api.ImageKey, this.ToString());
                return Win32.ERROR_INVALID_PARAMETER;
            }

            Debug.WriteLine(String.Format("+HidDevice.SendCommand({0}) devInst:{1}", hidApi.ToString(), this.DeviceInstance));

            hidApi.Tag = ++HidDevice._CdbTag;

            // tell the UI we are beginning a command.
            SendCommandProgressArgs cmdProgress = new SendCommandProgressArgs(hidApi.ToString(), hidApi.Direction, hidApi.TransferSize);
            cmdProgress.Status = String.Format("SendCommand({0}).Open()", hidApi.ToString());
            DoSendProgress(cmdProgress);

            // Open the device
            Win32.SECURITY_ATTRIBUTES secAttribs = new Win32.SECURITY_ATTRIBUTES();
            secAttribs.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES)); 
            SafeFileHandle hHidDevice = Win32.CreateFile(Path, Win32.GENERIC_READ | Win32.GENERIC_WRITE,
                Win32.FILE_SHARE_READ | Win32.FILE_SHARE_WRITE, ref secAttribs, Win32.OPEN_EXISTING, /*Win32.FILE_FLAG_OVERLAPPED*/Win32.FILE_FLAG_NO_BUFFERING, IntPtr.Zero);

            if ( hHidDevice.IsInvalid )
            {
                cmdProgress.Error = Marshal.GetLastWin32Error();
                string errorMsg = new Win32Exception(cmdProgress.Error).Message;
                ErrorString = String.Format(" ERROR: {0} ({1})", errorMsg, cmdProgress.Error);
                cmdProgress.InProgress = false;
                DoSendProgress(cmdProgress);

                Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                return cmdProgress.Error;
            }
	        Debug.WriteLine(String.Format("->HidDevice.SendCommand  OPEN:{0}", this.Path));

            FileStream fileStream = new FileStream(hHidDevice, FileAccess.ReadWrite, 8192);

            if (!ProcessWriteCommand(fileStream, hidApi, ref cmdProgress)) //CBW
            {
//                fileStream.Dispose();

                ErrorString = String.Format(" ERROR: {0}.", cmdProgress.Status);
                cmdProgress.InProgress = false;
                DoSendProgress(cmdProgress);

                Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                return cmdProgress.Error;
            }

            // Read/Write Data
            if (hidApi.TransferSize > 0)
            {
                if (hidApi.Direction != DevSupport.Api.Api.CommandDirection.ReadWithData)
                {
                    if (!ProcessWriteData(fileStream, hidApi, ref cmdProgress))
                    {
//                        fileStream.Dispose();

                        ErrorString = String.Format(" ERROR: {0}.", cmdProgress.Status);
                        cmdProgress.InProgress = false;
                        DoSendProgress(cmdProgress);

                        Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                        return cmdProgress.Error;
                    }
                }
                else
                {
                    if (!ProcessReadData(fileStream, ref hidApi, ref cmdProgress))
                    {
//                        fileStream.Dispose();

                        ErrorString = String.Format(" ERROR: {0}.", cmdProgress.Status);
                        cmdProgress.InProgress = false;
                        DoSendProgress(cmdProgress);

                        Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                        return cmdProgress.Error;
                    }
                }
            }

            if (!ProcessReadStatus(fileStream, hidApi, ref cmdProgress)) //CSW_REPORT
            {
//                fileStream.Dispose();

                ErrorString = String.Format(" ERROR: {0}.", cmdProgress.Status);
                cmdProgress.InProgress = false;
                DoSendProgress(cmdProgress);

                Trace.WriteLine(String.Format("-HidDevice.SendCommand({0}) {1}", hidApi.ToString(), cmdProgress.Status));
                return cmdProgress.Error;
            }

            fileStream.Dispose();

            Debug.WriteLine(String.Format("-HidDevice.SendCommand({0}) devInst:{1}", hidApi.ToString(), this.DeviceInstance));
            cmdProgress.Status = String.Empty;
            cmdProgress.InProgress = false;
            cmdProgress.Error = (Int32)this._Status;
            DoSendProgress(cmdProgress);

            return Win32.ERROR_SUCCESS;
        }

        private bool ProcessWriteCommand(FileStream fileStream, Api.HidApi api, ref SendCommandProgressArgs cmdProgress)
        {
	        Debug.WriteLine(String.Format(" HidDevice.ProcessWriteCommand() devInst:{0}, tag:{1}", this.DeviceInstance, api.Tag));

            UInt16 writeSize = this.HidCapabilities.OutputReportByteLength;
            if(writeSize < 1)
            {
                cmdProgress.Error = Win32.ERROR_BAD_LENGTH;
                Trace.WriteLine(String.Format(" -HidDevice.ProcessWriteCommand()  ERROR:({0})\r\n", cmdProgress.Error));
                return false;
            }

            // Send CBW 
            HidCommandReport cmdReport = new HidCommandReport();
            bool isBltc = String.Compare(api.ImageKey, "HidBltcCmd", true) == 0;
            
            cmdReport.ReportId = isBltc ? HidApi.HidReportType.BltcCommandOut : HidApi.HidReportType.PitcCommandOut;
	        cmdReport.Cbw.Tag = api.Tag;
            cmdReport.Cbw.Signature = isBltc ? CBW_BLTC_SIGNATURE : CBW_PITC_SIGNATURE;
	        cmdReport.Cbw.XferLength = Convert.ToUInt32(api.TransferSize);
            cmdReport.Cbw.Flags = (api.Direction == DevSupport.Api.Api.CommandDirection.ReadWithData) ? CbwFlagsType.DeviceToHost : CbwFlagsType.HostToDevice;
            api.Cdb.CopyTo(cmdReport.Cbw.Cdb, 0);

            Byte[] ouputBuffer = new Byte[writeSize];
            cmdReport.ToByteArray().CopyTo(ouputBuffer, 0);

            try
            {
                fileStream.Write(ouputBuffer, 0, ouputBuffer.Length);//, new AsyncCallback(IoCompletion), null);
                fileStream.Flush();
//                FlushFileBuffers(fileStream.SafeFileHandle.DangerousGetHandle());
            }
            catch (Exception e)
            {
                cmdProgress.Status = e.Message.Replace(".\r\n", "");
                Trace.WriteLine(cmdProgress.Status);
                cmdProgress.Error = Marshal.GetHRForException(e);
                try { fileStream.Dispose(); }
                catch { }
                return false;
            }
/*            BinaryWriter writer = new BinaryWriter(new FileStream(hDevice, FileAccess.Write));
            writer.Write(CmdReport.ToByteArray());

            memset(&_fileOverlapped, 0, sizeof(_fileOverlapped));
            _fileOverlapped.hEvent = this;
            BOOL temp = WriteFileEx(hDevice, _pWriteReport, writeSize, &_fileOverlapped, HidDevice.IoCompletion);
            if ( (nsInfo.error=GetLastError()) != ERROR_SUCCESS )
	        {
		        ATLTRACE2(_T(" -HidDevice::ProcessWriteCommand()  ERROR:(%d)\r\n"), nsInfo.error);
                return false;
            }

            nsInfo.error = ProcessTimeOut(api.GetTimeout());
            if( nsInfo.error != ERROR_SUCCESS )
	        {
       	        BOOL success = CancelIo(hDevice);
   		        ATLTRACE2(_T(" -HidDevice::ProcessWriteCommand()  ERROR:(%d)\r\n"), nsInfo.error);
		        return false;
	        }
*/
            Debug.WriteLine(" -HidDevice::ProcessWriteCommand()");
            return true;
        }

        // Changes data in api parameter, therefore must use ref Api instead of Api.
        private bool ProcessReadData(FileStream fileStream, ref Api.HidApi api, ref SendCommandProgressArgs cmdProgress)
        {
	        Debug.WriteLine(String.Format(" HidDevice.ProcessReadData() devInst:{0}, tag:{1}", this.DeviceInstance, api.Tag));

            UInt32 readSize = this.HidCapabilities.InputReportByteLength;
            bool isBltc = String.Compare(api.ImageKey, "HidBltcCmd", true) == 0;

            for ( UInt32 offset=0; offset < api.TransferSize; offset += (readSize - 1) )
            {
	            Debug.WriteLine(String.Format(" HidDevice.ProcessReadData() devInst:{0}, tag:{1}, offset:{2:X}", this.DeviceInstance, api.Tag, offset));

                Byte[] inputBuffer = new Byte[readSize];

                try
                {
                    fileStream.Read(inputBuffer, 0, inputBuffer.Length);//, new AsyncCallback(HidDevice.IoCompletion), null); 
                }
                catch (Exception e)
                {
                    cmdProgress.Status = e.Message.Replace(".\r\n", "");
                    Trace.WriteLine(cmdProgress.Status);
                    cmdProgress.Error = Marshal.GetHRForException(e);
                    fileStream.Dispose();
                    return false;
                }

                HidDataReport inputReport = new HidDataReport(inputBuffer);

                if (inputReport.ReportId == (isBltc ? HidApi.HidReportType.BltcDataIn : HidApi.HidReportType.PitcDataIn))
                {
                    cmdProgress.Error = api.ProcessResponse(inputReport.Payload, offset, readSize - 1);
                    if ( cmdProgress.Error != Win32.ERROR_SUCCESS )
                    {
                        ErrorString = String.Format(" ERROR: The \"{0}\" api was unable to process the response.", api.ToString());
                    }
                }

                // Update the UI
                cmdProgress.Position = (Int32)System.Math.Min(api.TransferSize, (offset + (readSize - 1)));
                cmdProgress.Status = String.Format("SendCommand({0}) Position = {1}.", api.ToString(), cmdProgress.Position);
                DoSendProgress(cmdProgress);
            }
            Debug.WriteLine(" -HidDevice::ProcessReadData()");
            return (cmdProgress.Error == Win32.ERROR_SUCCESS);
        }

        private bool ProcessWriteData(FileStream fileStream, Api.HidApi api, ref SendCommandProgressArgs cmdProgress)
        {
	        Debug.WriteLine(String.Format(" HidDevice.ProcessWriteData() devInst:{0}, tag:{1}", this.DeviceInstance, api.Tag));

            bool isBltc = String.Compare(api.ImageKey, "HidBltcCmd", true) == 0;

            Byte[] outputBuffer = new Byte[this.HidCapabilities.OutputReportByteLength];
            HidDataReport outputReport = new HidDataReport(outputBuffer);
            outputReport.ReportId = (isBltc ? HidApi.HidReportType.BltcDataOut : HidApi.HidReportType.PitcDataOut);

            for (Int32 offset = 0; offset < api.TransferSize; offset += HidCapabilities.OutputReportByteLength - 1)
            {
	            Debug.WriteLine(String.Format(" HidDevice.ProcessWriteData() devInst:{0}, tag:{1}, offset:{2:X}", this.DeviceInstance, api.Tag, offset));

                // bytes to write this time
                Int32 bytesToWrite = (Int32)Math.Min(api.TransferSize - offset,  outputReport.Payload.Length);
                
                // copy the bytes out of the api's data member into the output report.
                Array.Copy(api.Data, offset, outputReport.Payload, 0, bytesToWrite);

                try
                {
                    fileStream.Write(outputReport.ToByteArray(), 0, outputBuffer.Length);//, new AsyncCallback(IoCompletion), null); 
                    fileStream.Flush();
//                    FlushFileBuffers(fileStream.SafeFileHandle.DangerousGetHandle());
                }
                catch (Exception e)
                {
                    cmdProgress.Status = e.Message.Replace(".\r\n", "");
                    Trace.WriteLine(e.GetType().Name + ": " + cmdProgress.Status);
                    cmdProgress.Error = Marshal.GetHRForException(e);
                    try { fileStream.Dispose(); }
                    catch { };
                    return false;
                }

                // Update the UI
                cmdProgress.Position += bytesToWrite;
                cmdProgress.Status = String.Format("SendCommand({0}) Position = {1}.", api.ToString(), cmdProgress.Position);
                DoSendProgress(cmdProgress);
            }
            Debug.WriteLine(" -HidDevice::ProcessWriteData()");
            return (cmdProgress.Error == Win32.ERROR_SUCCESS);
        }

        private bool ProcessReadStatus(FileStream fileStream, Api.HidApi api, ref SendCommandProgressArgs cmdProgress)
        {
	        Debug.WriteLine(String.Format(" HidDevice.ProcessReadStatus() devInst:{0}, tag:{1}", this.DeviceInstance, api.Tag));
         
            UInt16 readSize = this.HidCapabilities.InputReportByteLength;
            bool isBltc = String.Compare(api.ImageKey, "HidBltcCmd", true) == 0;

            Byte[] inputBuffer = new Byte[readSize];
            
            try
            {
                fileStream.Read(inputBuffer, 0, inputBuffer.Length);//, new AsyncCallback(HidDevice.IoCompletion), null); 
            }
            catch (Exception e)
            {
                cmdProgress.Status = e.Message.Replace(".\r\n", "");
                Trace.WriteLine(cmdProgress.Status);
                cmdProgress.Error = Marshal.GetHRForException(e);
                fileStream.Dispose();
                return false;
            }

            // Allocate the CSW_REPORT
            HidStatusReport statusReport = new HidStatusReport(inputBuffer);

            // check status
            if ((statusReport.ReportId == (isBltc ? HidApi.HidReportType.BltcStatusIn : HidApi.HidReportType.PitcStatusIn)) &&
                statusReport.Csw.Tag == api.Tag)
                _Status = statusReport.Csw.Status;

            Debug.WriteLine(" -HidDevice.ProcessReadStatus()");
            return true;
        }

        [Browsable(false)]
        public override String ErrorString
        {
            get 
            {
                String errStr = String.IsNullOrEmpty(base.ErrorString) ? String.Empty : base.ErrorString + "\r\n";

	            switch ( _Status )
	            {
	                case CommandStatusType.Passed:
//                        errStr += String.Format(" HID Status: PASSED(0x{0:X2})\r\n", (Byte)_Status);
		                break;
	                case CommandStatusType.Failed:
                        errStr += String.Format(" HID Status: FAILED(0x{0:X2})\r\n", (Byte)_Status);
		                break;
	                case CommandStatusType.PhaseError:
                        errStr += String.Format(" HID Status: PHASE_ERROR(0x{0:X2})\r\n", (Byte)_Status);
		                break;
	                default:
                        errStr += String.Format(" HID Status: UNKNOWN(0x{0:X2})\r\n", (Byte)_Status);
                        break;
	            }
                return errStr;
            }
        }

        void IoCompletion(IAsyncResult result)
        {

        //	ATLTRACE2(_T("  +HidDevice::IoCompletion() dev: 0x%x, fn: 0x%x\r\n"), lpOverlapped->hEvent, HidDevice::IoCompletion);
//	        HidDevice* pDevice =  dynamic_cast<HidDevice*>((HidDevice*)lpOverlapped->hEvent);
        //	if ( pDevice )
        //        pDevice->_lastError = dwErrorCode;


        //	CSingleLock sLock(&m_mutex);
        //	if ( sLock.IsLocked() )
        //	{
        //		ATLTRACE2( _T("   HidDevice::IoCompletion() 0x%x: Had to wait for lock.\r\n"), pDevice );
        //	}
        //	int retries;
        //	for ( retries = 100; !sLock.Lock(50) && retries; --retries ) {
        //		ATLTRACE2( _T("  HidDevice::IoCompletion() 0x%x: Waiting for lock. Try: %d\r\n"), pDevice, 100-retries );
        //		Sleep(5);
        //	}
        //	if ( retries == 0 )
        //	{
        //		ATLTRACE2( _T("   HidDevice::IoCompletion() 0x%x: ERROR! Could not get lock.\r\n"), pDevice);
        //		ASSERT(0);
        //		return;
        //	}

/*	        switch (dwErrorCode)
	        {
		        case ERROR_HANDLE_EOF:
			        ATLTRACE2(_T("   HidDevice::IoCompletion() 0x%x - ERROR_HANDLE_EOF.\r\n"), pDevice);
                    break;
		        case 0:
        //			ATLTRACE2(_T("   HidDevice::IoCompletion() 0x%x - Transferred %d bytes.\r\n"), pDevice, dwNumberOfBytesTransfered);
                    break;
		        default:
			        ATLTRACE2(_T("   HidDevice::IoCompletion() 0x%x - Undefined Error(%x).\r\n"), pDevice, dwErrorCode);
                    break;
	        }
*/
        //	sLock.Unlock();
        //	ReleaseSemaphore(HidDevice::m_IOSemaphoreComplete, 1, NULL);
        }

        /// <summary>
        /// Loads the otpinit.sb firmware and initiates the OTP registers with the values contained in the firmware image.
        /// </summary>
        /// <param name="firmwareFilename">The name of the firmware file to use. 
        /// The current directory will be used if the full path is not specified. Defaults to 'otpinit.sb' if not specified.</param>
        /// <returns>0 for success, non-zero otherwise.</returns>
        public Int32 InitializeOtpRegs(String firmwareFilename)
        {
            if (String.IsNullOrEmpty(firmwareFilename))
                firmwareFilename = "otpinit.sb";

            ErrorString = String.Empty;
            Int32 retCode = Win32.ERROR_SUCCESS;

            HidApi.DownloadFw downloadApi = new HidApi.DownloadFw(firmwareFilename);
            if ( downloadApi.TransferSize == 0 )
            {
                ErrorString = downloadApi.FileInfo.FileStatus;
                retCode = Win32.ERROR_GEN_FAILURE;
            }

            if (retCode == Win32.ERROR_SUCCESS)
            {
                retCode = SendCommand(downloadApi);
            }
            
            if (retCode == Win32.ERROR_SUCCESS)
            {
                HidApi.PitcWrite writeApi = new HidApi.PitcWrite(0/*address*/, 0/*length*/, 0/*lock*/, null/*data*/);
                retCode = SendCommand(writeApi);
            }

            HidApi.PitcRequestSense senseApi = new HidApi.PitcRequestSense();
            retCode = SendCommand(senseApi);
            ErrorString = senseApi.ResponseString.Replace("\r\n", "");

            return (retCode != Win32.ERROR_SUCCESS) ? retCode : (Int32)senseApi.SenseCode;
        }

        #region IRecoverable Members

        public int LoadRecoveryDevice(string filename)
        {
            ErrorString = String.Empty;
            Int32 retCode = Win32.ERROR_SUCCESS;

            HidApi.DownloadFw downloadApi = new HidApi.DownloadFw(filename);
            if (downloadApi.TransferSize == 0)
            {
                ErrorString = downloadApi.FileInfo.FileStatus;
                retCode = Win32.ERROR_GEN_FAILURE;
            }

            if (retCode == Win32.ERROR_SUCCESS)
            {
//                SendCommandAsync sendCommandDelegate = new SendCommandAsync(SendCommand);
//                IAsyncResult ar = sendCommandDelegate.BeginInvoke(downloadApi, null, null);
                
                retCode = SendCommand(downloadApi);

                // Wait for the asynchronous operation to finish.
//                while (!ar.IsCompleted)
//                {
                    // Keep UI messages moving, so the form remains 
                    // responsive during the asynchronous operation.
//                    Application.DoEvents();
//                }

                // Get the result of the operation.
//                retCode = sendCommandDelegate.EndInvoke(ar);
            }

            return retCode;
        }

        #endregion
    } // class HidDevice
}
/*
int32_t HidDevice::ProcessTimeOut(const int32_t timeout)
{    
//	ATLTRACE2(_T("  +HidDevice::ProcessTimeOut() 0x%x\r\n"), this);
  
    HANDLE hTimer = CreateWaitableTimer(NULL, true, _T("SendCommandTimer"));
    LARGE_INTEGER waitTime; 
    waitTime.QuadPart = timeout * (-10000000);
    SetWaitableTimer(hTimer, &waitTime, 0, NULL, NULL, false);

    HANDLE waitHandles[1] = { hTimer };
    DWORD waitResult;
    bool done = false;
    while( !done )
    {
        waitResult = MsgWaitForMultipleObjectsEx(1, &waitHandles[0], INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
        switch (waitResult)
        {
            case WAIT_OBJECT_0:
            case WAIT_TIMEOUT:
			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - Timeout(%d) %d seconds.\r\n"), this, waitResult, timeout);
                return ERROR_SEM_TIMEOUT;
            case WAIT_OBJECT_0 + 1:
                {
                    // got a message that we need to handle while we are waiting.
                    MSG msg ; 
                    // Read all of the messages in this next loop, 
                    // removing each message as we read it.
                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
                    { 
                        // If it is a quit message, exit.
                        if (msg.message == WM_QUIT)  
                        {
                            done = true;
//                            break;
                        }
                        // Otherwise, dispatch the message.
                        DispatchMessage(&msg); 
                    } // End of PeekMessage while loop.
			        ATLTRACE2(_T("   HidDevice::ProcessTimeOut() 0x%x - Got a message(%0x).\r\n"), this, msg.message);
                    break;
                }
            case WAIT_ABANDONED:
			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - Wait abandoned.\r\n"), this);
                return ERROR_OPERATION_ABORTED;
            case WAIT_IO_COMPLETION:
                // this is what we are really waiting on.
//			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - I/O satisfied by CompletionRoutine.\r\n"), this);
                return ERROR_SUCCESS;
		    case WAIT_FAILED:
            {
			    int32_t error = GetLastError();
 			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - Wait failed(%d).\r\n"), this, error);
                return error;
            }
            default:
 			    ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x - Undefined error(%d).\r\n"), this, waitResult);
                return ERROR_WRITE_FAULT;
        }
    }
	ATLTRACE2(_T("  -HidDevice::ProcessTimeOut() 0x%x WM_QUIT\r\n"), this);
    return ERROR_OPERATION_ABORTED;
}


uint32_t HidDevice::ResetChip()
{
	api::HidDeviceReset api;

	return SendCommand(api);
}
*/