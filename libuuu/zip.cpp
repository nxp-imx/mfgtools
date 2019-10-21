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
#include <string.h>
#include <iostream>
#include <fstream>
#include "libcomm.h"
#include "libuuu.h"
#include "liberror.h"
#include "zip.h"

#define CHUNK 0x10000

int Zip::BuildDirInfo()
{
	shared_ptr<FileBuffer> zipfile = get_file_buffer(m_filename);
	if (zipfile == NULL)
	{
		return -1;
	}

	size_t i;
	Zip_eocd *peocd = NULL;
	Zip64_eocd_locator *peocd64_loc = NULL;
	Zip64_eocd *peocd64 = NULL;

	for (i = zipfile->size() - sizeof(Zip_eocd); i > 0; i--)
	{
		peocd = (Zip_eocd*)(zipfile->data() + i);
		if (peocd->sign == EOCD_SIGNATURE)
		{
			if (peocd->offset_of_central_dir == 0xFFFFFFFF)
			{//zip64
				for (size_t j = i - sizeof(Zip64_eocd_locator); j > 0; j--)
				{
					peocd64_loc = (Zip64_eocd_locator*)(zipfile->data() + j);
					if (peocd64_loc->sign == EOCD64_LOCATOR_SIGNATURE)
					{
						peocd64 = (Zip64_eocd*)(zipfile->data() + peocd64_loc->offset_of_eocd);
						if (peocd64->sign != EOCD64_SIGNATURE)
						{
							set_last_err_string("Can't find EOCD64_SIGNATURE, not a zip64 file");
							return -1;
						}
						break;
					}

					if (zipfile->size() - j > 0x10000)
					{
						set_last_err_string("Can't find EOCD, not a zip file");
						return -1;
					}
				}
			}
			break;
		}

		if (zipfile->size() - i > 0x10000)
		{
			set_last_err_string("Can't find EOCD, not a zip file");
			return -1;
		}
	}

	if (peocd == 0)
	{
		set_last_err_string("Can't find EOCD, not a zip file");
		return -1;
	}

	i = peocd64? peocd64->offset:peocd->offset_of_central_dir;
	size_t total = i;
	total += peocd64 ? peocd64->size: peocd->size_of_central_dir;

	while (i < total)
	{
		Zip_central_dir *pdir = (Zip_central_dir *)(zipfile->data() + i);
		if (pdir->sign != DIR_SIGNTURE)
		{
			set_last_err_string("DIR signature missmatched");
			return -1;
		}
		Zip_file_Info info;
		info.m_filename.append((char*)pdir->filename, pdir->file_name_length);
		info.m_offset = pdir->offset;
		info.m_filesize = pdir->uncompressed_size;
		info.m_timestamp = (pdir->last_modidfy_date << 16) + pdir->last_modidfy_time;
		info.m_compressedsize = pdir->compressed_size;

		if (pdir->extrafield_length)
		{
			size_t e;
			for (e = 0; e < pdir->extrafield_length; /*dummy*/)
			{
				Zip_ext *ext = (Zip_ext*)(zipfile->data() + e + i + sizeof(Zip_central_dir) + pdir->file_name_length);

				if (ext->tag == 0x1)
				{
					size_t cur64 = 0;
					if (info.m_filesize == 0xFFFFFFFF)
					{
						info.m_filesize = *((uint64_t*)(((uint8_t*)ext) + sizeof(Zip_ext) + cur64));
						cur64 += 8;
					}

					if (cur64 > ext->size)
					{
						set_last_err_string("error pass zip64");
						return -1;
					}

					if (info.m_compressedsize == 0xFFFFFFFF)
					{
						info.m_compressedsize = *((uint64_t*)(((uint8_t*)ext) + sizeof(Zip_ext) + cur64));
						cur64 += 8;
					}

					if (cur64 > ext->size)
					{
						set_last_err_string("error pass zip64");
						return -1;
					}

					if (info.m_offset == 0xFFFFFFFF)
					{
						info.m_offset = *((uint64_t*)(((uint8_t*)ext) + sizeof(Zip_ext) + cur64));
						cur64 += 8;
					}

					if (cur64 > ext->size)
					{
						set_last_err_string("error pass zip64");
						return -1;
					}

					break;
				}
				e += ext->size + sizeof(Zip_ext);
			}
		}
		i += sizeof(Zip_central_dir) + pdir->extrafield_length + pdir->file_name_length + pdir->file_comment_length;
		m_filemap[info.m_filename] = info;
	}

	return 0;
}

Zip_file_Info::Zip_file_Info()
{

}

Zip_file_Info::~Zip_file_Info()
{
	memset(&m_strm, 0, sizeof(m_strm));
}

int	Zip_file_Info::decompress(Zip *pZip, shared_ptr<FileBuffer>p)
{
	p->resize(m_filesize);
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_KNOWN_SIZE);
	
	uuu_notify ut;
	ut.type = uuu_notify::NOTIFY_DECOMPRESS_SIZE;
	ut.total = m_filesize;
	call_notify(ut);
	size_t lastpos = 0;

	shared_ptr<FileBuffer> zipfile = get_file_buffer(pZip->m_filename);
	if (zipfile == NULL)
		return -1;

	Zip_file_desc *file_desc=(Zip_file_desc *)(zipfile->data() + m_offset);
	if (file_desc->sign != FILE_SIGNATURE)
	{
		set_last_err_string("file signature miss matched");
		return -1;
	}

	size_t off = sizeof(Zip_file_desc) + file_desc->file_name_length + file_desc->extrafield_length;

	if (file_desc->compress_method == 0)
	{
		p->ref_other_buffer(zipfile, m_offset + off, m_filesize);
		atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
		return 0;
	}

	if (file_desc->compress_method != 8)
	{
		set_last_err_string("Unsupport compress method");
		return -1;
	}

	int ret;
	size_t pos = 0;

	memset(&m_strm, 0, sizeof(m_strm));
	inflateInit2(&m_strm, -MAX_WBITS);

	/* decompress until deflate stream ends or end of file */
	m_strm.avail_in = m_compressedsize;
	m_strm.next_in = zipfile->data() + m_offset + off;
	m_strm.total_in = m_compressedsize;

	/* run inflate() on input until output buffer not full */
	size_t each_out_size = CHUNK;
	do {

		if (p->size() - pos < each_out_size)
			each_out_size = p->size() - pos;

		m_strm.avail_out = each_out_size;
		m_strm.next_out = p->data() + pos;
		ret = inflate(&m_strm, Z_NO_FLUSH);
		//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
		switch (ret) {
		case Z_NEED_DICT:
			ret = Z_DATA_ERROR;     /* and fall through */
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			(void)inflateEnd(&m_strm);
			return -1;
		}
		size_t have = each_out_size - m_strm.avail_out;

		p->m_avaible_size = pos;
		p->m_request_cv.notify_all();

		pos += have;

		if (pos - lastpos > 100 * 1024 * 1024)
		{
			uuu_notify ut;
			ut.type = uuu_notify::NOTIFY_DECOMPRESS_POS;
			ut.index = pos;
			call_notify(ut);
			lastpos = pos;
		}

	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&m_strm);

	if (ret != Z_STREAM_END)
	{
		set_last_err_string("decompress error");
		return -1;
	}

	p->m_avaible_size = m_filesize;
	atomic_fetch_or(&p->m_dataflags, FILEBUFFER_FLAG_LOADED);
	p->m_request_cv.notify_all();

	ut.type = uuu_notify::NOTIFY_DECOMPRESS_POS;
	ut.index = m_filesize;
	call_notify(ut);

	return 0;
}
