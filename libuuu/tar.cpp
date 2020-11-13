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

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "zlib.h"
#include <memory>
#include "buffer.h"
#include "liberror.h"
#include "libuuu.h"
#include <stdio.h>
#include <string.h>
#include "tar.h"
#include <fstream>
#include <iostream>
using namespace std;

int Tar::Open(const string &filename)
{
	bool end_of_file=false;
	char end_of_file_blocks[2*TAR_BLOCK_SIZE];
	memset(end_of_file_blocks, 0, sizeof(end_of_file_blocks) );
	m_tarfilename=filename;

	shared_ptr<FileBuffer> file = get_file_buffer(filename);
	if(file == nullptr)
		return -1;

	uint8_t* data=file->data();
	uint64_t block_counter=0;
	while(!end_of_file)
	{
		if(!memcmp(end_of_file_blocks,data+block_counter*TAR_BLOCK_SIZE,TAR_BLOCK_SIZE))
		{
			end_of_file=true;
			break;
		}
		struct Tar_header* th=(Tar_header*)(data+block_counter*TAR_BLOCK_SIZE);
		uint64_t size;
		string octal_str((char*)th->size);
		//printf("block_counter: %d\n",block_counter );
		//printf("name: %s\n",th->name );
		//printf("signature: %s\n",th->ustar );
		//printf("size: %s\n", th->size);
		size=stoll(octal_str, 0, 8);
		string name((char*)th->name);
		m_filemap[name].size=size;
		m_filemap[name].offset=(block_counter+1)*TAR_BLOCK_SIZE; //+1 because the data located right after the header block
		m_filemap[name].filename.assign((char*)th->name);
		block_counter++;

		//skip the data blocks
		uint64_t data_block_num=size/TAR_BLOCK_SIZE;
		data_block_num += (size%TAR_BLOCK_SIZE>0)? 1:0;
		block_counter+=data_block_num;
	}
	return 0;
}

bool Tar::check_file_exist(const string &filename)
{

	if (m_filemap.find(filename) == m_filemap.end())
	{
		string err;
		err += "Can't find file ";
		err += filename;
		set_last_err_string(err);
		return false;
	}

	return true;
}


int Tar::get_file_buff(const string &filename, shared_ptr<FileBuffer> p )
{

	if (m_filemap.find(filename) == m_filemap.end())
	{
		string err;
		err += "Can't find file ";
		err += filename;
		set_last_err_string(err);
		return -1;
	}

	p->resize(m_filemap[filename].size);
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_KNOWN_SIZE);
	p->m_request_cv.notify_all();

	shared_ptr<FileBuffer> file;
	file = get_file_buffer(m_tarfilename);
	size_t offset= m_filemap[filename].offset;
	size_t size=m_filemap[filename].size;

	p->ref_other_buffer(file, offset, size);
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	p->m_request_cv.notify_all();

	return 0;
}

