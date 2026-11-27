/*
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

#include "compress_stream.h"

#include <zlib.h>

#include <cstring>

class Gzstream : public CommonStream
{
	z_stream m_strm;
	size_t m_in_size = 0;
	size_t m_out_size = 0;

public:
	Gzstream()
	{
		std::memset(&m_strm, 0, sizeof(m_strm));
		inflateInit2(&m_strm, 15 + 16);
	}
	~Gzstream() override
	{
		deflateEnd(&m_strm);
	}
	virtual int set_input_buff(void* p, size_t sz) override
	{
		m_strm.next_in = (Bytef*)p;
		m_strm.avail_in = m_in_size = sz;
		return 0;
	};
	virtual int set_output_buff(void* p, size_t sz) override
	{
		m_strm.next_out = (Bytef*)p;
		m_strm.avail_out = m_out_size = sz;
		return 0;
	};
	virtual size_t get_input_pos() override
	{
		return m_in_size - m_strm.avail_in;
	};
	virtual size_t get_output_pos() override
	{
		return m_out_size - m_strm.avail_out;
	};
	virtual int decompress() override
	{
		return inflate(&m_strm, Z_SYNC_FLUSH);
	};

	virtual size_t get_default_input_size() override { return 0x10000; }
};
