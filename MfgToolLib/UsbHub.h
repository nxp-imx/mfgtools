/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
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
    public:
        Hub(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
        virtual ~Hub();

        usb::Port* Port(const size_t portNumber); // portNumbers start at 1 not 0

        property::Property::PropertyList* Ports() 
		{ 
			return _ports.getList(); 
		}

        DWORD RefreshPort(const int portNumber);
        DWORD ClearPort(const int portNumber);

        // PROPERTIES
        class index : public Int32Property 
		{ 
		public: 
			void put(int val); 
			int get();
		}_index;	//save the index of the hub

        const BYTE GetNumPorts() const 
		{ 
			return  _nodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts; 
		}

        Property _ports;

        HANDLE Open();
        int CreatePorts();

        USB_NODE_INFORMATION _nodeInformation;
    };
}
