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

#include <memory>
#include <string.h>
#include "cmd.h"
#include "libcomm.h"
#include "libuuu.h"
#include "config.h"
#include "trans.h"
#include "sdps.h"
#include <atomic>
#include "buffer.h"
#include "sdp.h"
#include "fastboot.h"
#include <sys/stat.h>
#include <thread>

#include <stdio.h>  
#include <stdlib.h>  

static CmdMap g_cmd_map;
static CmdObjCreateMap g_cmd_create_map;
static string g_cmd_list_file;

int parser_cmd_list_file(shared_ptr<FileBuffer> pbuff, CmdMap *pCmdMap = NULL);

template <class T>
void * create_object() { return new T; }

typedef void * (*FN)();

FN g_fn = create_object<int>;


int CmdBase::parser(char *p)
{
	if (p != NULL)
		m_cmd = p;

	size_t pos = 0;
	string param = get_next_param(m_cmd, pos);

	if (param.find(':') != string::npos)
		param = get_next_param(m_cmd, pos);

	while (pos < m_cmd.size())
	{
		param = get_next_param(m_cmd, pos);

		struct Param *pp = NULL;
		for (size_t i = 0; i < m_param.size(); i++)
		{
			string key = string(m_param[i].key);
			if (compare_str(param, key, m_param[i].ignore_case))
			{
				pp = &(m_param[i]);
				break;
			}
		}

		if (pp == NULL)
		{
			string err;
			err = "unknown Option";
			err += param;
			set_last_err_string(err);
			return -1;
		}

		if (pp->type == Param::e_uint32)
		{
			param = get_next_param(m_cmd, pos);
			*(uint32_t*)pp->pData = str_to_uint(param);
		}

		if (pp->type == Param::e_string_filename)
		{
			param = get_next_param(m_cmd, pos);
			*(string*)pp->pData = param;

			if (!check_file_exist(param))
				return -1;
		}

		if (pp->type == Param::e_string)
		{
			param = get_next_param(m_cmd, pos);
			*(string*)pp->pData = param;
		}

		if (pp->type == Param::e_bool)
		{
			*(bool*)pp->pData = true;
		}

		if (pp->type == Param::e_null)
		{
		}
	}
	return 0;
}

int CmdList::run_all(CmdCtx *p, bool dry_run)
{
	CmdList::iterator it;
	int ret;

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_TOTAL;
	nt.total = size();
	call_notify(nt);

	int i = 0;

	for (it = begin(); it != end(); it++, i++)
	{
		if (dry_run)
		{
			(*it)->dump();
		}
		else
		{
			uuu_notify nt;

			nt.type = uuu_notify::NOTIFY_CMD_INDEX;
			nt.index = i;
			call_notify(nt);

			nt.type = uuu_notify::NOTIFY_CMD_START;
			nt.str = (char *)(*it)->m_cmd.c_str();
			call_notify(nt);

			ret = (*it)->run(p);

			nt.type = uuu_notify::NOTIFY_CMD_END;
			nt.status = ret;
			call_notify(nt);
			if (ret)
				return ret;
		}
	}
	return ret;
}

string get_next_param(string &cmd, size_t &pos, char sperate)
{
	string str;
	if (pos == string::npos)
		return str;
	if (pos >= cmd.size())
		return str;

	//trim left space
	while (cmd[pos] == sperate && pos < cmd.size())
		pos++;

	size_t end = cmd.find(sperate, pos);
	if (end == cmd.npos)
		end = cmd.size();

	str = cmd.substr(pos, end - pos);
	pos = end + 1;

	return str;
}

string remove_square_brackets(string &cmd)
{
	size_t sz=cmd.find('[');
	return cmd.substr(0, sz);
}

int get_string_in_square_brackets(string &cmd, string &context)
{
	size_t start = cmd.find('[');
	if (start == string::npos)
	{
		context.clear();
		return 0;
	}

	size_t end = cmd.find(']', start);
	if (end == string::npos)
	{
		set_last_err_string("missed ]");
		return -1;
	}

	context = cmd.substr(start + 1, end - start - 1);
	return 0;
}

uint32_t str_to_uint(string &str)
{
	if (str.size() > 2)
	{
		if (str.substr(0, 2).compare("0x") == 0)
			return strtoul(str.substr(2).c_str(), NULL, 16);
	}
	return strtoul(str.c_str(), NULL, 10);
}

template <class T> shared_ptr<CmdBase> new_cmd_obj(char *p)
{
	return shared_ptr<CmdBase>(new T(p));
}

