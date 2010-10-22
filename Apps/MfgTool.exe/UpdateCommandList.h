/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// UpdateCommandList.h : header file
//

#pragma once

#include "../../Libs/WinSupport/XMLite.h"

#define OP_UPDATE_INCOMPLETE	-1L

class UCL : public XNode
{
public:
	class Command : public XNode
	{
	public:
		// [XmlAttribute("type")]
		CString GetType() { return CString(GetAttrValue(_T("type"))); };

		// [XmlAttribute("body")]
		CString GetBody() { return CString(GetAttrValue(_T("body"))); };

        // [XmlAttribute("if")]
		CString GetIFCondition() { return CString(GetAttrValue(_T("if"))); };

		// [XmlAttribute("address")]
		unsigned int GetAddress()
		{ 
			unsigned int address = 0; // default to 0

			CString attr = GetAttrValue(_T("address"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				address = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				address = _tstoi64(attr);
			}

			return address; 
		};

		// [XmlAttribute("CodeOffset")]
		unsigned int GetCodeOffset()
		{ 
			unsigned int CodeOffset = 0; // default to 0

			CString attr = GetAttrValue(_T("CodeOffset"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				CodeOffset = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				CodeOffset = _tstoi64(attr);
			}

			return CodeOffset; 
		};

		// [XmlAttribute("param1")]
		unsigned int GetParam1()
		{ 
			unsigned int param = 0; // default to 0

			CString attr = GetAttrValue(_T("param1"));

			if ( attr.CompareNoCase(_T("true")) == 0 )
			{
				param = 1;
			}
			else if ( attr.CompareNoCase(_T("false")) == 0 )
			{
				param = 0;
			}
			else if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				param = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				param = _tstoi64(attr);
			}

			return param; 
		};

		// [XmlAttribute("param2")]
		unsigned int GetParam2()
		{ 
			unsigned int param = 0; // default to 0

			CString attr = GetAttrValue(_T("param2"));

			if ( attr.CompareNoCase(_T("true")) == 0 )
			{
				param = 1;
			}
			else if ( attr.CompareNoCase(_T("false")) == 0 )
			{
				param = 0;
			}
			else if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				param = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				param = _tstoi64(attr);
			}

			return param; 
		};

		// [XmlAttribute("file")]
		CString GetFile() { return CString(GetAttrValue(_T("file"))); };
		
		// [XmlAttribute("loadSection")]
		CString GetLoadSection()
		{ 
			return CString(GetAttrValue(_T("loadSection")));
		};

		// [XmlAttribute("setSection")]
		CString GetSetSection()
		{ 
			return CString(GetAttrValue(_T("setSection")));
		};

		// [XmlAttribute("HasFlashHeader")]
		BOOL HasFlashHeader()
		{ 
			if ( GetAttrValue(_T("HasFlashHeader")) )
			{
				 CString attr = GetAttrValue(_T("HasFlashHeader"));
				 if ( attr.CompareNoCase(_T("TRUE")) == 0 )
					 return TRUE;
			}
			
			return FALSE;
		};

		// [XmlAttribute("mode")]
		BOOL FlashModeAligned()
		{ 
			if ( GetAttrValue(_T("mode")) )
			{
				 CString attr = GetAttrValue(_T("mode"));
				 if ( attr.CompareNoCase(_T("aligned")) == 0 )
					 return TRUE;
			}
			
			return FALSE;
		};

		// [XmlAttribute("readback")]
		BOOL Readback()
		{ 
			if ( GetAttrValue(_T("readback")) )
			{
				 CString attr = GetAttrValue(_T("readback"));
				 if ( attr.CompareNoCase(_T("false")) == 0 )
					 return FALSE;
			}
			
			return TRUE;
		};

		// [XmlAttribute("jump")]
		CString GetJump()
		{ 
			return CString(GetAttrValue(_T("action")));
		};

		// [XmlAttribute("timeout")]
		int GetTimeout() 
		{ 
			int timeout = 10; // default to 10 seconds

			LPCTSTR attr = GetAttrValue(_T("timeout"));
			if ( attr != NULL )
			{
				timeout = _ttoi(attr);
			}

			return timeout; 
		};

		// [XmlAttribute("onError")]
		CString GetOnError() { return CString(GetAttrValue(_T("onError"))); };

