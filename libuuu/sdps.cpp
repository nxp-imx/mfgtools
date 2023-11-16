/*
* Copyright 2018 NXP.
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
* Neither the name of the NXP Semiconductor nor the names of its
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
#include <string>
#include "sdps.h"
#include "hidreport.h"
#include "liberror.h"
#include "libcomm.h"
#include "buffer.h"
#include "sdp.h"
#include "trans.h"
#include <libusb.h>

#include <cstring>

//------------------------------------------------------------------------------
// HID Command Block Wrapper (CBW)
//------------------------------------------------------------------------------
#pragma pack (1)

typedef struct _CDBHIDDOWNLOAD {
	uint8_t	Command;
	uint32_t	Length;
	uint8_t	Reserved[11];
} CDBHIDDOWNLOAD, *PCDBHIDDOWNLOAD;


struct _ST_HID_CBW
{
	uint32_t Signature;        // Signature: 0x43544C42:1129598018, o "BLTC" (little endian) for the BLTC CBW
	uint32_t Tag;              // Tag: to be returned in the csw
	uint32_t XferLength;       // XferLength: number of bytes to transfer
	uint8_t Flags;            // Flags:
	//   Bit 7: direction - device shall ignore this bit if the
	//     XferLength field is zero, otherwise:
	//     0 = data-out from the host to the device,
	//     1 = data-in from the device to the host.
	//   Bits 6..0: reserved - shall be zero.
	uint8_t Reserved[2];       // Reserved - shall be zero.
	CDBHIDDOWNLOAD Cdb;        // cdb: the command descriptor block
};

#define BLTC_DOWNLOAD_FW					2
#define HID_BLTC_REPORT_TYPE_DATA_OUT		2
#define HID_BLTC_REPORT_TYPE_COMMAND_OUT	1

#define CBW_BLTC_SIGNATURE  0x43544C42; // "BLTC" (little endian)
#define CBW_PITC_SIGNATURE  0x43544950; // "PITC" (little endian)
// Flags values for _ST_HID_CBW
#define CBW_DEVICE_TO_HOST_DIR 0x80; // "Data Out"
#define CBW_HOST_TO_DEVICE_DIR 0x00; // "Data In"

#pragma pack ()

#include "rominfo.h"

static bool is_ivt_barker_header(shared_ptr<DataBuffer> data, size_t off)
{
	if (off + sizeof(IvtHeader) > data->size())
		return false;

	IvtHeader *p = (IvtHeader*)(data->data() + off);
	if (p->IvtBarker == IVT_BARKER_HEADER || p->IvtBarker == IVT_BARKER2_HEADER)
		return true;

	return false;
}

int SDPSCmd::run(CmdCtx *pro)
{
	const ROM_INFO * rom = search_rom_info(pro->m_config_item);
	if (rom == nullptr)
	{
		string_ex err;
		err.format("%s:%d can't get rom info", __FUNCTION__, __LINE__);
		set_last_err_string(err);
		return -1;
	}

	HIDTrans dev(m_timeout);
	if (rom->flags & ROM_INFO_HID_EP1)
		dev.set_hid_out_ep(1);

	if(dev.open(pro->m_dev))
		return -1;

	shared_ptr<FileBuffer> p1 = get_file_buffer(m_filename, true);
	if (!p1)
		return -1;

	shared_ptr<DataBuffer> p;
	HIDReport report(&dev);
	report.set_skip_notify(false);

	size_t offset = m_offset;

	vector<uint8_t> buff;

	if (m_bscanterm)
	{
		p = p1->request_data(0, m_scan_limited);
		if (!p)
			return -1;

		if (IsMBR(p))
		{
			size_t pos = 0, length;
			length = ScanTerm(p, pos, 512, m_scan_limited);
			if (length == 0)
			{
				set_last_err_string("This wic have NOT terminate tag after bootloader, please use new yocto");
				return -1;
			}

			offset = pos - length;
			if (ssize_t(offset) < 0)
			{
				set_last_err_string("This wic boot length is wrong");
				return -1;
			}
			p->resize(pos);
		}
	}
	else
	{
		p = p1->request_data(0, SIZE_MAX); //request all data
		if (!p) return -1;
	}

	if (m_bskipflashheader)
		offset += GetFlashHeaderSize(p, offset);

	// Detect barebox binaries that have the IVT header at offset 32K
	if (!is_ivt_barker_header(p, offset) &&
	     is_ivt_barker_header(p, offset + 0x8000))
		offset += 0x8000;

	if (offset >= p->size())
	{
		set_last_err_string("Offset bigger than file size");
		return -1;
	}

	size_t sz = GetContainerActualSize(p, offset, rom->flags & ROM_INFO_HID_ROMAPI);

	if (!(rom->flags & ROM_INFO_HID_NO_CMD))
	{
		_ST_HID_CBW	cbw;
		uint32_t	length = (uint32_t) sz;

		memset(&cbw, 0, sizeof(_ST_HID_CBW));
		cbw.Cdb.Command = BLTC_DOWNLOAD_FW;
		cbw.Cdb.Length = EndianSwap(length);

		++cbw.Tag;
		cbw.Signature = CBW_BLTC_SIGNATURE;
		cbw.XferLength = (uint32_t)length;
		cbw.Flags = CBW_HOST_TO_DEVICE_DIR;

		int ret = report.write(&cbw, sizeof(_ST_HID_CBW), HID_BLTC_REPORT_TYPE_COMMAND_OUT);
		if (ret)
			return ret;
	}

	if (rom->flags & ROM_INFO_HID_PACK_SIZE_1020)
		report.set_out_package_size(1020);

	int ret = report.write(p->data() + offset, sz,  2);

	if (ret ==  0)
	{
		SDPBootlogCmd log(nullptr);
		log.run(pro);
	}

	return ret;
}
