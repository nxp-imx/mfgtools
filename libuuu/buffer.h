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

#include <vector>
#include <memory>
#include "liberror.h"
#include <assert.h>

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif
using namespace std;

class FileBuffer
{
public:
	vector<uint8_t> m_data;
	uint8_t *m_pMapbuffer;
	size_t m_MapSize;

	FileBuffer()
	{
		m_pMapbuffer = NULL;
		m_MapSize = 0;
	}
	~FileBuffer()
	{
		unmapfile();
	}

	uint8_t * data()
	{
		return m_pMapbuffer ? m_pMapbuffer : m_data.data();
	}

	size_t size()
	{
		return m_pMapbuffer ? m_MapSize : m_data.size();
	}

	uint8_t & operator[] (size_t index)
	{
		if (m_pMapbuffer) {
			assert(index < m_MapSize);
			return *(m_pMapbuffer + index);
		}
		else {
			return m_data[index];
		}
	}

	uint8_t & at(size_t index)
	{
		return (*this)[index];
	}
	void resize(size_t sz)
	{
		return m_data.resize(sz);
	}
	int swap(FileBuffer & a)
	{
		m_data.swap(a.m_data);
		std::swap(m_pMapbuffer, a.m_pMapbuffer);
		std::swap(m_MapSize, a.m_MapSize);
		return 0;
	}

	int mapfile(string filename, size_t sz)
	{
#ifdef _MSC_VER
		HANDLE file_handle = CreateFile(filename.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
			NULL);
		if (file_handle == INVALID_HANDLE_VALUE)
		{
			string err = "Create File Faiure ";
			err += filename;
			set_last_err_string(err);
			return -1;
		}

		HANDLE file_map = CreateFileMapping(file_handle,
			NULL, PAGE_READONLY, 0, 0, NULL);

		if (file_map == INVALID_HANDLE_VALUE)
		{
			set_last_err_string("Fail create Map");
			return -1;
		}

		m_pMapbuffer = (uint8_t *)MapViewOfFile(file_map, FILE_MAP_READ, 0, 0, sz);
		m_MapSize = sz;
		CloseHandle(file_map);
		CloseHandle(file_handle);
#else
		int fd = open(filename.c_str(), O_RDONLY);
		if (fd == -1)
		{
			string err;
			err += "xx Failure open file: ";
			err + filename;
			set_last_err_string(err);
			return -1;
		}

		m_pMapbuffer = (uint8_t *)mmap64(0, sz, PROT_READ, MAP_SHARED, fd, 0);
		if (m_pMapbuffer == MAP_FAILED) {
			m_pMapbuffer = NULL;
			set_last_err_string("mmap failure\n");
			return -1;
		}
		m_MapSize = sz;
		close(fd);
#endif
		if (m_pMapbuffer)
			return 0;

		set_last_err_string("mmap file failure");
		return -1;
	}

	int unmapfile()
	{
		if (m_pMapbuffer)
		{
#ifdef _MSC_VER
			UnmapViewOfFile(m_pMapbuffer);
#else
			munmap(m_pMapbuffer, m_MapSize);
#endif
			m_pMapbuffer = NULL;
		}
		return 0;
	}
	//Read write lock;
	uint64_t m_timesample;
	int reload(string filename);
};

shared_ptr<FileBuffer> get_file_buffer(string filename);
void set_current_dir(string dir);
