/*
* Copyright 2018-2021 NXP.
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

#include <algorithm>
#include <locale>
#include <regex>

static std::string replace_str(std::string str, std::string key, std::string replace);
static std::string str_to_upper(const std::string &str);

/**
 * @brief Parse characters between argument name and its description and check
 * if its an optional one
 * @param[in] option The characters between argument name and its description to
 * be parsed
 * @return `0` in any case
 */
void BuiltInScript::Arg::parser(const std::string &option)
{
	const auto pos = option.find('[');
	if (pos == std::string::npos)
	{
		return;
	}
	m_fallback_option = option.substr(pos + 1, option.find(']') - pos - 1);
	m_flags = ARG_OPTION | ARG_OPTION_KEY;
}

/**
 * @brief Create a new BuiltInScript instance by extracting information from a
 * BuiltInScriptRawData instance
 * @param[in] p The BuiltInScriptRawData containing all data of the script this
 * BuiltInScript instance shall represent
 */
BuiltInScript::BuiltInScript(const BuiltInScriptRawData * const p) :
	m_text{p->m_text},
	m_desc{p->m_desc ? p->m_desc : ""},
	m_name{p->m_name ? p->m_name : ""}
{
	// Regular expression to detect script argument name occurrences
	static const std::regex arg_name_regexp{R"####((@| )(_\S+))####"};

	for (std::sregex_iterator it
		 = std::sregex_iterator{m_text.cbegin(), m_text.cend(), arg_name_regexp};
					   it != std::sregex_iterator{}; ++it)
	{
		const std::string param{it->str(2)};
		if (!find_args(param))
		{
			Arg a;
			a.m_name = param;
			a.m_flags = Arg::ARG_MUST;
			m_args.emplace_back(std::move(a));
		}
	}

	for (size_t i = 0; i < m_args.size(); i++)
	{
		std::string str;
		str += "@";
		str += m_args[i].m_name;
		const auto pos = m_text.find(str);
		if (pos != std::string::npos) {
			const auto start_descript = m_text.find('|', pos);
			if (start_descript != std::string::npos)
			{
				m_args[i].m_desc = m_text.substr(start_descript + 1,
										m_text.find('\n', start_descript) - start_descript - 1);
				const std::string def{m_text.substr(pos, start_descript - pos)};
				m_args[i].parser(def);
			}
		}
	}
}

/**
 * @brief Check if the BuiltInScript instance has an argument called `arg`
 * @param[in] arg The argument for which its existence in the BuiltInScript
 * shall be checked
 * @return `true` if BuiltInScript has an argument named `arg`, `false`
 * otherwise
 */
bool BuiltInScript::find_args(const std::string &arg) const
{
	return std::any_of(m_args.cbegin(), m_args.cend(),
					   [&arg](const Arg &brg){ return brg.m_name == arg; });
}

/**
 * @brief Replace built-in script's arguments by actual values given in `args`
 * @param[in] args The actual values that shall replace the arguments (the order
 * must fit the order of the arguments in the script)
 * @return A copy of the built-in script with the arguments replaced by their
 * actual values
 */
std::string BuiltInScript::replace_script_args(const std::vector<std::string> &args) const
{
	std::string script = m_text;
	for (size_t i = 0; i < args.size() && i < m_args.size(); i++)
	{
		script = replace_str(script, m_args[i].m_name, args[i]);
	}

	//handle option args;
	for (size_t i = args.size(); i < m_args.size(); i++)
	{
		if (m_args[i].m_flags & Arg::ARG_OPTION_KEY)
		{
			for (size_t j = 0; j < args.size(); j++)
			{
				if (m_args[j].m_name == m_args[i].m_fallback_option)
				{
					script = replace_str(script, m_args[i].m_name, args[j]);
					break;
				}
			}
		}
	}
	return script;
}

/**
 * @brief Print the built-in script to `stdout` followed by a newline
 */
void BuiltInScript::show() const
{
	printf("%s\n", m_text.c_str());
}

/**
 * @brief Print the script's name, its description and its arguments to stdout
 */
void BuiltInScript::show_cmd() const
{
	printf("\t%s%s%s\t%s\n", g_vt_boldwhite, m_name.c_str(), g_vt_default,  m_desc.c_str());
	for (auto i = 0u; i < m_args.size(); ++i)
	{
		std::string desc{m_args[i].m_name};
		if (m_args[i].m_flags & Arg::ARG_OPTION)
		{
			desc += g_vt_boldwhite;
			desc += "[Optional]";
			desc += g_vt_default;
		}
		desc += " ";
		desc += m_args[i].m_desc;
		printf("\t\targ%u: %s\n", i, desc.c_str());
	}
}

/**
 * @brief Create a new map by parsing an array of BuiltInScriptRawData instances
 * @param[in] p Pointer to the first element of a BuiltInScriptRawData array
 */
