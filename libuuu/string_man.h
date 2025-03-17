
#pragma once
//! @file

#include <stdarg.h>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief String manipulation functions
 * @details
 * Functions modify the input string and to support chaining return its reference.
 * Chaining example: uppercase(trim(text)).
 */
namespace string_man {

	/**
	 * @brief Formats like printf with output to std::string and automatic and minimal size allocation
	 * @param[out] text Output text; input value is ignored
	 * @return Reference to text
	 */
	inline std::string& format(std::string& text, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		size_t len = std::vsnprintf(NULL, 0, fmt, args);
		va_end(args);

		text.resize(len);

		va_start(args, fmt);
		std::vsnprintf((char*)text.c_str(), len + 1, fmt, args);
		va_end(args);

		return text;
	}

	/**
	 * @brief Replaces each occurance of a substring
	 * @param[in,out] text Input/output text
	 * @param from Substring to replace
	 * @param to Text to replace found substring with
	 * @return Reference to text
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
	* @brief Replaces each uppercase letter with lowercase
	* @param[in,out] text Input/output text
	* @return Reference to text
	*/
	inline std::string& lowercase(std::string& text)
	{
		const std::locale loc;
		size_t length = text.size();
		for (size_t i = 0; i < length; ++i)
		{
			text[i] = std::tolower(text[i], loc);
		}
		return text;
	}

	/**
	* @brief Replaces each lowercase letter with uppercase
	* @param[in,out] text Input/output text
	* @return Reference to text
	*/
	inline std::string& uppercase(std::string& text)
	{
		const std::locale loc;
		size_t length = text.size();
		for (size_t i = 0; i < length; ++i)
		{
			text[i] = std::toupper(text[i], loc);
		}
		return text;
	}

	/**
	 * @brief Removes whitespace from the beginning 
	 * @param[in,out] text Input/output text
	 * @return Reference to text
	 */
	inline std::string& left_trim(std::string& text) {
		text.erase(text.begin(), std::find_if(text.begin(), text.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
		return text;
	}

	/**
	 * @brief Removes whitespace from the end
	 * @param[in,out] text Input/output text
	 * @return Reference to text
	 */
	inline std::string& right_trim(std::string& text) {
		text.erase(std::find_if(text.rbegin(), text.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), text.end());
		return text;
	}

	/**
	 * @brief Removes whitespace from both ends
	 * @param[in,out] text Input/output text
	 * @return Reference to text
	 */
	inline std::string& trim(std::string& text) {
		right_trim(text);
		left_trim(text);
		return text;
	}

	/**
	 * @brief Wraps text with double quotes if it contains a space
	 * @param[in,out] text Input/output text
	 * @return Reference to text
	 */
	inline std::string& quote_param_if_needed(std::string& text)
	{
		if (text.find(' ') != std::string::npos)
		{
			text.insert(text.begin(), '"');
			text.insert(text.end(), '"');
		}
		return text;
	}

	/**
	 * @brief Returns the uppercase hex representation of a number with minimum width; left-padded with zeros
	 * @param value number
	 * @return Text
	 */
	template <typename T>
	inline std::string to_hex(T value, std::streamsize width) {
		std::ostringstream ss;
		ss << std::setfill('0') << std::setw(width) << std::right << std::uppercase << std::hex << value;
		return ss.str();
	}

	/**
	 * @brief Returns a string that contains the 1st item of each vector item (a tuple) separated by pipe (|)
	 */
	template <typename T>
	std::string join(const std::vector<T>& items, const std::string& delimiter)
	{
		if (items.empty()) return "";
		std::ostringstream ss;
		for (auto& item : items)
		{
			ss << item << delimiter;
		}
		std::string text = ss.str();
		text.erase(text.size() - delimiter.size());
		return text;
	}

	/**
	 * @brief Returns a string that contains the 1st item of each vector item (a tuple) separated by pipe (|)
	 */
	template <typename T>
	std::string join_keys(const std::vector<T>& items)
	{
		if (items.empty()) return "";
		const std::string& delimiter = "|";
		std::ostringstream ss;
		for (auto& item : items)
		{
			std::string s = std::get<0>(item);
			ss << s << delimiter;
		}
		std::string text = ss.str();
		text.erase(text.size() - delimiter.size());
		return text;
	}
}