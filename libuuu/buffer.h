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
#include <map>
#include <queue>
#include "liberror.h"
#include <cstring>
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
#define FILEBUFFER_FLAG_NEVER_FREE		0x8
#define FILEBUFFER_FLAG_PARTIAL_RELOADABLE 0x10
#define FILEBUFFER_FLAG_SEG_DONE		0x20

#define FILEBUFFER_FLAG_LOADED		(FILEBUFFER_FLAG_LOADED_BIT|FILEBUFFER_FLAG_KNOWN_SIZE_BIT) // LOADED must be known size
#define FILEBUFFER_FLAG_KNOWN_SIZE	FILEBUFFER_FLAG_KNOWN_SIZE_BIT

class FileBuffer;
class FSBasic;

class FragmentBlock
{
public:
	enum
	{
		CONVERT_DONE = 0x1,
		USING = 0x2,
		CONVERT_START = 0x4,
		CONVERT_PARTIAL = 0x8,
	};
	size_t m_input_offset = 0;
	size_t m_input_sz = 0;
	std::shared_ptr<FileBuffer> m_input;
	size_t m_ret = 0;
	size_t m_actual_size = 0;
	size_t m_output_size = 0;
	size_t m_output_offset = 0;
	virtual int DataConvert() { return -1; };
	std::vector<uint8_t> m_data;
	std::mutex m_mutex;
	std::atomic_int m_dataflags{0};
	uint8_t* m_pData = NULL;
	uint8_t* data()
	{
		if (m_pData)
			return m_pData;
		return m_data.data();
	}

	virtual ~FragmentBlock() {}
};


class DataBuffer : public std::enable_shared_from_this<DataBuffer>
{
	enum class ALLOCATION_WAYS
	{
		MALLOC,
		REF,
	};

protected:

	ALLOCATION_WAYS get_m_allocate_way() const noexcept { return m_allocate_way; }
	uint8_t* m_pDatabuffer = NULL;
	size_t m_DataSize = 0;
	size_t m_MemSize = 0;
	std::shared_ptr<FileBuffer> m_ref;
	ALLOCATION_WAYS m_allocate_way = ALLOCATION_WAYS::MALLOC;

public:
	DataBuffer()
	{
		m_allocate_way = ALLOCATION_WAYS::MALLOC;
	}
	DataBuffer(void* p, size_t sz)
	{
		m_allocate_way = ALLOCATION_WAYS::MALLOC;
		resize(sz);
		memcpy(data(), p, sz);
	}
	uint8_t* data() { return m_pDatabuffer; }
	size_t size() { return m_DataSize; }
	int resize(size_t sz);
	int ref_other_buffer(std::shared_ptr<FileBuffer> p, size_t offset, size_t size);
	uint8_t& operator[] (size_t index)
	{
		assert(m_pDatabuffer);
		assert(index < m_DataSize);

		return *(m_pDatabuffer + index);
	}
	uint8_t& at(size_t index)
	{
		return (*this)[index];
	}
	virtual ~DataBuffer()
	{
		if (m_allocate_way == ALLOCATION_WAYS::MALLOC)
		{
			free(m_pDatabuffer);
		}
	}
	friend class FileBuffer;
};

class FileBuffer: public std::enable_shared_from_this<FileBuffer>
{
public:
	friend class DataBuffer;
	friend class FSBase;
	friend class FSFlat;
	friend class FSHttps;
	friend class FSHttp;
	friend class FSGz;
	friend class FSzstd;
	friend class FSCompressStream;
	friend class Fat;
	friend class Tar;
	friend class Zip;
	friend class Zip_file_Info;
	enum class ALLOCATION_WAYS
	{
		MALLOC,
		MMAP,
		REF,
		VMALLOC,
		SEGMENT,
	};

	std::mutex m_data_mutex;

	uint8_t *m_pDatabuffer;
	size_t m_DataSize;
	size_t m_MemSize;

	std::shared_ptr<FileBuffer> m_ref;

	int ref_other_buffer(std::shared_ptr<FileBuffer> p, size_t offset, size_t size);

	std::mutex m_async_mutex;

