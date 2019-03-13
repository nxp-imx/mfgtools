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
	{ "MXRT106X",	 0x1000,     ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	{ "MX8QXP",      0x0,        ROM_INFO_HID | ROM_INFO_HID_NO_CMD | ROM_INFO_HID_UID_STRING },
	{ "MX28",	 0x0,		 ROM_INFO_HID},
	{ "SPL",	 0x0,	     ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_SPL_JUMP },
	{ "SPL1",	 0x0,	     ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_SPL_JUMP | ROM_INFO_AUTO_SCAN_UBOOT_POS},
};

ROM_INFO * search_rom_info(const char *s)
{
	string s1 = s;
	for (size_t i = 0; i < sizeof(g_RomInfo) / sizeof(ROM_INFO); i++)
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


#define IV_MAX_LEN		32
#define HASH_MAX_LEN	64

#define CONTAINER_HDR_ALIGNMENT 0x400
#define CONTAINER_TAG 0x87

#pragma pack (1)
struct rom_container {
	uint8_t  version;
	uint8_t  length_l;
	uint8_t  length_m;
	uint8_t  tag;
	uint32_t flags;
	uint16_t sw_version;
	uint8_t  fuse_version;
	uint8_t  num_images;
	uint16_t sig_blk_offset;
	uint16_t reserved;
};

struct rom_bootimg {
	uint32_t offset;
	uint32_t size;
	uint64_t destination;
	uint64_t entry;
	uint32_t flags;
	uint32_t meta;
	uint8_t  hash[HASH_MAX_LEN];
	uint8_t  iv[IV_MAX_LEN];
};

#pragma pack ()

inline uint32_t round_up(uint32_t x, uint32_t align)
{
	uint32_t mask = align - 1;
	return (x + mask) & ~mask;
}

size_t GetContainerActualSize(shared_ptr<FileBuffer> p, size_t offset)
{
	struct rom_container *hdr;

	hdr = (struct rom_container *)(p->data() + offset + CONTAINER_HDR_ALIGNMENT);
	if (hdr->tag != CONTAINER_TAG)
		return p->size() - offset;

	struct rom_bootimg *image;
	image = (struct rom_bootimg *)(p->data() + offset + CONTAINER_HDR_ALIGNMENT
		+ sizeof(struct rom_container)
		+ sizeof(struct rom_bootimg) * (hdr->num_images - 1));

	uint32_t sz = image->size + image->offset + CONTAINER_HDR_ALIGNMENT;

	sz = round_up(sz, CONTAINER_HDR_ALIGNMENT);

	if (sz > (p->size() - offset))
		return p->size() - offset;

	hdr = (struct rom_container *)(p->data() + offset + sz);

	return sz;
}