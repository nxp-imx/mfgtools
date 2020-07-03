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

#include "gitversion.h"
#include "libuuu.h"

#include <string>

using namespace std;

static constexpr auto g_version = GIT_VERSION;

const char *uuu_get_version_string()
{
	return g_version;
}

int uuu_get_version()
{
	string version_str{g_version};

	// Find first dot because major version number must be before it
	auto pos = version_str.find(".");
	// Find the position of the character right before the start of the number
	auto vs = version_str.find_last_not_of("0123456789", pos - 1);
	// Let "vs" point exactly to the first character of the major version number
	++vs;

	string temp_num_str = version_str.substr(vs, pos - vs);
	const auto maj = static_cast<int>(stoll(temp_num_str, nullptr, 10));

	version_str = version_str.substr(pos + 1);
	pos = version_str.find(".");
	temp_num_str = version_str.substr(0, pos);
	const auto min = static_cast<int>(stoll(temp_num_str, nullptr, 10));

	version_str = version_str.substr(pos + 1);
	temp_num_str = version_str.substr(0, pos = version_str.find("-"));
	const auto build = static_cast<int>(stoll(temp_num_str, nullptr, 10));

	return (maj << 24) | (min << 12) | build;
}
