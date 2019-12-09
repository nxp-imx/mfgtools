/*
* Copyright 2018-2019 NXP.
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
#include "zip.h"
#include "fat.h"
#include "tar.h"
#include <string.h>
#include "bzlib.h"
#include "stdio.h"
#include <limits>
#include "http.h"

#ifdef _MSC_VER
#define stat64 _stat64
#else
#include "dirent.h"
#endif

static map<string, shared_ptr<FileBuffer>> g_filebuffer_map;
static mutex g_mutex_map;

#define MAGIC_PATH '>'

string g_current_dir = ">";

void set_current_dir(string dir)
{
	g_current_dir = MAGIC_PATH;
	g_current_dir += dir;
}

class FSBasic
{
public:
	const char * m_ext;
	const char * m_Prefix;
	FSBasic() { m_ext = NULL; m_Prefix = NULL; }
	virtual int get_file_timesample(string filename, uint64_t *ptime)=0;
	virtual int load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async)=0;
	virtual bool exist(string backfile, string filename)=0;
	virtual int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p) = 0;
	virtual int split(string filename, string *outbackfile, string *outfilename, bool dir=false)
	{
		string path = str_to_upper(filename);
		if (m_ext == NULL || strlen(m_ext) == 0)
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
			set_last_err_string("can't find ext name in path");
			return -1;
		}

		*outbackfile = filename.substr(0, pos + strlen(m_ext));

		if(filename.size() >= pos + strlen(m_ext) + 1)
			*outfilename = filename.substr(pos + strlen(m_ext) + 1);
		else
			outfilename->clear();
		return 0;
	}
};

static class FSFlat: public FSBasic
{
public:
	FSFlat() { m_ext = ""; }
	int get_file_timesample(string filename, uint64_t *ptime)
	{
		struct stat64 st;
		if (stat64(filename.c_str() + 1, &st))
		{
			set_last_err_string("stat64 failure");
			return -1;
		}

		*ptime = st.st_mtime;

		return 0;
	}

	bool exist(string backfile, string filename)
	{
		struct stat64 st;
		return stat64(backfile.c_str() + 1, &st) == 0 && ((st.st_mode & S_IFDIR) == 0);
	}

	int load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async)
	{
		struct stat64 st;
		if (stat64(backfile.c_str() + 1, &st))
		{
			set_last_err_string("stat64 failure");
			return -1;
		}
		p->unmapfile();

		if (p->mapfile(backfile.substr(1), st.st_size))
			return -1;

		p->m_avaible_size = st.st_size;
		
		atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);

		return 0;
	}

	int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p)
	{
		struct stat64 st;

		if(stat64(backfile.c_str() + 1, &st))
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
			while ((dp=readdir(dir)) != NULL)
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
public:
	virtual int split(string filename, string *outbackfile, string *outfilename, bool dir = false)
	{
		if (m_Prefix == NULL)
			return -1;

		if (filename.size() < strlen(m_Prefix))
			return -1;

		string path = str_to_upper(filename);
		if (path.compare(1, strlen(m_Prefix), m_Prefix) == 0)
		{
			size_t pos;
			pos = filename.find('/', 1 + strlen(m_Prefix));

			*outbackfile = filename.substr(1 + strlen(m_Prefix), pos - 1 - strlen(m_Prefix));
			*outfilename = filename.substr(pos);

			return 0;
		}

		return -1;
	}
};

static class FSHttp : public FSNetwork
{
protected:
	int m_Port;
public:
	FSHttp() { m_Prefix = "HTTP://"; m_Port = 80; }
	virtual int load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async);
	virtual bool exist(string backfile, string filename) { return true; };
	int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p) { return 0; };
	virtual int get_file_timesample(string filename, uint64_t *ptime) { return 0; };
}g_fshttp;

static class FSHttps : public FSHttp
{
public:
	FSHttps() { m_Prefix = "HTTPS://"; m_Port = 443; }
}g_fshttps;

int http_load(shared_ptr<HttpStream> http, shared_ptr<FileBuffer> p, string filename)
{
	size_t max = 0x10000;

	uuu_notify ut;
	ut.type = uuu_notify::NOTIFY_DOWNLOAD_START;
	ut.str = (char*)filename.c_str();
	call_notify(ut);

	ut.type = uuu_notify::NOTIFY_TRANS_SIZE;
	ut.total = p->size();
	call_notify(ut);

	for (size_t i = 0; i < p->size(); i += max)
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
		p->m_avaible_size = i + sz;
		p->m_request_cv.notify_all();

		ut.type = uuu_notify::NOTIFY_TRANS_POS;
		ut.total = i + sz;
		call_notify(ut);
	}

	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);

	ut.type = uuu_notify::NOTIFY_DOWNLOAD_END;
	ut.str = (char*)filename.c_str();
	call_notify(ut);
	return 0;
}

int FSHttp::load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async)
{
	shared_ptr<HttpStream> http = make_shared<HttpStream>();

	if (http->HttpGetHeader(backfile, filename, m_Port))
		return -1;

	size_t sz = http->HttpGetFileSize();

	p->resize(sz);

	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_KNOWN_SIZE);

	if (async)
	{
		p->m_aync_thread = thread(http_load, http, p, backfile + filename);
#ifdef WIN32
		SetThreadPriority(p->m_aync_thread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
	}
	else
	{
		if (http_load(http, p, backfile + filename))
			return -1;

		p->m_avaible_size = p->m_DataSize;
		atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	}

	return 0;
}

class FSBackFile : public FSBasic
{
public:
	virtual int get_file_timesample(string filename, uint64_t *ptime);

};

static class FSZip : public FSBackFile
{
public:
	FSZip() { m_ext = ".ZIP"; };
	virtual int load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async);
	virtual bool exist(string backfile, string filename);
	int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p);
}g_fszip;

static class FSTar: public FSBackFile
{
public:
	FSTar() {m_ext = ".TAR"; };
	virtual int load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async);
	virtual bool exist(string backfile, string filename);
	int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p);
}g_fstar;


static class FSFat : public FSBackFile
{
public:
	FSFat() { m_ext = ".SDCARD"; };
	virtual int load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async);
	virtual bool exist(string backfile, string filename);
	int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p);
}g_fsfat;

class FSCompressStream : public FSBackFile
{
public:
	virtual bool exist(string backfile, string filename);
	virtual int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p);
};

static class FSBz2 : public FSCompressStream
{
public:
	FSBz2() { m_ext = ".BZ2"; };
	virtual int load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async);
}g_fsbz2;

static class FSGz : public FSCompressStream
{
public:
	FSGz() { m_ext = ".GZ"; };
	virtual int load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async);
}g_fsgz;

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
		m_pFs.push_back(&g_fshttps);
		m_pFs.push_back(&g_fshttp);
	}

	int get_file_timesample(string filename, uint64_t *ptimesame)
	{
		if (ptimesame == NULL)
		{
			set_last_err_string("ptimesame is null\n");
			return -1;
		}

		for (int i = 0; i < m_pFs.size(); i++)
		{
			if (!m_pFs[i]->get_file_timesample(filename, ptimesame))
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

	bool exist(string filename)
	{
		for (int i = 0; i < m_pFs.size(); i++)
		{
			string back, fn;
			if (m_pFs[i]->split(filename, &back, &fn) == 0)
				if (m_pFs[i]->exist(back, fn))
					return true;
		}
		return false;
	}
	int load(string filename, shared_ptr<FileBuffer> p, bool async)
	{
		for (int i = 0; i < m_pFs.size(); i++)
		{
			string back, fn;
			if (m_pFs[i]->split(filename, &back, &fn) == 0)
				if(m_pFs[i]->load(back, fn, p, async) == 0)
					return 0;
		}

		string err;
		err = "fail open file: ";
		err += filename;
		set_last_err_string(err);
		return -1;
	}
}g_fs_data;

int FSBackFile::get_file_timesample(string filename, uint64_t *ptime)
{
	string back, file;
	if (split(filename, &back, &file))
		return -1;
	
	return g_fs_data.get_file_timesample(back, ptime);
}

bool FSZip::exist(string backfile, string filename)
{
	Zip zip;
	if (zip.Open(backfile))
		return false;

	return zip.check_file_exist(filename);
}

int FSZip::for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p)
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

	buff->m_avaible_size = buff->m_DataSize;
	atomic_fetch_or(&buff->m_dataflags, FILEBUFFER_FLAG_LOADED);

	buff->m_request_cv.notify_all();
	return 0;
}

int FSZip::load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async)
{
	Zip zip;

	if (zip.Open(backfile))
		return -1;

	if (!zip.check_file_exist(filename))
		return -1;

	if (async)
	{
		p->m_aync_thread = thread(zip_async_load, backfile, filename, p);
	}
	else
	{
		if(zip.get_file_buff(filename, p))
			return -1;

		atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	}
	return 0;
}

bool FSTar::exist(string backfile, string filename)
{
	Tar tar;
	if (tar.Open(backfile))
		return false;

	return tar.check_file_exist(filename);
}


int FSTar::for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p)
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

int FSTar::load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async)
{
	Tar tar;
	if (tar.Open(backfile))
		return -1;

	if (!tar.check_file_exist(filename))
		return -1;

	if(tar.get_file_buff(filename, p))
		return -1;
	p->m_avaible_size = p->m_DataSize;
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	return 0;
}

bool FSFat::exist(string backfile, string filename)
{
	Fat fat;
	if (fat.Open(backfile))
	{
		return false;
	}
	return fat.m_filemap.find(filename) != fat.m_filemap.end();
}

int FSFat::load(string backfile, string filename, shared_ptr<FileBuffer> p, bool async)
{
	Fat fat;
	if (fat.Open(backfile))
	{
		return -1;
	}
	
	if(fat.get_file_buff(filename, p))
		return -1;

	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);

	return 0;
}

int FSFat::for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p)
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

bool FSCompressStream::exist(string backfile, string filename)
{
	if (filename == "*")
		return true;

	return false;
}

int FSCompressStream::for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p)
{

	if(!g_fs_data.exist(backfile))
		return -1;

	string str;
	str = backfile + "/*";

	fn(str.c_str() + 1, p);
	return 0;
}

struct bz2_blk
{
	size_t start;
	size_t size;
	size_t decompress_offset;
	size_t decompress_size;
	size_t actual_size;
	int	error;
};

class bz2_blks
{
public:
	vector<bz2_blk> blk;
	mutex blk_mutex;

	condition_variable cv;
	mutex con_mutex;

	atomic<size_t> top;
	atomic<size_t> bottom;
	bz2_blks() { top = 0; bottom = ULLONG_MAX; }
};

int bz2_update_available(shared_ptr<FileBuffer> p, bz2_blks * pblk)
{
	lock_guard<mutex> lock(pblk->blk_mutex);
	size_t sz = 0;
	for (int i = 1; i < pblk->blk.size() - 1; i++)
	{
		if (pblk->blk[i].error)
			break;

		if (!pblk->blk[i].actual_size)
			break;

		sz += pblk->blk[i].actual_size;
	}

	p->m_avaible_size = sz;
	p->m_request_cv.notify_all();
	return 0;
}

int bz2_decompress(shared_ptr<FileBuffer> pbz, shared_ptr<FileBuffer> p, bz2_blks * pblk)
{
	bz2_blk one;
	size_t cur;
	vector<uint8_t> buff;

	while (pblk->top + 1 < pblk->bottom)
	{
		if (p->IsError())
			return -1;

		{
			std::unique_lock<std::mutex> lck(pblk->con_mutex);
			while (pblk->top + 1 >= pblk->blk.size()) {
				pblk->cv.wait(lck);
				if (p->IsError())
					return -1;
			}
		}

		{
			lock_guard<mutex> lock(pblk->blk_mutex);
			if (pblk->top < pblk->blk.size() - 1)
			{
				cur = pblk->top;
				one = pblk->blk[pblk->top];
				pblk->top++;
			}
			else
			{
				continue;
			}
		}

		unsigned int len = one.decompress_size;
		buff.resize(len);

		one.error = BZ2_bzBuffToBuffDecompress((char*)buff.data(),
			&len,
			(char*)pbz->data() + one.start,
			one.size,
			0,
			0);
		one.actual_size = len;

		{
			lock_guard<mutex> lock(pblk->blk_mutex);
			(*pblk).blk[cur] = one;
		}


		{
			lock_guard<mutex> lock(p->m_data_mutex);
			if (p->size() < one.decompress_offset + one.actual_size)
				if(p->resize(one.decompress_offset + one.actual_size))
					return -1;

			memcpy(p->data() + one.decompress_offset, buff.data(), one.actual_size);
		}

		bz2_update_available(p, pblk);

	}

	return 0;
}

int bz_async_load(string filename, shared_ptr<FileBuffer> p)
{
	shared_ptr<FileBuffer> pbz;

	pbz = get_file_buffer(filename, true);
	if (pbz == NULL) {
		string err;
		err = "Failure get file buffer: ";
		err += filename;
		set_last_err_string(err);
		return -1;
	}

	bz2_blks blks;
	bz2_blk one;
	memset(&one, 0, sizeof(one));
	blks.blk.push_back(one);
	blks.top = 1;

	size_t total = 0;

	uint8_t *p1 = &pbz->at(0);

	int nthread = thread::hardware_concurrency();

	vector<thread> threads;

	if (p->reserve(pbz->size() * 5)) //estimate uncompressed memory size;
	{
		set_last_err_string("Out of memory");
		return -1;
	}

	for (int i = 0; i < nthread; i++)
	{
		threads.push_back(thread(bz2_decompress, pbz, p, &blks));
#ifdef WIN32
		if( i!=0 )
			SetThreadPriority(threads[i].native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
	}

	size_t requested = 0;
	for (size_t i = 0; i < pbz->size() - 10; i++)
	{
		if(i >= requested)
		{
			requested = i + 0x10000;
			if (requested > pbz->size())
				requested = pbz->size();
			if (pbz->request_data(requested))
			{
				atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_ERROR_BIT);
				blks.cv.notify_all();
				for (int i = 0; i < nthread; i++)
				{
					threads[i].join();
				}
				return -1;
			}
		}

		uint16_t *header = (uint16_t *)p1++;
		if (*header == 0x5a42) //"BZ"
		{
			uint32_t *magic1 = (uint32_t *)&pbz->at(i+4);
			if (*magic1 == 0x26594131) //PI 3.1415926
			{
				uint16_t *magic2 = (uint16_t *)&pbz->at(i + 8);
				if (*magic2 == 0x5953)
				{     
					/*which is valude bz2 header*/
					struct bz2_blk one;
					memset(&one, 0, sizeof(one));

					one.start = i;
					{
						lock_guard<mutex> lock(blks.blk_mutex);

						blks.blk.back().size = i - blks.blk.back().start;
						one.decompress_offset = blks.blk.back().decompress_offset + blks.blk.back().decompress_size;
						one.decompress_size = (pbz->at(i + 3) - '0') * 100 * 1000; /* not l024 for bz2 */

						blks.blk.push_back(one);
					}
					total += one.decompress_size;

					blks.cv.notify_all();
				}
			}
		}
	}

	if (blks.blk.size() == 1) {
		set_last_err_string("Can't find validate bz2 magic number");
		return -1;
	}

	blks.blk.back().size = pbz->size() - blks.blk.back().start;

	{
		lock_guard<mutex> lock(p->m_data_mutex);
		if(p->resize(total))
			return -1;
	}

	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_KNOWN_SIZE);

	{
		lock_guard<mutex> lock(blks.blk_mutex);
		struct bz2_blk one;
		memset(&one, 0, sizeof(one));
		blks.blk.push_back(one);
		blks.bottom = blks.blk.size();
		blks.cv.notify_all();
	}

	for (int i = 0; i < nthread; i++)
	{
		threads[i].join();
	}

	for (int i = 1; i < blks.blk.size(); i++)
	{
		if (blks.blk[i].error)
		{
			set_last_err_string("decompress err");
			return -1;
		}
		if ((blks.blk[i].decompress_size != blks.blk[i].actual_size) && (i != blks.blk.size() - 2))  /*two dummy blks (one header and other end)*/
		{
			set_last_err_string("bz2: only support last block less then other block");
			return -1;
		}
	}

	bz2_update_available(p, &blks);

	if(p->resize(p->m_avaible_size))
		return -1;
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);

	return 0;
}

