#pragma once
//! @file

#include "../libuuu/libuuu.h"
#include "../libuuu/string_man.h"

#include <stdio.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

class HorizontalScrollingFormatter final
{
	static const clock_t end_delay = CLOCKS_PER_SEC * 3;
	static const clock_t scroll_delay = CLOCKS_PER_SEC / 2;
	unsigned scroll_pos = 0;
	clock_t start_ticks = 0;
	std::string last_message;

public:
	/**
	 * @brief Clears the horizontal scroll offset
	 * @detail
	 * Generally, called when message changes.
	 */
	void restart()
	{
		last_message = "";
		scroll_pos = 0;
	}

	/*
	 * @brief Formats text that renders as fixed-width based on a message with support for horizontal scrolling
	 * @param message Message text
	 * @param width Length of output text
	 * @param scroll_index Increments to support scrolling
	 * @return Formatted text
	 * @details
	 * If the message is too long to fit, then the middle of the message is extracted based on
	 * scroll_index which should start at 0 for a new message, then increment periodically while
	 * the same message applies.
	 */
	std::string format(std::string message, unsigned width)
	{
		if (message != last_message)
		{
			scroll_pos = 0;
			last_message = message;
		}

		// if message fits, return with padding to len
		if (message.size() <= width)
		{
			message.resize(width, ' ');
			return message;
		}

		// extract substring
		std::string text = message.substr(scroll_pos, width);
		text.resize(width, ' ');

		// update scroll position
		// long delay when showing first or last
		{
			const bool showing_first = scroll_pos == 0;
			const bool showing_last = scroll_pos == message.size() - width;
			const clock_t elapsed_ticks = clock() - start_ticks;
			const clock_t delay_ticks = showing_first || showing_last ? end_delay: scroll_delay;
			if (elapsed_ticks > delay_ticks)
			{
				scroll_pos += 1;
				if (showing_last) scroll_pos = 0;
				start_ticks = clock();
			}
		}

		return text;
	}
};

extern int g_verbose;

class TransferNotifyItem;

typedef size_t transfer_count_t;

class TransferContext final
{
	unsigned success_operation_count = 0;
	unsigned failed_operation_count = 0;
	std::vector<std::string> device_serial_filters;
	std::vector<std::string> usb_path_filters;
	bool start_usb_transfer = false;

public:
	// This should be in TransferNotifyItem, but due how items are cached, this needs global lifetime --
	// which TransferContext's singleton provides
	HorizontalScrollingFormatter cmd_field_formatter;

	/**
	 * @brief Enable verbose feedback to select append-based feedback; not overwritting
	 */
	void enable_appending_feedback() {
		g_verbose = 1;
	}

	void add_device_serial_filter(const std::string& item)
	{
		device_serial_filters.push_back(item);
	}

	void add_usb_path_filter(const std::string& item)
	{
		usb_path_filters.push_back(item);
	}

	unsigned get_success_operation_count() const { return success_operation_count; }

	unsigned get_failed_operation_count() const { return failed_operation_count; }
	
	bool get_start_usb_transfer() const { return start_usb_transfer; }
	
	void set_start_usb_transfer(bool to) { start_usb_transfer = to; }

	void record_operation_complete(int status_code)
	{
		if (status_code)
		{
			failed_operation_count++;
		}
		else
		{
			success_operation_count++;
		}
	}

	int get_overall_status_code() const
	{
		return failed_operation_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	/**
	 * @brief Overwrites the current console line with text -- padded/trimmed to fill the line
	 * @note
	 * Assumption: On entry, the cursor is at the first column. If not, result will probably be undesirable.
	 * Assumption: Text does not contain escape code.
	 * @note
	 * Does nothing if the console width is 0.
	 * @note
	 * Behavior if stdout is redirected to a file is somewhat unknown.
	 * For Windows, the result will probably be that the file contains lines with length of the
	 * console width (even though output is to a file).
	 * For Linux, result is unknown.
	 */
	void fill_console_line(std::string text) const
	{
		const size_t console_width = g_vt->get_console_width();
		if (console_width < 1) return;

		if (text.size() >= console_width)
		{
			// truncate to fit line
			text.resize(console_width - 1);
			if (console_width > 3)
			{
				text[text.size() - 1] = '.';
				text[text.size() - 2] = '.';
				text[text.size() - 3] = '.';
			}
		}
		else
		{
			// pad to fill line
			text.resize(console_width, ' ');
		}

		// move cursor to beginning of next line
		std::cout << text << std::endl;
	}

	std::string format_discovery_filter_desc() const
	{
		std::vector<std::string> filters;
		if (!usb_path_filters.empty())
		{
			filters.push_back("at path: " + string_man::join(usb_path_filters, "|"));
		}
		if (!device_serial_filters.empty())
		{
			filters.push_back("with serial# prefix: " + string_man::join(device_serial_filters, "|"));
		}
		if (filters.empty()) return "";
		return string_man::join(filters, "; ");
	}
};

extern TransferContext g_transfer_context;

class TransferNotifyItem final
{
	std::string cmd_desc;
	std::string device_desc;
	std::string last_err_msg;
	int last_status_code = EXIT_SUCCESS;
	bool is_done = false;
	bool is_empty_line = false;
	unsigned cmd_count = 0;
	unsigned cmd_index = 0;
	uint64_t cmd_start_timestamp = 0;
	uint64_t cmd_end_timestamp = 0;
	transfer_count_t trans_pos = 0;
	transfer_count_t trans_size = 0;
	static constexpr const char* busy_chars = "|/-\\";
	int busy_chars_index = 0;

