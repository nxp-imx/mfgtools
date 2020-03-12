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
#include <string.h>
#include "fastboot.h"
#include "libcomm.h"
#include "cmd.h"
#include "buffer.h"
#include "libuuu.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include "sparse.h"
#include "ffu_format.h"
#include "libcomm.h"

static constexpr size_t SparseLimit{0x1000000};

int FastBoot::Transport(string cmd, void *p, size_t size, vector<uint8_t> *input)
{
	if (m_pTrans->write((void*)cmd.data(), cmd.size()))
		return -1;

	char buff[65];
	memset(buff, 0, 65);

	while ( strncmp(buff, "OKAY", 4) && strncmp(buff, "FAIL", 4))
	{
		size_t actual;
		memset(buff, 0, 65);
		if (m_pTrans->read(buff, 64, &actual))
			return -1;

		if (strncmp(buff, "DATA",4) == 0)
		{
			size_t sz;
			sz = strtoul(buff+4, NULL, 16);

			if (input)
			{
				input->resize(sz);
				size_t rz;
				if (m_pTrans->read(input->data(), sz, &rz))
					return -1;
				input->resize(rz);
			}
			else
			{
				if (sz > size)
					sz = size;

				if (m_pTrans->write(p, sz))
					return -1;
			}
		}else
		{
			string s;
			s = buff + 4;
			m_info += s;
			uuu_notify nt;
			nt.type = uuu_notify::NOTIFY_CMD_INFO;
			nt.str = buff + 4;
			call_notify(nt);
		}
	}

	if (strncmp(buff, "OKAY", 4) == 0)
		return 0;

	set_last_err_string(m_info);
	return -1;
}

int FBGetVar::parser(char *p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;
	string param = get_next_param(m_cmd, pos);

	if (param.find(':') != string::npos)
		param = get_next_param(m_cmd, pos);

	if (str_to_upper(param) != "GETVAR")
	{
		string err = "Unknown Commnd:";
		err += param;
		set_last_err_string(err);
		return -1;
	}

	m_var = get_next_param(m_cmd, pos);
	return 0;
}
int FBGetVar::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	FastBoot fb(&dev);
	string cmd;
	cmd = "getvar:";
	cmd += m_var;

	if (fb.Transport(cmd, NULL, 0))
		return -1;

	m_val = fb.m_info;
	return 0;
}

int FBCmd::parser(char *p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;
	string s;
	
	if (parser_protocal(p, pos))
		return -1;
	
	s = get_next_param(m_cmd, pos);

	if (str_to_upper(s) != str_to_upper(m_fb_cmd))
	{
		string err = "Unknown command: ";
		err += s;
		set_last_err_string(s);
		return -1;
	}

	if(pos!=string::npos && pos < m_cmd.size())
		m_uboot_cmd = m_cmd.substr(pos);
	return 0;
}

int FBCmd::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	dev.m_timeout = m_timeout;

	FastBoot fb(&dev);
	string cmd;
	cmd = m_fb_cmd;
	cmd += m_separator;
	cmd += m_uboot_cmd;

	if (fb.Transport(cmd, NULL, 0))
		return -1;

	return 0;
}

int FBPartNumber::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	dev.m_timeout = m_timeout;

	FastBoot fb(&dev);

	string_ex cmd;
	cmd.format("%s:%s:%08x", m_fb_cmd.c_str(), m_partition_name.c_str(), (uint32_t)m_Size);

	if (fb.Transport(cmd, NULL, 0))
		return -1;

	return 0;
}

int FBUpdateSuper::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	dev.m_timeout = m_timeout;

	FastBoot fb(&dev);

	string_ex cmd;
	cmd.format("%s:%s:%s", m_fb_cmd.c_str(), m_partition_name.c_str(), m_opt.c_str());

	if (fb.Transport(cmd, NULL, 0))
		return -1;

	return 0;
}

int FBDownload::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	FastBoot fb(&dev);

	shared_ptr<FileBuffer> buff = get_file_buffer(m_filename);
	if (buff == NULL)
		return -1;

	string_ex cmd;
	cmd.format("download:%08x", buff->size());

	if (fb.Transport(cmd, buff->data(), buff->size()))
		return -1;

	return 0;
}