		CString ToString()
		{
			CString str;
			str.Format(_T("type=\"%s\" body=\"%s\" file=\"%s\" timeout=\"%d\" onError=\"%s\" text=\"%s\""), 
				GetType(), GetBody(), GetFile(), GetTimeout(), GetOnError(), value);
			return str;
		}
/*
		public Command() { }

		public Command(String type, String command, String filename, String description)
		{
			CmdType = type;
			CommandString = command;
			Filename = filename;
			Description = description;
		}
*/
	};
	
	class CommandList : public XNode
	{
	public:
		// [XmlAttribute("name")]
		CString GetName() { return CString(GetAttrValue(_T("name"))); };
		// [XmlAttribute("desc")]
		CString GetDescription() { return CString(GetAttrValue(_T("desc"))); };
		// [XmlElement(ElementName="CMD")]
		Command* GetCommand(size_t index)
		{
			XNodes cmds = GetChilds(_T("CMD"));

			if( index >= 0 && index < cmds.size() )
				return (Command*)cmds[index];
			
			return NULL;
		}

		typedef std::vector<Command*> Commands;

		Commands GetCommands()
		{
			Commands cmds;
			XNodes nodes = GetChilds(_T("CMD"));

			std::vector<LPXNode>::iterator it = nodes.begin();
			for( ; it != nodes.end(); ++(it))
			{
				cmds.push_back((Command*)(*it));
			}

			return cmds;
		}

		size_t GetCommandCount()
		{
			XNodes cmds = GetChilds(_T("CMD"));
			return cmds.size();
		}

		CString ToString()
		{
			CString str;
			str.Format(_T("%s, %s"), GetName(), GetDescription());
		}

	};

	class DeviceDesc : public XNode
	{
	public:
		// [XmlAttribute("name")]
		LPCTSTR GetChipType() { return GetAttrValue(_T("name")) ? GetAttrValue(_T("name")) : _T(""); }

		// [XmlAttribute("vid")]
		LPCTSTR GetVid() { return GetAttrValue(_T("vid")) ? GetAttrValue(_T("vid")) : _T("xxxx"); };

		// [XmlAttribute("pid")]
		LPCTSTR GetPid() { return GetAttrValue(_T("pid")) ? GetAttrValue(_T("pid")) : _T("xxxx"); };

		// [XmlAttribute("rev")]
//		LPCTSTR GetRevision() { return GetAttrValue(_T("rev")) ? GetAttrValue(_T("rev")) : _T(""); };

		// [XmlAttribute("ram")]
//		LPCTSTR GetMemoryType() { return GetAttrValue(_T("ram")) ? GetAttrValue(_T("ram")) : _T(""); };

//		LPCTSTR GetSecurity() { return GetAttrValue(_T("security")) ? GetAttrValue(_T("security")) : _T("xxxx"); }
//		LPCTSTR GetFlashModel() { return GetAttrValue(_T("FlashModel")) ? GetAttrValue(_T("FlashModel")) : _T("xxxx"); }
//		BOOL  GetBISWAP() { CString str = GetAttrValue(_T("BISWAP")) ? GetAttrValue(_T("BISWAP")) : _T("false"); return str.CompareNoCase(_T("true")) == 0;}
//		BOOL  GetInterleave() { CString str = GetAttrValue(_T("Interleave")) ? GetAttrValue(_T("Interleave")) : _T("false"); return str.CompareNoCase(_T("true")) == 0;}
//		BOOL  GetReadCheckBack() { CString str = GetAttrValue(_T("ReadCheckBack")) ? GetAttrValue(_T("ReadCheckBack")) : _T("false"); return str.CompareNoCase(_T("true")) == 0;}
//		BOOL  GetLBA() { CString str = GetAttrValue(_T("LBA")) ? GetAttrValue(_T("LBA")) : _T("false"); return str.CompareNoCase(_T("true")) == 0;}
//		BOOL  GetBBT() { CString str = GetAttrValue(_T("BBT")) ? GetAttrValue(_T("BBT")) : _T("false"); return str.CompareNoCase(_T("true")) == 0; }

		// [XmlAttribute("body")]
//		CString GetCommandString() { return GetAttrValue(_T("body")); };


		CString ToString()
		{
			CString str;
			str.Format(_T("%s - %s/%s"), GetChipType(), GetVid(), GetPid());

			return str;
		}
	};
	
	class DeviceState : public XNode
	{
	public:
		typedef enum DeviceState_t { Unknown, Recovery, BootStrap, Updater, RamKernel, UserMtp, UserMsc, User, ConnectedUnknown, Disconnected };
		
