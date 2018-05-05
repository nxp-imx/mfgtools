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

/*
 Android fastboot protocol define at 
 https://android.googlesource.com/platform/system/core/+/master/fastboot/
*/
#include <string.h>
#include "fastboot.h"
#include "libcomm.h"
#include "cmd.h"
#include "buffer.h"
#include "libuuu.h"
#include <iostream>
#include <fstream>

int FastBoot::Transport(string cmd, void *p, size_t size, vector<uint8_t> *input)
{
	if (m_pTrans->write((void*)cmd.data(), cmd.size()))
		return -1;

	char buff[65];
	memset(buff, 0, 65);

	while ( strncmp(buff, "OKAY", 4) && strncmp(buff, "FAIL", 4))
	{
		size_t actual;
		memset(buff, 0, 65);
		if (m_pTrans->read(buff, 64, &actual))
			return -1;
		
		if (strncmp(buff, "DATA",4) == 0)
		{
			size_t sz;
			sz = strtoul(buff+4, NULL, 16);
			if (input)
			{
				input->resize(sz);
				size_t rz;
				if (m_pTrans->read(input->data(), sz, &rz))
					return -1;
				input->resize(rz);
			}
			else
			{
				if (m_pTrans->write(p, sz))
					return -1;
			}
		}else
		{
			string s;
			s = buff + 4;
			m_info += s;
			uuu_notify nt;
			nt.type = uuu_notify::NOTIFY_CMD_INFO;
			nt.str = buff + 4;
			call_notify(nt);
		}
	}

	if (strncmp(buff, "OKAY", 4) == 0)
		return 0;

	set_last_err_string(m_info);
	return -1;
}

int FBGetVar::parser(char *p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;
	string param = get_next_param(m_cmd, pos);

	if (param.find(':') != string::npos)
		param = get_next_param(m_cmd, pos);

	if (str_to_upper(param) != "GETVAR")
	{
		string err = "Uknown Commnd:";
		err += param;
		set_last_err_string(err);
		return -1;
	}

	m_var = get_next_param(m_cmd, pos);
	return 0;
}
int FBGetVar::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	FastBoot fb(&dev);
	string cmd;
	cmd = "getvar:";
	cmd += m_var;

	if (fb.Transport(cmd, NULL, 0))
		return -1;

	m_val = fb.m_info;
	return 0;
}

int FBCmd::parser(char *p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;
	string s;
	s = get_next_param(m_cmd, pos);
	if(s.find(":") != s.npos)
		s = get_next_param(m_cmd, pos);

	if (str_to_upper(s) != str_to_upper(m_fb_cmd))
	{
		string err = "Unknown command: ";
		err += s;
		set_last_err_string(s);
		return -1;
	}

	m_uboot_cmd = m_cmd.substr(pos);
	return 0;
}

int FBCmd::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	FastBoot fb(&dev);
	string cmd;
	cmd = m_fb_cmd;
	cmd += ":";
	cmd += m_uboot_cmd;

	if (fb.Transport(cmd, NULL, 0))
		return -1;

	return 0;
}

int FBDownload::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	FastBoot fb(&dev);

	shared_ptr<FileBuffer> buff = get_file_buffer(m_filename);
	if (buff == NULL)
		return -1;

	string_ex cmd;
	cmd.format("download:%08x", buff->size());

	if (fb.Transport(cmd, buff->data(), buff->size()))
		return -1;

	return 0;
}

int FBCopy::parser(char *p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;
	string s;
	s = get_next_param(m_cmd, pos);
	if (s.find(":") != s.npos)
		s = get_next_param(m_cmd, pos);

	if ((str_to_upper(s) != "UCP"))
	{
		string err = "Unknown command: ";
		err += s;
		set_last_err_string(s);
		return -1;
	}

	string source;
	string dest;

	source = get_next_param(m_cmd, pos);
	dest = get_next_param(m_cmd, pos);

	if (source.empty())
	{
		set_last_err_string("ucp: source missed");
		return -1;
	}

	if (dest.empty())
	{
		set_last_err_string("ucp: destination missed");
		return -1;
	}
	
	if (source.find("T:") == 0)
	{
		if (dest.find("T:") == 0)
		{
			set_last_err_string("ucp just support one is remote file start with R:");
			return -1;
		}
		m_target_file = source.substr(2);
		m_bDownload = false; //upload a file
		m_local_file = dest;
	}
	else if (dest.find("T:") == 0)
	{
		m_target_file = dest.substr(2);
		m_bDownload = true;
		m_local_file = source;
	}
	else
	{
		set_last_err_string("ucp must a remote file name, start with R:<file name>");
		return -1;
	}
	return 0;
}

int FBCopy::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	FastBoot fb(&dev);
	string_ex cmd;

	if(m_bDownload)
	{
		size_t i;
		shared_ptr<FileBuffer> buff = get_file_buffer(m_local_file);
		if (buff == NULL)
		{
			return -1;
		}

		cmd.format("WOpen:%s", m_target_file);
		if (fb.Transport(cmd, NULL, 0))
			return -1;

		uuu_notify nt;
		nt.type = uuu_notify::NOTIFY_CMD_TOTAL;
		nt.total = buff->size();
		call_notify(nt);

		for (size_t i = 0; i < buff->size(); i += this->m_Maxsize_pre_cmd)
		{
			size_t sz = buff->size() - i;
			if (sz > m_Maxsize_pre_cmd)
				sz = m_Maxsize_pre_cmd;

			cmd.format("donwload:%08X", sz);
			if (fb.Transport(cmd, buff->data() + i, sz))
				return -1;

			nt.type = uuu_notify::NOTIFY_TRANS_POS;
			nt.index = i;
			call_notify(nt);
		}
	}
	else
	{
		cmd.format("ROpen:%s", m_target_file);
		if (fb.Transport(cmd, NULL, 0))
			return -1;
		
		uuu_notify nt;
		nt.type = uuu_notify::NOTIFY_CMD_TOTAL;
		size_t total = nt.total = strtoul(fb.m_info.c_str(), NULL, 16);
		call_notify(nt);

		nt.index = 0;
		ofstream of;
		of.open(m_local_file, ofstream::binary);

		if (!of)
		{
			string err;
			err = "Fail to open file";
			err += m_local_file;
			set_last_err_string(err);
		}
		do
		{
			vector<uint8_t> data;
			if (fb.Transport("upload", NULL, 0, &data))
				return -1;
			
			of.write((const char*)data.data(), data.size());

			nt.type = uuu_notify::NOTIFY_TRANS_POS;
			nt.index += data.size();
			call_notify(nt);
		} while (nt.index < total);
	}

	cmd.format("Close");
	if (fb.Transport(cmd, NULL, 0))
		return -1;
}