
#pragma once

#include <stdio.h>

#include <memory>

/**
 * @brief [what exactly does this do? what is it for?]
 */
class AutoCursor final
{
public:
	~AutoCursor()
	{
		// not sure what this does, but maybe it ensures that output color is normal
		printf("\x1b[?25h\n");
	}
};

/**
 * @brief Base class for VT terminal emulation
 */
class VtEmulation
{
public:
	VtEmulation() {
		select_default_palette();
	}
	const char* yellow;
	const char* default_fg;
	const char* green;
	const char* red;
	const char* kcyn;
	const char* boldwhite;

	void select_default_palette() {
		yellow = "\x1B[93m";
		default_fg = "\x1B[0m";
		green = "\x1B[92m";
		red = "\x1B[91m";
		kcyn = "\x1B[36m";
		boldwhite = "\x1B[97m";
	}

	virtual bool enable() = 0;
	virtual int get_console_width() = 0;
};

/**
 * @brief Global singleton for VT terminal emulation
 */
extern std::shared_ptr <VtEmulation> g_vt;

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
	void clear_pallet() noexcept
	{
		yellow =
			default_fg =
			green =
			red =
			kcyn =
			boldwhite = "";
	}

public:
	bool enable() override
	{
		// Set output mode to handle virtual terminal sequences
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut == INVALID_HANDLE_VALUE)
		{
			clear_pallet();
			return false;
		}

		DWORD dwMode = 0;
		if (!GetConsoleMode(hOut, &dwMode))
		{
			clear_pallet();
			return false;
		}

		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!SetConsoleMode(hOut, dwMode))
		{
			clear_pallet();
			return false;
		}
		return true;
	}

	int get_console_width() override
	{
		CONSOLE_SCREEN_BUFFER_INFO sbInfo;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfo);
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