int FBCopy::parser(char *p)
{
	if (p)
		m_cmd = p;

	size_t pos = 0;
	string s;
	s = get_next_param(m_cmd, pos);
	if (s.find(":") != s.npos)
		s = get_next_param(m_cmd, pos);

	if ((str_to_upper(s) != "UCP"))
	{
		string err = "Unknown command: ";
		err += s;
		set_last_err_string(s);
		return -1;
	}

	string source;
	string dest;

	source = get_next_param(m_cmd, pos);
	dest = get_next_param(m_cmd, pos);

	if (source.empty())
	{
		set_last_err_string("ucp: source missed");
		return -1;
	}

	if (dest.empty())
	{
		set_last_err_string("ucp: destination missed");
		return -1;
	}

	if (source.find("T:") == 0 || source.find("t:") == 0)
	{
		if (dest.find("T:") == 0 || dest.find("t:") == 0)
		{
			set_last_err_string("ucp just support one is remote file start with t:");
			return -1;
		}
		m_target_file = source.substr(2);
		m_bDownload = false; //upload a file
		m_local_file = dest;
	}
	else if (dest.find("T:") == 0 || dest.find("t:") == 0)
	{
		m_target_file = dest.substr(2);
		m_bDownload = true;
		m_local_file = source;
		get_file_buffer(source, true);
	}
	else
	{
		set_last_err_string("ucp must a remote file name, start with t:<file name>");
		return -1;
	}
	return 0;
}

int FBCopy::run(CmdCtx *ctx)
{
	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	FastBoot fb(&dev);
	string_ex cmd;

	if(m_bDownload)
	{
		size_t i;
		shared_ptr<FileBuffer> buff = get_file_buffer(m_local_file);
		if (buff == NULL)
		{
			return -1;
		}

		cmd.format("WOpen:%s", m_target_file.c_str());
		if (fb.Transport(cmd, NULL, 0))
		{
			if (fb.m_info == "DIR")
			{
				Path p;
				p.append(m_local_file);
				string target = m_target_file;
				target += "/";
				target += p.get_file_name();

				cmd.format("WOpen:%s", target.c_str());
				if (fb.Transport(cmd, NULL, 0))
					return -1;
			}
			else {
				return -1;
			}
		}

		uuu_notify nt;
		nt.type = uuu_notify::NOTIFY_TRANS_SIZE;
		nt.total = buff->size();
		call_notify(nt);

		for (i = 0; i < buff->size(); i += this->m_Maxsize_pre_cmd)
		{
			size_t sz = buff->size() - i;
			if (sz > m_Maxsize_pre_cmd)
				sz = m_Maxsize_pre_cmd;

			cmd.format("donwload:%08X", sz);
			if (fb.Transport(cmd, buff->data() + i, sz))
			{
				if (fb.m_info == "EPIPE")
					break;
				return -1;
			}

			nt.type = uuu_notify::NOTIFY_TRANS_POS;
			nt.index = i;
			call_notify(nt);
		}

		nt.type = uuu_notify::NOTIFY_TRANS_POS;
		nt.index = i;
		call_notify(nt);
	}
	else
	{
		cmd.format("ROpen:%s", m_target_file.c_str());
		if (fb.Transport(cmd, NULL, 0))
			return -1;

		uuu_notify nt;
		nt.type = uuu_notify::NOTIFY_TRANS_SIZE;
		size_t total = nt.total = strtoul(fb.m_info.c_str(), NULL, 16);
		call_notify(nt);

		nt.index = 0;
		ofstream of;

		struct stat st;

		Path localfile;
		localfile.append(m_local_file);

		if (stat(localfile.c_str(), &st) == 0)
		{
			if (st.st_mode & S_IFDIR)
			{
				localfile += "/";
				Path t;
				t.append(m_target_file);
				localfile += t.get_file_name();
			}
		}

		of.open(localfile, ofstream::binary);

		if (!of)
		{
			string err;
			err = "Fail to open file";
			err += localfile;
			set_last_err_string(err);
		}
		do
		{
			vector<uint8_t> data;
			if (fb.Transport("upload", NULL, 0, &data))
				return -1;

			of.write((const char*)data.data(), data.size());

			nt.type = uuu_notify::NOTIFY_TRANS_POS;
			nt.index += data.size();
			call_notify(nt);

			if (data.size() == 0)
				break;

		} while (nt.index < total ||  total == 0 ); // If total is 0, it is stream

		nt.type = uuu_notify::NOTIFY_TRANS_POS;
		call_notify(nt);
	}

	cmd.format("Close");
	if (fb.Transport(cmd, NULL, 0))
		return -1;

	return 0;
}

