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

#define MAGIC_PATH '>'

string g_current_dir = ">";

void set_current_dir(const string &dir)
{
	g_current_dir = MAGIC_PATH;
	g_current_dir += dir;
}

class FSBasic
{
public:
	virtual int get_file_timesample(const string &filename, uint64_t *ptime) = 0;
	virtual int load(const string &backfile, const string &filename, shared_ptr<FileBuffer> p) = 0;
	virtual bool exist(const string &backfile, const string &filename) = 0;
	virtual int for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p) = 0;
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

		p->m_avaible_size = st.st_size;
		
		atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
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

class FSCompressStream : public FSBackFile
{
public:
	int load(const string& backfile, const string& filename, shared_ptr<FileBuffer>outp);
	virtual int Decompress(const string& backfile, shared_ptr<FileBuffer>outp) = 0;
	bool exist(const string &backfile, const string &filename) override;
	int for_each_ls(uuu_ls_file fn, const string &backfile, const string &filename, void *p) override;
};

static class FSBz2 : public FSCompressStream
{
public:
	FSBz2() { m_ext = ".BZ2"; };
	virtual int Decompress(const string& backfile, shared_ptr<FileBuffer>outp) override;
}g_fsbz2;

static class FSGz : public FSCompressStream
{
public:
	FSGz() { m_ext = ".GZ"; };
	virtual int Decompress(const string& backfile, shared_ptr<FileBuffer>outp) override;
}g_fsgz;

static class FSzstd : public FSCompressStream
{
public:
	FSzstd() { m_ext = ".ZST"; };
	virtual int Decompress(const string& backfile, shared_ptr<FileBuffer>outp) override;
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

