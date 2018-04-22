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

#pragma once

#include <string>
#include <vector>
#include <map>
#include "liberror.h"
#include "libcomm.h"

using namespace std;

class CmdBase
{
public:
	std::string m_cmd;
	CmdBase(char *p) { m_cmd = p; }
	virtual int run()=0;
	virtual void dump() { dbg(m_cmd.c_str()); };
};

class CmdList : public std::vector<CmdBase *>
{
public:
	int run_all(bool dry_run = false);
};

class CmdMap : public std::map<std::string, CmdList *>
{
public:
	int run_all(std::string protocal, bool dry_run = false)
	{
		if (find(protocal) == end())
		{
			set_last_err_id(-1);
			std::string err;
			err.append("Uknown Protocal:");
			err.append(protocal);
			set_last_err_string(err);
			return -1;
		}
		return at(protocal)->run_all(dry_run);
	};
};

string get_next_param(string &cmd, size_t &pos);

int str_to_int(string &str);