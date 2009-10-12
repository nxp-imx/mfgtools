#include "StdAfx.h"
#include "AtkHostApiClass.h"
#include "protocol.h"
#include "Platform/MXDefine.h"
#include "ComPort/ComPortInit.h"
#include "usb_diag_lib.h"

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
/*
typedef struct 
{
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize0;

    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char iManufacturer;
    unsigned char iProduct;
    unsigned char iSerialNumber;
    unsigned char bNumConfigurations;
} USB_DEVICE_DESCRIPTOR;

/////////////////////////////////////////////////////////////
extern BOOL USB_OpenLocateDevice(int id, int openTimout);
extern void	USB_CloseLocateDevice(void);
extern BOOL USB_GetDeviceDescriptor(USB_DEVICE_DESCRIPTOR *pDescriptor);
extern BOOL ReadFromUsb(DWORD dwSize, int id, PVOID pBuffer, int timeOut);
extern BOOL WriteToUsb(DWORD dwSize,PUINT pCommand,int id);
extern BOOL init_com_port(char* Com, int Config,HANDLE &hComm, int iBaudRate);
extern BOOL close_com_port(void);
extern BOOL WriteBuffer(PUCHAR lpBuf, DWORD dwToWrite);
extern BOOL ReadBuffer(PUCHAR buf, DWORD dwToRead, int to);
/////////////////////////////////////////////////////////////
*/
AtkHostApiClass::AtkHostApiClass(void)
{
	m_hWnd = NULL;
	m_uartTimeout = DEFAULT_UART_TIMEOUT;
	m_usbOpenTimeOut = DEFAULT_USB_OPEN_TIMEOUT;
	m_usbTransTimeOut = DEFAULT_USB_TRANS_TIMEOUT;
}

AtkHostApiClass::~AtkHostApiClass(void)
{
	
}

void AtkHostApiClass::SetWinHandle(HWND hWnd)
{
	m_hWnd = hWnd;
}
void AtkHostApiClass::SetUpChannal(int channal,int usbId)
{
	m_channelMode = channal;
	m_usbId = usbId;
}

BOOL AtkHostApiClass::InitComPort(PCHAR port_num, int Config,HANDLE &hComPort, int iBaudRate)
{
	BOOL status = FALSE;

	status = init_com_port(port_num, Config,hComPort, iBaudRate);

	return status;
}

BOOL AtkHostApiClass::CloseComPort(void)
{
	BOOL status = FALSE;

	status = close_com_port();

	return status;
}

// Write the register of i.MX
BOOL AtkHostApiClass::WriteMemory(int mode, UINT address, UINT Data, UINT Format)
{

	long command[4];
	int developmentLevel = 0;
	PUINT pWriteAck;
	PUINT pCommandAck;
	const short header = ROM_KERNEL_CMD_WR_MEM;
	BOOL status = FALSE;

	//clear the command array
	for (int i=0; i<4;i++)
	{
		command[i] = 0;
	}

	command[0] = (((address & 0x00FF0000) <<8) 
				| ((address & 0xFF000000) >> 8) 
				|(header & 0xFFFF));

	command[1] = (((0x00 &0xFF000000) )
				|((Format &0xFF)<<16)
				| ((address & 0xFF) << 8)
				|((address & 0xFF00) >> 8 ));

	command[2] = ((Data & 0xFF000000)
				|((0x00 & 0xFF)<<16)
				|(0x00 & 0xFF00)
				|((0x00 & 0x00FF0000)>>16));

	command[3] = (((Data & 0x00FF0000) >> 16) 
				| (Data & 0xFF00)
				| ((Data & 0xFF)<<16));

	//Send write Command to USB
	status = WriteToDevice((unsigned char *)command, 16);

	if (!status)
	{
		return FALSE;
	}
	pCommandAck =(PUINT) malloc (4);

	//read the ACK
	if(!ReadFromDevice((unsigned char *)pCommandAck,4))
	{
		return FALSE;
	}

	TRACE("WriteMemory(): Receive ack:%x\n", pCommandAck[0]);
	if (pCommandAck[0] == 0x56787856)
	{
		free (pCommandAck);
		developmentLevel = 1;
	}
	else 
	{
		free (pCommandAck);
		developmentLevel = 0;

		status = FALSE;

		switch (mode)
		{
		case MX_MX31_TO1:	/* Fall through */
		case MX_MX31_TO2:	/* Fall through */
		case MX_MX31_TO201:	/* Fall through */
		case MX_MX32:
			if ( ((address <= MX31_NFCend)			&& (address >=MX31_NFCstart)) 
			      || ((address <= MX31_WEIMend)		&& (address >=MX31_WEIMstart))
			      || ((address <= MX31_MemoryEnd)	&& (address >=MX31_MemoryStart))
			      || ((address <= MX31_ESDCTLend)	&& (address >= MX31_ESDCTLstart))
			      || ((address <= MX31_CCMend)		&& (address >= MX31_CCMstart) && (address != MX31_CCM_CGR0)) )
			{ 
				status = TRUE;
				TRACE("Matching the MX31 address region\n");
			}
			break;
		case MX_MX27_TO1:	/* Fall through */
		case MX_MX27_TO2:
			if ( ((address <= MX27_NFCend)			&& (address >=MX27_NFCstart)) 
			      || ((address <= MX27_WEIMend)		&& (address >=MX27_WEIMstart))
			      || ((address <= MX27_MemoryEnd)	&& (address >=MX27_MemoryStart))
			      || ((address <= MX27_ESDCTLend)	&& (address >= MX27_ESDCTLstart))
			      || ((address <= MX27_CCMend)		&& (address >= MX27_CCMstart) && (address != MX27_CCM_CGR0)) )
		
			{
				status = TRUE;
				TRACE("Matching the MX27 address region\n");
			}
			break;
		case MX_MX35_TO1:
		case MX_MX35_TO2:
			if ( ((address <= MX35_WEIM_CS2_END)        && (address >= MX35_SDRAM_START))
                  || ((address <= MX35_IRAM_FREE_END)   && (address >= MX35_IRAM_FREE_START))
                  || ((address <= MX35_NFC_END)         && (address >= MX35_NFC_START))
                  || ((address <= MX35_SDRAM_CTL_END)   && (address >= MX35_SDRAM_CTL_START))
                  || ((address <= MX35_WEIM_END)        && (address >= MX35_WEIM_START)))
		
			{
				status = TRUE;
				TRACE("Matching the MX35 address region\n");
			}
			break;
		case MX_MX37:	/* Fall through */
			if ( ((address <= MX37_WEIM_CS2_END)        && (address >= MX37_SDRAM_START))
                  || ((address <= MX37_IRAM_FREE_END)   && (address >= MX37_IRAM_FREE_START))
                  || ((address <= MX37_NFC_END)         && (address >= MX37_NFC_START))
                  || ((address <= MX37_SDRAM_CTL_END)   && (address >= MX37_SDRAM_CTL_START))
                  || ((address <= MX37_WEIM_END)        && (address >= MX37_WEIM_START)))
		
			{
				status = TRUE;
				TRACE("Matching the MX37 address region\n");
			}
			break;
		case MX_MX51_TO1:
        case MX_MX51_TO2:
			 if ( ((address <= MX51_WEIM_CS2_END)        && (address >= MX51_SDRAM_START))
                   || ((address <= MX51_IRAM_FREE_END)   && (address >= MX51_IRAM_FREE_START))
                   || ((address <= MX51_NFC_END)         && (address >= MX51_NFC_START))
                   || ((address <= MX51_SDRAM_CTL_END)   && (address >= MX51_SDRAM_CTL_START))
                   || ((address <= MX51_WEIM_END)        && (address >= MX51_WEIM_START)))
		
			{
				status = TRUE;
				TRACE("Matching the MX51 address region\n");
			}
			break;
		default:
			status = FALSE;
			break;
		}
	}	
	
	if(!status)
		return FALSE;

	pWriteAck = (PUINT) malloc (4);

	if(!ReadFromDevice((PUCHAR)pWriteAck,4))
	{
		return FALSE;
	}

	if (pWriteAck[0] != 0x128A8A12)
	{
		free (pWriteAck);
		return FALSE; 
	}
	else
	{
		free (pWriteAck);
		return TRUE;
	}
}

