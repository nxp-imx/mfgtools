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
#include "rominfo.h"
#include "libusb.h"

IvtHeader *SDPCmdBase::search_ivt_header(shared_ptr<FileBuffer> data, size_t &off, size_t limit)
{
	if (limit >= data->size())
		limit = data->size();

	for (; off < limit; off += 0x100)
	{
		IvtHeader *p = (IvtHeader*)(data->data() + off);
		if (p->IvtBarker == IVT_BARKER_HEADER)
			return p;
		if (p->IvtBarker == IVT_BARKER2_HEADER)
		{
			BootData *pDB = (BootData *) &(data->at(off + p->BootData - p->SelfAddr));

			/*Skip HDMI firmware for i.MX8MQ*/
			if (pDB->PluginFlag & 0xFFFFFFFE)
				continue;
			return p;
		}
	}
	off = -1;
	return NULL;
}

int SDPDcdCmd::run(CmdCtx*ctx)
{
	ROM_INFO * rom;
	rom = search_rom_info(ctx->m_config_item);
	if (rom == NULL)
	{
		string_ex err;
		err.format("%s:%d can't get rom info", __FUNCTION__, __LINE__);
		set_last_err_string(err);
		return -1;
	}
	init_cmd();

	shared_ptr<FileBuffer> buff = get_file_buffer(m_filename);

	size_t off = 0;
	IvtHeader *pIVT = search_ivt_header(buff, off);
	if (pIVT == NULL)
	{
		return 0;
	}

	if (pIVT->DCDAddress == 0)
		return 0;

	uint8_t * pdcd = &(buff->at(off + pIVT->DCDAddress - pIVT->SelfAddr));

	if (pdcd[0] != HAB_TAG_DCD)
	{
		string_ex err;
		err.format("%s:%d DCD TAG miss matched", __FUNCTION__, __LINE__);
		set_last_err_string(err);
		return -1;
	}

	uint32_t size = (pdcd[1] << 8) | pdcd[2];

	m_spdcmd.m_cmd = ROM_KERNEL_CMD_DCD_WRITE;
	m_spdcmd.m_addr = EndianSwap(m_dcd_addr ? m_dcd_addr : rom->free_addr);
	m_spdcmd.m_count = EndianSwap(size);

	HIDTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);
	if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
		return -1;

	if (report.write(pdcd, size, 2))
		return -1;

	if (check_ack(&report, ROM_WRITE_ACK))
		return -1;

	return 0;
}

int SDPSkipDCDCmd::run(CmdCtx*ctx)
{
	HIDTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);
	if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
		return -1;

	if (check_ack(&report, ROM_OK_ACK))
		return -1;

	return 0;
}

int SDPBootCmd::run(CmdCtx *ctx)
{
	string str;
	str = "SDP: dcd -f ";
	str += m_filename;
	if (m_dcd_addr) {
		str += " -dcdaddr ";
		str += std::to_string(m_dcd_addr);
	}
	SDPDcdCmd dcd((char *)str.c_str());
	if (dcd.parser()) return -1;
	if (dcd.run(ctx)) return -1;

	str = "SDP: write -f ";
	str += m_filename;
	str += " -ivt 0";
	SDPWriteCmd wr((char *)str.c_str());
	if (wr.parser()) return -1;
	if (wr.run(ctx)) return -1;

	str = "SDP: jump -f ";
	str += m_filename;
	str += " -ivt";
	if (m_clear_dcd)
		str += " -cleardcd";

	SDPJumpCmd jmp((char *)str.c_str());
	if (!m_nojump)
	{
		if (jmp.parser()) return -1;
		if (jmp.run(ctx)) return -1;
	}

	SDPBootlogCmd log(NULL);
	log.run(ctx);

	return 0;
}

int SDPStatusCmd::run(CmdCtx *ctx)
{
	HIDTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);
	if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
		return -1;

	if (get_hab_type(&report) == HabUnknown)
		return -1;

	uint32_t status;
	if (get_status(&report, status, 4))
		return -1;

	return 0;
}

