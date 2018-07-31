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

#include <iostream>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <iomanip>
#include <map>
#include <mutex>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#include "../libuuu/libuuu.h"

using namespace std;

int get_console_width();
void print_oneline(string str);

char g_sample_cmd_list[] = {
#include "uuu.clst"
};

vector<string> g_usb_path_filter;

int g_verbose = 0;

class AutoCursor
{
public:
	~AutoCursor()
	{
		printf("\x1b[?25h\n\n\n");
	}
};

void ctrl_c_handle(int)
{
	do {
		AutoCursor a;
	} while(0);

	exit(1);
}

class string_ex : public std::string
{
public:
	int format(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		size_t len = std::vsnprintf(NULL, 0, fmt, args);
		va_end(args);

		this->resize(len);

		va_start(args, fmt);
		std::vsnprintf((char*)c_str(), len + 1, fmt, args);
		va_end(args);

		return 0;
	}
};

void print_help()
{
	printf("uuu [-d -m -v] u-boot.imx\\flash.bin\n");
	printf("\tDownload    u-boot.imx\\flash.bin to board by usb\n");
	printf("\t -d         Deamon mode, wait for forever.\n");
	printf("\t            Start download once detect known device attached\n");
	printf("\t -v         Print build in protocal config informaiton");
	printf("\t -m USBPATH Only monitor these pathes.");
	printf("\t            -m 1:2 -m 2:3");
	printf("\n");
	printf("uuu [-d -m -v] cmdlist\n");
	printf("\tRun all commands in file cmdlist\n");
	printf("\n");
	printf("uuu [-d -m -v] SDPS: boot flash.bin\n");
	printf("\tRun command SPDS: boot flash.bin\n");
	printf("uuu -s\n");
	printf("\tEnter shell mode. uuu.inputlog record all input's command\n");
	printf("\tyou can use \"uuu uuu.inputlog\" next time to run all commands\n");
	printf("\n");

	size_t start = 0, pos = 0;
	string str= g_sample_cmd_list;

	bool bprint = false;
	while ((pos = str.find('\n',pos)) != str.npos)
	{
		string s = str.substr(start, pos - start);
		if (s.substr(0, 6) == "# ----")
			bprint = true;

		if (bprint)
		{
			if (s[0] == '#')
			{
				printf("%s\n", &(s[1]));
			}
		}
		pos += 1;
		start = pos;
	}
}
void print_version()
{
	printf("uuu (Universal Update Utility) for nxp imx chips -- %s\n\n", uuu_get_version_string());
}

int print_cfg(const char *pro, const char * chip, const char * /*compatible*/, uint16_t pid, uint16_t vid, uint16_t bcdVersion, void * /*p*/)
{
	if (bcdVersion == 0xFFFF)
		printf("\t%s\t %s\t 0x%04x\t 0x%04x\n", pro, chip, pid, vid);
	else
		printf("\t%s\t %s\t 0x%04x\t 0x%04x\t 0x%04x\n", pro, chip, pid, vid, bcdVersion);
	return 0;
}

int polling_usb(std::atomic<int>& bexit);

int g_overall_status;
int g_overall_okay;
int g_overall_failure;
char g_wait[] = "|/-\\";
int g_wait_index;

#define YELLOW "\x1B[93m"
#define DEFAULT "\x1B[0m"
#define GREEN "\x1B[92m"
#define RED	"\x1B[91m"

string build_process_bar(size_t width, size_t pos, size_t total)
{
	string str;
	str.resize(width, ' ');
	str[0] = '[';
	str[width - 1] = ']';

	if (total == 0)
		return str;

	size_t i;

	if (pos > total)
		pos = total;

	for (i = 1; i < (width-2) * pos / total; i++)
	{
		str[i] = '=';
	}

	if (i > 1)
		str[i] = '>';

	if (pos == total)
		str[str.size() - 2] = '=';

	string_ex per;
	per.format("%d%%", pos * 100 / total);

	size_t start = (width - per.size()) / 2;
	str.replace(start, per.size(), per);
	str.insert(start, YELLOW);
	str.insert(start + per.size() + strlen(YELLOW), DEFAULT);
	return str;
}

