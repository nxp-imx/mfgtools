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

int Zip::BuildDirInfo()
{
	ifstream stream(m_filename, ifstream::binary);

	if (!stream)
	{
		string err;
		err = "Failure open file ";
		err += m_filename;
		set_last_err_string(m_filename);
		return -1;
	}

	stream.seekg(0, ifstream::end);
	size_t sz = stream.tellg();
	if (sz < 0x10000)
		stream.seekg(0, ifstream::beg);
	else
		stream.seekg(sz - 0x10000, ifstream::beg);

	vector<uint8_t> buff(0x10000);
	memset(buff.data(), 0, 0x10000);
	stream.read((char*)buff.data(), 0x10000);
	size_t i;
	Zip_eocd *peocd = NULL;

	for (i = buff.size() - sizeof(Zip_eocd); i > 0; i--)
	{
		peocd = (Zip_eocd*)(buff.data() + i);
		if (peocd->sign == EOCD_SIGNATURE)
		{
			break;
		}
	}

	if (peocd == 0)
	{
		set_last_err_string("Can't find EOCD, not a zip file");
		return -1;
	}

	stream.seekg(peocd->offset_of_central_dir);
	buff.resize(peocd->size_of_central_dir);

	stream.read((char*)buff.data(), peocd->size_of_central_dir);

	i = 0;
	while (i < buff.size())
	{
		Zip_central_dir *pdir = (Zip_central_dir *)(buff.data() + i);
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

	ifstream stream(pZip->m_filename, ifstream::binary);
	if (!stream)
	{
		string err;
		err += "Failure open file ";
		err += pZip->m_filename;
		set_last_err_string(err);
	}
	stream.seekg(m_offset, ifstream::beg);
	Zip_file_desc file_desc;
	stream.read((char*)&file_desc, sizeof(file_desc));
	if (file_desc.sign != FILE_SIGNATURE)
	{
		set_last_err_string("file signature miss matched");
		return -1;
	}

	size_t off = sizeof(file_desc) + file_desc.file_name_length + file_desc.extrafield_length;
	stream.seekg(m_offset + off, ifstream::beg);

	int CHUNK = 0x10000;
	vector<uint8_t> source(CHUNK);
	int ret;
	size_t pos = 0;

	memset(&m_strm, 0, sizeof(m_strm));
	inflateInit2(&m_strm, -MAX_WBITS);

	/* decompress until deflate stream ends or end of file */
	do {
		stream.read((char*)source.data(), CHUNK);
		m_strm.avail_in = stream.gcount();

		if (m_strm.avail_in == 0)
			break;
		m_strm.next_in = source.data();

		/* run inflate() on input until output buffer not full */
		do {
			m_strm.avail_out = CHUNK;
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
			size_t have = CHUNK - m_strm.avail_out;

			p->m_avaible_size = pos;
			p->m_request_cv.notify_all();

			pos += have;
		} while (m_strm.avail_out == 0);

		if(pos - lastpos > 100 * 1024 * 1024)
		{
			uuu_notify ut;
			ut.type = uuu_notify::NOTIFY_DECOMPRESS_POS;
			ut.index = pos;
			call_notify(ut);
			lastpos = pos;
		}
		/* done when inflate() says it's done */
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
