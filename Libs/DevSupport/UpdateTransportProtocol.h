/*
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using Microsoft.Win32.SafeHandles;

using DevSupport.DeviceManager;
using DevSupport.Api;
*/
#include "Volume.h"
#include "UpdateTransportProtocol.Api.h"

public class UpdateTransportProtocol
{
private:
	Volume UtpDevice = null;
    HANDLE hDevice = null;
    static UInt32 TransactionTag = 0;

public:
	class Transaction
    {
	public:
		Transaction(UpdateTransportProtocol* pUtpInstance, CString command);
        {
            m_pUpdateProtocol = pUtpInstance;
            m_Command = command;
            m_Tag = ++UpdateTransportProtocol.TransactionTag;
        }
        
		bool KeepPolling()
        {
            return m_BusyCount < m_pUpdateProtocol->MaxPollBusy;
        }

        virtual Int64 LParam = 0;

		virtual void Next() = 0;

	protected:
		State* m_pCurrentState;
        UpdateTransportProtocol m_pUpdateProtocol;
        UInt32 m_BusyCount;
        
	private:
		UInt32 m_Tag;
        CString m_Command;
        Int64 m_TotalSize;
        Int64 m_CurrentSize;
    }

    class CommandTransaction : Transaction
    {
	public:
		CommandTransaction(UpdateTransportProtocol* pUtpInstance, CString command)
            : base(utpInstance, command)
        {
            m_pCurrentState = new StartState(this);
        }

        void Next()
        {
            switch (m_pCurrentState->m_Msg->ResponseCode)
            {
				case ScsiUtpMsg::ResponseCodeType::BUSY:
                    if (CurrentState is StartState)
                    {
                        TotalSize = CurrentState.Msg.ResponseInfo;
                        CurrentSize = 0;

                    }
                    else
                    {
                        CurrentSize = Math.Min(TotalSize, TotalSize - CurrentState.Msg.ResponseInfo);
                    }
                    
                    CurrentState = new BusyState(this, CurrentState.Msg.ResponseInfo);
                    if (KeepPolling)
                        Thread.Sleep(UpdateProtocol.BusyDelay);
                    else
                        CurrentState = new DoneState(this, -65535, "ERROR: Polling stalled. (-65535)");
                    break;
                case ScsiUtpMsg.ResponseCodeType.EXIT:
                    CurrentState = new DoneState(this, CurrentState.Msg.ResponseInfo, CurrentState.Msg.ResponseString);
                    break;
                case ScsiUtpMsg.ResponseCodeType.PASS:
                    CurrentState = new DoneState(this, 0, CurrentState.Msg.ResponseString);
                    break;
                default:
//                        String msg = String.Format("In: {0}, Sent: {1}\r\n Response: {2}\r\n Moving to: {3}");
                    String msg = String.Format("Illegal ResponseCode({0}) for {1}->{2} message.",CurrentState.Msg.ResponseCode, CurrentState.GetType().Name, CurrentState.Msg); 
//                        InvalidTransactionStateException e = new InvalidTransactionStateException(msg2);
//                        throw e;
                    CurrentState = new DoneState(this, -65536, msg + " (-65536)");
                    break;
            }
        }

    }

    public class ReadTransaction : Transaction
    {
        public Int64 PacketCount
        {
            get { return CurrentSize / UpdateProtocol.MaxPacketSize; }
        }

        public Byte[] Data
        {
            get { return _Data; }
            private set { _Data = value; }
        }
        private Byte[] _Data = null;

        public ReadTransaction(UpdateTransportProtocol utpInstance, String command)
            : base(utpInstance, command)
        {
            CurrentState = new StartState(this);
        }

