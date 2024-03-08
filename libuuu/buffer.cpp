/*
* Copyright 2018-2022 NXP.
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

#include <map>
#include "buffer.h"
#include <sys/stat.h>
#include "liberror.h"
#include <iostream>
#include <fstream>
#include "libcomm.h"
#include "libuuu.h"
#include "zip.h"
#include "fat.h"
#include "tar.h"
#include <string.h>
#include "bzlib.h"
#include "stdio.h"
#include <limits>
#include "http.h"
#include "zstd.h"
#include "libusb.h"

#ifdef WIN32
#define stat_os _stat64
#elif defined(__APPLE__)
#define stat_os stat
#include "dirent.h"
#else
#define stat_os stat64
#include "dirent.h"
#endif

static map<string, shared_ptr<FileBuffer>> g_filebuffer_map;
static mutex g_mutex_map;
static bool g_small_memory = true;

#define MAGIC_PATH '>'

string g_current_dir = ">";

void set_current_dir(const string &dir)
{
	g_current_dir = MAGIC_PATH;
	g_current_dir += dir;
}


int DataBuffer:: resize(size_t sz)
{
	if (m_allocate_way != ALLOCATION_WAYS::MALLOC)
	{
		set_last_err_string("data buffer ref can't resize");
		assert(false);
		return -1;
	}

	if (sz > m_MemSize)
	{
		m_pDatabuffer = (uint8_t*)realloc(m_pDatabuffer, sz);
		if (!m_pDatabuffer)
		{
			set_last_err_string("fail alloc memory");
			return -1;
		}
		m_MemSize = sz;
	}

	m_DataSize = sz;
	return 0;
}

int DataBuffer::ref_other_buffer(std::shared_ptr<FileBuffer> p, size_t offset, size_t size)
{
	if (!p->IsRefable())
		return -1;

	if (p->m_allocate_way == FileBuffer::ALLOCATION_WAYS::SEGMENT)
	{
		shared_ptr<FragmentBlock> blk;
		blk = p->get_map_it(offset);
		if (offset + size < blk->m_output_offset + blk->m_actual_size)
		{
			m_pDatabuffer = blk->m_data.data() + offset - blk->m_output_offset;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		m_pDatabuffer = p->data() + offset;
	}

	m_DataSize = size;

	m_allocate_way = ALLOCATION_WAYS::REF;
	m_ref = p;
	return 0;
};

class FSBasic
{
public:
	friend class DataBuffer;
	friend class FileBuffer;
	virtual int get_file_timesample(const string &filename, uint64_t *ptime) = 0;
	virtual int load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p) = 0;
	virtual bool exist(const string &backfile, const string &filename) = 0;
	virtual int for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p) = 0;

	virtual int Decompress(const string& /*backfifle*/, shared_ptr<FileBuffer> /*outp*/) { return 0; };
	virtual bool seekable(const string& /*backfile*/) { return false; }
	virtual std::shared_ptr<FragmentBlock> ScanCompressblock(const string& /*backfile*/, size_t& /*input_offset*/, size_t& /*output_offset*/) { return NULL; };
	virtual int PreloadWorkThread(shared_ptr<FileBuffer>outp);

	virtual int split(const string &filename, string *outbackfile, string *outfilename, bool dir=false)
	{
		string path = str_to_upper(filename);
		if (m_ext == nullptr || strlen(m_ext) == 0)
		{
			if(dir)
			{
				size_t pos = path.rfind("/");
				if(pos == string::npos)
				{
					*outbackfile = MAGIC_PATH;
					*outbackfile += "./";
					*outfilename = filename;
				} else {
					*outbackfile = filename.substr(0, pos);
					if(filename.size() >= pos + 1)
						*outfilename = filename.substr(pos + 1);
					else
						outfilename->clear();
				}
			}else
			{
				*outbackfile = filename;
			}
			return 0;
		}

		string ext = m_ext;
		if(!dir)
			ext += "/";
		size_t pos = path.rfind(ext);
		if (pos == string::npos)
		{
			string err = "can't find ext name in path: ";
			err += filename;
			set_last_err_string(err);
			return -1;
		}

		*outbackfile = filename.substr(0, pos + strlen(m_ext));

		if(filename.size() >= pos + strlen(m_ext) + 1)
			*outfilename = filename.substr(pos + strlen(m_ext) + 1);
		else
			outfilename->clear();
		return 0;
	}

protected:
	const char * m_ext = nullptr;
	const char * m_Prefix = nullptr;
public:
	bool	m_small_pool = false;
};

static class FSFlat: public FSBasic
{
public:
	FSFlat() { m_ext = ""; }
	int get_file_timesample(const string &filename, uint64_t *ptime) override
	{
		struct stat_os st;
		if (stat_os(filename.c_str() + 1, &st))
		{
			set_last_err_string("stat_os failure");
			return -1;
		}

		*ptime = st.st_mtime;

		return 0;
	}

	bool exist(const string &backfile, const string & /*filename*/) override
	{
		struct stat_os st;
		int off = 1;

		if (backfile[0] != MAGIC_PATH)
			off = 0;

		return stat_os(backfile.c_str() + off, &st) == 0 && ((st.st_mode & S_IFDIR) == 0);
	}

	int load(const string &backfile, const string &/*filename*/, shared_ptr<FileBuffer> p) override
	{
		struct stat_os st;
		if (stat_os(backfile.c_str() + 1, &st))
		{
			set_last_err_string("stat_os failure");
			return -1;
		}
		p->unmapfile();

		if (p->mapfile(backfile.substr(1), st.st_size))
			return -1;

		p->m_available_size = st.st_size;

		atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED | FILEBUFFER_FLAG_NEVER_FREE);
		p->m_request_cv.notify_all();

		return 0;
	}

	int for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p) override
	{
		struct stat_os st;

		if(stat_os(backfile.c_str() + 1, &st))
		{
			return -1;
		}

		if(st.st_mode & S_IFDIR)
		{
#ifdef WIN32
			string str = backfile.substr(1);
			if (filename.empty())
				str += "/*";
			else
				str += "/" + filename;

			WIN32_FIND_DATA fd;
			HANDLE handle = FindFirstFile(str.c_str(), &fd);
			BOOL b = false;
			do
			{
				if (handle == INVALID_HANDLE_VALUE)
					break;
				string path = backfile + "/" + fd.cFileName;
				if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
					path += "/";
				fn(path.c_str() + 1, p);
			} while (FindNextFile(handle, &fd));
			CloseHandle(handle);
			return 0;
#else
			DIR *dir;
			dir = opendir(backfile.c_str() + 1);
			struct dirent *dp;
			while ((dp=readdir(dir)) != nullptr)
			{
				string name = dp->d_name;
				if(name.substr(0, filename.size()) == filename || filename.empty())
				{
					string path = backfile + "/" + name;
					if(dp->d_type == DT_DIR)
						path += "/";
					fn(path.c_str() + 1, p);
				}
			}
			closedir(dir);
			return 0;
#endif
		}else
		{
			return fn(backfile.c_str() + 1, p);
		}
	}

} g_fsflat;

