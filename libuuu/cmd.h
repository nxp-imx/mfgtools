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
	virtual ~CmdCtx();
	ConfigItem *m_config_item = nullptr;
	void *m_dev = nullptr;
};

class CmdUsbCtx : public CmdCtx
{
public:
	~CmdUsbCtx() override;
	int look_for_match_device(const char * procotol);
};

struct Param
{
	enum class Type
	{
		e_uint32,
		e_bool,
		e_string,
		e_null,
		e_string_filename,
	};

	const char * const key;
	const char * const Error;
	void *pData;
	const Type type;
	const bool ignore_case;
	Param(const char *ky, void *pD, Type tp, bool ignore = true, const char *error = nullptr) :
		key{ky}, Error{error}, pData{pD}, type{tp}, ignore_case{ignore}
	{
	}
};

class CmdBase
{
public:
	vector<Param> m_param;
	uint64_t m_timeout = 2000;
	bool m_lastcmd = false;
	std::string m_cmd;
	bool m_NoKeyParam = false;
	bool m_bCheckTotalParam = false;

	CmdBase() = default;
	CmdBase(char *p) { if (p) m_cmd = p; }
	virtual ~CmdBase();

	void insert_param_info(const char *key, void *pD, Param::Type tp, bool ignore_case = true, const char* err = nullptr)
	{
		m_param.emplace_back(Param{key, pD, tp, ignore_case, err});
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
	virtual int parser(char *p = nullptr);
	virtual int run(CmdCtx *p) = 0;
	virtual int dump();
};

using CreateCmdObj = shared_ptr<CmdBase> (*) (char *);

class CmdObjCreateMap:public map<string, CreateCmdObj>
{
public:
	CmdObjCreateMap();
};

class CmdDone :public CmdBase
{
public:
	CmdDone(char *p) :CmdBase(p) { m_lastcmd = true; }
	int run(CmdCtx *p) override;
};

class CmdDelay :public CmdBase
{
public:
	int m_ms = 0;
	CmdDelay(char *p) :CmdBase(p) {}
	int parser(char *p = nullptr) override;
	int run(CmdCtx *p) override;
};

class CmdShell : public CmdBase
{
public:
	string m_shellcmd;
	string m_protocal;
	bool	m_dyn = false;

	CmdShell(char *p) : CmdBase(p) {}
	int parser(char *p = nullptr) override;
	int run(CmdCtx *p) override;
};

class CmdList : public std::vector<shared_ptr<CmdBase>>
{
public:
	int run_all(CmdCtx *p, bool dry_run = false);
};

class CmdMap : public std::map<std::string, shared_ptr<CmdList>>
{
public:
	int run_all(const std::string &protocal, CmdCtx *p,  bool dry_run = false)
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
	CfgCmd(char *cmd) :CmdBase(cmd) {}
	int parser(char * /*p*/) override { return 0; }
	int run(CmdCtx *p) override;
};

int run_cmds(const char *procotal, CmdCtx *p);
