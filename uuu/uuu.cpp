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
//! @file

#include "environment.h"
#include "Logger.h"
#include "ScriptCatalog.h"
#include "TransferFeedback.h"

#include "../libuuu/string_man.h"

#include <signal.h>
#include <stdio.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <streambuf>
#include <sstream>
#include <thread>
#include <tuple>
#include <vector>

int auto_complete(int argc, char **argv);
void print_autocomplete_help();

/**
 * @brief Boolean indicating whether transfer feedback is verbose
 * @details
 * To support verbose status output: when truthy, statusing is stream-based; prints on subsequent lines.
 * When falsy, status is written to the same line; overwriting the last status.
 */
int g_verbose = 0;

/**
 * @brief Enable verbose feedback to select stream-based feedback; not overwritten
 */
static void enable_stream_feedback() {
	g_verbose = 1;
}

Logger g_logger;
std::shared_ptr<VtEmulation> g_vt = std::make_shared<PlatformVtEmulation>();
TransferContext g_transfer_context;
bmap_mode g_bmap_mode = bmap_mode::Default;

static bool dry_run = false;
static bool is_continuous_mode = false;
static TransferFeedback transfer_feedback;
static const char command_help_text[] = {
#include "uuu.clst"
};

/**
 * @brief Exits process with specified status and CLI feedback/cleanup
 * @details
 * Use this instead of directly calling exit().
 */
[[noreturn]]
static void exit_for_status(int status)
{
	// ensure the cursor is visible before returing to the shell
	std::cout << g_vt->show_cursor;

	g_logger.log_debug([&]() { return status == EXIT_SUCCESS ? "Success :)" : "Failed :("; });

	// add a blank line; to stderr so does not go to a file if stdout is redirected
	std::cerr << std::endl;

	exit(status);
}

/**
 * @brief Exits process for a CLI syntax error
 */
[[noreturn]]
static void exit_for_syntax_error(const std::string& message)
{
	g_logger.log_error(message);
	g_logger.log_hint("See help output from 'uuu -h'");
	exit_for_status(EXIT_FAILURE);
}

/**
 * @brief Exits after outputting feedback about interrupt
 */
[[noreturn]]
static void exit_for_interrupt(int)
{
	// move cursor below status output area
	printf("\n\n\n");

	printf("INTERRUPTED\n");

	exit_for_status(EXIT_FAILURE);
}

/**
 * @brief Writes application name and version to stderr
 * @details
 * Writes to stderr so that redirected standard output does not include this.
 */
static void print_app_title()
{
	std::cerr << "Universal Update Utility for NXP i.MX -- " << uuu_get_version_string() << std::endl << std::endl;
}

typedef std::vector<std::tuple<std::string, std::function<void()>, std::string>> help_handler_t;
static const help_handler_t get_help_handlers();

static void print_new_cli_help()
{
	std::string text =
		"Commands:\n"
		"    install [OPTION...] SPEC|BOOTLOADER\n"
		"        Install without parameters\n"
		"        SPEC\t\tSpecifies a script to run without parameters; use -s for parameters;\n"
		"        \t\tFor a directory, use contained uuu.auto at root;\n"
		"        \t\tFor a zip file, use uuu.auto at root of content\n"
		"        BOOTLOADER\tBoot device from bootloader image file\n"
		"        OPTION\n"
		"            --dry-run\tProcess input without performing actions\n"
		"            --continuous\n"
		"            \t\tRepeat install action when it competes; production mode\n"
		"            --interactive\n"
		"            \t\tEnter interactive mode after install action (if any);\n"
		"            \t\tFYI: commands are written to uuu.inputlog for future playback\n"
		"            -v\t\tVerbose feedback streams status instead of overwriting on a single line\n"
		"            -V\t\tExtends verbose feedback to include USB library feedback\n"
		"            -bmap\tUse .bmap files even if flash commands do not specify them\n"
		"            -no-bmap\tIgnore .bmap files even if flash commands specify them\n"
		"            -m USB_PATH\tLimits device discovery to USB port(s) specified by path\n"
		"            -ms SN\tMonitor the serial number prefix of the device\n"
		"            -t #\tSeconds to wait for a device to appear [what is default timeout?]\n"
		"            -T #\tSeconds to wait for a device to appeared at stage switch [for continuous mode?]\n"
		"            -e KEY=VAL\tSet environment variable named KEY to value VAL\n"
		"            -pp #\tUSB polling period in milliseconds [what is default?]\n"
		"            -dm\t\tDisable small memory [what does that mean?]\n"
		"    install [OPTION...] --script|-s BUILTIN|PATH [PARAM...]\n"
		"        Install via built-in or custom file script and optionally passing parameters\n"
		"        BUILTIN\t\tBuilt-in script name: $BUILTIN_NAMES\n"
		"        PATH\t\tPath to a script file\n"
		"        PARAM\t\tScript parameter\n"
		"        OPTION\t\tSee above\n"
		"    install [OPTION...] PROTOCOL: [ARG...]\n"
		"        Run a protocol command; see 'help protocol-commands'\n"
		"        OPTION\t\tSee above except for --dry-run, --continuous, --interactive\n"
		"    ls-devices\n"
		"        List connected devices\n"
		"    ls-builtin [BUILTIN]\n"
		"        List built-in script; all if none specified\n"
		"    cat-builtin BUILTIN\n"
		"        Output built-in script content\n"
#ifdef _WIN32
		"    device-discovery-by-serial ignore|match\n"
		"        Modify Windows registry to ignore or match (undo ignore) USB serial number when finding devices\n"
#endif
		"$HELP_COMMANDS"
		"    -h\tOutput basic help\n"
		"\n"
		"Examples:\n"
		"    uuu install subdir\n"
		"        Install from a subdirectory that contains an uuu.auto file.\n"
		"\n"
		"    uuu install foo.zip\n"
		"        Install from a zip file that contains an uuu.auto file.\n"
		"\n"
		"    uuu install -s emmc_all foo.wic\n"
		"        Install by running built-in script emmc_all and passing foo.wic as a parameter.\n"
		"\n"
		"    uuu install SDPS: boot -f flash.bin\n"
		"        Execute SDPS protocol command.\n"
		"\n"
		"    uuu ls-devices\n"
		"        List connected devices.\n"
		"\n"
		"    uuu install -m 1:2 -m 1:3 foo.zip\n"
		"        Limit device discovery to USB paths 1:2 and 1:3. Use ls-devices to discover paths.\n"
		"\n";
	
	std::string help_context_text;
	{
		std::ostringstream ss;
		for (auto& item : get_help_handlers())
		{
			std::string key = std::get<0>(item);
			std::string info = std::get<2>(item);
			ss << "    help " << key << "\n    \tOutput info for " << info << std::endl;
		}
		help_context_text = ss.str();
	}

	text = string_man::replace(text, "$HELP_COMMANDS", help_context_text);
	text = string_man::replace(text, "$BUILTIN_NAMES", g_ScriptCatalog.get_names());
	std::cout << text;
}

