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
 * @brief Script parameter definition
 */
class ScriptParam final
{
public:
	//! Name
	const std::string name;

	//! Description/documentation
	const std::string desc;

	//! Name of parameter that has the default value if this is optional
	//! and not explicitly specified; blank for no default source
	const std::string default_source_param_name;

	ScriptParam(std::string name, std::string desc, std::string default_source_param_name = "")
		:
		name(name), desc(desc), default_source_param_name(default_source_param_name)
	{
	}
};

/**
 * @brief Script definition
 */
class Script final
{
	/**
	 * @brief Indicates whether the script has a parameter specified by name
	 * @param name Parameter name
	 */
	bool has_param(const std::string& name) const
	{
		return std::any_of(formal_params.cbegin(), formal_params.cend(),
			[&name](const ScriptParam& param) { return param.name == name; });
	}

	void parse_parameters() {
		static const std::regex param_name_regexp{ R"####((@| )(_\S+))####" };
		for (std::sregex_iterator i = std::sregex_iterator{ text.cbegin(), text.cend(), param_name_regexp };
			i != std::sregex_iterator{}; ++i)
		{
			const std::string param_name{ i->str(2) };
			if (!has_param(param_name))
			{
				std::string desc;
				std::string default_source;
				const std::string name_spec = "@" + param_name;
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
							default_source = middle_part.substr(pos + 1, middle_part.find(']') - pos - 1);
						}
					}
				}

				ScriptParam param(param_name, desc, default_source);
				formal_params.emplace_back(std::move(param));
			}
		}
	}

	/**
	 * @brief Replaces matching sub-strings plus unknown logic related to file extensions
	 * @param[in,out] text Input/output string
	 * @param param_name Substring to be replaced
	 * @param param_value Text to substitute for match; passed by-copy since local is modified
	 */
	static void replace_param(std::string& text, const std::string& param_name, std::string param_value)
	{
		// conform replace text
		{
			std::string extensions[] = { ".BZ2", ".ZST" };
			if (param_value.size() > 4)
			{
				if (param_value[param_value.size() - 1] == '\"')
				{
					std::string s5 = param_value.substr(param_value.size() - 5);
					string_man::uppercase(s5);
					for (auto& ext : extensions)
					{
						if (s5 == ext)
						{
							param_value = param_value.substr(0, param_value.size() - 1);
							param_value += "/*\"";
						}
					}
				}
				else
				{
					std::string s4 = param_value.substr(param_value.size() - 4);
					string_man::uppercase(s4);
					for (auto& ext : extensions)
					{
						if (ext == s4)
						{
							param_value += "/*";
						}
					}
				}
			}
		}

		// wrap in quotes if contains space
		if (param_value.find(' ') != std::string::npos)
		{
			param_value.insert(param_value.begin(), '"');
			param_value.insert(param_value.end(), '"');
		}

		g_logger.log_debug([&]() { return "Replacing script parameter '" + param_name + "' with value '" + param_value + "'"; });

		for (size_t j = 0; (j = text.find(param_name, j)) != std::string::npos;)
		{
			if (j == 0 || (j != 0 && text[j - 1] == ' '))
				text.replace(j, param_name.size(), param_value);
			j += param_name.size();
		}
	}

public:
	/**
	 * @brief Constructs from a config object; caches the config and parses parameter definitions from the text
	 * @param config Configuation; name and desc can be null/empty, but text cannot
	 */
	Script(const ScriptConfig& config) :
		name{ config.name ? config.name : "" },
		desc{ config.desc ? config.desc : "" },
		text{ config.text }
	{
		parse_parameters();
	}

	/**
	 * @brief Returns a copy of the script content with parameter references replaced with values
	 * @param values Parameter values; ordered per the script definition
	 * @return Script text after replacement
	 * @details
	 * Ignores values in excess of defined parameter count.
	 * For parameters indexed beyond the last value, if it is configured for replacement with the
	 * value of another parameter, the other parameter occurs _before_ the target parameter in the
	 * script definition, and the parameter has a value in values, then parameter references are
	 * replaced with the value of the other parameter.
	 */
	std::string replace_parameters(const std::vector<std::string>& values) const
	{
		std::string text = this->text;
		for (size_t i = 0; i < values.size() && i < formal_params.size(); i++)
		{
			const std::string before_text = text;
			const std::string param_name = formal_params[i].name;
			const std::string value = values[i];
			replace_param(text, param_name, value);
			if (text == before_text)
			{
				g_logger.log_warning("Parameter '" + param_name + "' not found/replaced in the script text");
			}
		}

		// handle optional params
		for (size_t i = values.size(); i < formal_params.size(); i++)
		{
			auto& formal_param = formal_params[i];
			if (!formal_param.default_source_param_name.empty())
			{
				for (size_t j = 0; j < values.size(); j++)
				{
					if (formal_params[j].name == formal_param.default_source_param_name)
					{
						replace_param(text, formal_param.name, values[j]);
						break;
					}
				}
			}
		}

		return text;
	}

	//! @brief Name
	const std::string name;

	//! @brief Description/documentation
	const std::string desc;

	//! @brief Content
	const std::string text;

	//! @brief Defines input parameters
	std::vector<ScriptParam> formal_params;
};
