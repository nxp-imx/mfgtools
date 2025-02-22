
#pragma once
//! @file

#include "VtEmulation.h"

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
			std::cerr << color << label << ": " << g_vt->default_fg << message << std::endl;
		}
		else
		{
			std::cerr << label << ": " << message << std::endl;
		}
	}

public:
	bool is_color_output_enabled = false;

	bool is_verbose_enabled()
	{
		extern int g_verbose;
		return g_verbose;
	}

	void log_internal_error(const std::string& message) const
	{
		log("INTERNAL ERROR", message, g_vt->red);
	}

	void log_error(const std::string& message) const
	{
		log("Error", message, g_vt->red);
	}

	void log_warning(const std::string& message) const
	{
		log("Warning", message, g_vt->yellow);
	}

	void log_info(const std::string& message) const
	{
		log("Info", message, g_vt->green);
	}

	void log_verbose(const std::string& message) const
	{
		extern int g_verbose;
		if (g_verbose)
		{
			log("Verbose", message, g_vt->kcyn);
		}
	}
};

extern Logger g_logger;
