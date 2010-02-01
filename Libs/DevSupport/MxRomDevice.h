#pragma once

#include "Device.h"
//#include "Observer.h"
//#include "StFwComponent.h"

#include "Common/StdString.h"
#include "Common/StdInt.h"

#include "iMX/MXDefine.h"
#include "iMX/ADSTkConfigure.h"

//#include "Common/WinDriver/wdu_lib.h"

/// <summary>
/// A MxRomDevice device.
/// </summary>

class MxRomDevice : public Device//, IComparable
{

public:
	MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
	
	virtual ~MxRomDevice(void) {};
//    uint32_t Download(const StFwComponent& fwComponent, Device::UI_Callback callbackFn); 
	void SetIMXDevPara(CString cMXType, CString cSecurity, CString cRAMType, unsigned int RAMKNLAddr);
	BOOL DownloadRKL(unsigned char *rkl, int rklsize);

	// PROPERTIES
	class MaxPacketSize : public Int32Property { public: int32_t get(); } _MaxPacketSize;

private:
	BOOL InitMemoryDevice();
	BOOL WriteMemory(int mode, UINT address, UINT Data, UINT Format);
	BOOL GetHABType(int mode);
	BOOL Jump2Rak(int mode, BOOL is_hab_prod);
	BOOL DownloadImage(UINT byteCount, const unsigned char* pBuf);
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