class FSNetwork : public FSBasic
{
protected:
	int m_Port;

public:
	int split(const string &filename, string *outbackfile, string *outfilename, bool /*dir = false*/) override
	{
		if (m_Prefix == nullptr)
			return -1;

		if (filename.size() < strlen(m_Prefix))
			return -1;

		string path = str_to_upper(filename);
		if (path.compare(1, strlen(m_Prefix), m_Prefix) == 0)
		{
			size_t pos;
			pos = filename.find('/', 1 + strlen(m_Prefix));

			*outbackfile = filename.substr(1 + strlen(m_Prefix), pos - 1 - strlen(m_Prefix));

			size_t cpos;
			cpos = outbackfile->find(':');
			if (cpos != string::npos)
			{
				m_Port = str_to_uint32(outbackfile->substr(cpos + 1));

				*outbackfile = outbackfile->substr(0, cpos);
			}
			*outfilename = filename.substr(pos);

			return 0;
		}

		return -1;
	}
};

static class FSHttp : public FSNetwork
{
public:
	FSHttp() { m_Prefix = "HTTP://"; m_Port = 80; }
	int load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p) override;
	virtual bool exist(const string &backfile, const string &filename) override
	{
		shared_ptr<HttpStream> http = make_shared<HttpStream>();

		if (http->HttpGetHeader(backfile, filename, m_Port, typeid(*this) != typeid(FSHttp)))
			return false;

		return true;
	};
	int for_each_ls(uuu_ls_file /*fn*/, const string &/*backfile*/, const string &/*filename*/, void * /*p*/) override { return 0; };
	int get_file_timesample(const string &/*filename*/, uint64_t * /*ptime*/) override { return 0; };
	int http_load(shared_ptr<HttpStream> http, shared_ptr<FileBuffer> p, string filename);
}g_fshttp;

static class FSHttps : public FSHttp
{
public:
	FSHttps() { m_Prefix = "HTTPS://"; m_Port = 443; }
}g_fshttps;

int FSHttp::http_load(shared_ptr<HttpStream> http, shared_ptr<FileBuffer> p, string filename)
{
	size_t max = 0x10000;

	uuu_notify ut;
	ut.type = uuu_notify::NOTIFY_DOWNLOAD_START;
	ut.str = (char*)filename.c_str();
	call_notify(ut);

	ut.type = uuu_notify::NOTIFY_TRANS_SIZE;
	ut.total = p->size();
	call_notify(ut);

	for (size_t i = 0; i < p->size() && !p->m_reset_stream; i += max)
	{
		size_t sz = p->size() - i;
		if (sz > max)
			sz = max;
		if (http->HttpDownload((char*)(p->data() + i), sz) < 0)
		{
			atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_ERROR_BIT);
			p->m_request_cv.notify_all();
			return -1;
		}
		p->m_available_size = i + sz;
		p->m_request_cv.notify_all();

		ut.type = uuu_notify::NOTIFY_TRANS_POS;
		ut.total = i + sz;
		call_notify(ut);
	}

	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED | FILEBUFFER_FLAG_NEVER_FREE);
	p->m_request_cv.notify_all();

	ut.type = uuu_notify::NOTIFY_DOWNLOAD_END;
	ut.str = (char*)filename.c_str();
	call_notify(ut);
	return 0;
}

class FSBackFile : public FSBasic
{
public:
	int get_file_timesample(const string &filename, uint64_t *ptime) override;

};

static class FSZip : public FSBackFile
{
public:
	FSZip() { m_ext = ".ZIP"; };
	int load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p) override;
	bool exist(const string &backfile, const string &filename) override;
	int for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p) override;
}g_fszip;

static class FSTar: public FSBackFile
{
public:
	FSTar() {m_ext = ".TAR"; };
	int load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p) override;
	bool exist(const string &backfile, const string &filename) override;
	int for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p) override;
}g_fstar;


static class FSFat : public FSBackFile
{
public:
	FSFat() { m_ext = ".SDCARD"; };
	int load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p) override;
	bool exist(const string &backfile, const string &filename) override;
	int for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p) override;
}g_fsfat;


class CommonStream
{
public:
	virtual int set_input_buff(void* p, size_t sz) = 0;
	virtual int set_output_buff(void* p, size_t sz) = 0;
	virtual size_t get_input_pos() = 0;
	virtual size_t get_output_pos() = 0;
	virtual int decompress() = 0;

	virtual size_t get_default_input_size() { return 0x1000; }
	virtual size_t decompress_size(const string& /*backfile*/) { return 0; }
};

class FSCompressStream : public FSBackFile
{
public:
	FSCompressStream() { m_small_pool = g_small_memory; }
	int load(const string& backfile, const string& filename, shared_ptr<FileBuffer>outp) override;
	bool exist(const string& backfile, const string& filename) override;
	int for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p) override;
	int Decompress(const string& backfile, shared_ptr<FileBuffer>outp) override;
	virtual std::shared_ptr<CommonStream> create_stream() { return nullptr; };
};


class Bz2stream : public CommonStream
{
	bz_stream m_strm;
	size_t m_in_size = 0;
	size_t m_out_size = 0;

public:
	Bz2stream() { memset(&m_strm, 0, sizeof(m_strm));  BZ2_bzDecompressInit(&m_strm, 0, 0); }
	virtual ~Bz2stream()
	{
		BZ2_bzDecompressEnd(&m_strm);
	}
	virtual int set_input_buff(void* p, size_t sz) override
	{
		m_strm.next_in = (char*)p;
		m_strm.avail_in = m_in_size = sz;
		return 0;
	};
	virtual int set_output_buff(void* p, size_t sz) override
	{
		m_strm.next_out = (char*)p;
		m_strm.avail_out = m_out_size = sz;
		return 0;
	};
	virtual size_t get_input_pos() override
	{
		return m_in_size - m_strm.avail_in;
	};
	virtual size_t get_output_pos() override
	{
		return m_out_size - m_strm.avail_out;
	};
	virtual int decompress() override
	{
		return BZ2_bzDecompress(&m_strm);
	};

