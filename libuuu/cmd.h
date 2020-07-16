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

#include <map>
#include <memory>
#include <string>
#include <vector>

class ConfigItem;

std::string get_next_param(const std::string &cmd, size_t &pos, char sperate = ' ');

class CmdCtx
{
public:
	CmdCtx() = default;
	CmdCtx(const CmdCtx&) = delete;
	CmdCtx& operator=(const CmdCtx&) = delete;
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
	CmdBase() = default;
	CmdBase(char *p) { if (p) m_cmd = p; }
	virtual ~CmdBase();

	virtual int dump();
	const std::string& get_cmd() const noexcept { return m_cmd; }
	bool get_lastcmd() const noexcept { return m_lastcmd; }
	void insert_param_info(const char *key, void *pD, Param::Type tp, bool ignore_case = true, const char* err = nullptr)
	{
		m_param.emplace_back(Param{key, pD, tp, ignore_case, err});
	}
	virtual int parser_protocal(char *p, size_t &pos);
	virtual int parser(char *p = nullptr);
	virtual int run(CmdCtx *p) = 0;

protected:
	bool m_bCheckTotalParam = false;
	std::string m_cmd;
	bool m_lastcmd = false;
	bool m_NoKeyParam = false;
	uint64_t m_timeout = 2000;

private:
	std::vector<Param> m_param;
};

using CreateCmdObj = std::shared_ptr<CmdBase> (*) (char *);

class CmdObjCreateMap:public std::map<std::string, CreateCmdObj>
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
	CmdDelay(char *p) :CmdBase(p) {}

	int parser(char *p = nullptr) override;
	int run(CmdCtx *p) override;

private:
	int m_ms = 0;
};

class CmdShell : public CmdBase
{
public:
	CmdShell(char *p) : CmdBase(p) {}

	int parser(char *p = nullptr) override;
	int run(CmdCtx *p) override;

private:
	bool m_dyn = false;
	std::string m_protocal;
	std::string m_shellcmd;
};

class CmdEnv : public CmdBase
{
public:
	using CmdBase::CmdBase;

	int parser(char *p = nullptr) override;
	int run(CmdCtx *p) override;

private:
	std::string m_unfold_cmd;
};

class CmdList : public std::vector<std::shared_ptr<CmdBase>>
{
public:
	int run_all(CmdCtx *p, bool dry_run = false);
};

class CmdMap : public std::map<std::string, std::shared_ptr<CmdList>>
{
public:
	int run_all(const std::string &protocal, CmdCtx *p,  bool dry_run = false);
};

class CfgCmd :public CmdBase
{
public:
	CfgCmd(char *cmd) :CmdBase(cmd) {}

	int parser(char * /*p*/) override { return 0; }
	int run(CmdCtx *p) override;
};

int run_cmds(const char *procotal, CmdCtx *p);
int run_cmd(CmdCtx *pCtx, const char * cmd, int dry);
