#pragma once
#include "stdafx.h"
#include "MxRomDevice.h"
#include "DeviceManager.h"
#include <sys/stat.h>

#include "iMX/MemoryInit.h"
#include "iMX/AtkHostApiClass.h"

MxRomDevice::MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
	: Device(deviceClass, devInst, path)
{
}

void MxRomDevice::SetIMXDevPara(CString cMXType, CString cSecurity, CString cRAMType, unsigned int RAMKNLAddr)
{
	atkConfigure.SetChannel(ChannelType_USB, 0, 1);

	if(cMXType ==  _T("25_TO1"))
		atkConfigure.SetMXType(MX_MX25_TO1);
	else if(cMXType ==  _T("25_TO11"))
		atkConfigure.SetMXType(MX_MX25_TO11);
	else if(cMXType ==  _T("31_TO1"))
		atkConfigure.SetMXType(MX_MX31_TO1);
	else if(cMXType ==  _T("31_TO2"))
		atkConfigure.SetMXType(MX_MX31_TO2);
	else if(cMXType ==  _T("35_TO1"))
		atkConfigure.SetMXType(MX_MX35_TO1);
	else if(cMXType ==  _T("35_TO2"))
		atkConfigure.SetMXType(MX_MX35_TO2);
	else if(cMXType ==  _T("37"))
		atkConfigure.SetMXType(MX_MX37);
	else if(cMXType ==  _T("51_TO2"))
		atkConfigure.SetMXType(MX_MX51_TO2);


	if(cSecurity == _T("true"))
	{
		atkConfigure.SetSecurity(TRUE);
	}
	else
	{
		atkConfigure.SetSecurity(FALSE);
	}

	if(cRAMType == _T("DDR"))
		atkConfigure.SetRAMType(MM_DDR);
	else if(cRAMType == _T("SDRAM"))
		atkConfigure.SetRAMType(MM_SDRAM);
	else if(cRAMType == _T("DDR2"))
		atkConfigure.SetRAMType(MM_DDR2);
	else if(cRAMType == _T("MDDR"))
		atkConfigure.SetRAMType(MM_MDDR);
	else if(cRAMType == _T("CUSTOM"))
		atkConfigure.SetRAMType(MM_CUSTOM);

		atkConfigure.SetChannel(ChannelType_USB, 0, 1);
		//theApp.atkConfigure.SetSecurity(Security);
		atkConfigure.SetMemoryAddr(mm_addrs[atkConfigure.GetMXType()]);
		//theApp.atkConfigure.SetRAMType(RAMType);
//		theApp.objHostApiClass.SetUsbTimeout(USB_OPEN_USR_TIMEOUT,USB_TRANS_USR_TIMEOUT);
//		TRACE("Set the usb timeout open %d secs, trans %d msecs\n",USB_OPEN_USR_TIMEOUT,USB_TRANS_USR_TIMEOUT);
		
		// Check the memory initial script
		switch (atkConfigure.GetRAMType()) 
		{
			case MM_DDR:
				TRACE("setup the DDR memory init script!\n");
				atkConfigure.SetMemoryType(mmInitScripts[atkConfigure.GetMXType()][0].script, 
										mmInitScripts[atkConfigure.GetMXType()][0].lines);
				break;
			case MM_SDRAM:
				TRACE("setup the SDRAM memory init script!\n");
				atkConfigure.SetMemoryType(mmInitScripts[atkConfigure.GetMXType()][1].script, 
										mmInitScripts[atkConfigure.GetMXType()][1].lines);
				break;
			case MM_DDR2:
				TRACE("setup the DDR2 memory init script!\n");
				atkConfigure.SetMemoryType(mmInitScripts[atkConfigure.GetMXType()][0].script, 
										mmInitScripts[atkConfigure.GetMXType()][0].lines);
				break;
			case MM_MDDR:
				TRACE("setup the MDDR memory init script!\n");
				atkConfigure.SetMemoryType(mmInitScripts[atkConfigure.GetMXType()][1].script, 
										mmInitScripts[atkConfigure.GetMXType()][1].lines);
				break;
			case MM_CUSTOM:
			{
				break;
			}
			default:
				TRACE("Impossible, never reach here!\n");
				break;
		}
	atkConfigure.SetRAMKNLAddr(RAMKNLAddr);
}