	virtual size_t get_default_input_size() override { return 0x10000; }
};

static class FSBz2 : public FSCompressStream
{
public:
	FSBz2() { m_ext = ".BZ2"; };
	virtual bool seekable(const string& backfile) override;
	virtual std::shared_ptr<CommonStream> create_stream() override { return std::make_shared<Bz2stream>(); }
	virtual std::shared_ptr<FragmentBlock> ScanCompressblock(const string& backfile, size_t& input_offset, size_t& output_offset) override;

}g_fsbz2;

class Gzstream : public CommonStream
{
	z_stream m_strm;
	size_t m_in_size = 0;
	size_t m_out_size = 0;

public:
	Gzstream()
	{
		memset(&m_strm, 0, sizeof(m_strm));
		inflateInit2(&m_strm, 15 + 16);
	}
	virtual ~Gzstream()
	{
		deflateEnd(&m_strm);
	}
	virtual int set_input_buff(void* p, size_t sz) override
	{
		m_strm.next_in = (Bytef*)p;
		m_strm.avail_in = m_in_size = sz;
		return 0;
	};
	virtual int set_output_buff(void* p, size_t sz) override
	{
		m_strm.next_out = (Bytef*)p;
		m_strm.avail_out = m_out_size = sz;
		return 0;
	};
	virtual size_t get_input_pos() override
	{
		return m_in_size - m_strm.avail_in;
	};
	virtual size_t get_output_pos() override
	{
		return m_out_size - m_strm.avail_out;
	};
	virtual int decompress() override
	{
		return inflate(&m_strm, Z_SYNC_FLUSH);
	};

	virtual size_t get_default_input_size() override { return 0x10000; }
};

static class FSGz : public FSCompressStream
{
public:
	FSGz() { m_ext = ".GZ"; };
	virtual std::shared_ptr<CommonStream> create_stream() { return std::make_shared<Gzstream>(); }
}g_fsgz;

class ZstdStream:public CommonStream
{
	ZSTD_DCtx* m_dctx;
	ZSTD_outBuffer m_output = { 0, 0, 0 };
	ZSTD_inBuffer m_input = { 0, 0, 0 };

public:
	virtual int set_input_buff(void* p, size_t sz) override
	{
		m_input.src = p;
		m_input.pos = 0;
		m_input.size = sz;
		return 0;
	};
	virtual int set_output_buff(void* p, size_t sz) override
	{
		m_output.dst = p;
		m_output.pos = 0;
		m_output.size = sz;
		return 0;
	};
	virtual size_t get_input_pos() override
	{
		return m_input.pos;
	};
	virtual size_t get_output_pos() override
	{
		return m_output.pos;
	};

	virtual int decompress() override
	{
		return ZSTD_decompressStream(m_dctx, &m_output, &m_input);
	};

	virtual size_t get_default_input_size() override
	{
		return ZSTD_DStreamInSize();
	}

	size_t decompress_size(const string& backfile) override
	{
		shared_ptr<FileBuffer> inp = get_file_buffer(backfile, true);
		if (inp == nullptr)
		{
			return 0;
		}
		size_t sz = ZSTD_DStreamInSize();
		shared_ptr<DataBuffer> pb = inp->request_data(0, sz);
		if (!pb)
			return 0;
		size_t decompressed_sz = ZSTD_getFrameContentSize(pb->data(), sz);

		return decompressed_sz;
	}
	ZstdStream()
	{
		m_dctx = ZSTD_createDCtx();
	};
	virtual ~ZstdStream()
	{
		ZSTD_freeDCtx(m_dctx);
	}
};

static class FSzstd : public FSCompressStream
{
public:
	FSzstd() { m_ext = ".ZST"; };
	virtual std::shared_ptr<CommonStream> create_stream() { return make_shared<ZstdStream>(); };
}g_FSzstd;

static class FS_DATA
{
public:
	vector<FSBasic *> m_pFs;
	FS_DATA()
	{
		m_pFs.push_back(&g_fsflat);
		m_pFs.push_back(&g_fszip);
		m_pFs.push_back(&g_fstar);
		m_pFs.push_back(&g_fsbz2);
		m_pFs.push_back(&g_fsfat);
		m_pFs.push_back(&g_fsgz);
		m_pFs.push_back(&g_FSzstd);
		m_pFs.push_back(&g_fshttps);
		m_pFs.push_back(&g_fshttp);
	}

	int get_file_timesample(const string &filename, uint64_t *ptimesample)
	{
		if (ptimesample == nullptr)
		{
			set_last_err_string("ptimesample is null\n");
			return -1;
		}

		for (size_t i = 0; i < m_pFs.size(); i++)
		{
			if (!m_pFs[i]->get_file_timesample(filename, ptimesample))
				return 0;
		}

		return -1;
	}

	int for_each_ls(uuu_ls_file fn, string path, void *p)
	{
		for (int i = m_pFs.size() -1; i >= 0; i--)
                {
                        string back, filename;
                        if (m_pFs[i]->split(path, &back, &filename, true) == 0)
                                if(m_pFs[i]->for_each_ls(fn, back, filename, p)==0)
				{
					return 0;
				}
        }
		return 0;
	}

	bool exist(const string &filename)
	{
		for (size_t i = 0; i < m_pFs.size(); i++)
		{
			string back, fn;
			if (m_pFs[i]->split(filename, &back, &fn) == 0)
				if (m_pFs[i]->exist(back, fn))
					return true;
		}
		return false;
	}

	bool need_small_mem(const string& filename)
	{
		for (size_t i = 0; i < m_pFs.size(); i++)
		{
			string back, fn;
			if (m_pFs[i]->split(filename, &back, &fn) == 0)
				if (m_pFs[i]->exist(back, fn))
					return m_pFs[i]->m_small_pool;
		}
		return false;
	}
	int load(const string &filename, shared_ptr<FileBuffer> p)
	{
		for (size_t i = 0; i < m_pFs.size(); i++)
		{
			string back, fn;
			if (m_pFs[i]->split(filename, &back, &fn) == 0) {
				if (m_pFs[i]->load(back, fn, p) == 0)
					return 0;
			}
		}

		string err;
		err = "fail open file: ";
		err += filename;
		set_last_err_string(err);
		return -1;
	}
}g_fs_data;

int FSBackFile::get_file_timesample(const string &filename, uint64_t *ptime)
{
	string back, file;
	if (split(filename, &back, &file))
		return -1;
	
	return g_fs_data.get_file_timesample(back, ptime);
}

bool FSZip::exist(const string &backfile, const string &filename)
{
	Zip zip;
	if (zip.Open(backfile))
		return false;

	return zip.check_file_exist(filename);
}

