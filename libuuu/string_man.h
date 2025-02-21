
#pragma once

#include <stdarg.h>
#include <stdio.h>

#include <string>

/**
 * @brief Formats like printf with output to std::string and minimal size allocation
 */
inline void format(std::string s, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	size_t len = std::vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	s.resize(len);

	va_start(args, fmt);
	std::vsnprintf((char*)s.c_str(), len + 1, fmt, args);
	va_end(args);
}

/**
 * @brief Replaces each occurance of a substring
 * @param text Input text
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
