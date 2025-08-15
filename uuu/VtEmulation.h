#pragma once
//! @file

#include <stdio.h>

#include <iostream>
#include <memory>

/**
 * @brief Base class for VT terminal emulation
 * @details
 * For general escape code info, see https://en.wikipedia.org/wiki/ANSI_escape_code
 * For colors, see https://gist.github.com/raghav4/48716264a0f426cf95e4342c21ada8e7
 * For hiding cursor, see https://stackoverflow.com/questions/15011478/ansi-questions-x1b25h-and-x1be
 */
class VtEmulation
{
public:
	VtEmulation() {
		select_default_palette();
	}
	const char* hide_cursor;
	const char* show_cursor;
	const char* fg_default;
	const char* fg_cyan;
	const char* fg_light_red;
	const char* fg_light_green;
	const char* fg_light_yellow;
	const char* fg_light_blue;
	const char* fg_light_magenta;
	const char* fg_light_cyan;
	const char* fg_light_white;

	void select_default_palette() {
		hide_cursor = "\x1B[?25l";
		show_cursor = "\x1B[?25h";
		fg_default = "\x1B[0m";
		fg_cyan = "\x1B[36m";
		fg_light_red = "\x1B[91m";
		fg_light_green = "\x1B[92m";
		fg_light_yellow = "\x1B[93m";
		fg_light_blue = "\x1B[94m";
		fg_light_magenta = "\x1B[95m";
		fg_light_cyan = "\x1B[96m";
		fg_light_white = "\x1B[97m";
	}

	const char* fg_error() const { return fg_light_red; }
	const char* fg_warn() const { return fg_light_yellow; }
	const char* fg_ok() const { return fg_light_green; }
	const char* fg_info() const { return fg_light_blue; }

	virtual bool enable() = 0;
	virtual int get_console_width() = 0;
};

/**
 * @brief Global singleton for VT terminal emulation
 */
extern std::shared_ptr <VtEmulation> g_vt;

/**
 * @brief Shows (unhides) the cursor on destruction
 * @deprecated
 * This should be eliminated, but is maintained for the legacy CLI
 */
class CursorRestorer final
{
public:
	~CursorRestorer()
	{
		// show the cursor
		std::cout << g_vt->show_cursor; // "\x1b[?25h";

		// [why?]
		std::cout << "\n";
	}
};

/**
 * @brief Hides the cursor for the lifetime of a instance
 */
class CursorHider final
{
public:
	CursorHider()
	{
		// hide the cursor
		std::cout << g_vt->hide_cursor; // "\x1b[?25l";
	}
	~CursorHider()
	{
		// show the cursor
		std::cout << g_vt->show_cursor; // "\x1b[?25h";
	}
};

#ifdef _MSC_VER

#define DEFINE_CONSOLEV2_PROPERTIES
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @brief VT terminal emulation for Windows
 */
class PlatformVtEmulation final : public VtEmulation
{
	void clear_codes() noexcept
	{
		hide_cursor = 
			show_cursor =
			fg_default =
			fg_cyan =
			fg_light_red =
			fg_light_green =
			fg_light_yellow =
			fg_light_blue =
			fg_light_magenta =
			fg_light_cyan =
			fg_light_white = "";
	}

public:
	bool enable() override
	{
		// Set output mode to handle virtual terminal sequences
		HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut == INVALID_HANDLE_VALUE)
		{
			clear_codes();
			return false;
		}

		DWORD dwMode = 0;
		if (!::GetConsoleMode(hOut, &dwMode))
		{
			clear_codes();
			return false;
		}

		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!::SetConsoleMode(hOut, dwMode))
		{
			clear_codes();
			return false;
		}
		return true;
	}

	int get_console_width() override
	{
		CONSOLE_SCREEN_BUFFER_INFO sbInfo;
		::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &sbInfo);
		return sbInfo.dwSize.X;
	}
};

#else

#include <sys/ioctl.h>

/**
 * @brief VT terminal emulation for Linux
 */
class PlatformVtEmulation final : public VtEmulation
{
	bool enable() override
	{
		return true;
	}

	int get_console_width() override
	{
		struct winsize w;
		ioctl(0, TIOCGWINSZ, &w);
		return w.ws_col;
	}
};

#endif