	/**
	 * @brief Returns the sentinal value that indicates the device is not known
	 * @note
	 * Couldn't figure out how to declare this as a static-ish and const-ish variable
	 */
	static std::string get_unknown_device_desc() { return "Prep"; }

	/*
	 * @brief Formats text that renders as fixed-width that describes the device and the commands completed progress
	 * @return Formatted text
	 */
	std::string format_status_prefix() const
	{
		std::string device_desc_field = device_desc;
		device_desc_field.resize(12, ' ');

		std::string cmd_count_progress;
		string_man::format(cmd_count_progress, "%2d/%2d", cmd_index + 1, cmd_count);

		return device_desc_field + cmd_count_progress;
	}

	/*
	 * @brief Formats a representation of progress as text that renders as fixed-width
	 * @param[in,out] text Inbound as a number of chars (background/spaces) for the expected render width; outbound as formatted text
	 * @param pos Number of total items completed
	 * @param total Total number of items to complete
	 */
	static void format_percent_complete_bar(std::string& text, transfer_count_t pos, transfer_count_t total)
	{
		const size_t width = text.size();

		// deal with 0 total
		// [what causes total==0? why use 1 mibi? what is 'M'?]
		if (total == 0)
		{
			if (pos == 0) return;

			size_t size = pos / (1024 * 1024);
			std::string loc;
			string_man::format(loc, "%dM", size);
			text.replace(0, loc.size(), loc);
			return;
		}

		// conform pos to total; shouldn't happen, but just in case
		if (pos > total) pos = total;

		// overwrite background with '=' to indicate percent complete
		{
			size_t progress_width = width * pos / total;
			for (size_t i = 0; i < progress_width; i++)
			{
				text[i] = '=';
			}

			//if (progress_width > 0 && pos < total)
			//{
			//	text[progress_width-1] = '>';
			//}
		}

		// write percent in the middle; overwriting progress chars if it reaches the middle
		{
			size_t percent = pos * 100 / total;
			std::string percent_text;
			string_man::format(percent_text, "%d%%", percent);
			size_t start = (width - percent_text.size()) / 2;
			text.replace(start, percent_text.size(), g_vt->fg_light_yellow + percent_text + g_vt->fg_default);
		}
	}
	
	/**
	 * @brief Formats the device context prefix
	 * @details
	 * Starts with '\r' to force output to start at first column since sometimes the cursor is not
	 * at the start of the line as it was being used repeatedly (i.e. for percent complete).
	 */
	std::string format_device_context() const
	{
		return "\r" + device_desc + "> ";
	}

public:
	/**
	 * @brief Indicates whether the device is unknown based on the value of device_desc
	 */
	bool is_unknown_device() const { return device_desc == get_unknown_device_desc(); }

	/**
	 * @brief Sets device_desc to indicate that the device is unknown
	 */
	void set_unknown_device() { device_desc = get_unknown_device_desc(); }

	/**
	 * @brief Enables empty line mode
	 */
	void enable_empty_line() { is_empty_line = true; }

	std::string get_device_desc() const { return device_desc; }

	std::string get_cmd_desc() const { return cmd_desc; }

	void set_cmd_desc(const std::string& to)
	{
		if (cmd_desc != to)
		{
			cmd_desc = to;
			g_transfer_context.cmd_field_formatter.restart();
		}
	}
	
	unsigned get_cmd_index() const { return cmd_index; }
	
