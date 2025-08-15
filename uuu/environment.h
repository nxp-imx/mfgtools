#pragma once
//! @file

#include "Logger.h"

#include "../libuuu/libuuu.h"
#include "../libuuu/string_man.h"

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <string.h>
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

		std::string pd;
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

	class SerialMatchConfig final
	{
		enum class Mode { ignore, match };
		Mode mode = Mode::match;

		static int ignore_serial_number(
			const char* pro,
			const char* chip,
			const char* /*comp*/,
			uint16_t vid,
			uint16_t pid,
			uint16_t /*bcdlow*/,
			uint16_t /*bcdhigh*/,
			void* p)
		{
			SerialMatchConfig& context = *(SerialMatchConfig*)p;
			std::string info;
			string_man::format(info, "chip: '%s', pro: '%s', vid:0x%04X, pid:0x%04X", chip, pro, vid, pid);
			g_logger.log_info(info);

			const HKEY base_key = HKEY_LOCAL_MACHINE;
			const char* key_path = "SYSTEM\\CurrentControlSet\\Control\\UsbFlags";
			char value_name[128];
			snprintf(value_name, 128, "IgnoreHWSerNum%04x%04x", vid, pid);
			const BYTE value = 1;
			const std::string value_desc = "HKLM\\" + std::string(key_path) + " " + value_name;

			if (context.mode == Mode::ignore)
			{
				g_logger.log_debug([&]() { return "Writing registry value: " + value_desc; });
				LSTATUS status = ::RegSetKeyValueA(base_key, key_path, value_name, REG_BINARY, &value, 1);
				if (status != ERROR_SUCCESS)
				{
					g_logger.log_error("Unable to write registry " + value_desc);
					return EXIT_FAILURE;
				}
			}
			else // context.mode == Mode::match
			{
				g_logger.log_debug([&]() { return "Deleting registry value: " + value_desc; });
				HKEY key;
				if (::RegOpenKeyExA(base_key, key_path, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
				{
					g_logger.log_error("Unable to open registry key for value: " + value_desc);
					return EXIT_FAILURE;
				}
				LSTATUS delete_status = ::RegDeleteValueA(key, value_name);
				if (delete_status == ERROR_FILE_NOT_FOUND)
				{
					g_logger.log_warning("Value not found: " + value_desc);
					::RegCloseKey(key);
					return EXIT_SUCCESS;
				}
				else if (delete_status != ERROR_SUCCESS)
				{
					g_logger.log_error("Unable to delete registry value: " + value_desc);
					::RegCloseKey(key);
					return EXIT_FAILURE;
				}
				::RegCloseKey(key);
			}
			return EXIT_SUCCESS;
		}

		int apply()
		{
			std::string mode_desc = mode == Mode::ignore ? "ignore" : "match";
			g_logger.log_info("Modifying registry to " + mode_desc + " serial number for device discovery...");
			int status = uuu_for_each_cfg(ignore_serial_number, (void*)this);
			if (status)
			{
				g_logger.log_hint("Run as administrator");
			}
			return status;
		}

	public:
		int ignore()
		{
			mode = Mode::ignore;
			return apply();
		}
		int match()
		{
			mode = Mode::match;
			return apply();
		}
	};

	static int set_environment_variable(const std::string& name, const std::string& value)
	{
		return ::SetEnvironmentVariableA(name.c_str(), value.c_str()) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

#else

	static int set_ignore_serial_number(bool)
	{
		return EXIT_SUCCESS;
	}

	static int set_environment_variable(const std::string& name, const std::string& value)
	{
		return ::setenv(name.c_str(), value.c_str(), true);
	}

#endif

}