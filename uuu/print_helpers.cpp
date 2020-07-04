#include "print_helpers.h"

#include <cstdio>
#include <cstring>
#include <iostream>

using namespace std;

void print_auto_scroll(string str, size_t len, size_t start)
{
	if (str.size() <= len)
	{
		str.resize(len, ' ');
		cout << str;
		return;
	}

	if(str.size())
		start = start % str.size();
	else
		start = 0;

	string s = str.substr(start, len);
	s.resize(len, ' ');
	cout << s;
}

int print_cfg(const char *pro, const char * chip, const char * /*compatible*/,
	uint16_t pid, uint16_t vid, uint16_t bcdmin, uint16_t bcdmax, void * /*p*/)
{
	const char *ext;
	if (strlen(chip) >= 7)
		ext = "";
	else
		ext = "\t";

	if (bcdmin == 0 && bcdmax == 0xFFFF)
		printf("\t%s\t %s\t%s 0x%04x\t 0x%04x\n", pro, chip, ext, pid, vid);
	else
		printf("\t%s\t %s\t%s 0x%04x\t 0x%04x\t [0x%04x..0x%04x]\n", pro, chip, ext, pid, vid, bcdmin, bcdmax);
	return 0;
}

int print_udev_rule(const char * /*pro*/, const char * /*chip*/, const char * /*compatible*/,
	uint16_t vid, uint16_t pid, uint16_t /*bcdmin*/, uint16_t /*bcdmax*/, void * /*p*/)
{
	printf("SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"%04x\", ATTRS{idProduct}==\"%04x\", MODE=\"0666\"\n",
			vid, pid);
	return 0;
}

int print_usb_device(const char *path, const char *chip, const char *pro,
	uint16_t vid, uint16_t pid, uint16_t bcd, void * /*p*/)
{
	printf("\t%s\t %s\t %s\t 0x%04X\t0x%04X\t 0x%04X\n", path, chip, pro, vid, pid, bcd);
	return 0;
}