	transfer_count_t get_trans_pos() const { return trans_pos; }
	
	void increment_trans_pos(transfer_count_t by) { trans_pos += by; }
	
	transfer_count_t get_trans_size() const { return trans_size; }
	
	void increment_trans_size(transfer_count_t by) { trans_size += by; }

	/**
	 * @brief Caches transfer notify info
	 * @return Whether state changed that affects rendering; IOW whether caller should print
	 */
	bool cache(const uuu_notify& notify)
	{
		if (notify.type == uuu_notify::NOTIFY_DEV_ATTACH)
		{
			is_done = false;
			last_err_msg = "";
			last_status_code = EXIT_SUCCESS;
			device_desc = notify.str;
			set_cmd_desc("");
		}
		else if (notify.type == uuu_notify::NOTIFY_CMD_START)
		{
			set_cmd_desc(notify.str);
			cmd_start_timestamp = notify.timestamp;
		}
		else if (notify.type == uuu_notify::NOTIFY_DECOMPRESS_START)
		{
			set_cmd_desc(notify.str);
			cmd_start_timestamp = notify.timestamp;
			set_unknown_device();
		}
		else if (notify.type == uuu_notify::NOTIFY_DOWNLOAD_START)
		{
			set_cmd_desc(notify.str);
			cmd_start_timestamp = notify.timestamp;
			set_unknown_device();
		}
		else if (notify.type == uuu_notify::NOTIFY_DOWNLOAD_END)
		{
			is_empty_line = true;
		}
		else if (notify.type == uuu_notify::NOTIFY_TRANS_SIZE || notify.type == uuu_notify::NOTIFY_DECOMPRESS_SIZE)
		{
			trans_size = notify.total;
			return false;
		}
		else if (notify.type == uuu_notify::NOTIFY_CMD_TOTAL)
		{
			cmd_count = (unsigned)notify.total;
			return false;
		}
		else if (notify.type == uuu_notify::NOTIFY_CMD_INDEX)
		{
			cmd_index = (unsigned)notify.index;
			return false;
		}
		else if (notify.type == uuu_notify::NOTIFY_TRANS_POS || notify.type == uuu_notify::NOTIFY_DECOMPRESS_POS)
		{
			if (trans_size == 0)
			{
				trans_pos = notify.index;
				return true;
			}

			if ((notify.index - trans_pos) < (trans_size / 100) && notify.index != trans_size)
			{
				return false;
			}

			trans_pos = notify.index;
		}
		else if (notify.type == uuu_notify::NOTIFY_CMD_END)
		{
			cmd_end_timestamp = notify.timestamp;
			last_status_code = notify.status;
			last_err_msg = uuu_get_last_err_string();
			if (notify.status)
			{
				// seems that for failure, NOTIFY_DONE is not fired so need to record failure here
				g_transfer_context.record_operation_complete(last_status_code);
			}
		}
		else if (notify.type == uuu_notify::NOTIFY_DONE)
		{
			g_transfer_context.record_operation_complete(last_status_code);
			is_done = true;
		}

		return true;
	}

	/**
	 * @brief Appends status to stdout
	 */
	void print_to_append(const uuu_notify& notify)
	{
		if (is_unknown_device() && g_transfer_context.get_start_usb_transfer())
			return;

		if (notify.type == uuu_notify::NOTIFY_DEV_ATTACH)
		{
			std::cout << format_device_context() <<
				g_vt->fg_info()
				<< "Device detected"
				<< g_vt->fg_default
				<< std::endl;
		}
		else if (notify.type == uuu_notify::NOTIFY_CMD_START)
		{
			std::cout << format_device_context()
				<< g_vt->fg_info()
				<< "Start: " << notify.str
				<< g_vt->fg_default
				<< std::endl;
		}
		else if (notify.type == uuu_notify::NOTIFY_CMD_END)
		{
			double diff = ((double)(cmd_end_timestamp - cmd_start_timestamp)) / 1000;
			std::cout << format_device_context();
			if (notify.status)
			{
				std::cout << g_vt->fg_error() << "Failed: " << uuu_get_last_err_string();
			}
			else
			{
				std::cout << g_vt->fg_ok() << "OK";
			}
			std::cout << " (" << std::setprecision(4) << diff << "s)" << g_vt->fg_default << std::endl;
		}
		else if (notify.type == uuu_notify::NOTIFY_TRANS_POS || notify.type == uuu_notify::NOTIFY_DECOMPRESS_POS)
		{
			std::cout << "\r ";
			if (trans_size)
			{
				auto percent = trans_pos * 100 / trans_size;
				std::cout << g_vt->fg_light_yellow << percent << "% " << g_vt->fg_default;
			}
			else
			{
				std::cout << trans_pos;
			}
			std::cout.flush();
		}
		else if (notify.type == uuu_notify::NOTIFY_CMD_INFO)
		{
			if (notify.str[0])
			{
				std::cout << format_device_context()
					<< g_vt->fg_info()
					<< "Info: " << notify.str
					<< g_vt->fg_default
					<< std::endl;
			}
		}
		else if (notify.type == uuu_notify::NOTIFY_WAIT_FOR)
		{
			std::cout << "\r" << notify.str << " " << busy_chars[((busy_chars_index++) & 0x3)];
		}
		else if (notify.type == uuu_notify::NOTIFY_DECOMPRESS_START)
		{
			std::cout << "Decompress file:" << notify.str << std::endl;
		}
		else if (notify.type == uuu_notify::NOTIFY_DOWNLOAD_START)
		{
			std::cout << "Download file:" << notify.str << std::endl;
		}
	}

