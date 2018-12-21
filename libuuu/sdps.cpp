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

size_t SDPSCmd::GetActualSize(shared_ptr<FileBuffer> p, size_t offset)
{
	struct rom_container *hdr;

	hdr = (struct rom_container *)(p->data() + offset + CONTAINER_HDR_ALIGNMENT);
	if (hdr->tag != CONTAINER_TAG)
		return p->size() - offset;

	struct rom_bootimg *image;
	image = (struct rom_bootimg *)(p->data() + offset + CONTAINER_HDR_ALIGNMENT
				+ sizeof(struct rom_container) 
				+ sizeof(struct rom_bootimg) * (hdr->num_images-1));

	uint32_t sz = image->size + image->offset + offset + CONTAINER_HDR_ALIGNMENT;
	
	sz = round_up(sz, CONTAINER_HDR_ALIGNMENT);

	if (sz > (p->size() - offset))
		return p->size() - offset;

	hdr = (struct rom_container *)(p->data() + offset + sz);

	return sz;
}

int SDPSCmd::run(CmdCtx *pro)
{

	HIDTrans dev;
	if(dev.open(pro->m_dev))
		return -1;

	shared_ptr<FileBuffer> p = get_file_buffer(m_filename);
	if (!p)
		return -1;

	HIDReport report(&dev);
	report.m_skip_notify = false;

	if (m_offset >= p->size())
	{
		set_last_err_string("Offset bigger than file size");
		return -1;
	}

	size_t sz = GetActualSize(p, m_offset);
	int ret = report.write(p->data() + m_offset, sz,  2);

	SDPBootlogCmd log(NULL);
	log.run(pro);

	return ret;
}