void print_auto_scroll(string str, size_t len, size_t start)
{
	if (str.size() <= len)
	{
		str.resize(len, ' ');
		cout << str;
		return;
	}

	if(str.size())
		start = start % str.size();
	else
		start = 0;

	string s = str.substr(start, len);
	s.resize(len, ' ');
	cout << s;
}
class ShowNotify
{
public:
	string m_cmd;
	string m_dev;
	size_t m_trans_pos;
	int m_status;
	size_t m_cmd_total;
	size_t m_cmd_index;
	string m_last_err;
	int m_done;
	size_t m_start_pos;
	size_t	m_trans_size;
	clock_t m_start_time;

	ShowNotify()
	{
		m_trans_size = m_trans_pos = 0;
		m_status = 0;
		m_cmd_total = 0;
		m_cmd_index = 0;
		m_done = 0;
		m_start_pos = 0;
		m_start_time = clock();
	}

	bool update(uuu_notify nt)
	{
		if (nt.type == uuu_notify::NOFITY_DEV_ATTACH)
		{
			m_dev = nt.str;
			m_done = 0;
			m_status = 0;
		}
		if (nt.type == uuu_notify::NOTIFY_CMD_START)
		{
			m_start_pos = 0;
			m_cmd = nt.str;
		}
		if (nt.type == uuu_notify::NOTIFY_TRANS_SIZE)
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
			if(nt.status)
			{
				g_overall_status = nt.status;
				m_last_err = uuu_get_last_err_string();
			}
			m_status |= nt.status;
			if (m_status)
				g_overall_failure++;
		}
		if (nt.type == uuu_notify::NOTIFY_TRANS_POS)
		{
			if (m_trans_size == 0)
				return false;

			if ((nt.index - m_trans_pos) < (m_trans_size / 100)
				&& nt.index != m_trans_size)
				return false;

			m_trans_pos = nt.index;
		}
		return true;
	}
	void print_verbose(uuu_notify*nt)
	{
		if (nt->type == uuu_notify::NOFITY_DEV_ATTACH)
		{
			cout << "New USB Device Attached at " << nt->str << endl;
		}
		if (nt->type == uuu_notify::NOTIFY_CMD_START)
		{
			cout << m_dev << ">" << "Start Cmd:" << nt->str << endl;
		}
		if (nt->type == uuu_notify::NOTIFY_CMD_END)
		{
			if (nt->status)
			{
				cout << m_dev << ">" << RED <<"Fail " << uuu_get_last_err_string() << DEFAULT << endl;
			}
			else
			{
				cout << m_dev << ">" << GREEN << "Okay" << DEFAULT << endl;
			}
		}

		if (nt->type == uuu_notify::NOTIFY_TRANS_POS)
		{
			if (m_trans_size)
				cout << YELLOW << "\r" << m_trans_pos * 100 / m_trans_size <<"%" << DEFAULT;
			else
				cout << ".";

			cout.flush();
		}

		if (nt->type == uuu_notify::NOTIFY_CMD_INFO)
			cout << nt->str;

		if (nt->type == uuu_notify::NOTIFY_WAIT_FOR)
			cout << "\r" << nt->str << " "<< g_wait[((g_wait_index++) & 0x3)];
	}
	void print(int verbose = 0, uuu_notify*nt=NULL)
	{
		verbose ? print_verbose(nt) : print_simple();
	}
	string get_print_dev_string()
	{
		string str;
		str = m_dev;
		str.resize(6, ' ');

		string_ex s;
		s.format("%2d/%2d", m_cmd_index+1, m_cmd_total);

		str += s;
		return str;
	}
	void print_simple()
	{
		int width = get_console_width();

		if (width <= 45)
		{
			string_ex str;

			str += get_print_dev_string();

			str += g_wait[(g_wait_index++) & 0x3];

			print_oneline(str);
			return ;
		}
		else
		{
			string_ex str;
			int info, bar;
			info = 14;
			bar = 30;
			str += get_print_dev_string();

			str.resize(info, ' ');
			cout << str;

			if (m_done || m_status)
			{
				string str;
				str.resize(bar, ' ');
				str[0] = '[';
				str[str.size() - 1] = ']';
				string err;
				if (m_status)
				{
					err = uuu_get_last_err_string();
					err.resize(bar - 2, ' ');
					str.replace(1, err.size(), err);
					str.insert(1, RED);
					str.insert(1 + strlen(RED) + err.size(), DEFAULT);
				}
				else
				{
					str.replace(1, 4, "Done");
					str.insert(1, GREEN);
					str.insert(1 + strlen(GREEN) + strlen("Done"), DEFAULT);
				}
				cout << str;
			} else {
				cout << build_process_bar(bar, m_trans_pos, m_trans_size);
			}
			cout << " ";
			print_auto_scroll(m_cmd, width - bar - info-1, m_start_pos);

			if(clock() - m_start_time > CLOCKS_PER_SEC/4)
			{
				m_start_pos ++;
				m_start_time = clock();
			}
			cout << endl;

			return;
		}
	}
};