        public override void Next()
        {
            if (CurrentState is StartState)
            {
                switch (CurrentState.Msg.ResponseCode)
                {
                    case ScsiUtpMsg.ResponseCodeType.SIZE:
                        TotalSize = CurrentState.Msg.ResponseSize;
                        CurrentSize = 0;
                        Data = new Byte[TotalSize];
                        CurrentState = new GetDataState(this);
                        break;
                    case ScsiUtpMsg.ResponseCodeType.EXIT:
                        CurrentState = new DoneState(this, CurrentState.Msg.ResponseInfo, CurrentState.Msg.ResponseString);
                        break;
                    default:
//                            throw new Exception("Illegal Response Code for this transaction.");
                        String msg = String.Format("Illegal ResponseCode({0}) for {1}->{2} message.", CurrentState.Msg.ResponseCode, CurrentState.GetType().Name, CurrentState.Msg);
                        CurrentState = new DoneState(this, -65536, msg + " (-65536)");
                        break;
                }
            }
            else
            {
                switch (CurrentState.Msg.ResponseCode)
                {
                    case ScsiUtpMsg.ResponseCodeType.BUSY:
                        CurrentState = new BusyState(this, CurrentState.Msg.ResponseInfo);
                        if (KeepPolling)
                            Thread.Sleep(UpdateProtocol.BusyDelay);
                        else
                            CurrentState = new DoneState(this, -65535, "ERROR: Polling stalled. (-65535)");
                        break;
                    case ScsiUtpMsg.ResponseCodeType.PASS:
                        if (CurrentSize < TotalSize)
                            CurrentState = new GetDataState(this);
                        else
                            CurrentState = new DoneState(this, 0, CurrentState.Msg.ResponseString);
                        break;
                    case ScsiUtpMsg.ResponseCodeType.EXIT:
                        CurrentState = new DoneState(this, CurrentState.Msg.ResponseInfo, CurrentState.Msg.ResponseString);
                        break;
                    default:
//                            throw new Exception("Illegal Response Code for this transaction.");
                        String msg = String.Format("Illegal ResponseCode({0}) for {1}->{2} message.", CurrentState.Msg.ResponseCode, CurrentState.GetType().Name, CurrentState.Msg);
                        CurrentState = new DoneState(this, -65536, msg + " (-65536)");
                        break;
                }
            }
        }

        public void CopyData()
        {
            Array.Copy(CurrentState.Msg.Data, 0, Data, CurrentSize, CurrentState.Msg.Data.Length);
            CurrentSize += CurrentState.Msg.Data.Length;
        }

    }

    public class WriteTransaction : Transaction
    {
        public override Int64 LParam
        {
            get { return _LParam; }
        }
        private Int64 _LParam;

        public Int64 PacketCount
        {
            get { return CurrentSize/UpdateProtocol.MaxPacketSize; }
        }

        public Byte[] Data
        {
            get { return _Data; }
            private set { _Data = value; }
        }
        private Byte[] _Data = null;

        public WriteTransaction(UpdateTransportProtocol utpInstance, String command, Byte[] data)
            : base(utpInstance, command)
        {
            _LParam = TotalSize = data.Length;
            Data = new Byte[TotalSize];
            Array.Copy(data, Data, data.Length);

            CurrentState = new StartState(this);
        }