int FSZip::for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p)
{
	Zip zip;

        if (zip.Open(backfile))
                return -1;

	for(auto it = zip.m_filemap.begin(); it!=zip.m_filemap.end(); ++it)
	{
		if(it->first.substr(0, filename.size()) == filename || filename.empty())
		{
			string name = backfile;
			name += "/";
			name += it->first;
			fn(name.c_str()+1, p);
		}
	}

	return 0;
}

int zip_async_load(string zipfile, string fn, shared_ptr<FileBuffer> buff)
{
	std::lock_guard<mutex> lock(buff->m_async_mutex);

	Zip zip;
	if (zip.Open(zipfile))
		return -1;

	if(zip.get_file_buff(fn, buff))
		return -1;

	buff->m_available_size = buff->m_DataSize;
	atomic_fetch_or(&buff->m_dataflags, FILEBUFFER_FLAG_LOADED);

	buff->m_request_cv.notify_all();
	return 0;
}

int FSZip::load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p)
{
	Zip zip;

	if (zip.Open(backfile))
		return -1;

	if (!zip.check_file_exist(filename))
		return -1;

	if(zip.get_file_buff(filename, p))
		return -1;

	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	p->m_request_cv.notify_all();

	return 0;
}

bool FSTar::exist(const string &backfile, const string &filename)
{
	Tar tar;
	if (tar.Open(backfile))
		return false;

	return tar.check_file_exist(filename);
}


int FSTar::for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p)
{
	Tar tar;

        if (tar.Open(backfile))
                return -1;

	for(auto it = tar.m_filemap.begin(); it!=tar.m_filemap.end(); ++it)
	{
		if(it->first.substr(0, filename.size()) == filename || filename.empty())
		{
			string name = backfile;
			name += "/";
			name += it->first;
			fn(name.c_str()+1, p);
		}
	}

	return 0;
}

int FSTar::load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p)
{
	Tar tar;
	if (tar.Open(backfile))
		return -1;

	if (!tar.check_file_exist(filename))
		return -1;

	if(tar.get_file_buff(filename, p))
		return -1;
	p->m_available_size = p->m_DataSize;
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	p->m_request_cv.notify_all();
	return 0;
}

bool FSFat::exist(const string &backfile, const string &filename)
{
	Fat fat;
	if (fat.Open(backfile))
	{
		return false;
	}
	return fat.m_filemap.find(filename) != fat.m_filemap.end();
}

int FSFat::load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p)
{
	Fat fat;
	if (fat.Open(backfile))
	{
		return -1;
	}
	
	if(fat.get_file_buff(filename, p))
		return -1;

	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	p->m_request_cv.notify_all();

	return 0;
}

int FSFat::for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p)
{
	Fat fat;
        if (fat.Open(backfile))
        {
                return -1;
        }

	for(auto it = fat.m_filemap.begin(); it != fat.m_filemap.end(); ++it)
	{
		if(it->first.substr(0, filename.size()) == filename || filename.empty())
		{
			string name = backfile;
			name += "/";
			name += it->first;
			fn(name.c_str()+1, p);
		}
	}
	return 0;
}

bool FSCompressStream::exist(const string &backfile, const string &filename)
{

	if (!g_fs_data.exist(backfile))
		return false;

	if (filename == "*")
		return true;

	return false;
}

int FSCompressStream::for_each_ls(uuu_ls_file fn, const string &backfile, const string &/*filename*/, void *p)
{

	if(!g_fs_data.exist(backfile))
		return -1;

	string str;
	str = backfile + "/*";

	fn(str.c_str() + 1, p);
	return 0;
}

class Bz2FragmentBlock: public FragmentBlock
{
public:
	virtual ~Bz2FragmentBlock() {}
	int DataConvert() override
	{
		std::lock_guard<mutex> lock(m_mutex);

		m_actual_size = m_output_size;
		m_data.resize(m_output_size);

		shared_ptr<DataBuffer> input = m_input->request_data(m_input_offset, m_input_sz);
		if (!input)
			return -1;
		unsigned int len = m_output_size;
		m_ret = BZ2_bzBuffToBuffDecompress((char*)m_data.data(),
			&len,
			(char*)input->data(),
			m_input_sz,
			0,
			0);

		m_actual_size = len;
		m_data.resize(m_actual_size);

		assert(m_output_size >= m_actual_size);

		atomic_fetch_or(&m_dataflags, (int)CONVERT_DONE);
		return m_ret;
	}
};

shared_ptr<FragmentBlock> FSBz2::ScanCompressblock(const string& backfile, size_t& input_offset, size_t& output_offset)
{
	shared_ptr<FileBuffer> pbz;

	pbz = get_file_buffer(backfile, true);
	if (pbz == nullptr) {
		return NULL;
	}

	size_t request_size = 1 * 1000 * 1000;
	shared_ptr<DataBuffer> pd = pbz->request_data(input_offset, request_size);
	if (!pd)
		return NULL;

	uint8_t* p1 = pd->data();

	size_t sz = min(request_size - 10, pd->size());

	for (size_t i = 0; i < sz; i++)
	{
		uint16_t* header = (uint16_t*)p1++;
		if (*header == 0x5a42) //"BZ"
		{
			uint32_t* magic1 = (uint32_t*)&pd->at(i + 4);
			if (*magic1 == 0x26594131 && pd->at(i + 2) == 'h') //PI 3.1415926
			{
				uint16_t* magic2 = (uint16_t*)&pd->at(i + 8);
				if (*magic2 == 0x5953)
				{
					shared_ptr<FragmentBlock> p = shared_ptr<FragmentBlock>(new Bz2FragmentBlock);

					p->m_input = pbz;
					p->m_actual_size = 0;
					p->m_dataflags = 0;
					p->m_input_offset = input_offset + i;
					p->m_output_offset = output_offset;
					p->m_output_size = (pd->at(i + 3) - '0') * 100 * 1000; /* not l024 for bz2 */
					p->m_input_sz = request_size;

					input_offset += i + 8;

					output_offset += p->m_output_size;
					return p;
				}
			}
		}
	}

	return NULL;
}

bool FSBz2::seekable(const string& backfile)
{
	shared_ptr<FileBuffer> file = get_file_buffer(backfile, true);
	shared_ptr<DataBuffer> p = file->request_data(0, 1024*1024);

	if (!p)
		return false;

	int header_num = 0;
	uint8_t* ptr = p->data();

	for (size_t i = 0; i < p->size(); i++)
	{
		if (ptr[0] == 'B' && ptr[1] == 'Z' && ptr[2] == 'h' && ptr[4] == '1' && ptr[5] == 'A' && ptr[6] == 'Y' && ptr[7] == '&' && ptr[8] == 'S' && ptr[9] == 'Y')
		{
			header_num++;
		}
		ptr++;

		if (header_num > 1)
			return true;
	}

	return false;
}