static map<string, ShowNotify> g_map_path_nt;
mutex g_callback_mutex;

void print_oneline(string str)
{
	size_t w = get_console_width();
	if (str.size() >= w)
	{
		str.resize(w-1);
		str[str.size() - 1] = '.';
		str[str.size() - 2] = '.';
		str[str.size() - 3] = '.';
	}
	else
	{
		str.resize(w, ' ');
	}
	cout << str << endl;

}

int progress(uuu_notify nt, void *p)
{
	map<uint64_t, ShowNotify> *np = (map<uint64_t, ShowNotify>*)p;
	map<string, ShowNotify>::iterator it;

	std::lock_guard<std::mutex> lock(g_callback_mutex);

	if ((*np)[nt.id].update(nt))
	{
		if(!(*np)[nt.id].m_dev.empty())
			g_map_path_nt[(*np)[nt.id].m_dev] = (*np)[nt.id];

		if (g_verbose)
		{
			(*np)[nt.id].print(g_verbose, &nt);
		}
		else
		{
			string_ex str;
			str.format("Succuess %d    Failure %d", g_overall_okay, g_overall_failure);

			if (g_map_path_nt.empty())
				str += "Wait for Known USB Device Appear";

			if (!g_usb_path_filter.empty())
			{
				str += " at path ";
				for (size_t i = 0; i < g_usb_path_filter.size(); i++)
					str += g_usb_path_filter[i] + " ";
			}

			print_oneline(str);
			print_oneline("");

			for (it = g_map_path_nt.begin(); it != g_map_path_nt.end(); it++)
				it->second.print();

			for (size_t i = 0; i < g_map_path_nt.size() + 2; i++)
				cout << "\x1B[1F";
		}

		(*np)[nt.id] = g_map_path_nt[(*np)[nt.id].m_dev];
	}

	if (nt.type == uuu_notify::NOTIFY_THREAD_EXIT)
	{
		if(np->find(nt.id) != np->end())
			np->erase(nt.id);
	}
	return 0;
}
#ifdef _MSC_VER

#define DEFINE_CONSOLEV2_PROPERTIES
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

bool enable_vt_mode()
{
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		return false;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		return false;
	}
	return true;
}

