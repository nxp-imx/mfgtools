#pragma once

//#if defined(__cplusplus)
//    extern "C" {
//#endif
//#include "wdu_lib.h"
//#include "usb_diag_lib.h"
//#ifdef __cplusplus
//}
//#endif

typedef struct Response
{
	unsigned short ack;		// ack
	unsigned short csum;	// data checksum
	unsigned long len;		// data len
} response_t;

/*
class AtkHostApiClass
{
public:
	__declspec(dllexport) AtkHostApiClass(void);
	__declspec(dllexport) ~AtkHostApiClass(void);

	// To init the Serial Port, Config = 0 (MX31 TO1); 1, MX31 TO2/MX27 TO1
	__declspec(dllexport) BOOL InitComPort(char* port_num, int Config,HANDLE &hComPort, int mxType);
	__declspec(dllexport) BOOL CloseComPort(void);
	// Write the register of i.MX
	__declspec(dllexport) BOOL WriteMemory(int mode,UINT Address, UINT Data, UINT Format);
	// Read the response for Flash Status
	__declspec(dllexport) UINT FlashingStatus(void);
	// Issue after download complete and jump to it
	__declspec(dllexport) BOOL Jump2Rak(int mode, bool is_hab_prod);
	// download the CSF to the memory
	__declspec(dllexport) BOOL DownloadCSF(UINT address, int ByteCount, const unsigned char* pBuf);
	// Download the HWC to the Memory
	__declspec(dllexport) BOOL DownloadDCD(UINT address, int ByteCount, const unsigned char* pBuf);
	// Download the binary image file to the DownloadAddress
	__declspec(dllexport) BOOL DownloadImage(UINT address, UINT byteCount, const unsigned char* pBuf);
	// Check the present of the USB device connected
	__declspec(dllexport) BOOL OpenUSB(int imxType);
	// close the USB device
	__declspec(dllexport) void CloseUSB(void);
	// Read the data from i.MX
	__declspec(dllexport) BOOL ReadDataByRok(UINT ReadAddress, PUINT pReadData, UINT ByteCount, UINT Format);
	// This is used to read the HAB status return from the ROM when it enter Bootstrap
	__declspec(dllexport) int GetHABStatus(void);
	// This is used to send message to UI
	__declspec(dllexport) void SetWinHandle(HWND hWnd);
	// This is used to set up the channal mode
	__declspec(dllexport) void SetUpChannal(int channal,int usbId);
	// API to reset the board
	__declspec(dllexport) int CommonReset(void);
	// API to download image
	__declspec(dllexport) int CommonDownload(unsigned long addr, unsigned long size, const unsigned char *pBuf);
	// API to execute at any address
	__declspec(dllexport) int CommonExecute(unsigned long addr);
	// API To init the Flash device
	__declspec(dllexport) int InitFlash(void);
	// API to erase a select range of flash
	__declspec(dllexport) int EraseFlash(unsigned long addr, unsigned long size);
	// API to dump a select area of flash data
	__declspec(dllexport) int ReadFlash(unsigned long addr, unsigned char *buffer, unsigned long count, unsigned long done);
	// API to program flash
	__declspec(dllexport) int ProgramFlash(unsigned long addr, const unsigned char *buffer, unsigned long count, unsigned long done, int mode,UCHAR format =0);
	// API to read fuse word
	__declspec(dllexport) int ReadFuse(unsigned long addr, unsigned char *pval);
	// API to sense fuse word
	__declspec(dllexport) int SenseFuse(unsigned long addr, unsigned char *pval, unsigned char bit);
	// API to sense override word
	__declspec(dllexport) int OverrideFuse(unsigned long addr, unsigned char val);
	// API to program fuse element
	__declspec(dllexport) int ProgramFuse(unsigned long addr, unsigned char val);
	// API to detect the device status, running bootstrap or RAM Kernel
	__declspec(dllexport) int GetRKLVersion(unsigned char *fmodel, int *len, int *mxType);
	// API to set the event handle
	__declspec(dllexport) void SetEvtHandle(HANDLE hEvent);
	// API to get HAB Type
	__declspec(dllexport) bool GetHABType(int mode);
	// API to switch uart to usb
	__declspec(dllexport) bool DoCom2Usb(void);
	// API to set imx Type
	__declspec(dllexport) void SetiMXType(int mxType);
	// API to set bi swap flag
	__declspec(dllexport) int SetBISwapFlag(int flag);
	// API to set BBT flag
	__declspec(dllexport) int SetBBTFlag(int flag);
	// API To Get the Flash Size
	__declspec(dllexport) int GetFlashCapacity(unsigned long *size);
	// API to set interleave flag
	__declspec(dllexport) int SetINTLVFlag(int flag);
	// API to set LBA flag
	__declspec(dllexport) int SetLBAFlag(int flag);
	// API to set USB timeout
	__declspec(dllexport) void SetUsbTimeout(int openTimeOut, int TransTimeOut);
	
	BOOL GetDeviceDescriptor(USB_DEVICE_DESCRIPTOR *pDescriptor) {return USB_GetDeviceDescriptor(pDescriptor);};
private:
	// Send the command to rom bootstrap
	BOOL SendCommand2RoK(UINT address, UINT byteCount, UCHAR type);
	// Send the command to ram kenerl
	BOOL SendCommand2RaK(UINT address, USHORT cmd, UINT size);
	// trans data 
	BOOL TransData(UINT byteCount, const unsigned char * pBuf,int opMode);
	// encapsure the UART/USB transmit
	BOOL WriteToDevice(const unsigned char *buf, UINT count);
	// encapsure the UART/USB receive
	BOOL ReadFromDevice(PUCHAR buf, UINT count);
	// set UART timeout
	void SetUartTimeout(int timeout);


	//////////////////////////////////////////////////////////////////////////
	//Eric
	// private operation to make checksum for a range of buffer data
	unsigned short MakeChecksum(const unsigned char* buffer, unsigned long count);
	// pack command
	void PackCommand(unsigned char *cmd, unsigned short cmdId, unsigned long addr, unsigned long param, unsigned long param1);
	// unpack the response
	struct Response UnPackResponse(unsigned char *resBuf);
	//////////////////////////////////////////////////////////////////////////
private:

	HWND m_hWnd;
	int m_channelMode;	// 0 UART otherwise USB
	int m_usbId;
	HANDLE m_hEvent;
	int m_uartTimeout;
	int m_usbOpenTimeOut;
	int m_usbTransTimeOut;
	int m_iMxType;
*/
	
