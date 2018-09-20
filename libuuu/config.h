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

#include <string>
#include <vector>

using namespace std;

class ConfigItem
{
public:
	ConfigItem() { m_pid = m_vid = 0; m_bcdVersion = 0xFFFF; };
	ConfigItem(const char *pro, const char *chip, const char *comp, uint16_t vid, uint16_t pid, uint16_t ver = -1)
	{
		if (pro)
			m_protocol = pro;
		if (chip)
			m_chip = chip;
		if (comp)
			m_compatible = comp;
		m_pid = pid;
		m_vid = vid;
		m_bcdVersion = ver;
	};
	string m_protocol;
	string m_chip;
	string m_compatible;
	uint16_t m_pid;
	uint16_t m_vid;
	uint16_t m_bcdVersion;
};

class Config :public vector<ConfigItem>
{
public:
	Config();
	ConfigItem *find(uint16_t vid, uint16_t pid, uint16_t ver = -1);
	Config find(string protocal);
};

Config * get_config();
