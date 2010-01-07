#pragma once
#include "stdafx.h"
#include "MxRomDevice.h"
#include "DeviceManager.h"
#include <sys/stat.h>

#include "../MxLib/status_strings.h"
#include "../MxLib/ADSTkConfigure.h"
#include "../MxLib/AtkHostApiClass.h"

typedef struct _ADSToolkitApp
{
	AtkHostApiClass objHostApiClass;
	CADSTkConfigure atkConfigure;
}ADSToolkitApp;

ADSToolkitApp theApp;
AtkHostApiClass apiClass;

MxRomDevice::MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
	: Device(deviceClass, devInst, path)
{
    strcpy_s(&DefaultLicenseString[0], 128, "6c3cd57876b3a6e415f93fd697134bd97ee1ec50.Motorola Semiconductors");
}

void MxRomDevice::SetIMXDevPara(CString cMXType, CString cSecurity, CString cRAMType, unsigned int RAMKNLAddr)
{
	theApp.atkConfigure.SetChannel(USB, 0, 1);
	theApp.objHostApiClass.SetUpChannal(USB, DEFAULT_USB_ID);

	if(cMXType ==  _T("25_TO1"))
		theApp.atkConfigure.SetMXType(MX_MX25_TO1);
	else if(cMXType ==  _T("25_TO11"))
		theApp.atkConfigure.SetMXType(MX_MX25_TO11);
	else if(cMXType ==  _T("31_TO1"))
		theApp.atkConfigure.SetMXType(MX_MX31_TO1);
	else if(cMXType ==  _T("31_TO2"))
		theApp.atkConfigure.SetMXType(MX_MX31_TO2);
	else if(cMXType ==  _T("35_TO1"))
		theApp.atkConfigure.SetMXType(MX_MX35_TO1);
	else if(cMXType ==  _T("35_TO2"))
		theApp.atkConfigure.SetMXType(MX_MX35_TO2);
	else if(cMXType ==  _T("37"))
		theApp.atkConfigure.SetMXType(MX_MX37);
	else if(cMXType ==  _T("51_TO2"))
		theApp.atkConfigure.SetMXType(MX_MX51_TO2);


	if(cSecurity == _T("true"))
	{
		theApp.atkConfigure.SetSecurity(TRUE);
	}
	else
	{
		theApp.atkConfigure.SetSecurity(FALSE);
	}

	if(cRAMType == _T("DDR"))
		theApp.atkConfigure.SetRAMType(MM_DDR);
	else if(cRAMType == _T("SDRAM"))
		theApp.atkConfigure.SetRAMType(MM_SDRAM);
	else if(cRAMType == _T("DDR2"))
		theApp.atkConfigure.SetRAMType(MM_DDR2);
	else if(cRAMType == _T("MDDR"))
		theApp.atkConfigure.SetRAMType(MM_MDDR);
	else if(cRAMType == _T("CUSTOM"))
		theApp.atkConfigure.SetRAMType(MM_CUSTOM);

		theApp.atkConfigure.SetChannel(USB, 0, 1);
		//theApp.atkConfigure.SetSecurity(Security);
		theApp.atkConfigure.SetMemoryAddr(mm_addrs[theApp.atkConfigure.GetMXType()]);
		//theApp.atkConfigure.SetRAMType(RAMType);
		theApp.objHostApiClass.SetUsbTimeout(USB_OPEN_USR_TIMEOUT,USB_TRANS_USR_TIMEOUT);
		TRACE("Set the usb timeout open %d secs, trans %d msecs\n",USB_OPEN_USR_TIMEOUT,USB_TRANS_USR_TIMEOUT);
		
		// Check the memory initial script
		switch (theApp.atkConfigure.GetRAMType()) 
		{
			case MM_DDR:
				TRACE("setup the DDR memory init script!\n");
				theApp.atkConfigure.SetMemoryType(mmInitScripts[theApp.atkConfigure.GetMXType()][0].script, 
										mmInitScripts[theApp.atkConfigure.GetMXType()][0].lines);
				break;
			case MM_SDRAM:
				TRACE("setup the SDRAM memory init script!\n");
				theApp.atkConfigure.SetMemoryType(mmInitScripts[theApp.atkConfigure.GetMXType()][1].script, 
										mmInitScripts[theApp.atkConfigure.GetMXType()][1].lines);
				break;
			case MM_DDR2:
				TRACE("setup the DDR2 memory init script!\n");
				theApp.atkConfigure.SetMemoryType(mmInitScripts[theApp.atkConfigure.GetMXType()][0].script, 
										mmInitScripts[theApp.atkConfigure.GetMXType()][0].lines);
				break;
			case MM_MDDR:
				TRACE("setup the MDDR memory init script!\n");
				theApp.atkConfigure.SetMemoryType(mmInitScripts[theApp.atkConfigure.GetMXType()][1].script, 
										mmInitScripts[theApp.atkConfigure.GetMXType()][1].lines);
				break;
			case MM_CUSTOM:
			{
				break;
			}
			default:
				TRACE("Impossible, never reach here!\n");
				break;
		}
	theApp.atkConfigure.SetRAMKNLAddr(RAMKNLAddr);
}

BOOL MxRomDevice::OpenUSBPort()
{
		if(!apiClass.OpenUSB(theApp.atkConfigure.GetMXType()))
			return FALSE;

		USB_DEVICE_DESCRIPTOR Descriptor;
		if(!apiClass.GetDeviceDescriptor(&Descriptor))
			return FALSE;

		return TRUE;
}

BOOL MxRomDevice::InitMemoryDevice()
{
	stMemoryInit *pScript;
	int lines;
	
	pScript = theApp.atkConfigure.GetMemoryInitScript(&lines);
	if (pScript == NULL || lines == 0)
	{
		TRACE("Strange! never reach here in InitMemoryDevice\n");
		return FALSE;
	}
	
	for (int i = 0; i < lines; i++)
	{
		// TODO: the host dll MUST be modify for the mx type
		if (!theApp.objHostApiClass.WriteMemory(theApp.atkConfigure.GetMXType(),
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

	OpenUSBPort();

	if (theApp.objHostApiClass.GetHABType(theApp.atkConfigure.GetMXType()))
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
	
	theApp.atkConfigure.GetChannel(&cType, &cId, &cHandle);
	// set the communication mode through write memory interace
	if (!theApp.objHostApiClass.WriteMemory(theApp.atkConfigure.GetMXType(),
		theApp.atkConfigure.GetMemoryAddr(MEMORY_START_ADDR), 
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
	PhyRAMAddr4KRL = theApp.atkConfigure.GetMemoryAddr(MEMORY_START_ADDR) | \
		(theApp.atkConfigure.GetRAMKNLAddr() & (~0xF0000000));
	status = theApp.objHostApiClass.DownloadImage(PhyRAMAddr4KRL, rklsize, rkl);
	
	if (!status) {
		TRACE("Can not load RAM Kernel to device\n");
		return FALSE;
	} else {		
		// Send the complete command to the device 
		status = theApp.objHostApiClass.Jump2Rak(theApp.atkConfigure.GetMXType(), is_hab_prod);

		// check the status
		if (!status) {
			TRACE("Can not execute RAM Kernel\n");
			return FALSE;
		}
	}
	apiClass.CloseUSB();
	return TRUE;
}