#pragma once

#include "Device.h"
//#include "Observer.h"
//#include "StFwComponent.h"

#include "Common/StdString.h"
#include "Common/StdInt.h"

#include "../../Libs/WinSupport/XMLite.h"

// Code from AtkHostApiClass.h
// Could be moved to SDP protocol or MxRomApi or something like that.
#define ROM_KERNEL_CMD_COMPLETE 0x0
#define ROM_KERNEL_CMD_SIZE 0x10
//#define ROM_KERNEL_WF_FT_CSF 0xCC
//#define ROM_KERNEL_WF_FT_DCD 0xEE
//#define ROM_KERNEL_WF_FT_APP 0xAA
//#define ROM_KERNEL_WF_FT_OTH 0x0
#define MAX_SIZE_PER_FLASH_COMMAND 0x400000
#define MAX_SIZE_PER_DOWNLOAD_COMMAND 0x200000
#define ROM_KERNEL_CMD_RD_MEM 0x0101
#define ROM_KERNEL_CMD_WR_MEM 0x0202
#define ROM_KERNEL_CMD_WR_FILE 0x0404
#define ROM_KERNEL_CMD_GET_STAT 0x0505
#define RAM_KERNEL_CMD_HEADER 0x0606
#define ROM_KERNEL_CMD_RE_ENUM 0x0909

#define ROM_WRITE_ACK   0x128A8A12
#define ROM_STATUS_ACK  0x88888888
#define ROM_STATUS_ACK2 0xF0F0F0F0

#define RAM_KERNEL_ACK_SIZE 0x08
#define RAM_KERNEL_CMD_SIZE 0x10
#define WR_BUF_MAX_SIZE 0x4000

#define RAM_KERNEL_CMD_FLASH_INIT			0x0001
#define RAM_KERNEL_CMD_FLASH_ERASE			0x0002
#define RAM_KERNEL_CMD_FLASH_DUMP			0x0003
#define RAM_KERNEL_CMD_FLASH_PROGR_B		0x0004
#define RAM_KERNEL_CMD_FLASH_PROGR_UB		0x0005
#define RAM_KERNEL_CMD_FLASH_GET_CAPACITY	0x0006
#define RAM_KERNEL_CMD_FUSE_READ			0x0101
#define RAM_KERNEL_CMD_FUSE_SENSE			0x0102
#define RAM_KERNEL_CMD_FUSE_OVERRIDE		0x0103
#define RAM_KERNEL_CMD_FUSE_PROGRAM			0x0104
#define RAM_KERNEL_CMD_RESET				0x0201
#define RAM_KERNEL_CMD_DOWNLOAD				0x0202
#define RAM_KERNEL_CMD_EXECUTE				0x0203
#define RAM_KERNEL_CMD_GETVER				0x0204
#define RAM_KERNEL_CMD_COM2USB				0x0301
#define RAM_KERNEL_CMD_SWAP_BI				0x0302
#define RAM_KERNEL_CMD_FL_BBT				0x0303
#define RAM_KERNEL_CMD_FL_INTLV				0x0304
#define RAM_KERNEL_CMD_FL_LBA				0x0305

#define ERROR_COMMAND		0xffff
#define RET_SUCCESS		0
#define FLASH_PARTLY		1	/* response each dump/program size */
#define FUSE_PARTLY		1
#define FLASH_ERASE		2	/* response each erase size */
#define FLASH_VERIFY		3	/* response each verified bytes count */

/* flash program flags define */
#define FLASH_PROGRAM_NB0_FORMAT 0x00000001
#define FLASH_PROGRAM_GO_ON      0x00000100
#define FLASH_PROGRAM_READ_BACK  0x00010000
/* flash failed define */
#define FLASH_FAILED		-4
#define FLASH_ECC_FAILED	-5

#define FLASH_ERROR_NO		0
#define FLASH_ERROR_READ   	-100
#define FLASH_ERROR_ECC    	-101
#define FLASH_ERROR_PROG   	-102
#define FLASH_ERROR_ERASE  	-103
#define FLASH_ERROR_VERIFY 	-104
#define FLASH_ERROR_INIT   	-105
#define FLASH_ERROR_OVER_ADDR	-106
#define FLASH_ERROR_PART_ERASE	-107
#define FLASH_ERROR_EOF 		-108