CmdObjCreateMap::CmdObjCreateMap()
{
	(*this)["CFG:"] = new_cmd_obj<CfgCmd>;

	(*this)["SDPS:BOOT"] = new_cmd_obj<SDPSCmd>;

	(*this)["SDP:DCD"] = new_cmd_obj<SDPDcdCmd>;
	(*this)["SDP:JUMP"] = new_cmd_obj<SDPJumpCmd>;
	(*this)["SDP:WRITE"] = new_cmd_obj<SDPWriteCmd>;
	(*this)["SDP:STATUS"] = new_cmd_obj<SDPStatusCmd>;
	(*this)["SDP:BOOT"] = new_cmd_obj<SDPBootCmd>;
	(*this)["SDP:BLOG"] = new_cmd_obj<SDPBootlogCmd>;

	(*this)["SDPU:JUMP"] = new_cmd_obj<SDPJumpCmd>;
	(*this)["SDPU:WRITE"] = new_cmd_obj<SDPWriteCmd>;
	(*this)["SDPU:BLOG"] = new_cmd_obj<SDPBootlogCmd>;

	(*this)["FB:GETVAR"] = new_cmd_obj<FBGetVar>;
	(*this)["FASTBOOT:GETVAR"] = new_cmd_obj<FBGetVar>;
	(*this)["FB:UCMD"] = new_cmd_obj<FBUCmd>;
	(*this)["FASTBOOT:UCMD"] = new_cmd_obj<FBUCmd>;
	(*this)["FB:ACMD"] = new_cmd_obj<FBACmd>;
	(*this)["FASTBOOT:ACMD"] = new_cmd_obj<FBACmd>;
	(*this)["FB:DOWNLOAD"] = new_cmd_obj<FBDownload>;
	(*this)["FASTBOOT:DOWNLOAD"] = new_cmd_obj<FBDownload>;
	(*this)["FB:FLASH"] = new_cmd_obj<FBFlashCmd>;
	(*this)["FASTBOOT:FLASH"] = new_cmd_obj<FBFlashCmd>;
	(*this)["FB:ERASE"] = new_cmd_obj<FBEraseCmd>;
	(*this)["FASTBOOT:ERASE"] = new_cmd_obj<FBEraseCmd>;
	(*this)["FB:OEM"] = new_cmd_obj<FBOemCmd>;
	(*this)["FASTBOOT:OEM"] = new_cmd_obj<FBOemCmd>;
	(*this)["FB:FLASHING"] = new_cmd_obj<FBFlashingCmd>;
	(*this)["FASTBOOT:FLASHING"] = new_cmd_obj<FBFlashingCmd>;

	(*this)["FBK:UCMD"] = new_cmd_obj<FBUCmd>;
	(*this)["FBK:ACMD"] = new_cmd_obj<FBACmd>;
	(*this)["FBK:SYNC"] = new_cmd_obj<FBSyncCmd>;
	(*this)["FBK:UCP"] = new_cmd_obj<FBCopy>;

	(*this)["_ALL:DONE"] = new_cmd_obj<CmdDone>;
	(*this)["_ALL:DELAY"] = new_cmd_obj<CmdDelay>;
	(*this)["_ALL:SH"] = new_cmd_obj<CmdShell>;
	(*this)["_ALL:SHELL"] = new_cmd_obj<CmdShell>;
	(*this)["_ALL:<"] = new_cmd_obj<CmdShell>;

}

shared_ptr<CmdBase> create_cmd_obj(string cmd)
{
	string param;
	size_t pos = 0;
	param = get_next_param(cmd, pos, ':');
	param = remove_square_brackets(param);
	param += ":";
	param = str_to_upper(param);

	if (g_cmd_create_map.find(param) == g_cmd_create_map.end())
	{
		string s = param;
		param = get_next_param(cmd, pos);
		s += str_to_upper(param);
		if (g_cmd_create_map.find(s) != g_cmd_create_map.end())
			return g_cmd_create_map[s]((char*)cmd.c_str());

		string commoncmd = "_ALL:";
		commoncmd += str_to_upper(param);
		if (g_cmd_create_map.find(commoncmd) != g_cmd_create_map.end())
			return g_cmd_create_map[commoncmd]((char*)cmd.c_str());
	}
	else
	{
		return g_cmd_create_map[param]((char*)cmd.c_str());
	}

	string err;
	err = "Unknow Command:";
	err += cmd;
	set_last_err_string(err);
	return NULL;
}