		static CString DeviceStateToString(DeviceState_t state)
		{
			CString str;
			switch (state)
			{
				case Recovery:
					str = _T("Recovery");
					break;
				case BootStrap:
					str = _T("BootStrap");
					break;
				case Updater:
					str = _T("Updater");
					break;
				case RamKernel:
					str = _T("RamKernel");
					break;
				case UserMtp:
					str = _T("UserMtp");
					break;
				case UserMsc:
					str = _T("UserMsc");
					break;
				case User:
					str = _T("User");
					break;
				case Disconnected:
					str = _T("Disconnected");
					break;
				case Unknown:
				default:
					str = _T("Unknown");
					break;
			}

			return str;
		}

		static DeviceState_t StringToDeviceState(CString stateString)
		{
			DeviceState_t state = Unknown;

			if ( stateString == _T("Unknown") )
				state = Unknown;
			else if ( stateString == _T("Recovery") )
				state = Recovery;
			else if ( stateString == _T("BootStrap") )
				state = BootStrap;
			else if ( stateString == _T("Updater") )
				state = Updater;
			else if ( stateString == _T("RamKernel") )
				state = RamKernel;
			else if ( stateString == _T("UserMtp") )
				state = UserMtp;
			else if ( stateString == _T("UserMsc") )
				state = UserMsc;
			else if ( stateString == _T("User") )
				state = User;
			else if ( stateString == _T("Disconnected") )
				state = Disconnected;

			return state;
		}

		// [XmlAttribute("name")]
		CString GetDeviceState() { return CString(GetAttrValue(_T("name"))); };

		// [XmlAttribute("dev")]
		LPCTSTR GetDeviceDesc() { return GetAttrValue(_T("dev")); };

		// [XmlAttribute("response")]
		LPCTSTR GetResponse() { return GetAttrValue(_T("response")); };
	};

	class FirmwareVersion : public XNode
	{
	};

	class Configuration : public XNode
	{
	public:
		// [XmlElement("STATE")]
		std::map<DeviceState::DeviceState_t, DeviceDesc*> GetDeviceStates()
		{
			std::map<DeviceState::DeviceState_t, DeviceDesc*> devStates;

			XNodes states = GetChilds(_T("STATE"));
			
			// for each STATE
			std::vector<LPXNode>::iterator state = states.begin();
			for ( ; state != states.end(); ++state )
			{
				devStates[DeviceState::StringToDeviceState(((DeviceState*)*state)->GetDeviceState())] = GetDeviceDesc(((DeviceState*)*state)->GetDeviceDesc());
			}
			
			return devStates;
		};

		// [XmlElement(ElementName="DEV")]
		DeviceDesc* GetDeviceDesc(LPCTSTR name)
		{
			XNodes devs = GetChilds(_T("DEV"));

			std::vector<LPXNode>::iterator dev = devs.begin();
			for( ; dev != devs.end(); ++(dev))
			{
				DeviceDesc* pDev = (DeviceDesc*)(*dev);
				CString chipType = pDev->GetChipType();
				if( chipType.CompareNoCase(name) == 0 )
				{
					return pDev;
				}
			}

			return NULL;
		}

		// [XmlElement("VER")]
		FirmwareVersion* GetFirmwareVersion() { return (FirmwareVersion*)GetChild(_T("VER")); };
	};

	// [XmlElement(ElementName = "CFG")]
	Configuration* GetConfiguration()
	{
		return (Configuration*)GetChild(_T("CFG"));
	};

	// [XmlElement(ElementName="LIST")]
	CommandList* GetCommandList(LPCTSTR name)
	{
		XNodes lists = GetChilds(_T("LIST"));

		std::vector<LPXNode>::iterator it = lists.begin();
		for( ; it != lists.end(); ++(it))
		{
			CommandList* pList = (CommandList*)(*it);
			if( pList->GetName() == name )
			{
				return pList;
			}
		}

		return NULL;
	}

	typedef std::vector<CommandList*> CommandLists;

	CommandLists GetCommandLists()
	{
		CommandLists cmdLists;
		XNodes nodeLists = GetChilds(_T("LIST"));

		std::vector<LPXNode>::iterator it = nodeLists.begin();
		for( ; it != nodeLists.end(); ++(it))
		{
			cmdLists.push_back((CommandList*)(*it));
		}

		return cmdLists;
	}
};
