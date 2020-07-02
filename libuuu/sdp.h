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

#include <climits>

class FileBuffer;
class HIDReport;

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

#define ROM_KERNEL_CMD_RD_MEM							0x0101
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

protected:
	int check_ack(HIDReport *report, uint32_t ack);
	HAB_t get_hab_type(HIDReport *report);
	int get_status(HIDReport *p, uint32_t &status, uint8_t report_id);
	int init_cmd();
	IvtHeader * search_ivt_header(std::shared_ptr<FileBuffer> data, size_t &off, size_t limit=ULLONG_MAX);

	std::string m_filename;
	SDPCmd m_spdcmd;

private:
	int send_cmd(HIDReport *p);

	std::vector<uint8_t> m_input;
};

class SDPBootlogCmd : public SDPCmdBase
{
public:
	SDPBootlogCmd(char *p);
	int run(CmdCtx *) override;
};

class SDPDcdCmd : public SDPCmdBase
{
public:
	SDPDcdCmd(char *p);
	int run(CmdCtx *) override;

private:
	uint32_t m_dcd_addr;
};

class SDPReadMemCmd : public SDPCmdBase
{
public:
	SDPReadMemCmd(char*p);
	int run(CmdCtx *) override;

private:
	uint32_t m_mem_addr;
	uint8_t m_mem_format;
};

class SDPWriteMemCmd : public SDPCmdBase
{
public:
	SDPWriteMemCmd(char*p);
	int run(CmdCtx *p) override;

private:
	uint32_t m_mem_addr;
	uint8_t m_mem_format;
	uint32_t m_mem_value;
};

class SDPWriteCmd : public SDPCmdBase
{
public:
	SDPWriteCmd(char*p);

	int run(CmdCtx *p) override;
	int run(CmdCtx *p, void *buff, size_t size, uint32_t addr);

private:
	uint32_t m_download_addr;
	int32_t m_Ivt;
	int m_PlugIn;
	uint32_t m_max_download_pre_cmd;
	uint32_t m_offset;
	bool m_bIvtReserve;
	bool m_bskipspl;
	bool m_bskipfhdr;
};

class SDPJumpCmd : public SDPCmdBase
{
public:
	SDPJumpCmd(char*p);
	int run(CmdCtx *p) override;

private:
	bool m_clear_dcd = false;
	bool m_Ivt;
	uint32_t m_jump_addr = 0;
	bool m_PlugIn;
};

class SDPSkipDCDCmd :public SDPCmdBase
{
public:
	SDPSkipDCDCmd(char *p);
	int run(CmdCtx *p) override;
};

class SDPStatusCmd :public SDPCmdBase
{
public:
	SDPStatusCmd(char *p);
	int run(CmdCtx *p) override;
};

class SDPBootCmd : public SDPCmdBase
{
public:
	SDPBootCmd(char *p);
	int run(CmdCtx *p) override;

private:
	bool m_clear_dcd = false;
	uint32_t m_dcd_addr = 0;
	bool m_nojump = false;
};
