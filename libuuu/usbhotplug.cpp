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

static vector<thread> g_running_thread;



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

	if (libusb_open(dev, (libusb_device_handle **)&(ctx.m_dev)) < 0)
	{
		set_last_err_string("Failure open usb device");
		nt.type = uuu_notify::NOTIFY_CMD_END;
		nt.status = -1;
		call_notify(nt);

		return -1;
	}

	ret = run_cmds(item->m_protocol.c_str(), &ctx);

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

	ConfigItem *item = get_config()->find(desc.idVendor, desc.idProduct, desc.bcdDevice);
	if (item)
	{
		g_running_thread.push_back(std::thread(run_usb_cmds, item, dev));
	}
	return 0;
}

static int usb_remove(libusb_device *dev)
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

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if(newlist)
		libusb_free_device_list(newlist, 1);

	return 0;
}

int CmdUsbCtx::look_for_match_device(const char *pro)
{
	if (libusb_init(NULL) < 0)
	{
		set_last_err_string("Call libusb_init failure");
		return -1;
	}
	
	while (1)
	{
		libusb_device **newlist = NULL;
		ssize_t sz = libusb_get_device_list(NULL, &newlist);
		size_t i = 0;
		libusb_device *dev;

		while ((dev = newlist[i++]) != NULL)
		{
			struct libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(dev, &desc);
			if (r < 0) {
				set_last_err_string("failure get device descrior");
				return NULL;
			}

			ConfigItem *item = get_config()->find(desc.idVendor, desc.idProduct, desc.bcdDevice);
			if (item && item->m_protocol == pro)
				{
					uuu_notify nt;
					nt.type = uuu_notify::NOFITY_DEV_ATTACH;

					string str = get_device_path(dev);

					m_config_item = item;

					if (libusb_open(dev, (libusb_device_handle **)&(m_dev)) < 0)
					{
						set_last_err_string("Failure open usb device");
						return -1;
					}

					nt.str = (char*)str.c_str();
					call_notify(nt);

					return 0;
				}
		}

		libusb_free_device_list(newlist, 1);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	return -1;
}