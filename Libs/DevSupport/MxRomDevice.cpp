#pragma once
#include "stdafx.h"
#include "MxRomDevice.h"
#include "DeviceManager.h"
#include "Libs/WDK/usb100.h"
#include <sys/stat.h>

#include "iMX/AtkHostApiClass.h"
//#include "iMX/MxDefine.h"
#include "../../Drivers/iMX_BulkIO_Driver/sys/public.h"

static BOOL bMutex4RamKnlDld = TRUE;//mutex flag for ram kernel downloading process to prevent other object from starting downloading.
static HANDLE sHandle[8][2];
MxRomDevice::MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
	: Device(deviceClass, devInst, path)
	, _hDevice(INVALID_HANDLE_VALUE)
	, _hWrite(INVALID_HANDLE_VALUE)
	, _hRead(INVALID_HANDLE_VALUE)
	, _chipFamily(ChipUnknown)
	, _habType(HabUnknown)
	, _romVersion(0,0)

{
	_MaxPacketSize.describe(this, _T("Max Packet Size"), _T(""));
	
	_chipFamily = GetChipFamily();

	if ( USB_OpenDevice() )
	{
		_MaxPacketSize.get();
		_habType = GetHABType(_chipFamily);
		BOOL success = InitRomVersion(_chipFamily, _romVersion, _defaultAddress);
	}
}

MxRomDevice::~MxRomDevice()
{
	USB_CloseDevice();
}

/// <summary>
/// Property: Gets the max packet size for the device.
/// </summary>
int32_t MxRomDevice::MaxPacketSize::get()
{
	MxRomDevice* dev = dynamic_cast<MxRomDevice*>(_owner);
	ASSERT(dev);

	USB_DEVICE_DESCRIPTOR devDescriptor;

	if ( Value == 0 )
	{
		if ( dev->DeviceIoControl(IOCTL_IMXDEVICE_GET_DEVICE_DESCRIPTOR, &devDescriptor) )
		{
			Value = devDescriptor.bMaxPacketSize0;
		}
	}

	return Value;
}

MxRomDevice::ChipFamily_t MxRomDevice::GetChipFamily()
{
    if ( _chipFamily == Unknown )
    {
		CString devPath = UsbDevice()->_path.get();
		devPath.MakeUpper();

		if ( devPath.Find(_T("VID_15A2&PID_003A")) != -1 )
		{
			_chipFamily = MX25;
		}
		else if ( devPath.Find(_T("VID_15A2&PID_0028")) != -1 )
		{
			_chipFamily = MX32;
		}
		else if ( devPath.Find(_T("VID_15A2&PID_0030")) != -1 )
		{
			_chipFamily = MX35;
		}
		else if ( devPath.Find(_T("VID_15A2&PID_002C")) != -1 )
		{
			_chipFamily = MX51; // can't tell MX51 TO1 from MX37 by USB IDs
			_chipFamily = MX37; // assume MX37
		}
		else if ( devPath.Find(_T("VID_15A2&PID_0041")) != -1 )
		{
			_chipFamily = MX51;
		}
		else if ( devPath.Find(_T("VID_0425&PID_21FF")) != -1 )
		{
			_chipFamily = MX27; // can't tell MX27 from MX31 by USB IDs
			_chipFamily = MX31; // assume MX31
		}
    }

	return _chipFamily;
}

//-------------------------------------------------------------------------------------		
// Function to get HAB_TYPE value
//
// @return
//		HabEnabled: if is prodction
//		HabDisabled: if is development/disable
//-------------------------------------------------------------------------------------
MxRomDevice::HAB_t MxRomDevice::GetHABType(ChipFamily_t chipType)
{
	short header = ROM_KERNEL_CMD_WR_MEM;
	char format = 32;
	int dataSize = 0;
	int data = 0;
	char pointer = 0;
	int address = 0;

	if ( chipType == MX51 )
	{
		address = 0x1FFEA000;
	}
	else
	{
		address = 0xFFFFFFFF;
	}

	long command[4] = { 0 };

	command[0] = (  ((address  & 0x00FF0000) << 8) 
		          | ((address  & 0xFF000000) >> 8) 
		          |  (header   & 0x0000FFFF) );

	command[1] = (   (dataSize & 0xFF000000)
		          | ((format   & 0x000000FF) << 16)
		          | ((address  & 0x000000FF) <<  8)
		          | ((address  & 0x0000FF00) >>  8 ));

	command[2] = (   (data     & 0xFF000000)
		          | ((dataSize & 0x000000FF) << 16)
		          |  (dataSize & 0x0000FF00)
		          | ((dataSize & 0x00FF0000) >> 16));

	command[3] = (  ((pointer  & 0x000000FF) << 24)
		          | ((data     & 0x00FF0000) >> 16) 
		          |  (data     & 0x0000FF00)
		          | ((data     & 0x000000FF) << 16));

	//Send write Command to USB
	if ( !WriteToDevice((unsigned char *)command, 16) )
	{
		return HabDisabled;
	}

	//read the ACK
	unsigned char cmdAck[4] = { 0 };
	if(!ReadFromDevice(cmdAck, 4))
	{
		return HabDisabled;
	}

	// get response of HAB type
	TRACE("GetHABType(): Receive ack:%x\n", *(unsigned int *)cmdAck);
	if (*(unsigned int *)cmdAck == HabEnabled)
	{
		TRACE("GetHABType(): this is production type\n");
		// production parts
		if ( (_chipFamily == MX35) || (_chipFamily == MX37) || (_chipFamily == MX51) )
		{
			unsigned char writeAck[4] = { 0 };
			ReadFromDevice(writeAck,4);

			TRACE("GetHABType(): Receive ack2:%x\n", *(unsigned int *)writeAck);
		}

		return HabEnabled; 

	}
	else if (*(unsigned int *)cmdAck == HabDisabled)
	{
		TRACE("GetHABType(): this is develop/disable type\n");

		unsigned char writeAck[4] = { 0 };
		ReadFromDevice(writeAck,4);

		TRACE("GetHABType(): Receive ack2:%x\n", *(unsigned int *)writeAck);

		return HabDisabled;	
	}

	return HabDisabled;
}