static void print_old_cli_help()
{
	std::string text =
		"Default mode:\n"
		"uuu [OPTION...] SPEC|CMD|BOOTLOADER\n"
		"    SPEC\tSpecifies a script to run without parameters; use -b for parameters;\n"
		"    \t\tFor a directory, use contained uuu.auto at root;\n"
		"    \t\tFor a zip file, use uuu.auto at root of content\n"
		"    CMD\t\tRun a command; see -h-protocol-commands\n"
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
		"    -dm\t\tDisable small memory\n"
		"\n"
		"Parameter & built-in script mode:\n"
		"uuu [OPTION...] -b SPEC|BUILTIN [PARAM...]\n"
		"    SPEC\tSame as for default mode\n"
		"    BUILTIN\tBuilt-in script: $BUILTIN_NAMES\n"
		"    OPTION...\tSame as for default mode\n"
		"    PARAM...\tScript parameter values\n"
		"\n"
		"Special modes:\n"
		"uuu -ls-devices\tList connected devices\n"
		"uuu -ls-builtin [BUILTIN]\n"
		"    \t\tList built-in script; all if none specified\n"
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

	std::cout << std::endl << string_man::replace(text, "$BUILTIN_NAMES", g_ScriptCatalog.get_names());
}

static void (*print_cli_help)() = nullptr;

/**
 * @brief Writes the name, description and arguments to stdout
 */
static void print_definition(const Script& script)
{
	std::string id_highlight = g_vt->fg_light_green;
	std::string key_highlight = g_vt->fg_cyan;
	std::string no_highlight = g_vt->fg_default;
	std::cout << id_highlight << script.name << g_vt->fg_default << ": " << script.desc << std::endl;
	for (auto& arg : script.args)
	{
		std::string desc = id_highlight + arg.name + no_highlight;
		if (!arg.default_value_arg_name.empty())
		{
			desc += " " +
				key_highlight + "[default=" +
				id_highlight + arg.default_value_arg_name +
				key_highlight + "]" +
				no_highlight;
		}
		std::cout << "\t" << desc << ": " << arg.desc << std::endl;
	}
}

static void print_script_catalog(const std::string& script_name) {
	auto& items = g_ScriptCatalog.get_items();
	if (script_name.empty())
	{
		std::cout << "Built-in scripts:" << std::endl;
		for (const auto& item : items)
		{
			print_definition(item.second);
		}
	}
	else
	{
		auto item = items.find(script_name);
		if (item == items.end())
		{
			g_logger.log_error("Unknown script: " + script_name);
			exit_for_status(EXIT_FAILURE);
		}
		print_definition(item->second);
	}
}

static void print_protocol_help() {
	std::cout << command_help_text << std::endl;
}

static int print_cfg(const char *pro, const char * chip, const char * /*compatible*/, uint16_t vid, uint16_t pid, uint16_t bcdmin, uint16_t bcdmax, void * /*p*/)
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

static void print_protocol_support_info() {
	printf("Protocol support for devices:\n");
	printf("\tPctl\t Chip\t\t Vid\t Pid\t BcdVersion\t Serial_No\n");
	printf("\t==================================================\n");
	uuu_for_each_cfg(print_cfg, NULL);
}

static int print_udev_rule(const char * /*pro*/, const char * /*chip*/, const char * /*compatible*/,
	uint16_t vid, uint16_t pid, uint16_t /*bcdmin*/, uint16_t /*bcdmax*/, void * /*p*/)
{
	printf("SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"%04x\", ATTRS{idProduct}==\"%04x\", TAG+=\"uaccess\"\n",
			vid, pid);
	return EXIT_SUCCESS;
}

