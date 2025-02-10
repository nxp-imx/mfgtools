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

#include <string>
#include <vector>

struct bmap_t {
	using bmap_type = std::vector<std::pair<size_t, size_t>>;

	bmap_t() = default;
	// fully mapped image
	bmap_t(size_t img_size, size_t blk_size = 4096)
	: m_img_size(img_size),
	  m_blk_size(blk_size),
	  m_blk_count(default_blocks_count(img_size, blk_size))
	{
		set_mapped_range(0, m_blk_count - 1);
	}

	bmap_t& set_image_size(size_t size) {
		m_img_size = size;
		return *this;
	}

	bmap_t& set_block_size(size_t size = 4096) {
		m_blk_size = size;
		return *this;
	}

	bmap_t& set_blocks_count(size_t size = 0) {
		if (size)
			m_blk_count = size;
		else
			m_blk_count = default_blocks_count(m_img_size, m_blk_size);
		return *this;
	}

	bmap_t& set_mapped_range(size_t begin, size_t end) {
		m_blk_map.emplace_back(begin, end);
		return *this;
	}

	static size_t default_blocks_count(size_t img_size, size_t blk_size) {
		return img_size / blk_size + (img_size % blk_size ? 1 : 0);
	}

	size_t image_size() const { return m_img_size; }
	size_t block_size() const { return m_blk_size; }
	size_t blocks_count() const { return m_blk_count; }

	const bmap_type& mapped_ranges() const { return m_blk_map; }

	bool is_mapped_block(size_t index) const;

private:
	size_t m_img_size = 0;
	size_t m_blk_size = 4096;
	size_t m_blk_count = 0;
	bmap_type m_blk_map;
	mutable bool m_gap_set = false;
	mutable size_t m_gap_begin = 0;
	mutable size_t m_gap_end = 0;
	mutable size_t m_next_gap_begin = 0;
};

int load_bmap(const std::string& filename, bmap_t& bmap);
