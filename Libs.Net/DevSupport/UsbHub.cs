using System;
using System.Collections;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;

using Microsoft.Win32.SafeHandles;

namespace DevSupport.DeviceManager
{
    /// <summary>
    /// A USB UsbHub device.
    /// </summary>
    public sealed class UsbHub : Device, IEnumerable
    {
        #region IEnumerable Implementation

        /// <summary>
        /// Supports foreach(UsbPort port in UsbHub)
        /// </summary>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return _Ports.GetEnumerator();
        }
        
        #endregion

        #region IFormattable Implementation

        /// <summary>
        /// Prepends format to ToString(). Used for "Port 6 - Hub 3 - Generic USB Hub" type strings.
        /// If format is Empty or null, the default ToString() function is called which
        /// produces output like "Hub 5 - USB Root Hub"
        /// </summary>
        public override string ToString(string format, IFormatProvider formatProvider)
        {
            if (!String.IsNullOrEmpty(format))
            {
                return String.Format("{0} - {1}", format, this.ToString());
            }
            else
            {
                return this.ToString();
            }
        }

        #endregion
        
        /// <summary>
        /// Formats the hub info like "Hub 5 - USB Root Hub"
        /// </summary>
        public override string ToString()
        {
            return String.Format("Hub {0} - {1}", this.Index, this.Description);
        }

        /// <summary>
        /// Creates an instance of a UsbHub. Do not call this constructor
        /// directly. Instead, use the UsbHubClass.Instance to call the
        /// CreateDevice method.
        /// </summary>
        internal UsbHub(IntPtr deviceInstance, string path, int index)
            : base(deviceInstance, path)
        {
            _Index = index;

            CreatePorts();
        }

        /// <summary>
        /// Create the UsbPort for the hub.
        /// </summary>
        /// <returns></returns>
        private int CreatePorts()
        {
            // Port index is 1-based
            for (int index = 1; index <= NumberOfPorts; ++index)
            {
                this[index] = new UsbPort(this, index);
            }

            return this.NumberOfPorts;
        }

        /// <summary>
        /// The indexer returns a UsbPort based on a numerical 1-based index.
        /// </summary>
        /// <param name="port">The 1-based index of the Port on the UsbHub.</param>
        /// <returns>UsbPort</returns>
        public UsbPort this[int port]
        {
            get 
            { 
                return _Ports[port-1]; 
            }
            
            private set 
            { 
                _Ports.Add(value); 
            }
        }
        private Collection<UsbPort> _Ports = new Collection<UsbPort>();

        /// <summary>
        /// The number of USB ports on the USB hub.
        /// </summary>
        public int NumberOfPorts 
        { 
            get 
            {
                if (NodeInfo != null)
                {
                    return NodeInfo.HubInformation.HubDescriptor.bNumberOfPorts;
                }
                else
                {
                    return 0;
                }
            }
        }
	
        /// <summary>
        /// An aribtrary unique number used to identify a USB Hub in the system.
        /// </summary>
	    public int Index
	    {
		    get { return _Index;}
        }
        private int _Index;

        public SafeFileHandle Open()
        {
	         // Needed for Win2000
            Win32.SECURITY_ATTRIBUTES securityAttrib = new Win32.SECURITY_ATTRIBUTES();
            securityAttrib.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
//	        securityAttrib.bInheritHandle = false;
//	        securityAttrib.lpSecurityDescriptor = new IntPtr();
//	        securityAttrib.nLength = Marshal.SizeOf(typeof(Win32.SECURITY_ATTRIBUTES));
//            IntPtr ptrSecurityAttrib = Marshal.AllocHGlobal(securityAttrib.nLength);
//            Marshal.StructureToPtr(securityAttrib, ptrSecurityAttrib, true);

            //w98 - In Windows 98 the hub path starts with \DosDevices\
            //w98 - Replace it with \\.\ so we can use it with CreateFile.
            String hubPath = Path.Replace(@"\DosDevices", @"\\.");
            //wxp - In Windows XP the hub path may start with \??\
            //wxp - Replace it with \\.\ so we can use it for CreateFile
            hubPath = Path.Replace(@"\??", @"\\.");

            return Win32.CreateFile(
                hubPath,
		        Win32.GENERIC_WRITE,
		        Win32.FILE_SHARE_WRITE,
                ref securityAttrib,
                Win32.OPEN_EXISTING, 0, IntPtr.Zero);
        }

