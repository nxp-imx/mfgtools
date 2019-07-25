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
#include <stdint.h>
#include "zip.h"

#pragma pack(1)
struct Partition
{
	uint8_t status;
	uint8_t start_head;
	uint8_t start_sector;
	uint8_t start_cylinder;
	uint8_t type;
	uint8_t end_head;
	uint8_t end_sector;
	uint8_t end_cylinder;
	uint32_t lba_start;
	uint32_t lba_num;
};

struct FatDirEntry
{
	uint8_t filename[8];
	uint8_t ext[3];
	uint8_t attr;
	uint8_t user_attr;
	uint8_t delele_char;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t userid;
	uint16_t access;
	uint16_t modify_time;
	uint16_t modify_date;
	uint16_t start_cluster;
	uint32_t file_size;
};

struct FatLFN
{
	uint8_t seq;
	uint8_t name1[10];
	uint8_t attr;
	uint8_t type;
	uint8_t sum;
	uint8_t name2[12];
	uint16_t start_cluster;
	uint8_t name3[4];
};

#pragma pack()

class Fat : public Backfile
{
public:
	uint64_t m_fat_part_start;
	uint64_t m_fat_table_offset;
	uint64_t m_root_dir_offset;
	uint64_t m_logical_sector_perfat;
	uint64_t m_cluster;
	int m_num_of_rootdir;

	map<string, FatDirEntry> m_filemap;

	int Open(string filename);
	int get_file_buff(string filename, shared_ptr<FileBuffer>p);

	int get_next_cluster(shared_ptr<FileBuffer> p, int cluster);
	void *get_data_buff(shared_ptr<FileBuffer> p, int cluster);

	string lfn2string(FatLFN *p)
	{
		string str;
		for (int i = 0; i < 10; i += 2)
			if (p->name1[i] == 0)
				return str;
			else
				str += p->name1[i];

		for (int i = 0; i < 12; i += 2)
			if (p->name2[i] == 0)
				return str;
			else
				str += p->name2[i];

		for (int i = 0; i < 4; i += 2)
			if (p->name3[i] == 0)
				return str;
			else
				str += p->name3[i];

		return str;
	}
};