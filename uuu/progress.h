
#pragma once

#include "../libuuu/libuuu.h"
#include "../libuuu/string_man.h"

#include <stdio.h>

#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

extern int g_verbose;

// TODO should not have static vars in header file!

static std::vector<std::string> usb_serial_no_filter;
static bool start_usb_transfer;
static std::vector<std::string> usb_path_filter;

static int g_overall_status;
static int g_overall_okay;
static int g_overall_failure;
static char g_wait[] = "|/-\\";
static int g_wait_index;

static void print_oneline(std::string str)
{
	size_t w = g_vt->get_console_width();
	if (w <= 3)
		return;

	if (str.size() >= w)
	{
		str.resize(w - 1);
		str[str.size() - 1] = '.';
		str[str.size() - 2] = '.';
		str[str.size() - 3] = '.';
	}
	else
	{
		str.resize(w, ' ');
	}
	std::cout << str << std::endl;
}

static std::string build_process_bar(size_t width, size_t pos, size_t total)
{
	std::string str;
	str.resize(width, ' ');
	str[0] = '[';
	str[width - 1] = ']';

	if (total == 0)
	{
		if (pos == 0)
			return str;

		std::string loc;
		size_t s = pos / (1024 * 1024);
		format(loc, "%dM", s);
		str.replace(1, loc.size(), loc);
		return str;
	}

	size_t i;

	if (pos > total)
		pos = total;

	for (i = 1; i < (width - 2) * pos / total; i++)
	{
		str[i] = '=';
	}

	if (i > 1)
		str[i] = '>';

	if (pos == total)
		str[str.size() - 2] = '=';

	std::string per;
	format(per, "%d%%", pos * 100 / total);

	size_t start = (width - per.size()) / 2;
	str.replace(start, per.size(), per);
	str.insert(start, g_vt->yellow);
	str.insert(start + per.size() + strlen(g_vt->yellow), g_vt->default_fg);
	return str;
}

static void print_auto_scroll(std::string str, size_t len, size_t start)
{
	if (str.size() <= len)
	{
		str.resize(len, ' ');
		std::cout << str;
		return;
	}

	if (str.size())
		start = start % str.size();
	else
		start = 0;

	std::string s = str.substr(start, len);
	s.resize(len, ' ');
	std::cout << s;
}

class ShowNotify
{
public:
	std::string m_cmd;
	std::string m_dev;
	size_t m_trans_pos = 0;
	int m_status = 0;
	size_t m_cmd_total = 0;
	size_t m_cmd_index = 0;
	std::string m_last_err;
	int m_done = 0;
	size_t m_start_pos = 0;
	size_t	m_trans_size = 0;
	clock_t m_start_time;
	uint64_t m_cmd_start_time;
	uint64_t m_cmd_end_time;
	bool m_IsEmptyLine = false;

	ShowNotify() : m_start_time{ clock() } {}