	std::map<size_t, std::shared_ptr<FragmentBlock>, std::greater<size_t>> m_seg_map;
	std::mutex m_seg_map_mutex;
	std::queue<size_t> m_offset_request;
	size_t m_last_request_offset = 0;
	std::condition_variable m_pool_load_cv;
	std::mutex m_pool_load_cv_mutex;
	std::shared_ptr<FragmentBlock> m_last_db;
	size_t m_seg_blk_size = 0x800000;
	size_t m_total_buffer_size = 8 * m_seg_blk_size;
	std::atomic_bool m_reset_stream { false };

	//used for continue decompress\loading only
	std::shared_ptr<FragmentBlock> request_new_blk();
	bool check_offset_in_seg(size_t offset, std::shared_ptr<FragmentBlock> blk)
	{
		if (offset >= blk->m_output_offset
			&& offset < blk->m_output_offset + blk->m_output_size)
			return true;

		return false;
	}
	std::shared_ptr<FragmentBlock> get_map_it(size_t offset, bool alloc = false)
	{
		{
			std::lock_guard<std::mutex> lock(m_seg_map_mutex);
			auto it = m_seg_map.lower_bound(offset);
			if ( it == m_seg_map.end())
				return NULL;

			auto blk = it->second;
			if (check_offset_in_seg(offset, blk))
			{
				if (alloc)
				{
					std::lock_guard<std::mutex> lck(blk->m_mutex);
					blk->m_data.resize(blk->m_output_size);
				}
				return blk;
			}
			return NULL;
		}
	}
	void truncate_old_data_in_pool();

	std::atomic_int m_dataflags;

	std::thread m_async_thread;

	std::atomic_size_t m_available_size;
	std::condition_variable m_request_cv;
	std::mutex m_request_cv_mutex;

#ifdef WIN32
	OVERLAPPED m_OverLapped;
	REQUEST_OPLOCK_INPUT_BUFFER m_Request;
	HANDLE m_file_handle;
	HANDLE m_file_map;
	std::thread m_file_monitor;
#endif

	uint64_t m_timesample;

	FileBuffer();
	FileBuffer(void*p, size_t sz);
	~FileBuffer();

	ALLOCATION_WAYS get_m_allocate_way() const noexcept { return m_allocate_way; }

	int64_t request_data(void * data, size_t offset, size_t sz);
	int request_data(std::vector<uint8_t> &data, size_t offset, size_t sz);
	std::shared_ptr<DataBuffer> request_data(size_t offset, size_t sz);

	bool IsLoaded() const noexcept
	{
		return m_dataflags & FILEBUFFER_FLAG_LOADED_BIT;
	}

	bool IsRefable() const noexcept
	{
		return m_dataflags & FILEBUFFER_FLAG_NEVER_FREE;
	}

	bool IsKnownSize() const noexcept
	{
		return m_dataflags & FILEBUFFER_FLAG_KNOWN_SIZE_BIT;
	}

	bool IsError() const noexcept
	{
		return m_dataflags & FILEBUFFER_FLAG_ERROR_BIT;
	}

	int reload(std::string filename, bool async = false);

	size_t size()
	{
		if (IsKnownSize())
			return m_DataSize;

		std::unique_lock<std::mutex> lck(m_request_cv_mutex);
		while (!(m_dataflags & FILEBUFFER_FLAG_KNOWN_SIZE_BIT))
			m_request_cv.wait(lck);

		return m_DataSize;
	}

protected:
	uint8_t * data() noexcept
	{
		return m_pDatabuffer ;
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

	int mapfile(const std::string &filename, size_t sz);

	int unmapfile();
	//Read write lock;

protected:
	int64_t request_data_from_segment(void* data, size_t offset, size_t sz);
	int m_pool_size = 10;
	std::string m_filename;
private:
	ALLOCATION_WAYS m_allocate_way = ALLOCATION_WAYS::MALLOC;
};

std::shared_ptr<FileBuffer> get_file_buffer(std::string filename, bool async=false);
bool check_file_exist(std::string filename, bool start_async_load=true);

void set_current_dir(const std::string &dir);

bool IsMBR(std::shared_ptr<DataBuffer> p);
size_t ScanTerm(std::shared_ptr<DataBuffer> p, size_t& pos, size_t offset = 512, size_t limited = 0x800000);
void clean_up_filemap();
