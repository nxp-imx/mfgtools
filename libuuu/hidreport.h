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
#include <string>
#include <vector>
#include "libcomm.h"
#include "libuuu.h"
#include "trans.h"
#include <string.h>

#define ROM_INFO_HID					   0x1
#define ROM_INFO_HID_MX23				   0x2
#define ROM_INFO_HID_MX50				   0x4
#define ROM_INFO_HID_MX6				   0x8
#define ROM_INFO_HID_SKIP_DCD			  0x10
#define ROM_INFO_HID_MX8_MULTI_IMAGE	  0x20
#define ROM_INFO_HID_MX8_STREAM			  0x40
#define ROM_INFO_HID_UID_STRING			  0x80
#define ROM_INFO_HID_NO_CMD				 0x400
#define ROM_INFO_SPL_JUMP				 0x800
#define ROM_INFO_HID_EP1				0x1000

#pragma once

using namespace std;

class HIDReport
{
	size_t m_size_in;
	size_t m_size_out;
	size_t m_size_payload;
public:
	TransBase * m_pdev;
	vector<uint8_t> m_out_buff;
	size_t m_postion_base;
	size_t m_notify_total;
	size_t get_out_package_size() { return m_size_out; }
	bool m_skip_notify;
	void init()
	{
		m_size_in = 64;
		m_size_out = 1024;
		m_size_payload = 1;
		m_postion_base = 0;
		m_notify_total = 0;
		m_out_buff.resize(m_size_out + m_size_payload);
		m_skip_notify = true;
	}
	HIDReport()
	{
		init();
	}

	HIDReport(TransBase *trans)
	{
		init();
		m_pdev = trans;
	}

	int read(vector<uint8_t> &buff)
	{
		size_t rs;
		if (buff.size() < m_size_in + m_size_payload)
		{
			set_last_err_string("buffer to small to get a package");
			return -1;
		}
		int ret = m_pdev->read(buff.data(), m_size_in + m_size_payload, &rs);
		if (ret)
			return ret;

		return ret;
	}

	virtual void notify(size_t index, uuu_notify::NOTIFY_TYPE type)
	{
		uuu_notify nf;
		nf.type = type;
		if(type == uuu_notify::NOTIFY_TRANS_POS)
			nf.index = index + m_postion_base;
		if (type == uuu_notify::NOTIFY_TRANS_SIZE)
		{
			nf.index = m_notify_total > index ? m_notify_total : index;
		}
		call_notify(nf);
	}

	int write(void *p, size_t sz, uint8_t report_id)
	{
		size_t off;
		uint8_t *buff = (uint8_t *)p;

		notify(sz, uuu_notify::NOTIFY_TRANS_SIZE);

		for (off = 0; off < sz; off += m_size_out)
		{
			m_out_buff[0] = report_id;

			size_t s = sz - off;
			if (s > m_size_out)
				s = m_size_out;

			memcpy(m_out_buff.data() + m_size_payload, buff + off, s);

			int ret = m_pdev->write(m_out_buff.data(), report_id == 1? s + m_size_payload: m_size_out + m_size_payload);

			if ( ret < 0)
				return -1;

			if (off % 0x1F == 0)
			{
				notify(off, uuu_notify::NOTIFY_TRANS_POS);
			}
		}
		notify(off, uuu_notify::NOTIFY_TRANS_POS);
		return 0;
	}

	int write(vector<uint8_t> &buff, uint8_t report_id)
	{
		return write(buff.data(), buff.size(), report_id);
	}

};
