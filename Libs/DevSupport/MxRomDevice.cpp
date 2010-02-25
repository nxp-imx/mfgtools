#pragma once
#include "stdafx.h"
#include "MxRomDevice.h"
#include "DeviceManager.h"
#include "Libs/WDK/usb100.h"
#include <sys/stat.h>

//#include "iMX/MemoryInit.h"
#include "iMX/MxDefine.h"
#include "iMX/AtkHostApiClass.h"
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

void MxRomDevice::SetIMXDevPara(void)
{
	atkConfigure.SetChannel(ChannelType_USB, 0, 1);

	if(m_MxRomParamt.cMXType ==  _T("25_TO1"))
		atkConfigure.SetMXType(MX_MX25_TO1);
	else if(m_MxRomParamt.cMXType ==  _T("25_TO11"))
		atkConfigure.SetMXType(MX_MX25_TO11);
	else if(m_MxRomParamt.cMXType ==  _T("31_TO1"))
		atkConfigure.SetMXType(MX_MX31_TO1);
	else if(m_MxRomParamt.cMXType ==  _T("31_TO2"))
		atkConfigure.SetMXType(MX_MX31_TO2);
	else if(m_MxRomParamt.cMXType ==  _T("35_TO1"))
		atkConfigure.SetMXType(MX_MX35_TO1);
	else if(m_MxRomParamt.cMXType ==  _T("35_TO2"))
		atkConfigure.SetMXType(MX_MX35_TO2);
	else if(m_MxRomParamt.cMXType ==  _T("37"))
		atkConfigure.SetMXType(MX_MX37);
	else if(m_MxRomParamt.cMXType ==  _T("51_TO1"))
		atkConfigure.SetMXType(MX_MX51_TO1);
	else if(m_MxRomParamt.cMXType ==  _T("51_TO2"))
		atkConfigure.SetMXType(MX_MX51_TO2);
	else
	{
		TRACE(("Please check the MXType in ucl.xml, there is no handle for %s!\n"),m_MxRomParamt.cMXType);
	}

	if(m_MxRomParamt.cSecurity == _T("true"))
	{
		atkConfigure.SetSecurity(TRUE);
	}
	else
	{
		atkConfigure.SetSecurity(FALSE);
	}

	if(m_MxRomParamt.cRAMType == _T("DDR"))
		atkConfigure.SetRAMType(MM_DDR);
	else if(m_MxRomParamt.cRAMType == _T("SDRAM"))
		atkConfigure.SetRAMType(MM_SDRAM);
	else if(m_MxRomParamt.cRAMType == _T("DDR2"))
		atkConfigure.SetRAMType(MM_DDR2);
	else if(m_MxRomParamt.cRAMType == _T("MDDR"))
		atkConfigure.SetRAMType(MM_MDDR);
	else if(m_MxRomParamt.cRAMType == _T("CUSTOM"))
		atkConfigure.SetRAMType(MM_CUSTOM);

//	atkConfigure.SetChannel(ChannelType_USB, 0, 1);
//	atkConfigure.SetMemoryAddr(mm_addrs[atkConfigure.GetMXType()]);
	//theApp.atkConfigure.SetRAMType(RAMType);
	//		theApp.objHostApiClass.SetUsbTimeout(USB_OPEN_USR_TIMEOUT,USB_TRANS_USR_TIMEOUT);
	//		TRACE("Set the usb timeout open %d secs, trans %d msecs\n",USB_OPEN_USR_TIMEOUT,USB_TRANS_USR_TIMEOUT);

	// Read mmInitScripts from MxMemoryDefines.txt file
	atkConfigure.SetMemoryType(atkConfigure.GetMXType(),
		atkConfigure.GetRAMType(),
		m_MxRomParamt.cMemInitFilePath);
}

BOOL MxRomDevice::InitMemoryDevice(MxRomDevice::MemoryInitScript script)
{
	MemoryInitScript::iterator command;
	for ( command = script.begin(); command != script.end(); ++command )
	{
		if ( !WriteMemory((*command).Address, (*command).Data, (*command).Format) )
		{
			TRACE("In InitMemoryDevice(): write memory failed\n");
			return FALSE;
		}
	}

	return TRUE;
}