#define DEFAULT_USB_ID	0x1
#define USB_ID_2		0x2
#define USB_ID_3		0x3
#define USB_ID_4		0x4
#define USB_ID_5		0x5
#define USB_ID_6		0x6
#define USB_ID_7		0x7
#define USB_ID_8		0x8
#define USB_ID_9		0x9

#define ROM_KERNEL_CMD_COMPLETE 0x0
#define ROM_KERNEL_CMD_SIZE 0x10
//#define ROM_KERNEL_WF_FT_CSF 0xCC
//#define ROM_KERNEL_WF_FT_DCD 0xEE
//#define ROM_KERNEL_WF_FT_APP 0xAA
//#define ROM_KERNEL_WF_FT_OTH 0x0
#define MAX_SIZE_PER_FLASH_COMMAND 0x200000
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
#define RAM_KERNEL_CMD_FUSE_READ			0x0101  // #define RAM_KERNEL_CMD_FUSE_READ			0x0101
#define RAM_KERNEL_CMD_FUSE_SENSE			0x0102	// #define RAM_KERNEL_CMD_FUSE_OVRD			0x0102
#define RAM_KERNEL_CMD_FUSE_OVERRIDE		0x0103	// #define RAM_KERNEL_CMD_FUSE_PROG			0x0103
#define RAM_KERNEL_CMD_FUSE_PROGRAM			0x0104	// #define RAM_KERNEL_CMD_FUSE_SENS			0X0104
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

#define WM_USER_PROGRESS (WM_USER + 1)
#define PROGRESS_DWNLOAD 0
#define PROGRESS_FLASH_PROG 1
#define PROGRESS_FLASH_DUMP 2
#define PROGRESS_FLASH_ERASE 3
#define OPMODE_DOWNLOAD 0
#define OPMODE_FLASH_PROG 1
#define OPMODE_FLASH_DUMP 2
#define OPMODE_FLASH_ERASE 3
#define OPMODE_FLASH_VERIFY 4
#define OPMODE_FLASH_CHECKSUM 5
#define OPMODE_FLASH_DOWNLOAD 6
#define FLASH_PROG_TIMEOUT 50
#define DEFAULT_UART_TIMEOUT 3 //uart timeout count
#define DEFAULT_USB_OPEN_TIMEOUT 30 //in seconds
#define DEFAULT_USB_TRANS_TIMEOUT 30000 // in mseconds

typedef enum 
{
	FLASH_PRG_BOUNDARY,
	FLASH_PRG_UNBOUNDARY,
} FLASH_PRG_MODE;

typedef enum
{
	BI_SWAP_DISABLE,
	BI_SWAP_ENABLE
};

typedef enum
{
	FL_BBT_DISABLE,
	FL_BBT_ENABLE
}FL_BBT_FLAG;

typedef enum
{
	FL_INTLV_DISABLE,
	FL_INTLV_ENABLE
}FL_INTLV_FLAG;

typedef enum
{
	FL_RDBKCH_DISABLE,
	FL_RDBKCH_ENABLE
}FL_RDBKCH_FLAG;

typedef enum
{
	FL_LBA_DISABLE,
	FL_LBA_ENABLE
}FL_LBA_FLAG;


/// From AtkHostApiClass.cpp

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
