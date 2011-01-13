/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
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
//#include <ios>
//#include <iostream>


class UpdateTransportProtocol
{
private:
	Volume* m_pUtpDevice;
    HANDLE m_hDevice;
    /// The maximum consecutive Poll messages returning the same BusyState.ResponseInfo before Transaction times out.
    uint32_t m_MaxPollBusy;
    /// The maximum packet size that can be transfered by the UTP Device.
    int64_t m_MaxPacketSize;
	bool m_Disposed;
    ///     The version of the device implemented UTP Protocol.
    int32_t m_UtpVersion;

public:
    static uint32_t g_TransactionTag;
	uint32_t GetMaxPollBusy() { return m_MaxPollBusy; };
	int64_t GetMaxPacketSize() { return m_MaxPacketSize; };
    typedef enum { PowerDown, ChipReset, ResetToRecovery, ResetToUpdater } ResetType;

	class State;

	class Transaction
    {
	public:
		Transaction(UpdateTransportProtocol* pUtpInstance, CString command)
			: m_pCurrentState(NULL)
			, m_pUpdateProtocol(pUtpInstance)
			, m_Command(command)
            , m_BusyCount(0)
			, m_Tag(0)
			, m_TotalSize(0)
			, m_CurrentSize(0)
			, m_LParam(0)
		{
			m_Tag = ++UpdateTransportProtocol::g_TransactionTag;
        }

		virtual ~Transaction()
		{
			if ( m_pCurrentState )
			{
				delete m_pCurrentState;
				m_pCurrentState = NULL;
			}
		}
        
		bool KeepPolling()
        {
            return m_BusyCount < m_pUpdateProtocol->GetMaxPollBusy();
        };

		virtual void Next() = 0;
		State* GetCurrentState() { return m_pCurrentState; };
		uint32_t GetTag() { return m_Tag; };
		CString GetCommand() { return m_Command; };
		UpdateTransportProtocol* GetProtocol() { return m_pUpdateProtocol; };
		int64_t GetLParam() { return m_LParam; };
		int64_t GetMaxPacketSize() { return m_pUpdateProtocol->GetMaxPacketSize(); };
		int64_t GetTotalSize() { return m_TotalSize; };
		int64_t GetCurrentSize() { return m_CurrentSize; };
		void SetCurrentSize(int64_t size) { m_CurrentSize = size; };
		void IncrementBusyCount() { ++m_BusyCount; };
		void ClearBusyCount() { m_BusyCount = 0; };
		uint32_t GetBusyCount() { return m_BusyCount; };
		uint32_t GetBusyDelay() { return m_BusyDelay; };

	protected:
		State* m_pCurrentState;
        UpdateTransportProtocol* m_pUpdateProtocol;
        uint32_t m_BusyCount;
		uint32_t m_Tag;
        CString m_Command;
        int64_t m_TotalSize;
        int64_t m_CurrentSize;
		int64_t m_LParam;
		///     The time in miliseconds to delay between utpBusy messages.
		int32_t m_BusyDelay;
    };

    class CommandTransaction : public Transaction
    {
	public:
		CommandTransaction(UpdateTransportProtocol* pUtpInstance, CString command)
            : Transaction(pUtpInstance, command)
        {
			m_BusyDelay = 250;
            m_pCurrentState = new StartState(this);
        }

		int64_t PacketCount()
        {
            return m_CurrentSize / m_pUpdateProtocol->GetMaxPacketSize();
        }

