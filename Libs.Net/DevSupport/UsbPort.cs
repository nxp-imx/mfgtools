/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;

using Microsoft.Win32.SafeHandles;

namespace DevSupport.DeviceManager
{
    public class UsbPort : IFormattable
	{
        /// <summary>
        /// The hub to which this port belongs;
        /// </summary>
        private UsbHub _ParentHub;
        private int _Index;
        private String _DriverName;
        private Device _AttachedDevice;
        private Win32.USB_NODE_CONNECTION_INFORMATION_EX _NodeConnectionInfo;


        [TypeConverter(typeof(ExpandableObjectConverter))]
        public Win32.USB_NODE_CONNECTION_INFORMATION_EX NodeConnectionInfo
	    {
		    get 
            {
                return _NodeConnectionInfo;
            }
            // Set in this.Refresh()
            private set
            {
                _NodeConnectionInfo = value;
            }
	    }

        public String DriverName
        {
            get { return _DriverName; }
            // Set in this.Refresh()
            private set { _DriverName = value; }
        }

        /// <summary>
		/// The 1-based index of the port on the hub.
		/// </summary>
	    public int Index
	    {
		    get { return _Index;}
	    }

        /// <summary>
        /// The USB hub that owns this port.
        /// </summary>
        public UsbHub Hub
        {
            get { return _ParentHub; }
        }

        // TODO: add a format "short" defined as (device.UsbHub + "Port " + device.UsbPort) or simply ( Hub X Port Y)
        public string ToString(String format, IFormatProvider formatProvider)
        {
            String str = null;

            if (format == "P")
            {
                // Format simply as Port X
                str = String.Format("Port {0}", Index);
            }
            else if (format == "ID")
            {
                // Format as Hub X Port Y
                str = String.Format("Hub {0} Port {1}", this.Hub.Index, this.Index);
            }
            else if (format == "D")
            {
                // Format as Port X - Device.ToString()
                str = String.Format("Port {0} - {1}", Index, AttachedDevice.ToString());
            }
            else if (format == "H")
            {
                // If the connection is to a hub, format Port X - Hub Y
                UsbHub hub = AttachedDevice as UsbHub;
                str = String.Format("Port {0} - Hub {1}", Index, hub.Index);
            }
            else if (format == "E")
            {
                // Format as Port X [ConnectionStatus] Device.ToString()
                str = String.Format("Port {0} [{1}] {2}", Index, NodeConnectionInfo.ConnectionStatus, AttachedDevice.ToString());
            }
            else if (format == "U")
            {
                // Format as Port X [ConnectionStatus] UNKNOWN
                str = String.Format("Port {0} [{1}] UNKNOWN", Index, NodeConnectionInfo.ConnectionStatus);
            }
            else
            {
                str = ToString();
            }

            return str;
        }

        public override string ToString()
        {
            String str;

            // If we have Connection information about the port.
            if (NodeConnectionInfo != null)
            {
                // If the port is not connected to a device
                if (NodeConnectionInfo.ConnectionStatus == Win32.USB_CONNECTION_STATUS.NoDeviceConnected)
                {
                    // Format simply as Port X
                    str = this.ToString("P", null);
                }
                // Else the port is reportedly connected to a device
                else
                {
                    // If we really have a device object
                    if (AttachedDevice != null)
                    {
                        // And the ConnectionStatus is not an error
                        if (NodeConnectionInfo.ConnectionStatus == Win32.USB_CONNECTION_STATUS.DeviceConnected)
                        {
                            // If the connection is to a hub, format Port X - UsbHub Y
                            if (NodeConnectionInfo.DeviceIsHub != 0)
                            {
                                UsbHub hub = AttachedDevice as UsbHub;
                                if (hub != null)
                                {
                                    str = this.ToString("H", null);
                                }
                                else
                                {
                                    str = String.Format("Port {0} - UsbHub <null>", Index);
                                    throw new ArgumentNullException("hub");
                                }
                            }
                            else
                            {
                                // Format as Port X - Device.ToString()
                                str = this.ToString("D", null);
                            }
                        }
                        // Else the ConnectionStatus is an error, so show it
                        else
                        {
                            // Format as Port X [ConnectionStatus] Device.ToString()
                            str = this.ToString("E", null);
                        }
                    }
                    // Else we don't really have a device object so we report ConnectionStatus and UNKNOWN
                    else
                    {
                        // Format as Port X [ConnectionStatus] UNKNOWN
                        str = this.ToString("U", null);
                    }
                }
            }
            // Else we don't have Connection information about the port. 
            else
            {
                // Probably an error condition.
                str = String.Format("Port {0} - NO INFO!", Index);
            }

            return str;
        }

        /// <summary>
        /// Create a UsbPort instance.
        /// </summary>
        /// <param name="hub">The hub to which this port belongs</param>
        /// <param name="index">The 1-based index of the port on the hub.</param>
        public UsbPort(UsbHub hub, int index)
	    {
            _ParentHub = hub;
            _Index = index;

            this.Refresh();
	    }

        /// <summary>
        /// Gets the USB device connected to the USB Port or null if not connected.
        /// </summary>
        public Device AttachedDevice
        {
            get 
            {
                if (_AttachedDevice == null)
                {
                    if (NodeConnectionInfo.DeviceIsHub != 0)
                    {
                        _AttachedDevice = FindHub(DriverName);
                    }
                    else
                    {
                        _AttachedDevice = DeviceManager.FindChildDevice(DriverName);
                    }
                }
                return _AttachedDevice; 
            }
        }

