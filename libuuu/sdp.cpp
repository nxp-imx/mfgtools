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
#include "trans.h"

#include <cstring>

int SDPCmdBase::check_ack(HIDReport *report, uint32_t ack)
{
	if (get_hab_type(report) == HabUnknown)
		return -1;

	uint32_t status;
	if (get_status(report, status, 4))
		return -1;

	if (ack != status)
	{
		set_last_err_string("Status Miss matched");
		return -1;
	}
	return 0;
}

SDPCmdBase::HAB_t SDPCmdBase::get_hab_type(HIDReport *report)
{
	uint32_t status;
	if (get_status(report, status, 3))
		return HabUnknown;

	if (status == HabEnabled)
		return HabEnabled;

	if (status == HabDisabled)
		return HabDisabled;

	set_last_err_string("unknown hab type");
	return HabUnknown;
}

int SDPCmdBase::get_status(HIDReport *p, uint32_t &status, uint8_t report_id)
{
	m_input.resize(1025);
	m_input[0] = report_id;
	int ret = p->read(m_input);
	if (ret < 0)
		return -1;

	if (m_input.size() < (1 + sizeof(uint32_t)))
	{
		set_last_err_string("HID report size is too small");
		return -1;
	}

	status = *(uint32_t*)(m_input.data() + 1);
	return 0;
}

int SDPCmdBase::init_cmd()
{
	memset(&m_spdcmd, 0, sizeof(m_spdcmd)); return 0;
}

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
	return nullptr;
}

int SDPCmdBase::send_cmd(HIDReport *p)
{
	return p->write(&m_spdcmd, sizeof(m_spdcmd), 1);
}

SDPDcdCmd::SDPDcdCmd(char *p) : SDPCmdBase(p)
{
	insert_param_info("dcd", nullptr, Param::Type::e_null);
	insert_param_info("-f", &m_filename, Param::Type::e_string_filename);
	insert_param_info("-dcdaddr", &m_dcd_addr, Param::Type::e_uint32);
	m_dcd_addr = 0;
}

