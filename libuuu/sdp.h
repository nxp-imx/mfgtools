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

#pragma once

#include "cmd.h"
#include "trans.h"
#include "hidreport.h"

#pragma pack (1)
struct SDPCmd {
	uint16_t m_cmd;
	uint32_t m_addr;
	uint8_t  m_format;
	uint32_t m_count;
	uint32_t m_data;
	uint8_t  m_rsvd;
};

struct IvtHeader
{
	uint32_t IvtBarker;
	uint32_t ImageStartAddr;
	uint32_t Reserved;
	uint32_t DCDAddress;
	uint32_t BootData;
	uint32_t SelfAddr;
	uint32_t Reserved2[2];
};

struct BootData
{
	uint32_t ImageStartAddr;
	uint32_t ImageSize;
	uint32_t PluginFlag;
};

#pragma pack ()

#define ROM_KERNEL_CMD_WR_MEM							0x0202
#define ROM_KERNEL_CMD_WR_FILE							0x0404
#define ROM_KERNEL_CMD_ERROR_STATUS				0x0505
#define RAM_KERNEL_CMD_HEADER							0x0606
//#define ROM_KERNEL_CMD_RE_ENUM 0x0909
#define ROM_KERNEL_CMD_DCD_WRITE					0x0A0A
#define ROM_KERNEL_CMD_JUMP_ADDR					0x0B0B
#define ROM_KERNEL_CMD_SKIP_DCD_HEADER				0x0C0C

#define MAX_DCD_WRITE_REG_CNT		85
#define ROM_WRITE_ACK						0x128A8A12
#define ROM_STATUS_ACK					0x88888888
#define ROM_OK_ACK						0x900DD009

#define IVT_BARKER_HEADER				0x402000D1
#define IVT_BARKER2_HEADER				0x412000D1

#define HAB_TAG_DCD							0xd2       /**< Device Configuration Data */

#define ROM_WRITE_ACK						0x128A8A12
#define ROM_STATUS_ACK					0x88888888
#define ROM_OK_ACK						0x900DD009

class SDPCmdBase:public CmdBase
{
public:

	enum HAB_t
	{
		HabUnknown = -1,
		HabEnabled = 0x12343412,
		HabDisabled = 0x56787856
	};

	SDPCmdBase(char *p) :CmdBase(p) { init_cmd(); }
	SDPCmd m_spdcmd;
	string m_filename;
	vector<uint8_t> m_input;
	shared_ptr<FileBuffer> m_filebuff;

	int init_cmd() { memset(&m_spdcmd, 0, sizeof(m_spdcmd)); return 0; }
	int send_cmd(HIDReport *p) { return p->write(&m_spdcmd, sizeof(m_spdcmd), 1); };
	int get_status(HIDReport *p, uint32_t &status, uint8_t report_id)
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
	};
	IvtHeader * search_ivt_header(shared_ptr<FileBuffer> data, size_t &off);

	HAB_t get_hab_type(HIDReport *report)
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

	int check_ack(HIDReport *report, uint32_t ack)
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
};

class SDPBootlogCmd : public SDPCmdBase
{
public:
	SDPBootlogCmd(char *p) :SDPCmdBase(p)
	{
		insert_param_info("blog", NULL, Param::e_null);
	}
	int run(CmdCtx *);
};

class SDPDcdCmd : public SDPCmdBase
{
public:
	SDPDcdCmd(char *p):SDPCmdBase(p)
	{
		insert_param_info("dcd", NULL, Param::e_null);
		insert_param_info("-f", &m_filename, Param::e_string_filename);
	}
	int run(CmdCtx *);

};

class SDPWriteCmd : public SDPCmdBase
{
public:
	uint32_t m_download_addr;
	int32_t m_Ivt;
	int m_PlugIn;
	uint32_t m_max_download_pre_cmd;
	uint32_t m_offset;
	bool m_bIvtReserve;

	SDPWriteCmd(char*p) :SDPCmdBase(p) {
		m_spdcmd.m_cmd = ROM_KERNEL_CMD_WR_FILE;
		m_PlugIn = -1;
		m_Ivt = -1;
		m_max_download_pre_cmd = 0x200000;
		m_offset = 0;
		m_bIvtReserve = false;
		m_download_addr = 0;

		insert_param_info("write", NULL, Param::e_null);
		insert_param_info("-f", &m_filename, Param::e_string_filename);
		insert_param_info("-ivt", &m_Ivt, Param::e_uint32);
		insert_param_info("-addr", &m_download_addr, Param::e_uint32);
		insert_param_info("-offset", &m_offset, Param::e_uint32);
	};

	int run(CmdCtx *p);
	int run(CmdCtx *p, void *buff, size_t size, uint32_t addr);
};

class SDPJumpCmd : public SDPCmdBase
{
public:
	bool m_Ivt;
	bool m_PlugIn;
	uint32_t m_jump_addr;
	SDPJumpCmd(char*p):SDPCmdBase(p)
	{
		m_jump_addr = 0;
		m_spdcmd.m_cmd = ROM_KERNEL_CMD_JUMP_ADDR;
		insert_param_info("jump", NULL, Param::e_null);
		insert_param_info("-f", &m_filename, Param::e_string_filename);
		insert_param_info("-ivt", &m_Ivt, Param::e_bool);
		insert_param_info("-plugin", &m_Ivt, Param::e_bool);
		insert_param_info("-addr", &m_jump_addr, Param::e_uint32);
	};
	int run(CmdCtx *p);
};

class SDPSkipDCDCmd :public SDPCmdBase
{
public:
	SDPSkipDCDCmd(char *p) :SDPCmdBase(p) { m_spdcmd.m_cmd = ROM_KERNEL_CMD_SKIP_DCD_HEADER; };
	int run(CmdCtx *p);
};

class SDPStatusCmd :public SDPCmdBase
{
public:
	SDPStatusCmd(char *p) : SDPCmdBase(p)
	{
		m_spdcmd.m_cmd = ROM_KERNEL_CMD_ERROR_STATUS;
		insert_param_info("status", NULL, Param::e_null);
	};
	int run(CmdCtx *p);
};

class SDPBootCmd : public SDPCmdBase
{
public:
	bool m_nojump;
	SDPBootCmd(char *p) : SDPCmdBase(p)
	{
		insert_param_info("boot", NULL, Param::e_null);
		insert_param_info("-f", &m_filename, Param::e_string_filename);
		insert_param_info("-nojump", &m_nojump, Param::e_bool);
		m_nojump = false;
	}
	int run(CmdCtx *p);
};
