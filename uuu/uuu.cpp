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
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <streambuf>
#include <thread>
#include <vector>

int auto_complete(int argc, char **argv);
void print_autocomplete_help();

using namespace std;

int g_verbose = 0;
bmap_mode g_bmap_mode = bmap_mode::Default;
std::shared_ptr<VtEmulation> g_vt = std::make_shared<PlatformVtEmulation>();
TransferContext g_transfer_context;
Logger g_logger;

static TransferFeedback transfer_feedback;
static char sample_cmd_list[] = {
#include "uuu.clst"
};

/**
 * @brief Exits after outputting feedback about interrupt
 */
static void interrupt(int)
{
	// not sure what this does, but maybe it ensures that output color is normal
	{
		AutoCursor a;
	}

	// move cursor below status output area
	printf("\n\n\n");

	printf("INTERRUPTED\n");

	exit(1);
}

static void print_app_title()
{
	printf("Universal Update Utility for NXP i.MX chips -- %s\n", uuu_get_version_string());
}

static void print_cli_help()
{
	string text =
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
		"    BUILTIN\tBuilt-in script: [BUILTIN_NAMES]\n"
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

	cout << endl << string_man::replace(text, "[BUILTIN_NAMES]", g_ScriptCatalog.get_names());
}

static void print_syntax_error(const string& message) {
	g_logger.log_error(message);
	g_logger.log_info("Hint: see help output from 'uuu -h'");
}

/**
 * @brief Writes the name, description and arguments to stdout
 */
static void print_definition(const Script& script)
{
	std::string id_highlight = g_vt->green;
	std::string key_highlight = g_vt->kcyn;
	std::string no_highlight = g_vt->default_fg;
	std::cout << id_highlight << script.name << g_vt->default_fg << ": " << script.desc << std::endl;
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
		std::cout << std::endl << "Built-in scripts:" << std::endl;
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
			exit(EXIT_FAILURE);
		}
		print_definition(item->second);
	}
}

static void print_protocol_help() {
	size_t start = 0, pos = 0;
	string str= sample_cmd_list;

	bool bprint = false;
	while ((pos = str.find('\n',pos)) != str.npos)
	{
		string s = str.substr(start, pos - start);
		if (s.substr(0, 6) == "# ----")
			bprint = true;

		if (bprint)
		{
			if (s[0] == '#')
			{
				printf("%s\n", &(s[1]));
			}
		}
		pos += 1;
		start = pos;
	}
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

	cerr << endl <<
		"Enable udev rules via:" << endl <<
		"\tsudo sh -c \"uuu -udev >> /etc/udev/rules.d/70-uuu.rules\"" << endl <<
		"\tsudo udevadm control --reload" << endl <<
		"Note: These instructions output to standard error so are excluded from redirected standard output" << endl << endl;
}

static void print_usb_filter()
{
	if (!g_transfer_context.usb_path_filter.empty())
	{
		cout << " at path ";
		for (size_t i = 0; i < g_transfer_context.usb_path_filter.size(); i++)
			cout << g_transfer_context.usb_path_filter[i] << " ";
	}
}

