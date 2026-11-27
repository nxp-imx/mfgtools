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

#include <zstd.h>

class ZstdStream:public CommonStream
{
	ZSTD_DCtx*const m_dctx{nullptr};
	ZSTD_outBuffer m_output = { 0, 0, 0 };
	ZSTD_inBuffer m_input = { 0, 0, 0 };

public:
	virtual int set_input_buff(void* p, size_t sz) override
	{
		m_input.src = p;
		m_input.pos = 0;
		m_input.size = sz;
		return 0;
	};
	virtual int set_output_buff(void* p, size_t sz) override
	{
		m_output.dst = p;
		m_output.pos = 0;
		m_output.size = sz;
		return 0;
	};
	virtual size_t get_input_pos() override
	{
		return m_input.pos;
	};
	virtual size_t get_output_pos() override
	{
		return m_output.pos;
	};

	virtual int decompress() override
	{
		return ZSTD_decompressStream(m_dctx, &m_output, &m_input);
	};

	virtual size_t get_default_input_size() override
	{
		return ZSTD_DStreamInSize();
	}

	size_t decompress_size(const std::string& backfile) override;

	ZstdStream() : m_dctx{ZSTD_createDCtx()} {};
	~ZstdStream() override
	{
		ZSTD_freeDCtx(m_dctx);
	}
};
