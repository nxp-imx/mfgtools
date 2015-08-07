/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "MfgToolLib_Export.h"
#include "UpdateTransportProtocol.Api.h"
#ifdef __linux__
#include "libusbVolume.h"
#else
#include "Volume.h"
#endif
//only for debug
//extern LARGE_INTEGER g_t1, g_t2, g_t3, g_tc;
#if 0
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#endif

#define MIN_DATA_PROGRESS_BAR_CHANGE (6*1024*1024) //When a file is transferred, data transfer progress bar will be
//Updated once defined data quantity is reached.

class UpdateTransportProtocol
{
	private:
		Volume* m_pUtpDevice;
		HANDLE m_hDevice;
		/// The maximum consecutive Poll messages returning the same BusyState.ResponseInfo before Transaction times out.
		UINT m_MaxPollBusy;
		/// The maximum packet size that can be transfered by the UTP Device.
		__int64 m_MaxPacketSize;
		bool m_Disposed;
		///     The version of the device implemented UTP Protocol.
		int m_UtpVersion;

	public:
		BOOL m_bShouldStop;
		static UINT g_TransactionTag;
		UINT GetMaxPollBusy() { return m_MaxPollBusy; };
		__int64 GetMaxPacketSize() { return m_MaxPacketSize; };
		typedef enum { PowerDown, ChipReset, ResetToRecovery, ResetToUpdater } ResetType;
		libusb_device_handle *dev_handle;

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
				UINT GetTag() { return m_Tag; };
				CString GetCommand() { return m_Command; };
				UpdateTransportProtocol* GetProtocol() { return m_pUpdateProtocol; };
				__int64 GetLParam() { return m_LParam; };
				__int64 GetMaxPacketSize() { return m_pUpdateProtocol->GetMaxPacketSize(); };
				__int64 GetTotalSize() { return m_TotalSize; };
				__int64 GetCurrentSize() { return m_CurrentSize; };
				void SetCurrentSize(__int64 size) { m_CurrentSize = size; };
				void IncrementBusyCount() { ++m_BusyCount; };
				void ClearBusyCount() { m_BusyCount = 0; };
				UINT GetBusyCount() { return m_BusyCount; };
				UINT GetBusyDelay() { return m_BusyDelay; };

			protected:
				State* m_pCurrentState;
				UpdateTransportProtocol* m_pUpdateProtocol;
				UINT m_BusyCount;
				UINT m_Tag;
				CString m_Command;
				__int64 m_TotalSize;
				__int64 m_CurrentSize;
				__int64 m_LParam;
				///     The time in miliseconds to delay between utpBusy messages.
				int m_BusyDelay;
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

			__int64 PacketCount()
			{
				return m_CurrentSize / m_pUpdateProtocol->GetMaxPacketSize();
			}

