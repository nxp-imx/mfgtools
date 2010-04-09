#pragma once
#include "stdafx.h"
#include "MxRomDevice.h"
#include "DeviceManager.h"
#include "Libs/WDK/usb100.h"
#include <sys/stat.h>

#include "../../Drivers/iMX_BulkIO_Driver/sys/public.h"

static BOOL SyncAllDevFlag = FALSE;
typedef struct _ImgDownloadStatus
{
	CString Path;//The path of a device, please notice the path is fixed for a port even if the device attched to the port is different.
	BOOL Status;// If current device has finished downloading work, then mark status flag as true, or false;
} ImgDownloadStatus, * PImgDownloadStatus;

static std::list<ImgDownloadStatus> ImgDwdSts;

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

	int len = 0, type = 0;
	CString model;

	//We MUST check current device status, or a device will be treated as a new i.mx device 
	//after RAM kernel is downloaded and running since both bootstrap mode and RAM kernel mode
	//use the same USB driver under ATK mode. 
	if ( GetRKLVersion(model, len, type) == ERROR_SUCCESS )
	{
		if (!( len == 0 && type == 0 ))
			//It isn't bootstrap mode, which means it is not a new device, but a device 
			//running under RAM kernel, so we just quit.
			return;
	}

	ImgDownloadStatus CurImgDwdSts;

	CurImgDwdSts.Path = _path.get();
	CurImgDwdSts.Status = FALSE;

	std::list<ImgDownloadStatus>::iterator iter = ImgDwdSts.begin();
	std::list<ImgDownloadStatus>::iterator iter_end = ImgDwdSts.end();


	for(;iter != iter_end;++iter){
		if(iter->Path == CurImgDwdSts.Path){
			//Clear the status flag to invalid when the same port is attched with another device. 
			iter->Status = FALSE;
			break;
		}
	}	

	//This is a new port with is attached with a device.
	if(iter == iter_end)
		ImgDwdSts.push_back(CurImgDwdSts);

	TRACE(_T("Initialize new i.mx device of path: %s.\r\n"), CurImgDwdSts.Path);

	if(_MaxPacketSize.get() == 0)
		goto Exit;
	_habType = GetHABType(_chipFamily);
	if(_habType == HabUnknown)
		goto Exit;
	if(!InitRomVersion(_chipFamily, _romVersion, _defaultAddress))
		goto Exit;

	TRACE("************The new i.mx device is initialized**********\n");
	TRACE("\n");
	return;
Exit:
	TRACE("Failed to initialize the new i.MX device!!!\n");
}