bool is_pbzip2_file(string filename)
{
	shared_ptr<FileBuffer> file=get_file_buffer(filename, true);
	uint64_t filesize= file->size();
	uint64_t readsize= (filesize< (1024*1024) )? filesize:(1024*1024); //read at most 1MB, because maximum block size is 900kb

	file->request_data(readsize);

	int header_num=0;
	uint8_t* ptr= file->data();
	for(size_t i =0 ; i < readsize ; i++)
	{
		if(ptr[0]=='B'&& ptr[1]=='Z'&& ptr[2]=='h' && ptr[4]=='1'&& ptr[5]=='A' && ptr[6]=='Y' && ptr[7]=='&' && ptr[8]=='S'&& ptr[9]=='Y')
		{
			header_num++;
		}
		ptr++;
	}
	if(header_num>1)
		return true;
	else
		return false;
}

int decompress_single_thread(string name,shared_ptr<FileBuffer>p)
{
	uint8_t* decompressed_file;
	uint64_t decompressed_size;

	uint8_t* compressed_file;
	uint64_t compressed_size;

	shared_ptr<FileBuffer> filebuffer=get_file_buffer(name);

	compressed_file=filebuffer->data();
	compressed_size=filebuffer->size();

	decompressed_file=p->data();
	decompressed_size=0;

	p->reserve(7*compressed_size);//the usual compressed ratio is about 18%, so 7*18% > 100%

	bz_stream strm;
	strm.bzalloc  = NULL;
    strm.bzfree   = NULL;
	strm.opaque   = NULL;

	int ret;
	ret = BZ2_bzDecompressInit (&strm,0, 0 );
	if (ret != BZ_OK)
		return -1;
	strm.next_in  = (char*)compressed_file;
	strm.avail_in = compressed_size;

	uint64_t decompress_amount=5000; //decompress 5000 byte every iteration, choose 5000 only because the pbzip2 also used 5000 in their implementation.
	while(1)
	{
		p->reserve(decompressed_size+1000*decompress_amount);//make sure the space is enough,multiple by 1000 to avoid repeated realloc
		strm.next_out=(char*)p->data()+decompressed_size;
		strm.avail_out=decompress_amount;

		ret=BZ2_bzDecompress(&strm);
		decompressed_size+=decompress_amount;

		if(ret==BZ_STREAM_END)
		{
			decompressed_size-= strm.avail_out;
			break;
		}
		else if (ret != BZ_OK)//if it is not bz_ok nor bz_stream_end, decompression failed.
			return -1;

	}
	p->resize(decompressed_size);
	BZ2_bzDecompressEnd(&strm);
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	return 0;
}


