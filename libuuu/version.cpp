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
#include <string.h>

using namespace std;

static const char g_version[] = GIT_VERSION;

const char *uuu_get_version_string()
{
	return g_version;
}

int uuu_get_version()
{
	string str = g_version;
	int maj, min, build;

	size_t pos = 0;
	pos = str.find(".", pos);
	size_t vs = str.find_last_not_of("0123456789", pos - 1);
	vs++;

	string s = str.substr(vs, pos - vs);

	maj = stoll(s, 0, 10);

	str = str.substr(pos + 1);
	pos = str.find(".");
	s = str.substr(0, pos);

	min = stoll(s, 0, 10);

	str = str.substr(pos + 1);
	s = str.substr(0, pos = str.find("-"));

	build = stoll(s, 0, 10);

	return (maj << 16) | (min << 8) | build;
}
