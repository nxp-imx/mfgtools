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
#include <string>
#include <stdarg.h>
#include <locale>

#pragma once

using namespace std;

void call_notify(struct uuu_notify nf);

#define log printf
#define dbg printf

int get_libusb_debug_level();

class string_ex : public std::string
{
public:

	int format(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		size_t len = std::vsnprintf(NULL, 0, fmt, args);
		va_end(args);

		this->resize(len);

		va_start(args, fmt);
		std::vsnprintf((char*)c_str(), len+1, fmt, args);
		va_end(args);

		return 0;
	}
	void replace(char a, char b)
	{
		for (size_t i = 0; i < size(); i++)
			if (at(i) == a)
				(*this)[i] = b;
	}
};

class Path : public string_ex
{
public:
	string get_file_name()
	{
		replace('\\', '/');
		size_t pos;
		pos = rfind('/');
		if (pos == string::npos)
			return *this;
		return substr(pos + 1);
	}
};

inline uint64_t EndianSwap(uint64_t x) {
	return  (((x & 0x00000000000000ffLL) << 56) |
		((x & 0x000000000000ff00LL) << 40) |
		((x & 0x0000000000ff0000LL) << 24) |
		((x & 0x00000000ff000000LL) << 8) |
		((x & 0x000000ff00000000LL) >> 8) |
		((x & 0x0000ff0000000000LL) >> 24) |
		((x & 0x00ff000000000000LL) >> 40) |
		((x & 0xff00000000000000LL) >> 56));
}

inline uint32_t EndianSwap(uint32_t x)
{
	return (x >> 24) |
		((x << 8) & 0x00FF0000) |
		((x >> 8) & 0x0000FF00) |
		(x << 24);
}
inline uint16_t EndianSwap(uint16_t x)
{
	return (x >> 8) |
		((x << 8) & 0xFF00);
}

inline string str_to_upper(string str)
{
	std::locale loc;
	string s;

	for (size_t i = 0; i < str.size(); i++)
		s.push_back(std::toupper(str[i], loc));

	return s;
}

inline bool compare_str(string &str1, string &str2, bool ignore_case)
{
	if (ignore_case)
		return str_to_upper(str1) == str_to_upper(str2);
	else
		return str1 == str2;
}