        public override void Next()
        {
            if (CurrentState is StartState)
            {
                switch (CurrentState.Msg.ResponseCode)
                {
                    case ScsiUtpMsg.ResponseCodeType.BUSY:
                        
                        TotalSize = CurrentState.Msg.ResponseInfo;
                        CurrentSize = 0;

                        CurrentState = new BusyState(this, CurrentState.Msg.ResponseInfo);
                        Thread.Sleep(UpdateProtocol.BusyDelay);
                        
                        break;
                    
                    case ScsiUtpMsg.ResponseCodeType.EXIT:
                        CurrentState = new DoneState(this, CurrentState.Msg.ResponseInfo, CurrentState.Msg.ResponseString);
                        break;
                    case ScsiUtpMsg.ResponseCodeType.PASS:
                        CurrentState = new PutDataState(this);
                        break;
                    default:
//                            throw new Exception("Illegal Response Code for this transaction.");
                        String msg = String.Format("Illegal ResponseCode({0}) for {1}->{2} message.", CurrentState.Msg.ResponseCode, CurrentState.GetType().Name, CurrentState.Msg);
                        CurrentState = new DoneState(this, -65536, msg + " (-65536)");
                        break;
                }
            }
            else
            {
                switch (CurrentState.Msg.ResponseCode)
                {
                    case ScsiUtpMsg.ResponseCodeType.BUSY:

                        CurrentSize = Math.Min(TotalSize, TotalSize - CurrentState.Msg.ResponseInfo);

                        CurrentState = new BusyState(this, CurrentState.Msg.ResponseInfo);
                        if (KeepPolling)
                            Thread.Sleep(UpdateProtocol.BusyDelay);
                        else
                            CurrentState = new DoneState(this, -65535, "ERROR: Polling stalled. (-65535)");
                        
                        break;

                    case ScsiUtpMsg.ResponseCodeType.PASS:
                        if (CurrentSize < TotalSize)
                            CurrentState = new PutDataState(this);
                        else
                            CurrentState = new DoneState(this, 0, CurrentState.Msg.ResponseString);
                        break;
                    case ScsiUtpMsg.ResponseCodeType.EXIT:
                        CurrentState = new DoneState(this, CurrentState.Msg.ResponseInfo, CurrentState.Msg.ResponseString);
                        break;
                    default:
//                            throw new Exception("Illegal Response Code for this transaction.");
                        String msg = String.Format("Illegal ResponseCode({0}) for {1}->{2} message.", CurrentState.Msg.ResponseCode, CurrentState.GetType().Name, CurrentState.Msg);
                        CurrentState = new DoneState(this, -65536, msg + " (-65536)");
                        break;
                }
            }
        }

    }

    public class State
    {
        public State(Transaction* pTransaction)
        {
            _pTransaction = pTransaction;
        }

        protected Transaction* m_pTransaction;
        protected ScsiUtpMsg* m_pMsg;
    }

    public class StartState : State
    {
        public StartState(Transaction* pTransaction)
            : base(pTransaction)
        {
            _pMsg = new ScsiUtpMsg.Exec(pTransaction->m_Tag, pTransaction->m_LParam, pTransaction->m_Command);
        }

        public override string ToString()
        {
            CString str;
			return str.Format("   StartState - {0} // {1}", m_Msg->ToString(), m_Msg->ResponseString);
        }

    }

    public class BusyState : State
    {
        public BusyState(Transaction transaction, Int32 ticksRemaining)
            : base(transaction)
        {
            TicksRemaining = ticksRemaining;
            _Msg = new ScsiUtpMsg.Poll(transaction.Tag, 0);

            if (transaction.CurrentState is BusyState)
            {
                if (((BusyState)transaction.CurrentState).TicksRemaining == TicksRemaining)
                    ++transaction.BusyCount;
                else
                    transaction.BusyCount = 0;
            }
            else
                transaction.BusyCount = 0;

            BusyCount = transaction.BusyCount;
        }

        public Int32 TicksRemaining
        {
            get { return _TicksRemaining; }
            private set { _TicksRemaining = value; }
        }
        private Int32 _TicksRemaining;

        private readonly UInt32 BusyCount;

        public override string ToString()
        {
            return String.Format("   BusyState - {0} busyCount:{1} // {2}", Msg.ToString(), BusyCount, Msg.ResponseString);
        }
    }

    public class DoneState : State
    {
        public DoneState(Transaction transaction, Int32 info, String msg)
            : base(transaction)
        {
            ResponseInfo = info;
            ResponseString = msg;
            Tag = transaction.Tag;
        }

        private readonly UInt32 Tag;

        public Int32 ResponseInfo
        {
            get { return _ResponseInfo; }
            private set { _ResponseInfo = value; }
        }
        private Int32 _ResponseInfo;

        public String ResponseString
        {
            get { return _ResponseString; }
            private set { _ResponseString = value; }
        }
        private String _ResponseString;

        public override string ToString()
        {
            return String.Format("   DoneState - tag:{0} // {1}", Tag, ResponseString);
        }
    }

