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
#include "libuuu.h"
#include "liberror.h"
#include "libusb.h"

extern "C"
{
#include "libusb.h"
}

int USBTrans::open(void *p)
{
	m_devhandle = p;
	libusb_device_handle * handle = (libusb_device_handle *)m_devhandle;
	if (libusb_kernel_driver_active(handle, 0))
	{
		int ret = libusb_detach_kernel_driver((libusb_device_handle *)m_devhandle, 0);
		if(ret <0 && ret != LIBUSB_ERROR_NOT_SUPPORTED)
		{
			set_last_err_string("detach kernel driver failure");
			return -1;
		}
	}

	if (libusb_claim_interface(handle, 0))
	{
		set_last_err_string("Failure claim interface");
		return -1;
	}

	libusb_config_descriptor *config;
	if (libusb_get_active_config_descriptor(libusb_get_device(handle), &config))
	{
		set_last_err_string("Can't get config descriptor");
		return -1;
	}

	m_EPs.clear();
	for (int i = 0; i < config->interface[0].altsetting[0].bNumEndpoints; i++)
	{
		m_EPs.push_back(EPInfo(config->interface[0].altsetting[0].endpoint[i].bEndpointAddress,
							   config->interface[0].altsetting[0].endpoint[i].wMaxPacketSize));
	};

	libusb_free_config_descriptor(config);

	return 0;
}

int USBTrans::close()
{
	/* needn't clean resource here
	   libusb_close will release all resource when finish running cmd
	*/
	return 0;
}

int HIDTrans::write(void *buff, size_t size)
{
	int ret;
	uint8_t *p = (uint8_t *)buff;
	int actual_size;
	if (m_outEP)
	{
		ret = libusb_interrupt_transfer(
			(libusb_device_handle *)m_devhandle,
			m_outEP,
			p,
			size,
			&actual_size,
			1000
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
			1000
		);
	}

	if (ret < 0)
	{
		string err;
		err = "HID(W):";
		err += libusb_error_name(ret);
		set_last_err_string(err);
		return ret;
	}

	return ret;
}

int HIDTrans::read(void *buff, size_t size, size_t *rsize)
{
	int ret;
	int actual;
	ret = libusb_interrupt_transfer(
		(libusb_device_handle *)m_devhandle,
		0x81,
		(uint8_t*)buff,
		size,
		&actual,
		m_read_timeout
	);

	*rsize = actual;

	if (ret < 0)
	{
		string error;
		string err;
		err = "HID(R):";
		err += libusb_error_name(ret);
		set_last_err_string(err);
		return ret;
	}

	return 0;
}

int BulkTrans::write(void *buff, size_t size)
{
	int ret;
	int actual_lenght;
	for (size_t i = 0; i < size; i += m_MaxTransPreRequest)
	{
		uint8_t *p = (uint8_t *)buff;
		p += i;
		size_t sz;
		sz = size - i;
		if (sz > m_MaxTransPreRequest)
			sz = m_MaxTransPreRequest;

		ret = libusb_bulk_transfer(
			(libusb_device_handle *)m_devhandle,
			m_ep_out.addr,
			p,
			sz,
			&actual_lenght,
			m_timeout
		);

		if (ret < 0)
		{
			string error;
			string err;
			err = "Bulk(W):";
			err += libusb_error_name(ret);
			set_last_err_string(err);
			return ret;
		}
	}

	//Send zero package
	if (m_b_send_zero && ( (size%m_ep_out.package_size) == 0))
	{
		ret = libusb_bulk_transfer(
			(libusb_device_handle *)m_devhandle,
			m_ep_out.addr,
			NULL,
			0,
			&actual_lenght,
			2000
		);

		if (ret < 0)
		{
			string error;
			string err;
			err = "Bulk(W):";
			err += libusb_error_name(ret);
			set_last_err_string(err);
			return ret;
		}
	}

	return ret;
}

int BulkTrans::open(void *p)
{
	if (USBTrans::open(p))
		return -1;

	for (size_t i = 0; i < m_EPs.size(); i++)
	{
		if (m_EPs[i].addr > 0)
		{
			if ((m_EPs[0].addr & 0x80) && m_ep_in.addr == 0)
				m_ep_in = m_EPs[i];
			else if (m_ep_out.addr == 0)
				m_ep_out = m_EPs[i];
		}
	}
	return 0;
}
int BulkTrans::read(void *buff, size_t size, size_t *rsize)
{
	int ret;
	int actual_lenght;
	uint8_t *p = (uint8_t *)buff;
	ret = libusb_bulk_transfer(
		(libusb_device_handle *)m_devhandle,
		m_ep_in.addr,
		p,
		size,
		&actual_lenght,
		m_timeout
	);

	*rsize = actual_lenght;

	if (ret < 0)
	{
		string error;
		string err;
		err = "Bulk(R):";
		err += libusb_error_name(ret);
		set_last_err_string(err);
		return ret;
	}

	return ret;
}

