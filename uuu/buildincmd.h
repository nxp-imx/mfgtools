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
#include <string>
#include <vector>

extern const char * g_vt_yellow ;
extern const char * g_vt_default ;
extern const char * g_vt_green ;
extern const char * g_vt_red  ;
extern const char * g_vt_kcyn ;
extern const char * g_vt_boldwhite ;

struct BuildCmd
{
	BuildCmd() = default;
	constexpr BuildCmd(const char * const cmd, const char * const buildcmd,
		const char * const desc);

	const char *m_cmd;
	const char *m_buildcmd;
	const char *m_desc;
};

class Arg
{
public:
	enum
	{
		ARG_MUST = 0x1,
		ARG_OPTION = 0x2,
		ARG_OPTION_KEY = 0x4,
	};

	Arg(const std::string &arg) : m_arg{arg} {}

	const std::string &get_arg() const noexcept { return m_arg; }
	const std::string &get_options() const noexcept { return m_options; }
	int parser(const std::string &option);

	std::string m_desc;
	uint32_t m_flags = ARG_MUST;

private:
	std::string m_arg;
	std::string m_options;
};

class BuildInScript
{
public:
	BuildInScript() {};
	BuildInScript(const BuildCmd &p);

	inline const std::string &get_cmd() const noexcept { return m_cmd; }
	inline const std::string &get_script() const noexcept { return m_script; }
	std::string replace_script_args(std::vector<std::string> args);
	inline void show() { printf("%s\n", m_script.c_str()); }
	void show_cmd() const;

private:
	bool find_args(const std::string &arg) const;
	static std::string replace_str(std::string str, const std::string &key, std::string replace);
	static std::string str_to_upper(const std::string &str);


	std::string m_script;
	std::string m_desc;
	std::string m_cmd;
	std::vector<Arg> m_args;
};

class BuildInScriptVector : public std::map<std::string, BuildInScript>
{
public:
	BuildInScriptVector(const std::array<const BuildCmd, 8> &build_cmds);

	void PrintAutoComplete(const std::string &match, const char *space=" " );
	void ShowAll() const;
	void ShowCmds() const;
};

extern BuildInScriptVector g_BuildScripts;