    public class GetDataState : State
    {
        public GetDataState(ReadTransaction transaction)
            : base(transaction)
        {
            // How much data to get?
            Int64 numBytesToRead = Math.Min(transaction.UpdateProtocol.MaxPacketSize, transaction.TotalSize - transaction.CurrentSize);

            _Msg = new ScsiUtpMsg.Get(transaction.Tag, transaction.PacketCount, numBytesToRead);
        }

        public override string ToString()
        {
            return String.Format("   GetDataState - {0} // {1}", Msg.ToString(), Msg.ResponseString);
        }
    }

    public class PutDataState : State
    {
        public PutDataState(WriteTransaction transaction)
            : base(transaction)
        {
// TODO: CLW - USE THIS WHEN PAUL FIXES FW TO NOT DEPEND ON SECTOR SIZES 
//                Int64 numBytesToWrite = Math.Min(transaction.UpdateProtocol.MaxPacketSize, transaction.TotalSize - transaction.CurrentSize);
//                Byte[] buffer = new Byte[numBytesToWrite];
//                Array.Copy(transaction.Data, transaction.CurrentSize, buffer, 0, buffer.LongLength);

            // Get some data to put.
            Byte[] buffer = new Byte[transaction.UpdateProtocol.MaxPacketSize];
            Int64 numBytesToWrite = Math.Min(buffer.Length, transaction.TotalSize - transaction.CurrentSize);
            Array.Copy(transaction.Data, transaction.CurrentSize, buffer, 0, numBytesToWrite);
            // Send it.
            _Msg = new ScsiUtpMsg.Put(transaction.Tag, transaction.PacketCount, buffer);
            transaction.CurrentSize += numBytesToWrite;
        }
    
        public override string ToString()
        {
            return String.Format("   PutDataState - {0} // {1}", Msg.ToString(), Msg.ResponseString);
        }
    }

    private Volume UtpDevice = null;
    private SafeFileHandle hDevice = null;
    private static UInt32 TransactionTag = 0;
    private bool _Disposed = false;
    public enum ResetType : uint { PowerDown, ChipReset, ResetToRecovery, ResetToUpdater }

    /// <summary>
    ///     The version of the device implemented UTP Protocol.
    /// </summary>
    [Category("General"), Description("The version of the device implemented UTP Protocol.")]
    public Int32 UtpVersion
    {
        get { return _UtpVersion; }
        private set { _UtpVersion = value; }
    }
    private Int32 _UtpVersion;

    /// <summary>
    ///     The time in miliseconds to delay between utpBusy messages.
    /// </summary>
    [Category("General"), Description("The time in miliseconds to delay between utpBusy messages.")]
    public Int32 BusyDelay
    {
        get { return _BusyDelay; }
        set { _BusyDelay = value; }
    }
    private Int32 _BusyDelay = 250;

    /// <summary>
    ///     The maximum consecutive Poll messages returning the same BusyState.ResponseInfo before Transaction times out.
    /// </summary>
    [Category("General"), Description("The maximum consecutive Poll messages returning the same BusyState.ResponseInfo before Transaction times out.")]
    public UInt32 MaxPollBusy
    {
        get { return _MaxPollBusy; }
        set { _MaxPollBusy = value; }
    }
    private UInt32 _MaxPollBusy = 3;

    /// <summary>
    ///     The maximum packet size that can be transfered by the UTP Device.
    /// </summary>
    [Category("General"), Description("The maximum packet size that can be transfered by the UTP Device.")]
    public Int64 MaxPacketSize
    {
        get { return _MaxPacketSize; }
        private set { _MaxPacketSize = value; }
    }
    private Int64 _MaxPacketSize;

    public UpdateTransportProtocol(Volume device)
    {
        UtpDevice = device;
        MaxPacketSize = Volume.MaxTransferSize;

        // Open the device.
        hDevice = UtpDevice.Lock(Volume.LockType.Logical);

        // Get the UTP Protocol version.
        ScsiUtpMsg.Poll pollMsg = new ScsiUtpMsg.Poll(UTP.UpdateTransportProtocol.TransactionTag, (UInt32)ScsiUtpMsg.Poll.LParams.GetUtpVersion);
        UtpDevice.SendCommand(hDevice, pollMsg);
        if (pollMsg.ResponseCode == ScsiUtpMsg.ResponseCodeType.EXIT)
            UtpVersion = pollMsg.ResponseInfo;
    }


