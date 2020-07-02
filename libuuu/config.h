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

#include <cstdint>
#include <string>
#include <vector>

class ConfigItem
{
public:
	ConfigItem() = default;
	ConfigItem(const char *pro, const char *chip, const char *comp, uint16_t vid, uint16_t pid, uint16_t verLow = 0, uint16_t verUp = UINT16_MAX) :
		m_pid{pid}, m_vid{vid}, m_bcdVerMin{verLow}, m_bcdVerMax{verUp}
	{
		if (pro)
			m_protocol = pro;
		if (chip)
			m_chip = chip;
		if (comp)
			m_compatible = comp;
	}
	std::string m_protocol;
	std::string m_chip;
	std::string m_compatible;
	uint16_t m_pid = 0;
	uint16_t m_vid = 0;
	uint16_t m_bcdVerMin = 0;
	uint16_t m_bcdVerMax = UINT16_MAX;
};

class Config :public std::vector<ConfigItem>
{
public:
	Config();
	ConfigItem *find(uint16_t vid, uint16_t pid, uint16_t ver);
	Config find(const std::string &protocal);
};

Config * get_config() noexcept;
