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
#pragma once
//! @file

#include "Logger.h"

#include "../libuuu/string_man.h"

#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

/**
 * @brief Script creation data
 */
struct ScriptConfig final
{
	//! Script name
	const char *name = nullptr;
	//! Script content
	const char *text = nullptr;
	//! Script description/documentation
	const char *desc = nullptr;
};

/**
 * @brief Script argument definition
 */
class ScriptArg final
{
public:
	const std::string name;

	//! Argument description/documentation
	const std::string desc;

	//! Name of argument that has the default value if this is optional
	//! and not explicitly specified; blank for no default value
	const std::string default_value_arg_name;

	ScriptArg(std::string name, std::string desc, std::string default_value_arg_name = "")
		:
		name(name), desc(desc), default_value_arg_name(default_value_arg_name)
	{
	}
};

/**
 * @brief Script definition
 */
class Script final
{
	/**
	 * @brief Indicates whether the script has an argument specified by name
	 * @param name Argument name
	 */
	bool has_arg(const std::string& name) const
	{
		return std::any_of(args.cbegin(), args.cend(),
			[&name](const ScriptArg& arg) { return arg.name == name; });
	}

	void parse_arguments() {
		static const std::regex arg_name_regexp{ R"####((@| )(_\S+))####" };
		for (std::sregex_iterator i = std::sregex_iterator{ text.cbegin(), text.cend(), arg_name_regexp };
			i != std::sregex_iterator{}; ++i)
		{
			const std::string arg_name{ i->str(2) };
			if (!has_arg(arg_name))
			{
				std::string desc;
				std::string default_argument_value_name;
				const std::string name_spec = "@" + arg_name;
				const auto name_spec_start = text.find(name_spec);
				if (name_spec_start != std::string::npos) {
					const auto desc_delim_start = text.find('|', name_spec_start);
					if (desc_delim_start != std::string::npos)
					{
						desc = text.substr(
							desc_delim_start + 1,
							text.find('\n', desc_delim_start) - desc_delim_start - 1);
						string_man::trim(desc);
						const std::string middle_part{ text.substr(name_spec_start, desc_delim_start - name_spec_start) };
						const auto pos = middle_part.find('[');
						if (pos != std::string::npos)
						{
							default_argument_value_name = middle_part.substr(pos + 1, middle_part.find(']') - pos - 1);
						}
					}
				}

				ScriptArg arg(arg_name, desc, default_argument_value_name);
				args.emplace_back(std::move(arg));
			}
		}
	}

	/**
	 * @brief Replaces matching sub-strings plus unknown logic related to file extensions
	 * @param[in,out] text Input/output string
	 * @param[in] arg_name Substring to be replaced
	 * @param[in,out] arg_value Text to substitute for match; oddly, this is modified
	 */
	static void replace_arg(std::string& text, const std::string& arg_name, std::string arg_value)
	{
		// conform replace text
		{
			std::string extensions[] = { ".BZ2", ".ZST" };
			if (arg_value.size() > 4)
			{
				if (arg_value[arg_value.size() - 1] == '\"')
				{
					std::string s5 = arg_value.substr(arg_value.size() - 5);
					string_man::uppercase(s5);
					for (auto& ext : extensions)
					{
						if (s5 == ext)
						{
							arg_value = arg_value.substr(0, arg_value.size() - 1);
							arg_value += "/*\"";
						}
					}
				}
				else
				{
					std::string s4 = arg_value.substr(arg_value.size() - 4);
					string_man::uppercase(s4);
					for (auto& ext : extensions)
					{
						if (ext == s4)
						{
							arg_value += "/*";
						}
					}
				}
			}
		}

		if (arg_value.find(' ') != std::string::npos)
		{
			arg_value.insert(arg_value.begin(), '"');
			arg_value.insert(arg_value.end(), '"');
		}

		g_logger.log_verbose("Replacing script parameter '" + arg_name + "' with value '" + arg_value + "'");

		for (size_t j = 0; (j = text.find(arg_name, j)) != std::string::npos;)
		{
			if (j == 0 || (j != 0 && text[j - 1] == ' '))
				text.replace(j, arg_name.size(), arg_value);
			j += arg_name.size();
		}
	}

public:
	/**
	 * @brief Constructs from a config object; caches the config and parses argument definitions from the text
	 * @param config Configuation; name and desc can be null/empty, but text cannot
	 */
	Script(const ScriptConfig& config) :
		name{ config.name ? config.name : "" },
		desc{ config.desc ? config.desc : "" },
		text{ config.text }
	{
		parse_arguments();
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
	std::string replace_arguments(const std::vector<std::string>& values) const
	{
		std::string text = this->text;
		for (size_t i = 0; i < values.size() && i < args.size(); i++)
		{
			const std::string before_text = text;
			const std::string arg_name = args[i].name;
			const std::string arg_value = values[i];
			replace_arg(text, arg_name, arg_value);
			if (text == before_text)
			{
				g_logger.log_warning("Argument '" + arg_name + "' not found/replaced in the script text");
			}
		}

		// handle optional args
		for (size_t i = values.size(); i < args.size(); i++)
		{
			auto& arg = args[i];
			if (!arg.default_value_arg_name.empty())
			{
				for (size_t j = 0; j < values.size(); j++)
				{
					if (args[j].name == arg.default_value_arg_name)
					{
						replace_arg(text, arg.name, values[j]);
						break;
					}
				}
			}
		}

		return text;
	}

	const std::string name;

	//! Description/documentation
	const std::string desc;

	//! Content
	const std::string text;

	std::vector<ScriptArg> args;
};