        void Next()
        {
			State* pNextState = NULL;

			switch (m_pCurrentState->GetUtpMsg()->GetResponseCode())
            {
				case ScsiUtpMsg::BUSY:
					if (m_pCurrentState->GetStateType() == State::StartState)
                    {
                        m_TotalSize = m_pCurrentState->GetUtpMsg()->GetResponseInfo();
                        m_CurrentSize = 0;

                    }
                    else
                    {
                        m_CurrentSize = min(m_TotalSize, m_TotalSize - m_pCurrentState->GetUtpMsg()->GetResponseInfo());
                    }   
                    pNextState = new BusyState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo());
                    if (KeepPolling())
                        Sleep(GetBusyDelay());
                    else
					{
						delete pNextState; pNextState = NULL;
                        pNextState = new DoneState(this, -65535, _T("ERROR: Polling stalled. (-65535)"));
					}
                    break;
				case ScsiUtpMsg::EXIT:
                    pNextState = new DoneState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo(), m_pCurrentState->GetUtpMsg()->GetResponseString());
                    break;
				case ScsiUtpMsg::PASS:
                    pNextState = new DoneState(this, 0, m_pCurrentState->GetUtpMsg()->GetResponseString());
                    break;
                default:
//                        String msg = String.Format("In: {0}, Sent: {1}\r\n Response: {2}\r\n Moving to: {3}");
                    CString msg;
					msg.Format(_T("Illegal ResponseCode(%s) for %s->%s message."), 
						ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()),
						State::StateToString(m_pCurrentState->GetStateType()),
						m_pCurrentState->GetUtpMsg()->ToString()); 
//                        InvalidTransactionStateException e = new InvalidTransactionStateException(msg2);
//                        throw e;
                    pNextState = new DoneState(this, -65536, msg + _T(" (-65536)"));
                    break;
            }

			delete m_pCurrentState;
			m_pCurrentState = pNextState;
        }
    };

    class ReadTransaction : public Transaction
    {
	private:
		std::vector<uint8_t> m_Data;

	public:
        std::vector<uint8_t> GetData()
		{
            return m_Data;
        }

        ReadTransaction(UpdateTransportProtocol* pUtpInstance, CString command)
            : Transaction(pUtpInstance, command)
        {
			m_BusyDelay = 2;
            m_pCurrentState = new StartState(this);
        }

        int64_t GetPacketCount() { return m_CurrentSize/m_pUpdateProtocol->GetMaxPacketSize(); };

		void Next()
        {
			State* pNextState = NULL;

			if (m_pCurrentState->GetStateType() == State::StartState)
            {
				switch (m_pCurrentState->GetUtpMsg()->GetResponseCode())
                {
					case ScsiUtpMsg::SIZE:
                        m_TotalSize = m_pCurrentState->GetUtpMsg()->GetResponseSize();
                        m_CurrentSize = 0;
                        m_Data.resize((size_t)m_TotalSize);
						// Data = new Byte[TotalSize];
                        pNextState = new GetDataState(this);
                        break;
					case ScsiUtpMsg::EXIT:
                        pNextState = new DoneState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo(), m_pCurrentState->GetUtpMsg()->GetResponseString());
                        break;
                    default:
//                            throw new Exception("Illegal Response Code for this transaction.");
                        CString msg;
						msg.Format(_T("Illegal ResponseCode(%s) for %s->%s message."), 
							ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()),
							State::StateToString(m_pCurrentState->GetStateType()),
							m_pCurrentState->GetUtpMsg()->ToString());
                        pNextState = new DoneState(this, -65536, msg + _T(" (-65536)"));
                        break;
                }
            }
            else
            {
                switch (m_pCurrentState->GetUtpMsg()->GetResponseCode())
                {
					case ScsiUtpMsg::BUSY:
                        pNextState = new BusyState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo());
                        if (KeepPolling())
                            Sleep(GetBusyDelay());
                        else
						{
							delete pNextState;
                            pNextState = new DoneState(this, -65535, _T("ERROR: Polling stalled. (-65535)"));
						}
                        break;
					case ScsiUtpMsg::PASS:
                        if (m_CurrentSize < m_TotalSize)
                            pNextState = new GetDataState(this);
                        else
                            pNextState = new DoneState(this, 0, m_pCurrentState->GetUtpMsg()->GetResponseString());
                        break;
					case ScsiUtpMsg::EXIT:
                        pNextState = new DoneState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo(), m_pCurrentState->GetUtpMsg()->GetResponseString());
                        break;
                    default:
						CString msg;
						msg.Format(_T("Illegal ResponseCode(%s) for %s->%s message."), 
							ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()),
							State::StateToString(m_pCurrentState->GetStateType()),
							m_pCurrentState->GetUtpMsg()->ToString());
                        pNextState = new DoneState(this, -65536, msg + _T(" (-65536)"));
                        break;
                }
            }

			delete m_pCurrentState;
			m_pCurrentState = pNextState;
        }

        void CopyData()
        {
			const uint8_t* pSrc = m_pCurrentState->GetUtpMsg()->GetDataPtr();
//			std::vector<uint8_t>::iterator dest = m_Data.begin() + m_CurrentSize;
			for ( uint32_t i = 0; i < m_pCurrentState->GetUtpMsg()->GetTransferSize(); ++i )
			{
				m_Data[i] = pSrc[i];
			}
            m_CurrentSize += m_pCurrentState->GetUtpMsg()->GetTransferSize();
        }

    };

    class WriteTransaction : public Transaction
    {
	private:
		std::vector<uint8_t> m_Data;
		std::ifstream file;

	public:
        std::vector<uint8_t>& GetData(int64_t size)
		{
			if(m_Data.size()!=size)
				m_Data.resize((size_t)size);

			// read the file
			file.read(reinterpret_cast<char*>(&m_Data[0]), (size_t)size);

            return m_Data;
        }

        WriteTransaction(UpdateTransportProtocol* pUtpInstance, CString command, CString filename)
            : Transaction(pUtpInstance, command)
        {
			m_BusyDelay = 2;
			// open the file for binary reading
			file.open(filename, std::ios_base::binary);
			if(!file.is_open())
			{
				CString msg; msg.Format(_T("!ERROR(%d): UpdateTransportProtocol.UtpWrite(%s, %s), tag:%d"), ERROR_OPEN_FAILED, command, filename, g_TransactionTag);
				TRACE(msg);
			}
			else
			{
				// get the length of the file

				WIN32_FILE_ATTRIBUTE_DATA FileAttrData;
				if( !GetFileAttributesEx(filename, GetFileExInfoStandard,&FileAttrData) )
					return;

				m_LParam = m_TotalSize = FileAttrData.nFileSizeLow;

				m_pCurrentState = new StartState(this);				
			}
        }

        int64_t GetPacketCount() { return m_CurrentSize/m_pUpdateProtocol->GetMaxPacketSize(); };

        void Next()
        {
			State* pNextState = NULL;

			if (m_pCurrentState->GetStateType() == State::StartState)
            {
				switch (m_pCurrentState->GetUtpMsg()->GetResponseCode())
                {
					case ScsiUtpMsg::BUSY:
                        m_CurrentSize = 0;

                        pNextState = new BusyState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo());
                        Sleep(GetBusyDelay());
                        
                        break;
                    
					case ScsiUtpMsg::EXIT:
                        pNextState = new DoneState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo(), m_pCurrentState->GetUtpMsg()->GetResponseString());
                        break;
					case ScsiUtpMsg::PASS:
                        pNextState = new PutDataState(this);
                        break;
                    default:
						CString msg;
						msg.Format(_T("Illegal ResponseCode(%s) for %s->%s message."), 
							ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()),
							State::StateToString(m_pCurrentState->GetStateType()),
							m_pCurrentState->GetUtpMsg()->ToString());
                        pNextState = new DoneState(this, -65536, msg + _T(" (-65536)"));
                        break;
                }
            }
            else
            {
                switch (m_pCurrentState->GetUtpMsg()->GetResponseCode())
                {
					case ScsiUtpMsg::BUSY:
                        pNextState = new BusyState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo());
                        if (KeepPolling())
                            Sleep(GetBusyDelay());
                        else
						{
							delete pNextState;
                            pNextState = new DoneState(this, -65535, _T("ERROR: Polling stalled. (-65535)"));
						}
                        break;

					case ScsiUtpMsg::PASS:
                        if (m_CurrentSize < m_TotalSize)
                            pNextState = new PutDataState(this);
                        else
                            pNextState = new DoneState(this, 0, m_pCurrentState->GetUtpMsg()->GetResponseString());
                        break;
					case ScsiUtpMsg::EXIT:
                        pNextState = new DoneState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo(), m_pCurrentState->GetUtpMsg()->GetResponseString());
                        break;
                    default:
						CString msg;
						msg.Format(_T("Illegal ResponseCode(%s) for %s->%s message."), 
							ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()),
							State::StateToString(m_pCurrentState->GetStateType()),
							m_pCurrentState->GetUtpMsg()->ToString());
                        pNextState = new DoneState(this, -65536, msg + _T(" (-65536)"));
                        break;
                }
            }
			delete m_pCurrentState;
			m_pCurrentState = pNextState;

			// Close the file;
			if ( m_pCurrentState->GetStateType() == State::DoneState )
				file.close();
        }

    };

    class State
    {
	public:
		typedef enum { StartState = 0, DoneState, BusyState, GetDataState, PutDataState } StateType;
		
		static CString StateToString(StateType stateType)
		{
			CString str;

			switch (stateType)
			{
			case StartState:
				str = _T("StartState");
				break;
			case DoneState:
				str = _T("DoneState");
				break;
			case GetDataState:
				str = _T("GetDataState");
				break;
			case PutDataState:
				str = _T("PutDataState");
				break;
			default:
				str = _T("UnknownState");
				break;
			}

			return str;
		};

		State(Transaction* pTransaction, StateType stateType)
			: m_pTransaction(pTransaction)
			, m_StateType(stateType)
			, m_pMsg(NULL)
        {
		}

		virtual ~State()
		{
			if ( m_pMsg )
			{
				delete m_pMsg;
				m_pMsg = NULL;
			}
		}

		ScsiUtpMsg* GetUtpMsg() { return m_pMsg; };
		StateType GetStateType() { return m_StateType; };
		virtual CString ToString() = 0;

	protected:
		Transaction* m_pTransaction;
		ScsiUtpMsg* m_pMsg;
		StateType m_StateType;
    };

    class StartState : public State
    {
	public:
		StartState(Transaction* pTransaction)
			: State(pTransaction, State::StartState)
        {
			m_pMsg = new api::Exec(pTransaction->GetTag(), pTransaction->GetLParam(), pTransaction->GetCommand());
        }

        CString ToString()
        {
            CString str;
			str.Format(_T("   StartState - %s // %s"), m_pMsg->ToString(), m_pMsg->GetResponseString());
			return str;
        }
    };

    class BusyState : public State
    {
	private:
		int32_t m_TicksRemaining;

	public:
		BusyState(Transaction* pTransaction, int32_t ticksRemaining)
			: State(pTransaction, State::BusyState)
        {
			m_TicksRemaining = ticksRemaining;
			m_pMsg = new api::Poll(pTransaction->GetTag(), 0);

			if (pTransaction->GetCurrentState()->GetStateType() == State::BusyState)
            {
                if (((BusyState*)(pTransaction->GetCurrentState()))->m_TicksRemaining == ticksRemaining)
					pTransaction->IncrementBusyCount();
                else
					pTransaction->ClearBusyCount();
            }
            else
				pTransaction->ClearBusyCount();
        }

        CString ToString()
        {
            CString str;
			str.Format(_T("   BusyState - %s busyCount:%d // %s"), m_pMsg->ToString(), m_pTransaction->GetBusyCount(), m_pMsg->GetResponseString());
			return str;
        }
    };

    class DoneState : public State
    {
	private:
		uint32_t m_Tag;
	
	public:
        int32_t m_ResponseInfo;
		CString m_ResponseString;

		DoneState(Transaction* pTransaction, int32_t info, CString msg)
			: State(pTransaction, State::DoneState)
        {
            m_ResponseInfo = info;
            m_ResponseString = msg;
            m_Tag = pTransaction->GetTag();
        }

		int32_t GetResponseInfo() { return m_ResponseInfo; };

        CString ToString()
        {
            CString str;
			str.Format(_T("   DoneState - tag:%d // %s"), m_Tag, m_ResponseString);
			return str;
        }
    };

    class GetDataState : public State
    {
	public:
		GetDataState(ReadTransaction* pTransaction)
			: State(pTransaction, State::GetDataState)
        {
            // How much data to get?
            int64_t numBytesToRead = min(pTransaction->GetProtocol()->GetMaxPacketSize(), pTransaction->GetTotalSize() - pTransaction->GetCurrentSize());

			m_pMsg = new api::Get(pTransaction->GetTag(), pTransaction->GetPacketCount(), numBytesToRead);
        }

        CString ToString()
        {
			CString str;
			str.Format(_T("   GetDataState - %s // %s"), m_pMsg->ToString(), m_pMsg->GetResponseString());
			return str;
        }
    };

    class PutDataState : public State
    {
	public:
		PutDataState(WriteTransaction* pTransaction)
			: State(pTransaction, State::PutDataState)
        {
// TODO: CLW - USE THIS WHEN PAUL FIXES FW TO NOT DEPEND ON SECTOR SIZES 
//                Int64 numBytesToWrite = Math.Min(transaction.UpdateProtocol.MaxPacketSize, transaction.TotalSize - transaction.CurrentSize);
//                Byte[] buffer = new Byte[numBytesToWrite];
//                Array.Copy(transaction.Data, transaction.CurrentSize, buffer, 0, buffer.LongLength);

            // Get some data to put.
            int64_t numBytesToWrite = min(pTransaction->GetProtocol()->GetMaxPacketSize(), pTransaction->GetTotalSize() - pTransaction->GetCurrentSize());

			// Send it.
			m_pMsg = new api::Put(pTransaction->GetTag(), pTransaction->GetPacketCount(), pTransaction->GetData(numBytesToWrite));
            pTransaction->SetCurrentSize(pTransaction->GetCurrentSize() + numBytesToWrite);
        }
    
        CString ToString()
        {
			CString str;
            str.Format(_T("   PutDataState - %s // %s"), m_pMsg->ToString(), m_pMsg->GetResponseString());
			return str;
        }
    };



    UpdateTransportProtocol(Volume* pDevice)
    {
        m_pUtpDevice = pDevice;
		m_MaxPacketSize = Volume::MaxTransferSizeInBytes;
		m_MaxPollBusy = 3;
		m_Disposed = false;

		// Dummy info
		Device::NotifyStruct dummyInfo(_T("Not used"), Device::NotifyStruct::dataDir_Off, 0);

		// Open the device.
		m_hDevice = m_pUtpDevice->Lock(Volume::LockType_Logical);

        // Get the UTP Protocol version.
		Poll pollMsg(UpdateTransportProtocol::g_TransactionTag, Poll::GetUtpVersion);
        m_pUtpDevice->SendCommand(m_hDevice, pollMsg, NULL, dummyInfo);
		if (pollMsg.GetResponseCode() == ScsiUtpMsg::EXIT)
            m_UtpVersion = pollMsg.GetResponseInfo();
    }

    ~UpdateTransportProtocol()
    {
        Dispose(false);
    }

    void Dispose()
    {
        Dispose(true);
    }

