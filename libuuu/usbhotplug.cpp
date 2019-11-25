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
 * Windows libusb don't support hotplug yet
 * Will polling devices list every 100ms
 */

#include <thread>
#include <atomic>
#include "libusb.h"
#include "liberror.h"
#include "config.h"
#include "cmd.h"
#include "libcomm.h"
#include "libuuu.h"
#include "vector"
#include <time.h>

static vector<thread> g_running_thread;

static vector<string> g_filter_usbpath;

static int g_wait_usb_timeout = -1;
static int g_usb_poll_period = 0;

static int g_known_device_appeared;

#ifdef _MSC_VER
#define TRY_SUDO
#else
#define TRY_SUDO ",Try sudo uuu"
#endif

static bool is_match_filter(string path)
{
	if (g_filter_usbpath.empty())
		return true;
	for (size_t i = 0; i < g_filter_usbpath.size(); i++)
		if (g_filter_usbpath[i] == path)
			return true;

	return false;
}

static string get_device_path(libusb_device *dev)
{
	uint8_t path[8];

	int bus = libusb_get_bus_number(dev);

	string_ex str;

	str.format("%d:", bus);

	int ret = libusb_get_port_numbers(dev, path, sizeof(path));
	if (ret < 0)
		return "";

	string_ex s;
	s.format("%d", path[0]);
	str.append(s);

	for (int j = 1; j < ret; j++)
	{
		s.format("%d", path[j]);
		str.append(s);
	}
	return str;
}

static int run_usb_cmds(ConfigItem *item, libusb_device *dev)
{
	int ret;
	uuu_notify nt;
	nt.type = uuu_notify::NOFITY_DEV_ATTACH;

	string str;
	str = get_device_path(dev);
	nt.str = (char*)str.c_str();
	call_notify(nt);

	CmdUsbCtx ctx;
	ctx.m_config_item = item;

	/* work around windows open device failure 1/10
	 * sometime HID device detect need some time, refresh list
	 * to make sure HID driver installed.
	 */
	libusb_device **list = NULL;
	libusb_get_device_list(NULL, &list);

	if (libusb_open(dev, (libusb_device_handle **)&(ctx.m_dev)) < 0)
	{
		set_last_err_string("Failure open usb device" TRY_SUDO);
		nt.type = uuu_notify::NOTIFY_CMD_END;
		nt.status = -1;
		call_notify(nt);
		libusb_free_device_list(list, 1);
		return -1;
	}

	ret = run_cmds(item->m_protocol.c_str(), &ctx);

	libusb_free_device_list(list, 1);

	nt.type = uuu_notify::NOTIFY_THREAD_EXIT;
	call_notify(nt);

	return ret;
}


static int usb_add(libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		set_last_err_string("failure get device descrior");
		return r;
	}

	string str;
	str = get_device_path(dev);
	if (!is_match_filter(str))
		return -1;

	ConfigItem *item = get_config()->find(desc.idVendor, desc.idProduct, desc.bcdDevice);
	int poll = g_usb_poll_period ? g_usb_poll_period : 200;
	std::this_thread::sleep_for(std::chrono::milliseconds(poll));

	if (item)
	{
		g_known_device_appeared = 1;
		std::thread(run_usb_cmds, item, dev).detach();
	}
	return 0;
}

static int usb_remove(libusb_device * /*dev*/)
{

	return 0;
}

void compare_list(libusb_device ** old, libusb_device **nw)
{
	libusb_device * dev;
	int i = 0;

	if (old == NULL)
	{
		while ((dev = nw[i++]) != NULL)
		{
			usb_add(dev);
		}
		return;
	}

	while ((dev = nw[i++]) != NULL)
	{
		libusb_device * p;
		int j = 0;
		while ((p = old[j++]) != NULL)
		{
			if (p == dev)
				break;//find it.
		};
		if (p != dev)
			usb_add(dev);
	}

	i = 0;
	while ((dev = old[i++]) != NULL)
	{
		libusb_device * p;
		int j = 0;
		while ((p = nw[j++]) != NULL)
		{
			if (p == dev)
				break;//find it.
		};
		if (p != dev)
			usb_remove(dev);
	}
}