			void Next()
			{
				State* pNextState = NULL;

				CString cmd = GetCommand();
				if( cmd.CompareNoCase(_T("!3")) == 0 )
				{
					pNextState = new DoneState(this, 0, m_pCurrentState->GetUtpMsg()->GetResponseString());
				}
				else
				{
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
								m_CurrentSize = std::min(m_TotalSize, m_TotalSize - m_pCurrentState->GetUtpMsg()->GetResponseInfo());
							}
							pNextState = new BusyState(this, m_pCurrentState->GetUtpMsg()->GetResponseInfo());
							if (KeepPolling())
								sleep(GetBusyDelay());
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
									ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()).c_str(),
									State::StateToString(m_pCurrentState->GetStateType()).c_str(),
									m_pCurrentState->GetUtpMsg()->ToString().c_str());
							//                        InvalidTransactionStateException e = new InvalidTransactionStateException(msg2);
							//                        throw e;
							pNextState = new DoneState(this, -65536, msg + _T(" (-65536)"));
							break;
					}
				}

				delete m_pCurrentState;
				m_pCurrentState = pNextState;
			}
	};

		class ReadTransaction : public Transaction
	{
		private:
			std::vector<UCHAR> m_Data;

		public:
			std::vector<UCHAR> GetData()
			{
				return m_Data;
			}

			ReadTransaction(UpdateTransportProtocol* pUtpInstance, CString command)
				: Transaction(pUtpInstance, command)
			{
				m_BusyDelay = 2;
				m_pCurrentState = new StartState(this);
			}

			__int64 GetPacketCount() { return m_CurrentSize/m_pUpdateProtocol->GetMaxPacketSize(); };

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
									ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()).c_str(),
									State::StateToString(m_pCurrentState->GetStateType()).c_str(),
									m_pCurrentState->GetUtpMsg()->ToString().c_str());
							pNextState = new DoneState(this, 0, m_pCurrentState->GetUtpMsg()->GetResponseString());
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
								sleep(GetBusyDelay());
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
									ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()).c_str(),
									State::StateToString(m_pCurrentState->GetStateType()).c_str(),
									m_pCurrentState->GetUtpMsg()->ToString().c_str());
							pNextState = new DoneState(this, -65536, msg + _T(" (-65536)"));
							break;
					}
				}

				delete m_pCurrentState;
				m_pCurrentState = pNextState;
			}

			void CopyData()
			{
				const UCHAR* pSrc = m_pCurrentState->GetUtpMsg()->GetDataPtr();
				//			std::vector<UCHAR>::iterator dest = m_Data.begin() + m_CurrentSize;
				for ( UINT i = 0; i < m_pCurrentState->GetUtpMsg()->GetTransferSize(); ++i )
				{
					m_Data[i] = pSrc[i];
				}
				m_CurrentSize += m_pCurrentState->GetUtpMsg()->GetTransferSize();
			}

	};

		class WriteTransaction : public Transaction
	{
		private:
			std::vector<UCHAR> m_Data;
			std::ifstream file;

		public:
			std::vector<UCHAR>& GetData(__int64 size)
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
#ifndef __linux__ 
				// open the file for binary reading
				file.open(filename, std::ios_base::binary, NULL);
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
#else
				file.open(filename, std::ios::in|std::ios::binary);
				struct stat64 stat;
				if(stat64(filename, &stat))
					return;
				m_LParam = m_TotalSize = stat.st_size;
				m_pCurrentState = new StartState(this);
#endif
			}

			__int64 GetPacketCount() { return m_CurrentSize/m_pUpdateProtocol->GetMaxPacketSize(); };

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
							sleep(GetBusyDelay());

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
									ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()).c_str(),
									State::StateToString(m_pCurrentState->GetStateType()).c_str(),
									m_pCurrentState->GetUtpMsg()->ToString().c_str());
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
								sleep(GetBusyDelay());
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
									ScsiUtpMsg::CodeToString(m_pCurrentState->GetUtpMsg()->GetResponseCode()).c_str(),
									State::StateToString(m_pCurrentState->GetStateType()).c_str(),
									m_pCurrentState->GetUtpMsg()->ToString().c_str());
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
				str.Format(_T("   StartState - %s // %s"), m_pMsg->ToString().c_str(), m_pMsg->GetResponseString().c_str());
				return str;
			}
	};

		class BusyState : public State
	{
		private:
			int m_TicksRemaining;

		public:
			BusyState(Transaction* pTransaction, int ticksRemaining)
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
				str.Format(_T("   BusyState - %s busyCount:%d // %s"), m_pMsg->ToString().c_str(), m_pTransaction->GetBusyCount(), m_pMsg->GetResponseString().c_str());
				return str;
			}
	};

		class DoneState : public State
	{
		private:
			UINT m_Tag;

		public:
			int m_ResponseInfo;
			CString m_ResponseString;

			DoneState(Transaction* pTransaction, int info, CString msg)
				: State(pTransaction, State::DoneState)
			{
				m_ResponseInfo = info;
				m_ResponseString = msg;
				m_Tag = pTransaction->GetTag();
			}

			int GetResponseInfo() { return m_ResponseInfo; };

			CString ToString()
			{
				CString str;
				str.Format(_T("   DoneState - tag:%d // %s"), m_Tag, m_ResponseString.c_str());
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
				__int64 numBytesToRead = std::min(pTransaction->GetProtocol()->GetMaxPacketSize(), pTransaction->GetTotalSize() - pTransaction->GetCurrentSize());

				m_pMsg = new api::Get(pTransaction->GetTag(), pTransaction->GetPacketCount(), numBytesToRead);
			}

			CString ToString()
			{
				CString str;
				str.Format(_T("   GetDataState - %s // %s"), m_pMsg->ToString().c_str(), m_pMsg->GetResponseString().c_str());
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
				__int64 numBytesToWrite = std::min(pTransaction->GetProtocol()->GetMaxPacketSize(), pTransaction->GetTotalSize() - pTransaction->GetCurrentSize());

				// Send it.
				m_pMsg = new api::Put(pTransaction->GetTag(), pTransaction->GetPacketCount(), pTransaction->GetData(numBytesToWrite));
				pTransaction->SetCurrentSize(pTransaction->GetCurrentSize() + numBytesToWrite);
			}

			CString ToString()
			{
				CString str;
				str.Format(_T("   PutDataState - %s // %s"), m_pMsg->ToString().c_str(), m_pMsg->GetResponseString().c_str());
				return str;
			}
	};



