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

#include "../libuuu/libuuu.h"

#ifndef _MSC_VER
#include <unistd.h>
#include <limits.h>
#else
#include <Windows.h>
#endif

void linux_auto_arg(const char *space = " ", const char * filter = "")
{
	string str = filter;

	const char *param[] = { "-b", "-d", "-v", "-V", "-s", NULL };
	int i = 0;

	for (int i = 0; param[i]; i++)
	{
		if (str.find(param[i]) == string::npos)
			cout << param[i] << space << endl;
	}
}

int linux_autocomplete_ls(const char *path, void *p)
{
	cout << path + 2 << endl;
	return 0;
}

void linux_autocomplete(int argc, char **argv)
{
	string last = argv[3];
	string cur = argv[2];

	if (argv[2][0] == '-')
	{
		if (cur.size() == 1)
			linux_auto_arg();
		else
			cout << cur << " " << endl;

		return;
	}

	if (last.size()>=3)
	{
		if (last.substr(last.size() - 3) == "uuu" &&(cur.empty() || cur[0] == '-'))
		{
			linux_auto_arg();
			cout << cur << endl;
		}

	}else if(last.empty())
	{
		linux_auto_arg();
	}
	else if (last == "-b")
	{
		return g_BuildScripts.PrintAutoComplete(cur);

	}else if(last[0] == '-')
	{
		linux_auto_arg();
	}

	uuu_for_each_ls_file(linux_autocomplete_ls, cur.c_str(), NULL);
}

string get_next_word(string str, size_t &pos)
{
	size_t start = 0;
	start = str.find(' ', pos);
	string sub = str.substr(pos, start - pos);
	pos = start == string::npos ? start : start + 1;
	return sub;
}

void power_shell_autocomplete(const char *p)
{
	string pstr = p;
	size_t pos = 0;

	string file;

	vector<string> argv; string params;
	while (pos != string::npos && pos < pstr.size())
	{
		file = get_next_word(pstr, pos);
		argv.push_back(file);

		if (file.size() && file[0] == '-')
			params += " " + file;
	}

	string last = argv[argv.size() - 1];
	string prev = argv.size() > 1 ? argv[argv.size() - 2] : "";
	if (last == "-b" || prev == "-b")
	{
		string cur;
		if (prev == "-b")
			cur = last;

		if (g_BuildScripts.find(cur) == g_BuildScripts.end())
			g_BuildScripts.PrintAutoComplete(cur, "");

		last.clear();
	}
	else
	{
		if (last[0] == '-' || argv.size() == 1)
			linux_auto_arg("", params.c_str());
	}

	if (argv.size() == 1)
		last.clear();

	uuu_for_each_ls_file(linux_autocomplete_ls, last.c_str(), NULL);
}

int auto_complete(int argc, char**argv)
{
	if (argc == 4)
	{
		string str = argv[1];
		if (str.size() >= 3)
			if (str.substr(str.size() - 3) == "uuu")
			{
				linux_autocomplete(argc, argv);
				return 0;
			}
	}

	if (argc >= 2)
	{
		string str = argv[1];
		if (str == "-autocomplete")
		{
			power_shell_autocomplete(argc == 2 ? "" : argv[2]);
			return 0;
		}
	}

	return 1;
}

void print_autocomplete_help()
{

#ifndef _MSC_VER
	{
		cout << "Enjoy auto [tab] command complete by run below command" << endl;
		char result[PATH_MAX];
		memset(result, 0, PATH_MAX);
		ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
		cout << "  complete -o nospace -C " << result << " uuu" << endl << endl;
	}
#else
	{
		printf("Powershell: Enjoy auto [tab] command complete by run below command\n");
		
		HMODULE hModule = GetModuleHandleA(NULL);
		char path[MAX_PATH];
		GetModuleFileNameA(hModule, path, MAX_PATH);

		printf("   Register-ArgumentCompleter -CommandName uuu -ScriptBlock {param($commandName,$parameterName,$wordToComplete,$commandAst,$fakeBoundParameter); %s -autocomplete $parameterName }\n",
			path);
	}
#endif

}
