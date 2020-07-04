#include "print_helpers.h"

#include "libuuu.h"

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

void print_lsusb()
{
	cout << "Connected Known USB Devices\n";
	printf("\tPath\t Chip\t Pro\t Vid\t Pid\t BcdVersion\n");
	printf("\t==================================================\n");

	uuu_for_each_devices(print_usb_device, nullptr);
}

void print_oneline(string str, int console_width)
{
	size_t w = static_cast<size_t>(console_width);
	if (w <= 3)
		return;

	if (str.size() >= w)
	{
		str.resize(w - 1);
		str[str.size() - 1] = '.';
		str[str.size() - 2] = '.';
		str[str.size() - 3] = '.';
	}
	else
	{
		str.resize(w, ' ');
	}
	cout << str << endl;

}

void print_udev()
{
	uuu_for_each_cfg(print_udev_rule, nullptr);
	fprintf(stderr, "\n1: put above udev run into /etc/udev/rules.d/99-uuu.rules\n");
	fprintf(stderr, "\tsudo sh -c \"uuu -udev >> /etc/udev/rules.d/99-uuu.rules\"\n");
	fprintf(stderr, "2: update udev rule\n");
	fprintf(stderr, "\tsudo udevadm control --reload-rules\n");
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

void print_usb_filter(const vector<string> &usb_path_filters)
{
	if (!usb_path_filters.empty())
	{
		cout << " at path ";
		for (const auto &usb_path_filter : usb_path_filters)
		{
			cout << usb_path_filter << " ";
		}
	}
}

void print_version()
{
	printf("uuu (Universal Update Utility) for nxp imx chips -- %s\n\n",
		uuu_get_version_string());
}
