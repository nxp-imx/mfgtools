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

using namespace std;

#define YELLOW "\x1B[93m"
#define DEFAULT "\x1B[0m"
#define GREEN "\x1B[92m"
#define RED	"\x1B[91m"
#define KCYN  "\x1B[36m"
#define BOLDWHITE "\x1B[97m"

struct BuildCmd
{
	const char *m_cmd;
	const char *m_buildcmd;
	const char *m_desc;
};

struct Arg
{
	string m_arg;
	string m_desc;
};

class BuildInScript
{
public:
	string m_script;
	string m_desc;
	string m_cmd;
	vector<Arg> m_args;
	bool find_args(string arg)
	{
		for (size_t i = 0; i < m_args.size(); i++)
		{
			if (m_args[i].m_arg == arg)
				return true;
		}
		return false;
	}
	BuildInScript() {};
	BuildInScript(BuildCmd*p)
	{
		m_script = p->m_buildcmd;
		if(p->m_desc)
			m_desc = p->m_desc;
		if(p->m_cmd)
			m_cmd = p->m_cmd;

		for (size_t i = 1; i < m_script.size(); i++)
		{
			size_t off;
			string param;
			if (m_script[i] == '_' && m_script[i - 1] == ' ')
			{
				off = m_script.find(' ', i);
				size_t ofn = m_script.find('\n', i);
				if (ofn < off)
					off = ofn;

				if (off == string::npos)
					off = m_script.size() + 1;

				param = m_script.substr(i, off - i);
				if (!find_args(param))
				{
					Arg a;
					a.m_arg = param;
					m_args.push_back(a);
				}
			}
		}
	}

	void show()
	{
		printf("%s\n", m_script.c_str());
	}

	void show_cmd()
	{
		printf("\t%s%s%s\t%s\n", BOLDWHITE, m_cmd.c_str(), DEFAULT,  m_desc.c_str());
		for (size_t i=0; i < m_args.size(); i++)
		{
			printf("\t\targ%d: %s\n", (int)i, m_args[i].m_arg.c_str());
		}
	}

	string replace_script_args(vector<string> args)
	{
		string script = m_script;
		for (size_t i = 0; i < args.size(); i++)
		{
			for (size_t j = 0; (j = script.find(m_args[i].m_arg, j)) != string::npos;)
			{
				script.replace(j, m_args[i].m_arg.size(), args[i]);
				j += args[i].size();
			}
		}
		return script;
	}
};

class BuildInScriptVector : public map<string, BuildInScript>
{
public:
	BuildInScriptVector(BuildCmd*p)
	{
		while (p->m_cmd)
		{
			BuildInScript one(p);
			(*this)[one.m_cmd] = one;
			p++;
		}
	}

	void ShowAll()
	{
		for (auto iCol = begin(); iCol != end(); ++iCol)
		{
			iCol->second.show_cmd();
		}
	}

	void ShowCmds()
	{
		printf("<");
		for (auto iCol = begin(); iCol != end(); ++iCol)
		{
			printf("%s%s%s", BOLDWHITE, iCol->first.c_str(), DEFAULT);

			auto i = iCol;
			i++;
			if(i != end())
				printf("|");
		}
		printf(">");
	}
};

extern BuildInScriptVector g_BuildScripts;