BOOL MxRomDevice::InitRomVersion(ChipFamily_t chipType, RomVersion& romVersion, MxAddress& defaultAddrs) const
{
	BOOL success = TRUE;

	switch ( chipType )
	{
		case MX25:
			// IIM PREV register = 0xF8
			// IIM SREV register = 0x0 -- TO 1.0
			// IIM SREV register = 0x1 -- TO 1.1
			romVersion = RomVersion(1,1);
			defaultAddrs = MX25Addrs;
			break;

		case MX27: 
			// Read register 0x10027800 from bits 12-27 to get the part number 
			// Read register 0x10027800 from bits 28-31 to get the revision info
			// Bits 28-31 = 0x0 -- TO 1.0
			// Bits 28-31 = 0x1 -- TO 2.0
			// Bits 28-31 = 0x2 -- TO 2.1
			romVersion = RomVersion(2,1);
			defaultAddrs = MX27Addrs;
			break;

		case MX31:
			// IIM PREV register = 0x8
			// Look at the upper nibble of SREV reg (bits 4-8) to get  the revision info
			// IIM SREV register = 0x00 -- TO 1.0
			// IIM SREV register = 0x1x -- TO 1.1
			// IIM SREV register = 0x2x -- TO 2.0
			romVersion = RomVersion(2,0);
			defaultAddrs = MX31Addrs;
			break;

		case MX35:
			// IIM PREV register = 0x82
			// Get Silicon Revision information from ROM code (Read offset 0x40)
			// ROM Offset 0x40 = 0x1 -- TO 1.0
			// ROM Offset 0x40 = 0x2 -- TO 2.0
			romVersion = RomVersion(2,0);
			defaultAddrs = MX35Addrs;
			break;

		case MX37:
			// IIM PREV register = 0xA2
			// IIM SREV register = 0x0   -- TO 1.0
			// IIM SREV register = 0x10 -- TO 1.1
			romVersion = RomVersion(1,1);
			defaultAddrs = MX37Addrs;
			break;
	 
		case MX51:
			// IIM PREV register = 0xF2
			// Get Silicon Revision information from ROM code (Read offset 0x48)
			// ROM Offset 0x48 = 0x1 -- TO 1.0
			// ROM Offset 0x48 = 0x2 -- TO 1.1
			// ROM Offset 0x48 = 0x10 -- TO 2.0
			// ROM Offset 0x48 = 0x20 -- TO 3.0
			romVersion = RomVersion(2,0);
			defaultAddrs = MX51_TO2Addrs;
			break;
		
		default:
			success = FALSE;
			break;
	}

	return success;
}

BOOL MxRomDevice::InitMemoryDevice(CString filename)
{
	USES_CONVERSION;

	CFile scriptFile;
	CFileException fileException;
	if( !scriptFile.Open(filename, CFile::modeRead, &fileException) )
	{
		TRACE( _T("Can't open file %s, error = %u\n"), filename, fileException.m_cause );
	}

	CStringT<char,StrTraitMFC<char> > cmdString;
	scriptFile.Read(cmdString.GetBufferSetLength(scriptFile.GetLength()), scriptFile.GetLength());
	cmdString.ReleaseBuffer();

	
	XNode script;
	if ( script.Load(A2T(cmdString)) != NULL )
	{
		XNodes cmds = script.GetChilds(_T("CMD"));
		XNodes::iterator cmd = cmds.begin();
		for ( ; cmd != cmds.end(); ++cmd )
		{
			MemoryInitCommand* pCmd = (MemoryInitCommand*)(*cmd);
			if ( !WriteMemory(pCmd->GetAddress(), pCmd->GetData(), pCmd->GetFormat()) )
			{
				TRACE("In InitMemoryDevice(): write memory failed\n");
				return FALSE;
			}
		}
	}

	// set the communication mode through write memory interace
	if ( !WriteMemory(_defaultAddress.MemoryStart, ChannelType_USB, 32) )
	{
		TRACE(CString("Write channel type to memory failed!\n"));
		return ERROR_COMMAND;
	}

	return TRUE;
}

BOOL MxRomDevice::Reset()
{
	// If we do have to reset, can we use device commands? Seems there is both a ROM and RKL command to reset device.
	// Driver command seems heavy handed?
	// Send reset command to device.
	if(!DeviceIoControl(IOCTL_IMXDEVICE_RESET_DEVICE))
	{
		TRACE("Error: Device reset failed.\n");
		return ERROR_COMMAND;
	}

	return ERROR_SUCCESS;
}

