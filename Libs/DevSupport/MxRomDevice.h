#pragma once

#include "Device.h"
//#include "Observer.h"
//#include "StFwComponent.h"

#include "Common/StdString.h"
#include "Common/StdInt.h"

//#include "iMX/MXDefine.h"
//#include "iMX/MemoryInit.h"

//#include "Apps/MfgTool.exe/UpdateCommandList.h" // UGLY!!!!

#include "iMX/ADSTkConfigure.h"

//#include "Common/WinDriver/wdu_lib.h"

/// <summary>
/// A MxRomDevice device.
/// </summary>

class MxRomDevice : public Device//, IComparable
{
	friend class DeviceManager;
public:
	MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	virtual ~MxRomDevice(void);
	int GetRKLVersion(unsigned char *fmodel, int *len, int *mxType);
//	BOOL InitMemoryDevice(MxRomDevice::MemoryInitScript script);
	BOOL DownloadRKL(const StFwComponent& fwComponent, Device::UI_Callback callbackFn);

	void SetIMXDevPara(void);
	BOOL DownloadRKL(unsigned char *rkl, int rklsize, unsigned int RAMKNLAddr, bool bPreload);

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
	BOOL DownloadImage(UINT address, const StFwComponent& fwComponent, Device::UI_Callback callbackFn);
	BOOL SendCommand2RoK(UINT address, UINT byteCount, UCHAR type);
	BOOL TransData(UINT byteCount, const unsigned char * pBuf,int opMode);
	BOOL WriteToDevice(const unsigned char *buf, UINT count);
	BOOL ReadFromDevice(PUCHAR buf, UINT count);
	BOOL DeviceIoControl(DWORD controlCode, PVOID pRequest = NULL);
	BOOL OpenUSBHandle(HANDLE *pHandle, CString pipePath);
	BOOL USB_OpenDevice();
    BOOL USB_CloseDevice();

	CADSTkConfigure atkConfigure;

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
	
	enum MemoryType { MemUnknown, DDR, SDRAM, DDR2, mDDR };
	MemoryType StringToMemoryType(CString strMemType)
	{
		MemoryType memType = MemUnknown;

		if ( strMemType.CompareNoCase(_T("DDR")) == 0 )
			memType = DDR;
		else if ( strMemType.CompareNoCase(_T("SDRAM")) == 0 )
			memType = SDRAM;
		else if ( strMemType.CompareNoCase(_T("DDR2")) == 0 )
			memType = DDR2;
		else if ( strMemType.CompareNoCase(_T("mDDR")) == 0 )
			memType = mDDR;

		return memType;
	}

public:
	typedef struct MemoryInitCommand
	{
		UINT Address;
		UINT Data;
		UINT Format;

		MemoryInitCommand(UINT addr, UINT data, UINT format) : Address(addr), Data(data), Format(format) {};
	};

	typedef std::vector<MemoryInitCommand> MemoryInitScript;
	MemoryInitScript* GetMemoryScript(ChipFamily_t chipType, RomVersion romVersion, MemoryType memoryType) const;

	static void InializeMemoryScripts();

	BOOL InitMemoryDevice(MxRomDevice::MemoryInitScript script);
	static MemoryInitScript ddrMx31;
	static MemoryInitScript ddrMx27;
	static MemoryInitScript sdrMx31;
	static MemoryInitScript ddrMx35;
	static MemoryInitScript mddrMx35;
	static MemoryInitScript ddr2Mx35;
	static MemoryInitScript ddrMx37;
	static MemoryInitScript ddrMx51;
	static MemoryInitScript ddrMx51_To2;
	static MemoryInitScript mddrMx51_To2;
	static MemoryInitScript ddrMx25;
	static MemoryInitScript mddrMx25;
	static MemoryInitScript ddr2Mx25_To11;

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