int uuu_run_cmd(const char * cmd)
{
	shared_ptr<CmdBase> p;
	p = create_cmd_obj(cmd);
	int ret;

	if (p == NULL)
		return -1;

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_CMD_TOTAL;
	nt.total = 1;
	call_notify(nt);

	nt.type = uuu_notify::NOTIFY_CMD_START;
	nt.str = (char *)p->m_cmd.c_str();
	call_notify(nt);

	if (typeid(*p) != typeid(CfgCmd))
	{
		size_t pos = 0;
		string c = cmd;
		string pro = get_next_param(c, pos);
		if (p->parser())
			ret = -1;
		else
		{
			CmdUsbCtx ctx;
			ret = ctx.look_for_match_device(pro.c_str());
			if (ret)
				return ret;

			ret = p->run(&ctx);
		}
	}
	else
	{
		return ret =p->run(NULL);
	}

	nt.type = uuu_notify::NOTIFY_CMD_END;
	nt.status = ret;
	call_notify(nt);

	return ret;
}

int CmdDone::run(CmdCtx *)
{
	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_DONE;
	call_notify(nt);
	return 0;
}

int CmdDelay::parser(char * /*p*/)
{
	size_t pos = 0;
	string param = get_next_param(m_cmd, pos);

	if (param.find(':') != string::npos)
		param = get_next_param(m_cmd, pos);

	if (str_to_upper(param) != "DELAY")
	{
		string err = "Uknown Commnd:";
		err += param;
		set_last_err_string(err);
		return -1;
	}

	string ms = get_next_param(m_cmd, pos);
	m_ms = str_to_uint(ms);
	return 0;
}

int CmdDelay::run(CmdCtx *)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(m_ms));
	return 0;
}

int CmdShell::parser(char * p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;
	string s;

	if (parser_protocal(p, pos))
		return -1;

	m_protocal = m_cmd.substr(0, pos);

	s = get_next_param(m_cmd, pos);

	m_dyn = (s == "<");

	if (pos != string::npos && pos < m_cmd.size())
		m_shellcmd = m_cmd.substr(pos);

	return 0;
}

int CmdShell::run(CmdCtx*)
{
#ifndef WIN32
	#define _popen popen
	#define _pclose pclose
#endif
	FILE *pipe = _popen(m_shellcmd.c_str(), "r");

	if (pipe == NULL)
	{
		string err = "failure popen: ";
		err += m_shellcmd.c_str();
		set_last_err_string(err);
		return -1;
	}

	string str;
	str.resize(256);
	while (fgets((char*)str.c_str(), str.size(), pipe))
	{
		if (m_dyn)
		{
			string cmd;
			cmd = m_protocal;
			str.resize(strlen(str.c_str()));
			cmd += ' ';
			cmd += str;
			
			size_t pos = cmd.find_first_of("\r\n");
			if (pos != string::npos)
				cmd = cmd.substr(0, pos);

			return uuu_run_cmd(cmd.c_str());
		}
		uuu_notify nt;
		nt.type = uuu_notify::NOTIFY_CMD_INFO;
		nt.str = (char*)str.c_str();
		call_notify(nt);
	}

	/* Close pipe and print return value of pPipe. */
	if (feof(pipe))
	{
		int ret = _pclose(pipe);
		string_ex str;
		str.format("\nProcess returned %d\n", ret);;
		if (ret)
		{
			set_last_err_string(str.c_str());
			return ret;
		}
	}
	else
	{
		set_last_err_string("Error: Failed to read the pipe to the end.\n");
		return -1;
	}

	return 0;
}

int run_cmds(const char *procotal, CmdCtx *p)
{
	CmdMap cmdmap, *pCmdMap;

	if (!g_cmd_list_file.empty())
	{
		shared_ptr<FileBuffer> pbuff = get_file_buffer(g_cmd_list_file);
		if (pbuff == NULL)
			return -1;
		if(parser_cmd_list_file(pbuff, &cmdmap))
			return -1;
		pCmdMap = &cmdmap;
	}
	else
	{
		pCmdMap = &g_cmd_map;
	}

	if (pCmdMap->find(procotal) == pCmdMap->end())
	{
		return 0;
	}

	return (*pCmdMap)[procotal]->run_all(p);
}

static int insert_one_cmd(const char * cmd, CmdMap *pCmdMap)
{
	string s = cmd;
	size_t pos = 0;

	string pro = get_next_param(s, pos, ':');
	pro = remove_square_brackets(pro);
	pro += ":";

	pro = str_to_upper(pro);

	shared_ptr<CmdBase> p = create_cmd_obj(s);
	if (p == NULL)
		return -1;

	if (p->parser())
		return -1;

	if (pCmdMap->find(pro) == pCmdMap->end())
	{
		shared_ptr<CmdList> list(new CmdList);
		(*pCmdMap)[pro] = list;
	}

	(*pCmdMap)[pro]->push_back(p);

	return 0;
}


