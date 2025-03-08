
#pragma once
//! @file

#include "VtEmulation.h"

#include <functional>
#include <iostream>
#include <memory>
#include <string>

/**
 * @brief Application logging
 */
class Logger final
{
	void log(const std::string& label, const std::string& message, const std::string& color) const
	{
		if (is_color_output_enabled)
		{
			std::cerr << color << label << ": " << g_vt->fg_default << message << std::endl;
		}
		else
		{
			std::cerr << label << ": " << message << std::endl;
		}
	}

public:
	bool is_color_output_enabled = false;

	void log_internal_error(const std::string& message) const
	{
		log("INTERNAL ERROR", message, g_vt->fg_light_red);
	}

	void log_error(const std::string& message) const
	{
		log("Error", message, g_vt->fg_light_red);
	}

	void log_warning(const std::string& message) const
	{
		log("Warning", message, g_vt->fg_light_yellow);
	}

	void log_info(const std::string& message) const
	{
		log("Info", message, g_vt->fg_light_blue);
	}

	void log_hint(const std::string& message) const
	{
		log("Hint", message, g_vt->fg_light_green);
	}

	void log_dry_run(const std::string& message) const
	{
		log("Dry-run", message, g_vt->fg_light_magenta);
	}

	void log_debug(std::function<std::string()> format) const
	{
#ifdef _DEBUG
		bool is_debug_output_enabled = true;
		if (is_debug_output_enabled)
		{
			log("Debug", format(), g_vt->fg_light_cyan);
		}
#endif
	}
};

extern Logger g_logger;