int FSCompressStream::Decompress(const string& backfile, shared_ptr<FileBuffer>outp)
{
	shared_ptr<CommonStream> cs = create_stream();
	if (!cs)
		return -1;

	ssize_t lastRet = 0;
	size_t outOffset = 0;
	shared_ptr<FileBuffer> inp = get_file_buffer(backfile, true);
	if (inp == nullptr)
	{
		return -1;
	}

	size_t sz = cs->decompress_size(backfile);
	if (sz)
	{
		outp->resize(sz);
		atomic_fetch_or(&outp->m_dataflags, FILEBUFFER_FLAG_KNOWN_SIZE);
	}

	std::shared_ptr<DataBuffer> buff;
	buff = inp->request_data(0, 0x1000);
	if (!buff)
		return -1;

	size_t offset = 0;
	uuu_notify ut;
	ut.type = uuu_notify::NOTIFY_DECOMPRESS_START;
	ut.str = (char*)backfile.c_str();
	call_notify(ut);

	shared_ptr<FragmentBlock> blk;
	blk = outp->get_map_it(0, true);
	if (!blk)
		blk = outp->request_new_blk();

	{
		lock_guard<mutex> l(outp->m_seg_map_mutex);
		outp->m_last_db = blk;
	}

	cs->set_output_buff(blk->data(), blk->m_output_size);

	while ((buff = inp->request_data(offset, cs->get_default_input_size())))
	{
		if (!buff)
			return -1;

		//ZSTD_inBuffer input = { buff->data(), buff->size(), 0 };
		cs->set_input_buff(buff->data(), buff->size());
		/* Given a valid frame, zstd won't consume the last byte of the frame
		 * until it has flushed all of the decompressed data of the frame.
		 * Therefore, instead of checking if the return code is 0, we can
		 * decompress just check if input.pos < input.size.
		 */
		while (cs->get_input_pos() < min(buff->size(), inp->m_DataSize - offset))
		{
			/* The return code is zero if the frame is complete, but there may
			 * be multiple frames concatenated together. Zstd will automatically
			 * reset the context when a frame is complete. Still, calling
			 * ZSTD_DCtx_reset() can be useful to reset the context to a clean
			 * state, for instance if the last decompression call returned an
			 * error.
			 */
			size_t old = cs->get_output_pos();
			ssize_t const ret = cs->decompress();  //ZSTD_decompressStream(dctx, &output, &input);
			lastRet = min(lastRet, ret);
			outOffset += cs->get_output_pos() - old;

			if (ret < 0) {
				blk->m_ret = ret;
				set_last_err_string("decompress error");
				outp->m_request_cv.notify_all();
				return -1;
			}

			blk->m_ret = 0;
			blk->m_actual_size = cs->get_output_pos();

			atomic_fetch_or(&blk->m_dataflags, (int)FragmentBlock::CONVERT_PARTIAL);

			ut.type = uuu_notify::NOTIFY_DECOMPRESS_POS;
			ut.index = outOffset;
			call_notify(ut);
			outp->m_available_size = outOffset;
			outp->m_request_cv.notify_all();


			if (cs->get_output_pos() == blk->m_output_size)
			{
				atomic_fetch_or(&blk->m_dataflags, (int)FragmentBlock::CONVERT_DONE);
				if (!(cs->get_input_pos() == buff->size() &&
					buff->size() == (inp->size() - offset)))
				{
					blk = outp->get_map_it(outOffset, true);
					if(!blk)
						blk = outp->request_new_blk();
					cs->set_output_buff(blk->data(), blk->m_output_size);

					{
						lock_guard<mutex> l(outp->m_seg_map_mutex);
						outp->m_last_db = blk;
					}
				}
			}

			if (outp->m_reset_stream)
			{
				outp->m_reset_stream = false;
				return -1;
			}
		}

		offset += cs->get_default_input_size();
	}
	outp->resize(outOffset);

	atomic_fetch_or(&blk->m_dataflags, (int)FragmentBlock::CONVERT_DONE);
	atomic_fetch_or(&outp->m_dataflags, FILEBUFFER_FLAG_LOADED);

	outp->m_request_cv.notify_all();
	if (lastRet < 0)
		return -1;
	return 0;
}

uint64_t get_file_timesample(string filename)
{
	uint64_t time=0;
	g_fs_data.get_file_timesample(filename, &time);
	return time;
}

shared_ptr<FileBuffer> get_file_buffer(string filename, bool async)
{
	filename = remove_quota(filename);

	if (!filename.empty() && filename[0] != MAGIC_PATH)
	{
		if (filename == "..")
			filename = g_current_dir.substr(0, g_current_dir.size() - 1);
		else
			filename = g_current_dir + filename;
	}

	string_ex path;
	path += filename;

	path.replace('\\', '/');

	filename = path;

	bool find;
	{
		std::lock_guard<mutex> lock(g_mutex_map);
		find = (g_filebuffer_map.find(filename) == g_filebuffer_map.end());
	}

	if (find)
	{
		shared_ptr<FileBuffer> p(new FileBuffer);

		if (p->reload(filename, async))
			return nullptr;

		{
			std::lock_guard<mutex> lock(g_mutex_map);
			g_filebuffer_map[filename] = p;
		}
		return p;
	}
	else
	{
		shared_ptr<FileBuffer> p;
		{
			std::lock_guard<mutex> lock(g_mutex_map);
			p= g_filebuffer_map[filename];
		}
		if (p->m_timesample != get_file_timesample(filename))
			if (p->reload(filename, async))
			{
				return nullptr;
			}

		if (!p->IsLoaded() && !async)
		{
			std::lock_guard<mutex> lock(p->m_async_mutex);

			if(p->m_async_thread.joinable())
				p->m_async_thread.join();

			if(!p->IsLoaded())
			{
				return nullptr;
			}
		}

		return p;
	}
}

FileBuffer::FileBuffer()
{
	m_pDatabuffer = nullptr;
	m_DataSize = 0;
	m_MemSize = 0;
	m_dataflags = 0;
	m_available_size = 0;
}

FileBuffer::FileBuffer(void *p, size_t sz)
{
	m_pDatabuffer = nullptr;
	m_DataSize = 0;
	m_MemSize = 0;

	m_pDatabuffer = (uint8_t*)malloc(sz);
	m_MemSize = m_DataSize = sz;

	memcpy(m_pDatabuffer, p, sz);
	m_dataflags = 0;

	atomic_fetch_or(&m_dataflags, FILEBUFFER_FLAG_LOADED);
}