int FSBz2::load(string backfile, string filename, shared_ptr<FileBuffer>p, bool async)
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
		string star ("/*");
		string decompressed_name= backfile+ star;
		shared_ptr<FileBuffer> decompressed_file=get_file_buffer(decompressed_name);
		Tar tar;
		tar.Open(decompressed_name);
		if (tar.get_file_buff(filename, p))
			return -1;
		p->m_avaible_size = p->m_DataSize;
	}
	else
	{
		if(!check_file_exist(backfile.substr(1)))
			return -1;
		if(is_pbzip2_file(backfile.substr(1))==true)//the bz2 file can be decompressed with multithreading
			p->m_aync_thread = thread(bz_async_load, backfile, p);
		else//the bz2 file can only be decompressed using single thread
			p->m_aync_thread = thread(decompress_single_thread, backfile, p);

		if (!async) {
			p->m_aync_thread.join();
			if (! p->IsLoaded()) {
				set_last_err_string("async data load failure\n");
				return -1;
			}
		}
	}
	return 0;
}

int FSGz::load(string backfile, string filename, shared_ptr<FileBuffer>p, bool async)
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
		if (tar.get_file_buff(filename, p))
			return -1;
		p->m_avaible_size = p->m_DataSize;
	}
	else
	{
		gzFile fp = gzopen(backfile.c_str() + 1, "r");
		if (fp == NULL)
		{
			set_last_err_string("Open file failure");
			return -1;
		}

		shared_ptr<FileBuffer> pb = get_file_buffer(backfile);

		p->reserve(pb->size() * 4); /* guest uncompress size */

		size_t sz = 0x100000;
		if (sz > pb->size() * 4)
			sz = p->size();

		uuu_notify ut;
		ut.type = uuu_notify::NOTIFY_DECOMPRESS_START;
		ut.str = (char*)backfile.c_str();
		call_notify(ut);

		ut.type = uuu_notify::NOTIFY_DECOMPRESS_SIZE;
		ut.total = pb->size();
		call_notify(ut);

		size_t cur = 0;
		while (!gzeof(fp))
		{
			size_t ret = gzread(fp, p->data() + cur, sz);
			if (sz < 0)
			{
				set_last_err_string("decompress error");
				return -1;
			}
			cur += ret;
			p->reserve(cur + sz);

			ut.type = uuu_notify::NOTIFY_DECOMPRESS_POS;
			ut.index = gzoffset(fp);
			call_notify(ut);
		}

		p->resize(cur);
		p->m_avaible_size = cur;

		ut.type = uuu_notify::NOTIFY_DECOMPRESS_POS;
		ut.index = pb->size();
		call_notify(ut);

		gzclose(fp);
		atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	}
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
			return NULL;

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
				return NULL;
			}

		if (!p->IsLoaded() && !async)
		{
			std::lock_guard<mutex> lock(p->m_async_mutex);

			if(p->m_aync_thread.joinable())
				p->m_aync_thread.join();

			if(!p->IsLoaded())
			{
				return NULL;
			}
		}

		return p;
	}
}


