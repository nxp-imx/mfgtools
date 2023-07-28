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
#include <mutex>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include "libusb.h"
#include "liberror.h"
#include "config.h"
#include "cmd.h"
#include "libcomm.h"
#include "libuuu.h"
#include "vector"
#include <time.h>
#include "hidapi.h"

using chrono::milliseconds;
using chrono::operator ""ms;
using chrono::seconds;
using chrono::operator ""s;

static atomic<seconds> g_wait_usb_timeout{-1s};
static atomic<milliseconds> g_usb_poll_period{200ms};
static atomic<seconds> g_wait_next_usb_timeout{-1s};

bool g_using_hidapi = true;

#ifndef WIN32
#define _strdup strdup
#endif

enum KnownDeviceState {
	NoKnownDevice,
	KnownDeviceToDo,
	KnownDeviceDone,
	WaitNextKnownDevice,
};
static atomic<KnownDeviceState> g_known_device_state{NoKnownDevice};

class CAutoDeInit
{
public:
	CAutoDeInit()
	{
		if (libusb_init(nullptr) < 0)
				throw runtime_error{ "Call libusb_init failure" };

		if (hid_init())
			throw runtime_error{ "Call hid_init failure" };
	}
	~CAutoDeInit()
	{
		libusb_exit(nullptr);
		hid_exit();
	}
} g_autoDeInit;

class CAutoList
{
public:
	libusb_device **list = nullptr;

	CAutoList(libusb_device **list)
	{
		this->list = list;
		m_rc = -1;
	}

	CAutoList(CAutoList &&other)
	{
		this->list = other.list;
		this->m_rc = other.m_rc;
		other.list = nullptr;
	}

	CAutoList()
	{
		m_rc = libusb_get_device_list(nullptr, &list);
		if (m_rc < 0) {
			set_last_err_string(std::string("libusb_get_device_list failed: ") +
							    	libusb_strerror(static_cast<libusb_error>(m_rc)));
		}
	}

	~CAutoList()
	{
		if (list != nullptr) {
			libusb_free_device_list(list, 1);
		}
	}

	CAutoList& operator=(CAutoList &&other)
	{
		this->list = other.list;
		this->m_rc = other.m_rc;
		other.list = nullptr;
		return *this;
	}

	CAutoList& operator=(const CAutoList&) = delete;	// Prevent copy, allow move only
	CAutoList(const CAutoList&) = delete;	// Prevent copy, allow move only

	bool good() const
	{
		return m_rc >= 0;
	}

private:
	int m_rc = 0;
};

class CAutoHIDList
{
public:
	struct hid_device_info* m_list;

	CAutoHIDList(struct hid_device_info* list)
	{
		m_list = list;
	}

	CAutoHIDList()
	{
		m_list = hid_enumerate(0, 0);
		if (!m_list)
			set_last_err_string("Failure get HID devices or failure at hid_enumerate()");
	}
	~CAutoHIDList()
	{
		if (m_list != nullptr) {
			hid_free_enumeration(m_list);
		}
	}

	CAutoHIDList(CAutoHIDList&& other)
	{
		this->m_list = other.m_list;
		other.m_list = nullptr;
	}

	CAutoHIDList& operator=(CAutoHIDList&& other)
	{
		this->m_list = other.m_list;
		return *this;
	}

	CAutoHIDList& operator=(const CAutoHIDList&) = delete;	// Prevent copy, allow move only
	CAutoHIDList(const CAutoHIDList&) = delete;	// Prevent copy, allow move only

};
static struct {
	vector<string> list;
	mutex lock;

	void push_back(string filter)
	{
		lock_guard<mutex> guard{lock};
		list.emplace_back(std::move(filter));
	}

	bool is_valid(const string& path)
	{
		lock_guard<mutex> guard{lock};
		if (list.empty())
			return true;

		auto end = list.end();
		auto pos = find(list.begin(), end, path);
		return pos != end;
	}
} g_filter_usbpath;