int FBFlashCmd::parser(char *p)
{
	if (FBCmd::parser(p))
		return -1;

	string subcmd = m_uboot_cmd;
	size_t pos = 0;
	m_partition = get_next_param(subcmd, pos);
	if (m_partition == "-raw2sparse")
	{
		m_raw2sparse = true;
		m_partition = get_next_param(subcmd, pos);
	}

	if (pos == string::npos || m_partition.empty())
	{
		set_last_err_string("Missed partition name");
		return -1;
	}
	m_filename = get_next_param(subcmd, pos);
	if (m_filename.empty())
	{
		set_last_err_string("Missed file name");
		return -1;
	}

	if (!check_file_exist(m_filename))
		return -1;

	return 0;
}

int FBFlashCmd::flash(FastBoot *fb, void * pdata, size_t sz)
{
	string_ex cmd;
	cmd.format("download:%08x", sz);

	if (fb->Transport(cmd, pdata, sz))
		return -1;

	cmd.format("flash:%s", m_partition.c_str());
	if (fb->Transport(cmd, NULL, 0))
		return -1;

	return 0;
}

int FBFlashCmd::flash_raw2sparse(FastBoot *fb, shared_ptr<FileBuffer> pdata, size_t block_size, size_t max)
{
	SparseFile sf;

	vector<uint8_t> data;

	if (max > SparseLimit)
		 max = SparseLimit;

	sf.init_header(block_size, (max + block_size -1) / block_size);

	data.resize(block_size);

	uuu_notify nt;
	bool bload = pdata->IsKnownSize();

	nt.type = uuu_notify::NOTIFY_TRANS_SIZE;
	if (bload)
		nt.total = pdata->size();
	else
		nt.total = 0;

	call_notify(nt);
	

	size_t i = 0;
	while (!pdata->request_data(data, i*block_size, block_size))
	{
		int ret = sf.push_one_block(data.data());
		if (ret)
		{
			if (flash(fb, sf.m_data.data(), sf.m_data.size()))
				return -1;

			sf.init_header(block_size, (max + block_size - 1) / block_size);

			chunk_header_t ct;
			ct.chunk_type = CHUNK_TYPE_DONT_CARE;
			ct.chunk_sz = i + 1;
			ct.reserved1 = 0;
			ct.total_sz = sizeof(ct);

			sf.push_one_chuck(&ct, NULL);

			nt.type = uuu_notify::NOTIFY_TRANS_POS;
			nt.total = i * block_size;
			call_notify(nt);
		}

		i++;

		if (bload != pdata->IsKnownSize())
		{
			nt.type = uuu_notify::NOTIFY_TRANS_SIZE;
			nt.total = pdata->size();
			call_notify(nt);

			bload = pdata->IsKnownSize();
		}
	}

	if (flash(fb, sf.m_data.data(), sf.m_data.size()))
		return -1;

	nt.type = uuu_notify::NOTIFY_TRANS_SIZE;
	nt.total = pdata->size();
	call_notify(nt);

	nt.type = uuu_notify::NOTIFY_TRANS_POS;
	nt.total = pdata->size();
	call_notify(nt);

	return 0;
}

