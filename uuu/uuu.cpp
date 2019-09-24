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
#include "buildincmd.h"
#include <string>
#include <streambuf>

#include "../libuuu/libuuu.h"

char * g_vt_yellow = (char*)"\x1B[93m";
char * g_vt_default = (char*) "\x1B[0m";
char * g_vt_green = (char*)"\x1B[92m";
char * g_vt_red = (char*)"\x1B[91m";
char * g_vt_kcyn = (char*)"\x1B[36m";
char * g_vt_boldwhite = (char*)"\x1B[97m";

void clean_vt_color()
{
	g_vt_yellow = (char*)"";
	g_vt_default = g_vt_yellow;
	g_vt_green = g_vt_yellow;
	g_vt_red = g_vt_yellow;
	g_vt_kcyn = g_vt_yellow;
	g_vt_boldwhite = g_vt_yellow;
}

using namespace std;

int get_console_width();
void print_oneline(string str);
int auto_complete(int argc, char**argv);
void print_autocomplete_help();

char g_sample_cmd_list[] = {
#include "uuu.clst"
};

vector<string> g_usb_path_filter;

int g_verbose = 0;
static bool g_start_usb_transfer;

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

void print_help(bool detail = false)
{
	const char help[] =
		"uuu [-d -m -v -V] <" "bootloader|cmdlists|cmd" ">\n\n"
		"    bootloader  download bootloader to board by usb\n"
		"    cmdlist     run all commands in cmdlist file\n"
		"                If it is path, search uuu.auto in dir\n"
		"                If it is zip, search uuu.auto in zip\n"
		"    cmd         Run one command, use -H see detail\n"
		"                example: SDPS: boot -f flash.bin\n"
		"    -d          Daemon mode, wait for forever.\n"
		"    -v -V       verbose mode, -V enable libusb error\\warning info\n"
		"    -dry	 Dry run mode, check if script or cmd correct \n"
		"    -m          USBPATH Only monitor these paths.\n"
		"                    -m 1:2 -m 1:3\n\n"
		"    -t          Timeout second for wait known usb device appeared\n"
		"uuu -s          Enter shell mode. uuu.inputlog record all input commands\n"
		"                you can use \"uuu uuu.inputlog\" next time to run all commands\n\n"
		"uuu -udev       linux: show udev rule to avoid sudo each time \n"
		"uuu -lsusb      List connected know devices\n"
		"uuu -h -H       show help, -H means detail helps\n\n";
	printf("%s", help);
	printf("uuu [-d -m -v] -b[run] ");
	g_BuildScripts.ShowCmds();
	printf(" arg...\n");
	printf("\tRun Built-in scripts\n");
	g_BuildScripts.ShowAll();
	printf("\nuuu -bshow ");
	g_BuildScripts.ShowCmds();
	printf("\n");
	printf("\tShow built-in script\n");
	printf("\n");

	print_autocomplete_help();

	if (detail == false)
		return;

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

int print_cfg(const char *pro, const char * chip, const char * /*compatible*/, uint16_t pid, uint16_t vid, uint16_t bcdmin, uint16_t bcdmax, void * /*p*/)
{
	const char *ext;
	if (strlen(chip) >= 7)
		ext = "";
	else
		ext = "\t";

	if (bcdmin == 0 && bcdmax == 0xFFFF)
		printf("\t%s\t %s\t%s 0x%04x\t 0x%04x\n", pro, chip, ext, pid, vid);
	else
		printf("\t%s\t %s\t%s 0x%04x\t 0x%04x\t [0x%04x..0x%04x]\n", pro, chip, ext, pid, vid, bcdmin, bcdmax);
	return 0;
}

int print_udev_rule(const char *pro, const char * chip, const char * /*compatible*/, uint16_t vid, uint16_t pid, uint16_t bcdmin, uint16_t bcdmax, void * /*p*/)
{
	printf("SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"%04x\", ATTRS{idProduct}==\"%04x\", MODE=\"0666\"\n",
			vid, pid);
	return 0;
}

int polling_usb(std::atomic<int>& bexit);

int g_overall_status;
int g_overall_okay;
int g_overall_failure;
char g_wait[] = "|/-\\";
int g_wait_index;


string build_process_bar(size_t width, size_t pos, size_t total)
{
	string str;
	str.resize(width, ' ');
	str[0] = '[';
	str[width - 1] = ']';

	if (total == 0)
	{
		if (pos == 0)
			return str;

		string_ex loc;
		size_t s = pos / (1024 * 1024);
		loc.format("%dM", s);
		str.replace(1, loc.size(), loc);
		return str;
	}

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
	str.insert(start, g_vt_yellow);
	str.insert(start + per.size() + strlen(g_vt_yellow), g_vt_default);
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
	uint64_t m_cmd_start_time;
	uint64_t m_cmd_end_time;
	bool m_IsEmptyLine;

	ShowNotify()
	{
		m_trans_size = m_trans_pos = 0;
		m_status = 0;
		m_cmd_total = 0;
		m_cmd_index = 0;
		m_done = 0;
		m_start_pos = 0;
		m_IsEmptyLine = false;
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
			if(nt.status)
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
	void print_verbose(uuu_notify*nt)
	{
		if (this->m_dev == "Prep" && g_start_usb_transfer)
			return;

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
			double diff = m_cmd_end_time - m_cmd_start_time;
			diff /= 1000;
			if (nt->status)
			{
				cout << m_dev << ">" << g_vt_red <<"Fail " << uuu_get_last_err_string() << "("<< std::setprecision(4) << diff << "s)" <<  g_vt_default << endl;
			}
			else
			{
				cout << m_dev << ">" << g_vt_green << "Okay ("<< std::setprecision(4) << diff << "s)" << g_vt_default << endl;
			}
		}

		if (nt->type == uuu_notify::NOTIFY_TRANS_POS || nt->type == uuu_notify::NOTIFY_DECOMPRESS_POS)
		{
			if (m_trans_size)
				cout << g_vt_yellow << "\r" << m_trans_pos * 100 / m_trans_size <<"%" << g_vt_default;
			else
				cout << "\r" << m_trans_pos;

			cout.flush();
		}

		if (nt->type == uuu_notify::NOTIFY_CMD_INFO)
			cout << nt->str;

		if (nt->type == uuu_notify::NOTIFY_WAIT_FOR)
			cout << "\r" << nt->str << " "<< g_wait[((g_wait_index++) & 0x3)];

		if (nt->type == uuu_notify::NOTIFY_DECOMPRESS_START)
			cout << "Decompress file:" << nt->str << endl;

		if (nt->type == uuu_notify::NOTIFY_DOWNLOAD_START)
			cout << "Download file:" << nt->str << endl;

	}
	void print(int verbose = 0, uuu_notify*nt=NULL)
	{
		verbose ? print_verbose(nt) : print_simple();
	}
	string get_print_dev_string()
	{
		string str;
		str = m_dev;
		str.resize(8, ' ');

		string_ex s;
		s.format("%2d/%2d", m_cmd_index+1, m_cmd_total);

		str += s;
		return str;
	}
	void print_simple()
	{
		int width = get_console_width();
		int info, bar;
		info = 14;
		bar = 40;

		if (m_IsEmptyLine)
		{
			string str(width, ' ');
			cout << str;
			return;
		}
		if (width <= bar + info + 3)
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
					str.insert(1, g_vt_red);
					str.insert(1 + strlen(g_vt_red) + err.size(), g_vt_default);
				}
				else
				{
					str.replace(1, 4, "Done");
					str.insert(1, g_vt_green);
					str.insert(1 + strlen(g_vt_green) + strlen("Done"), g_vt_default);
				}
				cout << str;
			} else {
				cout << build_process_bar(bar, m_trans_pos, m_trans_size);
			}
			cout << " ";
			print_auto_scroll(m_cmd, width - bar - info-1, m_start_pos);

if (clock() - m_start_time > CLOCKS_PER_SEC / 4)
{
	m_start_pos++;
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
	cout << str << endl;

}

ShowNotify Summary(map<uint64_t, ShowNotify> *np)
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
				g_start_usb_transfer = true; // Hidden HTTP download when USB start transfer
		}
	}

	if(g_start_usb_transfer)
		sn.m_IsEmptyLine = true; // Hidden HTTP download when USB start transfer

	sn.m_dev = "Prep";
	sn.m_cmd = "Http Download\\Uncompress";
	return sn;
}

