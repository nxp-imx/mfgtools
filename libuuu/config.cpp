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

#include <vector>
#include "config.h"
#include "cmd.h"
#include "libuuu.h"

static Config g_config;

#define FSL_VID 0x15A2
#define NXP_VID 0x1FC9

Config::Config()
{
	push_back(ConfigItem("SDPS:", "MX8QXP", NULL,   NXP_VID, 0x012F, 0x0002));
	push_back(ConfigItem("SDPS:", "MX8QM",  "MX8QXP",   NXP_VID, 0x0129, 0x0002));
	push_back(ConfigItem("SDPS:", "MX28",   NULL,   FSL_VID, 0x004f));
	push_back(ConfigItem("SDP:", "MX7D",    NULL,   FSL_VID, 0x0076));
	push_back(ConfigItem("SDP:", "MX6Q",    NULL,   FSL_VID, 0x0054));
	push_back(ConfigItem("SDP:", "MX6D",    "MX6Q", FSL_VID, 0x0061));
	push_back(ConfigItem("SDP:", "MX6SL",   "MX6Q", FSL_VID, 0x0063));
	push_back(ConfigItem("SDP:", "MX6SX",   "MX6Q", FSL_VID, 0x0071));
	push_back(ConfigItem("SDP:", "MX6UL",   "MX7D", FSL_VID, 0x007D));
	push_back(ConfigItem("SDP:", "MX6ULL",  "MX7D", FSL_VID, 0x0080));
	push_back(ConfigItem("SDP:", "MX6SLL",  "MX7D", NXP_VID, 0x0128));
	push_back(ConfigItem("SDP:", "MX7ULP",   NULL,  NXP_VID, 0x0126));
	push_back(ConfigItem("SDP:", "MXRT106X",  NULL,  NXP_VID, 0x0135));
	push_back(ConfigItem("SDP:", "MX8MM",   "MX8MQ", NXP_VID, 0x0134));
	push_back(ConfigItem("SDP:", "MX8MQ",   "MX8MQ", NXP_VID, 0x012B));
	push_back(ConfigItem("SDPU:", "SPL",    "SPL",  0x0525, 0xB4A4, 0,      0x04FF));
	push_back(ConfigItem("SDPV:", "SPL1",   "SPL",  0x0525, 0xB4A4, 0x0500, 0x9998));
	push_back(ConfigItem("SDPU:", "SPL",    "SPL",  0x0525, 0xB4A4, 0x9999, 0x9999)); /*old i.MX8 MQEVk use bcd 9999*/
	push_back(ConfigItem("FBK:", NULL, NULL, 0x066F, 0x9AFE));
	push_back(ConfigItem("FBK:", NULL, NULL, 0x066F, 0x9BFF));
	push_back(ConfigItem("FB:", NULL, NULL,  0x0525, 0xA4A5));
	push_back(ConfigItem("FB:", NULL, NULL,  0x18D1, 0x0D02));
}

int uuu_for_each_cfg(uuu_show_cfg fn, void *p)
{
	for (size_t i = 0; i < g_config.size(); i++)
	{
		if (fn(g_config[i].m_protocol.c_str(),
			   g_config[i].m_chip.c_str(),
			   g_config[i].m_compatible.c_str(),
			   g_config[i].m_vid,
			   g_config[i].m_pid,
			   g_config[i].m_bcdVerMin,
			   g_config[i].m_bcdVerMax,
			   p))
			return -1;
	}
	return 0;
}

Config * get_config()
{
	return &g_config;
}

ConfigItem * Config::find(uint16_t vid, uint16_t pid, uint16_t ver)
{
	iterator it;
	for (it = begin(); it != end(); it++)
	{
		if (vid == it->m_vid && pid == it->m_pid)
		{
			if (ver >= it->m_bcdVerMin && ver <= it->m_bcdVerMax)
				return &(*it);
		}
	}
	return NULL;
}

Config Config::find(string pro)
{
	Config items;
	iterator it;
	for (it = begin(); it != end(); it++)
	{
		if (it->m_protocol == pro)
			items.push_back(*it);
	}
	return items;
}

int CfgCmd::run(CmdCtx *)
{
	size_t pos = 0;
	string param;

	ConfigItem item;
	param = get_next_param(m_cmd, pos);

	if (str_to_upper(param) == "CFG:")
		param = get_next_param(m_cmd, pos);

	if (param.empty())
	{
		set_last_err_string("Wrong param");
		return -1;
	}

	item.m_protocol = str_to_upper(param);

	while (pos < m_cmd.size())
	{
		param = get_next_param(m_cmd, pos);
		if (param == "-pid")
		{
			param = get_next_param(m_cmd, pos);
			item.m_pid = str_to_uint(param);
			continue;
		}
		if (param == "-vid")
		{
			param = get_next_param(m_cmd, pos);
			item.m_vid = str_to_uint(param);
			continue;
		}
		if (param == "-bcdversion")
		{
			param = get_next_param(m_cmd, pos);
			item.m_bcdVerMin = item.m_bcdVerMax = str_to_uint(param);
			continue;
		}
		if (param == "-bcdmin")
		{
			param = get_next_param(m_cmd, pos);
			item.m_bcdVerMin =  str_to_uint(param);
			continue;
		}
		if (param == "-bcdmax")
		{
			param = get_next_param(m_cmd, pos);
			item.m_bcdVerMax = str_to_uint(param);
			continue;
		}
		if (param == "-chip")
		{
			param = get_next_param(m_cmd, pos);
			item.m_chip = param;
			continue;
		}
		if (param == "-compatible")
		{
			param = get_next_param(m_cmd, pos);
			item.m_compatible = param;
			continue;
		}
	}

	ConfigItem *pItem= g_config.find(item.m_vid, item.m_pid, item.m_bcdVerMax);
	if (pItem)
		*pItem = item;
	else
		g_config.push_back(item);

	return 0;
}