int polling_usb(std::atomic<int>& bexit)
{
	libusb_device **oldlist = NULL;
	libusb_device **newlist = NULL;

	if (libusb_init(NULL) < 0)
	{
		set_last_err_string("Call libusb_init failure");
		return -1;
	}

	libusb_set_debug(NULL, get_libusb_debug_level());

	if (run_cmds("CFG:", NULL))
		return -1;

	time_t start = time(0);

	while(!bexit)
	{
		ssize_t sz = libusb_get_device_list(NULL, &newlist);
		if (sz < 0)
		{
			set_last_err_string("Call libusb_get_device_list failure");
			return -1;
		}

		compare_list(oldlist, newlist);

		if (oldlist)
			libusb_free_device_list(oldlist, 1);

		oldlist = newlist;

		int poll = g_usb_poll_period ? g_usb_poll_period : 200;
		std::this_thread::sleep_for(std::chrono::milliseconds(poll));

		if (g_wait_usb_timeout >= 0 && !g_known_device_appeared)
		{
			if (difftime(time(0), start) >= g_wait_usb_timeout)
			{
				set_last_err_string("Timeout: Wait for Known USB Device");
				return -1;
			}
		}
	}

	if(newlist)
		libusb_free_device_list(newlist, 1);

	return 0;
}

CmdUsbCtx::~CmdUsbCtx()
{
	if (m_dev)
	{
		libusb_close((libusb_device_handle*)m_dev);
		m_dev = 0;
	}
}

int CmdUsbCtx::look_for_match_device(const char *pro)
{
	if (libusb_init(NULL) < 0)
	{
		set_last_err_string("Call libusb_init failure");
		return -1;
	}

	libusb_set_debug(NULL, get_libusb_debug_level());

	if (run_cmds("CFG:", NULL))
		return -1;

	time_t start = time(0);

	while (1)
	{
		libusb_device **newlist = NULL;
		libusb_get_device_list(NULL, &newlist);
		size_t i = 0;
		libusb_device *dev;

		while ((dev = newlist[i++]) != NULL)
		{
			struct libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(dev, &desc);
			if (r < 0) {
				set_last_err_string("failure get device descrior");
				return -1;
			}
			string str = get_device_path(dev);

			if (!is_match_filter(str))
				continue;

			ConfigItem *item = get_config()->find(desc.idVendor, desc.idProduct, desc.bcdDevice);
			if (item && item->m_protocol == str_to_upper(pro))
				{
					uuu_notify nt;
					nt.type = uuu_notify::NOFITY_DEV_ATTACH;
					m_config_item = item;

					/* work around windows open device failure 1/10
					 * sometime HID device detect need some time, refresh list
					 * to make sure HID driver installed.
					 */
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					libusb_device **list = NULL;
					libusb_get_device_list(NULL, &list);

					if (libusb_open(dev, (libusb_device_handle **)&(m_dev)) < 0)
					{
						set_last_err_string("Failure open usb device" TRY_SUDO);
						libusb_free_device_list(list, 1);
						return -1;
					}

					libusb_free_device_list(list, 1);
					nt.str = (char*)str.c_str();
					call_notify(nt);

					return 0;
				}
		}

		libusb_free_device_list(newlist, 1);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		uuu_notify nt;
		nt.type = nt.NOTIFY_WAIT_FOR;
		nt.str = (char*)"Wait for Known USB";
		call_notify(nt);

		if (g_wait_usb_timeout >= 0)
		{
			if (difftime(time(0), start) >= g_wait_usb_timeout)
			{
				set_last_err_string("Timeout: Wait for USB Device Appear");
				return -1;
			}
		}
	}

	return -1;
}

int uuu_add_usbpath_filter(const char *path)
{
	g_filter_usbpath.push_back(path);
	return 0;
}

int uuu_for_each_devices(uuu_ls_usb_devices fn, void *p)
{
	if (libusb_init(NULL) < 0)
	{
		set_last_err_string("Call libusb_init failure");
		return -1;
	}

	libusb_device **newlist = NULL;
	libusb_get_device_list(NULL, &newlist);
	size_t i = 0;
	libusb_device *dev;

	while ((dev = newlist[i++]) != NULL)
	{
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			set_last_err_string("failure get device descrior");
			return -1;
		}
		string str = get_device_path(dev);

		ConfigItem *item = get_config()->find(desc.idVendor, desc.idProduct, desc.bcdDevice);
		if (item)
		{
			if (fn(str.c_str(), item->m_chip.c_str(), item->m_protocol.c_str(), desc.idVendor, desc.idProduct, desc.bcdDevice, p))
			{
				set_last_err_string("call back return error");
				return -1;
			}
		}
	}

	libusb_free_device_list(newlist, 1);
	libusb_exit(NULL);

	return 0;
}

int uuu_set_wait_timeout(int second)
{
	g_wait_usb_timeout = second;
	return 0;
}

void uuu_set_poll_period(int msecond)
{
	g_usb_poll_period = msecond;
}