UINT AtkHostApiClass::FlashingStatus(void)
{
	PUINT pCommandAck;	
	UINT FlashStatusReq = 0x9394;
	UINT command[5];
	const int CommandAckSize = 4;
	const int CommandSize = 16;
	UINT output = 0;
	//BOOL status = FALSE;


	//clear command[] to 0
	for (int i=0;i<4;i++)
	{
		command[i] = 0;
	}
	command[0] = FlashStatusReq;	
	command[1] = 0x12345678;
	command[2] = 0x90ABCDEF;
	command[3] = 0xAA55AA55;

	if(!WriteToDevice((unsigned char *)command,CommandSize))
	{
		return FALSE;
	}
	
	//read the ACK
	pCommandAck = (PUINT) malloc(4);
	memset(pCommandAck,0,4);

	if(!ReadFromDevice((unsigned char *)pCommandAck, CommandAckSize))
	{
		return FALSE;
	}

	output =  pCommandAck[0];

	free (pCommandAck);

	return output;


}

BOOL AtkHostApiClass::Jump2Rak(int mode, bool is_hab_prod)
{
	UINT resp = 0;	
	UINT command[4];
	int CommandSize = 16;
	int CommandAckSize = 4;
	const UINT Response = 0x88888888;
	PUINT pCommandAck;
	//BOOL status = FALSE;

	for (int i = 0; i<4 ;i++)
	{
		command[i] = 0;
	}

	command[0] = (((0 & 0x00FF0000) <<8) 
					| ((0 & 0xFF000000) >> 8) 
					|(0x0505 & 0xFFFF));
	command[1] = (((0 & 0xFF) << 8)
					|((0 & 0xFF00) >> 8 ));
	command[2] = 0;
	command[3] = 0;

	if(!WriteToDevice((PUCHAR)command,CommandSize))
	{
		return FALSE;
	}
	//read the ACK
	pCommandAck = (PUINT) malloc(4);
	if(!ReadFromDevice((PUCHAR)pCommandAck, CommandAckSize))
	{
		return FALSE;
	}
	resp = pCommandAck[0];

	CStringA str;
	str.Format(">> Jump2 Ramkernel result:0x%x\n", resp);
	TRACE(str.GetBuffer(0));

	free (pCommandAck);
	if (resp != Response)
	{
		return FALSE;
	}
	
	if (((mode == MX_MX35_TO1) || (mode == MX_MX35_TO2) || (mode == MX_MX37) || 
		 (mode == MX_MX51_TO1) || (mode == MX_MX51_TO2)) && is_hab_prod)
	{
		SetUartTimeout(1);
		pCommandAck = (PUINT) malloc(4);
		if(!ReadFromDevice((PUCHAR)pCommandAck, CommandAckSize))
		{
			free (pCommandAck);
			SetUartTimeout(DEFAULT_UART_TIMEOUT);
			return TRUE;
		}

		SetUartTimeout(DEFAULT_UART_TIMEOUT);

		resp = pCommandAck[0];

		str.Format(">> Jump2 Ramkernel result:0x%x\n", resp);
		TRACE(str.GetBuffer(0));

		free (pCommandAck);
		if (resp == 0xF0F0F0F0)
		{
			return FALSE;
		}

		return TRUE;
    }

	return TRUE;
}

