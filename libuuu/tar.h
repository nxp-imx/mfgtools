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

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>

class FileBuffer;

#define TAR_BLOCK_SIZE 512

#pragma pack(1)
struct Tar_header
{
	uint8_t name[100];
	uint8_t mode[8];
	uint8_t owner_id[8];
	uint8_t group_id[8];
	uint8_t size[12];
	uint8_t modi_time[12];
	uint8_t checksum[8];
	uint8_t type[1];
	uint8_t linkname[100];
	uint8_t ustar[6];
	uint8_t version[2];
	uint8_t uname[32];
	uint8_t gname[32];
	uint8_t major_num[8];
	uint8_t minor_num[8];
	uint8_t prefix[155];
};
#pragma pack()

class Tar_file_Info
{
public:
	std::string filename;
	uint64_t offset;
	uint64_t size;
};


class Tar
{
	std::string m_tarfilename;

public:
	std::map<std::string, Tar_file_Info> m_filemap;
	int Open(const std::string &filename);
	bool check_file_exist(const std::string &filename);
	int get_file_buff(const std::string &filename, std::shared_ptr<FileBuffer> p);
};