private:
    void Dispose(bool isDisposing)
    {
        if (!m_Disposed)
        {
            m_pUtpDevice->Unlock(m_hDevice, false);
        }
        CloseHandle(m_hDevice);
        m_Disposed = true;
    }

public:
	int32_t UtpDrop(CString cmd)
    {
        CString msg;

		// tell the UI we are beginning a command.
		Device::NotifyStruct cmdProgress(_T("UtpDrop"), Device::NotifyStruct::dataDir_ToDevice, 0);
		msg.Format(_T("UtpDrop(%s)"), cmd);
		cmdProgress.status = msg;
        ((Volume*)m_pUtpDevice)->Notify(cmdProgress);

        // Unlock the device, but don't close it.
		((Volume*)m_pUtpDevice)->Unlock(m_hDevice, false);

        // Send the ModeChange message.
		Exec modeMsg(++UpdateTransportProtocol::g_TransactionTag, 0, cmd);
        m_pUtpDevice->SendCommand(m_hDevice, modeMsg, NULL, cmdProgress);
		if (modeMsg.GetResponseCode() == ScsiUtpMsg::PASS)
        {
            // Set _Disposed to true so we don't try to unlock the device 
            // since it should be disconnecting from the bus and Windows will take care of it.
            m_Disposed = true;
        }

        // tell the UI we are done
		msg.Format(_T("   DoneState - tag:%d // %s"), modeMsg.GetTag(), modeMsg.GetResponseString());
		cmdProgress.status = msg;
		cmdProgress.error = modeMsg.GetResponseInfo();
		cmdProgress.inProgress = false;
		((Volume*)m_pUtpDevice)->Notify(cmdProgress);

        return modeMsg.GetResponseInfo();
    }

    int32_t UtpCommand(CString cmd)
    {
		CommandTransaction transaction(this, cmd);

        // tell the UI we are beginning a command.
		Device::NotifyStruct cmdProgress(_T("UtpCommand"), Device::NotifyStruct::dataDir_ToDevice, 0);
		cmdProgress.status.Format(_T("UtpCommand(%s) tag:%d"), cmd, transaction.GetTag());
		((Volume*)m_pUtpDevice)->Notify(cmdProgress);
		cmdProgress.maximum = (uint32_t)transaction.GetTotalSize();

		while (transaction.GetCurrentState()->GetStateType() != State::DoneState)
        {
			m_pUtpDevice->SendCommand(m_hDevice, *transaction.GetCurrentState()->GetUtpMsg(), NULL, cmdProgress);

            // Update the UI

            cmdProgress.position = (uint32_t)transaction.GetCurrentSize();
//            cmdProgress.status = transaction.GetCurrentState()->ToString() + _T(" // pos:") + transaction.GetCurrentSize().ToString("#,#0");
			//cmdProgress.status.Format(_T("%s // pos:%d"), transaction.GetCurrentState()->ToString(), transaction.GetCurrentSize());
			((Volume*)m_pUtpDevice)->Notify(cmdProgress);

            transaction.Next();
        }

        // tell the UI we are done
//            cmdProgress.Position = (Int32)totalTransferSize;
        cmdProgress.status = transaction.GetCurrentState()->ToString();
		cmdProgress.error = ((DoneState*)transaction.GetCurrentState())->GetResponseInfo();
        cmdProgress.inProgress = false;
		((Volume*)m_pUtpDevice)->Notify(cmdProgress);

        return cmdProgress.error; // ((DoneState)transaction.CurrentState).ResponseInfo;
    }

    int32_t UtpRead(CString cmd, CString filename, Device::UI_Callback callback)
    {
        ReadTransaction transaction(this, cmd);

        // tell the UI we are beginning a command.
		HANDLE hCallback = m_pUtpDevice->RegisterCallback(callback);
		Device::NotifyStruct cmdProgress(_T("UtpRead"), Device::NotifyStruct::dataDir_FromDevice, 0);
        cmdProgress.status.Format(_T("UtpRead(%s, %s) tag:%d"), cmd, filename, transaction.GetTag());
		((Volume*)m_pUtpDevice)->Notify(cmdProgress);


		while (transaction.GetCurrentState()->GetStateType() != State::DoneState)
        {
			m_pUtpDevice->SendCommand(m_hDevice, *transaction.GetCurrentState()->GetUtpMsg(), NULL, cmdProgress);
			if (transaction.GetCurrentState()->GetStateType() == State::GetDataState)
                transaction.CopyData();

            // Update the UI
            cmdProgress.maximum = (uint32_t)transaction.GetTotalSize();
            cmdProgress.position = (uint32_t)transaction.GetCurrentSize();
            cmdProgress.status.Format(_T("%s // pos:%d"), transaction.GetCurrentState()->ToString(), transaction.GetCurrentSize());
			((Volume*)m_pUtpDevice)->Notify(cmdProgress);

            transaction.Next();
        }

        // tell the UI we are done
//            cmdProgress.Position = (Int32)totalTransferSize;
        cmdProgress.status = transaction.GetCurrentState()->ToString();
		cmdProgress.error = ((DoneState*)transaction.GetCurrentState())->GetResponseInfo();
        cmdProgress.inProgress = false;
		((Volume*)m_pUtpDevice)->Notify(cmdProgress);

		m_pUtpDevice->UnregisterCallback(hCallback);

		// Create the file 
        CFile myFile;
		CFileException fileException;

		if ( !myFile.Open( filename, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &fileException ) )
		{
			CString msg;
			msg.Format(_T("!ERROR(%d): UpdateTransportProtocol.UtpRead(%s, %s), tag:%d"), fileException.m_cause, cmd, filename, g_TransactionTag);
			TRACE(msg);
			return fileException.m_cause;
		}
		myFile.Write( &transaction.GetData()[0], transaction.GetData().size() );
/*
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
*/
        return ((DoneState*)transaction.GetCurrentState())->GetResponseInfo();
    }

    int32_t UtpWrite(CString cmd, CString filename, Device::UI_Callback callback)
    {
#define TIMETEST 0
#ifdef TIMETEST 
		LARGE_INTEGER liStartTime = {0}, liStopTime = {0};
		LARGE_INTEGER liTemp = {0}, timeCount = {0};
		QueryPerformanceFrequency(&liTemp);
#endif
		//If @FILESIZE is found in a command body, then it indicates the macro ($FILESIZE) must be replaced by a real value of the size of the file to be sent. 
		if ( cmd.Find(_T("@FILESIZE")) != -1 )
		{
			// get the length of the file
			WIN32_FILE_ATTRIBUTE_DATA FileAttrData;
			if( !GetFileAttributesEx(filename, GetFileExInfoStandard,&FileAttrData) )
				return ERROR_OPEN_FAILED;

			CString sfileSize;
			sfileSize.Format(_T("%d"), FileAttrData.nFileSizeLow);

			cmd.Replace(_T("@FILESIZE"), sfileSize);
		}

        WriteTransaction transaction(this, cmd, filename);
		if(transaction.GetTotalSize()==0)
			return ERROR_OPEN_FAILED;

		// When the data is transfering,the app will crash if pull away the USB connection crab.
		// Because the DEVICE_REMOVAL_EVT will come and the operation will be done "_devices.erase(device)" when the USB cable is pulled.
		// And m_pUtpDevice will be erased too.So all operation related to m_pUtpDevice will fail.
		// And may cause invalid addr access:Access violation reading location 0x??????????
		// So add a judgement to check the validity of m_pUtpDevice by compare to dwUtpDeviceAddr 
		DWORD dwUtpDeviceAddr = (DWORD)m_pUtpDevice;
        // tell the UI we are beginning a command.
		HANDLE hCallback = m_pUtpDevice->RegisterCallback(callback);
//        Utils.ByteFormatConverter byteConverter = new Utils.ByteFormatConverter();
//	m_pPortMgrDlg->UpdateUI(NULL, m_iPercentComplete, (int)myNewFwCommandSupport.GetFwComponent().size(), 0);       // STAGE 2 of the Load Operation
		Device::NotifyStruct cmdProgress(_T("UtpWrite"), Device::NotifyStruct::dataDir_ToDevice, (uint32_t)transaction.GetTotalSize());
        cmdProgress.status.Format(_T(" UtpWrite(%s, %s) tag:%d totalSize:%d"), cmd, filename, transaction.GetTag(), transaction.GetTotalSize());
		((Volume*)m_pUtpDevice)->Notify(cmdProgress);

		while (transaction.GetCurrentState()->GetStateType() != State::DoneState)
        {
			#ifdef TIMETEST                       
			QueryPerformanceCounter(&liStartTime);
			#endif
			if ((uint32_t)m_pUtpDevice != dwUtpDeviceAddr)
				goto ERROR_NO_DEVICE;
			m_pUtpDevice->SendCommand(m_hDevice, *transaction.GetCurrentState()->GetUtpMsg(), NULL, cmdProgress);
            
            // Update the UI
            cmdProgress.maximum = (uint32_t)transaction.GetTotalSize();
            cmdProgress.position = (uint32_t)transaction.GetCurrentSize();
			cmdProgress.status.Format(_T("%s // pos:%d"), transaction.GetCurrentState()->ToString(), transaction.GetCurrentSize());
			//Tina +
			if ((uint32_t)m_pUtpDevice != dwUtpDeviceAddr)
				goto ERROR_NO_DEVICE;
			((Volume*)m_pUtpDevice)->Notify(cmdProgress);

            transaction.Next();

			#ifdef TIMETEST
			QueryPerformanceCounter(&liStopTime);        
			timeCount.QuadPart = ((liStopTime.QuadPart - liStartTime.QuadPart)*1000000/liTemp.QuadPart);
			//TRACE(_T("Time Test:write request total time = %I64d us\n"), timeCount.QuadPart);  
			#endif
        }

        // tell the UI we are done
//            cmdProgress.Position = (Int32)totalTransferSize;
        cmdProgress.status = transaction.GetCurrentState()->ToString();
		cmdProgress.error = ((DoneState*)transaction.GetCurrentState())->GetResponseInfo();
        cmdProgress.inProgress = false;
		if (dwUtpDeviceAddr != (uint32_t)m_pUtpDevice)
			goto ERROR_NO_DEVICE;
		((Volume*)m_pUtpDevice)->Notify(cmdProgress);

		m_pUtpDevice->UnregisterCallback(hCallback);

		return cmdProgress.error;

	ERROR_NO_DEVICE:
		TRACE(_T("The Device handle is destroyed.m_pUtpDevice is 0x%x, dwUtpdeviceAddr is 0x%x\r\n"),m_pUtpDevice,dwUtpDeviceAddr);
		return ERROR_INVALID_HANDLE;
    } // UtpWrite()

}; // class UpdateTransportProtocol