BOOL AtkHostApiClass::TransData(UINT byteCount, const unsigned char * pBuf,int opMode)
{

//	BOOL status = FALSE;
	const unsigned char * pBuffer = pBuf;
	UINT downloadPerOnce = WR_BUF_MAX_SIZE;

	for (; byteCount >= downloadPerOnce; byteCount -= downloadPerOnce, pBuffer += downloadPerOnce) {
		if (!WriteToDevice(pBuffer, downloadPerOnce))
			return FALSE;

		TRACE("Transfer Size: %d\n", downloadPerOnce);

		if(m_hWnd) {
			SendMessage(m_hWnd,WM_USER_PROGRESS, (WPARAM)opMode, (LPARAM)downloadPerOnce);
		}
	}

	if (m_channelMode == UART) {
		if (!WriteToDevice(pBuffer, byteCount))
			return FALSE;
	} else {
		if (((m_iMxType == MX_MX31_TO1) || (m_iMxType == MX_MX31_TO2) ||
			(m_iMxType == MX_MX31_TO201) || (m_iMxType == MX_MX32)) && byteCount > 0) {

			TRACE("Lat will Transfer Size: %d\n", byteCount);
			if (!WriteToDevice(pBuffer, byteCount))
			return FALSE;

			byteCount = 0;
		} else {

			USB_DEVICE_DESCRIPTOR stUSBDescriptor;
			UINT uintMaxPacketSize0 = 0;

			#define MINUM_TRANSFER_SIZE 0x20

			memset(&stUSBDescriptor, 0, sizeof(USB_DEVICE_DESCRIPTOR));

			if (!USB_GetDeviceDescriptor(&stUSBDescriptor))
				return FALSE;
			uintMaxPacketSize0 = stUSBDescriptor.bMaxPacketSize0;

			for (; byteCount > uintMaxPacketSize0; \
				byteCount -= uintMaxPacketSize0, pBuffer += uintMaxPacketSize0) {
				if (!WriteToDevice(pBuffer, uintMaxPacketSize0))
					return FALSE;
				TRACE("Transfer Size: %d\n", uintMaxPacketSize0);
			}

			for (; byteCount >= MINUM_TRANSFER_SIZE; \
				byteCount -= MINUM_TRANSFER_SIZE, pBuffer += MINUM_TRANSFER_SIZE) {
					if (!WriteToDevice(pBuffer, MINUM_TRANSFER_SIZE))
						return FALSE;
					TRACE("Transfer Size: %d\n", MINUM_TRANSFER_SIZE);
			}
		}
		if (0 != byteCount) {
			if (!WriteToDevice(pBuffer, byteCount))
				return FALSE;
		}

		TRACE("Transfer Size: %d\n", byteCount);
	}

	if(m_hWnd) {
		SendMessage(m_hWnd,WM_USER_PROGRESS, (WPARAM)opMode, (LPARAM)downloadPerOnce);
	}

/*
	while (byteCount > 0)
	{
		if (byteCount >= downloadPerOnce)
		{
			status = WriteToDevice(pBuffer,downloadPerOnce);

			if (!status)
			{
				return FALSE;
			}
			byteCount -= downloadPerOnce;
			pBuffer += downloadPerOnce;
			if(m_hWnd)
			{
				SendMessage(m_hWnd,WM_USER_PROGRESS,(WPARAM)opMode,(LPARAM)downloadPerOnce);
			}
		}	
		else
		{
			status = WriteToDevice(pBuffer,byteCount);
	
			if (!status)
			{
				return FALSE;
			}
			byteCount = 0;
			if(m_hWnd)
			{
				SendMessage(m_hWnd,WM_USER_PROGRESS,opMode,downloadPerOnce);
			}
        }
	}
	*/

	return TRUE;
}

BOOL AtkHostApiClass::DownloadDCD(UINT address, int byteCount, const unsigned char* pBuf)
{

	//BOOL status = FALSE;

	if (SendCommand2RoK(address, byteCount, ROM_KERNEL_WF_FT_DCD))
	{
		return (TransData(byteCount,pBuf,OPMODE_DOWNLOAD));
	}

	return FALSE;
}

BOOL AtkHostApiClass::DownloadCSF(UINT address, int byteCount, const unsigned char* pBuf)
{

	//BOOL status = FALSE;

	if (SendCommand2RoK(address, byteCount, ROM_KERNEL_WF_FT_CSF))
	{
		return (TransData(byteCount,pBuf,OPMODE_DOWNLOAD));
	}

	return FALSE;
}


BOOL AtkHostApiClass::DownloadImage(UINT address, UINT byteCount, const unsigned char* pBuf)

{

	int counter = 0;
	int bytePerCommand = 0;
	const unsigned char*  pBuffer = pBuf;		
	//BOOL status = FALSE;
	//int downloadPerOnce = WR_BUF_MAX_SIZE;

	while (1)
	{
		if (byteCount > MAX_SIZE_PER_DOWNLOAD_COMMAND)
		{
			bytePerCommand = MAX_SIZE_PER_DOWNLOAD_COMMAND;
			byteCount -= MAX_SIZE_PER_DOWNLOAD_COMMAND;
		}	
		else
		{
			bytePerCommand = byteCount;
			byteCount = 0;
		}

		if (!SendCommand2RoK(address + counter*MAX_SIZE_PER_DOWNLOAD_COMMAND, 
										   bytePerCommand, ROM_KERNEL_WF_FT_OTH))
		{
			return FALSE;
		}
		else
		{
			Sleep(10);
			if(!TransData(bytePerCommand,pBuffer,OPMODE_DOWNLOAD))
			{
				return FALSE;
			}

			pBuffer += bytePerCommand;

		}
		
		if (!byteCount)
		{
			if (!SendCommand2RoK(address, 0x400, ROM_KERNEL_WF_FT_APP))
			{
				return FALSE;
			}
			else
			{
				Sleep(10);
				pBuffer = pBuf;
				if(!TransData(0x400,pBuffer,OPMODE_DOWNLOAD))
				{
					return FALSE;
				}

				break;
			}
		}
		else
		{
			counter++;
		}

	} 

	return TRUE;
}


BOOL AtkHostApiClass::SendCommand2RoK(UINT address, UINT byteCount, UCHAR type)
{

	int format = 0;
	long data = 0;	
	PUINT pCommandAck;	
	UINT command[4];
	int commandAckSize = 4;
	int commandSize = ROM_KERNEL_CMD_SIZE;
	const short header = ROM_KERNEL_CMD_WR_FILE;
	//BOOL status = FALSE;

	for (int i=0;i<4;i++)
	{
		command[i] = 0;
	}

	command[0] = (((address & 0x00FF0000) <<8) 
						| ((address & 0xFF000000) >> 8) 
						|(header & 0xFFFF));
	command[1] = (((byteCount &0xFF000000) )
						|((format &0xFF)<<16) 
						| ((address & 0xFF) << 8)
						|((address & 0xFF00) >> 8 ));
	command[2] = ((data & 0xFF000000)
						|((byteCount & 0xFF)<<16)
						|(byteCount & 0xFF00)
						|((byteCount & 0x00FF0000)>>16));
	command[3] = (((data & 0x00FF0000) >> 16) 
						| (data & 0xFF00)
						| ((data & 0xFF)<<16) 
						| (type << 24) );
		

	if(!WriteToDevice((PUCHAR)command, commandSize))
	{
		return FALSE;
	}
	
	//read the ACK
	pCommandAck = (PUINT)malloc (4);

	if(!ReadFromDevice((PUCHAR)pCommandAck, commandAckSize))
	{
		return FALSE;
	}
	if (pCommandAck[0] != 0x56787856 && 
		pCommandAck[0] != 0x12343412 ) 
	{
		free(pCommandAck);
		return FALSE;	
	}

	free(pCommandAck); 
	return TRUE;			
	

}

BOOL AtkHostApiClass::OpenUSB(int imxType)
{
	BOOL USBPresent = FALSE;

	USBPresent = USB_OpenLocateDevice(imxType/*, m_usbOpenTimeOut*/);
	
	return USBPresent;

}

void AtkHostApiClass::CloseUSB(void)
{
	USB_CloseLocateDevice();
}

