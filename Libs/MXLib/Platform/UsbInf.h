#pragma once

#ifndef _USB_INF_H
#define _USB_INF_H

#include "MXDefine.h"

struct usb_inf {
	int vendor;
	int product;
};

static struct usb_inf mxusb_inf[MX_MAX] = {
    {0x15A2, 0x003A},
    {0x15A2, 0x003A},
	{0x0425, 0x21FF},
	{0x0425, 0x21FF},
	{0x0425, 0x21FF},
	{0x0425, 0x21FF},
    {0x0425, 0x21FF},
	{0x15A2, 0x0028},
	{0x15A2, 0x0030},
	{0x15A2, 0x0030},
	{0x15A2, 0x002C},
    {0x15A2, 0x002C},
	{0x15A2, 0x0041},
};
/*
typedef struct 
{
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize0;

    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char iManufacturer;
    unsigned char iProduct;
    unsigned char iSerialNumber;
    unsigned char bNumConfigurations;
} USB_DEVICE_DESCRIPTOR;
*/

#endif