int get_console_width()
{
	CONSOLE_SCREEN_BUFFER_INFO sbInfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfo);
	return sbInfo.dwSize.X;
}
#else
#include <sys/ioctl.h>
bool enable_vt_mode() { return true; }
int get_console_width()
{
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	return w.ws_col;
}
#endif
void print_usb_filter()
{
	if (!g_usb_path_filter.empty())
	{
		cout << " at path ";
		for (size_t i = 0; i < g_usb_path_filter.size(); i++)
			cout << g_usb_path_filter[i] << " ";
	}
}
int main(int argc, char **argv)
{
	AutoCursor a;

	print_version();

	enable_vt_mode();

	if (argc == 1)
		print_help();

	int deamon = 0;
	int shell = 0;
	string filename;
	string cmd;


	for (int i = 1; i < argc; i++)
	{
		string s = argv[i];
		if (!s.empty() && s[0] == '-')
		{
			if (s == "-d")
			{
				deamon = 1;
			}else if (s == "-s")
			{
				shell = 1;
				g_verbose = 1;
			}
			else if (s == "-v")
			{
				g_verbose = 1;
			}
			else if (s == "-V")
			{
				g_verbose = 1;
				uuu_set_debug_level(2);
			}
			else if (s == "-h")
			{
				print_help();
				return 0;
			}
			else if (s == "-m")
			{
				i++;
				uuu_add_usbpath_filter(argv[i]);
				g_usb_path_filter.push_back(argv[i]);
			}
			else
			{
				cout << "Unknown option: " << s.c_str();
				return -1;
			}
		}else if (!s.empty() && s[s.size() - 1] == ':')
		{
			for (int j = i; j < argc; j++)
			{
				s = argv[j];
				cmd.append(s);
				cmd.append(" ");
			}
			break;
		}
		else
		{
			filename = s;
			break;
		}
	}

	signal(SIGINT, ctrl_c_handle);

	if (deamon && shell)
	{
		printf("Error: -d -s Can't apply at the same time\n");
		return -1;
	}

	if (g_verbose)
	{
		printf("Build in config:\n");
		printf("\tPctl\tChip\tVid\tPid\tBcdVersion\n");
		printf("\t==========================================\n");
		uuu_for_each_cfg(print_cfg, NULL);
		if (!shell)
			cout << "Wait for Known USB Device Appear";

		print_usb_filter();

		printf("\n");
	}
	else {
		cout << "Wait for Known USB Device Appear";
		print_usb_filter();
		cout << "\r";
		cout << "\x1b[?25l";
	}

	map<uint64_t, ShowNotify> nt_session;

	uuu_register_notify_callback(progress, &nt_session);

	if (shell)
	{
		cout << "Please input command: " << endl;
		string cmd;
		ofstream log("uuu.inputlog", ofstream::binary);
		log << "uuu_version "
			<< ((uuu_get_version() & 0xFF0000) >> 16)
			<< "."
			<< ((uuu_get_version() & 0xFF00) >> 8)
			<< "."
			<< ((uuu_get_version() & 0xFF))
			<< endl;
		while (1)
		{
			cout << "U>";
			getline(cin, cmd);

			if (cmd == "help" || cmd == "?")
			{
				print_help();
			}
			else
			{
				log << cmd << endl;
				log.flush();

				int ret = uuu_run_cmd(cmd.c_str());
				if (ret)
					cout << uuu_get_last_err_string() << endl;
				else
					cout << "Okay" << endl;
			}
		}
		return 0;
	}

	if (!cmd.empty())
	{
		int ret;
		ret = uuu_run_cmd(cmd.c_str());

		for (size_t i = 0; i < g_map_path_nt.size()+3; i++)
			printf("\n");
		if(ret)
			printf("\nError: %s\n", uuu_get_last_err_string());
		else
			printf("Okay");

		return ret;
	}

	int ret = uuu_auto_detect_file(filename.c_str());
	if (ret)
	{
		cout << "Error:" << uuu_get_last_err_string();
		return ret;
	}
	uuu_wait_uuu_finish(deamon);

	return g_overall_status;
}