BOOL AtkHostApiClass::ReadDataByRok(UINT address, PUINT pReadData, UINT byteCount, UINT format)
{
	
	PUINT pCommandAck;
	UINT command[5];
	BOOL status = FALSE;
	USHORT header = ROM_KERNEL_CMD_RD_MEM;

	command[0] = (((address & 0x00FF0000) <<8) 
					| ((address & 0xFF000000) >> 8) 
					|(header & 0xFFFF));
	command[1] = (((byteCount &0xFF000000) )|((format &0xFF)<<16)
					| ((address & 0xFF) << 8)
					|((address & 0xFF00) >> 8 ));
	command[2] = ((0x00 & 0xFF000000)
					|((byteCount & 0xFF)<<16)
					|(byteCount & 0xFF00)
					|((byteCount & 0x00FF0000)>>16));
	command[3] = (((0x00 & 0x00FF0000) >> 16) 
					| (0x00 & 0xFF00)
					| ((0x00 & 0xFF)<<16));


	status = WriteToDevice((unsigned char *)command, 16);
	
	if(!status)
	{
		return FALSE;
	}

	pCommandAck =(PUINT) malloc (4);

	if(!ReadFromDevice((unsigned char *)pCommandAck,4))
	{
		return FALSE;
	}
	
	if (*pCommandAck != 0x56787856)
	{
		return FALSE;
	}
	status = ReadFromDevice((unsigned char *)pReadData,byteCount);

	if (!status)
	{
		return FALSE;
	}

	return TRUE;
}

int AtkHostApiClass::GetHABStatus(void)
{

	PLONG pCommandAck;
	UINT command[5];
	int commandHeader = ROM_KERNEL_CMD_GET_STAT;
	//BOOL status = FALSE;

	command[0] = (((0x0 & 0x00FF0000) << 8) 
				| ((0x0 & 0xFF000000) >> 8) 
				|(commandHeader & 0xFFFF));

	command[1] = 0x0;
	command[2] = 0x0;
	command[3] = 0x0;


	if(!WriteToDevice((unsigned char *)command, 16))
	{
		return FALSE;
	}

	pCommandAck =(PLONG) malloc (4);

	//read the ACK
	if(!ReadFromDevice((unsigned char *)pCommandAck,4))
	{
		return FALSE;
	}

	return pCommandAck[0];

}

BOOL AtkHostApiClass::SendCommand2RaK(UINT address, USHORT cmd, UINT size)
{

	int CommandSize = RAM_KERNEL_CMD_SIZE;
	unsigned char command[RAM_KERNEL_CMD_SIZE];
	
	const short header = RAM_KERNEL_CMD_HEADER;	

	*(unsigned short *)&command[0]	= header;
	*(unsigned int   *)&command[2]	= ((address & 0xFF000000)>>24)
									    | ((address & 0x00FF0000)>>16)
										| ((address & 0x0000FF00)>>8)
										| (address & 0x000000FF);

	*(unsigned short *)&command[6]	= (unsigned short)(((cmd & 0xFF00)>>8) 
										| ((cmd & 0x00FF)<<8));

	*(unsigned int   *)&command[8]	=  ((size & 0xFF000000)>>24)
									    | (size & 0x00FF0000)>>8
										| ((size & 0x0000FF00)<<8)
										| ((size & 0x000000FF)<<24);


	return (WriteToDevice((unsigned char *)command, CommandSize));

}


#define MAX_USHORT	((unsigned short)0 - 1)
#define MAX_PROGRAM_SIZE	(2 * 1024 * 1024)
//#define DEBUG	TRACE

//-------------------------------------------------------------------------------------		
// Function to reset the board device
//
// @return
//-------------------------------------------------------------------------------------
int AtkHostApiClass::CommonReset(void)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0};
	BOOL status;

	// pack command requeset
	PackCommand(command, CMD_RESET, 0, 0, 0);

	// send command to remote device
	status = WriteToDevice((unsigned char *)command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	return RET_SUCCESS;
}

//-------------------------------------------------------------------------------------		
// Function to inital the flash device
// Some nand flash needed to check the vendor and chipid
//
// @return
//-------------------------------------------------------------------------------------
int AtkHostApiClass::InitFlash(void)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	BOOL status;

	// pack command requeset
	PackCommand(command, CMD_FLASH_INITIAL, 0, 0, 0);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	// read repsonse from device
	status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
	if (!status) {
		return INVALID_CHANNEL;
	}

	// unpack the response
	res = UnPackResponse(retBuf);

	if (res.ack != RET_SUCCESS) {
		// if response is not ok, return failed
		TRACE("AtkFlashinitial(): get response: %d\n", res.ack);
		return -res.ack;
	}

	return RET_SUCCESS;

}

//-------------------------------------------------------------------------------------		
// Function to erase a select area of flash
//
// @addr  erase start address
// @size  erase size in bytes
// @return  
//   If successful, return the actual erase size in bytes; 
//   otherwise return FLASH_FAILED.
//-------------------------------------------------------------------------------------		
int AtkHostApiClass::EraseFlash(unsigned long addr, unsigned long size)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	unsigned long finished = 0;
	BOOL status;
	int to = DEFAULT_UART_TIMEOUT << 2;

	// pack command requeset
	PackCommand(command, CMD_FLASH_ERASE, addr, size, 0);

	// make timeout longer
	SetUartTimeout(to);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		SetUartTimeout(DEFAULT_UART_TIMEOUT);
		return INVALID_CHANNEL;
	}

	::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_ERASE, 1);

	do {
		// read repsonse from device
		status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
		if (!status) {
			SetUartTimeout(DEFAULT_UART_TIMEOUT);
			return INVALID_CHANNEL;
		}
		
		// unpack the response
		res = UnPackResponse(retBuf);
		
		if (res.ack == FLASH_ERASE) {
			/* send message to UI to show progress */
			::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_ERASE, (LPARAM)finished);
			TRACE("AtkFlashErase(): get flash erase partly message!erase block id:%d\n,res",res.csum);
		} else if (res.ack == RET_SUCCESS) {
			::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_ERASE, 0);
			break;
		} else {
			// if response is not ok, return failed
			TRACE("AtkFlasherase(): get response: %d\n", res.ack);
			SetUartTimeout(DEFAULT_UART_TIMEOUT);
			return -res.ack;
		}
		finished += res.len;
	} while (1);

	SetUartTimeout(DEFAULT_UART_TIMEOUT);
	return RET_SUCCESS;

}

