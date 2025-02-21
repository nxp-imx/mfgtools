
#pragma once

#include "VtEmulation.h"

#include <iostream>
#include <memory>
#include <string>

/**
 * @brief Application logging
 */
class Logger final
{
public:
	bool is_color_output_enabled = false;

	void log_error(const std::string& message) const
	{
		if (is_color_output_enabled)
		{
			std::cerr << g_vt->red << "Error: " << g_vt->default_fg << message << std::endl;
		}
		else
		{
			std::cerr << "Error: " << message << std::endl;
		}
	}

	void log_internal_error(const std::string& message) const
	{
		if (is_color_output_enabled)
		{
			std::cerr << g_vt->red << "INTERNAL ERROR: " << g_vt->default_fg << message << std::endl;
		}
		else
		{
			std::cerr << "Error: " << message << std::endl;
		}
	}

	void log_info(const std::string& message) const
	{
		std::cout << message << std::endl;
	}

	void log_verbose(const std::string& message) const
	{
		extern int g_verbose;
		if (g_verbose)
		{
			std::cout << "Verbose: " << message << std::endl;
		}
	}
};
