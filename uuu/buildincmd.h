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

extern char * g_vt_yellow ;
extern char * g_vt_default ;
extern char * g_vt_green ;
extern char * g_vt_red  ;
extern char * g_vt_kcyn ;
extern char * g_vt_boldwhite ;

struct BuildCmd
{
	const char *m_cmd;
	const char *m_buildcmd;
	const char *m_desc;
};

class Arg
{
public:
	string m_arg;
	string m_desc;
	uint32_t m_flags;
	string m_options;
	enum
	{
		ARG_MUST = 0x1,
		ARG_OPTION = 0x2,
		ARG_OPTION_KEY = 0x4,
	};
	Arg() {	m_flags = ARG_MUST;	}
	int parser(string option)
	{
		size_t pos;
		pos = option.find('[');
		if (pos == string::npos)
			return 0;
		m_options = option.substr(pos + 1, option.find(']') - pos - 1);
		m_flags = ARG_OPTION | ARG_OPTION_KEY;
		return 0;
	}
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
					a.m_flags = Arg::ARG_MUST;
					m_args.push_back(a);
				}
			}
		}

		for (size_t i = 0; i < m_args.size(); i++)
		{
			size_t pos = 0;
			string str;
			str += "@";
			str += m_args[i].m_arg;
			pos = m_script.find(str);
			if (pos != string::npos) {
				string def;
				size_t start_descript;
				start_descript = m_script.find('|', pos);
				if (start_descript != string::npos)
				{
					m_args[i].m_desc = m_script.substr(start_descript + 1,
											m_script.find('\n', start_descript) - start_descript - 1);
					def = m_script.substr(pos, start_descript - pos);
					m_args[i].parser(def);
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
		printf("\t%s%s%s\t%s\n", g_vt_boldwhite, m_cmd.c_str(), g_vt_default,  m_desc.c_str());
		for (size_t i=0; i < m_args.size(); i++)
		{
			string desc;
			desc += m_args[i].m_arg;
			if (m_args[i].m_flags & Arg::ARG_OPTION)
			{
				desc += g_vt_boldwhite;
				desc += "[Optional]";
				desc += g_vt_default;
			}
			desc += " ";
			desc += m_args[i].m_desc;
			printf("\t\targ%d: %s\n", (int)i, desc.c_str());
		}
	}

	string replace_str(string str, string key, string replace)
	{
		for (size_t j = 0; (j = str.find(key, j)) != string::npos;)
		{
			str.replace(j, key.size(), replace);
			j += key.size();
		}
		return str;
	}

	string replace_script_args(vector<string> args)
	{
		string script = m_script;
		for (size_t i = 0; i < args.size(); i++)
		{
			script = replace_str(script, m_args[i].m_arg, args[i]);
		}

		//handle option args;
		for (size_t i = args.size(); i < m_args.size(); i++)
		{
			if (m_args[i].m_flags & Arg::ARG_OPTION_KEY)
			{
				for (size_t j = 0; j < args.size(); j++)
				{
					if (m_args[j].m_arg == m_args[i].m_options)
					{
						script = replace_str(script, m_args[i].m_arg, args[j]);
						break;
					}
				}
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
			printf("%s%s%s", g_vt_boldwhite, iCol->first.c_str(), g_vt_default);

			auto i = iCol;
			i++;
			if(i != end())
				printf("|");
		}
		printf(">");
	}

	void PrintAutoComplete(string match, const char *space=" " )
	{
		for (auto iCol = begin(); iCol != end(); ++iCol)
                {
			if(iCol->first.substr(0, match.size()) == match)
				printf("%s%s\n", iCol->first.c_str(), space);
		}
	}

};

extern BuildInScriptVector g_BuildScripts;