	bool update(uuu_notify nt)
	{
		if (nt.type == uuu_notify::NOTIFY_DEV_ATTACH)
		{
			m_dev = nt.str;
			m_done = 0;
			m_status = 0;
		}
		if (nt.type == uuu_notify::NOTIFY_CMD_START)
		{
			m_start_pos = 0;
			m_cmd = nt.str;
			m_cmd_start_time = nt.timestamp;
		}
		if (nt.type == uuu_notify::NOTIFY_DECOMPRESS_START)
		{
			m_start_pos = 0;
			m_cmd = nt.str;
			m_cmd_start_time = nt.timestamp;
			m_dev = "Prep";
		}
		if (nt.type == uuu_notify::NOTIFY_DOWNLOAD_START)
		{
			m_start_pos = 0;
			m_cmd = nt.str;
			m_cmd_start_time = nt.timestamp;
			m_dev = "Prep";
		}
		if (nt.type == uuu_notify::NOTIFY_DOWNLOAD_END)
		{
			m_IsEmptyLine = true;
		}
		if (nt.type == uuu_notify::NOTIFY_TRANS_SIZE || nt.type == uuu_notify::NOTIFY_DECOMPRESS_SIZE)
		{
			m_trans_size = nt.total;
			return false;
		}
		if (nt.type == uuu_notify::NOTIFY_CMD_TOTAL)
		{
			m_cmd_total = nt.total;
			return false;
		}
		if (nt.type == uuu_notify::NOTIFY_CMD_INDEX)
		{
			m_cmd_index = nt.index;
			return false;
		}
		if (nt.type == uuu_notify::NOTIFY_DONE)
		{
			if (m_status)
				g_overall_failure++;
			else
				g_overall_okay++;

			m_done = 1;
		}
		if (nt.type == uuu_notify::NOTIFY_CMD_END)
		{
			m_cmd_end_time = nt.timestamp;
			if (nt.status)
			{
				g_overall_status = nt.status;
				m_last_err = uuu_get_last_err_string();
			}
			m_status |= nt.status;
			if (m_status)
				g_overall_failure++;
		}
		if (nt.type == uuu_notify::NOTIFY_TRANS_POS || nt.type == uuu_notify::NOTIFY_DECOMPRESS_POS)
		{
			if (m_trans_size == 0) {

				m_trans_pos = nt.index;
				return true;
			}

			if ((nt.index - m_trans_pos) < (m_trans_size / 100)
				&& nt.index != m_trans_size)
				return false;

			m_trans_pos = nt.index;
		}

		return true;
	}
	void print_verbose(uuu_notify* nt) const
	{
		if (this->m_dev == "Prep" && start_usb_transfer)
			return;

		if (nt->type == uuu_notify::NOTIFY_DEV_ATTACH)
		{
			std::cout << "New USB Device Attached at " << nt->str << std::endl;
		}
		if (nt->type == uuu_notify::NOTIFY_CMD_START)
		{
			std::cout << m_dev << ">" << "Start Cmd:" << nt->str << std::endl;
		}
		if (nt->type == uuu_notify::NOTIFY_CMD_END)
		{
			double diff = m_cmd_end_time - m_cmd_start_time;
			diff /= 1000;
			if (nt->status)
			{
				std::cout << m_dev << ">" << g_vt->red << "Fail " << uuu_get_last_err_string() << "(" << std::setprecision(4) << diff << "s)" << g_vt->default_fg << std::endl;
			}
			else
			{
				std::cout << m_dev << ">" << g_vt->green << "Okay (" << std::setprecision(4) << diff << "s)" << g_vt->default_fg << std::endl;
			}
		}

		if (nt->type == uuu_notify::NOTIFY_TRANS_POS || nt->type == uuu_notify::NOTIFY_DECOMPRESS_POS)
		{
			if (m_trans_size)
				std::cout << g_vt->yellow << "\r" << m_trans_pos * 100 / m_trans_size << "%" << g_vt->default_fg;
			else
				std::cout << "\r" << m_trans_pos;

			std::cout.flush();
		}

		if (nt->type == uuu_notify::NOTIFY_CMD_INFO)
			std::cout << nt->str;

		if (nt->type == uuu_notify::NOTIFY_WAIT_FOR)
			std::cout << "\r" << nt->str << " " << g_wait[((g_wait_index++) & 0x3)];

		if (nt->type == uuu_notify::NOTIFY_DECOMPRESS_START)
			std::cout << "Decompress file:" << nt->str << std::endl;

		if (nt->type == uuu_notify::NOTIFY_DOWNLOAD_START)
			std::cout << "Download file:" << nt->str << std::endl;

	}
	void print(int verbose = 0, uuu_notify* nt = NULL)
	{
		verbose ? print_verbose(nt) : print_simple();
	}
	std::string get_print_dev_string()
	{
		std::string str;
		str = m_dev;
		str.resize(12, ' ');

		std::string s;
		format(s, "%2d/%2d", m_cmd_index + 1, m_cmd_total);

		str += s;
		return str;
	}
	void print_simple()
	{
		int width = g_vt->get_console_width();
		int info, bar;
		info = 18;
		bar = 40;

		if (m_IsEmptyLine)
		{
			std::string str(width, ' ');
			std::cout << str;
			return;
		}
		if (width <= bar + info + 3)
		{
			std::string str;

			str += get_print_dev_string();

			str += g_wait[(g_wait_index++) & 0x3];

			print_oneline(str);
			return;
		}
		else
		{
			std::string str;
			str += get_print_dev_string();

			str.resize(info, ' ');
			std::cout << str;

			if (m_done || m_status)
			{
				std::string str;
				str.resize(bar, ' ');
				str[0] = '[';
				str[str.size() - 1] = ']';
				std::string err;
				if (m_status)
				{
					err = uuu_get_last_err_string();
					err.resize(bar - 2, ' ');
					str.replace(1, err.size(), err);
					str.insert(1, g_vt->red);
					str.insert(1 + strlen(g_vt->red) + err.size(), g_vt->default_fg);
				}
				else
				{
					str.replace(1, 4, "Done");
					str.insert(1, g_vt->green);
					str.insert(1 + strlen(g_vt->green) + strlen("Done"), g_vt->default_fg);
				}
				std::cout << str;
			}
			else {
				std::cout << build_process_bar(bar, m_trans_pos, m_trans_size);
			}
			std::cout << " ";
			print_auto_scroll(m_cmd, width - bar - info - 1, m_start_pos);

			if (clock() - m_start_time > CLOCKS_PER_SEC / 4)
			{
				m_start_pos++;
				m_start_time = clock();
			}
			std::cout << std::endl;

			return;
		}
	}
};