//-------------------------------------------------------------------------------------		
// Function to dump a select area of flash
//
// @addr  dump start address
// @count  dump size in bytes
// @buffer  data buffer store the read datas
// @return  
//   If successful, return the actual read size in bytes; 
//   if ECC error, return FLASH_ECC_ERROR; 
//   otherwise return FLASH_FAILED.
//-------------------------------------------------------------------------------------		
int AtkHostApiClass::ReadFlash(unsigned long addr, unsigned char *buffer, 
									unsigned long count,unsigned long done)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res = {1, 1, 1};
	unsigned short checksum = 0;
	unsigned long size = 0;
	unsigned char *buf = buffer;
	BOOL status;
	int go_on;

	//continue reading flag
	go_on = (done == 0) ? 0 : 1;

	TRACE("Dump:the go_on flag from host is %d\n",go_on);
	
	// check the size
	if (count == 0 || buffer == NULL)
		return INVALID_PARAM;
	
	// pack command requeset
	PackCommand(command, CMD_FLASH_DUMP, addr, count, go_on);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	while (count && res.ack != RET_SUCCESS) {

		// read repsonse from device
		status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
		if (!status) {
			return INVALID_CHANNEL;
		}
	
		// unpack the response
		res = UnPackResponse(retBuf);

		if (res.ack != RET_SUCCESS && res.ack != FLASH_PARTLY) {
			// if response is not ok, return failed
			TRACE("AtkFlashread(): get response: %d\n", res.ack);
			return -res.ack;
		}

		size += res.len;
		count -= res.len;

		// read datas into buffer
		status = ReadFromDevice(buf, res.len);
		if (!status) {
			return INVALID_CHANNEL;
		}

		// update UI progress
		::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_DUMP, (LPARAM)(size + done));
		TRACE("AtkFlashRead(): recv response:%d, %d, %d\n", res.ack, res.csum, res.len);
		
		// do checksum, and compare with the csum in response
		if (res.csum != (checksum = MakeChecksum(buf, res.len))) {
			TRACE("AtkFlashread(): invalid checksum (%x)<->(%x)\n", res.csum, checksum);
			return INVALID_CHECKSUM;
		}
		
		buf += res.len;
	}
	// return the actual read data count
	return size;
}

//-------------------------------------------------------------------------------------		
// Function to program flash
//
// @addr  program start address
// @count  program size in bytes
// @buffer  data buffer to program to flash
// @mode  un-boundray or not
// @return  
//   If successful, return the actual program size in bytes; 
//   if ECC error, return FLASH_ECC_ERROR; 
//   otherwise return FLASH_FAILED.
//-------------------------------------------------------------------------------------		
int AtkHostApiClass::ProgramFlash(unsigned long addr, const unsigned char *buffer, 
								  unsigned long count, unsigned long done, int mode,UCHAR format)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res = {1, 1, 1};
	unsigned short checksum = 0;
	//unsigned int size = 0,total = 0;
	const unsigned char *buf = buffer;
	unsigned long finished = 0;
	BOOL status;
	unsigned char flashMode;
	unsigned char readbackcheck;
	int param1 = 0;

	//int timeout =0;

	// check the size
	if (count == 0 || buffer == NULL)
		return INVALID_PARAM;
	
	flashMode = (unsigned char)(((mode & 0xff) == FLASH_PRG_BOUNDARY) ? 
								CMD_FLASH_PRORAM : CMD_FLASH_PRORAM_UB);

	readbackcheck = (unsigned char)(mode >> 8);

	param1 = format | readbackcheck << 16;

	TRACE("Prog:the go_on flag from host is %d\n", done ==0 ? 0 :1);

	// pack command requeset
	PackCommand(command, flashMode, addr, count, done == 0 ? param1 : param1 | 1 << 8);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
		
	if (!status) {
		return INVALID_CHANNEL;
	}

	// read repsonse from device
	status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
	if (!status) {
		return INVALID_CHANNEL;
	}
	// unpack the response
	res = UnPackResponse(retBuf);

	if (res.ack != RET_SUCCESS) {
		// if response is not ok, return failed
		TRACE("AtkFlashprogram(): get response: %d\n", res.ack);
		return -res.ack;
	}

	// keep track the progress bar
	if(done > 0)
		::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_PROG, done);

	DWORD dwStart = GetTickCount();
	// write program data to device
	status = TransData(count, buf, OPMODE_FLASH_DOWNLOAD);

	DWORD dwStop = GetTickCount();

	TRACE("Trans data cost %d ms\n", dwStop - dwStart);

	if (!status) {
		return INVALID_CHANNEL;
	}

	TRACE(">> after transdata\n");

	do {

		// read repsonse from device
		status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
		if (!status) {
			return INVALID_CHANNEL;
		}
		// unpack the response
		res = UnPackResponse(retBuf);

		TRACE("Do Flash program, programing: get response: %d:program block id: %d:program size:%d\n", res.ack,res.csum,res.len);

		if (res.ack != FLASH_PARTLY && res.ack != FLASH_VERIFY &&
			res.ack !=RET_SUCCESS) {
			// if response is not ok, return failed
			TRACE("AtkFlashprogram(): get response: %d\n", res.ack);
			return -res.ack;
		}
			
		finished += res.len;
		::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_PROG, (LPARAM)(finished + done));

	} while (res.ack == FLASH_PARTLY);
	
	if (readbackcheck) {
		/* FIXME, set the uart time due to the calculate 
		 * the checksum timeout,don't know why so long time
		 */
		SetUartTimeout(10);

		//reset the value
		finished = 0;

		do {
			
			// read repsonse from device
			status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
			if (!status) {
				SetUartTimeout(DEFAULT_UART_TIMEOUT);
				return INVALID_CHANNEL;
			}
			// unpack the response
			res = UnPackResponse(retBuf);

			TRACE("Do Flash program, verifing: get response: %d : block is %d\n", res.ack,res.csum);

			if (res.ack != RET_SUCCESS && res.ack != FLASH_VERIFY) {
				// if response is not ok, return failed
				TRACE("AtkFlashprogram(): ++++++get response-------: %d\n", res.ack);
				SetUartTimeout(DEFAULT_UART_TIMEOUT);
				//return -res.ack;
			}

			finished += res.len;
			::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_VERIFY, (LPARAM)(finished + done));

		} while (res.ack == FLASH_VERIFY);

		//reset the timeout value
		SetUartTimeout(DEFAULT_UART_TIMEOUT);

		// send the checksum message
		::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_CHECKSUM, 0);
		
		// do checksum, and compare with the csum in response
		if (res.csum != (checksum = MakeChecksum(buf, count))) {
			TRACE("AtkFlashprogram(): invalid checksum (%x)<->(%x)\n", res.csum, checksum);
			SetUartTimeout(DEFAULT_UART_TIMEOUT);
			return INVALID_CHECKSUM;
		}
	}		
	// return the actual read data count
	return count;


}