	/**
	 * @brief Overwrites the current console line with status
	 */
	void print_to_overwrite_console_line()
	{
		static const unsigned prefix_width = 18;
		static const unsigned bar_width = 40;
		static const unsigned bar_content_width = bar_width - 2;
		const unsigned console_width = g_vt->get_console_width();

		if (is_empty_line)
		{
			std::string line(console_width, ' ');
			std::cout << line;
			// why not simplify to: g_transfer_context.fill_console_line("");
		}
		else if (console_width <= bar_width + prefix_width + 3)
		{
			// output prefix and busy char; no room for progress bar

			std::string line = format_status_prefix() + busy_chars[(busy_chars_index++) & 0x3];
			g_transfer_context.fill_console_line(line);
		}
		else
		{
			// output prefix, progress bar and command info

			// output prefix
			{
				std::string prefix = format_status_prefix();
				prefix.resize(prefix_width, ' ');
				std::cout << prefix;
			}

			// output progress bar which shows % complete, done or error
			{
				std::string text;
				text.resize(bar_content_width, ' ');
				if (last_status_code)
				{
					std::string message = uuu_get_last_err_string();
					// truncate if too long; padding is not needed (since buffer already padded) but not harmful
					message.resize(bar_content_width, ' ');
					text.replace(0, message.size(), g_vt->fg_error() + message + g_vt->fg_default);
				}
				else if (is_done)
				{
					std::string message = "Done";
					text.replace(0, message.size(), g_vt->fg_ok() + message + g_vt->fg_default);
				}
				else
				{
					format_percent_complete_bar(text, trans_pos, trans_size);
				}
				std::cout << '[' << text << ']';
			}

			// show command description with horizontal scrolling
			{
				std::cout << " ";
				const unsigned cmd_width = console_width - bar_width - prefix_width - 1;
				std::cout << g_transfer_context.cmd_field_formatter.format(cmd_desc, cmd_width);
			}

			// move cursor to start of next line
			// [not exactly sure why do this, but if don't output is messed up!]
			std::cout << std::endl;
		}
	}
};

/**
 * @brief Manages user feedback related to data transfer operations
 */
class TransferFeedback final
{
	std::map<uint64_t, TransferNotifyItem> notify_items_by_id;
	std::map<std::string, TransferNotifyItem> notify_items_by_name;
	std::mutex update_mutex;

	/**
	 * @brief Returns an instance that summarizes trans size/pos
	 */
	TransferNotifyItem get_summary() const
	{
		TransferNotifyItem item;
		for (auto& i : notify_items_by_id)
		{
			if (i.second.is_unknown_device())
			{
				item.increment_trans_size(i.second.get_trans_size());
				item.increment_trans_pos(i.second.get_trans_pos());
			}
			else
			{
				if (i.second.get_trans_pos() || i.second.get_cmd_index())
				{
					// Hidden HTTP download when USB start transfer
					g_transfer_context.set_start_usb_transfer(true);
				}
			}
		}

		if (g_transfer_context.get_start_usb_transfer())
		{
			// Hidden HTTP download when USB start transfer
			item.enable_empty_line();
		}

		item.set_unknown_device();
		item.set_cmd_desc("Http Download\\Uncompress");
		return item;
	}