        public Win32.USB_NODE_INFORMATION NodeInfo
        {
            get
            {
                if (_NodeInfo == null)
                {
                    int error = 0;

                    SafeFileHandle hHub = Open();
                    if ( !hHub.IsInvalid )
                    {
                        int nBytesReturned;
                        _NodeInfo = new Win32.USB_NODE_INFORMATION();
                        _NodeInfo.NodeType = Win32.USB_HUB_NODE.UsbHub;
                        int nNodeInfoBytes = Marshal.SizeOf(_NodeInfo);
                        IntPtr ptrNodeInfo = Marshal.AllocHGlobal(nNodeInfoBytes);
                        Marshal.StructureToPtr(_NodeInfo, ptrNodeInfo, true);

                        // Get the UsbHub Node Information
                        if (Win32.DeviceIoControl(hHub, Win32.IOCTL_USB_GET_NODE_INFORMATION, ptrNodeInfo,
                            nNodeInfoBytes, ptrNodeInfo, nNodeInfoBytes, out nBytesReturned, IntPtr.Zero))
                        {
                            hHub.Close();

                            _NodeInfo = (Win32.USB_NODE_INFORMATION)Marshal.PtrToStructure(ptrNodeInfo, typeof(Win32.USB_NODE_INFORMATION));
                            Marshal.FreeHGlobal(ptrNodeInfo);
                        }
                        else
                        {
                            error = Marshal.GetLastWin32Error();
                            // Probably not a hub. We had to look at all USB devices in order to get all hubs. If it
                            // fails to open or doesn't have any ports, we don't create a UsbHub device.
                            Trace.WriteLineIf(false, String.Format("*** Warning 0x{0:X}({0}): UsbHub.NodeInfo failed to GetNodeInformation(). ", error));

                            hHub.Close();
                            Marshal.FreeHGlobal(ptrNodeInfo);
                            _NodeInfo = null;
                        }
                    }
                    else
                    {
                        error = Marshal.GetLastWin32Error();
                        String errMsg = new Win32Exception(error).Message;
                        // Probably not a hub. We had to look at all USB devices in order to get all hubs. If it
                        // fails to open or doesn't have any ports, we don't create a UsbHub device.
                        Trace.WriteLineIf(false, String.Format("*** Warning 0x{0:X}({0}): UsbHub.NodeInfo failed to Open() hub. {1}", error, Path));
                    }

                } // if ( _NodeInfo == null )

                return _NodeInfo;

            } // get
        
        } // NodeInfo
        private Win32.USB_NODE_INFORMATION _NodeInfo;

/*
        public int FindPortIndex(String deviceDriverInst)
        {
            int portIndex = 0;
    
            IntPtr hHub = Open();
            if (hHub.ToInt32() == Win32.INVALID_HANDLE_VALUE)
            {
//		        ATLTRACE(_T("*** ERROR 0x%X (%d): Line %d of file %s\n"), error, error, __LINE__, __TFILE__);
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            // Loop through the Ports on the UsbHub any get the unique DriverKey if there is a device connected to
            // the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our UsbHub Index.
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

            hHub.Close();

            return portIndex;
        }
*/
        public int FindPortIndex(String deviceDriverName)
        {
            int portIndex = 0;

            // Loop through the Ports on the UsbHub any get the unique DriverKey if there is a device connected to
            // the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our UsbHub Index.
            foreach (UsbPort port in _Ports)
            {
                // If the Driver Keys match, this is the correct Port
                if (String.Compare(port.DriverName, deviceDriverName, true) == 0)
                {
                    portIndex = port.Index;
                    break;
                }
            }

            // if we didn't find it already connected, then try again, but refresh the port before 
            // doing the compare.
            if ( portIndex == 0 )
            {
                // Loop through the Ports on the UsbHub any get the unique DriverKey if there is a device connected to
                // the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our UsbHub Index.
                foreach (UsbPort port in _Ports)
                {
                    port.Refresh();

                    // If the Driver Keys match, this is the correct Port
                    if (String.Compare(port.DriverName, deviceDriverName, true) == 0)
                    {
                        portIndex = port.Index;
                        break;
                    }
                }
            }

            return portIndex;
        }