#ifdef ATK_INTERNAL_REL
#include "FuseApiImplement.c"
#else
//-------------------------------------------------------------------------------------		
// Function to read fuse word
//
// @addr  fuse word address
// @pval  fuse value read out
// @return  
//   If read successful, return RET_SUCCESS; 
//   if the fuse element is read protected, return FUSE_READ_PROTECT; 
//   otherwise return FUSE_FAILED.
//-------------------------------------------------------------------------------------		
int AtkHostApiClass::ReadFuse(unsigned long addr, unsigned char *pval)
{
	return RET_SUCCESS;
}

//-------------------------------------------------------------------------------------		
// Function to sense fuse word
//
// @addr  fuse word address
// @pval  fuse value sense out
// @return  
//   If read successful, return RET_SUCCESS; 
//   if the fuse element is read protected, return FUSE_READ_PROTECT; 
//   otherwise return FUSE_FAILED.
//-------------------------------------------------------------------------------------		
int AtkHostApiClass::SenseFuse(unsigned long addr, unsigned char *pval, unsigned char bit)
{
	return RET_SUCCESS;
}

//-------------------------------------------------------------------------------------		
// Function to override fuse word in cache
//
// @addr  fuse word address
// @val  fuse value to override
// @return  
//   If override successful, return RET_SUCCESS; 
//   if the fuse element is override protected, return FUSE_OVERRIDE_PROTECT; 
//   otherwise return FUSE_FAILED.
//-------------------------------------------------------------------------------------		
int AtkHostApiClass::OverrideFuse(unsigned long addr, unsigned char val)
{	
	return RET_SUCCESS;
}

//-------------------------------------------------------------------------------------		
// Function to program fuse element in fuse box
//
// @addr  fuse element row address
// @val  fuse value to program
// @return  
//   If program successful and verify is passed, return RET_SUCCESS; 
//   if the fuse element is write protected, return FUSE_WRITE_PROTECT; 
//   if the program successfully , but verify can't be done, return FUSE_VERIFY_FAILED; 
//   otherwise return FUSE_FAILED.
//-------------------------------------------------------------------------------------		
int AtkHostApiClass::ProgramFuse(unsigned long addr, unsigned char val)
{
	return RET_SUCCESS;
}

#endif

//-------------------------------------------------------------------------------------		
// Function to get device status (bootstrap or RAM Kernel)
//
// @fmodel  flash model string
// @len  flash model string length
// @mxType  chip id
// @return
//   If RAM Kernel or bootstrap is running, return RET_SUCCESS; 
//   If no program running return INVALID_CHANNEL.
//-------------------------------------------------------------------------------------		
int AtkHostApiClass::GetRKLVersion(unsigned char *fmodel, int *len, int *mxType)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	BOOL status;
		
	// pack command requeset
	PackCommand(command, CMD_GETVER, 0, 0, 0);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		TRACE("atk_common_getver(): failed to send to device\n");
		return INVALID_CHANNEL;
	}
		
	if (m_channelMode == UART)
	{
		// read repsonse from device
		status = ReadFromDevice(retBuf, 4);
		if (!status) {
			TRACE("atk_common_getver(): failed to read bootstrap from device\n");
			return INVALID_CHANNEL;
		}
		
		// check if is bootstrap
		if (retBuf[0] == 0x56 || retBuf[0] == 0x12)
		{
			*len = 0;
			return RET_SUCCESS;
		}
		
		status = ReadFromDevice(retBuf + 4, RAM_KERNEL_ACK_SIZE - 4);
		if (!status) {
			TRACE("atk_common_getver(): failed to read RKL from device\n");
			return INVALID_CHANNEL;
		}
		
	} else {
		status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
		if (!status) {
			TRACE("atk_common_getver(): failed to read bootstrap from device\n");
			return INVALID_CHANNEL;
		}
		
		// check if is bootstrap
		if (retBuf[0] == 0x56 || retBuf[0] == 0x12)
		{
			*len = 0;
			return RET_SUCCESS;
		}
		
	}
	// unpack the response
	res = UnPackResponse(retBuf);

	if (res.ack == RET_SUCCESS) {
		
		status = ReadFromDevice(fmodel, res.len);
		if (!status) {
			return INVALID_CHANNEL;
		}
		*len = res.len;
		*mxType = res.csum;
		fmodel[res.len] = '\0';
		TRACE("atk_common_getver(): get version: %s\n", fmodel);
	} else {
		// if response is not ok, return failed
		TRACE("atk_common_getver(): get response: %d\n", res.ack);
	}
	
	return -res.ack;
}

int AtkHostApiClass::CommonDownload(unsigned long addr, unsigned long size, const unsigned char *pBuf)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	BOOL status;
	struct Response res;

	// pack command requeset
	PackCommand(command, CMD_DOWNLOAD, addr, size, 0);

	// send command to remote device
	status = WriteToDevice((unsigned char *)command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		TRACE("atk_common_download(): failed to send to device\n");
		return INVALID_CHANNEL;
	}
	
	// download image
	status = TransData(size, pBuf, OPMODE_FLASH_DOWNLOAD);
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
	if (!status) {
		TRACE("atk_common_getver(): failed to read bootstrap from device\n");
		return INVALID_CHANNEL;
	}
	
	// unpack the response
	res = UnPackResponse(retBuf);
	
	return res.ack;

}

int AtkHostApiClass::CommonExecute(unsigned long addr)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0};
	BOOL status;

	// pack command requeset
	PackCommand(command, CMD_EXECUTE, addr, 0, 0);

	// send command to remote device
	status = WriteToDevice((unsigned char *)command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	return RET_SUCCESS;
}

//-------------------------------------------------------------------------------------		
// Function to calculate the check sum of data
//
// @buffer data buffer
// @count  data length
//
// @return the checksum value.
//-------------------------------------------------------------------------------------		
unsigned short AtkHostApiClass::MakeChecksum(const unsigned char* buffer, unsigned long count)
{
	unsigned short csum = 0;

	while (count --) {
		csum = (unsigned short)(csum + (*(buffer++)));
	}

	return csum;
}

