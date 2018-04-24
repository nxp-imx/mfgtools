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

#include "../libuuu/libuuu.h"

using namespace std;
void print_help()
{
	printf("uuu u-boot.imx\\flash.bin\n");
	printf("\tDownload u-boot.imx\\flash.bin to board by usb\n");
}
void print_version()
{
	printf("uuu (universal update utitle) for nxp imx chips -- %s\n\n", get_version_string());
}

int polling_usb(std::atomic<int>& bexit);

class ShowNotify
{
public:
	string m_cmd;
	string m_dev;
	int	m_trans_size;
	int m_trans_pos;
	
	ShowNotify()
	{
		m_trans_size = m_trans_pos = 0;
	}

	void print(notify nt)
	{
		if (nt.type == notify::NOFITY_DEV_ATTACH)
		{
			m_dev = nt.str;
		}
		if (nt.type == notify::NOTIFY_CMD_START)
		{
			m_cmd = nt.str;
		}
		if (nt.type == notify::NOTIFY_TRANS_SIZE)
		{
			m_trans_size = nt.total;
		}
		if (nt.type == notify::NOTIFY_TRANS_POS)
		{
			if (m_trans_size == 0)
				return;
			
			if ((nt.index - m_trans_pos) < (m_trans_size / 100))
				return;

			m_trans_pos = nt.index;
		}

		cout << m_dev.c_str() << std::setw(8) << "[";
		int width=40;
		int s = 0;
		for (int i = 1; i <= width; i++)
		{
			if (m_trans_size == 0)
			{
				cout << " ";
				continue;
			}

			if (m_trans_pos*width / (m_trans_size*i))
			{
				cout << "=";
				s = 1;
			}
			else
			{
				if (s)
				{
					s = 0;
					cout << ">";
				}
				else
				{
					cout << " ";
				}
			}
		}

		cout << "] " << m_cmd.c_str() <<"\r";
	}
};

int progress(notify nt, void *p)
{
	ShowNotify *np = (ShowNotify*)p;
	np->print(nt);
	return 0;
}

int main(int argc, char **argv)
{
	print_version();

	if (argc == 1)
		print_help();

	ShowNotify sn;
	register_notify_callback(progress, &sn);

	string cmd = "SDPS: boot ";
	cmd += argv[1];

	if (run_cmd(cmd.c_str()))
		printf("Error: %s\n", get_last_err_string());
	return 0;
}