int FBFlashCmd::run(CmdCtx *ctx)
{
	FBGetVar getvar((char*)"FB: getvar max-download-size");
	if (getvar.parser(NULL))
		return -1;
	if (getvar.run(ctx))
		return -1;

	size_t max = getvar.m_val.empty() ? SparseLimit : str_to_uint(getvar.m_val);

	string str;
	str = "FB: getvar partition-size:";
	str += m_partition;

	if (getvar.parser((char*)str.c_str()))
		return -1;

	if (getvar.run(ctx))
		return -1;

	m_totalsize = str_to_uint64(getvar.m_val);

	BulkTrans dev;
	if (dev.open(ctx->m_dev))
		return -1;

	FastBoot fb(&dev);
	dev.m_timeout = m_timeout;

	if (m_raw2sparse)
	{
		if (getvar.parser((char*)"FB: getvar logical-block-size"))
			return -1;
		if (getvar.run(ctx))
			return -1;

		size_t block_size = str_to_uint(getvar.m_val);

		if (block_size == 0)
		{
			set_last_err_string("Device report block_size is 0");
			return -1;
		}

		shared_ptr<FileBuffer> pdata = get_file_buffer(m_filename, true);

		if (isffu(pdata))
			return flash_ffu(&fb, pdata);

		return flash_raw2sparse(&fb, pdata, block_size, max);
	}

	shared_ptr<FileBuffer> pdata = get_file_buffer(m_filename);
	if (pdata == NULL)
		return -1;

	if (SparseFile::is_validate_sparse_file(pdata->data(), pdata->size()))
	{	/* Limited max size to 16M for sparse file to avoid long timeout at read status*/
		if (max > SparseLimit)
			max = SparseLimit;
	}

	if (pdata->size() <= max)
	{
		if (flash(&fb, pdata->data(), pdata->size()))
			return -1;
	}
	else
	{
		size_t pos = 0;
		sparse_header * pfile = (sparse_header *)pdata->data();

		if (!SparseFile::is_validate_sparse_file(pdata->data(), pdata->size()))
		{
			set_last_err_string("Sparse file magic miss matched");
			return -1;
		}

		SparseFile sf;
		size_t startblock;
		chunk_header_t * pheader;

		uuu_notify nt;
		nt.type = uuu_notify::NOTIFY_TRANS_SIZE;
		nt.total = pfile->total_blks;
		call_notify(nt);

		sf.init_header(pfile->blk_sz, max / pfile->blk_sz);
		startblock = 0;

		for(size_t nblk=0; nblk < pfile->total_chunks && pos <= pdata->size(); nblk++)
		{
			pheader = SparseFile::get_next_chunk(pdata->data(), pos);

			size_t sz = sf.push_one_chuck(pheader, pheader + 1);
			if (sz == pheader->total_sz - sizeof(chunk_header_t))
			{
				startblock += pheader->chunk_sz;
			}
			else
			{
				size_t off = ((uint8_t*)pheader) - pdata->data() + sz + sizeof(chunk_header_t);
				startblock += sz / pfile->blk_sz;

				do
				{
					if (flash(&fb, sf.m_data.data(), sf.m_data.size()))
						return -1;

					sf.init_header(pfile->blk_sz, max / pfile->blk_sz);

					chunk_header_t ct;
					ct.chunk_type = CHUNK_TYPE_DONT_CARE;
					ct.chunk_sz = startblock;
					ct.reserved1 = 0;
					ct.total_sz = sizeof(ct);

					sz = sf.push_one_chuck(&ct, NULL);

					sz = sf.push_raw_data(pdata->data() + off, pos - off);
					off += sz;
					startblock += sz / pfile->blk_sz;

					uuu_notify nt;
					nt.type = uuu_notify::NOTIFY_TRANS_POS;
					nt.total = startblock;
					call_notify(nt);

				} while (off < pos);
			}
		}

		//send last data
		if (flash(&fb, sf.m_data.data(), sf.m_data.size()))
			return -1;

		sparse_header * pf = (sparse_header *)sf.m_data.data();
		nt.type = uuu_notify::NOTIFY_TRANS_POS;
		nt.total = startblock + pf->total_blks;
		call_notify(nt);
	}
	return 0;
}

bool FBFlashCmd::isffu(shared_ptr<FileBuffer> p)
{
	vector<uint8_t> data;
	data.resize(sizeof(FFU_SECURITY_HEADER));
	p->request_data(data, 0, sizeof(FFU_SECURITY_HEADER));

	FFU_SECURITY_HEADER *h = (FFU_SECURITY_HEADER*)data.data();
	if (strncmp((const char*)h->signature, FFU_SECURITY_SIGNATURE, sizeof(h->signature)) == 0)
		return true;
	else
		return false;
}

int FBFlashCmd::flash_ffu_oneblk(FastBoot *fb, shared_ptr<FileBuffer> p, size_t off, size_t blksz, size_t blkindex)
{
	SparseFile sf;

	sf.init_header(blksz, 10);

	p->request_data(off + blksz);
	
	chunk_header_t ct;
	ct.chunk_type = CHUNK_TYPE_DONT_CARE;
	ct.chunk_sz = blkindex;
	ct.reserved1 = 0;
	ct.total_sz = sizeof(ct);

	sf.push_one_chuck(&ct, NULL);

	if (sf.push_one_block(p->data() + off))
		return -1;

	return flash(fb, sf.m_data.data(), sf.m_data.size());
}