BOOL MxRomDevice::DownloadRKL(const StFwComponent& fwComponent, Device::UI_Callback callbackFn)
{
	// do initial DDR
//	if (!InitMemoryDevice()) {
//		InsertLog(CString("Initial memory failed!"));
//		return FALSE;
//	}
	
	// set the communication mode through write memory interace
	if ( !WriteMemory(_defaultAddress.MemoryStart, ChannelType_USB, 32) )
	{
		TRACE(CString("Write channel type to memory failed!\n"));
		return ERROR_COMMAND;
	}

     //  InsertLog(CString("Loading RAM Kernel!"));
	// do download
//	m_ctrlPro.SetRange32(0, rklsize/MINIFICATION);
//	m_ctrlPro.SetStep(WR_BUF_MAX_SIZE/MINIFICATION);
//	m_ctrlPro.SetPos(0);
	// download the RKL
	BOOL success = DownloadImage(_defaultAddress.DefaultRKL, fwComponent, callbackFn);
	
	if (!success) {
		TRACE(CString("Can not load RAM Kernel to device\n"));
		return ERROR_COMMAND;
	} else {
		
		// Send the complete command to the device 
		success = Jump2Rak();

		// check the status
		if (!success) {
			TRACE(CString("Can not execute RAM Kernel\n"));
			return ERROR_COMMAND;
		}
	}
//	m_ctrlPro.SetPos(rklsize/MINIFICATION);

	return ERROR_SUCCESS;
}

BOOL MxRomDevice::DownloadRKL(unsigned char *rkl, int rklsize, unsigned int RAMKNLAddr, bool bPreload)
{
	int cType, cId;
	unsigned long cHandle;
	BOOL status;
	BOOL is_hab_prod = false;
	CString m_strCSFName;
	CString m_strDCDName;

//	while(!bMutex4RamKnlDld)
//	{
//		Sleep(100);
//	}
	bMutex4RamKnlDld = FALSE;
/*
	// Get handles for write operation
	if(!USB_OpenDevice())
	{
		goto Exit;
	}

	TRACE("************Initialize i.mx device with port #%x***************\n",_hubIndex.get());

	if ( GetHABType() )
	{
		is_hab_prod = true;
	} else {
		is_hab_prod = false;
	}

	if(!m_bMemInited)
	{
		// do initial DDR
		if (!InitMemoryDevice()) {
			TRACE("Initial memory failed!\n");
			goto CloseThenExit;
		}

		atkConfigure.GetChannel(&cType, &cId, &cHandle);
		// set the communication mode through write memory interace
		if (!WriteMemory(atkConfigure.GetMXType(),
			atkConfigure.GetMemoryAddr(MEMORY_START_ADDR), 
			cType, 32))
		{
			TRACE("Write channel type to memory failed!\n");
			goto CloseThenExit;
		}
		m_bMemInited = TRUE;
	}

	// download the RKL
	//RamAddr = (bPreload)? m_MxRomParamt.PreLoadAddr : m_MxRomParamt.RAMKNLAddr;
	status = DownloadImage(rklsize, rkl, RAMKNLAddr, bPreload);


	if (!status) {
		TRACE("Error: Can not load RAM Kernel to device\n");
		goto CloseThenExit;
	} else if(!bPreload){		
		// Send the complete command to the device 
		status = Jump2Rak(atkConfigure.GetMXType(), is_hab_prod);

		// check the status
		if (!status) {
			TRACE("Error: Can not execute RAM Kernel\n");
			goto CloseThenExit;
		}

		// Send reset command to device.
		if(!DeviceIoControl(IOCTL_IMXDEVICE_RESET_DEVICE)) {
			TRACE("Error: Device reset failed.\n");
			goto CloseThenExit;
		}
	}

	if ( !USB_CloseDevice() )
		goto Exit;
	bMutex4RamKnlDld = TRUE;

	return TRUE;

CloseThenExit:
	USB_CloseDevice();
*/
Exit:
	bMutex4RamKnlDld = TRUE;
	return FALSE;
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
	TRACE("WriteMemory(): Receive ack:%x\n", *(unsigned int *)cmdAck);
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
	TRACE(">> Jump2 Ramkernel result: 0x%x\n", *(unsigned int *)cmdAck);
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
			TRACE(">> Jump2 Ramkernel result(2): 0x%x\n", *(unsigned int *)cmdAck2);
			if (*(unsigned int *)cmdAck2 == ROM_STATUS_ACK2)
			{
				return FALSE;
			}
		}
	}

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
	cmd[0] = (unsigned char)(RAM_KERNEL_CMD_HEADER >> 8);
	cmd[1] = (unsigned char)(RAM_KERNEL_CMD_HEADER & 0x00ff);
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
struct Response MxRomDevice::UnPackRklResponse(unsigned char *resBuf)
{
	struct Response res;