        public Win32.USB_NODE_CONNECTION_INFORMATION_EX GetNodeConnectionInfo(SafeFileHandle hHub, int portIndex)
        {
            int error, nBytesReturned;
            int nNodeConnectionBytesEx = Marshal.SizeOf(typeof(Win32.USB_NODE_CONNECTION_INFORMATION_EX));
            IntPtr ptrNodeConnectionEx = Marshal.AllocHGlobal(nNodeConnectionBytesEx);
            Win32.USB_NODE_CONNECTION_INFORMATION_EX nodeConnectionEx = new Win32.USB_NODE_CONNECTION_INFORMATION_EX();
            nodeConnectionEx.ConnectionIndex = portIndex;
            Marshal.StructureToPtr(nodeConnectionEx, ptrNodeConnectionEx, true);

            if (!Win32.DeviceIoControl(hHub, Win32.IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
                ptrNodeConnectionEx, nNodeConnectionBytesEx, ptrNodeConnectionEx, nNodeConnectionBytesEx,
                out nBytesReturned, IntPtr.Zero))
            {
                // Try sending IOCTL_USB_GET_NODE_CONNECTION_INFORMATION
                error = Marshal.GetLastWin32Error();
                Marshal.FreeHGlobal(ptrNodeConnectionEx);

                int nNodeConnectionBytes = Marshal.SizeOf(typeof(Win32.USB_NODE_CONNECTION_INFORMATION));
                IntPtr ptrNodeConnection = Marshal.AllocHGlobal(nNodeConnectionBytes);
                Win32.USB_NODE_CONNECTION_INFORMATION nodeConnection = new Win32.USB_NODE_CONNECTION_INFORMATION();
                nodeConnection.ConnectionIndex = portIndex;
                Marshal.StructureToPtr(nodeConnection, ptrNodeConnection, true);

                if (!Win32.DeviceIoControl(hHub, Win32.IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                    ptrNodeConnection, nNodeConnectionBytes, ptrNodeConnection, nNodeConnectionBytes,
                    out nBytesReturned, IntPtr.Zero))
                {
                    error = Marshal.GetLastWin32Error();
                    Marshal.FreeHGlobal(ptrNodeConnection);
                }
                else
                {
                    nodeConnection = (Win32.USB_NODE_CONNECTION_INFORMATION)Marshal.PtrToStructure(ptrNodeConnection,
                        typeof(Win32.USB_NODE_CONNECTION_INFORMATION));
                    Marshal.FreeHGlobal(ptrNodeConnection);

                    nodeConnectionEx = nodeConnection;
                }
            }
            else
            {
                nodeConnectionEx = (Win32.USB_NODE_CONNECTION_INFORMATION_EX)Marshal.PtrToStructure(ptrNodeConnectionEx,
                    typeof(Win32.USB_NODE_CONNECTION_INFORMATION_EX));
                Marshal.FreeHGlobal(ptrNodeConnectionEx);
            }
            
            return nodeConnectionEx;
        }

        public String GetNodeConnectionDriver(SafeFileHandle hHub, int portIndex)
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
usb::UsbHub::UsbHub(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
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
usb::Port* usb::UsbHub::Port(const size_t portNumber)
{
	assert ( portNumber <= _ports.getList()->size() );

	usb::Port* pPort = dynamic_cast<usb::Port*>(_ports.getList()->at(portNumber-1));
	
	return pPort;

}

int32_t usb::UsbHub::CreatePorts()
{
	DWORD error = ERROR_SUCCESS;

	HANDLE hHub = Open();
	if ( hHub == INVALID_HANDLE_VALUE )
	{
		// Probably not a port. We had to look at all USB devices in order to get all hubs. If it
		// fails to open or doesn't have any ports, we don't create a usb::UsbHub device.
		error=GetLastError();
//		ATLTRACE(_T("*** ERROR 0x%X (%d): Line %d of file %s\n"), error, error, __LINE__, __TFILE__);
		return 0;
	}

	// See how many Ports are on the UsbHub
	DWORD BytesReturned;
	BOOL Success = DeviceIoControl(hHub, IOCTL_USB_GET_NODE_INFORMATION, &_nodeInformation, sizeof(_nodeInformation),
										&_nodeInformation, sizeof(_nodeInformation),&BytesReturned, NULL);

	hHub.Close();

	if (!Success) 
	{
		error = GetLastError();
		return 0;
	}

	// Loop through the Ports on the UsbHub any get the unique DriverKey if there is a device connected to
	// the Port. If the DriveryKey from the hub matches the DriverKey for our device, we have our UsbHub Index.
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

void usb::UsbHub::RefreshPort(const int32_t portNumber)
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