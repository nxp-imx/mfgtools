#pragma once
//! @file

#include "../libuuu/libuuu.h"
#include "../libuuu/string_man.h"

#include <stdio.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

extern int g_verbose;

class TransferNotifyItem;

class TransferContext final
{
public:
	std::map<std::string, TransferNotifyItem> map_path_nt;
	std::vector<std::string> usb_serial_no_filter;
	std::vector<std::string> usb_path_filter;
	bool start_usb_transfer = false;
	int overall_status = 0;
	int success_count = 0;
	int failure_count = 0;

	void print_oneline(std::string str) const
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
};

extern TransferContext g_transfer_context;

class TransferNotifyItem
{
	static constexpr const char* wait_chars = "|/-\\";
	int wait_index;
	int m_status = 0;
	size_t m_cmd_total = 0;
	std::string m_last_err;
	int m_done = 0;
	size_t m_start_pos = 0;
	clock_t m_start_time = 0;
	uint64_t m_cmd_start_time = 0;
	uint64_t m_cmd_end_time = 0;

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
			string_man::format(loc, "%dM", s);
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
		string_man::format(per, "%d%%", pos * 100 / total);

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

	void render_verbose(const uuu_notify& nt)
	{
		if (m_dev == "Prep" && g_transfer_context.start_usb_transfer)
			return;

		if (nt.type == uuu_notify::NOTIFY_DEV_ATTACH)
		{
			std::cout << "New USB Device Attached at " << nt.str << std::endl;
		}
		if (nt.type == uuu_notify::NOTIFY_CMD_START)
		{
			std::cout << m_dev << ">" << "Start Cmd:" << nt.str << std::endl;
		}
		if (nt.type == uuu_notify::NOTIFY_CMD_END)
		{
			double diff = (double)(m_cmd_end_time - m_cmd_start_time);
			diff /= 1000;
			if (nt.status)
			{
				std::cout << m_dev << ">" << g_vt->red << "Fail " << uuu_get_last_err_string() << "(" << std::setprecision(4) << diff << "s)" << g_vt->default_fg << std::endl;
			}
			else
			{
				std::cout << m_dev << ">" << g_vt->green << "Okay (" << std::setprecision(4) << diff << "s)" << g_vt->default_fg << std::endl;
			}
		}

		if (nt.type == uuu_notify::NOTIFY_TRANS_POS || nt.type == uuu_notify::NOTIFY_DECOMPRESS_POS)
		{
			if (m_trans_size)
				std::cout << g_vt->yellow << "\r" << m_trans_pos * 100 / m_trans_size << "%" << g_vt->default_fg;
			else
				std::cout << "\r" << m_trans_pos;

			std::cout.flush();
		}

		if (nt.type == uuu_notify::NOTIFY_CMD_INFO)
			std::cout << nt.str;

		if (nt.type == uuu_notify::NOTIFY_WAIT_FOR)
			std::cout << "\r" << nt.str << " " << wait_chars[((wait_index++) & 0x3)];

		if (nt.type == uuu_notify::NOTIFY_DECOMPRESS_START)
			std::cout << "Decompress file:" << nt.str << std::endl;

		if (nt.type == uuu_notify::NOTIFY_DOWNLOAD_START)
			std::cout << "Download file:" << nt.str << std::endl;

	}

	void render_simple()
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

			str += wait_chars[(wait_index++) & 0x3];

			g_transfer_context.print_oneline(str);
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

public:
	std::string m_cmd;
	std::string m_dev;
	size_t m_trans_pos = 0;
	size_t m_cmd_index = 0;
	size_t	m_trans_size = 0;
	bool m_IsEmptyLine = false;

	TransferNotifyItem() : m_start_time{clock()} {}

	std::string get_print_dev_string() const
	{
		std::string str = m_dev;
		str.resize(12, ' ');

		std::string s;
		string_man::format(s, "%2d/%2d", m_cmd_index + 1, m_cmd_total);

		str += s;
		return str;
	}

	bool update(const uuu_notify& nt)
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
				g_transfer_context.failure_count++;
			else
				g_transfer_context.success_count++;

