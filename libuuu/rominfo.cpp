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

ROM_INFO g_RomInfo[] =
{
	{ "MX6Q",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 },
	{ "MX6D",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 },
	{ "MX6SL",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 },
	{ "MX7D",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{ "MX6UL",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{ "MX6ULL",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{ "MX6SLL",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{ "MX8MQ",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{ "MX7ULP",	 0x2f018000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{ "MXRT106X",0x1000,     ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{ "MX8QXPB0",0x0,		 ROM_INFO_HID | ROM_INFO_HID_NO_CMD | ROM_INFO_HID_UID_STRING },
	{ "MX815",   0x0,		 ROM_INFO_HID | ROM_INFO_HID_NO_CMD | ROM_INFO_HID_UID_STRING | ROM_INFO_HID_EP1 | ROM_INFO_HID_PACK_SIZE_1020 },
	{ "SPL",	 0x0,		 ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_SPL_JUMP }
};

ROM_INFO * search_rom_info(const char *s)
{
	string s1 = s;
	for (int i = 0; i < sizeof(g_RomInfo) / sizeof(ROM_INFO); i++)
	{
		string s2;
		s2 = g_RomInfo[i].m_name;
		if (s1 == s2)
			return g_RomInfo + i;
	}
	return 0;
}

ROM_INFO * search_rom_info(ConfigItem *item)
{
	if (item == NULL)
		return NULL;

	ROM_INFO *p = search_rom_info(item->m_chip.c_str());
	if (p)
		return p;

	return search_rom_info(item->m_compatible.c_str());
}