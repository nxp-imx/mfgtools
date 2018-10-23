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

#ifdef _MSC_VER
#define stat64 _stat64
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
	int split(string filename, string *outbackfile, string *outfilename)
	{
		if (m_ext == NULL || strlen(m_ext) == 0)
		{
			*outbackfile = filename;
			return 0;
		}

		string path = str_to_upper(filename);
		size_t pos = path.rfind(m_ext);
		if (pos == string::npos)
		{
			set_last_err_string("can't find ext name in path");
			return -1;
		}

		*outbackfile = filename.substr(0, pos + strlen(m_ext));
		*outfilename = filename.substr(pos + strlen(m_ext) + 1);
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
		return stat64(backfile.c_str() + 1, &st) == 0;
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
}g_fszip;

static class FSFat : public FSBackFile
{
public:
	FSFat() { m_ext = ".SDCARD"; };
	virtual int load(string backfile, string filename, FileBuffer *p, bool async);
	virtual bool exist(string backfile, string filename);
}g_fsfat;

static class FSBz2 : public FSBackFile
{
public:
	FSBz2() { m_ext = ".BZ2"; };
	virtual int load(string backfile, string filename, FileBuffer *p, bool async);
	virtual bool exist(string backfile, string filename);
}g_fsbz2;

static class FS_DATA
{
public:
	vector<FSBasic *> m_pFs;
	FS_DATA()
	{
		m_pFs.push_back(&g_fsflat);
		m_pFs.push_back(&g_fszip);
		m_pFs.push_back(&g_fsfat);
		m_pFs.push_back(&g_fsbz2);
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
	if (fat.Open(backfile.substr(1)))
	{
		return false;
	}
	return fat.m_filemap.find(filename) != fat.m_filemap.end();
}

int FSFat::load(string backfile, string filename, FileBuffer *p, bool async)
{
	Fat fat;
	if (fat.Open(backfile.substr(1)))
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

bool FSBz2::exist(string backfile, string filename)
{
	return false;
}

int FSBz2::load(string backfile, string filename, FileBuffer *p, bool async)
{
	return -1;
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
