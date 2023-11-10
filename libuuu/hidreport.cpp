/*
 * Copyright 2020 NXP.
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

#include "hidreport.h"
#include "libcomm.h"
#include "liberror.h"
#include "trans.h"
#include "zip.h"

#include <cstring>

HIDReport::~HIDReport()
{
}

void HIDReport::notify(size_t index, uuu_notify::NOTIFY_TYPE type)
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

int HIDReport::read(std::vector<uint8_t> &buff)
{
	if (buff.size() < m_size_in + m_size_payload)
	{
		set_last_err_string("buffer to small to get a package");
		return -1;
	}
	size_t rs;
	int ret = m_pdev->read(buff.data(), m_size_in + m_size_payload, &rs);

	return ret;
}

int HIDReport::write(const void *p, size_t sz, uint8_t report_id)
{
	notify(sz, uuu_notify::NOTIFY_TRANS_SIZE);

	const uint8_t * const buff = reinterpret_cast<const uint8_t *>(p);
	size_t off = 0;
	for (; off < sz; off += m_size_out)
	{
		m_out_buff[0] = report_id;

		size_t s = sz - off;

		if (s > m_size_out)
			s = m_size_out;

		memcpy(m_out_buff.data() + m_size_payload, buff + off, s);

		/*
		 * The Windows HIDAPI is ver strict. It always require to send
		 * buffers of the size reported by the HID Report Descriptor.
		 * Therefore we must to send m_size_out buffers for HID ID 2
		 * albeit it may not required for the last buffer.
		 */
		if (report_id == 2)
			s = m_size_out;

		int ret = m_pdev->write(m_out_buff.data(), s + m_size_payload);

		if (ret < 0)
			return -1;

		if (off % 0x1F == 0)
		{
			notify(off, uuu_notify::NOTIFY_TRANS_POS);
		}
	}

	notify(sz, uuu_notify::NOTIFY_TRANS_POS);
	return 0;
}
