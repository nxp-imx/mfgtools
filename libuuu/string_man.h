
#pragma once

#include <stdarg.h>
#include <stdio.h>

#include <algorithm> 
#include <cctype>
#include <locale>
#include <string>

namespace string_man {

	/**
	 * @brief Formats like printf with output to std::string and minimal size allocation
	 * @param[out] text Output text
	 */
	inline void format(std::string& text, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		size_t len = std::vsnprintf(NULL, 0, fmt, args);
		va_end(args);

		text.resize(len);

		va_start(args, fmt);
		std::vsnprintf((char*)text.c_str(), len + 1, fmt, args);
		va_end(args);
	}

	/**
	 * @brief Replaces each occurance of a substring
	 * @param[in,out] text Input/output text
	 * @param from Substring to replace
	 * @param to Text to replace found substring with
	 * @return Reference to text (supports chaining)
	 */
	inline std::string& replace(std::string& text, const std::string& from, const std::string& to) {
		if (!from.empty())
		{
			size_t start_pos = 0;
			while ((start_pos = text.find(from, start_pos)) != std::string::npos) {
				text.replace(start_pos, from.length(), to);
				start_pos += to.length();
			}
		}
		return text;
	}

	/**
	* @brief Replaces each lowercase letter with uppercase
	* @param[in,out] text Input/output text
	* @return Uppercase text
	* @return Reference to text (supports chaining)
	*/
	inline std::string& uppercase(std::string& text)
	{
		const std::locale loc;
		for (size_t i = 0; i < text.size(); ++i)
		{
			text.push_back(std::toupper(text[i], loc));
		}
		return text;
	}

	/**
	* @brief Returns a copy of the input with lowercase letters replaced with uppercase
	* @param[in] text Input text
	* @return Uppercase text
	* @return Result text
	*/
	inline std::string uppercase_copy(const std::string& text)
	{
		const std::locale loc;
		std::string upper;
		upper.reserve(text.size());

		for (size_t i = 0; i < text.size(); ++i)
		{
			upper.push_back(std::toupper(text[i], loc));
		}

		return upper;
	}

	/**
	 * @brief Removes whitespace from the beginning 
	 * @param[in,out] text Input/output text
	 * @return Reference to text (supports chaining)
	 */
	inline std::string& left_trim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
		return s;
	}

	/**
	 * @brief Removes whitespace from the end
	 * @param[in,out] text Input/output text
	 * @return Reference to text (supports chaining)
	 */
	inline std::string& right_trim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());
		return s;
	}

	/**
	 * @brief Removes whitespace from both ends
	 * @param[in,out] text Input/output text
	 * @return Reference to text (supports chaining)
	 */
	inline std::string& trim(std::string& s) {
		right_trim(s);
		left_trim(s);
		return s;
	}

	/**
	 * @brief Returns the input string but with whitespace removed from the beginning
	 * @param[in] text Input text
	 * @return Result text
	 */
	inline std::string left_trim_copy(const std::string& s) {
		std::string copy(s);
		left_trim(copy);
		return copy;
	}

	/**
	 * @brief Returns the input string but with whitespace removed from the end
	 * @param[in] text Input text
	 * @return Result text
	 */
	inline std::string right_trim_copy(const std::string& s) {
		std::string copy(s);
		right_trim(copy);
		return copy;
	}

	/**
	 * @brief Returns the input string but with whitespace removed from both ends
	 * @param[in] text Input text
	 * @return Result text
	 */
	inline std::string trim_copy(const std::string& s) {
		std::string copy(s);
		trim(copy);
		return copy;
	}
}