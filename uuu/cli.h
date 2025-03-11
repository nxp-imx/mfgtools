//! @file
//! @brief New command line interface (CLI)

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
#include <thread>
#include <tuple>
#include <vector>

int auto_complete(int argc, char **argv);
void print_autocomplete_help();

Logger g_logger;
std::shared_ptr<VtEmulation> g_vt = std::make_shared<PlatformVtEmulation>();
TransferContext g_transfer_context;

static bool dry_run = false;
static bool is_continuous_mode = false;
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
	// NOTE: the cursor might be hidden at this time since it is hidden while doing transfer operations
	std::cout << g_vt->show_cursor;

	// provide feedback on overall status
	g_logger.log_debug([&]() { return status == EXIT_SUCCESS ? "Success :)" : "Failed :("; });

	// add a blank line; to stderr so does not go to a file if stdout is redirected
	std::cerr << std::endl;

	exit(status);
}

/**
 * @brief Exits process for a runtime error
 */
[[noreturn]]
static void exit_for_runtime_error(const std::string& message)
{
	g_logger.log_error(message);
	exit_for_status(EXIT_FAILURE);
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
 * @details
 * Attach to the interrupt (ctrl-c) signal.
 * 
 * Without this handling for interrupt, the process would exit as desired, but the cursor might
 * not be visible and this prints a cool red message.
 */
[[noreturn]]
static void exit_for_interrupt(int)
{
	std::cout << g_vt->fg_error() << "INTERRUPTED" << g_vt->fg_default << std::endl;
	exit_for_status(EXIT_FAILURE);
}

static void set_environment_variable(const std::string& spec)
{
	size_t equal_pos = spec.find("=");
	if (equal_pos == std::string::npos)
	{
		exit_for_syntax_error("Invalid input '" + spec + "'; must be formatted as: key=value");
	}
	std::string name = spec.substr(0, equal_pos);
	std::string value = spec.substr(equal_pos + 1);
	if (environment::set_environment_variable(name, value))
	{
		exit_for_runtime_error("Failed to set environment variable with expression '" + spec + "'");
	}
	g_logger.log_debug([&]() { return "Set environment variable '" + name + "' to '" + value + "'"; });
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

static void print_cmd_cli_help()
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
		"            --interactive | -i\n"
		"            \t\tEnter interactive mode after install action (if any);\n"
		"            \t\tFYI: commands are written to uuu.inputlog for future playback\n"
		"            -v\t\tVerbose feedback streams status instead of overwriting on a single line\n"
		"            -V\t\tExtend verbose feedback to include USB library feedback\n"
		"            --bmap\tUse .bmap files even if flash commands do not specify them\n"
		"            --no-bmap\tIgnore .bmap files even if flash commands specify them\n"
		"            --filter-path USB_PATH\n"
		"            \t\tLimits device discovery to a USB port path; can include multiple\n"
		"            --filter-serial SN\n"
		"            \t\tLimits device discovery to a serial# prefix; can include multiple\n"
		"            -t #\tSeconds to wait for a device to appear [what is default timeout?]\n"
		"            -T #\tSeconds to wait for a device to appear at stage switch [for continuous mode?]\n"
		"            -e KEY=VAL\tSet environment variable named KEY to value VAL\n"
		"            --pp #\tUSB polling period in milliseconds [what is default?]\n"
		"            --dm\tDisable small memory [what does that mean?]\n"
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
		"    expand-script PATH|BUILTIN [ARG...]\n"
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
		"        Install from a subdirectory that contains a uuu.auto script file.\n"
		"\n"
		"    uuu install foo.zip\n"
		"        Install from a zip file that contains a uuu.auto script file.\n"
		"\n"
		"    uuu install --script emmc_all foo.wic\n"
		"        Install by running built-in script emmc_all and passing foo.wic as a parameter.\n"
		"\n"
		"    uuu install SDPS: boot -f flash.bin\n"
		"        Execute SDPS protocol command.\n"
		"\n"
		"    uuu ls-devices\n"
		"        List connected devices.\n"
		"\n"
		"    uuu install --filter-path 1:2 --filter-path 1:3 foo.zip\n"
		"        Limit device discovery to USB paths 1:2 and 1:3. Use ls-devices to discover paths.\n"
		"\n"
		"    uuu install --interactive --script emmc bootloader\n"
		"        Load a bootloader image to the device EMMC and enter interactive mode.\n";

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

static void print_option_cli_help()
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
	for (auto& formal_param : script.formal_params)
	{
		std::string desc = id_highlight + formal_param.name + no_highlight;
		if (!formal_param.default_source_param_name.empty())
		{
			desc += " " +
				key_highlight + "[default=" +
				id_highlight + formal_param.default_source_param_name +
				key_highlight + "]" +
				no_highlight;
		}
		std::cout << "\t" << desc << ": " << formal_param.desc << std::endl;
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
			exit_for_syntax_error("Unknown script: " + script_name);
		}
		print_definition(item->second);
	}
}