static void print_udev_info()
{
	uuu_for_each_cfg(print_udev_rule, NULL);

	std::cerr << std::endl <<
		"Enable udev rules via:" << std::endl <<
		"\tsudo sh -c \"uuu -udev >> /etc/udev/rules.d/70-uuu.rules\"" << std::endl <<
		"\tsudo udevadm control --reload" << std::endl <<
		"Note: These instructions output to standard error so are excluded from redirected standard output" << std::endl << std::endl;
}

static std::string get_discovery_filter()
{
	std::vector<std::string> filters;
	if (!g_transfer_context.usb_path_filter.empty())
	{
		filters.push_back("at path: " + string_man::join(g_transfer_context.usb_path_filter, "|"));
	}
	if (!g_transfer_context.usb_serial_no_filter.empty())
	{
		filters.push_back("with serial#: " + string_man::join(g_transfer_context.usb_serial_no_filter, "|"));
	}
	if (filters.empty()) return "";
	return " (" + string_man::join(filters, "; ") + ")";
}

static const help_handler_t get_help_handlers()
{
	const help_handler_t help_handlers
	{
		{ "cli", print_cli_help, "command line use" },
		{ "protocol-support", print_protocol_support_info, "protocol support by device" },
		{ "protocol-commands", print_protocol_help, "protocol command use" },
		{ "auto-complete", print_autocomplete_help, "auto/tab completion setup" },
	#ifndef _WIN32
		{ "udev", print_udev_info, "Linux udev rule info for avoiding using sudo each time" },
	#endif
	};
	return help_handlers;
}

static void process_interactive_commands()
{
	int uboot_cmd = 0;
	std::string prompt = "U>";

	std::cout << "Please input command: " << std::endl;
	std::string cmd;
	std::ofstream log("uuu.inputlog", std::ofstream::binary);
	log << "uuu_version "
		<< ((uuu_get_version() & 0xFF000000) >> 24)
		<< "."
		<< ((uuu_get_version() & 0xFFF000) >> 12)
		<< "."
		<< ((uuu_get_version() & 0xFFF))
		<< std::endl;
	while (1)
	{
		std::cout << prompt;
		std::getline(std::cin, cmd);

		if (cmd == "uboot")
		{
			uboot_cmd = 1;
			prompt = "=>";
			std::cout << "Enter into u-boot cmd mode" << std::endl;
			std::cout << "Okay" << std::endl;
		}
		else if (cmd == "exit" && uboot_cmd == 1)
		{
			uboot_cmd = 0;
			prompt = "U>";
			std::cout << "Exit u-boot cmd mode" << std::endl;
			std::cout << "Okay" << std::endl;
		}
		else if (cmd == "help" || cmd == "?")
		{
			print_cli_help();
		}
		else if (cmd == "q" || cmd == "quit")
		{
			return;
		}
		else
		{
			log << cmd << std::endl;
			log.flush();

			if (uboot_cmd)
				cmd = "fb: ucmd " + cmd;

			int ret = uuu_run_cmd(cmd.c_str(), 0);
			if (ret)
				std::cout << uuu_get_last_err_string() << std::endl;
			else
				std::cout << "Okay" << std::endl;
		}
	}
}

/**
 * @brief Writes the content of a script to stdout
 */
static void print_content(const Script& script) {
	std::string text = script.text;
	while (text.size() > 0 && (text[0] == '\n' || text[0] == ' '))
		text = text.erase(0, 1);
	std::cout << text;
}

static int print_device_info(const char *path, const char *chip, const char *pro, uint16_t vid, uint16_t pid, uint16_t bcd, const char *serial_no, void * /*p*/)
{
	printf("%s\t%s\t%s\t0x%04X\t0x%04X\t0x%04X\t%s\n", path, chip, pro, vid, pid, bcd, serial_no);
	return EXIT_SUCCESS;
}

static void print_device_list()
{
	std::cout << "Path\tChip\tPro.\tVid\tPid\tVersion\tSerial#" << std::endl;
	std::cout << "----\t----\t----\t---\t---\t-------\t-------" << std::endl;
	uuu_for_each_devices(print_device_info, NULL);
}

static constexpr ScriptConfig builtin_script_configs[] =
{
	{
		"emmc",
#include "emmc_burn_loader.clst"
		,"burn boot loader to eMMC boot partition"
	},
	{
		"emmc_all",
#include "emmc_burn_all.clst"
		,"burn whole image to eMMC"
	},
	{
		"fat_write",
#include "fat_write.clst"
		,"update one file in fat partition, require uboot fastboot running in board"
	},
	{
		"nand",
#include "nand_burn_loader.clst"
		,"burn boot loader to NAND flash"
	},
	{
		"qspi",
#include "qspi_burn_loader.clst"
		,"burn boot loader to qspi nor flash"
	},
	{
		"spi_nand",
#include "fspinand_burn_loader.clst"
		,"burn boot loader to spi nand flash"
	},
	{
		"sd",
#include "sd_burn_loader.clst"
		,"burn boot loader to sd card"
	},
	{
		"sd_all",
#include "sd_burn_all.clst"
		,"burn whole image to sd card"
	},
	{
		"spl",
#include "spl_boot.clst"
		,"boot spl and uboot"
	},
	{
		"nvme_all",
#include "nvme_burn_all.clst"
		,"burn whole image io nvme storage"
	},
	{
		nullptr,
		nullptr,
		nullptr,
	}
};