int progress(uuu_notify nt, void *p)
{
	map<uint64_t, ShowNotify> *np = (map<uint64_t, ShowNotify>*)p;
	map<string, ShowNotify>::iterator it;

	std::lock_guard<std::mutex> lock(g_callback_mutex);

	if ((*np)[nt.id].update(nt))
	{
		if (!(*np)[nt.id].m_dev.empty())
			if ((*np)[nt.id].m_dev != "Prep")
				g_map_path_nt[(*np)[nt.id].m_dev] = (*np)[nt.id];

		if (g_verbose)
		{
			if((*np)[nt.id].m_dev == "Prep")
				Summary(np).print(g_verbose, &nt);
			else
				(*np)[nt.id].print(g_verbose, &nt);
		}
		else
		{
			string_ex str;
			str.format("\rSuccess %d    Failure %d    ", g_overall_okay, g_overall_failure);

			if (g_map_path_nt.empty())
				str += "Wait for Known USB Device Appear...";

			if (!g_usb_path_filter.empty())
			{
				str += " at path ";
				for (size_t i = 0; i < g_usb_path_filter.size(); i++)
					str += g_usb_path_filter[i] + " ";
			}

			print_oneline(str);
			print_oneline("");
			if ((*np)[nt.id].m_dev == "Prep" && !g_start_usb_transfer)
			{
				Summary(np).print();
			}else
				print_oneline("");

			for (it = g_map_path_nt.begin(); it != g_map_path_nt.end(); it++)
				it->second.print();

			for (size_t i = 0; i < g_map_path_nt.size() + 3; i++)
				cout << "\x1B[1F";

		}

		//(*np)[nt.id] = g_map_path_nt[(*np)[nt.id].m_dev];
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
		clean_vt_color();
		return false;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		clean_vt_color();
		return false;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		clean_vt_color();
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

int runshell(int shell)
{
	int uboot_cmd = 0;
	string prompt = "U>";

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
			cout << prompt;
			getline(cin, cmd);

			if (cmd == "uboot")
			{
				uboot_cmd = 1;
				prompt = "=>";
				cout << "Enter into u-boot cmd mode" << endl;
				cout << "Okay" << endl;
			}
			else if (cmd == "exit" && uboot_cmd == 1)
			{
				uboot_cmd = 0;
				prompt = "U>";
				cout << "Exit u-boot cmd mode" << endl;
				cout << "Okay" << endl;
			}else if (cmd == "help" || cmd == "?")
			{
				print_help();
			}
			else if (cmd == "q" || cmd == "quit")
			{
				return 0;
			}
			else
			{
				log << cmd << endl;
				log.flush();

				if (uboot_cmd)
					cmd = "fb: ucmd " + cmd;

				int ret = uuu_run_cmd(cmd.c_str(), 0);
				if (ret)
					cout << uuu_get_last_err_string() << endl;
				else
					cout << "Okay" << endl;
			}
		}
		return 0;
	}

	return -1;
}