int FBFlashCmd::flash_ffu(FastBoot *fb, shared_ptr<FileBuffer> p)
{
	p->request_data(sizeof(FFU_SECURITY_HEADER));
	FFU_SECURITY_HEADER *h = (FFU_SECURITY_HEADER*)p->data();
	if (strncmp((const char*)h->signature, FFU_SECURITY_SIGNATURE, sizeof(h->signature)) != 0)
	{
		set_last_err_string("Invalidate FFU Security header signature");
		return -1;
	}

	size_t off;
	off = h->dwCatalogSize + h->dwHashTableSize;
	off = round_up(off, (size_t)h->dwChunkSizeInKb * 1024);

	p->request_data(off + sizeof(FFU_IMAGE_HEADER));
	FFU_IMAGE_HEADER *pIh = (FFU_IMAGE_HEADER *)(p->data() + off);

	if (strncmp((const char*)pIh->Signature, FFU_SIGNATURE, sizeof(pIh->Signature)) != 0)
	{
		set_last_err_string("Invalidate FFU Security header signature");
		return -1;
	}

	off += pIh->ManifestLength + pIh->cbSize;
	off = round_up(off, (size_t)h->dwChunkSizeInKb * 1024);

	p->request_data(off + sizeof(FFU_STORE_HEADER));
	FFU_STORE_HEADER *pIs = (FFU_STORE_HEADER*) (p->data() + off);

	if(pIs->MajorVersion == 1)
		off += pIs->dwValidateDescriptorLength + offsetof(FFU_STORE_HEADER, NumOfStores);
	else
		off += pIs->dwValidateDescriptorLength + sizeof(FFU_STORE_HEADER);

	p->request_data(off + pIs->dwWriteDescriptorLength);

	size_t block_off = off + pIs->dwWriteDescriptorLength;
	block_off = round_up(block_off, (size_t)h->dwChunkSizeInKb * 1024);

	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_TRANS_SIZE;
	nt.total = pIs->dwWriteDescriptorCount;
	call_notify(nt);

	size_t currrent_block = 0;
	size_t i;
	for (i = 0; i < pIs->dwWriteDescriptorCount; i++)
	{
		FFU_BLOCK_DATA_ENTRY *entry = (FFU_BLOCK_DATA_ENTRY*)(p->data() + off);
		
		off += sizeof(FFU_BLOCK_DATA_ENTRY) + (entry->dwLocationCount -1) * sizeof(_DISK_LOCATION);

		if (currrent_block >= pIs->dwInitialTableIndex && currrent_block < pIs->dwInitialTableIndex + pIs->dwInitialTableCount)
		{
			//Skip Init Block
		}
		else
		{
			for (uint32_t loc = 0; loc < entry->dwLocationCount; loc++)
			{
				//printf("block 0x%x write to 0x%x seek %d\n", currrent_block, entry->rgDiskLocations[loc].dwBlockIndex, entry->rgDiskLocations[loc].dwDiskAccessMethod);
				uint32_t access = entry->rgDiskLocations[loc].dwDiskAccessMethod;
				uint32_t blockindex;
				if (entry->rgDiskLocations[loc].dwDiskAccessMethod == DISK_BEGIN)
					blockindex = entry->rgDiskLocations[loc].dwBlockIndex;
				else
					blockindex = m_totalsize / pIs->dwBlockSizeInBytes - 1 - entry->rgDiskLocations[loc].dwBlockIndex;

				for (uint32_t blk = 0; blk < entry->dwBlockCount; blk++)
				{
					if (flash_ffu_oneblk(fb,
							p,
							block_off + (currrent_block + blk) * pIs->dwBlockSizeInBytes,
							pIs->dwBlockSizeInBytes,
							blockindex + blk))
						return -1;
				}
			}
		}

		nt.type = uuu_notify::NOTIFY_TRANS_POS;
		nt.total = i;
		call_notify(nt);

		currrent_block += entry->dwBlockCount;
	}

	nt.type = uuu_notify::NOTIFY_TRANS_POS;
	nt.total = i;
	call_notify(nt);

	return 0;
}