//-------------------------------------------------------------------------------------		
// Function to encapsure the UART/USB transmit
//
// @buf data buffer to transmit
// @count  data length
//
// @return TRUE or FALSE.
//-------------------------------------------------------------------------------------		
BOOL AtkHostApiClass::WriteToDevice(const unsigned char *buf, unsigned int count)
{
	BOOL ret;
	if(m_hEvent !=NULL){
		if(::WaitForSingleObject(m_hEvent,0) == WAIT_OBJECT_0){
			TRACE("receive the stop event\n");
			return FALSE;
		}
	}
	if (m_channelMode) { // USB	
		ret = WriteToUsb(count, (unsigned int*)buf, m_usbId);
	} else { // UART
		ret = WriteBuffer((unsigned char*)buf, count);
	}

	TRACE("write to device(%d):%d\n", m_channelMode, ret);
	return ret;
}

//-------------------------------------------------------------------------------------		
// Function to encapsure the UART/USB receive
//
// @buf data buffer to receive
// @count  data length
//
// @return TRUE or FALSE.
//-------------------------------------------------------------------------------------		
BOOL AtkHostApiClass::ReadFromDevice(unsigned char *buf, unsigned int count)
{
	BOOL ret = 0;
	unsigned int size = count;
	
	while (count > 0) {
		if(m_hEvent !=NULL){
			if(::WaitForSingleObject(m_hEvent,0) == WAIT_OBJECT_0){
				TRACE("receive the stop event\n");
				return FALSE;
			}
		}
		// we read data one by one with 16K
		size = count > 0x4000 ? 0x4000:count;
		
		if (m_channelMode) { // USB	
			ret = ReadFromUsb(size, m_usbId, buf/*, m_usbTransTimeOut*/);
		} else { // UART
			ret = ReadBuffer(buf, (DWORD)size, m_uartTimeout);
		}
		
		buf += size;
		count -= size;
	}

	TRACE("Read from device(%d):%d\n", m_channelMode, ret);
	return ret;
}

//-------------------------------------------------------------------------------------		
// Function to pack the command request
//
// @cmd  command id
// @addr  address element
// @param  parameter element
//
// @return
//-------------------------------------------------------------------------------------		
void AtkHostApiClass::PackCommand(unsigned char *cmd, unsigned short cmdId, unsigned long addr, 
								  unsigned long param, unsigned long param1)
{
	cmd[0] = (unsigned char)(RKL_COMMAND_MAGIC >> 8);
	cmd[1] = (unsigned char)(RKL_COMMAND_MAGIC & 0x00ff);
	cmd[2] = (unsigned char)(cmdId >> 8);
	cmd[3] = (unsigned char)cmdId;
	cmd[4] = (unsigned char)(addr >> 24);
	cmd[5] = (unsigned char)(addr >> 16);
	cmd[6] = (unsigned char)(addr >> 8);
	cmd[7] = (unsigned char)addr;
	cmd[8] = (unsigned char)(param >> 24);
	cmd[9] = (unsigned char)(param >> 16);
	cmd[10] = (unsigned char)(param >> 8);
	cmd[11] = (unsigned char)param;
	cmd[12] = (unsigned char)(param1 >> 24);
	cmd[13] = (unsigned char)(param1 >> 16);
	cmd[14] = (unsigned char)(param1 >> 8);
	cmd[15] = (unsigned char)param1;
	return;
}

//-------------------------------------------------------------------------------------		
// Function to unpack the response
//
// @response  response read from device
//
// @return
//-------------------------------------------------------------------------------------		
struct Response AtkHostApiClass::UnPackResponse(unsigned char *resBuf)
{
	struct Response res;

	res.ack = (unsigned short)(((unsigned short)resBuf[0] << 8) | resBuf[1]);
	res.csum = (unsigned short)(((unsigned short)resBuf[2] << 8) | resBuf[3]);
	res.len = ((unsigned long)resBuf[4] << 24) | ((unsigned long)resBuf[5] << 16) |
				((unsigned long)resBuf[6] << 8) | ((unsigned long)resBuf[7]);

	return res;
}
//-------------------------------------------------------------------------------------		
// Function to set event handle
//
// @return
//-------------------------------------------------------------------------------------
void AtkHostApiClass::SetEvtHandle(HANDLE hEvent)
{
	m_hEvent = hEvent;
}

//-------------------------------------------------------------------------------------		
// Function to set UART receive timeout
//
// @return
//-------------------------------------------------------------------------------------
void AtkHostApiClass::SetUartTimeout(int timeout)
{
	m_uartTimeout = timeout;
}

//-------------------------------------------------------------------------------------		
// Function to set UART receive timeout
//
// @return
//-------------------------------------------------------------------------------------
void AtkHostApiClass::SetUsbTimeout(int openTimeOut, int TransTimeOut)
{
	m_usbOpenTimeOut = openTimeOut;
	m_usbTransTimeOut = TransTimeOut;
}

//-------------------------------------------------------------------------------------		
// Function to get HAB_TYPE value
//
// @return
//		true: if is prodction
//		false: if is development/disable
//-------------------------------------------------------------------------------------
bool AtkHostApiClass::GetHABType(int mode)
{
	long command[4];
	//int developmentLevel = 0;
	PUINT pWriteAck;
	PUINT pCommandAck;
	const short header = ROM_KERNEL_CMD_WR_MEM;
	BOOL status = FALSE;

	//clear the command array
	for (int i=0; i<4;i++)
	{
		command[i] = 0;
	}

    if ((mode == MX_MX51_TO1) || (mode == MX_MX51_TO2))
    {
	    command[0] = (((0x1ffea000 & 0x00FF0000) <<8) 
				    | ((0x1ffea000 & 0xFF000000) >> 8) 
				    |(header & 0xFFFF));

	    command[1] = (((0x00 &0xFF000000) )
				    |((0x20 &0xFF)<<16)
				    | ((0x1ffea000 & 0xFF) << 8)
				    |((0x1ffea000 & 0xFF00) >> 8 ));

	    command[2] = ((0 & 0xFF000000)
				    |((0x00 & 0xFF)<<16)
				    |(0x00 & 0xFF00)
				    |((0x00 & 0x00FF0000)>>16));

	    command[3] = (((0 & 0x00FF0000) >> 16) 
				    | (0 & 0xFF00)
				    | ((0 & 0xFF)<<16));

    }
    else
    {
	    command[0] = (0xFFFF << 16) |(header & 0xFFFF);

	    command[1] = (((0x00 &0xFF000000) )
				    |((32 & 0xFF)<<16)
				    | 0xFFFF);

	    command[2] = ((0 & 0xFF000000)
				    |((0x00 & 0xFF)<<16)
				    |(0x00 & 0xFF00)
				    |((0x00 & 0x00FF0000)>>16));

	    command[3] = (((0 & 0x00FF0000) >> 16) 
				    | (0 & 0xFF00)
				    | ((0 & 0xFF)<<16));
    }

	//Send write Command to USB
	status = WriteToDevice((unsigned char *)command, 16);

	if (!status)
	{
		return FALSE;
	}
	pCommandAck =(PUINT) malloc (4);

	//read the ACK
	if(!ReadFromDevice((unsigned char *)pCommandAck,4))
	{
		return FALSE;
	}

	// get response of HAB type
	TRACE("GetHABType(): Receive ack:%x\n", pCommandAck[0]);
	if (pCommandAck[0] == 0x12343412)
	{
		TRACE("GetHABType(): this is production type\n");
		free (pCommandAck);
		// production parts
		if ((mode == MX_MX35_TO1) || (mode == MX_MX35_TO2) || 
			(mode == MX_MX37) || (mode == MX_MX51_TO1) || (mode == MX_MX51_TO2))
		{
			pWriteAck = (PUINT) malloc (4);

    	    ReadFromDevice((PUCHAR)pWriteAck,4);
	        TRACE("GetHABType(): Receive ack2:%x\n", pWriteAck[0]);

			free(pWriteAck);
		}

		return TRUE; 

	} else if (pCommandAck[0] == 0x56787856) {
        PUINT pWriteAck;

		TRACE("GetHABType(): this is develop/disable type\n");
        free (pCommandAck);

	    pWriteAck = (PUINT) malloc (4);

	    ReadFromDevice((PUCHAR)pWriteAck,4);

   	    free (pWriteAck);
	    
        return FALSE;	
	}

    return FALSE;
}