    private void Dispose(bool isDisposing)
    {
        if (!_Disposed)
        {
            UtpDevice.Unlock(hDevice, false);
        }
        hDevice.Close();
        _Disposed = true;
    }

    ~UpdateTransportProtocol()
    {
        Dispose(false);
    }

    #region IDisposable Members

    public void Dispose()
    {
        Dispose(true);
        GC.SuppressFinalize(this);
    }

    #endregion

    public Int32 UtpDrop(String cmd)
    {
        // tell the UI we are beginning a command.
        Device.SendCommandProgressArgs cmdProgress =
            new Device.SendCommandProgressArgs("UtpDrop", Api.Api.CommandDirection.WriteWithData, 0);
        cmdProgress.Status = String.Format("UtpDrop({0})", cmd);
        ((Volume)UtpDevice).DoSendProgress(cmdProgress);

        // Unlock the device, but don't close it.
        ((Volume)UtpDevice).Unlock(hDevice, false);

        // Send the ModeChange message.
        ScsiUtpMsg.Exec modeMsg = new ScsiUtpMsg.Exec(++UTP.UpdateTransportProtocol.TransactionTag, 0, cmd);
        UtpDevice.SendCommand(hDevice, modeMsg);
        if (modeMsg.ResponseCode == ScsiUtpMsg.ResponseCodeType.PASS)
        {
            // Set _Disposed to true so we don't try to unlock the device 
            // since it should be disconnecting from the bus and Windows will take care of it.
            _Disposed = true;
        }

        // tell the UI we are done
        cmdProgress.Status = String.Format("   DoneState - tag:{0} // {1}", modeMsg.Tag, modeMsg.ResponseString);
        cmdProgress.Error = modeMsg.ResponseInfo;
        cmdProgress.InProgress = false;
        ((Volume)UtpDevice).DoSendProgress(cmdProgress);

        return Convert.ToInt32(modeMsg.ResponseInfo);
    }

    public Int32 UtpCommand(String cmd)
    {
        CommandTransaction transaction = new CommandTransaction(this, cmd);

        // tell the UI we are beginning a command.
        Device.SendCommandProgressArgs cmdProgress =
            new Device.SendCommandProgressArgs("UtpCommand", Api.Api.CommandDirection.WriteWithData, 0);
        cmdProgress.Status = String.Format("UtpCommand({0}) tag:{1}", cmd, transaction.Tag);
        ((Volume)UtpDevice).DoSendProgress(cmdProgress);

        while (!(transaction.CurrentState is DoneState))
        {
            UtpDevice.SendCommand(hDevice, transaction.CurrentState.Msg);

            // Update the UI
            cmdProgress.Maximum = transaction.TotalSize;
            cmdProgress.Position = transaction.CurrentSize;
            cmdProgress.Status = transaction.CurrentState.ToString() + " // pos:" + transaction.CurrentSize.ToString("#,#0");
            ((Volume)UtpDevice).DoSendProgress(cmdProgress);

            transaction.Next();
        }

        // tell the UI we are done
//            cmdProgress.Position = (Int32)totalTransferSize;
        cmdProgress.Status = transaction.CurrentState.ToString();
        cmdProgress.Error = ((DoneState)transaction.CurrentState).ResponseInfo;
        cmdProgress.InProgress = false;
        ((Volume)UtpDevice).DoSendProgress(cmdProgress);

        return ((DoneState)transaction.CurrentState).ResponseInfo;
    }

