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

#include "cmd.h"
#include <cstdint>

class SDPSCmd : public CmdBase
{
public:
	SDPSCmd(char *cmd) :CmdBase(cmd)
	{
		insert_param_info("boot", nullptr, Param::Type::e_null);
		insert_param_info("-f", &m_filename, Param::Type::e_string_filename);
		insert_param_info("-offset", &m_offset, Param::Type::e_uint32);
		insert_param_info("-skipfhdr", &m_bskipflashheader, Param::Type::e_bool);
		insert_param_info("-scanterm", &m_bscanterm, Param::Type::e_bool);
		insert_param_info("-scanlimited", &m_scan_limited, Param::Type::e_uint64);
	}
	int run(CmdCtx *p) override;

private:
	bool m_bskipflashheader=0;
	bool m_bscanterm=0;
	std::string m_filename;
	uint32_t m_offset = 0;
	uint64_t m_scan_limited = UINT64_MAX;
};
