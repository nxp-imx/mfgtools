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

#include "cmd.h"

#include <cstdint>

class FBFlashCmd;
class FileBuffer;
class TransBase;

/*
Android fastboot protocol define at
https://android.googlesource.com/platform/system/core/+/master/fastboot/
*/

class FastBoot
{
public:
	FastBoot(TransBase *p) : m_pTrans{p} {}

	int Transport(std::string cmd, void *p = nullptr, size_t size = 0, std::vector<uint8_t> *input = nullptr);
	int Transport(std::string cmd, std::vector<uint8_t> data, std::vector<uint8_t> *input = nullptr) { return Transport(cmd, data.data(), data.size(), input); }

	std::string m_info;

private:
	TransBase *const m_pTrans = nullptr;
};

class FBGetVar : public CmdBase
{
public:
	FBGetVar(char *p) :CmdBase(p) {}

	int parser(char *p = nullptr) override;
	int run(CmdCtx *ctx) override;

private:
	std::string m_val;
	std::string m_var;

	friend FBFlashCmd;
};

class FBCmd: public CmdBase
{
public:
	FBCmd(char *p, std::string &&fb_cmd, char separator =':') :
		CmdBase(p), m_fb_cmd{std::move(fb_cmd)}, m_separator(separator) {}

	int parser(char *p = nullptr) override;
	int run(CmdCtx *ctx) override;

protected:
	std::string m_uboot_cmd;

private:
	const std::string m_fb_cmd;
	const char m_separator = ':';
};

class FBUCmd : public FBCmd
{
public:
	FBUCmd(char *p) :FBCmd(p, "UCmd") {}
};

class FBACmd : public FBCmd
{
public:
	FBACmd(char *p) :FBCmd(p, "ACmd") {}
};

class FBSyncCmd: public FBCmd
{
public:
	FBSyncCmd(char *p) : FBCmd(p, "Sync") {}
};

class FBFlashingCmd : public FBCmd
{
public:
	FBFlashingCmd(char *p) : FBCmd(p, "flashing") {}
};

class FBOemCmd : public FBCmd
{
public:
	FBOemCmd(char *p) : FBCmd(p, "oem", ' ') {}
};

class FBFlashCmd : public FBCmd
{
public:
	FBFlashCmd(char *p) : FBCmd(p, "flash") { m_timeout = 10000; }
	int parser(char *p = nullptr) override;
	int run(CmdCtx *ctx) override;
	int flash(FastBoot *fb, void *p, size_t sz);
	int flash_raw2sparse(FastBoot *fb, std::shared_ptr<FileBuffer> p, size_t blksz, size_t max);
	bool isffu(std::shared_ptr<FileBuffer> p);
	int flash_ffu(FastBoot *fb, std::shared_ptr<FileBuffer> p);
	int flash_ffu_oneblk(FastBoot *fb, std::shared_ptr<FileBuffer> p, size_t off, size_t blksz, size_t blkindex);

private:
	std::string m_filename;
	std::string m_partition;
	bool m_raw2sparse = false;
	size_t m_sparse_limit = 0x1000000;
	uint64_t m_totalsize;
};

class FBDelPartition : public FBCmd
{
public:
	FBDelPartition(char*p) : FBCmd(p, "delete-logical-partition") {}
};

class FBPartNumber : public CmdBase
{
public:
	FBPartNumber(char *p, std::string &&fb_cmd) :CmdBase(p), m_fb_cmd{std::move(fb_cmd)}
	{
		m_Size = 0;
		m_bCheckTotalParam = true;
		m_NoKeyParam = true;
		insert_param_info(nullptr, &m_partition_name, Param::Type::e_string, false, "partition name");
		insert_param_info(nullptr, &m_Size, Param::Type::e_uint32, false, "partition size");
	}

	int run(CmdCtx *ctx) override;

private:
	const std::string m_fb_cmd;
	std::string m_partition_name;
	uint32_t m_Size;
};

class FBCreatePartition : public FBPartNumber
{
public:
	FBCreatePartition(char*p) :FBPartNumber(p, "create-logical-partition") {}
};

class FBResizePartition : public FBPartNumber
{
public:
	FBResizePartition(char*p) :FBPartNumber(p, "resize-logical-partition") {}
};

class FBUpdateSuper : public CmdBase
{
public:
	FBUpdateSuper(char *p) :CmdBase(p)
	{
		m_bCheckTotalParam = true;
		m_NoKeyParam = true;
		insert_param_info(nullptr, &m_partition_name, Param::Type::e_string, false, "partition name");
		insert_param_info(nullptr, &m_opt, Param::Type::e_string, false, "partition size");
	}

	int run(CmdCtx *ctx) override;

private:
	const std::string m_fb_cmd = "update-super";
	std::string m_opt;
	std::string m_partition_name;
};

class FBEraseCmd : public FBCmd
{
public:
	FBEraseCmd(char *p) : FBCmd(p, "erase") {}
};


class FBRebootCmd : public FBCmd
{
public:
	FBRebootCmd(char *p) : FBCmd(p, "reboot") {}
};


class FBSetActiveCmd : public FBCmd
{
public:
	FBSetActiveCmd(char *p) : FBCmd(p, "set_active") {}
};

class FBDownload : public CmdBase
{
public:
	FBDownload(char *p) :CmdBase(p)
	{
		insert_param_info("download", nullptr, Param::Type::e_null);
		insert_param_info("-f", &m_filename, Param::Type::e_string_filename);
	}

	int run(CmdCtx *ctx) override;

private:
	std::string m_filename;
};

class FBCopy : public CmdBase
{
public:
	FBCopy(char *p) :CmdBase(p) {}
	int parser(char *p = nullptr) override;
	int run(CmdCtx *ctx) override;

private:
	bool m_bDownload;
	std::string m_local_file;
	size_t m_Maxsize_pre_cmd = 0x10000;
	std::string m_target_file;
};

class FBContinueCmd : public FBCmd
{
public:
	FBContinueCmd(char *p) : FBCmd(p, "continue") {}
};
