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

#include "buildincmd.h"

using namespace std;
constexpr BuildCmd::BuildCmd(const char * const cmd, const char * const buildcmd,
	const char * const desc) :
	m_cmd{cmd},
	m_buildcmd{buildcmd},
	m_desc{desc}
{
}

static constexpr std::array<const BuildCmd, 8> g_buildin_cmd
{
	BuildCmd{
		"emmc",
#include "emmc_burn_loader.clst"
		,"burn boot loader to eMMC boot partition"
	},
	BuildCmd{
		"emmc_all",
#include "emmc_burn_all.clst"
		,"burn whole image to eMMC"
	},
	BuildCmd{
		"fat_write",
#include "fat_write.clst"
		,"update one file in fat partition, require uboot fastboot running in board"
	},
	BuildCmd{
		"nand",
#include "nand_burn_loader.clst"
		,"burn boot loader to NAND flash"
	},
	BuildCmd{
		"qspi",
#include "qspi_burn_loader.clst"
		,"burn boot loader to qspi nor flash"
	},
	BuildCmd{
		"sd",
#include "sd_burn_loader.clst"
		,"burn boot loader to sd card"
	},
	BuildCmd{
		"sd_all",
#include "sd_burn_all.clst"
		,"burn whole image to sd card"
	},
	BuildCmd{
		"spl",
#include "spl_boot.clst"
		,"boot spl and uboot"
	}
};

BuildInScriptVector g_BuildScripts(g_buildin_cmd);

BuildInScriptVector::BuildInScriptVector(const array<const BuildCmd, 8> &build_cmds)
{
	for (const auto &build_cmd : build_cmds) {
		BuildInScript one{build_cmd};
		(*this)[one.get_cmd()] = one;
	}
}

void BuildInScriptVector::PrintAutoComplete(string match, const char *space)
{
	for (auto iCol = begin(); iCol != end(); ++iCol)
	{
		if(iCol->first.substr(0, match.size()) == match)
		{
			printf("%s%s\n", iCol->first.c_str(), space);
		}
	}
}

void BuildInScriptVector::ShowAll()
{
	for (auto iCol = begin(); iCol != end(); ++iCol)
	{
		iCol->second.show_cmd();
	}
}

void BuildInScriptVector::ShowCmds()
{
	printf("<");
	for (auto iCol = begin(); iCol != end(); ++iCol)
	{
		printf("%s%s%s", g_vt_boldwhite, iCol->first.c_str(), g_vt_default);

		auto i = iCol;
		i++;
		if(i != end())
		{
			printf("|");
		}
	}
	printf(">");
}