#ifndef __linux__
		UpdateTransportProtocol(Volume* pDevice)
		{
			m_pUtpDevice = pDevice;
			m_MaxPacketSize = Volume::MaxTransferSizeInBytes;
			m_MaxPollBusy = 3;
			m_Disposed = false;

			m_bShouldStop = FALSE;

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
#else
		UpdateTransportProtocol(libusbVolume* pDevice)
		{
			m_pUtpDevice = pDevice;
			m_MaxPacketSize = Volume::MaxTransferSizeInBytes;
			m_MaxPollBusy = 3;
			m_Disposed = false;

			m_bShouldStop = FALSE;

			// Dummy info
			Device::NotifyStruct dummyInfo(_T("Not used"), Device::NotifyStruct::dataDir_Off, 0);

			// Get the UTP Protocol version.
		}
#endif
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
				//QueryPerformanceFrequency(&g_tc);
				//QueryPerformanceCounter(&g_t1);
				//m_pUtpDevice->Unlock(m_hDevice, false);
				//QueryPerformanceCounter(&g_t2);
				//double d1 = (double)(g_t2.QuadPart-g_t1.QuadPart) / (double)g_tc.QuadPart;
				//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Volume Unlock times(%f)"), d1);
			}
			// CloseHandle(m_hDevice);
			m_Disposed = true;
		}

	public:
		int UtpDrop(CString cmd)
		{
			CString msg;

			// tell the UI we are beginning a command.
			Device::NotifyStruct cmdProgress(_T("UtpDrop"), Device::NotifyStruct::dataDir_ToDevice, 0);
			msg.Format(_T("UtpDrop(%s)"), cmd.c_str());
			cmdProgress.status = msg;
			//        ((Volume*)m_pUtpDevice)->Notify(cmdProgress);

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
			msg.Format(_T("   DoneState - tag:%d // %s"), modeMsg.GetTag(), modeMsg.GetResponseString().c_str());
			cmdProgress.status = msg;
			cmdProgress.error = modeMsg.GetResponseInfo();
			cmdProgress.inProgress = false;
			//		((Volume*)m_pUtpDevice)->Notify(cmdProgress);

			return modeMsg.GetResponseInfo();
		}

		int UtpCommand(CString cmd)
		{
			CommandTransaction transaction(this, cmd);

			// tell the UI we are beginning a command.
			Device::NotifyStruct cmdProgress(_T("UtpCommand"), Device::NotifyStruct::dataDir_ToDevice, 0);
			cmdProgress.status.Format(_T("UtpCommand(%s) tag:%d"), cmd.c_str(), transaction.GetTag());
			//		if(m_pUtpDevice != NULL)
			//			((Volume*)m_pUtpDevice)->Notify(cmdProgress);
			//		else
			//			return FALSE;
			if(m_pUtpDevice == NULL)
			{
				//TODO:
				return 0;
				return -1;
			}
			cmdProgress.maximum = (UINT)transaction.GetTotalSize();

			while (transaction.GetCurrentState()->GetStateType() != State::DoneState)
			{
				if(m_bShouldStop)
				{
					//TODO:
					return 0;
					return -1;
				}
				m_pUtpDevice->SendCommand(m_hDevice, *transaction.GetCurrentState()->GetUtpMsg(), NULL, cmdProgress);

				// Update the UI

				cmdProgress.position = (UINT)transaction.GetCurrentSize();
				//            cmdProgress.status = transaction.GetCurrentState()->ToString() + _T(" // pos:") + transaction.GetCurrentSize().ToString("#,#0");
				//cmdProgress.status.Format(_T("%s // pos:%d"), transaction.GetCurrentState()->ToString(), transaction.GetCurrentSize());
				//			((Volume*)m_pUtpDevice)->Notify(cmdProgress);

				transaction.Next();
			}

			// tell the UI we are done
			//            cmdProgress.Position = (Int32)totalTransferSize;
			cmdProgress.status = transaction.GetCurrentState()->ToString();
			cmdProgress.error = ((DoneState*)transaction.GetCurrentState())->GetResponseInfo();
			cmdProgress.inProgress = false;
			//		((Volume*)m_pUtpDevice)->Notify(cmdProgress);

			//TODO:
			return 0;
			return cmdProgress.error; // ((DoneState)transaction.CurrentState).ResponseInfo;
		}

		//    int UtpRead(CString cmd, CString filename, Device::UI_Callback callback)
		int UtpRead(CString cmd, CString filename, int cmdOpIndex)
		{
			ReadTransaction transaction(this, cmd);

			// tell the UI we are beginning a command.
			//		HANDLE hCallback = m_pUtpDevice->RegisterCallback(callback);
			Device::NotifyStruct cmdProgress(_T("UtpRead"), Device::NotifyStruct::dataDir_FromDevice, 0);
			cmdProgress.status.Format(_T("UtpRead(%s, %s) tag:%d"), cmd.c_str(), filename.c_str(), transaction.GetTag());
			//		((Volume*)m_pUtpDevice)->Notify(cmdProgress);


			while (transaction.GetCurrentState()->GetStateType() != State::DoneState)
			{
				(libusbVolume*)m_pUtpDevice->SendCommand(m_hDevice, *transaction.GetCurrentState()->GetUtpMsg(), NULL, cmdProgress);
				if (transaction.GetCurrentState()->GetStateType() == State::GetDataState)
					transaction.CopyData();

				// Update the UI
				cmdProgress.maximum = (UINT)transaction.GetTotalSize();
				cmdProgress.position = (UINT)transaction.GetCurrentSize();
				cmdProgress.status.Format(_T("%s // pos:%d"), transaction.GetCurrentState()->ToString().c_str(), transaction.GetCurrentSize());
				//			((Volume*)m_pUtpDevice)->Notify(cmdProgress);
				((Volume*)m_pUtpDevice)->NotifyUpdateUI(cmdOpIndex, cmdProgress.position, cmdProgress.maximum);

				transaction.Next();
			}

			// tell the UI we are done
			//            cmdProgress.Position = (Int32)totalTransferSize;
			cmdProgress.status = transaction.GetCurrentState()->ToString();
			cmdProgress.error = ((DoneState*)transaction.GetCurrentState())->GetResponseInfo();
			cmdProgress.inProgress = false;
			//		((Volume*)m_pUtpDevice)->Notify(cmdProgress);
			((Volume*)m_pUtpDevice)->NotifyUpdateUI(cmdOpIndex, cmdProgress.position, cmdProgress.maximum);

			//		m_pUtpDevice->UnregisterCallback(hCallback);
#ifdef WINVER
			//USES_CONVERSION;
			CString afilename=(filename.c_str());
#else
#define afilename filename
#endif

			// Create the file
			FILE * myFile;
			//CFileException fileException;
			myFile = std::fopen(afilename, "w");
			if ( myFile==NULL )
			{
				CString msg;
				msg.Format(_T("!ERROR(%d): UpdateTransportProtocol.UtpRead(%s, %s), tag:%d"), strerror(errno), cmd.c_str(), filename.c_str(), g_TransactionTag);
				TRACE(msg);
				//TODO:
				return 0;
				return errno;
			}
			std::fwrite( &transaction.GetData()[0],sizeof(transaction.GetData()[0]), transaction.GetData().size() ,myFile);
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

		//    int UtpWrite(CString cmd, CString filename, Device::UI_Callback callback)
		int UtpWrite(CString cmd, CString filename, int cmdOpIndex)
		{
#ifndef __linux__
			//#define TIMETEST
#ifdef TIMETEST
			LARGE_INTEGER liStartTime = {0}, liStopTime = {0};
			LARGE_INTEGER liTemp = {0}, timeCount = {0};
			QueryPerformanceFrequency(&liTemp);
			liTemp.QuadPart /= 1000;
			UINT BusyCnt = 0;
#endif
			//If @FILESIZE is found in a command body, then it indicates the macro ($FILESIZE) must be replaced by a real value of the size of the file to be sent.
			if ( cmd.Find(_T("@FILESIZE")) != -1 )
			{
				// get the length of the file

				WIN32_FILE_ATTRIBUTE_DATA FileAttrData;			if( !GetFileAttributesEx(filename, GetFileExInfoStandard,&FileAttrData) )
					return ERROR_OPEN_FAILED;

				CString sfileSize;
				sfileSize.Format(_T("%d"), FileAttrData.nFileSizeLow);

				cmd.Replace(_T("@FILESIZE"), sfileSize);
			}

			WriteTransaction transaction(this, cmd, filename);
			if (transaction.GetTotalSize() == 0)
				return ERROR_OPEN_FAILED;
			// When the data is transfering,the app will crash if pull away the USB connection crab.
			// Because the DEVICE_REMOVAL_EVT will come and the operation will be done "_devices.erase(device)" when the USB cable is pulled.
			// And m_pUtpDevice will be erased too.So all operation related to m_pUtpDevice will fail.
			// And may cause invalid addr access:Access violation reading location 0x??????????
			// So add a judgement to check the validity of m_pUtpDevice by compare to dwUtpDeviceAddr
			DWORD_PTR dwUtpDeviceAddr = (DWORD_PTR)m_pUtpDevice;
#else 
			//CString cmd, CString filename, int cmdOpIndex
			dev_handle = m_pUtpDevice->m_libusbdevHandle;
			int filelength = -1;

			struct stat64 stat;
			if(stat64(filename, &stat))
				return ERROR_OPEN_FAILED;
			CString sfileSize;
			sfileSize.Format(_T("%d"), stat.st_size);
			cmd.Replace(_T("@FILESIZE"), sfileSize);

			WriteTransaction transaction(this, cmd, filename);
			if(transaction.GetTotalSize()==0)
				return ERROR_OPEN_FAILED;

			// When the data is transfering,the app will crash if pull away the USB connection crab.
			// Because the DEVICE_REMOVAL_EVT will come and the operation will be done "_devices.erase(device)" when the USB cable is pulled.
			// And m_pUtpDevice will be erased too.So all operation related to m_pUtpDevice will fail.
			// And may cause invalid addr access:Access violation reading location 0x??????????
			// So add a judgement to check the validity of m_pUtpDevice by compare to dwUtpDeviceAddr
			DWORD_PTR dwUtpDeviceAddr = libusb_get_device_address(libusb_get_device(dev_handle));
#endif

			// tell the UI we are beginning a command.
			//		HANDLE hCallback = m_pUtpDevice->RegisterCallback(callback);
			//        Utils.ByteFormatConverter byteConverter = new Utils.ByteFormatConverter();
			//	m_pPortMgrDlg->UpdateUI(NULL, m_iPercentComplete, (int)myNewFwCommandSupport.GetFwComponent().size(), 0);       // STAGE 2 of the Load Operation
			Device::NotifyStruct cmdProgress(_T("UtpWrite"), Device::NotifyStruct::dataDir_ToDevice, (UINT)transaction.GetTotalSize());
			cmdProgress.status.Format(_T(" UtpWrite(%s, %s) tag:%d totalSize:%d"),cmd, filename, transaction.GetTag(), transaction.GetTotalSize());
			cmdProgress.error = ERROR_SUCCESS;
			//		((Volume*)m_pUtpDevice)->Notify(cmdProgress);
			cmdProgress.maximum = (UINT)transaction.GetTotalSize();
			UINT PreviousPosition = 0;
			cmdProgress.position = 0;
			DWORD DataTransferred = 0;

			((Volume*)m_pUtpDevice)->NotifyUpdateUI(cmdOpIndex, cmdProgress.position, cmdProgress.maximum);
			//for update ui
			while (transaction.GetCurrentState()->GetStateType() != State::DoneState)
			{
#ifdef TIMETEST
				QueryPerformanceCounter(&liStartTime);
#endif
#ifndef __linux__
				if ((UINT)m_pUtpDevice != dwUtpDeviceAddr)
					goto ERROR_NO_DEVICE;
				if(m_bShouldStop)
				{
					return -1;
				}
				PreviousPosition = cmdProgress.position;
				//Send data or command
				m_pUtpDevice->SendCommand(m_hDevice, *transaction.GetCurrentState()->GetUtpMsg(), NULL, cmdProgress);
				if(ERROR_SUCCESS != cmdProgress.error)
				{
					TRACE(_T("!!!!!!!!!!!!!!!!!!!!!!!!!Error returned by device in command sending, Error code: 0x%x, data transferred: 0x%x. Error info: %s. \
								\n"), cmdProgress.error, cmdProgress.position, cmdProgress.status);
					//Don't return since  it may succeed later.
					//break;
				}
#else
				if(m_bShouldStop)
				{
					return -1;
				}
				PreviousPosition = cmdProgress.position;
				//Send data or command
				m_pUtpDevice->SendCommand(m_hDevice, *transaction.GetCurrentState()->GetUtpMsg(), NULL, cmdProgress);
				if(ERROR_SUCCESS != cmdProgress.error)
				{
					TRACE(_T("!!!!!!!!!!!!!!!!!!!!!!!!!Error returned by device in command sending, Error code: 0x%x, data transferred: 0x%x. Error info: %s. \
								\n"), (unsigned int)cmdProgress.error, (unsigned int)cmdProgress.position, (const TCHAR*)cmdProgress.status);
					//Don't return since  it may succeed later.
					//break;
				}
#endif
#ifdef TIMETEST
				QueryPerformanceCounter(&liStopTime);
				timeCount.QuadPart = ((liStopTime.QuadPart - liStartTime.QuadPart)/liTemp.QuadPart);
				TRACE(_T("Command time = %I64d ms\n"), timeCount.QuadPart);
				QueryPerformanceCounter(&liStartTime);
#endif
				//copy data to internal buffer
				transaction.Next();

				cmdProgress.position = (UINT)transaction.GetCurrentSize();
				DataTransferred = cmdProgress.position - PreviousPosition;

#ifdef TIMETEST
				QueryPerformanceCounter(&liStopTime);
				timeCount.QuadPart = ((liStopTime.QuadPart - liStartTime.QuadPart)/liTemp.QuadPart);
				TRACE(_T("Data = 0x%x, Total = 0x%x, time = %I64d ms\n"), \
						DataTransferred, cmdProgress.position, timeCount.QuadPart);
				BusyCnt = 0;
#endif
				if(DataTransferred == 0)
				{
					//Here must be busy state.
#ifdef TIMETEST
					BusyCnt++;
					if(!(BusyCnt % 50))
						TRACE(_T("*************Device hang!!!!!!!!!!!!!!!!!!!!!!!!!\n"));
#endif
					continue;
				}
				// Update the UI
				//To improve performance, data transfer progress bar in UI will be updated only if 10MB data is
				//transferred since UI updating can be time-consuming.
				if((cmdProgress.position % MIN_DATA_PROGRESS_BAR_CHANGE) == 0)
				{
					cmdProgress.status.Format(_T("%s // pos:%d"), (const TCHAR*)transaction.GetCurrentState()->ToString(), (const TCHAR*)transaction.GetCurrentSize());
#ifndef __linux__
					if ((UINT)m_pUtpDevice != dwUtpDeviceAddr)
						goto ERROR_NO_DEVICE;
					//    			((Volume*)m_pUtpDevice)->Notify(cmdProgress);
#endif
					((Volume*)m_pUtpDevice)->NotifyUpdateUI(cmdOpIndex, cmdProgress.position, cmdProgress.maximum);
#ifdef TIMETEST
					QueryPerformanceCounter(&liStopTime);
					timeCount.QuadPart = ((liStopTime.QuadPart - liStartTime.QuadPart)/liTemp.QuadPart);
					TRACE(_T("TimeOfWrite: Notify timeused = %I64d ms\n"), timeCount.QuadPart);
					QueryPerformanceCounter(&liStartTime);
#endif
				}

			}
			// tell the UI we are done
			//            cmdProgress.Position = (Int32)totalTransferSize;
			cmdProgress.status = transaction.GetCurrentState()->ToString();
			cmdProgress.error = ((DoneState*)transaction.GetCurrentState())->GetResponseInfo();
			cmdProgress.inProgress = false;
#ifndef __linux__
			if (dwUtpDeviceAddr != (UINT)m_pUtpDevice)
				goto ERROR_NO_DEVICE;
			//		((Volume*)m_pUtpDevice)->Notify(cmdProgress);
#endif
			((Volume*)m_pUtpDevice)->NotifyUpdateUI(cmdOpIndex, cmdProgress.position, cmdProgress.maximum);

			//		m_pUtpDevice->UnregisterCallback(hCallback);
			if(cmdProgress.error != ERROR_SUCCESS)
				TRACE(_T("!!!!!!!!Error in data sending: Error code: 0x%x, data transferred: 0x%x. Error info: %s. \
							\n"), (unsigned int)cmdProgress.error, (unsigned int)cmdProgress.position, (const TCHAR*)cmdProgress.status);

			return cmdProgress.error;

#ifndef __linux__
ERROR_NO_DEVICE:
			TRACE(_T("The Device handle is destroyed.m_pUtpDevice is 0x%x, dwUtpdeviceAddr is 0x%x\r\n"),m_pUtpDevice,dwUtpDeviceAddr);
			return ERROR_INVALID_HANDLE;
#endif

		} // UtpWrite()

}; // class UpdateTransportProtocol
