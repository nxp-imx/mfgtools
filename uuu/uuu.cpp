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

const char * g_vt_yellow = "\x1B[93m";
const char * g_vt_default = "\x1B[0m";
const char * g_vt_green = "\x1B[92m";
const char * g_vt_red = "\x1B[91m";
const char * g_vt_kcyn = "\x1B[36m";
const char * g_vt_boldwhite = "\x1B[97m";

void clean_vt_color() noexcept
{
	g_vt_yellow = "";
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
vector<string> g_usb_serial_no_filter;

int g_verbose = 0;
static bool g_start_usb_transfer;

bmap_mode g_bmap_mode = bmap_mode::Default;

class AutoCursor
{
public:
	~AutoCursor()
	{
		printf("\x1b[?25h\n");
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

		return EXIT_SUCCESS;
	}
};

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

int ask_passwd(char* prompt, char user[MAX_USER_LEN], char passwd[MAX_USER_LEN])
{
	cout << endl << prompt << " Required Login"<<endl;
	cout << "Username:";
	cin.getline(user, 128);
	cout << "Password:";
	int i = 0;

#ifdef _WIN32
	while ((passwd[i] = _getch()) != '\r') {
		if (passwd[i] == '\b') {
			if (i != 0) {
				cout << "\b \b";
				i--;
			}
		}
		else {
			cout << '*';
			i++;
		}
	}
#else
	struct termios old, tty;
	tcgetattr(STDIN_FILENO, &tty);
	old = tty;
	tty.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &tty);

	string pd;
	getline(cin, pd);

	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	if(pd.size() > MAX_USER_LEN -1)
		return EXIT_FAILURE;
	memcpy(passwd, pd.data(), pd.size());
	i=pd.size();

#endif
	passwd[i] = 0;
	cout << endl;
	return EXIT_SUCCESS;
}

void print_cli_help()
{
	const char default_mode[] =
		"uuu [OPTION...] SPEC|CMD|BOOTLOADER\n"
		"    SPEC\tSpecifies a script to run without parameters; use -b for parameters;\n"
		"    \t\tFor a directory, use contained uuu.auto at root;\n"
		"    \t\tFor a zip file, expand, then use uuu.auto at root of expanded content\n"
		"    CMD\t\tRun a command; see -H for details\n"
		"    \t\tExample: SDPS: boot -f flash.bin\n"
		"    BOOTLOADER\tBoot device from bootloader image file\n"
		"    -d\t\tProduction (daemon) mode\n"
		"    -v\t\tVerbose feedback\n"
		"    -V\t\tExtends verbose feedback to include USB library feedback\n"
		"    -dry\tDry-run; displays verbose output without performing actions\n"
		"    -bmap\tUse .bmap files even if flash commands do not specify them\n"
		"    -no-bmap\tIgnore .bmap files even if flash commands specify them\n"
		"    -m PATH\tLimits USB port monitoring. Example: -m 1:2 -m 1:3\n"
		"    -ms SN\tMonitor the serial number prefix of the device\n"
		"    -t #\tSeconds to wait for a device to appear\n"
		"    -T #\tSeconds to wait for a device to appeared at stage switch [for deamon mode?]\n"
		"    -e KEY=VAL\tSet environment variable KEY to value VAL\n"
		"    -pp #\tUSB polling period in milliseconds\n"
		"    -dm\t\tDisable small memory\n";

	const char param_and_builtin_mode1[] =
		"uuu [OPTION...] -b SPEC|BUILTIN [PARAM...]\n"
		"    SPEC\tSame as for default mode\n"
		"    BUILTIN\tBuilt-in script: ";

	const char param_and_builtin_mode2[] =
		"    OPTION...\tSame as for default mode\n"
		"    PARAM...\tScript parameter values\n";

	const char special_modes[] =
		"uuu -ls-devices\tList connected devices\n"
		"uuu -ls-builtin\tList built-in scripts\n"
		"uuu -cat-builtin BUILTIN\n"
		"    \t\tOutput built-in script\n"
		"uuu -s\t\tInteractive (shell) mode; records commands in uuu.inputlog\n"
		"uuu -udev\tFor Linux, output udev rule for avoiding using sudo each time\n"
		"uuu -IgSerNum\tFor Windows, modify registry to ignore USB serial number to find devices\n"
		"uuu -h\t\tOutput basic help\n"
		"uuu -h-protocol-commands\n"
		"    \t\tOutput protocol command help info\n"
		"uuu -h-protocol-support\n"
		"    \t\tOutput protocol support by device info\n"
		"uuu -h-auto-complete\n"
		"    \t\tOutput auto/tab completion help info\n";

	printf("\nDefault mode:\n%s", default_mode);

	printf("\nParameter & built-in script mode:\n%s", param_and_builtin_mode1);
	g_BuildScripts.ShowCmds();
	printf("\n%s", param_and_builtin_mode2);

	printf("\nSpecial modes:\n%s", special_modes);
}