MxRomDevice::~MxRomDevice()
{
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
		else
		{
			//We must indicate caller this is an invalid value 
			//0 is used to indicate an invalid value.
			Value = 0;
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
	const short header = ROM_KERNEL_CMD_WR_MEM;
	const UINT address = chipType == MX51 ? 0x1FFEA000 : 0xFFFFFFFF;
	const UINT data = 0;
	const char format = 32;
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
		return HabUnknown;
	}

	//read the ACK
	unsigned char cmdAck[4] = { 0 };
	if(!ReadFromDevice(cmdAck, 4))
	{
		return HabUnknown;
	}

	// get response of HAB type
	//TRACE("GetHABType(): Receive ack:%x\n", *(unsigned int *)cmdAck);
	if (*(unsigned int *)cmdAck == HabEnabled)
	{
		TRACE("GetHABType(): this is production type\n");
		// production parts
		if ( (_chipFamily == MX35) || (_chipFamily == MX37) || (_chipFamily == MX51) )
		{
			unsigned char writeAck[4] = { 0 };
			if(!ReadFromDevice(writeAck, 4))
			{
				return HabUnknown;
			}

			//TRACE("GetHABType(): Receive ack2:%x\n", *(unsigned int *)writeAck);
		}

		return HabEnabled; 

	}
	else if (*(unsigned int *)cmdAck == HabDisabled)
	{
		TRACE("GetHABType(): this is develop/disable type\n");

		unsigned char writeAck[4] = { 0 };

		if(!ReadFromDevice(writeAck, 4))
		{
			return HabUnknown;
		}
		//TRACE("GetHABType(): Receive ack2:%x\n", *(unsigned int *)writeAck);

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
	if( !scriptFile.Open(filename, CFile::modeRead | CFile::shareDenyNone, &fileException) )
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
		if ( ! ValidAddress(address, format) )
		{
			TRACE(_T("WriteMemory(): Invalid memory address found! 0x%X/n"), address);
			return FALSE;
		}
	}

	unsigned char writeAck[4] = { 0 };
	if(!ReadFromDevice(writeAck, 4))
	{
		return FALSE;
	}

	if (*(unsigned int *)writeAck != ROM_WRITE_ACK)
	{
		TRACE("WriteMemory(): Invalid write ack: 0x%x\n", *(unsigned int *)writeAck);
		return FALSE; 
	}
	return TRUE;
}
BOOL MxRomDevice::ValidAddress(const UINT address, const UINT format) const
{
	BOOL status = FALSE;
	UINT topByteAddress = address + ((format/8) - 1); 

	switch ( _chipFamily )
	{
		case MX31:
		case MX32:
			if (   ((topByteAddress <= MX31_NFCend)	   && (address >= MX31_NFCstart)) 
				|| ((topByteAddress <= MX31_WEIMend)   && (address >= MX31_WEIMstart))
				|| ((topByteAddress <= MX31_MemoryEnd) && (address >= MX31_MemoryStart))
				|| ((topByteAddress <= MX31_ESDCTLend) && (address >= MX31_ESDCTLstart))
				|| ((topByteAddress <= MX31_CCMend)	   && (address >= MX31_CCMstart) && (address != MX31_CCM_CGR0)) )
			{ 
				status = TRUE;
				TRACE("Matching the MX31 address region\n");
			}
			break;

		case MX27:
			if (   ((topByteAddress <= MX27_NFCend)	   && (address >= MX27_NFCstart)) 
				|| ((topByteAddress <= MX27_WEIMend)   && (address >= MX27_WEIMstart))
				|| ((topByteAddress <= MX27_MemoryEnd) && (address >= MX27_MemoryStart))
				|| ((topByteAddress <= MX27_ESDCTLend) && (address >= MX27_ESDCTLstart))
				|| ((topByteAddress <= MX27_CCMend)	   && (address >= MX27_CCMstart) && (address != MX27_CCM_CGR0)) )
			{
				status = TRUE;
				TRACE("Matching the MX27 address region\n");
			}
			break;

		case MX35:
			if (   ((topByteAddress <= MX35_WEIM_CS2_END)  && (address >= MX35_SDRAM_START))
				|| ((topByteAddress <= MX35_IRAM_FREE_END) && (address >= MX35_IRAM_FREE_START))
				|| ((topByteAddress <= MX35_NFC_END)       && (address >= MX35_NFC_START))
				|| ((topByteAddress <= MX35_SDRAM_CTL_END) && (address >= MX35_SDRAM_CTL_START))
				|| ((topByteAddress <= MX35_WEIM_END)      && (address >= MX35_WEIM_START)) )
			{
				status = TRUE;
				TRACE("Matching the MX35 address region\n");
			}
			break;

		case MX37:
			if (   ((topByteAddress <= MX37_WEIM_CS2_END)  && (address >= MX37_SDRAM_START))
				|| ((topByteAddress <= MX37_IRAM_FREE_END) && (address >= MX37_IRAM_FREE_START))
				|| ((topByteAddress <= MX37_NFC_END)       && (address >= MX37_NFC_START))
				|| ((topByteAddress <= MX37_SDRAM_CTL_END) && (address >= MX37_SDRAM_CTL_START))
				|| ((topByteAddress <= MX37_WEIM_END)      && (address >= MX37_WEIM_START)))
			{
				status = TRUE;
				TRACE("Matching the MX37 address region\n");
			}
			break;

		case MX51:
			if (   ((topByteAddress <= MX51_WEIM_CS2_END)  && (address >= MX51_SDRAM_START))
				|| ((topByteAddress <= MX51_IRAM_FREE_END) && (address >= MX51_IRAM_FREE_START))
				|| ((topByteAddress <= MX51_NFC_END)       && (address >= MX51_NFC_START))
				|| ((topByteAddress <= MX51_SDRAM_CTL_END) && (address >= MX51_SDRAM_CTL_START))
				|| ((topByteAddress <= MX51_WEIM_END)      && (address >= MX51_WEIM_START)))
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
	
	//Sleep(5000);
	const short header = ROM_KERNEL_CMD_GET_STAT;
	const UINT address = 0;
	const UINT data = 0;
	const char format = 0;
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
								  unsigned long param1, unsigned long param2)
{
	cmd[0]  = (unsigned char)(RAM_KERNEL_CMD_HEADER >> 8);
	cmd[1]  = (unsigned char)(RAM_KERNEL_CMD_HEADER & 0x00ff);
	cmd[2]  = (unsigned char)(cmdId >> 8);
	cmd[3]  = (unsigned char) cmdId;
	cmd[4]  = (unsigned char)(addr >> 24);
	cmd[5]  = (unsigned char)(addr >> 16);
	cmd[6]  = (unsigned char)(addr >> 8);
	cmd[7]  = (unsigned char) addr;
	cmd[8]  = (unsigned char)(param1 >> 24);
	cmd[9]  = (unsigned char)(param1 >> 16);
	cmd[10] = (unsigned char)(param1 >> 8);
	cmd[11] = (unsigned char) param1;
	cmd[12] = (unsigned char)(param2 >> 24);
	cmd[13] = (unsigned char)(param2 >> 16);
	cmd[14] = (unsigned char)(param2 >> 8);
	cmd[15] = (unsigned char) param2;
	return;
}

//-------------------------------------------------------------------------------------		
// Function to unpack the response
//
// @response  response read from device
//
// @return
//-------------------------------------------------------------------------------------		
struct MxRomDevice::Response MxRomDevice::UnPackRklResponse(unsigned char *resBuf)
{
	struct Response res;
	res.romResponse = *((PUINT)&resBuf[0]);
	res.ack  =  (unsigned short)(((unsigned short)resBuf[0] << 8) | resBuf[1]);
	res.csum =  (unsigned short)(((unsigned short)resBuf[2] << 8) | resBuf[3]);
	res.len  = ((unsigned long)resBuf[4] << 24) | ((unsigned long)resBuf[5] << 16) |
			   ((unsigned long)resBuf[6] << 8) | ((unsigned long)resBuf[7]);

	return res;
}

//-------------------------------------------------------------------------------------		
// Function to RAM Kernel command
//
// @return
//-------------------------------------------------------------------------------------
struct MxRomDevice::Response MxRomDevice::SendRklCommand(unsigned short cmdId, unsigned long addr, unsigned long param1, unsigned long param2)
{
	unsigned char command[RAM_KERNEL_CMD_SIZE] = {0},
				  retBuf[RAM_KERNEL_ACK_SIZE] = {0};
	struct Response res = {0};
	res.ack = ERROR_COMMAND;

	// pack command requeset
	PackRklCommand(command, cmdId, addr, param1, param2);

	// send command to remote device
	if (!WriteToDevice(command, RAM_KERNEL_CMD_SIZE))
	{
		TRACE("MxRomDevice::SendRklCommand(): failed to send Ram Kernel command to device.\n");
		return res;
	}
		
	if (!ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE))
	{
		TRACE("MxRomDevice::SendRklCommand(): failed to read response from Ram Kernel command.\n");
		return res;
	}
	
	// unpack the response
	return UnPackRklResponse(retBuf);
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
	int i =0;
	struct Response res = SendRklCommand(RAM_KERNEL_CMD_GETVER, 0, 0, 0);

	//TRACE("atk_common_getver(): romResponse: 0x%X\n", res.romResponse);
	
	// check if is bootstrap
	if (res.romResponse == HabEnabled || res.romResponse == HabDisabled)
	{
		len = 0;
		TRACE("The device is running under Bootstrap mode.\r\n");
		return RET_SUCCESS;
	}

	while(res.ack != RET_SUCCESS){
		if(i == 10)
			break;
		i++;
		Sleep(1000);
		res = SendRklCommand(RAM_KERNEL_CMD_GETVER, 0, 0, 0);
	}

	if (res.ack == RET_SUCCESS) {
		
		//TRACE("atk_common_getver(): chip id: %x\n", res.csum);

		unsigned char model[MAX_MODEL_LEN] = {0};
		if ( !ReadFromDevice(model, res.len) )
		{
			return INVALID_CHANNEL;
		}
		
		len = res.len;
		mxType = res.csum;
		fmodel = model;
		fmodel.AppendChar(_T('\0'));
		//TRACE("atk_common_getver(): get version: %s\n", fmodel);
	}
	else
	{
		// if response is not ok, return failed
		TRACE("MxRomDevice::GetRKLVersion: failed to get device status.\n");
	}

	TRACE("The device is running under RAM kernel mode.\r\n");

	return -res.ack;
}