	std::string format_overall_status_line() const
	{
		std::string text;
		string_man::format(text, "\rSuccess %d    Failure %d    ", g_transfer_context.get_success_operation_count(), g_transfer_context.get_failed_operation_count());

		if (notify_items_by_name.empty())
		{
			// does this ever show? it doesn't for me; maybe it does for continuous mode
			text += "Waiting for device...";
		}

		text += g_transfer_context.format_discovery_filter_desc();

		return text;
	}

	/**
	 * @brief Prints the status of the selected item or the summary info if the selected item lacks device info
	 * @details
	 * [Why is the summary info used here?]
	 */
	void print_to_append(const TransferNotifyItem& selected_item, const uuu_notify& notify) const
	{
		TransferNotifyItem item = selected_item.is_unknown_device() ? get_summary() : selected_item;
		item.print_to_append(notify);
	}

	/**
	 * @brief Renders the status area
	 * @details
	 * On first call, draws the status area on new lines.
	 * On subsequent calls, this assumes the curser is at the end of the status display,
	 * moves the curser up a number of lines equal to the height of the last render.
	 * Other output while using this function will probably mess up the status area.
	 */
	void render_status_area(const TransferNotifyItem& selected_item)
	{
		static unsigned lines_below_top_of_status_area = 0;

		// According to https://en.wikipedia.org/wiki/ANSI_escape_code, '[#F' moves the cursor
		// to the beginning of the line that is # lines above the current position and # can be
		// omitted to specify one line.
		static const char* cursor_previous_line = "\x1B[1F";

		// move cursor up to first line of status area based on how many lines this function wrote last time
		for (unsigned i = 0; i < lines_below_top_of_status_area; i++)
		{
			std::cout << cursor_previous_line;
		}
		lines_below_top_of_status_area = 0;

		// output overall status line
		g_transfer_context.fill_console_line(format_overall_status_line());
		lines_below_top_of_status_area += 1;

		// output blank line; [why?]
		//g_transfer_context.fill_console_line("");
		//lines_below_top_of_status_area += 1;

		// write summary for unknown device or blank line
		// [what is the deal with this line? why so special? for unknown and !start-usb-transfer?]
		if (selected_item.is_unknown_device() && !g_transfer_context.get_start_usb_transfer())
		{
			get_summary().print_to_overwrite_console_line();
		}
		else
		{
			g_transfer_context.fill_console_line("");
		}
		lines_below_top_of_status_area += 1;

		// write line for each notify item
		// [How can there be more than one item since they are keyed by device ID?
		// Maybe only happens in continuous mode when install to multiple devices in a single session.]
		for (auto& item : notify_items_by_name)
		{
			item.second.print_to_overwrite_console_line();
			lines_below_top_of_status_area += 1;
		}
	}

	/**
	 * @brief Updates feedback for a notify event
	 */
	void update(const uuu_notify& notify)
	{
		std::lock_guard<std::mutex> lock(update_mutex);

		// NOTE: creates a new instance if match not found
		auto& selected_item = notify_items_by_id[notify.id];

		if (selected_item.cache(notify))
		{
			if (!selected_item.get_device_desc().empty())
			{
				if (!selected_item.is_unknown_device())
				{
					const std::string name = selected_item.get_device_desc();

					// this copies an item from notify_items_by_id to notify_items_by_name which is
					// problematic since the state of the item by name gets overwritten each update
					notify_items_by_name[name] = selected_item;
				}
			}

			if (g_verbose)
			{
				print_to_append(selected_item, notify);
			}
			else
			{
				render_status_area(selected_item);
			}
		}

		if (notify.type == uuu_notify::NOTIFY_THREAD_EXIT)
		{
			if (notify_items_by_id.find(notify.id) != notify_items_by_id.end())
			{
				notify_items_by_id.erase(notify.id);
			}
		}
	}

	/**
	 * @brief Notification handler
	 */
	static int handle_notify(uuu_notify notify, void* p)
	{
		auto p_progress = (TransferFeedback*)p;
		p_progress->update(notify);
		return EXIT_SUCCESS;
	}

public:
	~TransferFeedback()
	{
		disable();
	}

	/**
	 * @brief Registers to receive notifications
	 */
	void enable() const
	{
		(void)uuu_register_notify_callback(handle_notify, (void*)&notify_items_by_id);

		if (!g_verbose)
		{
			std::cout << g_vt->hide_cursor;
		}
	}

	/**
	 * @brief Unregisters to receive notifications
	 */
	void disable() const
	{
		(void)uuu_unregister_notify_callback(handle_notify);

		if (!g_verbose)
		{
			std::cout << g_vt->show_cursor;
		}
	}
};