//-------------------------------------------------------------------------------------		
// Function to switch from UART to USB value
// This is the hot fix for MX31 & MX32
// @return
//		true:  if successful
//		false: if failed
//-------------------------------------------------------------------------------------
bool AtkHostApiClass::DoCom2Usb(void)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0};
				  //retBuf[RAM_KERNEL_ACK_SIZE] = {0};

	BOOL status;
	
	// pack command requeset
	PackCommand(command, CMD_COM2USB, 0, 0, 0);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	if (!status) {
		TRACE("Failed to send COM2USB command\n");
		return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------		
// Function to Set the IC type
// @input IMX type
// @return
//-------------------------------------------------------------------------------------
void AtkHostApiClass::SetiMXType(int mxType)
{
	m_iMxType = mxType;
}

//-------------------------------------------------------------------------------------		
// Function to set the BI Swap Flag
// Fix the NFC imcompatible problem with
// the NAND flash out of factory in terms of BI field.
// @input int flag
// @return int,none zero mean failed
//-------------------------------------------------------------------------------------
int AtkHostApiClass::SetBISwapFlag(int flag)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	BOOL status;

	// pack command request,use para field
	PackCommand(command, CMD_SWAP_BI, 0, flag, 0);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	// read response from device
	status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
	if (!status) {
		return INVALID_CHANNEL;
	}

	// unpack the response
	res = UnPackResponse(retBuf);

	if (res.ack != RET_SUCCESS) {
		// if response is not ok, return failed
		TRACE("ATKSetBISwapFlag(): get response: %d\n", res.ack);
		return -res.ack;
	}

	return RET_SUCCESS;

}

//-------------------------------------------------------------------------------------		
// Function to set the flash based bbt Flag
// This flash indicate whether store the bad 
// block information to the nand falsh 
// @input int flag
// @return int,none zero mean failed
//-------------------------------------------------------------------------------------
int AtkHostApiClass::SetBBTFlag(int flag)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	BOOL status;

	// pack command request,use para field
	PackCommand(command, CMD_FL_BBT, 0, flag, 0);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	// read response from device
	status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
	if (!status) {
		return INVALID_CHANNEL;
	}

	// unpack the response
	res = UnPackResponse(retBuf);

	if (res.ack != RET_SUCCESS) {
		// if response is not ok, return failed
		TRACE("ATKSetBBTFlag(): get response: %d\n", res.ack);
		return -res.ack;
	}

	return RET_SUCCESS;

}

//-------------------------------------------------------------------------------------		
// Function to get the flash capacity
//
// @return
//-------------------------------------------------------------------------------------
int AtkHostApiClass::GetFlashCapacity(unsigned long *size)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	BOOL status;

	// pack command requeset
	PackCommand(command, CMD_FLASH_GET_CAPACITY, 0, 0, 0);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	// read repsonse from device
	status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
	if (!status) {
		return INVALID_CHANNEL;
	}

	// unpack the response
	res = UnPackResponse(retBuf);

	if (res.ack != RET_SUCCESS) {
		// if response is not ok, return failed
		TRACE("GetFlashCapacity(): get response: %d\n", res.ack);
		return -res.ack;
	}

	*size = res.len;
	return RET_SUCCESS;
}

//-------------------------------------------------------------------------------------		
// Function to set the interleave mode Flag
// This flash indicate whether store the 
// interleave information to the nand falsh 
// @input int flag
// @return int,none zero mean failed
//-------------------------------------------------------------------------------------
int AtkHostApiClass::SetINTLVFlag(int flag)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	BOOL status;

	// pack command request,use para field
	PackCommand(command, CMD_FL_INTLV, 0, flag, 0);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);
	
	if (!status) {
		return INVALID_CHANNEL;
	}
	
	// read response from device
	status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
	if (!status) {
		return INVALID_CHANNEL;
	}

	// unpack the response
	res = UnPackResponse(retBuf);

	if (res.ack != RET_SUCCESS) {
		// if response is not ok, return failed
		TRACE("ATKSetINTLVFlag(): get response: %d\n", res.ack);
		return -res.ack;
	}

	return RET_SUCCESS;

}

//-------------------------------------------------------------------------------------		
// Function to set the LBA mode Flag
// This flash indicate whether store the 
// LBA information to the nand falsh 
// @input int flag
// @return int,none zero mean failed
//-------------------------------------------------------------------------------------
int AtkHostApiClass::SetLBAFlag(int flag)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
		retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	BOOL status;

	// pack command request,use para field
	PackCommand(command, CMD_FL_LBA, 0, flag, 0);

	// send command to remote device
	status = WriteToDevice(command, RAM_KERNEL_CMD_SIZE);

	if (!status) {
		return INVALID_CHANNEL;
	}

	// read response from device
	status = ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE);
	if (!status) {
		return INVALID_CHANNEL;
	}

	// unpack the response
	res = UnPackResponse(retBuf);

	if (res.ack != RET_SUCCESS) {
		// if response is not ok, return failed
		TRACE("ATKSetLBAFlag(): get response: %d\n", res.ack);
		return -res.ack;
	}

	return RET_SUCCESS;

}