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

#include <fstream>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Script creation data
 */
struct ScriptConfig final
{
	//! Script name
	const char *m_name = nullptr;
	//! Script content
	const char *m_text = nullptr;
	//! Script description/documentation
	const char *m_desc = nullptr;
};

/**
 * @brief Script definition
 */
class Script final
{
	/**
	 * @brief Argument definition
	 */
	class Arg final
	{
	public:
		enum
		{
			//! Required
			ARG_MUST = 0x1,
			//! Optional; no default
			ARG_OPTION = 0x2,
			//! Optional with default from another argument
			ARG_OPTION_KEY = 0x4,
		};

		void parser(const std::string& option);

		//! Argument name
		std::string m_name;
		//! Argument description/documentation
		std::string m_desc;
		//! Optionality
		uint32_t optionality = ARG_MUST;
		//! Name of argument that has the default value if this is optional
		//! and not explicitly specified
		std::string m_default_argument_value_name;
	};

public:
	Script(const ScriptConfig&);

	std::string replace_arguments(const std::vector<std::string>& args) const;
	void print_definition() const;
	void print_content() const;

	//! Script content
	const std::string m_text;
	//! Script description/documentation
	const std::string m_desc;
	//! Script name
	const std::string m_name;
	//! Arguments
	std::vector<Arg> m_args;

private:
	bool has_arg(const std::string &arg) const;
};

/**
 * @brief Script catalog
 */
class ScriptCatalog final
{
	std::map<std::string, Script> items;

public:
	/**
	 * @brief Constructs from an array of config objects; terminated by an item with a null name
	 * @param[in] config Pointer to the first object
	 */
	ScriptCatalog(const ScriptConfig configs[])
	{
		while (configs->m_name)
		{
			items.emplace(configs->m_name, *configs);
			++configs;
		}
	}

	/**
	 * @brief Returns a pointer to the script specified by name or null if not found
	 */
	const Script* find(const std::string& name)
	{
		auto item = items.find(name);
		return item == items.end() ? nullptr : &item->second;
	}

	/**
	 * @brief Loads a file as a script; adding it to the catalog
	 * @param path File system path
	 * @return Pointer to the new script object or null if unable to load
	 */
	const Script* add_from_file(const std::string& path)
	{
		std::ifstream t(path);
		std::string fileContents((std::istreambuf_iterator<char>(t)),
			std::istreambuf_iterator<char>());

		if (fileContents.empty()) {
			return nullptr;
		}

		ScriptConfig script_definition{
			path.c_str(),
			fileContents.c_str(),
			"Script loaded from file"
		};

		auto item = items.emplace(path, script_definition);
		return &item.first->second;
	}

	/**
	 * @brief Gets a string that lists each script name separated by comma
	 */
	std::string get_names() const
	{
		std::string text;
		for (const auto& item : items)
		{
			text += item.first + ",";
		}
		text.pop_back();
		return text;
	}

	/**
	 * @brief Writes (to stdout) usage information about each script
	 */
	void print_usage() const
	{
		for (const auto& item : items)
		{
			item.second.print_definition();
		}
	}

	/**
	 * @brief Prints the name of each script that matches; for use with auto-complete
	 * @param match Search text
	 * @param space Text printed after each script name
	 */
	void print_auto_complete(const std::string& match, const char* space = " ") const
	{
		for (const auto& item : items)
		{
			if (item.first.substr(0, match.size()) == match)
			{
				printf("%s%s\n", item.first.c_str(), space);
			}
		}
	}
};

extern ScriptCatalog g_ScriptCatalog;
