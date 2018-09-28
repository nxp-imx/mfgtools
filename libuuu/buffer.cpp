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

uint64_t get_file_timesample(string filename)
{
	struct stat64 st;
	if (stat64(filename.c_str() + 1, &st))
	{
		string path = str_to_upper(filename);
		size_t pos = path.find(".ZIP");
		if (pos == string::npos)
			return 0;
		if (stat64(filename.substr(1, pos + 3).c_str(), &st))
		{
			string path = str_to_upper(filename);
			size_t pos = path.find(".SDCARD");
			if (pos == string::npos)
				return 0;

			stat64(filename.substr(1, pos + 7).c_str(), &st);
		}
		return st.st_mtime;
	}

	return st.st_mtime;
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

	BOOL find;
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
int zip_async_load(string zipfile, string fn, FileBuffer * buff)
{
	std::lock_guard<mutex> lock(buff->m_async_mutex);

	Zip zip;
	if (zip.Open(zipfile))
		return -1;

	shared_ptr<FileBuffer> p = zip.get_file_buff(fn);
	if (p == NULL)
		return -1;

	buff->swap(*p);
	buff->m_loaded = true;
	return 0;
}

int FileBuffer::reload(string filename, bool async)
{
	struct stat64 st;
	size_t pos_zip = string::npos;
	size_t pos_sdcard = string::npos;

	if (stat64(filename.c_str() + 1, &st))
	{
		string path = str_to_upper(filename);
		pos_zip = path.find(".ZIP");
		string zipfile = filename.substr(0, pos_zip + 4);
		if (pos_zip == string::npos || (stat64(zipfile.c_str() + 1, &st)))
		{
			pos_sdcard = path.find(".SDCARD");
			string sdcardfile = filename.substr(0, pos_sdcard + strlen(".SDCARD"));
			if (pos_sdcard == string::npos || (stat64(sdcardfile.c_str() + 1, &st)))
			{
				string err = "Fail Open File: ";
				err.append(filename);
				set_last_err_string(err);
				return -1;
			}
		}
	}

	if (pos_zip != string::npos)
	{
		Zip zip;
		string zipfile = filename.substr(1, pos_zip + 3);
		if (zip.Open(zipfile))
			return -1;

		string fn = filename.substr(pos_zip + 5);

		if (!zip.check_file_exist(fn))
			return -1;

		this->m_loaded = false;

		if (async)
		{
			m_aync_thread = thread(zip_async_load, zipfile, fn, this);

		}else
		{
			shared_ptr<FileBuffer> p = zip.get_file_buff(fn);
			if (p == NULL)
				return -1;

			this->swap(*p);
			this->m_loaded = true;
		}
		m_timesample = st.st_mtime;
		return 0;
	}

	if (pos_sdcard != string::npos)
	{
		Fat fat;
		if (fat.Open(filename.substr(0, pos_sdcard + strlen(".SDCARD"))))
		{
			string err = "Fail Open File: ";
			err.append(filename.substr(1, pos_sdcard + strlen(".SDCARD")));
			set_last_err_string(err);
			return -1;
		}
		string fn = filename.substr(pos_sdcard + strlen(".SDCARD") + 1);
		shared_ptr<FileBuffer> p = fat.get_file_buff(fn);
		if (p == NULL)
			return -1;

		this->swap(*p);
		m_timesample = st.st_mtime;
		return 0;
	}

	m_timesample = st.st_mtime;

	this->unmapfile();

	if (this->mapfile(filename.substr(1), st.st_size))
		return -1;
	
	this->m_loaded = true;

	return 0;
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