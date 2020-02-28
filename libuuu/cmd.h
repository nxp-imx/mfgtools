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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "liberror.h"
#include "libcomm.h"
#include "config.h"

using namespace std;

string get_next_param(string &cmd, size_t &pos, char sperate = ' ');
string remove_square_brackets(string &cmd);
int get_string_in_square_brackets(string &cmd, string &context);
uint32_t str_to_uint(string &str);

class CmdCtx
{
public:
	CmdCtx() { m_config_item = NULL; m_dev = NULL; };
	virtual ~CmdCtx() {};
	ConfigItem *m_config_item;
	void *m_dev;
};

class CmdUsbCtx : public CmdCtx
{
public:
	CmdUsbCtx() :CmdCtx() {};
	~CmdUsbCtx();
	int look_for_match_device(const char * procotol);
};

struct Param
{
	enum Param_Type
	{
		e_uint32,
		e_bool,
		e_string,
		e_null,
		e_string_filename,
	};

	const char * key;
	const char * Error;
	void *pData;
	int type;
	bool ignore_case;
	Param(const char *ky, void *pD, Param_Type tp, bool ignore=true, const char *error = NULL)
	{
		key = ky; pData = pD; type = tp; ignore_case = ignore; Error = error;
	}
};

class CmdBase
{
public:
	vector<Param> m_param;
	uint64_t m_timeout;
	bool m_lastcmd;
	std::string m_cmd;
	bool m_NoKeyParam;
	bool m_bCheckTotalParam;

	void CmdBaseInit() { m_timeout = 2000; m_lastcmd = false; m_NoKeyParam = false; m_bCheckTotalParam = false; }
	CmdBase() { CmdBaseInit(); };
	CmdBase(char *p) { CmdBaseInit(); if (p) m_cmd = p; }

	void insert_param_info(const char *key, void *pD, Param::Param_Type tp, bool ignore_case = true, const char* err = NULL)
	{
		m_param.push_back(Param(key, pD, tp, ignore_case, err));
	}

	virtual int parser_protocal(char *p, size_t &pos)
	{
		if (p)
			m_cmd = *p;

		string prot = get_next_param(m_cmd, pos, ':');
		string param;
		if (get_string_in_square_brackets(prot, param))
			return -1;
		
		if (!param.empty())
		{
			size_t param_pos = 0;
			string s = get_next_param(param, param_pos);

			if (s == "-t")
			{
				string timeout;
				timeout = get_next_param(param, param_pos);
				m_timeout = str_to_uint(timeout);
			}
			else
			{
				string err;
				err = "Unknown option: ";
				err += s;
				err += " for protocol: ";
				err += remove_square_brackets(prot);
				set_last_err_string(err);
				return -1;
			}
		}
		return 0;
	}
	virtual int parser(char *p = NULL);
	virtual int run(CmdCtx *p)=0;
	virtual int dump();
};

typedef shared_ptr<CmdBase> (*CreateCmdObj) (char *);

class CmdObjCreateMap:public map<string, CreateCmdObj>
{
public:
	CmdObjCreateMap();
};

class CmdDone :public CmdBase
{
public:
	CmdDone(char *p) :CmdBase(p) { m_lastcmd = true; };
	int run(CmdCtx *p);
};

class CmdDelay :public CmdBase
{
public:
	int m_ms;
	virtual int parser(char *p = NULL);
	CmdDelay(char *p) :CmdBase(p) { m_ms = 0; };
	int run(CmdCtx *p);
};

class CmdShell : public CmdBase
{
public:
	string m_shellcmd;
	string m_protocal;
	bool	m_dyn;

	CmdShell(char *p) : CmdBase(p) { m_dyn = false; };
	virtual int parser(char *p = NULL);
	int run(CmdCtx *p);
};

class CmdList : public std::vector<shared_ptr<CmdBase>>
{
public:
	int run_all(CmdCtx *p, bool dry_run = false);
};

class CmdMap : public std::map<std::string, shared_ptr<CmdList>>
{
public:
	int run_all(std::string protocal, CmdCtx *p,  bool dry_run = false)
	{
		if (find(protocal) == end())
		{
			set_last_err_id(-1);
			std::string err;
			err.append("Unknown Protocal:");
			err.append(protocal);
			set_last_err_string(err);
			return -1;
		}
		return at(protocal)->run_all(p, dry_run);
	};
};

class CfgCmd :public CmdBase
{
public:
	int parser(char * /*p*/) { return 0; };
	CfgCmd(char *cmd) :CmdBase(cmd) {};
	int run(CmdCtx *p);
};

int run_cmds(const char *procotal, CmdCtx *p);
