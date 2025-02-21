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

#include <cstdint>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Script definition data
 */
struct BuiltInScriptRawData final
{
	//! Script name
	const char * const m_name = nullptr;
	//! Script content
	const char * const m_text = nullptr;
	//! Script description/documentation
	const char * const m_desc = nullptr;
};

/**
 * @brief Parameterized script
 * @note
 * Is mostly for built-in scripts, but is also used for custom scripts sometimes.
 */
class BuiltInScript final
{
public:
	/**
	 * @brief Defines a formal argument to a script
	 */
	class Arg final
	{
	public:
		enum
		{
			ARG_MUST = 0x1,
			ARG_OPTION = 0x2,
			ARG_OPTION_KEY = 0x4,
		};

		void parser(const std::string &option);

		//! Argument name
		std::string m_name;
		//! Argument description/documentation
		std::string m_desc;
		//! Flags (basically if it's optional or not)
		uint32_t m_flags = ARG_MUST;
		//! Argument whose value this one defaults to if this is optional
		//! and not specified
		std::string m_fallback_option;
	};

	BuiltInScript() {};
	BuiltInScript(const BuiltInScriptRawData*p);

	std::string replace_script_args(const std::vector<std::string> &args) const;
	void show() const;
	void show_cmd() const;

	//! Script content
	const std::string m_text;
	//! Script description/documentation
	const std::string m_desc;
	//! Script name
	const std::string m_name;
	//! Arguments
	std::vector<Arg> m_args;

private:
	bool find_args(const std::string &arg) const;
};

/**
 * @brief Script catalog; indexed by name
 */
class BuiltInScriptMap final : public std::map<std::string, BuiltInScript>
{
public:
	BuiltInScriptMap(const BuiltInScriptRawData *p);
	bool add_from_file(const std::string& path);
	void print_auto_complete(const std::string &match, const char *space = " ") const;
	void print_usage() const;
	std::string get_names() const;
};

extern BuiltInScriptMap g_ScriptCatalog;
