/*
 * Copyright 2020 NXP.
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

#include "sparse.h"
#include "liberror.h"

#include <cstddef>
#include <cstring>

chunk_header_t * SparseFile::get_next_chunk(uint8_t *p, size_t &pos)
{
	if (pos == 0)
	{
		sparse_header *pheader = (sparse_header*)p;
		if (pheader->magic != SPARSE_HEADER_MAGIC) {
			set_last_err_string("Sparse heade Magic missed");
			return nullptr;
		}
		pos += pheader->file_hdr_sz;
	}

	chunk_header_t *pchunk = (chunk_header_t*)(p + pos);
	pos += pchunk->total_sz;
	return pchunk;
}

int SparseFile::init_header(size_t blsz, int blcount)
{
	sparse_header header;

	memset(&header, 0, sizeof(header));
	header.magic = SPARSE_HEADER_MAGIC;
	header.major_version = 1;
	header.minor_version = 0;
	header.file_hdr_sz = sizeof(header);
	header.chunk_hdr_sz = sizeof(chunk_header);
	header.blk_sz = blsz;
	m_cur_chunk_header_pos = 0;
	if (blcount)
	{
		m_data.reserve(blsz*blcount + 0x1000);
		m_max_size = blsz * blcount;
	}
	m_data.clear();
	push(&header, sizeof(header));
	m_pcrc = (uint32_t*)(m_data.data() + offsetof(sparse_header, image_checksum));
	return 0;
}

bool SparseFile::is_append_old_chuck(int type, void *p)
{
	chunk_header_t *pchunk;
	pchunk = (chunk_header_t *)(m_data.data() + m_cur_chunk_header_pos);

	if (m_cur_chunk_header_pos == 0)
		return false;

	if (pchunk->chunk_type != type)
		return false;

	if (type == CHUNK_TYPE_FILL)
	{
		uint32_t a = *(uint32_t*)(pchunk + 1);
		uint32_t b = *(uint32_t*)p;
		if (a != b)
			return false;
	}
	return true;
}

bool SparseFile::is_same_value(void *data, size_t sz)
{
	uint32_t *p = (uint32_t *)data;
	uint32_t val = *p;
	for (size_t i = 0; i < sz / sizeof(uint32_t); i++)
		if (val != p[i])
			return false;
	return true;
}

bool SparseFile::is_validate_sparse_file(void *p, size_t)
{
	sparse_header *pheader = (sparse_header*)p;
	if (pheader->magic == SPARSE_HEADER_MAGIC)
		return true;
	return false;
}

int SparseFile::push(void *p, size_t sz)
{
	size_t pos = m_data.size();
	m_data.resize(pos + sz);
	memcpy(m_data.data() + pos, p, sz);
	return 0;
}

int SparseFile::push_one_block(void *data)
{
	chunk_header_t *pchunk;
	pchunk = (chunk_header_t *)(m_data.data() + m_cur_chunk_header_pos);

	sparse_header *pheader;
	pheader = (sparse_header *)m_data.data();

	pheader->total_blks++;

	//int type = is_same_value(data, pheader->blk_sz) ? CHUNK_TYPE_FILL : CHUNK_TYPE_RAW;
	int type = CHUNK_TYPE_RAW;

	if (!is_append_old_chuck(type, data))
	{
		chunk_header_t header;
		header.chunk_type = type;
		header.chunk_sz = 1;
		header.total_sz = (type == CHUNK_TYPE_FILL) ? sizeof(uint32_t) : pheader->blk_sz;
		header.total_sz += sizeof(chunk_header_t);
		header.reserved1 = 0;

		pheader->total_chunks++;

		m_cur_chunk_header_pos = m_data.size();

		push(&header, sizeof(chunk_header_t));

		if (type == CHUNK_TYPE_RAW)
			push(data, pheader->blk_sz);
		else
			push(data, sizeof(uint32_t));
	}
	else
	{
		pchunk->chunk_sz++;
		if (type == CHUNK_TYPE_RAW)
		{
			push(data, pheader->blk_sz);
			pchunk->total_sz += pheader->blk_sz;
		}
	}

	if (m_data.size() + 2 * pheader->blk_sz > m_max_size )
		return -1;

	return 0;
}

size_t SparseFile::push_one_chuck(chunk_header_t *p, void *data)
{
	chunk_header_t cheader = *p;
	sparse_header *pheader;
	pheader = (sparse_header *)m_data.data();

	size_t sz = p->total_sz - sizeof(chunk_header);

	if (p->total_sz + m_data.size() > m_max_size)
	{
		if (p->chunk_type == CHUNK_TYPE_RAW)
		{
			size_t blk = (m_max_size - m_data.size()) / pheader->blk_sz;
			if (blk < 2)
				return 0;

			blk -= 2;

			cheader.chunk_sz = blk;
			sz = blk * pheader->blk_sz;
			cheader.total_sz = sizeof(chunk_header_t) + sz;
		}
		else
			return 0;
	}

	push(&cheader, sizeof(chunk_header));
	pheader->total_chunks ++;
	pheader->total_blks += cheader.chunk_sz;

	if (data) {
		push(data, sz);
	}

	return sz;
}

size_t SparseFile::push_raw_data(void *data, size_t sz)
{
	chunk_header_t cheader;
	cheader.chunk_type = CHUNK_TYPE_RAW;

	sparse_header *pheader;
	pheader = (sparse_header *)m_data.data();

	cheader.chunk_sz = sz / pheader->blk_sz;
	cheader.total_sz = cheader.chunk_sz*pheader->blk_sz + sizeof(chunk_header_t);
	pheader = (sparse_header *)m_data.data();

	return push_one_chuck(&cheader, data);
}
