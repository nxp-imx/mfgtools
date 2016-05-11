/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once
#include "UpdateTransportProtocol.Api.h"
#include "Volume.h"
#pragma pack (push, 1)
struct CDB_UTP
{
	uint8_t UTPOpcode;
	uint8_t UTPMessage;
	uint32_t UTPTag;
	uint32_t UTPParam1;
	uint32_t UTPParam2;
	uint16_t SCSIExtra;
};
struct CBW_UTP
{
	//CBW
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	//CDB
	CDB_UTP CDB;
};
struct CSW_UTP
{
	uint8_t dCBWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
};
struct SENSE_UTP
{
	uint8_t UTPResponseCode;
	uint8_t UTPSpec;
	uint8_t UTPSenseKey;
	uint32_t UTPReplyInfoLower;
	uint8_t UTPSenseLength;
	uint32_t UTPReplyInfoUpper;
	uint16_t UTPSenseCode;
	uint32_t SCSIExtra;
};
#pragma pack (pop)
class libusbVolume : public Volume
{
	public:
		libusbVolume(DeviceClass* deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
		int pollDevice(CSW_UTP* sResponse, StApi& api);
		UINT ReinitializeUSBHandle(CBW_UTP response,StApi& api);
		UINT SendCommand(StApi& api, UCHAR* additionalInfo);
		UINT SendCommand(HANDLE hDrive, StApi& api, UCHAR* additionalInfo, NotifyStruct& nsInfo);
};