BuiltInScriptMap::BuiltInScriptMap(const BuiltInScriptRawData*p)
{
	while (p->m_name)
	{
		emplace(p->m_name, p);
		++p;
	}
}

/**
 * @brief Auto-complete names of built-in scripts if they match `match`
 * @param[in] match The string against which the scripts' names will be matched
 * @param[in] space A separator character which shall be printed after the
 * completed script name
 */
void BuiltInScriptMap::PrintAutoComplete(const std::string &match, const char *space) const
{
	for (const auto &script_pair : *this)
	{
		if(script_pair.first.substr(0, match.size()) == match)
		{
			printf("%s%s\n", script_pair.first.c_str(), space);
		}
	}
}

/**
 * @brief Print information about all contained scripts to stdout
 */
void BuiltInScriptMap::ShowAll() const
{
	for (const auto &script_pair : *this)
	{
		script_pair.second.show_cmd();
	}
}

/**
 * @brief Print the names of all contained scripts to the given stream
 * @param[in] file The stream to which the names shall be printed
 */
void BuiltInScriptMap::ShowCmds(FILE * const file) const
{
	fprintf(file, "<");
	for (auto iCol = begin(); iCol != end(); ++iCol)
	{
		fprintf(file, "%s", iCol->first.c_str());

		auto i = iCol;
		i++;
		if(i != end())
		{
			fprintf(file, "|");
		}
	}
	fprintf(file, ">");
}

/**
 * @brief Replace a `key` substring of a string `str` by a replacement `replace`
 * @param[in] str The string of which a copy with the replacements shall be
 * created
 * @param[in] key The string which shall be replaced
 * @param[in] replace The string that shall replace occurrences of `key`
 * @return A new string instance with the replacements conducted on it
 */
static std::string replace_str(std::string str, std::string key, std::string replace)
{
	std::string s5, s4;
	std::string match[] = { ".BZ2", ".ZST" };
	if (replace.size() > 4)
	{
		if (replace[replace.size() - 1] == '\"')
		{
			s5 = str_to_upper(replace.substr(replace.size() - 5));
			for (std::string it : match)
			{
				if (s5 == it)
				{
					replace = replace.substr(0, replace.size() - 1);
					replace += "/*\"";
				}
			}

		}
		else
		{
			s4 = str_to_upper(replace.substr(replace.size() - 4));
			for (std::string it : match)
			{
				if (it == s4)
				{
					replace += "/*";
				}
			}
		}
	}

	for (size_t j = 0; (j = str.find(key, j)) != std::string::npos;)
	{
		if (j == 0 || (j!=0 && str[j - 1] == ' '))
			str.replace(j, key.size(), replace);
		j += key.size();
	}
	return str;
}

/**
 * @brief Returns a copy of `str` with all applicable characters converted to
 * uppercase
 * @param[in] str The string for which an uppercase copy shall be created
 * @return The copy of `str` converted to uppercase
 */
static std::string str_to_upper(const std::string &str)
{
	const std::locale loc;
	std::string s;
	s.reserve(str.size());

	for (size_t i = 0; i < str.size(); ++i)
	{
		s.push_back(std::toupper(str[i], loc));
	}

	return s;
}

//! Array containing raw information about all the built-in scripts of uuu
static constexpr BuiltInScriptRawData g_builtin_cmd[] =
{
	{
		"emmc",
#include "emmc_burn_loader.clst"
		,"burn boot loader to eMMC boot partition"
	},
	{
		"emmc_all",
#include "emmc_burn_all.clst"
		,"burn whole image to eMMC"
	},
	{
		"fat_write",
#include "fat_write.clst"
		,"update one file in fat partition, require uboot fastboot running in board"
	},
	{
		"nand",
#include "nand_burn_loader.clst"
		,"burn boot loader to NAND flash"
	},
	{
		"qspi",
#include "qspi_burn_loader.clst"
		,"burn boot loader to qspi nor flash"
	},
	{
		"spi_nand",
#include "fspinand_burn_loader.clst"
		,"burn boot loader to spi nand flash"
	},
	{
		"sd",
#include "sd_burn_loader.clst"
		,"burn boot loader to sd card"
	},
	{
		"sd_all",
#include "sd_burn_all.clst"
		,"burn whole image to sd card"
	},
	{
		"spl",
#include "spl_boot.clst"
		,"boot spl and uboot"
	},
	{
		"nvme_all",
#include "nvme_burn_all.clst"
		,"burn whole image io nvme storage"
	},
	{
		nullptr,
		nullptr,
		nullptr,
	}
};

//! A map of the built-in scripts' names to their BuiltInScript representations
BuiltInScriptMap g_BuildScripts(g_builtin_cmd);