static void proces_interactive_commands()
{
	int uboot_cmd = 0;
	string prompt = "U>";

	cout << "Please input command: " << endl;
	string cmd;
	ofstream log("uuu.inputlog", ofstream::binary);
	log << "uuu_version "
		<< ((uuu_get_version() & 0xFF000000) >> 24)
		<< "."
		<< ((uuu_get_version() & 0xFFF000) >> 12)
		<< "."
		<< ((uuu_get_version() & 0xFFF))
		<< endl;
	while (1)
	{
		cout << prompt;
		getline(cin, cmd);

		if (cmd == "uboot")
		{
			uboot_cmd = 1;
			prompt = "=>";
			cout << "Enter into u-boot cmd mode" << endl;
			cout << "Okay" << endl;
		}
		else if (cmd == "exit" && uboot_cmd == 1)
		{
			uboot_cmd = 0;
			prompt = "U>";
			cout << "Exit u-boot cmd mode" << endl;
			cout << "Okay" << endl;
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
			log << cmd << endl;
			log.flush();

			if (uboot_cmd)
				cmd = "fb: ucmd " + cmd;

			int ret = uuu_run_cmd(cmd.c_str(), 0);
			if (ret)
				cout << uuu_get_last_err_string() << endl;
			else
				cout << "Okay" << endl;
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
	printf("\t%s\t %s\t %s\t 0x%04X\t0x%04X\t 0x%04X\t %s\n", path, chip, pro, vid, pid, bcd, serial_no);
	return EXIT_SUCCESS;
}

static void print_device_list()
{
	cout << "Connected devices\n";
	printf("\tPath\t Chip\t Pro\t Vid\t Pid\t BcdVersion\t Serial_no\n");
	printf("\t====================================================================\n");

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

static std::string load_script_text(const std::string& script_spec, const vector<string>& args, std::string& script_name_feedback)
{
	script_name_feedback = script_spec;
	const Script* script = g_ScriptCatalog.find(script_spec);
	if (!script) {
		script_name_feedback += " (custom)";
		script = g_ScriptCatalog.add_from_file(script_spec);
		if (!script)
		{
			g_logger.log_error("Unable to load script from file: " + script_spec);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		script_name_feedback += " (built-in)";
	}

	if (args.size() > script->args.size())
	{
		g_logger.log_error("Too many parameters for script: " + args[script->args.size()]);
		exit(EXIT_FAILURE);
	}

	const std::string text = script->replace_arguments(args);

	if (g_verbose)
	{
		std::string message(text);
		string_man::trim(message);
		string_man::replace(message, "\n", "\n\t");
		message = "Script with parameters replaced with values:\n\t" + message;
		g_logger.log_verbose(message);
	}

	return text;
}

int main(int argc, char **argv)
{
	// commented out since causes failure when pass script (that ends with 'uuu'?) file name/path as first arg plus -v after
	//if (auto_complete(argc, argv) == 0) return EXIT_SUCCESS;

	std::unique_ptr<AutoCursor> auto_cursor;
	if (g_vt->enable())
	{
		g_logger.is_color_output_enabled = true;
		auto_cursor = std::make_unique<AutoCursor>();
	}
	else
	{
		// [why enable verbose in this case?]
		cout << "Warning: Console doesn't support VT mode; enabling verbose feedback" << endl;
		g_verbose = 1;
	}

	// handle modes that should _not_ print the app title
	if (argc >= 2)
	{
		string arg = argv[1];
		if(arg == "-udev")
		{
			print_udev_info();
			return EXIT_SUCCESS;
		}
		if (arg == "-cat-builtin")
		{
			if (2 == argc)
			{
				print_syntax_error("Missing built-in script name; options: " + g_ScriptCatalog.get_names());
				return EXIT_FAILURE;
			}
			string script_name = argv[2];
			const Script *script = g_ScriptCatalog.find(script_name);
			if (!script)
			{
				print_syntax_error("Unknown built-in script '" + script_name + "'; options: " + g_ScriptCatalog.get_names());
				return EXIT_FAILURE;
			}
			print_content(*script);
			return EXIT_SUCCESS;
		}
	}

	print_app_title();

	if (argc == 1)
	{
		print_cli_help();
		return EXIT_FAILURE;
	}

	int deamon = 0;
	int shell = 0;
	int dryrun = 0;
	string input_path;
	string protocol_cmd;
	string script_name_feedback;
	string script_text;

	for (int i = 1; i < argc; i++)
	{
		string arg = argv[i];
		if (!arg.empty() && arg[0] == '-')
		{
			if (arg == "-d")
			{
				deamon = 1;
				uuu_set_small_mem(0);
			}
			else if (arg == "-dm")
			{
				uuu_set_small_mem(0);
			}
			else if (arg == "-s")
			{
				shell = 1;
				// why set verbose for shell mode?
				g_verbose = 1;
			}
			else if (arg == "-v")
			{
				g_verbose = 1;
			}
			else if (arg == "-V")
			{
				g_verbose = 1;
				uuu_set_debug_level(2);
			}else if (arg == "-dry")
			{
				dryrun = 1;
				// why is verbose set for dry-run? 
				g_verbose = 1;
			}
			else if (arg == "-h")
			{
				print_cli_help();
				return EXIT_SUCCESS;
			}
			else if (arg == "-h-auto-complete")
			{
				print_autocomplete_help();
				return EXIT_SUCCESS;
			}
			else if (arg == "-h-protocol-commands")
			{
				print_protocol_help();
				return EXIT_SUCCESS;
			}
			else if (arg == "-h-protocol-support")
			{
				print_protocol_support_info();
				return EXIT_SUCCESS;
			}
			else if (arg == "-ls-builtin")
			{
				std::string script_name;
				if (++i < argc)
				{
					script_name = argv[i];
				}
				print_script_catalog(script_name);
				return EXIT_SUCCESS;
			}
			else if (arg == "-m")
			{
				if (++i >= argc)
				{
					print_syntax_error("Missing USB path argument");
					return EXIT_FAILURE;
				}
				uuu_add_usbpath_filter(argv[i]);
				g_transfer_context.usb_path_filter.push_back(argv[i]);
			}
			else if (arg == "-ms")
			{
				if (++i >= argc)
				{
					print_syntax_error("Missing serial # argument");
					return EXIT_FAILURE;
				}
				uuu_add_usbserial_no_filter(argv[i]);
				g_transfer_context.usb_serial_no_filter.push_back(argv[i]);
			}
			else if (arg == "-t")
			{
				if (++i >= argc)
				{
					print_syntax_error("Missing seconds argument");
					return EXIT_FAILURE;
				}
				uuu_set_wait_timeout(atol(argv[i]));
			}
			else if (arg == "-T")
			{
				if (++i >= argc)
				{
					print_syntax_error("Missing seconds argument");
					return EXIT_FAILURE;
				}
				uuu_set_wait_next_timeout(atol(argv[i]));
			}
			else if (arg == "-pp")
			{
				if (++i >= argc)
				{
					print_syntax_error("Missing milliseconds argument");
					return EXIT_FAILURE;
				}
				uuu_set_poll_period(atol(argv[i]));
			}
			else if (arg == "-ls-devices")
			{
				print_device_list();
				return EXIT_SUCCESS;
			}
			#ifdef _WIN32
			else if (arg == "-IgSerNum")
			{
				return environment::set_ignore_serial_number();
			}
			#endif
			else if (arg == "-bmap")
			{
				g_bmap_mode = bmap_mode::Force;
			}
			else if (arg == "-no-bmap")
			{
				g_bmap_mode = bmap_mode::Ignore;
			}
			else if (arg == "-e")
			{
				if (++i >= argc)
				{
					print_syntax_error("Missing key=value argument");
					return EXIT_FAILURE;
				}
				string spec = argv[i];
				size_t equal_pos = spec.find("=");
				if (equal_pos == std::string::npos)
				{
					g_logger.log_error("Invalid input '" + spec + "'; must be formatted as: key=value");
					return EXIT_FAILURE;
				}
				std::string name = spec.substr(0, equal_pos);
				std::string value = spec.substr(equal_pos + 1);
				if (environment::set_environment_variable(name, value))
				{
					g_logger.log_error("Failed to set environment variable with expression '" + spec + "'");
					return EXIT_FAILURE;
				}
				g_logger.log_verbose("Set environment variable '" + name + "' to '" + value + "'");
			}
			else if (arg == "-b" || arg == "-brun")
			{
				if (++i >= argc)
				{
					print_syntax_error("Missing path or built-in script name");
					return EXIT_FAILURE;
				}

				vector<string> args;
				for (int j = i + 1; j < argc; j++)
				{
					args.push_back(argv[j]);
				}

				script_text = load_script_text(argv[i], args, script_name_feedback);
				break;
			}
			else
			{
				print_syntax_error("Unknown option: " + arg);
				return EXIT_FAILURE;
			}
		}
		else if (!arg.empty() && arg[arg.size() - 1] == ':')
		{
			// treat as a protocol command

			for (int j = i; j < argc; j++)
			{
				arg = argv[j];
				if (arg.find(' ') != string::npos && arg[arg.size() - 1] != ':')
				{
					arg.insert(arg.begin(), '"');
					arg.insert(arg.end(), '"');
				}
				protocol_cmd.append(arg);
				if (j != (argc -1)) // don't add space after last arg
					protocol_cmd.append(" ");
			}
			break;
		}
		else
		{
			// treat as a file system path

			if (argc - 1 > i)
			{
				print_syntax_error("Too many arguments - " + string(argv[i + 1]) + "; Hint: use -b to pass parameters to a script");
				return EXIT_FAILURE;
			}
			input_path = arg;
		}
	}

	if (deamon && shell)
	{
		g_logger.log_error("Incompatible options: deamon (-d) and shell (-s)");
		return EXIT_FAILURE;
	}

	if (deamon && dryrun)
	{
		g_logger.log_error("Incompatible options: deamon (-d) and dry-run (-dry)");
		return EXIT_FAILURE;
	}

	if (shell && dryrun)
	{
		g_logger.log_error("Incompatible options: shell (-s) and dry-run (-dry)");
		return EXIT_FAILURE;
	}

	if (g_verbose)
	{
		// commented out since can print script content via -cat-builtin; print here is noise
		//if (!cmd_script.empty())
		//	printf("\n%sRunning built-in script:%s\n %s\n\n", g_vt->boldwhite, g_vt->default_foreground, cmd_script.c_str());

		// why not log for !shell? it's logged for !g_verbose regardless
		if (!shell) {
			cout << "Waiting for device";
			print_usb_filter();
			cout << "...";
			printf("\n");
		}
	}
	else {
		cout << "Waiting for device";
		print_usb_filter();
		cout << "...";
		cout << "\r"; // why is this needed?
		cout << "\x1b[?25l"; // what does this do?
		cout.flush();
	}

	signal(SIGINT, interrupt);

	uuu_set_askpasswd(environment::ask_passwd);
	transfer_feedback.enable();

	if (shell)
	{
		proces_interactive_commands();
		return EXIT_SUCCESS;
	}
	else if (!protocol_cmd.empty())
	{
		g_logger.log_verbose("Executing single command: " + protocol_cmd);
		int ret = uuu_run_cmd(protocol_cmd.c_str(), dryrun);

		// what is the purpose of printing blank lines? Don't know about success, but on error, there are several blank lines on screen
		for (size_t i = 0; i < g_transfer_context.map_path_nt.size()+3; i++)
			printf("\n");

		if (ret)
		{
			g_logger.log_error(uuu_get_last_err_string());
			return EXIT_FAILURE;
		}

		g_logger.log_info("Command succeeded :)");

		if (shell) proces_interactive_commands();

		return EXIT_SUCCESS;
	}
	else if (!script_text.empty())
	{
		if (script_name_feedback.empty())
		{
			g_logger.log_internal_error("Expected non-empty script_spec");
			return EXIT_FAILURE;
		}
		g_logger.log_verbose("Running script: " + script_name_feedback);
		if (uuu_run_cmd_script(script_text.c_str(), dryrun))
		{
			g_logger.log_error(uuu_get_last_err_string());
			return EXIT_FAILURE;
		}
	}
	else if (!input_path.empty())
	{
		g_logger.log_verbose("Running as auto detect file: " + input_path);
		if (uuu_auto_detect_file(input_path.c_str()))
		{
			g_logger.log_error(uuu_get_last_err_string());
			return EXIT_FAILURE;
		}
	}
	else
	{
		g_logger.log_error("No operation");
		return EXIT_FAILURE;
	}

	if (uuu_wait_uuu_finish(deamon, dryrun))
	{
		g_logger.log_error(uuu_get_last_err_string());
		return EXIT_FAILURE;
	}

	// wait for the thread exit, after send out CMD_DONE
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// move cursor below status area; [why 3?]
	if(!g_verbose) printf("\n\n\n");

	g_logger.log_info(g_transfer_context.overall_status == 0 ? "Success :)" : "Failed :(");

	return g_transfer_context.overall_status; // [why return this value??]
}
