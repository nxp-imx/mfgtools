#include <iostream>
#include <map>
#include <tinyxml2.h>

#include "bmap.h"
#include "buffer.h"

extern int g_verbose;

bool bmap_t::is_mapped_block(size_t index) const
{
	if (index >= m_gap_begin && index < m_gap_end)
		return false;

	if (index >= m_gap_end && index < m_next_gap_begin)
		return true;

	if (index >= m_blk_count)
		return false;

	m_gap_begin = 0;

	for(auto iter = m_blk_map.begin(); iter != m_blk_map.end(); ++iter) {
		m_gap_end = iter->first;
		m_next_gap_begin = iter->second + 1;

		if (index >= m_gap_begin && index < m_gap_end)
			return false;

		if (index >= m_gap_end && index < m_next_gap_begin)
			return true;

		m_gap_begin = m_next_gap_begin;
	}

	return true;
}

static bool parse_image_size(bmap_t &bmap, const tinyxml2::XMLElement* elem)
{
	auto img_size = elem->Unsigned64Text();
	if (!img_size) {
		std::cerr << "Invalid image size." << std::endl;
		return false;
	}
	bmap.set_image_size(img_size);
	return true;
}

static bool parse_block_size(bmap_t &bmap, const tinyxml2::XMLElement* elem)
{
	auto blk_size = elem->Unsigned64Text();
	if (!blk_size) {
		std::cerr << "Invalid block size." << std::endl;
		return false;
	}
	bmap.set_block_size(blk_size);
	return true;
}

static bool parse_blocks_count(bmap_t &bmap, const tinyxml2::XMLElement* elem)
{
	auto blk_count = elem->Unsigned64Text();
	if (!blk_count) {
		std::cerr << "Invalid blocks count." << std::endl;
		return false;
	}
	bmap.set_blocks_count(blk_count);
	return true;
}

static bool parse_block_map(bmap_t &bmap, const tinyxml2::XMLElement* elem)
{
	for (auto ch = elem->FirstChildElement(); ch != nullptr; ch = ch->NextSiblingElement()) {
		if (strcmp(ch->Name(), "Range")) {
			continue;
		}

		std::string text = ch->GetText();

		auto f = std::strtoul(text.data(), nullptr, 0);
		auto l = f;

		auto pos = text.find('-');
		if (pos != std::string::npos)
			l = std::strtoul(text.data() + pos + 1, nullptr, 0);

		bmap.set_mapped_range(f, l);
	}

	return true;
}

static const std::map<std::string, bool (*)(bmap_t &, const tinyxml2::XMLElement*)> handlers{
	{ "ImageSize", parse_image_size },
	{ "BlockSize", parse_block_size },
	{ "BlocksCount", parse_blocks_count },
	{ "BlockMap", parse_block_map },
};

bool load_bmap(const std::string& filename, bmap_t& bmap)
{
	tinyxml2::XMLDocument doc;
	auto fbuf = get_file_buffer(filename, true);
	if (fbuf == nullptr) {
		return -1;
	}

	auto dbuf = fbuf->request_data(0, fbuf->size());
	if (dbuf == nullptr) {
		return -1;
	}

	auto err = doc.Parse((char*)dbuf->data(), dbuf->size());
	if (err != tinyxml2::XML_SUCCESS) {
		return -1;
	}

	auto elem = doc.FirstChildElement();

	if (!elem) {
		std::cerr << "No bmap element" << std::endl;
		return -1;
	}

	if (elem) {
		if (!elem->Attribute("version", "2.0")) {
			std::cerr << "Invalid bmap version. 2.0 is expected." << std::endl;
			return -1;
		}
	}

	for (auto ch = elem->FirstChildElement(); ch != nullptr; ch = ch->NextSiblingElement()) {
		auto it = handlers.find(ch->Name());
		if (it == handlers.end())
			continue;
		if (!it->second(bmap, ch))
			return -1;
	}

	if (g_verbose) {
		std::cout << std::endl << "Using block map:" << std::endl;
		std::cout << "  ImageSize: " << bmap.image_size() << std::endl;
		std::cout << "  BlockSize: " << bmap.block_size() << std::endl;
		std::cout << "  BlocksCount: " << bmap.blocks_count() << std::endl;
		std::cout << "  BlockMap:" <<  std::endl;
		for (auto& r: bmap.mapped_ranges()) {
			if (r.first == r.second)
				std::cout << "    Range:  " << r.first << std::endl;
			else
				std::cout << "    Range:  " << r.first << "-" << r.second << std::endl;
		}
	}

	return true;
}
