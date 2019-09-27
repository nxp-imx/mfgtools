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

#include <string.h>
#include <iostream>
#include <fstream>
#include "libcomm.h"
#include "libuuu.h"
#include "liberror.h"

#include "fat.h"

int Fat::Open(string filename)
{
	m_filename = filename;

	shared_ptr<FileBuffer> pbuff = get_file_buffer(m_filename);
	if (pbuff == NULL)
		return -1;
	if (pbuff->size() < 512)
	{
		set_last_err_string("File too small");
		return -1;
	}
	if (pbuff->at(510) != 0x55|| pbuff->at(511) != 0xAA)
	{
		set_last_err_string("Partition signature miss matched");
		return -1;
	}

	Partition *pPart = (Partition *)(pbuff->data() + 446);

	m_fat_part_start = pPart->lba_start * 512;

	uint8_t *boot = pbuff->data() + m_fat_part_start;
	if (boot[510] != 0x55 || boot[511] != 0xAA)
	{
		set_last_err_string("Boot Sector signature miss matched");
		return -1;
	}

	m_logical_sector_perfat = boot[0x16];
	m_logical_sector_perfat += boot[0x17] << 8;
	if (m_logical_sector_perfat == 0)
	{
		m_logical_sector_perfat = boot[0x24];
		m_logical_sector_perfat += boot[0x25] << 8;
		m_logical_sector_perfat += boot[0x26] << 16;
		m_logical_sector_perfat += boot[0x27] << 24;
	}

	m_fat_table_offset = boot[0xE];
	m_fat_table_offset += boot[0xF] << 8;

	m_fat_table_offset *= 512;

	m_cluster = boot[0xD];
	m_cluster *= 512;

	int num_of_fat = boot[0x10];

	m_root_dir_offset = m_logical_sector_perfat * 512 * num_of_fat + m_fat_table_offset;

	m_num_of_rootdir = boot[0x11];
	m_num_of_rootdir = boot[0x12] << 8;

	FatDirEntry *entry;
	entry = (FatDirEntry*)(boot + m_root_dir_offset);
	m_filemap.clear();

	for (int i = 0; i < m_num_of_rootdir; i++)
	{
		string filename;
		if (entry->attr == 0x8)
			entry++;

		if (entry->filename[0] == 0)
			break;

		filename.clear();

		while (entry->attr == 0xF)
		{
			filename.insert(0, lfn2string((FatLFN *)entry));
			entry++;
		}

		if (filename.empty())
		{
			filename.append((char*)entry->filename, 8);
			if (entry->ext[0])
			{
				filename.append(".");
				filename.append((char*)entry->ext, 3);
			}
		}
		m_filemap[filename] = *entry;
		entry++;

		if (entry->filename[0] == 0)
			break;
	}
	return 0;
}

int Fat::get_next_cluster(shared_ptr<FileBuffer> p, int cluster)
{
	uint16_t *pfat = (uint16_t*)(p->data() +  m_fat_part_start + m_fat_table_offset);
	return pfat[cluster];
}

void *Fat::get_data_buff(shared_ptr<FileBuffer> p, int cluster)
{
	void *p1 = p->data() + m_fat_part_start + m_root_dir_offset + (cluster-2) * m_cluster + m_num_of_rootdir * 32;
	return p1;
}

int Fat::get_file_buff(string filename, shared_ptr<FileBuffer>p)
{
	if (m_filemap.find(filename) == m_filemap.end())
	{
		string err;
		err = "Can't find file ";
		err += filename;
		set_last_err_string(err);
		return -1;
	}

	shared_ptr<FileBuffer> pbuff = get_file_buffer(m_filename);

	size_t filesize = m_filemap[filename].file_size;
	p->resize(filesize);

	int cur = m_filemap[filename].start_cluster;

	size_t off;
	for (off = 0; off < filesize; off += m_cluster)
	{
		size_t sz;
		sz = filesize - off;
		if (sz > m_cluster)
			sz = m_cluster;

		if (cur == 0xFFFF)
		{
			set_last_err_string("Early finished at fat");
			return -1;
		}
		void *pcluster = get_data_buff(pbuff, cur);
		memcpy(p->data() + off, pcluster, sz);

		cur = get_next_cluster(pbuff, cur);
	}
	return 0;
}