//! Script catalog global instance
ScriptCatalog g_ScriptCatalog(builtin_script_configs);

static std::string load_script_text(
	const std::string& script_spec,
	const std::vector<std::string>& args,
	std::string& script_name_feedback)
{
	script_name_feedback = script_spec;
	const Script* script = g_ScriptCatalog.find(script_spec);
	if (!script) {
		script_name_feedback += " (custom)";
		script = g_ScriptCatalog.add_from_file(script_spec);
		if (!script)
		{
			g_logger.log_error("Unable to load script from file: " + script_spec);
			exit_for_status(EXIT_FAILURE);
		}
	}
	else
	{
		script_name_feedback += " (built-in)";
	}

	if (args.size() > script->args.size())
	{
		g_logger.log_error("Too many parameters for script: " + args[script->args.size()]);
		exit_for_status(EXIT_FAILURE);
	}

	const std::string text = script->replace_arguments(args);

	g_logger.log_debug([&]()
	{
		std::string message(text);
		string_man::trim(message);
		string_man::replace(message, "\n", "\n\t");
		return "Script content (with parameters replaced with values):\n\t" + message;
	});

	return text;
}

static void handle_help(const std::vector<std::string>& args)
{
	if (args.size() > 1)
	{
		exit_for_syntax_error("Too many arguments: " + args[1]);
	}

	const auto help_handlers = get_help_handlers();
	if (args.size() == 0)
	{
		exit_for_syntax_error("Missing help context; options: " + string_man::join_keys(help_handlers));
	}
	else
	{
		const std::string context = args[0];
		const auto& handler = std::find_if(help_handlers.begin(), help_handlers.end(), [&](const auto& item) {
			return std::get<0>(item) == context;
			});
		if (handler == help_handlers.end())
		{
			exit_for_syntax_error("Unknown help context '" + context + "'; options: " + string_man::join_keys(help_handlers));
		}
		std::get<1>(*handler)();
	}
	exit_for_status(EXIT_SUCCESS);
}

class InstallConfig final
{
public:
	bool enter_interactive_after_action = false;
	std::string input_path;
	std::string protocol_cmd;
	std::string script_text;
	std::string script_text_source;

