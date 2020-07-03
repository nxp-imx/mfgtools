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

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

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

#ifdef WIN32
class FileBuffer;
int file_overwrite_monitor(std::string filename, FileBuffer *p);
#endif 

//bit 0, data loaded
//bit 1, data total size known
#define FILEBUFFER_FLAG_LOADED_BIT		0x1
#define FILEBUFFER_FLAG_KNOWN_SIZE_BIT  0x2
#define FILEBUFFER_FLAG_ERROR_BIT		0x4

#define FILEBUFFER_FLAG_LOADED		(FILEBUFFER_FLAG_LOADED_BIT|FILEBUFFER_FLAG_KNOWN_SIZE_BIT) // LOADED must be knownsize
#define FILEBUFFER_FLAG_KNOWN_SIZE	FILEBUFFER_FLAG_KNOWN_SIZE_BIT

class FileBuffer: public std::enable_shared_from_this<FileBuffer>
{
public:
	enum class ALLOCATION_WAYS
	{
		MALLOC,
		MMAP,
		REF,
		VMALLOC,
	};

	std::mutex m_data_mutex;

	uint8_t *m_pDatabuffer;
	size_t m_DataSize;
	size_t m_MemSize;

	std::shared_ptr<FileBuffer> m_ref;

	int ref_other_buffer(std::shared_ptr<FileBuffer> p, size_t offset, size_t size);

	std::mutex m_async_mutex;
	
	std::atomic_int m_dataflags;

	std::thread m_aync_thread;

	std::atomic_size_t m_avaible_size;
	std::condition_variable m_request_cv;
	std::mutex m_requext_cv_mutex;

#ifdef WIN32
	OVERLAPPED m_OverLapped;
	REQUEST_OPLOCK_INPUT_BUFFER m_Request;
	HANDLE m_file_handle;
	HANDLE m_file_map;
	std::thread m_file_monitor;
#endif

	FileBuffer();
	FileBuffer(void*p, size_t sz);
	~FileBuffer();

	ALLOCATION_WAYS get_m_allocate_way() const noexcept { return m_allocate_way; }
	int request_data(std::vector<uint8_t> &data, size_t offset, size_t sz);
	int request_data(size_t total);

	bool IsLoaded() const noexcept
	{
		return m_dataflags & FILEBUFFER_FLAG_LOADED_BIT;
	}

	bool IsKnownSize() const noexcept
	{
		return m_dataflags & FILEBUFFER_FLAG_KNOWN_SIZE_BIT;
	}

	bool IsError() const noexcept
	{
		return m_dataflags & FILEBUFFER_FLAG_ERROR_BIT;
	}
	uint8_t * data() noexcept
	{
		return m_pDatabuffer ;
	}

	size_t size() const noexcept
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
	int resize(size_t sz);

	int reserve(size_t sz);

	int swap(FileBuffer & a);

	int mapfile(std::string filename, size_t sz);

	int unmapfile();
	//Read write lock;
	uint64_t m_timesample;
	int reload(std::string filename, bool async=false);

private:
	ALLOCATION_WAYS m_allocate_way = ALLOCATION_WAYS::MALLOC;
};

std::shared_ptr<FileBuffer> get_file_buffer(std::string filename, bool aysnc=false);
bool check_file_exist(std::string filename, bool start_async_load=true);

void set_current_dir(const std::string &dir);
