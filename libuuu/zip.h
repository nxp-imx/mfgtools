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

#include "backfile.h"

#include "zlib.h"

#include <cstdint>
#include <map>
#include <memory>

/* Allow opportunistic use of the C++17 fall-through attribute . */
#if defined(__cplusplus) && __cplusplus >= 201703L
#define FALLTHROUGH [[fallthrough]]
#else
#define FALLTHROUGH
#endif

class FileBuffer;

#pragma pack(1)
struct Zip_data_desc
{
	uint32_t sign;
	uint32_t crc;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
};

struct Zip_file_desc
{
	uint32_t sign;
	uint16_t version_mini_extract;
	uint16_t flags;
	uint16_t compress_method;
	uint16_t last_modify_time;
	uint16_t last_modify_date;
	uint32_t crc;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t file_name_length;
	uint16_t extrafield_length;
	uint8_t filename[0];
};
struct Zip_central_dir
{
	uint32_t sign;
	uint16_t version;
	uint16_t version_mini_extract;
	uint16_t flags;
	uint16_t compress_method;
	uint16_t last_modify_time;
	uint16_t last_modify_date;
	uint32_t crc;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t file_name_length;
	uint16_t extrafield_length;
	uint16_t file_comment_length;
	uint16_t disk_number;
	uint16_t internal_file_attr;
	uint32_t external_file_attr;
	uint32_t offset;
	uint8_t filename[0];
};

struct Zip64_central_dir
{
	uint32_t sign;
	uint16_t version;
	uint16_t version_mini_extract;
	uint16_t flags;
	uint16_t compress_method;
	uint16_t last_modify_time;
	uint16_t last_modify_date;
	uint32_t crc;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t file_name_length;
	uint16_t extrafield_length;
	uint16_t file_comment_length;
	uint16_t disk_number;
	uint16_t internal_file_attr;
	uint32_t external_file_attr;
	uint32_t offset;
	uint8_t filename[0];
};
struct Zip_eocd
{
	uint32_t sign;
	uint16_t num_of_thisdisk;
	uint16_t start_disk_of_dir;
	uint16_t num_of_dir_ondisk;
	uint16_t num_of_dir;
	uint32_t size_of_central_dir;
	uint32_t offset_of_central_dir;
	uint16_t length_of_comment;
	uint8_t  comment[0];
};

struct Zip64_eocd_locator
{
	uint32_t sign;
	uint32_t num_of_thisdisk;
	uint64_t offset_of_eocd;
	uint32_t total_num_disks;
};

struct Zip64_eocd
{
	uint32_t sign;
	uint64_t size_of_eocd;
	uint16_t version;
	uint16_t version_mini_extract;
	uint32_t num_of_dir_ondisk;
	uint32_t num_of_disk;
	uint64_t total_ondisk;
	uint64_t total;
	uint64_t size;
	uint64_t offset;
};

struct Zip_ext
{
	uint16_t tag;
	uint16_t size;
};

#define EOCD_SIGNATURE 0x06054b50
#define DIR_SIGNATURE 0x02014b50
#define DATA_SIGNATURE 0x08074b50
#define FILE_SIGNATURE 0x04034b50
#define EOCD64_LOCATOR_SIGNATURE 0x07064b50
#define EOCD64_SIGNATURE 0x06064b50

class Zip;

class Zip_file_Info
{
public:
	Zip_file_Info();
	~Zip_file_Info();

	int decompress(Zip *pZip, std::shared_ptr<FileBuffer> p);

private:
	std::string m_filename;
	uint32_t m_timestamp;
	size_t m_filesize;
	size_t m_compressedsize;
	size_t m_offset;
	z_stream m_strm;

	friend Zip;
};

class Zip : public Backfile
{
public:
	int BuildDirInfo();
	bool check_file_exist(std::string filename);
	int get_file_buff(std::string filename, std::shared_ptr<FileBuffer>p);
	int Open(std::string filename);

	std::map<std::string, Zip_file_Info> m_filemap;
};


#pragma pack()