static void print_protocol_help() {
	std::cout << command_help_text << std::endl;
}

static int _print_cfg(const char *pro, const char * chip, const char * /*compatible*/, uint16_t vid, uint16_t pid, uint16_t bcdmin, uint16_t bcdmax, void * /*p*/)
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
	uuu_for_each_cfg(_print_cfg, NULL);
}

static int _print_udev_rule(const char * /*pro*/, const char * /*chip*/, const char * /*compatible*/,
	uint16_t vid, uint16_t pid, uint16_t /*bcdmin*/, uint16_t /*bcdmax*/, void * /*p*/)
{
	printf("SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"%04x\", ATTRS{idProduct}==\"%04x\", TAG+=\"uaccess\"\n",
			vid, pid);
	return EXIT_SUCCESS;
}

static void print_udev_info()
{
	uuu_for_each_cfg(_print_udev_rule, NULL);

	std::cerr << std::endl
		<< "Enable udev rules via:" << std::endl
		<< "\tsudo sh -c \"uuu -udev > /etc/udev/rules.d/70-uuu.rules\"" << std::endl
		<< "\tsudo udevadm control --reload" << std::endl
		<< "Note: These instructions output to standard error so are excluded from redirected standard output" << std::endl;
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

/**
 * @brief Loads the content of a script
 * @param spec Name of a built-in script or a path to a file
 * @param values Parameter values to use with the script
 * @param[out] source Describes the source of the script
 */
static std::string load_script_text(
	const std::string& spec,
	const std::vector<std::string>& values,
	std::string& source)
{
	source = spec;
	const Script *script = g_ScriptCatalog.find(spec);
	if (!script) {
		source += " (custom)";
		script = g_ScriptCatalog.add_from_file(spec);
		if (!script)
		{
			exit_for_runtime_error("Specifies neither a built-in script nor a file: " + spec);
		}
	}
	else
	{
		source += " (built-in)";
	}

	if (values.size() > script->formal_params.size())
	{
		exit_for_runtime_error("Too many parameters for script: " + values[script->formal_params.size()]);
	}

	const std::string text = script->replace_parameters(values);

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
	std::string file_system_path;
	std::string protocol_cmd;
	std::string script_text;
	std::string script_source;

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
						exit_for_syntax_error("Options are either single dash with single letter or two dashes; got: " + arg);
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

					g_logger.log_info("Enabling verbose (append-based) output since interactive UX is append-based");
					g_transfer_context.enable_appending_feedback();
					g_logger.log_debug([&]() { return "Interactive mode enabled"; });
				}
				else if (opt == "v")
				{
					g_verbose = 1;
					uuu_set_debug_level(LIBUUU_DETAIL);
					g_logger.log_debug([&]() { return "Verbose enabled"; });
				}
				else if (opt == "V")
				{
					g_verbose = 1;
					uuu_set_debug_level(2);
					uuu_set_debug_level(LIBUUU_DETAIL | 2);
					g_logger.log_debug([&]() { return "Verbose+ enabled"; });
				}
				else if (opt == "dry-run")
				{
					dry_run = true;

					g_logger.log_info("Enabling verbose (append-based) output since dry-run UX is append-based");
					g_transfer_context.enable_appending_feedback();

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
					g_transfer_context.add_usb_path_filter(path);
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
					g_transfer_context.add_device_serial_filter(sn);
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
					uuu_set_bmap_mode(bmap_mode::Force);
					g_logger.log_debug([&]() { return "Set bmap to force"; });
				}
				else if (opt == "no-bmap")
				{
					uuu_set_bmap_mode(bmap_mode::Ignore);
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
					set_environment_variable(spec);
				}
				else if (opt == "s" || opt == "script")
				{
					if (args.empty())
					{
						exit_for_syntax_error("Missing path or built-in script name");
					}

					std::string script_spec = args.front();
					args.erase(args.begin());
					script_text = load_script_text(script_spec, args, script_source);
					g_logger.log_debug([&]()
						{
							std::string message(script_text);
							string_man::trim(message);
							string_man::replace(message, "\n", "\n\t");
							return "Content of script " + script_source + " with parameters replaced with values:\n\t" + message;
						});
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
				std::vector<std::string> cmd_args;
				cmd_args.push_back(arg);
				for (auto& cmd_arg : args)
				{
					string_man::quote_param_if_needed(cmd_arg);
					cmd_args.push_back(cmd_arg);
				}
				protocol_cmd = string_man::join(cmd_args, " ");
				g_logger.log_debug([&]() { return "Protocol command: '" + protocol_cmd + "'"; });
				args.clear();
				break;
			}
			else
			{
				file_system_path = arg;
				g_logger.log_debug([&]() { return "Input path: " + file_system_path; });
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

static void with_transfer_feedback(std::function<void(const TransferFeedback&)> callback)
{
	TransferFeedback transfer_feedback;
	transfer_feedback.enable();
	callback(transfer_feedback);
}

static void handle_install(const std::vector<std::string>& args)
{
	InstallConfig config(args);

	// [what is this needed for?]
	uuu_set_askpasswd(environment::ask_passwd);

	if (!config.protocol_cmd.empty())
	{
		with_transfer_feedback([&](const TransferFeedback& feedback) {
			// this can fail to parse the command or to run the command
			int ret = uuu_run_cmd(config.protocol_cmd.c_str(), dry_run);

			if (ret)
			{
				exit_for_runtime_error(uuu_get_last_err_string());
			}
		});

		g_logger.log_info("Command succeeded :)");

		if (config.enter_interactive_after_action)
		{
			process_interactive_commands();
		}

		exit_for_status(EXIT_SUCCESS);
	}
	else if (!config.script_text.empty())
	{
		// queue commands
		if (uuu_run_cmd_script(config.script_text.c_str(), dry_run))
		{
			exit_for_runtime_error(uuu_get_last_err_string());
		}

		// wait for queued commands to complete
		with_transfer_feedback([&](const TransferFeedback&) {
			if (uuu_wait_uuu_finish(is_continuous_mode, dry_run))
			{
				exit_for_runtime_error(uuu_get_last_err_string());
			}
		});
	}
	else if (!config.file_system_path.empty())
	{
		// queue commands
		if (uuu_auto_detect_file(config.file_system_path.c_str()))
		{
			exit_for_runtime_error(uuu_get_last_err_string());
		}

		// wait for queued commands to complete
		with_transfer_feedback([&](const TransferFeedback&) {
			if (uuu_wait_uuu_finish(is_continuous_mode, dry_run))
			{
				exit_for_runtime_error(uuu_get_last_err_string());
			}
		});
	}
	else if (!config.enter_interactive_after_action)
	{
		exit_for_syntax_error("No operation");
	}

	if (config.enter_interactive_after_action)
	{
		process_interactive_commands();
	}

	// wait for the thread exit, after send out CMD_DONE
	// [why is wait needed?]
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
	std::string text = script->text;
	string_man::trim(text);
	std::cout << text << std::endl;
	exit_for_status(EXIT_SUCCESS);
}

static void handle_expand_script(const std::vector<std::string>& args)
{
	if (args.size() < 1)
	{
		exit_for_syntax_error("Missing script specifier");
	}

	std::string spec = args[0];
	std::vector<std::string> params(args.begin() + 1, args.end());
	std::string source;
	std::string text = load_script_text(spec, params, source);
	string_man::trim(text);

	std::cout << text << std::endl;
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
	{ "expand-script", handle_expand_script },
#ifdef _WIN32
	{ "device-discovery-by-serial", handle_setup_serial_matching },
#endif
};

[[noreturn]]
static void process_cmd_command_line(int argc, char** argv)
{
	if (argc < 2)
	{
		exit_for_syntax_error("Missing command; options: " + string_man::join_keys(command_handlers));
	}
	
	const std::string command = argv[1];
	if (command == "-h")
	{
		print_cli_help();
		exit_for_status(EXIT_SUCCESS);
	}

	const auto& handler = std::find_if(command_handlers.begin(), command_handlers.end(), [&](const auto& item) {
		return std::get<0>(item) == command;
	});
	if (handler == command_handlers.end())
	{
		exit_for_syntax_error("Unknown command '" + command + "'; options: " + string_man::join_keys(command_handlers));
	}

	std::vector<std::string> args;
	for (int j = 2; j < argc; j++)
	{
		args.push_back(argv[j]);
	}
	auto& handle = std::get<1>(*handler);
	handle(args);

	// exit process with transfer status code
	// NOTE: should only get here if performed transfer (installed a file or ran a script)
	exit_for_status(g_transfer_context.get_overall_status_code());
}

[[noreturn]]
static void process_option_command_line(int argc, char** argv)
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
				const Script *script = g_ScriptCatalog.find(script_name);
				if (!script)
				{
					exit_for_syntax_error("Unknown built-in script '" + script_name + "'; options: " + g_ScriptCatalog.get_names());
				}
				std::string text = script->text;
				string_man::trim(text);
				std::cout << text << std::endl;
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

				g_logger.log_info("Enabling verbose (append-based) output since shell UX is append-based");
				g_transfer_context.enable_appending_feedback();
			}
			else if (opt == "v")
			{
				g_verbose = 1;
				uuu_set_debug_level(LIBUUU_DETAIL);
			}
			else if (opt == "V")
			{
				g_verbose = 1;
				uuu_set_debug_level(2);
				uuu_set_debug_level(LIBUUU_DETAIL | 2);
			}
			else if (opt == "dry")
			{
				dry_run = true;

				g_logger.log_info("Enabling verbose (append-based) output since dry-run UX is append-based");
				g_transfer_context.enable_appending_feedback();
			}
			else if (opt == "m")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing USB path argument");
				}
				uuu_add_usbpath_filter(argv[i]);
				g_transfer_context.add_usb_path_filter(argv[i]);
			}
			else if (opt == "ms")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing serial # argument");
				}
				uuu_add_usbserial_no_filter(argv[i]);
				g_transfer_context.add_device_serial_filter(argv[i]);
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
				uuu_set_bmap_mode(bmap_mode::Force);
			}
			else if (opt == "no-bmap")
			{
				uuu_set_bmap_mode(bmap_mode::Ignore);
			}
			else if (opt == "e")
			{
				if (++i >= argc)
				{
					exit_for_syntax_error("Missing key=value argument");
				}
				set_environment_variable(argv[i]);
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
			g_logger.log_debug([&]() { return "Protocol command: '" + protocol_cmd + "'"; });
			break;
		}
		else
		{
			if (argc - 1 > i)
			{
				exit_for_syntax_error("Too many arguments: " + std::string(argv[i + 1]) + "; Hint: use -b to pass parameters to a script");
			}
			input_path = arg;
			g_logger.log_debug([&]() { return "Input path: " + input_path; });
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

	// setup for transfer feedback
	if (g_verbose)
	{
		// commented out since can print script content via -cat-builtin; print here is noise
		//if (!cmd_script.empty())
		//	printf("\n%sRunning built-in script:%s\n %s\n\n", g_vt->boldwhite, g_vt->default_foreground, cmd_script.c_str());

		// why not log for !shell? it's logged for !g_verbose regardless
		if (!is_shell_mode) {
			//g_logger.log_info("Waiting for device..." + g_transfer_context.format_discovery_suffix());
		}
	}
	else {
		//std::cout << "Waiting for device..." << g_transfer_context.format_discovery_suffix();
		
		// move cursor to begining of line so that next output will overwrite waiting line;
		// NOTE: This is nice when it works since it erases the transitory state of waiting, 
		// but if the next line is shorter than the waiting line, then the portion of the waiting
		// line that is longer will remain after the next line is output. 
		// FWIW I think it's better to leave the waiting msg on screen to than allow a partial msg
		// to show and think that \r sould be replaed with \n
		//std::cout << "\r";
		
		// hide the cursor
		std::cout << g_vt->hide_cursor;
		
		// is this needed?
		std::cout.flush();
	}

	CursorRestorer cursor_restorer;
	uuu_set_askpasswd(environment::ask_passwd);
	TransferFeedback transfer_feedback;
	transfer_feedback.enable();

	if (is_shell_mode)
	{
		process_interactive_commands();
		exit_for_status(EXIT_SUCCESS);
	}
	else if (!protocol_cmd.empty())
	{
		int ret = uuu_run_cmd(protocol_cmd.c_str(), dry_run);

		if (ret)
		{
			exit_for_runtime_error(uuu_get_last_err_string());
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
		if (uuu_run_cmd_script(script_text.c_str(), dry_run))
		{
			exit_for_runtime_error(uuu_get_last_err_string());
		}
	}
	else if (!input_path.empty())
	{
		if (uuu_auto_detect_file(input_path.c_str()))
		{
			exit_for_runtime_error(uuu_get_last_err_string());
		}
	}
	else
	{
		exit_for_syntax_error("No operation");
	}

	if (uuu_wait_uuu_finish(is_continuous_mode, dry_run))
	{
		exit_for_runtime_error(uuu_get_last_err_string());
	}

	// wait for the thread exit, after send out CMD_DONE
	// [why is wait needed?]
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// exit process with transfer status code
	// NOTE: should only get here if performed transfer (installed a file or ran a script)
	exit_for_status(g_transfer_context.get_overall_status_code());
}