	res.ack = (unsigned short)(((unsigned short)resBuf[0] << 8) | resBuf[1]);
	res.csum = (unsigned short)(((unsigned short)resBuf[2] << 8) | resBuf[3]);
	res.len = ((unsigned long)resBuf[4] << 24) | ((unsigned long)resBuf[5] << 16) |
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
int MxRomDevice::GetRKLVersion(unsigned char *fmodel, int *len, int *mxType)
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
		DEBUG("atk_common_getver(): failed to send to device\n");
		return INVALID_CHANNEL;
	}
		
	if (!ReadFromDevice(retBuf, RAM_KERNEL_ACK_SIZE))
	{
		DEBUG("atk_common_getver(): failed to read bootstrap from device\n");
		return INVALID_CHANNEL;
	}
	
	DEBUG("atk_common_getver(): retBuf[0]: %d\n", retBuf[0]);
	// check if is bootstrap
	PUINT pReturn = (PUINT)&retBuf[0];
	if (*pReturn == HabEnabled || *pReturn == HabDisabled)
	{
		*len = 0;
		return RET_SUCCESS;
	}
		
//	}
	// unpack the response
	res = UnPackRklResponse(retBuf);

	if (res.ack == RET_SUCCESS) {
		
		DEBUG("atk_common_getver(): chip id: %x\n", res.csum);

		status = ReadFromDevice(fmodel, res.len);
		if (!status) {
			return INVALID_CHANNEL;
		}
		*len = res.len;
		*mxType = res.csum;
		fmodel[res.len] = '\0';
		DEBUG("atk_common_getver(): get version: %s\n", fmodel);
	} else {
		// if response is not ok, return failed
		DEBUG("atk_common_getver(): get response: %d\n", res.ack);
	}
	
	return -res.ack;
}

#define FLASH_HEADER_SIZE	0x20
#define ROM_TRANSFER_SIZE	0x400
//#define MX_51_NK_LOAD_ADDRESS			0x90200000
//#define MX_51_EBOOT_LOAD_ADDRESS		0x90040000

