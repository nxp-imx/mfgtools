/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.Collections;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A USB Non-Hub device.
    /// </summary>
    public class UsbDevice : Device//, IComparable
    {
        internal UsbDevice(IntPtr deviceInstance, string path)
            : base(deviceInstance, path)
        {}


        private Win32.USB_NODE_INFORMATION NodeInfo
        {
            get
            {
                if (_NodeInfo == null)
                {
                    int error = 0;

                    IntPtr hHub = Open();
                    if (hHub.ToInt32() != Win32.INVALID_HANDLE_VALUE)
                    {
                        int nBytesReturned;
                        _NodeInfo = new Win32.USB_NODE_INFORMATION();
                        _NodeInfo.NodeType = Win32.USB_HUB_NODE.UsbHub;
                        int nNodeInfoBytes = Marshal.SizeOf(_NodeInfo);
                        IntPtr ptrNodeInfo = Marshal.AllocHGlobal(nNodeInfoBytes);
                        Marshal.StructureToPtr(_NodeInfo, ptrNodeInfo, true);

                        // Get the Hub Node Information
                        if (Win32.DeviceIoControl(hHub, Win32.IOCTL_USB_GET_NODE_INFORMATION, ptrNodeInfo,
                            nNodeInfoBytes, ptrNodeInfo, nNodeInfoBytes, out nBytesReturned, IntPtr.Zero))
                        {
                            Win32.CloseHandle(hHub);

                            _NodeInfo = (Win32.USB_NODE_INFORMATION)Marshal.PtrToStructure(ptrNodeInfo, typeof(Win32.USB_NODE_INFORMATION));
                            Marshal.FreeHGlobal(ptrNodeInfo);
                        }
                        else
                        {
                            error = Marshal.GetLastWin32Error();
                            // Probably not a hub. We had to look at all USB devices in order to get all hubs. If it
                            // fails to open or doesn't have any ports, we don't create a UsbHub device.
                            Trace.WriteLineIf(false, String.Format("*** Warning 0x{0:X}({0}): UsbHub.NodeInfo failed to GetNodeInformation(). ", error));

                            Win32.CloseHandle(hHub);
                            Marshal.FreeHGlobal(ptrNodeInfo);
                            _NodeInfo = null;
                        }
                    }
                    else
                    {
                        error = Marshal.GetLastWin32Error();
                        // Probably not a hub. We had to look at all USB devices in order to get all hubs. If it
                        // fails to open or doesn't have any ports, we don't create a UsbHub device.
                        Trace.WriteLineIf(false, String.Format("*** Warning 0x{0:X}({0}): UsbHub.NodeInfo failed to Open() hub. {1}", error, Path));
                    }

                } // if ( _NodeInfo == null )

                return _NodeInfo;

            } // get
        
        } // NodeInfo

        public int CreatePorts()
        {
	        // Loop through the Ports on the Hub any get the unique DriverKey if there is a device connected to
	        // the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our Hub Index.
	        //
	        // Port index is 1-based
	        for	( int index = 1; index <= NumberOfPorts; ++index ) 
	        {
		        this[index] = new UsbPort(this, index);
                this[index].Refresh();
	        }

	        return this.NumberOfPorts;
        }

        public int FindPortIndex(String deviceDriverInst)
        {
            int portIndex = 0;
            
            IntPtr hHub = Open();
            if (hHub.ToInt32() == Win32.INVALID_HANDLE_VALUE)
            {
//		        ATLTRACE(_T("*** ERROR 0x%X (%d): Line %d of file %s\n"), error, error, __LINE__, __TFILE__);
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            // Loop through the Ports on the Hub any get the unique DriverKey if there is a device connected to
            // the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our Hub Index.
            foreach (UsbPort port in _Ports)
            {
                Win32.USB_NODE_CONNECTION_INFORMATION_EX nodeConnection = this.GetNodeConnectionInfo(hHub, port.Index);
                
                // There is a device connected to this Port
                if (nodeConnection.ConnectionStatus == Win32.USB_CONNECTION_STATUS.DeviceConnected)
                {
                    // Get the Driver Key Name
                    String driverKey = this.GetNodeConnectionDriver(hHub, port.Index);

                    if (!String.IsNullOrEmpty(driverKey))
                    {
                        // If the Driver Keys match, this is the correct Port
                        if (String.Compare(driverKey, deviceDriverInst, true) == 0)
                        {
                            portIndex = port.Index;
                            break;
                        }
                    } // if ( got the driverKey )

                } // if(connected)
            
            } // foreach ( port )

            Win32.CloseHandle(hHub);

            return portIndex;
        }

        public Win32.USB_NODE_CONNECTION_INFORMATION_EX GetNodeConnectionInfo(IntPtr hHub, int portIndex)
        {
            int nBytesReturned;
            int nNodeConnectionBytes = Marshal.SizeOf(typeof(Win32.USB_NODE_CONNECTION_INFORMATION_EX));
            IntPtr ptrNodeConnection = Marshal.AllocHGlobal(nNodeConnectionBytes);
            Win32.USB_NODE_CONNECTION_INFORMATION_EX nodeConnection = new Win32.USB_NODE_CONNECTION_INFORMATION_EX();
            nodeConnection.ConnectionIndex = portIndex;
            Marshal.StructureToPtr(nodeConnection, ptrNodeConnection, true);

            if (!Win32.DeviceIoControl(hHub, Win32.IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
                ptrNodeConnection, nNodeConnectionBytes, ptrNodeConnection, nNodeConnectionBytes,
                out nBytesReturned, IntPtr.Zero))
            {
                int error = Marshal.GetLastWin32Error();
                Marshal.FreeHGlobal(ptrNodeConnection);
                return null;
            }

            nodeConnection = (Win32.USB_NODE_CONNECTION_INFORMATION_EX)Marshal.PtrToStructure(ptrNodeConnection,
                typeof(Win32.USB_NODE_CONNECTION_INFORMATION_EX));
            Marshal.FreeHGlobal(ptrNodeConnection);

            return nodeConnection;
        }

        public String GetNodeConnectionDriver(IntPtr hHub, int portIndex)
        {
            // Get the Driver Key Name
            int nBytesReturned;
            Win32.USB_NODE_CONNECTION_DRIVERKEY_NAME driverKey = new Win32.USB_NODE_CONNECTION_DRIVERKEY_NAME();
            driverKey.ConnectionIndex = portIndex;
            int nDriverKeyBytes = Marshal.SizeOf(driverKey);
            IntPtr ptrDriverKey = Marshal.AllocHGlobal(nDriverKeyBytes);
            Marshal.StructureToPtr(driverKey, ptrDriverKey, true);

            // Get the name of the Driver Key of the device attached to the specified port.
            if (!Win32.DeviceIoControl(hHub, Win32.IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                ptrDriverKey, nDriverKeyBytes, ptrDriverKey, nDriverKeyBytes, out nBytesReturned, IntPtr.Zero))
            {
                int error = Marshal.GetLastWin32Error();
                Trace.WriteLine(String.Format("*** ERROR 0x{0:X}({0}): UsbHub.GetNodeConnectionDriver()failed. H:{1}, P{2}", error, Index, portIndex));

                Marshal.FreeHGlobal(ptrDriverKey);
                return String.Empty;
            }
            driverKey = (Win32.USB_NODE_CONNECTION_DRIVERKEY_NAME)Marshal.PtrToStructure(ptrDriverKey, typeof(Win32.USB_NODE_CONNECTION_DRIVERKEY_NAME));
            Marshal.FreeHGlobal(ptrDriverKey);

            return driverKey.DriverKeyName;
        }

    } // class UsbHub

} // namespace DevSupport.DeviceManager

