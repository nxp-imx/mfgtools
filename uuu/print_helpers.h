#pragma once

#include <cstdint>
#include <string>
#include <vector>

std::string build_progress_bar(size_t width, size_t pos, size_t total,
	const char * vt_default, const char * vt_yellow);
void print_auto_scroll(std::string str, size_t len, size_t start);
int print_cfg(const char *pro, const char * chip, const char * /*compatible*/,
        uint16_t pid, uint16_t vid, uint16_t bcdmin, uint16_t bcdmax, void * /*p*/);
void print_lsusb();
void print_oneline(std::string str, int console_width);
void print_udev();
int print_udev_rule(const char * /*pro*/, const char * /*chip*/, const char * /*compatible*/,
        uint16_t vid, uint16_t pid, uint16_t /*bcdmin*/, uint16_t /*bcdmax*/, void * /*p*/);
int print_usb_device(const char *path, const char *chip, const char *pro,
        uint16_t vid, uint16_t pid, uint16_t bcd, void * /*p*/);
void print_usb_filter(const std::vector<std::string> &usb_path_filters);
void print_version();
