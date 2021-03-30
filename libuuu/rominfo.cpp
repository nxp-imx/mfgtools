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

#include "rominfo.h"
#include "buffer.h"
#include "config.h"
#include "libcomm.h"

#include <array>

using namespace std;

static constexpr std::array<ROM_INFO, 15> g_RomInfo
{
	ROM_INFO{ "MX6Q",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 },
	ROM_INFO{ "MX6D",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 },
	ROM_INFO{ "MX6SL",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 },
	ROM_INFO{ "MX7D",	 0x00911000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	ROM_INFO{ "MX6UL",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	ROM_INFO{ "MX6ULL",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	ROM_INFO{ "MX6SLL",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	ROM_INFO{ "MX8MQ",	 0x00910000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	ROM_INFO{ "MX7ULP",	 0x2f018000, ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	ROM_INFO{ "MXRT106X",	 0x1000,     ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_HID_SKIP_DCD },
	ROM_INFO{ "MX8QXP",      0x0,        ROM_INFO_HID | ROM_INFO_HID_NO_CMD | ROM_INFO_HID_UID_STRING },
	ROM_INFO{ "MX28",	 0x0,        ROM_INFO_HID},
	ROM_INFO{ "MX815",       0x0,        ROM_INFO_HID | ROM_INFO_HID_NO_CMD | ROM_INFO_HID_UID_STRING | ROM_INFO_HID_EP1 | ROM_INFO_HID_PACK_SIZE_1020 | ROM_INFO_HID_ROMAPI},
	ROM_INFO{ "SPL",	 0x0,	     ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_SPL_JUMP | ROM_INFO_HID_SDP_NO_MAX_PER_TRANS},
	ROM_INFO{ "SPL1",	 0x0,	     ROM_INFO_HID | ROM_INFO_HID_MX6 | ROM_INFO_SPL_JUMP | ROM_INFO_HID_SDP_NO_MAX_PER_TRANS | ROM_INFO_AUTO_SCAN_UBOOT_POS},
};

const ROM_INFO * search_rom_info(const std::string &s)
{
	for (const auto &rom_info : g_RomInfo) {
		if (s == rom_info.m_name)
		{
			return &rom_info;
		}
	}

	return nullptr;
}

const ROM_INFO * search_rom_info(const ConfigItem *item)
{
	if (item == nullptr)
	{
		return nullptr;
	}

	const ROM_INFO * const p = search_rom_info(item->m_chip);
	if (p)
	{
		return p;
	}

	return search_rom_info(item->m_compatible);
}


#define IV_MAX_LEN		32
#define HASH_MAX_LEN	64

#define CONTAINER_HDR_ALIGNMENT 0x400
static constexpr uint8_t CONTAINER_TAG = 0x87;

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


static constexpr uint32_t IMG_V2X = 0x0B;

#pragma pack ()


size_t GetContainerActualSize(shared_ptr<FileBuffer> p, size_t offset, bool bROMAPI)
{
	if(bROMAPI)
		return p->size() - offset;

	auto hdr = reinterpret_cast<struct rom_container *>(p->data() + offset + CONTAINER_HDR_ALIGNMENT);
	if (hdr->tag != CONTAINER_TAG)
	{
		return p->size() - offset;
	}

	/* Check if include V2X container*/
	auto image = reinterpret_cast<struct rom_bootimg *>(p->data() + offset + CONTAINER_HDR_ALIGNMENT
		+ sizeof(struct rom_container));

	unsigned int cindex = 1;
	if ((image->flags & 0xF) == IMG_V2X)
	{
		cindex = 2;
		hdr = reinterpret_cast<struct rom_container *>(p->data() + offset + cindex * CONTAINER_HDR_ALIGNMENT);
		if (hdr->tag != CONTAINER_TAG)
		 {
			return p->size() - offset;
		}
	}

	image = reinterpret_cast<struct rom_bootimg *>(p->data() + offset + cindex * CONTAINER_HDR_ALIGNMENT
		+ sizeof(struct rom_container)
		+ sizeof(struct rom_bootimg) * (hdr->num_images - 1));

	uint32_t sz = image->size + image->offset + cindex * CONTAINER_HDR_ALIGNMENT;

	sz = round_up(sz, static_cast<uint32_t>(CONTAINER_HDR_ALIGNMENT));

	if (sz > (p->size() - offset))
	{
		return p->size() - offset;
	}

	return sz;
}

bool CheckHeader(uint32_t *p)
{
	static constexpr std::array <uint32_t, 2> FlashHeaderMagic
	{
		0xc0ffee01,
		0x42464346
	};

	for (const auto magic_val : FlashHeaderMagic)
	{
		if (*p == magic_val)
		{
			return true;
		}
	}

	return false;
}

size_t GetFlashHeaderSize(shared_ptr<FileBuffer> p, size_t offset)
{
	static constexpr std::array<size_t, 4> offsets
	{
		0,
		0x400,
		0x1fc,
		0x5fc
	};

	for (const auto test_offset : offsets) {
		if (p->size() < (offset + test_offset)) {
			return 0;
		}

		if (CheckHeader(reinterpret_cast<uint32_t*>(p->data() + offset + test_offset))) {
			return 0x1000;
		}
	}

	return 0;
}
