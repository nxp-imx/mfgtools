/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "Device.h"
#include "UsbPort.h"

namespace usb
{

	class Hub : public Device
	{
		friend class usb::Port;

	public:
		Hub(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
		virtual ~Hub(void);

		usb::Port* Port(const size_t portNumber); // portNumbers start at 1 not 0

		property::Property::PropertyList* Ports() { return _ports.getList(); };
		int32_t RefreshPort(const int32_t portNumber);
		int32_t ClearPort(const int32_t portNumber);

		// PROPERTIES
		class index : public Int32Property { public: void put(int32_t val); }_index;
		const uint8_t GetNumPorts() const { return  _nodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts; };
		Property _ports;

	private:
		HANDLE Open();
		int32_t CreatePorts();

//		std::vector<usb::Port*> _ports;
		USB_NODE_INFORMATION _nodeInformation;
	};

}