BOOL MxRomDevice::DownloadImage(UINT address, const StFwComponent& fwComponent, Device::UI_Callback callbackFn)
{
	int counter = 0;
	int bytePerCommand = 0;
	
	//Here we must transfer virtual address to physical address before downloading ram kernel.
//	int PhyRAMAddr4KRL = _defaultAddress.MemoryStart | (address & (~0xF0000000));
//	int ActualImageAddr = PhyRAMAddr4KRL;
//	int FlashHdrAddr = PhyRAMAddr4KRL - FLASH_HEADER_SIZE;

	size_t dataSize = fwComponent.size();
	if ( dataSize < MAX_SIZE_PER_DOWNLOAD_COMMAND )
//	uint32_t byteIndex, numBytesToWrite = 0;
//	for ( byteIndex = 0; byteIndex < dataSize; byteIndex += numBytesToWrite )
	{
		// Init the buffer to 0
//		memset(buffer, 0, sizeof(buffer));

		// Get some data
//		numBytesToWrite = min(MAX_SIZE_PER_DOWNLOAD_COMMAND, dataSize - byteIndex);
//		memcpy_s(buffer, sizeof(buffer), &fwComponent.GetData()[byteIndex], numBytesToWrite);

		if (!SendCommand2RoK(address, dataSize, ROM_KERNEL_WF_FT_OTH))
		{
			return FALSE;
		}
		else
		{
			Sleep(10);
			if(!TransData(dataSize, fwComponent.GetDataPtr(), OPMODE_DOWNLOAD))
			{
				return FALSE;
			}
		}
	}

//	if (byteIndex == dataSize)
	{
		//transfer length of ROM_TRANSFER_SIZE is a must to ROM code.
		unsigned char FlashHdr[ROM_TRANSFER_SIZE] = { 0 };
		//Init the memory
//		memset(FlashHdr,0, ROM_TRANSFER_SIZE);
		//Copy image data with an offset of FLASH_HEADER_SIZE to the temp buffer.
//		memcpy(FlashHdr+FLASH_HEADER_SIZE, pBuf, ROM_TRANSFER_SIZE-FLASH_HEADER_SIZE);
		//We should write actual image address to the first dword of flash header.
//		((int *)FlashHdr)[0] = ActualImageAddr;
		memcpy_s(FlashHdr, ROM_TRANSFER_SIZE, fwComponent.GetDataPtr(), ROM_TRANSFER_SIZE);

		//Jump to RAM kernel to execute it.
		if (!SendCommand2RoK(address, ROM_TRANSFER_SIZE, /*(bPreload)? ROM_KERNEL_WF_FT_OTH:*/ ROM_KERNEL_WF_FT_APP))
		{
			return FALSE;
		}
		else
		{
			Sleep(10);
//			pBuffer = pBuf;
			if(!TransData(ROM_TRANSFER_SIZE,(const unsigned char *)FlashHdr,OPMODE_DOWNLOAD))
			{
				return FALSE;
			}
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
	if (pCommandAck[0] != HabDisabled && 
		pCommandAck[0] != HabEnabled ) 
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
	}
	if (((atkConfigure.GetMXType() == MX_MX31_TO1) || (atkConfigure.GetMXType() == MX_MX31_TO2) ||
		(atkConfigure.GetMXType() == MX_MX31_TO201) || (atkConfigure.GetMXType() == MX_MX32)) && byteCount > 0) {

			TRACE("Lat will Transfer Size: %d\n", byteCount);
			if (!WriteToDevice(pBuffer, byteCount))
				return FALSE;

			byteCount = 0;
	} else {
		UINT uintMaxPacketSize0 = _MaxPacketSize.get();

#define MINUM_TRANSFER_SIZE 0x20

		for (; byteCount > uintMaxPacketSize0; \
			byteCount -= uintMaxPacketSize0, pBuffer += uintMaxPacketSize0) {
				if (!WriteToDevice(pBuffer, uintMaxPacketSize0))
					return FALSE;
				//TRACE("Transfer Size: %d\n", uintMaxPacketSize0);
		}

		for (; byteCount >= MINUM_TRANSFER_SIZE; \
			byteCount -= MINUM_TRANSFER_SIZE, pBuffer += MINUM_TRANSFER_SIZE) {
				if (!WriteToDevice(pBuffer, MINUM_TRANSFER_SIZE))
					return FALSE;
				//TRACE("Transfer Size: %d\n", MINUM_TRANSFER_SIZE);
		}
	}
	if (0 != byteCount) {
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

MxRomDevice::MemoryInitScript* MxRomDevice::GetMemoryScript(ChipFamily_t chipType, RomVersion romVersion, MemoryType memoryType) const
{
	MemoryInitScript* pScript = NULL;

	switch ( chipType )
	{
		case MX25:
			if ( romVersion == RomVersion(1,0) )
				pScript = &mddrMx25;
			else if ( romVersion == RomVersion(1,1) )
				pScript = &ddr2Mx25_To11;
			break;

		case MX27:
			pScript = &ddrMx27;
			break;

		case MX31:
		case MX32:
			if ( memoryType == DDR || memoryType == DDR2 )
				pScript = &ddrMx31;
			else if ( memoryType == SDRAM || memoryType == mDDR )
				pScript = &sdrMx31;
			break;

		case MX35:
			if ( memoryType == DDR || memoryType == DDR2 )
				pScript = &ddr2Mx35;
			else if ( memoryType == SDRAM || memoryType == mDDR )
				pScript = &mddrMx35;
			break;

		case MX37:
			pScript = &ddrMx37;
			break;

		case MX51:
			if ( romVersion == RomVersion(1,0) )
				pScript = &ddrMx51;
			else if ( romVersion == RomVersion(2,0) )
				if ( memoryType == DDR || memoryType == DDR2 )
					pScript = &ddrMx51_To2;
				else if ( memoryType == SDRAM || memoryType == mDDR )
					pScript = &mddrMx51_To2;
			break;
		
		default:
			break;
	}

	return pScript;
}

MxRomDevice::MemoryInitScript MxRomDevice::ddrMx31;
MxRomDevice::MemoryInitScript MxRomDevice::ddrMx27;
MxRomDevice::MemoryInitScript MxRomDevice::sdrMx31;
MxRomDevice::MemoryInitScript MxRomDevice::ddrMx35;
MxRomDevice::MemoryInitScript MxRomDevice::mddrMx35;
MxRomDevice::MemoryInitScript MxRomDevice::ddr2Mx35;
MxRomDevice::MemoryInitScript MxRomDevice::ddrMx37;
MxRomDevice::MemoryInitScript MxRomDevice::ddrMx51;
MxRomDevice::MemoryInitScript MxRomDevice::ddrMx51_To2;
MxRomDevice::MemoryInitScript MxRomDevice::mddrMx51_To2;
MxRomDevice::MemoryInitScript MxRomDevice::ddrMx25;
MxRomDevice::MemoryInitScript MxRomDevice::mddrMx25;
MxRomDevice::MemoryInitScript MxRomDevice::ddr2Mx25_To11;

void MxRomDevice::InializeMemoryScripts()
{
	// MX31 - DDR
	ddrMx31.push_back( MemoryInitCommand(0xB8002050, 0x0000DCF6, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xB8002054, 0x444a4541, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xb8002058, 0x44443302, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xB6000000, 0xCAFECAFE, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xb8002000, 0x0000CC03, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xb8002004, 0xa0330D01, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xb8002008, 0x00220800, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xB8001010, 0x00000004, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xB8001004, 0x006ac73a, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xB8001000, 0x92100000, 32) );
	ddrMx31.push_back( MemoryInitCommand(0x80000f00, 0x12344321, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xB8001000, 0xa2100000, 32) );
	ddrMx31.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	ddrMx31.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	ddrMx31.push_back( MemoryInitCommand(0xB8001000, 0xb2100000, 32) );
	ddrMx31.push_back( MemoryInitCommand(0x80000033, 0xda, 8) );
	ddrMx31.push_back( MemoryInitCommand(0x81000000, 0xff, 8) );
	ddrMx31.push_back( MemoryInitCommand(0xB8001000, 0x82226080, 32) );
	ddrMx31.push_back( MemoryInitCommand(0x80000000, 0xDEADBEEF, 32) );

	// MX27 - DDR
	ddrMx27.push_back( MemoryInitCommand(0xd8002000,  0x0000CC03, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xd8002004,  0xa0330D01, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xd8002008,  0x00220800, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xD8001010,  0x00000004, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xD8001004,  0x006AC73A, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xD8001000,  0x92100000, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xA0000400,  0x00000000, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xD8001000,  0xA2100000, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xA0000000,  0x00000000, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xA0000000,  0x00000000, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xD8001000,  0xB2100000, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xA0000033,  0x00, 8) );
	ddrMx27.push_back( MemoryInitCommand(0xA1000000,  0x00, 8) );
	ddrMx27.push_back( MemoryInitCommand(0xD8001000,  0x82226080, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xA0000000,  0x00000000, 32) );
	ddrMx27.push_back( MemoryInitCommand(0xD8001010,  0x0000000C, 32) );

	// MX31 - SDRAM
	sdrMx31.push_back( MemoryInitCommand(0xB8001004, 0x0079E7BA, 32) );
	sdrMx31.push_back( MemoryInitCommand(0xB8001000, 0x92100000, 32) );
	sdrMx31.push_back( MemoryInitCommand(0x80000f00, 0x12344321, 32) );
	sdrMx31.push_back( MemoryInitCommand(0xB8001000, 0xa2100000, 32) );
	sdrMx31.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	sdrMx31.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	sdrMx31.push_back( MemoryInitCommand(0xB8001000, 0xb2100000, 32) );
	sdrMx31.push_back( MemoryInitCommand(0x80000037, 0xda, 8) );
	sdrMx31.push_back( MemoryInitCommand(0x81000000, 0xff, 8) );
	sdrMx31.push_back( MemoryInitCommand(0xB8001000, 0x82126180, 32) );
	sdrMx31.push_back( MemoryInitCommand(0x80000000, 0xDEADBEEF, 32) );

	// MX35 - DDR
	// CS0 settings
//	ddrMx35.push_back( MemoryInitCommand(0xB8002000, 0x0000CC03, 32) );
//	ddrMx35.push_back( MemoryInitCommand(0xB8002004, 0xA0330D01, 32) );
//	ddrMx35.push_back( MemoryInitCommand(0xB8002008, 0x00220800, 32) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001010, 0x00000244, 32) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001004, 0x00795429, 32) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001000, 0x92220000, 32) );
	ddrMx35.push_back( MemoryInitCommand(0x80000400, 0x12344321, 32) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001000, 0xA2220000, 32) );
	ddrMx35.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	ddrMx35.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );

	ddrMx35.push_back( MemoryInitCommand(0xB8001000, 0xb2220000, 32) );
	ddrMx35.push_back( MemoryInitCommand(0x80000033, 0xda, 8) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001000, 0x82226C80, 32) );

	ddrMx35.push_back( MemoryInitCommand(0xB800100C, 0x00795429, 32) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001008, 0x92220000, 32) );
	ddrMx35.push_back( MemoryInitCommand(0x90000400, 0x12344321, 32) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001008, 0xA2220000, 32) );
	ddrMx35.push_back( MemoryInitCommand(0x90000000, 0x12344321, 32) );
	ddrMx35.push_back( MemoryInitCommand(0x90000000, 0x12344321, 32) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001008, 0xb2220000, 32) );
	ddrMx35.push_back( MemoryInitCommand(0x90000033, 0xda, 8) );
	ddrMx35.push_back( MemoryInitCommand(0xB8001008, 0x82226C80, 32) );

	// MX35 - MDDR
	mddrMx35.push_back( MemoryInitCommand(0xB8001010, 0x0000004C, 32) );
	mddrMx35.push_back( MemoryInitCommand(0xB8001004, 0x006ac73a, 32) );
	mddrMx35.push_back( MemoryInitCommand(0xB8001000, 0x92100000, 32) );
	mddrMx35.push_back( MemoryInitCommand(0x80000f00, 0x12344321, 32) );
	mddrMx35.push_back( MemoryInitCommand(0xB8001000, 0xa2100000, 32) );
	mddrMx35.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	mddrMx35.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	mddrMx35.push_back( MemoryInitCommand(0xB8001000, 0xb2100000, 32) );
	mddrMx35.push_back( MemoryInitCommand(0x80000033, 0xda, 8) );
	mddrMx35.push_back( MemoryInitCommand(0x81000000, 0xff, 8) );
	mddrMx35.push_back( MemoryInitCommand(0xB8001000, 0x82226080, 32) );
	mddrMx35.push_back( MemoryInitCommand(0xB8001000, 0x82226007, 32) );
	mddrMx35.push_back( MemoryInitCommand(0x80000000, 0xDEADBEEF, 32) );
	mddrMx35.push_back( MemoryInitCommand(0xB8001010, 0x0000000c, 32) );

	// MX35 - DDR2