FileBuffer::~FileBuffer()
{
	m_reset_stream = true;

	if(m_async_thread.joinable())
		m_async_thread.join();

	if (m_pDatabuffer)
	{
		if(m_allocate_way == ALLOCATION_WAYS::MMAP)
			unmapfile();
		if(m_allocate_way == ALLOCATION_WAYS::MALLOC)
			free(m_pDatabuffer);
	}
}

int FileBuffer::mapfile(const string &filename, size_t sz)
{
#ifdef _MSC_VER

		m_Request.StructureVersion = REQUEST_OPLOCK_CURRENT_VERSION;
		m_Request.StructureLength = sizeof(REQUEST_OPLOCK_INPUT_BUFFER);
		m_Request.RequestedOplockLevel = (OPLOCK_LEVEL_CACHE_READ | OPLOCK_LEVEL_CACHE_HANDLE);
		m_Request.Flags = REQUEST_OPLOCK_INPUT_FLAG_REQUEST;

		REQUEST_OPLOCK_OUTPUT_BUFFER Response;

		m_OverLapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		ResetEvent(m_OverLapped.hEvent);

		m_file_handle = CreateFile(filename.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED,
			nullptr);

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
			nullptr,
			&m_OverLapped);

		if (bSuccess || GetLastError() == ERROR_IO_PENDING)
		{
			m_file_monitor = thread(file_overwrite_monitor, filename, this);
		}

		m_file_map = CreateFileMapping(m_file_handle,
			nullptr, PAGE_READONLY, 0, 0, nullptr);

		if (m_file_map == INVALID_HANDLE_VALUE)
		{
			set_last_err_string("Fail create Map");
			return -1;
		}

		m_pDatabuffer = (uint8_t *)MapViewOfFile(m_file_map, FILE_MAP_READ, 0, 0, sz);
		m_DataSize = sz;
		m_MemSize = sz;
		m_allocate_way = ALLOCATION_WAYS::MMAP;

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
			m_pDatabuffer = nullptr;
			set_last_err_string("mmap failure\n");
			return -1;
		}
		m_DataSize = sz;
		m_MemSize = sz;
		m_allocate_way = ALLOCATION_WAYS::MMAP;
		close(fd);
#endif

		if (m_pDatabuffer)
			return 0;

		set_last_err_string("mmap file failure");
		return -1;
}

int FileBuffer::ref_other_buffer(shared_ptr<FileBuffer> p, size_t offset, size_t size)
{
	m_pDatabuffer = p->data() + offset;
	m_DataSize = m_MemSize = size;
	m_available_size = m_DataSize;
	m_allocate_way = ALLOCATION_WAYS::REF;
	m_ref = p;

	atomic_fetch_or(&m_dataflags, FILEBUFFER_FLAG_LOADED);
	return 0;
}

int FileBuffer::reload(string filename, bool async)
{
	if(async) {
		if(!g_fs_data.exist(filename))
			return - 1;

		if (g_fs_data.need_small_mem(filename))
			m_allocate_way = ALLOCATION_WAYS::SEGMENT;

		if(m_async_thread.joinable())
                        m_async_thread.join();

		m_dataflags = 0;
		m_async_thread = thread(&FS_DATA::load, &g_fs_data, filename, shared_from_this());
	}
	else
	{
		if(g_fs_data.load(filename, shared_from_this()))
			return - 1;
	}
	m_timesample = get_file_timesample(filename);
	m_filename = filename;

	return 0;
}

int FileBuffer::request_data(std::vector<uint8_t> &data, size_t offset, size_t sz)
{
	int64_t ret;
	ret = request_data(data.data(), offset, sz);
	if (ret < 0)
	{
		data.clear();
		return -1;
	}

	data.resize(ret);
	return 0;
}

void FileBuffer::truncate_old_data_in_pool()
{
	if (!g_small_memory)
		return;


	std::unique_lock<std::mutex> lock(this->m_seg_map_mutex);

	if (m_last_request_offset < m_total_buffer_size/2)
		return;

	size_t off = m_last_request_offset - m_total_buffer_size/2;

	for (auto it= m_seg_map.lower_bound(off); it != m_seg_map.end(); it++)
	{
		auto blk = it->second;
		std::unique_lock<std::mutex> lock(blk->m_mutex);

		if ((blk->m_dataflags & FragmentBlock::CONVERT_DONE)
			/* && !(blk->m_dataflags & FragmentBlock::USING)*/
			)
		{
			blk->m_dataflags = 0;
			blk->m_actual_size = 0;
			vector<uint8_t> v;
			blk->m_data.swap(v);
		}
	}
}

int64_t FileBuffer::request_data_from_segment(void *data, size_t offset, size_t sz)
{
	size_t return_sz = 0;

	do
	{
		m_last_request_offset = offset;
		std::unique_lock<std::mutex> lck(m_request_cv_mutex);

		shared_ptr<FragmentBlock> blk;

		m_pool_load_cv.notify_all();

		while (!(blk = get_map_it(offset)))
		{
			if (IsKnownSize())
			{
				if(offset >= this->m_DataSize)
					return -1;
			}
			auto now = std::chrono::system_clock::now();
			m_request_cv.wait_until(lck, now + 500ms);
		}
		do
		{
			shared_ptr<FragmentBlock> last_decompress_db;
			{
				std::unique_lock<std::mutex> lock(m_seg_map_mutex);
				last_decompress_db = m_last_db;
			}

			{   /*lock hold*/
				std::unique_lock<std::mutex> lock(blk->m_mutex);

				if (blk->m_actual_size >= (offset + sz - blk->m_output_offset))
					break;

				if (!(m_dataflags & FILEBUFFER_FLAG_PARTIAL_RELOADABLE))
				{
					if (last_decompress_db)
					{
						if (offset < last_decompress_db->m_output_offset && !(blk->m_dataflags & FragmentBlock::CONVERT_DONE))
						{
							m_reset_stream = true;
							break;
						}
					}
				}

				if (blk->m_ret)
					return -1;

				if ((blk->m_dataflags & FragmentBlock::CONVERT_DONE))
				{
					atomic_fetch_or(&blk->m_dataflags, (int)FragmentBlock::USING);
					break;
				}
			}
			auto now = std::chrono::system_clock::now();
			m_request_cv.wait_until(lck, now + 500ms);
		} while (1);

		if (m_reset_stream)
		{
			m_dataflags = 0;
			m_available_size = 0;
			this->m_async_thread.join();
			m_reset_stream = false;

			this->reload(m_filename, true);
			continue;
		}

		{   /*hold lock*/
			std::unique_lock<std::mutex> lock(blk->m_mutex);

			size_t off = offset - blk->m_output_offset;

			assert(offset >= blk->m_output_offset);

			size_t item_sz = blk->m_actual_size - off;

			if (off > blk->m_actual_size)
				return -1;

			if (item_sz >= sz)
			{
				memcpy(data, blk->data() + off, sz);
				atomic_fetch_and(&blk->m_dataflags, ~FragmentBlock::USING);

				return_sz += sz;
				return return_sz;
			}
			else if (item_sz == 0)
			{
				return return_sz;
			}
			else
			{
				memcpy(data, blk->m_data.data() + off, item_sz);
				data = ((uint8_t*)data) + item_sz;
				sz -= item_sz;
				offset += item_sz;

				return_sz += item_sz;
			}
		}
	} while (1);

	return -1;
}