	InstallConfig(std::vector<std::string> args)
	{
		while (!args.empty())
		{
			std::string arg = args.front();
			args.erase(args.begin());
			if (!arg.empty() && arg[0] == '-')
			{
				std::string opt = arg.substr(1);
				if (arg.size() > 2)
				{
					if (arg[1] == '-')
					{
						opt = arg.substr(2);
					}
					else
					{
						exit_for_syntax_error("Options are either single dash with single letter or multiple dashes; got: " + arg);
					}
				}
				if (opt == "continuous")
				{
					is_continuous_mode = true;
					uuu_set_small_mem(0);
					g_logger.log_debug([&]() { return "Continuous mode enabled"; });
				}
				else if (opt == "small-memory")
				{
					uuu_set_small_mem(0);
					g_logger.log_debug([&]() { return "Small memory enabled"; });
				}
				else if (opt == "i" || opt == "interactive")
				{
					enter_interactive_after_action = true;
					enable_stream_feedback();
					g_logger.log_debug([&]() { return "Interactive mode enabled"; });
				}
				else if (opt == "v")
				{
					g_verbose = 1;
					g_logger.log_debug([&]() { return "Verbose enabled"; });
				}
				else if (opt == "V")
				{
					g_verbose = 1;
					uuu_set_debug_level(2);
					g_logger.log_debug([&]() { return "Verbose+ enabled"; });
				}
				else if (opt == "dry")
				{
					dry_run = true;
					enable_stream_feedback();
					g_logger.log_debug([&]() { return "Dry run enabled"; });
				}
				else if (opt == "filter-path")
				{
					if (args.empty())
					{
						exit_for_syntax_error("Missing USB path");
					}
					std::string path = args.front();
					args.erase(args.begin());
					uuu_add_usbpath_filter(path.c_str());
					g_transfer_context.usb_path_filter.push_back(path);
					g_logger.log_debug([&]() { return "Added device filter for USB path '" + path + "'"; });
				}
				else if (opt == "filter-serial")
				{
					if (args.empty())
					{
						exit_for_syntax_error("Missing serial #");
					}
					std::string sn = args.front();
					args.erase(args.begin());
					uuu_add_usbserial_no_filter(sn.c_str());
					g_transfer_context.usb_serial_no_filter.push_back(sn);
					g_logger.log_debug([&]() { return "Added device filter for serial# '" + sn + "'"; });
				}
				else if (opt == "t")
				{
					if (args.empty())
					{
						exit_for_syntax_error("Missing seconds");
					}
					std::string period_text = args.front();
					args.erase(args.begin());
					auto period = atoi(period_text.c_str());
					uuu_set_wait_timeout(period);
					g_logger.log_debug([&]() { return "Set wait timeout to " + std::to_string(period); });
				}
				else if (opt == "T")
				{
					if (args.empty())
					{
						exit_for_syntax_error("Missing seconds");
					}
					std::string period_text = args.front();
					args.erase(args.begin());
					auto period = atoi(period_text.c_str());
					uuu_set_wait_next_timeout(period);
					g_logger.log_debug([&]() { return "Set next wait timeout to " + std::to_string(period); });
				}
				else if (opt == "pp")
				{
					if (args.empty())
					{
						exit_for_syntax_error("Missing milliseconds");
					}
					std::string period_text = args.front();
					args.erase(args.begin());
					auto period = atoi(period_text.c_str());
					uuu_set_poll_period(period);
					g_logger.log_debug([&]() { return "Set poll period to " + std::to_string(period); });
				}
				else if (opt == "bmap")
				{
					g_bmap_mode = bmap_mode::Force;
					g_logger.log_debug([&]() { return "Set bmap to force"; });
				}
				else if (opt == "no-bmap")
				{
					g_bmap_mode = bmap_mode::Ignore;
					g_logger.log_debug([&]() { return "Set bmap to ignore"; });
				}
				else if (opt == "e")
				{
					if (args.empty())
					{
						exit_for_syntax_error("Missing key=value");
					}
					std::string spec = args.front();
					args.erase(args.begin());
					size_t equal_pos = spec.find("=");
					if (equal_pos == std::string::npos)
					{
						g_logger.log_error("Invalid input '" + spec + "'; must be formatted as: key=value");
						exit_for_status(EXIT_FAILURE);
					}
					std::string name = spec.substr(0, equal_pos);
					std::string value = spec.substr(equal_pos + 1);
					if (environment::set_environment_variable(name, value))
					{
						g_logger.log_error("Failed to set environment variable with expression '" + spec + "'");
						exit_for_status(EXIT_FAILURE);
					}
					g_logger.log_debug([&]() { return "Set environment variable '" + name + "' to '" + value + "'"; });
				}
				else if (opt == "s" || opt == "script")
				{
					if (args.empty())
					{
						exit_for_syntax_error("Missing path or built-in script name");
					}

					std::string script_spec = args.front();
					args.erase(args.begin());
					script_text = load_script_text(script_spec, args, script_text_source);
					g_logger.log_debug([&]() { return "Loaded script: " + script_text_source; });
					args.clear();
					break;
				}
				else
				{
					exit_for_syntax_error("Unknown option: " + arg);
				}
			}
			else if (!arg.empty() && arg[arg.size() - 1] == ':')
			{
				// treat as a protocol command
				const std::string delim = " ";
				protocol_cmd = arg + delim;
				for (auto& cmd_arg : args)
				{
					if (cmd_arg.find(' ') != std::string::npos)
					{
						cmd_arg.insert(cmd_arg.begin(), '"');
						cmd_arg.insert(cmd_arg.end(), '"');
					}
					protocol_cmd.append(cmd_arg);
					protocol_cmd.append(delim);
				}
				// remove last delim
				protocol_cmd.erase(protocol_cmd.size() - delim.size());
				g_logger.log_debug([&]() { return "Protocol command: '" + protocol_cmd + "'"; });
				args.clear();
				break;
			}
			else
			{
				input_path = arg;
				g_logger.log_debug([&]() { return "Input path: " + input_path; });
				break;
			}
		}

		if (!args.empty())
		{
			exit_for_syntax_error("Too many arguments: " + args[0]);
		}

		if (is_continuous_mode && enter_interactive_after_action)
		{
			exit_for_syntax_error("Incompatible options: continuous and interactive");
		}

		if (is_continuous_mode && dry_run)
		{
			exit_for_syntax_error("Incompatible options: continuous and dry-run");
		}

		if (enter_interactive_after_action && dry_run)
		{
			exit_for_syntax_error("Incompatible options: interactive and dry-run");
		}
	}
};

static void handle_install(const std::vector<std::string>& args)
{
	InstallConfig config(args);

	// note: probably need the cursor visible for this
	uuu_set_askpasswd(environment::ask_passwd);

	g_logger.log_info("Waiting for device..." + get_discovery_filter());
	std::unique_ptr<CursorHider> cursor_hider;
	if (!g_verbose)
	{
		cursor_hider = std::make_unique<CursorHider>();
	}

	transfer_feedback.enable();

	if (!config.protocol_cmd.empty())
	{
		//g_logger.log_debug([&]() { return "Executing command: " + config.protocol_cmd; });
		int ret = uuu_run_cmd(config.protocol_cmd.c_str(), dry_run);

		// what is the purpose of printing blank lines? Don't know about success, but on error, there are several blank lines on screen
		for (size_t i = 0; i < g_transfer_context.map_path_nt.size() + 3; i++)
			printf("\n");

		if (ret)
		{
			g_logger.log_error(uuu_get_last_err_string());
			exit_for_status(EXIT_FAILURE);
		}

		g_logger.log_info("Command succeeded :)");

		if (config.enter_interactive_after_action)
		{
			process_interactive_commands();
		}

		exit_for_status(EXIT_SUCCESS);
	}
	else if (!config.script_text.empty())
	{
		//g_logger.log_debug([&]() { return "Running script: " + config.script_text_source; });
		if (uuu_run_cmd_script(config.script_text.c_str(), dry_run))
		{
			g_logger.log_error(uuu_get_last_err_string());
			exit_for_status(EXIT_FAILURE);
		}

		// wait for queued commands to complete
		if (uuu_wait_uuu_finish(is_continuous_mode, dry_run))
		{
			g_logger.log_error(uuu_get_last_err_string());
			exit_for_status(EXIT_FAILURE);
		}
	}
	else if (!config.input_path.empty())
	{
		//g_logger.log_debug([&]() { return "Running as auto detect file: " + config.input_path; });
		if (uuu_auto_detect_file(config.input_path.c_str()))
		{
			g_logger.log_error(uuu_get_last_err_string());
			exit_for_status(EXIT_FAILURE);
		}

		// wait for queued commands to complete
		if (uuu_wait_uuu_finish(is_continuous_mode, dry_run))
		{
			g_logger.log_error(uuu_get_last_err_string());
			exit_for_status(EXIT_FAILURE);
		}
	}
	else if (!config.enter_interactive_after_action)
	{
		exit_for_syntax_error("No operation");
	}

	if (config.enter_interactive_after_action)
	{
		cursor_hider.reset();
		process_interactive_commands();
	}

	// wait for the thread exit, after send out CMD_DONE
	// [why is wait needed?]
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// move cursor below status area if not in streaming status mode
	if (!g_verbose)
	{
		// NOTE: the last status output seems to be to success/fail line which is 4 lines above the end of the status area
		// NOTE: old version only output 3; not 4
		printf("\n\n\n\n");
	}
}