struct Timer
{
	using Clock = chrono::steady_clock;
	Clock::time_point start;

	explicit Timer(Clock::time_point start) : start{start} {}
	Timer() : Timer{Clock::now()} {}

	bool is_elapsed(Clock::duration interval) const
	{
		return (Clock::now() - start) >= interval;
	}

	void reset(Clock::time_point start)
	{
		this->start = start;
	}

	void reset()
	{
		reset(Clock::now());
	}
};

#ifdef _MSC_VER
#define TRY_SUDO
#else
#define TRY_SUDO ",Try sudo uuu"
#endif

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

static int open_libusb(libusb_device *dev, void **usb_device_handle)
{
	int retry = 1;
#ifdef WIN32
	retry = 5;
#endif

	while (retry)
	{
		retry--;

		/* work around windows open device failure 1/10
		 * sometime HID device detect need some time, refresh list
		 * to make sure HID driver installed.
		 */
		CAutoList l;

		int ret;
		if ((ret = libusb_open(dev, (libusb_device_handle **)(usb_device_handle))) < 0)
		{
			if ((ret != LIBUSB_ERROR_NOT_SUPPORTED) || (retry == 0))
			{
				set_last_err_string("Failure open usb device" TRY_SUDO);
				return -1;
			}
			this_thread::sleep_for(200ms);
		}
		else
		{
			return 0;
		}
	}

	return -1;
}

/**
 Thread function. Didn't call this function directly.
 Unbalance libusb_unref_device.
 Before start thread, need call libusb_ref_device to dev is free

 libusb_get_list()
 libusb_ref_device        // avoid free at libusb_free_list if run_usb_cmd have not open device in time.
 thread start run_usb_cmds;
 libusb_free_list()
*/
static int run_usb_cmds(ConfigItem *item, void *dev, short bcddevice)
{
	int ret;
	uuu_notify nt;
	nt.type = uuu_notify::NOTIFY_DEV_ATTACH;

	string str;

	if (is_using_hidapi() && item->m_bHID)
		str = "HID";
	else
		str = get_device_path((libusb_device*)dev);
	nt.str = (char*)str.c_str();
	call_notify(nt);

	CmdUsbCtx ctx;
	ctx.m_config_item = item;
	ctx.m_current_bcd = bcddevice;

	if (is_using_hidapi() && item->m_bHID)
	{
		ctx.m_dev = hid_open_path((char*)dev);
		ctx.m_bHIDAPI = true;
		if (!ctx.m_dev)
		{
			nt.status = -1;
			call_notify(nt);
			set_last_err_string("failure open HID devices");
			return -1;
		}
	}
	else if ((ret = open_libusb((libusb_device*)dev, &(ctx.m_dev))))
	{
		nt.type = uuu_notify::NOTIFY_CMD_END;
		nt.status = -1;
		call_notify(nt);
		return ret;
	}

	ret = run_cmds(item->m_protocol.c_str(), &ctx);
	g_known_device_state = KnownDeviceDone;

	nt.type = uuu_notify::NOTIFY_THREAD_EXIT;
	call_notify(nt);

	if (ctx.m_bHIDAPI)
	{
		hid_close((hid_device*)ctx.m_dev);
		free(dev);
	}
	else
	{
		libusb_unref_device((libusb_device*)dev); //ref_device when start thread
	}
	clear_env();
	return ret;
}

static int usb_add(libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		set_last_err_string("failure get device descriptor");
		return r;
	}

	string str;
	str = get_device_path(dev);
	if (!g_filter_usbpath.is_valid(str))
		return -1;

	ConfigItem *item = get_config()->find(desc.idVendor, desc.idProduct, desc.bcdDevice);

	if (item)
	{
		g_known_device_state = KnownDeviceToDo;

		/*
		 * start new thread, need increase dev ref number.
		 * otherwise polling thread, free_device_list free device if open device call after free_device_list. 
		 */
		libusb_ref_device(dev);

		std::thread(run_usb_cmds, item, dev, desc.bcdDevice).detach();
	}
	return 0;
}