	int get_file_timesample(const string &filename, uint64_t *ptimesame)
	{
		if (ptimesame == nullptr)
		{
			set_last_err_string("ptimesame is null\n");
			return -1;
		}

		for (size_t i = 0; i < m_pFs.size(); i++)
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
	int load(const string &filename, shared_ptr<FileBuffer> p)
	{
		for (size_t i = 0; i < m_pFs.size(); i++)
		{
			string back, fn;
			if (m_pFs[i]->split(filename, &back, &fn) == 0)
				if(m_pFs[i]->load(back, fn, p) == 0)
					return 0;
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

	buff->m_avaible_size = buff->m_DataSize;
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
	p->m_avaible_size = p->m_DataSize;
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
	for (size_t i = 1; i < pblk->blk.size() - 1; i++)
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
			if (p->m_MemSize < one.decompress_offset + one.actual_size)
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
	if (pbz == nullptr) {
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

	int nthread = thread::hardware_concurrency();

	vector<thread> threads;

	if (p->reserve(pbz->size() * 5)) //estimate uncompressed memory size;
	{
		set_last_err_string("Out of memory");
		return -1;
	}

	uint8_t* p1 = &pbz->at(0);//buffer of pbz no longer changes after calling size()

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
			if (*magic1 == 0x26594131 && pbz->at(i + 2) == 'h') //PI 3.1415926
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
						total += one.decompress_size;
						if (total >= p->m_MemSize)
						{
							lock_guard<mutex> lock(p->m_data_mutex);
							if (p->reserve(total*1.2))
								return -1;
						}
						blks.blk.push_back(one);
					}

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
	p->m_request_cv.notify_all();

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

	for (size_t i = 1; i < blks.blk.size(); i++)
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
	p->m_request_cv.notify_all();

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
	uint64_t decompressed_size;

	uint8_t* compressed_file;
	uint64_t compressed_size;

	shared_ptr<FileBuffer> filebuffer=get_file_buffer(name);

	compressed_file=filebuffer->data();
	compressed_size=filebuffer->size();

	decompressed_size=0;

	p->reserve(7*compressed_size);//the usual compressed ratio is about 18%, so 7*18% > 100%

	bz_stream strm;
	strm.bzalloc  = nullptr;
	strm.bzfree   = nullptr;
	strm.opaque   = nullptr;

	uuu_notify ut;
	ut.type = uuu_notify::NOTIFY_DECOMPRESS_START;
	ut.str = (char*)name.c_str();
	call_notify(ut);

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
		ut.type = uuu_notify::NOTIFY_DECOMPRESS_POS;
		ut.index = decompressed_size;
		call_notify(ut);
		p->m_avaible_size = decompressed_size;
		p->m_request_cv.notify_all();
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
	p->m_request_cv.notify_all();
	return 0;
}


int FSBz2::Decompress(const string &backfile, shared_ptr<FileBuffer>p)
{
	if (is_pbzip2_file(backfile) == true)//the bz2 file can be decompressed with multithreading
		return bz_async_load(backfile, p);
	else//the bz2 file can only be decompressed using single thread
		return decompress_single_thread(backfile, p);
}

int FSGz::Decompress(const string& backfile, shared_ptr<FileBuffer>p)
{
	gzFile fp = gzopen(backfile.c_str() + 1, "r");
	if (fp == nullptr)
	{
		set_last_err_string("Open file failure");
		return -1;
	}

	shared_ptr<FileBuffer> pb = get_file_buffer(backfile);

	p->reserve(pb->size() * 8); /* guest uncompress size */

	ssize_t sz = 0x100000;
	if (sz > pb->size())
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
	p->m_request_cv.notify_all();

	return 0;
}

int FSzstd::Decompress(const string& backfile, shared_ptr<FileBuffer>outp)
{
	typedef struct ZSTD_DCtx_s ZSTD_DCtx;
	ZSTD_DCtx* const dctx = ZSTD_createDCtx();
	ssize_t lastRet = 0;
	size_t outOffset = 0;
	shared_ptr<FileBuffer> inp = get_file_buffer(backfile, true);
	if (inp == nullptr)
	{
		return -1;
	}

	shared_ptr<FileBuffer> buff;
	buff = inp->request_data(0, 0x1000);
	size_t decompress_size = ZSTD_getFrameContentSize(buff->data(), 0x1000);

	if (decompress_size < inp->size())
	{
		decompress_size = inp->size() * 16;
		if (outp->reserve(decompress_size))
			return -1;
	}
	else
	{
		atomic_fetch_or(&outp->m_dataflags, FILEBUFFER_FLAG_KNOWN_SIZE);
		if (outp->resize(decompress_size))
			return -1;

	}

	size_t offset = 0;
	uuu_notify ut;
	ut.type = uuu_notify::NOTIFY_DECOMPRESS_START;
	ut.str = (char*)backfile.c_str();
	call_notify(ut);
	size_t const toRead = ZSTD_DStreamInSize();

	while ((buff = inp->request_data(offset, toRead)))
	{
		ZSTD_inBuffer input = { buff->data(), buff->size(), 0 };
		/* Given a valid frame, zstd won't consume the last byte of the frame
		 * until it has flushed all of the decompressed data of the frame.
		 * Therefore, instead of checking if the return code is 0, we can
		 * decompress just check if input.pos < input.size.
		 */
		while (input.pos < min(input.size, inp->m_DataSize - offset))
		{
			ZSTD_outBuffer output = { outp->data() + outOffset, outp->m_MemSize - outOffset, 0 };
			/* The return code is zero if the frame is complete, but there may
			 * be multiple frames concatenated together. Zstd will automatically
			 * reset the context when a frame is complete. Still, calling
			 * ZSTD_DCtx_reset() can be useful to reset the context to a clean
			 * state, for instance if the last decompression call returned an
			 * error.
			 */
			ssize_t const ret = ZSTD_decompressStream(dctx, &output, &input);
			lastRet = min(lastRet, ret);
			outOffset += output.pos;
			ut.type = uuu_notify::NOTIFY_DECOMPRESS_POS;
			ut.index = outOffset;
			call_notify(ut);
			outp->m_avaible_size = outOffset;
			outp->m_request_cv.notify_all();
		}

		offset += toRead;
	}
	outp->resize(outOffset);
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

			if(p->m_aync_thread.joinable())
				p->m_aync_thread.join();

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
	m_avaible_size = 0;
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
	if(m_aync_thread.joinable())
		m_aync_thread.join();

	if (m_pDatabuffer)
	{
		if(m_allocate_way == ALLOCATION_WAYS::MMAP)
			unmapfile();
		if(m_allocate_way == ALLOCATION_WAYS::MALLOC)
			free(m_pDatabuffer);
	}
}

int FileBuffer::mapfile(string filename, size_t sz)
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
	m_avaible_size = m_DataSize;
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
		m_aync_thread = thread(&FS_DATA::load, &g_fs_data, filename, shared_from_this());
	}
	else
	{
		if(g_fs_data.load(filename, shared_from_this()))
			return - 1;
	}
	m_timesample = get_file_timesample(filename);
	return 0;
}


int FileBuffer::request_data(size_t sz)
{
	std::unique_lock<std::mutex> lck(m_requext_cv_mutex);

	while(!(this->m_dataflags & FILEBUFFER_FLAG_KNOWN_SIZE_BIT))
		m_request_cv.wait(lck);

	if (IsLoaded())
	{
		if (sz > this->size())
		{
			set_last_err_string("exceed data size");
			return -1;
		}
	}

	
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

int64_t FileBuffer::request_data(void *data, size_t offset, size_t sz)
{
	bool needlock = false;
	int ret = 0;

	if (IsLoaded())
	{
		if (offset >= this->size())
		{
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
				set_last_err_string("request offset execeed memory size");
				return -1;
			}
		}
		needlock = true;
	}

	size_t size = sz;
	if (offset + size >= m_avaible_size)
		size = m_avaible_size - offset;

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

std::shared_ptr<FileBuffer> FileBuffer::request_data(size_t offset, size_t sz)
{
	shared_ptr<FileBuffer> p(new FileBuffer);

	if (IsLoaded())
	{
		if (offset >= this->size())
		{
			set_last_err_string("request offset bigger than file size");
			return nullptr;
		}

		size_t size = sz;
		if (offset + sz > this->size())
			size = this->size() - offset;
		p->ref_other_buffer(shared_from_this(), offset, size);
		return p;
	}

	if (sz == UINT64_MAX)
		sz = size() - offset;

	p->reserve(sz);
	int64_t ret = request_data(p->m_pDatabuffer, offset, sz);
	if (ret < 0)
		return nullptr;

	p->resize(ret);
	p->m_avaible_size = ret;
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
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
		outp->m_avaible_size = outp->m_DataSize;
	}
	return Decompress(backfile, outp);
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