#define FLASH_HEADER_SIZE	0x20
#define ROM_TRANSFER_SIZE	0x400

BOOL MxRomDevice::DownloadImage(UINT address, MemorySection loadSection, MemorySection setSection, BOOL HasFlashHeader, const StFwComponent& fwComponent, Device::UI_Callback callbackFn)
{
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
			//Sleep(10);
			if(!TransData(numBytesToWrite, pBuffer + byteIndex))
			{
				TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X) failed.\n"), numBytesToWrite, pBuffer + byteIndex);
				return FALSE;
			}
		}
	}

	// If we are downloading to DCD or CSF, we don't need to send 
	if ( loadSection == MemSectionDCD || loadSection == MemSectionCSF )
	{
		return TRUE;
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
		//Sleep(10);
		if(!TransData(ROM_TRANSFER_SIZE, pHeaderData))
		{
			TRACE(_T("DownloadImage(): TransData(0x%X, 0x%X) failed.\n"), ROM_TRANSFER_SIZE, pHeaderData);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL MxRomDevice::ProgramFlash(std::ifstream& file, UINT address, UINT cmdID, UINT flags, Device::UI_Callback callback)
{
	BOOL success = FALSE;
	const unsigned char * pBuffer = new unsigned char[MAX_SIZE_PER_FLASH_COMMAND];

	// Get file size
	file.seekg(0, std::ios::end);
	const size_t dataSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// Set UI

	uint32_t byteIndex, numBytesToWrite = 0;
	for ( byteIndex = 0; byteIndex < dataSize; byteIndex += numBytesToWrite )
	{
		// reset buffer
		memset((unsigned char *)pBuffer, 0, MAX_SIZE_PER_FLASH_COMMAND);
		
		// How much data to write
		numBytesToWrite = min(MAX_SIZE_PER_FLASH_COMMAND, dataSize - byteIndex);

		// read the data from the file
		file.read((char *)pBuffer, (size_t)numBytesToWrite);

		// The data size may not be a multiple of 512. The buffer space is enough 
		// to contain extra data after data byte to write is aligned to 512 boundry
		// since it is fixed to MAX_SIZE_PER_FLASH_COMMAND which can be divided by 512 without reminders.
		if( numBytesToWrite % 512 )
      	{
          numBytesToWrite = ((numBytesToWrite / 512) + 1) * 512; 
      	}

		// Adjust flags if not the first time through
		if ( byteIndex != 0 )
			flags |= FLASH_PROGRAM_GO_ON;

		//
		// Program the flash
		//
		struct Response res = SendRklCommand(cmdID, address + byteIndex, numBytesToWrite, flags);
		if ( res.ack != RET_SUCCESS )
		{
			TRACE(_T("ProgramFlash(): SendRklCommand(cmd:0x%X, addr:0x%X, size:0x%X, flags:0x%X) failed.\n"), cmdID, address + byteIndex, numBytesToWrite, flags);
			delete[] pBuffer;
			return FALSE;
		}

		// write data to device
		if(!TransData(numBytesToWrite, pBuffer))
		{
			TRACE(_T("ProgramFlash(): TransData(size:0x%X, addr:0x%X) failed.\n"), numBytesToWrite, address + byteIndex);
			delete[] pBuffer;
			return FALSE;
		}
	
		unsigned char retBuf[RAM_KERNEL_ACK_SIZE] = {0};
				  
		res.ack = FLASH_PARTLY;
		while ( res.ack == FLASH_PARTLY )
		{
			// read response from device
			if ( !ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE) )
			{
				TRACE(_T("ProgramFlash(): ReadFromDevice() failed.\n"));
				delete[] pBuffer;
				return FALSE;
			}
			// unpack the response
			res = UnPackRklResponse(retBuf);

			//TRACE(_T("Do Flash program, programing: get response: %d:program block id: %d:program size:%d\n"), res.ack, res.csum, res.len);

			if ( res.ack != FLASH_PARTLY && res.ack != FLASH_VERIFY && res.ack !=RET_SUCCESS )
			{
				// if response is not ok, return failed
				TRACE(_T("ProgramFlash(): get response: %d\n"), res.ack);
				delete[] pBuffer;
				return FALSE;
			}
				
			// finished += res.len;
			// ::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_PROG, (LPARAM)(finished + done));
		}

		//
		// Readback check the flash
		//
		if ( flags & FLASH_PROGRAM_READ_BACK )
		{
			res.ack = FLASH_VERIFY;
			while (res.ack == FLASH_VERIFY)
			{
				// read response from device
				if ( !ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE) )
				{
					TRACE(_T("ProgramFlash(): ReadFromDevice() failed.\n"));
					delete[] pBuffer;
					return FALSE;
				}
				// unpack the response
				res = UnPackRklResponse(retBuf);

				//TRACE(_T("Do Flash program, verifing: get response: %d : block is %d\n"), res.ack, res.csum);

				if ( res.ack != RET_SUCCESS && res.ack != FLASH_VERIFY )
				{
					// if response is not ok, return failed
					TRACE(_T("ProgramFlash(): ++++++get response-------: %d\n"), res.ack);
					//delete[] pBuffer;
					//return FALSE;
				}

				// finished += res.len;
				// ::SendMessage(m_hWnd, WM_USER_PROGRESS, (WPARAM)OPMODE_FLASH_VERIFY, (LPARAM)(finished + done));
			}
		
#if 0
			// do checksum, and compare with the csum in response
			if (res.csum != (checksum = MakeChecksum(buf, count))) {
				DEBUG("AtkFlashprogram(): invalid checksum (%x)<->(%x)\n", res.csum, checksum);
				SetUartTimeout(DEFAULT_UART_TIMEOUT);
				return INVALID_CHECKSUM;
			}
#endif
		} // end if ( FLASH_PROGRAM_READ_BACK )

		// Update UI

	} // end for ( fileSize )

	// Clean up
	delete[] pBuffer;

	// Finish UI

	return TRUE;
}