// Write the register of i.MX
BOOL MxRomDevice::WriteMemory(UINT address, UINT data, UINT format)
{
	BOOL status = FALSE;

	const short header = ROM_KERNEL_CMD_WR_MEM;
	const UINT dataSize = 0;
	const char pointer = 0;

	long command[4] = { 0 };

	command[0] = (  ((address  & 0x00FF0000) << 8) 
		          | ((address  & 0xFF000000) >> 8) 
		          |  (header   & 0x0000FFFF) );

	command[1] = (   (dataSize & 0xFF000000)
		          | ((format   & 0x000000FF) << 16)
		          | ((address  & 0x000000FF) <<  8)
		          | ((address  & 0x0000FF00) >>  8 ));

	command[2] = (   (data     & 0xFF000000)
		          | ((dataSize & 0x000000FF) << 16)
		          |  (dataSize & 0x0000FF00)
		          | ((dataSize & 0x00FF0000) >> 16));

	command[3] = (  ((pointer  & 0x000000FF) << 24)
		          | ((data     & 0x00FF0000) >> 16) 
		          |  (data     & 0x0000FF00)
		          | ((data     & 0x000000FF) << 16));

	//Send write Command to USB
	if ( !WriteToDevice((unsigned char *)command, 16) )
	{
		return FALSE;
	}

	//read the ACK
	unsigned char cmdAck[4] = { 0 };
	if(!ReadFromDevice(cmdAck, 4))
	{
		return FALSE;
	}

	// get response of HAB type
	//TRACE("WriteMemory(): Receive ack:%x\n", *(unsigned int *)cmdAck);
	if (*(unsigned int *)cmdAck == HabEnabled)
	{
		// Validate production address
		if ( ! ValidAddress(address) )
			return FALSE;
	}

	unsigned char writeAck[4] = { 0 };
	if(!ReadFromDevice(writeAck, 4))
	{
		return FALSE;
	}

	if (*(unsigned int *)writeAck == ROM_WRITE_ACK)
	{
		return TRUE;
	}
	else
	{
		TRACE("WriteMemory(): Invalid write ack: 0x%x\n", *(unsigned int *)writeAck);
		return FALSE; 
	}
}
BOOL MxRomDevice::ValidAddress(const UINT address) const
{
	BOOL status = FALSE;

	switch ( _chipFamily )
	{
		case MX31:
		case MX32:
			if (   ((address <= MX31_NFCend)	&& (address >= MX31_NFCstart)) 
				|| ((address <= MX31_WEIMend)	&& (address >= MX31_WEIMstart))
				|| ((address <= MX31_MemoryEnd)	&& (address >= MX31_MemoryStart))
				|| ((address <= MX31_ESDCTLend)	&& (address >= MX31_ESDCTLstart))
				|| ((address <= MX31_CCMend)	&& (address >= MX31_CCMstart) && (address != MX31_CCM_CGR0)) )
			{ 
				status = TRUE;
				TRACE("Matching the MX31 address region\n");
			}
			break;

		case MX27:
			if (   ((address <= MX27_NFCend)	&& (address >= MX27_NFCstart)) 
				|| ((address <= MX27_WEIMend)	&& (address >= MX27_WEIMstart))
				|| ((address <= MX27_MemoryEnd)	&& (address >= MX27_MemoryStart))
				|| ((address <= MX27_ESDCTLend)	&& (address >= MX27_ESDCTLstart))
				|| ((address <= MX27_CCMend)	&& (address >= MX27_CCMstart) && (address != MX27_CCM_CGR0)) )
			{
				status = TRUE;
				TRACE("Matching the MX27 address region\n");
			}
			break;

		case MX35:
			if (   ((address <= MX35_WEIM_CS2_END)    && (address >= MX35_SDRAM_START))
				|| ((address <= MX35_IRAM_FREE_END)   && (address >= MX35_IRAM_FREE_START))
				|| ((address <= MX35_NFC_END)         && (address >= MX35_NFC_START))
				|| ((address <= MX35_SDRAM_CTL_END)   && (address >= MX35_SDRAM_CTL_START))
				|| ((address <= MX35_WEIM_END)        && (address >= MX35_WEIM_START)) )
			{
				status = TRUE;
				TRACE("Matching the MX35 address region\n");
			}
			break;

		case MX37:
			if (   ((address <= MX37_WEIM_CS2_END)    && (address >= MX37_SDRAM_START))
				|| ((address <= MX37_IRAM_FREE_END)   && (address >= MX37_IRAM_FREE_START))
				|| ((address <= MX37_NFC_END)         && (address >= MX37_NFC_START))
				|| ((address <= MX37_SDRAM_CTL_END)   && (address >= MX37_SDRAM_CTL_START))
				|| ((address <= MX37_WEIM_END)        && (address >= MX37_WEIM_START)))
			{
				status = TRUE;
				TRACE("Matching the MX37 address region\n");
			}
			break;

		case MX51:
			if (   ((address <= MX51_WEIM_CS2_END)    && (address >= MX51_SDRAM_START))
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
			TRACE("Invalid address.\n");
			status = FALSE;
			break;
	}

	return status;
}

BOOL MxRomDevice::Jump2Rak()
{
	BOOL status = FALSE;

	const short header = ROM_KERNEL_CMD_GET_STAT;
	const UINT address = 0;
	const UINT data = 0;
	const UINT dataSize = 0;
	const char format = 0;
	const char pointer = 0;

	long command[4] = { 0 };

	command[0] = (  ((address  & 0x00FF0000) << 8) 
		          | ((address  & 0xFF000000) >> 8) 
		          |  (header   & 0x0000FFFF) );

	command[1] = (   (dataSize & 0xFF000000)
		          | ((format   & 0x000000FF) << 16)
		          | ((address  & 0x000000FF) <<  8)
		          | ((address  & 0x0000FF00) >>  8 ));

	command[2] = (   (data     & 0xFF000000)
		          | ((dataSize & 0x000000FF) << 16)
		          |  (dataSize & 0x0000FF00)
		          | ((dataSize & 0x00FF0000) >> 16));

	command[3] = (  ((pointer  & 0x000000FF) << 24)
		          | ((data     & 0x00FF0000) >> 16) 
		          |  (data     & 0x0000FF00)
		          | ((data     & 0x000000FF) << 16));

	//Send write Command to USB
	if ( !WriteToDevice((unsigned char *)command, 16) )
	{
		return FALSE;
	}

	//read the ACK
	unsigned char cmdAck[4] = { 0 };
	if(!ReadFromDevice(cmdAck, 4))
	{
		return FALSE;
	}

	// 1st CHECK: If first ACK == 0x88888888 then 1st check is successful.
	// TRACE(">> Jump2 Ramkernel result: 0x%x\n", *(unsigned int *)cmdAck);
	if (*(unsigned int *)cmdAck != ROM_STATUS_ACK)
	{
		return FALSE;
	}

	// 2nd CHECK: For MX35, MX37 or MX51 HAB-Enabled devices
	//				If can Read second ack AND secondACK == 0xF0F0F0F0
	//                FAILED JUMP 2 RAM KERNEL
	if ( (_chipFamily == MX35 || _chipFamily == MX37 || _chipFamily == MX51) && _habType == HabEnabled )
	{
		unsigned char cmdAck2[4] = { 0 };
		if(ReadFromDevice(cmdAck2, 4))
		{
			// TRACE(">> Jump2 Ramkernel result(2): 0x%x\n", *(unsigned int *)cmdAck2);
			if (*(unsigned int *)cmdAck2 == ROM_STATUS_ACK2)
			{
				return FALSE;
			}
		}
	}

	TRACE("*********Jump to Ramkernel successfully!**********\r\n");
	return TRUE;
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
void MxRomDevice::PackRklCommand(unsigned char *cmd, unsigned short cmdId, unsigned long addr, 
								  unsigned long param, unsigned long param1)
{
	cmd[0]  = (unsigned char)(RAM_KERNEL_CMD_HEADER >> 8);
	cmd[1]  = (unsigned char)(RAM_KERNEL_CMD_HEADER & 0x00ff);
	cmd[2]  = (unsigned char)(cmdId >> 8);
	cmd[3]  = (unsigned char) cmdId;
	cmd[4]  = (unsigned char)(addr >> 24);
	cmd[5]  = (unsigned char)(addr >> 16);
	cmd[6]  = (unsigned char)(addr >> 8);
	cmd[7]  = (unsigned char) addr;
	cmd[8]  = (unsigned char)(param >> 24);
	cmd[9]  = (unsigned char)(param >> 16);
	cmd[10] = (unsigned char)(param >> 8);
	cmd[11] = (unsigned char) param;
	cmd[12] = (unsigned char)(param1 >> 24);
	cmd[13] = (unsigned char)(param1 >> 16);
	cmd[14] = (unsigned char)(param1 >> 8);
	cmd[15] = (unsigned char) param1;
	return;
}

//-------------------------------------------------------------------------------------		
// Function to unpack the response
//
// @response  response read from device
//
// @return
//-------------------------------------------------------------------------------------		
struct Response MxRomDevice::UnPackRklResponse(unsigned char *resBuf)
{
	struct Response res;

	res.ack  =  (unsigned short)(((unsigned short)resBuf[0] << 8) | resBuf[1]);
	res.csum =  (unsigned short)(((unsigned short)resBuf[2] << 8) | resBuf[3]);
	res.len  = ((unsigned long)resBuf[4] << 24) | ((unsigned long)resBuf[5] << 16) |
			   ((unsigned long)resBuf[6] << 8) | ((unsigned long)resBuf[7]);

	return res;
}
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
int MxRomDevice::GetRKLVersion(CString& fmodel, int& len, int& mxType)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res;
	BOOL status;
		
	// pack command requeset
	PackRklCommand(command, RAM_KERNEL_CMD_GETVER, 0, 0, 0);

	// send command to remote device
	if (!WriteToDevice(command, RAM_KERNEL_CMD_SIZE))
	{
		TRACE("atk_common_getver(): failed to send to device\n");
		return INVALID_CHANNEL;
	}
		
	if (!ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE))
	{
		TRACE("atk_common_getver(): failed to read bootstrap from device\n");
		return INVALID_CHANNEL;
	}
	
	TRACE("atk_common_getver(): retBuf[0]: %d\n", retBuf[0]);
	// check if is bootstrap
	PUINT pReturn = (PUINT)&retBuf[0];
	if (*pReturn == HabEnabled || *pReturn == HabDisabled)
	{
		len = 0;
		return RET_SUCCESS;
	}
		
	// unpack the response
	res = UnPackRklResponse(retBuf);

	if (res.ack == RET_SUCCESS) {
		
		TRACE("atk_common_getver(): chip id: %x\n", res.csum);

		unsigned char model[MAX_MODEL_LEN] = {0};
		status = ReadFromDevice(model, res.len);
		if (!status) {
			return INVALID_CHANNEL;
		}
		len = res.len;
		mxType = res.csum;
		fmodel = model;
		fmodel.AppendChar(_T('\0'));
		TRACE("atk_common_getver(): get version: %s\n", fmodel);
	} else {
		// if response is not ok, return failed
		TRACE("atk_common_getver(): get response: %d\n", res.ack);
	}
	
	return -res.ack;
}