			m_done = 1;
		}
		if (nt.type == uuu_notify::NOTIFY_CMD_END)
		{
			m_cmd_end_time = nt.timestamp;
			if (nt.status)
			{
				g_transfer_context.overall_status = nt.status;
				m_last_err = uuu_get_last_err_string();
			}
			m_status |= nt.status;
			if (m_status)
				g_transfer_context.failure_count++;
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

	void render(const uuu_notify& nt)
	{
		g_verbose ? render_verbose(nt) : render_simple();
	}

	void render()
	{
		render_simple();
	}
};

class TransferFeedback final
{
	std::map<uint64_t, TransferNotifyItem> nt_session;
	std::mutex callback_mutex;

	TransferNotifyItem get_summary()
	{
		TransferNotifyItem sn;
		for (auto it = nt_session.begin(); it != nt_session.end(); it++)
		{
			if (it->second.m_dev == "Prep")
			{
				sn.m_trans_size += it->second.m_trans_size;
				sn.m_trans_pos += it->second.m_trans_pos;
			}
			else
			{
				if (it->second.m_trans_pos || it->second.m_cmd_index)
					g_transfer_context.start_usb_transfer = true; // Hidden HTTP download when USB start transfer
			}
		}

		if (g_transfer_context.start_usb_transfer)
			sn.m_IsEmptyLine = true; // Hidden HTTP download when USB start transfer

		sn.m_dev = "Prep";
		sn.m_cmd = "Http Download\\Uncompress";
		return sn;
	}

	// TODO make param const
	void update(const uuu_notify& nt)
	{
		std::map<std::string, TransferNotifyItem>::iterator it;

		std::lock_guard<std::mutex> lock(callback_mutex);

		if (nt_session[nt.id].update(nt))
		{
			if (!nt_session[nt.id].m_dev.empty())
				if (nt_session[nt.id].m_dev != "Prep")
					g_transfer_context.map_path_nt[nt_session[nt.id].m_dev] = nt_session[nt.id];

			if (g_verbose)
			{
				if (nt_session[nt.id].m_dev == "Prep")
					get_summary().render(nt);
				else
					nt_session[nt.id].render(nt);
			}
			else
			{
				std::string str;
				string_man::format(str, "\rSuccess %d    Failure %d    ", g_transfer_context.success_count, g_transfer_context.failure_count);

				if (g_transfer_context.map_path_nt.empty())
					str += "Waiting for device...";

				if (!g_transfer_context.usb_path_filter.empty())
				{
					str += " at path ";
					for (size_t i = 0; i < g_transfer_context.usb_path_filter.size(); i++)
						str += g_transfer_context.usb_path_filter[i] + " ";
				}

				if (!g_transfer_context.usb_serial_no_filter.empty())
				{
					str += " at serial_no ";
					for (auto it : g_transfer_context.usb_serial_no_filter)
						str += it + "*";
				}

				g_transfer_context.print_oneline(str);
				g_transfer_context.print_oneline("");
				if (nt_session[nt.id].m_dev == "Prep" && !g_transfer_context.start_usb_transfer)
				{
					get_summary().render();
				}
				else
					g_transfer_context.print_oneline("");

				for (it = g_transfer_context.map_path_nt.begin(); it != g_transfer_context.map_path_nt.end(); it++)
					it->second.render();

				for (size_t i = 0; i < g_transfer_context.map_path_nt.size() + 3; i++)
					std::cout << "\x1B[1F";

			}

			//nt_session[nt.id] = g_map_path_nt[nt_session[nt.id].m_dev];
		}

		if (nt.type == uuu_notify::NOTIFY_THREAD_EXIT)
		{
			if (nt_session.find(nt.id) != nt_session.end())
				nt_session.erase(nt.id);
		}
	}

	static int update(uuu_notify nt, void* p)
	{
		auto p_progress = (TransferFeedback*)p;
		p_progress->update(nt);
		return EXIT_SUCCESS;
	}

public:
	void enable() const
	{
		uuu_register_notify_callback(update, (void*)&nt_session);
	}

	void disable() const
	{
		uuu_unregister_notify_callback(update);
	}
};