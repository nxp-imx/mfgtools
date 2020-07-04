#pragma once

#include <cstdint>
#include <string>

void print_auto_scroll(std::string str, size_t len, size_t start);
int print_cfg(const char *pro, const char * chip, const char * /*compatible*/,
        uint16_t pid, uint16_t vid, uint16_t bcdmin, uint16_t bcdmax, void * /*p*/);
int print_udev_rule(const char * /*pro*/, const char * /*chip*/, const char * /*compatible*/,
        uint16_t vid, uint16_t pid, uint16_t /*bcdmin*/, uint16_t /*bcdmax*/, void * /*p*/);
int print_usb_device(const char *path, const char *chip, const char *pro,
        uint16_t vid, uint16_t pid, uint16_t bcd, void * /*p*/);