void print_udev()
{
	uuu_for_each_cfg(print_udev_rule, NULL);
	fprintf(stderr, "\n1: put above udev run into /etc/udev/rules.d/99-uuu.rules\n");
	fprintf(stderr, "\tsudo sh -c \"uuu -udev >> /etc/udev/rules.d/99-uuu.rules\"\n");
	fprintf(stderr, "2: update udev rule\n");
	fprintf(stderr, "\tsudo udevadm control --reload-rules\n");
}

int print_usb_device(const char *path, const char *chip, const char *pro, uint16_t vid, uint16_t pid, uint16_t bcd, void *p)
{
	printf("\t%s\t %s\t %s\t 0x%04X\t0x%04X\t 0x%04X\n", path, chip, pro, vid, pid, bcd);
	return 0;
}

void print_lsusb()
{
	cout << "Connected Known USB Devices\n";
	printf("\tPath\t Chip\t Pro\t Vid\t Pid\t BcdVersion\n");
	printf("\t==================================================\n");

	uuu_for_each_devices(print_usb_device, NULL);
}

int main(int argc, char **argv)
{
	if (auto_complete(argc, argv) == 0)
		return 0;

	if (argc >= 2)
	{
		string s = argv[1];
		if(s == "-udev")
		{
			print_udev();
			return 0;
		}
	}

	AutoCursor a;

	print_version();

	if (!enable_vt_mode())
	{
		cout << "Your console don't support VT mode, fail back to verbose mode" << endl;
		g_verbose = 1;
	}

	if (argc == 1)
	{
		print_help();
		return 0;
	}

	int deamon = 0;
	int shell = 0;
	string filename;
	string cmd;
	int ret;
	int dryrun  = 0;

	string cmd_script;

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
			}else if (s == "-dry")
			{
				dryrun = 1;
				g_verbose = 1;
			}
			else if (s == "-h")
			{
				print_help(false);
				return 0;
			}
			else if (s == "-H")
			{
				print_help(true);
				return 0;
			}
			else if (s == "-m")
			{
				i++;
				uuu_add_usbpath_filter(argv[i]);
				g_usb_path_filter.push_back(argv[i]);
			}
			else if (s == "-t")
			{
				i++;
				uuu_set_wait_timeout(atoll(argv[i]));
			}
			else if (s == "-lsusb")
			{
				print_lsusb();
				return 0;
			}
			else if (s == "-b" || s == "-brun")
			{
				if (i + 1 == argc)
				{
					printf("error, must be have script name: ");
					g_BuildScripts.ShowCmds();
					printf("\n");
					return -1;
				}

				vector<string> args;
				for (int j = i + 2; j < argc; j++)
				{
					string s = argv[j];
					if (s.find(' ') != string::npos)
					{
						s.insert(s.begin(), '"');
						s.insert(s.end(), '"');
					}
					args.push_back(s);
				}

				// if script name is not build-in, try to look for a file
				if (g_BuildScripts.find(argv[i + 1]) == g_BuildScripts.end()) {
					BuildCmd tmpCmd;
					string tmpCmdFileName = argv[i + 1];
					tmpCmd.m_cmd = tmpCmdFileName.c_str();

					size_t filesize;

					std::ifstream t(tmpCmdFileName);
					std::string fileContents((std::istreambuf_iterator<char>(t)),
						std::istreambuf_iterator<char>());

					if (fileContents.empty()) {
						printf("%s is not built-in script or fail load external script file", tmpCmdFileName.c_str());
						return -1;
					}

					tmpCmd.m_buildcmd = fileContents.c_str();

					tmpCmd.m_desc = "Script loaded from file";

					BuildInScript tmpBuildInScript(&tmpCmd);
					g_BuildScripts[tmpCmdFileName] = tmpBuildInScript;

					cmd_script = g_BuildScripts[tmpCmdFileName].replace_script_args(args);
				}
				else {
					cmd_script = g_BuildScripts[argv[i + 1]].replace_script_args(args);
				}
				break;
			}
			else if (s == "-bshow")
			{
				if (i + 1 == argc || g_BuildScripts.find(argv[i+1]) == g_BuildScripts.end())
				{
					printf("error, must be have script name: ");
					g_BuildScripts.ShowCmds();
					printf("\n");
					return -1;
				}
				else
				{
					printf("%s", g_BuildScripts[argv[i + 1]].m_script.c_str());
					return 0;
				}
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
				if (s.find(' ') != string::npos && s[s.size() - 1] != ':')
				{
					s.insert(s.begin(), '"');
					s.insert(s.end(), '"');
				}
				cmd.append(s);
				if(j != (argc -1)) /* Don't add space at last arg */
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

	if (deamon && dryrun)
	{
		printf("Error: -d -dry Can't apply at the same time\n");
		return -1;
	}

	if (shell && dryrun)
	{
		printf("Error: -dry -s Can't apply at the same time\n");
		return -1;
	}

	if (g_verbose)
	{
		printf("%sBuild in config:%s\n", g_vt_boldwhite, g_vt_default);
		printf("\tPctl\t Chip\t\t Vid\t Pid\t BcdVersion\n");
		printf("\t==================================================\n");
		uuu_for_each_cfg(print_cfg, NULL);

		if (!cmd_script.empty())
			printf("\n%sRun built-in script:%s\n %s\n\n", g_vt_boldwhite, g_vt_default, cmd_script.c_str());

		if (!shell)
			cout << "Wait for Known USB Device Appear...";

		print_usb_filter();

		printf("\n");
	}
	else {
		cout << "Wait for Known USB Device Appear...";
		print_usb_filter();
		cout << "\r";
		cout << "\x1b[?25l";
		cout.flush();
	}

	map<uint64_t, ShowNotify> nt_session;

	uuu_register_notify_callback(progress, &nt_session);


	if (!cmd.empty())
	{
		ret = uuu_run_cmd(cmd.c_str(), dryrun);

		for (size_t i = 0; i < g_map_path_nt.size()+3; i++)
			printf("\n");
		if(ret)
			printf("\nError: %s\n", uuu_get_last_err_string());
		else
			printf("Okay\n");

		runshell(shell);
		return ret;
	}

	if (!cmd_script.empty())
		ret = uuu_run_cmd_script(cmd_script.c_str(), dryrun);
	else
		ret = uuu_auto_detect_file(filename.c_str());

	if (ret)
	{
		runshell(shell);

		cout << g_vt_red << "\nError: " << g_vt_default <<  uuu_get_last_err_string();
		return ret;
	}

	if (uuu_wait_uuu_finish(deamon, dryrun))
	{
		cout << g_vt_red << "\nError: " << g_vt_default << uuu_get_last_err_string();
		return -1;
	}

	runshell(shell);

	/*Wait for the other thread exit, after send out CMD_DONE*/
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	return g_overall_status;
}
