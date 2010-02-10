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

		// [XmlAttribute("addr")]
		unsigned int GetAddr()
		{ 
			unsigned int addr = 0; // default to 0

			CString attr = GetAttrValue(_T("addr"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				addr = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				addr = _tstoi64(attr);
			}

			return addr; 
		};

		// [XmlAttribute("file")]
		CString GetFile() { return CString(GetAttrValue(_T("file"))); };

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
		typedef enum _DeviceMode { Unknown, Recovery, Updater, UserMtp, UserMsc, User, ConnectedUnknown, Disconnected, IMXInfo } DeviceMode;
		
		static CString DeviceModeToString(DeviceMode mode)
		{
			CString str;
			switch (mode)
			{
				case Recovery:
					str = _T("Recovery");
					break;
				case Updater:
					str = _T("Updater");
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
				case IMXInfo:
					str = _T("IMXInfo");
					break;
				case Unknown:
				default:
					str = _T("Unknown");
					break;
			}

			return str;
		}

		static DeviceMode StringToDeviceMode(CString modeString)
		{
			DeviceMode mode = Unknown;

			if ( modeString == _T("Unknown") )
				mode = Unknown;
			else if ( modeString == _T("Recovery") || modeString == _T("Load"))
				mode = Recovery;
			else if ( modeString == _T("Updater") )
				mode = Updater;
			else if ( modeString == _T("UserMtp") )
				mode = UserMtp;
			else if ( modeString == _T("UserMsc") )
				mode = UserMsc;
			else if ( modeString == _T("User") )
				mode = User;
			else if ( modeString == _T("Disconnected") )
				mode = Disconnected;
			else if ( modeString == _T("IMXInfo") )
				mode = IMXInfo;

			return mode;
		}

		// [XmlAttribute("mode")]
		CString GetDeviceMode() { return GetAttrValue(_T("mode")); };

		// [XmlAttribute("vid")]
		CString GetVid() { return GetAttrValue(_T("vid")) ? GetAttrValue(_T("vid")) : _T("xxxx"); };

//			[XmlIgnore()]
//			public UInt16? Vid
//			{
//				get { if (XmlVid == null) return null; else return UInt16.Parse(XmlVid, System.Globalization.NumberStyles.HexNumber | System.Globalization.NumberStyles.AllowHexSpecifier); }
//				set { XmlVid = value == null ? null : value.ToString(); }
//			}

		// [XmlAttribute("pid")]
		CString GetPid() { return GetAttrValue(_T("pid")) ? GetAttrValue(_T("pid")) : _T("xxxx"); };
		CString GetMXType() { return GetAttrValue(_T("MXType")) ? GetAttrValue(_T("MXType")) : _T("xxxx"); }
		CString GetSecurity() { return GetAttrValue(_T("security")) ? GetAttrValue(_T("security")) : _T("xxxx"); }
		CString GetRAMType() { return GetAttrValue(_T("RAMType")) ? GetAttrValue(_T("RAMType")) : _T("xxxx"); }
		CString GetRamScript() { return GetAttrValue(_T("RamScript")) ? GetAttrValue(_T("RamScript")) : _T("xxxx"); }

		// [XmlAttribute("body")]
		CString GetCommandString() { return GetAttrValue(_T("body")); };


		CString ToString()
		{
			CString str;
			str.Format(_T("%s - %s/%s"), GetDeviceMode(), GetVid(), GetPid());

			return str;
		}
	};

	class FirmwareVersion : public XNode
	{
	};

	class Configuration : public XNode
	{
	public:
		// [XmlElement("DEV")]
		std::map<DeviceDesc::DeviceMode, DeviceDesc*> GetDeviceDescs()
		{
			std::map<DeviceDesc::DeviceMode, DeviceDesc*> devDescs;

			XNodes nodes = GetChilds(_T("DEV"));
			std::vector<LPXNode>::iterator it = nodes.begin();
			for ( ; it != nodes.end(); ++it )
				devDescs[DeviceDesc::StringToDeviceMode(((DeviceDesc*)*it)->GetDeviceMode())] = (DeviceDesc*)(*it);

			return devDescs;
		};

		// [XmlElement("VER")]
		FirmwareVersion* GetFirmwareVersion() { return (FirmwareVersion*)GetChild(_T("VER")); };
	};

	// [XmlElement(ElementName = "CFG")]
	Configuration* GetConfiguration()
	{
		return (Configuration*)GetChild(_T("CFG"));
	};

	// [XmlElement(ElementName="LIST")]
//		CArray<CommandList> CommandLists;
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