static int usb_remove(libusb_device * /*dev*/)
{
	return 0;
}

static int usb_hid_add(hid_device_info* info)
{
#if HID_API_VERSION_MINOR >= 13
	if (info->bus_type != HID_API_BUS_USB)
		return 0;
#endif

	ConfigItem* item = get_config()->find(info->vendor_id, info->product_id, info->release_number);
	if (item)
	{
		g_known_device_state = KnownDeviceToDo;
		std::thread(run_usb_cmds, item, _strdup(info->path), info->release_number).detach();
	}
	return 0;
}

static int usb_hid_remove(hid_device_info* info)
{
	return 0;
}

void compare_list(libusb_device ** old, libusb_device **nw)
{
	libusb_device * dev;
	int i = 0;

	if (old == nullptr)
	{
		while ((dev = nw[i++]) != nullptr)
		{
			usb_add(dev);
		}
		return;
	}

	while ((dev = nw[i++]) != nullptr)
	{
		libusb_device * p;
		int j = 0;
		while ((p = old[j++]) != nullptr)
		{
			if (p == dev)
				break;//find it.
		};
		if (p != dev)
			usb_add(dev);
	}

	i = 0;
	while ((dev = old[i++]) != nullptr)
	{
		libusb_device * p;
		int j = 0;
		while ((p = nw[j++]) != nullptr)
		{
			if (p == dev)
				break;//find it.
		};
		if (p != dev)
			usb_remove(dev);
	}
}

void compare_hid_list(struct hid_device_info* old_list, struct hid_device_info* nw_list)
{
	char* dev;
	hid_device_info* old = old_list;
	hid_device_info* nw = nw_list;

	if (old == nullptr)
	{
		while (nw)
		{
			dev = nw->path;
			usb_hid_add(nw);
			nw = nw->next;
		}
		return;
	}

	while (nw)
	{
		hid_device_info* p = old_list;
		dev = nw->path;

		while (p)
		{
			if (strcmp(p->path, dev) == 0)
				break;//find it.
			p = p->next;
		};

		if (!p)
			usb_hid_add(nw);

		nw = nw->next;
	}

	old = old_list;

	while (old)
	{
		nw = nw_list;
		while (nw)
		{
			if (strcmp(nw->path, old->path) == 0)
				break;//find it.

			nw = nw->next;
		};

		if (!nw)
			usb_hid_remove(old);

		old = old->next;
	}
}

static int check_usb_timeout(Timer& usb_timer)
{
	auto known_device_state = g_known_device_state.load();
	if (known_device_state == KnownDeviceDone)
	{
		g_known_device_state = known_device_state = WaitNextKnownDevice;
		usb_timer.reset();
	}

	auto usb_timeout = g_wait_usb_timeout.load();
	if (usb_timeout >= 0s && known_device_state == NoKnownDevice)
	{
		if (usb_timer.is_elapsed(usb_timeout))
		{
			set_last_err_string("Timeout: Wait for Known USB Device");
			return -1;
		}
	}

	usb_timeout = g_wait_next_usb_timeout.load();
	if (usb_timeout >= 0s && g_known_device_state == WaitNextKnownDevice)
	{
		if (usb_timer.is_elapsed(usb_timeout))
		{
			set_last_err_string("Timeout: Wait for next USB Device");
			return -1;
		}
	}

	return 0;
}

int polling_usb(std::atomic<int>& bexit)
{
	if (run_cmds("CFG:", nullptr))
		return -1;

	Timer usb_timer;

	CAutoList oldlist(nullptr);
	CAutoHIDList oldHIDList(nullptr);

	while(!bexit)
	{
		CAutoList newlist;
		if (!newlist.good())
		{
			return -1;
		}

		compare_list(oldlist.list, newlist.list);

		std::swap(oldlist, newlist);

		if (is_using_hidapi())
		{
			CAutoHIDList newHIDList;
			compare_hid_list(oldHIDList.m_list, newHIDList.m_list);
			std::swap(oldHIDList, newHIDList);
		}

		this_thread::sleep_for(g_usb_poll_period.load());

		if (check_usb_timeout(usb_timer))
			return -1;
	}

	return 0;
}