static std::map<std::string, ShowNotify> g_map_path_nt;
static std::mutex g_callback_mutex;

static ShowNotify Summary(std::map<uint64_t, ShowNotify>* np)
{
	ShowNotify sn;
	for (auto it = np->begin(); it != np->end(); it++)
	{
		if (it->second.m_dev == "Prep")
		{
			sn.m_trans_size += it->second.m_trans_size;
			sn.m_trans_pos += it->second.m_trans_pos;
		}
		else
		{
			if (it->second.m_trans_pos || it->second.m_cmd_index)
				start_usb_transfer = true; // Hidden HTTP download when USB start transfer
		}
	}

	if (start_usb_transfer)
		sn.m_IsEmptyLine = true; // Hidden HTTP download when USB start transfer

	sn.m_dev = "Prep";
	sn.m_cmd = "Http Download\\Uncompress";
	return sn;
}

static int update_progress(uuu_notify nt, void* p)
{
	std::map<uint64_t, ShowNotify>* np = (std::map<uint64_t, ShowNotify>*)p;
	std::map<std::string, ShowNotify>::iterator it;

	std::lock_guard<std::mutex> lock(g_callback_mutex);

	if ((*np)[nt.id].update(nt))
	{
		if (!(*np)[nt.id].m_dev.empty())
			if ((*np)[nt.id].m_dev != "Prep")
				g_map_path_nt[(*np)[nt.id].m_dev] = (*np)[nt.id];

		if (g_verbose)
		{
			if ((*np)[nt.id].m_dev == "Prep")
				Summary(np).print(g_verbose, &nt);
			else
				(*np)[nt.id].print(g_verbose, &nt);
		}
		else
		{
			std::string str;
			format(str, "\rSuccess %d    Failure %d    ", g_overall_okay, g_overall_failure);

			if (g_map_path_nt.empty())
				str += "Waiting for device...";

			if (!usb_path_filter.empty())
			{
				str += " at path ";
				for (size_t i = 0; i < usb_path_filter.size(); i++)
					str += usb_path_filter[i] + " ";
			}

			if (!usb_serial_no_filter.empty())
			{
				str += " at serial_no ";
				for (auto it : usb_serial_no_filter)
					str += it + "*";
			}

			print_oneline(str);
			print_oneline("");
			if ((*np)[nt.id].m_dev == "Prep" && !start_usb_transfer)
			{
				Summary(np).print();
			}
			else
				print_oneline("");

			for (it = g_map_path_nt.begin(); it != g_map_path_nt.end(); it++)
				it->second.print();

			for (size_t i = 0; i < g_map_path_nt.size() + 3; i++)
				std::cout << "\x1B[1F";

		}

		//(*np)[nt.id] = g_map_path_nt[(*np)[nt.id].m_dev];
	}

	if (nt.type == uuu_notify::NOTIFY_THREAD_EXIT)
	{
		if (np->find(nt.id) != np->end())
			np->erase(nt.id);
	}
	return EXIT_SUCCESS;
}