BOOL MxRomDevice::InitMemoryDevice()
{
	stMemoryInit *pScript;
	int lines;
	
	pScript = atkConfigure.GetMemoryInitScript(&lines);
	if (pScript == NULL || lines == 0)
	{
		TRACE("Strange! never reach here in InitMemoryDevice\n");
		return FALSE;
	}
	
	for (int i = 0; i < lines; i++)
	{
		// TODO: the host dll MUST be modify for the mx type
		if (!WriteMemory(atkConfigure.GetMXType(),
			pScript[i].addr, pScript[i].data, pScript[i].format))
		{
			TRACE("In InitMemoryDevice(): write memory failed\n");
			return FALSE;
		}

	}

	return TRUE;
}

BOOL MxRomDevice::DownloadRKL(unsigned char *rkl, int rklsize)
{
	int cType, cId, PhyRAMAddr4KRL;
	unsigned long cHandle;
	BOOL status;
	BOOL is_hab_prod = false;
	CString m_strCSFName;
	CString m_strDCDName;

	if ( GetHABType(atkConfigure.GetMXType()) )
	{
		is_hab_prod = true;
	} else {
		is_hab_prod = false;
	}

	// do initial DDR
	if (!InitMemoryDevice()) {
		TRACE("Initial memory failed!\n");
		return FALSE;
	}
	
	atkConfigure.GetChannel(&cType, &cId, &cHandle);
	// set the communication mode through write memory interace
	if (!WriteMemory(atkConfigure.GetMXType(),
		atkConfigure.GetMemoryAddr(MEMORY_START_ADDR), 
		cType, 32))
	{
		TRACE("Write channel type to memory failed!\n");
		return FALSE;
	}

	// Download DCD and CSF file if security
	/*if (theApp.atkConfigure.GetSecurity()) {

		UINT size;
		PUCHAR buffer = NULL;
		FILE *fp;
		struct stat results;
		
		if(!m_strDCDName.IsEmpty())
		{
			// get DCD file size and open
			if (::stat((const char *)m_strDCDName.GetBuffer(), &results) == 0)
			{
				size = results.st_size;
				fp = fopen((const char *)m_strDCDName.GetBuffer(), "rb");
			} else {
				TRACE(("Can not open the DCD file: ") + m_strDCDName);
				return FALSE;
			}
			// read CSF file to buffer
			if(NULL != fp)
			{
				buffer = (PUCHAR)malloc(size);
				fread(buffer, 1, size, fp);
				fclose(fp);
			} else {
				TRACE(("Can not read the DCD file: ") + m_strDCDName);
				return FALSE;
			}
			// download CSF file to memory
			status = theApp.objHostApiClass.DownloadDCD(theApp.atkConfigure.GetMemoryAddr(DEFAULT_HWC_ADDR), 
														size, buffer);
			// free buffer
			if (buffer)
				free(buffer);

			if (!status)
			{
				TRACE("Can not load the DCD file to device\n");
				return FALSE;
			}
		}

		if(!m_strCSFName.IsEmpty())
		{
			// get CSF file size and open
			if (::stat(m_strCSFName, &results) == 0)
			{
				size = results.st_size;
				fp = fopen(m_strCSFName, "rb");
			} else {
				TRACE(("Can not open the CSF file: ") + m_strCSFName);
				return FALSE;
			}
			// read CSF file to buffer
			if(NULL != fp)
			{
				buffer = (PUCHAR)malloc(size);
				fread(buffer, 1, size, fp);
				fclose(fp);
			} else {
				TRACE(("Can not read the CSF file: " + m_strCSFName));
				return FALSE;
			}
			// download CSF file to memory
			status = theApp.objHostApiClass.DownloadCSF(theApp.atkConfigure.GetMemoryAddr(DEFAULT_CSF_ADDR), 
														size, buffer);
			if (!status)
			{
				TRACE("Can not load the CSF file to device\n");
				// free buffer
				if (buffer)
					free(buffer);
				return FALSE;
			}
			
			// free buffer
			if (buffer)
				free(buffer);
		}
	}*/

	// do download
	// download the RKL
	//Here we must transfer virtual address to physical address before downloading ram kernel.
	PhyRAMAddr4KRL = atkConfigure.GetMemoryAddr(MEMORY_START_ADDR) | \
		(atkConfigure.GetRAMKNLAddr() & (~0xF0000000));
	status = DownloadImage(PhyRAMAddr4KRL, rklsize, rkl);
	
	if (!status) {
		TRACE("Can not load RAM Kernel to device\n");
		return FALSE;
	} else {		
		// Send the complete command to the device 
		status = Jump2Rak(atkConfigure.GetMXType(), is_hab_prod);

		// check the status
		if (!status) {
			TRACE("Can not execute RAM Kernel\n");
			return FALSE;
		}
	}
	return TRUE;
}

