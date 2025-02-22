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

#include "Script.h"
#include "VtEmulation.h"

#include "../libuuu/string_man.h"

#include <iostream>
#include <regex>

/**
 * @brief Parses characters between argument name and its description and checks if it's optional
 * @param option Text between argument name and description
 */
void Script::Arg::parser(const std::string& option)
{
	const auto pos = option.find('[');
	if (pos == std::string::npos)
	{
		return;
	}
	m_default_argument_value_name = option.substr(pos + 1, option.find(']') - pos - 1);
	optionality = ARG_OPTION | ARG_OPTION_KEY;
}

/**
 * @brief Constructs from a config object; caches the config and parses arguments from the text
 * @param config Configuation, name and desc can be null/empty, but text cannot
 */
Script::Script(const ScriptConfig& config) :
	m_text{config.m_text},
	m_desc{config.m_desc ? config.m_desc : ""},
	m_name{config.m_name ? config.m_name : ""}
{
	// Regular expression to detect script argument name occurrences
	static const std::regex arg_name_regexp{R"####((@| )(_\S+))####"};

	for (std::sregex_iterator it
		 = std::sregex_iterator{m_text.cbegin(), m_text.cend(), arg_name_regexp};
					   it != std::sregex_iterator{}; ++it)
	{
		const std::string arg_name{it->str(2)};
		if (!has_arg(arg_name))
		{
			Arg arg;
			arg.m_name = arg_name;
			arg.optionality = Arg::ARG_MUST;
			m_args.emplace_back(std::move(arg));
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
 * @brief Indicates whether the script has an argument specified by name
 * @param name Argument name
 */
bool Script::has_arg(const std::string &name) const
{
	return std::any_of(m_args.cbegin(), m_args.cend(),
					   [&name](const Arg &arg){ return arg.m_name == name; });
}

/**
 * @brief Replaces matching substrings plus unknown logic related to file extensions
 * @param[in,out] text Input/output string
 * @param[in] match Substring to be replaced
 * @param[in,out] replace Text to substitute for match; oddly, this is modified
 * @return Modified input string object
 */
static std::string replace_str(std::string text, const std::string& match, std::string replace)
{
	// conform replace text
	{
		std::string s5, s4;
		std::string extensions[] = { ".BZ2", ".ZST" };
		if (replace.size() > 4)
		{
			if (replace[replace.size() - 1] == '\"')
			{
				s5 = string_man::toupper(replace.substr(replace.size() - 5));
				for (std::string it : extensions)
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
				s4 = string_man::toupper(replace.substr(replace.size() - 4));
				for (std::string it : extensions)
				{
					if (it == s4)
					{
						replace += "/*";
					}
				}
			}
		}
	}

	for (size_t j = 0; (j = text.find(match, j)) != std::string::npos;)
	{
		if (j == 0 || (j != 0 && text[j - 1] == ' '))
			text.replace(j, match.size(), replace);
		j += match.size();
	}

	return text;
}

/**
 * @brief Returns a copy of the script content with argument references replaced with values
 * @param values Argument values; ordered per the arguments of the script definition
 * @return Script text after replacement
 * @details
 * Ignores values in excess of defined argument count.
 * For arguments indexed beyond the last value, if it is configured for replacement with the
 * value of another argument (ARG_OPTION_KEY), the other argument occurs _before_ the target
 * argument in the script definition, and the argument has a value in values, then argument
 * references are replaced with the value of the other argument.
 */
std::string Script::replace_arguments(const std::vector<std::string>& values) const
{
	std::string text = m_text;
	for (size_t i = 0; i < values.size() && i < m_args.size(); i++)
	{
		text = replace_str(text, m_args[i].m_name, values[i]);
	}

	// handle optional args
	for (size_t i = values.size(); i < m_args.size(); i++)
	{
		if (m_args[i].optionality & Arg::ARG_OPTION_KEY)
		{
			for (size_t j = 0; j < values.size(); j++)
			{
				if (m_args[j].m_name == m_args[i].m_default_argument_value_name)
				{
					text = replace_str(text, m_args[i].m_name, values[j]);
					break;
				}
			}
		}
	}

	return text;
}

/**
 * @brief Writes the script content to stdout
 */
void Script::print_content() const {
	std::string text = m_text;
	while (text.size() > 0 && (text[0] == '\n' || text[0] == ' '))
		text = text.erase(0, 1);
	std::cout << text;
}

/**
 * @brief Writes the name, description and arguments to stdout
 */
void Script::print_definition() const
{
	printf("\t%s%s%s\t%s\n", g_vt->boldwhite, m_name.c_str(), g_vt->default_fg,  m_desc.c_str());
	for (auto i = 0u; i < m_args.size(); ++i)
	{
		std::string desc{m_args[i].m_name};
		if (m_args[i].optionality & Arg::ARG_OPTION)
		{
			desc += g_vt->boldwhite;
			desc += "[Optional]";
			desc += g_vt->default_fg;
		}
		desc += " ";
		desc += m_args[i].m_desc;
		printf("\t\targ%u: %s\n", i, desc.c_str());
	}
}

static constexpr ScriptConfig builtin_script_configs[] =
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

//! Script catalog global instance
ScriptCatalog g_ScriptCatalog(builtin_script_configs);