int SDPDcdCmd::run(CmdCtx*ctx)
{
	const ROM_INFO * rom = search_rom_info(ctx->m_config_item);
	if (rom == nullptr)
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
	if (pIVT == nullptr)
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

SDPSkipDCDCmd::SDPSkipDCDCmd(char *p) : SDPCmdBase(p)
{
	m_spdcmd.m_cmd = ROM_KERNEL_CMD_SKIP_DCD_HEADER;
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

SDPBootCmd::SDPBootCmd(char *p) : SDPCmdBase(p)
{
	insert_param_info("boot", nullptr, Param::Type::e_null);
	insert_param_info("-f", &m_filename, Param::Type::e_string_filename);
	insert_param_info("-nojump", &m_nojump, Param::Type::e_bool);
	insert_param_info("-cleardcd", &m_clear_dcd, Param::Type::e_bool);
	insert_param_info("-dcdaddr", &m_dcd_addr, Param::Type::e_uint32);
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

	SDPBootlogCmd log(nullptr);
	log.run(ctx);

	return 0;
}

SDPStatusCmd::SDPStatusCmd(char *p) : SDPCmdBase(p)
{
	m_spdcmd.m_cmd = ROM_KERNEL_CMD_ERROR_STATUS;
	insert_param_info("status", nullptr, Param::Type::e_null);
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

SDPWriteCmd::SDPWriteCmd(char *p) : SDPCmdBase(p)
{
	m_spdcmd.m_cmd = ROM_KERNEL_CMD_WR_FILE;
	m_PlugIn = -1;
	m_Ivt = -1;
	m_max_download_pre_cmd = 0x200000;
	m_offset = 0;
	m_bIvtReserve = false;
	m_download_addr = 0;
	m_bskipspl = false;

	insert_param_info("write", nullptr, Param::Type::e_null);
	insert_param_info("-f", &m_filename, Param::Type::e_string_filename);
	insert_param_info("-ivt", &m_Ivt, Param::Type::e_uint32);
	insert_param_info("-addr", &m_download_addr, Param::Type::e_uint32);
	insert_param_info("-offset", &m_offset, Param::Type::e_uint32);
	insert_param_info("-skipspl", &m_bskipspl, Param::Type::e_bool);
	insert_param_info("-skipfhdr", &m_bskipfhdr, Param::Type::e_bool);
}

int SDPWriteCmd::run(CmdCtx*ctx)
{
	size_t size;
	uint8_t *pbuff;
	int offset = 0;

	shared_ptr<FileBuffer> fbuff = get_file_buffer(m_filename);

	if (fbuff == nullptr)
		return -1;

	if (m_Ivt < 0)
	{
		pbuff = fbuff->data();
		size = fbuff->size();

		offset = m_offset;

		if (m_bskipfhdr)
			offset += GetFlashHeaderSize(fbuff, offset);

		if (m_bskipspl) {
			const ROM_INFO * rom = search_rom_info(ctx->m_config_item);
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
		if (pIvt == nullptr)
		{
			set_last_err_string("Cannot find valid IVT header");
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

	report.set_notify_total(size);

	const ROM_INFO * rom = search_rom_info(ctx->m_config_item);

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

		report.set_position_base(i);
		report.set_skip_notify(true);

		if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
			return -1;

		report.set_skip_notify(false);

		if (report.write(((uint8_t*)pbuff)+i, sz, 2))
			return -1;

		if (check_ack(&report, ROM_STATUS_ACK))
			return -1;
	}

	return 0;
}

SDPReadMemCmd::SDPReadMemCmd(char *p) : SDPCmdBase(p)
{
	m_spdcmd.m_cmd = ROM_KERNEL_CMD_RD_MEM;

	insert_param_info("rdmem", nullptr, Param::Type::e_null);
	insert_param_info("-addr", &m_mem_addr, Param::Type::e_uint32);
	insert_param_info("-format", &m_mem_format, Param::Type::e_uint32);
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

SDPWriteMemCmd::SDPWriteMemCmd(char *p) : SDPCmdBase(p)
{
	m_spdcmd.m_cmd = ROM_KERNEL_CMD_WR_MEM;

	insert_param_info("wrmem", nullptr, Param::Type::e_null);
	insert_param_info("-addr", &m_mem_addr, Param::Type::e_uint32);
	insert_param_info("-format", &m_mem_format, Param::Type::e_uint32);
	insert_param_info("-value", &m_mem_value, Param::Type::e_uint32);
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

SDPJumpCmd::SDPJumpCmd(char *p) : SDPCmdBase(p)
{
	m_spdcmd.m_cmd = ROM_KERNEL_CMD_JUMP_ADDR;
	insert_param_info("jump", nullptr, Param::Type::e_null);
	insert_param_info("-f", &m_filename, Param::Type::e_string_filename);
	insert_param_info("-ivt", &m_Ivt, Param::Type::e_bool);
	insert_param_info("-plugin", &m_Ivt, Param::Type::e_bool);
	insert_param_info("-addr", &m_jump_addr, Param::Type::e_uint32);
	insert_param_info("-cleardcd", &m_clear_dcd, Param::Type::e_bool);
}

int SDPJumpCmd::run(CmdCtx *ctx)
{
	const ROM_INFO * rom = search_rom_info(ctx->m_config_item);

	HIDTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	HIDReport report(&dev);

	if (rom == nullptr)
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
		SDPSkipDCDCmd skipcmd(nullptr);
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

		SDPWriteCmd writecmd(nullptr);
		if(writecmd.run(ctx, header, ivt.size(), pIVT->SelfAddr))
			return -1;
	}

	if (report.write(&m_spdcmd, sizeof(m_spdcmd), 1))
		return -1;

	//Omit last return value.
	check_ack(&report, ROM_OK_ACK);

	return 0;
}

SDPBootlogCmd::SDPBootlogCmd(char *p) : SDPCmdBase(p)
{
	insert_param_info("blog", nullptr, Param::Type::e_null);
}

int SDPBootlogCmd::run(CmdCtx *ctx)
{
	HIDTrans dev{2000};

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
