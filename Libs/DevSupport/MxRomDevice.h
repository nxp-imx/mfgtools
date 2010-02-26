#pragma once

#include "Device.h"
//#include "Observer.h"
//#include "StFwComponent.h"

#include "Common/StdString.h"
#include "Common/StdInt.h"

#include "../../Libs/WinSupport/XMLite.h"

/// <summary>
/// A MxRomDevice device.
/// </summary>

class MxRomDevice : public Device//, IComparable
{
	friend class DeviceManager;
public:
	
	enum MemorySection { MemSectionOTH = 0x00, MemSectionAPP = 0xAA, MemSectionCSF = 0xCC, MemSectionDCD = 0xEE };
	static MemorySection StringToMemorySection(CString section)
	{
		if ( section.CompareNoCase(_T("DCD")) == 0 )
			return MemSectionDCD;
		else if ( section.CompareNoCase(_T("CSF")) == 0 )
			return MemSectionCSF;
		else if ( section.CompareNoCase(_T("APP")) == 0 )
			return MemSectionAPP;
		else return MemSectionOTH;
	}

	enum MemoryAction { MemAction_None, MemAction_Set, MemAction_Jump };
	static MemoryAction StringToMemoryAction(CString action)
	{
		if ( action.CompareNoCase(_T("Set")) == 0 )
			return MemAction_Set;
		else if ( action.CompareNoCase(_T("Jump")) == 0 )
			return MemAction_Jump;
		else return MemAction_None;
	}

	MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~MxRomDevice(void);
	int GetRKLVersion(CString& fmodel, int& len, int& mxType);
	BOOL InitMemoryDevice(CString filename);
	BOOL DownloadImage(UINT address, MemorySection loadSection, MemorySection setSection, const StFwComponent& fwComponent, Device::UI_Callback callbackFn);
	BOOL Jump();
	BOOL Reset();

	// PROPERTIES
	class MaxPacketSize : public Int32Property { public: int32_t get(); } _MaxPacketSize;
	typedef struct _MxRomParamt
	{
		CString cMXType;
		CString cSecurity; 
		CString cRAMType;
		CString cMemInitFilePath;
	}MxRomParamt, * PMxRomParamt;
	MxRomParamt m_MxRomParamt;

private:
	enum HAB_t
	{
		HabUnknown  = -1,
		HabEnabled  = 0x12343412,
		HabDisabled = 0x56787856
	};

    enum ChipFamily_t
    {
        ChipUnknown = 0,
        MX25,
        MX27,
        MX31,
        MX32,
        MX35,
        MX37,
        MX51
    };

	ChipFamily_t GetChipFamily();

	BOOL WriteMemory(UINT address, UINT data, UINT format);
	BOOL ValidAddress(const UINT address) const;
	HAB_t GetHABType(ChipFamily_t chipType);
	void PackRklCommand(unsigned char *cmd, unsigned short cmdId, unsigned long addr, unsigned long param, unsigned long param1);
	struct Response UnPackRklResponse(unsigned char *resBuf);
	BOOL Jump2Rak();
	BOOL SendCommand2RoK(UINT address, UINT byteCount, UCHAR type);
	BOOL TransData(UINT byteCount, const unsigned char * pBuf,int opMode);
	BOOL WriteToDevice(const unsigned char *buf, UINT count);
	BOOL ReadFromDevice(PUCHAR buf, UINT count);
	BOOL DeviceIoControl(DWORD controlCode, PVOID pRequest = NULL);
	BOOL OpenUSBHandle(HANDLE *pHandle, CString pipePath);
	BOOL USB_OpenDevice();
    BOOL USB_CloseDevice();

	HANDLE _hDevice;
	HANDLE _hWrite;
	HANDLE _hRead;
	ChipFamily_t _chipFamily;
	HAB_t _habType;

	enum ChannelType { ChannelType_UART = 0, ChannelType_USB };
	typedef struct RomVersion
	{
		UINT Major;
		UINT Minor;
		
		RomVersion(UINT major, UINT minor) : Major(major), Minor(minor) {};
		BOOL operator==(const RomVersion& rhs) const { return Major == rhs.Major && Minor == rhs.Minor; };
	};
	RomVersion _romVersion;
	
public:
	struct MxAddress
	{
		UINT MemoryStart;
		UINT MemoryEnd;
		UINT DefaultRKL;
		UINT DefaultHWC;
		UINT DefaultCSF;
		UINT DefaultDownload;
		UINT DefaultNorFlash;
		UINT InternalRamStart;
		UINT InternalRamEnd;
		UINT ImageStart;

		MxAddress(UINT memStart, UINT memEnd, UINT rkl, UINT hwc, UINT csf, UINT download, UINT nor, UINT ramStart, UINT ramEnd, UINT img)
			: MemoryStart(memStart), MemoryEnd(memEnd), DefaultRKL(rkl), DefaultHWC(hwc), DefaultCSF(csf), DefaultDownload(download)
			, DefaultNorFlash(nor), InternalRamStart(ramStart), InternalRamEnd(ramEnd), ImageStart(img)
		{}
		
		MxAddress()
			: MemoryStart(0), MemoryEnd(0), DefaultRKL(0), DefaultHWC(0), DefaultCSF(0), DefaultDownload(0)
			, DefaultNorFlash(0), InternalRamStart(0), InternalRamEnd(0), ImageStart(0)
		{}
	};
	
