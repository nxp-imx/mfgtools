/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "HubClass.h"
#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"

usb::HubClass::HubClass(INSTANCE_HANDLE handle)
//: DeviceClass(NULL/*&GUID_DEVINTERFACE_USB_HUB*/, &GUID_DEVCLASS_USB, _T("USB"), DeviceTypeUsbHub)
: DeviceClass(/*NULL*/&GUID_DEVINTERFACE_USB_HUB, &GUID_DEVCLASS_USB, NULL/*_T("USB")*/, DeviceTypeUsbHub, handle)
{
	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("new HubClass"));
}

usb::HubClass::~HubClass()
{
	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("delete HubClass"));
}

Device* usb::HubClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
    usb::Hub* hub = new usb::Hub(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);
    
    // Enumerator finds all USB devices, so we don't create the device if it doesn't have any ports.
    if ( hub->GetNumPorts() == 0 )
    {
        delete hub;
        hub = NULL;
		return NULL;
    }

    hub->_index.put((int)_devices.size()+1);
/*	for(int i=0; i<hub->GetNumPorts(); i++)
	{
		USB_PORT_NODE *pPortNode = new USB_PORT_NODE;
		pPortNode->pHubDevice = hub;
		pPortNode->hubPath = hub->_path.get();
		pPortNode->hubIndex = hub->_index.get();
		pPortNode->portIndex = (i+1);
		pPortNode->portID = pPortNode->hubIndex * 1000 + pPortNode->portIndex;
		g_PortTable.push_back(pPortNode);
	} */

    return hub;
}

void usb::HubClass::RefreshHubs()
{
    // Init the list of hubs
    DeviceClass::Devices();

    std::list<Device*>::iterator deviceItem;

    for ( deviceItem = _devices.begin(); deviceItem != _devices.end(); ++deviceItem )
    {
        usb::Hub* pHub = dynamic_cast<usb::Hub*>(*deviceItem);
        pHub->RefreshPort(0); // 0 means refresh all the ports
    }
}

usb::Hub* usb::HubClass::FindHubByDriver(LPCTSTR driverName)
{
    CString driverNameStr = driverName;
    usb::Hub* pHub = NULL;

	if ( driverNameStr.IsEmpty() )
    {
        return NULL;
    }

    if ( _devices.empty() )
    {
        DeviceClass::Devices();
    }

    // Find the Hub in our list of hubs
    std::list<Device*>::iterator hub;
    for ( hub = _devices.begin(); hub != _devices.end(); ++hub )
    {
        if ( driverNameStr.CompareNoCase( (*hub)->_driver.get() ) == 0 )
        {
            pHub = dynamic_cast<usb::Hub*>(*hub);
            break;
        }
    }

    return pHub;
}

usb::Hub* usb::HubClass::FindHubByPath(LPCTSTR pathName)
{
    CString pathNameStr = pathName;
    usb::Hub* pHub = NULL;

	if ( pathNameStr.IsEmpty() )
    {
        return NULL;
    }

    if ( _devices.empty() )
    {
        DeviceClass::Devices();
    }

    // Find the Hub in our list of hubs
    std::list<Device*>::iterator hub;
    for ( hub = _devices.begin(); hub != _devices.end(); ++hub )
    {
        if ( pathNameStr.CompareNoCase( (*hub)->_path.get() ) == 0 )
        {
            pHub = dynamic_cast<usb::Hub*>(*hub);
            break;
        }
    }

    return pHub;
}