    public Int32 UtpRead(String cmd, String filename)
    {
        ReadTransaction transaction = new ReadTransaction(this, cmd);

        // tell the UI we are beginning a command.
        Device.SendCommandProgressArgs cmdProgress =
            new Device.SendCommandProgressArgs("UtpRead", Api.Api.CommandDirection.ReadWithData, 0);
        cmdProgress.Status = String.Format("UtpRead({0}, {1}) tag:{2}", cmd, filename, transaction.Tag);
        ((Volume)UtpDevice).DoSendProgress(cmdProgress);


        while (!(transaction.CurrentState is DoneState))
        {
            UtpDevice.SendCommand(hDevice, transaction.CurrentState.Msg);
            if (transaction.CurrentState is GetDataState)
                transaction.CopyData();

            // Update the UI
            cmdProgress.Maximum = transaction.TotalSize;
            cmdProgress.Position = (Int32)transaction.CurrentSize;
            cmdProgress.Status = transaction.CurrentState.ToString() + " // pos:" + transaction.CurrentSize.ToString("#,#0");
            ((Volume)UtpDevice).DoSendProgress(cmdProgress);

            transaction.Next();
        }

        // tell the UI we are done
//            cmdProgress.Position = (Int32)totalTransferSize;
        cmdProgress.Status = transaction.CurrentState.ToString();
        cmdProgress.Error = ((DoneState)transaction.CurrentState).ResponseInfo;
        cmdProgress.InProgress = false;
        ((Volume)UtpDevice).DoSendProgress(cmdProgress);

        // Create the file 
        try
        {
            File.WriteAllBytes(filename, transaction.Data);
        }
        catch (Exception e)
        {
            Int32 retVal = Marshal.GetHRForException(e);
            Debug.WriteLine(String.Format("!ERROR: UpdateTransportProtocol.UtpRead({0}, {1}), tag:{2} - {3}", cmd, filename, TransactionTag, e.Message));
            return retVal;
        }

        return ((DoneState)transaction.CurrentState).ResponseInfo;

    }

    public Int32 UtpWrite(String cmd, String filename)
    {
        // Get the data to send
        Byte[] data = null;
        try
        {
            data = File.ReadAllBytes(filename);
        }
        catch (Exception e)
        {
            Int32 retVal = Marshal.GetHRForException(e);
            Debug.WriteLine(String.Format("!ERROR: UpdateTransportProtocol.UtpWrite({0}, {1}), tag:{2} - {3}", cmd, filename, TransactionTag, e.Message));
            return retVal;
        }

        WriteTransaction transaction = new WriteTransaction(this, cmd, data);

        // tell the UI we are beginning a command.
        Utils.ByteFormatConverter byteConverter = new Utils.ByteFormatConverter();
        Device.SendCommandProgressArgs cmdProgress =
            new Device.SendCommandProgressArgs("UtpWrite", Api.Api.CommandDirection.WriteWithData, transaction.TotalSize);
        cmdProgress.Status = String.Format(" UtpWrite({0}, {1}) tag:{2} totalSize:{3}", cmd, filename, transaction.Tag, byteConverter.ConvertToString(transaction.TotalSize));
        ((Volume)UtpDevice).DoSendProgress(cmdProgress);

        while (!(transaction.CurrentState is DoneState))
        {
            UtpDevice.SendCommand(hDevice, transaction.CurrentState.Msg);
            
            // Update the UI
            cmdProgress.Maximum = transaction.TotalSize;
            cmdProgress.Position = (Int32)transaction.CurrentSize;
            cmdProgress.Status = transaction.CurrentState.ToString() + " // pos:" + transaction.CurrentSize.ToString("#,#0");
            ((Volume)UtpDevice).DoSendProgress(cmdProgress);

            transaction.Next();
        }

        // tell the UI we are done
//            cmdProgress.Position = (Int32)totalTransferSize;
        cmdProgress.Status = transaction.CurrentState.ToString();
        cmdProgress.Error = ((DoneState)transaction.CurrentState).ResponseInfo;
        cmdProgress.InProgress = false;
        ((Volume)UtpDevice).DoSendProgress(cmdProgress);

        return ((DoneState)transaction.CurrentState).ResponseInfo;

    } // UtpWrite()

} // class UpdateTransportProtocol