/*
usb::Hub::Hub(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
: Device(deviceClass, devInst, path)
, _ports(true)
{
	_ports.describe(this, _T("USB Ports"), _T(""));
	_index.describe(this, _T("Index"), _T(""));

	memset(&_nodeInformation, 0, sizeof(_nodeInformation));
	_nodeInformation.NodeType = UsbHub;

	CreatePorts();
}



// Provides access to the individual Ports.
// portNumbers start at 1 not 0
usb::Port* usb::Hub::Port(const size_t portNumber)
{
	assert ( portNumber <= _ports.getList()->size() );

	usb::Port* pPort = dynamic_cast<usb::Port*>(_ports.getList()->at(portNumber-1));
	
	return pPort;

}

int32_t usb::Hub::CreatePorts()
{
	DWORD error = ERROR_SUCCESS;

	HANDLE hHub = Open();
	if ( hHub == INVALID_HANDLE_VALUE )
	{
		// Probably not a port. We had to look at all USB devices in order to get all hubs. If it
		// fails to open or doesn't have any ports, we don't create a usb::Hub device.
		error=GetLastError();
//		ATLTRACE(_T("*** ERROR 0x%X (%d): Line %d of file %s\n"), error, error, __LINE__, __TFILE__);
		return 0;
	}

	// See how many Ports are on the Hub
	DWORD BytesReturned;
	BOOL Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_INFORMATION, &_nodeInformation, sizeof(_nodeInformation),
										&_nodeInformation, sizeof(_nodeInformation),&BytesReturned, NULL);

	CloseHandle(hHub);

	if (!Success) 
	{
		error = GetLastError();
		return 0;
	}

	// Loop through the Ports on the Hub any get the unique DriverKey if there is a device connected to
	// the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our Hub Index.
	//
	uint8_t index;
	// Port index is 1-based
	for	( index=1; index<=GetNumPorts(); ++index ) 
	{
		Property* pPort = new usb::Port(this, index);
		_ports.getList()->push_back(pPort);
	} // end for(ports)

	return GetNumPorts();
}

void usb::Hub::RefreshPort(const int32_t portNumber)
{
	usb::Port* pPort = NULL;

	if ( portNumber == 0 )
	{
		property::Property::PropertyIterator port;
		for (port = _ports.getList()->begin(); port != _ports.getList()->end(); ++port)
		{
			pPort = dynamic_cast<usb::Port*>(*port);
			pPort->Refresh();
		} // end for(ports)
	}
	else
	{
		pPort = dynamic_cast<usb::Port*>(_ports.getList()->at(portNumber-1));
		pPort->Refresh();
	}
}
*/