#define FLASH_HEADER_SIZE	0x20
#define ROM_TRANSFER_SIZE	0x400

BOOL MxRomDevice::DownloadImage(UINT address, MemorySection loadSection, MemorySection setSection, BOOL HasFlashHeader, const StFwComponent& fwComponent, Device::UI_Callback callbackFn)
{
	int counter = 0;
	int bytePerCommand = 0;
	const unsigned char* const pBuffer = fwComponent.GetDataPtr();
	const size_t dataSize = fwComponent.size();

	//Here we must transfer virtual address to physical address before downloading ram kernel.
	int PhyRAMAddr4KRL = _defaultAddress.MemoryStart | (address & (~0xF0000000));

	uint32_t byteIndex, numBytesToWrite = 0;
	for ( byteIndex = 0; byteIndex < dataSize; byteIndex += numBytesToWrite )
	{
		// Get some data
		numBytesToWrite = min(MAX_SIZE_PER_DOWNLOAD_COMMAND, dataSize - byteIndex);

		if (!SendCommand2RoK(PhyRAMAddr4KRL + byteIndex, numBytesToWrite, loadSection))
		{
			TRACE(_T("DownloadImage(): SendCommand2RoK(0x%X, 0x%X, 0x%X) failed.\n"), PhyRAMAddr4KRL + byteIndex, numBytesToWrite, loadSection);
			return FALSE;
		}
		else
		{
			Sleep(10);
			if(!TransData(numBytesToWrite, pBuffer + byteIndex))
			{
				TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X) failed.\n"), numBytesToWrite, pBuffer + byteIndex);
				return FALSE;
			}
		}
	}

	int FlashHdrAddr;
	const unsigned char * pHeaderData = NULL;

	//transfer length of ROM_TRANSFER_SIZE is a must to ROM code.
	unsigned char FlashHdr[ROM_TRANSFER_SIZE] = { 0 };
	
	// Just use the front of the data buffer if the data includes the FlashHeader
	if( HasFlashHeader )
	{
		FlashHdrAddr = PhyRAMAddr4KRL;
		pHeaderData = pBuffer;
	}
	else
	{
		// Otherwise, create a header and append the data
		
		//Copy image data with an offset of FLASH_HEADER_SIZE to the temp buffer.
		memcpy(FlashHdr + FLASH_HEADER_SIZE, pBuffer, ROM_TRANSFER_SIZE - FLASH_HEADER_SIZE);
		
		//We should write actual image address to the first dword of flash header.
		((int *)FlashHdr)[0] = PhyRAMAddr4KRL;

		FlashHdrAddr = PhyRAMAddr4KRL - FLASH_HEADER_SIZE;
		pHeaderData = (const unsigned char *)FlashHdr;
	}

	//Set execute address.
	if ( !SendCommand2RoK(FlashHdrAddr, ROM_TRANSFER_SIZE, setSection) )
	{
		TRACE(_T("DownloadImage(): SendCommand2RoK(0x%X, 0x%X, 0x%X) failed.\n"), FlashHdrAddr, ROM_TRANSFER_SIZE, setSection);
		return FALSE;
	}
	else
	{
		Sleep(10);
		if(!TransData(ROM_TRANSFER_SIZE, pHeaderData))
		{
			TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X) failed.\n"), ROM_TRANSFER_SIZE, pHeaderData);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL MxRomDevice::Jump()
{
	// Send the complete command to the device 
	if ( !Jump2Rak() )
	{
		TRACE(_T("DownloadImage(): Jump2Rak() failed.\n"));
		return FALSE;
	}
/*  I can't get GetRKLVersion() to work. What am I doing wrong?	
	USB_CloseDevice();
	Sleep(500);
	if ( USB_OpenDevice() )
	{
		int len = 0, type = 0;
		CString model;
		if ( GetRKLVersion(model, len, type) == ERROR_SUCCESS )
		{
			if ( len == 0 && type == 0 )
				TRACE(_T("DownloadImage(): BootStrap mode.\n"));
			else
				TRACE(_T("DownloadImage(): RamKernel mode.\n"));
		}
		TRACE(_T("DownloadImage(): ReOpened Device handles.\n"));
	}
*/
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
	if (pCommandAck[0] != HabDisabled && 
		pCommandAck[0] != HabEnabled ) 
	{
		free(pCommandAck);
		return FALSE;	
	}

	free(pCommandAck); 
	return TRUE;			


}

#define MINUM_TRANSFER_SIZE 0x20

BOOL MxRomDevice::TransData(UINT byteCount, const unsigned char * pBuf)
{
	//	BOOL status = FALSE;
	const unsigned char * pBuffer = pBuf;
	UINT downloadPerOnce = WR_BUF_MAX_SIZE;

	for ( ; byteCount >= downloadPerOnce; byteCount -= downloadPerOnce, pBuffer += downloadPerOnce )
	{
		if ( !WriteToDevice(pBuffer, downloadPerOnce) )
			return FALSE;
	}
	if ( ( _chipFamily == MX31 || _chipFamily == MX32 ) && byteCount > 0)
	{

			TRACE("Lat will Transfer Size: %d\n", byteCount);
			if (!WriteToDevice(pBuffer, byteCount))
				return FALSE;

			byteCount = 0;
	}
	else
	{
		UINT uintMaxPacketSize0 = _MaxPacketSize.get();

		for ( ; byteCount > uintMaxPacketSize0; byteCount -= uintMaxPacketSize0, pBuffer += uintMaxPacketSize0 )
		{
				if (!WriteToDevice(pBuffer, uintMaxPacketSize0))
					return FALSE;
				//TRACE("Transfer Size: %d\n", uintMaxPacketSize0);
		}

		for ( ; byteCount >= MINUM_TRANSFER_SIZE; byteCount -= MINUM_TRANSFER_SIZE, pBuffer += MINUM_TRANSFER_SIZE )
		{
				if (!WriteToDevice(pBuffer, MINUM_TRANSFER_SIZE))
					return FALSE;
				//TRACE("Transfer Size: %d\n", MINUM_TRANSFER_SIZE);
		}
	}
	
	if (0 != byteCount)
	{
		if (!WriteToDevice(pBuffer, byteCount))
			return FALSE;
	}

	return TRUE;
}

BOOL MxRomDevice::DeviceIoControl(DWORD controlCode, PVOID pRequest)
{
	BOOL retValue = TRUE;
	DWORD totalSize = 0;
	BOOL closeOnExit = FALSE;

	switch (controlCode)
	{
	case IOCTL_IMXDEVICE_GET_CONFIG_DESCRIPTOR:
		totalSize = sizeof(USB_CONFIGURATION_DESCRIPTOR);
		break;
	case IOCTL_IMXDEVICE_RESET_DEVICE:
		break;
	case IOCTL_IMXDEVICE_RESET_PIPE:
		break;
	case IOCTL_IMXDEVICE_GET_DEVICE_DESCRIPTOR:
		totalSize = sizeof(USB_DEVICE_DESCRIPTOR);
		break;
	default:
		break;
	}

	// Open the device
	if ( _hDevice == INVALID_HANDLE_VALUE )
	{
		_hDevice = CreateFile(_path.get(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if( _hDevice == INVALID_HANDLE_VALUE )
		{
			TRACE(_T("MxRomDevice::DeviceIoControl() CreateFile() ERROR = %d.\n"), GetLastError());
			return FALSE;
		}
		closeOnExit = TRUE;
	}

	// Send the DeviceIo command
	DWORD dwBytesReturned = 0;
	retValue = ::DeviceIoControl(_hDevice, controlCode, pRequest, totalSize, pRequest, totalSize, &dwBytesReturned, NULL);
	if (!retValue)
	{
		TRACE(_T("MxRomDevice::DeviceIoControl() DeviceIoControl() ERROR = %d.\n"), GetLastError());
		retValue = FALSE;
	}

	if ( dwBytesReturned != totalSize )
	{
		TRACE(_T("MxRomDevice::DeviceIoControl() DeviceIoControl() Only returned %d of %d requested bytes.\n"), dwBytesReturned, totalSize);
		retValue = FALSE;
	}

	// Close the device
	if ( closeOnExit )
	{
		if ( !CloseHandle(_hDevice) )
		{
			TRACE(_T("MxRomDevice::DeviceIoControl() CloseHandle() ERROR = %d.\n"), GetLastError());
			retValue = FALSE;
		}
	}

	return retValue;
}

BOOL MxRomDevice::OpenUSBHandle(HANDLE* pHandle, CString pipePath)
{
	//TRACE(_T("complete pipe handle name is (%s)\n"), pipePath);

	*pHandle = CreateFile(pipePath,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (pHandle== INVALID_HANDLE_VALUE) {
		TRACE(_T("MxRomDevice::OpenUSBHandle() Failed to open (%s) = %d"), pipePath, GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL MxRomDevice::USB_OpenDevice(void)
{
	CString pipePath = _path.get();

	// Get handles for write operation
	if( !OpenUSBHandle(&_hDevice, pipePath) ||
		!OpenUSBHandle(&_hWrite, pipePath+_T("\\PIPE00")) ||
		!OpenUSBHandle(&_hRead, pipePath+_T("\\PIPE01"))
		)
	{
		return FALSE;
	}

	sHandle[_hubIndex.get()][0] = _hWrite;
	sHandle[_hubIndex.get()][1] = _hRead;
	//	_MaxPacketSize0 = _maxPacketSize.get();

	return TRUE;
}

BOOL MxRomDevice::USB_CloseDevice(void)
{

	//if(!DeviceIoControl(IOCTL_IMXDEVICE_RESET_DEVICE, NULL))
	//	return FALSE;

	if (!CloseHandle(_hRead) || 
		!CloseHandle(_hWrite) || 
		!CloseHandle(_hDevice))
	{
		TRACE(_T("MxRomDevice::USB_CloseDevice() CloseHandle() ERROR = %d.\n"), GetLastError());
		return FALSE;
	}
	
	_hRead = INVALID_HANDLE_VALUE;
	_hWrite = INVALID_HANDLE_VALUE;
	_hDevice = INVALID_HANDLE_VALUE;
	
	return TRUE;
}

BOOL MxRomDevice::WriteToDevice(const unsigned char *buf, UINT count)
{
	int    nBytesWrite; // for bytes actually written
	if(sHandle[_hubIndex.get()][0] != _hWrite)
		TRACE(_T("Different handle found: _hWrite = 0x%x.\r\n"), _hWrite);
	if( !WriteFile(_hWrite, buf, count, (PULONG) &nBytesWrite, NULL) )
	{
		TRACE(_T("MxRomDevice::WriteToDevice() Error writing to device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		/*USB_CloseDevice();
		TRACE(_T("MxRomDevice::USB device closed.\r\n"));
		USB_OpenDevice();
		TRACE(_T("MxRomDevice::USB device re-opened.\r\n"));
		if(!DeviceIoControl(IOCTL_IMXDEVICE_RESET_DEVICE, NULL))
			return FALSE;
		USB_CloseDevice();
		TRACE(_T("MxRomDevice::USB device closed.\r\n"));
		USB_OpenDevice();
		TRACE(_T("MxRomDevice::USB device re-opened.\r\n"));
		if( !WriteFile(_hWrite, buf, count, (PULONG) &nBytesWrite, NULL) )
			return FALSE;*/
	}
	return TRUE;	
}

BOOL MxRomDevice::ReadFromDevice(PUCHAR buf, UINT count)
{
	int    nBytesRead; // for bytes actually read
	if(sHandle[_hubIndex.get()][0] != _hWrite)
		TRACE(_T("Different handle found: _hRead = 0x%x.\r\n"), _hRead);

	if( !ReadFile(_hRead, buf, count, (PULONG) &nBytesRead, NULL) )
	{
		TRACE(_T("MxRomDevice::ReadFromDevice() Error reading from device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		return FALSE;
	}

	return TRUE;
}

#define HWC_OFFSET					0x1010
#define CSF_OFFSET					0x1200
#define EXE_OFFSET					0x4000
#define IMG_OFFSET				   0x10000

MxRomDevice::MxAddress MxRomDevice::MX25Addrs(
	/* #define MX25_MEMORY_START_ADDR  */	0x80000000,
	/* #define MX25_MEMORY_END_ADDR    */	0x8FFFFFFF,
	/* #define MX25_DEF_HWC_ADDR	   */   0x80000000 + HWC_OFFSET,
	/* #define MX25_DEF_CSF_ADDR	   */	0x80000000 + CSF_OFFSET,
	/* #define MX25_DEF_RKL_ADDR	   */	0x80000000 + EXE_OFFSET,
	/* #define MX25_DEF_DOWNLOAD_ADDR  */	0x80000000 + EXE_OFFSET,
	/* #define MX25_DEF_NOR_FLASH_ADDR */	0xA0000000,
	/* #define MX25_INT_RAM_START	   */	0x78000000,
	/* #define MX25_INT_RAM_END		   */ 	0x7801FFFF,
	/* #define MX25_IMAGE_START_ADDR   */ 	0x80000000 + IMG_OFFSET
	);

	/************ MX27 Default Address Setting *******************************/
MxRomDevice::MxAddress MxRomDevice::MX27Addrs(
	/* #define MX27_MEMORY_START_ADDR	*/	0xA0000000,
	/* #define MX27_MEMORY_END_ADDR		*/	0xD8000FFF,
	/* #define MX27_DEF_HWC_ADDR		*/  0xA0000000 + HWC_OFFSET,
	/* #define MX27_DEF_CSF_ADDR		*/	0xA0000000 + CSF_OFFSET,
	/* #define MX27_DEF_RKL_ADDR		*/	0xA0000000 + EXE_OFFSET,
	/* #define MX27_DEF_DOWNLOAD_ADDR	*/	0xA0000000 + EXE_OFFSET,
	/* #define MX27_DEF_NOR_FLASH_ADDR	*/	0xC0000000,
	/* #define MX27_INT_RAM_START		*/	0xFFFF4C00,
	/* #define MX27_INT_RAM_END			*/ 	0xFFFFFFFF,
	/* #define MX27_IMAGE_START_ADDR	*/ 	0xA0000000 + IMG_OFFSET
	);

	/************ MX31 Default Address Setting *******************************/
MxRomDevice::MxAddress MxRomDevice::MX31Addrs(
	/* #define MX31_MEMORY_START_ADDR	*/	0x80000000,
	/* #define MX31_MEMORY_END_ADDR		*/	0xB8000000,
	/* #define MX31_DEF_HWC_ADDR		*/  0x80000000 + HWC_OFFSET,
	/* #define MX31_DEF_CSF_ADDR		*/	0x80000000 + CSF_OFFSET,
	/* #define MX31_DEF_RKL_ADDR		*/	0x80000000 + EXE_OFFSET,
	/* #define MX31_DEF_DOWNLOAD_ADDR	*/	0x80000000 + EXE_OFFSET,
	/* #define MX31_DEF_NOR_FLASH_ADDR	*/	0xA0000000,
	/* #define MX31_INT_RAM_START		*/	0x1FFFC000,
	/* #define MX31_INT_RAM_END			*/ 	0x1FFFFFFF,
	/* #define MX31_IMAGE_START_ADDR	*/ 	0x80000000 + IMG_OFFSET
	);

	/************ MX35 Default Address Setting *******************************/
MxRomDevice::MxAddress MxRomDevice::MX35Addrs(
	/* #define MX35_MEMORY_START_ADDR	*/	0x80000000,
	/* #define MX35_MEMORY_END_ADDR		*/	0x8FFFFFFF,
	/* #define MX35_DEF_HWC_ADDR		*/  0x80000000 + HWC_OFFSET,
	/* #define MX35_DEF_CSF_ADDR		*/	0x80000000 + CSF_OFFSET,
	/* #define MX35_DEF_RKL_ADDR		*/	0x80000000 + EXE_OFFSET,
	/* #define MX35_DEF_DOWNLOAD_ADDR	*/	0x80000000 + EXE_OFFSET,
	/* #define MX35_DEF_NOR_FLASH_ADDR	*/	0xA0000000,
	/* #define MX35_INT_RAM_START		*/	0x10000000,
	/* #define MX35_INT_RAM_END			*/ 	0x1001FFFF,
	/* #define MX35_IMAGE_START_ADDR	*/ 	0x80000000 + IMG_OFFSET
	);

	/************ MX37 Default Address Setting *******************************/
MxRomDevice::MxAddress MxRomDevice::MX37Addrs(
	/* #define MX37_MEMORY_START_ADDR	*/	0x40000000,
	/* #define MX37_MEMORY_END_ADDR		*/	0x4FFFFFFF,
	/* #define MX37_DEF_HWC_ADDR		*/  0x40000000 + HWC_OFFSET,
	/* #define MX37_DEF_CSF_ADDR		*/	0x40000000 + CSF_OFFSET,
	/* #define MX37_DEF_RKL_ADDR		*/	0x40000000 + EXE_OFFSET,
	/* #define MX37_DEF_DOWNLOAD_ADDR	*/	0x40000000 + EXE_OFFSET,
	/* #define MX37_DEF_NOR_FLASH_ADDR	*/	0x60000000,
	/* #define MX37_INT_RAM_START		*/	0x10000000,
	/* #define MX37_INT_RAM_END			*/ 	0x10011FFF,
	/* #define MX37_IMAGE_START_ADDR	*/ 	0x40000000 + IMG_OFFSET
	);

	/************ MX51 Default Address Setting *******************************/
MxRomDevice::MxAddress MxRomDevice::MX51Addrs(
	/* #define MX51_MEMORY_START_ADDR	*/	0x90000000,
	/* #define MX51_MEMORY_END_ADDR		*/	0x9FFFFFFF,
	/* #define MX51_DEF_HWC_ADDR		*/	0x90000000 + HWC_OFFSET,
	/* #define MX51_DEF_CSF_ADDR		*/	0x90000000 + CSF_OFFSET,
	/* #define MX51_DEF_RKL_ADDR		*/	0x90000000 + EXE_OFFSET,
	/* #define MX51_DEF_DOWNLOAD_ADDR	*/	0x90000000 + EXE_OFFSET,
	/* #define MX51_DEF_NOR_FLASH_ADDR	*/	0xB0000000,
	/* #define MX51_INT_RAM_START		*/	0x1FFE8000,
	/* #define MX51_INT_RAM_END			*/	0x1FFFFFFF,
	/* #define MX51_IMAGE_START_ADDR	*/	0x90000000 + IMG_OFFSET
	);

	/************ MX51_TO2 Default Address Setting *******************************/
MxRomDevice::MxAddress MxRomDevice::MX51_TO2Addrs(
	/* #define MX51_MEMORY_START_ADDR	*/	0x90000000,
	/* #define MX51_MEMORY_END_ADDR		*/	0x9FFFFFFF,
	/* #define MX51_DEF_HWC_ADDR		*/	0x90000000 + HWC_OFFSET,
	/* #define MX51_DEF_CSF_ADDR		*/	0x90000000 + CSF_OFFSET,
	/* #define MX51_DEF_RKL_ADDR		*/	0x90000000 + EXE_OFFSET,
	/* #define MX51_DEF_DOWNLOAD_ADDR	*/	0x90000000 + EXE_OFFSET,
	/* #define MX51_DEF_NOR_FLASH_ADDR	*/	0xB0000000,
	/* #define MX51_INT_RAM_START	    */  0x1FFE0000,
	/* #define MX51_INT_RAM_END		    */  0x1FFFFFFF,
	/* #define MX51_IMAGE_START_ADDR	*/	0x90000000 + IMG_OFFSET
	);
