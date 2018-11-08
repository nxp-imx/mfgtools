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

#include <map>
#include "buffer.h"
#include <sys/stat.h>
#include "liberror.h"
#include <iostream>
#include <fstream>
#include "libcomm.h"
#include "zip.h"
#include "fat.h"
#include <string.h>
#include "bzlib.h"
#include "stdio.h"

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
	FSBasic() { m_ext = NULL; }
	virtual int get_file_timesample(string filename, uint64_t *ptime)=0;
	virtual int load(string backfile, string filename, FileBuffer *p, bool async)=0;
	virtual bool exist(string backfile, string filename)=0;
	virtual int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p) = 0;
	int split(string filename, string *outbackfile, string *outfilename, bool dir=false)
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

	int load(string backfile, string filename, FileBuffer *p, bool async)
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

		p->m_loaded = true;
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

class FSBackFile : public FSBasic
{
public:
	virtual int get_file_timesample(string filename, uint64_t *ptime);

};

static class FSZip : public FSBackFile
{
public:
	FSZip() { m_ext = ".ZIP"; };
	virtual int load(string backfile, string filename, FileBuffer *p, bool async);
	virtual bool exist(string backfile, string filename);
	int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p);
}g_fszip;

static class FSFat : public FSBackFile
{
public:
	FSFat() { m_ext = ".SDCARD"; };
	virtual int load(string backfile, string filename, FileBuffer *p, bool async);
	virtual bool exist(string backfile, string filename);
	int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p);
}g_fsfat;

static class FSBz2 : public FSBackFile
{
public:
	FSBz2() { m_ext = ".BZ2"; };
	virtual int load(string backfile, string filename, FileBuffer *p, bool async);
	virtual bool exist(string backfile, string filename);
	int for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p);
}g_fsbz2;

