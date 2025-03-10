#pragma once
//! @file

//#include "../libuuu/libuuu.h"
//#include "../libuuu/string_man.h"

//#include <stdio.h>
//#include <string.h>

//#include <iomanip>
//#include <iostream>
//#include <mutex>
#include <string>
//#include <vector>

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