	MxAddress _defaultAddress;
	BOOL InitRomVersion(ChipFamily_t chipType, RomVersion& romVersion, MxAddress& defaultAddrs) const;

	static MxAddress MX25Addrs;
	static MxAddress MX27Addrs;
	static MxAddress MX31Addrs;
	static MxAddress MX35Addrs;
	static MxAddress MX37Addrs;
	static MxAddress MX51Addrs;
	static MxAddress MX51_TO2Addrs;

	class MemoryInitCommand : public XNode
	{
	public:
		// [XmlAttribute("addr")]
		unsigned int GetAddress()
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

		// [XmlAttribute("data")]
		unsigned int GetData()
		{ 
			unsigned int data = 0; // default to 0

			CString attr = GetAttrValue(_T("data"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				data = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				data = _tstoi64(attr);
			}

			return data; 
		};

		// [XmlAttribute("format")]
		unsigned int GetFormat()
		{ 
			unsigned char format = 0; // default to 0

			CString attr = GetAttrValue(_T("format"));

			if(attr.Left(2) == _T("0x"))
			{
				TCHAR *p;
				format = _tcstoul(attr.Mid(2),&p,16);
			}
			else
			{
				format = _tstoi64(attr);
			}

			return format; 
		};

		CString ToString()
		{
			CString str;
			str.Format(_T("addr=\"0x%X\" data=\"0x%X\" format=\"%d\""), GetAddress(), GetData(), GetFormat());
			return str;
		}
	};
};
//private:
//	static const uint32_t PipeSize = 4096;      //TODO:??? where did this come from?
//	static const uint32_t DeviceTimeout = 1000; // 1 second
//    HANDLE m_RecoveryHandle;
//    OVERLAPPED	_fileOverlapped;
//
//    DWORD Open(void);
//    DWORD Close(void);
//	int32_t ProcessTimeOut(const int32_t timeout);
//	static void CALLBACK IoCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

//	internal const String DefaultLicenseString = "6c3cd57876b3a6e415f93fd697134bd97ee1ec50.Motorola Semiconductors";

//    [DllImport("wdapi1001.dll")]
//    internal static extern UInt32 WDU_Init(
//        out IntPtr phDriver,
//        IntPtr pMatchTables,
//        UInt32 dwNumMatchTables,
//        IntPtr pEventTable,
//        IntPtr sLicense,
//        UInt32 dwOptions);

//    [StructLayout(LayoutKind.Sequential, Pack = 1)]
//    internal struct WDU_MATCH_TABLE
//    {
//        internal UInt16 wVendorId;
//        internal UInt16 wProductId;
//        internal Byte bDeviceClass;
//        internal Byte bDeviceSubClass;
//        internal Byte bInterfaceClass;
//        internal Byte bInterfaceSubClass;
//        internal Byte bInterfaceProtocol;

//        public WDU_MATCH_TABLE(UInt16 vid, UInt16 pid)
//        {
//            wVendorId = vid;
//            wProductId = pid;
//            bDeviceClass = bDeviceSubClass = bInterfaceClass = bInterfaceSubClass = bInterfaceProtocol = 0;
//        }
//    } ;
/*
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Size = 136, Pack = 1)]
    internal class WD_LICENSE
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 128)]
        internal Byte[] cLicense;  // Buffer with license string to put.
                          // If empty string then get current license setting
                          // into dwLicense.
        internal UInt32 dwLicense; // Returns license settings: LICENSE_DEMO, LICENSE_WD 
                          // etc..., or 0 for invalid license.
        internal UInt32 dwLicense2;// Returns additional license settings, if dwLicense 
                          // could not hold all the information.
                          // Then dwLicense will return 0.
        public WD_LICENSE()
        {
            cLicense = Utils.Utils.StringToAsciiBytes(DefaultLicenseString);
        }
    };
*/

//    internal MxRomDevice(/*DeviceClass deviceClass,*/ IntPtr deviceInstance, string path/*, int index*/)
//        : base(/*deviceClass,*/ deviceInstance, path/*, index*/)
//    {
/*
        if (IsUsb)
        {
            IntPtr hDriver = IntPtr.Zero;
            WDU_MATCH_TABLE matchTable = new WDU_MATCH_TABLE(Vid, Pid);

            int nMatchTableBytes = Marshal.SizeOf(matchTable);
            IntPtr ptrMatchTable = Marshal.AllocHGlobal(nMatchTableBytes);
            Marshal.StructureToPtr(matchTable, ptrMatchTable, true);

            int nMatchTableBytes = Marshal.SizeOf(matchTable);
            IntPtr ptrMatchTable = Marshal.AllocHGlobal(nMatchTableBytes);
            Marshal.StructureToPtr(matchTable, ptrMatchTable, true);

            UInt32 dwError = WDU_Init(out hDriver, ptrMatchTable, 1, IntPtr.Zero, IntPtr.Zero, 0);
        }
//            if (dwError != 0)
//            {
//                ERR("main: failed to initialize USB driver: error 0x%x (\"%s\")\n",
//                        dwError, Stat2Str(dwError));
//                goto Exit;
//            }

//            if (GetDeviceHandle(path))
//            {
//                InitializeDevice();
//            }
    }
}
*/