int64_t FileBuffer::request_data(void *data, size_t offset, size_t sz)
{
	bool needlock = false;
	int ret = 0;

	if (IsLoaded())
	{
		if (offset >= this->size())
		{
			set_last_err_string("request offset exceed memory size");
			return -1;
		}

		if (this->m_allocate_way == FileBuffer::ALLOCATION_WAYS::SEGMENT)
			return request_data_from_segment(data, offset, sz);

	}
	else
	{
		if (this->m_allocate_way == FileBuffer::ALLOCATION_WAYS::SEGMENT)
			return request_data_from_segment(data, offset, sz);

		std::unique_lock<std::mutex> lck(m_request_cv_mutex);
		while ((offset + sz > m_available_size) && !IsLoaded())
		{
			if (IsError())
			{
				set_last_err_string("Async request data error");
				return -1;
			}
			m_request_cv.wait(lck);
		}

		if (IsLoaded())
		{
			if (offset > m_available_size)
			{
				set_last_err_string("request offset execeed memory size");
				return -1;
			}
		}
		needlock = true;
	}

	size_t size = sz;
	if (offset + size >= m_available_size)
		size = m_available_size - offset;

	if (needlock) m_data_mutex.lock();

	if (this->data())
	{
		memcpy(data, this->data() + offset, size);
		ret = size;
	}
	else
	{
		set_last_err_string("Out of memory");
		ret = ERR_OUT_MEMORY;
	}
	if (needlock) m_data_mutex.unlock();

	return ret;
}

std::shared_ptr<FragmentBlock> FileBuffer::request_new_blk()
{
	if (m_allocate_way == ALLOCATION_WAYS::SEGMENT)
	{
		if (m_seg_map.empty())
		{
			std::shared_ptr<FragmentBlock> p(new FragmentBlock);
			lock_guard<mutex> lock(m_seg_map_mutex);
			p->m_output_size = m_seg_blk_size;
			p->m_data.resize(m_seg_blk_size);
			m_seg_map[0] = p;
			return p;
		}
		
		size_t offset;

		if (g_small_memory)
		{
			truncate_old_data_in_pool();

			{
				lock_guard<mutex> lock(m_seg_map_mutex);
				shared_ptr <FragmentBlock> p = m_seg_map.begin()->second;
				offset = p->m_output_offset;
			}

			while (offset > m_last_request_offset + m_total_buffer_size)
			{
				if (m_reset_stream)
					return NULL;
				std::unique_lock<std::mutex> lck(m_pool_load_cv_mutex);
				m_pool_load_cv.wait(lck);
			}
		}

		{
			lock_guard<mutex> lock(m_seg_map_mutex);
			shared_ptr <FragmentBlock> p = m_seg_map.begin()->second;
			offset = p->m_output_offset;
		}

		offset += m_seg_blk_size;

		std::shared_ptr<FragmentBlock> p(new FragmentBlock);
		p->m_output_size = m_seg_blk_size;
		p->m_output_offset = offset;
		p->m_data.resize(m_seg_blk_size);

		{
			lock_guard<mutex> lock(m_seg_map_mutex);
			m_seg_map[offset] = p;
		}

		return p;
	}
	else
	{
		std::shared_ptr<FragmentBlock> p(new FragmentBlock);
		p->m_pData = this->m_pDatabuffer;
		p->m_output_size = this->m_MemSize;
		return p;
	}
}

std::shared_ptr<DataBuffer> FileBuffer::request_data(size_t offset, size_t sz)
{
	shared_ptr<DataBuffer> p(new DataBuffer);

	if (IsLoaded() && IsRefable())
	{
		if (offset >= this->size())
		{
			set_last_err_string("request offset bigger than file size");
			return nullptr;
		}

		size_t size = sz;
		if (offset + sz > this->size())
			size = this->size() - offset;

		if (!p->ref_other_buffer(shared_from_this(), offset, size))
			return p;
	}

	if (sz == SIZE_MAX)
		sz = size() - offset;

	p->resize(sz);
	int64_t ret = request_data(p->m_pDatabuffer, offset, sz);
	if (ret < 0)
		return nullptr;

	p->resize(ret);
	return p;
}

int FileBuffer::reserve(size_t sz)
{
	assert(m_allocate_way == ALLOCATION_WAYS::MALLOC);

	if (sz > m_MemSize)
	{
		m_pDatabuffer = (uint8_t*)realloc(m_pDatabuffer, sz);
		m_MemSize = sz;

		if (m_pDatabuffer == nullptr)
		{
			set_last_err_string("Out of memory\n");
			return -1;
		}
	}

	return 0;
}

int FileBuffer::resize(size_t sz)
{
	if (this->m_allocate_way == ALLOCATION_WAYS::SEGMENT)
	{
		m_DataSize = sz;
		return 0;
	}

	if (this->m_allocate_way == ALLOCATION_WAYS::REF)
	{
		if (sz > m_DataSize)
		{
			assert(true);
			return 0;
		}
		m_DataSize = sz;
		return m_DataSize;
	}
	int ret = reserve(sz);

	m_DataSize = sz;
	return ret;
}

int FileBuffer::swap(FileBuffer &a)
{
	std::swap(m_pDatabuffer, a.m_pDatabuffer);
	std::swap(m_DataSize, a.m_DataSize);
	std::swap(m_MemSize, a.m_MemSize);
	std::swap(m_allocate_way, a.m_allocate_way);

	return 0;
}