BOOL MxRomDevice::Jump()
{
	CString CurPath = _path.get();
	std::list<ImgDownloadStatus>::iterator iter = ImgDwdSts.begin();
	std::list<ImgDownloadStatus>::iterator iter_end = ImgDwdSts.end();

	//Current device has finished downloading work, mark status flag as true;
	for(;iter != iter_end;++iter){
		if(iter->Path == CurPath)
			iter->Status = TRUE;
	}

	//Poll the status of each device till all devices have finished downloading work.
	//In a word, we need to syncronize all the device's status in case a reset occurs 
	//when one device is on downloading work, which will leads to transfer failure.
	do{
		SyncAllDevFlag = TRUE;
		for(iter = ImgDwdSts.begin(); iter != iter_end; ++iter){
			if(iter->Status == FALSE)
				SyncAllDevFlag = FALSE;
		}
		if(SyncAllDevFlag)
			break;
		else
			Sleep(100);
	}while(!SyncAllDevFlag);

	// Send the complete command to the device 
	if ( !Jump2Rak() )
	{
		TRACE(_T("Jump(): Jump2Rak() failed.\n"));
		return FALSE;
	}

	return TRUE;
}

BOOL MxRomDevice::SendCommand2RoK(UINT address, UINT dataSize, UCHAR pointer)
{
	BOOL status = FALSE;

	const short header = ROM_KERNEL_CMD_WR_FILE;
	const UINT data = 0;	
	const UINT format = 0;
	
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

	if ( (*(unsigned int *)cmdAck != HabEnabled)  && 
		 (*(unsigned int *)cmdAck != HabDisabled) ) 
	{
		return FALSE;	
	}

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

			//TRACE("Lat will Transfer Size: %d\n", byteCount);
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
	CString DevPath = _path.get();

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
	if (!OpenUSBHandle(&_hDevice, DevPath))
	{
		TRACE(_T("MxRomDevice::DeviceIoControl() CreateFile() ERROR = %d.\n"), GetLastError());
		return FALSE;
	}
	if( _hDevice == INVALID_HANDLE_VALUE )
	{
		TRACE(_T("MxRomDevice::DeviceIoControl() CreateFile() ERROR = %d.\n"), GetLastError());
		return FALSE;
	}
	closeOnExit = TRUE;

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

	if (*pHandle == INVALID_HANDLE_VALUE) {
		TRACE(_T("MxRomDevice::OpenUSBHandle() Failed to open (%s) = %d"), pipePath, GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL MxRomDevice::WriteToDevice(const unsigned char *buf, UINT count)
{
	int    nBytesWrite; // for bytes actually written
	CString pipePath = _path.get();

	if (!OpenUSBHandle(&_hDevice, pipePath) || !OpenUSBHandle(&_hWrite, pipePath+_T("\\PIPE00")))
	{
		TRACE(_T("MxRomDevice::WriteToDevice() failed to open the device. Error code = 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		return FALSE;
	}

	if( !WriteFile(_hWrite, buf, count, (PULONG) &nBytesWrite, NULL) )
	{
		TRACE(_T("MxRomDevice::WriteToDevice() Error writing to device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		return FALSE;
	}

	if (!CloseHandle(_hWrite))
	{
		TRACE(_T("MxRomDevice::WriteToDevice() CloseHandle() ERROR = %d.\n"), GetLastError());
		return FALSE;
	}

	return TRUE;	
}

BOOL MxRomDevice::ReadFromDevice(PUCHAR buf, UINT count)
{
	int    nBytesRead; // for bytes actually read
	CString pipePath = _path.get();

	if (!OpenUSBHandle(&_hDevice, pipePath) || !OpenUSBHandle(&_hRead, pipePath+_T("\\PIPE01")))
	{
		TRACE(_T("MxRomDevice::ReadFromDevice() failed to open the device. Error code = 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		return FALSE;
	}

	if( !ReadFile(_hRead, buf, count, (PULONG) &nBytesRead, NULL) )
	{
		TRACE(_T("MxRomDevice::ReadFromDevice() Error reading from device 0x%x(%d).\r\n"), GetLastError(), GetLastError());
		return FALSE;
	}

	if (!CloseHandle(_hRead))
	{
		TRACE(_T("MxRomDevice::ReadFromDevice() CloseHandle() ERROR = %d.\n"), GetLastError());
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
