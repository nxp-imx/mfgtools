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

string g_current_dir;

void set_current_dir(string dir)
{
	g_current_dir = dir;
}

uint64_t get_file_timesample(string filename)
{
	struct stat64 st;
	if (stat64(filename.c_str(), &st))
	{
		string path = str_to_upper(filename);
		size_t pos = path.find(".ZIP");
		if (pos == string::npos)
			return 0;
		stat64(filename.substr(0, pos + 4).c_str(), &st);
		return st.st_mtime;
	}

	return st.st_mtime;
}

shared_ptr<FileBuffer> get_file_buffer(string filename)
{
	filename = g_current_dir + filename;

	if (g_filebuffer_map.find(filename) == g_filebuffer_map.end())
	{
		shared_ptr<FileBuffer> p(new FileBuffer);

		if (p->reload(filename))
			return NULL;
		
		g_filebuffer_map[filename]=p;

		return p;
	}
	else
	{
		shared_ptr<FileBuffer> p = g_filebuffer_map[filename];
		if (p->m_timesample != get_file_timesample(filename))
			if (p->reload(filename))
			{
				return NULL;
			}
		return p;
	}
}

int FileBuffer::reload(string filename)
{
	struct stat64 st;
	size_t pos_zip = string::npos;
	size_t pos_sdcard = string::npos;

	if (stat64(filename.c_str(), &st))
	{
		string path = str_to_upper(filename);
		pos_zip = path.find(".ZIP");
		string zipfile = filename.substr(0, pos_zip + 4);
		if (pos_zip == string::npos || (stat64(zipfile.c_str(), &st)))
		{
			pos_sdcard = path.find(".SDCARD");
			string sdcardfile = filename.substr(0, pos_sdcard + strlen(".SDCARD"));
			if (pos_sdcard == string::npos || (stat64(sdcardfile.c_str(), &st)))
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
		if (zip.Open(filename.substr(0, pos_zip + 4)))
		{
			string err = "Fail Open File: ";
			err.append(filename.substr(0, pos_zip + 4));
			set_last_err_string(err);
			return -1;
		}
		string fn = filename.substr(pos_zip + 5);
		shared_ptr<FileBuffer> p = zip.get_file_buff(fn);
		if (p == NULL)
			return -1;

		this->swap(*p);
		m_timesample = st.st_mtime;
		return 0;
	}

	if (pos_sdcard != string::npos)
	{
		Fat fat;
		if (fat.Open(filename.substr(0, pos_sdcard + strlen(".SDCARD"))))
		{
			string err = "Fail Open File: ";
			err.append(filename.substr(0, pos_sdcard + strlen(".SDCARD")));
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
	resize(st.st_size);

	ifstream is(filename, ifstream::binary);
	if (is)
	{
		is.read((char*)data(), st.st_size);
		if (is)
			return 0;
		else
		{
			string err = "Fail Read File: ";
			err.append(filename);
			set_last_err_string(err);
			return -1;
		}
	}
	return 0;
}