static void handle_list_devices(const std::vector<std::string>& args)
{
	if (args.size() > 0)
	{
		exit_for_syntax_error("Too many arguments: " + args[0]);
	}

	print_device_list();
	exit_for_status(EXIT_SUCCESS);
}

static void handle_list_builtin(const std::vector<std::string>& args)
{
	if (args.size() > 1)
	{
		exit_for_syntax_error("Too many arguments: " + args[1]);
	}

	std::string script_name;
	if (args.size() == 1)
	{
		script_name = args[0];
	}
	print_script_catalog(script_name);
	exit_for_status(EXIT_SUCCESS);
}

static void handle_print_builtin(const std::vector<std::string>& args)
{
	if (args.size() > 1)
	{
		exit_for_syntax_error("Too many arguments: " + args[1]);
	}
	if (args.size() < 1)
	{
		exit_for_syntax_error("Missing built-in script name; options: " + g_ScriptCatalog.get_names());
	}
	std::string script_name = args[0];
	const Script* script = g_ScriptCatalog.find(script_name);
	if (!script)
	{
		exit_for_syntax_error("Unknown built-in script '" + script_name + "'; options: " + g_ScriptCatalog.get_names());
	}
	print_content(*script);
	exit_for_status(EXIT_SUCCESS);
}

#ifdef _WIN32

static void handle_setup_serial_matching(const std::vector<std::string>& args)
{
	if (args.size() < 1)
	{
		exit_for_syntax_error("Missing match/ignore");
	}
	if (args.size() > 1)
	{
		exit_for_syntax_error("Too many arguments: " + args[1]);
	}

	const std::string arg = args[0];
	environment::SerialMatchConfig serial_match_config;
	if (arg == "match")
	{
		exit_for_status(serial_match_config.match());
	}
	else if (arg == "ignore")
	{
		exit_for_status(serial_match_config.ignore());
	}
	else
	{
		exit_for_syntax_error("Expected match/ignore; got '" + arg + "'");
	}
}

#endif

typedef std::tuple<std::string, std::function<void(const std::vector<std::string>&)>> command_handler_t;
static std::vector<command_handler_t> command_handlers
{
	{ "help", handle_help },
	{ "install", handle_install },
	{ "ls-devices", handle_list_devices },
	{ "ls-builtin", handle_list_builtin },
	{ "cat-builtin", handle_print_builtin },
#ifdef _WIN32
	{ "device-discovery-by-serial", handle_setup_serial_matching },
#endif
};

static void process_new_command_line(int argc, char** argv)
{
	if (argc == 1)
	{
		print_cli_help();
		exit_for_status(EXIT_FAILURE);
	}

	const std::string arg = argv[1];

	if (arg == "-h")
	{
		print_cli_help();
		exit_for_status(EXIT_SUCCESS);
	}
	else
	{
		const auto& handler = std::find_if(command_handlers.begin(), command_handlers.end(), [&](const auto& item) {
			return std::get<0>(item) == arg;
		});
		if (handler == command_handlers.end())
		{
			exit_for_syntax_error("Unknown command '" + arg + "'; options: " + string_man::join_keys(command_handlers));
		}

		std::vector<std::string> args;
		for (int j = 2; j < argc; j++)
		{
			args.push_back(argv[j]);
		}
		auto& handle = std::get<1>(*handler);
		handle(args);
	}
}