int SDPWriteCmd::run(CmdCtx*ctx)
{
	size_t size;
	uint8_t *pbuff;
	int offset = 0;

	shared_ptr<FileBuffer> fbuff = get_file_buffer(m_filename);

	if (fbuff == NULL)
		return -1;

	if (m_Ivt < 0)
	{
		pbuff = fbuff->data();
		size = fbuff->size();

		offset = m_offset;

		if (m_bskipfhdr)
			offset += GetFlashHeaderSize(fbuff, offset);

		if (m_bskipspl) {
			ROM_INFO * rom;
			rom = search_rom_info(ctx->m_config_item);
			if(! (rom->flags & ROM_INFO_AUTO_SCAN_UBOOT_POS))
			{
				set_last_err_string("SPL doesn't support auto scan uboot position");
				return -1;
			}

			size_t off = offset;
			IvtHeader *pIvt = search_ivt_header(fbuff, off, 0x100000);
			if (pIvt)
			{
				BootData *pDB = (BootData *) &(fbuff->at(off + pIvt->BootData - pIvt->SelfAddr));
				offset = off + pDB->ImageSize - (pIvt->SelfAddr - pDB->ImageStartAddr);
			}
			else
			{
				offset += GetContainerActualSize(fbuff, offset);
			}

			if (offset >= fbuff->size())
			{
				set_last_err_string("Unknown Image type, can't use skipspl format");
				return -1;
			}
		}

		size -= offset;
	}
	else
	{
		size_t off = 0;
		IvtHeader *pIvt = search_ivt_header(fbuff, off);
		for (int i = 0; i < m_Ivt; i++)
		{
			off += 0x100;
			pIvt = search_ivt_header(fbuff, off);
		}
		if (pIvt == NULL)
		{
			set_last_err_string("Can find validate IVT header");
			return -1;
		}

		BootData *pDB = (BootData *) &(fbuff->at(off + pIvt->BootData - pIvt->SelfAddr));

		m_download_addr = pIvt->SelfAddr;
		//size = fbuff->size() - off;
		size = pDB->ImageSize - (pIvt->SelfAddr - pDB->ImageStartAddr);

		//ImageSize may be bigger than Imagesize because ImageSize include IVT offset
		//Difference boot storage have difference IVT offset. 
		if (size > fbuff->size() - off)
			size = fbuff->size() - off;

		pbuff = (uint8_t*)pIvt;
	}
	return run(ctx, pbuff + offset, size, m_download_addr);
}

int SDPWriteCmd::run(CmdCtx *ctx, void *pbuff, size_t size, uint32_t addr)
{
	HIDTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);

	report.m_notify_total = size;

	ROM_INFO * rom;
	rom = search_rom_info(ctx->m_config_item);

	size_t max = m_max_download_pre_cmd;

	/* SPL needn't split transfer */
	if (rom && (rom ->flags & ROM_INFO_HID_SDP_NO_MAX_PER_TRANS))
		max = size;

	for (size_t i=0; i < size; i += max)
	{
		size_t sz;
		sz = size - i;
		if (sz > max)
			sz = max;

		m_spdcmd.m_addr = EndianSwap((uint32_t)(addr + i)); // force use 32bit endian swap function;
		m_spdcmd.m_count = EndianSwap((uint32_t)sz); //force use 32bit endian swap function;

		report.m_postion_base = i;
		report.m_skip_notify = true;

		if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
			return -1;

		report.m_skip_notify = false;

		if (report.write(((uint8_t*)pbuff)+i, sz, 2))
			return -1;

		if (check_ack(&report, ROM_STATUS_ACK))
			return -1;
	}

	return 0;
}

int SDPReadMemCmd::run(CmdCtx *ctx)
{
	HIDTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);

	printf("\nReading address 0x%08X ...\n", m_mem_addr);
	m_spdcmd.m_addr = EndianSwap(m_mem_addr);
	m_spdcmd.m_format = m_mem_format;
	switch (m_mem_format) {
		case 0x8:
			m_spdcmd.m_count = EndianSwap((uint32_t)0x1);
			break;
		case 0x10:
			m_spdcmd.m_count = EndianSwap((uint32_t)0x2);
			break;
		case 0x20:
			m_spdcmd.m_count = EndianSwap((uint32_t)0x4);
			break;
		default:
			set_last_err_string("Invalid format, use <8|16|32>");
			return -1;
			break;
	}

	if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
		return -1;

	if (get_hab_type(&report) == HabUnknown)
		return -1;

	uint32_t mem_value;
	if (get_status(&report, mem_value, 4) == 0)
	{
		printf("\nValue of address 0x%08X: ", m_mem_addr);
		switch (m_mem_format) {
			case 0x8:
				printf("0x%02X\n", mem_value & 0xff);
				break;
			case 0x10:
				printf("0x%04X\n", mem_value & 0xffff);
				break;
			case 0x20:
				printf("0x%08X\n", mem_value);
				break;
			default:
				set_last_err_string("Invalid format, use <8|16|32>");
				return -1;
		}
	}

	return 0;
}

