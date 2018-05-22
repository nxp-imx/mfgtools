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


static CmdMap g_cmd_map;
static CmdObjCreateMap g_cmd_create_map;

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

			if (get_file_buffer(param) == NULL)
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

string get_next_param(string &cmd, size_t &pos)
{
	string str;
	if (pos == string::npos)
		return str;
	if (pos >= cmd.size())
		return str;
	
	//trim left space
	while (cmd[pos] == ' ' && pos < cmd.size())
		pos++;

	size_t end = cmd.find(' ', pos);
	if (end == cmd.npos)
		end = cmd.size();

	str = cmd.substr(pos, end - pos);
	pos = end + 1;

	return str;
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
	(*this)["SDPS:DONE"] = new_cmd_obj<CmdDone>;

	(*this)["SDP:DCD"] = new_cmd_obj<SDPDcdCmd>;
	(*this)["SDP:JUMP"] = new_cmd_obj<SDPJumpCmd>;
	(*this)["SDP:WRITE"] = new_cmd_obj<SDPWriteCmd>;
	(*this)["SDP:STATUS"] = new_cmd_obj<SDPStatusCmd>;
	(*this)["SDP:BOOT"] = new_cmd_obj<SDPBootCmd>;
	(*this)["SDP:DONE"] = new_cmd_obj<CmdDone>;

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
	(*this)["FB:DONE"] = new_cmd_obj<CmdDone>;
	(*this)["FASTBOOT:DONE"] = new_cmd_obj<CmdDone>;

	(*this)["FBK:UCMD"] = new_cmd_obj<FBUCmd>;
	(*this)["FBK:ACMD"] = new_cmd_obj<FBACmd>;
	(*this)["FBK:SYNC"] = new_cmd_obj<FBSyncCmd>;
	(*this)["FBK:UCP"] = new_cmd_obj<FBCopy>;
	(*this)["FBK:DONE"] = new_cmd_obj<CmdDone>;
}

shared_ptr<CmdBase> create_cmd_obj(string cmd)
{
	string param;
	size_t pos = 0;
	param = get_next_param(cmd, pos);
	param = str_to_upper(param);
	if (g_cmd_create_map.find(param) == g_cmd_create_map.end())
	{
		string s = param;
		param = get_next_param(cmd, pos);
		s += str_to_upper(param);
		if (g_cmd_create_map.find(s) != g_cmd_create_map.end())
			return g_cmd_create_map[s]((char*)cmd.c_str());
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

int run_cmds(const char *procotal, CmdCtx *p)
{
	if (g_cmd_map.find(procotal) == g_cmd_map.end())
	{
		string s = procotal;
		if (s == "CFG:")
		{
			return 0; //Allow no CFG command
		}
		string_ex str;
		str.format("%s:%d Can't find protocal: %s", __FUNCTION__, __LINE__, procotal);
		set_last_err_string(str.c_str());
		return -1;
	}
	
	return g_cmd_map[procotal]->run_all(p);
}

static int insert_one_cmd(const char * cmd)
{
	string s = cmd;
	size_t pos = 0;

	string pro = get_next_param(s, pos);
	shared_ptr<CmdBase> p = create_cmd_obj(s);
	if (p == NULL)
		return -1;

	if (p->parser())
		return -1;

	if (g_cmd_map.find(pro) == g_cmd_map.end())
	{
		shared_ptr<CmdList> list(new CmdList);
		g_cmd_map[pro] = list;
	}

	g_cmd_map[pro]->push_back(p);

	return 0;
}


static int added_default_boot_cmd(const char *filename)
{
	string str;
	str = "SDPS: boot -f ";
	str += filename;

	int ret = insert_one_cmd(str.c_str());
	if (ret) return ret;

	insert_one_cmd("SDPS: done");

	str = "SDP: boot -f ";
	str += filename;

	ret = insert_one_cmd(str.c_str());
	if (ret) return ret;

	insert_one_cmd("SDP: done");

	return 0;
}

int check_version(string str)
{
	int x = 0;
	int ver = 0;
	for (int i = 0; i < str.size(); i++)
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

int parser_cmd_list_file(shared_ptr<FileBuffer> pbuff)
{
	char uuu_version[] = "uuu_version";
	string str;

	for (int i = 0; i < pbuff->size(); i++)
	{
		uint8_t c = pbuff->at(i);
		if (c == '\r')
			continue;
		
		if(c != '\n')
			str.push_back(c);

		if (c == '\n' || c == 0)
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
					if (insert_one_cmd(str.c_str()))
						return -1;
			}
			str.clear();
		}
	}
	return 0;
}

int uuu_auto_detect_file(const char *filename)
{
	string fn = filename;
	if (fn.empty())
		fn = "./";
	
	string oldfn =fn;

	fn += "/uuu.auto";
	shared_ptr<FileBuffer> buffer = get_file_buffer(fn);
	if (buffer == NULL)
	{
		fn = oldfn;
		size_t pos = str_to_upper(fn).find("ZIP");
		if(pos == string::npos || pos != fn.size() - 3)
			buffer = get_file_buffer(fn); //we don't try open a zip file here

		if(buffer == NULL)
			return -1;
	}

	string str= "uuu_version";
	void *p1 = buffer->data();
	void *p2 = (void*)str.data();
	if (memcmp(p1, p2, str.size()) == 0)
	{
		for (size_t i = 0; i < fn.size(); i++)
			if (fn[i] == '\\')
				fn[i] = '/';

		size_t pos = fn.rfind('/');
		if (pos != string::npos)
			set_current_dir(fn.substr(0, pos + 1));

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
