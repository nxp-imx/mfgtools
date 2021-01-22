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

#include "config.h"
#include "cmd.h"
#include "libcomm.h"
#include "liberror.h"
#include "libuuu.h"

using namespace std;

static Config g_config;

constexpr uint16_t FSL_VID = 0x15A2;
constexpr uint16_t NXP_VID = 0x1FC9;
constexpr uint16_t BD_VID = 0x3016;

Config::Config()
{
	emplace_back(ConfigItem{"SDPS:", "MX8QXP", nullptr,   NXP_VID, 0x012F, 0x0002});
	emplace_back(ConfigItem{"SDPS:", "MX8QM",  "MX8QXP",   NXP_VID, 0x0129, 0x0002});
	emplace_back(ConfigItem{"SDPS:", "MX8DXL", "MX8QXP",   NXP_VID, 0x0147});
	emplace_back(ConfigItem{"SDPS:", "MX28",   nullptr,   FSL_VID, 0x004f});
	emplace_back(ConfigItem{"SDPS:", "MX815",  nullptr,   NXP_VID, 0x013E});
	emplace_back(ConfigItem{"SDPS:", "MX865",  "MX815",   NXP_VID, 0x0146});
	emplace_back(ConfigItem{"SDPS:", "MX8ULP",   "MX815",  NXP_VID, 0x014A});
	emplace_back(ConfigItem{"SDPS:", "MX8ULP",   "MX815",  NXP_VID, 0x014B});	
	emplace_back(ConfigItem{"SDP:", "MX7D",    nullptr,   FSL_VID, 0x0076});
	emplace_back(ConfigItem{"SDP:", "MX6Q",    nullptr,   FSL_VID, 0x0054});
	emplace_back(ConfigItem{"SDP:", "MX6D",    "MX6Q", FSL_VID, 0x0061});
	emplace_back(ConfigItem{"SDP:", "MX6SL",   "MX6Q", FSL_VID, 0x0063});
	emplace_back(ConfigItem{"SDP:", "MX6SX",   "MX6Q", FSL_VID, 0x0071});
	emplace_back(ConfigItem{"SDP:", "MX6UL",   "MX7D", FSL_VID, 0x007D});
	emplace_back(ConfigItem{"SDP:", "MX6ULL",  "MX7D", FSL_VID, 0x0080});
	emplace_back(ConfigItem{"SDP:", "MX6SLL",  "MX7D", NXP_VID, 0x0128});
	emplace_back(ConfigItem{"SDP:", "MX7ULP",   nullptr,  NXP_VID, 0x0126});
	emplace_back(ConfigItem{"SDP:", "MXRT106X",  nullptr,  NXP_VID, 0x0135});
	emplace_back(ConfigItem{"SDP:", "MX8MM",   "MX8MQ", NXP_VID, 0x0134});
	emplace_back(ConfigItem{"SDP:", "MX8MQ",   "MX8MQ", NXP_VID, 0x012B});
	emplace_back(ConfigItem{"SDPU:", "SPL",    "SPL",  0x0525, 0xB4A4, 0,      0x04FF});
	emplace_back(ConfigItem{"SDPV:", "SPL1",   "SPL",  0x0525, 0xB4A4, 0x0500, 0x9998});
	emplace_back(ConfigItem{"SDPU:", "SPL",    "SPL",  0x0525, 0xB4A4, 0x9999, 0x9999}); /*old i.MX8 MQEVk use bcd 9999*/
	emplace_back(ConfigItem{"SDPU:", "SPL",    "SPL",  BD_VID, 0x1001, 0,      0x04FF});
	emplace_back(ConfigItem{"SDPV:", "SPL1",   "SPL",  BD_VID, 0x1001, 0x0500, 0x9998});
	emplace_back(ConfigItem{"FBK:", nullptr, nullptr, 0x066F, 0x9AFE});
	emplace_back(ConfigItem{"FBK:", nullptr, nullptr, 0x066F, 0x9BFF});
	emplace_back(ConfigItem{"FB:", nullptr, nullptr,  0x0525, 0xA4A5});
	emplace_back(ConfigItem{"FB:", nullptr, nullptr,  0x18D1, 0x0D02});
	emplace_back(ConfigItem{"FB:", nullptr, nullptr,  BD_VID, 0x0001});
}

int uuu_for_each_cfg(uuu_show_cfg fn, void *p)
{
	for (const auto &configItem : g_config)
	{
		if (fn(configItem.m_protocol.c_str(),
			   configItem.m_chip.c_str(),
			   configItem.m_compatible.c_str(),
			   configItem.m_vid,
			   configItem.m_pid,
			   configItem.m_bcdVerMin,
			   configItem.m_bcdVerMax,
			   p))
			return -1;
	}
	return 0;
}

Config * get_config() noexcept
{
	return &g_config;
}

ConfigItem * Config::find(uint16_t vid, uint16_t pid, uint16_t ver)
{
	for (auto it = begin(); it != end(); it++)
	{
		if (vid == it->m_vid && pid == it->m_pid)
		{
			if (ver >= it->m_bcdVerMin && ver <= it->m_bcdVerMax)
				return &(*it);
		}
	}
	return nullptr;
}

Config Config::find(const string &pro)
{
	Config items;
	for (auto it = begin(); it != end(); it++)
	{
		if (it->m_protocol == pro)
			items.emplace_back(*it);
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

	bool conversion_succeeded = false;
	while (pos < m_cmd.size())
	{
		param = get_next_param(m_cmd, pos);
		if (param == "-pid")
		{
			param = get_next_param(m_cmd, pos);
			item.m_pid = str_to_uint16(param, &conversion_succeeded);
			if (!conversion_succeeded) return -1;
			continue;
		}
		if (param == "-vid")
		{
			param = get_next_param(m_cmd, pos);
			item.m_vid = str_to_uint16(param, &conversion_succeeded);
			if (!conversion_succeeded) return -1;
			continue;
		}
		if (param == "-bcdversion")
		{
			param = get_next_param(m_cmd, pos);
			item.m_bcdVerMin = item.m_bcdVerMax = str_to_uint16(param, &conversion_succeeded);
			if (!conversion_succeeded) return -1;
			continue;
		}
		if (param == "-bcdmin")
		{
			param = get_next_param(m_cmd, pos);
			item.m_bcdVerMin =  str_to_uint16(param, &conversion_succeeded);
			if (!conversion_succeeded) return -1;
			continue;
		}
		if (param == "-bcdmax")
		{
			param = get_next_param(m_cmd, pos);
			item.m_bcdVerMax = str_to_uint16(param, &conversion_succeeded);
			if (!conversion_succeeded) return -1;
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
		g_config.emplace_back(item);

	return 0;
}