        public int Refresh()
        {
	        int error = 0;

            // Reset Port Properties
            if (_AttachedDevice != null)
            {
                _AttachedDevice.Dispose();
                _AttachedDevice = null;
            }
            DriverName = String.Empty;

            // Open parent hub
            SafeFileHandle hHub = _ParentHub.Open();
            if ( hHub.IsInvalid )
            {
		        error = Marshal.GetLastWin32Error();
		        Trace.WriteLine(String.Format("*** ERROR 0x{0:X} ({0}): UsbPort.Refresh() hub: {1} port: {2}", error, _ParentHub.Index, Index));
		        return error;
            }

	        // Get Connection Information
            NodeConnectionInfo = _ParentHub.GetNodeConnectionInfo(hHub, Index);
            if (NodeConnectionInfo == null)
            {
		        error = Marshal.GetLastWin32Error();
                hHub.Close();
                Trace.WriteLine(String.Format("*** ERROR 0x{0:X} ({0}): UsbPort.Refresh() hub: {1} port: {2}", error, _ParentHub.Index, Index));
		        return error;
	        }

	        //
	        // There is a device connected to this Port
	        //
            if ( NodeConnectionInfo.ConnectionStatus == Win32.USB_CONNECTION_STATUS.DeviceConnected )
	        {
		        // Get the name of the driver key of the device attached to the specified port.
		        DriverName = _ParentHub.GetNodeConnectionDriver(hHub, Index);
		        if ( String.IsNullOrEmpty(DriverName) ) 
		        {
		            error = Marshal.GetLastWin32Error();
                    hHub.Close();
                    Trace.WriteLine(String.Format("*** ERROR 0x{0:X} ({0}): UsbPort.Refresh() failed. H:{1}, P{2}", error, _ParentHub.Index, Index));
		            return error;
		        }

	        } // end if(connected)

	        // Close the hub
            hHub.Close();

            Trace.WriteLine(String.Format("UsbPort.Refresh() - Hub {0} Port {1}", _ParentHub.Index, Index));

	        return error;
        }

        private UsbHub FindHub(String driverName)
        {
            if (String.IsNullOrEmpty(driverName))
            {
                return null;
            }

            UsbHub hub = UsbHubClass.Instance.FindDeviceByDriver(driverName) as UsbHub;

	        return hub;
        }


    } // class UsbPort
		 
} // namespace DevSupport.DeviceManager