static void process_old_command_line(int argc, char** argv)
{
	if (argc == 1)
	{
		print_cli_help();
		exit_for_status(EXIT_FAILURE);
	}

	bool is_shell_mode = false;
	std::string input_path;
	std::string protocol_cmd;
	std::string script_name_feedback;
	std::string script_text;

	for (int i = 1; i < argc; i++)
	{
		const std::string arg = argv[i];
		if (!arg.empty() && arg[0] == '-')
		{
			const std::string opt = arg.substr(1);
			if (opt == "b" || opt == "brun")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing path or built-in script name");
				}

				std::vector<std::string> args;
				for (int j = i + 1; j < argc; j++)
				{
					args.push_back(argv[j]);
				}

				script_text = load_script_text(argv[i], args, script_name_feedback);
				break;
			}
			else if (opt == "h")
			{
				print_cli_help();
				exit_for_status(EXIT_SUCCESS);
			}
			else if (opt == "h-auto-complete")
			{
				print_autocomplete_help();
				exit_for_status(EXIT_SUCCESS);
			}
			else if (opt == "h-protocol-commands")
			{
				print_protocol_help();
				exit_for_status(EXIT_SUCCESS);
			}
			else if (opt == "h-protocol-support")
			{
				print_protocol_support_info();
				exit_for_status(EXIT_SUCCESS);
			}
			else if (opt == "ls-devices" || opt == "lsusb")
			{
				print_device_list();
				exit_for_status(EXIT_SUCCESS);
			}
			else if (opt == "ls-builtin")
			{
				std::string script_name;
				if (++i < argc)
				{
					script_name = argv[i];
				}
				print_script_catalog(script_name);
				exit_for_status(EXIT_SUCCESS);
			}
			else if (opt == "cat-builtin" || opt == "bshow")
			{
				if (2 == argc)
				{
					exit_for_syntax_error("Missing built-in script name; options: " + g_ScriptCatalog.get_names());
				}
				std::string script_name = argv[2];
				const Script* script = g_ScriptCatalog.find(script_name);
				if (!script)
				{
					exit_for_syntax_error("Unknown built-in script '" + script_name + "'; options: " + g_ScriptCatalog.get_names());
				}
				print_content(*script);
				exit_for_status(EXIT_SUCCESS);
			}
			else if (opt == "d")
			{
				is_continuous_mode = true;
				uuu_set_small_mem(0);
			}
			else if (opt == "dm")
			{
				uuu_set_small_mem(0);
			}
			else if (opt == "s")
			{
				is_shell_mode = true;
				enable_stream_feedback();
			}
			else if (opt == "v")
			{
				g_verbose = 1;
			}
			else if (opt == "V")
			{
				g_verbose = 1;
				uuu_set_debug_level(2);
			}
			else if (opt == "dry")
			{
				dry_run = true;
				enable_stream_feedback();
			}
			else if (opt == "m")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing USB path argument");
				}
				uuu_add_usbpath_filter(argv[i]);
				g_transfer_context.usb_path_filter.push_back(argv[i]);
			}
			else if (opt == "ms")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing serial # argument");
				}
				uuu_add_usbserial_no_filter(argv[i]);
				g_transfer_context.usb_serial_no_filter.push_back(argv[i]);
			}
			else if (opt == "t")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing seconds argument");
				}
				uuu_set_wait_timeout(atol(argv[i]));
			}
			else if (opt == "T")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing seconds argument");
				}
				uuu_set_wait_next_timeout(atol(argv[i]));
			}
			else if (opt == "pp")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing milliseconds argument");
				}
				uuu_set_poll_period(atol(argv[i]));
			}
			else if (opt == "bmap")
			{
				g_bmap_mode = bmap_mode::Force;
			}
			else if (opt == "no-bmap")
			{
				g_bmap_mode = bmap_mode::Ignore;
			}
			else if (opt == "e")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing key=value argument");
				}
				std::string spec = argv[i];
				size_t equal_pos = spec.find("=");
				if (equal_pos == std::string::npos)
				{
					g_logger.log_error("Invalid input '" + spec + "'; must be formatted as: key=value");
					exit_for_status(EXIT_FAILURE);
				}
				std::string name = spec.substr(0, equal_pos);
				std::string value = spec.substr(equal_pos + 1);
				if (environment::set_environment_variable(name, value))
				{
					g_logger.log_error("Failed to set environment variable with expression '" + spec + "'");
					exit_for_status(EXIT_FAILURE);
				}
				g_logger.log_debug([&]() { return "Set environment variable '" + name + "' to '" + value + "'"; });
			}
#ifdef _WIN32
			else if (opt == "IgSerNum")
			{
				environment::SerialMatchConfig serial_match_config;
				serial_match_config.ignore();
			}
#else
			else if (opt == "udev")
			{
				print_udev_info();
				exit_for_status(EXIT_SUCCESS);
			}