/* fuse failed define */
#define FUSE_FAILED		-4
#define FUSE_READ_PROTECT	-5
#define FUSE_SENSE_PROTECT	-6
#define FUSE_OVERRIDE_PROTECT	-7
#define FUSE_WRITE_PROTECT	-8
#define FUSE_VERIFY_FAILED	-9

/* ram kernel error define */
#define INVALID_CHANNEL		-256
#define INVALID_CHECKSUM	-257
#define INVALID_PARAM		-258

#define MAX_MODEL_LEN	128

// Address ranges for Production parts: 

#define MX25_SDRAM_START       0x80000000 //Senna
#define MX25_WEIM_CS2_END      0xB1FFFFFF  
#define MX25_NFC_START         0xBB000000   
#define MX25_NFC_END           0xBB000FFF        
#define MX25_SDRAM_CTL_START   0xB8001000 
#define MX25_SDRAM_CTL_END     0xB8001FFF
#define MX25_WEIM_START        0xA0000000 
#define MX25_WEIM_END          0xA7FFFFFF
#define MX25_IRAM_FREE_START   0x78000000
#define MX25_IRAM_FREE_END     0x7801FFFF
#define MX27_MemoryStart        0xA0000000 // Bono
#define MX27_MemoryEnd          0xAFFFFFFF // Bono
#define MX27_NFCstart           0xD8000000 // Bono
#define MX27_NFCend             0xD8000FFF // Bono
#define MX27_WEIMstart          0xD8002000 // Bono
#define MX27_WEIMend            0xD8002fff // Bono
#define MX27_CCMstart           0x10027000 // Bono
#define MX27_CCMend             0x10027fff // Bono
#define MX27_ESDCTLstart        0xD8001000 // Bono
#define MX27_ESDCTLend          0xD8001FFF // Bono
#define MX27_CCM_CGR0           0x53F80020 // Bono

// 0xB800_0000 0xB800_0FFF (ARM) NANDFC
#define MX31_NFCstart	0xB8000000  // Tortola 
#define MX31_NFCend		0xB8000FFF  // Tortola
// 0xB800_20000 xB800_2FFF (ARM) WEIM 
#define MX31_WEIMstart	0xB8002000  // Tortola
#define MX31_WEIMend	0xB8002060  // Tortola
// 0x8000_0000 0x8FFF_FFFF CSD0 SDRAM 
#define MX31_MemoryStart 0x80000000 // Tortola
//0xB600_0000 0xB7FF_FFFF WEIM CS5 
#define MX31_MemoryEnd	 0xB7FFFFFF // Tortola
//0x53F8_0000 (CCMR) Control Register 
#define MX31_CCMstart	0x53f80000  // Tortola
// 0x53F8_ 0064 (PDR2) Post Divider Register 2 
#define MX31_CCMend		0x53f80064  // Tortola 

// ESDCTL/MDDRC registers space (4K) READ/WRITE
#define MX31_ESDCTLstart 0xB8001000 // Tortola
#define MX31_ESDCTLend	 0xB8001FFF // Tortola
// 0x53F8_ 0020 (CGR0)Clock Gating Register 
#define MX31_CCM_CGR0	0x53F80020  // Tortola

#define MX35_SDRAM_START        0x80000000 //Ringo
#define MX35_WEIM_CS2_END       0xB1FFFFFF
#define MX35_NFC_START          0xBB000000
#define MX35_NFC_END            0xBB001FFF
#define MX35_SDRAM_CTL_START    0xB8001000 
#define MX35_SDRAM_CTL_END      0xB8001fff
#define MX35_WEIM_START         0xB8002000
#define MX35_WEIM_END           0xB8002fff
#define MX35_IRAM_FREE_START    0x10001B00
#define MX35_IRAM_FREE_END      0x1000FFFC

#define MX37_SDRAM_START       0x40000000 //marley
#define MX37_WEIM_CS2_END      0x71ffffff  
#define MX37_NFC_START         0x7fff0000   
#define MX37_NFC_END           0x7fffffff        
#define MX37_SDRAM_CTL_START   0xe3fd9000 
#define MX37_SDRAM_CTL_END     0xe3fd9fff 
#define MX37_WEIM_START        0xe3fda000 
#define MX37_WEIM_END          0xe3fdafff
#define MX37_IRAM_FREE_START   0x10002000
#define MX37_IRAM_FREE_END     0x1000fffc

