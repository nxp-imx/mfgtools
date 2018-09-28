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
#include <mutex>
#include <atomic>
#include <thread>

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif
using namespace std;

#ifdef WIN32
class FileBuffer;
int file_overwrite_monitor(string filename, FileBuffer *p);
#endif 

class FileBuffer
{
public:
	vector<uint8_t> m_data;
	uint8_t *m_pMapbuffer;
	size_t m_MapSize;
	mutex m_async_mutex;
	atomic_bool m_loaded;
	thread m_aync_thread;

#ifdef WIN32
	OVERLAPPED m_OverLapped;
	REQUEST_OPLOCK_INPUT_BUFFER m_Request;
	HANDLE m_file_handle;
	HANDLE m_file_map;
	thread m_file_monitor;
#endif

	FileBuffer()
	{
		m_pMapbuffer = NULL;
		m_MapSize = 0;
		m_loaded = false;
	}
	~FileBuffer()
	{
		if(m_aync_thread.joinable())
			m_aync_thread.join();
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
		
		m_Request.StructureVersion = REQUEST_OPLOCK_CURRENT_VERSION;
		m_Request.StructureLength = sizeof(REQUEST_OPLOCK_INPUT_BUFFER);
		m_Request.RequestedOplockLevel = (OPLOCK_LEVEL_CACHE_READ | OPLOCK_LEVEL_CACHE_HANDLE);
		m_Request.Flags = REQUEST_OPLOCK_INPUT_FLAG_REQUEST;
		
		REQUEST_OPLOCK_OUTPUT_BUFFER Response;

		m_OverLapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		ResetEvent(m_OverLapped.hEvent);

		m_file_handle = CreateFile(filename.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED,
			NULL);

		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			string err = "Create File Failure ";
			err += filename;
			set_last_err_string(err);
			return -1;
		}

		BOOL bSuccess = DeviceIoControl(m_file_handle,
			FSCTL_REQUEST_OPLOCK,
			&m_Request,
			sizeof(m_Request),
			&Response,
			sizeof(Response),
			NULL,
			&m_OverLapped);

		if (bSuccess || GetLastError() == ERROR_IO_PENDING)
		{
			m_file_monitor = thread(file_overwrite_monitor, filename, this);
		}

		m_file_map = CreateFileMapping(m_file_handle,
			NULL, PAGE_READONLY, 0, 0, NULL);

		if (m_file_map == INVALID_HANDLE_VALUE)
		{
			set_last_err_string("Fail create Map");
			return -1;
		}

		m_pMapbuffer = (uint8_t *)MapViewOfFile(m_file_map, FILE_MAP_READ, 0, 0, sz);
		m_MapSize = sz;
		
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
			m_pMapbuffer = NULL;
			CloseHandle(m_file_map);
			CloseHandle(m_file_handle);
			SetEvent(m_OverLapped.hEvent);
			
			if (m_file_monitor.joinable())
				m_file_monitor.join();

			CloseHandle(m_OverLapped.hEvent);
			m_OverLapped.hEvent = m_file_map = m_file_handle = INVALID_HANDLE_VALUE;
#else
			munmap(m_pMapbuffer, m_MapSize);
#endif
			m_pMapbuffer = NULL;
		}
		return 0;
	}
	//Read write lock;
	uint64_t m_timesample;
	int reload(string filename, bool async=false);
};

shared_ptr<FileBuffer> get_file_buffer(string filename);
bool check_file_exist(string filename, bool start_async_load=true);

void set_current_dir(string dir);
