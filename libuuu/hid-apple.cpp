/*
* Copyright 2026 NXP.
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

#include <hidapi.h>
#include <hidapi_darwin.h>
#include <mutex>

using namespace std;

/*
 * On macOS the HID interface is owned by the IOHIDFamily kernel driver.
 * libusb can only detach it by re-enumerating the device with capture,
 * which requires root, so claiming the interface fails with
 * LIBUSB_ERROR_ACCESS for normal users. Talk to the device through the
 * IOHIDFamily driver (hidapi) instead, which needs no extra privileges.
 */
static uint32_t get_darwin_location_id(libusb_device *dev)
{
	uint8_t ports[7];
	int n = libusb_get_port_numbers(dev, ports, sizeof(ports));
	uint32_t location = (uint32_t)libusb_get_bus_number(dev) << 24;

	for (int i = 0; i < n && i < 6; i++)
		location |= (uint32_t)(ports[i] & 0xf) << (20 - 4 * i);

	return location;
}

int HIDTrans::open_hidapi(TransHandle &usb)
{
	if (usb.handle == nullptr) {
		set_last_err_string("Invalid USB device handle");
		return -1;
	}

	/*
	 * Each device runs in its own thread, but hidapi's init, enumerate and
	 * open are not thread safe, so serialize them.
	 */
	static std::mutex hid_open_mutex;
	std::lock_guard<std::mutex> lock(hid_open_mutex);

	if (hid_init()) {
		set_last_err_string("Call hid_init failure");
		return -1;
	}

	m_devhandle = usb.handle;
	libusb_device *dev = libusb_get_device((libusb_device_handle *)usb.handle);
	libusb_device_descriptor desc;
	if (libusb_get_device_descriptor(dev, &desc)) {
		set_last_err_string("failure get device descriptor");
		return -1;
	}

	const uint32_t location = get_darwin_location_id(dev);

	hid_darwin_set_open_exclusive(0);

	hid_device_info *list = hid_enumerate(desc.idVendor, desc.idProduct);
	for (hid_device_info *info = list; info && !m_hid; info = info->next)
	{
		hid_device *hid = hid_open_path(info->path);
		if (!hid)
			continue;

		uint32_t id = 0;
		if (hid_darwin_get_location_id(hid, &id) == 0 && id == location)
			m_hid = hid;
		else
			hid_close(hid);
	}
	hid_free_enumeration(list);

	if (!m_hid) {
		string_ex err;
		err.format("Failure open HID device %04x:%04x at location 0x%08x",
			   desc.idVendor, desc.idProduct, location);
		set_last_err_string(err);
		return -1;
	}

	/*
	 * IOHIDFamily queues input reports received while the device is open.
	 * Drop anything stale so the next read returns the reply to our command.
	 */
	unsigned char flush[1025];
	while (hid_read_timeout((hid_device *)m_hid, flush, sizeof(flush), 0) > 0)
		;

	return 0;
}

int HIDTrans::close()
{
	if (m_hid) {
		hid_close((hid_device *)m_hid);
		m_hid = nullptr;
	}
	return 0;
}

int HIDTrans::open(TransHandle &p)
{
	return open_hidapi(p);
}

int HIDTrans::write_simple(void *buff, size_t size)
{
	int ret;
	uint8_t *p = (uint8_t *)buff;

	ret = hid_write((hid_device *)m_hid, p, size);
	if (ret < 0)
	{
		string_ex err;
		err.format("HID(W): %ls", hid_error((hid_device *)m_hid));
		set_last_err_string(err);
		return LIBUSB_ERROR_IO;
	}
	return ret;
}

int HIDTrans::read_simple(void *buff, size_t size, size_t *rsize)
{
	int ret;

	/*
	 * hidapi returns whichever input report arrived first, while libusb
	 * callers pass the expected report ID in buff[0]. Skip reports with
	 * a different ID, e.g. ones left over from an earlier command.
	 */
	const uint8_t report_id = ((uint8_t *)buff)[0];
	do {
		ret = hid_read_timeout((hid_device *)m_hid, (unsigned char *)buff, size, m_timeout);
	} while (ret > 0 && report_id && ((uint8_t *)buff)[0] != report_id);

	if (ret < 0)
	{
		string_ex err;
		err.format("HID(R): %ls", hid_error((hid_device *)m_hid));
		set_last_err_string(err);
		return LIBUSB_ERROR_IO;
	}
	if (ret == 0)
	{
		set_last_err_string("HID(R): LIBUSB_ERROR_TIMEOUT (-7)");
		return LIBUSB_ERROR_TIMEOUT;
	}
	*rsize = ret;
	return 0;
}
