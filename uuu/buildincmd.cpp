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

#include "buildincmd.h"

using namespace std;
constexpr BuildCmd::BuildCmd(const char * const cmd, const char * const buildcmd,
	const char * const desc) :
	m_cmd{cmd},
	m_buildcmd{buildcmd},
	m_desc{desc}
{
}

static constexpr std::array<const BuildCmd, 8> g_buildin_cmd
{
	BuildCmd{
		"emmc",
#include "emmc_burn_loader.clst"
		,"burn boot loader to eMMC boot partition"
	},
	BuildCmd{
		"emmc_all",
#include "emmc_burn_all.clst"
		,"burn whole image to eMMC"
	},
	BuildCmd{
		"fat_write",
#include "fat_write.clst"
		,"update one file in fat partition, require uboot fastboot running in board"
	},
	BuildCmd{
		"nand",
#include "nand_burn_loader.clst"
		,"burn boot loader to NAND flash"
	},
	BuildCmd{
		"qspi",
#include "qspi_burn_loader.clst"
		,"burn boot loader to qspi nor flash"
	},
	BuildCmd{
		"sd",
#include "sd_burn_loader.clst"
		,"burn boot loader to sd card"
	},
	BuildCmd{
		"sd_all",
#include "sd_burn_all.clst"
		,"burn whole image to sd card"
	},
	BuildCmd{
		"spl",
#include "spl_boot.clst"
		,"boot spl and uboot"
	}
};

BuildInScriptVector g_BuildScripts(g_buildin_cmd);

int Arg::parser(string option)
{
	size_t pos;
	pos = option.find('[');
	if (pos == std::string::npos)
		return 0;
	m_options = option.substr(pos + 1, option.find(']') - pos - 1);
	m_flags = ARG_OPTION | ARG_OPTION_KEY;
	return 0;
}

BuildInScript::BuildInScript(const BuildCmd &p)
{
	m_script = p.m_buildcmd;
	if(p.m_desc)
		m_desc = p.m_desc;
	if(p.m_cmd)
		m_cmd = p.m_cmd;

	for (size_t i = 1; i < m_script.size(); i++)
	{
		size_t off;
		size_t off_tab;
		std::string param;
		if (m_script[i] == '_'
			&& (m_script[i - 1] == '@' || m_script[i - 1] == ' '))
		{
			off = m_script.find(' ', i);
			off_tab = m_script.find('\t', i);
			size_t ofn = m_script.find('\n', i);
			if (off_tab < off)
				off = off_tab;
			if (ofn < off)
				off = ofn;

			if (off == std::string::npos)
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
		std::string str;
		str += "@";
		str += m_args[i].m_arg;
		pos = m_script.find(str);
		if (pos != std::string::npos) {
			std::string def;
			size_t start_descript;
			start_descript = m_script.find('|', pos);
			if (start_descript != std::string::npos)
			{
				m_args[i].m_desc = m_script.substr(start_descript + 1,
										m_script.find('\n', start_descript) - start_descript - 1);
				def = m_script.substr(pos, start_descript - pos);
				m_args[i].parser(def);
			}
		}
	}
}

bool BuildInScript::find_args(string arg)
{
	for (size_t i = 0; i < m_args.size(); i++)
	{
		if (m_args[i].m_arg == arg)
		{
			return true;
		}
	}
	return false;
}

string BuildInScript::replace_script_args(vector<string> args)
{
	std::string script = m_script;
	for (size_t i = 0; i < args.size() && i < m_args.size(); i++)
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

string BuildInScript::replace_str(string str, string key, string replace)
{
	if (replace.size() > 4)
	{
		if (str_to_upper(replace.substr(replace.size() - 4)) == ".BZ2")
		{
			replace += "/*";
		}
	}

	for (size_t j = 0; (j = str.find(key, j)) != std::string::npos;)
	{
		str.replace(j, key.size(), replace);
		j += key.size();
	}
	return str;
}

void BuildInScript::show_cmd()
{
	printf("\t%s%s%s\t%s\n", g_vt_boldwhite, m_cmd.c_str(), g_vt_default,  m_desc.c_str());
	for (size_t i=0; i < m_args.size(); i++)
	{
		std::string desc;
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

string BuildInScript::str_to_upper(string str)
{
	std::locale loc;
	std::string s;

	for (size_t i = 0; i < str.size(); i++)
	{
		s.push_back(std::toupper(str[i], loc));
	}

	return s;
}

BuildInScriptVector::BuildInScriptVector(const array<const BuildCmd, 8> &build_cmds)
{
	for (const auto &build_cmd : build_cmds) {
		BuildInScript one{build_cmd};
		(*this)[one.get_cmd()] = one;
	}
}

void BuildInScriptVector::PrintAutoComplete(string match, const char *space)
{
	for (auto iCol = begin(); iCol != end(); ++iCol)
	{
		if(iCol->first.substr(0, match.size()) == match)
		{
			printf("%s%s\n", iCol->first.c_str(), space);
		}
	}
}

void BuildInScriptVector::ShowAll()
{
	for (auto iCol = begin(); iCol != end(); ++iCol)
	{
		iCol->second.show_cmd();
	}
}

void BuildInScriptVector::ShowCmds()
{
	printf("<");
	for (auto iCol = begin(); iCol != end(); ++iCol)
	{
		printf("%s%s%s", g_vt_boldwhite, iCol->first.c_str(), g_vt_default);

		auto i = iCol;
		i++;
		if(i != end())
		{
			printf("|");
		}
	}
	printf(">");
}
