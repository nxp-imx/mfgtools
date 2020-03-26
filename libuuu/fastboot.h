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

/*
Android fastboot protocol define at
https://android.googlesource.com/platform/system/core/+/master/fastboot/
*/

#include <string>
#include <vector>

#include "trans.h"
#include "cmd.h"
#include "buffer.h"

using namespace std;

class FastBoot
{
	TransBase *const m_pTrans = nullptr;
public:
	string m_info;

	FastBoot(TransBase *p) : m_pTrans{p} {}

	int Transport(string cmd, void *p = nullptr, size_t size = 0, vector<uint8_t> *input = nullptr);
	int Transport(string cmd, vector<uint8_t> data, vector<uint8_t> *input = nullptr) { return Transport(cmd, data.data(), data.size(), input); }
};

class FBGetVar : public CmdBase
{
public:
	string m_var;
	string m_val;
	FBGetVar(char *p) :CmdBase(p) {}
	int parser(char *p = nullptr) override;
	int run(CmdCtx *ctx) override;
};

class FBCmd: public CmdBase
{
public:
	string m_fb_cmd;
	string m_uboot_cmd;
	char m_separator;
	FBCmd(char *p) :CmdBase(p), m_separator(':') {}
	int parser(char *p = nullptr) override;
	int run(CmdCtx *ctx) override;
};

class FBUCmd : public FBCmd
{
public:
	FBUCmd(char *p) :FBCmd(p) { m_fb_cmd = "UCmd"; }
};

class FBACmd : public FBCmd
{
public:
	FBACmd(char *p) :FBCmd(p) { m_fb_cmd = "ACmd"; }
};

class FBSyncCmd: public FBCmd
{
public:
	FBSyncCmd(char *p) : FBCmd(p) { m_fb_cmd = "Sync"; }
};

class FBFlashingCmd : public FBCmd
{
public:
	FBFlashingCmd(char *p) : FBCmd(p) { m_fb_cmd = "flashing"; }
};

class FBOemCmd : public FBCmd
{
public:
	FBOemCmd(char *p) : FBCmd(p) { m_fb_cmd = "oem"; m_separator = ' ';}
};

class FBFlashCmd : public FBCmd
{
public:
	string m_filename;
	string m_partition;
	uint64_t m_totalsize;
	bool m_raw2sparse = false;
	FBFlashCmd(char *p) : FBCmd(p) { m_timeout = 10000; m_fb_cmd = "flash"; }
	int parser(char *p = nullptr) override;
	int run(CmdCtx *ctx) override;
	int flash(FastBoot *fb, void *p, size_t sz);
	int flash_raw2sparse(FastBoot *fb, shared_ptr<FileBuffer> p, size_t blksz, size_t max);
	bool isffu(shared_ptr<FileBuffer> p);
	int flash_ffu(FastBoot *fb, shared_ptr<FileBuffer> p);
	int flash_ffu_oneblk(FastBoot *fb, shared_ptr<FileBuffer> p, size_t off, size_t blksz, size_t blkindex);
};

class FBDelPartition : public FBCmd
{
public:
	FBDelPartition(char*p) : FBCmd(p) { m_fb_cmd = "delete-logical-partition"; }
};

class FBPartNumber : public CmdBase
{
public:
	string m_partition_name;
	string m_fb_cmd;
	uint32_t m_Size;

	FBPartNumber(char *p) :CmdBase(p)
	{
		m_Size = 0;
		m_bCheckTotalParam = true;
		m_NoKeyParam = true;
		insert_param_info(nullptr, &m_partition_name, Param::Type::e_string, false, "partition name");
		insert_param_info(nullptr, &m_Size, Param::Type::e_uint32, false, "partition size");
	}
	int run(CmdCtx *ctx) override;
};

class FBCreatePartition : public FBPartNumber
{
public:
	FBCreatePartition(char*p) :FBPartNumber(p) {
		m_fb_cmd = "create-logical-partition";
	}
};

class FBResizePartition : public FBPartNumber
{
public:
	FBResizePartition(char*p) :FBPartNumber(p) {
		m_fb_cmd = "resize-logical-partition";
	}
};

class FBUpdateSuper : public CmdBase
{
public:
	string m_partition_name;
	const string m_fb_cmd = "update-super";
	string m_opt;

	FBUpdateSuper(char *p) :CmdBase(p)
	{
		m_bCheckTotalParam = true;
		m_NoKeyParam = true;
		insert_param_info(nullptr, &m_partition_name, Param::Type::e_string, false, "partition name");
		insert_param_info(nullptr, &m_opt, Param::Type::e_string, false, "partition size");
	}
	int run(CmdCtx *ctx) override;
};

class FBEraseCmd : public FBCmd
{
public:
	FBEraseCmd(char *p) : FBCmd(p) { m_fb_cmd = "erase"; }
};

class FBSetActiveCmd : public FBCmd
{
public:
	FBSetActiveCmd(char *p) : FBCmd(p) { m_fb_cmd = "set_active"; }
};

class FBDownload : public CmdBase
{
public:
	string m_filename;
	FBDownload(char *p) :CmdBase(p)
	{
		insert_param_info("download", nullptr, Param::Type::e_null);
		insert_param_info("-f", &m_filename, Param::Type::e_string_filename);
	}
	int run(CmdCtx *ctx) override;
};

class FBCopy : public CmdBase
{
public:
	string m_local_file;
	string m_target_file;
	bool m_bDownload;
	size_t m_Maxsize_pre_cmd = 0x10000;
	FBCopy(char *p) :CmdBase(p) {}
	int parser(char *p = nullptr) override;
	int run(CmdCtx *ctx) override;
};

class FBContinueCmd : public FBCmd
{
public:
	FBContinueCmd(char *p) : FBCmd(p) { m_fb_cmd = "continue"; }
};