int SDPWriteMemCmd::run(CmdCtx *ctx)
{
	HIDTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);

	printf("\nWriting 0x%08X to address 0x%08X ...\n", m_mem_value, m_mem_addr);
	m_spdcmd.m_addr = EndianSwap(m_mem_addr);
	m_spdcmd.m_format = m_mem_format;
	switch (m_mem_format) {
		case 0x8:
			m_spdcmd.m_count = EndianSwap((uint32_t)0x1);
			break;
		case 0x10:
			m_spdcmd.m_count = EndianSwap((uint32_t)0x2);
			break;
		case 0x20:
			m_spdcmd.m_count = EndianSwap((uint32_t)0x4);
			break;
		default:
			set_last_err_string("Invalid format, use <8|16|32>");
			return -1;
			break;
	}
	m_spdcmd.m_data = EndianSwap(m_mem_value);

	if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
		return -1;

	if (get_hab_type(&report) == HabUnknown)
		return -1;

	uint32_t status;

	if (get_status(&report, status, 4) < 0 || status != ROM_WRITE_ACK) {

		string_ex err;
		err.format("%s:%d Failed to write to address 0x%X",
				__FUNCTION__, __LINE__, m_mem_addr);
		set_last_err_string(err);
	}

	return 0;
}

int SDPJumpCmd::run(CmdCtx *ctx)
{
	ROM_INFO * rom;
	rom = search_rom_info(ctx->m_config_item);

	HIDTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);

	if (rom == NULL)
	{
		string_ex err;
		err.format("%s:%d can't get rom info", __FUNCTION__, __LINE__);
		set_last_err_string(err);
		return -1;
	}

	if (rom->flags & ROM_INFO_SPL_JUMP)
	{
		m_spdcmd.m_addr = EndianSwap(m_jump_addr);
		if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
			return -1;

		//Omit last return value.
		check_ack(&report, ROM_OK_ACK);
		return 0;
	}

	shared_ptr<FileBuffer> buff = get_file_buffer(m_filename);

	size_t off = 0;
	IvtHeader *pIVT = search_ivt_header(buff, off);

	m_spdcmd.m_addr = EndianSwap(pIVT->SelfAddr);


	if (rom->flags & ROM_INFO_HID_SKIP_DCD && !m_clear_dcd)
	{
		SDPSkipDCDCmd skipcmd(NULL);
		if (skipcmd.run(ctx))
			return -1;
	}
	else
	{	/*Clear DCD*/
		vector<uint8_t> ivt;
		/* Need send out whole report size buffer avoid overwrite other data
		 * Some platform require receive whole package for report id = 2
		 */
		ivt.resize(report.get_out_package_size());

		size_t sz = buff->size();
		sz -= (uint8_t*)pIVT - (uint8_t*)buff->data();

		if (sz > ivt.size())
			sz = ivt.size();

		memcpy(ivt.data(), pIVT, sz);

		IvtHeader *header = (IvtHeader *)ivt.data();
		header->DCDAddress = 0;

		SDPWriteCmd writecmd(NULL);
		if(writecmd.run(ctx, header, ivt.size(), pIVT->SelfAddr))
			return -1;
	}

	if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
		return -1;

	//Omit last return value.
	check_ack(&report, ROM_OK_ACK);

	return 0;
}

int SDPBootlogCmd::run(CmdCtx *ctx)
{
	HIDTrans dev;
	dev.m_read_timeout = 2000;

	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);

	vector<uint8_t> v(65);
	v[0] = 'I';

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_INFO;
	
	int ret;
	while (1)
	{
		ret = report.read(v);
		if (ret)
			return 0;
		else
		{
			nt.str = (char*)(v.data() + 4);
			v[5] = 0;
			call_notify(nt);
			continue;
		}
	}
	return 0;
}