static class FS_DATA
{
public:
	vector<FSBasic *> m_pFs;
	FS_DATA()
	{
		m_pFs.push_back(&g_fsflat);
		m_pFs.push_back(&g_fszip);
		m_pFs.push_back(&g_fsbz2);
		m_pFs.push_back(&g_fsfat);
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
	int load(string filename, FileBuffer *p, bool async)
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
	if (zip.Open(backfile.substr(1)))
		return false;

	return zip.check_file_exist(filename);
}

int FSZip::for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p)
{
	Zip zip;

        if (zip.Open(backfile.substr(1)))
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

int zip_async_load(string zipfile, string fn, FileBuffer * buff)
{
	std::lock_guard<mutex> lock(buff->m_async_mutex);

	Zip zip;
	if (zip.Open(zipfile.substr(1)))
		return -1;

	shared_ptr<FileBuffer> p = zip.get_file_buff(fn);
	if (p == NULL)
		return -1;

	buff->swap(*p);
	buff->m_loaded = true;
	return 0;
}

int FSZip::load(string backfile, string filename, FileBuffer *p, bool async)
{
	Zip zip;

	if (zip.Open(backfile.substr(1)))
		return -1;

	if (!zip.check_file_exist(filename))
		return -1;

	if (async)
	{
		p->m_aync_thread = thread(zip_async_load, backfile, filename, p);
	}
	else
	{
		shared_ptr<FileBuffer> pzip = zip.get_file_buff(filename);
		if (pzip == NULL)
			return -1;

		p->swap(*pzip);
		p->m_loaded = true;
	}
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

int FSFat::load(string backfile, string filename, FileBuffer *p, bool async)
{
	Fat fat;
	if (fat.Open(backfile))
	{
		return -1;
	}
	shared_ptr<FileBuffer> pfat = fat.get_file_buff(filename);
	if (pfat == NULL)
		return -1;

	p->swap(*pfat);
	p->m_loaded = true;
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

bool FSBz2::exist(string backfile, string filename)
{
	if (filename == "*")
		return true;

	return false;
}

int FSBz2::for_each_ls(uuu_ls_file fn, string backfile, string filename, void *p)
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

int bz2_decompress(shared_ptr<FileBuffer> pbz, FileBuffer *p, vector<bz2_blk> * pblk, size_t start, size_t skip)
{
	for (int i = start; i < pblk->size(); i += skip)
	{
		unsigned int len;
		if (i) /*skip first dummy one*/
		{
			(*pblk)[i].error = BZ2_bzBuffToBuffDecompress((char*)p->data() + pblk->at(i).decompress_offset,
				&len,
				(char*)pbz->data() + pblk->at(i).start,
				pblk->at(i).size,
				0,
				0);
			(*pblk)[i].actual_size = len;
		}
	}
	return 0;
}

int bz_async_load(string filename, FileBuffer *p)
{
	shared_ptr<FileBuffer> pbz;

	pbz = get_file_buffer(filename);
	if (pbz == NULL) {
		string err;
		err = "Failure get file buffer: ";
		err += filename;
		set_last_err_string(err);
		return -1;
	}

	vector<bz2_blk> blk;
	bz2_blk one;
	memset(&one, 0, sizeof(one));
	blk.push_back(one);
	size_t total = 0;

	uint8_t *p1 = &pbz->at(0);

	for (size_t i = 0; i < pbz->size() - 10; i++)
	{
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
					one.start = i;
		
					blk[blk.size() - 1].size = i - blk[blk.size() - 1].start;
					one.decompress_offset = blk[blk.size() - 1].decompress_offset + blk[blk.size() - 1].decompress_size;
					one.decompress_size = (pbz->at(i + 3) - '0') * 100 * 1000; /* not l024 for bz2 */

					blk.push_back(one);

					total += one.decompress_size;
				}
			}
		}
	}

	if (blk.size() == 1) {
		set_last_err_string("Can't find validate bz2 magic number");
		return -1;
	}

	blk[blk.size() - 1].size = pbz->size() - blk[blk.size() - 1].start;

	int nthread = thread::hardware_concurrency();

	vector<thread> threads;
	
	p->resize(total);

	for (int i = 0; i < nthread; i++)
	{
		threads.push_back(thread(bz2_decompress, pbz, p, &blk, i, nthread));
	}

	for (int i = 0; i < nthread; i++)
	{
		threads[i].join();
	}

	for (int i = 1; i < blk.size(); i++)
	{
		if (blk[i].error)
		{
			set_last_err_string("decompress err");
			return -1;
		}
		if ((blk[i].decompress_size != blk[i].actual_size) && (i != blk.size() - 1))
		{
			set_last_err_string("bz2: only support last block less then other block");
			return -1;
		}
	}

	size_t sz =  blk[blk.size() - 1].decompress_size - blk[blk.size() - 1].actual_size;

	p->resize(total - sz);
	p->m_loaded = true;

	return 0;
}

int FSBz2::load(string backfile, string filename, FileBuffer *p, bool async)
{
	if (filename != "*")
	{
		set_last_err_string("bz just support . decompress itself");
		return -1;
	}

	if (!g_fs_data.exist(backfile))
	{
		string str;
		str = "Failure open file:";
		str += backfile;
		set_last_err_string(str);
		return -1;
	}

	p->m_aync_thread = thread(bz_async_load, backfile, p);
	
	if (!async)
		p->m_aync_thread.join();

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

		if (!p->m_loaded && !async)
		{
			std::lock_guard<mutex> lock(p->m_async_mutex);

			if(p->m_aync_thread.joinable())
				p->m_aync_thread.join();
		}

		return p;
	}
}


int FileBuffer::reload(string filename, bool async)
{
	m_loaded = false;
	if (g_fs_data.load(filename, this, async) == 0)
	{
		m_timesample = get_file_timesample(filename);
		return 0;
	}
	return -1;
}

shared_ptr<FileBuffer> get_file_buffer(string filename)
{
	return get_file_buffer(filename, false);
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

	if(p->m_pMapbuffer)
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