#define MX51_SDRAM_START       0x90000000 //elvis
#define MX51_WEIM_CS2_END      0xc7ffffff  
#define MX51_NFC_START         0xcfff0000   
#define MX51_NFC_END           0xcfffffff        
#define MX51_SDRAM_CTL_START   0x83fd9000 
#define MX51_SDRAM_CTL_END     0x83fd9fff 
#define MX51_WEIM_START        0x83fda000 
#define MX51_WEIM_END          0x83fdafff
#define MX51_IRAM_FREE_START   0x1ffe8000
#define MX51_IRAM_FREE_END     0x1fffffff

#define MX53_SDRAM_START       0x70000000
#define MX53_SDRAM_END         0xEFFFFFFF
#define MX53_NFC_START         0xf7ff0000   
#define MX53_NFC_END           0xf7ffffff        
#define MX53_SDRAM_CTL_START   0x63fd9000 
#define MX53_SDRAM_CTL_END     0x63fd9fff 
#define MX53_WEIM_START        0x63fda000 
#define MX53_WEIM_END          0x63fdafff
#define MX53_IRAM_FREE_START   0xf8000000
#define MX53_IRAM_FREE_END     0xf8020000

#define IVT_BARKER_HEADER      0x402000D1
#define ROM_TRANSFER_SIZE	   0x400

/// <summary>
/// A MxRomDevice device.
/// </summary>
class MxRomDevice : public Device//, IComparable
{
	friend class DeviceManager;

public:

	enum MemorySection { MemSectionOTH = 0x00, MemSectionAPP = 0xAA, MemSectionCSF = 0xCC, MemSectionDCD = 0xEE };

	struct Response
	{
		unsigned int romResponse;
		struct
		{
			unsigned short ack;		// ack
			unsigned short csum;	// data checksum
			unsigned long len;		// data len
		};
	};

	typedef struct _IvtHeader
	{
           unsigned long IvtBarker;
           unsigned long ImageStartAddr;// LONG(0x70004020)
           unsigned long Reserved[3];
           unsigned long SelfAddr;// LONG(0x70004000)
           unsigned long Reserved2[2];
	}IvtHeader, *PIvtHeader;

	typedef struct _FlashHeader
	{
           unsigned long ImageStartAddr;
           unsigned long Reserved[4];
	}FlashHeader, *PFlashHeader;

	typedef struct _ImageParameter
	{
        UINT PhyRAMAddr4KRL;//The physical address in RAM where an image locates. 
        MemorySection loadSection;
		MemorySection setSection;
		BOOL HasFlashHeader;//Does an image have a flash header or ivt header.
		UINT CodeOffset;//The offset in an image where the first byte of code locates, the parameter is used to 
						//skip flash header or ivt header embedded in an image.
	}ImageParameter, *PImageParameter;

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
	struct Response SendRklCommand(unsigned short cmdId, unsigned long addr, unsigned long param1, unsigned long param2);
	int GetRKLVersion(CString& fmodel, int& len, int& mxType);
	BOOL InitMemoryDevice(CString filename);
	BOOL ProgramFlash(std::ifstream& file, UINT address, UINT cmdID, UINT flags, Device::UI_Callback callback);
	BOOL DownloadImage(PImageParameter pImageParameter, const StFwComponent& fwComponent, Device::UI_Callback callbackFn);
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
        MX51,
		MX53
    };

	ChipFamily_t GetChipFamily();

	BOOL WriteMemory(UINT address, UINT data, UINT format);
	BOOL ValidAddress(const UINT address, const UINT format) const;
	//HAB_t GetHABType(ChipFamily_t chipType);
	void PackRklCommand(unsigned char *cmd, unsigned short cmdId, unsigned long addr, unsigned long param1, unsigned long param2);
	CString CommandToString(unsigned char *cmd, int size);
	struct Response UnPackRklResponse(unsigned char *resBuf);
	BOOL Jump2Rak();
	BOOL SendCommand2RoK(UINT address, UINT byteCount, UCHAR type);
	BOOL TransData(UINT byteCount, const unsigned char * pBuf);
	BOOL WriteToDevice(const unsigned char *buf, UINT count);
	BOOL ReadFromDevice(PUCHAR buf, UINT count);
	BOOL DeviceIoControl(DWORD controlCode, PVOID pRequest = NULL);
	BOOL OpenUSBHandle(HANDLE *pHandle, CString pipePath);

	HANDLE _hDevice;
	HANDLE _hWrite;
	HANDLE _hRead;
	ChipFamily_t _chipFamily;
	HAB_t _habType;
    BOOL _SyncAllDevEnable;

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
	static MxAddress MX53Addrs;

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