void print_script_directory() {
	printf("\nBuilt-in scripts:\n");
	g_BuildScripts.ShowAll();
}

void print_protocol_help() {
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

void print_app_title()
{
	printf("Universal Update Utility for NXP i.MX chips -- %s\n", uuu_get_version_string());
}

int print_cfg(const char *pro, const char * chip, const char * /*compatible*/, uint16_t vid, uint16_t pid, uint16_t bcdmin, uint16_t bcdmax, void * /*p*/)
{
	const char *ext;
	if (strlen(chip) >= 7)
		ext = "";
	else
		ext = "\t";

	if (bcdmin == 0 && bcdmax == 0xFFFF)
		printf("\t%s\t %s\t%s 0x%04x\t 0x%04x\n", pro, chip, ext, vid, pid);
	else
		printf("\t%s\t %s\t%s 0x%04x\t 0x%04x\t [0x%04x..0x%04x]\n", pro, chip, ext, vid, pid, bcdmin, bcdmax);
	return EXIT_SUCCESS;
}

void print_protocol_support_help() {
	printf("Protocol support for devices:\n");
	printf("\tPctl\t Chip\t\t Vid\t Pid\t BcdVersion\t Serial_No\n");
	printf("\t==================================================\n");
	uuu_for_each_cfg(print_cfg, NULL);
}

int print_udev_rule(const char * /*pro*/, const char * /*chip*/, const char * /*compatible*/,
	uint16_t vid, uint16_t pid, uint16_t /*bcdmin*/, uint16_t /*bcdmax*/, void * /*p*/)
{
	printf("SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"%04x\", ATTRS{idProduct}==\"%04x\", TAG+=\"uaccess\"\n",
			vid, pid);
	return EXIT_SUCCESS;
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
	size_t m_trans_pos = 0;
	int m_status = 0;
	size_t m_cmd_total = 0;
	size_t m_cmd_index = 0;
	string m_last_err;
	int m_done = 0;
	size_t m_start_pos = 0;
	size_t	m_trans_size = 0;
	clock_t m_start_time;
	uint64_t m_cmd_start_time;
	uint64_t m_cmd_end_time;
	bool m_IsEmptyLine = false;

	ShowNotify() : m_start_time{clock()} {}

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

		if (nt->type == uuu_notify::NOTIFY_DEV_ATTACH)
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
		str.resize(12, ' ');

		string_ex s;
		s.format("%2d/%2d", m_cmd_index+1, m_cmd_total);

		str += s;
		return str;
	}
	void print_simple()
	{
		int width = get_console_width();
		int info, bar;
		info = 18;
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
				str += "Waiting for device...";

			if (!g_usb_path_filter.empty())
			{
				str += " at path ";
				for (size_t i = 0; i < g_usb_path_filter.size(); i++)
					str += g_usb_path_filter[i] + " ";
			}

			if (!g_usb_serial_no_filter.empty())
			{
				str += " at serial_no ";
				for (auto it: g_usb_serial_no_filter)
					str += it + "*";
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
	return EXIT_SUCCESS;
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
			<< ((uuu_get_version() & 0xFF000000) >> 24)
			<< "."
			<< ((uuu_get_version() & 0xFFF000) >> 12)
			<< "."
			<< ((uuu_get_version() & 0xFFF))
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
			}
			else if (cmd == "help" || cmd == "?")
			{
				print_cli_help();
			}
			else if (cmd == "q" || cmd == "quit")
			{
				return EXIT_SUCCESS;
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
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}

void print_udev()
{
	uuu_for_each_cfg(print_udev_rule, NULL);

	cerr << endl <<
		"Enable udev rules via:" << endl <<
		"\tsudo sh -c \"uuu -udev >> /etc/udev/rules.d/70-uuu.rules\"" << endl <<
		"\tsudo udevadm control --reload" << endl <<
		"Note: These instructions output to standard error so are excluded" << endl << endl;
}

int print_usb_device(const char *path, const char *chip, const char *pro, uint16_t vid, uint16_t pid, uint16_t bcd, const char *serial_no, void * /*p*/)
{
	printf("\t%s\t %s\t %s\t 0x%04X\t0x%04X\t 0x%04X\t %s\n", path, chip, pro, vid, pid, bcd, serial_no);
	return EXIT_SUCCESS;
}

void print_lsusb()
{
	cout << "Connected Known USB Devices\n";
	printf("\tPath\t Chip\t Pro\t Vid\t Pid\t BcdVersion\t Serial_no\n");
	printf("\t====================================================================\n");

	uuu_for_each_devices(print_usb_device, NULL);
}

#ifdef WIN32

int ignore_serial_number(const char *pro, const char *chip, const char */*comp*/, uint16_t vid, uint16_t pid, uint16_t /*bcdlow*/, uint16_t /*bcdhigh*/, void */*p*/)
{
	printf("\t %s\t %s\t 0x%04X\t0x%04X\n", chip, pro, vid, pid);

	char sub[128];
	snprintf(sub, 128, "IgnoreHWSerNum%04x%04x", vid, pid);
	const BYTE value = 1;

	LSTATUS ret = RegSetKeyValueA(HKEY_LOCAL_MACHINE,
								  "SYSTEM\\CurrentControlSet\\Control\\UsbFlags",
									sub, REG_BINARY, &value, 1);
	if(ret == ERROR_SUCCESS)
		return EXIT_SUCCESS;

	printf("Set key failure, try run as administrator permission\n");
	return EXIT_FAILURE;
}
#endif

int set_ignore_serial_number()
{
#ifndef WIN32
	printf("Only windows system need set ignore serial number registry");
	return EXIT_FAILURE;
#else
	printf("Set window registry to ignore usb hardware serial number for known uuu device:\n");
	return uuu_for_each_cfg(ignore_serial_number, NULL);
#endif
}

void log_error(const string& message) {
	cerr << "Error: " << message << endl;
}

void log_syntax_error(const string& message) {
	log_error(message);
	print_cli_help();
}

int main(int argc, char **argv)
{
	// commented out causes failure when pass script file name/path as first arg plus -v after
	//if (auto_complete(argc, argv) == 0) return EXIT_SUCCESS;

	// handle modes that should _not_ print the app title
	if (argc >= 2)
	{
		string s = argv[1];
		if(s == "-udev")
		{
			print_udev();
			return EXIT_SUCCESS;
		}
		if (s == "-cat-builtin")
		{
			if (2 == argc)
			{
				fprintf(stderr, "Error: Missing built-in script name; options: ");
				g_BuildScripts.ShowCmds(stderr);
				fprintf(stderr, "\n");
				return EXIT_FAILURE;
			}
			if (g_BuildScripts.find(argv[2]) == g_BuildScripts.end())
			{
				fprintf(stderr, "Error: Unknown built-in script name; options: ");
				g_BuildScripts.ShowCmds(stderr);
				fprintf(stderr, "\n");
				return EXIT_FAILURE;
			}
			string str = g_BuildScripts[argv[2]].m_text;
			while (str.size() > 0 && (str[0] == '\n' || str[0] == ' '))
				str = str.erase(0,1);
			printf("%s", str.c_str());
			return EXIT_SUCCESS;
		}
	}

	AutoCursor a;

	print_app_title();

	if (!enable_vt_mode())
	{
		// [why enable verbose in this case?]
		cout << "Warning: Console doesn't support VT mode; enabling verbose feedback" << endl;
		g_verbose = 1;
	}

	if (argc == 1)
	{
		log_error("Invalid input");
		print_cli_help();
		return EXIT_FAILURE;
	}

	int deamon = 0;
	int shell = 0;
	int dryrun = 0;
	string input_path;
	string protocol_cmd;
	string cmd_script;

	for (int i = 1; i < argc; i++)
	{
		string arg = argv[i];
		if (!arg.empty() && arg[0] == '-')
		{
			if (arg == "-d")
			{
				deamon = 1;
				uuu_set_small_mem(0);
			}
			else if (arg == "-dm")
			{
				uuu_set_small_mem(0);
			}
			else if (arg == "-s")
			{
				shell = 1;
				// why set verbose for shell mode?
				g_verbose = 1;
			}
			else if (arg == "-v")
			{
				g_verbose = 1;
			}
			else if (arg == "-V")
			{
				g_verbose = 1;
				uuu_set_debug_level(2);
			}else if (arg == "-dry")
			{
				dryrun = 1;
				// why is verbose set for dry-run? 
				g_verbose = 1;
			}
			else if (arg == "-h")
			{
				print_cli_help();
				return EXIT_SUCCESS;
			}
			else if (arg == "-h-auto-complete")
			{
				print_autocomplete_help();
				return EXIT_SUCCESS;
			}
			else if (arg == "-h-protocol-commands")
			{
				print_protocol_help();
				return EXIT_SUCCESS;
			}
			else if (arg == "-h-protocol-support")
			{
				print_protocol_support_help();
				return EXIT_SUCCESS;
			}
			else if (arg == "-ls-builtin")
			{
				print_script_directory();
				return EXIT_SUCCESS;
			}
			else if (arg == "-m")
			{
				if (++i >= argc)
				{
					log_syntax_error("Missing USB path argument");
					return EXIT_FAILURE;
				}
				uuu_add_usbpath_filter(argv[i]);
				g_usb_path_filter.push_back(argv[i]);
			}
			else if (arg == "-ms")
			{
				if (++i >= argc)
				{
					log_syntax_error("Missing serial # argument");
					return EXIT_FAILURE;
				}
				uuu_add_usbserial_no_filter(argv[i]);
				g_usb_serial_no_filter.push_back(argv[i]);
			}
			else if (arg == "-t")
			{
				if (++i >= argc)
				{
					log_syntax_error("Missing seconds argument");
					return EXIT_FAILURE;
				}
				uuu_set_wait_timeout(atoll(argv[i]));
			}
			else if (arg == "-T")
			{
				if (++i >= argc)
				{
					log_syntax_error("Missing seconds argument");
					return EXIT_FAILURE;
				}
				uuu_set_wait_next_timeout(atoll(argv[i]));
			}
			else if (arg == "-pp")
			{
				if (++i >= argc)
				{
					log_syntax_error("Missing milliseconds argument");
					return EXIT_FAILURE;
				}
				uuu_set_poll_period(atoll(argv[i]));
			}
			else if (arg == "-ls-devices")
			{
				print_lsusb();
				return EXIT_SUCCESS;
			}
			else if (arg == "-IgSerNum")
			{
				return set_ignore_serial_number();
			}
			else if (arg == "-bmap")
			{
				g_bmap_mode = bmap_mode::Force;
			}
			else if (arg == "-no-bmap")
			{
				g_bmap_mode = bmap_mode::Ignore;
			}
			else if (arg == "-e")
			{
#ifndef WIN32
	#define _putenv putenv
#endif
				if (++i >= argc)
				{
					log_syntax_error("Missing key=value argument");
					return EXIT_FAILURE;
				}
				if (_putenv(argv[i]))
				{
					printf("Error: Failed to set '%s'. Hint: parameter must have the form key=value\n", argv[i]);
					return EXIT_FAILURE;
				}
			}
			else if (arg == "-b" || arg == "-brun")
			{
				if (i + 1 == argc)
				{
					log_syntax_error("Missing path or built-in script name");
					return EXIT_FAILURE;
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

				// if script name is not built-in, try to look for a file
				if (g_BuildScripts.find(argv[i + 1]) == g_BuildScripts.end()) {
					const string tmpCmdFileName{argv[i + 1]};

					std::ifstream t(tmpCmdFileName);
					std::string fileContents((std::istreambuf_iterator<char>(t)),
						std::istreambuf_iterator<char>());

					if (fileContents.empty()) {
						printf("%s is not built-in script or fail load external script file", tmpCmdFileName.c_str());
						return EXIT_FAILURE;
					}

					BuiltInScriptRawData tmpCmd{
						tmpCmdFileName.c_str(),
						fileContents.c_str(),
						"Script loaded from file"
					};

					g_BuildScripts.emplace(tmpCmdFileName, &tmpCmd);

					cmd_script = g_BuildScripts[tmpCmdFileName].replace_script_args(args);
				}
				else {
					cmd_script = g_BuildScripts[argv[i + 1]].replace_script_args(args);
				}
				break;
			}
			else
			{
				cout << "Error: Unknown option: " << arg << endl;
				print_cli_help();
				return EXIT_FAILURE;
			}
		}else if (!arg.empty() && arg[arg.size() - 1] == ':')
		{
			// looks like a protocol command
			for (int j = i; j < argc; j++)
			{
				arg = argv[j];
				if (arg.find(' ') != string::npos && arg[arg.size() - 1] != ':')
				{
					arg.insert(arg.begin(), '"');
					arg.insert(arg.end(), '"');
				}
				protocol_cmd.append(arg);
				if(j != (argc -1)) /* Don't add space at last arg */
					protocol_cmd.append(" ");
			}
			break;
		}
		else
		{
			// treat as a file system path
			if (!input_path.empty())
			{
				printf("Error: Too many path arguments - %s\n", arg.c_str());
				return EXIT_FAILURE;
			}
			input_path = arg;
		}
	}

	signal(SIGINT, ctrl_c_handle);

	uuu_set_askpasswd(ask_passwd);

	if (deamon && shell)
	{
		log_error("Can't use deamon (-d) and shell (-s) together");
		return EXIT_FAILURE;
	}

	if (deamon && dryrun)
	{
		log_error("Can't use deamon (-d) and dry-run (-dry) together");
		return EXIT_FAILURE;
	}

	if (shell && dryrun)
	{
		log_error("Error: Can't use shell (-s) and dry-run (-dry) together");
		return EXIT_FAILURE;
	}

	if (g_verbose)
	{
		// commented out since seems overkill since can jprint it via -cat-builtin
		//if (!cmd_script.empty())
		//	printf("\n%sRunning built-in script:%s\n %s\n\n", g_vt_boldwhite, g_vt_default, cmd_script.c_str());

		// why not log for !shell? it's logged for !g_verbose regardless
		if (!shell) {
			cout << "Waiting for device";
			print_usb_filter();
			cout << "...";
			printf("\n");
		}
	}
	else {
		cout << "Waiting for device";
		print_usb_filter();
		cout << "...";
		cout << "\r";
		cout << "\x1b[?25l";
		cout.flush();
	}

	map<uint64_t, ShowNotify> nt_session;

	uuu_register_notify_callback(progress, &nt_session);

	if (!protocol_cmd.empty())
	{
		int ret = uuu_run_cmd(protocol_cmd.c_str(), dryrun);

		for (size_t i = 0; i < g_map_path_nt.size()+3; i++)
			printf("\n");
		if(ret)
			printf("Error: %s\n", uuu_get_last_err_string());
		else
			printf("Okay\n");

		runshell(shell);
		return ret;
	}

	{
		int ret;
		if (!cmd_script.empty())
			ret = uuu_run_cmd_script(cmd_script.c_str(), dryrun);
		else
			ret = uuu_auto_detect_file(input_path.c_str());

		if (ret)
		{
			ret = runshell(shell);
			if (ret)
				cout << g_vt_red << "\nError: " << g_vt_default << uuu_get_last_err_string();
			return ret;
		}
	}

	if (uuu_wait_uuu_finish(deamon, dryrun))
	{
		cout << g_vt_red << "\nError: " << g_vt_default << uuu_get_last_err_string();
		return EXIT_FAILURE;
	}

	runshell(shell);

	// wait for the other thread exit, after send out CMD_DONE
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//if(!g_verbose)
	//	printf("\n");
	return g_overall_status;
}
