/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using System.Xml.Serialization;

namespace UniversalUpdater
{
    [XmlRoot("UCL")]
    public class UpdaterOperations
    {
        [XmlElement(ElementName="LIST")]
        public CommandList[] CommandLists;

        [XmlElement(ElementName = "CFG")]
        public Configuration ConfigData;

        public CommandList LoadOperation
        {
            get
            {
                foreach (CommandList list in CommandLists)
                {
                    if (list.Name == "Load")
                        return list;
                }

                return null;
            }
        }
    }

    public class CommandList
    {
        [XmlAttribute("name")]
        public String Name;

        [XmlAttribute("desc")]
        public String Description;

        [XmlElement(ElementName="CMD")]
        public Command[] Commands;

        public override string ToString()
        {
            return String.Format("{0}, {1}", Name, Description);
        }
    }

    public class Command
    {
        public override string ToString()
        {
            return String.Format("type=\"{0}\" body=\"{1}\" file=\"{2}\" onError=\"{3}\" text=\"{4}\"", CmdType, CommandString, Filename, OnError, Description);
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
        [XmlAttribute("type")]
        public String CmdType;

        [XmlAttribute("body")]
        public String CommandString;

        [XmlAttribute("file")]
        public String Filename;

        [XmlAttribute("onError")]
        public String OnError;

        [XmlText()]
        public String Description;
    }

    public class Configuration
    {
        [XmlElement("DEV")]
        public DeviceDesc[] DeviceDescs;

        [XmlElement("VER")]
        public FirmwareVersion FirmwareVersion;
    }

    public enum DeviceMode { Unknown, Recovery, Updater, UserMtp, UserMsc, User, Disconnected };

    public class DeviceDesc
    {
        [XmlAttribute("mode")]
        public DeviceMode Mode
        {
            get { return _Mode; }
            set { _Mode = value; }
        }
        private DeviceMode _Mode;

        [XmlAttribute("vid")]
        public String XmlVid;

        [XmlIgnore()]
        public UInt16? Vid
        {
            get { if (XmlVid == null) return null; else return UInt16.Parse(XmlVid, System.Globalization.NumberStyles.HexNumber | System.Globalization.NumberStyles.AllowHexSpecifier); }
            set { XmlVid = value == null ? null : value.ToString(); }
        }

        [XmlAttribute("pid")]
        public String XmlPid;

        [XmlIgnore()]
        public UInt16? Pid
        {
            get { if (XmlPid == null) return null; else return UInt16.Parse(XmlPid, System.Globalization.NumberStyles.HexNumber | System.Globalization.NumberStyles.AllowHexSpecifier); }
            set { XmlPid = value == null ? null : value.ToString(); }
        }

        [XmlAttribute("body")]
        public String CommandString
        {
            get { return _CommandString; }
            set { _CommandString = value; }
        }
        private String _CommandString;


        public override string ToString()
        {
            String retString = Mode.ToString();
            retString += Vid == null ? " - xxxx/" : String.Format(" - {0:X4}/", Vid);
            retString += Pid == null ? "xxxx" : String.Format("{0:X4}", Pid);

            return retString;
        }
    }

    public class FirmwareVersion
    {
        [XmlAttribute("file")]
        public String Filename
        {
            get { return _Filename; }
            set { _Filename = value; }
        }
        private String _Filename;

        [XmlAttribute("label")]
        public String Label
        {
            get { return _Label; }
            set { _Label = value; }
        }
        private String _Label;
    }

    [XmlRoot("DEVICE")]
    public class DeviceInfo
    {
        [XmlElement(ElementName = "DCE")]
        public String DCE_Version;

        [XmlElement(ElementName = "FW")]
        public String FW_Version;

        [XmlElement(ElementName = "SN")]
        public String SerialNumber;

        [XmlElement(ElementName = "VID")]
        public String Vid;

        [XmlElement(ElementName = "PID")]
        public String Pid;

        [XmlElement(ElementName = "CID")]
        public String Cid;
    }
}