static void LIBUSB_CALL transfer_cb(struct libusb_transfer *transfer)
{
	int *completed = (int *)transfer->user_data;
	*completed = 1;
}

int BulkTrans::prepare_multi_request(size_t size, size_t count)
{
	m_size_prerequest = size;
	
	for (size_t i = 0; i < count; i++)
	{
		struct libusb_transfer * transfer_in = libusb_alloc_transfer(0);
		if (transfer_in == NULL)
		{
			set_last_err_string("Allocate transfer failure");
			return -1;
		}

		Transfer t;
		m_vector_transfer.push(t);

		m_vector_transfer.back().libusb_transfer = transfer_in;
		m_vector_transfer.back().buffer= new uint8_t[size];
		m_vector_transfer.back().complete = new int;
		*m_vector_transfer.back().complete = 0;

		libusb_fill_bulk_transfer(transfer_in, (libusb_device_handle *)m_devhandle, m_ep_in.addr,
			m_vector_transfer.back().buffer, size,
			transfer_cb, m_vector_transfer.back().complete, m_timeout);

		int r = libusb_submit_transfer(transfer_in);
		if (r)
		{
			set_last_err_string("submit transfer failure");
			return r;
		}
		
	}
	return 0;
}

int BulkTrans::free_multi_request()
{
	while(!m_vector_transfer.empty())
	{
		if(*m_vector_transfer.front().complete == 0)
		{
			libusb_cancel_transfer(m_vector_transfer.front().libusb_transfer);

			int *completed = m_vector_transfer.front().complete;

			while (!*completed) {
				int ret = libusb_handle_events_completed(NULL, completed);
				if (ret < 0) {
					if (ret == LIBUSB_ERROR_INTERRUPTED)
						continue;
				}
			}

		}

		libusb_free_transfer(m_vector_transfer.front().libusb_transfer);
		delete m_vector_transfer.front().buffer;
		delete m_vector_transfer.front().complete;
		m_vector_transfer.pop();
	}
	return 0;
}

int BulkTrans::read_multi_request(void *buff, size_t size, size_t *return_size)
{
	int ret = -1;

	if (m_vector_transfer.size() == 0)
	{
		set_last_err_string("need call prepare_multi_request before call read_multi_request");
		return -1;
	}

	int *completed = m_vector_transfer.front().complete;

	while (!*completed) {
		int ret = libusb_handle_events_completed(NULL, completed);
		if (ret < 0) {
			if (ret == LIBUSB_ERROR_INTERRUPTED)
				continue;
		}
	}

	if (m_vector_transfer.front().libusb_transfer->status == LIBUSB_TRANSFER_COMPLETED)
	{
		*return_size = m_vector_transfer.front().libusb_transfer->actual_length;
		if (*return_size > size)
			*return_size = size;

		memcpy(buff, m_vector_transfer.front().buffer, *return_size);

		ret = 0;
	}
	else
	{
		string error;
		string err;
		err = "Bulk(R):";
		err += "Read Error";
		set_last_err_string(err);
	}

	libusb_free_transfer(m_vector_transfer.front().libusb_transfer);
	delete m_vector_transfer.front().buffer;
	delete m_vector_transfer.front().complete;
	m_vector_transfer.pop();

	struct libusb_transfer * transfer_in = libusb_alloc_transfer(0);
	if (transfer_in == NULL)
	{
		set_last_err_string("Allocate transfer failure");
		return -1;
	}

	Transfer t;
	m_vector_transfer.push(t);

	size_t last = m_vector_transfer.size() - 1;
	m_vector_transfer.back().libusb_transfer = transfer_in;
	m_vector_transfer.back().buffer = new uint8_t[m_size_prerequest];
	m_vector_transfer.back().complete = new int;
	*m_vector_transfer.back().complete = 0;

	libusb_fill_bulk_transfer(transfer_in, (libusb_device_handle *)m_devhandle, m_ep_in.addr,
		m_vector_transfer.back().buffer, m_size_prerequest,
		transfer_cb, m_vector_transfer.back().complete, m_timeout);

	int r = libusb_submit_transfer(transfer_in);
	if (r)
	{
		set_last_err_string("submit transfer failure");
		return r;
	}

	return ret;
}