/*


Device* usb::Port::FindDeviceInstance(LPCTSTR driverName)
{
	CStdString driverNameStr = driverName;
	Device* pUsbDevice = NULL;

	// Find our Device in gDeviceManager's Master List of [*DeviceClass].Devices()
	std::list<Device*> DeviceList = gDeviceManager::Instance().Devices();
	std::list<Device*>::iterator device;
	for ( device = DeviceList.begin(); device != DeviceList.end(); ++device )
	{
		if ( (*device)->AttachedDevice() != NULL )
		{
			if ( driverNameStr.CompareNoCase( (*device)->AttachedDevice()->_driver.get() ) == 0 )
			{
	//			ATLTRACE2(_T("DeviceClass::AddUsbDevice()  Found existing(%d): %s\r\n"), _devices.size(), pDevice->_usbPath.get().c_str());
				pUsbDevice = (*device);
				break;
			}
		}
	}

//	assert (pUsbDevice);

	return pUsbDevice;
}

CStdString usb::Port::GetUsbDevicePath(void)
{
	CStdString pathStr = _T("");

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL ) //assert(_device);
		{
			if ( _device->AttachedDevice() != NULL )
			{
				pathStr = _device->AttachedDevice()->_path.get();
			}
		}
	}

	return pathStr;
}

CStdString usb::Port::GetDriverKeyName(void)
{
	CStdString driverStr = _T("");

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL ) //assert(_device);
		{
			driverStr = _device->_driver.get();
		}
	}

	return driverStr;
}

CStdString usb::Port::GetDeviceDescription(void)
{
	CStdString descStr = _T("");

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL )
		{
			switch( _device->GetDeviceType() )
			{
				case DeviceClass::DeviceTypeMsc:
				{
					Volume* pVolume = dynamic_cast<Volume*>(_device);
					descStr = pVolume->_friendlyName.get();
					break;
				}
				case DeviceClass::DeviceTypeHid:
				case DeviceClass::DeviceTypeMtp:
				case DeviceClass::DeviceTypeRecovery:
				case DeviceClass::DeviceTypeUsbController:
				case DeviceClass::DeviceTypeUsbHub:
				default:
					descStr = _device->_description.get().c_str();
			}
		}
	}

	return descStr;
}

CStdString usb::Port::GetDriveLetters(void)
{
	CStdString driveStr = _T("");

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL && 
			 _device->GetDeviceType() == DeviceClass::DeviceTypeMsc )
		{
			// Go though the list of Volumes and see if any of them have the same USB hub and hubIndex
			// that identify this usb::Port.
			std::list<Device*> VolumeList = gDeviceManager::Instance()[DeviceClass::DeviceTypeMsc]->Devices();
			std::list<Device*>::iterator vol;
			for ( vol = VolumeList.begin(); vol != VolumeList.end(); ++vol )
			{
				if ( (_index.get() == (*vol)->_hubIndex.get()) &&
					 (_parentHub->_path.get().CompareNoCase((*vol)->_hub.get().c_str()) == 0) )
				{
					if ( !driveStr.IsEmpty() )
					{
						driveStr += _T(',');
					}
					Volume* pVol = dynamic_cast<Volume*>(*vol);
					driveStr += pVol->_LogicalDrive.get().c_str();
				}
			}
		}
	}

	return driveStr;
}

uint32_t usb::Port::GetDeviceType(void)
{
	uint32_t devType = DeviceClass::DeviceTypeNone;

	if ( _connectionInfo.ConnectionStatus == DeviceConnected )
	{
		if ( _device != NULL )
		{
			devType = _device->GetDeviceType();
		}
	}

	return devType;
}

CStdString  usb::Port::GetName(void)
{
	return _parentHub->_path.get();
}


/*
NODE_CONNECTION_INFORMATION CUSBPort::GetConnectionInformation(void)
{
	return m_ConnectionInformation;
}

int CUSBPort::GetDriveCount(void)
{
	int num =  0;
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	GetDriveCount() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		num = m_iDriveCount;
		sLock.Unlock();
	}
	else 
		ATLTRACE(_T("ERROR: GetDriveCount() - Could not get lock."));
	return num;
}
CString CUSBPort::GetDeviceDescription(void)
{
	CString str;
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	GetDeviceDescription() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		str = m_sDeviceDescription;
		sLock.Unlock();
	}
	else 
		ATLTRACE(_T("ERROR: GetDeviceDescription() - Could not get lock."));
	return str;
}

CStdString usb::Port::GetDriveLetters(void)
{
	CString str;
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	GetDriveLetters() - Had to wait for lock."));
	if ( sLock.Lock(500) ) {
		str = m_sDriveLetter;
		sLock.Unlock();
	}
	else 
		ATLTRACE(_T("ERROR: GetDriveLetters() - Could not get lock."));
	return str;
}

DWORD CUSBPort::Refresh(bool DriveLetterRefreshOnly)
{
	CSingleLock sLock(&m_mutex);
	if ( sLock.IsLocked() )
		ATLTRACE(_T("	Refresh() - Had to wait for lock."));
	if ( !sLock.Lock(500) ) {
		ATLTRACE(_T("ERROR: Refresh() - Could not get lock."));
		return ERROR_ACCESS_DENIED;
	}

	BOOL Success;
	DWORD BytesReturned;
	DWORD ErrorCode = ERROR_SUCCESS;
	NODE_INFORMATION m_NodeInformation;

	m_iDriveCount = 0;
	m_sDriveLetter.Empty();

	if(!DriveLetterRefreshOnly)
	{
		m_sDevicePath.Empty();
		m_sPortInfoStr.Empty();
		m_sDriverKeyName.Empty();
		m_sDeviceDescription.Empty();
		m_cConnectionStatus = 0;
		m_sConnectionStatusStr.Format(_T("%s"), ConnectionStatus[0]);
		SetName();
	}

	m_hHubHandle = CreateFile(m_sHubDeviceName, GENERIC_WRITE, FILE_SHARE_WRITE, &m_SecurityAttrib, OPEN_EXISTING, 0, NULL);
	
	if (m_hHubHandle == INVALID_HANDLE_VALUE) 
	{
		ErrorCode=GetLastError();
		m_sTempStr.Format(_T(" *** ERROR *** Invalid handle value returned, ErrorCode = %d\r\n"), ErrorCode);
		m_sPortInfoStr += m_sTempStr;
		sLock.Unlock();
		ATLTRACE(m_sTempStr);
		return ErrorCode;
	}

	m_sTempStr.Format(_T("UsbHub handle %d created\r\n"), m_hHubHandle);
	m_sPortDataStr += m_sTempStr;

	// Root UsbHub is open. Collect the node information
	Success = DeviceIoControl(m_hHubHandle, IOCTL_USB_GET_NODE_INFORMATION, &m_NodeInformation, sizeof(m_NodeInformation),
							  &m_NodeInformation, sizeof(m_NodeInformation), &BytesReturned, NULL);
	if (!Success) 
	{
		ErrorCode=GetLastError();
		m_sTempStr.Format(_T(" *** ERROR *** Connection information not returned, ErrorCode = %d\r\n"), ErrorCode);
		m_sPortInfoStr += m_sTempStr;
		sLock.Unlock();
		ATLTRACE(m_sTempStr);
		return ErrorCode;
	}

	m_ConnectionInformation.ConnectionIndex = m_iConnectionIndex;

	Success = DeviceIoControl(m_hHubHandle, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION, &m_ConnectionInformation, sizeof(m_ConnectionInformation),
							  &m_ConnectionInformation, sizeof(m_ConnectionInformation), &BytesReturned, NULL);
	if (!Success) 
	{
		ErrorCode=GetLastError();
		m_sTempStr.Format(_T(" *** ERROR *** Node connection information not returned\r\n"));
		m_sPortInfoStr += m_sTempStr;
		sLock.Unlock();
		ATLTRACE(m_sTempStr);
		return ErrorCode = ERROR_PATH_NOT_FOUND;
	}

	m_cConnectionStatus = m_ConnectionInformation.ConnectionStatus[0];
	m_sConnectionStatusStr.Format(_T("%s"), ConnectionStatus[m_cConnectionStatus]);

	if (m_cConnectionStatus != DeviceConnected) {
		sLock.Unlock();
		return ErrorCode = ERROR_PATH_NOT_FOUND;
	}

	SetName();

	GetPortData(m_hHubHandle);
	
	m_sTempStr.Format(_T("Closing UsbHub handle %d\r\n"), m_hHubHandle);
	m_sPortDataStr += m_sTempStr;	
	
	
   CloseHandle(m_hHubHandle);
	
	sLock.Unlock();
	return ErrorCode;
}

void CUSBPort::GetPortData(HANDLE HubHandle)
{
	BOOL Success;
	DWORD BytesReturned;
	USHORT LanguageID = 0;
	struct {ULONG ConnectionIndex; ULONG ActualLength; WCHAR Name[MAX_PATH];} ConnectedHub;

	m_hHubHandle = HubHandle;
	
	m_sDriverKeyName.Empty();

	m_sPortDataStr += _T("Device connected\r\n");	

	m_sPortDataStr += _T("---------------------------------------------- Device -----------------------------------------------\r\n");

	LanguageID = GetDeviceDescriptor(LanguageID, &m_ConnectionInformation.DeviceDescriptor.bLength);
	
	LanguageID = GetConfigurationDescriptor(LanguageID);

	ConnectedHub.ConnectionIndex = m_iConnectionIndex;

	// Get the name of the driver key of the device attached to the specified port.
	Success = DeviceIoControl(m_hHubHandle, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, &ConnectedHub,
							  sizeof(ConnectedHub), &ConnectedHub, sizeof(ConnectedHub), &BytesReturned, NULL);

	if (!Success) 
	{
		m_sTempStr.Format(_T(" *** ERROR *** IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME not returned, ErrorCode = %d\r\n"), GetLastError());
		m_sPortInfoStr += m_sTempStr;
	}
	else
	{
		m_sPortInfoStr += _T("\r\nMisc:");

		m_sDriverKeyName = &ConnectedHub.Name[0];
		m_sTempStr.Format(_T("\r\n\tDriver Key Name: %s"), m_sDriverKeyName);
		m_sPortInfoStr += m_sTempStr;	

		if(GetDeviceDescAndPath() == ERROR_SUCCESS)
		{
			m_sName += " ";
			m_sName += m_sDeviceDescription;

			m_sPortDataStr += m_sPortInfoStr;

			GetDriveLetters(&m_DeviceInfoData);

			if(!m_sDriveLetter.IsEmpty())
			{
				m_sTempStr.Format(_T("\r\n\tDriver Letter(s) = %s"), m_sDriveLetter);
				m_sPortDataStr += m_sTempStr;
				m_sTempStr.Format(_T(" (%s)"), m_sDriveLetter);
				m_sName += m_sTempStr;
			}		
		}
		else
			m_sPortDataStr += m_sPortInfoStr;
	}

	m_sPortDataStr += _T("\r\n-----------------------------------------------------------------------------------------------------\r\n");
}

DWORD CUSBPort::GetDeviceDescAndPath(void)
{
	BOOL Success;
	HANDLE hHCDevice;
	CString DriverName;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	DWORD ErrorCode = ERROR_SUCCESS, DeviceIndex, BytesReturned;
	struct {DWORD cbSize; WCHAR DevicePath[MAX_PATH];}DeviceInterfaceDetailData;	 

	m_sDevicePath.Empty();
	m_sDeviceDescription.Empty();

	m_HardwareDeviceInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));
	
	if (m_HardwareDeviceInfo == INVALID_HANDLE_VALUE) 
	{
		ErrorCode=GetLastError();
		m_sTempStr.Format(_T("\r\n *** ERROR *** Invalid SetupDiGetClassDevs() handle value returned, ErrorCode = %d"), ErrorCode);
		m_sPortInfoStr += m_sTempStr;
		return ErrorCode;
	}

	m_DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DeviceIndex=0; ErrorCode = SetupDiEnumDeviceInfo(m_HardwareDeviceInfo, DeviceIndex, &m_DeviceInfoData); DeviceIndex++)
	{
		DriverName = GetDeviceRegistryProperty(SPDRP_DRIVER);

		if(!DriverName.IsEmpty())
		{
			if (m_sDriverKeyName.CompareNoCase(DriverName) == 0)
			{
				m_sDeviceDescription = GetDeviceRegistryProperty(SPDRP_DEVICEDESC);
				
				m_sTempStr.Format(_T("\r\n\tDevice Description = %s"), m_sDeviceDescription);
				m_sPortInfoStr += m_sTempStr;

				DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

				Success = SetupDiEnumDeviceInterfaces(m_HardwareDeviceInfo, 0, &GUID_DEVINTERFACE_USB_DEVICE, DeviceIndex, &DeviceInterfaceData);
				
				if (!Success) 
				{
					ErrorCode=GetLastError();
					m_sTempStr.Format(_T("\r\n *** ERROR *** SetupDiEnumDeviceInterfaces() Interface information not returned, ErrorCode = %d"), ErrorCode);
					m_sPortInfoStr += m_sTempStr;
					break;
				}

				DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

				SetupDiGetDeviceInterfaceDetail(m_HardwareDeviceInfo, &DeviceInterfaceData, (PSP_DEVICE_INTERFACE_DETAIL_DATA)
												&DeviceInterfaceDetailData, MAX_PATH, &BytesReturned, NULL);

				hHCDevice = CreateFile(m_sHubDeviceName, GENERIC_WRITE, FILE_SHARE_WRITE, &m_SecurityAttrib, OPEN_EXISTING, 0, NULL);

				if(hHCDevice != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hHCDevice);
					ErrorCode = ERROR_SUCCESS;
					m_sDevicePath = DeviceInterfaceDetailData.DevicePath;
					m_sTempStr.Format(_T("\r\n\tDevice Path: %s"), m_sDevicePath);
					m_sPortInfoStr += m_sTempStr;
				}
				else
				{
					ErrorCode=GetLastError();
					m_sTempStr.Format(_T("\r\n *** ERROR *** Invalid SetupDiEnumDeviceInterfaces() handle value returned, ErrorCode = %d"), ErrorCode);
					m_sPortInfoStr += m_sTempStr;
				}

//test				Success = SetupDiGetDeviceInterfaceAlias( m_HardwareDeviceInfo, DeviceInterfaceData,  const GUID* AliasInterfaceClassGuid,
//test  PSP_DEVICE_INTERFACE_DATA AliasDeviceInterfaceData
//test);

				break;
			}
		}
	}  // end for()

	SetupDiDestroyDeviceInfoList(m_HardwareDeviceInfo);

	return ErrorCode;
}

CString CUSBPort::GetDeviceRegistryProperty(DWORD Property)
{
    DWORD BytesReturned;
    PBYTE pRegBuf = NULL;
	CString sRegistryProperty(_T(""));

    // get the required length of the buffer first
    if(SetupDiGetDeviceRegistryProperty(m_HardwareDeviceInfo, &m_DeviceInfoData,
										Property, NULL, NULL, 0, &BytesReturned))
    {
		m_sTempStr.Format(_T(" *** ERROR *** SetupDiGetDeviceRegistryProperty() failed, ErrorCode = %d\r\n"), GetLastError());
		m_sPortDataStr += m_sTempStr;
        return sRegistryProperty;
    }

    pRegBuf = (PBYTE) malloc(BytesReturned);

    if(pRegBuf == NULL)
    {
		m_sTempStr.Format(_T(" *** ERROR *** malloc() failed, ErrorCode = %d\r\n"), GetLastError());
		m_sPortDataStr += m_sTempStr;
        return sRegistryProperty;
    }

    if(!SetupDiGetDeviceRegistryProperty(m_HardwareDeviceInfo, &m_DeviceInfoData, Property,
                                          NULL, pRegBuf, BytesReturned, &BytesReturned))
        
    {
		m_sTempStr.Format(_T(" *** ERROR *** SetupDiGetDeviceRegistryProperty() failed, ErrorCode = %d\r\n"), GetLastError());
		m_sPortDataStr += m_sTempStr;
    }
	
	sRegistryProperty += (LPTSTR)pRegBuf;

	free(pRegBuf);

    return sRegistryProperty;
}

USHORT CUSBPort::GetDeviceDescriptor (USHORT LanguageID, PUCHAR BufferPtr)
{
	UCHAR LowByte;

	m_sPortInfoStr += _T("Device Descriptor:");
	
	BufferPtr--; // Backup pointer to prepare for pre-increment
	
	m_sTempStr.Format(_T("\r\n\tbLength %2.2x"), *++BufferPtr);
	m_sPortInfoStr += m_sTempStr;

	m_sTempStr.Format(_T("\r\n\tbDescriptorType %2.2x"), *++BufferPtr);
	m_sPortInfoStr += m_sTempStr;

	LowByte = *++BufferPtr;
			
	m_sTempStr.Format(_T("\r\n\tbcdUSB %4.4x"), LowByte + (*++BufferPtr << 8));
	m_sPortInfoStr += m_sTempStr;

	m_sTempStr.Format(_T("\r\n\tbDeviceClass %2.2x"), *++BufferPtr);
	m_sPortInfoStr += m_sTempStr;

	m_sTempStr.Format(_T("\r\n\tbDeviceSubClass %2.2x"), *++BufferPtr);
	m_sPortInfoStr += m_sTempStr;

	m_sTempStr.Format(_T("\r\n\tbDeviceProtocol %2.2x"), *++BufferPtr);
	m_sPortInfoStr += m_sTempStr;

	m_sTempStr.Format(_T("\r\n\tbMaxEP0Size %2.2x"), *++BufferPtr);
	m_sPortInfoStr += m_sTempStr;
	
	LowByte = *++BufferPtr;
	m_sTempStr.Format(_T("\r\n\twVendorID %4.4x"), LowByte + (*++BufferPtr << 8));
	m_sPortInfoStr += m_sTempStr;
	
	LowByte = *++BufferPtr;
	m_sTempStr.Format(_T("\r\n\twProductID %4.4x"), LowByte + (*++BufferPtr << 8));
	m_sPortInfoStr += m_sTempStr;

	LowByte = *++BufferPtr;
	
	m_sTempStr.Format(_T("\r\n\twDeviceID %4.4x"), LowByte + (*++BufferPtr << 8));
	m_sPortInfoStr += m_sTempStr;

	m_sTempStr.Format(_T("\r\n\tiManufacturer %2.2x"), *++BufferPtr);
	m_sPortInfoStr += m_sTempStr;
	
	if (*BufferPtr != 0) LanguageID = GetStringDescriptor(LanguageID, *BufferPtr);
	{
		m_sTempStr.Format(_T("\r\n\tiProduct %2.2x "), *++BufferPtr);
		m_sPortInfoStr += m_sTempStr;
	}
	
	if (*BufferPtr != 0) LanguageID = GetStringDescriptor(LanguageID, *BufferPtr);
	{
		m_sTempStr.Format(_T("\r\n\tiSerialNumber %2.2x"), *++BufferPtr);
		m_sPortInfoStr += m_sTempStr;
	}

	if (*BufferPtr != 0) LanguageID = GetStringDescriptor(LanguageID, *BufferPtr);
	{
		m_sTempStr.Format(_T("\r\n\tbNumConfigurations %2.2x\r\n"), *++BufferPtr);
		m_sPortInfoStr += m_sTempStr;
	}

	return LanguageID;
}

USHORT CUSBPort::GetStringDescriptor (USHORT LanguageID, UCHAR Index)
{
	BOOL Success;
	DWORD BytesReturned;
	DESCRIPTOR_REQUEST Packet;
	
	if (LanguageID == 0) 
	{ // Get the language ID
		memset(&Packet, 0, sizeof(Packet));
		Packet.ConnectionIndex = m_ConnectionInformation.ConnectionIndex;
		Packet.SetupPacket.bmRequest = 0x80;
		Packet.SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;
		Packet.SetupPacket.wValue[1] = USB_STRING_DESCRIPTOR_TYPE;
		Packet.SetupPacket.wLength[0] = 4;
		
		Success = DeviceIoControl(m_hHubHandle, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, &Packet,
								  sizeof(Packet), &Packet, sizeof(Packet), &BytesReturned, NULL);
		if (!Success) 
		{
				m_sTempStr.Format(_T("\r\n *** ERROR *** String Descriptor 0 not returned, ErrorCode = %d"), GetLastError());
				m_sPortInfoStr += m_sTempStr;
				return LanguageID;
		}

		LanguageID = Packet.Data[2] + (Packet.Data[3] << 8);
	}
	
	memset(&Packet, 0, sizeof(Packet));
	Packet.ConnectionIndex = m_ConnectionInformation.ConnectionIndex;
	Packet.SetupPacket.bmRequest = 0x80;
	Packet.SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;
	Packet.SetupPacket.wValue[1] = USB_STRING_DESCRIPTOR_TYPE;
	Packet.SetupPacket.wValue[0] = Index;
	Packet.SetupPacket.wIndex[0] = LanguageID & 0xFF;
	Packet.SetupPacket.wIndex[1] = (LanguageID >> 8) & 0xFF;
	Packet.SetupPacket.wLength[0] = 255;
	
	Success = DeviceIoControl(m_hHubHandle, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, &Packet,
							  sizeof(Packet), &Packet, sizeof(Packet), &BytesReturned, NULL);
	if (!Success)
	{
		m_sTempStr.Format(_T("\r\n *** ERROR *** String Descriptor %d not returned. ErrorCode = %d"), Index, GetLastError());
		m_sPortInfoStr += m_sTempStr;
		return LanguageID;
	}

	m_sTempStr.Format(_T(" = %s"), &Packet.Data[2]);
	m_sPortInfoStr += m_sTempStr;	
	
	return LanguageID;
}

USHORT CUSBPort::GetConfigurationDescriptor(USHORT LanguageID)
{
	int i;
	BOOL Success;
	UCHAR LowByte;
	DWORD BytesReturned;
	DESCRIPTOR_REQUEST Packet;

	m_sPortInfoStr += _T("Configuration Descriptor:");
	
	// First need to get the configuration descriptor
	memset(&Packet, 0, sizeof(Packet));
	Packet.ConnectionIndex = m_ConnectionInformation.ConnectionIndex;
	Packet.SetupPacket.bmRequest = 0x80;
	Packet.SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;
	Packet.SetupPacket.wValue[1] = USB_CONFIGURATION_DESCRIPTOR_TYPE;
	Packet.SetupPacket.wLength[1] = 1; // Using a 2K buffer
	
	Success = DeviceIoControl(m_hHubHandle, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, &Packet,
							  sizeof(Packet), &Packet, sizeof(Packet), &BytesReturned, NULL);
	
	if (!Success)
	{
		m_sTempStr.Format(_T(" *** ERROR *** Configuration Descriptor not returned. ErrorCode = %d\r\n"), GetLastError());
		m_sPortInfoStr += m_sTempStr;
	}
	
	PUCHAR BufferPtr = &Packet.Data[0];
	
	UCHAR Length = *BufferPtr;
	
	while (Length != 0) 
	{
		UCHAR Type = *++BufferPtr;

		switch (Type) 
		{
			case 2:
				m_sTempStr.Format(_T("\r\n\tbLength %2.2x"), Length);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbDescriptorType %2.2x = Configuration Header"), Type);
				m_sPortInfoStr += m_sTempStr;
				
				LowByte = *++BufferPtr;
				
				m_sTempStr.Format(_T("\r\n\twTotalLength %4.4x"), LowByte + (*++BufferPtr << 8));
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbNumInterfaces %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbConfigValue %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tiConfiguration %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;

				if (*BufferPtr != 0) 
					LanguageID = GetStringDescriptor(LanguageID, *BufferPtr);
				
				m_sTempStr.Format(_T("\r\n\tbmAttributes %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;

				LowByte = *++BufferPtr;
				
				m_sTempStr.Format(_T("\r\n\tbMaxPower %2.2x = %d mA"), LowByte, (LowByte << 1));
				m_sPortInfoStr += m_sTempStr;
				break;
			case 4:
				m_sPortInfoStr += _T("\r\nInterface Descriptor:");
				m_sTempStr.Format(_T("\r\n\tbLength %2.2x"), Length);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbDescriptorType %2.2x = Interface Descriptor"), Type);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbInterfaceNum %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbAlternateSetting %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbNumEndpoints %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;

				LowByte = *++BufferPtr;

				if ((LowByte > 9) & (LowByte < 255)) 
					LowByte = 11;
				
				if (LowByte == 255) 
					LowByte = 10;
				
				m_sTempStr.Format(_T("\r\n\tbInterfaceClass %2.2x = %s"), *BufferPtr, ClassName[LowByte]);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbSubClass %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbProtocol %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tiInterface %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;

				if (*BufferPtr != 0) 
					LanguageID = GetStringDescriptor(LanguageID, *BufferPtr);
				break;
			case 5:
				m_sPortInfoStr += _T("\r\nEndpoint Descriptor:");
				m_sTempStr.Format(_T("\r\n\tbLength %2.2x"), Length);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbDescriptorType %2.2x = Endpoint Descriptor"), Type);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbEndpointAddress %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbmAttributes %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;

				LowByte = *++BufferPtr;
				
				m_sTempStr.Format(_T("\r\n\twMaxPacketSize %4.4x"), LowByte + (*++BufferPtr << 8));
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbInterval %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				break;
			case 0x21:
				m_sPortInfoStr += _T("\r\nHID Descriptor:");
				m_sTempStr.Format(_T("\r\n\tbLength %2.2x"), Length);
				m_sPortInfoStr += m_sTempStr;

				m_sTempStr.Format(_T("\r\n\tbDescriptorType %2.2x = HID Descriptor"), Type);
				m_sPortInfoStr += m_sTempStr;

				LowByte = *++BufferPtr;

				m_sTempStr.Format(_T("\r\n\twHIDversion %4.4x"), LowByte + (*++BufferPtr << 8));
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbCountryCode %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbHIDDescriptorCount %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
				m_sTempStr.Format(_T("\r\n\tbHIDReportType %2.2x"), *++BufferPtr);
				m_sPortInfoStr += m_sTempStr;
		
				LowByte = *++BufferPtr;

				m_sTempStr.Format(_T("\r\n\twHIDReportLength %4.4x"), LowByte + (*++BufferPtr << 8));
				m_sPortInfoStr += m_sTempStr;
				break;
			default:
				m_sTempStr.Format(_T("\r\nUnknown descriptor with Length = %2.2xH and Type = %2.2xH"), Length, Type);
				m_sPortInfoStr += m_sTempStr;
				
				BufferPtr-=2; // Back up to start of descriptor
				
				for (i = 0; i < Length; i++) 
				{
					if ((i % 16) == 0) 
						m_sPortDataStr += _T("\r\n");
	
					m_sTempStr.Format(_T("%2.2x "), *++BufferPtr);
					m_sPortInfoStr += m_sTempStr;
				}
				break;
		}
		Length = *++BufferPtr;
	}
	
	return LanguageID;
}


void CUSBPort::GetDriveLetters(PSP_DEVINFO_DATA pDeviceInfoData)
{
    ULONG ulSize;
    LPTSTR DeviceInterface = NULL;
    TCHAR DeviceID[MAX_DEVICE_ID_LEN];
	DEVINST DevUSBSTORInst, DevUSBSTORInstNext, DevSTORAGEInst;

	m_iDriveCount = 0;
	m_sDriveLetter.Empty();

	// get USBSTOR devnode from USB devnode
	if (CM_Get_Child(&DevUSBSTORInst, pDeviceInfoData->DevInst, 0) != CR_SUCCESS)
		return;

	for (;;) 
	{	// get STORAGE devnode from USBSTOR devnode
		if (CM_Get_Child(&DevSTORAGEInst, DevUSBSTORInst, 0) == CR_SUCCESS) 
		{
			if(CM_Get_Device_ID_Ex(DevSTORAGEInst, DeviceID, sizeof(DeviceID)/sizeof(TCHAR), 0, NULL) != CR_SUCCESS)
				break;
			ulSize = 0;
			if ((CM_Get_Device_Interface_List_Size(&ulSize, (LPGUID)&VolumeClassGuid,DeviceID, 0) == CR_SUCCESS) 
				 && (ulSize > 1) && ((DeviceInterface = (LPTSTR)GlobalAlloc(LPTR, ulSize*sizeof(TCHAR))) != NULL) 
				 && (CM_Get_Device_Interface_List((LPGUID)&VolumeClassGuid, DeviceID, DeviceInterface, ulSize, 0) == CR_SUCCESS) 
				 && *DeviceInterface)
			{
				CString DevicePath;
				TCHAR ThisVolumeName[MAX_PATH];
				TCHAR EnumVolumeName[MAX_PATH];
				TCHAR DriveName[4];
				BOOL bResult;

				DevicePath = DeviceInterface;

				// No refstring is present in the symbolic link; add a trailing
				// '\' char (as required by GetVolumeNameForVolumeMountPoint).
				if (DevicePath.GetAt(DevicePath.GetLength()) != _T('\\')) 
					DevicePath += _T('\\');

				ThisVolumeName[0] = UNICODE_NULL;
				bResult = GetVolumeNameForVolumeMountPoint(DevicePath, ThisVolumeName, MAX_PATH);
				if (bResult && ThisVolumeName[0]) 
				{
					DriveName[1] = TEXT(':');
					DriveName[2] = TEXT('\\');
					DriveName[3] = TEXT('\0');

					for (DriveName[0] = TEXT('A'); DriveName[0] <= TEXT('Z'); DriveName[0]++) 
					{
						EnumVolumeName[0] = TEXT('\0');
						GetVolumeNameForVolumeMountPoint(DriveName, EnumVolumeName, MAX_PATH);
						if (!lstrcmpi(ThisVolumeName, EnumVolumeName)) 
						{
							++m_iDriveCount;
							if(m_iDriveCount>1)
								m_sDriveLetter += ",";
							m_sDriveLetter += DriveName;
							m_sDriveLetter.Remove('\\');
							break;
						}
					} // end for ( all drive letters)
				} // end if (got a mountpoint for our device)
			} // end if (we got our device interface list)

			if (DeviceInterface) 
				GlobalFree(DeviceInterface);

			// get the next STORAGE devnode if one exists
			if (CM_Get_Sibling(&DevUSBSTORInstNext, DevUSBSTORInst, 0) == CR_SUCCESS) 
				DevUSBSTORInst = DevUSBSTORInstNext;
			else // no more STORAGE devnodes
				break;
		}
		else 
			break;
	}
}
*/
/* JWE Maybe later
DWORD CUSBPort::GetDriveLetters(void)
{
	BOOL Success;
	HANDLE hHCDevice;
	CString DriverName;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	DWORD ErrorCode = ERROR_SUCCESS, DeviceIndex, BytesReturned;
	struct {DWORD cbSize; WCHAR DevicePath[MAX_PATH];}DeviceInterfaceDetailData;	 

	m_sPortDataStr +=(_T("In CUSBPort::GetDriveLetters()\r\n"));

	m_HardwareDeviceInfo = SetupDiGetClassDevs((LPGUID)&VolumeClassGuid, NULL, NULL, (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));
	
	if (m_HardwareDeviceInfo == INVALID_HANDLE_VALUE) 
	{
		ErrorCode=GetLastError();
		m_sTempStr.Format(_T(" *** ERROR *** Invalid SetupDiGetClassDevs() handle value returned, ErrorCode = %d\r\n"), ErrorCode);
		m_sPortDataStr += m_sTempStr;
		return ErrorCode;
	}

	m_DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for	(DeviceIndex=0; ErrorCode = SetupDiEnumDeviceInfo(m_HardwareDeviceInfo, DeviceIndex, &m_DeviceInfoData); DeviceIndex++)
	{
		DriverName = GetDeviceRegistryProperty(SPDRP_DRIVER);

		if(!DriverName.IsEmpty())
		{
		//	if (m_sDriverKeyName.CompareNoCase(DriverName) == 0)
			{
				m_sDeviceDescription = GetDeviceRegistryProperty(SPDRP_FRIENDLYNAME);
				
				m_sTempStr.Format(_T("Device Description: %s\r\n"), m_sDeviceDescription);
				m_sPortDataStr += m_sTempStr;

				DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

				Success = SetupDiEnumDeviceInterfaces(m_HardwareDeviceInfo, 0, (LPGUID)&VolumeClassGuid, DeviceIndex, &DeviceInterfaceData);
				
				if (!Success) 
				{
					ErrorCode=GetLastError();
					m_sTempStr.Format(_T(" *** ERROR *** SetupDiEnumDeviceInterfaces() Interface information not returned, ErrorCode = %d\r\n"), ErrorCode);
					m_sPortDataStr += m_sTempStr;
					break;
				}

				DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

				SetupDiGetDeviceInterfaceDetail(m_HardwareDeviceInfo, &DeviceInterfaceData, (PSP_DEVICE_INTERFACE_DETAIL_DATA)
												&DeviceInterfaceDetailData, MAX_PATH, &BytesReturned, NULL);

				hHCDevice = CreateFile(m_sHubDeviceName, GENERIC_WRITE, FILE_SHARE_WRITE, &m_SecurityAttrib, OPEN_EXISTING, 0, NULL);

				if (hHCDevice != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hHCDevice);	
					ErrorCode = ERROR_SUCCESS;
					m_sDevicePath = DeviceInterfaceDetailData.DevicePath;					
					m_sTempStr.Format(_T("Device Path: %s\r\n"), m_sDevicePath);
					m_sPortDataStr += m_sTempStr;
				}
				else
				{
					ErrorCode=GetLastError();
					m_sTempStr.Format(_T(" *** ERROR *** Invalid SetupDiEnumDeviceInterfaces() handle value returned, ErrorCode = %d\r\n"), ErrorCode);
					m_sPortDataStr += m_sTempStr;
				}
				//break;
			}
		}
	}  // end for()

	SetupDiDestroyDeviceInfoList(m_HardwareDeviceInfo);

	return ErrorCode;
}*/