CmdUsbCtx::~CmdUsbCtx()
{
	if (m_dev)
	{
		if (this->m_bHIDAPI)
			hid_close((hid_device*)m_dev);
		else
			libusb_close((libusb_device_handle*)m_dev);
		m_dev = 0;
	}
}

int CmdUsbCtx::look_for_match_device(const char *pro)
{
	if (run_cmds("CFG:", nullptr))
		return -1;

	Timer usb_timer;

	while (1)
	{
		CAutoList l;

		if (!l.good()) {
			break;
		}

		size_t i = 0;
		libusb_device *dev;

		while ((dev = l.list[i++]) != nullptr)
		{
			struct libusb_device_descriptor desc;
			int r = libusb_get_device_descriptor(dev, &desc);
			if (r < 0) {
				set_last_err_string("failure get device descriptor");
				return -1;
			}
			string str = get_device_path(dev);

			if (!g_filter_usbpath.is_valid(str))
				continue;

			ConfigItem *item = get_config()->find(desc.idVendor, desc.idProduct, desc.bcdDevice);

			if (is_using_hidapi() && item && item->m_bHID)
				continue;

			if (item && item->m_protocol == str_to_upper(pro))
				{
					uuu_notify nt;
					nt.type = uuu_notify::NOTIFY_DEV_ATTACH;
					m_config_item = item;
					m_current_bcd = desc.bcdDevice;

					int ret;
					if ((ret = open_libusb(dev, &(m_dev))))
						return ret;

					nt.str = (char*)str.c_str();
					call_notify(nt);

					return 0;
				}
		}

		//scan HID devices
		if (is_using_hidapi())
		{
			CAutoHIDList l;
			hid_device_info* p = l.m_list;
			while (p)
			{
#if HID_API_VERSION_MINOR >= 13
				if (p->bus_type == HID_API_BUS_USB)
#endif
				{
					ConfigItem* item = get_config()->find(p->vendor_id, p->product_id, p->release_number);

					if (item && item->m_protocol == str_to_upper(pro))
					{
						uuu_notify nt;
						nt.type = uuu_notify::NOTIFY_DEV_ATTACH;
						m_config_item = item;
						m_current_bcd = p->release_number;
						m_bHIDAPI = true;
						m_dev = hid_open_path(p->path);
						if (!m_dev)
						{
							set_last_err_string("open HID failure");
							return -1;
						}
						nt.str = (char*)"HID";
						call_notify(nt);

						return 0;
					}
				}
				p = p->next;
			}
		}

		this_thread::sleep_for(200ms);

		uuu_notify nt;
		nt.type = nt.NOTIFY_WAIT_FOR;
		nt.str = (char*)"Wait for Known USB";
		call_notify(nt);

		check_usb_timeout(usb_timer);
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
	CAutoList l;
	size_t i = 0;
	libusb_device *dev;

	if (!l.good()) {
		return -1;
	}

	while ((dev = l.list[i++]) != nullptr)
	{
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			set_last_err_string("failure get device descriptor");
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

	return 0;
}

int uuu_set_wait_timeout(int timeout_in_seconds)
{
	g_wait_usb_timeout = seconds{timeout_in_seconds};
	return 0;
}

void uuu_set_poll_period(int period_in_milliseconds)
{
	g_usb_poll_period = milliseconds{period_in_milliseconds};
}

int uuu_set_wait_next_timeout(int timeout_in_seconds)
{
	g_wait_next_usb_timeout = seconds{timeout_in_seconds};
	return 0;
}

void uuu_set_using_hidapi(uint32_t val)
{
	g_using_hidapi = !!val;
}

bool is_using_hidapi()
{
	return g_using_hidapi;
}
