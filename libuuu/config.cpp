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
	push_back(ConfigItem("SDP:", "MX7D",    NULL,   FSL_VID, 0x0076));
	push_back(ConfigItem("SDP:", "MX6Q",    NULL,   FSL_VID, 0x0054));
	push_back(ConfigItem("SDP:", "MX6D",    "MX6Q", FSL_VID, 0x0061));
	push_back(ConfigItem("SDP:", "MX6SL",   "MX6Q", FSL_VID, 0x0063));
	push_back(ConfigItem("SDP:", "MX6SX",   "MX6Q", FSL_VID, 0x0071));
	push_back(ConfigItem("SDP:", "MX6UL",   "MX7D", FSL_VID, 0x007D));
	push_back(ConfigItem("SDP:", "MX6ULL",  "MX7D", FSL_VID, 0x0080));
	push_back(ConfigItem("SDP:", "MX6SLL",  "MX7D", NXP_VID, 0x0128));
	push_back(ConfigItem("SDP:", "MX7ULP",   NULL,  NXP_VID, 0x0054));
}

int uuu_for_each_cfg(uuu_show_cfg fn, void *p)
{
	for (int i = 0; i < g_config.size(); i++)
	{
		if (fn(g_config[i].m_protocol.c_str(),
			   g_config[i].m_chip.c_str(),
			   g_config[i].m_compatible.c_str(),
			   g_config[i].m_vid,
			   g_config[i].m_pid,
			   g_config[i].m_bcdVersion,
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
			if (it->m_bcdVersion == 0xFFFF)
				return &(*it);
			if (ver == it->m_bcdVersion)
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

	if (param == "cfg:")
		param = get_next_param(m_cmd, pos);

	if (param.empty())
	{
		set_last_err_string("Wrong param");
		return -1;
	}

	item.m_protocol = param;

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
			item.m_bcdVersion = str_to_uint(param);
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

	g_config.push_back(item);

	return 0;
}