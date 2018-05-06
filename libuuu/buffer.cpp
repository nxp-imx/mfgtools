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

static map<string, shared_ptr<FileBuffer>> g_filebuffer_map;

string g_current_dir;

void set_current_dir(string dir)
{
	g_current_dir = dir;
}

uint64_t get_file_timesample(string filename)
{
	struct stat st;
	stat(filename.c_str(), &st);

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
	struct stat st;
	if (stat(filename.c_str(), &st))
	{
		string err = "Fail Open File: ";
		err.append(filename);
		set_last_err_string(err);
		return -1;
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