/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
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
 * Neither the name of the Freescale Semiconductor nor the names of its
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
			return 0;// _nodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts;
		}

        Property _ports;

        HANDLE Open();
        int CreatePorts();
#if 0
        USB_NODE_INFORMATION _nodeInformation;
#endif
    };
}
