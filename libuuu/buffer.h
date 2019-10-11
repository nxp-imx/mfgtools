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
#include <string>
#include <condition_variable>
#include <string.h>

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#define mmap64 mmap
#endif

using namespace std;

#ifdef WIN32
class FileBuffer;
int file_overwrite_monitor(string filename, FileBuffer *p);
#endif 

//bit 0, data loaded
//bit 1, data total size known
#define FILEBUFFER_FLAG_LOADED_BIT		0x1
#define FILEBUFFER_FLAG_KNOWN_SIZE_BIT  0x2
#define FILEBUFFER_FLAG_ERROR_BIT		0x4

#define FILEBUFFER_FLAG_LOADED		(FILEBUFFER_FLAG_LOADED_BIT|FILEBUFFER_FLAG_KNOWN_SIZE_BIT) // LOADED must be knownsize
#define FILEBUFFER_FLAG_KNOWN_SIZE	FILEBUFFER_FLAG_KNOWN_SIZE_BIT

class FileBuffer: public enable_shared_from_this<FileBuffer>
{
public:
	mutex	m_data_mutex;

	uint8_t *m_pDatabuffer;
	size_t m_DataSize;
	size_t m_MemSize;

	shared_ptr<FileBuffer> m_ref;

	enum
	{
		ALLOCATE_MALLOC,
		ALLOCATE_MMAP,
		ALLOCATE_REF,
		ALLOCATE_VMALLOC,
	}m_allocate_way;

	int ref_other_buffer(shared_ptr<FileBuffer> p, size_t offset, size_t size)
	{
		m_pDatabuffer = p->data() + offset;
		m_DataSize = m_MemSize = size;
		m_allocate_way = ALLOCATE_REF;
		m_ref = p;
		return 0;
	}

	mutex m_async_mutex;
	
	atomic_int m_dataflags;

	thread m_aync_thread;

	atomic_size_t m_avaible_size;
	condition_variable m_request_cv;
	mutex m_requext_cv_mutex;

#ifdef WIN32
	OVERLAPPED m_OverLapped;
	REQUEST_OPLOCK_INPUT_BUFFER m_Request;
	HANDLE m_file_handle;
	HANDLE m_file_map;
	thread m_file_monitor;
#endif

	FileBuffer()
	{
		m_pDatabuffer = NULL;
		m_DataSize = 0;
		m_MemSize = 0;
		m_allocate_way = ALLOCATE_MALLOC;
		m_dataflags = 0;
		m_avaible_size = 0;
	}

	FileBuffer(void*p, size_t sz)
	{
		m_pDatabuffer = NULL;
		m_DataSize = 0;
		m_allocate_way = ALLOCATE_MALLOC;
		m_MemSize = 0;

		m_pDatabuffer = (uint8_t*)malloc(sz);
		m_MemSize = m_DataSize = sz;

		memcpy(m_pDatabuffer, p, sz);
		m_dataflags = 0;
	}

	~FileBuffer()
	{
		if(m_aync_thread.joinable())
			m_aync_thread.join();

		if (m_pDatabuffer)
		{
			if(m_allocate_way == ALLOCATE_MMAP)
				unmapfile();
			if(m_allocate_way == ALLOCATE_MALLOC)
				free(m_pDatabuffer);
		}
	}

	int request_data(vector<uint8_t> &data, size_t offset, size_t sz);
	int request_data(size_t total);

	bool IsLoaded()
	{
		return m_dataflags & FILEBUFFER_FLAG_LOADED_BIT;
	}

	bool IsKnownSize()
	{
		return m_dataflags & FILEBUFFER_FLAG_KNOWN_SIZE_BIT;
	}

	bool IsError()
	{
		return m_dataflags & FILEBUFFER_FLAG_ERROR_BIT;
	}
	uint8_t * data()
	{
		return m_pDatabuffer ;
	}

	size_t size()
	{
		return m_DataSize;
	}

	uint8_t & operator[] (size_t index)
	{
		assert(m_pDatabuffer);
		assert(index < m_DataSize);

		return *(m_pDatabuffer + index);
	}

	uint8_t & at(size_t index)
	{
		return (*this)[index];
	}
	int resize(size_t sz)
	{
		int ret = reserve(sz);

		m_DataSize = sz;
		return ret;
	}

	int reserve(size_t sz)
	{
		assert(m_allocate_way == ALLOCATE_MALLOC);

		if (sz > m_MemSize)
		{
			m_pDatabuffer = (uint8_t*)realloc(m_pDatabuffer, sz);
			m_MemSize = sz;

			if (m_pDatabuffer == NULL)
			{
				set_last_err_string("Out of memory\n");
				return -1;
			}
		}

		return 0;
	}

	int swap(FileBuffer & a)
	{
		std::swap(m_pDatabuffer, a.m_pDatabuffer);
		std::swap(m_DataSize, a.m_DataSize);
		std::swap(m_MemSize, a.m_MemSize);
		std::swap(m_allocate_way, a.m_allocate_way);

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

		m_pDatabuffer = (uint8_t *)MapViewOfFile(m_file_map, FILE_MAP_READ, 0, 0, sz);
		m_DataSize = sz;
		m_MemSize = sz;
		m_allocate_way = ALLOCATE_MMAP;
		
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

		m_pDatabuffer = (uint8_t *)mmap64(0, sz, PROT_READ, MAP_SHARED, fd, 0);
		if (m_pDatabuffer == MAP_FAILED) {
			m_pDatabuffer = NULL;
			set_last_err_string("mmap failure\n");
			return -1;
		}
		m_DataSize = sz;
		m_MemSize = sz;
		m_allocate_way = ALLOCATE_MMAP;
		close(fd);
#endif
		if (m_pDatabuffer)
			return 0;

		set_last_err_string("mmap file failure");
		return -1;
	}

	int unmapfile()
	{
		if (m_pDatabuffer)
		{
#ifdef _MSC_VER
			UnmapViewOfFile(m_pDatabuffer);
			m_pDatabuffer = NULL;
			CloseHandle(m_file_map);
			CloseHandle(m_file_handle);
			SetEvent(m_OverLapped.hEvent);
			
			if (m_file_monitor.joinable())
				m_file_monitor.join();

			CloseHandle(m_OverLapped.hEvent);
			m_OverLapped.hEvent = m_file_map = m_file_handle = INVALID_HANDLE_VALUE;
#else
			munmap(m_pDatabuffer, m_DataSize);
#endif
			m_pDatabuffer = NULL;
		}
		return 0;
	}
	//Read write lock;
	uint64_t m_timesample;
	int reload(string filename, bool async=false);
};

shared_ptr<FileBuffer> get_file_buffer(string filename, bool aysnc=false);
bool check_file_exist(string filename, bool start_async_load=true);

void set_current_dir(string dir);