/*	ddr2Mx35.push_back( MemoryInitCommand(0xB8001010, 0x00000204, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001004, 0x007fff2f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0x92220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000400, 0x12345678, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0xB2220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x84000000, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x86000000, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x82000400, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000333, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0x92220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000400, 0x12345678, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0xA2220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000000, 0x87654321, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000000, 0x87654321, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0xB2220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000233, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x82000780, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x82000400, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0x82220080, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001008, 0x00002000, 32) );
*/
	// MX35 - DDR2
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC368, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC36C, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC370, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC374, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC378, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC37C, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC380, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC384, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC388, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC38C, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC390, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC394, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC398, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC39C, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3A0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3A4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3A8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3AC, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3B0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3B4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3B8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3BC, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3C0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3C4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3C8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3CC, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3D0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3D4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3D8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3DC, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3E0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3E4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3E8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3EC, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3F0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3F4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3F8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC3FC, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC400, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC404, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC408, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC40C, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC410, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC414, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC418, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC41c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC420, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC424, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC428, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC42c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC430, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC434, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC438, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC43c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC440, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC444, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC448, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC44c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC450, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC454, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC458, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC45c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC460, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC464, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC468, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC46c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC470, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC474, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC478, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC47c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC480, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC484, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC488, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC48c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC490, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC494, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC498, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC49c, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4A0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4A4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4A8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4Ac, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4B0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4B4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4B8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4Bc, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4C0, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4C4, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x43FAC4C8, 0x00000002, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001010, 0x0000030C, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001004, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0x92220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000400, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0xB2220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x84000000, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x86000000, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x82000400, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000333, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0x92220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000400, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0xA2220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000000, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0xB2220000, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0x80000233, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x82000780, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0x82000400, 0xda, 8) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0x82220080, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB800100C, 0x007ffc3f, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001010, 0x00000304, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001000, 0x82228080, 32) );
	ddr2Mx35.push_back( MemoryInitCommand(0xB8001008, 0x00002000, 32) );

	// MX37 - DDR
	ddrMx37.push_back( MemoryInitCommand(0xc3f98000, 0x0030, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9000, 0x80000000, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9014, 0x04008008, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9014, 0x00008010, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9014, 0x00008010, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9014, 0x00338018, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9000, 0xB2220000, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9004, 0x899f6bba, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9010, 0x000a0104, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xe3fd9014, 0x00000000, 32) );

	ddrMx37.push_back( MemoryInitCommand(0xc3fa84fc, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8504, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa848c, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa849c, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8270, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8274, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84f0, 0x0200, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84a8, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84b0, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84b4, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84e0, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8278, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa827c, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8280, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8284, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8288, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa828c, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8290, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8294, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8298, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84f8, 0x0200, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84d4, 0x0, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa829c, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa82a0, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa82a4, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8008, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8230, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84d8, 0x0200, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84c4, 0x0, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84e4, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa800c, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8234, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8010, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8238, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8014, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa823c, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8018, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8240, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa801c, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8244, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8020, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8248, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8024, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa824c, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8028, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8250, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84e8, 0x0200, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa84f4, 0x0004, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa802c, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8254, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8030, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8258, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8034, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa825c, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8038, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8260, 0x0080,32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa803c, 0x1,32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8264, 0x0080,32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8040, 0x1,32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8268, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8044, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa826c, 0x0080, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8048, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa82a8, 0x0204, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa804c, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa82ac, 0x0204, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa805c, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa82bc, 0x02c4, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa8060, 0x1, 32) );
	ddrMx37.push_back( MemoryInitCommand(0xc3fa82c0, 0x02c4, 32) );

	// MX51 - DDR
	//ddrMx51.push_back( MemoryInitCommand({0x73f98000, 0x30, 16) );
	ddrMx51.push_back( MemoryInitCommand(0x73fd4018, 0x0000EBC0, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x73fd4014, 0x012B9145, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9000, 0x80000000, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9014, 0x04008008, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9014, 0x00008010, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9014, 0x00008010, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9014, 0x00338018, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9000, 0xB2220000, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9004, 0x899f6bba, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9010, 0x000a0104, 32) );
	ddrMx51.push_back( MemoryInitCommand(0x83fd9014, 0x00000000, 32) );

	// MX51_TO2 - DDR
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa88a0, 0x0000020,  32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa850c, 0x000020c5, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa8510, 0x000020c5, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa883c, 0x00000002, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa8848, 0x00000002, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84b8, 0x000000e7, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84bc, 0x00000045, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84c0, 0x00000045, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84c4, 0x00000045, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84c8, 0x00000045, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa8820, 0x00000000, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84a4, 0x00000003, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84a8, 0x00000003, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84ac, 0x000000e3, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84b0, 0x000000e3, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84b4, 0x000000e3, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84cc, 0x000000e3, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x73fa84d0, 0x000000e2, 32) );

	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9000, 0x82a20000, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9008, 0x82a20000, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9010, 0x000ad0d0, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9004, 0x333574aa, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd900c, 0x333574aa, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x04008008, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0000801a, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0000801b, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00448019, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x07328018, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x04008008, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00008010, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00008010, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x06328018, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x03808019, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00408019, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00008000, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0400800c, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0000801e, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0000801f, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0000801d, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0732801c, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0400800c, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00008014, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00008014, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0632801c, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0380801d, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x0040801d, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00008004, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9000, 0xb2a20000, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9008, 0xb2a20000, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9010, 0x000ad6d0, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9034, 0x90000000, 32) );
	ddrMx51_To2.push_back( MemoryInitCommand(0x83fd9014, 0x00000000, 32) );

	// MX51_TO2 - MDDR
	mddrMx51_To2.push_back( MemoryInitCommand(0x73FA84B8, 0x000000E7, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9000, 0x83220000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x04008008, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x00008010, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x00008010, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x00338018, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x0020801a, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x00008000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9000, 0xC3220000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9004, 0xC33574AA, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9010, 0x000A1700, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9008, 0x83220000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x0400800C, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x00008014, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x00008014, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x0033801C, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x0020801E, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x00008004, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9008, 0xC3220000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD900C, 0xC33574AA, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9014, 0x00000000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9020, 0x00F68000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9024, 0x00FC8000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9028, 0x00EE8000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD902C, 0x00EF8000, 32) );
	mddrMx51_To2.push_back( MemoryInitCommand(0x83FD9030, 0x00F78000, 32) );

	// MX25 - DDR
	ddrMx25.push_back( MemoryInitCommand(0xB8001004, 0x0076EB3F, 32) );
	ddrMx25.push_back( MemoryInitCommand(0xB8001010, 0x0000004C, 32) );
	ddrMx25.push_back( MemoryInitCommand(0xB8001000, 0x92100000, 32) );
	ddrMx25.push_back( MemoryInitCommand(0x80000f00, 0x12344321, 32) );
	ddrMx25.push_back( MemoryInitCommand(0xB8001000, 0xA2200000, 32) );
	ddrMx25.push_back( MemoryInitCommand(0x80000F00, 0x12344321, 32) );
	ddrMx25.push_back( MemoryInitCommand(0x80000F00, 0x12344321, 32) );
	ddrMx25.push_back( MemoryInitCommand(0xB8001000, 0xb2200000, 32) );
	ddrMx25.push_back( MemoryInitCommand(0x80000033, 0xda, 8) );
	ddrMx25.push_back( MemoryInitCommand(0x81000000, 0xff, 8) );
	ddrMx25.push_back( MemoryInitCommand(0xB8001000, 0x82118485, 32) );

	// MX25 - MDDR
	mddrMx25.push_back( MemoryInitCommand(0xB8001010, 0x00000004, 32) );
	mddrMx25.push_back( MemoryInitCommand(0xB8001000, 0x92100000, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x80000400, 0x12344321, 32) );
	mddrMx25.push_back( MemoryInitCommand(0xB8001000, 0xa2100000, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x80000000, 0x12344321, 32) );
	mddrMx25.push_back( MemoryInitCommand(0xB8001000, 0xb2100000, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x80000033, 0xda, 8) );
	mddrMx25.push_back( MemoryInitCommand(0x81000000, 0xff, 8) );
	mddrMx25.push_back( MemoryInitCommand(0xB8001000, 0x82216080, 32) );
	mddrMx25.push_back( MemoryInitCommand(0xB8001000, 0x82216880, 32) );
	mddrMx25.push_back( MemoryInitCommand(0xB8001004, 0x00295729, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x80000000, 0x0000, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x43F00040, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x43F00044, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x43F00048, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x43F0004C, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x43F00050, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x43F00000, 0x77777777, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x43F00004, 0x77777777, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x53F00040, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x53F00044, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x53F00048, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x53F0004C, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x53F00050, 0x0, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x53F00000, 0x77777777, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x53F00004, 0x77777777, 32) );
	mddrMx25.push_back( MemoryInitCommand(0x53f80064, 0x43300000, 32) );

	// MX25_TO11 - DDR2
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8002050, 0x0000D843, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8002054, 0x22252521, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8002058, 0x22220A00, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8001004, 0x0076E83a, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8001010, 0x00000204, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8001000, 0x92210000, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x80000f00, 0x12344321, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8001000, 0xB2210000, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x82000000, 0xda, 8) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x83000000, 0xda, 8) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x81000400, 0xda, 8) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x80000333, 0xda, 8) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8001000, 0x92210000, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x80000400, 0x12345678, 8) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8001000, 0xA2210000, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x80000000, 0x87654321, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x80000000, 0x87654321, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8001000, 0xB2210000, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x80000233, 0xda, 8) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x81000780, 0xda, 8) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x81000400, 0xda, 8) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0xB8001000, 0x82216080, 32) );
	ddr2Mx25_To11.push_back( MemoryInitCommand(0x43FAC454, 0x00001000, 32) );
}

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
