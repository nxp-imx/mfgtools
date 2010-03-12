#include "stdafx.h"
#include "MxRomDeviceClass.h"
#include "MxRomDevice.h"

/// <summary>
/// Initializes a new instance of the MxRomDeviceClass class.
/// </summary>
MxRomDeviceClass::MxRomDeviceClass()
: DeviceClass(&GUID_DEVINTERFACE_MX_ROM_WDF_USB_BULK_DEVICE, NULL, NULL, DeviceTypeMxRom)
{
}

Device* MxRomDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	TRACE("DeviceClass::Devices:Init i.mx device object.\r\n");

	MxRomDevice* pDev = new MxRomDevice(deviceClass, deviceInfoData.DevInst, path);

	return pDev;
}

Device* MxRomDeviceClass::FindDeviceByUsbPath(CStdString pathToFind, const DeviceListType devListType, const DeviceListAction devListAction )
{

	if (pathToFind.IsEmpty())
    {
        return NULL;
    }

    // Remove the GUID_DEVINTERFACE_USB_DEVICE from devPath before doing the compare.
    GuidProperty usbGuid;
	usbGuid.put(GUID_DEVINTERFACE_USB_DEVICE);
	CStdString usbGuidStr = usbGuid.ToString();
	usbGuidStr.MakeUpper();
    pathToFind.MakeUpper();
	pathToFind.Replace(usbGuidStr, _T(""));

    Device * pDevice = NULL;

	// existing application device list or new OS device list?
    switch ( devListType )
	{
		/*case DeviceListType_Old:
		{
			// Find the Device in our list of OLD devices.
			std::list<Device*>::iterator device;
			for ( device=_oldDevices.begin(); device != _oldDevices.end(); ++device )
			{
				if ( (*device)->IsUsb() )
				{
					CStdString usbDevPath = (*device)->_usbPath.get();
					usbDevPath = usbDevPath.Left(pathToFind.GetLength());
					if ( pathToFind.CompareNoCase( usbDevPath ) == 0 )
					{
						if ( devListAction == DeviceListAction_Remove )
						{
							delete (*device);
							_oldDevices.erase(device);
						}
						break;
					}
				}
			}
			break;
		}*/
		case DeviceListType_Current:
		{		
			// Find the Device in our list of CURRENT devices.
			// If we the action is remove, then just move the device from _devices list to _oldDevices list.
			std::list<Device*>::iterator device;
			for ( device=_devices.begin(); device != _devices.end(); ++device )
			{
				if ( (*device)->IsUsb() )
				{
					CStdString usbDevPath = (*device)->_usbPath.get();
					usbDevPath = usbDevPath.Left(pathToFind.GetLength());
					if ( pathToFind.CompareNoCase( usbDevPath ) == 0 )
					{
						pDevice = (*device);

						if ( devListAction == DeviceListAction_Remove )
						{
							WaitForSingleObject(devicesMutex, INFINITE);
							//_oldDevices.push_back(pDevice);
							_devices.erase(device);
							ReleaseMutex(devicesMutex);
						}
						break;
					}
				}
			}
			break;
		}
		case DeviceListType_New:
		{
			// Get a new list of our devices from Windows and see if it is there.

			int32_t error;

			SP_DEVINFO_DATA devData;
			devData.cbSize = sizeof(SP_DEVINFO_DATA);
			CStdString devPath = _T("");

			GetClassDevs();
			
			for (int32_t index=0; /*no condition*/; ++index)
			{
				if ( *_classIfaceGuid.get() != GUID_NULL /*&&
						gWinVersionInfo().IsWinNT()*/ )
				{
					error = EnumDeviceInterfaceDetails(index, devPath, &devData);
					if ( error != ERROR_SUCCESS )
					{	// No match
						// Enum() will return ERROR_NO_MORE_ITEMS when done
						// but regardless, we can't add the device
						pDevice = NULL;
						break;
					}
				}
				else
				{
					if (!gSetupApi().SetupDiEnumDeviceInfo(_deviceInfoSet, index, &devData))
					{
						// Enum() will return ERROR_NO_MORE_ITEMS when done
						// but regardless, we can't add the device
						pDevice = NULL;
						break;
					}
				}

				//We should realized that function: EnumDeviceInterfaceDetails will enumerate 
				//all the devices with same GUID when index is set to 0, as a result, those already
				//exist in device list will be recreated if we don't skip them.
				//If devPath is same with pathToFind, then create a new device, or just skip
				CStdString CurUsbDevPath = devPath.Right(devPath.GetLength()-4);

				CurUsbDevPath = CurUsbDevPath.Left(pathToFind.GetLength());
				if ( pathToFind.CompareNoCase( CurUsbDevPath ) != 0 )
				{
					continue;
				}

				pDevice = CreateDevice(this, devData, devPath);
				if ( pDevice && pDevice->IsUsb() )
				{
					CStdString usbDevPath = pDevice->_usbPath.get();
					usbDevPath = usbDevPath.Left(pathToFind.GetLength());
					if ( pathToFind.CompareNoCase( usbDevPath ) == 0 )
					{
						// Found what we are looking for
						if ( devListAction == DeviceListAction_Add )
						{
							WaitForSingleObject(devicesMutex, INFINITE);
							_devices.push_back(pDevice);
							ReleaseMutex(devicesMutex);
						}
						break;
					}
				}
				// if we got here, the device isn't the device we
				// are looking for, so clean up.
				if ( pDevice )
				{
					delete pDevice;
					pDevice = NULL;
				}
			}
			break;
		}  // end case DeviceListNew:
		
		default:
			break;
	} // end switch (devListType)

	return pDevice;
}

