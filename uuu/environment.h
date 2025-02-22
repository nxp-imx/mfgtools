
#pragma once

#include "../libuuu/libuuu.h"

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#include <iostream>
#include <string>

/**
 * @brief System environment functions
 */
namespace environment {

	static int ask_passwd(char* prompt, char user[MAX_USER_LEN], char passwd[MAX_USER_LEN])
	{
		std::cout << std::endl << prompt << " Required Login" << std::endl;
		std::cout << "Username:";
		std::cin.getline(user, 128);
		std::cout << "Password:";
		int i = 0;

#ifdef _WIN32
		while ((passwd[i] = _getch()) != '\r') {
			if (passwd[i] == '\b') {
				if (i != 0) {
					std::cout << "\b \b";
					i--;
				}
			}
			else {
				std::cout << '*';
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
		getline(std::cin, pd);

		tcsetattr(STDIN_FILENO, TCSANOW, &old);
		if (pd.size() > MAX_USER_LEN - 1)
			return EXIT_FAILURE;
		memcpy(passwd, pd.data(), pd.size());
		i = pd.size();

#endif
		passwd[i] = 0;
		std::cout << std::endl;
		return EXIT_SUCCESS;
	}

#ifdef _WIN32

	static int ignore_serial_number(const char* pro, const char* chip, const char*/*comp*/, uint16_t vid, uint16_t pid, uint16_t /*bcdlow*/, uint16_t /*bcdhigh*/, void*/*p*/)
	{
		printf("\t %s\t %s\t 0x%04X\t0x%04X\n", chip, pro, vid, pid);

		char sub[128];
		snprintf(sub, 128, "IgnoreHWSerNum%04x%04x", vid, pid);
		const BYTE value = 1;

		LSTATUS ret = RegSetKeyValueA(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\UsbFlags",
			sub, REG_BINARY, &value, 1);
		if (ret == ERROR_SUCCESS)
			return EXIT_SUCCESS;

		printf("Set key failure, try run as administrator permission\n");
		return EXIT_FAILURE;
	}

	static int set_ignore_serial_number()
	{
		printf("Modifying registry to ignore serial number for finding devices...\n");
		return uuu_for_each_cfg(ignore_serial_number, NULL);
	}

#define environment_putenv _putenv

#else

#define environment_putenv putenv

#endif

}