int FileBuffer::reload(string filename, bool async)
{
	atomic_init(&this->m_dataflags, 0);

	if (g_fs_data.load(filename, shared_from_this(), async) == 0)
	{
		m_timesample = get_file_timesample(filename);
		return 0;
	}
	return -1;
}

int FileBuffer::request_data(size_t sz)
{
	assert(this->m_dataflags & FILEBUFFER_FLAG_KNOWN_SIZE_BIT);

	if (IsLoaded())
	{
		if (sz > this->size())
		{
			set_last_err_string("exceed data size");
			return -1;
		}
	}

	std::unique_lock<std::mutex> lck(m_requext_cv_mutex);
	while ((sz > m_avaible_size) && !IsLoaded())
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
		if (sz > m_avaible_size)
		{
			set_last_err_string("request offset execeed memory size");
			return -1;
		}
	}

	return 0;
}

int FileBuffer::request_data(vector<uint8_t> &data, size_t offset, size_t sz)
{
	bool needlock = false;

	if (IsLoaded())
	{
		if (offset >= this->size())
		{
			data.clear();
			set_last_err_string("request offset execeed memory size");
			return -1;
		}
	}
	else
	{
		std::unique_lock<std::mutex> lck(m_requext_cv_mutex);
		while ((offset + sz > m_avaible_size) && !IsLoaded())
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
			if (offset > m_avaible_size)
			{
				data.clear();
				set_last_err_string("request offset execeed memory size");
				return -1;
			}
		}
		needlock = true;
	}

	size_t size = sz;
	if (offset + size >= m_avaible_size)
		size = m_avaible_size - offset;

	data.resize(size);

	if (needlock) m_data_mutex.lock();

	memcpy(data.data(), this->data() + offset, size);

	if (needlock) m_data_mutex.unlock();

	return 0;
}

bool check_file_exist(string filename, bool start_async_load)
{
	return get_file_buffer(filename, true) != NULL;
}

#ifdef WIN32

int file_overwrite_monitor(string filename, FileBuffer *p)
{
	WaitForSingleObject(p->m_OverLapped.hEvent, INFINITE);

	string str;
	str = ">";
	str += filename;

	if(p->m_pDatabuffer && p->m_allocate_way == FileBuffer::ALLOCATE_MMAP)
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
