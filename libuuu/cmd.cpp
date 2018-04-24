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

#include "cmd.h"
#include "libcomm.h"
#include "libuuu.h"
#include "config.h"
#include "trans.h"
#include "sdps.h"

static CmdMap g_cmd_map;

int CmdList::run_all(void *p, bool dry_run)
{
	CmdList::iterator it;
	int ret;
	for (it = begin(); it != end(); it++)
	{
		if (dry_run)
		{
			(*it)->dump();
		}
		else
		{
			notify nt;
			nt.type = notify::NOTIFY_CMD_START;
			nt.str = (char *)(*it)->m_cmd.c_str();
			call_notify(nt);
			ret = (*it)->run(p);
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
	
	size_t end = cmd.find(' ', pos);
	if (end < 0)
		end = cmd.size();

	str = cmd.substr(pos, end - pos);
	pos = end + 1;

	return str;
}

int str_to_int(string &str)
{
	if (str.size() > 2)
	{
		if (str.substr(0, 2).compare("0x") == 0)
			return strtol(str.substr(2).c_str(), NULL, 16);
	}
	return strtol(str.c_str(), NULL, 10);
}

shared_ptr<CmdBase> CreateCmdObj(string cmd)
{
	size_t pos = cmd.find(": ", 0);
	if (pos == string::npos)
		return NULL;
	
	string c;
	c = cmd.substr(0, pos+1);
	if (c == "CFG:")
		return shared_ptr<CmdBase>(new CfgCmd((char*)cmd.c_str()));
	if (c == "SDPS:")
		return shared_ptr<CmdBase>(new SDPSCmd((char*)cmd.c_str()));
	return NULL;
}

int run_cmd(const char * cmd)
{
	shared_ptr<CmdBase> p;
	p = CreateCmdObj(cmd);
	int ret;

	notify nt;
	nt.type = notify::NOTIFY_CMD_START;
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
			ret = p->run(get_dev(pro.c_str()));
	}
	else
	{
		return ret =p->run(NULL);
	}

	nt.type = notify::NOTIFY_CMD_END;
	nt.status = ret;
	call_notify(nt);

	return -1;
}


int run_cmds(const char *procotal, void *p)
{
	if (g_cmd_map.find(procotal) == g_cmd_map.end())
	{
		string_ex str;
		str.format("%s:%d Can't find protocal: %s", __FUNCTION__, __LINE__, procotal);
		set_last_err_string(str.c_str());
	}

	return g_cmd_map[procotal]->run_all(p);;
}