static int added_default_boot_cmd(const char *filename)
{
	string str;
	str = "SDPS: boot -f ";
	str += filename;

	int ret = insert_one_cmd(str.c_str(), &g_cmd_map);
	if (ret) return ret;

	insert_one_cmd("SDPS: done", &g_cmd_map);

	str = "SDP: boot -f ";
	str += filename;

	ret = insert_one_cmd(str.c_str(), &g_cmd_map);
	if (ret) return ret;

	insert_one_cmd("SDP: done", &g_cmd_map);

	str = "SDPU: write -f ";
	str += filename;
	str += " -offset 0x57c00";
	insert_one_cmd(str.c_str(), &g_cmd_map);
	insert_one_cmd("SDPU: jump", &g_cmd_map);
	insert_one_cmd("SDPU: done", &g_cmd_map);

	return 0;
}

int check_version(string str)
{
	int x = 0;
	int ver = 0;
	for (size_t i = 0; i < str.size(); i++)
	{
		char c = str[i];
		if (c >= '0' && c <= '9')
		{
			x *= 10;
			x += c - '0';
		}
		if (c == '.' || i == str.size()-1 || c == '\n')
		{
			ver <<= 8;
			ver += x;
			x = 0;
		}
	}

	int cur = uuu_get_version();

	if (ver > cur)
	{
		string str;
		str = "Current uuu verison is too low, please download latest one";
		set_last_err_string(str);
		return -1;
	}
	return 0;
}

int uuu_run_cmd_script(const char * buff)
{
	shared_ptr<FileBuffer> p(new FileBuffer);
	p->m_data.resize(strlen(buff));
	memcpy(p->m_data.data(), buff, strlen(buff));
	return parser_cmd_list_file(p);
}

int parser_cmd_list_file(shared_ptr<FileBuffer> pbuff, CmdMap *pCmdMap)
{
	char uuu_version[] = "uuu_version";
	string str;

	if (pCmdMap == NULL)
		pCmdMap = &g_cmd_map;

	pCmdMap->clear();

	for (size_t i = 0; i < pbuff->size(); i++)
	{
		uint8_t c = pbuff->at(i);
		if (c == '\r')
			continue;

		if(c != '\n')
			str.push_back(c);

		if (c == '\n' || c == 0 || i == pbuff->size() - 1)
		{
			if (str.substr(0, strlen(uuu_version)) == uuu_version)
			{
				if (check_version(str.substr(strlen(uuu_version), 10)))
				{
					return -1;
				}
			}else if (str.size() > 1)
			{
				if (str[0] != '#')
					if (insert_one_cmd(str.c_str(), pCmdMap))
						return -1;
			}
			str.clear();
		}
	}
	return 0;
}

int uuu_auto_detect_file(const char *filename)
{
	string_ex fn;
	fn += filename;
	fn.replace('\\', '/');

	if (fn.empty())
		fn += "./";

	string oldfn =fn;

	fn += "/uuu.auto";
	shared_ptr<FileBuffer> buffer = get_file_buffer(fn);
	if (buffer == NULL)
	{
		fn.clear();
		fn += oldfn;
		size_t pos = str_to_upper(fn).find("ZIP");
		if(pos == string::npos || pos != fn.size() - 3)
		{
			pos = str_to_upper(fn).find("SDCARD");
			if (pos == string::npos || pos != fn.size() - 6)
				buffer = get_file_buffer(fn); //we don't try open a zip file here
		}

		if(buffer == NULL)
			return -1;
	}

	string str= "uuu_version";
	void *p1 = buffer->data();
	void *p2 = (void*)str.data();
	if (memcmp(p1, p2, str.size()) == 0)
	{
		size_t pos = fn.rfind('/');
		if (pos != string::npos)
			set_current_dir(fn.substr(0, pos + 1));

		g_cmd_list_file = fn.substr(pos+1);

		return parser_cmd_list_file(buffer);
	}

	//flash.bin or uboot.bin
	return added_default_boot_cmd(fn.c_str());
}

int notify_done(uuu_notify nt, void *p)
{
	if(nt.type == uuu_notify::NOTIFY_DONE)
		*(std::atomic<int> *) p = 1;
	if (nt.type == uuu_notify::NOTIFY_CMD_END && nt.status)
		*(std::atomic<int> *) p = 1;

	return 0;
}
int uuu_wait_uuu_finish(int deamon)
{
	std::atomic<int> exit;
	exit = 0;
	if (!deamon)
		uuu_register_notify_callback(notify_done, &exit);

	polling_usb(exit);

	return 0;
}