#endif
			else
			{
				exit_for_syntax_error("Unknown option: " + arg);
			}
		}
		else if (!arg.empty() && arg[arg.size() - 1] == ':')
		{
			g_logger.log_debug([&]() { return "Treat as a protocol command"; });
			for (int j = i; j < argc; j++)
			{
				std::string cmd_arg = argv[j];
				if (cmd_arg.find(' ') != std::string::npos && cmd_arg[cmd_arg.size() - 1] != ':')
				{
					cmd_arg.insert(cmd_arg.begin(), '"');
					cmd_arg.insert(cmd_arg.end(), '"');
				}
				protocol_cmd.append(cmd_arg);
				if (j != (argc - 1)) // don't add space after last arg
					protocol_cmd.append(" ");
			}
			break;
		}
		else
		{
			g_logger.log_debug([&]() { return "Treat as a file system path"; });
			if (argc - 1 > i)
			{
				exit_for_syntax_error("Too many arguments: " + std::string(argv[i + 1]) + "; Hint: use -b to pass parameters to a script");
			}
			input_path = arg;
		}
	}

	if (is_continuous_mode && is_shell_mode)
	{
		exit_for_syntax_error("Incompatible options: deamon (-d) and shell (-s)");
	}

	if (is_continuous_mode && dry_run)
	{
		exit_for_syntax_error("Incompatible options: deamon (-d) and dry-run (-dry)");
	}

	if (is_shell_mode && dry_run)
	{
		exit_for_syntax_error("Incompatible options: shell (-s) and dry-run (-dry)");
	}

	if (g_verbose)
	{
		// commented out since can print script content via -cat-builtin; print here is noise
		//if (!cmd_script.empty())
		//	printf("\n%sRunning built-in script:%s\n %s\n\n", g_vt->boldwhite, g_vt->default_foreground, cmd_script.c_str());

		// why not log for !shell? it's logged for !g_verbose regardless
		if (!is_shell_mode) {
			g_logger.log_info("Waiting for device..." + get_discovery_filter());
		}
	}
	else {
		std::cout << "Waiting for device..." << get_discovery_filter();
		
		// move cursor to begining of line so that next output will overwrite waiting line;
		// NOTE: This is nice when it works since it erases the transitory state of waiting, 
		// but if the next line is shorter than the waiting line, then the portion of the waiting
		// line that is longer will remain after the next line is output. 
		// FWIW I think it's better to leave the waiting msg on screen to than allow a partial msg
		// to show and think that \r sould be replaed with \n
		std::cout << "\r";
		
		// hide the cursor
		std::cout << g_vt->hide_cursor;
		
		// is this needed?
		std::cout.flush();
	}

	uuu_set_askpasswd(environment::ask_passwd);
	transfer_feedback.enable();

	if (is_shell_mode)
	{
		process_interactive_commands();
		exit_for_status(EXIT_SUCCESS);
	}
	else if (!protocol_cmd.empty())
	{
		g_logger.log_debug([&]() { return "Executing command: " + protocol_cmd; });
		int ret = uuu_run_cmd(protocol_cmd.c_str(), dry_run);

		// what is the purpose of printing blank lines? Don't know about success, but on error, there are several blank lines on screen
		for (size_t i = 0; i < g_transfer_context.map_path_nt.size() + 3; i++)
			printf("\n");

		if (ret)
		{
			g_logger.log_error(uuu_get_last_err_string());
			exit_for_status(EXIT_FAILURE);
		}

		g_logger.log_info("Command succeeded :)");

		if (is_shell_mode) process_interactive_commands();

		exit_for_status(EXIT_SUCCESS);
	}
	else if (!script_text.empty())
	{
		if (script_name_feedback.empty())
		{
			g_logger.log_internal_error("Expected non-empty script_spec");
			exit_for_status(EXIT_FAILURE);
		}
		g_logger.log_debug([&]() { return "Running script: " + script_name_feedback; });
		if (uuu_run_cmd_script(script_text.c_str(), dry_run))
		{
			g_logger.log_error(uuu_get_last_err_string());
			exit_for_status(EXIT_FAILURE);
		}
	}
	else if (!input_path.empty())
	{
		g_logger.log_debug([&]() { return "Running as auto detect file: " + input_path; });
		if (uuu_auto_detect_file(input_path.c_str()))
		{
			g_logger.log_error(uuu_get_last_err_string());
			exit_for_status(EXIT_FAILURE);
		}
	}
	else
	{
		exit_for_syntax_error("No operation");
	}

	if (uuu_wait_uuu_finish(is_continuous_mode, dry_run))
	{
		g_logger.log_error(uuu_get_last_err_string());
		exit_for_status(EXIT_FAILURE);
	}

	// wait for the thread exit, after send out CMD_DONE
	// [why is wait needed?]
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// move cursor below status area if not in streaming status mode
	if (!g_verbose)
	{
		printf("\n\n\n");
	}
}

int main(int argc, char** argv)
{
	// handle auto-complete
	if (auto_complete(argc, argv) == 0) return EXIT_SUCCESS;

	// setup ctrl-c interrupt
	signal(SIGINT, exit_for_interrupt);

	print_app_title();
	// enable rich output if possible
	if (g_vt->enable())
	{
		g_logger.is_color_output_enabled = true;
	}
	else
	{
		// note: if output is re-directed to a file, then output does not supports color.
		// There may be other ways color is not supported.
		g_logger.log_warning("Rich output is not supported; feedback will be monochrome and stream-based (not overwriting)");

		enable_stream_feedback();
	}

	bool use_new_cli = true;
	if (use_new_cli)
	{
		print_cli_help = print_new_cli_help;
		process_new_command_line(argc, argv);
	}
	else
	{
		CursorRestorer cursor_restorer;
		print_cli_help = print_old_cli_help;
		process_old_command_line(argc, argv);
	}

	// NOTE: should only get here if installed a file or ran a script

	// log and return install result
	// FYI: g_transfer_context.overall_status indicate failure if any USB command failed
	g_logger.log_info(g_transfer_context.overall_status == 0 ? "Success :)" : "Failed :(");
	return g_transfer_context.overall_status;
}
