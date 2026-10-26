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

#include "trans.h"
#include "libcomm.h"
#include "liberror.h"

extern "C"
{
#include "libusb.h"
}

using namespace std;

extern bool g_force_hid_use_ctrl_ep;

int HIDTrans::open(TransHandle &p)
{
	int ret;

	if ((ret = USBTrans::open(p)))
		return ret;

	for (const auto &ep : m_EPs)
	{
		if (ep.addr > 0 && ((ep.addr & 0x80) == 0))
			m_outEP = ep.addr;
	}

	return 0;
}

int HIDTrans::write_simple(void *buff, size_t size)
{
	int ret;
	uint8_t *p = (uint8_t *)buff;
	int actual_size;

	if (m_outEP && !g_force_hid_use_ctrl_ep)
	{
		ret = libusb_interrupt_transfer(
			(libusb_device_handle *)m_devhandle,
			m_outEP,
			p,
			size,
			&actual_size,
			m_timeout
		);
	}
	else
	{
		ret = libusb_control_transfer(
			(libusb_device_handle *)m_devhandle,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
			m_set_report,
			(2 << 8) | p[0],
			0,
			p,
			size,
			m_timeout
		);
	}

	if (ret < 0)
	{
		string_ex err;
		err.format("HID(W): %s (%d)", libusb_error_name(ret), ret);
		set_last_err_string(err);
		return ret;
	}

	return ret;
}

int HIDTrans::read_simple(void *buff, size_t size, size_t *rsize)
{
	int ret;
	int actual;
	ret = libusb_interrupt_transfer(
		(libusb_device_handle *)m_devhandle,
		0x81,
		(uint8_t*)buff,
		size,
		&actual,
		m_timeout
	);

	*rsize = actual;

	if (ret < 0)
	{
		string_ex err;
		err.format("HID(R): %s (%d)", libusb_error_name(ret), ret);
		set_last_err_string(err);
		return ret;
	}

	return 0;
}