// Write the register of i.MX
#define ROM_KERNEL_CMD_WR_MEM 0x0202
BOOL MxRomDevice::WriteMemory(int mode, UINT address, UINT Data, UINT Format)
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

//-------------------------------------------------------------------------------------		
// Function to get HAB_TYPE value
//
// @return
//		true: if is prodction
//		false: if is development/disable
//-------------------------------------------------------------------------------------
BOOL MxRomDevice::GetHABType(int mode)
{
	long command[4];
	//int developmentLevel = 0;
	PUINT pWriteAck;
	PUINT pCommandAck;
	const short header = ROM_KERNEL_CMD_WR_MEM;
	BOOL status = FALSE;

	//clear the command array
	memset(&command[0], 0, sizeof(command));

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
	memset(pCommandAck, 0, 4);

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

BOOL MxRomDevice::Jump2Rak(int mode, BOOL is_hab_prod)
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
//		SetUartTimeout(1);
		pCommandAck = (PUINT) malloc(4);
		if(!ReadFromDevice((PUCHAR)pCommandAck, CommandAckSize))
		{
			free (pCommandAck);
//			SetUartTimeout(DEFAULT_UART_TIMEOUT);
			return TRUE;
		}

//		SetUartTimeout(DEFAULT_UART_TIMEOUT);

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

#define FLASH_HEADER_SIZE	0x20
#define ROM_TRANSFER_SIZE	0x400
//#define MX_51_NK_LOAD_ADDRESS			0x90200000
//#define MX_51_EBOOT_LOAD_ADDRESS		0x90040000

BOOL MxRomDevice::DownloadImage(UINT address, UINT byteCount, const unsigned char* pBuf)
{
	int counter = 0;
	int bytePerCommand = 0;
	const unsigned char*  pBuffer = pBuf;
	int ActualImageAddr = address, FlashHdrAddr = address - FLASH_HEADER_SIZE;

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

		if (!SendCommand2RoK(ActualImageAddr + counter*MAX_SIZE_PER_DOWNLOAD_COMMAND, 
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
			//transfer length of ROM_TRANSFER_SIZE is a must to ROM code.
			unsigned char FlashHdr[ROM_TRANSFER_SIZE];
			//Init the memory
			memset(FlashHdr,0, ROM_TRANSFER_SIZE);
			//Copy image data with an offset of FLASH_HEADER_SIZE to the temp buffer.
			memcpy(FlashHdr+FLASH_HEADER_SIZE, pBuf, ROM_TRANSFER_SIZE-FLASH_HEADER_SIZE);
			//We should write actual image address to the first dowrd of flash header.
			((int *)FlashHdr)[0] = ActualImageAddr;
			//Jump to RAM kernel to execute it.
			if (!SendCommand2RoK(FlashHdrAddr, ROM_TRANSFER_SIZE, ROM_KERNEL_WF_FT_APP))
			{
				return FALSE;
			}
			else
			{
				Sleep(10);
				pBuffer = pBuf;
				if(!TransData(ROM_TRANSFER_SIZE,(const unsigned char *)FlashHdr,OPMODE_DOWNLOAD))
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

BOOL MxRomDevice::SendCommand2RoK(UINT address, UINT byteCount, UCHAR type)
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

BOOL MxRomDevice::TransData(UINT byteCount, const unsigned char * pBuf,int opMode)
{

//	BOOL status = FALSE;
	const unsigned char * pBuffer = pBuf;
	UINT downloadPerOnce = WR_BUF_MAX_SIZE;

	for (; byteCount >= downloadPerOnce; byteCount -= downloadPerOnce, pBuffer += downloadPerOnce) {
		if (!WriteToDevice(pBuffer, downloadPerOnce))
			return FALSE;

		TRACE("Transfer Size: %d\n", downloadPerOnce);

//		if(m_hWnd) {
//			SendMessage(m_hWnd,WM_USER_PROGRESS, (WPARAM)opMode, (LPARAM)downloadPerOnce);
//		}
	}

//	if (m_channelMode == UART) {
//		if (!WriteToDevice(pBuffer, byteCount))
//			return FALSE;
//	} else {
		if (((atkConfigure.GetMXType() == MX_MX31_TO1) || (atkConfigure.GetMXType() == MX_MX31_TO2) ||
			(atkConfigure.GetMXType() == MX_MX31_TO201) || (atkConfigure.GetMXType() == MX_MX32)) && byteCount > 0) {

			TRACE("Lat will Transfer Size: %d\n", byteCount);
			if (!WriteToDevice(pBuffer, byteCount))
			return FALSE;

			byteCount = 0;
		} else {

			UINT uintMaxPacketSize0 = _maxPacketSize.get();

			#define MINUM_TRANSFER_SIZE 0x20

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
//	}

//	if(m_hWnd) {
//		SendMessage(m_hWnd,WM_USER_PROGRESS, (WPARAM)opMode, (LPARAM)downloadPerOnce);
//	}

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


//HANDLE MxRomDevice::Open()
//{
//	HANDLE hDevice = CreateFile(_path.get(), GENERIC_READ|GENERIC_WRITE,
//        FILE_SHARE_READ /*|FILE_SHARE_WRITE*/, NULL, OPEN_EXISTING, 0/*FILE_FLAG_OVERLAPPED*/, NULL);
//
//    if( hDevice == INVALID_HANDLE_VALUE )
//	{
//        error = GetLastError();
//	}
// 
//    return hDevice;
//}

/*
BOOL MxRomDevice::Close(HANDLE hDevice)
{
    DWORD error = ERROR_SUCCESS;

	if( m_Handle != INVALID_HANDLE_VALUE )
	{
        if ( !CloseHandle(m_Handle) )
			error = GetLastError();

		m_Handle = INVALID_HANDLE_VALUE;
	}
	
	return error;
}
*/
BOOL MxRomDevice::WriteToDevice(const unsigned char *buf, UINT count)
{
	BOOL retValue = TRUE;

	// Open the device
	HANDLE hDevice = CreateFile(_path.get(), GENERIC_WRITE,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if( hDevice == INVALID_HANDLE_VALUE )
	{
        TRACE(_T("MxRomDevice::WriteToDevice() CreateFile() ERROR = %d.\n"), GetLastError());
		return FALSE;
	}

	// Write the data
	DWORD bytesWritten = 0;
	if( !WriteFile(hDevice, buf, count, &bytesWritten, NULL) )
	{
		TRACE(_T("MxRomDevice::WriteToDevice() Error writing to device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		retValue = FALSE;
	}

	// Close the device
	if ( !CloseHandle(hDevice) )
	{
        TRACE(_T("MxRomDevice::WriteToDevice() CloseHandle() ERROR = %d.\n"), GetLastError());
		retValue = FALSE;
	}
	
	return retValue;
}

BOOL MxRomDevice::ReadFromDevice(PUCHAR buf, UINT count)
{
	BOOL retValue = TRUE;

	// Open the device
	HANDLE hDevice = CreateFile(_path.get(), GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if( hDevice == INVALID_HANDLE_VALUE )
	{
        TRACE(_T("MxRomDevice::ReadFromDevice() CreateFile() ERROR = %d.\n"), GetLastError());
		return FALSE;
	}

	// Write the data
	DWORD bytesRead = 0;
	if( !ReadFile(hDevice, buf, count, &bytesRead, NULL) )
	{
		TRACE(_T("MxRomDevice::ReadFromDevice() Error reading from device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		retValue = FALSE;
	}

	// Close the device
	if ( !CloseHandle(hDevice) )
	{
        TRACE(_T("MxRomDevice::ReadFromDevice() CloseHandle() ERROR = %d.\n"), GetLastError());
		retValue = FALSE;
	}

	return retValue;
}