int FileBuffer::unmapfile()
{
	if (m_pDatabuffer)
	{
#ifdef _MSC_VER
		UnmapViewOfFile(m_pDatabuffer);
		m_pDatabuffer = nullptr;
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
		m_pDatabuffer = nullptr;
	}
	return 0;
}

bool check_file_exist(string filename, bool /*start_async_load*/)
{
	string_ex fn;
	fn += remove_quota(filename);
	string_ex path;
	if (!fn.empty() && fn[0] != MAGIC_PATH)
	{
		if (fn == "..")
			path += g_current_dir.substr(0, g_current_dir.size() - 1);
		else
			path += g_current_dir + fn;
	}
	else {
		path = fn;
	}

	path.replace('\\', '/');

	if (path.empty())
		path += "./";
	return g_fs_data.exist(path);
}

#ifdef WIN32

int file_overwrite_monitor(string filename, FileBuffer *p)
{
	WaitForSingleObject(p->m_OverLapped.hEvent, INFINITE);

	string str;
	str = ">";
	str += filename;

	if(p->m_pDatabuffer && p->get_m_allocate_way() == FileBuffer::ALLOCATION_WAYS::MMAP)
	{
		std::lock_guard<mutex> lock(g_mutex_map);
		p->m_file_monitor.detach(); /*Detach itself, erase will delete p*/
		if(g_filebuffer_map.find(str) != g_filebuffer_map.end())
			g_filebuffer_map.erase(str);
	}

	return 0;
}
#endif

int uuu_for_each_ls_file(uuu_ls_file fn, const char *file_path, void *p)
{
	string_ex path;
	path +=">";

	string f = file_path;

	if(f.size() == 0)
	{
		path += "./";
	}else if( f[0] == '/')
	{
		path += "//";
	}else
	{
		path += "./";
	}

	path+=file_path;
	path.replace('\\', '/');

	f = path;
	return g_fs_data.for_each_ls(fn, f, p);
}

int FSCompressStream::load(const string& backfile, const string& filename, shared_ptr<FileBuffer>outp)
{
	if (!g_fs_data.exist(backfile))
	{
		string str;
		str = "Failure open file:";
		str += backfile;
		set_last_err_string(str);
		return -1;
	}
	if (filename != "*")
	{
		string star("/*");
		string decompressed_name = backfile + star;
		shared_ptr<FileBuffer> decompressed_file = get_file_buffer(decompressed_name);
		Tar tar;
		tar.Open(decompressed_name);
		if (tar.get_file_buff(filename, outp))
			return -1;
		outp->m_available_size = outp->m_DataSize;
	}

	if (seekable(backfile))
	{
		size_t offset = 0;
		size_t decompress_off = 0;
		shared_ptr<FragmentBlock> p;
		size_t total_size = 0;

		atomic_fetch_or(&outp->m_dataflags, FILEBUFFER_FLAG_PARTIAL_RELOADABLE);

		int nthread = thread::hardware_concurrency();

		vector<thread> threads;

		for (int i = 0; i < nthread; i++)
		{
			threads.push_back(thread(&FSCompressStream::PreloadWorkThread, this, outp));
		}

		while ((p = ScanCompressblock(backfile, offset, decompress_off)))
		{
			if (!p)
				return 0;

			{
				lock_guard<mutex> lock(outp->m_seg_map_mutex);
				outp->m_seg_map[p->m_output_offset] = p;
			}

			outp->m_request_cv.notify_all();
			outp->m_pool_load_cv.notify_all();

			total_size = p->m_output_offset + p->m_output_size;

			if (outp->m_reset_stream)
				return -1;
		}

		outp->m_DataSize = total_size;

		atomic_fetch_or(&outp->m_dataflags, FILEBUFFER_FLAG_KNOWN_SIZE | FILEBUFFER_FLAG_SEG_DONE);
		outp->m_request_cv.notify_one();

		for (int i = 0; i < nthread; i++)
		{
			threads[i].join();
		}
		return 0;
	}

	return Decompress(backfile, outp);
}

int FSBasic::PreloadWorkThread(shared_ptr<FileBuffer>outp)
{
	while (!outp->m_reset_stream)
	{
		size_t request_offset = outp->m_last_request_offset;
		{
			lock_guard<mutex> lock(outp->m_seg_map_mutex);
			if (!outp->m_offset_request.empty())
			{
				request_offset = outp->m_offset_request.front();
				outp->m_offset_request.pop();
			}
		}

		shared_ptr<FragmentBlock> blk;

		{
			lock_guard<mutex> lock(outp->m_seg_map_mutex);

			auto low = outp->m_seg_map.begin();

			low = outp->m_seg_map.lower_bound(request_offset);

			int count = 0;
			while (low != outp->m_seg_map.end() )
			{
				if (!(low->second->m_dataflags & (FragmentBlock::CONVERT_START)))
					break;

				low--;
				count++;
				if (g_small_memory && count >= 5)
					break;
			}

			if (low != outp->m_seg_map.end()) {
				if (!(low->second->m_dataflags & FragmentBlock::CONVERT_START))
				{
					atomic_fetch_or(&low->second->m_dataflags, (int)FragmentBlock::CONVERT_START);
					blk = low->second;
				}
			}
		}

		if (!blk ||
			(blk && (blk->m_dataflags & FragmentBlock::CONVERT_DONE))
			)
		{
			std::unique_lock<std::mutex> lck(outp->m_pool_load_cv_mutex);
			outp->m_pool_load_cv.wait(lck);
			continue;
		}

		outp->truncate_old_data_in_pool();

		if (blk->DataConvert() < 0)
		{
				// todo error handle;
				continue;
		}

		atomic_fetch_or(&blk->m_dataflags, (int)FragmentBlock::CONVERT_DONE);
		outp->m_request_cv.notify_all();
	}
	return 0;
}

int FSHttp::load(const string& backfile, const string& filename, shared_ptr<FileBuffer> p)
{
	shared_ptr<HttpStream> http = make_shared<HttpStream>();

	if (http->HttpGetHeader(backfile, filename, m_Port, typeid(*this) == typeid(FSHttps)))
		return -1;

	size_t sz = http->HttpGetFileSize();

	p->resize(sz);

	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_KNOWN_SIZE);
	p->m_request_cv.notify_all();

	return http_load(http, p, backfile);
}

void uuu_set_small_mem(uint32_t val)
{
	g_small_memory = !!val;
}

void clean_up_filemap()
{
	for (auto it : g_filebuffer_map)
	{
		it.second->m_reset_stream = true;
		it.second->m_pool_load_cv.notify_all();
		if (it.second->m_async_thread.joinable())
			it.second->m_async_thread.join();
	}
	g_filebuffer_map.clear();
}
